/**************************************************************************//**
 * @file     nand.c
 * @version  V1.00
 * @brief    NuMicro ARM9 FMI NAND driver source file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include <string.h>
#include "N9H31.h"
#include "nandlib.h"
#include "sys.h"
#include "nand.h"


/** @addtogroup N9H30_Device_Driver N9H30 Device Driver
  @{
*/

/** @addtogroup N9H30_NAND_Driver NAND Driver
  @{
*/


/** @addtogroup N9H30_NAND_EXPORTED_FUNCTIONS NAND Exported Functions
  @{
*/
/// @cond HIDDEN_SYMBOLS

#define NAND_EN     0x08
#define READYBUSY   (0x01 << 18)
#define ENDADDR     (0x80000000)


NAND_INFO_T tNAND;
NDRV_T NandDiskDriver;

int NAND_WaitReady(void)
{
    int volatile tick;

    tick = sysGetTicks(TIMER0);
    while(1) {
        if (inpw(REG_NANDINTSTS) & 0x400) {
            while(!(inpw(REG_NANDINTSTS) & 0x40000));
            outpw(REG_NANDINTSTS, 0x400);
            return 1;
        }
        if ((sysGetTicks(TIMER0) - tick) > 500)
            return 0;
    }
}

int NAND_Reset(void)
{
    int volatile i;

    //sysprintf("NAND_Reset\n");
    outpw(REG_NANDINTSTS, 0x400);
    outpw(REG_NANDCMD, 0xff);
    for (i=100; i>0; i--);
    if (!NAND_WaitReady())
        return 1;
    return 0;
}

