/**************************************************************************//**
 * @file     nand.h
 * @version  V1.00
 * @brief    NuMicro ARM9 FMI NAND driver header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __NAND_H__
#define __NAND_H__

#include "nandlib.h"

/**
    @addtogroup NAND_CONST NAND Bit Field Definition
    Constant Definitions for NAND Controller
@{ */


#define FMI_DMACTL_DMAEN_Pos             (0)                                               /*!< FMI DMACTL: DMAEN Position             */
#define FMI_DMACTL_DMAEN_Msk             (0x1ul << FMI_DMACTL_DMAEN_Pos)                   /*!< FMI DMACTL: DMAEN Mask                 */

#define FMI_DMACTL_DMARST_Pos            (1)                                               /*!< FMI DMACTL: DMARST Position            */
#define FMI_DMACTL_DMARST_Msk            (0x1ul << FMI_DMACTL_DMARST_Pos)                  /*!< FMI DMACTL: DMARST Mask                */

#define FMI_DMACTL_SGEN_Pos              (3)                                               /*!< FMI DMACTL: SGEN Position              */
#define FMI_DMACTL_SGEN_Msk              (0x1ul << FMI_DMACTL_SGEN_Pos)                    /*!< FMI DMACTL: SGEN Mask                  */

#define FMI_DMACTL_DMABUSY_Pos           (9)                                               /*!< FMI DMACTL: DMABUSY Position           */
#define FMI_DMACTL_DMABUSY_Msk           (0x1ul << FMI_DMACTL_DMABUSY_Pos)                 /*!< FMI DMACTL: DMABUSY Mask               */

#define FMI_DMASA_ORDER_Pos              (0)                                               /*!< FMI DMASA: ORDER Position              */
#define FMI_DMASA_ORDER_Msk              (0x1ul << FMI_DMASA_ORDER_Pos)                    /*!< FMI DMASA: ORDER Mask                  */

#define FMI_DMASA_DMASA_Pos              (1)                                               /*!< FMI DMASA: DMASA Position              */
#define FMI_DMASA_DMASA_Msk              (0x7ffffffful << FMI_DMASA_DMASA_Pos)             /*!< FMI DMASA: DMASA Mask                  */

#define FMI_DMABCNT_BCNT_Pos             (0)                                               /*!< FMI DMABCNT: BCNT Position             */
#define FMI_DMABCNT_BCNT_Msk             (0x3fffffful << FMI_DMABCNT_BCNT_Pos)             /*!< FMI DMABCNT: BCNT Mask                 */

#define FMI_DMAINTEN_ABORTIEN_Pos        (0)                                               /*!< FMI DMAINTEN: ABORTIEN Position        */
#define FMI_DMAINTEN_ABORTIEN_Msk        (0x1ul << FMI_DMAINTEN_ABORTIEN_Pos)              /*!< FMI DMAINTEN: ABORTIEN Mask            */

#define FMI_DMAINTEN_WEOTIEN_Pos         (1)                                               /*!< FMI DMAINTEN: WEOTIEN Position         */
#define FMI_DMAINTEN_WEOTIEN_Msk         (0x1ul << FMI_DMAINTEN_WEOTIEN_Pos)               /*!< FMI DMAINTEN: WEOTIEN Mask             */

#define FMI_DMAINTSTS_ABORTIF_Pos        (0)                                               /*!< FMI DMAINTSTS: ABORTIF Position        */
#define FMI_DMAINTSTS_ABORTIF_Msk        (0x1ul << FMI_DMAINTSTS_ABORTIF_Pos)              /*!< FMI DMAINTSTS: ABORTIF Mask            */

#define FMI_DMAINTSTS_WEOTIF_Pos         (1)                                               /*!< FMI DMAINTSTS: WEOTIF Position         */
#define FMI_DMAINTSTS_WEOTIF_Msk         (0x1ul << FMI_DMAINTSTS_WEOTIF_Pos)               /*!< FMI DMAINTSTS: WEOTIF Mask             */

#define FMI_CTL_CTLRST_Pos               (0)                                               /*!< FMI CTL: CTLRST Position               */
#define FMI_CTL_CTLRST_Msk               (0x1ul << FMI_CTL_CTLRST_Pos)                     /*!< FMI CTL: CTLRST Mask                   */

