/* Driver for USB MTP/PTP compliant devices
 *
 * $Id: usb.c,v 1.00 2011/02/25 03:39:43 mdharm Exp $
 *
 * (c) 2011 Reatltek Semiconductor Corp. Ching-Yuh Huang (cyhuang@realtek.com)
 *
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/suspend.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#include <linux/mtp/libmtp.h>
#include <linux/mtp/usb.h>
/*
#include "device-flags.h"
#include "libusb-glue.h"
*/

#include <asm/io.h>

#include <linux/mtp/ptp.h>

#include "util.h"
#include "transport.h"
#include "byteorder.h"

// #include "ptp-pack.c"


/* Some informational data */
MODULE_AUTHOR("Ching-Yuh Huang <cyhuang@realtek.com>");
MODULE_DESCRIPTION("USB MTP/PTP driver for Linux");
// MODULE_LICENSE("GPL");

/* Internal data types */
struct mtpdevice_list_struct
{
    struct usb_device *libusb_device;
    PTPParams *params;
    PTP_USB *ptp_usb;
    uint32_t bus_location;
    struct mtpdevice_list_struct *next;
};
typedef struct mtpdevice_list_struct mtpdevice_list_t;

static const LIBMTP_device_entry_t mtp_device_table[] = 
{
    /* We include an .h file which is shared between us and libgphoto2 */
    #include "music-players.h"
};
static const int mtp_device_table_size = sizeof(mtp_device_table) / sizeof
    (LIBMTP_device_entry_t);

/* Aha, older libusb does not have USB_CLASS_PTP */
#ifndef USB_CLASS_MTP
    #define USB_CLASS_MTP 6
#endif 

/* Default USB timeout length.  This can be overridden as needed
 * but should start with a reasonable value so most common 
 * requests can be completed.  The original value of 4000 was
 * not long enough for large file transfer.  Also, players can
 * spend a bit of time collecting data.  Higher values also
 * make connecting/disconnecting more reliable.
 */
#define USB_TIMEOUT_DEFAULT     (10 * HZ)

/* USB control message data phase direction */
#ifndef USB_DP_HTD
    #define USB_DP_HTD		(0x00 << 7)	/* host to device */
#endif 
#ifndef USB_DP_DTH
    #define USB_DP_DTH		(0x01 << 7)	/* device to host */
#endif 

/* USB Feature selector HALT */
#ifndef USB_FEATURE_HALT
    #define USB_FEATURE_HALT	0x00
#endif 

static DECLARE_MUTEX(mtp_table_mutex);
static LIBMTP_mtpdevice_t *mtp_tables[MTP_MAX_DEVICES];

/* The entries in this table, except for final ones here
 * (USB_CLASS_MTP and the empty entry), correspond,
 * line for line with the entries of us_unsuaul_dev_list[].
 */

static struct usb_device_id mtp_usb_ids[] = 
{
    /* Control/Bulk transport for all SubClass values */
    {
        USB_INTERFACE_INFO(USB_CLASS_MTP, 1, 1)        
    } , 
    {
        USB_MTP_DEV_CLASS_VENDOR_SPEC
    } , 
    {
        USB_MTP_INTF_CLASS_VENDOR_SPEC
    } ,
    /* Terminating entry */
    {}
};

MODULE_DEVICE_TABLE(usb, mtp_usb_ids);

extern uint16_t ptp_exit_recv_memory_handler(PTPDataHandler *, unsigned char **, unsigned long*);
extern uint16_t ptp_init_recv_memory_handler(PTPDataHandler*);
extern uint16_t ptp_init_send_memory_handler(PTPDataHandler *, unsigned char *, unsigned long len);
extern uint16_t ptp_exit_send_memory_handler(PTPDataHandler *handler);

static void usb_mtp_release_resources(PTP_USB *ptp_usb);
static inline void usb_mtp_buffer_free(PTP_USB *ptp_usb);
static LIBMTP_error_number_t mtp_usb_device_open(LIBMTP_mtpdevice_t *mtp_device);
// static void mtp_usb_device_close(LIBMTP_mtpdevice_t *mtp_device);

// ------------------------------------------------------------------------------------------------
// Local functions
// static struct usb_bus* init_usb();
static inline int usb_mtp_buffer_init(PTP_USB *ptp_usb);

static void close_usb(PTP_USB *ptp_usb);
/*
static int find_interface_and_endpoints(struct usb_device *dev,
uint8_t *interface,
int* inep,
int* inep_maxpacket,
int* outep,
int* outep_maxpacket,
int* intep);
 */
static void clear_stall(PTP_USB *ptp_usb);
static int init_ptp_usb(PTPParams *params, PTP_USB *ptp_usb, struct usb_device
                        *dev);
                        
static short ptp_write_func(unsigned long, PTPDataHandler *, void *data,
                            unsigned long*);                       
static short ptp_read_func(unsigned long, PTPDataHandler *, void *data,
                           unsigned long *, int);
                                                        
static int usb_clear_stall_feature(PTP_USB *ptp_usb, int ep);
static int usb_get_endpoint_status(PTP_USB *ptp_usb, int ep, uint16_t *status);

static int usb_mtp_get_pipes(PTP_USB *ptp_usb);

static LIBMTP_error_number_t configure_usb_device( struct usb_device *usb_dev , struct usb_interface *intf,/*LIBMTP_raw_device_t *device,*/
					   PTPParams *params,
					   void **usbinfo);
// ------------------------------------------------------------------------------------------------
/**
 * Get a list of the supported USB devices.
 *
 * The developers depend on users of this library to constantly
 * add in to the list of supported devices. What we need is the
 * device name, USB Vendor ID (VID) and USB Product ID (PID).
 * put this into a bug ticket at the project homepage, please.
 * The VID/PID is used to let e.g. udev lift the device to
 * console userspace access when it's plugged in.
 *
 * @param devices a pointer to a pointer that will hold a device
 *        list after the call to this function, if it was
 *        successful.
 * @param numdevs a pointer to an integer that will hold the number
 *        of devices in the device list if the call was successful.
 * @return 0 if the list was successfull retrieved, any other
 *        value means failure.
 */
int LIBMTP_Get_Supported_Devices_List(LIBMTP_device_entry_t **const devices,
                                      int *const numdevs)
{
    *devices = (LIBMTP_device_entry_t*) &mtp_device_table;
    *numdevs = mtp_device_table_size;
    return 0;
} 

/**
 * Small recursive function to append a new usb_device to the linked list of
 * USB MTP devices
 * @param devlist dynamic linked list of pointers to usb devices with MTP
 *        properties, to be extended with new device.
 * @param newdevice the new device to add.
 * @param bus_location bus for this device.
 * @return an extended array or NULL on failure.
 */
/*
static mtpdevice_list_t *append_to_mtpdevice_list(mtpdevice_list_t *devlist,
struct usb_device *newdevice,
uint32_t bus_location)
{
mtpdevice_list_t *new_list_entry;

new_list_entry = (mtpdevice_list_t *) kmalloc(sizeof(mtpdevice_list_t), GFP_KERNEL);
if (new_list_entry == NULL) {
return NULL;
}
// Fill in USB device, if we *HAVE* to make a copy of the device do it here.
new_list_entry->libusb_device = newdevice;
new_list_entry->bus_location = bus_location;
new_list_entry->next = NULL;

if (devlist == NULL) {
return new_list_entry;
} else {
mtpdevice_list_t *tmp = devlist;
while (tmp->next != NULL) {
tmp = tmp->next;
}
tmp->next = new_list_entry;
}
return devlist;
}
 */

/**
 * Small recursive function to free dynamic memory allocated to the linked list
 * of USB MTP devices
 * @param devlist dynamic linked list of pointers to usb devices with MTP
 * properties.
 * @return nothing
 */
/* 
static void free_mtpdevice_list(mtpdevice_list_t *devlist)
{
mtpdevice_list_t *tmplist = devlist;

if (devlist == NULL)
return;
while (tmplist != NULL) {
mtpdevice_list_t *tmp = tmplist;
tmplist = tmplist->next;
// Do not free() the fields (ptp_usb, params)! These are used elsewhere.
kfree(tmp);
}
return;
}
 */

/**
 * This checks if a device has an MTP descriptor. The descriptor was
 * elaborated about in gPhoto bug 1482084, and some official documentation
 * with no strings attached was published by Microsoft at
 * http://www.microsoft.com/whdc/system/bus/USB/USBFAQ_intermed.mspx#E3HAC
 *
 * @param dev a device struct from libusb.
 * @param dumpfile set to non-NULL to make the descriptors dump out
 *        to this file in human-readable hex so we can scruitinze them.
 * @return 1 if the device is MTP compliant, 0 if not.
 */
