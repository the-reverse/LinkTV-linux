/*
 * kobject_uevent.h - list of kobject user events that can be generated
 *
 * Copyright (C) 2004 IBM Corp.
 * Copyright (C) 2004 Greg Kroah-Hartman <greg@kroah.com>
 *
 * This file is released under the GPLv2.
 *
 */

#ifndef _KOBJECT_EVENT_H_
#define _KOBJECT_EVENT_H_

#define HOTPLUG_PATH_LEN	256

/* path to the hotplug userspace helper executed on an event */
extern char hotplug_path[];

/*
 * If you add an action here, you must also add the proper string to the
 * lib/kobject_uevent.c file.
 */
typedef int __bitwise kobject_action_t;
enum kobject_action {
	KOBJ_ADD	= (__force kobject_action_t) 0x01,	/* add event, for hotplug */
	KOBJ_REMOVE	= (__force kobject_action_t) 0x02,	/* remove event, for hotplug */
	KOBJ_CHANGE	= (__force kobject_action_t) 0x03,	/* a sysfs attribute file has changed */
	KOBJ_MOUNT	= (__force kobject_action_t) 0x04,	/* mount event for block devices */
	KOBJ_UMOUNT	= (__force kobject_action_t) 0x05,	/* umount event for block devices */
	KOBJ_OFFLINE	= (__force kobject_action_t) 0x06,	/* offline event for hotplug devices */
	KOBJ_ONLINE	= (__force kobject_action_t) 0x07,	/* online event for hotplug devices */
	KOBJ_OVERCUR	= (__force kobject_action_t) 0x08,	/* over-current event for hotplug devices */
	KOBJ_TIER	= (__force kobject_action_t) 0x09,	/* tier event for hotplug devices */
	KOBJ_UNKNOWN	= (__force kobject_action_t) 0x0a,	/* usb unknown device */
	KOBJ_LINKUP	= (__force kobject_action_t) 0x0b,      /* MII link up event */
        KOBJ_LINKDOWN	= (__force kobject_action_t) 0x0c,      /* MII link down event */	
        KOBJ_NET_PBC	= (__force kobject_action_t) 0x0d,      /* MII PBC pressed */	
        KOBJ_USBAI_UP	= (__force kobject_action_t) 0x0e,      /* MII link down event */	
        KOBJ_USBAI_DOWN	= (__force kobject_action_t) 0x0f,      /* MII link down event */	
        KOBJ_USBAO_UP	= (__force kobject_action_t) 0x10,      /* MII link down event */	
        KOBJ_USBAO_DOWN	= (__force kobject_action_t) 0x11,      /* MII link down event */	
        KOBJ_USBKBD_UP	= (__force kobject_action_t) 0x12,
        KOBJ_USBKBD_DOWN	= (__force kobject_action_t) 0x13,
        KOBJ_USBMICE_UP	= (__force kobject_action_t) 0x14,
        KOBJ_USBMICE_DOWN	= (__force kobject_action_t) 0x15,
        KOBJ_SCSI_OFFLINE	= (__force kobject_action_t) 0x16,
        KOBJ_USB_CABLE_BAD	= (__force kobject_action_t) 0x17,
        KOBJ_USBVI_UP	= (__force kobject_action_t) 0x18,
        KOBJ_USBVI_DOWN	= (__force kobject_action_t) 0x19,
        KOBJ_USBVO_UP	= (__force kobject_action_t) 0x1a,
        KOBJ_USBVO_DOWN	= (__force kobject_action_t) 0x1b,
};


#ifdef CONFIG_KOBJECT_UEVENT
int kobject_uevent(struct kobject *kobj,
		   enum kobject_action action,
		   struct attribute *attr);
int kobject_uevent_atomic(struct kobject *kobj,
			  enum kobject_action action,
			  struct attribute *attr);
#else
static inline int kobject_uevent(struct kobject *kobj,
				 enum kobject_action action,
				 struct attribute *attr)
{
	return 0;
}
static inline int kobject_uevent_atomic(struct kobject *kobj,
				        enum kobject_action action,
					struct attribute *attr)
{
	return 0;
}
#endif

#endif
