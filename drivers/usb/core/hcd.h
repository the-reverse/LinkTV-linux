/*
 * Copyright (c) 2001-2002 by David Brownell
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifdef __KERNEL__

/* This file contains declarations of usbcore internals that are mostly
 * used or exposed by Host Controller Drivers.
 */

/*
 * USB Packet IDs (PIDs)
 */
#define USB_PID_UNDEF_0			0xf0
#define USB_PID_OUT			0xe1
#define USB_PID_ACK			0xd2
#define USB_PID_DATA0			0xc3
#define USB_PID_PING			0xb4	/* USB 2.0 */
#define USB_PID_SOF			0xa5
#define USB_PID_NYET			0x96	/* USB 2.0 */
#define USB_PID_DATA2			0x87	/* USB 2.0 */
#define USB_PID_SPLIT			0x78	/* USB 2.0 */
#define USB_PID_IN			0x69
#define USB_PID_NAK			0x5a
#define USB_PID_DATA1			0x4b
#define USB_PID_PREAMBLE		0x3c	/* Token mode */
#define USB_PID_ERR			0x3c	/* USB 2.0: handshake mode */
#define USB_PID_SETUP			0x2d
#define USB_PID_STALL			0x1e
#define USB_PID_MDATA			0x0f	/* USB 2.0 */

/*-------------------------------------------------------------------------*/

/*
 * USB Host Controller Driver (usb_hcd) framework
 *
 * Since "struct usb_bus" is so thin, you can't share much code in it.
 * This framework is a layer over that, and should be more sharable.
 */

/*-------------------------------------------------------------------------*/

struct usb_hcd {	/* usb_bus.hcpriv points to this */

	/*
	 * housekeeping
	 */
	struct usb_bus		self;		/* hcd is-a bus */

	const char		*product_desc;	/* product/vendor string */
	char			irq_descr[24];	/* driver + bus # */

	struct timer_list	rh_timer;	/* drives root-hub polling */
	struct urb		*status_urb;	/* the current status urb */

	/*
	 * hardware info/state
	 */
	const struct hc_driver	*driver;	/* hw-specific hooks */

	/* Flags that need to be manipulated atomically */
	unsigned long		flags;
#define HCD_FLAG_HW_ACCESSIBLE	0x00000001
#define HCD_FLAG_SAW_IRQ	0x00000002

	unsigned		can_wakeup:1;	/* hw supports wakeup? */
	unsigned		remote_wakeup:1;/* sw should use wakeup? */
	unsigned		rh_registered:1;/* is root hub registered? */

	/* The next flag is a stopgap, to be removed when all the HCDs
	 * support the new root-hub polling mechanism. */
	unsigned		uses_new_polling:1;
	unsigned		poll_rh:1;	/* poll for rh status? */
	unsigned		poll_pending:1;	/* status has changed? */

	int			irq;		/* irq allocated */
	void __iomem		*regs;		/* device memory/io */
	u64			rsrc_start;	/* memory/io resource start */
	u64			rsrc_len;	/* memory/io resource length */
	unsigned		power_budget;	/* in mA, 0 = no limit */

#define HCD_BUFFER_POOLS	4
	struct dma_pool		*pool [HCD_BUFFER_POOLS];

	int			state;
#	define	__ACTIVE		0x01
#	define	__SUSPEND		0x04
#	define	__TRANSIENT		0x80

#	define	HC_STATE_HALT		0
#	define	HC_STATE_RUNNING	(__ACTIVE)
#	define	HC_STATE_QUIESCING	(__SUSPEND|__TRANSIENT|__ACTIVE)
#	define	HC_STATE_RESUMING	(__SUSPEND|__TRANSIENT)
#	define	HC_STATE_SUSPENDED	(__SUSPEND)

#define	HC_IS_RUNNING(state) ((state) & __ACTIVE)
#define	HC_IS_SUSPENDED(state) ((state) & __SUSPEND)

