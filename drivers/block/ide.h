#ifndef _IDE_H
#define _IDE_H
/*
 *  linux/drivers/block/ide.h
 *
 *  Copyright (C) 1994-1998  Linus Torvalds & authors
 */

#include <linux/config.h>
#include <asm/ide.h>

/*
 * This is the multiple IDE interface driver, as evolved from hd.c.  
 * It supports up to four IDE interfaces, on one or more IRQs (usually 14 & 15).
 * There can be up to two drives per interface, as per the ATA-2 spec.
 *
 * Primary i/f:    ide0: major=3;  (hda)         minor=0; (hdb)         minor=64
 * Secondary i/f:  ide1: major=22; (hdc or hd1a) minor=0; (hdd or hd1b) minor=64
 * Tertiary i/f:   ide2: major=33; (hde)         minor=0; (hdf)         minor=64
 * Quaternary i/f: ide3: major=34; (hdg)         minor=0; (hdh)         minor=64
 */

/******************************************************************************
 * IDE driver configuration options (play with these as desired):
 * 
 * REALLY_SLOW_IO can be defined in ide.c and ide-cd.c, if necessary
 */
#undef REALLY_FAST_IO			/* define if ide ports are perfect */
#define INITIAL_MULT_COUNT	0	/* off=0; on=2,4,8,16,32, etc.. */

#ifndef SUPPORT_SLOW_DATA_PORTS		/* 1 to support slow data ports */
#define SUPPORT_SLOW_DATA_PORTS	1	/* 0 to reduce kernel size */
#endif
#ifndef SUPPORT_VLB_SYNC		/* 1 to support weird 32-bit chips */
#define SUPPORT_VLB_SYNC	1	/* 0 to reduce kernel size */
#endif
#ifndef DISK_RECOVERY_TIME		/* off=0; on=access_delay_time */
#define DISK_RECOVERY_TIME	0	/*  for hardware that needs it */
#endif
#ifndef OK_TO_RESET_CONTROLLER		/* 1 needed for good error recovery */
#define OK_TO_RESET_CONTROLLER	1	/* 0 for use with AH2372A/B interface */
#endif
#ifndef FAKE_FDISK_FOR_EZDRIVE		/* 1 to help linux fdisk with EZDRIVE */
#define FAKE_FDISK_FOR_EZDRIVE 	1	/* 0 to reduce kernel size */
#endif
#ifndef FANCY_STATUS_DUMPS		/* 1 for human-readable drive errors */
#define FANCY_STATUS_DUMPS	1	/* 0 to reduce kernel size */
#endif

#ifdef CONFIG_BLK_DEV_CMD640
#if 0	/* change to 1 when debugging cmd640 problems */
void cmd640_dump_regs (void);
#define CMD640_DUMP_REGS cmd640_dump_regs() /* for debugging cmd640 chipset */
#endif
#endif  /* CONFIG_BLK_DEV_CMD640 */

/*
 * IDE_DRIVE_CMD is used to implement many features of the hdparm utility
 */
#define IDE_DRIVE_CMD		99	/* (magic) undef to reduce kernel size*/

/*
 *  "No user-serviceable parts" beyond this point  :)
 *****************************************************************************/

typedef unsigned char	byte;	/* used everywhere */

/*
 * Probably not wise to fiddle with these
 */
#define ERROR_MAX	8	/* Max read/write errors per sector */
#define ERROR_RESET	3	/* Reset controller every 4th retry */
#define ERROR_RECAL	1	/* Recalibrate every 2nd retry */

/*
 * Ensure that various configuration flags have compatible settings
 */
#ifdef REALLY_SLOW_IO
#undef REALLY_FAST_IO
#endif

#define HWIF(drive)		((ide_hwif_t *)((drive)->hwif))
#define HWGROUP(drive)		((ide_hwgroup_t *)(HWIF(drive)->hwgroup))

/*
 * Definitions for accessing IDE controller registers
 */
#define IDE_NR_PORTS		(10)

#define IDE_DATA_OFFSET		(0)
#define IDE_ERROR_OFFSET	(1)
#define IDE_NSECTOR_OFFSET	(2)
#define IDE_SECTOR_OFFSET	(3)
#define IDE_LCYL_OFFSET		(4)
#define IDE_HCYL_OFFSET		(5)
#define IDE_SELECT_OFFSET	(6)
#define IDE_STATUS_OFFSET	(7)
#define IDE_CONTROL_OFFSET	(8)
#define IDE_IRQ_OFFSET		(9)