int NAND_ReadID(NAND_INFO_T *pNAND)
{
    uint32_t tempID[5], u32PowerOn, IsID=0;

    //sysprintf("NAND_ReadID\n");
    if (NAND_Reset())
        return 1;

    outpw(REG_NANDCMD, 0x90);     // read ID command
    outpw(REG_NANDADDR, 0x80000000);  // address 0x00

    tempID[0] = inpw(REG_NANDDATA);
    tempID[1] = inpw(REG_NANDDATA);
    tempID[2] = inpw(REG_NANDDATA);
    tempID[3] = inpw(REG_NANDDATA);
    tempID[4] = inpw(REG_NANDDATA);

    //sysprintf("NAND ID [%x][%x][%x][%x][%x]\n", tempID[0], tempID[1], tempID[2], tempID[3], tempID[4]);

    /* Without Power-On-Setting for NAND */
    pNAND->uPagePerBlock = 32;
    pNAND->nPageSize = 512;
    pNAND->uNandECC = NAND_BCH_T4;
    pNAND->bIsMulticycle = 1;
    pNAND->uSpareSize = 8;

    switch (tempID[1]) {

    case 0xf1:  // 128M
    case 0xd1:  // 128M
        pNAND->uBlockPerFlash = 1023;
        pNAND->uPagePerBlock = 64;
        pNAND->uSectorPerBlock = 256;
        pNAND->nPageSize = 2048;
        pNAND->uNandECC = NAND_BCH_T4;
        pNAND->bIsMulticycle = 0;
        pNAND->uSpareSize = 64;
        break;

    case 0xda:  // 256M
        if ((tempID[3] & 0x33) == 0x11) {
            pNAND->uBlockPerFlash = 2047;
            pNAND->uPagePerBlock = 64;
            pNAND->uSectorPerBlock = 256;
        } else if ((tempID[3] & 0x33) == 0x21) {
            pNAND->uBlockPerFlash = 1023;
            pNAND->uPagePerBlock = 128;
            pNAND->uSectorPerBlock = 512;
        } else { // Unrecognized ID[3]
            pNAND->uBlockPerFlash = 2047;
            pNAND->uPagePerBlock = 64;
            pNAND->uSectorPerBlock = 256;
        }
        pNAND->nPageSize = 2048;
        pNAND->uNandECC = NAND_BCH_T4;
        pNAND->bIsMulticycle = 1;
        pNAND->uSpareSize = 64;
        break;

    case 0xdc:  // 512M
        pNAND->uBlockPerFlash = 64;
        if((tempID[0]==0x98) && (tempID[1]==0xDC) &&(tempID[2]==0x90)&&(tempID[3]==0x26)&&(tempID[4]==0x76)) {
            pNAND->uBlockPerFlash = 2047;
            pNAND->uPagePerBlock = 64;
            pNAND->uSectorPerBlock = 256;
            pNAND->nPageSize = 4096;
            pNAND->uNandECC = NAND_BCH_T12;
            pNAND->bIsMLCNand = TRUE;
            pNAND->uSpareSize = 192;
            pNAND->bIsMulticycle = TRUE;
            break;
        } else if ((tempID[3] & 0x33) == 0x11) {
            pNAND->uBlockPerFlash = 4095;
            pNAND->uPagePerBlock = 64;
            pNAND->uSectorPerBlock = 256;
        } else if ((tempID[3] & 0x33) == 0x21) {
            pNAND->uBlockPerFlash = 2047;
            pNAND->uPagePerBlock = 128;
            pNAND->uSectorPerBlock = 512;
        }
        pNAND->nPageSize = 2048;
        pNAND->uNandECC = NAND_BCH_T4;
        pNAND->bIsMulticycle = TRUE;
        pNAND->uSpareSize = 64;
        break;

    case 0xd3:  // 1024M
        if ((tempID[3] & 0x33) == 0x32) {
            pNAND->uBlockPerFlash = 2047;
            pNAND->uPagePerBlock = 128;
            pNAND->uSectorPerBlock = 1024;    /* 128x8 */
            pNAND->nPageSize = 4096;
            pNAND->uNandECC = NAND_BCH_T8;
            pNAND->uSpareSize = 128;
        } else if ((tempID[3] & 0x33) == 0x11) {
            pNAND->uBlockPerFlash = 8191;
            pNAND->uPagePerBlock = 64;
            pNAND->uSectorPerBlock = 256;
            pNAND->nPageSize = 2048;
            pNAND->uNandECC = NAND_BCH_T4;
            pNAND->uSpareSize = 64;
        } else if ((tempID[3] & 0x33) == 0x21) {
            pNAND->uBlockPerFlash = 4095;
            pNAND->uPagePerBlock = 128;
            pNAND->uSectorPerBlock = 512;
            pNAND->nPageSize = 2048;
            pNAND->uNandECC = NAND_BCH_T4;
            pNAND->uSpareSize = 64;
        } else if ((tempID[3] & 0x3) == 0x3) {
            pNAND->uBlockPerFlash = 4095;//?
            pNAND->uPagePerBlock = 128;
            pNAND->uSectorPerBlock = 512;//?
            pNAND->nPageSize = 8192;
            pNAND->uNandECC = NAND_BCH_T12;
            pNAND->uSpareSize = 368;
        } else if ((tempID[3] & 0x33) == 0x22) {
            pNAND->uBlockPerFlash = 4095;
            pNAND->uPagePerBlock = 64;
            pNAND->uSectorPerBlock = 512; /* 64x8 */
            pNAND->nPageSize = 4096;
            pNAND->uNandECC = NAND_BCH_T8;
            pNAND->uSpareSize = 128;
        }
        pNAND->bIsMulticycle = 1;
        break;

    case 0xd5:  // 2048M
        // H27UAG8T2A
        if ((tempID[0]==0xAD)&&(tempID[2] == 0x94)&&(tempID[3] == 0x25)) {
            pNAND->uBlockPerFlash = 4095;
            pNAND->uPagePerBlock = 128;
            pNAND->uSectorPerBlock = 1024;    /* 128x8 */
            pNAND->nPageSize = 4096;
            pNAND->uNandECC = NAND_BCH_T12;
            pNAND->bIsMulticycle = 1;
            pNAND->uSpareSize = 224;
            break;
        }
        // 2011/7/28, To support Hynix H27UAG8T2B NAND flash
        else if ((tempID[0]==0xAD)&&(tempID[2]==0x94)&&(tempID[3]==0x9A)) {
            pNAND->uBlockPerFlash = 1023;        // block index with 0-base. = physical blocks - 1
            pNAND->uPagePerBlock = 256;
            pNAND->nPageSize = 8192;
            pNAND->uSectorPerBlock = pNAND->nPageSize / 512 * pNAND->uPagePerBlock;
            pNAND->uNandECC = NAND_BCH_T24;
            pNAND->bIsMulticycle = 1;
            pNAND->uSpareSize = 448;
            break;
        }
        // 2011/7/28, To support Toshiba TC58NVG4D2FTA00 NAND flash
        else if ((tempID[0]==0x98)&&(tempID[2]==0x94)&&(tempID[3]==0x32)) {
            pNAND->uBlockPerFlash = 2075;        // block index with 0-base. = physical blocks - 1
            pNAND->uPagePerBlock = 128;
            pNAND->nPageSize = 8192;
            pNAND->uSectorPerBlock = pNAND->nPageSize / 512 * pNAND->uPagePerBlock;
            pNAND->uNandECC = NAND_BCH_T24;
            pNAND->bIsMulticycle = 1;
            pNAND->uSpareSize = 376;
            break;
        } else if ((tempID[3] & 0x33) == 0x32) {
            pNAND->uBlockPerFlash = 4095;
            pNAND->uPagePerBlock = 128;
            pNAND->uSectorPerBlock = 1024;    /* 128x8 */
            pNAND->nPageSize = 4096;
            pNAND->uNandECC = NAND_BCH_T8;
            pNAND->bIsMLCNand = 1;
            pNAND->uSpareSize = 128;
        } else if ((tempID[3] & 0x33) == 0x11) {
            pNAND->uBlockPerFlash = 16383;
            pNAND->uPagePerBlock = 64;
            pNAND->uSectorPerBlock = 256;
            pNAND->nPageSize = 2048;
            pNAND->uNandECC = NAND_BCH_T4;
            pNAND->bIsMLCNand = 0;
            pNAND->uSpareSize = 64;
        } else if ((tempID[3] & 0x33) == 0x21) {
            pNAND->uBlockPerFlash = 8191;
            pNAND->uPagePerBlock = 128;
            pNAND->uSectorPerBlock = 512;
            pNAND->nPageSize = 2048;
            pNAND->uNandECC = NAND_BCH_T4;
            pNAND->bIsMLCNand = 1;
            pNAND->uSpareSize = 64;
        } else if ((tempID[3] & 0x3) == 0x3) {
            pNAND->uBlockPerFlash = 8191;//?
            pNAND->uPagePerBlock = 128;
            pNAND->uSectorPerBlock = 512;//?
            pNAND->nPageSize = 8192;
            pNAND->uNandECC = NAND_BCH_T12;
            pNAND->bIsMLCNand = 1;
            pNAND->uSpareSize = 368;
        }
        pNAND->bIsMulticycle = 1;
        break;

    default:
        // 2017/3/21, To support Micron MT29F32G08CBACA NAND flash
        if ((tempID[0]==0x2C)&&(tempID[1]==0x68)&&(tempID[2]==0x04)&&(tempID[3]==0x4A)&&(tempID[4]==0xA9)) {
            pNAND->uBlockPerFlash  = 4095;        // block index with 0-base. = physical blocks - 1
            pNAND->uPagePerBlock   = 256;
            pNAND->uSectorPerBlock = pNAND->nPageSize / 512 * pNAND->uPagePerBlock;
            pNAND->nPageSize       = 4096;
            pNAND->uNandECC        = NAND_BCH_T24;
            pNAND->bIsMulticycle   = 1;
            //pNAND->uSpareSize      = 224;
            pNAND->uSpareSize      = 188;
            pNAND->bIsMLCNand      = 1;
            break;
        }
        IsID=1;
    }

    /* Using PowerOn setting*/
    u32PowerOn = inpw(REG_SYS_PWRON);
    if ((u32PowerOn & 0xC0) != 0xC0) { /* PageSize PWRON[7:6] */
        const uint16_t BCH12_SPARE[3] = { 92,184,368};/* 2K, 4K, 8K */
        const uint16_t BCH15_SPARE[3] = {116,232,464};/* 2K, 4K, 8K */
        const uint16_t BCH24_SPARE[3] = { 90,180,360};/* 2K, 4K, 8K */
        unsigned int volatile gu_fmiSM_PageSize;
        unsigned int volatile g_u32ExtraDataSize;

        //sysprintf("Using PowerOn setting(0x%x): ", (u32PowerOn>>6)&0xf);
        gu_fmiSM_PageSize = 1024 << (((u32PowerOn >> 6) & 0x3) + 1);
        switch(gu_fmiSM_PageSize) {
        case 2048:
            sysprintf(" PageSize = 2KB ");
            pNAND->uPagePerBlock   = 64;
            pNAND->uNandECC        = NAND_BCH_T4;
            break;
        case 4096:
            sysprintf(" PageSize = 4KB ");
            pNAND->uPagePerBlock   = 128;
            pNAND->uNandECC        = NAND_BCH_T8;
            break;
        case 8192:
            sysprintf(" PageSize = 8KB ");
            pNAND->uPagePerBlock   = 128;
            pNAND->uNandECC        = NAND_BCH_T12;
            break;
        }

        if((u32PowerOn & 0x300) != 0x300) { /* ECC PWRON[9:8] */
            switch((u32PowerOn & 0x300)) {
            case 0x000:
                sysprintf(" ECC = T12\n");
                g_u32ExtraDataSize = BCH12_SPARE[gu_fmiSM_PageSize >> 12] + 8;
                pNAND->uNandECC = NAND_BCH_T12;
                break;
            case 0x100:
                sysprintf(" ECC = T15\n");
                g_u32ExtraDataSize = BCH15_SPARE[gu_fmiSM_PageSize >> 12] + 8;
                pNAND->uNandECC = NAND_BCH_T15;
                break;
            case 0x200:
                sysprintf(" ECC = T24\n");
                g_u32ExtraDataSize = BCH24_SPARE[gu_fmiSM_PageSize >> 12] + 8;
                pNAND->uNandECC = NAND_BCH_T24;
                break;
            }
        } else {
            switch(gu_fmiSM_PageSize) {
            case 512:
                g_u32ExtraDataSize = NAND_EXTRA_512;
                break;
            case 2048:
                g_u32ExtraDataSize = NAND_EXTRA_2K;
                break;
            case 4096:
                g_u32ExtraDataSize = NAND_EXTRA_4K;
                break;
            case 8192:
                g_u32ExtraDataSize = NAND_EXTRA_8K;
                break;
            }
        }

        pNAND->nPageSize       = gu_fmiSM_PageSize;
        pNAND->uSectorPerBlock = pNAND->nPageSize / 512 * pNAND->uPagePerBlock;
        pNAND->uSpareSize      = g_u32ExtraDataSize;

    } else {
        if(IsID==1) {
            sysprintf("SM ID not support!! [%x][%x][%x][%x][%x]\n", tempID[0], tempID[1], tempID[2], tempID[3], tempID[4]);
            return 1;
        }
//        else {
//            sysprintf("Auto Detect:\nBlockPerFlash= %d, PagePerBlock= %d\n", pNAND->uBlockPerFlash, pNAND->uPagePerBlock);
//        }
    }

    //sysprintf("PageSize= %d, ECC= 0x%x, ExtraDataSize= %d, SectorPerBlock= %d\n\n", pNAND->nPageSize, pNAND->uNandECC, pNAND->uSpareSize, pNAND->uSectorPerBlock);
    return 0;
}

