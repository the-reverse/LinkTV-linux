/*
 * Copyright (C) 2001-2004 by David Brownell
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

/* this file is part of ehci-hcd.c */

/*-------------------------------------------------------------------------*/

/*
 * EHCI hardware queue manipulation ... the core.  QH/QTD manipulation.
 *
 * Control, bulk, and interrupt traffic all use "qh" lists.  They list "qtd"
 * entries describing USB transactions, max 16-20kB/entry (with 4kB-aligned
 * buffers needed for the larger number).  We use one QH per endpoint, queue
 * multiple urbs (all three types) per endpoint.  URBs may need several qtds.
 *
 * ISO traffic uses "ISO TD" (itd, and sitd) records, and (along with
 * interrupts) needs careful scheduling.  Performance improvements can be
 * an ongoing challenge.  That's in "ehci-sched.c".
 * 
 * USB 1.1 devices are handled (a) by "companion" OHCI or UHCI root hubs,
 * or otherwise through transaction translators (TTs) in USB 2.0 hubs using
 * (b) special fields in qh entries or (c) split iso entries.  TTs will
 * buffer low/full speed data so the host collects it at high speed.
 */

/*-------------------------------------------------------------------------*/

#ifdef USB_EHCI_DEBUG_QH_QTD
static void ehci_debug_qh(struct ehci_qh *qh)
{
	printk("====================================\n");
	printk(" qh\t\t\t = %p\n", qh);
	printk(" qh->hw_next\t\t = %p\n", qh->hw_next);
	printk(" qh->hw_info1\t\t = %p\n", qh->hw_info1);
	printk(" qh->hw_info2\t\t = %p\n", qh->hw_info2);
	printk(" qh->hw_current\t\t = %p\n", qh->hw_current);
	printk(" qh->hw_qtd_next\t = %p\n", qh->hw_qtd_next);
	printk(" qh->hw_alt_next\t = %p\n", qh->hw_alt_next);
	printk(" qh->hw_token\t\t = %p\n", qh->hw_token);
	printk(" qh->hw_buf[0]\t\t = %p\n", qh->hw_buf[0]);
	printk(" qh->hw_buf[1]\t\t = %p\n", qh->hw_buf[1]);
	printk(" qh->hw_buf[2]\t\t = %p\n", qh->hw_buf[2]);
	printk(" qh->hw_buf[3]\t\t = %p\n", qh->hw_buf[3]);
	printk(" qh->hw_buf[4]\t\t = %p\n", qh->hw_buf[4]);
	printk(" qh->hw_buf_hi[0]\t = %p\n", qh->hw_buf_hi[0]);
	printk(" qh->hw_buf_hi[1]\t = %p\n", qh->hw_buf_hi[1]);
	printk(" qh->hw_buf_hi[2]\t = %p\n", qh->hw_buf_hi[2]);
	printk(" qh->hw_buf_hi[3]\t = %p\n", qh->hw_buf_hi[3]);
	printk(" qh->hw_buf_hi[4]\t = %p\n", qh->hw_buf_hi[4]);
	printk(" qh->qh_dma\t\t = %p\n", qh->qh_dma);
	printk(" qh->qh_next\t\t = %p\n", qh->qh_next);
	printk(" qh->qtd_list\t\t = %p\n", qh->qtd_list);
	printk(" qh->dummy\t\t = %p\n", qh->dummy);
	printk(" qh->reclaim\t\t = %p\n", qh->reclaim);
	printk(" qh->ehci\t\t = %p\n", qh->ehci);
	printk(" qh->kref\t\t = %p\n", qh->kref);
	printk(" qh->stamp\t\t = %p\n", qh->stamp);
	printk(" qh->qh_state\t\t = %p\n", qh->qh_state);
	printk(" qh->usecs\t\t = %p\n", qh->usecs);
	printk(" qh->gap_uf\t\t = %p\n", qh->gap_uf);
	printk(" qh->c_usecs\t\t = %p\n", qh->c_usecs);
	printk(" qh->period\t\t = %p\n", qh->period);
	printk(" qh->start\t\t = %p\n", qh->start);
	printk(" qh->dev\t\t = %p\n", qh->dev);
	printk("====================================\n\n");

	return;
}

static void ehci_debug_qtd(struct ehci_qtd *qtd)
{
	printk("====================================\n");
	printk(" qtd\t\t\t = %p\n", qtd);
	printk(" qtd->hw_next\t\t = %p\n", qtd->hw_next);
	printk(" qtd->hw_alt_next\t = %p\n", qtd->hw_alt_next);
	printk(" qtd->hw_token\t\t = %p\n", qtd->hw_token);
	printk(" qtd->hw_buf[0]\t\t = %p\n", qtd->hw_buf[0]);
	printk(" qtd->hw_buf[1]\t\t = %p\n", qtd->hw_buf[1]);
	printk(" qtd->hw_buf[2]\t\t = %p\n", qtd->hw_buf[2]);
	printk(" qtd->hw_buf[3]\t\t = %p\n", qtd->hw_buf[3]);
	printk(" qtd->hw_buf[4]\t\t = %p\n", qtd->hw_buf[4]);
	printk(" qtd->hw_buf_hi[0]\t = %p\n", qtd->hw_buf_hi[0]);
	printk(" qtd->hw_buf_hi[1]\t = %p\n", qtd->hw_buf_hi[1]);
	printk(" qtd->hw_buf_hi[2]\t = %p\n", qtd->hw_buf_hi[2]);
	printk(" qtd->hw_buf_hi[3]\t = %p\n", qtd->hw_buf_hi[3]);
	printk(" qtd->hw_buf_hi[4]\t = %p\n", qtd->hw_buf_hi[4]);
	printk(" qtd->qtd_dma\t\t = %p\n", qtd->qtd_dma);
	printk(" qtd->qh_list\t\t = %p\n", qtd->qtd_list);
	printk(" qtd->urb\t\t = %p\n", qtd->urb);
	printk(" qtd->length\t\t = %p\n", qtd->length);
	printk("====================================\n\n");

	return;
}
#endif /* USB_EHCI_DEBUG_QH_QTD */

