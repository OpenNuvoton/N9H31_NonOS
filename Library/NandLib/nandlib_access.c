/**************************************************************************//**
 * @file     nandlib_access.c
 * @version  V1.00
 * @brief    NAND library functions to access NAND flash
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <string.h>
#include "nandlib.h"
#include "nandlib_global.h"

/** @cond HIDDEN_SYMBOLS */

static void NANDLIB_SetDirtyPage(NDISK_T *ptNDisk, uint32_t pba, uint32_t page)
{
    uint32_t  idx, shift;

    idx   = (pba * ptNDisk->nPagePerBlock + page) / 8;
    shift = page % 8;

    ptNDisk->dp_tbl[idx] = (ptNDisk->dp_tbl[idx]) | (1 << shift);
}

static void NANDLIB_ClrDirtyPage(NDISK_T *ptNDisk, uint32_t pba, uint32_t page)
{
    uint32_t  idx, shift;

    idx   = (pba * ptNDisk->nPagePerBlock + page) / 8;
    shift = page % 8;

    ptNDisk->dp_tbl[idx] = (ptNDisk->dp_tbl[idx]) & (~(1 << shift));
}

static int32_t NANDLIB_ChkDirtyPage(NDISK_T *ptNDisk, uint32_t pba, uint32_t page)
{
    uint32_t  idx, shift;

    idx   = (pba * ptNDisk->nPagePerBlock + page) / 8;
    shift = page % 8;

    if ((ptNDisk->dp_tbl[idx]) & (1 << shift)) {
        if (ptNDisk->driver->isDirtyPage(pba, page))
            return TRUE;
        NANDLIB_ClrDirtyPage(ptNDisk, pba, page);
        return FALSE;
    } else
        return FALSE;   // clear
}



int32_t NANDLIB_Erase(NDISK_T *ptNDisk, uint32_t pba)
{
    int32_t     status;

    status = ptNDisk->driver->erase(pba);
    if (status < 0)
        return NANDLIB_IO_ERR;

    if (ptNDisk->dp_tbl != NULL)
        NANDLIB_ClrDirtyBlock(ptNDisk, pba);

    return 0;
}

int32_t NANDLIB_UpdateBlock(NDISK_T *ptNDisk, uint32_t lba, uint32_t page)
{
    NDRV_T *ndriver = ptNDisk->driver;
    int32_t status, i;
    uint16_t newblock, oldpba;

    oldpba = ptNDisk->l2pm[lba].pba;
    status = NANDLIB_GetDataBlock(ptNDisk, lba, &newblock);
    if (status < 0)
        return status;

    ptNDisk->p2lm[oldpba].lba = BAD_BLOCK;
    ptNDisk->l2pm[lba].pba = newblock;
    ptNDisk->p2lm[newblock].lba = lba;
    status = NANDLIB_Erase(ptNDisk, newblock);
    if (status < 0) 
    {
        DBG_INFO("[NANDLIB] ERROR: NANDLIB_WriteBlock(): erase block %d error, status [%x]\n", newblock, status);
        return status;
    }
    /* copy pages  */
    for (i = 0; i < ptNDisk->nPagePerBlock; i++) 
    {
        if (NANDLIB_ChkDirtyPage(ptNDisk, oldpba, i) == 0)
            continue;
        status = ndriver->pread(oldpba, i, _pNandBuffer);
        if (status < 0) 
            return NANDLIB_IO_ERR;
        status = ndriver->pwrite(newblock, i, _pNandBuffer);
        if (status < 0) 
            return NANDLIB_IO_ERR;

        /* update ptNDisk->dp_tbl (dirty page table) */
        NANDLIB_SetDirtyPage(ptNDisk, newblock, i);
    }
    status = NANDLIB_OpRelink(ptNDisk, lba, &newblock, page, 0, oldpba);
    if (status < 0)
        return status;
    status = NANDLIB_UpdateP2LN(ptNDisk);
    if (status < 0)
        return status;
    /* mark as bad block */
    ptNDisk->driver->markBad(oldpba);
    return 0;
}