int NAND_ReadRA(NAND_INFO_T *pnand, uint32_t uPage, uint32_t ucColAddr)
{
	//sysprintf("readra: %d, %d\n", uPage, ucColAddr);

    // clear R/B flag
    while(!(inpw(REG_NANDINTSTS) & 0x40000));
    outpw(REG_NANDINTSTS, 0x400);

    outpw(REG_NANDCMD, 0x00);     // read command
    outpw(REG_NANDADDR, ucColAddr);                   // CA0 - CA7
    outpw(REG_NANDADDR, (ucColAddr >> 8) & 0xFF);     // CA8 - CA11
    outpw(REG_NANDADDR, uPage & 0xff);                // PA0 - PA7
	if (!pnand->bIsMulticycle)
        outpw(REG_NANDADDR, ((uPage >> 8) & 0xff)|0x80000000);    // PA8 - PA15
	else
	{
        outpw(REG_NANDADDR, (uPage >> 8) & 0xff);             // PA8 - PA15
        outpw(REG_NANDADDR, ((uPage >> 16) & 0xff)|0x80000000);   // PA16 - PA18
	}
    outpw(REG_NANDCMD, 0x30);     // read command

    if (!NAND_WaitReady())
        return 1;
    else
        return 0;
}


void NAND_CorrectDataBCH(uint8_t ucFieidIndex, uint8_t ucErrorCnt, uint8_t * pDAddr)
{
    uint32_t uaData[24], uaAddr[24];
    uint32_t uaErrorData[6];
    uint8_t  ii, jj;
	uint32_t nPageSize;
	uint32_t field_len, padding_len, parity_len;
	uint32_t total_field_num;
	uint8_t  *smra_index;

    //--- assign some parameters for different BCH and page size
    switch (inpw(REG_NANDCTL) & 0x7c0000) {
    case NAND_BCH_T24:
        field_len   = 1024;
        padding_len = BCH_PADDING_LEN_1024;
        parity_len  = BCH_PARITY_LEN_T24;
        break;
    case NAND_BCH_T15:
        field_len   = 512;
        padding_len = BCH_PADDING_LEN_512;
        parity_len  = BCH_PARITY_LEN_T15;
        break;
    case NAND_BCH_T12:
        field_len   = 512;
        padding_len = BCH_PADDING_LEN_512;
        parity_len  = BCH_PARITY_LEN_T12;
        break;
    case NAND_BCH_T8:
        field_len   = 512;
        padding_len = BCH_PADDING_LEN_512;
        parity_len  = BCH_PARITY_LEN_T8;
        break;
    case NAND_BCH_T4:
        field_len   = 512;
        padding_len = BCH_PADDING_LEN_512;
        parity_len  = BCH_PARITY_LEN_T4;
        break;
    default:
        sysprintf("ERROR: NAND_CorrectDataBCH(): invalid BCH_TSEL = 0x%08X\n", (UINT32)(inpw(REG_NANDCTL) & 0x7c0000));
        return;
    }

	nPageSize = inpw(REG_NANDCTL) & 0x30000;
    switch (nPageSize) {
    case 0x30000: total_field_num = 8192 / field_len; break;
    case 0x20000: total_field_num = 4096 / field_len; break;
    case 0x10000: total_field_num = 2048 / field_len; break;
    case 0x00000: total_field_num =  512 / field_len; break;
    default:
        sysprintf("ERROR: fmiSM_CorrectData_BCH(): invalid PSIZE = 0x%08X\n", nPageSize);
        return;
    }

    //--- got valid BCH_ECC_DATAx and parse them to uaData[]
    // got the valid register number of BCH_ECC_DATAx since one register include 4 error bytes
    jj = ucErrorCnt/4;
	jj ++;
	if (jj > 6)
	    jj = 6;     // there are 6 BCH_ECC_DATAx registers to support BCH T24

    for (ii=0; ii<jj; ii++)
    {
		uaErrorData[ii] = inpw(REG_NANDECCED0 + ii*4);
	}

    for (ii=0; ii<jj; ii++)
    {
	    uaData[ii*4+0] = uaErrorData[ii] & 0xff;
	    uaData[ii*4+1] = (uaErrorData[ii]>>8) & 0xff;
	    uaData[ii*4+2] = (uaErrorData[ii]>>16) & 0xff;
	    uaData[ii*4+3] = (uaErrorData[ii]>>24) & 0xff;
	}

    //--- got valid REG_BCH_ECC_ADDRx and parse them to uaAddr[]
    // got the valid register number of REG_BCH_ECC_ADDRx since one register include 2 error addresses
    jj = ucErrorCnt/2;
    jj ++;
	if (jj > 12)
	    jj = 12;    // there are 12 REG_BCH_ECC_ADDRx registers to support BCH T24

    for (ii=0; ii<jj; ii++)
    {
        uaAddr[ii*2+0] = inpw(REG_NANDECCEA0 + ii*4) & 0x07ff;   // 11 bits for error address
        uaAddr[ii*2+1] = (inpw(REG_NANDECCEA0 + ii*4)>>16) & 0x07ff;
    }

    //--- pointer to begin address of field that with data error
    pDAddr += (ucFieidIndex-1) * field_len;

    //--- correct each error bytes
    for (ii=0; ii<ucErrorCnt; ii++)
    {
        // for wrong data in field
        if (uaAddr[ii] < field_len) {
            sysprintf("BCH error corrected for data: address 0x%08X, data [0x%02X] --> ", pDAddr+uaAddr[ii], *(pDAddr+uaAddr[ii]));

            *(pDAddr+uaAddr[ii]) ^= uaData[ii];

            sysprintf("[0x%02X]\n", *(pDAddr+uaAddr[ii]));
        }
  	    // for wrong first-3-bytes in redundancy area
        else if (uaAddr[ii] < (field_len+3)) {
            uaAddr[ii] -= field_len;
            uaAddr[ii] += (parity_len*(ucFieidIndex-1));    // field offset

            sysprintf("BCH error corrected for 3 bytes: address 0x%08X, data [0x%02X] --> ",
                      (uint8_t *)REG_NANDRA0+uaAddr[ii], *((uint8_t *)REG_NANDRA0+uaAddr[ii]));

            *((uint8_t *)REG_NANDRA0+uaAddr[ii]) ^= uaData[ii];

            sysprintf("[0x%02X]\n", *((uint8_t *)REG_NANDRA0+uaAddr[ii]));
        }
    	// for wrong parity code in redundancy area
    	else
    	{
    	    // BCH_ERR_ADDRx = [data in field] + [3 bytes] + [xx] + [parity code]
    	    //                                   |<--     padding bytes      -->|
    	    // The BCH_ERR_ADDRx for last parity code always = field size + padding size.
    	    // So, the first parity code = field size + padding size - parity code length.
    	    // For example, for BCH T12, the first parity code = 512 + 32 - 23 = 521.
    	    // That is, error byte address offset within field is
    	    uaAddr[ii] = uaAddr[ii] - (field_len + padding_len - parity_len);

            // smra_index point to the first parity code of first field in register SMRA0~n
            smra_index = (uint8_t *)
                         (REG_NANDRA0 + (inpw(REG_NANDRACTL) & 0x1ff) - // bottom of all parity code -
                          (parity_len * total_field_num)             // byte count of all parity code
                         );

            // final address = first parity code of first field +
            //                 offset of fields +
            //                 offset within field
            sysprintf("BCH error corrected for parity: address 0x%08X, data [0x%02X] --> ",
                      smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii],
                      *((uint8_t *)smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii]));
            *((uint8_t *)smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii]) ^= uaData[ii];
            sysprintf("[0x%02X]\n", *((uint8_t *)smra_index + (parity_len * (ucFieidIndex-1)) + uaAddr[ii]));
        }
  	}   // end of for (ii<ucErrorCnt)
}


