/*
	drivers/net/tulip/21142.c

	Maintained by Jeff Garzik <jgarzik@mandrakesoft.com>
	Copyright 2000  The Linux Kernel Team
	Written/copyright 1994-1999 by Donald Becker.

	This software may be used and distributed according to the terms
	of the GNU General Public License, incorporated herein by reference.

	Please refer to Documentation/networking/tulip.txt for more
	information on this driver.

*/

#include "tulip.h"
#include <linux/pci.h>
#include <linux/delay.h>


static u16 t21142_csr13[] = { 0x0001, 0x0009, 0x0009, 0x0000, 0x0001, };
u16 t21142_csr14[] =	    { 0xFFFF, 0x0705, 0x0705, 0x0000, 0x7F3D, };
static u16 t21142_csr15[] = { 0x0008, 0x0006, 0x000E, 0x0008, 0x0008, };


/* Handle the 21143 uniquely: do autoselect with NWay, not the EEPROM list
   of available transceivers.  */
void t21142_timer(unsigned long data)
{
	struct net_device *dev = (struct net_device *)data;
	struct tulip_private *tp = (struct tulip_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int csr12 = inl(ioaddr + CSR12);
	int next_tick = 60*HZ;
	int new_csr6 = 0;

	if (tulip_debug > 2)
		printk(KERN_INFO"%s: 21143 negotiation status %8.8x, %s.\n",
			   dev->name, csr12, medianame[dev->if_port]);
	if (tulip_media_cap[dev->if_port] & MediaIsMII) {
		tulip_check_duplex(dev);
		next_tick = 60*HZ;
	} else if (tp->nwayset) {
		/* Don't screw up a negotiated session! */
		if (tulip_debug > 1)
			printk(KERN_INFO"%s: Using NWay-set %s media, csr12 %8.8x.\n",
				   dev->name, medianame[dev->if_port], csr12);
	} else if (tp->medialock) {
			;
	} else if (dev->if_port == 3) {
		if (csr12 & 2) {	/* No 100mbps link beat, revert to 10mbps. */
			if (tulip_debug > 1)
				printk(KERN_INFO"%s: No 21143 100baseTx link beat, %8.8x, "
					   "trying NWay.\n", dev->name, csr12);
			t21142_start_nway(dev);
			next_tick = 3*HZ;
		}
	} else if ((csr12 & 0x7000) != 0x5000) {
		/* Negotiation failed.  Search media types. */
		if (tulip_debug > 1)
			printk(KERN_INFO"%s: 21143 negotiation failed, status %8.8x.\n",
				   dev->name, csr12);
		if (!(csr12 & 4)) {		/* 10mbps link beat good. */
			new_csr6 = 0x82420000;
			dev->if_port = 0;
			outl(0, ioaddr + CSR13);
			outl(0x0003FFFF, ioaddr + CSR14);
			outw(t21142_csr15[dev->if_port], ioaddr + CSR15);
			outl(t21142_csr13[dev->if_port], ioaddr + CSR13);
		} else {
			/* Select 100mbps port to check for link beat. */
			new_csr6 = 0x83860000;
			dev->if_port = 3;
			outl(0, ioaddr + CSR13);
			outl(0x0003FF7F, ioaddr + CSR14);
			outw(8, ioaddr + CSR15);
			outl(1, ioaddr + CSR13);
		}
		if (tulip_debug > 1)
			printk(KERN_INFO"%s: Testing new 21143 media %s.\n",
				   dev->name, medianame[dev->if_port]);
		if (new_csr6 != (tp->csr6 & ~0x00D5)) {
			tp->csr6 &= 0x00D5;
			tp->csr6 |= new_csr6;
			outl(0x0301, ioaddr + CSR12);
			tulip_restart_rxtx(tp, tp->csr6);
		}
		next_tick = 3*HZ;
	}

	/* mod_timer synchronizes us with potential add_timer calls
	 * from interrupts.
	 */
	mod_timer(&tp->timer, RUN_AT(next_tick));
}


void t21142_start_nway(struct net_device *dev)
{
	struct tulip_private *tp = (struct tulip_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int csr14 = ((tp->to_advertise & 0x0780) << 9)  |
		((tp->to_advertise&0x0020)<<1) | 0xffbf;

	DPRINTK("ENTER\n");

	dev->if_port = 0;
	tp->nway = tp->mediasense = 1;
	tp->nwayset = tp->lpar = 0;
	if (tulip_debug > 1)
		printk(KERN_DEBUG "%s: Restarting 21143 autonegotiation, csr14=%8.8x.\n",
			   dev->name, csr14);
	outl(0x0001, ioaddr + CSR13);
	udelay(100);
	outl(csr14, ioaddr + CSR14);
	if (tp->chip_id == PNIC2)
	  tp->csr6 = 0x01a80000 | (tp->to_advertise & 0x0040 ? 0x0200 : 0);
	else
	  tp->csr6 = 0x82420000 | (tp->to_advertise & 0x0040 ? 0x0200 : 0);
	tulip_outl_csr(tp, tp->csr6, CSR6);
	if (tp->mtable  &&  tp->mtable->csr15dir) {
		outl(tp->mtable->csr15dir, ioaddr + CSR15);
		outl(tp->mtable->csr15val, ioaddr + CSR15);
	} else
		outw(0x0008, ioaddr + CSR15);
	outl(0x1301, ioaddr + CSR12); 		/* Trigger NWAY. */
}


void t21142_lnk_change(struct net_device *dev, int csr5)
{
	struct tulip_private *tp = (struct tulip_private *)dev->priv;
	long ioaddr = dev->base_addr;
	int csr12 = inl(ioaddr + CSR12);

	if (tulip_debug > 1)
		printk(KERN_INFO"%s: 21143 link status interrupt %8.8x, CSR5 %x, "
			   "%8.8x.\n", dev->name, csr12, csr5, inl(ioaddr + CSR14));

	/* If NWay finished and we have a negotiated partner capability. */
	if (tp->nway  &&  !tp->nwayset  &&  (csr12 & 0x7000) == 0x5000) {
		int setup_done = 0;
		int negotiated = tp->to_advertise & (csr12 >> 16);
		tp->lpar = csr12 >> 16;
		tp->nwayset = 1;
		if (negotiated & 0x0100)		dev->if_port = 5;
		else if (negotiated & 0x0080)	dev->if_port = 3;
		else if (negotiated & 0x0040)	dev->if_port = 4;
		else if (negotiated & 0x0020)	dev->if_port = 0;
		else {
			tp->nwayset = 0;
			if ((csr12 & 2) == 0  &&  (tp->to_advertise & 0x0180))
				dev->if_port = 3;
		}
		tp->full_duplex = (tulip_media_cap[dev->if_port] & MediaAlwaysFD) ? 1:0;

		if (tulip_debug > 1) {
			if (tp->nwayset)
				printk(KERN_INFO "%s: Switching to %s based on link "
					   "negotiation %4.4x & %4.4x = %4.4x.\n",
					   dev->name, medianame[dev->if_port], tp->to_advertise,
					   tp->lpar, negotiated);
			else
				printk(KERN_INFO "%s: Autonegotiation failed, using %s,"
					   " link beat status %4.4x.\n",
					   dev->name, medianame[dev->if_port], csr12);
		}

		if (tp->mtable) {
			int i;
			for (i = 0; i < tp->mtable->leafcount; i++)
				if (tp->mtable->mleaf[i].media == dev->if_port) {
					tp->cur_index = i;
					tulip_select_media(dev, 0);
					setup_done = 1;
					break;
				}
		}
		if ( ! setup_done) {
			tp->csr6 = dev->if_port & 1 ? 0x83860000 : 0x82420000;
			if (tp->full_duplex)
				tp->csr6 |= 0x0200;
			outl(1, ioaddr + CSR13);
		}
#if 0							/* Restart shouldn't be needed. */
		tulip_outl_csr(tp, tp->csr6 | csr6_sr, CSR6);
		if (tulip_debug > 2)
			printk(KERN_DEBUG "%s:  Restarting Tx and Rx, CSR5 is %8.8x.\n",
				   dev->name, inl(ioaddr + CSR5));
#endif
		tulip_outl_csr(tp, tp->csr6 | csr6_st | csr6_sr, CSR6);
		if (tulip_debug > 2)
			printk(KERN_DEBUG "%s:  Setting CSR6 %8.8x/%x CSR12 %8.8x.\n",
				   dev->name, tp->csr6, inl(ioaddr + CSR6),
				   inl(ioaddr + CSR12));
	} else if ((tp->nwayset  &&  (csr5 & 0x08000000)
				&& (dev->if_port == 3  ||  dev->if_port == 5)
				&& (csr12 & 2) == 2) ||
			   (tp->nway && (csr5 & (TPLnkFail)))) {
		/* Link blew? Maybe restart NWay. */
		del_timer_sync(&tp->timer);
		t21142_start_nway(dev);
		tp->timer.expires = RUN_AT(3*HZ);
		add_timer(&tp->timer);
	} else if (dev->if_port == 3  ||  dev->if_port == 5) {
		if (tulip_debug > 1)
			printk(KERN_INFO"%s: 21143 %s link beat %s.\n",
				   dev->name, medianame[dev->if_port],
				   (csr12 & 2) ? "failed" : "good");
		if ((csr12 & 2)  &&  ! tp->medialock) {
			del_timer_sync(&tp->timer);
			t21142_start_nway(dev);
			tp->timer.expires = RUN_AT(3*HZ);
			add_timer(&tp->timer);
		}
	} else if (dev->if_port == 0  ||  dev->if_port == 4) {
		if ((csr12 & 4) == 0)
			printk(KERN_INFO"%s: 21143 10baseT link beat good.\n",
				   dev->name);
	} else if (!(csr12 & 4)) {		/* 10mbps link beat good. */
		if (tulip_debug)
			printk(KERN_INFO"%s: 21143 10mbps sensed media.\n",
				   dev->name);
		dev->if_port = 0;
	} else if (tp->nwayset) {
		if (tulip_debug)
			printk(KERN_INFO"%s: 21143 using NWay-set %s, csr6 %8.8x.\n",
				   dev->name, medianame[dev->if_port], tp->csr6);
	} else {		/* 100mbps link beat good. */
		if (tulip_debug)
			printk(KERN_INFO"%s: 21143 100baseTx sensed media.\n",
				   dev->name);
		dev->if_port = 3;
		tp->csr6 = 0x83860000;
		outl(0x0003FF7F, ioaddr + CSR14);
		outl(0x0301, ioaddr + CSR12);
		tulip_restart_rxtx(tp, tp->csr6);
	}
}