/* Note: Only support page size 2048, 4096, and 8192 */
int32_t NANDLIB_Read(NDISK_T *ptNDisk, uint32_t u32SectorNo, int32_t n32SectorCnt, uint8_t *pu8Buff)
{
    NDRV_T *ndriver = ptNDisk->driver;
    uint32_t  lba, pageNo, colNo, count;
    int32_t status;
    int32_t nSectorPerPage = ptNDisk->nPageSize / 512;

    DBG_MSG("read: sector[%d], count[%d], 0x%x\n", u32SectorNo, n32SectorCnt, (uint32_t)pu8Buff);

    /* Use page as access unit */
    /* translate nSectorNo to lba + pageNo */
    lba = (u32SectorNo / nSectorPerPage) / ptNDisk->nPagePerBlock;
    pageNo = (u32SectorNo / nSectorPerPage) % ptNDisk->nPagePerBlock;
    colNo = u32SectorNo % nSectorPerPage;
    //sysprintf("read: sector[%d], count[%d], [%d / %d / %d]\n", u32SectorNo, n32SectorCnt, lba, pageNo, colNo);

    if (colNo != 0)
    {
        count = MIN(n32SectorCnt, nSectorPerPage - colNo);
        /* look up L2P, if not found return GNERR_READ_L2P_MISS */
        if (ptNDisk->l2pm[lba].pba == FREE_BLOCK)
        {
            memset(pu8Buff, 0xFF, count * 512);
            n32SectorCnt += count;
            n32SectorCnt -= count;
            pu8Buff += 512 * count;
            goto _GNAND_READ_;
        }
        status = ndriver->pread(ptNDisk->l2pm[lba].pba, pageNo, _pNandBuffer);
        if (status < 0)
        {
            DBG_MSG("2. GNAND_read: error pba %d, page %d, status[%x]\n", ptNDisk->l2pm[lba].pba, pageNo, status);
            return status;
        }
        else if (status > (ptNDisk->driver->ecc - 2))  /* update block */
        {
            //sysprintf("1. update: ecc %d, %d\n", ptNDisk->driver->ecc, status);
            status = NANDLIB_UpdateBlock(ptNDisk, lba, pageNo);
            if (status < 0)
                return status;
        }
        memcpy(pu8Buff, _pNandBuffer + colNo * 512, count * 512);
        pu8Buff += count * 512;
        u32SectorNo += count;
        n32SectorCnt -= count;
    }

_GNAND_READ_:
    while (n32SectorCnt > 0) 
    { 
        lba = (u32SectorNo / nSectorPerPage) / ptNDisk->nPagePerBlock;
        pageNo = (u32SectorNo / nSectorPerPage) % ptNDisk->nPagePerBlock;
        //DBG_MSG("read:lba[%d], pba[%d], page[%d], count[%d]\n", lba, ptNDisk->l2pm[lba].pba, pageNo, n32SectorCnt);

        if ((n32SectorCnt - nSectorPerPage) < 0)
        {
            //sysprintf("1.read:lba[%d], pba[%d], page[%d], count[%d]\n", lba, ptNDisk->l2pm[lba].pba, pageNo, n32SectorCnt);
            /* look up L2P, if not found return GNERR_READ_L2P_MISS */
            if (ptNDisk->l2pm[lba].pba == FREE_BLOCK)
            {
                memset(pu8Buff, 0xFF, n32SectorCnt*512);
                n32SectorCnt = 0;
                continue;
            }
            status = ndriver->pread(ptNDisk->l2pm[lba].pba, pageNo, _pNandBuffer);
            if (status < 0)
            {
                DBG_MSG("3. GNAND_read: error pba %d, page %d, status[%x]\n", ptNDisk->l2pm[lba].pba, pageNo, status);
                return status;
            }
            else if (status > (ptNDisk->driver->ecc - 2))  /* update block */
            {
                //sysprintf("2. update: ecc %d, %d\n", ptNDisk->driver->ecc, status);
                status = NANDLIB_UpdateBlock(ptNDisk, lba, pageNo);
                if (status < 0)
                    return status;
            }
            memcpy(pu8Buff, _pNandBuffer, n32SectorCnt*512);
            n32SectorCnt = 0;
        }
        else
        {
            //sysprintf("2.read:lba[%d], pba[%d], page[%d], count[%d]\n", lba, ptNDisk->l2pm[lba].pba, pageNo, n32SectorCnt);
            /* look up L2P */
            if (ptNDisk->l2pm[lba].pba == FREE_BLOCK) 
            {     // No need to read a free block.
                memset(pu8Buff, 0xFF, ptNDisk->nPageSize);
                u32SectorNo += nSectorPerPage;
                n32SectorCnt -= nSectorPerPage;
                pu8Buff += ptNDisk->nPageSize;
                continue;
            }
            status = ndriver->pread(ptNDisk->l2pm[lba].pba, pageNo, pu8Buff);
            if (status < 0) 
            {
                DBG_MSG("4. NANDLIB_read: error pba %d, page %d, status[%x]\n", ptNDisk->l2pm[lba].pba, pageNo, status);
                return NANDLIB_IO_ERR;
            }
            else if (status > (ptNDisk->driver->ecc - 2))  /* update block */
            {
                //sysprintf("3. update: ecc %d, %d\n", ptNDisk->driver->ecc, status);
                status = NANDLIB_UpdateBlock(ptNDisk, lba, pageNo);
                if (status < 0)
                    return status;
            }
            u32SectorNo += nSectorPerPage;
            pu8Buff += ptNDisk->nPageSize;
            n32SectorCnt -= nSectorPerPage;
        }
    }

    return NANDLIB_OK;
}