#define FMI_CTL_EMMCEN_Pos               (1)                                               /*!< FMI CTL: EMMCEN Position               */
#define FMI_CTL_EMMCEN_Msk               (0x1ul << FMI_CTL_EMMCEN_Pos)                     /*!< FMI CTL: EMMCEN Mask                   */

#define FMI_CTL_NANDEN_Pos               (3)                                               /*!< FMI CTL: NANDEN Position               */
#define FMI_CTL_NANDEN_Msk               (0x1ul << FMI_CTL_NANDEN_Pos)                     /*!< FMI CTL: NANDEN Mask                   */

#define FMI_INTEN_DTAIEN_Pos             (0)                                               /*!< FMI INTEN: DTAIEN Position             */
#define FMI_INTEN_DTAIEN_Msk             (0x1ul << FMI_INTEN_DTAIEN_Pos)                   /*!< FMI INTEN: DTAIEN Mask                 */

#define FMI_INTSTS_DTAIF_Pos            (0)                                                /*!< FMI INTSTS: DTAIF Position             */
#define FMI_INTSTS_DTAIF_Msk            (0x1ul << FMI_INTSTS_DTAIF_Pos)                    /*!< FMI INTSTS: DTAIF Mask                 */

#define FMI_NANDCTL_SWRST_Pos            (0)                                               /*!< FMI_T::NANDCTL: SWRST Position         */
#define FMI_NANDCTL_SWRST_Msk            (0x1ul << FMI_NANDCTL_SWRST_Pos)                  /*!< FMI_T::NANDCTL: SWRST Mask             */

#define FMI_NANDCTL_DRDEN_Pos            (1)                                               /*!< FMI_T::NANDCTL: DRDEN Position         */
#define FMI_NANDCTL_DRDEN_Msk            (0x1ul << FMI_NANDCTL_DRDEN_Pos)                  /*!< FMI_T::NANDCTL: DRDEN Mask             */

#define FMI_NANDCTL_DWREN_Pos            (2)                                               /*!< FMI_T::NANDCTL: DWREN Position         */
#define FMI_NANDCTL_DWREN_Msk            (0x1ul << FMI_NANDCTL_DWREN_Pos)                  /*!< FMI_T::NANDCTL: DWREN Mask             */

#define FMI_NANDCTL_REDUNREN_Pos         (3)                                               /*!< FMI_T::NANDCTL: REDUNREN Position      */
#define FMI_NANDCTL_REDUNREN_Msk         (0x1ul << FMI_NANDCTL_REDUNREN_Pos)               /*!< FMI_T::NANDCTL: REDUNREN Mask          */

#define FMI_NANDCTL_REDUNAUTOWEN_Pos     (4)                                               /*!< FMI_T::NANDCTL: REDUNAUTOWEN Position  */
#define FMI_NANDCTL_REDUNAUTOWEN_Msk     (0x1ul << FMI_NANDCTL_REDUNAUTOWEN_Pos)           /*!< FMI_T::NANDCTL: REDUNAUTOWEN Mask      */

#define FMI_NANDCTL_PROTREGIONEN_Pos     (5)                                               /*!< FMI_T::NANDCTL: PROTREGIONEN Position  */
#define FMI_NANDCTL_PROTREGIONEN_Msk     (0x1ul << FMI_NANDCTL_PROTREGIONEN_Pos)           /*!< FMI_T::NANDCTL: PROTREGIONEN Mask      */

#define FMI_NANDCTL_ECCCHK_Pos           (7)                                               /*!< FMI_T::NANDCTL: ECCCHK Position        */
#define FMI_NANDCTL_ECCCHK_Msk           (0x1ul << FMI_NANDCTL_ECCCHK_Pos)                 /*!< FMI_T::NANDCTL: ECCCHK Mask            */

#define FMI_NANDCTL_PROT3BEN_Pos         (8)                                               /*!< FMI_T::NANDCTL: PROT3BEN Position      */
#define FMI_NANDCTL_PROT3BEN_Msk         (0x1ul << FMI_NANDCTL_PROT3BEN_Pos)               /*!< FMI_T::NANDCTL: PROT3BEN Mask          */

