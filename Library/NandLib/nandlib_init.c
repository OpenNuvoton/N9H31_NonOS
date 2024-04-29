/**************************************************************************//**
 * @file     nandlib_init.c
 * @version  V1.00
 * @brief    NAND library init function
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <string.h> 
#include <stdlib.h> 
#include "nandlib.h"
#include "nandlib_global.h"

/** @cond HIDDEN_SYMBOLS */

// P2LN_INFO_LIB_VERSION means the version of running GNAND library.
//      Lib version 1.02 and 1.01 use same format on NAND chip, that is, P2LN_INFO_VERSION 1.01
#define P2LN_INFO_LIB_VERSION   "V1.02.5"

uint8_t _NandBuffer[8192] __attribute__((aligned(32)));
uint8_t *_pNandBuffer = NULL;

static int32_t NANDLIB_ParseNandDisk(NDISK_T *ptNDisk, int32_t bEraseIfErrFormat);
static int32_t NANDLIB_EraseNandDisk(NDISK_T *ptNDisk);
static int32_t NANDLIB_CheckIntegrity(NDISK_T *ptNDisk);
static int32_t NANDLIB_IsValidP2LN(NDISK_T *ptNDisk, uint16_t pba, P2LN_INFO_T *p2ln_info);
static int32_t NANDLIB_CheckID(NDISK_T *ptNDisk, uint16_t pba, P2LN_INFO_T *p2ln_info);

volatile uint8_t u8gInitNand = 0;
int32_t NANDLIB_Init(NDISK_T *ptNDisk, NDRV_T *ptNDriver)
{
    int32_t status;

    DBG_INFO("[NANDLib] NAND Library Version: %s\n", P2LN_INFO_LIB_VERSION);
    if (u8gInitNand)
        return 0;
    memset((char *)ptNDisk, 0, sizeof(NDISK_T));
    ptNDisk->driver = ptNDriver;

    status = ptNDriver->init(ptNDisk);
    if (status < 0) {
        return NANDLIB_UNKNOW_ID;
    }
    _pNandBuffer = (uint8_t *)((uint32_t)_NandBuffer | 0x80000000);
    u8gInitNand = 1;

    return NANDLIB_ParseNandDisk(ptNDisk, 1);  // The 1 means force erase if NAND is not in NANDLIB format
}

void NANDLIB_DeInit(NDISK_T *ptNDisk)
{
    u8gInitNand = 0;
    if (ptNDisk->l2pm != NULL)
        free(ptNDisk->l2pm);
    if (ptNDisk->p2lm != NULL)
        free(ptNDisk->p2lm);
    if (ptNDisk->dp_tbl != NULL)
        free(ptNDisk->dp_tbl);
    if (ptNDisk->driver->deinit != 0)
        ptNDisk->driver->deinit();
}