static int32_t NANDLIB_WriteBlock(NDISK_T *ptNDisk, uint32_t sectorNo, uint32_t nWriteSectors, uint8_t *buff)
{
    NDRV_T *ndriver = ptNDisk->driver;
    uint16_t  PBlockAddr;
    int32_t   status, dirty = FALSE, nSectorPerPage, nWritePages;
    uint32_t  lba, pageNo, i, oldpba, colNo;
    uint32_t volatile cnt;

    nSectorPerPage = ptNDisk->nPageSize / 512;

    lba = (sectorNo / nSectorPerPage) / ptNDisk->nPagePerBlock;
    pageNo = (sectorNo / nSectorPerPage) % ptNDisk->nPagePerBlock;
    colNo = sectorNo % nSectorPerPage;

    DBG_MSG("write: sector[%d], lba[%d], count[%d]\n", sectorNo, lba, nWriteSectors);
    //sysprintf("write block: sector[%d], lba[%d][%d], count[%d]\n", sectorNo, lba, ptNDisk->l2pm[lba].pba, nWriteSectors);

    /* look up L2P */
    /* Because L2P loop up miss, so we call NANDLIB_OpLink(), */
    /* to get new block PBlockAddr */
    if (ptNDisk->l2pm[lba].pba == FREE_BLOCK) {
        // Only get new data block. Don't update OP/P2LM/L2PM tables here.
        status = NANDLIB_GetDataBlock(ptNDisk, lba, &PBlockAddr);
        if (status < 0)
            return status;

        /* update ptNDisk->l2pm and ptNDisk->p2lm */
        ptNDisk->l2pm[lba].pba = PBlockAddr;
        ptNDisk->p2lm[PBlockAddr].lba = lba;

        //  To make sure new block is valid, erase it here.
        //      The free block could be invalid since power off during erasing last time.
        // erase new data block.
        status = NANDLIB_Erase(ptNDisk, PBlockAddr);
        if (status < 0) {
            DBG_INFO("[NANDLIB] ERROR: NANDLIB_WriteBlock(): erase block %d error, status [%x]\n", PBlockAddr, status);
            return status;
        }

        /* not match page alignment */
        if (colNo != 0)
        {
            cnt = MIN(nWriteSectors, nSectorPerPage-colNo);
            memset(_pNandBuffer, 0xFF, ptNDisk->nPageSize);
            memcpy(_pNandBuffer+colNo*512, buff, cnt*512);
            status = ndriver->pwrite(PBlockAddr, pageNo, _pNandBuffer);
            if (status < 0)
                return status;

            /* update ptNDisk->dp_tbl (dirty page table) */
            NANDLIB_SetDirtyPage(ptNDisk, PBlockAddr, pageNo);

            buff += cnt * 512;
            nWriteSectors -= cnt;
            pageNo++;
        }

        nWritePages = nWriteSectors / nSectorPerPage;
        for (i = pageNo; i < pageNo + nWritePages; i++) {
            /* write data to PBlockAddr page i */
            status = ndriver->pwrite(PBlockAddr, i, buff);
            if (status < 0)
                return NANDLIB_IO_ERR;
            buff += ptNDisk->nPageSize;

            /* update ptNDisk->dp_tbl (dirty page table) */
            NANDLIB_SetDirtyPage(ptNDisk, PBlockAddr, i);
        }
        nWriteSectors -= nWritePages * nSectorPerPage;
        pageNo += nWritePages;

        /* not match page alignment */
        if ((nWriteSectors % nSectorPerPage) != 0)
        {
            memset(_pNandBuffer, 0xFF, ptNDisk->nPageSize);
            memcpy(_pNandBuffer, buff, nWriteSectors*512);
            status = ndriver->pwrite(PBlockAddr, pageNo, _pNandBuffer);
            if (status < 0)
                return status;

            /* update ptNDisk->dp_tbl (dirty page table) */
            NANDLIB_SetDirtyPage(ptNDisk, PBlockAddr, pageNo);
        }

        // Update OP/P2LM/L2PM tables here after NAND page write successfully.
        status = NANDLIB_OpLink(ptNDisk, lba, &PBlockAddr);
        if (status < 0)
            return status;

        status = NANDLIB_UpdateP2LN(ptNDisk);
        if (status < 0)
            return status;

    } else {    //--- target block not free, could be write directly (free pages) or need Relink

        nWritePages = (colNo + nWriteSectors) / nSectorPerPage;
        if (((colNo + nWriteSectors) % nSectorPerPage) != 0)
            nWritePages++;

        for (i = pageNo; (i < pageNo+nWritePages) && (dirty!=TRUE); i++) {
            /* check dirty page bits of page i */
            dirty = NANDLIB_ChkDirtyPage(ptNDisk, ptNDisk->l2pm[lba].pba, i);
            if (dirty == TRUE) { // not all pages are free, need to do block update
                /* have dirty page in pageNo ~ pageNo+nWriteSectors, */
                /* so we call NANDLIB_ReLink(), to get a free block PBlockAddr */
                oldpba = ptNDisk->l2pm[lba].pba;

                // Only get new data block. Don't update OP/P2LM/L2PM tables here.
                status = NANDLIB_GetDataBlock(ptNDisk, lba, &PBlockAddr);
                if (status < 0)
                    return status;

                /* update ptNDisk->l2pm and ptNDisk->p2lm */
                ptNDisk->p2lm[oldpba].lba = FREE_BLOCK;
                ptNDisk->l2pm[lba].pba = PBlockAddr;
                ptNDisk->p2lm[PBlockAddr].lba = lba;

                // To make sure new block is valid, erase it here.
                //      The free block could be invalid since power off during erasing last time.
                // erase new data block.
                status = NANDLIB_Erase(ptNDisk, PBlockAddr);
                if (status < 0) {
                    DBG_INFO("[NANDLIB] ERROR: NANDLIB_WriteBlock(): erase block %d error, status [%x]\n", PBlockAddr, status);
                    return status;
                }

                /* copy pages  */
                for (i = 0; i < pageNo; i++) 
                {
                    if (NANDLIB_ChkDirtyPage(ptNDisk, oldpba, i) == 0)
                        continue;

                    cnt = 0;
                    __retry_1:
                    /* copy data from old block page i to PBlockAddr page i */
                    status = ndriver->pread(oldpba, i, _pNandBuffer);
                    if (status < 0) {
                        if (++cnt > 3)
                        {
                            DBG_MSG("read error 1\n");
                            return NANDLIB_IO_ERR;
                        }
                        else
                        {
                            DBG_MSG("retry_1 %d\n", cnt);
                            goto __retry_1;
                        }
                    }
                    status = ndriver->pwrite(PBlockAddr, i, _pNandBuffer);
                    if (status < 0) {
                        return NANDLIB_IO_ERR;
                    }

                    /* update ptNDisk->dp_tbl (dirty page table) */
                    NANDLIB_SetDirtyPage(ptNDisk, PBlockAddr, i);
                }

                /* begin update */
                /* not match page alignment */
                if (colNo != 0)
                {
                    cnt = MIN(nWriteSectors, nSectorPerPage-colNo);
                    status = ndriver->pread(oldpba, i, _pNandBuffer);
                    if (status < 0)
                    {
                        //GNAND_OP_ReCover(ptNDisk, lba, pageNo, nWritePages);
                        return status;
                    }
                    memcpy(_pNandBuffer+colNo*512, buff, cnt*512);
                    status = ndriver->pwrite(PBlockAddr, pageNo, _pNandBuffer);
                    if (status < 0)
                    {
                        //GNAND_OP_ReCover(ptNDisk, lba, pageNo, nWritePages);
                        return status;
                    }

                    /* update ptNDisk->dp_tbl (dirty page table) */
                    NANDLIB_SetDirtyPage(ptNDisk, PBlockAddr, pageNo);

                    buff += cnt * 512;
                    nWriteSectors -= cnt;
                    pageNo++;
                }

                /* match page alignment */
                nWritePages = nWriteSectors / nSectorPerPage;
                for (i = pageNo; i < pageNo+nWritePages; i++) {
                    /* write data to PBlockAddr page i */
                    status = ndriver->pwrite(PBlockAddr, i, buff);
                    if (status < 0) {
                        return NANDLIB_IO_ERR;
                    }
                    buff += ptNDisk->nPageSize;

                    /* update ptNDisk->dp_tbl (dirty page table) */
                    NANDLIB_SetDirtyPage(ptNDisk, PBlockAddr, i);
                }
                nWriteSectors -= nWritePages * nSectorPerPage;
                pageNo += nWritePages;

                /* not match page alignment */
                if ((nWriteSectors % nSectorPerPage) != 0)
                {
                    status = ndriver->pread(oldpba, i, _pNandBuffer);
                    if (status < 0)
                    {
                        return status;
                    }
                    memcpy(_pNandBuffer, buff, nWriteSectors*512);
                    status = ndriver->pwrite(PBlockAddr, pageNo, _pNandBuffer);
                    if (status < 0)
                    {
                        return status;
                    }

                    /* update ptNDisk->dp_tbl (dirty page table) */
                    //--- _20110601_ECC_ERROR
                    //      According to above ndriver->pwrite(),
                    //      we should set dirty page for PBlockAddr, not ptNDisk->l2pm[lba].pba
                    NANDLIB_SetDirtyPage(ptNDisk, PBlockAddr, pageNo);
                    pageNo++;
                }

                for (i = pageNo; i < ptNDisk->nPagePerBlock; i++) 
                {
                    if (NANDLIB_ChkDirtyPage(ptNDisk, oldpba, i) == 0)
                        continue;

                    cnt = 0;
                    __retry_2:
                    /* copy data from old block page i to PBlockAddr page i */
                    status = ndriver->pread(oldpba, i, _pNandBuffer);
                    if (status < 0) {
                        if (++cnt > 3)
                        {
                            DBG_MSG("read error 2\n");
                            return NANDLIB_IO_ERR;
                        }
                        else
                        {
                            DBG_MSG("retry_2 %d\n", cnt);
                            goto __retry_2;
                        }
                    }
                    status = ndriver->pwrite(PBlockAddr, i, _pNandBuffer);
                    if (status < 0) {
                        return NANDLIB_IO_ERR;
                    }

                    /* update ptNDisk->dp_tbl (dirty page table) */
                    NANDLIB_SetDirtyPage(ptNDisk, PBlockAddr, i);
                }

                status = NANDLIB_OpRelink(ptNDisk, lba, &PBlockAddr, pageNo, nWriteSectors, oldpba);
                if (status < 0)
                    return status;

                status = NANDLIB_UpdateP2LN(ptNDisk);
                if (status < 0)
                    return status;
                return NANDLIB_OK;
            }
        }

        //--- gets here if all pages are free, begin to write directly.

        /* all pages are erased  */
        /* not match page alignment */
        if (colNo != 0)
        {
            cnt = MIN(nWriteSectors, nSectorPerPage-colNo);
            memset(_pNandBuffer, 0xFF, ptNDisk->nPageSize);
            memcpy(_pNandBuffer+colNo*512, buff, cnt*512);
            status = ndriver->pwrite(ptNDisk->l2pm[lba].pba, pageNo, _pNandBuffer);
            if (status < 0)
                return status;

            /* update ptNDisk->dp_tbl (dirty page table) */
            NANDLIB_SetDirtyPage(ptNDisk, ptNDisk->l2pm[lba].pba, pageNo);

            buff += cnt * 512;
            nWriteSectors -= cnt;
            pageNo++;
        }

        /* match page alignment */
        nWritePages = nWriteSectors / nSectorPerPage;
        for (i = pageNo; i < pageNo+nWritePages; i++) {
            /* write data to PBlockAddr page i */
            status = ndriver->pwrite(ptNDisk->l2pm[lba].pba, i, buff);
            if (status < 0)
                return NANDLIB_IO_ERR;
            buff += ptNDisk->nPageSize;

            /* update ptNDisk->dp_tbl (dirty page table) */
            NANDLIB_SetDirtyPage(ptNDisk, ptNDisk->l2pm[lba].pba, i);
        }
        nWriteSectors -= nWritePages * nSectorPerPage;
        pageNo += nWritePages;

        /* not match page alignment */
        if ((nWriteSectors % nSectorPerPage) != 0)
        {
            memset(_pNandBuffer, 0xFF, ptNDisk->nPageSize);
            memcpy(_pNandBuffer, buff, nWriteSectors*512);
            status = ndriver->pwrite(ptNDisk->l2pm[lba].pba, pageNo, _pNandBuffer);
            if (status < 0)
                return status;

            /* update ptNDisk->dp_tbl (dirty page table) */
            NANDLIB_SetDirtyPage(ptNDisk, ptNDisk->l2pm[lba].pba, pageNo);
        }
    }

    return NANDLIB_OK;
}