int NAND_ReadDataEccCheck(long addr)
{
	uint32_t uStatus;
	uint32_t uErrorCnt=0, ii, jj;
	uint32_t volatile uError = 0;
	uint32_t uLoop;

    //--- uLoop is the number of SM_ECC_STx should be check.
    //      One SM_ECC_STx include ECC status for 4 fields.
    //      Field size is 1024 bytes for BCH_T24 and 512 bytes for other BCH.
    switch (inpw(REG_NANDCTL) & 0x30000) {	/* page size */
    case 0x10000:
        uLoop = 1;
        break;
    case 0x20000:
        if ((inpw(REG_NANDCTL) & 0x7c0000) == NAND_BCH_T24)
            uLoop = 1;
        else
            uLoop = 2;
        break;
    case 0x30000:
        if ((inpw(REG_NANDCTL) & 0x7c0000) == NAND_BCH_T24)
            uLoop = 2;
        else
            uLoop = 4;
        break;
    default:
        return -1;     // don't work for 512 bytes page
    }

    outpw(REG_FMI_DMASA, addr);    // set DMA transfer starting address
    outpw(REG_NANDINTSTS, 0x1|0x4); // clear DMA flag and ECC_FIELD flag
    outpw(REG_NANDCTL, inpw(REG_NANDCTL) | 0x2);    // begin to move data by DMA

    //--- waiting for DMA transfer stop since complete or ECC error
    // IF no ECC error, DMA transfer complete and make SMCR[DRD_EN]=0
    // IF ECC error, DMA transfer suspend     and make SMISR[ECC_FIELD_IF]=1 but keep keep SMCR[DRD_EN]=1
    //      If we clear SMISR[ECC_FIELD_IF] to 0, DMA transfer will resume.
    // So, we should keep wait if DMA not complete (SMCR[DRD_EN]=1) and no ERR error (SMISR[ECC_FIELD_IF]=0)
    while((inpw(REG_NANDCTL) & 0x2) && ((inpw(REG_NANDINTSTS) & 0x4)==0))
        ;

    //--- DMA transfer completed or suspend by ECC error, check and correct ECC error
	if (inpw(REG_NANDCTL) & 0x80)
	{
		while(1)
		{
            if (inpw(REG_NANDINTSTS) & 0x4)
			{
				for (jj=0; jj<uLoop; jj++)
				{
                    uStatus = inpw(REG_NANDECCES0+jj*4);
		            if (!uStatus)
		            	continue;   // no error on this register for 4 fields
                    // ECC error !! Check 4 fields. Each field has 512 bytes data
		        	for (ii=1; ii<5; ii++)
		        	{
			            if (!(uStatus & 0x3))     // no error for this field
			            {
							uStatus >>= 8;  // next field
			            	continue;
						}

			    		if ((uStatus & 0x3) == 0x01)  // correctable error in field (jj*4+ii)
			            {
			                uErrorCnt = (uStatus >> 2) & 0x1F;
			                NAND_CorrectDataBCH(jj*4+ii, uErrorCnt, (uint8_t *)addr);
							sysprintf("Warning: Field %d have %d BCH error. Corrected!!\n", jj*4+ii, uErrorCnt);
			                break;
			            }
			    		else if (((uStatus & 0x3) == 0x02) ||
			    		         ((uStatus & 0x3) == 0x03)) // uncorrectable error or ECC error in 1st field
			            {
			        	    sysprintf("ERROR: Field %d encountered uncorrectable BCH error!! 0x%x\n", jj*4+ii, uStatus);
				        	uError = 1;
			                break;
			            }
						uStatus >>= 8;  // next field
					}
				}
                outpw(REG_NANDINTSTS, 0x4);      // clear ECC_FIELD_IF to resume DMA transfer
			}

            if (inpw(REG_NANDINTSTS) & 0x1)    // wait to finish DMAC transfer.
			{
                if (!(inpw(REG_NANDINTSTS) & 0x4))
					break;
			}
		}   // end of while(1)
	}
	//--- Don't check ECC. Just wait the DMA finish.
	else
	{
		while(1)
		{
            outpw(REG_NANDINTSTS, 0x4);
            if (inpw(REG_NANDINTSTS) & 0x1) // wait to finish DMAC transfer.
			{
                outpw(REG_NANDINTSTS, 0x1); // clear DMA flag
				break;
			}
		}   // end of while(1)
	}

    if (uError)
   		return -1;
    else
	    return uErrorCnt;
}

