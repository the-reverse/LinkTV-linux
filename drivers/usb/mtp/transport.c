/* Driver for USB MTP compliant devices
 *
 * $Id: transport.c,v 1.00 2011/03/03 13:39:43 cyhuang Exp $
 *
 * Current development and maintenance by:
 *   (c) 2011 Ching-Yu Huang (cyhuang@realtek.com)
 *
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>

#include <linux/usb.h>
#include <linux/mtp/usb.h>
#include "transport.h"

 /* This is the timeout handler which will cancel an URB when its timeout
 * expires.
 */
static void usb_mtp_timeout_handler(unsigned long _ptp_usb)
{
	PTP_USB *ptp_usb = (PTP_USB *) _ptp_usb;
     
	if (test_and_clear_bit(MTP_FLIDX_URB_ACTIVE, &ptp_usb->flags)) {
		// MTP_DEBUGP("Timeout -- cancelling URB\n");
		usb_unlink_urb(ptp_usb->current_urb);
	}
}

static void usb_mtp_blocking_completion(struct urb *urb, struct pt_regs *regs)
{
	struct completion *urb_done_ptr = (struct completion *)urb->context;
 
    
	complete(urb_done_ptr);
}

/* This is a version of usb_clear_halt() that allows early termination and
 * doesn't read the status from the device -- this is because some devices
 * crash their internal firmware when the status is requested after a halt.
 *
 * A definitive list of these 'bad' devices is too difficult to maintain or
 * make complete enough to be useful.  This problem was first observed on the
 * Hagiwara FlashGate DUAL unit.  However, bus traces reveal that neither
 * MacOS nor Windows checks the status after clearing a halt.
 *
 * Since many vendors in this space limit their testing to interoperability
 * with these two OSes, specification violations like this one are common.
 */
int usb_mtp_clear_halt(PTP_USB *ptp_usb, unsigned int pipe)
{
	int result;
	int endp = usb_pipeendpoint(pipe);

	if (usb_pipein (pipe))
		endp |= USB_DIR_IN;

	result = usb_mtp_control_msg(ptp_usb, ptp_usb->send_ctrl_pipe,
		USB_REQ_CLEAR_FEATURE, USB_RECIP_ENDPOINT,
		USB_ENDPOINT_HALT, endp,
		NULL, 0, 3*HZ);

	/* reset the endpoint toggle */
	if (result >= 0)
		usb_settoggle(ptp_usb->pusb_dev, usb_pipeendpoint(pipe),
				usb_pipeout(pipe), 0);

	MTP_DEBUGP("%s: result = %d\n", __FUNCTION__, result);
	return result;
}

/*
 * Interpret the results of a URB transfer
 *
 * This function prints appropriate debugging messages, clears halts on
 * non-control endpoints, and translates the status to the corresponding
 * USB_STOR_XFER_xxx return code.
 */
static int interpret_urb_result(PTP_USB *ptp_usb, unsigned int pipe,
		unsigned int length, int result, unsigned int partial)
{
	MTP_DEBUGP("Status code %d; transferred %u/%u\n",
			result, partial, length);
	switch (result) {

	/* no error code; did we send all the data? */
	case 0:
	    /* 
		if (partial != length) {
			MTP_DEBUGP("-- short transfer\n");
			printk("partial = %d , length = %d\n",partial,length);
			return USB_MTP_XFER_SHORT;
		}
        */
        
		MTP_DEBUGP("-- transfer complete\n");
		return USB_MTP_XFER_GOOD;

	/* stalled */
	case -EPIPE:
		/* for control endpoints, (used by CB[I]) a stall indicates
		 * a failed command */
		if (usb_pipecontrol(pipe)) {
			MTP_DEBUGP("-- stall on control pipe\n");
			return USB_MTP_XFER_STALLED;
		}

		/* for other sorts of endpoint, clear the stall */
		MTP_DEBUGP("clearing endpoint halt for pipe 0x%x\n", pipe);
		if (usb_mtp_clear_halt(ptp_usb, pipe) < 0)
			return USB_MTP_XFER_ERROR;
		return USB_MTP_XFER_STALLED;

	/* timeout or excessively long NAK */
	case -ETIMEDOUT:
		MTP_DEBUGP("-- timeout or NAK\n");
		return USB_MTP_XFER_ERROR;

	/* babble - the device tried to send more than we wanted to read */
	case -EOVERFLOW:
		MTP_DEBUGP("-- babble\n");
		return USB_MTP_XFER_LONG;

	/* the transfer was cancelled by abort, disconnect, or timeout */
	case -ECONNRESET:
		MTP_DEBUGP("-- transfer cancelled\n");
		return USB_MTP_XFER_ERROR;

	/* short scatter-gather read transfer */
	case -EREMOTEIO:
		MTP_DEBUGP("-- short read transfer\n");
		return USB_MTP_XFER_SHORT;

	/* abort or disconnect in progress */
	case -EIO:
		MTP_DEBUGP("-- abort or disconnect in progress\n");
		return USB_MTP_XFER_ERROR;

	/* the catch-all error case */
	default:
		MTP_DEBUGP("-- unknown error\n");
		return USB_MTP_XFER_ERROR;
	}
}


 /* This is the common part of the URB message submission code
 *
 * All URBs from the MTP driver involved in handling a queued PTP
 * command _must_ pass through this function (or something like it) for the
 * abort mechanisms to work properly.
 */