static int probe_device_descriptor(PTP_USB *ptp_usb)
{
    struct usb_device *dev = ptp_usb->pusb_dev;
    unsigned char buf[1024], cmd;
    int i;
    int ret;
    /* This is to indicate if we find some vendor interface */
    int found_vendor_spec_interface = 0;

    /*
     * Don't examine devices that are not likely to
     * contain any MTP interface, update this the day
     * you find some weird combination...
     */
    if (!(dev->descriptor.bDeviceClass == USB_CLASS_PER_INTERFACE || dev
        ->descriptor.bDeviceClass == USB_CLASS_COMM || dev
        ->descriptor.bDeviceClass == USB_CLASS_PTP || dev
        ->descriptor.bDeviceClass == USB_CLASS_VENDOR_SPEC))
    {
        return 0;
    } 

    
    /*
     * This sometimes crashes on the j for loop below
     * I think it is because config is NULL yet
     * dev->descriptor.bNumConfigurations > 0
     * this check should stop this
     */
    if (dev->config)
    {
        /*
         * Loop over the device configurations and interfaces. Nokia MTP-capable
         * handsets (possibly others) typically have the string "MTP" in their
         * MTP interface descriptions, that's how they can be detected, before
         * we try the more esoteric "OS descriptors" (below).
         */
        
        for (i = 0; i < dev->descriptor.bNumConfigurations; i++)
        {
            uint8_t j;
            
            for (j = 0; j < dev->config[i].desc.bNumInterfaces; j++)
            {
                int k;
                for (k = 0; k < dev->config[i].interface[j]->num_altsetting; k++)
                {
                    /* Current interface descriptor */
                    struct usb_interface_descriptor *intf = &dev
                        ->config[i].interface[j]->altsetting[k].desc;

                    /*
                     * We only want to probe for the OS descriptor if the
                     * device is USB_CLASS_VENDOR_SPEC or one of the interfaces
                     * in it is, so flag if we find an interface like this.
                     */
                    if (intf->bInterfaceClass == USB_CLASS_VENDOR_SPEC)
                    {
                        found_vendor_spec_interface = 1;
                    } 

                    /*
                     * Check for Still Image Capture class with PIMA 15740 protocol,
                     * also known as PTP
                     */
                    #if 0
                        if (intf->bInterfaceClass == USB_CLASS_PTP && intf
                            ->bInterfaceSubClass == 0x01 && intf
                            ->bInterfaceProtocol == 0x01)
                        {
                            if (dumpfile != NULL)
                            {
                                fprintf(dumpfile, 
                                        "   Found PTP device, check vendor "
                                        "extension...\n");
                            }
                            // This is where we may insert code to open a PTP
                            // session and query the vendor extension ID to see
                            // if it is 0xffffffff, i.e. MTP according to the spec.
                            if (was_mtp_extension)
                            {
                                // usb_close(devh);
                                return 1;
                            }
                        }
                    #endif 

                    /*
                     * Next we search for the MTP substring in the interface name.
                     * For example : "RIM MS/MTP" should work.
                     */
                    
                    /*
                    buf[0] = '\0';        
                      
                    ret = usb_get_string_simple(dev,
                    dev->config[i].interface[j].altsetting[k].iInterface,
                    (char *) buf,
                    1024);
                    
                    if (ret < 3)
                        continue;
                    */
                    ret = usb_get_string(dev, 0,
                    dev->config[i].interface[j]->altsetting[k].desc.iInterface,
                    (char *) buf,
                    1024);
                    
                    if (ret < 3)
                        continue;
                    
                                
                    // printk("buf = %s\n",buf);       

                    if (strstr((char*)buf, "MTP") != NULL)
                    {
                        /*    
                        if (dumpfile != NULL) {
                        fprintf(dumpfile, "Configuration %d, interface %d, altsetting %d:\n", i, j, k);
                        fprintf(dumpfile, "   Interface description contains the string \"MTP\"\n");
                        fprintf(dumpfile, "   Device recognized as MTP, no further probing.\n");
                        }

                        usb_close(devh);
                         */
                        printk("Configuration %d, interface %d, altsetting %d:\n", i, j, k);
                        printk("   Interface description contains the string \"MTP\"\n");
                        printk("   Device recognized as MTP, no further probing.\n");
                         
                        return 1;
                    }
                    #ifdef LIBUSB_HAS_GET_DRIVER_NP
                        {
                            /*
                             * Specifically avoid probing anything else than USB mass storage devices
                             * and non-associated drivers in Linux.
                             */
                            char devname[0x10];

                            devname[0] = '\0';
                            ret = usb_get_driver_np(devh, dev
                                ->config[i].interface[j].altsetting[k].iInterface, devname, sizeof(devname));
                            if (devname[0] != '\0' && strcmp(devname, 
                                "usb-storage"))
                            {
                                LIBMTP_INFO(
                                    "avoid probing device using kernel interface \"%s\"\n", devname);
                                return 0;
                            }
                        }
                    #endif 
                }
            }
        }
    }
    else
    {
        if (dev->descriptor.bNumConfigurations)
            LIBMTP_INFO(
                        "dev->config is NULL in probe_device_descriptor yet dev->descriptor.bNumConfigurations > 0\n");
    }

    /*
     * Only probe for OS descriptor if the device is vendor specific
     * or one of the interfaces found is.
     */
    if (dev->descriptor.bDeviceClass == USB_CLASS_VENDOR_SPEC ||
        found_vendor_spec_interface)
    {

        /* Read the special descriptor */
        ret = usb_get_descriptor(dev, 0x03, 0xee, buf, sizeof(buf));

        // printk("1 : ret = %d\n",ret);
        /*
         * If something failed we're probably stalled to we need
         * to clear the stall off the endpoint and say this is not
         * MTP.
         */
        if (ret < 0)
        {
            /* EP0 is the default control endpoint */
            // usb_clear_halt(dev, 0);
            usb_mtp_clear_halt(ptp_usb,0);
            return 0;
        }

        // Dump it, if requested
        /*
        if (dumpfile != NULL && ret > 0) {
        fprintf(dumpfile, "Microsoft device descriptor 0xee:\n");
        data_dump_ascii(dumpfile, buf, ret, 16);
        }
         */

        /* Check if descriptor length is at least 10 bytes */
        if (ret < 10)
        {
            // usb_close(devh);
            return 0;
        }

        /* Check if this device has a Microsoft Descriptor */
        if (!((buf[2] == 'M') && (buf[4] == 'S') && (buf[6] == 'F') && (buf[8] 
            == 'T')))
        {
            // usb_close(devh);
            return 0;
        }

        /* Check if device responds to control message 1 or if there is an error */
        cmd = buf[16];
        /*
        ret = usb_control_msg (dev,
        USB_ENDPOINT_IN | USB_RECIP_DEVICE | USB_TYPE_VENDOR,
        cmd,
        0,
        4,
        (char *) buf,
        sizeof(buf),
        USB_TIMEOUT_DEFAULT);
         */

        /*
        ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), cmd, USB_DIR_IN |
                              USB_RECIP_DEVICE | USB_TYPE_VENDOR, 0, 4, (char*)
                              buf, sizeof(buf), USB_TIMEOUT_DEFAULT);
                              
        */                      
                              
        ret = usb_mtp_control_msg(ptp_usb, usb_rcvctrlpipe(dev, 0), cmd, USB_DIR_IN |
                              USB_RECIP_DEVICE | USB_TYPE_VENDOR, 0, 4, (char*)
                              buf, sizeof(buf), USB_TIMEOUT_DEFAULT);                      

        // printk("2 : ret = %d\n",ret);
        // Dump it, if requested
        /*
        if (dumpfile != NULL && ret > 0) {
        fprintf(dumpfile, "Microsoft device response to control message 1, CMD 0x%02x:\n", cmd);
        data_dump_ascii(dumpfile, buf, ret, 16);
        }
         */

        /* If this is true, the device either isn't MTP or there was an error */
        if (ret <= 0x15)
        {
            /* TODO: If there was an error, flag it and let the user know somehow */
            /* if(ret == -1) {} */
            // usb_close(devh);
            return 0;
        }

        /* Check if device is MTP or if it is something like a USB Mass Storage
        device with Janus DRM support */
        if ((buf[0x12] != 'M') || (buf[0x13] != 'T') || (buf[0x14] != 'P'))
        {
            // usb_close(devh);
            return 0;
        }

        /* After this point we are probably dealing with an MTP device */

        /*
         * Check if device responds to control message 2, which is
         * the extended device parameters. Most devices will just
         * respond with a copy of the same message as for the first
         * message, some respond with zero-length (which is OK)
         * and some with pure garbage. We're not parsing the result
         * so this is not very important.
         */
        /* 
        ret = usb_control_msg (dev,
        USB_ENDPOINT_IN | USB_RECIP_DEVICE | USB_TYPE_VENDOR,
        cmd,
        0,
        5,
        (char *) buf,
        sizeof(buf),
        USB_TIMEOUT_DEFAULT);
         */
        /* 
        ret = usb_control_msg(dev, usb_rcvctrlpipe(dev, 0), cmd, USB_DIR_IN |
                              USB_RECIP_DEVICE | USB_TYPE_VENDOR, 0, 5, (char*)
                              buf, sizeof(buf), USB_TIMEOUT_DEFAULT);
        */
        
        ret = usb_mtp_control_msg(ptp_usb, usb_rcvctrlpipe(dev, 0), cmd, USB_DIR_IN |
                              USB_RECIP_DEVICE | USB_TYPE_VENDOR, 0, 5, (char*)
                              buf, sizeof(buf), USB_TIMEOUT_DEFAULT);

        // Dump it, if requested
        /*
        if (dumpfile != NULL && ret > 0) {
        fprintf(dumpfile, "Microsoft device response to control message 2, CMD 0x%02x:\n", cmd);
        data_dump_ascii(dumpfile, buf, ret, 16);
        }
         */
        // printk("3 : ret = %d\n",ret);
        
        /* If this is true, the device errored against control message 2 */
        if (ret ==  - 1)
        {
            /* TODO: Implement callback function to let managing program know there
            was a problem, along with description of the problem */
            LIBMTP_ERROR("Potential MTP Device with VendorID:%04x and "
                         "ProductID:%04x encountered an error responding to "
                         "control message 2.\n"
                         "Problems may arrise but continuing\n", dev
                         ->descriptor.idVendor, dev->descriptor.idProduct);
        }
        /*
        else if (dumpfile != NULL && ret == 0) {
        fprintf(dumpfile, "Zero-length response to control message 2 (OK)\n");
        } else if (dumpfile != NULL) {
        fprintf(dumpfile, "Device responds to control message 2 with some data.\n");
        }
         */
        /* Close the USB device handle */
        // usb_close(devh);
        return 1;
    }

    /* Close the USB device handle */
    // usb_close(devh);
    return 0;
}

/**
 * Retrieve the apropriate playlist extension for this
 * device. Rather hacky at the moment. This is probably
 * desired by the managing software, but when creating
 * lists on the device itself you notice certain preferences.
 * @param ptp_usb the USB device to get suggestion for.
 * @return the suggested playlist extension.
 */
const char *get_playlist_extension(PTP_USB *ptp_usb)
{
    struct usb_device *dev = ptp_usb->pusb_dev;
    static char creative_pl_extension[] = ".zpl";
    static char default_pl_extension[] = ".pla";

    // dev = usb_device(ptp_usb->handle);
    if (dev->descriptor.idVendor == 0x041e)
    {
        return creative_pl_extension;
    } return default_pl_extension;
}

/**
 * This routine just dumps out low-level
 * USB information about the current device.
 * @param ptp_usb the USB device to get information from.
 */
void dump_usbinfo(PTP_USB *ptp_usb)
{
    struct usb_device *dev;

    #ifdef LIBUSB_HAS_GET_DRIVER_NP
        char devname[0x10];
        int res;

        devname[0] = '\0';
        res = usb_get_driver_np(ptp_usb->handle, (int)ptp_usb->interface,
                                devname, sizeof(devname));
        if (devname[0] != '\0')
        {
            LIBMTP_INFO("   Using kernel interface \"%s\"\n", devname);
        } 
    #endif 
    dev = ptp_usb->pusb_dev; // usb_device(ptp_usb->handle);
    LIBMTP_INFO("   bcdUSB: %d\n", dev->descriptor.bcdUSB);
    LIBMTP_INFO("   bDeviceClass: %d\n", dev->descriptor.bDeviceClass);
    LIBMTP_INFO("   bDeviceSubClass: %d\n", dev->descriptor.bDeviceSubClass);
    LIBMTP_INFO("   bDeviceProtocol: %d\n", dev->descriptor.bDeviceProtocol);
    LIBMTP_INFO("   idVendor: %04x\n", dev->descriptor.idVendor);
    LIBMTP_INFO("   idProduct: %04x\n", dev->descriptor.idProduct);
    /*
    LIBMTP_INFO("   IN endpoint maxpacket: %d bytes\n", ptp_usb->inep_maxpacket);
    LIBMTP_INFO("   OUT endpoint maxpacket: %d bytes\n", ptp_usb->outep_maxpacket);
     */
    LIBMTP_INFO("   Raw device info:\n");
    // LIBMTP_INFO("      Bus location: %d\n", ptp_usb->rawdevice.bus_location);
    // LIBMTP_INFO("      Device number: %d\n", ptp_usb->rawdevice.devnum);
    LIBMTP_INFO("      Device entry info:\n");
    LIBMTP_INFO("         Vendor: %s\n", ptp_usb->device_entry.vendor);
    LIBMTP_INFO("         Vendor id: 0x%04x\n", ptp_usb->device_entry.vendor_id)
                ;
    LIBMTP_INFO("         Product: %s\n", ptp_usb->device_entry.product);
    LIBMTP_INFO("         Vendor id: 0x%04x\n", ptp_usb
                ->device_entry.product_id);
    LIBMTP_INFO("         Device flags: 0x%08x\n", ptp_usb
                ->device_entry.device_flags);
    (void)probe_device_descriptor(ptp_usb);
}

// --------------------------------------------------------------------------------------------------------
/*
static void mtp_device_dump_mtp_table(void)
{
    int i;
    LIBMTP_mtpdevice_t *mtp_device = NULL;
    
    down(&mtp_table_mutex);
    for (i=0;i<MTP_MAX_DEVICES;i++)
    {
        mtp_device = mtp_tables[i];
        
        if (mtp_device != NULL)
        {               
            printk("========== kobj_name = %s ==========\n",mtp_device->kobj_name); 
            printk("mtp_device->index = %d\n",mtp_device->index); 
            printk("mtp_device->usecount = %d\n",mtp_device->usecount); 
            printk("mtp_device->is_new_probe = %d\n",mtp_device->is_new_probe);
        }    
    }        
    up(&mtp_table_mutex);     
}
*/

static int mtp_device_register(LIBMTP_mtpdevice_t *mtp_device)
{
    int i;
    
    down(&mtp_table_mutex);
    for (i=0;i<MTP_MAX_DEVICES;i++)
    {
        if (mtp_tables[i] == NULL)
        {    
            mtp_device->index = i;
            mtp_device->usecount = 1;
            // Dirty hack for /sbin/hotplug
            mtp_device->is_new_probe = true;            
            mtp_tables[i] =  mtp_device;
            up(&mtp_table_mutex);   
             __module_get(THIS_MODULE);            
            return 0;
        }    
    }        
    up(&mtp_table_mutex); 
    // mtp_device_dump_mtp_table();
    return 1; 
                    
}

static int mtp_device_unregister(LIBMTP_mtpdevice_t *mtp_device)
{
    int ret;
    
    // down(&mtp_table_mutex);

    if (mtp_tables[mtp_device->index] != mtp_device) 
    {
        ret = -ENODEV;
    } else if (mtp_device->usecount) {
        printk(KERN_NOTICE "Removing MTP device #%d with use count %d\n",
               mtp_device->index, mtp_device->usecount);
        ret = -EBUSY;
    } 
    else 
    {
        mtp_tables[mtp_device->index] = NULL;
        module_put(THIS_MODULE);
        ret = 0;        
    }   
    // up(&mtp_table_mutex);           
    return ret;                     
}

LIBMTP_mtpdevice_t *Mtp_Get_Device(LIBMTP_mtpdevice_t *mtp_device,int num)
{
    int i;
    LIBMTP_mtpdevice_t *ret = NULL;
    
    if ((num >= MTP_MAX_DEVICES) || (num < 0))
        return NULL;
     
    down(&mtp_table_mutex);

    if (num == -1) {
        for (i=0; i< MTP_MAX_DEVICES; i++)
            if (mtp_tables[i] == mtp_device)
                ret = mtp_tables[i];
    } else if (num < MTP_MAX_DEVICES) {
        ret = mtp_tables[num];
        if (mtp_device && mtp_device != ret)
            ret = NULL;
    }

    if (ret && !try_module_get(ret->owner))
        ret = NULL;

    if (ret)
    {    
        ret->usecount++;    
        ret->is_new_probe = false;        
    }    
        

    up(&mtp_table_mutex);
    return ret;
}

LIBMTP_mtpdevice_t *Mtp_Get_Device_By_Kobj_Name(char *kobj_name)
{
    int i;
    LIBMTP_mtpdevice_t *ret = NULL;    
    
    if (kobj_name == NULL)
        return NULL;
     
    down(&mtp_table_mutex);

    for (i=0;i<MTP_MAX_DEVICES;i++)
    {
        LIBMTP_mtpdevice_t *mtp_device = NULL;
        
        mtp_device = mtp_tables[i];
        if (mtp_device && (strcmp(mtp_device->kobj_name, kobj_name)==0))
        {              
            // Dirty hack for /sbin/hotplug , kobj_name is not unique.             
            if (mtp_device->is_new_probe == false)
                continue;                
                
            up(&mtp_table_mutex); 
            ret = Mtp_Get_Device(mtp_device,i);                   
            return ret;
        }    
    }    
    
    up(&mtp_table_mutex);
    return NULL;
}