#define FMI_NANDCTL_SRAMINT_Pos          (9)                                               /*!< FMI_T::NANDCTL: SRAMINT Position       */
#define FMI_NANDCTL_SRAMINT_Msk          (0x1ul << FMI_NANDCTL_SRAMINT_Pos)                /*!< FMI_T::NANDCTL: SRAMINT Mask           */

#define FMI_NANDCTL_PSIZE_Pos            (16)                                              /*!< FMI_T::NANDCTL: PSIZE Position         */
#define FMI_NANDCTL_PSIZE_Msk            (0x3ul << FMI_NANDCTL_PSIZE_Pos)                  /*!< FMI_T::NANDCTL: PSIZE Mask             */

#define FMI_NANDCTL_BCHTSEL_Pos          (18)                                              /*!< FMI_T::NANDCTL: BCHTSEL Position       */
#define FMI_NANDCTL_BCHTSEL_Msk          (0x1ful << FMI_NANDCTL_BCHTSEL_Pos)               /*!< FMI_T::NANDCTL: BCHTSEL Mask           */

#define FMI_NANDCTL_ECCEN_Pos            (23)                                              /*!< FMI_T::NANDCTL: ECCEN Position         */
#define FMI_NANDCTL_ECCEN_Msk            (0x1ul << FMI_NANDCTL_ECCEN_Pos)                  /*!< FMI_T::NANDCTL: ECCEN Mask             */

#define FMI_NANDCTL_CS0_Pos              (25)                                              /*!< FMI_T::NANDCTL: CS0 Position           */
#define FMI_NANDCTL_CS0_Msk              (0x1ul << FMI_NANDCTL_CS0_Pos)                    /*!< FMI_T::NANDCTL: CS0 Mask               */

#define FMI_NANDCTL_CS1_Pos              (26)                                              /*!< FMI_T::NANDCTL: CS1 Position           */
#define FMI_NANDCTL_CS1_Msk              (0x1ul << FMI_NANDCTL_CS1_Pos)                    /*!< FMI_T::NANDCTL: CS1 Mask               */

#define FMI_NANDTMCTL_LOWID_Pos          (0)                                               /*!< FMI_T::NANDTMCTL: LOWID Position       */
#define FMI_NANDTMCTL_LOWID_Msk          (0xfful << FMI_NANDTMCTL_LOWID_Pos)               /*!< FMI_T::NANDTMCTL: LOWID Mask           */

#define FMI_NANDTMCTL_HIWID_Pos          (8)                                               /*!< FMI_T::NANDTMCTL: HIWID Position       */
#define FMI_NANDTMCTL_HIWID_Msk          (0xfful << FMI_NANDTMCTL_HIWID_Pos)               /*!< FMI_T::NANDTMCTL: HIWID Mask           */

#define FMI_NANDTMCTL_CALESH_Pos         (16)                                              /*!< FMI_T::NANDTMCTL: CALESH Position      */
#define FMI_NANDTMCTL_CALESH_Msk         (0x7ful << FMI_NANDTMCTL_CALESH_Pos)              /*!< FMI_T::NANDTMCTL: CALESH Mask          */

#define FMI_NANDINTEN_DMAIE_Pos          (0)                                               /*!< FMI_T::NANDINTEN: DMAIE Position       */
#define FMI_NANDINTEN_DMAIE_Msk          (0x1ul << FMI_NANDINTEN_DMAIE_Pos)                /*!< FMI_T::NANDINTEN: DMAIE Mask           */

#define FMI_NANDINTEN_ECCFLDIE_Pos       (2)                                               /*!< FMI_T::NANDINTEN: ECCFLDIE Position    */
#define FMI_NANDINTEN_ECCFLDIE_Msk       (0x1ul << FMI_NANDINTEN_ECCFLDIE_Pos)             /*!< FMI_T::NANDINTEN: ECCFLDIE Mask        */

#define FMI_NANDINTEN_PROTREGIONWRIE_Pos (3)                                               /*!< FMI_T::NANDINTEN: PROTREGIONWRIE Position */
#define FMI_NANDINTEN_PROTREGIONWRIE_Msk (0x1ul << FMI_NANDINTEN_PROTREGIONWRIE_Pos)       /*!< FMI_T::NANDINTEN: PROTREGIONWRIE Mask     */