static int32_t NANDLIB_EraseNandDisk(NDISK_T *ptNDisk)
{
    NDRV_T  *ndriver = ptNDisk->driver;
    int32_t volatile i, status, pageCount;

    // Chip erase
    for(i = 0; i < ptNDisk->nPBlockCount; i++) {
        status = ndriver->erase(i);
        if (status < 0) {
            DBG_INFO("[NANDLIB] NAND chip erase error!! status [0x%x]\n", status);
            return NANDLIB_IO_ERR;
        }
    }
    ptNDisk->p2lm[ptNDisk->p2ln_block].lba = P2LN_BLOCK;

    pageCount = (4 * ptNDisk->nPBlockCount) / ptNDisk->nPageSize;
    if (((4 * ptNDisk->nPBlockCount) % ptNDisk->nPageSize) != 0)
        pageCount++;

    status = NANDLIB_GetDataBlock(ptNDisk, 0, &(ptNDisk->op_block));
    if (status < 0)
        return status;
    ptNDisk->op_offset = 0;

    DBG_MSG("EraseNand: P2LN %d, OP %d\n", ptNDisk->p2ln_block, ptNDisk->op_block);

    /* write page 0 of P2LN table */
    memset(_pNandBuffer, 0xff, ptNDisk->nPageSize);
    NANDLIB_PUT32_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, magic), P2LN_INFO_MAGIC);
    NANDLIB_PUT32_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, version), P2LN_INFO_VERSION);
    NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, op_block), ptNDisk->op_block);      /* OP block address */
    NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, old_op_block), ptNDisk->op_block);    /* Old OP block address */
    NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, old_p2ln), ptNDisk->p2ln_block);  /* old P2LN block address */
    NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, block), ptNDisk->nPBlockCount);


    status = ndriver->pwrite(ptNDisk->p2ln_block, 0, _pNandBuffer);
    if (status < 0) {
        DBG_INFO("[NANDLIB] P2LN block initial error! status [0x%x]\n", status);
        return NANDLIB_IO_ERR;
    }

    /* set lba to op_block */
    ptNDisk->p2lm[ptNDisk->op_block].lba = OP_BLOCK;
    return NANDLIB_OK;
}


