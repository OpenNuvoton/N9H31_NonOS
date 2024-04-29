/**************************************************************************//**
 * @file     nandlib.h
 * @version  V1.00
 * @brief    NAND library header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#include "N9H31.h"
#include "sys.h"
#ifndef __NANDLIB_H__
#define __NANDLIB_H__


#define NANDLIB_OK                    0                     /*!<NANDLIB no error \hideinitializer */
#define NANDLIB_ERR                   0xFFFFC000            /*!<NANDLIB error base \hideinitializer */

/* NANDLIB ERRORs */
#define NANDLIB_MEMORY_OUT            (NANDLIB_ERR|0x5)     /*!<NANDLIB out of memory \hideinitializer */
#define NANDLIB_FORMAT                (NANDLIB_ERR|0x10)    /*!<NANDLIB incorrect format \hideinitializer */
#define NANDLIB_BLOCK_OUT             (NANDLIB_ERR|0x20)    /*!<NANDLIB out of NAND block \hideinitializer */
#define NANDLIB_P2LN_SYNC             (NANDLIB_ERR|0x25)    /*!<NANDLIB physical to logical table sync error \hideinitializer */
#define NANDLIB_IO_ERR                (NANDLIB_ERR|0x30)    /*!<NANDLIB I/O error \hideinitializer */
#define NANDLIB_UNKNOW_ID             (NANDLIB_ERR|0x42)    /*!<NANDLIB un-recognized NAND flash \hideinitializer */

#define OP_CMD_LEN                  16  /*!<OP command length I/O error \hideinitializer */ // max is gnop_relink_t, 16 bytes

/**
  * @brief  Physical to logical block mapping store in SRAM
  */
typedef struct p2lm_t {
    uint16_t  lba;            /*!<  logical block address                 */
}  P2LM_T;

/**
  * @brief  Logical to physical block mapping store in SRAM
  */
typedef struct l2pm_t {
    uint16_t  pba;            /*!<  physical block address                */
}  L2PM_T;



typedef struct ndrv_t NDRV_T;
/**
  * @brief  Structue holds NAND disk information
  */
typedef struct ndisk_t {
    int16_t     nPBlockCount;       /*!< Physical block count                  */
    int16_t     nPagePerBlock;      /*!< pages per block                       */
    int16_t     nLBlockCount;       /*!< Logical block count                   */
    int16_t     nPageSize;          /*!< NAND flash page size                  */
    NDRV_T      *driver;            /*!< NAND driver to work on this NAND disk */
    P2LM_T      *p2lm;              /*!< Physical to logical mapping table     */
    L2PM_T      *l2pm;              /*!< Logical to physical mapping table     */
    uint8_t     *dp_tbl;            /*!< Dirty page bit map                    */
    uint16_t    db_idx;             /*!< Data block search index               */
    uint16_t    p2ln_block;         /*!< P2LN block No.                        */
    uint16_t    op_block;           /*!< OP block No.                          */
    uint16_t    op_offset;          /*!< operation index                       */
    uint8_t     last_op[OP_CMD_LEN];    /*!< the last op code in op table, large enough to hold 1 op history */
}NDISK_T;

/**
  * @brief  Structue holds NAND driver information
  */