static int usb_mtp_msg_common(PTP_USB *ptp_usb, int timeout)
{
	struct completion urb_done;
	struct timer_list to_timer;
	int status;
    
	/* don't submit URBs during abort/disconnect processing */
	if (ptp_usb->flags & ABORTING_OR_DISCONNECTING)
		return -EIO;

	/* set up data structures for the wakeup system */
	init_completion(&urb_done);

	/* fill the common fields in the URB */
	ptp_usb->current_urb->context = &urb_done;
	ptp_usb->current_urb->actual_length = 0;
	ptp_usb->current_urb->error_count = 0;
	ptp_usb->current_urb->status = 0;

	/* we assume that if transfer_buffer isn't ptp_usb->iobuf then it
	 * hasn't been mapped for DMA.  Yes, this is clunky, but it's
	 * easier than always having the caller tell us whether the
	 * transfer buffer has already been mapped. */
	ptp_usb->current_urb->transfer_flags =
			URB_ASYNC_UNLINK | URB_NO_SETUP_DMA_MAP;
	if (ptp_usb->current_urb->transfer_buffer == ptp_usb->iobuf)
		ptp_usb->current_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
	ptp_usb->current_urb->transfer_dma = ptp_usb->iobuf_dma;
	ptp_usb->current_urb->setup_dma = ptp_usb->cr_dma;
    
    // printk("%s : ptp_usb->current_urb = 0x%x , transfer_buffer = 0x%x , transfer_dma = 0x%x\n",__func__,ptp_usb->current_urb,ptp_usb->current_urb->transfer_buffer,ptp_usb->current_urb->transfer_dma);
	/* submit the URB */
	status = usb_submit_urb(ptp_usb->current_urb, GFP_NOIO);
	if (status) {
		/* something went wrong */
		return status;
	}

	/* since the URB has been submitted successfully, it's now okay
	 * to cancel it */
	set_bit(MTP_FLIDX_URB_ACTIVE, &ptp_usb->flags);

	/* did an abort/disconnect occur during the submission? */
	if (ptp_usb->flags & ABORTING_OR_DISCONNECTING) {
        
		/* cancel the URB, if it hasn't been cancelled already */
		if (test_and_clear_bit(MTP_FLIDX_URB_ACTIVE, &ptp_usb->flags)) {
			// MTP_DEBUGP("-- cancelling URB\n");
			usb_kill_urb(ptp_usb->current_urb);
		}
	}
 
	/* submit the timeout timer, if a timeout was requested */
	if (timeout > 0) {
		init_timer(&to_timer);
		to_timer.expires = jiffies + timeout;
		to_timer.function = usb_mtp_timeout_handler;
		to_timer.data = (unsigned long) ptp_usb;
		add_timer(&to_timer);
	}

	/* wait for the completion of the URB */	
	wait_for_completion(&urb_done);	
	clear_bit(MTP_FLIDX_URB_ACTIVE, &ptp_usb->flags);
 
	/* clean up the timeout timer */
	if (timeout > 0)
		del_timer_sync(&to_timer);

	/* return the URB status */
	return ptp_usb->current_urb->status;
}

