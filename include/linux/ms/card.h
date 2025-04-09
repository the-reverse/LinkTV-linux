/*
 *  linux/include/linux/ms/card.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Card driver specific definitions.
 *
 * Modification History:
 *    #000 2009-07-09 CMYu   modify from card.h for ms usage
 */
#ifndef LINUX_MS_CARD_H
#define LINUX_MS_CARD_H

#include <linux/ms/ms.h>

struct ms_host;

/*
 * MS device
 */
struct ms_card {
	//struct list_head	node;		/* node in hosts devices list */
	struct ms_host		*host;		/* the host this device belongs to */
	struct device		dev;		/* the device */
	unsigned int		state;		/* (our) card state */
#define MS_STATE_PRESENT	(1<<0)		/* present in sysfs */
#define MS_STATE_DEAD		(1<<1)		/* device no longer in stack */
#define MS_STATE_BAD		(1<<2)		/* unrecognised device */
#define MS_STATE_IS_MSPRO	(1<<3)		/* device is a SD card */
	//CMYu, 20090621
	unsigned int card_type;	/* NONE=0, MS=1, MSPRO=2 */
//#define CR_SD       (1<<0)          /* device is a SD card */
//#define CR_SDHC     (1<<1)          /* device is a SDHC card */
//#define CR_MMC      (1<<2)          /* device is a MMC card */
//#define CR_MS       (1<<3)          /* device is a Memory Stick card */
//#define CR_MS_PRO   (1<<4)          /* device is a Memory Stick Pro card */

	//CMYu, 20090717
	struct ms_register_addr  reg_rw_addr;
	struct memstick_device_id id;
	//CMYu, 20090723
	unsigned int capacity;
	int write_protect_flag;
	//CMYu, 20090727
	struct mspro_sys_info mspro_sysinfo;
	//CMYu, 20090804
	unsigned int page_size;
	unsigned short cylinders;
	unsigned short heads;
	unsigned short sectors_per_track;
	//CMYu, 20090811
	char MS_4bit;
	//MS
	unsigned char MSBootBlock;
	unsigned char MSBlockShift;
	unsigned char MSPageOff;
	unsigned short MSTotalBlock;
};

#define ms_card_present(c)	    ((c)->state & MS_STATE_PRESENT)
#define ms_card_dead(c)	        ((c)->state & MS_STATE_DEAD)
#define ms_card_bad(c)		    ((c)->state & MS_STATE_BAD)

#define ms_card_set_present(c)	((c)->state |= MS_STATE_PRESENT)
#define ms_card_set_dead(c)	    ((c)->state |= MS_STATE_DEAD)
#define ms_card_set_bad(c)	    ((c)->state |= MS_STATE_BAD)

#define ms_card_sn(c)	        ((c)->mspro_sysinfo.serial_number)
#define ms_card_id(c)		    ((c)->dev.bus_id)

#define ms_list_to_card(l)	    container_of(l, struct ms_card, node)
#define ms_get_drvdata(c)	    dev_get_drvdata(&(c)->dev)
#define ms_set_drvdata(c,d)	    dev_set_drvdata(&(c)->dev, d)

/*
 * MS device driver (e.g., Flash card, I/O card...)
 */
struct ms_driver {
	struct device_driver drv;
	int (*probe)(struct ms_card *);
	void (*remove)(struct ms_card *);
	int (*suspend)(struct ms_card *, pm_message_t);
	int (*resume)(struct ms_card *);
};

extern int ms_register_driver(struct ms_driver *);
extern void ms_unregister_driver(struct ms_driver *);

static inline int ms_card_claim_host(struct ms_card *card)
{
	return __ms_claim_host(card->host, card);
}

#define ms_card_release_host(c)	ms_release_host((c)->host)

#endif
