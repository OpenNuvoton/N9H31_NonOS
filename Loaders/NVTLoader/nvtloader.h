/**************************************************************************//**
 * @file     nvtloader.h
 * @version  V3.00
 * @brief    N9H20 series NVTLoader header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/

#define AP_PATH         "0:\\conprog.bin"

#define CP_SIZE 16 * 1024


#define PANEL_BPP       2
#define FB_ADDR     0x500000

#ifdef __DEBUG__
#define DBG_PRINTF      sysprintf
#else
#define DBG_PRINTF(...)
#endif


 /* Turn on the optional. Back light enable */
 /* Turn off the optional, ICE can connect to */
 /* Default Demo Board  GPD1 keep pull high */
 /*                                 */
//#define __BACKLIGHT__


/* NAND1-1 Size */
#define NAND1_1_SIZE     32 /* MB unit */
#define SD1_1_SIZE      128 /* MB unit */

