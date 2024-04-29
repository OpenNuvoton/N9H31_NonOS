/**************************************************************************//**
 * @file     nandlib_p2lm.c
 * @version  V1.00
 * @brief    NAND library physical to logical mapping code
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <string.h> 
#include "nandlib.h"
#include "nandlib_global.h"

/** @cond HIDDEN_SYMBOLS */

int32_t NANDLIB_CheckEmpty(NDISK_T *ptNDisk, uint32_t pba)
{
    NDRV_T *ndriver = ptNDisk->driver;
    int32_t status;

    if (ndriver->isDirtyPage(pba, 0)) {
        status = ndriver->erase(pba);
        if (status < 0)
            return NANDLIB_IO_ERR;
    }
    return 0;
}


int32_t NANDLIB_GetDataBlock(NDISK_T *ptNDisk, uint16_t LBlockAddr, uint16_t *PBlockAddr)
{
    int32_t  idx;

    //--- search from current block index to bottom
    for (idx = ptNDisk->db_idx; idx < ptNDisk->nPBlockCount; idx++) 
    {
        if (ptNDisk->p2lm[idx].lba == FREE_BLOCK) 
        {
            if (ptNDisk->driver->isValidBlock(idx)) 
            {
                if (NANDLIB_CheckEmpty(ptNDisk, idx) < 0)
                    continue;

                *PBlockAddr = idx;
                ptNDisk->db_idx = idx+1;
                if (ptNDisk->db_idx >= ptNDisk->nPBlockCount)
                    ptNDisk->db_idx = ptNDisk->nPBlockCount;
                DBG_MSG("1. get block %d\n", *PBlockAddr);

                return NANDLIB_OK;
            } 
            else 
            { // the free block is invalid
                ptNDisk->p2lm[idx].lba = BAD_BLOCK;
            }
        }
    }   // end of for(idx)
    //--- have no free block between current block index and bottom
    //--- search from top to current block index
    for (idx = 0; idx < ptNDisk->db_idx; idx++) 
    {
        if (ptNDisk->p2lm[idx].lba == FREE_BLOCK) 
        {
            if (ptNDisk->driver->isValidBlock(idx)) 
            {
                if (NANDLIB_CheckEmpty(ptNDisk, idx) < 0)
                    continue;
                //--- young and free block found
                *PBlockAddr = idx;
                ptNDisk->db_idx = idx+1;
                if (ptNDisk->db_idx > ptNDisk->nPBlockCount)  // FIXME...
                    ptNDisk->db_idx = ptNDisk->nPBlockCount;
                DBG_MSG("2. get block %d\n", *PBlockAddr);

                return NANDLIB_OK;
            } 
            else 
            { // the free block is invalid
                ptNDisk->p2lm[idx].lba = BAD_BLOCK;
            }
        }
    }   // end of for(idx)

    return NANDLIB_BLOCK_OUT;
}


static int32_t NANDLIB_AddOpHistory(NDISK_T *ptNDisk, uint8_t *buff)
{
    NDRV_T *ndriver = ptNDisk->driver;
    int32_t     status;

    /* write OP_CMD_LEN bytes buff to page ptNDisk->op_offset */
    memset(_pNandBuffer, 0xFF, ptNDisk->nPageSize);
    memcpy(_pNandBuffer, buff, OP_CMD_LEN);
    status = ndriver->pwrite(ptNDisk->op_block, ptNDisk->op_offset, _pNandBuffer);
    if (status < 0)
        return NANDLIB_IO_ERR;
    DBG_MSG("OP block %d, offset %d\n", ptNDisk->op_block, ptNDisk->op_offset);
    ptNDisk->op_offset ++;
    return NANDLIB_OK;
}

int32_t NANDLIB_OpRelink(NDISK_T *ptNDisk, uint16_t LBlockAddr, uint16_t *PBlockAddr,
                         int32_t nStartPage, int32_t nPageCnt, uint16_t oldpba)
{
// NANDLIB_OpRelink() just responsible to update OP/P2LM/L2PM tables, don't need to allocate new block.
// NANDLIB_OpRelink() ignore the parameter bIsBegin since don't need the second Relink OP code now.

    GNOP_RELINK_T  relink_cmd;
    int32_t status;

    relink_cmd.op = OP_RELINK;
    relink_cmd.lba = LBlockAddr;
    relink_cmd.new_pba = *PBlockAddr;
    relink_cmd.start_page = nStartPage;
    relink_cmd.page_cnt = nPageCnt;
    relink_cmd.old_pba = oldpba;

    status = NANDLIB_AddOpHistory(ptNDisk, (uint8_t *)&relink_cmd);
    if (status < 0)
        return status;

    DBG_MSG("ReLink: pba %d, lba %d, db_idx %d\n", ptNDisk->l2pm[relink_cmd.lba].pba,
            ptNDisk->p2lm[relink_cmd.new_pba].lba, ptNDisk->db_idx);

    return NANDLIB_OK;
}