void Mtp_Put_Device(LIBMTP_mtpdevice_t *mtp_device)
{
    // PTP_USB *ptp_usb = (PTP_USB*)mtp_device->usbinfo;
    int c;    

    down(&mtp_table_mutex);
    // printk("%s [Start]: mtp_device->usecount = %d\n",__func__,mtp_device->usecount);
    c = --mtp_device->usecount;            
    up(&mtp_table_mutex);
    BUG_ON(c < 0);
    
    module_put(mtp_device->owner);
    
    down(&mtp_table_mutex);
    
    // printk("%s [End] : mtp_device->usecount %d \n",__func__,mtp_device->usecount);    
    
    if (mtp_device->usecount == 0)
    {           
        mtp_device_unregister(mtp_device);   
        LIBMTP_Release_Device(mtp_device);                                                           
    } 
            
    up(&mtp_table_mutex);        
}

LIBMTP_error_number_t Mtp_Usb_Device_Open(LIBMTP_mtpdevice_t *mtp_device)
{
    return mtp_usb_device_open(mtp_device);    
}

// --------------------------------------------------------------------------------------------------------
static void libusb_glue_debug(PTPParams *params, const char *format, ...)
{ 
    va_list args;

    va_start(args, format);

    if (params->debug_func != NULL)
        params->debug_func(params->data, format, args);
    else
    {
        printk(format, args);
        printk("\n");        
    }
    va_end(args);    
}

static void libusb_glue_error(PTPParams *params, const char *format, ...)
{
 
    va_list args;

    va_start(args, format);

    if (params->error_func != NULL)
        params->error_func(params->data, format, args);
    else
    {
        printk(format, args);
        printk("\n");       
    }
    va_end(args);    
}

// --------------------------------------------------------------------------------------------------------

/*
 * ptp_read_func() and ptp_write_func() are
 * based on same functions usb.c in libgphoto2.
 * Much reading packet logs and having fun with trials and errors
 * reveals that WMP / Windows is probably using an algorithm like this
 * for large transfers:
 *
 * 1. Send the command (0x0c bytes) if headers are split, else, send
 *    command plus sizeof(endpoint) - 0x0c bytes.
 * 2. Send first packet, max size to be sizeof(endpoint) but only when using
 *    split headers. Else goto 3.
 * 3. REPEAT send 0x10000 byte chunks UNTIL remaining bytes < 0x10000
 *    We call 0x10000 CONTEXT_BLOCK_SIZE.
 * 4. Send remaining bytes MOD sizeof(endpoint)
 * 5. Send remaining bytes. If this happens to be exactly sizeof(endpoint)
 *    then also send a zero-length package.
 *
 * Further there is some special quirks to handle zero reads from the
 * device, since some devices can't do them at all due to shortcomings
 * of the USB slave controller in the device.
 */
/* 
#define CONTEXT_BLOCK_SIZE_1	0x3e00
#define CONTEXT_BLOCK_SIZE_2  0x200
#define CONTEXT_BLOCK_SIZE    CONTEXT_BLOCK_SIZE_1+CONTEXT_BLOCK_SIZE_2
*/

static short ptp_read_func(unsigned long size, PTPDataHandler *handler, void
                           *data, unsigned long *readbytes, int readzero)
{
    PTP_USB *ptp_usb = (PTP_USB*) data;
    unsigned long toread = 0;
    int result = 0;
    unsigned long curread = 0;
    unsigned long written;
    unsigned char *bytes;
    int expect_terminator_byte = 0;

    // This is the largest block we'll need to read in.
    // bytes = kmalloc(CONTEXT_BLOCK_SIZE,GFP_KERNEL);
    if (handler->IsMemMapping == true)
    {    
        bytes = (unsigned char *) handler->GetDataPtrFunc(ptp_usb->params,handler->priv);        
    }    
    else
    {    
        bytes =  ptp_usb->read_buf;     
    }    
        
    while (curread < size)
    {

        LIBMTP_USB_DEBUG("Remaining size to read: 0x%04lx bytes\n", size -
                         curread);

        // check equal to condition here                
        if (size - curread < CONTEXT_BLOCK_SIZE)
        {
            // this is the last packet
            toread = size - curread;
            // this is equivalent to zero read for these devices
            // if (readzero && FLAG_NO_ZERO_READS(ptp_usb) && toread % 64 == 0)
            if (readzero && FLAG_NO_ZERO_READS(ptp_usb) && ((toread % ptp_usb->inep_maxpacket) == 0))
            {
                toread += 1;
                expect_terminator_byte = 1;
            }
        }
        else if (curread == 0)
        // we are first packet, but not last packet
            toread = CONTEXT_BLOCK_SIZE_1;
        else if (toread == CONTEXT_BLOCK_SIZE_1)
            toread = CONTEXT_BLOCK_SIZE_2;
        else if (toread == CONTEXT_BLOCK_SIZE_2)
            toread = CONTEXT_BLOCK_SIZE_1;
        else
            LIBMTP_INFO(
                        "unexpected toread size 0x%04x, 0x%04x remaining bytes\n", (unsigned int)toread, (unsigned int)(size - curread));

#if defined(CONFIG_USB_MTP_MAX_TRANSFER_SIZE_ENABLE)
        if (handler->IsMemMapping == true)
        {
            if ((size - curread) >= MTP_USB_MAX_TRANSFER_SIZE)
            {    
                toread = MTP_USB_MAX_TRANSFER_SIZE; 
            }    
            else
            {
                // this is the last packet
                toread = size - curread;
                // this is equivalent to zero read for these devices
                if (readzero && FLAG_NO_ZERO_READS(ptp_usb) && ((toread % ptp_usb->inep_maxpacket) == 0))
                {
                    toread += 1;
                    expect_terminator_byte = 1;
                }    
            }                   
        }
#endif // CONFIG_USB_MTP_MAX_TRANSFER_SIZE_ENABLE        
                                   
        // printk("toread = 0x%x\n",toread);

        LIBMTP_USB_DEBUG("Reading in 0x%04lx bytes\n", toread);

        result = USB_BULK_READ(ptp_usb, ptp_usb->recv_bulk_pipe, bytes,
                               toread, ptp_usb->timeout);                                                             

        LIBMTP_USB_DEBUG("Result of read: 0x%04x\n", result);
                
        if (result < 0)
        {
            return PTP_ERROR_IO;
        }

        LIBMTP_USB_DEBUG("<==USB IN\n");
        if (result == 0)
            LIBMTP_USB_DEBUG("Zero Read\n");
        else
            LIBMTP_USB_DATA(bytes, result, 16);
            
        /*
        if (result >= 1024)    
        {    
            printk("toread = 0x%x , result = 0x%x, bytes = 0x%x\n",toread,result,bytes);
            data_dump_ascii(bytes, 512, 16);
        } 
        */        

        // want to discard extra byte
        if (expect_terminator_byte && result == toread)
        {
            LIBMTP_USB_DEBUG("<==USB IN\nDiscarding extra byte\n");

            result--;
        }
        
        if (handler->IsMemMapping == true)
        {
            handler->SetDataOffsetFunc(ptp_usb->params,handler->priv,result);  
            bytes += result;                    
        }    
        else
        {        
            int putfunc_ret = handler->putfunc(NULL, handler->priv, result, bytes,&written);
            if (putfunc_ret != PTP_RC_OK)
                return putfunc_ret;
        }    

        ptp_usb->current_transfer_complete += result;
        curread += result;         
        
        // Increase counters, call callback
        if (ptp_usb->callback_active)
        {
            if (ptp_usb->current_transfer_complete >= ptp_usb
                ->current_transfer_total)
            {
                // send last update and disable callback.
                ptp_usb->current_transfer_complete = ptp_usb
                    ->current_transfer_total;
                ptp_usb->callback_active = 0;
            }
            if (ptp_usb->current_transfer_callback != NULL)
            {
                int ret;
                ret = ptp_usb->current_transfer_callback(ptp_usb
                    ->current_transfer_complete, ptp_usb
                    ->current_transfer_total, ptp_usb
                    ->current_transfer_callback_data);
                if (ret != 0)
                {
                    return PTP_ERROR_CANCEL;
                }
            }
        }

        if (result < toread)
        /* short reads are common */
            break;
    }
    if (readbytes)
        *readbytes = curread;
    // kfree(bytes);

    // there might be a zero packet waiting for us...
    if (readzero && !FLAG_NO_ZERO_READS(ptp_usb) && curread % ptp_usb
        ->outep_maxpacket == 0)
    {
        char temp;
        int zeroresult = 0;

        LIBMTP_USB_DEBUG("<==USB IN\n");
        LIBMTP_USB_DEBUG("Zero Read\n");

        zeroresult = USB_BULK_READ(ptp_usb, ptp_usb->recv_bulk_pipe, &temp, 0,
                                   ptp_usb->timeout);
        if (zeroresult != 0)
            LIBMTP_INFO(
                        "LIBMTP panic: unable to read in zero packet, response 0x%04x", zeroresult);
    }

    return PTP_RC_OK;
}

static short ptp_write_func(unsigned long size, PTPDataHandler *handler, void
                            *data, unsigned long *written)
{
    PTP_USB *ptp_usb = (PTP_USB*)data;
    unsigned long towrite = 0;
    int result = 0;
    unsigned long curwrite = 0;
    unsigned char *bytes;    

    // This is the largest block we'll need to read in.
    // bytes = kmalloc(CONTEXT_BLOCK_SIZE,GFP_KERNEL);
    bytes = (unsigned char *) handler->GetDataPtrFunc(ptp_usb->params,handler->priv);

    if (!bytes)
    {
        return PTP_ERROR_IO;
    }

    while (curwrite < size)
    {
        unsigned long usbwritten = 0;
        towrite = size - curwrite;
        if (towrite > CONTEXT_BLOCK_SIZE)
        {
            towrite = CONTEXT_BLOCK_SIZE;
        }
        else
        {
            // This magic makes packets the same size that WMP send them.
            if (towrite > ptp_usb->outep_maxpacket && towrite % ptp_usb
                ->outep_maxpacket != 0)
            {
                towrite -= towrite % ptp_usb->outep_maxpacket;
            }
        }
                  
        /*               
        int getfunc_ret = handler->getfunc(NULL, handler->priv, towrite, bytes,
            &towrite);
            
        if (getfunc_ret != PTP_RC_OK)
            return getfunc_ret;
        */       

        while (usbwritten < towrite)
        {
            
            result = USB_BULK_WRITE(ptp_usb, ptp_usb->send_bulk_pipe, ((char*)
                                    bytes + usbwritten), towrite - usbwritten,
                                    ptp_usb->timeout);

            LIBMTP_USB_DEBUG("USB OUT==> (result = %d)\n",result);
            LIBMTP_USB_DATA(bytes + usbwritten, result, 16);                        

            if (result < 0)
            {
                return PTP_ERROR_IO;
            }
            // check for result == 0 perhaps too.
            // Increase counters
            ptp_usb->current_transfer_complete += result;
            curwrite += result;
            usbwritten += result;
            bytes += result;
            handler->SetDataOffsetFunc(ptp_usb->params,handler->priv,result);
        }

        // call callback
        if (ptp_usb->callback_active)
        {
            if (ptp_usb->current_transfer_complete >= ptp_usb->current_transfer_total)
            {
                // send last update and disable callback.
                ptp_usb->current_transfer_complete = ptp_usb
                    ->current_transfer_total;
                ptp_usb->callback_active = 0;
            }
            if (ptp_usb->current_transfer_callback != NULL)
            {
                int ret;
                ret = ptp_usb->current_transfer_callback(ptp_usb
                    ->current_transfer_complete, ptp_usb
                    ->current_transfer_total, ptp_usb
                    ->current_transfer_callback_data);
                if (ret != 0)
                {
                    return PTP_ERROR_CANCEL;
                }
            }
        }

        if (result < towrite)
        /* short writes happen */
            break;
    }
    
    // kfree(bytes);
    if (written)
    {
        *written = curwrite;
    }

    // If this is the last transfer send a zero write if required
    if (ptp_usb->current_transfer_complete >= ptp_usb->current_transfer_total)
    {
        if ((towrite % ptp_usb->outep_maxpacket) == 0)
        {

            LIBMTP_USB_DEBUG("USB OUT==>\n");
            LIBMTP_USB_DEBUG("Zero Write\n");

            result = USB_BULK_WRITE(ptp_usb, ptp_usb->send_bulk_pipe, (char*)
                                    "x", 0, ptp_usb->timeout);
        }
    }

    if (result < 0)
        return PTP_ERROR_IO;
    return PTP_RC_OK;
}

// --------------------------------------------------------------------------------------------------
/* send / receive functions */

