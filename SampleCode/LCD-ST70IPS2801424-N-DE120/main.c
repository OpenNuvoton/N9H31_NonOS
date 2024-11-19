/**************************************************************************//**
* @file     main.c
* @brief    N9H31 LCD ST70IPS2801424-N-DE120 sample source file
*
* @note
* Copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
#include <stdio.h>
#include <string.h>

#include "N9H31.h"
#include "sys.h"
#include "lcd.h"

#define DISPLAY_RGB888

#ifdef DISPLAY_RGB888
#include "image_rgb888_wall2.dat"
#endif

#if defined ( __GNUC__ ) && !(__CC_ARM)
__attribute__((aligned(32))) uint8_t _frame_buffer[280 * 1424 * 4];
__attribute__((aligned(32))) uint32_t u32CursorBuf[512];
#else
__align(32) uint8_t _frame_buffer[280 * 1424 * 4];
__align(32) uint32_t u32CursorBuf[512];
#endif

//=====================================================================
//  for panel init
//=====================================================================
//CS - PI.1
#define SpiEnable()     outpw(REG_GPIOI_DATAOUT, inpw(REG_GPIOI_DATAOUT) & ~0x2)
#define SpiDisable()    outpw(REG_GPIOI_DATAOUT, inpw(REG_GPIOI_DATAOUT) | 0x2)
//SCK - PG.0
#define SpiHighSCK()    outpw(REG_GPIOG_DATAOUT, inpw(REG_GPIOG_DATAOUT) | 0x1)
#define SpiLowSCK()     outpw(REG_GPIOG_DATAOUT, inpw(REG_GPIOG_DATAOUT) & ~0x1)
//SDA - PG.1
#define SpiHighSDA()    outpw(REG_GPIOG_DATAOUT, inpw(REG_GPIOG_DATAOUT) | 0x2)
#define SpiLowSDA()     outpw(REG_GPIOG_DATAOUT, inpw(REG_GPIOG_DATAOUT) & ~0x2)

void SpiDelay(uint32_t u32TimeCnt)
{
    uint32_t ii, jj;

    for (ii = 0; ii < u32TimeCnt; ii++)
        for (jj = 0; jj < 100; jj++)
            jj++;
}

void LCD_DelayMs(uint32_t Ms)
{
    volatile uint32_t i;

      for(; Ms; Ms--)
          for (i = 1 * 1000; i; i--);
}

void ILI_RdWtRegInit()
{
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x8);  /* Enable GPIO clock */

    // Init GPIO control pins
    outpw(REG_SYS_GPG_MFPL, inpw(REG_SYS_GPG_MFPL) & ~0xff);     /* PG.0:  SCL; PG1: SDA */
    outpw(REG_GPIOG_DIR, inpw(REG_GPIOG_DIR) | (1 << 1) | (1 << 0));
    outpw(REG_GPIOG_PUEN, inpw(REG_GPIOG_PUEN) | (1 << 1) | (1 << 0));
    outpw(REG_GPIOG_DATAOUT, inpw(REG_GPIOG_DATAOUT) | (1 << 1) | (1 << 0));

    outpw(REG_SYS_GPI_MFPL, inpw(REG_SYS_GPI_MFPL) & ~0xf0);     /* PI.1:  CS */
    outpw(REG_SYS_GPI_MFPH, inpw(REG_SYS_GPI_MFPH) & ~0xf000);   /* PI.11: RESET */
    outpw(REG_GPIOI_DIR, inpw(REG_GPIOI_DIR) | (1 << 11) | (1 << 1));
    outpw(REG_GPIOI_PUEN, inpw(REG_GPIOI_PUEN) | (1 << 11) | (1 << 1));
    outpw(REG_GPIOI_DATAOUT, inpw(REG_GPIOI_DATAOUT) | (1 << 11) | (1 << 1));

    outpw(REG_SYS_GPH_MFPL, inpw(REG_SYS_GPH_MFPL) & ~0xf00);    /* PH.2:  Backlight */
    outpw(REG_GPIOH_DIR, inpw(REG_GPIOH_DIR) | (1 << 2));
    outpw(REG_GPIOH_PUEN, inpw(REG_GPIOH_PUEN) | (1 << 2));
    // outpw(REG_GPIOH_DATAOUT, inpw(REG_GPIOH_DATAOUT) & ~(1 << 2));
    outpw(REG_GPIOH_DATAOUT, inpw(REG_GPIOH_DATAOUT) | (1 << 2));

    outpw(REG_GPIOI_DATAOUT, inpw(REG_GPIOI_DATAOUT) & ~(1 << 11));
    LCD_DelayMs(50);
    outpw(REG_GPIOI_DATAOUT, inpw(REG_GPIOI_DATAOUT) | (1 << 11));
    LCD_DelayMs(150);
}

