#ifndef _MTP_TRANSPORT_H_
#define _MTP_TRANSPORT_H_

/* Dynamic flag definitions: used in set_bit() etc. */
#define MTP_FLIDX_URB_ACTIVE 18  /* 0x00040000  current_urb is in use  */
#define MTP_FLIDX_SG_ACTIVE  19  /* 0x00080000  current_sg is in use   */
#define MTP_FLIDX_ABORTING   20  /* 0x00100000  abort is in progress   */
#define MTP_FLIDX_DISCONNECTING  21  /* 0x00200000  disconnect in progress */
#define ABORTING_OR_DISCONNECTING   ((1UL << MTP_FLIDX_ABORTING) | \
                     (1UL << MTP_FLIDX_DISCONNECTING))
#define MTP_FLIDX_RESETTING  22  /* 0x00400000  device reset in progress */
#define MTP_FLIDX_TIMED_OUT  23  /* 0x00800000  SCSI midlayer timed out  */


/*
 * usb_mtp_bulk_transfer_xxx() return codes, in order of severity
 */

#define USB_MTP_XFER_GOOD  0   /* good transfer                 */
#define USB_MTP_XFER_SHORT 1   /* transferred less than expected */
#define USB_MTP_XFER_STALLED   2   /* endpoint stalled              */
#define USB_MTP_XFER_LONG  3   /* device tried to send too much */
#define USB_MTP_XFER_ERROR 4   /* transfer died in the middle   */

#ifdef USB_MTP_DEBUG
#define MTP_DEBUGP(format, arg...) printk(KERN_DEBUG "%s: " format "\n" , __FILE__ , ## arg)
#else
#define MTP_DEBUGP(format, arg...) do {} while (0)
#endif

int usb_mtp_control_msg(PTP_USB *ptp_usb, unsigned int pipe,
		 u8 request, u8 requesttype, u16 value, u16 index, 
		 void *data, u16 size, int timeout);
/*
int usb_mtp_bulk_transfer_buf(PTP_USB *ptp_usb, unsigned int pipe,
	void *buf, unsigned int length, unsigned int *act_len,int timeout);		 
*/

int usb_mtp_bulk_read(PTP_USB *ptp_usb,unsigned int pipe,void *buf, int size ,int timeout);
int usb_mtp_bulk_write(PTP_USB *ptp_usb,unsigned int pipe,void *buf, int size ,int timeout);
int usb_mtp_clear_halt(PTP_USB *ptp_usb, unsigned int pipe);

#endif // _MTP_TRANSPORT_H_