#define IDE_FEATURE_OFFSET	IDE_ERROR_OFFSET
#define IDE_COMMAND_OFFSET	IDE_STATUS_OFFSET

#define IDE_DATA_REG		(HWIF(drive)->io_ports[IDE_DATA_OFFSET])
#define IDE_ERROR_REG		(HWIF(drive)->io_ports[IDE_ERROR_OFFSET])
#define IDE_NSECTOR_REG		(HWIF(drive)->io_ports[IDE_NSECTOR_OFFSET])
#define IDE_SECTOR_REG		(HWIF(drive)->io_ports[IDE_SECTOR_OFFSET])
#define IDE_LCYL_REG		(HWIF(drive)->io_ports[IDE_LCYL_OFFSET])
#define IDE_HCYL_REG		(HWIF(drive)->io_ports[IDE_HCYL_OFFSET])
#define IDE_SELECT_REG		(HWIF(drive)->io_ports[IDE_SELECT_OFFSET])
#define IDE_STATUS_REG		(HWIF(drive)->io_ports[IDE_STATUS_OFFSET])
#define IDE_CONTROL_REG		(HWIF(drive)->io_ports[IDE_CONTROL_OFFSET])
#define IDE_IRQ_REG		(HWIF(drive)->io_ports[IDE_IRQ_OFFSET])

#define IDE_FEATURE_REG		IDE_ERROR_REG
#define IDE_COMMAND_REG		IDE_STATUS_REG
#define IDE_ALTSTATUS_REG	IDE_CONTROL_REG
#define IDE_IREASON_REG		IDE_NSECTOR_REG
#define IDE_BCOUNTL_REG		IDE_LCYL_REG
#define IDE_BCOUNTH_REG		IDE_HCYL_REG

#ifdef REALLY_FAST_IO
#define OUT_BYTE(b,p)		outb((b),(p))
#define IN_BYTE(p)		(byte)inb(p)
#else
#define OUT_BYTE(b,p)		outb_p((b),(p))
#define IN_BYTE(p)		(byte)inb_p(p)
#endif /* REALLY_FAST_IO */

#define GET_ERR()		IN_BYTE(IDE_ERROR_REG)
#define GET_STAT()		IN_BYTE(IDE_STATUS_REG)
#define OK_STAT(stat,good,bad)	(((stat)&((good)|(bad)))==(good))
#define BAD_R_STAT		(BUSY_STAT   | ERR_STAT)
#define BAD_W_STAT		(BAD_R_STAT  | WRERR_STAT)
#define BAD_STAT		(BAD_R_STAT  | DRQ_STAT)
#define DRIVE_READY		(READY_STAT  | SEEK_STAT)
#define DATA_READY		(DRQ_STAT)

/*
 * Some more useful definitions
 */
#define IDE_MAJOR_NAME	"ide"	/* the same for all i/f; see also genhd.c */
#define MAJOR_NAME	IDE_MAJOR_NAME
#define PARTN_BITS	6	/* number of minor dev bits for partitions */
#define PARTN_MASK	((1<<PARTN_BITS)-1)	/* a useful bit mask */
#define MAX_DRIVES	2	/* per interface; 2 assumed by lots of code */
#define SECTOR_WORDS	(512 / 4)	/* number of 32bit words per sector */
#define IDE_LARGE_SEEK(b1,b2,t)	(((b1) > (b2) + (t)) || ((b2) > (b1) + (t)))
#define IDE_MIN(a,b)	((a)<(b) ? (a):(b))
#define IDE_MAX(a,b)	((a)>(b) ? (a):(b))

/*
 * Timeouts for various operations:
 */
#define WAIT_DRQ	(5*HZ/100)	/* 50msec - spec allows up to 20ms */
#ifdef CONFIG_APM
#define WAIT_READY	(5*HZ)		/* 5sec - some laptops are very slow */
#else
#define WAIT_READY	(3*HZ/100)	/* 30msec - should be instantaneous */
#endif /* CONFIG_APM */
#define WAIT_PIDENTIFY	(10*HZ)	/* 10sec  - should be less than 3ms (?)
				            if all ATAPI CD is closed at boot */