	/* more shared queuing code would be good; it should support
	 * smarter scheduling, handle transaction translators, etc;
	 * input size of periodic table to an interrupt scheduler. 
	 * (ohci 32, uhci 1024, ehci 256/512/1024).
	 */

	/* The HC driver's private data is stored at the end of
	 * this structure.
	 */
	unsigned long hcd_priv[0]
			__attribute__ ((aligned (sizeof(unsigned long))));
};

/* 2.4 does this a bit differently ... */
static inline struct usb_bus *hcd_to_bus (struct usb_hcd *hcd)
{
	return &hcd->self;
}


// urb.hcpriv is really hardware-specific

struct hcd_timeout {	/* timeouts we allocate */
	struct list_head	timeout_list;
	struct timer_list	timer;
};

/*-------------------------------------------------------------------------*/

/*
 * FIXME usb_operations should vanish or become hc_driver,
 * when usb_bus and usb_hcd become the same thing.
 */

struct usb_operations {
	int (*get_frame_number) (struct usb_device *usb_dev);
	int (*submit_urb) (struct urb *urb, int mem_flags);
	int (*unlink_urb) (struct urb *urb, int status);

	/* allocate dma-consistent buffer for URB_DMA_NOMAPPING */
	void *(*buffer_alloc)(struct usb_bus *bus, size_t size,
			int mem_flags,
			dma_addr_t *dma);
	void (*buffer_free)(struct usb_bus *bus, size_t size,
			void *addr, dma_addr_t dma);

	void (*disable)(struct usb_device *udev,
			struct usb_host_endpoint *ep);

	/* global suspend/resume of bus */
	int (*hub_suspend)(struct usb_bus *);
	int (*hub_resume)(struct usb_bus *);
};

/* each driver provides one of these, and hardware init support */

struct pt_regs;

struct hc_driver {
	const char	*description;	/* "ehci-hcd" etc */
	const char	*product_desc;	/* product/vendor string */
	size_t		hcd_priv_size;	/* size of private data */

	/* irq handler */
	irqreturn_t	(*irq) (struct usb_hcd *hcd, struct pt_regs *regs);

	int	flags;
#define	HCD_MEMORY	0x0001		/* HC regs use memory (else I/O) */
#define	HCD_USB11	0x0010		/* USB 1.1 */
#define	HCD_USB2	0x0020		/* USB 2.0 */

	/* called to init HCD and root hub */
	int	(*reset) (struct usb_hcd *hcd);
	int	(*start) (struct usb_hcd *hcd);

	/* NOTE:  these suspend/resume calls relate to the HC as
	 * a whole, not just the root hub; they're for PCI bus glue.
	 */
	/* called after suspending the hub, before entering D3 etc */
	int	(*suspend) (struct usb_hcd *hcd, pm_message_t message);

	/* called after entering D0 (etc), before resuming the hub */
	int	(*resume) (struct usb_hcd *hcd);

	/* cleanly make HCD stop writing memory and doing I/O */
	void	(*stop) (struct usb_hcd *hcd);

	/* return current frame number */
	int	(*get_frame_number) (struct usb_hcd *hcd);

	/* manage i/o requests, device state */
	int	(*urb_enqueue) (struct usb_hcd *hcd,
					struct usb_host_endpoint *ep,
					struct urb *urb,
					int mem_flags);
	int	(*urb_dequeue) (struct usb_hcd *hcd, struct urb *urb);

	/* hw synch, freeing endpoint resources that urb_dequeue can't */
	void 	(*endpoint_disable)(struct usb_hcd *hcd,
			struct usb_host_endpoint *ep);

	/* root hub support */
	int		(*hub_status_data) (struct usb_hcd *hcd, char *buf);
	int		(*hub_control) (struct usb_hcd *hcd,
				u16 typeReq, u16 wValue, u16 wIndex,
				char *buf, u16 wLength);
	int		(*hub_suspend)(struct usb_hcd *);
	int		(*hub_resume)(struct usb_hcd *);
	int		(*start_port_reset)(struct usb_hcd *, unsigned port_num);
	void		(*hub_irq_enable)(struct usb_hcd *);
		/* Needed only if port-change IRQs are level-triggered */
};