int32_t NANDLIB_Write(NDISK_T *ptNDisk, uint32_t u32SectorNo, int32_t n32SectorCnt, uint8_t *pu8Buff)
{
    int32_t  status, nWriteSectors;
    int32_t remain_sectors, nSectorPerPage, nSectorPerBlock;

    //DBG_MSG("write: sector[%d], count[%d]\n", u32SectorNo, n32SectorCnt);
    nSectorPerPage = ptNDisk->nPageSize / 512;

    /* translate nSectorNo to lba + pageNo */
    nSectorPerBlock = ptNDisk->nPagePerBlock * nSectorPerPage;
    remain_sectors = nSectorPerBlock - (u32SectorNo % nSectorPerBlock);
    nWriteSectors = MIN(remain_sectors, n32SectorCnt);
    while (1) 
    { /* Use block as program unit */

        status = NANDLIB_WriteBlock(ptNDisk, u32SectorNo, nWriteSectors, pu8Buff);
        if (status < 0)
        {
            DBG_MSG("10. NANDLIB_write: write block error!! status[%x]\n", status);
            return status;
        }
        u32SectorNo += nWriteSectors;
        n32SectorCnt -= nWriteSectors;
        if (n32SectorCnt <= 0)
            break;
        pu8Buff += nWriteSectors * 512;
        nWriteSectors = MIN(n32SectorCnt, nSectorPerBlock);
    }

    return NANDLIB_OK;
}

/** @endcond HIDDEN_SYMBOLS */
