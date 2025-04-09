/******************************************************************************
* rtl8192cu_recv.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :
*
*
*                                                                                                                                       *
* Copyright 2008, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _RTL8192CU_RECV_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <mlme_osdep.h>
#include <ip.h>
#include <if_ether.h>
#include <ethernet.h>

#ifdef CONFIG_USB_HCI
#include <usb_ops.h>
#endif

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#include <wifi.h>
#include <circ_buf.h>

#include <rtl8192c_hal.h>


void rtl8192cu_init_recv_priv(_adapter * padapter)
{
	struct recv_priv	*precvpriv = &padapter->recvpriv;

_func_enter_;

	precvpriv->max_recvbuf_sz = MAX_RECVBUF_SZ;
	precvpriv->nr_recvbuf = NR_RECVBUFF;

#ifdef PLATFORM_LINUX
	tasklet_init(&precvpriv->recv_tasklet,
	     (void(*)(unsigned long))rtl8192cu_recv_tasklet,
	     (unsigned long)padapter);
#endif

_func_exit_;
}

void rtl8192cu_update_recvframe_attrib_from_recvstat(union recv_frame *precvframe, struct recv_stat *prxstat)
{
	u8 physt, qos, shift, icverr, htc;
	u32 *pphy_info;
	u16 drvinfo_sz=0;
	struct rx_pkt_attrib *pattrib = &precvframe->u.hdr.attrib;


	//Offset 0
	drvinfo_sz = (le32_to_cpu(prxstat->rxdw0)&0x000f0000)>>16;
	drvinfo_sz = drvinfo_sz<<3;

	pattrib->bdecrypted = ((le32_to_cpu(prxstat->rxdw0) & BIT(27)) >> 27)? 0:1;

	physt = ((le32_to_cpu(prxstat->rxdw0) & BIT(26)) >> 26)? 1:0;

	shift = (le32_to_cpu(prxstat->rxdw0)&0x03000000)>>24;

	qos = ((le32_to_cpu(prxstat->rxdw0) & BIT(23)) >> 23)? 1:0;

	icverr = ((le32_to_cpu(prxstat->rxdw0) & BIT(15)) >> 15)? 1:0;

	pattrib->crc_err = ((le32_to_cpu(prxstat->rxdw0) & BIT(14)) >> 14 );


	//Offset 4

	//Offset 8

	//Offset 12
#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_RX
	if ( le32_to_cpu(prxstat->rxdw3) & BIT(13)) {
		pattrib->tcpchk_valid = 1; // valid
		if ( le32_to_cpu(prxstat->rxdw3) & BIT(11) ) {
			pattrib->tcp_chkrpt = 1; // correct
			//printk("tcp csum ok\n");
		} else
			pattrib->tcp_chkrpt = 0; // incorrect

		if ( le32_to_cpu(prxstat->rxdw3) & BIT(12) )
			pattrib->ip_chkrpt = 1; // correct
		else
			pattrib->ip_chkrpt = 0; // incorrect

	} else {
		pattrib->tcpchk_valid = 0; // invalid
	}

#endif

	pattrib->mcs_rate=(u8)((le32_to_cpu(prxstat->rxdw3))&0x3f);
	pattrib->rxht=(u8)((le32_to_cpu(prxstat->rxdw3) >>6)&0x1);

	htc = (u8)((le32_to_cpu(prxstat->rxdw3) >>10)&0x1);

	//Offset 16
	//Offset 20


#if 0 //dump rxdesc for debug
	printk("drvinfo_sz=%d\n", drvinfo_sz);
	printk("physt=%d\n", physt);
	printk("shift=%d\n", shift);
	printk("qos=%d\n", qos);
	printk("icverr=%d\n", icverr);
	printk("htc=%d\n", htc);
	printk("bdecrypted=%d\n", pattrib->bdecrypted);
	printk("mcs_rate=%d\n", pattrib->mcs_rate);
	printk("rxht=%d\n", pattrib->rxht);
#endif

	//phy_info
	if(drvinfo_sz && physt)
	{

		pphy_info=(u32 *)prxstat+1;

		//printk("pphy_info, of0=0x%08x\n", *pphy_info);
		//printk("pphy_info, of1=0x%08x\n", *(pphy_info+1));
		//printk("pphy_info, of2=0x%08x\n", *(pphy_info+2));
		//printk("pphy_info, of3=0x%08x\n", *(pphy_info+3));
		//printk("pphy_info, of4=0x%08x\n", *(pphy_info+4));
		//printk("pphy_info, of5=0x%08x\n", *(pphy_info+5));
		//printk("pphy_info, of6=0x%08x\n", *(pphy_info+6));
		//printk("pphy_info, of7=0x%08x\n", *(pphy_info+7));

		rtl8192c_query_rx_phy_status(precvframe, prxstat);

#if 0 //dump phy_status for debug

		printk("signal_qual=%d\n", pattrib->signal_qual);
		printk("signal_strength=%d\n", pattrib->signal_strength);
#endif

	}


}