#define WAIT_WORSTCASE	(30*HZ)	/* 30sec  - worst case when spinning up */
#define WAIT_CMD	(10*HZ)	/* 10sec  - maximum wait for an IRQ to happen */
#define WAIT_MIN_SLEEP	(2*HZ/100)	/* 20msec - minimum sleep time */

#if defined(CONFIG_BLK_DEV_HT6560B) || defined(CONFIG_BLK_DEV_PDC4030) || defined(CONFIG_BLK_DEV_TRM290)
#define SELECT_DRIVE(hwif,drive)				\
{								\
	if (hwif->selectproc)					\
		hwif->selectproc(drive);			\
	else							\
		OUT_BYTE((drive)->select.all, hwif->io_ports[IDE_SELECT_OFFSET]); \
}
#else
#define SELECT_DRIVE(hwif,drive)  OUT_BYTE((drive)->select.all, hwif->io_ports[IDE_SELECT_OFFSET]);
#endif	/* CONFIG_BLK_DEV_HT6560B || CONFIG_BLK_DEV_PDC4030 */
		
/*
 * Now for the data we need to maintain per-drive:  ide_drive_t
 */

#define ide_scsi	0x21
#define ide_disk	0x20
#define ide_cdrom	0x5
#define ide_tape	0x1
#define ide_floppy	0x0

typedef union {
	unsigned all			: 8;	/* all of the bits together */
	struct {
		unsigned set_geometry	: 1;	/* respecify drive geometry */
		unsigned recalibrate	: 1;	/* seek to cyl 0      */
		unsigned set_multmode	: 1;	/* set multmode count */
		unsigned set_tune	: 1;	/* tune interface for drive */
		unsigned reserved	: 4;	/* unused */
		} b;
	} special_t;

typedef struct ide_drive_s {
	struct request		*queue;	/* request queue */
	struct ide_drive_s 	*next;	/* circular list of hwgroup drives */
	unsigned long sleep;		/* sleep until this time */
	unsigned long service_start;	/* time we started last request */
	unsigned long service_time;	/* service time of last request */
	special_t	special;	/* special action flags */
	unsigned present	: 1;	/* drive is physically present */
	unsigned noprobe 	: 1;	/* from:  hdx=noprobe */
	unsigned keep_settings  : 1;	/* restore settings after drive reset */
	unsigned busy		: 1;	/* currently doing revalidate_disk() */
	unsigned removable	: 1;	/* 1 if need to do check_media_change */
	unsigned using_dma	: 1;	/* disk is using dma for read/write */
	unsigned forced_geom	: 1;	/* 1 if hdx=c,h,s was given at boot */
	unsigned unmask		: 1;	/* flag: okay to unmask other irqs */
	unsigned no_unmask	: 1;	/* disallow setting unmask bit */
	unsigned no_io_32bit	: 1;	/* disallow enabling 32bit I/O */
	unsigned nobios		: 1;	/* flag: do not probe bios for drive */
	unsigned slow		: 1;	/* flag: slow data port */
	unsigned autotune	: 2;	/* 1=autotune, 2=noautotune, 0=default */
	unsigned revalidate	: 1;	/* request revalidation */
	unsigned bswap		: 1;	/* flag: byte swap data */
	unsigned dsc_overlap	: 1;	/* flag: DSC overlap */
	unsigned atapi_overlap	: 1;	/* flag: ATAPI overlap (not supported) */
	unsigned nice0		: 1;	/* flag: give obvious excess bandwidth */
	unsigned nice1		: 1;	/* flag: give potential excess bandwidth */
	unsigned nice2		: 1;	/* flag: give a share in our own bandwidth */
#if FAKE_FDISK_FOR_EZDRIVE
	unsigned remap_0_to_1	: 1;	/* flag: partitioned with ezdrive */
#endif /* FAKE_FDISK_FOR_EZDRIVE */
	byte		media;		/* disk, cdrom, tape, floppy, ... */
	select_t	select;		/* basic drive/head select reg value */
	byte		ctl;		/* "normal" value for IDE_CONTROL_REG */
	byte		ready_stat;	/* min status value for drive ready */
	byte		mult_count;	/* current multiple sector setting */
	byte 		mult_req;	/* requested multiple sector setting */
	byte 		tune_req;	/* requested drive tuning setting */
	byte		io_32bit;	/* 0=16-bit, 1=32-bit, 2/3=32bit+sync */
	byte		bad_wstat;	/* used for ignoring WRERR_STAT */
	byte		sect0;		/* offset of first sector for DM6:DDO */
	byte 		usage;		/* current "open()" count for drive */
	byte 		head;		/* "real" number of heads */
	byte		sect;		/* "real" sectors per track */
	/*
	 * HACK: enforce alignment for sake of IDE CDROM driver on
	 * architectures with strict alignment rules.
	 */
	byte		bios_head __attribute__ ((aligned (8)));	/* BIOS/fdisk/LILO number of heads */
	byte		bios_sect __attribute__ ((aligned (8)));	/* BIOS/fdisk/LILO sectors per track */
	unsigned short	bios_cyl;	/* BIOS/fdisk/LILO number of cyls */
	unsigned short	cyl;		/* "real" number of cyls */
	unsigned int	timing_data;	/* for use by tuneproc()'s */
	void		  *hwif;	/* actually (ide_hwif_t *) */
	struct wait_queue *wqueue;	/* used to wait for drive in open() */
	struct hd_driveid *id;		/* drive model identification info */
	struct hd_struct  *part;	/* drive partition table */
	char		name[4];	/* drive name, such as "hda" */
	void 		*driver;	/* (ide_driver_t *) */
	void		*driver_data;	/* extra driver data */
	} ide_drive_t;

