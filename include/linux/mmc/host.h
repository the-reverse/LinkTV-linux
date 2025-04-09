/*
 *  linux/include/linux/mmc/host.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Host driver specific definitions.
 */
#ifndef LINUX_MMC_HOST_H
#define LINUX_MMC_HOST_H

#include <linux/mmc/mmc.h>
#include <linux/mmc/ms.h>

//////////
struct mmc_data;
struct mmc_request;


#define MMC_ERR_NONE	0
#define MMC_ERR_TIMEOUT	1
#define MMC_ERR_BADCRC	2
#define MMC_ERR_RMOVE	3
#define MMC_ERR_FAILED	4
#define MMC_ERR_INVALID	5
struct mmc_command {
	u32			opcode;
	u32			arg;
	u32			resp[4];            /* u32=4byte; 4x4=16byte */
	unsigned int		flags;		/* expected response type */

#define MMC_RSP_NONE	(0 << 0)
#define MMC_RSP_SHORT	(1 << 0)
#define MMC_RSP_LONG	(2 << 0)
#define MMC_RSP_MASK	(3 << 0)
#define MMC_RSP_CRC	    (1 << 3)		/* expect valid crc */
#define MMC_RSP_BUSY	(1 << 4)		/* card may send busy */

/*
 * These are the response types, and correspond to valid bit
 * patterns of the above flags.  One additional valid pattern
 * is all zeros, which means we don't expect a response.
 */
#define MMC_RSP_R1	(MMC_RSP_SHORT|MMC_RSP_CRC)
#define MMC_RSP_R1B	(MMC_RSP_SHORT|MMC_RSP_CRC|MMC_RSP_BUSY)
#define MMC_RSP_R2	(MMC_RSP_LONG|MMC_RSP_CRC)
#define MMC_RSP_R3	(MMC_RSP_SHORT)
#define MMC_RSP_R6	(MMC_RSP_SHORT|MMC_RSP_CRC)

#define mmc_resp_type(cmd)	((cmd)->flags & (MMC_RSP_SHORT|MMC_RSP_LONG|MMC_RSP_MASK|MMC_RSP_CRC|MMC_RSP_BUSY))

#define MMC_CMD_MASK	(3 << 5)		/* non-SPI command type */
#define MMC_CMD_AC	    (0 << 5)
#define MMC_CMD_ADTC	(1 << 5)
#define MMC_CMD_BC	    (2 << 5)
#define MMC_CMD_BCR	    (3 << 5)

#define mmc_cmd_type(cmd)	((cmd)->flags & MMC_CMD_MASK)

	unsigned int		retries;	/* max number of retries */
	unsigned int		error;		/* command error */
//#define MMC_ERR_NONE	    0
//#define MMC_ERR_TIMEOUT	1
//#define MMC_ERR_BADCRC	2
//#define MMC_ERR_RMOVE	    3
//#define MMC_ERR_FAILED	4
//#define MMC_ERR_INVALID	5

	struct mmc_data		*data;		/* data segment associated with cmd */
	struct mmc_request	*mrq;		/* assoicated request */
};

struct ms_tpc_set {
    unsigned char tpc;
	struct ms_register_addr *reg_rw_addr;
	struct memstick_device_id *id;

	unsigned char reg_data[0x20];
	unsigned char status;
	unsigned char *long_data;
	unsigned char option;
	unsigned char byte_count;
	unsigned int sector_cnt;
	unsigned int timeout;
	unsigned int endian;
#define LITTLE  0
#define BIG     1

//	unsigned int sector_start;

	//unsigned int		flags;		/* expected response type */
	unsigned int		retries;	/* max number of retries */
	unsigned int		error;		/* command error */

//#define MMC_ERR_NONE	    0
//#define MMC_ERR_TIMEOUT	1
//#define MMC_ERR_BADCRC	2
//#define MMC_ERR_RMOVE	    3
//#define MMC_ERR_FAILED	4
//#define MMC_ERR_INVALID	5

	struct mmc_data		*data;		/* data segment associated with cmd */
	struct mmc_request	*mrq;		/* assoicated request */
};
////////////////////////////////////////////////////////////////

struct mmc_data {
	unsigned int		timeout_ns;	    /* data timeout (in ns, max 80ms) */
	unsigned int		timeout_clks;	/* data timeout (in clocks) */
	unsigned int		blksz;	    /* data block size */
	unsigned int		blocks;		    /* number of blocks */
	unsigned int		error;		    /* data error */
	unsigned int		flags;

#define MMC_DATA_WRITE	(1 << 8)
#define MMC_DATA_READ	(1 << 9)
#define MMC_DATA_STREAM	(1 << 10)
#define RETURN_DATA	    (1 << 11)

	unsigned int		bytes_xfered;

	struct mmc_command	*stop;		/* stop command */
	struct mmc_request	*mrq;		/* assoicated request */

	unsigned int		sg_len;		/* size of scatter list */
	struct scatterlist	*sg;		/* I/O scatter list */
};

struct mmc_request {
	struct mmc_command	*cmd;   //MMC only
	struct mmc_data		*data;
	struct mmc_command	*stop;  //MMC only

