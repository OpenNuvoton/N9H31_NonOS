/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "N9H31.h"
#include "diskio.h"     /* FatFs lower layer API */
#include "ff.h"
#include "nandlib.h"

extern NDRV_T *ptNDriver;
extern NDISK_T *ptNDisk;

#define NAND_DRIVE      0        /* for NAND          */


/* Definitions of physical drive number for each media */

/*-----------------------------------------------------------------------*/
/* Initialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (BYTE pdrv)       /* Physical drive number (0..) */
{

    if (pdrv == 0) {
        if (NANDLIB_Init(ptNDisk, ptNDriver) < 0)
            return STA_NOINIT;
    } 
    return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Get Disk Status                                                       */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (BYTE pdrv)       /* Physical drive number (0..) */
{
    return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE pdrv,      /* Physical drive number (0..) */
    BYTE *buff,     /* Data buffer to store read data */
    DWORD sector,   /* Sector address (LBA) */
    UINT count      /* Number of sectors to read (1..128) */
)
{
    if(NANDLIB_Read(ptNDisk, sector, count, buff) != NANDLIB_OK)
        return RES_ERROR;
    return RES_OK;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

DRESULT disk_write (
    BYTE pdrv,          /* Physical drive number (0..) */
    const BYTE *buff,   /* Data to be written */
    DWORD sector,       /* Sector address (LBA) */
    UINT count          /* Number of sectors to write (1..128) */
)
{
    if(NANDLIB_Write(ptNDisk, sector, count, (uint8_t *)buff) != NANDLIB_OK)
        return RES_ERROR;
    return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive number (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{

    DRESULT res = RES_OK;
    int spp = ptNDriver->pagesize / 512;    /* sector per page */

    switch(cmd) {
    case CTRL_SYNC:
        break;
    case GET_SECTOR_COUNT:
        *(DWORD*)buff = ptNDriver->ppb * ptNDriver->lblock * spp;
        break;
    case GET_SECTOR_SIZE:
        *(WORD*)buff = 512; // sector size must be between _MAX_SS and _MIN_SS defied in ffconf.h
        break;
    case GET_BLOCK_SIZE:
        *(WORD*)buff = ptNDriver->ppb * ptNDriver->pagesize;
        break;
    default:
        res = RES_PARERR;
        break;
    }
    return res;
}