/* fill a qtd, returning how much of the buffer we were able to queue up */

static int
qtd_fill (struct ehci_qtd *qtd, dma_addr_t buf, size_t len,
		int token, int maxpacket)
{
	int	i, count;
	u64	addr = buf;

	/* one buffer entry per 4K ... first might be short or unaligned */
	qtd->hw_buf [0] = cpu_to_le32 ((u32)addr);
	qtd->hw_buf_hi [0] = cpu_to_le32 ((u32)(addr >> 32));
	count = 0x1000 - (buf & 0x0fff);	/* rest of that page */
	if (likely (len < count))		/* ... iff needed */
		count = len;
	else {
		buf +=  0x1000;
		buf &= ~0x0fff;

		/* per-qtd limit: from 16K to 20K (best alignment) */
		for (i = 1; count < len && i < 5; i++) {
			addr = buf;
			qtd->hw_buf [i] = cpu_to_le32 ((u32)addr);
			qtd->hw_buf_hi [i] = cpu_to_le32 ((u32)(addr >> 32));
			buf += 0x1000;
			if ((count + 0x1000) < len)
				count += 0x1000;
			else
				count = len;
		}

		/* short packets may only terminate transfers */
		if (count != len)
			count -= (count % maxpacket);
	}
	qtd->hw_token = cpu_to_le32 ((count << 16) | token);
	qtd->length = count;

#ifdef USB_EHCI_CHECK_ALIGNMENT
	if(is_venus_cpu() || is_neptune_cpu())
	if ((unsigned int)qtd->hw_buf[0] & (USB_EHCI_CHECK_ALIGNMENT_SIZE - 1))
	{
		if(count > (int)(USB_EHCI_CHECK_ALIGNMENT_SIZE - ((unsigned int)qtd->hw_buf[0] & (USB_EHCI_CHECK_ALIGNMENT_SIZE - 1))))
		{
			// ehci qtd hw buffer cross 1k byte boundary
			printk("#### [cfyeh] EHCI Cross 0x%x Bytes : qtd->hw_buf[0] = 0x%x, qtd->length = 0x%x\n", USB_EHCI_CHECK_ALIGNMENT_SIZE, qtd->hw_buf[0], count);
			WARN_ON(1);
		}
	}
#endif /* USB_EHCI_CHECK_ALIGNMENT */

	return count;
}

/*-------------------------------------------------------------------------*/

static inline void
qh_update (struct ehci_hcd *ehci, struct ehci_qh *qh, struct ehci_qtd *qtd)
{
	/* writes to an active overlay are unsafe */
	BUG_ON(qh->qh_state != QH_STATE_IDLE);

	qh->hw_qtd_next = QTD_NEXT (qtd->qtd_dma);
	qh->hw_alt_next = EHCI_LIST_END;

	/* Except for control endpoints, we make hardware maintain data
	 * toggle (like OHCI) ... here (re)initialize the toggle in the QH,
	 * and set the pseudo-toggle in udev. Only usb_clear_halt() will
	 * ever clear it.
	 */
	if (!(qh->hw_info1 & cpu_to_le32(1 << 14))) {
		unsigned	is_out, epnum;

		is_out = !(qtd->hw_token & cpu_to_le32(1 << 8));
		epnum = (le32_to_cpup(&qh->hw_info1) >> 8) & 0x0f;
		if (unlikely (!usb_gettoggle (qh->dev, epnum, is_out))) {
			qh->hw_token &= ~__constant_cpu_to_le32 (QTD_TOGGLE);
			usb_settoggle (qh->dev, epnum, is_out, 1);
		}
	}

	/* HC must see latest qtd and qh data before we clear ACTIVE+HALT */
	wmb ();
	qh->hw_token &= __constant_cpu_to_le32 (QTD_TOGGLE | QTD_STS_PING);
}

/* if it weren't for a common silicon quirk (writing the dummy into the qh
 * overlay, so qh->hw_token wrongly becomes inactive/halted), only fault
 * recovery (including urb dequeue) would need software changes to a QH...
 */
static void
qh_refresh (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	struct ehci_qtd *qtd;

	if (list_empty (&qh->qtd_list))
		qtd = qh->dummy;
	else {
		qtd = list_entry (qh->qtd_list.next,
				struct ehci_qtd, qtd_list);
		/* first qtd may already be partially processed */
		if (cpu_to_le32 (qtd->qtd_dma) == qh->hw_current)
			qtd = NULL;
	}

	if (qtd)
		qh_update (ehci, qh, qtd);
}

/*-------------------------------------------------------------------------*/