static int32_t  NANDLIB_CheckIntegrity(NDISK_T *ptNDisk)
{
    NDRV_T          *ndriver = ptNDisk->driver;
    P2LN_INFO_T     p2ln_info;
    int32_t         i, status, /*compare_match = 1,*/ pageCount;
    uint8_t         *buff;
    struct gnop_relink_t    *op_relink;
    struct gnop_p2ln_t      *op_p2ln;


    // check OP table size always when system boot up.
    NANDLIB_UpdateP2LN(ptNDisk);

    /*------------------------------------------------------------------------------------*/
    /* There's no OP code in OP block.                                                    */
    /*------------------------------------------------------------------------------------*/
    if (ptNDisk->op_offset == 0) 
    {
        if (NANDLIB_IsValidP2LN(ptNDisk, ptNDisk->p2ln_block, &p2ln_info) == NANDLIB_OK) 
        {
            if (p2ln_info.old_p2ln == ptNDisk->p2ln_block)
                return NANDLIB_OK;

            //----- If new P2LN block is valid, erase blocks for old P2LN and OP tables.
            if (ndriver->isDirtyPage(p2ln_info.old_p2ln, 0) == 1) {
                NANDLIB_Erase(ptNDisk, p2ln_info.old_p2ln);

                ptNDisk->p2lm[p2ln_info.old_p2ln].lba = FREE_BLOCK;
            }

            if (ndriver->isDirtyPage(p2ln_info.old_op_block, 0) == 1) {
                ptNDisk->p2lm[p2ln_info.old_op_block].lba = FREE_BLOCK;
            }
        }
        return NANDLIB_OK;
    }

    /*------------------------------------------------------------------------------------*/
    /* The last OP code is "LINK".                                                        */
    /*------------------------------------------------------------------------------------*/
    if (*(uint32_t *)ptNDisk->last_op == OP_LINK) {
        return NANDLIB_OK;
    }

    /*------------------------------------------------------------------------------------*/
    /* The last OP code is "RELINK"                                                       */
    /*------------------------------------------------------------------------------------*/
    if (*(uint32_t *)ptNDisk->last_op == OP_RELINK) 
    {
        op_relink = (struct gnop_relink_t *)ptNDisk->last_op;

        /* set ptNDisk->p2lm and ptNDisk->l2pm */
        ptNDisk->l2pm[ptNDisk->p2lm[op_relink->new_pba].lba].pba = op_relink->new_pba;

        return NANDLIB_OK;
    }

    /*------------------------------------------------------------------------------------*/
    /* The last OP code is "P2LN".                                                        */
    /*------------------------------------------------------------------------------------*/
    if (*(uint32_t *)ptNDisk->last_op == OP_P2LN) 
    {
        /*
         * From "P2LN" OP code, NANDLIB can get the new P2LN block address,
         * and new OP block address.
         */
        op_p2ln = (struct gnop_p2ln_t *)ptNDisk->last_op;

        if ((ptNDisk->p2ln_block != op_p2ln->old_p2ln) || (ptNDisk->op_block   != op_p2ln->old_op)) 
            return NANDLIB_P2LN_SYNC;

        /*
         * Erase new P2LN block and new OP block.
         */
        status = NANDLIB_Erase(ptNDisk, op_p2ln->new_p2ln);
        if (status < 0)
            return status;

        ptNDisk->p2lm[op_p2ln->new_p2ln].lba = FREE_BLOCK;

        status = NANDLIB_Erase(ptNDisk, op_p2ln->new_op);
        if (status < 0)
            return status;

        /* set op block free to ptNDisk->p2lm */
        ptNDisk->p2lm[op_p2ln->new_op].lba = FREE_BLOCK;

        /*
         * Update to new P2LN block.
         *    a. write write P2LN_INFO_T to newP2LN block page 0
         */
        memset(_pNandBuffer, 0xff, ptNDisk->nPageSize);
        NANDLIB_PUT32_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, magic), P2LN_INFO_MAGIC);
        NANDLIB_PUT32_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, version), P2LN_INFO_VERSION);
        NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, op_block), op_p2ln->new_op);      /* OP block address */
        NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, old_op_block), ptNDisk->op_block);    /* Old OP block address */
        NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, old_p2ln), ptNDisk->p2ln_block);  /* old P2LN block address */
        NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, block), ptNDisk->nPBlockCount);
        status = ndriver->pwrite(op_p2ln->new_p2ln, 0, _pNandBuffer);
        if (status < 0)
            return NANDLIB_IO_ERR;

        /*
         * Update to new P2LN block.
         *    b. write ptNDisk->p2lm to newP2LN block page 1 ~
         */
        pageCount = (4 * ptNDisk->nPBlockCount) / ptNDisk->nPageSize;
        if (((4 * ptNDisk->nPBlockCount) % ptNDisk->nPageSize) != 0)
            pageCount++;

        if (pageCount >= ptNDisk->nPagePerBlock) 
        {
            for (i = 0, buff = (uint8_t *)ptNDisk->p2lm;i < ptNDisk->nPagePerBlock-1; i++, buff += ptNDisk->nPageSize) 
            {
                memcpy(_pNandBuffer, buff, ptNDisk->nPageSize);
                status = ndriver->pwrite(op_p2ln->new_p2ln, i+1, _pNandBuffer);
                if (status < 0)
                    return NANDLIB_IO_ERR;
                pageCount--;
            }
        } 
        else 
        {
            for (i=0, buff = (uint8_t *)ptNDisk->p2lm; i < pageCount; i++, buff += ptNDisk->nPageSize) 
            {
                memcpy(_pNandBuffer, buff, ptNDisk->nPageSize);
                status = ndriver->pwrite(op_p2ln->new_p2ln, i+1, _pNandBuffer);
                if (status < 0)
                    return NANDLIB_IO_ERR;
            }
        }

        /*
         * Update to new P2LN block.
         *    c. Erase old P2LN block
         */
        status = NANDLIB_Erase(ptNDisk, ptNDisk->p2ln_block);
        if (status < 0)
            return status;

        ptNDisk->p2lm[ptNDisk->p2ln_block].lba = FREE_BLOCK;
        ptNDisk->p2lm[op_p2ln->new_p2ln].lba = P2LN_BLOCK;


        /*
         * Update to new OP block.
         *    d. Erase old OP block
         */
        status = NANDLIB_Erase(ptNDisk, ptNDisk->op_block);
        if (status < 0)
            return status;

        /* set op block free to ptNDisk->p2lm */
        ptNDisk->p2lm[ptNDisk->op_block].lba = FREE_BLOCK;
        ptNDisk->p2lm[op_p2ln->new_op].lba = OP_BLOCK;

        /*
         * update internal data structure
         */
        ptNDisk->p2ln_block = op_p2ln->new_p2ln;
        ptNDisk->op_block   = op_p2ln->new_op;
        ptNDisk->op_offset  = 0;
    }
    return NANDLIB_OK;
}


