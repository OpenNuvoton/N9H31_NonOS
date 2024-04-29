/**************************************************************************//**
 * @file     nandlib_global.c
 * @version  V1.00
 * @brief    NAND library internal header file
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2024 Nuvoton Technology Corp. All rights reserved.
 *****************************************************************************/
#ifndef __NANDLIB_GLOBAL_H__
#define __NANDLIB_GLOBAL_H__
/** @cond HIDDEN_SYMBOLS */

//#define DBG_MSG       sysprintf
#define DBG_MSG(...)

// DBG_INFO() should show minimum information for RD under development stage.
//      For example, NANDLIB version, fetal error, critical check point, and so on.
#define DBG_INFO        sysprintf

#define FREE_BLOCK          0xFFFF
#define BAD_BLOCK           0xFFF0
#define OP_BLOCK            0xFFAA
#define L2PN_BLOCK          0xFF55
#define P2LN_BLOCK          0xFF55

/* global string define */
#define P2LN_INFO_MAGIC         0x50544F4C  //"PTOL"
#define P2LN_INFO_VERSION       0x01000000  //"1.00"

#define P2LN_INFO_T     struct p2ln_info_t

struct p2ln_info_t {
    uint32_t magic;          /* 0x50544F4C, PTOL                      */
    uint32_t version;        /* 0x01000000                            */
    uint16_t op_block;       /* OP block address                      */
    uint16_t old_op_block;   /* Old OP block address                  */
    uint16_t old_p2ln;       /* old P2LN block address                */
    uint32_t block;          /* block count                           */
};


/*-------------------------------------------------------------------*/
/* Operation history                                                 */
/*-------------------------------------------------------------------*/
#define OP_LINK         'L'//"LINK"
#define OP_RELINK       'R'//"RELINK"
#define OP_P2LN         'P'//"P2LN"

#define GNOP_LINK_T     struct gnop_link_t
#define GNOP_RELINK_T   struct gnop_relink_t
#define GNOP_P2LN_T     struct gnop_p2ln_t

struct gnop_link_t {
    uint32_t  op;          /* 'L' */
    uint16_t  lba;
    uint16_t  pba;
} ;


struct gnop_relink_t {
    uint32_t  op;          /* 'R' */
    uint16_t  lba;
    uint16_t  old_pba;
    uint16_t  new_pba;
    uint16_t  start_page;
    uint16_t  page_cnt;
    uint16_t  check_mark;
} ;


struct gnop_p2ln_t {
    uint32_t op;          /* 'P' */
    uint16_t old_p2ln;
    uint16_t old_op;
    uint16_t new_p2ln;
    uint16_t new_op;
} ;


/*-------------------------------------------*/
/* global variable extern                    */
/*-------------------------------------------*/
extern uint8_t *_pNandBuffer;

#define NANDLIB_GET16_L(bptr,n)       *(uint16_t *)((bptr) + (n))
#define NANDLIB_GET32_L(bptr,n)       *(uint32_t *)((bptr) + (n))
#define NANDLIB_PUT16_L(bptr,n,val)   *(uint16_t *)((bptr) + (n)) = (val)
#define NANDLIB_PUT32_L(bptr,n,val)   *(uint32_t *)((bptr) + (n)) = (val)


#ifndef MIN
#define MIN(x,y)          (((x) < (y)) ? (x) : (y))
#endif

#ifndef OFFSETOF
#define OFFSETOF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#endif

int32_t NANDLIB_OpRelink(NDISK_T *ptNDisk, uint16_t LBlockAddr, uint16_t *PBlockAddr, 
                         int32_t nStartPage, int32_t nPageCnt, uint16_t oldpba);
int32_t NANDLIB_GetDataBlock(NDISK_T *ptNDisk, uint16_t LBlockAddr, uint16_t *PBlockAddr);
int32_t NANDLIB_CheckEmpty(NDISK_T *ptNDisk, uint32_t pba);
int32_t NANDLIB_UpdateP2LN(NDISK_T *ptNDisk);
int32_t NANDLIB_OpLink(NDISK_T *ptNDisk, uint16_t LBlockAddr, uint16_t *PBlockAddr);
int32_t NANDLIB_Erase(NDISK_T *ptNDisk, uint32_t pba);
void NANDLIB_ClrDirtyBlock(NDISK_T *ptNDisk, uint32_t pba);

/** @endcond HIDDEN_SYMBOLS */
#endif  /* __NANDLIB_GLOBAL_H__ */
