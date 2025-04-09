/**
 * \file libusb-glue.h
 * Low-level USB interface glue towards libusb.
 *
 * Copyright (C) 2005-2007 Richard A. Low <richard@wentnet.com>
 * Copyright (C) 2005-2007 Linus Walleij <triad@df.lth.se>
 * Copyright (C) 2006-2007 Marcus Meissner
 * Copyright (C) 2007 Ted Bullock
 * Copyright (C) 2008 Chris Bagwell <chris@cnpbagwell.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * Created by Richard Low on 24/12/2005.
 * Modified by Linus Walleij
 *
 */
#ifndef LIBUSB_GLUE_H
#define LIBUSB_GLUE_H

#include <linux/mtp/ptp.h>
#include <linux/usb.h>
#include <linux/mtp/libmtp.h>
#include <linux/mtp/device-flags.h>

#define MTP_USB_MAX_TRANSFER_SIZE           (64 * 1024)

/**
 * USB_MTP_DEV_CLASS_VENDOR_SPEC - macro used to describe a class of usb interfaces
 *
 * This macro is used to create a struct usb_device_id that matches a
 * specific class of interfaces.
 */
#define USB_MTP_DEV_CLASS_VENDOR_SPEC \
    .match_flags = USB_DEVICE_ID_MATCH_DEV_CLASS, .bDeviceClass = (0xFF) 

/**
 * USB_MTP_INTF_VENDOR_SPEC - macro used to describe a class of usb interfaces
 *
 * This macro is used to create a struct usb_device_id that matches a
 * specific class of interfaces.
 */
#define USB_MTP_INTF_CLASS_VENDOR_SPEC \
    .match_flags = USB_DEVICE_ID_MATCH_INT_CLASS, .bInterfaceClass = (0xFF) 

/**
 * Debug macro
 */