uint16_t ptp_usb_sendreq(PTPParams *params, PTPContainer *req)
{
    uint16_t ret;
    // PTPUSBBulkContainer usbreq;    
    PTPDataHandler memhandler;
    unsigned long written = 0;
    unsigned long towrite;

    char txt[256];    

    
    PTP_USB *ptp_usb = (PTP_USB*) params->data;
    PTPUSBBulkContainer *usbreq;
    usbreq = (PTPUSBBulkContainer *) ptp_usb->iobuf;    
        
    // printk("req->Code = 0x%x\n",req->Code);   
                       
    memset(txt,0,256);       
        
    (void)ptp_render_opcode(params, req->Code, sizeof(txt), txt);
    LIBMTP_USB_DEBUG("REQUEST: 0x%04x, %s , sizeof(txt) = %d\n", req->Code, txt , sizeof(txt));    
                                                           
    /* build appropriate USB container */
    usbreq->length = htod32(PTP_USB_BULK_REQ_LEN - (sizeof(uint32_t)*(5-req->Nparam)));
    usbreq->type = htod16(PTP_USB_CONTAINER_COMMAND);
    usbreq->code = htod16(req->Code);
    usbreq->trans_id = htod32(req->Transaction_ID);
    usbreq->payload.params.param1 = htod32(req->Param1);
    usbreq->payload.params.param2 = htod32(req->Param2);
    usbreq->payload.params.param3 = htod32(req->Param3);
    usbreq->payload.params.param4 = htod32(req->Param4);
    usbreq->payload.params.param5 = htod32(req->Param5);
    /* send it to responder */
    towrite = PTP_USB_BULK_REQ_LEN - (sizeof(uint32_t)*(5-req->Nparam));
              
    ptp_init_send_memory_handler(&memhandler, (unsigned char *) usbreq, towrite);                  
    ret = ptp_write_func(towrite, &memhandler, params->data, &written);                
    ptp_exit_send_memory_handler(&memhandler); 
                              
    if (ret != PTP_RC_OK && ret != PTP_ERROR_CANCEL)
    {
        ret = PTP_ERROR_IO;
    }

    if (written != towrite && ret != PTP_ERROR_CANCEL && ret != PTP_ERROR_IO)
    {
        libusb_glue_error(params,"PTP: request code 0x%04x sending req wrote only %ld bytes instead of %d", req->Code, written, towrite);
        ret = PTP_ERROR_IO;
    }
    
    return ret;
}

uint16_t ptp_usb_senddata(PTPParams *params, PTPContainer *ptp, unsigned long
                          size, PTPDataHandler *handler)
{
    uint16_t ret;
    int wlen, datawlen;
    unsigned long written;
    // PTPUSBBulkContainer usbdata;
    uint32_t bytes_left_to_transfer;
    PTPDataHandler memhandler;
       
    PTPUSBBulkContainer *usbdata;
    PTP_USB *ptp_usb = (PTP_USB*) params->data;
    usbdata = (PTPUSBBulkContainer *) ptp_usb->iobuf;
        
    LIBMTP_USB_DEBUG("SEND DATA PHASE\n");

    /* build appropriate USB container */
    usbdata->length = htod32(PTP_USB_BULK_HDR_LEN + size);
    usbdata->type = htod16(PTP_USB_CONTAINER_DATA);
    usbdata->code = htod16(ptp->Code);
    usbdata->trans_id = htod32(ptp->Transaction_ID);

    
    ((PTP_USB*)params->data)->current_transfer_complete = 0;
    ((PTP_USB*)params->data)->current_transfer_total = size + PTP_USB_BULK_HDR_LEN;
    
    /*
    ptp_usb->current_transfer_complete = 0;
    ptp_usb->current_transfer_total = size + PTP_USB_BULK_HDR_LEN;
    */ 

    if (params->split_header_data)
    {
        datawlen = 0;
        wlen = PTP_USB_BULK_HDR_LEN;
    }
    else
    {
        unsigned long gotlen;
        /* For all camera devices. */
        datawlen = (size < PTP_USB_BULK_PAYLOAD_LEN_WRITE) ? size :
                    PTP_USB_BULK_PAYLOAD_LEN_WRITE;
        wlen = PTP_USB_BULK_HDR_LEN + datawlen;

        ret = handler->getfunc(params, handler->priv, datawlen,
                               usbdata->payload.data, &gotlen);
        if (ret != PTP_RC_OK)
            return ret;
        if (gotlen != datawlen)
            return PTP_RC_GeneralError;
    }   
    
    ptp_init_send_memory_handler(&memhandler, (unsigned char*) usbdata, wlen);
    // send first part of data 
    ret = ptp_write_func(wlen, &memhandler, params->data, &written);
    ptp_exit_send_memory_handler(&memhandler);    
    
    // ret = ptp_write_func(ptp_usb ,usbdata ,wlen,&written);
    
    if (ret != PTP_RC_OK)
    {
        return ret;
    }
    if (size <= datawlen)
        return ret;
    /* if everything OK send the rest */
    bytes_left_to_transfer = size - datawlen;
    ret = PTP_RC_OK;
    while (bytes_left_to_transfer > 0)
    {        
        ret = ptp_write_func(bytes_left_to_transfer, handler, params->data, 
                             &written);
        /*
        unsigned long gotlen;
        int getfunc_ret = handler->getfunc(NULL, handler->priv, sizeof(PTPUSBBulkContainer), usbdata, &gotlen);
        if (getfunc_ret != PTP_RC_OK)
            return getfunc_ret;
        
        ret = ptp_write_func(ptp_usb ,usbdata ,bytes_left_to_transfer,&written);                     
        */
        
        if (ret != PTP_RC_OK)
            break;
        if (written == 0)
        {
            ret = PTP_ERROR_IO;
            break;
        }
        bytes_left_to_transfer -= written;
    }
    if (ret != PTP_RC_OK && ret != PTP_ERROR_CANCEL)
        ret = PTP_ERROR_IO;
    return ret;
}

static uint16_t ptp_usb_getpacket(PTPParams *params, PTPUSBBulkContainer
                                  *packet, unsigned long *rlen)
{
    PTPDataHandler memhandler;
    unsigned long readlen;
    unsigned char *x = NULL;
    uint16_t ret;        
    PTP_USB *ptp_usb = (PTP_USB*) params->data;

    /* read the header and potentially the first data */
    if (params->response_packet_size > 0)
    {
        /* If there is a buffered packet, just use it. */
        memcpy(packet, params->response_packet, params->response_packet_size);
        *rlen = params->response_packet_size;
        kfree(params->response_packet);
        params->response_packet = NULL;
        params->response_packet_size = 0;
        /* Here this signifies a "virtual read" */
        return PTP_RC_OK;
    }
    
    ptp_init_recv_memory_handler(&memhandler);       
    memhandler.IsMemMapping = true;
    ret = memhandler.SetDataPtrFunc(params,memhandler.priv,packet,sizeof(PTPUSBBulkContainer));        
          
    if (ret != PTP_RC_OK)
    {    
        return ret;                   
    }      
          
    ret = ptp_read_func(ptp_usb->inep_maxpacket/*PTP_USB_BULK_HS_MAX_PACKET_LEN_READ*/, &memhandler,
                        params->data, rlen, 0);
    ptp_exit_recv_memory_handler(&memhandler, &x, &readlen);
    
    /*
    if (x)
    {
        // printk("packet = 0x%x , x = 0x%x , rlen = %d\n",packet, x,  *rlen);
        memcpy(packet, x,  *rlen);
        kfree(x);
    }    
    */ 
           
    return ret;
}

static uint16_t ptp_usb_get_non_align_data(PTPParams *params, PTPDataHandler *handler, unsigned long size,unsigned long *rlen,int readzero)
{
    PTP_USB *ptp_usb = (PTP_USB*)params->data;
    PTPDataHandler memhandler;    
    unsigned long readsize;
    unsigned char *x = NULL;
    uint16_t ret;
    uint32_t addr = 0;    
    
    ptp_init_recv_memory_handler(&memhandler);    
    memhandler.IsMemMapping = true;
    ret = memhandler.SetDataPtrFunc(params,memhandler.priv,params->session_buf,ptp_usb->inep_maxpacket);
    
    params->obj_session.session_buf_offset = params->obj_session.cur_offset;
    params->obj_session.session_buf_size   = ptp_usb->inep_maxpacket;
            
    if (ret != PTP_RC_OK)
    {    
        return ret;                   
    } 
       
    ret = ptp_read_func(ptp_usb->inep_maxpacket, &memhandler,params->data, rlen, readzero);
    ptp_exit_recv_memory_handler(&memhandler, &x, &readsize);
    
    
    if (ret == PTP_RC_OK)
    {   
        if (handler->IsMemMapping)
        {    
            addr = (uint32_t) handler->GetDataPtrFunc(params,handler->priv);
        }
         
        ret = handler->putfunc(params, handler->priv, size, params->session_buf, &readsize);         
        
        if ((size > 0) && (handler->IsMemMapping)) 
        {         
            // printk("addr = 0x%x\n",addr);       
            dma_cache_wback(addr, size);    
        }
                
    }    
    
    
    /*
    if (x)
    {        
        // printk("x = 0x%x , rlen = %d , size = %d , params->session_buf =  0x%x\n",x, *rlen ,  size, params->session_buf);
        // memcpy(buf, x,  size);
        if (ret == PTP_RC_OK)
        {                
            memcpy(params->session_buf,x,*rlen); 
            ret = handler->putfunc(params, handler->priv, size, x, &readsize);                                    
        }    

        kfree(x);
    }       
    */
            
    return ret;
}

