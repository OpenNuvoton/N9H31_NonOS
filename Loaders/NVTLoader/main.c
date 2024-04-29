/**************************************************************************//**
 * @file     main.c
 * @brief    Load conprog.bin code from NAND device for next booting stage
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <string.h>
#include "N9H31.h"
#include "sys.h"
#include "usbd.h"
#include "nand.h"
#include "nandlib.h"
#include "nvtloader.h"
#include "ff.h"
#include "diskio.h"
#include "massstorage.h"
#include "lcd.h"


uint8_t kbuf[CP_SIZE] __attribute__((aligned(32)));  /* save first 16k of buffer. Copy to 0 after vector table is no longer needed */
uint8_t *pkbuf;


BOOL bIsIceMode = FALSE;

/***********************************************/
/* Volume management table defined by user (required when FF_MULTI_PARTITION == 1) */

PARTITION VolToPart[] = {
    {0, 1},    /* "0:" ==> Physical drive 0, 1st partition */
    {0, 2},    /* "1:" ==> Physical drive 0, 2nd partition */
    {1, 0}     /* "2:" ==> Physical drive 1, auto detection */
};

static FIL kfd;        /* File objects */
FATFS gFatfsVol0, gFatfsVol1;

NDISK_T tNDisk;
NDISK_T *ptNDisk = &tNDisk;
extern NDRV_T *ptNDriver;
extern void USBD_IRQHandler(void);

UINT32 u32TimerChannel = 0;
void Timer0_300msCallback(void)
{
    sysClearTimerEvent(TIMER0, u32TimerChannel);
}

/*---------------------------------------------------------*/
/* User Provided RTC Function for FatFs module             */
/*---------------------------------------------------------*/
/* This is a real time clock service to be called from     */
/* FatFs module. Any valid time must be returned even if   */
/* the system does not support an RTC.                     */
/* This function is not required in read-only cfg.         */

unsigned long get_fattime (void)
{
    unsigned long tmr;

    tmr=0x00000;

    return tmr;
}


void sysInit(void)
{
    UINT32 u32Cke = inp32(REG_CLK_HCLKEN);

    /* Reset SIC engine to fix USB update app file */
    outp32(REG_CLK_HCLKEN, u32Cke | 0x700000);
    outp32(REG_SYS_AHBIPRST, 0x100000);
    outp32(REG_SYS_AHBIPRST, 0);

    outp32(REG_SYS_APBIPRST0, 0x300);
    outp32(REG_SYS_APBIPRST0, 0);

    outp32(REG_CLK_HCLKEN, u32Cke);
    sysEnableCache(CACHE_WRITE_BACK);

    /*--- init timer ---*/
    sysSetTimerReferenceClock (TIMER0, 12000000);
    sysStartTimer(TIMER0, 100, PERIODIC_MODE);
    u32TimerChannel = sysSetTimerEvent(TIMER0, 30, (PVOID)Timer0_300msCallback);

    sysInitializeUART();
    sysprintf("NVT Loader start\n");

//    kpi_init();
//  kpi_open(3); // use nIRQ0 as external interrupt source
//  bIsIceMode = kpi_read(KPI_NONBLOCK);
//  if(bIsIceMode!=FALSE)
//      bIsIceMode=TRUE;
    sysSetLocalInterrupt(ENABLE_IRQ);
}