void Wrt_Reg_3052(UINT8 nRegAddr, UINT8 nData)
{
    uint32_t i;

    SpiEnable();
    // send WR bit
    SpiLowSCK();
    SpiDelay(2);
    SpiLowSDA();
    SpiDelay(2);
    SpiHighSCK();
    SpiDelay(2);

    // Send register address
    for (i = 0; i < 8; i ++)
    {
        SpiLowSCK();

        if (nRegAddr & 0x80)
            SpiHighSDA();
        else
            SpiLowSDA();

        SpiDelay(3);
        SpiHighSCK();
        nRegAddr <<= 1;
        SpiDelay(2);
    }

    // send WR bit
    SpiLowSCK();
    SpiDelay(2);
    SpiHighSDA();
    SpiDelay(2);
    SpiHighSCK();
    SpiDelay(2);

    // Send register data
    for (i = 0; i < 8; i ++)
    {
        SpiLowSCK();
        if (nData & 0x80)
            SpiHighSDA();
        else
            SpiLowSDA();

        SpiDelay(3);
        SpiHighSCK();
        nData<<=1;
        SpiDelay(2);
    }
    SpiDisable();
    SpiDelay(15);
}

void Wrt_Reg_3052_1(UINT8 nRegAddr)
{
    uint32_t i;

    SpiEnable();
    // send WR bit
    SpiLowSCK();
    SpiDelay(2);
    SpiLowSDA();
    SpiDelay(2);
    SpiHighSCK();
    SpiDelay(2);

    // Send register address, MSB first
    for (i = 0; i < 8; i ++)
    {
        SpiLowSCK();
        if (nRegAddr & 0x80)
            SpiHighSDA();
        else
            SpiLowSDA();

        SpiDelay(3);
        SpiHighSCK();
        nRegAddr<<=1;
        SpiDelay(2);
    }
    SpiDisable();
    SpiDelay(15);
}

