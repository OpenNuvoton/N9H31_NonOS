/**************************************************************************//**
 * @file     main.c
 * @brief    N9H31 Driver Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "N9H31.h"
#include "sys.h"

/*-----------------------------------------------------------------------------*/
int main(void)
{
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    sysInitializeUART();
	sysprintf("\n\n Hello N9H31 !!!\n");

    sysprintf("APLL    clock %d MHz\n", sysGetClock(SYS_APLL));
    sysprintf("UPLL    clock %d MHz\n", sysGetClock(SYS_UPLL));
    sysprintf("CPU     clock %d MHz\n", sysGetClock(SYS_CPU));
    sysprintf("System  clock %d MHz\n", sysGetClock(SYS_SYSTEM));
    sysprintf("HCLK1   clock %d MHz\n", sysGetClock(SYS_HCLK1));
    sysprintf("HCLK234 clock %d MHz\n", sysGetClock(SYS_HCLK234));
    sysprintf("PCLK    clock %d MHz\n", sysGetClock(SYS_PCLK));

    return 0;
}