/*
 * An ide_dmaproc_t() initiates/aborts DMA read/write operations on a drive.
 *
 * The caller is assumed to have selected the drive and programmed the drive's
 * sector address using CHS or LBA.  All that remains is to prepare for DMA
 * and then issue the actual read/write DMA/PIO command to the drive.
 *
 * Returns 0 if all went well.
 * Returns 1 if DMA read/write could not be started, in which case the caller
 * should either try again later, or revert to PIO for the current request.
 */
typedef enum {	ide_dma_read = 0,	ide_dma_write = 1,
		ide_dma_abort = 2,	ide_dma_check = 3,
		ide_dma_status_bad = 4,	ide_dma_transferred = 5,
		ide_dma_begin = 6,	ide_dma_on = 7,
		ide_dma_off = 8,	ide_dma_off_quietly = 9 }
	ide_dma_action_t;

typedef int (ide_dmaproc_t)(ide_dma_action_t, ide_drive_t *);


/*
 * An ide_tuneproc_t() is used to set the speed of an IDE interface
 * to a particular PIO mode.  The "byte" parameter is used
 * to select the PIO mode by number (0,1,2,3,4,5), and a value of 255
 * indicates that the interface driver should "auto-tune" the PIO mode
 * according to the drive capabilities in drive->id;
 *
 * Not all interface types support tuning, and not all of those
 * support all possible PIO settings.  They may silently ignore
 * or round values as they see fit.
 */
typedef void (ide_tuneproc_t)(ide_drive_t *, byte);

/*
 * This is used to provide HT6560B & PDC4030 & TRM290 interface support.
 */
typedef void (ide_selectproc_t) (ide_drive_t *);

/*
 * hwif_chipset_t is used to keep track of the specific hardware
 * chipset used by each IDE interface, if known.
 */
typedef enum {	ide_unknown,	ide_generic,	ide_pci,
		ide_cmd640,	ide_dtc2278,	ide_ali14xx,
		ide_qd6580,	ide_umc8672,	ide_ht6560b,
		ide_pdc4030,	ide_rz1000,	ide_trm290,
		ide_4drives }
	hwif_chipset_t;

