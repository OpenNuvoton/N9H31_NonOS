/**************************************************************************//**
* @file     i2c.h
* @brief    N9H31 I2C driver header file
*
* @note
* SPDX-License-Identifier: Apache-2.0
* Copyright (C) 2018 Nuvoton Technology Corp. All rights reserved.
*****************************************************************************/
 
#ifndef _I2C_H_
#define _I2C_H_

#ifdef __cplusplus
extern "C"
{
#endif


/** @addtogroup N9H31_Device_Driver N9H31 Device Driver
  @{
*/

/** @addtogroup N9H31_I2C_Driver I2C Driver
  @{
*/

/** @addtogroup N9H31_I2C_EXPORTED_CONSTANTS I2C Exported Constants
  @{
*/
/// @cond HIDDEN_SYMBOLS

/*-----------------------------------------*/
/* marco, type and constant definitions    */
/*-----------------------------------------*/
#define I2C_NUMBER              2
#define I2C_MAX_BUF_LEN         450

#define I2C_INPUT_CLOCK         75000           /* Unit: kHz */

/*-----------------------------------------*/
/* global interface variables declarations */
/*-----------------------------------------*/
/* 
    bit map in CMDR 
*/
#define I2C_CMD_START           0x10
#define I2C_CMD_STOP            0x08
#define I2C_CMD_READ            0x04
#define I2C_CMD_WRITE           0x02
#define I2C_CMD_NACK            0x01

/* 
    for transfer use 
*/
#define I2C_WRITE               0x00
#define I2C_READ                0x01

#define I2C_STATE_NOP           0x00
#define I2C_STATE_READ          0x01
#define I2C_STATE_WRITE         0x02
#define I2C_STATE_PROBE         0x03

/* 
    i2c register offset 
*/
#define     I2C_CSR     (0x00)  /*!< Control and Status Register */
#define     I2C_DIVIDER (0x04)  /*!< Clock Prescale Register */
#define     I2C_CMDR    (0x08)  /*!< Command Register */
#define     I2C_SWR     (0x0C)  /*!< Software Mode Control Register */
#define     I2C_RxR     (0x10)  /*!< Data Receive Register */
#define     I2C_TxR     (0x14)  /*!< Data Transmit Register */

/// @endcond HIDDEN_SYMBOLS

/*
    ioctl commands 
*/
#define I2C_IOC_SET_DEV_ADDRESS     0  /*!< Set device slave address */
#define I2C_IOC_SET_SUB_ADDRESS     1  /*!< Set sub address */
#define I2C_IOC_SET_SPEED           2  /*!< Set I2C interface speed */

/* 
    error code 
*/
#define I2C_ERR_ID                  0xFFFF1100           /*!< I2C library ID                  */
#define I2C_ERR_NOERROR             (0x00)               /*!< No error                        */
#define I2C_ERR_LOSTARBITRATION     (0x01 | I2C_ERR_ID)  /*!< Arbitration lost error          */
#define I2C_ERR_BUSBUSY             (0x02 | I2C_ERR_ID)  /*!< Bus busy error                  */
#define I2C_ERR_NACK                (0x03 | I2C_ERR_ID)  /*!< data transfer error             */
#define I2C_ERR_SLAVENACK           (0x04 | I2C_ERR_ID)  /*!< slave not respond after address */
#define I2C_ERR_NODEV               (0x05 | I2C_ERR_ID)  /*!< Wrong device                    */
#define I2C_ERR_BUSY                (0x06 | I2C_ERR_ID)  /*!< Device busy                     */
#define I2C_ERR_IO                  (0x07 | I2C_ERR_ID)  /*!< Interface not open              */
#define I2C_ERR_NOTTY               (0x08 | I2C_ERR_ID)  /*!< Command not support             */

/*@}*/ /* end of group N9H31_I2C_EXPORTED_CONSTANTS */

/** @addtogroup N9H31_I2C_EXPORTED_FUNCTIONS I2C Exported Functions
  @{
*/

/*-----------------------------------------*/
/* interface function declarations         */
/*-----------------------------------------*/
int32_t i2cInit(int32_t fd);
int32_t i2cOpen(void* param);
int32_t i2cClose(int32_t fd);
int32_t i2cRead(int32_t fd, uint8_t* buf, uint32_t len);
int32_t i2cWrite(int32_t fd, uint8_t* buf, uint32_t len);
int32_t i2cIoctl(int32_t fd, uint32_t cmd, uint32_t arg0, uint32_t arg1);
int32_t i2cExit(void);

/*@}*/ /* end of group N9H31_I2C_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group N9H31_I2C_Driver */

/*@}*/ /* end of group N9H31_Device_Driver */

#ifdef __cplusplus
}
#endif

#endif //__I2C_H__

/*** (C) COPYRIGHT 2018 Nuvoton Technology Corp. ***/