uint8_t gNandbuffer[8192] __attribute__((aligned(32)));
uint8_t *pNandbuf;

int NAND_CheckBootHeader(NAND_INFO_T *pNAND)
{
    int fmiNandSysArea=0;
    int volatile status, i, count;
    unsigned int *ptr;

    pNandbuf = (UINT8 *)((UINT32)gNandbuffer | 0x80000000);
    ptr = (UINT32 *)((UINT32)gNandbuffer | 0x80000000);

    for (i=0; i<4; i++)
    {
        status = NAND_ReadPage(i, 0, pNandbuf);
        if (!status)
        {
            if ((*(ptr+0)) == 0x4E565420)
            {
                if ((*(ptr+4)) == 0xAA55AA55)
                {
                    count = (*(ptr+5));
                    for(i = 0; i<count; i++)
                    {
                        if ((*(ptr + 4 + (i * 2) + 2)) == 0xBC007FF0)
                        {
                            fmiNandSysArea = *(ptr + 4 + (i * 2) + 3);
                        }
                    }
                }
            }
        }
    }

    if ((fmiNandSysArea != 0xFFFFFFFF) && (fmiNandSysArea != 0))
    {
        pNAND->uLibStartBlock = fmiNandSysArea;
    }
    return 0;
}


/// @endcond HIDDEN_SYMBOLS