void NV3052_RGB_init(void)
{
    sysprintf("NV3052_RGB_init...\n");
    ILI_RdWtRegInit();

    sysprintf("Issue commands...\n");
    Wrt_Reg_3052(0xFF,0x30);
    Wrt_Reg_3052(0xFF,0x52);
    Wrt_Reg_3052(0xFF,0x01);
    Wrt_Reg_3052(0xE3,0x00);
    Wrt_Reg_3052(0x08,0x00);
    Wrt_Reg_3052(0x09,0x07);
    Wrt_Reg_3052(0x0A,0xfd);
    Wrt_Reg_3052(0x0B,0x32);
    Wrt_Reg_3052(0x0C,0x32);
    Wrt_Reg_3052(0x0D,0x0B);
    Wrt_Reg_3052(0x0E,0x00);
    Wrt_Reg_3052(0x24,0x10);
    Wrt_Reg_3052(0x25,0x0A);
    Wrt_Reg_3052(0x23,0x02);    // DE+SYNC
//      Wrt_Reg_3052(0x23,0x11);    // only SYNC
    Wrt_Reg_3052(0x20,0xA0);
    Wrt_Reg_3052(0x28,0x50);
    Wrt_Reg_3052(0x29,0xc5);
    Wrt_Reg_3052(0x2a,0x90);
    Wrt_Reg_3052(0x38,0x9C);
    Wrt_Reg_3052(0x39,0xA7);
    Wrt_Reg_3052(0x3A,0x4A);//VCOM  0x46: -1.0625V / 0x4A: -1.1125V / 0x4E: -1.1625V
    Wrt_Reg_3052(0x49,0x3C);
    Wrt_Reg_3052(0x91,0x77);
    Wrt_Reg_3052(0x92,0x77);
//    Wrt_Reg_3052(0x99,0x51);            // 0x51 VGH : 12V , 0x52 VGH : 13V   , 0x53 VGH : 14V , 0x54 VGH : 15V
//    Wrt_Reg_3052(0x9b,0x59);            // 0x59 VGL :-12V , 0x5A VGH :-12.5V , 0x5B VGH :-13V , 0x5C VGH :-14V
//    Wrt_Reg_3052(0x99,0x53);            // 0x51 VGH : 12V , 0x52 VGH : 13V   , 0x53 VGH : 14V , 0x54 VGH : 15V
//    Wrt_Reg_3052(0x9b,0x5b);            // 0x59 VGL :-12V , 0x5A VGH :-12.5V , 0x5B VGH :-13V , 0x5C VGH :-14V
//    Wrt_Reg_3052(0x99,0x54);            // 0x51 VGH : 12V , 0x52 VGH : 13V   , 0x53 VGH : 14V , 0x54 VGH : 15V
//    Wrt_Reg_3052(0x9b,0x5c);            // 0x59 VGL :-12V , 0x5A VGH :-12.5V , 0x5B VGH :-13V , 0x5C VGH :-14V
    Wrt_Reg_3052(0x99,0x54);            // 0x50 VGH : 11V , 0x51 VGH : 12V , 0x52 VGH : 13V   , 0x53 VGH : 14V , 0x54 VGH : 15V
    Wrt_Reg_3052(0x9b,0x59);            // 0x54 VGL :-09V , 0x56 VGL :-10V , 0x57 VGL :-11V , 0x59 VGL :-12V , 0x5A VGH :-12.5V , 0x5B VGH :-13V , 0x5C VGH :-14V
    Wrt_Reg_3052(0xA0,0x55);
    Wrt_Reg_3052(0xA1,0x50);
    Wrt_Reg_3052(0xA3,0xD8);
    Wrt_Reg_3052(0xA4,0x9C);
    Wrt_Reg_3052(0xA7,0x02);
    Wrt_Reg_3052(0xA8,0x01);
    Wrt_Reg_3052(0xA9,0x01);
    Wrt_Reg_3052(0xAA,0xFC);
    Wrt_Reg_3052(0xAB,0x28);
    Wrt_Reg_3052(0xAC,0x06);
    Wrt_Reg_3052(0xAD,0x06);
    Wrt_Reg_3052(0xAE,0x06);
    Wrt_Reg_3052(0xAF,0x03);
    Wrt_Reg_3052(0xB0,0x08);
    Wrt_Reg_3052(0xB1,0x26);
    Wrt_Reg_3052(0xB2,0x28);
    Wrt_Reg_3052(0xB3,0x28);
    Wrt_Reg_3052(0xB4,0x03);
    Wrt_Reg_3052(0xB5,0x08);
    Wrt_Reg_3052(0xB6,0x26);
    Wrt_Reg_3052(0xB7,0x08);
    Wrt_Reg_3052(0xB8,0x26);

    Wrt_Reg_3052(0xFF,0x30);
    Wrt_Reg_3052(0xFF,0x52);
    Wrt_Reg_3052(0xFF,0x02);
    Wrt_Reg_3052(0xB1,0x0B);
    Wrt_Reg_3052(0xD1,0x0B);
    Wrt_Reg_3052(0xB4,0x31);
    Wrt_Reg_3052(0xD4,0x2F);
    Wrt_Reg_3052(0xB2,0x09);
    Wrt_Reg_3052(0xD2,0x09);
    Wrt_Reg_3052(0xB3,0x2F);
    Wrt_Reg_3052(0xD3,0x2D);
    Wrt_Reg_3052(0xB6,0x14);
    Wrt_Reg_3052(0xD6,0x13);
    Wrt_Reg_3052(0xB7,0x37);
    Wrt_Reg_3052(0xD7,0x36);
    Wrt_Reg_3052(0xC1,0x08);
    Wrt_Reg_3052(0xE1,0x07);
    Wrt_Reg_3052(0xB8,0x0C);
    Wrt_Reg_3052(0xD8,0x0C);
    Wrt_Reg_3052(0xB9,0x03);
    Wrt_Reg_3052(0xD9,0x02);
    Wrt_Reg_3052(0xBD,0x13);
    Wrt_Reg_3052(0xDD,0x13);
    Wrt_Reg_3052(0xBC,0x11);
    Wrt_Reg_3052(0xDC,0x11);
    Wrt_Reg_3052(0xBB,0x10);
    Wrt_Reg_3052(0xDB,0x0F);
    Wrt_Reg_3052(0xBA,0x10);
    Wrt_Reg_3052(0xDA,0x10);
    Wrt_Reg_3052(0xBE,0x18);
    Wrt_Reg_3052(0xDE,0x1A);
    Wrt_Reg_3052(0xBF,0x0F);
    Wrt_Reg_3052(0xDF,0x11);
    Wrt_Reg_3052(0xC0,0x16);
    Wrt_Reg_3052(0xE0,0x18);
    Wrt_Reg_3052(0xB5,0x37);
    Wrt_Reg_3052(0xD5,0x32);
    Wrt_Reg_3052(0xB0,0x02);
    Wrt_Reg_3052(0xD0,0x05);

    Wrt_Reg_3052(0xFF,0x30);
    Wrt_Reg_3052(0xFF,0x52);
    Wrt_Reg_3052(0xFF,0x03);
    Wrt_Reg_3052(0x04,0x51);
    Wrt_Reg_3052(0x05,0x50);
    Wrt_Reg_3052(0x06,0x50);
    Wrt_Reg_3052(0x07,0x03);
    Wrt_Reg_3052(0x08,0x04);
    Wrt_Reg_3052(0x09,0x05);
    Wrt_Reg_3052(0x0A,0x06);
    Wrt_Reg_3052(0x0B,0x07);
    Wrt_Reg_3052(0x24,0x51);
    Wrt_Reg_3052(0x25,0x50);
    Wrt_Reg_3052(0x26,0x50);
    Wrt_Reg_3052(0x27,0x03);
    Wrt_Reg_3052(0x28,0x55);
    Wrt_Reg_3052(0x29,0x55);
    Wrt_Reg_3052(0x2A,0xa7);
    Wrt_Reg_3052(0x2B,0xaB);
    Wrt_Reg_3052(0x2c,0xaB);
    Wrt_Reg_3052(0x2d,0xaB);
    Wrt_Reg_3052(0x34,0x51);
    Wrt_Reg_3052(0x35,0x50);
    Wrt_Reg_3052(0x36,0x50);
    Wrt_Reg_3052(0x37,0x03);
    Wrt_Reg_3052(0x40,0x03);
    Wrt_Reg_3052(0x41,0x04);
    Wrt_Reg_3052(0x42,0x05);
    Wrt_Reg_3052(0x43,0x06);
    Wrt_Reg_3052(0x44,0x55);
    Wrt_Reg_3052(0x45,0x9E);
    Wrt_Reg_3052(0x46,0x9F);
    Wrt_Reg_3052(0x47,0x55);
    Wrt_Reg_3052(0x48,0xa0);
    Wrt_Reg_3052(0x49,0xa1);
    Wrt_Reg_3052(0x50,0x07);
    Wrt_Reg_3052(0x51,0x08);
    Wrt_Reg_3052(0x52,0x09);
    Wrt_Reg_3052(0x53,0x0A);
    Wrt_Reg_3052(0x54,0x55);
    Wrt_Reg_3052(0x55,0xa2);
    Wrt_Reg_3052(0x56,0xa3);
    Wrt_Reg_3052(0x57,0x55);
    Wrt_Reg_3052(0x58,0xa4);
    Wrt_Reg_3052(0x59,0xa5);
    Wrt_Reg_3052(0x60,0x02);
    Wrt_Reg_3052(0x61,0x02);
    Wrt_Reg_3052(0x64,0x00);
    Wrt_Reg_3052(0x65,0x0a);
    Wrt_Reg_3052(0x66,0x0a);
    Wrt_Reg_3052(0x80,0x06);
    Wrt_Reg_3052(0x81,0x11);
    Wrt_Reg_3052(0x82,0x07);
    Wrt_Reg_3052(0x83,0x03);
    Wrt_Reg_3052(0x84,0x1f);
    Wrt_Reg_3052(0x85,0x1e);
    Wrt_Reg_3052(0x86,0x01);
    Wrt_Reg_3052(0x87,0x1e);
    Wrt_Reg_3052(0x88,0x09);
    Wrt_Reg_3052(0x89,0x0b);
    Wrt_Reg_3052(0x8A,0x0d);
    Wrt_Reg_3052(0x8B,0x0f);
    Wrt_Reg_3052(0x96,0x06);
    Wrt_Reg_3052(0x97,0x11);
    Wrt_Reg_3052(0x98,0x07);
    Wrt_Reg_3052(0x99,0x04);
    Wrt_Reg_3052(0x9a,0x1f);
    Wrt_Reg_3052(0x9b,0x1e);
    Wrt_Reg_3052(0x9c,0x02);
    Wrt_Reg_3052(0x9d,0x1e);
    Wrt_Reg_3052(0x9e,0x0a);
    Wrt_Reg_3052(0x9f,0x0c);
    Wrt_Reg_3052(0xA0,0x0e);
    Wrt_Reg_3052(0xA1,0x10);
    Wrt_Reg_3052(0xFF,0x30);
    Wrt_Reg_3052(0xFF,0x52);
    Wrt_Reg_3052(0xFF,0x02);
    Wrt_Reg_3052(0x28,0x0B);
    Wrt_Reg_3052(0x29,0x07);
    Wrt_Reg_3052(0x2A,0x81);
    Wrt_Reg_3052(0x01,0x01);
    Wrt_Reg_3052(0x02,0xDA);
    Wrt_Reg_3052(0x03,0xBA);
    Wrt_Reg_3052(0x04,0xA8);
    Wrt_Reg_3052(0x05,0x9A);
    Wrt_Reg_3052(0x06,0x70);
    Wrt_Reg_3052(0x07,0xFF);
    Wrt_Reg_3052(0x08,0x91);
    Wrt_Reg_3052(0x09,0x90);
    Wrt_Reg_3052(0x0A,0xFF);
    Wrt_Reg_3052(0x0B,0x8F);
    Wrt_Reg_3052(0x0C,0x60);
    Wrt_Reg_3052(0x0D,0x58);
    Wrt_Reg_3052(0x0E,0x48);
    Wrt_Reg_3052(0x0F,0x38);
    Wrt_Reg_3052(0x10,0x2B);

    Wrt_Reg_3052(0xFF,0x30);
    Wrt_Reg_3052(0xFF,0x52);
    Wrt_Reg_3052(0xFF,0x00);
    Wrt_Reg_3052(0x36,0x0A);
    Wrt_Reg_3052(0x3A,0x70);

    Wrt_Reg_3052_1(0x11);
    SpiDelay(200);
    Wrt_Reg_3052_1(0x29);
    SpiDelay(30);
    sysprintf("NV3052_RGB_init done\n");
}