int32_t NANDLIB_UpdateP2LN(NDISK_T *ptNDisk)
{
    NDRV_T *ndriver = ptNDisk->driver;
    GNOP_P2LN_T  p2ln_cmd;
    int32_t newP2LN, newOP, status, nWritePages;
    uint16_t i;
    uint8_t *buff;

    if (ptNDisk->op_offset < ptNDisk->nPagePerBlock - 2)
        return NANDLIB_OK;

    /*
     * OP table remains one entry only, need to do P2LN block update
     */

    NANDLIB_GetDataBlock(ptNDisk, (uint16_t)P2LN_BLOCK, &i);

    //--- free block for P2LN table found, and then, modify block status for them
    /* set P2LN block */
    newP2LN = i;
    ptNDisk->p2lm[newP2LN].lba = P2LN_BLOCK;

    //---  looking for free block for OP table
    // search free block for OP table by NANDLIB_GetDataBlock() that support simple wear-leveling
    NANDLIB_GetDataBlock(ptNDisk, (uint16_t)OP_BLOCK, &i);

    newOP = i;
    /* set op_block */
    ptNDisk->p2lm[newOP].lba = OP_BLOCK;

    p2ln_cmd.op = OP_P2LN;
    p2ln_cmd.old_p2ln = ptNDisk->p2ln_block;
    p2ln_cmd.old_op = ptNDisk->op_block;
    p2ln_cmd.new_p2ln = newP2LN;
    p2ln_cmd.new_op = newOP;

    status = NANDLIB_AddOpHistory(ptNDisk, (uint8_t *)&p2ln_cmd);
    if (status < 0)
        return status;

    // To make sure new block is valid, erase it here.
    //      The free block could be invalid since power off during erasing last time.
    // erase new OP block
    status = NANDLIB_Erase(ptNDisk, newOP);
    if (status < 0) {
        DBG_INFO("[NANDLIB] ERROR: NANDLIB_UpdateP2LN(): erase block %d error, status [%x]\n", newOP, status);
        return status;
    }


    /* write P2LN_INFO_T to newP2LN block page 0 */
    memset(_pNandBuffer, 0xff, ptNDisk->nPageSize);
    NANDLIB_PUT32_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, magic), P2LN_INFO_MAGIC);
    NANDLIB_PUT32_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, version), P2LN_INFO_VERSION);
    NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, op_block), p2ln_cmd.new_op);      /* OP block address */
    NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, old_op_block), p2ln_cmd.old_op);    /* Old OP block address */
    NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, old_p2ln), p2ln_cmd.old_p2ln);  /* old P2LN block address */
    NANDLIB_PUT16_L(_pNandBuffer, OFFSETOF(P2LN_INFO_T, block), ptNDisk->nPBlockCount);

    status = ndriver->pwrite(newP2LN, 0, _pNandBuffer);
    if (status < 0)
        return NANDLIB_IO_ERR;

    /* write ptNDisk->p2lm to newP2LN block page 1 ~ */
    nWritePages = (sizeof(P2LM_T) * ptNDisk->nPBlockCount) / ptNDisk->nPageSize;
    if (((sizeof(P2LM_T) * ptNDisk->nPBlockCount) % ptNDisk->nPageSize) != 0)
        nWritePages++;

    buff = (uint8_t *)ptNDisk->p2lm;
    DBG_MSG("UpdateP2LN: buff 0x%x\n", (int)buff);

    for (i=1; i<= nWritePages; i++) {
        memset(_pNandBuffer, 0xff, ptNDisk->nPageSize);
        memcpy(_pNandBuffer, buff, ptNDisk->nPageSize);
        status = ndriver->pwrite(newP2LN, i, _pNandBuffer);
        if (status < 0)
            return NANDLIB_IO_ERR;
        buff += ptNDisk->nPageSize;

    }

    /* erase old P2LN block */
    status = NANDLIB_Erase(ptNDisk, p2ln_cmd.old_p2ln);
    if (status < 0)
        return status;

    ptNDisk->p2lm[p2ln_cmd.old_p2ln].lba = FREE_BLOCK;

    /* set old op_block to free block */
    ptNDisk->p2lm[p2ln_cmd.old_op].lba = FREE_BLOCK;

    ptNDisk->op_offset = 0;
    ptNDisk->p2ln_block = newP2LN;
    ptNDisk->op_block = newOP;

    return NANDLIB_OK;
}


int32_t NANDLIB_OpLink(NDISK_T *ptNDisk, uint16_t LBlockAddr, uint16_t *PBlockAddr)
{
    GNOP_LINK_T  link_cmd;
    int32_t status;


    link_cmd.op = OP_LINK;
    link_cmd.lba = LBlockAddr;
    link_cmd.pba = *PBlockAddr;
    status = NANDLIB_AddOpHistory(ptNDisk, (uint8_t *)&link_cmd);
    if (status < 0)
        return status;


    DBG_MSG("LINK_New: pba %d, lba %d, db_idx %d\n", ptNDisk->l2pm[link_cmd.lba].pba,
            ptNDisk->p2lm[link_cmd.pba].lba, ptNDisk->db_idx);

    return NANDLIB_OK;
}


void NANDLIB_ClrDirtyBlock(NDISK_T *ptNDisk, uint32_t pba)
{
    int32_t  idx;

    idx   = (pba * ptNDisk->nPagePerBlock) / 8;
    memset(&(ptNDisk->dp_tbl[idx]), 0, ptNDisk->nPagePerBlock / 8);
//sysprintf("## %d, %d, 0x%x\n", pba, idx, ptNDisk->dp_tbl[idx]);
}


/** @endcond HIDDEN_SYMBOLS */