#define LIBMTP_USB_DEBUG(format, args...) \
  do { \
    if ((LIBMTP_debug & LIBMTP_DEBUG_USB) != 0) \
        printk("LIBMTP %s[%d]: " format, __FUNCTION__, __LINE__, ##args); \
  } while (0)

#define LIBMTP_USB_DATA(buffer, length, base) \
  do { \
    if ((LIBMTP_debug & LIBMTP_DEBUG_DATA) != 0) \
      data_dump_ascii (buffer, length, base); \
  } while (0)

/*
#define LIBMTP_USB_DEBUG(format, args...) do { \
                                             } while(0)
*/

#define USB_BULK_READ usb_mtp_bulk_read
#define USB_BULK_WRITE usb_mtp_bulk_write

/**
 * Internal USB struct.
 */
typedef struct _PTP_USB PTP_USB;
struct _PTP_USB {
  PTPParams *params;
  // usb_dev_handle* handle;
  // -------------------------------------------------
  struct usb_device *pusb_dev;
  struct usb_interface *pusb_intf;
    
  struct urb  *current_urb;            /* USB requests     */
  struct usb_ctrlrequest  *cr;         /* control requests     */
  unsigned char       *iobuf;      /* I/O buffer       */
  unsigned char       *read_buf;   /* read data buffer */
  // unsigned char       *sensebuf;   /* sense data buffer */
  dma_addr_t      cr_dma;      /* buffer DMA addresses */
  dma_addr_t      iobuf_dma;

  /* mutual exclusion and synchronization structures */
  // struct semaphore    sema;        /* to sleep thread on      */
  struct completion   notify;      /* thread begin/end        */
  wait_queue_head_t   delay_wait;  /* wait during scan, reset */
    
  unsigned int        send_bulk_pipe;  /* cached pipe values */
  unsigned int        recv_bulk_pipe;
  
  unsigned int        send_ctrl_pipe;
  int                 outep_maxpacket;
  unsigned int        recv_ctrl_pipe;
  int                 inep_maxpacket;
  
  unsigned int        recv_intr_pipe;
  u8                  ep_bInterval;
  
  unsigned long flags;
  // -------------------------------------------------
  uint8_t interface;
  /*
  int inep;
  int inep_maxpacket;
  int outep;
  int outep_maxpacket;
  int intep;
  */
  
  int timeout;  
  
  /** File transfer callbacks and counters */
  int callback_active;
  uint64_t current_transfer_total;
  uint64_t current_transfer_complete;
  LIBMTP_progressfunc_t current_transfer_callback;
  void const * current_transfer_callback_data;
  /** Any special device flags, only used internally */
  // LIBMTP_raw_device_t rawdevice;
  LIBMTP_device_entry_t device_entry;  
};

int open_device (int busn, int devn, short force, PTP_USB *ptp_usb, PTPParams *params, struct usb_device **dev);
void dump_usbinfo(PTP_USB *ptp_usb);
const char *get_playlist_extension(PTP_USB *ptp_usb);
void close_device(PTP_USB *ptp_usb, PTPParams *params);
void set_usb_device_timeout(PTP_USB *ptp_usb, int timeout);
void get_usb_device_timeout(PTP_USB *ptp_usb, int *timeout);

/* Flag check macros */
#define FLAG_BROKEN_MTPGETOBJPROPLIST_ALL(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_BROKEN_MTPGETOBJPROPLIST_ALL)
#define FLAG_UNLOAD_DRIVER(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_UNLOAD_DRIVER)
#define FLAG_BROKEN_MTPGETOBJPROPLIST(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_BROKEN_MTPGETOBJPROPLIST)
#define FLAG_NO_ZERO_READS(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_NO_ZERO_READS)
#define FLAG_IRIVER_OGG_ALZHEIMER(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_IRIVER_OGG_ALZHEIMER)
#define FLAG_ONLY_7BIT_FILENAMES(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_ONLY_7BIT_FILENAMES)
#define FLAG_NO_RELEASE_INTERFACE(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_NO_RELEASE_INTERFACE)
#define FLAG_IGNORE_HEADER_ERRORS(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_IGNORE_HEADER_ERRORS)
#define FLAG_BROKEN_SET_OBJECT_PROPLIST(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_BROKEN_SET_OBJECT_PROPLIST)
#define FLAG_OGG_IS_UNKNOWN(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_OGG_IS_UNKNOWN)
#define FLAG_BROKEN_SET_SAMPLE_DIMENSIONS(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_BROKEN_SET_SAMPLE_DIMENSIONS)
#define FLAG_ALWAYS_PROBE_DESCRIPTOR(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_ALWAYS_PROBE_DESCRIPTOR)
#define FLAG_PLAYLIST_SPL_V1(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_PLAYLIST_SPL_V1)
#define FLAG_PLAYLIST_SPL_V2(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_PLAYLIST_SPL_V2)
#define FLAG_PLAYLIST_SPL(a) \
  ((a)->device_entry.device_flags & (DEVICE_FLAG_PLAYLIST_SPL_V1 | DEVICE_FLAG_PLAYLIST_SPL_V2))
#define FLAG_CANNOT_HANDLE_DATEMODIFIED(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_CANNOT_HANDLE_DATEMODIFIED)
#define FLAG_BROKEN_SEND_OBJECT_PROPLIST(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_BROKEN_SEND_OBJECT_PROPLIST)
#define FLAG_BROKEN_BATTERY_LEVEL(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_BROKEN_BATTERY_LEVEL)
#define FLAG_FLAC_IS_UNKNOWN(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_FLAC_IS_UNKNOWN)
#define FLAG_UNIQUE_FILENAMES(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_UNIQUE_FILENAMES)
#define FLAG_SWITCH_MODE_BLACKBERRY(a) \
  ((a)->device_entry.device_flags & DEVICE_FLAG_SWITCH_MODE_BLACKBERRY)

/* connect_first_device return codes */
#define PTP_CD_RC_CONNECTED	0
#define PTP_CD_RC_NO_DEVICES	1
#define PTP_CD_RC_ERROR_CONNECTING	2


#endif //  LIBUSB-GLUE_H
