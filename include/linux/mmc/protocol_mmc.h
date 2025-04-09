/*
 * Header for MultiMediaCard (MMC)
 *
 * Copyright 2002 Hewlett-Packard Company
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Based strongly on code by:
 *
 * Author: Yong-iL Joh <tolkien@mizi.com>
 * Date  : $Date: 2002/06/18 12:37:30 $
 *
 * Author:  Andrew Christian
 *          15 May 2002
 */

#ifndef MMC_MMC_PROTOCOL_H
#define MMC_MMC_PROTOCOL_H

/*  ===== SD/MMC Command Sets ===== */
//                                                  Cmd     Resp    MMC     SD
//                                                  type    type    Class   Class
#define CMD0_GO_IDLE_STATE              0       //  bc      --      0       0
#define CMD1_SEND_OP_COND               1       //  bcr     r3      0       --
#define CMD2_ALL_SEND_CID               2       //  bcr     r2      0       0
#define CMD3_SET_RELATIVE_ADDR          3       //  ac      r1      0       --
#define CMD3_SEND_RELATIVE_ADDR         3       //  bcr     r6      --      0
#define CMD4_SET_DSR                    4       //  bc      --      0       0
#define CMD6_SWITCH                     6       //  ac      r1b     0       --
#define CMD6_SWITCH_FUNC                6       //  adtc    r1      --      10
#define CMD7_SELECT_CARD                7       //  ac      r1      0       --
//      CMD7_SELECT_CARD                7       //  ac      r1b     --      0
#define CMD7_DESELECT_CARD              7       //  ac      r1b     0       0
#define CMD8_SEND_EXT_CSD               8       //  adtc    r1      0       --
#define CMD8_SEND_IF_COND               8       //  bcr     r7      --      0
#define CMD9_SEND_CSD                   9       //  ac      r2      0       0
#define CMD10_SEND_CID                  10      //  ac      r2      0       0
#define CMD11_READ_DAT_UTIL_STOP        11      //  adtc    r1      1       --
#define CMD12_STOP_TRANSMISSION         12      //  ac      r1/r1b  0       --
//      CMD12_STOP_TRANSMISSION         12      //  ac      r1b     --      0
#define CMD13_SEND_STATUS               13      //  ac      r1      0       0
#define CMD14_BUSTEST_R                 14      //  adtc    r1      0       --
#define CMD15_GO_INACTIVE_STATE         15      //  ac      --      0       0
#define CMD16_SET_BLOCKLEN              16      //  ac      r1      2,4,7   2,4,7
#define CMD17_READ_SINGLE_BLOCK         17      //  adtc    r1      2       2
#define CMD18_READ_MULTIPLE_BLOCK       18      //  adtc    r1      2       2
#define CMD19_BUSTEST_W                 19      //  adtc    r1      0       --
#define CMD20_WRITE_DAT_UNTIL_STOP      20      //  adtc    r1      3       --
#define CMD23_SET_BLOCK_COUNT           23      //  ac      r1      2,4     --
#define CMD24_WRITE_BLOCK               24      //  adtc    r1      4       4
#define CMD25_WRITE_MULTIPLE_BLOCK      25      //  adtc    r1      4       4
#define CMD26_PROGRAM_CID               26      //  adtc    r1      4       --
#define CMD27_PROGRAM_CSD               27      //  adtc    r1      4       4
#define CMD28_SET_WRITE_PROT            28      //  ac      r1b     6       6
#define CMD29_CLR_WRITE_PROT            29      //  ac      r1b     6       6
#define CMD30_SEND_WRITE_PROT           30      //  adtc    r1      6       6
#define CMD32_ERASE_WR_BLK_START        32      //  ac      r1      --      5
#define CMD33_ERASE_WR_BLK_END          33      //  ac      r1      --      5
#define CMD35_ERASE_GROUP_START         35      //  ac      r1      5       --
#define CMD36_ERASE_GROUP_END           36      //  ac      r1      5       --
#define CMD38_ERASE                     38      //  ac      r1b     5       5
#define CMD39_FAST_IO                   39      //  ac      r4      9       --
#define CMD40_GO_IRQ_STATE              40      //  bcr     r5      9       --
#define CMD42_LOCK_UNLOCK               42      //  adtc    r1      7       7
#define CMD55_APP_CMD                   55      //  ac      r1      8       8
#define CMD56_GEN_CMD                   56      //  adtc    r1      8       8
#define ACMD6_SET_BUS_WIDTH             6       //  ac      r1      --      app
#define ACMD13_SD_STATUS                13      //  adtc    r1      --      app
#define ACMD22_SEND_NUM_WR_BLOCK        22      //  adtc    r1      --      app
#define ACMD23_SET_WR_BLK_ERASE_COUNT   23      //  ac      r1      --      app
#define ACMD41_SD_APP_OP_COND           41      //  bcr     r3      --      app
#define ACMD42_SET_CLR_CARD_DETECT      42      //  ac      r1      --      app
#define ACMD51_SEND_SCR                 51      //  adtc    r1      --      app