#define FMI_NANDINTEN_RB0IE_Pos          (10)                                              /*!< FMI_T::NANDINTEN: RB0IE Position       */
#define FMI_NANDINTEN_RB0IE_Msk          (0x1ul << FMI_NANDINTEN_RB0IE_Pos)                /*!< FMI_T::NANDINTEN: RB0IE Mask           */

#define FMI_NANDINTEN_RB1IE_Pos          (11)                                              /*!< FMI_T::NANDINTEN: RB1IE Position       */
#define FMI_NANDINTEN_RB1IE_Msk          (0x1ul << FMI_NANDINTEN_RB1IE_Pos)                /*!< FMI_T::NANDINTEN: RB1IE Mask           */

#define FMI_NANDINTSTS_DMAIF_Pos         (0)                                               /*!< FMI_T::NANDINTSTS: DMAIF Position      */
#define FMI_NANDINTSTS_DMAIF_Msk         (0x1ul << FMI_NANDINTSTS_DMAIF_Pos)               /*!< FMI_T::NANDINTSTS: DMAIF Mask          */

#define FMI_NANDINTSTS_ECCFLDIF_Pos      (2)                                               /*!< FMI_T::NANDINTSTS: ECCFLDIF Position   */
#define FMI_NANDINTSTS_ECCFLDIF_Msk      (0x1ul << FMI_NANDINTSTS_ECCFLDIF_Pos)            /*!< FMI_T::NANDINTSTS: ECCFLDIF Mask       */

#define FMI_NANDINTSTS_PROTREGIONWRIF_Pos (3)                                              /*!< FMI_T::NANDINTSTS: PROTREGIONWRIF Position */
#define FMI_NANDINTSTS_PROTREGIONWRIF_Msk (0x1ul << FMI_NANDINTSTS_PROTREGIONWRIF_Pos)     /*!< FMI_T::NANDINTSTS: PROTREGIONWRIF Mask     */

#define FMI_NANDINTSTS_RB0IF_Pos         (10)                                              /*!< FMI_T::NANDINTSTS: RB0IF Position      */
#define FMI_NANDINTSTS_RB0IF_Msk         (0x1ul << FMI_NANDINTSTS_RB0IF_Pos)               /*!< FMI_T::NANDINTSTS: RB0IF Mask          */

#define FMI_NANDINTSTS_RB1IF_Pos         (11)                                              /*!< FMI_T::NANDINTSTS: RB1IF Position      */
#define FMI_NANDINTSTS_RB1IF_Msk         (0x1ul << FMI_NANDINTSTS_RB1IF_Pos)               /*!< FMI_T::NANDINTSTS: RB1IF Mask          */

#define FMI_NANDINTSTS_RB0Status_Pos     (18)                                              /*!< FMI_T::NANDINTSTS: RB0Status Position  */
#define FMI_NANDINTSTS_RB0Status_Msk     (0x1ul << FMI_NANDINTSTS_RB0Status_Pos)           /*!< FMI_T::NANDINTSTS: RB0Status Mask      */

#define FMI_NANDINTSTS_RB1Status_Pos     (19)                                              /*!< FMI_T::NANDINTSTS: RB1Status Position  */
#define FMI_NANDINTSTS_RB1Status_Msk     (0x1ul << FMI_NANDINTSTS_RB1Status_Pos)           /*!< FMI_T::NANDINTSTS: RB1Status Mask      */

#define FMI_NANDCMD_COMMAND_Pos          (0)                                               /*!< FMI_T::NANDCMD: COMMAND Position       */
#define FMI_NANDCMD_COMMAND_Msk          (0xfful << FMI_NANDCMD_COMMAND_Pos)               /*!< FMI_T::NANDCMD: COMMAND Mask           */

#define FMI_NANDADDR_ADDRESS_Pos         (0)                                               /*!< FMI_T::NANDADDR: ADDRESS Position      */
#define FMI_NANDADDR_ADDRESS_Msk         (0xfful << FMI_NANDADDR_ADDRESS_Pos)              /*!< FMI_T::NANDADDR: ADDRESS Mask          */