static void qtd_copy_status (
	struct ehci_hcd *ehci,
	struct urb *urb,
	size_t length,
	u32 token
)
{
	int i, len, offset;
	unsigned char * ptr;
	/* count IN/OUT bytes, not SETUP (even short packets) */
	if (likely (QTD_PID (token) != 2))
		urb->actual_length += length - QTD_LENGTH (token);

	/* don't modify error codes */
	if (unlikely (urb->status != -EINPROGRESS))
		return;

	/* force cleanup after short read; not always an error */
	if (unlikely (IS_SHORT_READ (token)))
		urb->status = -EREMOTEIO;

	/* serious "can't proceed" faults reported by the hardware */
	if (token & QTD_STS_HALT) {
		if (token & QTD_STS_BABBLE) {
			/* FIXME "must" disable babbling device's port too */
			urb->status = -EOVERFLOW;
		} else if (token & QTD_STS_MMF) {
			/* fs/ls interrupt xfer missed the complete-split */
			urb->status = -EPROTO;
		} else if (token & QTD_STS_DBE) {
			urb->status = (QTD_PID (token) == 1) /* IN ? */
				? -ENOSR  /* hc couldn't read data */
				: -ECOMM; /* hc couldn't write data */
		} else if (token & QTD_STS_XACT) {
			/* timeout, bad crc, wrong PID, etc; retried */
			if (QTD_CERR (token))
				urb->status = -EPIPE;
			else {
				ehci_dbg (ehci, "devpath %s ep%d%s 3strikes\n",
					urb->dev->devpath,
					usb_pipeendpoint (urb->pipe),
					usb_pipein (urb->pipe) ? "in" : "out");
				urb->status = -EPROTO;
#if 0
				printk("actual_length 0x%x transfer_buffer_length 0x%x\n", urb->actual_length, urb->transfer_buffer_length);
				offset = ((urb->actual_length - 1) / 0x400)* 0x400;
				printk("offset 0x%x\n", offset);
				ptr = KSEG1ADDR(urb->transfer_dma);
				for(i=offset; i < (urb->actual_length + 0x400); i++) {
					if((i % 64) == 0)
						printk("0x%.8x: ", i);
					printk("%.2x ", ptr[i]);
					if((i % 64) == 63)
						printk("\n");
				}
#endif
			}
		/* CERR nonzero + no errors + halt --> stall */
		} else if (QTD_CERR (token))
			urb->status = -EPIPE;
		else	/* unknown */
			urb->status = -EPROTO;

		ehci_vdbg (ehci,
			"dev%d ep%d%s qtd token %08x --> status %d\n",
			usb_pipedevice (urb->pipe),
			usb_pipeendpoint (urb->pipe),
			usb_pipein (urb->pipe) ? "in" : "out",
			token, urb->status);

		/* if async CSPLIT failed, try cleaning out the TT buffer */
		if (urb->status != -EPIPE
				&& urb->dev->tt && !usb_pipeint (urb->pipe)
				&& ((token & QTD_STS_MMF) != 0
					|| QTD_CERR(token) == 0)
				&& (!ehci_is_TDI(ehci)
                	                || urb->dev->tt->hub !=
					   ehci_to_hcd(ehci)->self.root_hub)) {
#ifdef DEBUG
			struct usb_device *tt = urb->dev->tt->hub;
			dev_dbg (&tt->dev,
				"clear tt buffer port %d, a%d ep%d t%08x\n",
				urb->dev->ttport, urb->dev->devnum,
				usb_pipeendpoint (urb->pipe), token);
#endif /* DEBUG */
			usb_hub_tt_clear_buffer (urb->dev, urb->pipe);
		}
	}
}

extern int bForEhciDebug;

static void
ehci_urb_done (struct ehci_hcd *ehci, struct urb *urb, struct pt_regs *regs)
__releases(ehci->lock)
__acquires(ehci->lock)
{
	if (likely (urb->hcpriv != NULL)) {
		struct ehci_qh	*qh = (struct ehci_qh *) urb->hcpriv;

		/* S-mask in a QH means it's an interrupt urb */
		if ((qh->hw_info2 & __constant_cpu_to_le32 (QH_SMASK)) != 0) {

			/* ... update hc-wide periodic stats (for usbfs) */
			ehci_to_hcd(ehci)->self.bandwidth_int_reqs--;
		}
		qh_put (qh);
	}

	spin_lock (&urb->lock);
	urb->hcpriv = NULL;
	switch (urb->status) {
	case -EINPROGRESS:		/* success */
		urb->status = 0;
	default:			/* fault */
		COUNT (ehci->stats.complete);
		break;
	case -EREMOTEIO:		/* fault or normal */
		if (!(urb->transfer_flags & URB_SHORT_NOT_OK))
			urb->status = 0;
		COUNT (ehci->stats.complete);
		break;
	case -ECONNRESET:		/* canceled */
	case -ENOENT:
		COUNT (ehci->stats.unlink);
		break;
	}
	spin_unlock (&urb->lock);

#ifdef EHCI_URB_TRACE
	ehci_dbg (ehci,
		"%s %s urb %p ep%d%s status %d len %d/%d\n",
		__FUNCTION__, urb->dev->devpath, urb,
		usb_pipeendpoint (urb->pipe),
		usb_pipein (urb->pipe) ? "in" : "out",
		urb->status,
		urb->actual_length, urb->transfer_buffer_length);
#else
	if(bForEhciDebug)
	ehci_force_dbg (ehci,
		"%s %s urb %p ep%d%s status %d len %d/%d\n",
		__FUNCTION__, urb->dev->devpath, urb,
		usb_pipeendpoint (urb->pipe),
		usb_pipein (urb->pipe) ? "in" : "out",
		urb->status,
		urb->actual_length, urb->transfer_buffer_length);
#endif

	/* complete() can reenter this HCD */
	spin_unlock (&ehci->lock);
	usb_hcd_giveback_urb (ehci_to_hcd(ehci), urb, regs);
	spin_lock (&ehci->lock);
}

static void start_unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh);
static void unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh);

static void intr_deschedule (struct ehci_hcd *ehci, struct ehci_qh *qh);
static int qh_schedule (struct ehci_hcd *ehci, struct ehci_qh *qh);


#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING
static inline void ehci_connection_stat_polling(struct ehci_qtd	*_qtd)
{
	struct usb_device *dev;
	struct urb	*urb;
	int port = 0;
			
	if(!is_mars_cpu()) // for mars
		return;
	// VENUS_SB2_CHIP_INFO: bit[31,16]
	// 0xa0 => A version
	// 0xb0 => B version
	if((inl(VENUS_SB2_CHIP_INFO) >> 16) == 0xa0)
		return;

	if (!_qtd)
		return;
	
	urb = _qtd->urb;				
	dev = urb->dev;
	
	if ((!urb) || (!dev))
		return;
						
	port = dev->devpath[0] - '1';			
	// printk("devpath = %s\n",dev->devpath);	
	
	if ((port > 2) || (port < 0)) {			
		printk("%s [Error] : devpath = %s\n",__func__,dev->devpath);	
		return;
	}
				
	// printk("urb->status = 0x%x\n",urb->status);	
	// printk("port = %d\n", port);					
				
	if (urb->status == 0)
		ehci_port_jiffies[port] = jiffies;
}
#endif // #ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING

