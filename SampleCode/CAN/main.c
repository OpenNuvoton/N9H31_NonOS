/**************************************************************************//**
 * @file     main.c
 * @version  V1.00
 * $Date: 15/06/11 2:44p $
 * @brief    N9H31 Driver Sample Code
 *
 * @note
 * Copyright (C) 2015 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/
#include "N9H31.h"
#include "sys.h"
#include "can.h"

#define PLLCON_SETTING      SYSCLK_PLLCON_50MHz_XTAL
#define PLL_CLOCK           50000000

//extern char GetChar(void);

/*---------------------------------------------------------------------------------------------------------*/
/* Global variables                                                                                        */
/*---------------------------------------------------------------------------------------------------------*/
STR_CANMSG_T rrMsg;
uint32_t TX_FLAG = 0;

void CAN_ShowMsg(STR_CANMSG_T* Msg);

/*---------------------------------------------------------------------------------------------------------*/
/* ISR to handle CAN interrupt event                                                            */
/*---------------------------------------------------------------------------------------------------------*/
void CAN_MsgInterrupt(UINT32 uCAN, uint32_t u32IIDR)
{
    if(CAN_Receive(uCAN, (u32IIDR-1),&rrMsg))
    {
        sysprintf("Msg-%d INT and Callback \n", (u32IIDR-1));
        CAN_ShowMsg(&rrMsg);
    }
}


/**
  * @brief  CAN0_IRQ Handler.
  * @param  None.
  * @return None.
  */
void CAN0_IRQHandler(void)
{
    uint32_t u32IIDRstatus;
    uint32_t u32Status;

    u32Status = inpw(REG_CAN0_STATUS);
    u32IIDRstatus = inpw(REG_CAN0_IIDR);

    if(u32Status & CAN_STATUS_RXOK_Msk)
    {
        outpw(REG_CAN0_STATUS, u32Status & ~CAN_STATUS_RXOK_Msk);  /* Clear Rx Ok status*/

            sysprintf("RX OK INT\n") ;
        }

    if(u32Status & CAN_STATUS_TXOK_Msk)
    {
        outpw(REG_CAN0_STATUS, u32Status & ~CAN_STATUS_TXOK_Msk);  /* Clear Tx Ok status*/
        TX_FLAG = 1;
        sysprintf("TX OK INT\n") ;
    }

    if(u32IIDRstatus == 0x00008000)
    {
        /**************************/
        /* Error Status interrupt */
        /**************************/
        if(u32Status & CAN_STATUS_EWARN_Msk)
        {
            sysprintf("EWARN INT\n") ;
        }

        if(u32Status & CAN_STATUS_BOFF_Msk)
        {
            sysprintf("BOFF INT\n") ;
        }
    }

    if (u32IIDRstatus!=0)
    {
        sysprintf("=> Interrupt Pointer = %d\n", (u32IIDRstatus-1));

        CAN_MsgInterrupt(CAN0, u32IIDRstatus);

        CAN_CLR_INT_PENDING_BIT(CAN0, (u32IIDRstatus-1));      /* Clear Interrupt Pending */

    }

    if(inpw(REG_CAN0_WU_STATUS) == 1)
    {
        sysprintf("Wake up\n");

        outpw(REG_CAN0_WU_STATUS, 0x0);  /* Write '0' to clear */
    }

}

/*----------------------------------------------------------------------------*/
/*  Some description about how to create test environment                     */
/*----------------------------------------------------------------------------*/
void Note_Configure()
{
    sysprintf("\n\n");
    sysprintf("+------------------------------------------------------------------------+\n");
    sysprintf("|  About CAN sample code configure                                       |\n");
    sysprintf("+------------------------------------------------------------------------+\n");
    sysprintf("|   The sample code provide a simple sample code for you study CAN       |\n");
    sysprintf("|   Before execute it, please check description as below                 |\n");
    sysprintf("|                                                                        |\n");
    sysprintf("|   1.CAN0 connect to CAN BUS                                            |\n");
    sysprintf("|   2.Using UART0 as print message port                                  |\n");
    sysprintf("|                                                                        |\n");
    sysprintf("|  |--------|       |-----------|  CANBUS |-----------|       |--------| |\n");
    sysprintf("|  |        |------>|           |<------->|           |<------|        | |\n");
    sysprintf("|  |        |CAN0_TX|   CAN0    |  CAN1_H |   CAN1    |CAN1_TX|        | |\n");
    sysprintf("|  | N9H31  |       |Transceiver|         |Transceiver|       | User's | |\n");
    sysprintf("|  |        |<------|           |<------->|           |------>| Device | |\n");
    sysprintf("|  |        |CAN0_RX|           |  CAN1_L |           |CAN1_RX|        | |\n");
    sysprintf("|  |--------|       |-----------|         |-----------|       |--------| |\n");
    sysprintf("|   |                                                                    |\n");
    sysprintf("|   |                                                                    |\n");
    sysprintf("|   V                                                                    |\n");
    sysprintf("| UART0                                                                  |\n");
    sysprintf("|(print message)                                                         |\n");
    sysprintf("+------------------------------------------------------------------------+\n");
}

/*----------------------------------------------------------------------------*/
/*  Test Function                                                             */
/*----------------------------------------------------------------------------*/
void CAN_ShowMsg(STR_CANMSG_T* Msg)
{
    uint8_t i;
    sysprintf("Read ID=%8X, Type=%s, DLC=%d,Data=",Msg->Id,Msg->IdType?"EXT":"STD",Msg->DLC);
    for(i=0; i<Msg->DLC; i++)
        sysprintf("%02X,",Msg->Data[i]);
    sysprintf("\n\n");
}

