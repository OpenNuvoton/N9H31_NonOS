/**************************************************************************//**
 * @file     main.c
 * @brief    N9H31 ETIMER Sample Code
 *
 * @note
 * Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "N9H31.h"
#include "sys.h"
#include "etimer.h"

/*-----------------------------------------------------------------------------*/
int main(void)
{
    
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    sysInitializeUART();

    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | (1 << 4)); // Enable ETIMER0 engine clock
    outpw(REG_SYS_GPB_MFPL, (inpw(REG_SYS_GPB_MFPL) & ~(0xF << 8)) | (0xF << 8)); // Enable ETIMER0 toggle out pin @ PB2

    sysprintf("\nThis sample code use timer 0 to generate 500Hz toggle output to PB.2 pin\n");
    
    /* To generate 500HZ toggle output, timer frequency must set to 1000Hz.
       Because toggle output state change on every timer timeout event */
    ETIMER_Open(0, ETIMER_TOGGLE_MODE, 1000);
    ETIMER_Start(0);

    while(1);

}