extern void usb_hcd_giveback_urb (struct usb_hcd *hcd, struct urb *urb, struct pt_regs *regs);

extern struct usb_hcd *usb_create_hcd (const struct hc_driver *driver,
		struct device *dev, char *bus_name);
extern void usb_put_hcd (struct usb_hcd *hcd);
extern int usb_add_hcd(struct usb_hcd *hcd,
		unsigned int irqnum, unsigned long irqflags);
extern void usb_remove_hcd(struct usb_hcd *hcd);

#ifdef CONFIG_REALTEK_VENUS_USB	//cfyeh+ 2005/11/07
extern int usb_hcd_rbus_probe (const struct hc_driver *driver,
				struct platform_device *pdev);
void usb_hcd_rbus_remove (struct usb_hcd *hcd, struct platform_device *pdev);
#ifdef CONFIG_PM
extern int usb_hcd_rbus_suspend (struct usb_hcd *hcd, struct platform_device *pdev,
		                                pm_message_t state, u32 level);
extern int usb_hcd_rbus_resume  (struct usb_hcd *hcd, struct platform_device *pdev,
		                                u32 level);
#endif /* CONFIG_PM */
#else
#ifdef _CONFIG_PCI
struct pci_dev;
struct pci_device_id;
extern int usb_hcd_pci_probe (struct pci_dev *dev,
				const struct pci_device_id *id);
extern void usb_hcd_pci_remove (struct pci_dev *dev);

#ifdef CONFIG_PM
extern int usb_hcd_pci_suspend (struct pci_dev *dev, pm_message_t state);
extern int usb_hcd_pci_resume (struct pci_dev *dev);
#endif /* CONFIG_PM */

#endif /* _CONFIG_PCI */
#endif /* CONFIG_REALTEK_VENUS_USB */	//cfyeh- 2005/11/07

/* pci-ish (pdev null is ok) buffer alloc/mapping support */
int hcd_buffer_create (struct usb_hcd *hcd);
void hcd_buffer_destroy (struct usb_hcd *hcd);

void *hcd_buffer_alloc (struct usb_bus *bus, size_t size,
	int mem_flags, dma_addr_t *dma);
void hcd_buffer_free (struct usb_bus *bus, size_t size,
	void *addr, dma_addr_t dma);

/* generic bus glue, needed for host controllers that don't use PCI */
extern irqreturn_t usb_hcd_irq (int irq, void *__hcd, struct pt_regs *r);

extern void usb_hc_died (struct usb_hcd *hcd);
extern void usb_hcd_poll_rh_status(struct usb_hcd *hcd);

/* -------------------------------------------------------------------------- */

/* Enumeration is only for the hub driver, or HCD virtual root hubs */
extern struct usb_device *usb_alloc_dev(struct usb_device *parent,
					struct usb_bus *, unsigned port);
extern int usb_new_device(struct usb_device *dev);
extern void usb_disconnect(struct usb_device **);

extern int usb_get_configuration(struct usb_device *dev);
extern void usb_destroy_configuration(struct usb_device *dev);

/*-------------------------------------------------------------------------*/

/*
 * HCD Root Hub support
 */

#include "hub.h"

