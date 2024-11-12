/**************************************************************************//**
 * @file     NandLoader.c
 * @brief    NandLoader source code.
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#include <string.h>
#include "N9H31.h"
#include "sys.h"
#include "lcd.h"
#include "nand.h"

// define DATE CODE and show it when running to make maintaining easy.
#define DATE_CODE   "20240408"

/* define Loader and LOGO */
#define CONFIG_LOGO_ADDRESS     0x1C80000
#define CONFIG_LOGO_OFFSET      0x80000
#define CONFIG_LOGO_SIZE        0x30000     /* 192KB */

#define CONFIG_LOADER_ADDRESS   0x1C00000
#define CONFIG_LOADER_OFFSET    0xC0000
#define CONFIG_LOADER_SIZE      0x20000     /* 128KB */

/* global variable */
typedef struct nand_info
{
    unsigned int startBlock;
    unsigned int fileLen;
    unsigned int executeAddr;
} NVT_NAND_INFO_T;


INT MoveData(NVT_NAND_INFO_T *image, BOOL IsExecute)
{
    unsigned int page_count, block_count, curBlock, addr;
    int volatile i, j;
    void    (*fw_func)(void);

    //sysprintf("Load file length 0x%x, execute address 0x%x\n", image->fileLen, image->executeAddr);

    page_count = image->fileLen / tNAND.nPageSize;
    if ((image->fileLen % tNAND.nPageSize) != 0)
        page_count++;

    block_count = page_count / tNAND.uPagePerBlock;

    curBlock = image->startBlock;
    addr = image->executeAddr;
    j=0;
    while(1)
    {
        if (j >= block_count)
            break;
        if (NAND_IsValidBlock(curBlock))
        {
            for (i=0; i<tNAND.uPagePerBlock; i++)
            {
                NAND_ReadPage(curBlock, i, (UINT8 *)addr);
                addr += tNAND.nPageSize;
            }
            j++;
        }
        curBlock++;
    }

    if ((page_count % tNAND.uPagePerBlock) != 0)
    {
        page_count = page_count - block_count * tNAND.uPagePerBlock;
_read_:
        if (NAND_IsValidBlock(curBlock))
        {
            for (i=0; i<page_count; i++)
            {
                NAND_ReadPage(curBlock, i, (UINT8 *)addr);
                addr += tNAND.nPageSize;
            }
        }
        else
        {
            curBlock++;
            goto _read_;
        }
    }

    if (IsExecute == TRUE)
    {
        /* disable NAND control pin used */
        if (inpw(REG_SYS_PWRON) & 0x08000000)
        {
            outpw(REG_SYS_GPI_MFPL, 0);
            outpw(REG_SYS_GPI_MFPH, 0);
        }
        else
        {
            outpw(REG_SYS_GPC_MFPL, 0);
            outpw(REG_SYS_GPC_MFPH, 0);
        }

        fw_func = (void(*)(void))(image->executeAddr);
        fw_func();
    }
    return 0;
}

/*----------------------------------*/
/* Initial NAND                     */
/*----------------------------------*/

void FMI_Init(void)
{
    /* enable FMI NAND */
    outpw(REG_CLK_HCLKEN, (inpw(REG_CLK_HCLKEN) | 0x300000));
    // DMAC Initial
    outpw(REG_FMI_DMACTL, FMI_DMACTL_DMARST_Msk | FMI_DMACTL_DMAEN_Msk);
    while(inpw(REG_FMI_DMACTL) & FMI_DMACTL_DMARST_Msk);

    outpw(REG_FMI_CTL, FMI_CTL_CTLRST_Msk);      // reset FMI engine
    while(inpw(REG_FMI_CTL) & FMI_CTL_CTLRST_Msk);

    /* select NAND function pins */
    if (inpw(REG_SYS_PWRON) & 0x08000000)
    {
        /* Set GPI1~15 for NAND */
        outpw(REG_SYS_GPI_MFPL, 0x55555550);
        outpw(REG_SYS_GPI_MFPH, 0x55555555);
    }
    else
    {
        /* Set GPC0~14 for NAND */
        outpw(REG_SYS_GPC_MFPL, 0x55555555);
        outpw(REG_SYS_GPC_MFPH, 0x05555555);
    }
}


int main()
{
    NVT_NAND_INFO_T image;

    /* Clear Boot Code Header in SRAM to avoid booting fail issue */
    outp32(0xBC000000, 0);

    sysInitializeUART();
    sysprintf("N9H3x Nand Boot Loader entry (%s).\n", DATE_CODE);

    /* Initial DMAC and NAND interface */
    FMI_Init();
    NAND_Init();

    memset((char *)&image, 0, sizeof(NVT_NAND_INFO_T));
    /* read logo */
    image.startBlock = CONFIG_LOGO_OFFSET / (tNAND.uPagePerBlock * tNAND.nPageSize);
    image.executeAddr = CONFIG_LOGO_ADDRESS;
    image.fileLen = CONFIG_LOGO_SIZE;
    sysprintf("LOGO sb: %d, addr 0x%x, len 0x%x\n", image.startBlock, image.executeAddr, image.fileLen);
    MoveData(&image, FALSE);

    memset((char *)&image, 0, sizeof(NVT_NAND_INFO_T));
    /* read nvtloader */
    image.startBlock = CONFIG_LOADER_OFFSET / (tNAND.uPagePerBlock * tNAND.nPageSize);
    image.executeAddr = CONFIG_LOADER_ADDRESS;
    image.fileLen = CONFIG_LOADER_SIZE;
    sysprintf("Loader sb: %d, addr 0x%x, len 0x%x\n", image.startBlock, image.executeAddr, image.fileLen);
    MoveData(&image, TRUE);
    return 0;
}