/**
 *  @brief  This function use to initil FMI NAND.
 *
 *  @return None
 */

int32_t NAND_Init(NDISK_T *NDInfo)
{
    //sysprintf("NAND_Init\n");
    // enable NAND
    outpw(REG_FMI_CTL, FMI_CTL_NANDEN_Msk);

    /* set page size = 512B (default) enable CS0, disable CS1 */
    outpw(REG_NANDCTL, inpw(REG_NANDCTL) & ~0x02030000 | 0x04000000);

    //outpw(REG_NANDCTL, 0x20305);
    outpw(REG_NANDCTL, (inpw(REG_NANDCTL) & ~0x30000) | 0x00000);   //512 byte
    outpw(REG_NANDCTL, inpw(REG_NANDCTL) |  0x100); //protect RA 3 byte
    outpw(REG_NANDCTL, inpw(REG_NANDCTL) | 0x10);

	memset((char *)&tNAND, 0, sizeof(NAND_INFO_T));

	if (NAND_ReadID(&tNAND))
	{
		sysprintf("Not support ID !!\n");
	}

    outpw(REG_NANDCTL,  inpw(REG_NANDCTL) | 0x800080);  // enable ECC

    //--- Set register to disable Mask ECC feature
    outpw(REG_NANDRACTL, inpw(REG_NANDRACTL) & ~0xffff0000);

    //--- Set registers that depend on page size. According to FA95 sepc, the correct order is
    //--- 1. SMCR_BCH_TSEL  : to support T24, MUST set SMCR_BCH_TSEL before SMCR_PSIZE.
    //--- 2. SMCR_PSIZE     : set SMCR_PSIZE will auto change SMRE_REA128_EXT to default value.
    //--- 3. SMRE_REA128_EXT: to use non-default value, MUST set SMRE_REA128_EXT after SMCR_PSIZE.
    outpw(REG_NANDCTL, (inpw(REG_NANDCTL) & ~0x7c0000) | tNAND.uNandECC);
	if (tNAND.nPageSize == 8192)
        outpw(REG_NANDCTL, (inpw(REG_NANDCTL)&(~0x30000)) | 0x30000);
	else if (tNAND.nPageSize == 4096)
        outpw(REG_NANDCTL, (inpw(REG_NANDCTL)&(~0x30000)) | 0x20000);
	else if (tNAND.nPageSize == 2048)
        outpw(REG_NANDCTL, (inpw(REG_NANDCTL)&(~0x30000)) | 0x10000);
    else    // Page size should be 512 bytes
        outpw(REG_NANDCTL, (inpw(REG_NANDCTL)&(~0x30000)) | 0x00000);
    outpw(REG_NANDRACTL, (inpw(REG_NANDRACTL) & ~0x1ff) | tNAND.uSpareSize);

    //TODO: if need protect --- config register for Region Protect
    outpw(REG_NANDCTL, inpw(REG_NANDCTL) & ~0x20);   // disable Region Protect

    /* check boot header */
    NAND_CheckBootHeader(&tNAND);
    while(1)
    {
        if (NAND_IsValidBlock(tNAND.uLibStartBlock))
            break;
        else
            tNAND.uLibStartBlock++;
    }
    if (tNAND.uLibStartBlock == 0)
        tNAND.uLibStartBlock++;
    sysprintf("NAND start block: %d\n", tNAND.uLibStartBlock);

    /* set NDInfo */
    tNAND.uBlockPerFlash -= tNAND.uLibStartBlock;
    NDInfo->nPBlockCount = tNAND.uBlockPerFlash;
    NandDiskDriver.lblock = NDInfo->nLBlockCount = (tNAND.uBlockPerFlash / 1000) * 1000;
    NandDiskDriver.ppb = NDInfo->nPagePerBlock = tNAND.uPagePerBlock;
    NandDiskDriver.pagesize = NDInfo->nPageSize = tNAND.nPageSize;

    switch (tNAND.uNandECC) {
    case NAND_BCH_T8:
        NandDiskDriver.ecc = 8;
        break;
    case NAND_BCH_T12:
        NandDiskDriver.ecc = 12;
        break;
    case NAND_BCH_T15:
        NandDiskDriver.ecc = 15;
        break;
    case NAND_BCH_T24:
        NandDiskDriver.ecc = 24;
        break;
    default:
        NandDiskDriver.ecc = 4;
    }

#if 0
    {
        int volatile i;
        for (i=0; i<tNAND.uBlockPerFlash; i++)
            NAND_IsValidBlock(i);
    }
#endif
    // Disable Write Protect
    outpw(REG_NANDECTL, 0x01);
    return 0;
}