/*
 * Process and free completed qtds for a qh, returning URBs to drivers.
 * Chases up to qh->hw_current.  Returns number of completions called,
 * indicating how much "real" work we did.
 */
#define HALT_BIT __constant_cpu_to_le32(QTD_STS_HALT)
static unsigned
qh_completions (struct ehci_hcd *ehci, struct ehci_qh *qh, struct pt_regs *regs)
{
	struct ehci_qtd		*last = NULL, *end = qh->dummy;
	struct list_head	*entry, *tmp;
	int			stopped;
	unsigned		count = 0;
	int			do_status = 0;
	u8			state;

	if (unlikely (list_empty (&qh->qtd_list)))
		return count;

	/* completions (or tasks on other cpus) must never clobber HALT
	 * till we've gone through and cleaned everything up, even when
	 * they add urbs to this qh's queue or mark them for unlinking.
	 *
	 * NOTE:  unlinking expects to be done in queue order.
	 */
	state = qh->qh_state;
	qh->qh_state = QH_STATE_COMPLETING;
	stopped = (state == QH_STATE_IDLE);

	/* remove de-activated QTDs from front of queue.
	 * after faults (including short reads), cleanup this urb
	 * then let the queue advance.
	 * if queue is stopped, handles unlinks.
	 */
	list_for_each_safe (entry, tmp, &qh->qtd_list) {
		struct ehci_qtd	*qtd;
		struct urb	*urb;
		u32		token = 0;

		qtd = list_entry (entry, struct ehci_qtd, qtd_list);
		urb = qtd->urb;

		/* clean up any state from previous QTD ...*/
		if (last) {
			if (likely (last->urb != urb)) {
				ehci_urb_done (ehci, last->urb, regs);
#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING
				ehci_connection_stat_polling(last);		
#endif /* USB_MARS_EHCI_CONNECTION_STATE_POLLING */
				count++;
			}
			ehci_qtd_free (ehci, last);
			last = NULL;
		}

		/* ignore urbs submitted during completions we reported */
		if (qtd == end)
			break;

		/* hardware copies qtd out of qh overlay */
		rmb ();
		token = le32_to_cpu (qtd->hw_token);

		/* always clean up qtds the hc de-activated */
		if ((token & QTD_STS_ACTIVE) == 0) {

			if ((token & QTD_STS_HALT) != 0) {
				stopped = 1;

			/* magic dummy for some short reads; qh won't advance.
			 * that silicon quirk can kick in with this dummy too.
			 */
			} else if (IS_SHORT_READ (token)
					&& !(qtd->hw_alt_next & EHCI_LIST_END)) {
				stopped = 1;
				goto halt;
			}

		/* stop scanning when we reach qtds the hc is using */
		} else if (likely (!stopped
				&& HC_IS_RUNNING (ehci_to_hcd(ehci)->state))) {
			break;

		} else {
			stopped = 1;

			if (unlikely (!HC_IS_RUNNING (ehci_to_hcd(ehci)->state)))
				urb->status = -ESHUTDOWN;

			/* ignore active urbs unless some previous qtd
			 * for the urb faulted (including short read) or
			 * its urb was canceled.  we may patch qh or qtds.
			 */
			if (likely (urb->status == -EINPROGRESS))
				continue;
			
			/* issue status after short control reads */
			if (unlikely (do_status != 0)
					&& QTD_PID (token) == 0 /* OUT */) {
				do_status = 0;
				continue;
			}

			/* token in overlay may be most current */
			if (state == QH_STATE_IDLE
					&& cpu_to_le32 (qtd->qtd_dma)
						== qh->hw_current)
				token = le32_to_cpu (qh->hw_token);

			/* force halt for unlinked or blocked qh, so we'll
			 * patch the qh later and so that completions can't
			 * activate it while we "know" it's stopped.
			 */
			if ((HALT_BIT & qh->hw_token) == 0) {
halt:
				qh->hw_token |= HALT_BIT;
				wmb ();
			}
		}
 
		/* remove it from the queue */
		spin_lock (&urb->lock);
		qtd_copy_status (ehci, urb, qtd->length, token);
		do_status = (urb->status == -EREMOTEIO)
				&& usb_pipecontrol (urb->pipe);
		spin_unlock (&urb->lock);

		if (stopped && qtd->qtd_list.prev != &qh->qtd_list) {
			last = list_entry (qtd->qtd_list.prev,
					struct ehci_qtd, qtd_list);
			last->hw_next = qtd->hw_next;
		}
		list_del (&qtd->qtd_list);
		last = qtd;
	}

	/* last urb's completion might still need calling */
	if (likely (last != NULL)) {
		ehci_urb_done (ehci, last->urb, regs);
#ifdef USB_MARS_EHCI_CONNECTION_STATE_POLLING
		ehci_connection_stat_polling(last);		
#endif /* USB_MARS_EHCI_CONNECTION_STATE_POLLING */
		count++;
		ehci_qtd_free (ehci, last);
	}

	/* restore original state; caller must unlink or relink */
	qh->qh_state = state;

	/* be sure the hardware's done with the qh before refreshing
	 * it after fault cleanup, or recovering from silicon wrongly
	 * overlaying the dummy qtd (which reduces DMA chatter).
	 */
	if (stopped != 0 || qh->hw_qtd_next == EHCI_LIST_END) {
		switch (state) {
		case QH_STATE_IDLE:
			qh_refresh(ehci, qh);
			break;
		case QH_STATE_LINKED:
			/* should be rare for periodic transfers,
			 * except maybe high bandwidth ...
			 */
			if ((__constant_cpu_to_le32 (QH_SMASK)
					& qh->hw_info2) != 0) {
				intr_deschedule (ehci, qh);
				(void) qh_schedule (ehci, qh);
			} else
				unlink_async (ehci, qh);
			break;
		/* otherwise, unlink already started */
		}
	}

	return count;
}

/*-------------------------------------------------------------------------*/