static int32_t  NANDLIB_ParseNandDisk(NDISK_T *ptNDisk, int32_t bEraseIfErrFormat)
{
    NDRV_T *ndriver = ptNDisk->driver;
    P2LN_INFO_T  p2ln_info;
    int i, status, pageCount;
    int32_t bIsFinishParseOP = FALSE;
    uint8_t *ptr;
    uint32_t p2lmSize;

    DBG_MSG("NANDLIB_ParseNandDisk  %d\n", ptNDisk->nPBlockCount);

    for (i=0; i<ptNDisk->nPBlockCount; i++) {
        if (NANDLIB_IsValidP2LN(ptNDisk, i, &p2ln_info) == NANDLIB_OK) {
            ptNDisk->p2ln_block = i;        /* P2LN block No. */
            ptNDisk->op_block = p2ln_info.op_block;     /* OP block No. */
            DBG_MSG("Valid P2LN %d\n", ptNDisk->p2ln_block);
            break;
        }
    }

    /* Allocate memory for ptNDisk->p2lm, ptNDisk->l2pm, ptNDisk->dp_tbl */
    ptNDisk->l2pm = (L2PM_T *)malloc(sizeof(L2PM_T) * ptNDisk->nLBlockCount);
    if (ptNDisk->l2pm == NULL)
        return NANDLIB_MEMORY_OUT;
    DBG_MSG("l2pm 0x%x\n", (uint32_t)ptNDisk->l2pm);
    memset(ptNDisk->l2pm, 0xFF, sizeof(L2PM_T) * ptNDisk->nLBlockCount);

    p2lmSize = sizeof(P2LM_T) * ptNDisk->nPBlockCount;
    ptNDisk->p2lm = (P2LM_T *)malloc(p2lmSize);
    if (ptNDisk->p2lm == NULL)
        return NANDLIB_MEMORY_OUT;
    DBG_MSG("p2lm 0x%x\n", (uint32_t)ptNDisk->p2lm);
    memset(ptNDisk->p2lm, 0xFF, p2lmSize);

    ptNDisk->dp_tbl = malloc(ptNDisk->nPBlockCount * ptNDisk->nPagePerBlock / 8);
    if (ptNDisk->dp_tbl == NULL)
        return NANDLIB_MEMORY_OUT;
    DBG_MSG("dp_tbl 0x%x\n", (uint32_t)ptNDisk->dp_tbl);
    memset(ptNDisk->dp_tbl, 0xFF, ptNDisk->nPBlockCount * ptNDisk->nPagePerBlock / 8);

    /* Is NANDLIB  Format? */
    if ((i >= ptNDisk->nPBlockCount) || ((p2ln_info.block & 0xffff) != ptNDisk->nPBlockCount)) {
        /* P2LN not found */
        if (bEraseIfErrFormat) {
            status = NANDLIB_EraseNandDisk(ptNDisk);
            if (status < 0)
                return status;
            p2ln_info.op_block = ptNDisk->op_block;     /* OP block address */
            p2ln_info.old_op_block = ptNDisk->op_block; /* Old OP block address */
            p2ln_info.old_p2ln = ptNDisk->p2ln_block;   /* old P2LN block address */
        } else
            return NANDLIB_FORMAT;      // won't go here since bEraseIfErrFormat is 1.
    }

    /* copy P2LN table to ptNDisk->p2lm */
    DBG_MSG("P2LN %d\n", ptNDisk->p2ln_block);
    pageCount = (4 * ptNDisk->nPBlockCount) / ptNDisk->nPageSize;
    if (((4 * ptNDisk->nPBlockCount) % ptNDisk->nPageSize) != 0)
        pageCount++;

    for (i=0, ptr=(uint8_t *)ptNDisk->p2lm; i < pageCount; i++, ptr+=ptNDisk->nPageSize) 
    {
        status = ndriver->pread(ptNDisk->p2ln_block, i+1, _pNandBuffer);
        if (status < 0)
            return NANDLIB_IO_ERR;
        if (p2lmSize >= ptNDisk->nPageSize) 
        {
            memcpy((char *)ptr, (char *)_pNandBuffer, ptNDisk->nPageSize);
            p2lmSize -= ptNDisk->nPageSize;
        } 
        else
            memcpy((char *)ptr, (char *)_pNandBuffer, p2lmSize);
    }

    /* create ptNDisk->l2pm from ptNDisk->p2lm */
    for (i=0; i<ptNDisk->nPBlockCount; i++) 
    {
        if ((ptNDisk->p2lm[i].lba != FREE_BLOCK) && (ptNDisk->p2lm[i].lba != OP_BLOCK) &&
            (ptNDisk->p2lm[i].lba != BAD_BLOCK) && (ptNDisk->p2lm[i].lba != P2LN_BLOCK)) 
        {
            ptNDisk->l2pm[ptNDisk->p2lm[i].lba].pba = i;
        }
    }

    DBG_MSG("OP Block %d\n", p2ln_info.op_block);
    /* set op block to ptNDisk->p2lm */
    ptNDisk->p2lm[p2ln_info.op_block].lba = OP_BLOCK;
    /* set P2LN block to ptNDisk->p2lm */
    ptNDisk->p2lm[ptNDisk->p2ln_block].lba = P2LN_BLOCK;

    /* Update ptNDisk->p2lm and ptNDisk->l2pm by OP table */
    pageCount=0;
    while (1) 
    {
        //memset(_pNandBuffer, 0xff, ptNDisk->nPageSize);
        status = ndriver->pread(p2ln_info.op_block, pageCount, _pNandBuffer);
        if (status < 0) 
        {
            DBG_INFO("[NANDLIB] read OP table error ! status = [0x%x]\n", status);

            // 2011/09/30 by,
            //      Do nothing and keep to process next entry if NAND page data for OP table is invalid. (BCH error)
            ptNDisk->op_offset++;
            if ((++pageCount >= ptNDisk->nPagePerBlock) || (bIsFinishParseOP))
                break;
            else
                continue;
        }

        if (*(int *)_pNandBuffer == OP_LINK) 
        {
            uint16_t lba, pba;
            lba = NANDLIB_GET16_L(_pNandBuffer, OFFSETOF(GNOP_LINK_T, lba));    /* lba */
            pba = NANDLIB_GET16_L(_pNandBuffer, OFFSETOF(GNOP_LINK_T, pba));    /* pba */
            ptNDisk->p2lm[pba].lba = lba;
            ptNDisk->l2pm[lba].pba = pba;
            ptNDisk->op_offset++;
            memcpy(ptNDisk->last_op, _pNandBuffer, 16);
            DBG_MSG("LINK: lba %d, pba %d\n", ptNDisk->p2lm[pba].lba, ptNDisk->l2pm[lba].pba);
        } 
        else if (*(int *)_pNandBuffer == OP_RELINK) 
        {
            uint16_t lba, pba, opba;
            lba = NANDLIB_GET16_L(_pNandBuffer, OFFSETOF(GNOP_RELINK_T, lba));      /* lba */
            pba = NANDLIB_GET16_L(_pNandBuffer, OFFSETOF(GNOP_RELINK_T, new_pba));  /* pba */
            opba = NANDLIB_GET16_L(_pNandBuffer, OFFSETOF(GNOP_RELINK_T, old_pba)); /* old pba */

            ptNDisk->p2lm[pba].lba = lba;
            ptNDisk->p2lm[opba].lba = FREE_BLOCK;
            ptNDisk->l2pm[lba].pba = pba;
            DBG_MSG("ReLINK: lba %d, pba %d, old pba %d\n", ptNDisk->p2lm[pba].lba, ptNDisk->l2pm[lba].pba, opba);
            ptNDisk->op_offset++;
            memcpy(ptNDisk->last_op, _pNandBuffer, 16);
        } 
        else if (*(int *)_pNandBuffer == OP_P2LN) 
        {
            memcpy(ptNDisk->last_op, _pNandBuffer, 16);
            DBG_MSG("OP_P2LN: do nothing \n");
        } 
        else 
        {
            bIsFinishParseOP = TRUE;
        }

        if ((++pageCount >= ptNDisk->nPagePerBlock) || (bIsFinishParseOP))
            break;
    }   // end of while(1) to trace OP table

    NANDLIB_CheckIntegrity(ptNDisk);

    /* update ptNDisk->dp_tbl by ptNDisk->l2pm */
    for (i=0; i<ptNDisk->nPBlockCount; i++) 
    {
        if (ptNDisk->p2lm[i].lba == FREE_BLOCK) 
        {
            NANDLIB_ClrDirtyBlock(ptNDisk, i);
        }
        if ((ptNDisk->p2lm[i].lba == OP_BLOCK) && (i != p2ln_info.op_block)) 
        {
            NANDLIB_CheckEmpty(ptNDisk, i);
            ptNDisk->p2lm[i].lba = FREE_BLOCK;
            NANDLIB_ClrDirtyBlock(ptNDisk, i);
        }
    }
    //ptNDisk->db_idx = SysTick->VAL % ptNDisk->nPBlockCount;

    return NANDLIB_OK;
}