int32_t NAND_MarkBadBlock(int32_t uBlock)
{
    uint32_t page;

	while(!(inpw(REG_NANDINTSTS) & 0x40000));
	outpw(REG_NANDINTSTS, 0x400);

    if (tNAND.bIsMLCNand)
        page = (uBlock + 1) * tNAND.uPagePerBlock - 1;
    else
        page = uBlock * tNAND.uPagePerBlock;

	// send command
	outpw(REG_NANDCMD, 0x80);	    // serial data input command
	outpw(REG_NANDADDR, tNAND.nPageSize & 0xff);        // CA0 - CA7
	outpw(REG_NANDADDR, (tNAND.nPageSize >> 8) & 0xff);	// CA8 - CA12
	outpw(REG_NANDADDR, page & 0xff);	        // PA0 - PA7
	if (!tNAND.bIsMulticycle)
		outpw(REG_NANDADDR, ((page >> 8) & 0xff)|0x80000000);  // PA8 - PA15
	else
	{
		outpw(REG_NANDADDR, (page >> 8) & 0xff);		    // PA8 - PA15
		outpw(REG_NANDADDR, ((page >> 16) & 0xff)|0x80000000); // PA16 - PA17
	}

	outpw(REG_NANDDATA, 0xf0);  // mark bad block (use 0xf0 instead of 0x00 to differ from Old (Factory) Bad Blcok Mark)
	outpw(REG_NANDCMD, 0x10);	// auto program command

    if (!NAND_WaitReady())
        return 1;

    NAND_Reset();
	return 0;
}


/**
 *  @brief  This function use to read NAND one page.
 *
 *  @param[in]     pba   The physical block.
 *  @param[in]     page  page of block
 *  @param[out]    buff  The buffer to receive the data from NAND.
 *
 *  @return   - 0  Successful.
 *            - 1  Fail.
 */
int32_t NAND_ReadPage(int32_t pba, int32_t page, uint8_t *buff)
{
	int pageNo;
	int volatile i;
	unsigned char *ptr;
	int spareSize;

    //sysprintf("pread: %d, %d, 0x%x\n", pba, page, buff);

    pba += tNAND.uLibStartBlock;
	pageNo = pba * tNAND.uPagePerBlock + page;

    //--- read redunancy area to register SMRAx
	spareSize = inpw(REG_NANDRACTL) & 0x1ff;
	ptr = (unsigned char *)REG_NANDRA0;
	NAND_ReadRA(&tNAND, pageNo, tNAND.nPageSize);
	for (i=0; i<spareSize; i++)
		*ptr++ = inpw(REG_NANDDATA) & 0xff;		// copy RA data from NAND to SMRA by SW

    while(!(inpw(REG_NANDINTSTS) & 0x40000));
    outpw(REG_NANDINTSTS, 0x400);
	outpw(REG_NANDCMD, 0x00);
	outpw(REG_NANDADDR, 0);	        // CA0 - CA7
	outpw(REG_NANDADDR, 0);			// CA8 - CA12
	outpw(REG_NANDADDR, pageNo & 0xff);	// PA0 - PA7
	if (!tNAND.bIsMulticycle)
		outpw(REG_NANDADDR, ((pageNo >> 8) & 0xff)|0x80000000);  // PA8 - PA15
	else
	{
		outpw(REG_NANDADDR, (pageNo >> 8) & 0xff);				// PA8 - PA15
		outpw(REG_NANDADDR, ((pageNo >> 16) & 0xff)|0x80000000); // PA16 - PA17
	}
	outpw(REG_NANDCMD, 0x30);		// read command

    if (!NAND_WaitReady())
        return -1;

	if (NAND_ReadDataEccCheck((long)buff))
	{
		return -1;
	}

	return 0;
}

/**
 *  @brief  This function use to write one page data to NAND.
 *
 *  @param[in]     pba   The physical block.
 *  @param[in]     page  page of block
 *  @param[out]    buff  The buffer to send data to NAND.
 *
 *  @return   - 1  Fail.
 *            - 0  Successful.
 */
int32_t NAND_WritePage(int32_t pba, int32_t page, uint8_t *buff)
{
	int pageNo;

    //sysprintf("pwrite: %d, %d, 0x%x\n", pba, page, buff);

    pba += tNAND.uLibStartBlock;
	pageNo = pba * tNAND.uPagePerBlock + page;

	outpw(REG_FMI_DMASA, (long)(buff));

	// set the spare area configuration
	/* write byte 2050, 2051 as used page */
	outpw(REG_NANDRA0, 0x0000FFFF);
	while(!(inpw(REG_NANDINTSTS) & 0x40000));
	outpw(REG_NANDINTSTS, 0x400);

	// send command
	outpw(REG_NANDCMD, 0x80);	    // serial data input command
	outpw(REG_NANDADDR, 0);	        // CA0 - CA7
	outpw(REG_NANDADDR, 0);			// CA8 - CA12
	outpw(REG_NANDADDR, pageNo & 0xff);	        // PA0 - PA7
	if (!tNAND.bIsMulticycle)
		outpw(REG_NANDADDR, ((pageNo >> 8) & 0xff)|0x80000000);  // PA8 - PA15
	else
	{
		outpw(REG_NANDADDR, (pageNo >> 8) & 0xff);		    // PA8 - PA15
		outpw(REG_NANDADDR, ((pageNo >> 16) & 0xff)|0x80000000); // PA16 - PA17
	}

	outpw(REG_NANDINTSTS, (0x1|0x4|0x8));
	outpw(REG_NANDCTL, inpw(REG_NANDCTL) | (0x10|0x4));

	while(1)
	{
		if (inpw(REG_NANDINTSTS) & 0x1)
			break;
	}

	outpw(REG_NANDINTSTS, 0x1);		// clear DMA flag
	outpw(REG_NANDCMD, 0x10);	    // auto program command

    if (!NAND_WaitReady())
        return 1;

    //--- check Region Protect result
    if (inpw(REG_NANDINTSTS) & 0x8) {
        sysprintf("ERROR: NAND_WritePage(): region write protect detected!!\n");
        outpw(REG_NANDINTSTS, 0x8);      // clear Region Protect flag
        return 1;
    }

	outpw(REG_NANDCMD, 0x70);		    // status read command
	if (inpw(REG_NANDDATA) & 0x01)	// 1:fail; 0:pass
	{
		sysprintf("write NAND page fail !!!\n");
		return 1;
	}

	return 0;
}

