/*
 *  linux/include/linux/ms/host.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Host driver specific definitions.
 *
 * Modification History:
 *    #000 2009-07-09 CMYu   modify from host.h for ms usage
 */
#ifndef LINUX_MS_HOST_H
#define LINUX_MS_HOST_H

#include <linux/ms/ms.h>

struct ms_ios {
	unsigned int	clock;			/* clock rate */
	unsigned short	vdd;
/*
#define	MS_VDD_150	0
#define	MS_VDD_155	1
#define	MS_VDD_160	2
#define	MS_VDD_165	3
#define	MS_VDD_170	4
#define	MS_VDD_180	5
#define	MS_VDD_190	6
#define	MS_VDD_200	7
#define	MS_VDD_210	8
#define	MS_VDD_220	9
#define	MS_VDD_230	10
#define	MS_VDD_240	11
#define	MS_VDD_250	12
#define	MS_VDD_260	13
#define	MS_VDD_270	14
#define	MS_VDD_280	15
#define	MS_VDD_290	16
#define	MS_VDD_300	17
#define	MS_VDD_310	18
#define	MS_VDD_320	19
#define	MS_VDD_330	20
#define	MS_VDD_340	21
#define	MS_VDD_350	22
#define	MS_VDD_360	23
*/
	unsigned char	bus_mode;		/* command output mode */

#define MS_BUSMODE_OPENDRAIN	1
#define MS_BUSMODE_PUSHPULL	    2

	unsigned char	power_mode;		/* power supply mode */

#define MS_POWER_OFF		0
#define MS_POWER_UP		    1
#define MS_POWER_ON		    2

	char card_type;
//	char MS_4bit;
	unsigned char	bus_width;	/* data bus width */

	#define MS_BUS_WIDTH_1		    0
    #define MS_BUS_WIDTH_4		    2
    #define MS_BUS_WIDTH_8		    3

	unsigned char	timing;			/* timing specification used */

#define MS_TIMING_LEGACY	0
#define MS_TIMING_HS	    1

	unsigned int card_present;
	unsigned int set_parameter_flag;
};

struct ms_host_ops {
	void	(*request)(struct ms_host *host, struct ms_request *req);
	void	(*set_ios)(struct ms_host *host, struct ms_ios *ios);
};

struct ms_card;
struct device;

/* Flag Definitions */
#define HOST_CAP_SD_4_BIT       (1 << 0)

/*
struct memstick_host {
	struct mutex        lock;
	unsigned int        id;
	unsigned int        caps;
#define MEMSTICK_CAP_AUTO_GET_INT  1
#define MEMSTICK_CAP_PAR4          2
#define MEMSTICK_CAP_PAR8          4

	struct work_struct  media_checker;
	struct device       dev;

	struct memstick_dev *card;
	unsigned int        retries;

	// Notify the host that some requests are pending.
	void                (*request)(struct memstick_host *host);
	// Set host IO parameters (power, clock, etc).
	int                 (*set_param)(struct memstick_host *host,
					 enum memstick_param param,
					 int value);
	unsigned long       private[0] ____cacheline_aligned;
};
*/
struct ms_host {
	struct device		*dev;
	struct ms_host_ops	*ops;
	unsigned int		f_min;
	unsigned int		f_max;
	u32			ocr_avail;
	char			host_name[8];
	unsigned long           flags;

	/* host specific block data */
	unsigned int		max_seg_size;	/* see blk_queue_max_segment_size */
	unsigned short		max_hw_segs;	/* see blk_queue_max_hw_segments */
	unsigned short		max_phys_segs;	/* see blk_queue_max_phys_segments */
	unsigned short		max_sectors;	/* see blk_queue_max_sectors */
	unsigned short		unused;

	/* private data */
	struct ms_ios		ios;		/* current io bus settings */

	//struct list_head	cards;		/* devices attached to this host */
	struct ms_card		*card;		/* device attached to this host */

	wait_queue_head_t	wq;
	spinlock_t		lock;		/* card_busy lock */
	struct semaphore            ms_sem;
	struct ms_card		*card_busy;	/* the MS card claiming host */
	struct ms_card		*card_selected;	/* the selected MS card */

    u8 ins_event;
#define EVENT_NON		    0
#define EVENT_INSER		    1
#define EVENT_REMOV		    2
	struct work_struct	detect;
	int type;	/* NONE=0, MS=1, MSPRO=2 */
};


extern struct ms_host *ms_alloc_host(int extra, struct device *);
extern int ms_add_host(struct ms_host *);
extern void ms_remove_host(struct ms_host *);
extern void ms_free_host(struct ms_host *);

#define ms_priv(x)	((void *)((x) + 1))
#define ms_dev(x)	((x)->dev)

extern int ms_suspend_host(struct ms_host *, pm_message_t);
extern int ms_resume_host(struct ms_host *);

extern void ms_detect_change(struct ms_host *);
extern void ms_request_done(struct ms_host *, struct ms_request *);

#endif