static int32_t NANDLIB_CheckID(NDISK_T *ptNDisk, uint16_t pba, P2LN_INFO_T *p2ln_info)
{
    NDRV_T *ndriver = ptNDisk->driver;

    /* read pba page0 to get P2LN_INFO_T */
    if(ndriver->pread(pba, 0, _pNandBuffer) < 0)
        return -1;
    memcpy((char *)p2ln_info, (char *)_pNandBuffer, sizeof(P2LN_INFO_T));

    /* Check P2LN_INFO_T->magic is "PTOL" */
    if (p2ln_info->magic != P2LN_INFO_MAGIC)
        return -1;

    DBG_INFO("[NANDLIB] NANDLIB format version on NAND chip: %08x\n", p2ln_info->version);

    /* Check P2LN_INFO_T->version is compatible */

    if (p2ln_info->version != P2LN_INFO_VERSION) {
        DBG_INFO("[NANDLIB] Incompatibility NANDLIB format version on NAND chip: %08x\n", p2ln_info->version);
        return -1;
    }

    return 0;
}


static int32_t  NANDLIB_IsValidP2LN(NDISK_T *ptNDisk, uint16_t pba, P2LN_INFO_T *p2ln_info)
{
    P2LN_INFO_T  temp;

    if (NANDLIB_CheckID(ptNDisk, pba, p2ln_info) < 0)
        return -1;

    if (p2ln_info->old_p2ln == pba)
        return NANDLIB_OK;

    if (NANDLIB_CheckID(ptNDisk, p2ln_info->old_p2ln, &temp) < 0) {
        /* old P2LN doesn't exist, this is the only P2LN */
        return NANDLIB_OK;
    } else {
        /* old P2LN and new P2LN both exist, which means unexpected power-off while doing P2LN update
        Simply return -1 and give up this P2LN. Because NANDLIB_ParseNandDisk() will search and find the 
        valid old P2LN and recover the P2LN update in NANDLIB_ChekcIntegrity(). */
        return -1;
    }
}

/** @endcond HIDDEN_SYMBOLS */
