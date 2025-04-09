/*
 *  linux/include/linux/mmc/card.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Card driver specific definitions.
 */
#ifndef LINUX_MMC_CARD_H
#define LINUX_MMC_CARD_H

#include <linux/mmc/mmc.h>
#include <linux/mmc/ms.h>

struct mmc_cid {    //Card Identifcation Number Register(CID)
	unsigned int		manfid;
	char			prod_name[8];
	unsigned char		prv;	//CMYu, 20090703, for MMC 4.2
	unsigned int		serial;
	unsigned short		oemid;
	unsigned short		year;
	unsigned char		hwrev;
	unsigned char		fwrev;
	unsigned char		month;
};

struct mmc_csd {    //Card Specific Data Register(CSD)
	unsigned char		ver; /* CSD struct version */
	unsigned char		mmca_vsn;
	unsigned short		cmdclass;
	unsigned short		tacc_clks;
	unsigned int		tacc_ns;
	unsigned int		r2w_factor;
	unsigned int		max_dtr;
	unsigned int		read_blkbits;
	unsigned int		write_blkbits;
	unsigned int		capacity;
	unsigned int		read_partial:1,
        				read_misalign:1,
        				write_partial:1,
        				write_misalign:1;
};

struct mmc_ext_csd {
	u8			rev;
	unsigned int		sa_timeout;		/* Units: 100ns */
	unsigned int		hs_max_dtr;
	unsigned int		sectors;
};

struct sd_scr {
	unsigned char		sda_vsn;
	unsigned char		bus_widths;
#define SD_SCR_BUS_WIDTH_1	(1<<0)
#define SD_SCR_BUS_WIDTH_4	(1<<2)
};


#define SCR_STRUCTURE           0xf0000000
#define SD_SPEC                 0x0f000000
#define DATA_STAT_AFTER_ERASE   0x00800000
#define SD_SECURITY             0x00700000
#define SD_BUS_WIDTHS           0x000f0000

/*SCR_STRUCTURE */
#define SCR_VER_10  0

/*SD_SPEC*/
#define PHY_VER10   0
#define PHY_VER11   1
#define PHY_VER20   2

/*SD_SECURITY*/
#define SECURITY_NONE       0
#define SECURITY_NOT_USED   1
#define SECURITY_VER101     2
#define SECURITY_VER200     3

//#define get_scr_para(scr_val,para)    (scr_val&para)>>(ffs(para)-1);

struct mmc_host;
/*
 * MMC device
 */
struct mmc_card {   //for card information

	struct mmc_host		*host;		/* the host this device belongs to */
	struct device		dev;		/* the device */
	unsigned int		rca;		/* relative card address of device */
	unsigned int card_type;

//#define CR_SD       (1<<0)          /* device is a SD card */
//#define CR_SDHC     (1<<1)          /* device is a SDHC card */
//#define CR_MMC      (1<<2)          /* device is a MMC card */
//#define CR_MS       (1<<3)          /* device is a Memory Stick card */
//#define CR_MS_PRO   (1<<4)          /* device is a Memory Stick Pro card */

	unsigned int		state;		/* (our) card state */
#define MMC_STATE_PRESENT	(1<<0)	/* present in sysfs */
#define MMC_STATE_DEAD		(1<<1)	/* device no longer in stack */
#define MMC_STATE_BAD		(1<<2)	/* unrecognised device */

#define MMC_STATE_READONLY	(1<<4)		/* card is read-only */
#define MMC_STATE_HIGHSPEED	(1<<5)		/* card is in high speed mode */
#define MMC_STATE_BLOCKADDR	(1<<6)		/* card uses block-addressing */
#define MMC_STATE_WP	    (1<<7)		/* card write protect status */

#define MMC_STATE_ONE_BLK	(1<<8)		/* writting use one block mode */
#define MMC_STATE_IDENT_RDY	(1<<9)		/* for check identify state, */

/* SD/MMC use *** */
	u32			raw_cid[4];	        /* raw card CID */
	u32			raw_csd[4];	        /* raw card CSD */
	u32			raw_scr[2];	        /* raw card CSD */
	struct mmc_cid		cid;		/* card identification */
	struct mmc_csd		csd;		/* card specific */
	struct mmc_ext_csd	ext_csd;	/* mmc v4 extended card specific */
	struct sd_scr		scr;		/* extra SD information */