typedef struct hwif_s {
	struct hwif_s	*next;		/* for linked-list in ide_hwgroup_t */
	void		*hwgroup;	/* actually (ide_hwgroup_t *) */
	ide_ioreg_t	io_ports[IDE_NR_PORTS];	/* task file registers */
	ide_drive_t	drives[MAX_DRIVES];	/* drive info */
	struct gendisk	*gd;		/* gendisk structure */
	ide_tuneproc_t	*tuneproc;	/* routine to tune PIO mode for drives */
#if defined(CONFIG_BLK_DEV_HT6560B) || defined(CONFIG_BLK_DEV_PDC4030) || defined(CONFIG_BLK_DEV_TRM290)
	ide_selectproc_t *selectproc;	/* tweaks hardware to select drive */
#endif
	ide_dmaproc_t	*dmaproc;	/* dma read/write/abort routine */
	unsigned long	*dmatable;	/* dma physical region descriptor table */
	struct hwif_s	*mate;		/* other hwif from same PCI chip */
	unsigned short	dma_base;	/* base addr for dma ports (triton) */
	int		irq;		/* our irq number */
	byte		major;		/* our major number */
	char 		name[6];	/* name of interface, eg. "ide0" */
	byte		index;		/* 0 for ide0; 1 for ide1; ... */
	hwif_chipset_t	chipset;	/* sub-module for tuning.. */
	unsigned	noprobe    : 1;	/* don't probe for this interface */
	unsigned	present    : 1;	/* this interface exists */
	unsigned	serialized : 1;	/* serialized operation with mate hwif */
	unsigned	sharing_irq: 1;	/* 1 = sharing irq with another hwif */
#ifdef CONFIG_BLK_DEV_PDC4030
	unsigned	is_pdc4030_2: 1;/* 2nd i/f on pdc4030 */
#endif /* CONFIG_BLK_DEV_PDC4030 */
	unsigned	reset      : 1; /* reset after probe */
	unsigned	pci_port   : 1; /* for dual-port chips: 0=primary, 1=secondary */
#if (DISK_RECOVERY_TIME > 0)
	unsigned long	last_time;	/* time when previous rq was done */
#endif
	} ide_hwif_t;

/*
 *  internal ide interrupt handler type
 */
typedef void (ide_handler_t)(ide_drive_t *);

typedef struct hwgroup_s {
	ide_handler_t		*handler;/* irq handler, if active */
	ide_drive_t		*drive;	/* current drive */
	ide_hwif_t		*hwif;	/* ptr to current hwif in linked-list */
	struct request		*rq;	/* current request */
	struct timer_list	timer;	/* failsafe timer */
	struct request		wrq;	/* local copy of current write rq */
	unsigned long		poll_timeout;	/* timeout value during long polls */
	int			active;	/* set when servicing requests */
	} ide_hwgroup_t;

/*
 * Subdrivers support.
 */
#define IDE_SUBDRIVER_VERSION	1

typedef int	(ide_cleanup_proc)(ide_drive_t *);
typedef void 	(ide_do_request_proc)(ide_drive_t *, struct request *, unsigned long);
typedef void	(ide_end_request_proc)(byte, ide_hwgroup_t *);
typedef int 	(ide_ioctl_proc)(ide_drive_t *, struct inode *, struct file *, unsigned int, unsigned long);
typedef int	(ide_open_proc)(struct inode *, struct file *, ide_drive_t *);
typedef void 	(ide_release_proc)(struct inode *, struct file *, ide_drive_t *);
typedef int	(ide_check_media_change_proc)(ide_drive_t *);
typedef void	(ide_pre_reset_proc)(ide_drive_t *);
typedef unsigned long (ide_capacity_proc)(ide_drive_t *);
typedef void	(ide_special_proc)(ide_drive_t *);

typedef struct ide_driver_s {
	byte				media;
	unsigned busy			: 1;
	unsigned supports_dma		: 1;
	unsigned supports_dsc_overlap	: 1;
	ide_cleanup_proc		*cleanup;
	ide_do_request_proc		*do_request;
	ide_end_request_proc		*end_request;
	ide_ioctl_proc			*ioctl;
	ide_open_proc			*open;
	ide_release_proc		*release;
	ide_check_media_change_proc	*media_change;
	ide_pre_reset_proc		*pre_reset;
	ide_capacity_proc		*capacity;
	ide_special_proc		*special;
	} ide_driver_t;

#define DRIVER(drive)		((ide_driver_t *)((drive)->driver))

/*
 * IDE modules.
 */
#define IDE_CHIPSET_MODULE		0	/* not supported yet */
#define IDE_PROBE_MODULE		1
#define IDE_DRIVER_MODULE		2

typedef int	(ide_module_init_proc)(void);

typedef struct ide_module_s {
	int				type;
	ide_module_init_proc		*init;
	struct ide_module_s		*next;
} ide_module_t;

/*
 * ide_hwifs[] is the master data structure used to keep track
 * of just about everything in ide.c.  Whenever possible, routines
 * should be using pointers to a drive (ide_drive_t *) or 
 * pointers to a hwif (ide_hwif_t *), rather than indexing this
 * structure directly (the allocation/layout may change!).
 *
 */