// high bandwidth multiplier, as encoded in highspeed endpoint descriptors
#define hb_mult(wMaxPacketSize) (1 + (((wMaxPacketSize) >> 11) & 0x03))
// ... and packet size, for any kind of endpoint descriptor
#define max_packet(wMaxPacketSize) ((wMaxPacketSize) & 0x07ff)

/*
 * reverse of qh_urb_transaction:  free a list of TDs.
 * used for cleanup after errors, before HC sees an URB's TDs.
 */
static void qtd_list_free (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*qtd_list
) {
	struct list_head	*entry, *temp;

	list_for_each_safe (entry, temp, qtd_list) {
		struct ehci_qtd	*qtd;

		qtd = list_entry (entry, struct ehci_qtd, qtd_list);
		list_del (&qtd->qtd_list);
		ehci_qtd_free (ehci, qtd);
	}
}

/*
 * create a list of filled qtds for this URB; won't link into qh.
 */
static struct list_head *
qh_urb_transaction (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*head,
	int			flags
) {
	struct ehci_qtd		*qtd, *qtd_prev;
	dma_addr_t		buf;
	int			len, maxpacket;
	int			is_input;
	u32			token;

	/*
	 * URBs map to sequences of QTDs:  one logical transaction
	 */
	qtd = ehci_qtd_alloc (ehci, flags);
	if (unlikely (!qtd))
		return NULL;
	list_add_tail (&qtd->qtd_list, head);
	qtd->urb = urb;

	token = QTD_STS_ACTIVE;
	token |= (EHCI_TUNE_CERR << 10);
	/* for split transactions, SplitXState initialized to zero */

	len = urb->transfer_buffer_length;
	is_input = usb_pipein (urb->pipe);
	if (usb_pipecontrol (urb->pipe)) {
		/* SETUP pid */
		qtd_fill (qtd, urb->setup_dma, sizeof (struct usb_ctrlrequest),
			token | (2 /* "setup" */ << 8), 8);

		/* ... and always at least one more pid */
		token ^= QTD_TOGGLE;
		qtd_prev = qtd;
		qtd = ehci_qtd_alloc (ehci, flags);
		if (unlikely (!qtd))
			goto cleanup;
		qtd->urb = urb;
		qtd_prev->hw_next = QTD_NEXT (qtd->qtd_dma);
		list_add_tail (&qtd->qtd_list, head);
	} 

	/*
	 * data transfer stage:  buffer setup
	 */
	if (likely (len > 0))
		buf = urb->transfer_dma;
	else
		buf = 0;

	/* for zero length DATA stages, STATUS is always IN */
	if (!buf || is_input)
		token |= (1 /* "in" */ << 8);
	/* else it's already initted to "out" pid (0 << 8) */

	maxpacket = max_packet(usb_maxpacket(urb->dev, urb->pipe, !is_input));

	/*
	 * buffer gets wrapped in one or more qtds;
	 * last one may be "short" (including zero len)
	 * and may serve as a control status ack
	 */
	for (;;) {
		int this_qtd_len;

		this_qtd_len = qtd_fill (qtd, buf, len, token, maxpacket);
		len -= this_qtd_len;
		buf += this_qtd_len;
		if (is_input)
			qtd->hw_alt_next = ehci->async->hw_alt_next;

		/* qh makes control packets use qtd toggle; maybe switch it */
		if ((maxpacket & (this_qtd_len + (maxpacket - 1))) == 0)
			token ^= QTD_TOGGLE;

		if (likely (len <= 0))
			break;

		qtd_prev = qtd;
		qtd = ehci_qtd_alloc (ehci, flags);
		if (unlikely (!qtd))
			goto cleanup;
		qtd->urb = urb;
		qtd_prev->hw_next = QTD_NEXT (qtd->qtd_dma);
		list_add_tail (&qtd->qtd_list, head);
	}

	/* unless the bulk/interrupt caller wants a chance to clean
	 * up after short reads, hc should advance qh past this urb
	 */
	if (likely ((urb->transfer_flags & URB_SHORT_NOT_OK) == 0
				|| usb_pipecontrol (urb->pipe)))
		qtd->hw_alt_next = EHCI_LIST_END;

	/*
	 * control requests may need a terminating data "status" ack;
	 * bulk ones may need a terminating short packet (zero length).
	 */
	if (likely (buf != 0)) {
		int	one_more = 0;

		if (usb_pipecontrol (urb->pipe)) {
			one_more = 1;
			token ^= 0x0100;	/* "in" <--> "out"  */
			token |= QTD_TOGGLE;	/* force DATA1 */
		} else if (usb_pipebulk (urb->pipe)
				&& (urb->transfer_flags & URB_ZERO_PACKET)
				&& !(urb->transfer_buffer_length % maxpacket)) {
			one_more = 1;
		}
		if (one_more) {
			qtd_prev = qtd;
			qtd = ehci_qtd_alloc (ehci, flags);
			if (unlikely (!qtd))
				goto cleanup;
			qtd->urb = urb;
			qtd_prev->hw_next = QTD_NEXT (qtd->qtd_dma);
			list_add_tail (&qtd->qtd_list, head);

			/* never any data in such packets */
			qtd_fill (qtd, 0, 0, token, 0);
		}
	}

	/* by default, enable interrupt on urb completion */
	if (likely (!(urb->transfer_flags & URB_NO_INTERRUPT)))
		qtd->hw_token |= __constant_cpu_to_le32 (QTD_IOC);
	return head;

cleanup:
	qtd_list_free (ehci, urb, head);
	return NULL;
}

/*-------------------------------------------------------------------------*/

// Would be best to create all qh's from config descriptors,
// when each interface/altsetting is established.  Unlink
// any previous qh and cancel its urbs first; endpoints are
// implicitly reset then (data toggle too).
// That'd mean updating how usbcore talks to HCDs. (2.7?)