uint16_t ptp_usb_getdata(PTPParams *params, PTPContainer *ptp, PTPDataHandler
                         *handler)
{
    uint16_t ret = 0;
    PTP_USB *ptp_usb = (PTP_USB*)params->data;
    PTPUSBBulkContainer usbdata;      
    unsigned long written , session_len,align_len = 0;    
    int putfunc_ret;
    int readzero = 1,align_readzero = 1;
    uint32_t packet_addr = 0;
       
    LIBMTP_USB_DEBUG("GET DATA PHASE\n");    
    
    memset(&usbdata, 0, sizeof(usbdata));            
    
    do
    {
        unsigned long len, rlen;
        
        if (handler->mode == PtpDataHandlerModeSession)
        {                            
            if (params->obj_session.cur_offset > 0)
            {     
                readzero = 1;    
                align_readzero = 1;
                                                       
                session_len = params->obj_session.req_size;
                // Not EOF.
                if ((params->obj_session.cur_offset + session_len) < params->obj_session.obj_size)
                {    
                    readzero = 0; 
                    align_readzero = 0;
                }  
                else
                {
                    session_len =  params->obj_session.obj_size - params->obj_session.cur_offset;   
                }      
                    
                align_len = session_len % ptp_usb->inep_maxpacket;
            
                if (align_len != 0)
                {
                    session_len -= align_len;  
                    align_readzero = 0;                    
                }    
                
                if (session_len > 0)
                {    
                    ret = ptp_read_func(session_len, handler,params->data, &rlen, align_readzero);
                
                    if (ret == PTP_RC_OK)
                        params->obj_session.cur_offset += rlen;
                }    
                    
                if (align_len != 0)
                {
                    ret = ptp_usb_get_non_align_data(params,handler,align_len,&rlen,readzero);
                    
                    if (ret == PTP_RC_OK)
                        params->obj_session.cur_offset += rlen;
                }           
                                            
                return ret;                    
            }
                                            
        } // PtpDataHandlerModeSession  
        
        ret = ptp_usb_getpacket(params, &usbdata, &rlen);                     
        
        // printk("%s : ret = 0x%x , rlen = 0x%x\n",__func__,ret,rlen);
                
        if (ret != PTP_RC_OK)
        {            
            ret = PTP_ERROR_IO;
            break;
        }
        
        if (dtoh16(usbdata.type) != PTP_USB_CONTAINER_DATA)
        {
            ret = PTP_ERROR_DATA_EXPECTED;
            break;
        }
        
        if (dtoh16(usbdata.code) != ptp->Code)
        {
            if (FLAG_IGNORE_HEADER_ERRORS(ptp_usb))
            {
                libusb_glue_debug(params, 
                                  "ptp2/ptp_usb_getdata: detected a broken "
                                  "PTP header, code field insane, expect problems! (But continuing)");
                // Repair the header, so it won't wreak more havoc, don't just ignore it.
                // Typically these two fields will be broken.
                usbdata.code = htod16(ptp->Code);
                usbdata.trans_id = htod32(ptp->Transaction_ID);
                ret = PTP_RC_OK;
            }
            else
            {
                ret = dtoh16(usbdata.code);
                // This filters entirely insane garbage return codes, but still
                // makes it possible to return error codes in the code field when
                // getting data. It appears Windows ignores the contents of this
                // field entirely.
                if (ret < PTP_RC_Undefined || ret >
                    PTP_RC_SpecificationOfDestinationUnsupported)
                {
                    libusb_glue_debug(params, 
                                      "ptp2/ptp_usb_getdata: detected a broken ""PTP header, code field insane.");
                    ret = PTP_ERROR_IO;
                }
                break;
            }
        }                
                        
        if (usbdata.length == 0xffffffffU)
        {            
            /* Copy first part of data to 'data' */
            putfunc_ret = handler->putfunc(params, handler->priv, rlen -
                PTP_USB_BULK_HDR_LEN, usbdata.payload.data, &written);
            if (putfunc_ret != PTP_RC_OK)
                return putfunc_ret;

            if (handler->mode == PtpDataHandlerModeSession)
            {    
                params->obj_session.cur_offset += written;  
                params->obj_session.obj_size += written;  
            }

            /* stuff data directly to passed data handler */
            while (1)
            {
                unsigned long readdata;
                uint16_t xret;

                xret = ptp_read_func(ptp_usb->inep_maxpacket/* PTP_USB_BULK_HS_MAX_PACKET_LEN_READ */,
                                     handler, params->data, &readdata, 0);
                if (xret != PTP_RC_OK)
                    return xret;
                    
                if (handler->mode == PtpDataHandlerModeSession)
                {    
                    params->obj_session.cur_offset += readdata;  
                    params->obj_session.obj_size += readdata;  
                }    
                    
                if (readdata < ptp_usb->inep_maxpacket/*PTP_USB_BULK_HS_MAX_PACKET_LEN_READ*/)
                    break;
            }
            return PTP_RC_OK;
        }
        
        if (rlen > dtoh32(usbdata.length))
        {
            /*
             * Buffer the surplus response packet if it is >=
             * PTP_USB_BULK_HDR_LEN
             * (i.e. it is probably an entire package)
             * else discard it as erroneous surplus data.
             * This will even work if more than 2 packets appear
             * in the same transaction, they will just be handled
             * iteratively.
             *
             * Marcus observed stray bytes on iRiver devices;
             * these are still discarded.
             */
            unsigned int packlen = dtoh32(usbdata.length);
            unsigned int surplen = rlen - packlen;

            if (surplen >= PTP_USB_BULK_HDR_LEN)
            {
                params->response_packet = kmalloc(surplen, GFP_KERNEL);                
                memcpy(params->response_packet, (uint8_t*) &usbdata + packlen,
                       surplen);
                                                      
                params->response_packet_size = surplen;
                /* Ignore reading one extra byte if device flags have been set */
            }
            else if (!FLAG_NO_ZERO_READS(ptp_usb) && (rlen - dtoh32
                     (usbdata.length) == 1))
            {
                libusb_glue_debug(params, 
                                  "ptp2/ptp_usb_getdata: read %d bytes "
                                  "too much, expect problems!", rlen - dtoh32
                                  (usbdata.length));
            }
            rlen = packlen;
        }

        /* For most PTP devices rlen is ptp_usb->inep_maxpacket == sizeof(usbdata)
         * here. For MTP devices splitting header and data it might
         * be 12.
         */
        /* Evaluate full data length. */
        len = dtoh32(usbdata.length) - PTP_USB_BULK_HDR_LEN;
                        
        session_len = rlen - PTP_USB_BULK_HDR_LEN;
        
        if (handler->mode == PtpDataHandlerModeSession)
        {    
            params->obj_session.obj_size = len;
            
            if (session_len > params->obj_session.req_size)
                session_len = params->obj_session.req_size;    
        }    

        /* autodetect split header/data MTP devices */
        if (dtoh32(usbdata.length) > 12 && (rlen == 12))
        {    
            params->split_header_data = 1;
            params->obj_session.split_header_data = 1;
        }                    
        
        if (handler->IsMemMapping)
        {    
            packet_addr = (uint32_t) handler->GetDataPtrFunc(params,handler->priv);
        }
        
        /* Copy first part of data to 'data' */
        if (session_len > 0)
        {    
            putfunc_ret = handler->putfunc(params, handler->priv, session_len,
                                       usbdata.payload.data, &written);
                                               
            if (putfunc_ret != PTP_RC_OK)
                return putfunc_ret;
                                
            if (handler->IsMemMapping) 
            {         
                // printk("packet_addr = 0x%x\n",packet_addr);       
                dma_cache_wback(packet_addr, session_len);    
            }    
        }        
                                
        if ((handler->mode == PtpDataHandlerModeSession))
        {                                
            params->obj_session.cur_offset = rlen - PTP_USB_BULK_HDR_LEN;                                
       
            if (session_len == params->obj_session.req_size)
            {  
                params->obj_session.session_buf_offset = 0;
                params->obj_session.session_buf_size   = rlen - PTP_USB_BULK_HDR_LEN;
                memcpy(params->session_buf,usbdata.payload.data,params->obj_session.session_buf_size); 
                return PTP_RC_OK;
            }    
        }    
        
        
        /*
        if (FLAG_NO_ZERO_READS(ptp_usb) && len + PTP_USB_BULK_HDR_LEN ==
            PTP_USB_BULK_HS_MAX_PACKET_LEN_READ)
        */   
        
        if (FLAG_NO_ZERO_READS(ptp_usb) && (len + PTP_USB_BULK_HDR_LEN == ptp_usb->inep_maxpacket)) 
        {

            LIBMTP_USB_DEBUG("Reading in extra terminating byte\n");

            // need to read in extra byte and discard it
            int result = 0;
            char byte = 0;
            result = USB_BULK_READ(ptp_usb, ptp_usb->recv_bulk_pipe, &byte, 1,
                                   ptp_usb->timeout);

            if (result != 1)
                LIBMTP_INFO(
                            "Could not read in extra byte for PTP_USB_BULK_HS_MAX_PACKET_LEN_READ long file, return value 0x%04x\n", result);
        }
        /*
        else if (len + PTP_USB_BULK_HDR_LEN ==
                 PTP_USB_BULK_HS_MAX_PACKET_LEN_READ && params->split_header_data == 0)
        */       
        else if ((len + PTP_USB_BULK_HDR_LEN == ptp_usb->inep_maxpacket) && (params->split_header_data == 0)) 
        {
            int zeroresult = 0;
            char zerobyte = 0;


            LIBMTP_INFO("Reading in zero packet after header\n");

            zeroresult = USB_BULK_READ(ptp_usb, ptp_usb->recv_bulk_pipe, 
                                       &zerobyte, 0, ptp_usb->timeout);

            if (zeroresult != 0)
                LIBMTP_INFO(
                            "LIBMTP panic: unable to read in zero packet, response 0x%04x", zeroresult);
        }

        // printk("len = %d , rlen = %d\n",len ,rlen);
        /* Is that all of data? */
        if (len + PTP_USB_BULK_HDR_LEN <= rlen)
        {
            break;
        }

        session_len = len - (rlen - PTP_USB_BULK_HDR_LEN); 
        readzero = 1;       
        
        if (handler->mode == PtpDataHandlerModeSession)
        {            
             align_readzero = 1;
                                                           
            if (session_len > params->obj_session.req_size)
            {    
                session_len = params->obj_session.req_size - (rlen - PTP_USB_BULK_HDR_LEN);   
            }    
                
            // Not Eof.                
            if ((params->obj_session.cur_offset + session_len) < params->obj_session.obj_size)
            {    
                readzero = 0; 
                align_readzero = 0;
            }    
            else
            {
                session_len =  params->obj_session.obj_size - params->obj_session.cur_offset;   
            }
            
            align_len = session_len % ptp_usb->inep_maxpacket;
            
            if (align_len != 0)
            {
                session_len -= align_len;  
                align_readzero = 0;        
            }                                    
        } 
        
        rlen = 0; 
        if (session_len > 0)
        {    
            ret = ptp_read_func(session_len, handler,params->data, &rlen, align_readzero);  
                 
            // printk("%s : session_len = %d ,len = %d , rlen = %d , ret = 0x%x\n",__func__,session_len,len ,rlen,ret);
        
            if (ret != PTP_RC_OK)
            {
                break;
            }
        }                    
                                      
        if (handler->mode == PtpDataHandlerModeSession)
        {
            params->obj_session.cur_offset += rlen; 
            
            if (align_len != 0)
            {    
                ret = ptp_usb_get_non_align_data(params,handler,align_len,&rlen,readzero);  
            
                if (ret != PTP_RC_OK)
                {
                    break;
                }  
                            
                params->obj_session.cur_offset += rlen; 
            }    
        }                                
        
    }
    while (0);
    
    return ret;
}

uint16_t ptp_usb_getresp(PTPParams *params, PTPContainer *resp)
{
    uint16_t ret;
    unsigned long rlen;
    PTPUSBBulkContainer usbresp;
    PTP_USB *ptp_usb = (PTP_USB*)(params->data);

    LIBMTP_USB_DEBUG("RESPONSE: ");

    memset(&usbresp, 0, sizeof(usbresp));
    /* read response, it should never be longer than sizeof(usbresp) */   
    ret = ptp_usb_getpacket(params, &usbresp, &rlen);
    
    // Fix for bevahiour reported by Scott Snyder on Samsung YP-U3. The player
    // sends a packet containing just zeroes of length 2 (up to 4 has been seen too)
    // after a NULL packet when it should send the response. This code ignores
    // such illegal packets.
    while (ret == PTP_RC_OK && rlen < PTP_USB_BULK_HDR_LEN && usbresp.length ==
           0)
    {
        libusb_glue_debug(params, "ptp_usb_getresp: detected short response "
                          "of %d bytes, expect problems! (re-reading "
                          "response), rlen");
        ret = ptp_usb_getpacket(params, &usbresp, &rlen);
    }

    if (ret != PTP_RC_OK)
    {
        ret = PTP_ERROR_IO;
    }
    else if (dtoh16(usbresp.type) != PTP_USB_CONTAINER_RESPONSE)
    {
        ret = PTP_ERROR_RESP_EXPECTED;
    }
    else if (dtoh16(usbresp.code) != resp->Code)
    {
        ret = dtoh16(usbresp.code);
    }

    LIBMTP_USB_DEBUG("%04x\n", ret);

    if (ret != PTP_RC_OK)
    {
        /*
        libusb_glue_error (params,
        "PTP: request code 0x%04x getting resp error 0x%04x",
        resp->Code, ret);
        */
        printk("PTP: request code 0x%04x getting resp error 0x%04x\n",resp->Code, ret);
        return ret;
    }
    /* build an appropriate PTPContainer */
    resp->Code = dtoh16(usbresp.code);
    resp->SessionID = params->session_id;
    resp->Transaction_ID = dtoh32(usbresp.trans_id);
    if (FLAG_IGNORE_HEADER_ERRORS(ptp_usb))
    {
        if (resp->Transaction_ID != params->transaction_id - 1)
        {
            libusb_glue_debug(params, "ptp_usb_getresp: detected a broken "
                              "PTP header, transaction ID insane, expect "
                              "problems! (But continuing)");
            // Repair the header, so it won't wreak more havoc.
            resp->Transaction_ID = params->transaction_id - 1;
        }
    }
    resp->Param1 = dtoh32(usbresp.payload.params.param1);
    resp->Param2 = dtoh32(usbresp.payload.params.param2);
    resp->Param3 = dtoh32(usbresp.payload.params.param3);
    resp->Param4 = dtoh32(usbresp.payload.params.param4);
    resp->Param5 = dtoh32(usbresp.payload.params.param5);
    return ret;
}

// --------------------------------------------------------------------------------------------------
/* Event handling functions */

/* PTP Events wait for or check mode */
#define PTP_EVENT_CHECK			0x0000	/* waits for */
#define PTP_EVENT_CHECK_FAST		0x0001	/* checks */

static inline uint16_t ptp_usb_event(PTPParams *params, PTPContainer *event,
                                     int wait)
{
    uint16_t ret;
    int result;
    unsigned long rlen;
    PTPUSBEventContainer usbevent;
    PTP_USB *ptp_usb = (PTP_USB*)(params->data);

    memset(&usbevent, 0, sizeof(usbevent));

    if ((params == NULL) || (event == NULL))
        return PTP_ERROR_BADPARAM;
    ret = PTP_RC_OK;
    switch (wait)
    {
        case PTP_EVENT_CHECK:
            result = USB_BULK_READ(ptp_usb, ptp_usb->recv_intr_pipe, (char*)
                                   &usbevent, sizeof(usbevent), ptp_usb
                                   ->timeout);
            if (result == 0)
                result = USB_BULK_READ(ptp_usb, ptp_usb->recv_intr_pipe, (char*)
                                       &usbevent, sizeof(usbevent), ptp_usb
                                       ->timeout);
            if (result < 0)
                ret = PTP_ERROR_IO;
            break;
        case PTP_EVENT_CHECK_FAST:
            result = USB_BULK_READ(ptp_usb, ptp_usb->recv_intr_pipe, (char*)
                                   &usbevent, sizeof(usbevent), ptp_usb
                                   ->timeout);
            if (result == 0)
                result = USB_BULK_READ(ptp_usb, ptp_usb->recv_intr_pipe, (char*)
                                       &usbevent, sizeof(usbevent), ptp_usb
                                       ->timeout);
            if (result < 0)
                ret = PTP_ERROR_IO;
            break;
        default:
            ret = PTP_ERROR_BADPARAM;
            break;
    }
    if (ret != PTP_RC_OK)
    {
        libusb_glue_error(params, "PTP: reading event an error 0x%04x occurred",
                          ret);
        return PTP_ERROR_IO;
    }
    rlen = result;
    if (rlen < 8)
    {
        libusb_glue_error(params, 
                          "PTP: reading event an short read of %ld bytes occurred", rlen);
        return PTP_ERROR_IO;
    }
    /* if we read anything over interrupt endpoint it must be an event */
    /* build an appropriate PTPContainer */
    event->Code = dtoh16(usbevent.code);
    event->SessionID = params->session_id;
    event->Transaction_ID = dtoh32(usbevent.trans_id);
    event->Param1 = dtoh32(usbevent.param1);
    event->Param2 = dtoh32(usbevent.param2);
    event->Param3 = dtoh32(usbevent.param3);
    return ret;
}

uint16_t ptp_usb_event_check(PTPParams *params, PTPContainer *event)
{

    return ptp_usb_event(params, event, PTP_EVENT_CHECK_FAST);
}

uint16_t ptp_usb_event_wait(PTPParams *params, PTPContainer *event)
{

    return ptp_usb_event(params, event, PTP_EVENT_CHECK);
}

uint16_t ptp_usb_control_cancel_request(PTPParams *params, uint32_t
                                        transactionid)
{
    PTP_USB *ptp_usb = (PTP_USB*)(params->data);
    int ret;
    unsigned char buffer[6];

    htod16a(&buffer[0], PTP_EC_CancelTransaction);
    htod32a(&buffer[2], transactionid);
    ret = usb_mtp_control_msg(ptp_usb, usb_sndctrlpipe(ptp_usb->pusb_dev,
                          0), 0x64, USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                          0x0000, 0x0000, (char*)buffer, sizeof(buffer),
                          ptp_usb->timeout);
    // Delay 200 ms                          
    mdelay(200);
                              
    if (ret < sizeof(buffer))
        return PTP_ERROR_IO;
    return PTP_RC_OK;
}