typedef struct ndrv_t {
#if 0
    int16_t     vid;                /*!< vendor id              */
    int16_t     did;                /*!< device id              */
    int16_t     mcycle;             /*!< multi cycle            */
    int16_t     pblock;             /*!< physical block count   */
    int16_t     oobsize;            /*!< redundant area size    */
    int16_t     eccbyte;            /*!< ECC byte per 512 byte data */
#endif
    int16_t     ppb;                /*!< page per block         */
    int16_t     pagesize;           /*!< page size              */
    int16_t     lblock;             /*!< logical block count     */
    int16_t     ecc;                /*!< correctable error bit  */

    /**
      * @brief  NAND driver init function
      * @param[in] ptNDisk A pointer to NDISK_T holds NAND disk information
      * @retval 0 Success
      * @retval Otherwise failed
      */
    int32_t     (*init)(NDISK_T *NDInfo);
    /**
      * @brief  NAND driver deinit function
      * @retval 0 Success
      * @retval Otherwise Failed
      */
    int32_t     (*deinit)(void);
    /**
      * @brief  Read a page
      * @param[in] pba Block number
      * @param[in] page Page number
      * @param[in] buff Buffer to hold read data
      * @retval 0 Success
      * @retval >0 Number of error bit corrected
      * @retval <0 Failed
      */
    int32_t     (*pread)(int32_t pba, int32_t page, uint8_t *buff);
    /**
      * @brief  Program a page
      * @param[in] pba Block number
      * @param[in] page Page number
      * @param[in] buff Buffer to hold program data
      * @retval 0 Success
      * @retval Otherwise Failed
      */
    int32_t     (*pwrite)(int32_t pba, int32_t page, uint8_t *buff);
    /**
      * @brief  Check if a page is dirty or not
      * @param[in] pba Block number
      * @param[in] nPageNo Page number
      * @retval TRUE Page is dirty
      * @retval FALSE Page is not dirty
      */
    int32_t     (*isDirtyPage)(int32_t pba, int32_t nPageNo);
    /**
      * @brief  Check if a block is bad or not
      * @param[in] pba Block number
      * @retval TRUE Inquired block is a bad block
      * @retval FALSE Inquired block is not a bad block
      */
    int32_t     (*isValidBlock)(int32_t pba);
    /**
      * @brief  Erase a block
      * @param[in] pba Block number
      * @retval 0 Success
      * @retval Otherwise Failed
      */
    int32_t     (*erase)(int32_t pba);
    /**
      * @brief  Mark a bad block
      * @param[in] uBlock Block number
      * @retval 0 Success
      * @retval Otherwise Failed
      */
    int32_t     (*markBad)(int32_t uBlock);
}NDRV_T ;


/**
  * @brief Initialize NAND library.
  * @param[in] ptNDisk A pointer to NDISK_T holds NAND disk information
  * @param[in] ptNDriver A pointer to NDRV_T holds NAND driver information
  * @retval NANDLIB_UNKNOW_ID Unrecognized NAND flash
  * @retval NANDLIB_IO_ERR NAND flash I/O error occur
  * @retval NANDLIB_MEMORY_OUT System out of memory. NAND library allocates 
  *                            (3 * block count + block count * page per block / 8 + page size + oob size) bytes in total
  * @retval NANDLIB_OK NAND library init successfully
  * @note If the data format stores on NAND disk is unrecognized, this API will erase and
  *       format the  NAND flash
  */
int32_t NANDLIB_Init(NDISK_T *ptNDisk, NDRV_T *ptNDriver);
/**
  * @brief De-initialize NAND library.
  * @param[in] ptNDisk A pointer to NDISK_T holds NAND disk information
  * @return None
  * @details If API free the memory allocated in NANDLIB_Init()
  */
void NANDLIB_DeInit(NDISK_T *ptNDisk);
/**
  * @brief Read data from NAND flash
  * @param[in] ptNDisk A pointer to NDISK_T holds NAND disk information
  * @param[in] u32SectorNo Starting sector to read from
  * @param[in] n32SectorCnt Number of sector to read
  * @param[in] pu8Buff A buffer pointer to hold read data
  * @retval NANDLIB_IO_ERR NAND flash I/O error
  * @retval NANDLIB_OK Read success
  * @note Sector size must be equal with page size
  */
int32_t NANDLIB_Read(NDISK_T *ptNDisk, uint32_t u32SectorNo, int32_t n32SectorCnt, uint8_t *pu8Buff);
/**
  * @brief Program data to NAND flash
  * @param[in] ptNDisk A pointer to NDISK_T holds NAND disk information
  * @param[in] u32SectorNo Starting sector to program to
  * @param[in] n32SectorCnt Number of sector to program
  * @param[in] pu8Buff A buffer pointer to hold program data
  * @retval NANDLIB_BLOCK_OUT NAND flash is full
  * @retval NANDLIB_IO_ERR NAND flash I/O error
  * @retval NANDLIB_OK Read success
  * @note Sector size must be equal with page size
  */
int32_t NANDLIB_Write(NDISK_T *ptNDisk, uint32_t u32SectorNo, int32_t n32SectorCnt, uint8_t *pu8Buff);


#endif  /* __NANDLIB_H__ */
