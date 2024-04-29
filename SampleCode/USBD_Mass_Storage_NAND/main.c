/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Date: 15/05/11 10:06a $
 * @brief    Use internal SRAM as back end storage media to simulate a
 *           30 KB USB pen drive
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include <string.h>
#include "N9H31.h"
#include "sys.h"
#include "usbd.h"
#include "nand.h"
#include "nandlib.h"
#include "massstorage.h"

/*--------------------------------------------------------------------------*/
extern uint8_t volatile g_u8MscStart;
extern void USBD_IRQHandler(void);

NDISK_T tNDisk;
NDISK_T *ptNDisk = &tNDisk;
extern NDRV_T *ptNDriver;

uint32_t volatile g_u8InitFlag = 0;

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


/*---------------------------------------------------------------------------------------------------------*/
/*  Main Function                                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
int32_t main (void)
{
    sysDisableCache();
    sysInvalidCache();
    sysSetMMUMappingMethod(MMU_DIRECT_MAPPING);
    sysEnableCache(CACHE_WRITE_BACK);
    sysInitializeUART();
    sysprintf("\n\n\n\n");
    sysprintf("==========================\n");
    sysprintf("     USB Mass Storage     \n");
    sysprintf("==========================\n");

    sysInstallISR(HIGH_LEVEL_SENSITIVE|IRQ_LEVEL_1, USBD_IRQn, (PVOID)USBD_IRQHandler);
    sysEnableInterrupt(USBD_IRQn);

	/*--- init NAND ---*/
    FMI_Init();
    if (NANDLIB_Init(ptNDisk, ptNDriver) < 0)
        g_u8InitFlag = 0;
    else
        g_u8InitFlag = 1;

	/*--- init timer ---*/
	sysSetTimerReferenceClock (TIMER0, 12000000);
	sysStartTimer(TIMER0, 100, PERIODIC_MODE);

    USBD_Open(&gsInfo, MSC_ClassRequest, NULL);

    /* Endpoint configuration */
    MSC_Init();

    /* Start transaction */
    while(1)
    {
        if (USBD_IS_ATTACHED())
        {
            USBD_Start();
            break;
        }
    }

    while(1)
    {
        if (g_usbd_Configured)
            MSC_ProcessCmd();
    }
}



/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/