#define FMI_NANDADDR_EOA_Pos             (31)                                              /*!< FMI_T::NANDADDR: EOA Position          */
#define FMI_NANDADDR_EOA_Msk             (0x1ul << FMI_NANDADDR_EOA_Pos)                   /*!< FMI_T::NANDADDR: EOA Mask              */

#define FMI_NANDDATA_DATA_Pos            (0)                                               /*!< FMI_T::NANDDATA: DATA Position         */
#define FMI_NANDDATA_DATA_Msk            (0xfful << FMI_NANDDATA_DATA_Pos)                 /*!< FMI_T::NANDDATA: DATA Mask             */

#define FMI_NANDRACTL_RA128EN_Pos        (0)                                               /*!< FMI_T::NANDRACTL: RA128EN Position     */
#define FMI_NANDRACTL_RA128EN_Msk        (0x1fful << FMI_NANDRACTL_RA128EN_Pos)            /*!< FMI_T::NANDRACTL: RA128EN Mask         */

#define FMI_NANDRACTL_MECC_Pos           (16)                                              /*!< FMI_T::NANDRACTL: MECC Position        */
#define FMI_NANDRACTL_MECC_Msk           (0xfffful << FMI_NANDRACTL_MECC_Pos)              /*!< FMI_T::NANDRACTL: MECC Mask            */

#define FMI_NANDECTL_WP_Pos              (0)                                               /*!< FMI_T::NANDECTL: WP Position           */
#define FMI_NANDECTL_WP_Msk              (0x1ul << FMI_NANDECTL_WP_Pos)                    /*!< FMI_T::NANDECTL: WP Mask               */

#define FMI_NANDECCES_F1STAT_Pos         (0)                                               /*!< FMI_T::NANDECCES: F1STAT Position     */
#define FMI_NANDECCES_F1STAT_Msk         (0x3ul << FMI_NANDECCES_F1STAT_Pos)               /*!< FMI_T::NANDECCES: F1STAT Mask         */

#define FMI_NANDECCES_F1ECNT_Pos         (2)                                               /*!< FMI_T::NANDECCES: F1ECNT Position     */
#define FMI_NANDECCES_F1ECNT_Msk         (0x1ful << FMI_NANDECCES_F1ECNT_Pos)              /*!< FMI_T::NANDECCES: F1ECNT Mask         */

#define FMI_NANDECCES_F2STAT_Pos         (8)                                               /*!< FMI_T::NANDECCES: F2STAT Position     */
#define FMI_NANDECCES_F2STAT_Msk         (0x3ul << FMI_NANDECCES_F2STAT_Pos)               /*!< FMI_T::NANDECCES: F2STAT Mask         */

#define FMI_NANDECCES_F2ECNT_Pos         (10)                                              /*!< FMI_T::NANDECCES: F2ECNT Position     */
#define FMI_NANDECCES_F2ECNT_Msk         (0x1ful << FMI_NANDECCES_F2ECNT_Pos)              /*!< FMI_T::NANDECCES: F2ECNT Mask         */

#define FMI_NANDECCES_F3STAT_Pos         (16)                                              /*!< FMI_T::NANDECCES: F3STAT Position     */
#define FMI_NANDECCES_F3STAT_Msk         (0x3ul << FMI_NANDECCES_F3STAT_Pos)               /*!< FMI_T::NANDECCES: F3STAT Mask         */

#define FMI_NANDECCES_F3ECNT_Pos         (18)                                              /*!< FMI_T::NANDECCES: F3ECNT Position     */
#define FMI_NANDECCES_F3ECNT_Msk         (0x1ful << FMI_NANDECCES_F3ECNT_Pos)              /*!< FMI_T::NANDECCES: F3ECNT Mask         */

#define FMI_NANDECCES_F4STAT_Pos         (24)                                              /*!< FMI_T::NANDECCES: F4STAT Position     */
#define FMI_NANDECCES_F4STAT_Msk         (0x3ul << FMI_NANDECCES_F4STAT_Pos)               /*!< FMI_T::NANDECCES: F4STAT Mask         */