#ifndef _IDE_C
extern	ide_hwif_t	ide_hwifs[];		/* master data repository */
#endif

/*
 * One final include file, which references some of the data/defns from above
 */
#define IDE_DRIVER	/* "parameter" for blk.h */
#include <linux/blk.h>

/*
 * This is used for (nearly) all data transfers from/to the IDE interface
 */
void ide_input_data (ide_drive_t *drive, void *buffer, unsigned int wcount);
void ide_output_data (ide_drive_t *drive, void *buffer, unsigned int wcount);

/*
 * This is used for (nearly) all ATAPI data transfers from/to the IDE interface
 */
void atapi_input_bytes (ide_drive_t *drive, void *buffer, unsigned int bytecount);
void atapi_output_bytes (ide_drive_t *drive, void *buffer, unsigned int bytecount);

/*
 * This is used on exit from the driver, to designate the next irq handler
 * and also to start the safety timer.
 */
void ide_set_handler (ide_drive_t *drive, ide_handler_t *handler, unsigned int timeout);

/*
 * Error reporting, in human readable form (luxurious, but a memory hog).
 */
byte ide_dump_status (ide_drive_t *drive, const char *msg, byte stat);

/*
 * ide_error() takes action based on the error returned by the controller.
 * The calling function must return afterwards, to restart the request.
 */
void ide_error (ide_drive_t *drive, const char *msg, byte stat);

/*
 * Issue a simple drive command
 * The drive must be selected beforehand.
 */
void ide_cmd(ide_drive_t *drive, byte cmd, byte nsect, ide_handler_t *handler);

/*
 * ide_fixstring() cleans up and (optionally) byte-swaps a text string,
 * removing leading/trailing blanks and compressing internal blanks.
 * It is primarily used to tidy up the model name/number fields as
 * returned by the WIN_[P]IDENTIFY commands.
 */
void ide_fixstring (byte *s, const int bytecount, const int byteswap);

/*
 * This routine busy-waits for the drive status to be not "busy".
 * It then checks the status for all of the "good" bits and none
 * of the "bad" bits, and if all is okay it returns 0.  All other
 * cases return 1 after invoking ide_error() -- caller should return.
 *
 */
int ide_wait_stat (ide_drive_t *drive, byte good, byte bad, unsigned long timeout);

/*
 * This routine is called from the partition-table code in genhd.c
 * to "convert" a drive to a logical geometry with fewer than 1024 cyls.
 *
 * The second parameter, "xparm", determines exactly how the translation
 * will be handled:
 *		 0 = convert to CHS with fewer than 1024 cyls
 *			using the same method as Ontrack DiskManager.
 *		 1 = same as "0", plus offset everything by 63 sectors.
 *		-1 = similar to "0", plus redirect sector 0 to sector 1.
 *		>1 = convert to a CHS geometry with "xparm" heads.
 *
 * Returns 0 if the translation was not possible, if the device was not
 * an IDE disk drive, or if a geometry was "forced" on the commandline.
 * Returns 1 if the geometry translation was successful.
 */
int ide_xlate_1024 (kdev_t, int, const char *);

/*
 * Start a reset operation for an IDE interface.
 * The caller should return immediately after invoking this.
 */
void ide_do_reset (ide_drive_t *);

/*
 * This function is intended to be used prior to invoking ide_do_drive_cmd().
 */
void ide_init_drive_cmd (struct request *rq);

/*
 * "action" parameter type for ide_do_drive_cmd() below.
 */
typedef enum
	{ide_wait,	/* insert rq at end of list, and wait for it */
	 ide_next,	/* insert rq immediately after current request */
	 ide_preempt,	/* insert rq in front of current request */
	 ide_end}	/* insert rq at end of list, but don't wait for it */
 ide_action_t;

/*
 * This function issues a special IDE device request
 * onto the request queue.
 *
 * If action is ide_wait, then then rq is queued at the end of
 * the request queue, and the function sleeps until it has been
 * processed.  This is for use when invoked from an ioctl handler.
 *
 * If action is ide_preempt, then the rq is queued at the head of
 * the request queue, displacing the currently-being-processed
 * request and this function returns immediately without waiting
 * for the new rq to be completed.  This is VERY DANGEROUS, and is
 * intended for careful use by the ATAPI tape/cdrom driver code.
 *
 * If action is ide_next, then the rq is queued immediately after
 * the currently-being-processed-request (if any), and the function
 * returns without waiting for the new rq to be completed.  As above,
 * This is VERY DANGEROUS, and is intended for careful use by the 
 * ATAPI tape/cdrom driver code.
 *
 * If action is ide_end, then the rq is queued at the end of the
 * request queue, and the function returns immediately without waiting
 * for the new rq to be completed. This is again intended for careful
 * use by the ATAPI tape/cdrom driver code.
 */