static int init_ptp_usb(PTPParams *params, PTP_USB *ptp_usb, struct usb_device
                        *dev)
{    
    char buf[255];
   

    ptp_usb->params = params;

    usb_mtp_buffer_init(ptp_usb);
    
    params->sendreq_func = ptp_usb_sendreq;
    params->senddata_func = ptp_usb_senddata;
    params->getresp_func = ptp_usb_getresp;
    params->getdata_func = ptp_usb_getdata;
    params->cancelreq_func = ptp_usb_control_cancel_request;
    params->data = ptp_usb;
    params->transaction_id = 0;
    /*
     * This is hardcoded here since we have no devices whatsoever that are BE.
     * Change this the day we run into our first BE device (if ever).
     */
    params->byteorder = PTP_DL_LE;

    ptp_usb->timeout = USB_TIMEOUT_DEFAULT;
    
    if (dev)
    {

        // FIXME : Discovered in the Barry project
        // kernels >= 2.6.28 don't set the interface the same way as
        // previous versions did, and the Blackberry gets confused
        // if it isn't explicitly set
        // See above, same issue with pthreads means that if this fails it is not
        // fatal
        // However, this causes problems on Macs so disable here

        if (FLAG_SWITCH_MODE_BLACKBERRY(ptp_usb))
        {
            int ret;

            // FIXME : Only for BlackBerry Storm
            // What does it mean? Maybe switch mode...
            // This first control message is absolutely necessary
            udelay(1000);

            ret = usb_mtp_control_msg(ptp_usb, usb_rcvctrlpipe(dev, 0), 0xaa,
                                  USB_TYPE_VENDOR | USB_RECIP_DEVICE |
                                  USB_DIR_IN, 0x00, 0x04, buf, 0x40, 1000);
            LIBMTP_USB_DEBUG("BlackBerry magic part 1:\n");
            LIBMTP_USB_DATA(buf, ret, 16);

            udelay(1000);
            // This control message is unnecessary
            ret = usb_mtp_control_msg(ptp_usb, usb_rcvctrlpipe(dev, 0), 0xa5,
                                  USB_TYPE_VENDOR | USB_RECIP_DEVICE |
                                  USB_DIR_IN, 0x00, 0x01, buf, 0x02, 1000);
            LIBMTP_USB_DEBUG("BlackBerry magic part 2:\n");
            LIBMTP_USB_DATA(buf, ret, 16);

            udelay(1000);
            // This control message is unnecessary
            ret = usb_mtp_control_msg(ptp_usb, usb_rcvctrlpipe(dev, 0), 0xa8,
                                  USB_TYPE_VENDOR | USB_RECIP_DEVICE |
                                  USB_DIR_IN, 0x00, 0x01, buf, 0x05, 1000);
            LIBMTP_USB_DEBUG("BlackBerry magic part 3:\n");
            LIBMTP_USB_DATA(buf, ret, 16);

            udelay(1000);
            // This control message is unnecessary
            ret = usb_mtp_control_msg(ptp_usb, usb_rcvctrlpipe(dev, 0), 0xa8,
                                  USB_TYPE_VENDOR | USB_RECIP_DEVICE |
                                  USB_DIR_IN, 0x00, 0x01, buf, 0x11, 1000);
            LIBMTP_USB_DEBUG("BlackBerry magic part 4:\n");
            LIBMTP_USB_DATA(buf, ret, 16);

            udelay(1000);
        }
    }

    return 0;
}

static void clear_stall(PTP_USB *ptp_usb)
{
    uint16_t status;
    int ret;

    /* check the inep status */
    status = 0;
    ret = usb_get_endpoint_status(ptp_usb, ptp_usb->recv_bulk_pipe, &status);
    if (ret < 0)
    {
        printk(KERN_ERR "inep: usb_get_endpoint_status()");
    }
    else if (status)
    {
        LIBMTP_INFO("Clearing stall on IN endpoint\n");
        ret = usb_clear_stall_feature(ptp_usb, ptp_usb->recv_bulk_pipe);
        if (ret < 0)
        {
            printk(KERN_ERR "usb_clear_stall_feature()");
        }
    }

    /* check the outep status */
    status = 0;
    ret = usb_get_endpoint_status(ptp_usb, ptp_usb->send_bulk_pipe, &status);
    if (ret < 0)
    {
        printk(KERN_ERR "outep: usb_get_endpoint_status()");
    }
    else if (status)
    {
        LIBMTP_INFO("Clearing stall on OUT endpoint\n");
        ret = usb_clear_stall_feature(ptp_usb, ptp_usb->send_bulk_pipe);
        if (ret < 0)
        {
            printk(KERN_ERR "usb_clear_stall_feature()");
        }
    }

    /* TODO: do we need this for INTERRUPT (ptp_usb->recv_intr_pipe) too? */
}

static void clear_halt(PTP_USB *ptp_usb)
{
    int ret;

    // ret = usb_clear_halt(ptp_usb->pusb_dev, ptp_usb->recv_bulk_pipe);
    ret = usb_mtp_clear_halt(ptp_usb,ptp_usb->recv_bulk_pipe);
    if (ret < 0)
    {
        printk(KERN_ERR "usb_clear_halt() on IN endpoint");
    }
    // ret = usb_clear_halt(ptp_usb->pusb_dev, ptp_usb->send_bulk_pipe);
    ret = usb_mtp_clear_halt(ptp_usb,ptp_usb->send_bulk_pipe);
    if (ret < 0)
    {
        printk(KERN_ERR "usb_clear_halt() on OUT endpoint");
    }
    // ret = usb_clear_halt(ptp_usb->pusb_dev, ptp_usb->recv_intr_pipe);
    ret = usb_mtp_clear_halt(ptp_usb,ptp_usb->recv_intr_pipe);
    if (ret < 0)
    {
        printk(KERN_ERR "usb_clear_halt() on INTERRUPT endpoint");
    }
}

static void close_usb(PTP_USB *ptp_usb)
{
    // Commented out since it was confusing some
    // devices to do these things.
    if (!FLAG_NO_RELEASE_INTERFACE(ptp_usb))
    {
        /*
         * Clear any stalled endpoints
         * On misbehaving devices designed for Windows/Mac, quote from:
         * http://www2.one-eyed-alien.net/~mdharm/linux-usb/target_offenses.txt
         * Device does Bad Things(tm) when it gets a GET_STATUS after CLEAR_HALT
         * (...) Windows, when clearing a stall, only sends the CLEAR_HALT command,
         * and presumes that the stall has cleared.  Some devices actually choke
         * if the CLEAR_HALT is followed by a GET_STATUS (used to determine if the
         * STALL is persistant or not).
         */
        clear_stall(ptp_usb);
        // Clear halts on any endpoints
        clear_halt(ptp_usb);
        // Added to clear some stuff on the OUT endpoint
        // TODO: is this good on the Mac too?
        // HINT: some devices may need that you comment these two out too.
        // usb_resetep(ptp_usb->pusb_dev, ptp_usb->send_bulk_pipe);
        // usb_release_interface(ptp_usb->pusb_dev, (int) ptp_usb->interface);
    }    
}

/**
 * Self-explanatory?
 */
/* 
static int find_interface_and_endpoints(struct usb_device *dev,
uint8_t *interface,
int* inep,
int* inep_maxpacket,
int* outep,
int *outep_maxpacket,
int* intep)
{
int i;

// Loop over the device configurations
for (i = 0; i < dev->descriptor.bNumConfigurations; i++) {
uint8_t j;

// Loop over each configurations interfaces
// for (j = 0; j < dev->config[i].bNumInterfaces; j++) {
for (j = 0; j < dev->config[i].desc.bNumInterfaces; j++) {    
uint8_t k;
uint8_t no_ep;
int found_inep = 0;
int found_outep = 0;
int found_intep = 0;
// struct usb_endpoint_descriptor *ep;
struct usb_host_endpoint *ep;

// MTP devices shall have 3 endpoints, ignore those interfaces
// that haven't.
no_ep = dev->config[i].interface[j]->cur_altsetting->desc.bNumEndpoints;
if (no_ep != 3)
continue;

 *interface = dev->config[i].interface[j]->cur_altsetting->desc.bInterfaceNumber;
ep = dev->config[i].interface[j]->cur_altsetting->endpoint;

// Loop over the three endpoints to locate two bulk and
// one interrupt endpoint and FAIL if we cannot, and continue.
for (k = 0; k < no_ep; k++) {
if (ep[k].desc.bmAttributes == USB_ENDPOINT_XFER_BULK) {
if ((ep[k].desc.bEndpointAddress & USB_ENDPOINT_DIR_MASK) ==
USB_ENDPOINT_DIR_MASK) {
 *inep = ep[k].desc.bEndpointAddress;
 *inep_maxpacket = ep[k].desc.wMaxPacketSize;
found_inep = 1;
}
if ((ep[k].desc.bEndpointAddress & USB_ENDPOINT_DIR_MASK) == 0) {
 *outep = ep[k].desc.bEndpointAddress;
 *outep_maxpacket = ep[k].desc.wMaxPacketSize;
found_outep = 1;
}
} else if (ep[k].desc.bmAttributes == USB_ENDPOINT_XFER_INT) {
if ((ep[k].desc.bEndpointAddress & USB_ENDPOINT_DIR_MASK) ==
USB_ENDPOINT_DIR_MASK) {
 *intep = ep[k].desc.bEndpointAddress;
found_intep = 1;
}
}
}
if (found_inep && found_outep && found_intep)
// We assigned the endpoints so return here.
return 0;
// Else loop to next interface/config
}
}
return -1;
}
 */