/*
  MMC status in R1
  Type
  	e : error bit
	s : status bit
	r : detected and set for the actual command response
	x : detected and set during command execution. the host must poll
            the card by sending status command in order to read these bits.
  Clear condition
  	a : according to the card state
	b : always related to the previous command. Reception of
            a valid command will clear it (with a delay of one command)
	c : clear by read
 */

#define R1_OUT_OF_RANGE		    (1 << 31)	/* er, c */
#define R1_ADDRESS_ERROR	    (1 << 30)	/* erx, c */
#define R1_BLOCK_LEN_ERROR	    (1 << 29)	/* er, c */
#define R1_ERASE_SEQ_ERROR      (1 << 28)	/* er, c */
#define R1_ERASE_PARAM		    (1 << 27)	/* ex, c */
#define R1_WP_VIOLATION		    (1 << 26)	/* erx, c */
#define R1_CARD_IS_LOCKED	    (1 << 25)	/* sx, a */
#define R1_LOCK_UNLOCK_FAILED	(1 << 24)	/* erx, c */
#define R1_COM_CRC_ERROR	    (1 << 23)	/* er, b */
#define R1_ILLEGAL_COMMAND	    (1 << 22)	/* er, b */
#define R1_CARD_ECC_FAILED	    (1 << 21)	/* ex, c */
#define R1_CC_ERROR		        (1 << 20)	/* erx, c */
#define R1_ERROR		        (1 << 19)	/* erx, c */
#define R1_UNDERRUN		        (1 << 18)	/* ex, c */
#define R1_OVERRUN		        (1 << 17)	/* ex, c */
#define R1_CID_CSD_OVERWRITE	(1 << 16)	/* erx, c, CID/CSD overwrite */
#define R1_WP_ERASE_SKIP	    (1 << 15)	/* sx, c */
#define R1_CARD_ECC_DISABLED	(1 << 14)	/* sx, a */
#define R1_ERASE_RESET		    (1 << 13)	/* sr, c */
#define R1_STATUS(x)            (x & 0xFFFFE000)
#define R1_CURRENT_STATE(x)    	((x & 0x00001E00) >> 9)	/* sx, b (4 bits) */
#define R1_READY_FOR_DATA	    (1 << 8)	/* sx, a */
#define R1_APP_CMD		        (1 << 5)	/* sr, c */

/* These are unpacked versions of the actual responses */

struct _mmc_csd {
	u8  csd_structure;
	u8  spec_vers;
	u8  taac;
	u8  nsac;
	u8  tran_speed;
	u16 ccc;
	u8  read_bl_len;
	u8  read_bl_partial;
	u8  write_blk_misalign;
	u8  read_blk_misalign;
	u8  dsr_imp;
	u16 c_size;
	u8  vdd_r_curr_min;
	u8  vdd_r_curr_max;
	u8  vdd_w_curr_min;
	u8  vdd_w_curr_max;
	u8  c_size_mult;
	union {
		struct { /* MMC system specification version 3.1 */
			u8  erase_grp_size;
			u8  erase_grp_mult;
		} v31;
		struct { /* MMC system specification version 2.2 */
			u8  sector_size;
			u8  erase_grp_size;
		} v22;
	} erase;
	u8  wp_grp_size;
	u8  wp_grp_enable;
	u8  default_ecc;
	u8  r2w_factor;
	u8  write_bl_len;
	u8  write_bl_partial;
	u8  file_format_grp;
	u8  copy;
	u8  perm_write_protect;
	u8  tmp_write_protect;
	u8  file_format;
	u8  ecc;
};