/*
 * Transfer one control message, with timeouts, and allowing early
 * termination.  Return codes are usual -Exxx, *not* USB_STOR_XFER_xxx.
 */
int usb_mtp_control_msg(PTP_USB *ptp_usb, unsigned int pipe,
		 u8 request, u8 requesttype, u16 value, u16 index, 
		 void *data, u16 size, int timeout)
{
	int status;

	MTP_DEBUGP("%s: rq=%02x rqtype=%02x value=%04x index=%02x len=%u\n",
			__FUNCTION__, request, requesttype,
			value, index, size);

	/* fill in the devrequest structure */
	ptp_usb->cr->bRequestType = requesttype;
	ptp_usb->cr->bRequest = request;
	ptp_usb->cr->wValue = cpu_to_le16(value);
	ptp_usb->cr->wIndex = cpu_to_le16(index);
	ptp_usb->cr->wLength = cpu_to_le16(size);

	/* fill and submit the URB */
	usb_fill_control_urb(ptp_usb->current_urb, ptp_usb->pusb_dev, pipe, 
			 (unsigned char*) ptp_usb->cr, data, size, 
			 usb_mtp_blocking_completion, NULL);
			 
	status = usb_mtp_msg_common(ptp_usb, timeout);

	/* return the actual length of the data transferred if no error */
	if (status == 0)
		status = ptp_usb->current_urb->actual_length;
	return status;
}

/*
 * Transfer one buffer via bulk pipe, without timeouts, but allowing early
 * termination.  Return codes are USB_STOR_XFER_xxx.  If the bulk pipe
 * stalls during the transfer, the halt is automatically cleared.
 */
static int usb_mtp_bulk_transfer_buf(PTP_USB *ptp_usb, unsigned int pipe,
	void *buf, unsigned int length, unsigned int *act_len,int timeout)
{
	int result;

	// MTP_DEBUGP("%s: xfer %u bytes\n", __FUNCTION__, length);
    
	/* fill and submit the URB */
	usb_fill_bulk_urb(ptp_usb->current_urb, ptp_usb->pusb_dev, pipe, buf, length,
		      usb_mtp_blocking_completion, NULL);
	result = usb_mtp_msg_common(ptp_usb, timeout);
        
    
	/* store the actual length of the data transferred */
	if (act_len)
		*act_len = ptp_usb->current_urb->actual_length;
		
	// result = usb_bulk_msg (ptp_usb->pusb_dev,pipe , bytes , length , act_len , 10*HZ );
	
	return interpret_urb_result(ptp_usb, pipe, length, result, 
			ptp_usb->current_urb->actual_length);
}

int usb_mtp_bulk_read(PTP_USB *ptp_usb,unsigned int pipe,void *buf, int size ,int timeout)
{
    int result = 0; 
    int act_len = 0;
    
    // printk("%s : ptp_usb = 0x%x, buf = 0x%x , size = %d\n",__func__,ptp_usb,buf,size);
    
    result = usb_mtp_bulk_transfer_buf(ptp_usb,pipe,buf,size,&act_len, timeout);        
    
    // printk("%s : result = 0x%x \n",__func__,result);
    
    /*
    if ((result != USB_MTP_XFER_GOOD) && (result != USB_MTP_XFER_LONG))            
        return -EIO;
    */    
    if (result != USB_MTP_XFER_GOOD)    
        return -EIO;
            
    return act_len;    
}

int usb_mtp_bulk_write(PTP_USB *ptp_usb,unsigned int pipe,void *buf, int size ,int timeout)
{
    int result = 0; 
    int act_len = 0;
    
    // printk("%s : ptp_usb = 0x%x, buf = 0x%x , size = %d\n",__func__,ptp_usb,buf,size);    
    
    result = usb_mtp_bulk_transfer_buf(ptp_usb,pipe,buf,size,&act_len, timeout);
    if (result != USB_MTP_XFER_GOOD)
        return -EIO;
    
    return act_len;
}