/*
 * Each QH holds a qtd list; a QH is used for everything except iso.
 *
 * For interrupt urbs, the scheduler must set the microframe scheduling
 * mask(s) each time the QH gets scheduled.  For highspeed, that's
 * just one microframe in the s-mask.  For split interrupt transactions
 * there are additional complications: c-mask, maybe FSTNs.
 */
static struct ehci_qh *
qh_make (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	int			flags
) {
	struct ehci_qh		*qh = ehci_qh_alloc (ehci, flags);
	u32			info1 = 0, info2 = 0;
	int			is_input, type;
	int			maxp = 0;

	if (!qh)
		return qh;

	/*
	 * init endpoint/device data for this QH
	 */
	info1 |= usb_pipeendpoint (urb->pipe) << 8;
	info1 |= usb_pipedevice (urb->pipe) << 0;

	is_input = usb_pipein (urb->pipe);
	type = usb_pipetype (urb->pipe);
	maxp = usb_maxpacket (urb->dev, urb->pipe, !is_input);

	/* Compute interrupt scheduling parameters just once, and save.
	 * - allowing for high bandwidth, how many nsec/uframe are used?
	 * - split transactions need a second CSPLIT uframe; same question
	 * - splits also need a schedule gap (for full/low speed I/O)
	 * - qh has a polling interval
	 *
	 * For control/bulk requests, the HC or TT handles these.
	 */
	if (type == PIPE_INTERRUPT) {
		qh->usecs = NS_TO_US (usb_calc_bus_time (USB_SPEED_HIGH, is_input, 0,
				hb_mult (maxp) * max_packet (maxp)));
		qh->start = NO_FRAME;

		if (urb->dev->speed == USB_SPEED_HIGH) {
			qh->c_usecs = 0;
			qh->gap_uf = 0;

			qh->period = urb->interval >> 3;
			if (qh->period == 0 && urb->interval != 1) {
				/* NOTE interval 2 or 4 uframes could work.
				 * But interval 1 scheduling is simpler, and
				 * includes high bandwidth.
				 */
				dbg ("intr period %d uframes, NYET!",
						urb->interval);
				goto done;
			}
		} else {
			struct usb_tt	*tt = urb->dev->tt;
			int		think_time;

			/* gap is f(FS/LS transfer times) */
			qh->gap_uf = 1 + usb_calc_bus_time (urb->dev->speed,
					is_input, 0, maxp) / (125 * 1000);

			/* FIXME this just approximates SPLIT/CSPLIT times */
			if (is_input) {		// SPLIT, gap, CSPLIT+DATA
				qh->c_usecs = qh->usecs + HS_USECS (0);
				qh->usecs = HS_USECS (1);
			} else {		// SPLIT+DATA, gap, CSPLIT
				qh->usecs += HS_USECS (1);
				qh->c_usecs = HS_USECS (0);
			}

			think_time = tt ? tt->think_time : 0;
			qh->tt_usecs = NS_TO_US (think_time +
					usb_calc_bus_time (urb->dev->speed,
					is_input, 0, max_packet (maxp)));
			qh->period = urb->interval;
		}
	}

	/* support for tt scheduling, and access to toggles */
	qh->dev = usb_get_dev (urb->dev);

	/* using TT? */
	switch (urb->dev->speed) {
	case USB_SPEED_LOW:
		info1 |= (1 << 12);	/* EPS "low" */
		/* FALL THROUGH */

	case USB_SPEED_FULL:
		/* EPS 0 means "full" */
		if (type != PIPE_INTERRUPT)
			info1 |= (EHCI_TUNE_RL_TT << 28);
		if (type == PIPE_CONTROL) {
			info1 |= (1 << 27);	/* for TT */
			info1 |= 1 << 14;	/* toggle from qtd */
		}
		info1 |= maxp << 16;

		info2 |= (EHCI_TUNE_MULT_TT << 30);
		info2 |= urb->dev->ttport << 23;

		/* set the address of the TT; for TDI's integrated
		 * root hub tt, leave it zeroed.
		 */
		if (!ehci_is_TDI(ehci)
				|| urb->dev->tt->hub !=
					ehci_to_hcd(ehci)->self.root_hub)
			info2 |= urb->dev->tt->hub->devnum << 16;

		/* NOTE:  if (PIPE_INTERRUPT) { scheduler sets c-mask } */

		break;

	case USB_SPEED_HIGH:		/* no TT involved */
		info1 |= (2 << 12);	/* EPS "high" */
		if (type == PIPE_CONTROL) {
			info1 |= (EHCI_TUNE_RL_HS << 28);
			info1 |= 64 << 16;	/* usb2 fixed maxpacket */
			info1 |= 1 << 14;	/* toggle from qtd */
			info2 |= (EHCI_TUNE_MULT_HS << 30);
		} else if (type == PIPE_BULK) {
			info1 |= (EHCI_TUNE_RL_HS << 28);
			info1 |= max_packet (maxp) << 16;
			info2 |= (EHCI_TUNE_MULT_HS << 30);
		} else {		/* PIPE_INTERRUPT */
			info1 |= max_packet (maxp) << 16;
			info2 |= hb_mult (maxp) << 30;
		}
		break;
	default:
 		dbg ("bogus dev %p speed %d", urb->dev, urb->dev->speed);
done:
		qh_put (qh);
		return NULL;
	}

	/* NOTE:  if (PIPE_INTERRUPT) { scheduler sets s-mask } */

	/* init as live, toggle clear, advance to dummy */
	qh->qh_state = QH_STATE_IDLE;
	qh->hw_info1 = cpu_to_le32 (info1);
	qh->hw_info2 = cpu_to_le32 (info2);
	usb_settoggle (urb->dev, usb_pipeendpoint (urb->pipe), !is_input, 1);
	qh_refresh (ehci, qh);
	return qh;
}

/*-------------------------------------------------------------------------*/

/* move qh (and its qtds) onto async queue; maybe enable queue.  */