UINT32 NVT_LoadAppFromNAND(void)
{
    uint32_t bytes, result, offset=0;
    INT found_app = 0;
    UINT32 u32TotalSize;
    void    (*_jump)(void);

    TCHAR nand_path[] = { '0', ':', 0 };
    TCHAR nand_path1[] = { '1', ':', 0 };
    DWORD fre_clust0, fre_clust1;
    FATFS *fs0, *fs1;
    DWORD plist[4];  /* Divide drive into two partitions */
    INT32 nStatus;

    pkbuf = (uint8_t *)((UINT32)&kbuf[0] | 0x80000000);   /* use non-cache buffer */

    /* In here for USB VBus stable. Othwise, USB library can not judge VBus correct  */
    USBD_Open(&gsInfo, MSC_ClassRequest, NULL);

    /* Mount NAND disk */
    f_mount(&gFatfsVol0, nand_path, 1);
    f_mount(&gFatfsVol1, nand_path1, 1);

    /* Get NAND disk information*/
    u32TotalSize = ptNDisk->nLBlockCount * ptNDisk->nPagePerBlock * ptNDisk->nPageSize;

    /* set partition size */
    plist[0] = NAND1_1_SIZE * 1024 * 2;
    plist[1] = (u32TotalSize >> 9) - plist[0];
    plist[2] = 0;
    plist[3] = 0;
    sysprintf("Total Disk Size %d, [%d / %d]\n", u32TotalSize, plist[0], plist[1]);

    /* Format NAND if necessery */
    if ((f_getfree(nand_path, &fre_clust0, &fs0)) || (f_getfree(nand_path1, &fre_clust1, &fs1)))
    {
        /* unMount NAND disk */
        f_mount(0, nand_path, 0);
        f_mount(0, nand_path1, 0);
        sysprintf("unknow disk type, format device .....\n");
        if (f_fdisk(0, plist, pkbuf) != FR_OK) {
            sysprintf("Format failed\n");
            goto halt;
        }
        f_mkfs("0:", FM_ANY, FF_MAX_SS, pkbuf, FF_MAX_SS);    /* Create FAT volume on the logical drive 0 */
        f_mkfs("1:", FM_ANY, FF_MAX_SS, pkbuf, FF_MAX_SS);    /* Create FAT volume on the logical drive 1 */

        /* Mount NAND disk */
        f_mount(&gFatfsVol0, nand_path, 1);
        f_mount(&gFatfsVol1, nand_path1, 1);

        f_setlabel("0:NAND1-1");
        f_setlabel("1:NAND1-2");
    }

#if 1
    /* Detect USB */
    if (USBD_IS_ATTACHED())
    {
        /* unMount NAND disk */
        f_mount(0, nand_path, 0);
        f_mount(0, nand_path1, 0);

        sysprintf("USB plug in [0x%x]\n", USBD->PHYCTL);
        sysInstallISR(HIGH_LEVEL_SENSITIVE|IRQ_LEVEL_1, USBD_IRQn, (PVOID)USBD_IRQHandler);
        sysEnableInterrupt(USBD_IRQn);
        MSC_Init();
        USBD_Start();
        while(1)
        {
            if (g_usbd_Configured)
                MSC_ProcessCmd();
            if (!USBD_IS_ATTACHED())
                break;
        }
        sysprintf("USB plug out\n");

        NANDLIB_DeInit(ptNDisk);
        /* Mount NAND disk */
        f_mount(&gFatfsVol0, nand_path, 1);
        f_mount(&gFatfsVol1, nand_path1, 1);
    }
#endif
    
    nStatus = f_open(&kfd, AP_PATH, FA_READ);
    if (nStatus == FR_OK)
    {
        found_app = 1;
        sysprintf("app found\n");
    }

    if (found_app)
    {
        while(1) {
            result = f_read(&kfd, (UINT8 *)offset, CP_SIZE, &bytes);
            if ((result == FR_OK) && (bytes != 0))
                offset += bytes;
            else
                break;
        }
    }

    if (found_app)
    {
        f_mount(0, nand_path, 0);
        f_mount(0, nand_path1, 0);
        /* Disable interrupt */
        sysSetGlobalInterrupt(DISABLE_ALL_INTERRUPTS);
        sysSetLocalInterrupt(DISABLE_FIQ_IRQ);
        /* Invalid and disable cache */
        sysDisableCache();
        sysInvalidCache();

        /* Reset IPs */
        sysprintf("Jump to app\n");
        //outp32(REG_SYS_AHBIPRST, 0x580000); // JPGRST | SICRST |UDCRST
        outp32(REG_SYS_AHBIPRST, 0x580008);
        outp32(REG_SYS_AHBIPRST, 0);
        outp32(REG_SYS_APBIPRST0, 0x30300); // UART1RST | UART0RST | TMR1RST | TMR0RST
        outp32(REG_SYS_APBIPRST0, 0);

        sysFlushCache(I_D_CACHE);

        _jump = (void(*)(void))(0x0); /* Jump to 0x0 and execute app */
        _jump();
    }
    else
    {
        sysprintf("Cannot find conprog.bin\n");
    }
halt:
    sysprintf("systen exit\n");
    while(1); // never return
}

void delay(UINT32 u32Tick)
{
    UINT32 btime, etime;
    btime = sysGetTicks(TIMER0);
    while (1)
    {
        etime = sysGetTicks(TIMER0);
        if ((etime - btime) >= u32Tick)
        {
            break;
        }
    }
}

static UINT32 bIsInitVpost = FALSE;
void initVPostShowLogo(void)
{
	if (bIsInitVpost == FALSE)
	{
		bIsInitVpost = TRUE;
        // Configure multi-function pin for LCD interface
        //GPG6 (CLK), GPG7 (HSYNC)
        outpw(REG_SYS_GPG_MFPL, (inpw(REG_SYS_GPG_MFPL)& ~0xFF000000) | 0x22000000);
        //GPG8 (VSYNC), GPG9 (DEN)
        outpw(REG_SYS_GPG_MFPH, (inpw(REG_SYS_GPG_MFPH)& ~0xFF) | 0x22);

        //DATA pin
        //GPA0 ~ GPA7 (DATA0~7)
        outpw(REG_SYS_GPA_MFPL, 0x22222222);
        //GPA8 ~ GPA15 (DATA8~15)
        outpw(REG_SYS_GPA_MFPH, 0x22222222);
        //GPD8 ~ GPD15 (DATA16~23)
        outpw(REG_SYS_GPD_MFPH, 0x22222222);
        // LCD clock is selected from UPLL and divide to 20MHz
        outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0xff1f) | 0xE18);
        // Init LCD interface for FW070TFT LCD module
        vpostLCMInit(DIS_PANEL_FW070TFT);
        // Set display color depth
        vpostSetVASrc(VA_SRC_RGB565);
        //vpostSetFrameBuffer((uint8_t *)0x1000000);

		/* 1. If backlight control signal is different from nuvoton¡¦s demo board,
           please don't call this function and must implement another similar one to enable LCD backlight. */
        //vpostEnaBacklight();
	}	
}


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


int main(void)
{
    sysDisableCache();
    sysFlushCache(I_D_CACHE);

    initVPostShowLogo();
    sysInit();

    /*--- init NAND ---*/
    FMI_Init();

    sysprintf("Load code from NAND\n");
    NVT_LoadAppFromNAND();

    return(0); // avoid compilation warning
}