/* (shifted) direction/type/recipient from the USB 2.0 spec, table 9.2 */
#define DeviceRequest \
	((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8)
#define DeviceOutRequest \
	((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_DEVICE)<<8)

#define InterfaceRequest \
	((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)

#define EndpointRequest \
	((USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)
#define EndpointOutRequest \
	((USB_DIR_OUT|USB_TYPE_STANDARD|USB_RECIP_INTERFACE)<<8)

/* class requests from the USB 2.0 hub spec, table 11-15 */
/* GetBusState and SetHubDescriptor are optional, omitted */
#define ClearHubFeature		(0x2000 | USB_REQ_CLEAR_FEATURE)
#define ClearPortFeature	(0x2300 | USB_REQ_CLEAR_FEATURE)
#define GetHubDescriptor	(0xa000 | USB_REQ_GET_DESCRIPTOR)
#define GetHubStatus		(0xa000 | USB_REQ_GET_STATUS)
#define GetPortStatus		(0xa300 | USB_REQ_GET_STATUS)
#define SetHubFeature		(0x2000 | USB_REQ_SET_FEATURE)
#define SetPortFeature		(0x2300 | USB_REQ_SET_FEATURE)


/*-------------------------------------------------------------------------*/

/*
 * Generic bandwidth allocation constants/support
 */
#define FRAME_TIME_USECS	1000L
#define BitTime(bytecount)  (7 * 8 * bytecount / 6)  /* with integer truncation */
		/* Trying not to use worst-case bit-stuffing
                   of (7/6 * 8 * bytecount) = 9.33 * bytecount */
		/* bytecount = data payload byte count */

#define NS_TO_US(ns)	((ns + 500L) / 1000L)
			/* convert & round nanoseconds to microseconds */

extern void usb_claim_bandwidth (struct usb_device *dev, struct urb *urb,
		int bustime, int isoc);
extern void usb_release_bandwidth (struct usb_device *dev, struct urb *urb,
		int isoc);

/*
 * Full/low speed bandwidth allocation constants/support.
 */
#define BW_HOST_DELAY	1000L		/* nanoseconds */
#define BW_HUB_LS_SETUP	333L		/* nanoseconds */
                        /* 4 full-speed bit times (est.) */

#define FRAME_TIME_BITS         12000L		/* frame = 1 millisecond */
#define FRAME_TIME_MAX_BITS_ALLOC	(90L * FRAME_TIME_BITS / 100L)
#define FRAME_TIME_MAX_USECS_ALLOC	(90L * FRAME_TIME_USECS / 100L)

extern int usb_check_bandwidth (struct usb_device *dev, struct urb *urb);

/*
 * Ceiling [nano/micro]seconds (typical) for that many bytes at high speed
 * ISO is a bit less, no ACK ... from USB 2.0 spec, 5.11.3 (and needed
 * to preallocate bandwidth)
 */
#define USB2_HOST_DELAY	5	/* nsec, guess */
#define HS_NSECS(bytes) ( ((55 * 8 * 2083) \
	+ (2083UL * (3 + BitTime(bytes))))/1000 \
	+ USB2_HOST_DELAY)
#define HS_NSECS_ISO(bytes) ( ((38 * 8 * 2083) \
	+ (2083UL * (3 + BitTime(bytes))))/1000 \
	+ USB2_HOST_DELAY)
#define HS_USECS(bytes) NS_TO_US (HS_NSECS(bytes))
#define HS_USECS_ISO(bytes) NS_TO_US (HS_NSECS_ISO(bytes))

extern long usb_calc_bus_time (int speed, int is_input,
			int isoc, int bytecount);

/*-------------------------------------------------------------------------*/

extern struct usb_bus *usb_alloc_bus (struct usb_operations *);

extern int usb_hcd_register_root_hub (struct usb_device *usb_dev,
		struct usb_hcd *hcd);

extern void usb_hcd_resume_root_hub (struct usb_hcd *hcd);

extern void usb_set_device_state(struct usb_device *udev,
		enum usb_device_state new_state);

/*-------------------------------------------------------------------------*/

/* exported only within usbcore */

extern struct list_head usb_bus_list;
extern struct semaphore usb_bus_list_lock;
extern wait_queue_head_t usb_kill_urb_queue;

extern struct usb_bus *usb_bus_get (struct usb_bus *bus);
extern void usb_bus_put (struct usb_bus *bus);

extern void usb_enable_root_hub_irq (struct usb_bus *bus);