int32_t NAND_IsDirtyPage(int32_t pba, int32_t nPageNo)
{
    uint32_t page;
    uint8_t c0, c1;

	//sysprintf("NAND_IsDirtyPage: %d, %d\n", uPage, ucColAddr);
    pba += tNAND.uLibStartBlock;
    page = pba * tNAND.uPagePerBlock + nPageNo;

    // clear R/B flag
    while(!(inpw(REG_NANDINTSTS) & 0x40000));
    outpw(REG_NANDINTSTS, 0x400);

    outpw(REG_NANDCMD, 0x00);     // read command
    outpw(REG_NANDADDR, (tNAND.nPageSize & 0xff) + 2);      // CA0 - CA7
    outpw(REG_NANDADDR, (tNAND.nPageSize >> 8) & 0xFF);     // CA8 - CA11
    outpw(REG_NANDADDR, page & 0xff);                // PA0 - PA7
	if (!tNAND.bIsMulticycle)
        outpw(REG_NANDADDR, ((page >> 8) & 0xff)|0x80000000);    // PA8 - PA15
	else
	{
        outpw(REG_NANDADDR, (page >> 8) & 0xff);             // PA8 - PA15
        outpw(REG_NANDADDR, ((page >> 16) & 0xff)|0x80000000);   // PA16 - PA18
	}
    outpw(REG_NANDCMD, 0x30);     // read command

    if (!NAND_WaitReady())
        return 1;

    c0 = inpw(REG_NANDDATA) & 0xff;
    c1 = inpw(REG_NANDDATA) & 0xff;

    if((c0 != 0xFF) || (c1 != 0xFF))
        return 1;

    return 0; // not dirty
}

int32_t NAND_IsValidBlock(int32_t pba)
{
	NAND_INFO_T *pnand;
	int volatile status=0;
	unsigned int volatile sector, blockStatus=0xff;

    pba += tNAND.uLibStartBlock;
	if (pba == 0)
		return 1;

	pnand = &tNAND;
	if (pnand->bIsMLCNand == 1)
		sector = (pba+1) * pnand->uPagePerBlock - 2;
	else
		sector = pba * pnand->uPagePerBlock;

	status = NAND_ReadRA(pnand, sector, pnand->nPageSize);

	if (status < 0)
	{
		sysprintf("ERROR: NAND_IsValidBlock(), for block %d, return 0x%x\n", pba, status);
		return 0;
	}

	blockStatus = inpw(REG_NANDDATA) & 0xff;
	if (blockStatus == 0xFF)
	{
		NAND_Reset();
		status = NAND_ReadRA(pnand, sector+1, pnand->nPageSize);
		if (status < 0)
		{
            sysprintf("ERROR: NAND_IsValidBlock(), for block %d, return 0x%x\n", pba, status);
            return 0;
		}
		blockStatus = inpw(REG_NANDDATA) & 0xff;
		if (blockStatus != 0xFF)
		{
			NAND_Reset();
			return 0;	// invalid block
		}
	}
	else
	{
		NAND_Reset();
		return 0;	// invalid block
	}

	NAND_Reset();
	return 1;
}

int32_t NAND_EraseBlock(int32_t pba)
{
	NAND_INFO_T *pnand;
	uint32_t page_no;

    pba += tNAND.uLibStartBlock;
	pnand = &tNAND;
	if (NAND_IsValidBlock(pba) == 1)
	{
		page_no = pba * pnand->uPagePerBlock;		// get page address

		while (!(inpw(REG_NANDINTSTS) & 0x40000));
		outpw(REG_NANDINTSTS, 0x400);
		if (inpw(REG_NANDINTSTS) & 0x4)
		{
			sysprintf("erase: error sector !!\n");
			outpw(REG_NANDINTSTS, 0x4);
		}

		outpw(REG_NANDCMD, 0x60);				// erase setup command
		outpw(REG_NANDADDR, page_no & 0xff);		// PA0 - PA7
		if (pnand->bIsMulticycle)
		{
			outpw(REG_NANDADDR, ((page_no >> 8) & 0xff));				// PA8 - PA15
			outpw(REG_NANDADDR, ((page_no >> 16) & 0xff)|0x80000000);	// PA16 - PA17
		}
		else
			outpw(REG_NANDADDR, ((page_no >> 8) & 0xff)|0x80000000);    // PA8 - PA15

		outpw(REG_NANDCMD, 0xd0);		// erase command

        if (!NAND_WaitReady())
            return -1;

		outpw(REG_NANDCMD, 0x70);		// status read command
		if (inpw(REG_NANDDATA) & 0x01)	// 1:fail; 0:pass
		{
			//sysprintf("NAND_EraseBlock error!!\n");
			return -1;
		}
	}
	else
		return -1;

	return 0;
}

/*@}*/ /* end of group N9H30_NAND_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group N9H30_NAND_Driver */

/*@}*/ /* end of group N9H30_Device_Driver */

NDRV_T NandDiskDriver = {
    0,
    0,
    0,
    0,
    NAND_Init,
    0,
    NAND_ReadPage,
    NAND_WritePage,
    NAND_IsDirtyPage,
    NAND_IsValidBlock,
    NAND_EraseBlock,
    NAND_MarkBadBlock
};

NDRV_T *ptNDriver = &NandDiskDriver;