static LIBMTP_error_number_t mtp_usb_device_open(LIBMTP_mtpdevice_t *mtp_device)
{
    int i;
    uint8_t bs = 0;
    uint16_t ret = 0;
    PTPParams *params = (PTPParams *) mtp_device->params;
    PTP_USB *ptp_usb = NULL;
    struct usb_device *usb_dev=NULL;    
            
    if (params == NULL)
        return LIBMTP_ERROR_CONNECTING;
                
    ptp_usb =(PTP_USB *) params->data;   
    
    if (ptp_usb == NULL)
        return LIBMTP_ERROR_CONNECTING;
             
    usb_dev = ptp_usb->pusb_dev;  
    
    if (usb_dev == NULL)
        return LIBMTP_ERROR_CONNECTING;  
                         
    /*
     * This works in situations where previous bad applications
     * have not used LIBMTP_Release_Device on exit
     */
    if ((ret = ptp_opensession(params, 1)) == PTP_ERROR_IO)
    {
        LIBMTP_ERROR(
                     "PTP_ERROR_IO: failed to open session, trying again after resetting USB interface\n");
        close_usb(ptp_usb);

        /*
        LIBMTP_ERROR("LIBMTP libusb: Attempt to reset device\n");
        usb_reset_device(ptp_usb->pusb_dev);
        */
        
        /*
        if (init_ptp_usb(params, ptp_usb, usb_dev) < 0)
        {
            LIBMTP_ERROR("LIBMTP PANIC: Could not init USB on second attempt\n")
                         ;
            return LIBMTP_ERROR_CONNECTING;
        }
        */
        // params->transaction_id = 0;

        /* Device has been reset, try again */       
        if ((ret = ptp_opensession(params, 1)) == PTP_ERROR_IO)
        {
            LIBMTP_ERROR(
                         "LIBMTP PANIC: failed to open session on second attempt\n");
            return LIBMTP_ERROR_CONNECTING;
        }
       
    }
    
    /* Was the transaction id invalid? Try again */
    if (ret == PTP_RC_InvalidTransactionID)
    {
        LIBMTP_ERROR(
                     "LIBMTP WARNING: Transaction ID was invalid, increment and try again\n");
        params->transaction_id += 10;
        ret = ptp_opensession(params, 1);
    }
    
    if (ret != PTP_RC_SessionAlreadyOpened && ret != PTP_RC_OK)
    {
        LIBMTP_ERROR("LIBMTP PANIC: Could not open session! "
                     "(Return code %d)\n  Try to reset the device.\n", ret);
        /*    
        usb_release_interface(ptp_usb->pusb_dev,
        (int) ptp_usb->interface);
         */
         
        if (ptp_closesession(params)!=PTP_RC_OK)
        {
            printk(KERN_INFO "Could not close session ERROR\n");
            return LIBMTP_ERROR_CONNECTING;
        }
        else
        {
            if (ptp_opensession(params,1)!=PTP_RC_OK)
            {
                printk(KERN_INFO "Could not open session again ERROR\n");
                return LIBMTP_ERROR_CONNECTING;
            }
         } 
         
        // return LIBMTP_ERROR_CONNECTING;
    }    
        
    /* Cache the device information for later use */
    if (ptp_getdeviceinfo(params, &params->deviceinfo) != PTP_RC_OK)
    {
        
        LIBMTP_ERROR("LIBMTP PANIC: Unable to read device information on device, trying to continue");
       
        /* Prevent memory leaks for this device */
        // kfree(mtp_device->usbinfo);
        // kfree(mtp_device->params);
        // current_params = NULL;
        // kfree(mtp_device);
        // return  - ENODEV;
        return LIBMTP_ERROR_CONNECTING;
    }  
    
    printk("==== PTP Standard Version = %d         ====\n",params->deviceinfo.StandardVersion);	
    printk("==== MTP Vendor Extension Version = %d ====\n",params->deviceinfo.VendorExtensionVersion);	
    printk("==== MTP Vendor Extension ID = %d      ====\n",params->deviceinfo.VendorExtensionID);       

    /* Check: if this is a PTP device, is it really tagged as MTP? */
    if (params->deviceinfo.VendorExtensionID != 0x00000006)
    {
        LIBMTP_ERROR("LIBMTP WARNING: no MTP vendor extension on device "); 
        LIBMTP_ERROR("LIBMTP WARNING: VendorExtensionID: %08x", params->deviceinfo.VendorExtensionID);
        LIBMTP_ERROR("LIBMTP WARNING: VendorExtensionDesc: %s", params->deviceinfo.VendorExtensionDesc);
        LIBMTP_ERROR("LIBMTP WARNING: this typically means the device is PTP "
                     "(i.e. a camera) but not an MTP device at all. "
                     "Trying to continue anyway.");
    }
            
    if (params->deviceinfo.VendorExtensionDesc)
        LIBMTP_Parse_Extension_Descriptor(mtp_device, params->deviceinfo.VendorExtensionDesc);   
    /*
     * If the OGG or FLAC filetypes are flagged as "unknown", check
     * if the firmware has been updated to actually support it.
     */
    if (FLAG_OGG_IS_UNKNOWN(ptp_usb))
    {
        for (i = 0; i < params->deviceinfo.ImageFormats_len; i++)
        {
            if (params->deviceinfo.ImageFormats[i] == PTP_OFC_MTP_OGG)
            {
                /* This is not unknown anymore, unflag it */
                ptp_usb->device_entry.device_flags &=
                    ~DEVICE_FLAG_OGG_IS_UNKNOWN;
                break;
            }
        } // End of for
    }

    if (FLAG_FLAC_IS_UNKNOWN(ptp_usb))
    {
        for (i = 0; i < params->deviceinfo.ImageFormats_len; i++)
        {
            if (params->deviceinfo.ImageFormats[i] == PTP_OFC_MTP_FLAC)
            {
                /* This is not unknown anymore, unflag it */
                ptp_usb->device_entry.device_flags &=
                    ~DEVICE_FLAG_FLAC_IS_UNKNOWN;
                break;
            }
        } // End of for
    }    

    /* Determine if the object size supported is 32 or 64 bit wide */
    for (i = 0; i < params->deviceinfo.ImageFormats_len; i++)
    {
        PTPObjectPropDesc opd;

        if (ptp_mtp_getobjectpropdesc(params, PTP_OPC_ObjectSize,
            params->deviceinfo.ImageFormats[i], &opd) != PTP_RC_OK)
        {
            LIBMTP_ERROR(
                         "LIBMTP PANIC: could not inspect object property descriptions!\n");
        }
        else
        {
            if (opd.DataType == PTP_DTC_UINT32)
            {
                if (bs == 0)
                {
                    bs = 32;
                }
                else if (bs != 32)
                {
                    LIBMTP_ERROR(
                                 "LIBMTP PANIC: different objects support different object sizes!\n");
                    bs = 0;
                    break;
                }
            }
            else if (opd.DataType == PTP_DTC_UINT64)
            {

                if (bs == 0)
                {
                    bs = 64;
                }
                else if (bs != 64)
                {
                    LIBMTP_ERROR(
                                 "LIBMTP PANIC: different objects support different object sizes!\n");
                    bs = 0;
                    break;
                }
            }
            else
            {
                // Ignore if other size.
                LIBMTP_ERROR(
                             "LIBMTP PANIC: awkward object size data type: %04x\n", opd.DataType);
                bs = 0;
                break;
            }
        }
    } // End of for        

    if (bs == 0)
    {
        // Could not detect object bitsize, assume 32 bits
        bs = 32;
    }        

    mtp_device->object_bitsize = bs;

    /* No Errors yet for this device */
    mtp_device->errorstack = NULL;

    /* Default Max Battery Level, we will adjust this if possible */
    mtp_device->maximum_battery_level = 100;

    /* Check if device supports reading maximum battery level */
    if (!FLAG_BROKEN_BATTERY_LEVEL(ptp_usb) && ptp_property_issupported
        (params, PTP_DPC_BatteryLevel))
    {
        PTPDevicePropDesc dpd;

        /* Try to read maximum battery level */
        if (ptp_getdevicepropdesc(params, PTP_DPC_BatteryLevel, &dpd) 
            != PTP_RC_OK)
        {
            LIBMTP_Add_Error_To_ErrorStack(mtp_device, LIBMTP_ERROR_CONNECTING,
                "Unable to read Maximum Battery Level for this "
                "device even though the device supposedly "
                "supports this functionality");
        }

        /* TODO: is this appropriate? */
        /* If max battery level is 0 then leave the default, otherwise assign */
        if (dpd.FORM.Range.MaximumValue.u8 != 0)
        {
            mtp_device->maximum_battery_level = dpd.FORM.Range.MaximumValue.u8;
        }

        ptp_free_devicepropdesc(&dpd);
    }
    
    /* Set all default folders to 0xffffffffU (root directory) */
    mtp_device->default_music_folder = 0xffffffffU;
    mtp_device->default_playlist_folder = 0xffffffffU;
    mtp_device->default_picture_folder = 0xffffffffU;
    mtp_device->default_video_folder = 0xffffffffU;
    mtp_device->default_organizer_folder = 0xffffffffU;
    mtp_device->default_zencast_folder = 0xffffffffU;
    mtp_device->default_album_folder = 0xffffffffU;
    mtp_device->default_text_folder = 0xffffffffU;

    /* Set initial storage information */
    mtp_device->storage = NULL;
    if (LIBMTP_Get_Storage(mtp_device, LIBMTP_STORAGE_SORTBY_NOTSORTED) ==  - 1)
    {
        LIBMTP_Add_Error_To_ErrorStack(mtp_device, LIBMTP_ERROR_GENERAL, 
                                       "Get Storage information failed.");
        mtp_device->storage = NULL;
    }

    /*
     * Then get the handles and try to locate the default folders.
     * This has the desired side effect of caching all handles from
     * the device which speeds up later operations.
     */
    // cyhuang (2011/05/26) : Do not fetch all meta data at all.
    // LIBMTP_Flush_Handles(mtp_device); 
    
    // +++ cyhuang (2011/06/09) 
    if (ptp_operation_issupported(params,PTP_OC_GetPartialObject) == 1)
        printk("==== MTP GetPartialObject Support ====\n");	  
    // +++ cyhuang (2011/06/09) 
        
    return LIBMTP_ERROR_NONE;
}

/*
static void mtp_usb_device_close(LIBMTP_mtpdevice_t *mtp_device)
{
    PTPParams *params = (PTPParams *) mtp_device->params;
    PTP_USB *ptp_usb = NULL;    
    
    if (params == NULL)
        return LIBMTP_ERROR_CONNECTING;
                
    ptp_usb =(PTP_USB *) params->data;   
    
    if (ptp_usb == NULL)
        return LIBMTP_ERROR_CONNECTING;
                
    if (!(ptp_usb->flags & ABORTING_OR_DISCONNECTING))
        close_device(params,ptp_usb);
}
*/

/**
 * This function assigns params and usbinfo given a raw device
 * as input.
 * @param device the device to be assigned.
 * @param usbinfo a pointer to the new usbinfo.
 * @return an error code.
 */
static LIBMTP_error_number_t configure_usb_device(struct usb_device *usb_dev, struct usb_interface *intf, 
                                           PTPParams *params, void **usbinfo)
{
    PTP_USB *ptp_usb;
    // uint16_t ret = 0;
    int err;

    /* Allocate structs */
    ptp_usb = (PTP_USB*)kmalloc(sizeof(PTP_USB), GFP_KERNEL);
    if (ptp_usb == NULL)
    {
        return LIBMTP_ERROR_MEMORY_ALLOCATION;
    } 
    /* Start with a blank slate (includes setting device_flags to 0) */
    memset(ptp_usb, 0, sizeof(PTP_USB));

    /* Copy the raw device */
    // memcpy(&ptp_usb->rawdevice, device, sizeof(LIBMTP_raw_device_t));
    ptp_usb->pusb_dev = usb_dev;
    ptp_usb->pusb_intf = intf;

    /*
     * Some devices must have their "OS Descriptor" massaged in order
     * to work.
     */
    
    if (FLAG_ALWAYS_PROBE_DESCRIPTOR(ptp_usb))
    {
        // Massage the device descriptor
        (void)probe_device_descriptor(ptp_usb);
    }

    /* Assign interface and endpoints to usbinfo... */    
    err = usb_mtp_get_pipes(ptp_usb);    

    if (err)
    {
        LIBMTP_ERROR(
                     "LIBMTP PANIC: Unable to find interface & endpoints of device\n");
        kfree(ptp_usb);             
        return LIBMTP_ERROR_CONNECTING;
    }

    /* Attempt to initialize this device */
    if (init_ptp_usb(params, ptp_usb, usb_dev) < 0)
    {
        LIBMTP_ERROR("LIBMTP PANIC: Unable to initialize device\n");
        kfree(ptp_usb); 
        return LIBMTP_ERROR_CONNECTING;
    }
           
    /* OK configured properly */
    *usbinfo = (void*)ptp_usb;
    return LIBMTP_ERROR_NONE;
}

void close_device(PTP_USB *ptp_usb, PTPParams *params)
{
    if (ptp_closesession(params) != PTP_RC_OK)
        LIBMTP_ERROR("ERROR: Could not close session!\n");
    close_usb(ptp_usb);
}

void set_usb_device_timeout(PTP_USB *ptp_usb, int timeout)
{
    ptp_usb->timeout = timeout;
}

void get_usb_device_timeout(PTP_USB *ptp_usb, int *timeout)
{
    *timeout = ptp_usb->timeout;
}

static int usb_clear_stall_feature(PTP_USB *ptp_usb, int ep)
{
    return (usb_mtp_control_msg(ptp_usb, usb_rcvctrlpipe(ptp_usb
            ->pusb_dev, 0), USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT,
            USB_FEATURE_HALT, ep, NULL, 0, ptp_usb->timeout));
}

static int usb_get_endpoint_status(PTP_USB *ptp_usb, int ep, uint16_t *status)
{
    return (usb_mtp_control_msg(ptp_usb, usb_rcvctrlpipe(ptp_usb
            ->pusb_dev, 0), USB_REQ_GET_STATUS, USB_DP_DTH | USB_RECIP_ENDPOINT,
            USB_FEATURE_HALT, ep, (char*)status, 2, ptp_usb->timeout));
}

// --------------------------------------------------------------------------------------------------
static void LIBMTP_ptp_debug(void *data, const char *format, va_list args)
{
    if ((LIBMTP_debug &LIBMTP_DEBUG_PTP) != 0)
    {
        printk(format, args);
    }
}

/**
 * Overriding error function.
 * This way we can capture all error etc to our errorstack.
 */
static void LIBMTP_ptp_error(void *data, const char *format, va_list args)
{
    printk(format, args);
}

static inline void mtp_device_set_flags(LIBMTP_mtpdevice_t *mtp_device)
{
    int j;
    PTP_USB *ptp_usb = (PTP_USB*)mtp_device->usbinfo;
    struct usb_device *usb_dev = ptp_usb->pusb_dev;
    PTPParams *params = ptp_usb->params;

    for (j = 0; j < mtp_device_table_size; j++)
    {
        if (usb_dev->descriptor.idVendor == mtp_device_table[j].vendor_id &&
            usb_dev->descriptor.idProduct == mtp_device_table[j].product_id)
        {

            ptp_usb->device_entry.device_flags =
                mtp_device_table[j].device_flags;
            params->device_flags = ptp_usb->device_entry.device_flags;                        
        } 
    } // End of for        
}

/* Initialize all the dynamic resources we need */
static int usb_mtp_acquire_resources(PTP_USB *ptp_usb)
{

    ptp_usb->current_urb = usb_alloc_urb(0, GFP_KERNEL);
    if (!ptp_usb->current_urb)
    {
        // MTP_DEBUGP("URB allocation failed\n");
        return  - ENOMEM;
    }

    return 0;
}

/* Release all our dynamic resources */
static void usb_mtp_release_resources(PTP_USB *ptp_usb)
{
    // MTP_DEBUGP("-- %s\n", __FUNCTION__);

    /* Tell the control thread to exit.  The SCSI host must
     * already have been removed so it won't try to queue
     * any more commands.
     */
    // MTP_DEBUGP("-- sending exit command to thread\n");
    set_bit(MTP_FLIDX_DISCONNECTING, &ptp_usb->flags);
    // up(&ptp_usb->sema);

    /* Free the URB */
    usb_free_urb(ptp_usb->current_urb);
}