static void qh_link_async (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	__le32		dma = QH_NEXT (qh->qh_dma);
	struct ehci_qh	*head;

	/* (re)start the async schedule? */
	head = ehci->async;
#ifndef DISABLE_EHCI_WATCHDOG
	timer_action_done (ehci, TIMER_ASYNC_OFF);
#endif /* ! DISABLE_EHCI_WATCHDOG */
	if (!head->qh_next.qh) {
		u32	cmd = readl (&ehci->regs->command);

		if (!(cmd & CMD_ASE)) {
			/* in case a clear of CMD_ASE didn't take yet */
			(void) handshake (&ehci->regs->status, STS_ASS, 0, 150);
			cmd |= CMD_ASE | CMD_RUN;
			writel (cmd, &ehci->regs->command);
			ehci_to_hcd(ehci)->state = HC_STATE_RUNNING;
			/* posted write need not be known to HC yet ... */
		}
	}

	/* clear halt and/or toggle; and maybe recover from silicon quirk */
	if (qh->qh_state == QH_STATE_IDLE)
		qh_refresh (ehci, qh);

	/* splice right after start */
	qh->qh_next = head->qh_next;
	qh->hw_next = head->hw_next;
	wmb ();

	head->qh_next.qh = qh;
	head->hw_next = dma;

	qh->qh_state = QH_STATE_LINKED;
	/* qtd completions reported later by interrupt */
}

/*-------------------------------------------------------------------------*/

#define	QH_ADDR_MASK	__constant_cpu_to_le32(0x7f)

/*
 * For control/bulk/interrupt, return QH with these TDs appended.
 * Allocates and initializes the QH if necessary.
 * Returns null if it can't allocate a QH it needs to.
 * If the QH has TDs (urbs) already, that's great.
 */
static struct ehci_qh *qh_append_tds (
	struct ehci_hcd		*ehci,
	struct urb		*urb,
	struct list_head	*qtd_list,
	int			epnum,
	void			**ptr
)
{
	struct ehci_qh		*qh = NULL;

	qh = (struct ehci_qh *) *ptr;
	if (unlikely (qh == NULL)) {
		/* can't sleep here, we have ehci->lock... */
		qh = qh_make (ehci, urb, GFP_ATOMIC);
		*ptr = qh;
	}
	if (likely (qh != NULL)) {
		struct ehci_qtd	*qtd;

		if (unlikely (list_empty (qtd_list)))
			qtd = NULL;
		else
			qtd = list_entry (qtd_list->next, struct ehci_qtd,
					qtd_list);

		/* control qh may need patching ... */
		if (unlikely (epnum == 0)) {

                        /* usb_reset_device() briefly reverts to address 0 */
                        if (usb_pipedevice (urb->pipe) == 0)
                                qh->hw_info1 &= ~QH_ADDR_MASK;
		}

		/* just one way to queue requests: swap with the dummy qtd.
		 * only hc or qh_refresh() ever modify the overlay.
		 */
		if (likely (qtd != NULL)) {
			struct ehci_qtd		*dummy;
			dma_addr_t		dma;
			__le32			token;

			/* to avoid racing the HC, use the dummy td instead of
			 * the first td of our list (becomes new dummy).  both
			 * tds stay deactivated until we're done, when the
			 * HC is allowed to fetch the old dummy (4.10.2).
			 */
			token = qtd->hw_token;
			qtd->hw_token = HALT_BIT;
			wmb ();
			dummy = qh->dummy;

			dma = dummy->qtd_dma;
			*dummy = *qtd;
			dummy->qtd_dma = dma;

			list_del (&qtd->qtd_list);
			list_add (&dummy->qtd_list, qtd_list);
			__list_splice (qtd_list, qh->qtd_list.prev);

			ehci_qtd_init (qtd, qtd->qtd_dma);
			qh->dummy = qtd;

			/* hc must see the new dummy at list end */
			dma = qtd->qtd_dma;
			qtd = list_entry (qh->qtd_list.prev,
					struct ehci_qtd, qtd_list);
			qtd->hw_next = QTD_NEXT (dma);

			/* let the hc process these next qtds */
			wmb ();
			dummy->hw_token = token;

			urb->hcpriv = qh_get (qh);
		}
	}
	return qh;
}

/*-------------------------------------------------------------------------*/

static int
submit_async (
	struct ehci_hcd		*ehci,
	struct usb_host_endpoint *ep,
	struct urb		*urb,
	struct list_head	*qtd_list,
	int			mem_flags
) {
	struct ehci_qtd		*qtd;
	int			epnum;
	unsigned long		flags;
	struct ehci_qh		*qh = NULL;
	int			rc = 0;

	qtd = list_entry (qtd_list->next, struct ehci_qtd, qtd_list);
	epnum = ep->desc.bEndpointAddress;

#ifdef EHCI_URB_TRACE
	ehci_dbg (ehci,
		"%s %s urb %p ep%d%s len %d, qtd %p [qh %p]\n",
		__FUNCTION__, urb->dev->devpath, urb,
		epnum & 0x0f, (epnum & USB_DIR_IN) ? "in" : "out",
		urb->transfer_buffer_length,
		qtd, ep->hcpriv);
#else
	if(bForEhciDebug)
	ehci_force_dbg (ehci,
		"%s %s urb %p ep%d%s len %d, qtd %p [qh %p]\n",
		__FUNCTION__, urb->dev->devpath, urb,
		epnum & 0x0f, (epnum & USB_DIR_IN) ? "in" : "out",
		urb->transfer_buffer_length,
		qtd, ep->hcpriv);
#endif

	spin_lock_irqsave (&ehci->lock, flags);
	qh = qh_append_tds (ehci, urb, qtd_list, epnum, &ep->hcpriv);
	if (unlikely(qh == NULL)) {
		rc = -ENOMEM;
		goto done;
	}

	/* Control/bulk operations through TTs don't need scheduling,
	 * the HC and TT handle it when the TT has a buffer ready.
	 */
	if (likely (qh->qh_state == QH_STATE_IDLE))
		qh_link_async (ehci, qh_get (qh));
 done:
	spin_unlock_irqrestore (&ehci->lock, flags);
	if (unlikely (qh == NULL))
		qtd_list_free (ehci, urb, qtd_list);
	return rc;
}