    u32 test_count;
/* SD/MMC use &&& */

/* MS-Pro use *** */
    struct ms_register_addr reg_rw_addr;
	struct memstick_device_id id;
	unsigned int capacity;
	int write_protect_flag;
	struct mspro_sys_info mspro_sysinfo;
	unsigned int page_size;
	unsigned short cylinders;
	unsigned short heads;
	unsigned short sectors_per_track;
/* MS-Pro use *** */

};

#define mmc_card_present(c)	    ((c)->state & MMC_STATE_PRESENT)
#define mmc_card_dead(c)	    ((c)->state & MMC_STATE_DEAD)
#define mmc_card_bad(c)		    ((c)->state & MMC_STATE_BAD)
#define mmc_card_readonly(c)	((c)->state & MMC_STATE_READONLY)
#define mmc_card_highspeed(c)	((c)->state & MMC_STATE_HIGHSPEED)
#define mmc_card_blockaddr(c)	((c)->state & MMC_STATE_BLOCKADDR)
#define mmc_card_wp(c)	        ((c)->state & MMC_STATE_WP)         //liao
#define mmc_card_one_blk(c)	    ((c)->state & MMC_STATE_ONE_BLK)    //liao
#define mmc_card_ident_rdy(c)	((c)->state & MMC_STATE_IDENT_RDY)  //liao

#define mmc_card_set_present(c)	((c)->state |= MMC_STATE_PRESENT)
#define mmc_card_set_dead(c)	((c)->state |= MMC_STATE_DEAD)
#define mmc_card_set_bad(c)	    ((c)->state |= MMC_STATE_BAD)
#define mmc_set_card_readonly(c)	((c)->state |= MMC_STATE_READONLY)
#define mmc_card_set_highspeed(c)   ((c)->state |= MMC_STATE_HIGHSPEED)
#define mmc_card_set_blockaddr(c) ((c)->state |= MMC_STATE_BLOCKADDR)
#define mmc_set_card_wp(c)	        ((c)->state |= MMC_STATE_WP)        //liao
#define mmc_card_set_one_blk(c)	    ((c)->state |= MMC_STATE_ONE_BLK)   //liao
#define mmc_card_set_ident_rdy(c)	((c)->state |= MMC_STATE_IDENT_RDY) //liao

#define mmc_card_name(c)	    ((c)->cid.prod_name)
#define mmc_card_id(c)		    ((c)->dev.bus_id)

#define mmc_list_to_card(l)	    container_of(l, struct mmc_card, node)
#define mmc_get_drvdata(c)	    dev_get_drvdata(&(c)->dev)
#define mmc_set_drvdata(c,d)	dev_set_drvdata(&(c)->dev, d)

/*
 * MMC device driver (e.g., Flash card, I/O card...)
 */
struct mmc_driver {
	struct device_driver drv;
	int (*probe)(struct mmc_card *);
	void (*remove)(struct mmc_card *);
	int (*suspend)(struct mmc_card *, pm_message_t);
	int (*resume)(struct mmc_card *);
};

extern int mmc_register_driver(struct mmc_driver *);
extern void mmc_unregister_driver(struct mmc_driver *);

extern int __mmc_claim_host(struct mmc_host *host, struct mmc_card *card);
static inline int mmc_card_claim_host(struct mmc_card *card)
{
	return __mmc_claim_host(card->host, card);
}

#define mmc_card_release_host(c)	mmc_release_host((c)->host)

#define MMC_BLK_BITS 9

#endif