static int mtp_device_init(LIBMTP_mtpdevice_t *mtp_device, struct usb_interface
                           *intf)
{    
    PTPParams *current_params;
    // PTP_USB *ptp_usb;
    LIBMTP_error_number_t err;    
    struct usb_device *usb_dev = interface_to_usbdev(intf);  

    /* Create PTP params */
    current_params = (PTPParams*)kmalloc(sizeof(PTPParams), GFP_KERNEL);
    if (current_params == NULL)
    {
        return  - ENOMEM;
    } 

    memset(current_params, 0, sizeof(PTPParams));
    // current_params->device_flags = rawdevice->device_entry.device_flags;
    init_MUTEX (&current_params->mutex_lock);
    current_params->nrofobjects = 0;
    current_params->objects = NULL;
    current_params->response_packet_size = 0;
    current_params->response_packet = NULL;
    /* This will be a pointer to PTP_USB later */
    current_params->data = NULL;
    /* Set upp local debug and error functions */
    current_params->debug_func = LIBMTP_ptp_debug;
    current_params->error_func = LIBMTP_ptp_error;
    /* TODO: Will this always be little endian? */
    current_params->byteorder = PTP_DL_LE;  
    
    // +++ cyhuang (2011/06/09)
    /* Add session_mutex_lock for get object session.*/ 
    init_MUTEX (&current_params->session_mutex_lock);      
    // +++ cyhuang (2011/06/09) 

    mtp_device->params = current_params;    

    /* Create usbinfo, this also opens the session */
    err = configure_usb_device(usb_dev, intf, current_params, &mtp_device->usbinfo);
    if (err != LIBMTP_ERROR_NONE)
    {
        kfree(current_params);        
        return  -ENODEV;
    }    

    // ptp_usb = (PTP_USB*) mtp_device->usbinfo;
    /* Set pointer back to params */
    // ptp_usb->params = current_params;

    mtp_device_set_flags(mtp_device);            

    return 0;
}

// --------------------------------------------------------------------------------------------------
static inline int usb_mtp_buffer_init(PTP_USB *ptp_usb)
{
    PTPParams *params = ptp_usb->params;
    /* Allocate the device-related DMA-mapped buffers */
    ptp_usb->cr = usb_buffer_alloc(ptp_usb->pusb_dev, sizeof(*ptp_usb->cr),
            GFP_KERNEL, &ptp_usb->cr_dma);
    if (!ptp_usb->cr) {
        // MTP_DEBUGP("usb_ctrlrequest allocation failed\n");
        printk("usb_ctrlrequest allocation failed\n");
        return -ENOMEM;
    }

    ptp_usb->iobuf = usb_buffer_alloc(ptp_usb->pusb_dev, sizeof(PTPUSBBulkContainer),
            GFP_KERNEL, &ptp_usb->iobuf_dma);
    if (!ptp_usb->iobuf) {
        // MTP_DEBUGP("I/O buffer allocation failed\n");
        printk("I/O buffer allocation failed\n");
        return -ENOMEM;
    }

    ptp_usb->read_buf = kmalloc(CONTEXT_BLOCK_SIZE, GFP_KERNEL);    
    if (!ptp_usb->read_buf) {
        MTP_DEBUGP("Read buffer allocation failed\n");
        return -ENOMEM;
    }
            
    params->session_buf = kmalloc(ptp_usb->inep_maxpacket,GFP_KERNEL);
    if (!params->session_buf) {
        // MTP_DEBUGP("I/O buffer allocation failed\n");
        printk("Session buffer allocation failed\n");
        return -ENOMEM;
    }
            
}

static inline void usb_mtp_buffer_free(PTP_USB *ptp_usb)
{
    PTPParams *params = ptp_usb->params;

    /* Free the device-related DMA-mapped buffers */    
    if (ptp_usb->cr)
        usb_buffer_free(ptp_usb->pusb_dev, sizeof(*ptp_usb->cr), ptp_usb->cr,
                ptp_usb->cr_dma);
                                       
    if (ptp_usb->iobuf)
        usb_buffer_free(ptp_usb->pusb_dev, sizeof(PTPUSBBulkContainer), ptp_usb->iobuf,
                ptp_usb->iobuf_dma);
                           
    if (ptp_usb->read_buf)
       kfree(ptp_usb->read_buf);                 
                            
    if (params->session_buf)
        kfree(params->session_buf);
}

/* Associate our private data with the USB device */
static int associate_dev(LIBMTP_mtpdevice_t *mtp_device, struct usb_interface
                         *intf)
{
    int result = 0;
    PTP_USB *ptp_usb;

    result = mtp_device_init(mtp_device, intf);

    if (result)
        return result;

    ptp_usb = (PTP_USB*)mtp_device->usbinfo;        

    /*
    if (!probe_device_descriptor(ptp_usb->pusb_dev))
    {
        printk(KERN_ERR "%s [ERROR] : probe_device_descriptor() fail.\n",
               __func__);
        usb_mtp_buffer_free(ptp_usb);       
        result =  -ENODEV;
        return result;
    } 
    */
    
    /* Store our private data in the interface */
    usb_set_intfdata(intf, mtp_device);
    // init_MUTEX_LOCKED(&(ptp_usb->sema));    
               
    return result;
}

/* Get the pipe settings */
static int usb_mtp_get_pipes(PTP_USB *ptp_usb)
{
    struct usb_host_interface *altsetting = NULL;
    int i;
    struct usb_endpoint_descriptor *ep;
    struct usb_endpoint_descriptor *ep_in = NULL;
    struct usb_endpoint_descriptor *ep_out = NULL;
    struct usb_endpoint_descriptor *ep_int = NULL;

    if (ptp_usb == NULL)
        return  - ENODEV;

    altsetting = ptp_usb->pusb_intf->cur_altsetting;

    if (altsetting == NULL)
        return  - ENODEV;

    /*
     * Find the endpoints we need.
     * We are expecting a minimum of 2 endpoints - in and out (bulk).
     * An optional interrupt is OK (necessary for CBI protocol).
     * We will ignore any others.
     */
    for (i = 0; i < altsetting->desc.bNumEndpoints; i++)
    {
        ep = &altsetting->endpoint[i].desc;

        /* Is it a BULK endpoint? */
        if ((ep->bmAttributes &USB_ENDPOINT_XFERTYPE_MASK) ==
            USB_ENDPOINT_XFER_BULK)
        {
            /* BULK in or out? */
            if (ep->bEndpointAddress &USB_DIR_IN)
                ep_in = ep;
            else
                ep_out = ep;
        } 

        /* Is it an interrupt endpoint? */
        else if ((ep->bmAttributes &USB_ENDPOINT_XFERTYPE_MASK) ==
                 USB_ENDPOINT_XFER_INT)
        {
            ep_int = ep;
        }
    }

    
    if (!ep_in || !ep_out || !ep_int) {
        // MTP_DEBUGP("Endpoint sanity check failed! Rejecting dev.\n");
        printk("Endpoint sanity check failed! Rejecting dev.\n");
        return -EIO;
    }
   
    
    /* Calculate and store the pipe values */
    ptp_usb->send_ctrl_pipe = usb_sndctrlpipe(ptp_usb->pusb_dev, 0);
    ptp_usb->recv_ctrl_pipe = usb_rcvctrlpipe(ptp_usb->pusb_dev, 0);

    ptp_usb->send_bulk_pipe = usb_sndbulkpipe(ptp_usb->pusb_dev, ep_out
        ->bEndpointAddress &USB_ENDPOINT_NUMBER_MASK);
    ptp_usb->outep_maxpacket = usb_maxpacket(ptp_usb->pusb_dev, ptp_usb
        ->send_bulk_pipe, usb_pipeout(ptp_usb->send_bulk_pipe));

    ptp_usb->recv_bulk_pipe = usb_rcvbulkpipe(ptp_usb->pusb_dev, ep_in
        ->bEndpointAddress &USB_ENDPOINT_NUMBER_MASK);
    ptp_usb->inep_maxpacket = usb_maxpacket(ptp_usb->pusb_dev, ptp_usb
        ->recv_bulk_pipe, usb_pipeout(ptp_usb->recv_bulk_pipe));
    
    if (ep_int)
    {
        ptp_usb->recv_intr_pipe = usb_rcvintpipe(ptp_usb->pusb_dev, ep_int
            ->bEndpointAddress &USB_ENDPOINT_NUMBER_MASK);
        ptp_usb->ep_bInterval = ep_int->bInterval;
    }

    usb_mtp_acquire_resources(ptp_usb);

    return 0;
}

// +++ cyhuang (2011/06/08) 
/* 
 *  Add for bDeviceClass = 0xFF or bInterfaceClass = 0xFF (Vendor SPEC) USB MTP Device. 
 *  Give one more chance to search in mtp_device_table[].
 */
static inline bool mtp_device_vendor_spec_match(struct usb_interface *interface)
{
    int j;
    struct usb_host_interface *intf;        
    struct usb_device *usb_dev = interface_to_usbdev(interface);

    intf = interface->cur_altsetting;
    
    if (intf->desc.bInterfaceClass == USB_CLASS_MTP) 
        return true;   

    if ((usb_dev->descriptor.bDeviceClass != 0xFF) && (intf->desc.bInterfaceClass != 0xFF))
    {
        return true;
    }        
        
    // printk("bInterfaceClass = 0x%x , bInterfaceSubClass = 0x%x , bInterfaceProtocol = 0x%x\n",intf->desc.bInterfaceClass,intf->desc.bInterfaceSubClass,intf->desc.bInterfaceProtocol);

    for (j = 0; j < mtp_device_table_size; j++)
    {
        if (usb_dev->descriptor.idVendor == mtp_device_table[j].vendor_id &&
            usb_dev->descriptor.idProduct == mtp_device_table[j].product_id)
        {
            intf->desc.bInterfaceClass = USB_CLASS_MTP;
            intf->desc.bInterfaceSubClass = 0x1;
            intf->desc.bInterfaceProtocol = 0x1;
            
            kobject_hotplug(&interface->dev.kobj, KOBJ_ADD);
            return true;    
        } 
    } // End of for 
    
    return false;        
}
// +++ cyhuang (2011/06/08) 

/* Probe to see if we can drive a newly-connected USB device */
static int mtp_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    int result = 0;
    LIBMTP_mtpdevice_t *mtp_device;
    
// +++ cyhuang (2011/06/08)  
/* 
 *  Add for bDeviceClass = 0xFF or bInterfaceClass = 0xFF (Vendor SPEC) USB MTP Device. 
 *  Give one more chance to search in mtp_device_table[].
 */   
    if (mtp_device_vendor_spec_match(intf) == false)
        return -ENODEV;
// +++ cyhuang (2011/06/08)         

    mtp_device = (LIBMTP_mtpdevice_t*)kmalloc(sizeof(LIBMTP_mtpdevice_t),
                  GFP_KERNEL);

    if (mtp_device == NULL)
    {
        printk(KERN_ERR "%s [ERROR] : Can not allocate mtp_dev.\n", __func__);
        return  -ENOMEM;
    } 
    
    memset(mtp_device, 0, sizeof(LIBMTP_mtpdevice_t));
    mtp_device->owner = THIS_MODULE;
    
    result = associate_dev(mtp_device, intf);
    
    if (result)
    {
        printk(KERN_ERR "%s [ERROR] : associate_dev() fail (result = %d)\n",
               __func__, result);
        goto err;
    }  
        
    result = mtp_device_register(mtp_device);
    
    if (result)
    {
        printk(KERN_ERR "%s [ERROR] : Can not register mtp_device (MTP_MAX_DEVICES = %d)\n",__func__,MTP_MAX_DEVICES);
        return -ENODEV;        
    }    

    // Save kobj_name for hotplug.
    mtp_device->kobj_name = intf->dev.kobj.k_name;        
    printk("==== MTP kobj_name : %s ====\n",mtp_device->kobj_name);	         
    
    return 0;
    
err: 
    kfree(mtp_device);
    return result;
}

static inline bool mtp_is_mtp_device(PTPParams *params)
{
    if (params->deviceinfo.VendorExtensionID != 0x00000006)
        return false;
    else
        return true;    
}

/* Handle a disconnect event from the USB core */
static void mtp_disconnect(struct usb_interface *intf)
{
    LIBMTP_mtpdevice_t *mtp_device = usb_get_intfdata(intf);
    PTP_USB *ptp_usb = (PTP_USB*)mtp_device->usbinfo;    
    
    down(&mtp_table_mutex);    
        
    // Reset current session to avoid dead lock.   
    if (mtp_is_mtp_device((PTPParams *) ptp_usb->params) == false)     
    {             
        if (ptp_usb->params->obj_session.state != PtpObjectSessionClose)
        {
            init_MUTEX (&ptp_usb->params->session_mutex_lock);
            ptp_getobject_session(ptp_usb->params,0,NULL,0,true);              
        }        
    }
            
    usb_mtp_release_resources(ptp_usb);
    usb_mtp_buffer_free(ptp_usb); 
    up(&mtp_table_mutex);  
     
    Mtp_Put_Device(mtp_device);  
    usb_set_intfdata(intf, NULL);   
} 


static struct usb_driver usb_mtp_driver = 
{
    .owner = THIS_MODULE, 
    .name = "usb-mtp", 
    .probe = mtp_probe, 
    .disconnect = mtp_disconnect, 
    .id_table = mtp_usb_ids, 
};

/***********************************************************************
 * Initialization and registration
 ***********************************************************************/

static int __init usb_mtp_init(void)
{
    int i,retval;
    printk(KERN_INFO "Initializing USB MTP/PTP driver...\n");

    for (i=0;i<MTP_MAX_DEVICES;i++)
        mtp_tables[i] = NULL;

    LIBMTP_Init();
    /* register the driver, return usb_register return code if error */
    retval = usb_register(&usb_mtp_driver);
    if (retval == 0)
        printk(KERN_INFO "USB MTP/PTP support registered.\n");

    return retval;
}

static void __exit usb_mtp_exit(void)
{
    MTP_DEBUGP("usb_mtp_exit() called\n");

    /* Deregister the driver
     * This will cause disconnect() to be called for each
     * attached unit
     */
    MTP_DEBUGP("-- calling usb_deregister()\n");
    usb_deregister(&usb_mtp_driver);

}

module_init(usb_mtp_init);
module_exit(usb_mtp_exit);