/*----------------------------------------------------------------------------*/
/*  Send Tx Msg by Normal Mode Function (With Message RAM)                    */
/*----------------------------------------------------------------------------*/
void Test_NormalMode_Tx(UINT32 uCAN)
{
    STR_CANMSG_T tMsg;
    uint32_t i;

    sysprintf("\nMSG(1).Send STD_ID:0x7FF, Data[07,FF] \n");
    for(i=0; i < 30000; i++); // wait print debug message

    TX_FLAG = 0;

    /* Send a 11-bits message */
    tMsg.FrameType= DATA_FRAME;
    tMsg.IdType   = CAN_STD_ID;
    tMsg.Id       = 0x7FF;
    tMsg.DLC      = 2;
    tMsg.Data[0]  = 7;
    tMsg.Data[1]  = 0xFF;

    if(CAN_Transmit(uCAN, MSG(1),&tMsg) == FALSE)   // Configure Msg RAM and send the Msg in the RAM
    {
        sysprintf("Set Tx Msg Object failed\n");
        return;
    }

    while(TX_FLAG == 0);

    sysprintf("\nMSG(2).Send EXT:0x12345 ,Data[01,23,45] \n");
    for(i=0; i < 30000; i++); // wait print debug message

    TX_FLAG = 0;

    /* Send a 29-bits message */
    tMsg.FrameType= DATA_FRAME;
    tMsg.IdType   = CAN_EXT_ID;
    tMsg.Id       = 0x12345;
    tMsg.DLC      = 3;
    tMsg.Data[0]  = 1;
    tMsg.Data[1]  = 0x23;
    tMsg.Data[2]  = 0x45;

    if(CAN_Transmit(uCAN, MSG(2),&tMsg) == FALSE)
    {
        sysprintf("Set Tx Msg Object failed\n");
        return;
    }

    while(TX_FLAG == 0);

    sysprintf("\nMSG(3).Send EXT:0x7FF01 ,Data[A1,B2,C3,D4] \n");
    for(i=0; i < 30000; i++); // wait print debug message

    TX_FLAG = 0;

    /* Send a data message */
    tMsg.FrameType= DATA_FRAME;
    tMsg.IdType   = CAN_EXT_ID;
    tMsg.Id       = 0x7FF01;
    tMsg.DLC      = 4;
    tMsg.Data[0]  = 0xA1;
    tMsg.Data[1]  = 0xB2;
    tMsg.Data[2]  = 0xC3;
    tMsg.Data[3]  = 0xD4;

    if(CAN_Transmit(uCAN, MSG(3),&tMsg) == FALSE)
    {
        sysprintf("Set Tx Msg Object failed\n");
        return;
    }

    while(TX_FLAG == 0);

    for(i=0; i < 10000; i++);

    sysprintf("Trasmit Done!\nCheck the receive host received data\n\n");

}

/*----------------------------------------------------------------------------*/
/*  Receive Rx Msg by Normal Mode Function (With Message RAM)                    */
/*----------------------------------------------------------------------------*/
void Test_NormalMode_SetRxMsg(UINT32 uCAN)
{
    sysprintf("\n Set Rx Msg Filer: Extended ID '0x12345'");
    sysprintf("\n If a frame with ID 0x12345 is received, the frame information will be displayed \n\n");

    if(CAN_SetRxMsg(uCAN, MSG(0),CAN_EXT_ID, 0x12345) == FALSE)
    {
        sysprintf("Set Rx Msg Object failed\n");
        return;
    }
}

void Test_NormalMode_WaitRxMsg(UINT32 uCAN)
{
    /*Choose one mode to test*/
    /* INT Mode */
    CAN_EnableInt(uCAN, CAN_CON_IE_Msk);

    if(uCAN == CAN0)
    {
        sysInstallISR((IRQ_LEVEL_1 | HIGH_LEVEL_SENSITIVE), CAN0_IRQn, (PVOID)CAN0_IRQHandler);
        sysSetLocalInterrupt(ENABLE_IRQ);                            /* enable CPSR I bit */
        sysEnableInterrupt(CAN0_IRQn);
    }
}

int main()
{
    sysDisableCache();
    sysFlushCache(I_D_CACHE);
    sysEnableCache(CACHE_WRITE_BACK);
    sysInitializeUART();

    // CAN0
    outpw(REG_SYS_GPI_MFPL, (inpw(REG_SYS_GPI_MFPL) & 0xfff00fff) | 0xCC000 ); // GPI_3,GPI_4 // RX, TX

    outpw(REG_CLK_PCLKEN1, (inpw(REG_CLK_PCLKEN1) | (1 << 8)) );

    Note_Configure();

    CAN_Open(CAN0,  500000, CAN_NORMAL_MODE);

    sysprintf("\n");
    sysprintf("+------------------------------------------------------------------ +\n");
    sysprintf("|  Nuvoton CAN BUS DRIVER DEMO                                      |\n");
    sysprintf("+-------------------------------------------------------------------+\n");
    sysprintf("|  Transmit/Receive a message by normal mode                        |\n");
    sysprintf("+-------------------------------------------------------------------+\n");

    sysprintf("Press any key to continue ...\n\n");
    sysGetChar();

    Test_NormalMode_SetRxMsg(CAN0);
    Test_NormalMode_WaitRxMsg(CAN0);

    while(1)
    {
        sysprintf("\n\n Press any key to start TX transmission ...\n");
        sysGetChar();
    Test_NormalMode_Tx(CAN0);
    }

}