#define FMI_NANDECCES_F4ECNT_Pos         (26)                                              /*!< FMI_T::NANDECCES: F4ECNT Position     */
#define FMI_NANDECCES_F4ECNT_Msk         (0x1ful << FMI_NANDECCES_F4ECNT_Pos)              /*!< FMI_T::NANDECCES: F4ECNT Mask         */

#define FMI_NANDRA_Data_Pos              (0)                                               /*!< FMI_T::NANDRA: Data Position          */
#define FMI_NANDRA_Data_Msk              (0xfffffffful << FMI_NANDRA_Data_Pos)             /*!< FMI_T::NANDRA: Data Mask              */


/**@}*/ /* NAND_CONST */


/** @addtogroup Standard_Driver Standard Driver
  @{
*/

/** @addtogroup NAND_Driver NAND Driver
  @{
*/

/** @addtogroup NAND_EXPORTED_CONSTANTS NAND Exported Constants
  @{
*/
/*---------------------------------------------------------------------------------------------------------*/
/*  NAND BCH Constant Definitions                                                                          */
/*---------------------------------------------------------------------------------------------------------*/
#define NAND_BCH_T15     0x00400000
#define NAND_BCH_T12     0x00200000
#define NAND_BCH_T8      0x00100000
#define NAND_BCH_T4      0x00080000
#define NAND_BCH_T24     0x00040000

#define NAND_TYPE_SLC		0x01
#define NAND_TYPE_MLC		0x00

#define NAND_PAGE_2KB		2048
#define NAND_PAGE_4KB		4096
#define NAND_PAGE_8KB		8192

/*-----------------------------------------------------------------------------
 * Define some constants for BCH
 *---------------------------------------------------------------------------*/
// define the total padding bytes for 512/1024 data segment
#define BCH_PADDING_LEN_512     32
#define BCH_PADDING_LEN_1024    64
// define the BCH parity code lenght for 512 bytes data pattern
#define BCH_PARITY_LEN_T4       8
#define BCH_PARITY_LEN_T8       15
#define BCH_PARITY_LEN_T12      23
#define BCH_PARITY_LEN_T15      29
// define the BCH parity code lenght for 1024 bytes data pattern
#define BCH_PARITY_LEN_T24      45

#define NAND_EXTRA_512			16
#define NAND_EXTRA_2K			64
#define NAND_EXTRA_4K			128
#define NAND_EXTRA_8K			376


/*@}*/ /* end of group NAND_EXPORTED_CONSTANTS */


/** @addtogroup NAND_EXPORTED_FUNCTIONS NAND Exported Functions
  @{
*/


/*@}*/ /* end of group NAND_EXPORTED_FUNCTIONS */

/*@}*/ /* end of group NAND_Driver */

/*@}*/ /* end of group Standard_Driver */


/** @addtogroup NAND_EXPORTED_TYPEDEF NAND Exported Type Defines
  @{
*/
/** \brief  Structure type of NAND information.
 */
typedef struct nand_info_t
{
	uint32_t uSectorPerFlash;
	uint32_t uBlockPerFlash;
	uint32_t uPagePerBlock;
	uint32_t uSectorPerBlock;
	uint32_t nPageSize;
	uint32_t uBadBlockCount;
	uint32_t uLibStartBlock;    // NANDLib start block, reserve system area
	uint32_t uNandECC;
	uint32_t uSpareSize;
	uint8_t  bIsMulticycle;
	uint8_t  bIsMLCNand;
	uint8_t  bIsCheckECC;
} NAND_INFO_T;

/*@}*/ /* end of group NAND_EXPORTED_TYPEDEF */

/// @cond HIDDEN_SYMBOLS
extern NAND_INFO_T tNAND;

/// @endcond HIDDEN_SYMBOLS


int32_t NAND_Init(NDISK_T *NDInfo);
int32_t NAND_ReadPage(int pba, int page, uint8_t *buff);
int32_t NAND_WritePage(int pba, int page, uint8_t *buff);
int32_t NAND_IsDirtyPage(int32_t pba, int32_t nPageNo);
int32_t NAND_IsValidBlock(int32_t pba);
int32_t NAND_EraseBlock(int32_t pba);
int32_t NAND_MarkBadBlock(int32_t uBlock);


#endif /* __NAND_H__ */

