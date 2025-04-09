/*
 *  linux/include/linux/mmc/mmc.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef MMC_H
#define MMC_H

#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/device.h>

/*
 * CSD field definitions
 */

#define CSD_STRUCT_VER_1_0  0           /* Valid for system specification 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1           /* Valid for system specification 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2           /* Valid for system specification 3.1 - 3.2 - 3.31 - 4.0 - 4.1 */
#define CSD_STRUCT_EXT_CSD  3           /* Version is coded in CSD_STRUCTURE in EXT_CSD */

#define CSD_SPEC_VER_0      0           /* Implements system specification 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1           /* Implements system specification 1.4 */
#define CSD_SPEC_VER_2      2           /* Implements system specification 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3           /* Implements system specification 3.1 - 3.2 - 3.31 */
#define CSD_SPEC_VER_4      4           /* Implements system specification 4.0 - 4.1 */


/*
 * EXT_CSD fields
 */

#define EXT_CSD_BUS_WIDTH   183 /* R/W */
#define EXT_CSD_HS_TIMING   185 /* R/W */
#define EXT_CSD_CARD_TYPE   196 /* RO */
#define EXT_CSD_REV         192 /* RO */
#define EXT_CSD_SEC_CNT     212 /* RO, 4 bytes */
#define EXT_CSD_S_A_TIMEOUT 217

/*
 * EXT_CSD field definitions
 */

#define EXT_CSD_CMD_SET_NORMAL      (1<<0)
#define EXT_CSD_CMD_SET_SECURE      (1<<1)
#define EXT_CSD_CMD_SET_CPSECURE    (1<<2)

#define EXT_CSD_CARD_TYPE_26        (1<<0)  /* Card can run at 26MHz */
#define EXT_CSD_CARD_TYPE_52        (1<<1)  /* Card can run at 52MHz */

#define EXT_CSD_BUS_WIDTH_1         0   /* Card is in 1 bit mode */
#define EXT_CSD_BUS_WIDTH_4         1   /* Card is in 4 bit mode */
#define EXT_CSD_BUS_WIDTH_8         2   /* Card is in 8 bit mode */

/*
 * MMC_SWITCH access modes
 */

#define MMC_SWITCH_MODE_CMD_SET     0x00    /* Change the command set */
#define MMC_SWITCH_MODE_SET_BITS    0x01    /* Set bits which are 1 in value */
#define MMC_SWITCH_MODE_CLEAR_BITS  0x02    /* Clear bits which are 1 in value */
#define MMC_SWITCH_MODE_WRITE_BYTE  0x03    /* Set target to value */

///struct mmc_data;
///struct mmc_request;
///
///struct mmc_command {
///    u32         opcode;
///    u32         arg;
///    u32         resp[4];            /* u32=4byte; 4x4=16byte */
///    unsigned int        flags;      /* expected response type */
///
///#define MMC_RSP_NONE    (0 << 0)
///#define MMC_RSP_SHORT   (1 << 0)
///#define MMC_RSP_LONG    (2 << 0)
///#define MMC_RSP_MASK    (3 << 0)
///#define MMC_RSP_CRC     (1 << 3)        /* expect valid crc */
///#define MMC_RSP_BUSY    (1 << 4)        /* card may send busy */
///
////*
/// * These are the response types, and correspond to valid bit
/// * patterns of the above flags.  One additional valid pattern
/// * is all zeros, which means we don't expect a response.
/// */
///#define MMC_RSP_R1  (MMC_RSP_SHORT|MMC_RSP_CRC)
///#define MMC_RSP_R1B (MMC_RSP_SHORT|MMC_RSP_CRC|MMC_RSP_BUSY)
///#define MMC_RSP_R2  (MMC_RSP_LONG|MMC_RSP_CRC)
///#define MMC_RSP_R3  (MMC_RSP_SHORT)
///#define MMC_RSP_R6  (MMC_RSP_SHORT|MMC_RSP_CRC)
///
///#define mmc_resp_type(cmd)  ((cmd)->flags & (MMC_RSP_SHORT|MMC_RSP_LONG|MMC_RSP_MASK|MMC_RSP_CRC|MMC_RSP_BUSY))
///
///#define MMC_CMD_MASK    (3 << 5)        /* non-SPI command type */
///#define MMC_CMD_AC      (0 << 5)
///#define MMC_CMD_ADTC    (1 << 5)
///#define MMC_CMD_BC      (2 << 5)
///#define MMC_CMD_BCR     (3 << 5)
///
///#define mmc_cmd_type(cmd)   ((cmd)->flags & MMC_CMD_MASK)
///
///    unsigned int        retries;    /* max number of retries */
///    unsigned int        error;      /* command error */
///
///#define MMC_ERR_NONE    0
///#define MMC_ERR_TIMEOUT 1
///#define MMC_ERR_BADCRC  2
///#define MMC_ERR_RMOVE   3
///#define MMC_ERR_FAILED  4
///#define MMC_ERR_INVALID 5
///
///    struct mmc_data     *data;      /* data segment associated with cmd */
///    struct mmc_request  *mrq;       /* assoicated request */
///};
///
///struct mmc_data {
///    unsigned int        timeout_ns;     /* data timeout (in ns, max 80ms) */
///    unsigned int        timeout_clks;   /* data timeout (in clocks) */
///    unsigned int        blksz;      /* data block size */
///    unsigned int        blocks;         /* number of blocks */
///    unsigned int        error;          /* data error */
///    unsigned int        flags;
///
///#define MMC_DATA_WRITE  (1 << 8)
///#define MMC_DATA_READ   (1 << 9)
///#define MMC_DATA_STREAM (1 << 10)
///#define RETURN_DATA     (1 << 11)
///
///    unsigned int        bytes_xfered;
///
///    struct mmc_command  *stop;      /* stop command */
///    struct mmc_request  *mrq;       /* assoicated request */
///
///    unsigned int        sg_len;     /* size of scatter list */
///    struct scatterlist  *sg;        /* I/O scatter list */
///};
///
///struct mmc_request {
///    struct mmc_command  *cmd;
///    struct mmc_data     *data;
///    struct mmc_command  *stop;
///
//////////////////
///    struct ms_register_addr *reg_rw_addr;
///    struct memstick_device_id *id;
///    unsigned char tpc;
///    //struct ms_command *stop;
///    //CMYu, 20090722
///    unsigned char reg_data[0x20];
///    //CMYu, 20090723
///    int error;
///    unsigned char status;
///    unsigned char *long_data;
///    //CMYu, 20090729
///    unsigned char option;
///    unsigned char byte_count;
///    unsigned int sector_cnt;
///    unsigned int timeout;
///    unsigned int endian;
///    unsigned int sector_start;
//////////////////
///
///
///    void            *done_data; /* completion data */
///    void            (*done)(struct mmc_request *);/* completion function */
///};
///
///
///
///struct mmc_host;
///struct mmc_card;
///
///extern int mmc_wait_for_req(struct mmc_host *, struct mmc_request *);
///extern int mmc_wait_for_cmd(struct mmc_host *, struct mmc_command *, int);


//////////////////////


//////////////////////

#endif