extern int usb_find_interface_driver (struct usb_device *dev,
	struct usb_interface *interface);

#define usb_endpoint_out(ep_dir)	(!((ep_dir) & USB_DIR_IN))

/*
 * USB device fs stuff
 */

#ifdef CONFIG_USB_DEVICEFS

/*
 * these are expected to be called from the USB core/hub thread
 * with the kernel lock held
 */
extern void usbfs_add_bus(struct usb_bus *bus);
extern void usbfs_remove_bus(struct usb_bus *bus);
extern void usbfs_add_device(struct usb_device *dev);
extern void usbfs_remove_device(struct usb_device *dev);
extern void usbfs_update_special (void);

extern int usbfs_init(void);
extern void usbfs_cleanup(void);

#else /* CONFIG_USB_DEVICEFS */

static inline void usbfs_add_bus(struct usb_bus *bus) {}
static inline void usbfs_remove_bus(struct usb_bus *bus) {}
static inline void usbfs_add_device(struct usb_device *dev) {}
static inline void usbfs_remove_device(struct usb_device *dev) {}
static inline void usbfs_update_special (void) {}

static inline int usbfs_init(void) { return 0; }
static inline void usbfs_cleanup(void) { }

#endif /* CONFIG_USB_DEVICEFS */

/*-------------------------------------------------------------------------*/

#if defined(CONFIG_USB_MON) || defined(CONFIG_USB_MON_MODULE)

struct usb_mon_operations {
	void (*urb_submit)(struct usb_bus *bus, struct urb *urb);
	void (*urb_submit_error)(struct usb_bus *bus, struct urb *urb, int err);
	void (*urb_complete)(struct usb_bus *bus, struct urb *urb);
	/* void (*urb_unlink)(struct usb_bus *bus, struct urb *urb); */
	void (*bus_add)(struct usb_bus *bus);
	void (*bus_remove)(struct usb_bus *bus);
};

extern struct usb_mon_operations *mon_ops;

static inline void usbmon_urb_submit(struct usb_bus *bus, struct urb *urb)
{
	if (bus->monitored)
		(*mon_ops->urb_submit)(bus, urb);
}

static inline void usbmon_urb_submit_error(struct usb_bus *bus, struct urb *urb,
    int error)
{
	if (bus->monitored)
		(*mon_ops->urb_submit_error)(bus, urb, error);
}

static inline void usbmon_urb_complete(struct usb_bus *bus, struct urb *urb)
{
	if (bus->monitored)
		(*mon_ops->urb_complete)(bus, urb);
}
 
static inline void usbmon_notify_bus_add(struct usb_bus *bus)
{
	if (mon_ops)
		(*mon_ops->bus_add)(bus);
}

static inline void usbmon_notify_bus_remove(struct usb_bus *bus)
{
	if (mon_ops)
		(*mon_ops->bus_remove)(bus);
}

int usb_mon_register(struct usb_mon_operations *ops);
void usb_mon_deregister(void);

#else

static inline void usbmon_urb_submit(struct usb_bus *bus, struct urb *urb) {}
static inline void usbmon_urb_submit_error(struct usb_bus *bus, struct urb *urb,
    int error) {}
static inline void usbmon_urb_complete(struct usb_bus *bus, struct urb *urb) {}
static inline void usbmon_notify_bus_add(struct usb_bus *bus) {}
static inline void usbmon_notify_bus_remove(struct usb_bus *bus) {}

#endif /* CONFIG_USB_MON */

/*-------------------------------------------------------------------------*/

/* hub.h ... DeviceRemovable in 2.4.2-ac11, gone in 2.4.10 */
// bleech -- resurfaced in 2.4.11 or 2.4.12
#define bitmap 	DeviceRemovable


/*-------------------------------------------------------------------------*/

/* random stuff */

#define	RUN_CONTEXT (in_irq () ? "in_irq" \
		: (in_interrupt () ? "in_interrupt" : "can sleep"))


#endif /* __KERNEL__ */