/*-------------------------------------------------------------------------*/

/* the async qh for the qtds being reclaimed are now unlinked from the HC */

static void end_unlink_async (struct ehci_hcd *ehci, struct pt_regs *regs)
{
	struct ehci_qh		*qh = ehci->reclaim;
	struct ehci_qh		*next;

#ifndef DISABLE_EHCI_WATCHDOG
	timer_action_done (ehci, TIMER_IAA_WATCHDOG);
#endif /* ! DISABLE_EHCI_WATCHDOG */

	// qh->hw_next = cpu_to_le32 (qh->qh_dma);
	qh->qh_state = QH_STATE_IDLE;
	qh->qh_next.qh = NULL;
	qh_put (qh);			// refcount from reclaim 

	/* other unlink(s) may be pending (in QH_STATE_UNLINK_WAIT) */
	next = qh->reclaim;
	ehci->reclaim = next;
	ehci->reclaim_ready = 0;
	qh->reclaim = NULL;

	qh_completions (ehci, qh, regs);

	if (!list_empty (&qh->qtd_list)
			&& HC_IS_RUNNING (ehci_to_hcd(ehci)->state))
		qh_link_async (ehci, qh);
	else {
		qh_put (qh);		// refcount from async list

#ifndef DISABLE_EHCI_WATCHDOG
		/* it's not free to turn the async schedule on/off; leave it
		 * active but idle for a while once it empties.
		 */
		if (HC_IS_RUNNING (ehci_to_hcd(ehci)->state)
				&& ehci->async->qh_next.qh == NULL)
			timer_action (ehci, TIMER_ASYNC_OFF);
#endif /* ! DISABLE_EHCI_WATCHDOG */
	}

	if (next) {
		ehci->reclaim = NULL;
		start_unlink_async (ehci, next);
	}
}

/* makes sure the async qh will become idle */
/* caller must own ehci->lock */

static void start_unlink_async (struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	int		cmd = readl (&ehci->regs->command);
	struct ehci_qh	*prev;

#ifdef DEBUG
	assert_spin_locked(&ehci->lock);
	if (ehci->reclaim
			|| (qh->qh_state != QH_STATE_LINKED
				&& qh->qh_state != QH_STATE_UNLINK_WAIT)
			)
		BUG ();
#endif

	/* stop async schedule right now? */
	if (unlikely (qh == ehci->async)) {
		/* can't get here without STS_ASS set */
		if (ehci_to_hcd(ehci)->state != HC_STATE_HALT) {
			writel (cmd & ~CMD_ASE, &ehci->regs->command);
			wmb ();
			// handshake later, if we need to
		}
#ifndef DISABLE_EHCI_WATCHDOG
		timer_action_done (ehci, TIMER_ASYNC_OFF);
#endif /* ! DISABLE_EHCI_WATCHDOG */
		return;
	} 

	qh->qh_state = QH_STATE_UNLINK;
	ehci->reclaim = qh = qh_get (qh);

	prev = ehci->async;
	while (prev->qh_next.qh != qh)
		prev = prev->qh_next.qh;

	prev->hw_next = qh->hw_next;
	prev->qh_next = qh->qh_next;
	wmb ();

	if (unlikely (ehci_to_hcd(ehci)->state == HC_STATE_HALT)) {
		/* if (unlikely (qh->reclaim != 0))
		 * 	this will recurse, probably not much
		 */
		end_unlink_async (ehci, NULL);
		return;
	}

	ehci->reclaim_ready = 0;
	cmd |= CMD_IAAD;
	writel (cmd, &ehci->regs->command);
	(void) readl (&ehci->regs->command);
#ifndef DISABLE_EHCI_WATCHDOG
	timer_action (ehci, TIMER_IAA_WATCHDOG);
#endif /* ! DISABLE_EHCI_WATCHDOG */
}

/*-------------------------------------------------------------------------*/

static void
scan_async (struct ehci_hcd *ehci, struct pt_regs *regs)
{
	struct ehci_qh		*qh;
	enum ehci_timer_action	action = TIMER_IO_WATCHDOG;

	if (!++(ehci->stamp))
		ehci->stamp++;
#ifndef DISABLE_EHCI_WATCHDOG
	timer_action_done (ehci, TIMER_ASYNC_SHRINK);
#endif /* ! DISABLE_EHCI_WATCHDOG */
rescan:
	qh = ehci->async->qh_next.qh;
	if (likely (qh != NULL)) {
		do {
			/* clean any finished work for this qh */
			if (!list_empty (&qh->qtd_list)
					&& qh->stamp != ehci->stamp) {
				int temp;

				/* unlinks could happen here; completion
				 * reporting drops the lock.  rescan using
				 * the latest schedule, but don't rescan
				 * qhs we already finished (no looping).
				 */
				qh = qh_get (qh);
				qh->stamp = ehci->stamp;
				temp = qh_completions (ehci, qh, regs);
				qh_put (qh);
				if (temp != 0) {
					goto rescan;
				}
			}

			/* unlink idle entries, reducing HC PCI usage as well
			 * as HCD schedule-scanning costs.  delay for any qh
			 * we just scanned, there's a not-unusual case that it
			 * doesn't stay idle for long.
			 * (plus, avoids some kind of re-activation race.)
			 */
			if (list_empty (&qh->qtd_list)) {
				if (qh->stamp == ehci->stamp)
					action = TIMER_ASYNC_SHRINK;
				else if (!ehci->reclaim
					    && qh->qh_state == QH_STATE_LINKED)
					start_unlink_async (ehci, qh);
			}

			qh = qh->qh_next.qh;
		} while (qh);
	}
#ifndef DISABLE_EHCI_WATCHDOG
	if (action == TIMER_ASYNC_SHRINK)
		timer_action (ehci, TIMER_ASYNC_SHRINK);
#endif /* ! DISABLE_EHCI_WATCHDOG */
}