#define MMC_VDD_145_150	0x00000001	/* VDD voltage 1.45 - 1.50 */
#define MMC_VDD_150_155	0x00000002	/* VDD voltage 1.50 - 1.55 */
#define MMC_VDD_155_160	0x00000004	/* VDD voltage 1.55 - 1.60 */
#define MMC_VDD_160_165	0x00000008	/* VDD voltage 1.60 - 1.65 */
#define MMC_VDD_165_170	0x00000010	/* VDD voltage 1.65 - 1.70 */
#define MMC_VDD_17_18	0x00000020	/* VDD voltage 1.7 - 1.8 */
#define MMC_VDD_18_19	0x00000040	/* VDD voltage 1.8 - 1.9 */
#define MMC_VDD_19_20	0x00000080	/* VDD voltage 1.9 - 2.0 */
#define MMC_VDD_20_21	0x00000100	/* VDD voltage 2.0 ~ 2.1 */
#define MMC_VDD_21_22	0x00000200	/* VDD voltage 2.1 ~ 2.2 */
#define MMC_VDD_22_23	0x00000400	/* VDD voltage 2.2 ~ 2.3 */
#define MMC_VDD_23_24	0x00000800	/* VDD voltage 2.3 ~ 2.4 */
#define MMC_VDD_24_25	0x00001000	/* VDD voltage 2.4 ~ 2.5 */
#define MMC_VDD_25_26	0x00002000	/* VDD voltage 2.5 ~ 2.6 */
#define MMC_VDD_26_27	0x00004000	/* VDD voltage 2.6 ~ 2.7 */
#define MMC_VDD_27_28	0x00008000	/* VDD voltage 2.7 ~ 2.8 */
#define MMC_VDD_28_29	0x00010000	/* VDD voltage 2.8 ~ 2.9 */
#define MMC_VDD_29_30	0x00020000	/* VDD voltage 2.9 ~ 3.0 */
#define MMC_VDD_30_31	0x00040000	/* VDD voltage 3.0 ~ 3.1 */
#define MMC_VDD_31_32	0x00080000	/* VDD voltage 3.1 ~ 3.2 */
#define MMC_VDD_32_33	0x00100000	/* VDD voltage 3.2 ~ 3.3 */
#define MMC_VDD_33_34	0x00200000	/* VDD voltage 3.3 ~ 3.4 */
#define MMC_VDD_34_35	0x00400000	/* VDD voltage 3.4 ~ 3.5 */
#define MMC_VDD_35_36	0x00800000	/* VDD voltage 3.5 ~ 3.6 */
#define MMC_CARD_BUSY	0x80000000	/* Card Power up status bit */
#define RTL_HOST_VDD	MMC_VDD_33_34|MMC_VDD_32_33|MMC_VDD_31_32|MMC_VDD_30_31


//CMYu, 20090625
#define SD_CCS	0x40000000	/* Card Power up status bit */
#define SD_HCS	0x40000000	/* host capacitty support bit */

/*
 * Card Command Classes (CCC)
 */
#define CCC_BASIC		    (1<<0)	/* (0) Basic protocol functions */
					                /* (CMD0,1,2,3,4,7,9,10,12,13,15) */
#define CCC_STREAM_READ		(1<<1)	/* (1) Stream read commands */
					                /* (CMD11) */
#define CCC_BLOCK_READ		(1<<2)	/* (2) Block read commands */
					                /* (CMD16,17,18) */
#define CCC_STREAM_WRITE	(1<<3)	/* (3) Stream write commands */
					                /* (CMD20) */
#define CCC_BLOCK_WRITE		(1<<4)	/* (4) Block write commands */
					                /* (CMD16,24,25,26,27) */
#define CCC_ERASE		    (1<<5)	/* (5) Ability to erase blocks */
					                /* (CMD32,33,34,35,36,37,38,39) */
#define CCC_WRITE_PROT		(1<<6)	/* (6) Able to write protect blocks */
					                /* (CMD28,29,30) */
#define CCC_LOCK_CARD		(1<<7)	/* (7) Able to lock down card */
					                /* (CMD16,CMD42) */
#define CCC_APP_SPEC		(1<<8)	/* (8) Application specific */
					                /* (CMD55,56,57,ACMD*) */
#define CCC_IO_MODE		    (1<<9)	/* (9) I/O mode */
					                /* (CMD5,39,40,52,53) */
#define CCC_SWITCH		    (1<<10)	/* (10) High speed switch */
					                /* (CMD6,34,35,36,37,50) */
					                /* (11) Reserved */
					                /* (CMD?) */

/*
 * CSD field definitions
 */

#define CSD_STRUCT_VER_1_0  0           /* Valid for system specification 1.0 - 1.2 */
#define CSD_STRUCT_VER_1_1  1           /* Valid for system specification 1.4 - 2.2 */
#define CSD_STRUCT_VER_1_2  2           /* Valid for system specification 3.1       */

#define CSD_SPEC_VER_0      0           /* Implements system specification 1.0 - 1.2 */
#define CSD_SPEC_VER_1      1           /* Implements system specification 1.4 */
#define CSD_SPEC_VER_2      2           /* Implements system specification 2.0 - 2.2 */
#define CSD_SPEC_VER_3      3           /* Implements system specification 3.1 */

#endif  /* MMC_MMC_PROTOCOL_H */