int ide_do_drive_cmd (ide_drive_t *drive, struct request *rq, ide_action_t action);
 
/*
 * Clean up after success/failure of an explicit drive cmd.
 * stat/err are used only when (HWGROUP(drive)->rq->cmd == IDE_DRIVE_CMD).
 */
void ide_end_drive_cmd (ide_drive_t *drive, byte stat, byte err);

/*
 * ide_system_bus_speed() returns what we think is the system VESA/PCI
 * bus speed (in MHz).  This is used for calculating interface PIO timings.
 * The default is 40 for known PCI systems, 50 otherwise.
 * The "idebus=xx" parameter can be used to override this value.
 */
int ide_system_bus_speed (void);

/*
 * ide_multwrite() transfers a block of up to mcount sectors of data
 * to a drive as part of a disk multwrite operation.
 */
void ide_multwrite (ide_drive_t *drive, unsigned int mcount);

/*
 * ide_stall_queue() can be used by a drive to give excess bandwidth back
 * to the hwgroup by sleeping for timeout jiffies.
 */
void ide_stall_queue (ide_drive_t *drive, unsigned long timeout);

/*
 * ide_get_queue() returns the queue which corresponds to a given device.
 */
struct request **ide_get_queue (kdev_t dev);

void ide_timer_expiry (unsigned long data);
void ide_intr (int irq, void *dev_id, struct pt_regs *regs);
void ide_geninit (struct gendisk *gd);
void do_ide0_request (void);
#if MAX_HWIFS > 1
void do_ide1_request (void);
#endif
#if MAX_HWIFS > 2
void do_ide2_request (void);
#endif
#if MAX_HWIFS > 3
void do_ide3_request (void);
#endif
void ide_init_subdrivers (void);

#ifndef _IDE_C
extern struct file_operations ide_fops[];
#endif

#ifdef _IDE_C
#ifdef CONFIG_BLK_DEV_IDEDISK
int idedisk_init (void);
#endif /* CONFIG_BLK_DEV_IDEDISK */
#ifdef CONFIG_BLK_DEV_IDECD
int ide_cdrom_init (void);
#endif /* CONFIG_BLK_DEV_IDECD */
#ifdef CONFIG_BLK_DEV_IDETAPE
int idetape_init (void);
#endif /* CONFIG_BLK_DEV_IDETAPE */
#ifdef CONFIG_BLK_DEV_IDEFLOPPY
int idefloppy_init (void);
#endif /* CONFIG_BLK_DEV_IDEFLOPPY */
#ifdef CONFIG_BLK_DEV_IDESCSI
int idescsi_init (void);
#endif /* CONFIG_BLK_DEV_IDESCSI */
#endif /* _IDE_C */

int ide_register_module (ide_module_t *module);
void ide_unregister_module (ide_module_t *module);
ide_drive_t *ide_scan_devices (byte media, ide_driver_t *driver, int n);
int ide_register_subdriver (ide_drive_t *drive, ide_driver_t *driver, int version);
int ide_unregister_subdriver (ide_drive_t *drive);

#ifdef CONFIG_BLK_DEV_IDEDMA
int ide_build_dmatable (ide_drive_t *drive);
int ide_dmaproc (ide_dma_action_t func, ide_drive_t *drive);
void ide_setup_dma (ide_hwif_t *hwif, unsigned short dmabase, unsigned int num_ports);
#endif

#ifdef CONFIG_BLK_DEV_IDE
int ideprobe_init (void);
#endif /* CONFIG_BLK_DEV_IDE */

#ifdef CONFIG_BLK_DEV_PDC4030
#include "pdc4030.h"
#define IS_PDC4030_DRIVE (HWIF(drive)->chipset == ide_pdc4030)
#else
#define IS_PDC4030_DRIVE (0)	/* auto-NULLs out pdc4030 code */
#endif /* CONFIG_BLK_DEV_PDC4030 */

#endif /* _IDE_H */