    /* MS use only*** */
	struct ms_register_addr *reg_rw_addr;
	struct memstick_device_id *id;
	unsigned char tpc;
	unsigned char reg_data[0x20];
	int error;
	unsigned char status;
	unsigned char *long_data;
	unsigned char option;
	unsigned char byte_count;
	unsigned int sector_cnt;
	unsigned int timeout;
	unsigned int endian;
#define LITTLE  0
#define BIG     1

//	unsigned int sector_start;
    /* MS use onl&&& */

	void			*done_data;	/* completion data */
	void			(*done)(struct mmc_request *);/* completion function */
};

//////////


struct mmc_host;
struct mmc_card;

extern int mmc_wait_for_req(struct mmc_host *, struct mmc_request *);
extern int mmc_wait_for_cmd(struct mmc_host *, struct mmc_command *, int);

extern int __mmc_claim_host(struct mmc_host *host, struct mmc_card *card);

static inline void mmc_claim_host(struct mmc_host *host)
{
	__mmc_claim_host(host, (struct mmc_card *)-1);
}

extern void mmc_release_host(struct mmc_host *host);


struct mmc_ios {
	unsigned int	clock;		/* clock rate */
	unsigned short	vdd;        /* vdd stores the bit number of the selected voltage range from below. */

	unsigned char	bus_mode;	/* command output mode */

#define MMC_BUSMODE_OPENDRAIN	1
#define MMC_BUSMODE_PUSHPULL	2

	unsigned char	power_mode;	/* power supply mode */

#define MMC_POWER_OFF		    0
#define MMC_POWER_UP		    1
#define MMC_POWER_ON		    2

    unsigned char	bus_width;	/* data bus width */
#define MMC_BUS_WIDTH_1		    0
#define MMC_BUS_WIDTH_4		    2
//#define MMC_BUS_WIDTH_8		    3

	unsigned char	timing;			/* timing specification used */

#define MMC_TIMING_LEGACY	0
#define MMC_TIMING_MMC_HS	1
#define MMC_TIMING_SD_HS	2
};

struct mmc_request;
struct mmc_ios;
struct mmc_host_ops {
	void	(*request)(struct mmc_host *host, struct mmc_request *req);
	void	(*set_ios)(struct mmc_host *host, struct mmc_ios *ios);
	void	(*fake_proc)(struct mmc_host *host);
};

struct mmc_card;
struct device;

/* Flag Definitions */
struct mmc_host {
	struct device		*dev;
	struct mmc_host_ops	*ops;
	unsigned int		f_min;
	unsigned int		f_max;
	u32			ocr_avail;
	char			host_name[8];
    u8 irq;
	unsigned long           flags;
    unsigned int card_type_pre;
    //#define CR_SD       (1<<0)          /* device is a SD card */
    //#define CR_SDHC     (1<<1)          /* device is a SDHC card */
    //#define CR_MMC      (1<<2)          /* device is a MMC card */
    //#define CR_MS       (1<<3)          /* device is a Memory Stick card */
    //#define CR_MS_PRO   (1<<4)          /* device is a Memory Stick Pro card */
	/* host specific block data */
	unsigned int		max_seg_size;	/* see blk_queue_max_segment_size */
	unsigned short		max_hw_segs;	/* see blk_queue_max_hw_segments */
	unsigned short		max_phys_segs;	/* see blk_queue_max_phys_segments */
	unsigned short		max_sectors;	/* see blk_queue_max_sectors */
	unsigned short		unused;

	/* private data */
	struct mmc_ios		ios;		/* current io bus settings */
	u32			ocr;		/* the current OCR setting */

	//struct list_head	cards;		/* devices attached to this host */
	struct mmc_card		*card;		/* device attached to this host */

	wait_queue_head_t	wq;
	spinlock_t		lock;		/* card_busy lock */
	struct semaphore            mmc_sema;
	struct mmc_card		*card_busy;	/* the MMC card claiming host */
	struct mmc_card		*card_selected;	/* the selected MMC card */
	u8 ins_event;
#define EVENT_NON		    0
#define EVENT_INSER		    1
#define EVENT_REMOV		    2

	struct work_struct	detect;     /*detect card function work*/
};

extern struct mmc_host *mmc_alloc_host(int extra, struct device *);
extern int mmc_add_host(struct mmc_host *);
extern void mmc_remove_host(struct mmc_host *);
extern void mmc_free_host(struct mmc_host *);

#define mmc_priv(x)	((void *)((x) + 1))
#define mmc_dev(x)	((x)->dev)

#define UNSTUFF_BITS(resp,start,size)                   \
    ({                              \
        const int __size = size;                \
        const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1; \
        const int __off = 3 - ((start) / 32);           \
        const int __shft = (start) & 31;            \
        u32 __res;                      \
                                    \
        __res = resp[__off] >> __shft;              \
        if (__size + __shft > 32)               \
            __res |= resp[__off-1] << ((32 - __shft) % 32); \
        __res & __mask;                     \
    })

extern int mmc_suspend_host(struct mmc_host *, pm_message_t);
extern int mmc_resume_host(struct mmc_host *);

extern void mmc_detect_change(struct mmc_host *);
extern void mmc_request_done(struct mmc_host *, struct mmc_request *);

#endif