static void vpostIntHandler(void)
{
    /* clear VPOST interrupt state */
    uint32_t uintstatus;

    uintstatus = inpw(REG_LCM_INT_CS);
    if (uintstatus & VPOSTB_DISP_F_INT)
        outpw(REG_LCM_INT_CS,inpw(REG_LCM_INT_CS) | VPOSTB_DISP_F_INT);
    else if (uintstatus & VPOSTB_BUS_ERROR_INT)
        outpw(REG_LCM_INT_CS,inpw(REG_LCM_INT_CS) | VPOSTB_BUS_ERROR_INT);
}

int32_t main(void)
{
    uint8_t *u8FrameBufPtr;
    int  i;

    outpw(REG_CLK_HCLKEN, 0x2000527);
    outpw(REG_CLK_PCLKEN0, inpw(REG_CLK_PCLKEN0) | 0x8);  /* Enable GPIO clock */
    outpw(REG_CLK_PCLKEN1, 0);

    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    sysInitializeUART();

    sysprintf("\n\nLCD ST70IPS2801424-N-DE120 demo ==>\n");

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

    // LCD clock is selected from UPLL and divide to 30MHz
    outpw(REG_CLK_DIVCTL1, (inpw(REG_CLK_DIVCTL1) & ~0xff1f) | 0x918);

    // Init LCD interface for ST70IPS2801424 LCD module
    vpostLCMInit(DIS_PANEL_ST70IPS2801424);

    // Set scale to 1:1
    vpostVAScalingCtrl(1, 0, 1, 0, VA_SCALE_INTERPOLATION);

    NV3052_RGB_init();

    // Set display color depth
#ifdef DISPLAY_RGB888
    vpostSetVASrc(VA_SRC_RGB888);
#endif

    // Enable LCD interrupt
    outpw(REG_LCM_DCCS, inpw(REG_LCM_DCCS) | VPOSTB_DISP_INT_EN);
    outpw(REG_LCM_INT_CS, inpw(REG_LCM_INT_CS) | VPOSTB_DISP_F_EN);

    sysInstallISR(HIGH_LEVEL_SENSITIVE | IRQ_LEVEL_1, LCD_IRQn, (PVOID)vpostIntHandler);
    sysSetLocalInterrupt(ENABLE_IRQ);
    sysEnableInterrupt(LCD_IRQn);

    vpostSetFrameBuffer(_frame_buffer);
    u8FrameBufPtr = (uint8_t *)((uint32_t)_frame_buffer | 0x80000000);

    // Prepare image
#ifdef DISPLAY_RGB888
    memcpy((void *)u8FrameBufPtr, (void *)&video_img[0], 280*1424*4);
#endif

    // Prepare hardware cursor image (color bar)
    for (i=0; i<16; i++) {
        u32CursorBuf[i] = 0x00;
        u32CursorBuf[i+16*1] = 0x55555555;
        u32CursorBuf[i+16*2] = 0xaaaaaaaa;
        u32CursorBuf[i+16*3] = 0xffffffff;
        u32CursorBuf[i+16*4] = 0x00;
        u32CursorBuf[i+16*5] = 0x55555555;
        u32CursorBuf[i+16*6] = 0xaaaaaaaa;
        u32CursorBuf[i+16*7] = 0xffffffff;
        u32CursorBuf[i+16*8] = 0x00;
        u32CursorBuf[i+16*9] = 0x55555555;
        u32CursorBuf[i+16*10] = 0xaaaaaaaa;
        u32CursorBuf[i+16*11] = 0xffffffff;
        u32CursorBuf[i+16*12] = 0x00;
        u32CursorBuf[i+16*13] = 0x55555555;
        u32CursorBuf[i+16*14] = 0xaaaaaaaa;
        u32CursorBuf[i+16*15] = 0xffffffff;
        u32CursorBuf[i+16*16] = 0x00;
        u32CursorBuf[i+16*17] = 0x55555555;
        u32CursorBuf[i+16*18] = 0xaaaaaaaa;
        u32CursorBuf[i+16*19] = 0xffffffff;
        u32CursorBuf[i+16*20] = 0x00;
        u32CursorBuf[i+16*21] = 0x55555555;
        u32CursorBuf[i+16*22] = 0xaaaaaaaa;
        u32CursorBuf[i+16*23] = 0xffffffff;
        u32CursorBuf[i+16*24] = 0x00;
        u32CursorBuf[i+16*25] = 0x55555555;
        u32CursorBuf[i+16*26] = 0xaaaaaaaa;
        u32CursorBuf[i+16*27] = 0xffffffff;
        u32CursorBuf[i+16*28] = 0x00;
        u32CursorBuf[i+16*29] = 0x55555555;
        u32CursorBuf[i+16*30] = 0xaaaaaaaa;
        u32CursorBuf[i+16*31] = 0xffffffff;
    }

    // Start video and OSD
    vpostVAStartTrigger();

    // Start hardware cursor
    // vpostHCInit(u32CursorBuf, HC_MODE0);
    // Set hardware cursor position
    // vpostHCPosCtrl(50, 50);

    sysprintf("REG_LCM_DCCS = 0x%x\n", inpw(REG_LCM_DCCS));
    sysprintf("REG_LCM_CRTC_SIZE = 0x%x\n", inpw(REG_LCM_CRTC_SIZE));
    sysprintf("REG_LCM_CRTC_DEND = 0x%x\n", inpw(REG_LCM_CRTC_DEND));
    sysprintf("REG_LCM_CRTC_HR = 0x%x\n", inpw(REG_LCM_CRTC_HR));
    sysprintf("REG_LCM_CRTC_HSYNC = 0x%x\n", inpw(REG_LCM_CRTC_HSYNC));
    sysprintf("REG_LCM_CRTC_VR = 0x%x\n", inpw(REG_LCM_CRTC_VR));
    sysprintf("REG_LCM_VA_BADDR0 = 0x%x\n", inpw(REG_LCM_VA_BADDR0));
    sysprintf("REG_LCM_VA_BADDR1 = 0x%x\n", inpw(REG_LCM_VA_BADDR1));
    sysprintf("REG_LCM_VA_FBCTRL = 0x%x\n", inpw(REG_LCM_VA_FBCTRL));
    sysprintf("REG_LCM_VA_SCALE = 0x%x\n", inpw(REG_LCM_VA_SCALE));
    sysprintf("REG_LCM_VA_WIN = 0x%x\n", inpw(REG_LCM_VA_WIN));
    sysprintf("REG_LCM_VA_STUFF = 0x%x\n", inpw(REG_LCM_VA_STUFF));

    while(1);
}
