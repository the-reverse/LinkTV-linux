/******************************************************************************
* rtl8192ce_xmit.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :									*
*                                                                                                                                       *
*										*
*										*
* Copyright 2010, Realtek Corp.							*
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _RTL8192C_XMIT_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_byteorder.h>
#include <wifi.h>
#include <osdep_intf.h>
#include <circ_buf.h>
#include <pci_ops.h>
#include <rtl8192c_hal.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)
#error "Shall be Linux or Windows, but not both!\n"
#endif


s32	rtl8192ce_init_xmit_priv(_adapter *padapter)
{
	s32	ret = _SUCCESS;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	_spinlock_init(&pxmitpriv->irq_th_lock);
	_spinlock_init(&pHalData->rf_lock);

#ifdef PLATFORM_LINUX
	tasklet_init(&pxmitpriv->xmit_tasklet,
	     (void(*)(unsigned long))rtl8192ce_xmit_tasklet,
	     (unsigned long)padapter);
#endif

	return ret;
}

void	rtl8192ce_free_xmit_priv(_adapter *padapter)
{
	u8	i;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	_spinlock_free(&pxmitpriv->irq_th_lock);
	_spinlock_free(&pHalData->rf_lock);

}

s32 rtl8192ce_enqueue_xmitbuf(struct rtw_tx_ring	*ring, struct xmit_buf *pxmitbuf)
{
	_irqL irqL;
	_queue *ppending_queue = &ring->queue;
	
_func_enter_;	

	//printk("+enqueue_xmitbuf\n");

	if(pxmitbuf==NULL)
	{		
		return _FAIL;
	}
	
	_enter_critical(&ppending_queue->lock, &irqL);
	
	list_delete(&pxmitbuf->list);	
	
	list_insert_tail(&(pxmitbuf->list), get_list_head(ppending_queue));

	ring->qlen++;

	//printk("FREE, free_xmitbuf_cnt=%d\n", pxmitpriv->free_xmitbuf_cnt);
		
	_exit_critical(&ppending_queue->lock, &irqL);	

_func_exit_;	 

	return _SUCCESS;	
} 

struct xmit_buf * rtl8192ce_dequeue_xmitbuf(struct rtw_tx_ring	*ring)
{
	_irqL irqL;
	_list *plist, *phead;
	struct xmit_buf *pxmitbuf =  NULL;
	_queue *ppending_queue = &ring->queue;

_func_enter_;

	_enter_critical(&ppending_queue->lock, &irqL);

	if(_queue_empty(ppending_queue) == _TRUE) {
		pxmitbuf = NULL;
	} else {

		phead = get_list_head(ppending_queue);

		plist = get_next(phead);

		pxmitbuf = LIST_CONTAINOR(plist, struct xmit_buf, list);

		list_delete(&(pxmitbuf->list));
	}

	ring->qlen--;
		
	_exit_critical(&ppending_queue->lock, &irqL);	

_func_exit_;	 

	return pxmitbuf;	
} 

static u32 get_ff_hwaddr(struct xmit_frame *pxmitframe)
{
	u32 addr;
	struct pkt_attrib *pattrib = &pxmitframe->attrib;	
	
	switch(pattrib->qsel)
	{
		case 0:
		case 3:
			addr = BE_QUEUE_INX;
		 	break;
		case 1:
		case 2:
			addr = BK_QUEUE_INX;
			break;				
		case 4:
		case 5:
			addr = VI_QUEUE_INX;
			break;		
		case 6:
		case 7:
			addr = VO_QUEUE_INX;
			break;
		case 0x10:
			addr = BCN_QUEUE_INX;
			break;
		case 0x11://BC/MC in PS (HIQ)
			addr = HIGH_QUEUE_INX;
			break;
		case 0x12:
			addr = MGT_QUEUE_INX;
			break;
		default:
			addr = BE_QUEUE_INX;
			break;		
			
	}

	return addr;

}

static u16 ffaddr2dma(u32 addr)
{
	u16	dma_ctrl;
	switch(addr)
	{
		case VO_QUEUE_INX:
			dma_ctrl = BIT3;
			break;
		case VI_QUEUE_INX:
			dma_ctrl = BIT2;
			break;
		case BE_QUEUE_INX:
			dma_ctrl = BIT1;
			break;
		case BK_QUEUE_INX:
			dma_ctrl = BIT0;
			break;
		case BCN_QUEUE_INX:
			dma_ctrl = BIT4;
			break;
		case MGT_QUEUE_INX:
			dma_ctrl = BIT6;
			break;
		case HIGH_QUEUE_INX:
			dma_ctrl = BIT7;
			break;
		default:
			dma_ctrl = 0;
			break;
	}

	return dma_ctrl;
}

void count_tx_stats(_adapter *padapter, struct xmit_frame *pxmitframe, int sz)
{
	struct sta_info *psta = NULL;
	struct stainfo_stats *pstats = NULL;
	struct pkt_attrib	*pattrib = &pxmitframe->attrib;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);


	if((pxmitframe->frame_tag&0x0f) == DATA_FRAMETAG)
	{
		pxmitpriv->tx_bytes += sz;
		pmlmepriv->LinkDetectInfo.NumTxOkInPeriod++;

		psta = pxmitframe->attrib.psta;

		if(psta)
		{
			pstats = &psta->sta_stats;

			pstats->tx_pkts++;

			pstats->tx_bytes += sz;
		}
	}
	
}

void do_queue_select(_adapter	*padapter, struct pkt_attrib *pattrib)
{
	u8	qsel;
		
	qsel = pattrib->priority;	
	RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("### do_queue_select priority=%d ,qsel = %d\n",pattrib->priority ,qsel));
	pattrib->qsel = qsel;
}


static void fill_txdesc_for_mp(struct xmit_frame *pxmitframe, struct tx_desc *ptxdesc)
{		
#ifdef CONFIG_MP_INCLUDED

		struct pkt_attrib	*pattrib = &pxmitframe->attrib;
		sint bmcst = IS_MCAST(pattrib->ra);

		if (pattrib->pctrl == 1) // mp tx packets
		{

			struct tx_desc txdesc, *ptxdesc_mp;
			struct pkt_file pktfile;

			ptxdesc_mp = &txdesc;
			_open_pktfile(pxmitframe->pkt, &pktfile);
			_pktfile_read(&pktfile, NULL, ETH_HLEN);
			_pktfile_read(&pktfile, (u8*)ptxdesc_mp, TXDESC_SIZE);

			//offset 8
			ptxdesc->txdw2 = cpu_to_le32(ptxdesc_mp->txdw2);
			if (bmcst) ptxdesc->txdw2 |= cpu_to_le32(BMC);
			ptxdesc->txdw2 |= cpu_to_le32(BK);	
			//RT_TRACE(_module_rtl871x_xmit_c_,_drv_alert_,("mp pkt offset8-txdesc=0x%8x\n", ptxdesc->txdw2));			

			ptxdesc->txdw4 = cpu_to_le32(ptxdesc_mp->txdw4);
			//RT_TRACE(_module_rtl871x_xmit_c_,_drv_alert_,("mp pkt offset16-txdesc=0x%8x\n", ptxdesc->txdw4));

			//offset 20
			ptxdesc->txdw5 = cpu_to_le32(ptxdesc_mp->txdw5);
			//RT_TRACE(_module_rtl871x_xmit_c_,_drv_alert_,("mp pkt offset20-txdesc=0x%8x\n", ptxdesc->txdw5));				

			pattrib->pctrl = 0;//reset to zero;				
		}
		
#endif
}

static void fill_txdesc_sectype(struct pkt_attrib *pattrib, struct tx_desc *ptxdesc)
{
	if ((pattrib->encrypt > 0) && !pattrib->bswenc)
	{
		switch (pattrib->encrypt)
		{	
			//SEC_TYPE
			case _WEP40_:
			case _WEP104_:
					ptxdesc->txdw1 |= cpu_to_le32((0x01<<22)&0x00c00000);
					break;				
			case _TKIP_:
			case _TKIP_WTMIC_:	
					//ptxdesc->txdw1 |= cpu_to_le32((0x02<<22)&0x00c00000);
					ptxdesc->txdw1 |= cpu_to_le32((0x01<<22)&0x00c00000);
					break;
			case _AES_:
					ptxdesc->txdw1 |= cpu_to_le32((0x03<<22)&0x00c00000);
					break;
			case _NO_PRIVACY_:
			default:
					break;
		
		}
		
	}

}

static void fill_txdesc_vcs(struct pkt_attrib *pattrib, u32 *pdw)
{
	//DBG_8192C("cvs_mode=%d\n", pattrib->vcs_mode);	

	switch(pattrib->vcs_mode)
	{
		case RTS_CTS:
			*pdw |= cpu_to_le32(BIT(12));
			break;
		case CTS_TO_SELF:
			*pdw |= cpu_to_le32(BIT(11));
			break;
		case NONE_VCS:
		default:
			break;		
	}

	if(pattrib->vcs_mode)
		*pdw |= cpu_to_le32(BIT(13));
}

static void fill_txdesc_phy(struct pkt_attrib *pattrib, u32 *pdw)
{
	//DBG_8192C("bwmode=%d, ch_off=%d\n", pattrib->bwmode, pattrib->ch_offset);

	if(pattrib->ht_en)
	{
		*pdw |= (pattrib->bwmode&HT_CHANNEL_WIDTH_40)?	cpu_to_le32(BIT(25)):0;

		if(pattrib->ch_offset == HAL_PRIME_CHNL_OFFSET_LOWER)
			*pdw |= cpu_to_le32((0x01<<20)&0x003f0000);
		else if(pattrib->ch_offset == HAL_PRIME_CHNL_OFFSET_UPPER)
			*pdw |= cpu_to_le32((0x02<<20)&0x003f0000);
		else if(pattrib->ch_offset == HAL_PRIME_CHNL_OFFSET_DONT_CARE)
			*pdw |= 0;
		else
			*pdw |= cpu_to_le32((0x03<<20)&0x003f0000);
	}
}

static s32 update_txdesc(struct xmit_frame *pxmitframe, u32 *pmem, s32 sz)
{
	uint	qsel;
	_adapter				*padapter = pxmitframe->padapter;
	struct mlme_priv		*pmlmepriv = &padapter->mlmepriv;
	struct dvobj_priv		*pdvobjpriv = &padapter->dvobjpriv;
	struct pkt_attrib		*pattrib = &pxmitframe->attrib;
	struct ht_priv			*phtpriv = &pmlmepriv->htpriv;
	struct mlme_ext_info	*pmlmeinfo = &padapter->mlmeextpriv.mlmext_info;
	struct tx_desc		*ptxdesc = (struct tx_desc *)pmem;
	sint	bmcst = IS_MCAST(pattrib->ra);

	dma_addr_t	mapping = pci_map_single(pdvobjpriv->ppcidev, pxmitframe->buf_addr , sz, PCI_DMA_TODEVICE);

	_memset(ptxdesc, 0, TX_DESC_NEXT_DESC_OFFSET);

	if(bmcst)	
	{
		ptxdesc->txdw0 |= cpu_to_le32(BIT(24));
	}	

	RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("offset0-txdesc=0x%x\n", ptxdesc->txdw0));

	if((pxmitframe->frame_tag&0x0f) == DATA_FRAMETAG)
	{
		//printk("pxmitframe->frame_tag == DATA_FRAMETAG\n");			

		//offset 4
		//ptxdesc->txdw1 |= cpu_to_le32((pattrib->mac_id-4)&0x1f);//CAM_ID(MAC_ID)
		ptxdesc->txdw1 |= cpu_to_le32((pattrib->mac_id)&0x1f);//CAM_ID(MAC_ID)

		qsel = (uint)(pattrib->qsel & 0x0000001f);
		ptxdesc->txdw1 |= cpu_to_le32((qsel << QSEL_SHT) & 0x00001f00);

		ptxdesc->txdw1 |= cpu_to_le32((pattrib->raid<< 16) & 0x000f0000);

		fill_txdesc_sectype(pattrib, ptxdesc);

		if(pattrib->ampdu_en==_TRUE)
			ptxdesc->txdw1 |= cpu_to_le32(BIT(5));//AGG EN
		else
			ptxdesc->txdw1 |= cpu_to_le32(BIT(6));//AGG BK
		
		//offset 8
		

		//offset 12
		ptxdesc->txdw3 |= cpu_to_le32((pattrib->seqnum<<16)&0xffff0000);


		//offset 16 , offset 20
		if (pattrib->qos_en)
			ptxdesc->txdw4 |= cpu_to_le32(BIT(6));//QoS

		if ((pattrib->ether_type != 0x888e) && (pattrib->ether_type != 0x0806) && (pattrib->dhcp_pkt != 1))
		{
              	//Non EAP & ARP & DHCP type data packet
              	
			fill_txdesc_vcs(pattrib, &ptxdesc->txdw4);
			fill_txdesc_phy(pattrib, &ptxdesc->txdw4);

			ptxdesc->txdw4 |= cpu_to_le32(0x00000008);//RTS Rate=24M
			ptxdesc->txdw5 |= cpu_to_le32(0x0001ff00);//
			ptxdesc->txdw5 |= cpu_to_le32(0x0000000b);//DataRate - 54M
				
              	if(0)//for driver dbg
			{
				ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate
				
				if(pattrib->ht_en)
					ptxdesc->txdw5 |= cpu_to_le32(BIT(6));//SGI

				ptxdesc->txdw5 |= cpu_to_le32(0x00000013);//init rate - mcs7
			}
		}
		else
		{
			// EAP data packet and ARP packet.
			// Use the 1M data rate to send the EAP/ARP packet.
			// This will maybe make the handshake smooth.

			ptxdesc->txdw1 |= cpu_to_le32(BIT(6));//AGG BK
			
		   	ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate

		}
		
		//offset 24

#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_TX
		if ( pattrib->hw_tcp_csum == 1 ) {
			// ptxdesc->txdw6 = 0; // clear TCP_CHECKSUM and IP_CHECKSUM. It's zero already!!
			u8 ip_hdr_offset = 32 + pattrib->hdrlen + pattrib->iv_len + 8;
			ptxdesc->txdw7 = (1 << 31) | (ip_hdr_offset << 16);
			printk("ptxdesc->txdw7 = %08x\n", ptxdesc->txdw7);
		}
#endif

		fill_txdesc_for_mp(pxmitframe, ptxdesc);

	}
	else if((pxmitframe->frame_tag&0x0f)== MGNT_FRAMETAG)
	{
		//printk("pxmitframe->frame_tag == MGNT_FRAMETAG\n");	
		
		//offset 4		
		//ptxdesc->txdw1 |= cpu_to_le32((pattrib->mac_id-4)&0x1f);//CAM_ID(MAC_ID)
		ptxdesc->txdw1 |= cpu_to_le32((pattrib->mac_id)&0x1f);//CAM_ID(MAC_ID)
		
		qsel = (uint)(pattrib->qsel&0x0000001f);
		ptxdesc->txdw1 |= cpu_to_le32((qsel<<QSEL_SHT)&0x00001f00);

		ptxdesc->txdw1 |= cpu_to_le32((pattrib->raid<< 16) & 0x000f0000);
		
		//fill_txdesc_sectype(pattrib, ptxdesc);
		
		//offset 8		

		//offset 12
		ptxdesc->txdw3 |= cpu_to_le32((pattrib->seqnum<<16)&0xffff0000);
		
		//offset 16
		ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate
		
		//offset 20
		
	}
	else if((pxmitframe->frame_tag&0x0f) == TXAGG_FRAMETAG)
	{
		DBG_8192C("pxmitframe->frame_tag == TXAGG_FRAMETAG\n");
	}
	else
	{
		DBG_8192C("pxmitframe->frame_tag = %d\n", pxmitframe->frame_tag);
		
		//offset 4	
		ptxdesc->txdw1 |= cpu_to_le32((4)&0x1f);//CAM_ID(MAC_ID)
		
		ptxdesc->txdw1 |= cpu_to_le32((6<< 16) & 0x000f0000);//raid
		
		//offset 8		

		//offset 12
		ptxdesc->txdw3 |= cpu_to_le32((pattrib->seqnum<<16)&0xffff0000);
		
		//offset 16
		ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate
		
		//offset 20
	}

	//offset 28
	ptxdesc->txdw7 |= cpu_to_le32(sz&0x0000ffff);

	//offset 32
	ptxdesc->txdw8 = cpu_to_le32(mapping);

	// 2009.11.05. tynli_test. Suggested by SD4 Filen for FW LPS.
	// (1) The sequence number of each non-Qos frame / broadcast / multicast /
	// mgnt frame should be controled by Hw because Fw will also send null data
	// which we cannot control when Fw LPS enable.
	// --> default enable non-Qos data sequense number. 2010.06.23. by tynli.
	// (2) Enable HW SEQ control for beacon packet, because we use Hw beacon.
	// (3) Use HW Qos SEQ to control the seq num of Ext port non-Qos packets.
	// 2010.06.23. Added by tynli.
	if(!pattrib->qos_en)
	{		
		ptxdesc->txdw4 |= cpu_to_le32(BIT(7)); // Hw set sequence number
		ptxdesc->txdw3 |= cpu_to_le32((8 <<28)); //set bit3 to 1. Suugested by TimChen. 2009.12.29.
	}

	//offset 0
	ptxdesc->txdw0 |= cpu_to_le32(sz&0x0000ffff);
	ptxdesc->txdw0 |= cpu_to_le32(FSG | LSG);
	ptxdesc->txdw0 |= cpu_to_le32(((TXDESC_SIZE+OFFSET_SZ)<<OFFSET_SHT)&0x00ff0000);//32 bytes for TX Desc
	ptxdesc->txdw0 |= cpu_to_le32(OWN);
		
	return 0;
		
}


static struct tx_desc *get_txdesc(struct xmit_frame *pxmitframe, u8 queue_index)
{
	_adapter			*padapter = pxmitframe->padapter;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	struct rtw_tx_ring	*ring;
	struct tx_desc	*pdesc = NULL;
	int	idx;

   	ring = &pxmitpriv->tx_ring[queue_index];
    	if (queue_index != BCN_QUEUE_INX) {
		idx = (ring->idx + ring->qlen) % ring->entries;
    	} else {
		idx = 0;
    	}

    	pdesc = &ring->desc[idx];
	if((pdesc->txdw0 & OWN) && (queue_index != BCN_QUEUE_INX)) {
		DBG_8192C("No more TX desc@%d, ring->idx = %d,idx = %d \n", queue_index, ring->idx, idx);
		return NULL;
	}

	return pdesc;
}


void dump_xframe(_adapter *padapter, struct xmit_frame *pxmitframe)
{
       _irqL irqL;
	int	t, sz, w_sz, pull=0;
	//u8	*mem_addr;
	u32	ff_hwaddr;
	struct xmit_buf		*pxmitbuf = pxmitframe->pxmitbuf;
	struct pkt_attrib		*pattrib = &pxmitframe->attrib;
	struct xmit_priv		*pxmitpriv = &padapter->xmitpriv;
	struct security_priv	*psecuritypriv = &padapter->securitypriv;
	struct tx_desc		*ptxdesc;

	if ((pxmitframe->frame_tag == DATA_FRAMETAG) &&
	    (pxmitframe->attrib.ether_type != 0x0806) &&
	    (pxmitframe->attrib.ether_type != 0x888e) &&
	    (pxmitframe->attrib.dhcp_pkt != 1))
		issue_addbareq_cmd(padapter, pxmitframe);

	//mem_addr = pxmitframe->buf_addr;

       RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("dump_xframe()\n"));
	
	for (t = 0; t < pattrib->nr_frags; t++)
	{
		if (t != (pattrib->nr_frags - 1))
		{
			RT_TRACE(_module_rtl871x_xmit_c_,_drv_err_,("pattrib->nr_frags=%d\n", pattrib->nr_frags));

			sz = pxmitpriv->frag_len;
			sz = sz - 4 - (psecuritypriv->sw_encrypt ? 0 : pattrib->icv_len);					
		}
		else //no frag
		{
			sz = pattrib->last_txcmdsz;
		}

		ff_hwaddr = get_ff_hwaddr(pxmitframe);

		_enter_critical(&pxmitpriv->irq_th_lock, &irqL);

		ptxdesc = get_txdesc(pxmitframe, ff_hwaddr);
		if(ptxdesc == NULL)
		{
			_exit_critical(&pxmitpriv->irq_th_lock, &irqL);
			free_xmitbuf(pxmitpriv, pxmitbuf);
			DBG_8192C("##### Tx desc unavailable !#####\n");
			break;
		}

		update_txdesc(pxmitframe, (uint*)ptxdesc, sz);

		rtl8192ce_enqueue_xmitbuf(&pxmitpriv->tx_ring[ff_hwaddr], pxmitbuf);

		pxmitbuf->len = sz;

		_exit_critical(&pxmitpriv->irq_th_lock, &irqL);

		w_sz = sz;

		write16(padapter, REG_PCIE_CTRL_REG, ffaddr2dma(ff_hwaddr));

		write_port(padapter, ff_hwaddr, w_sz, (unsigned char*)pxmitbuf);

		count_tx_stats(padapter, pxmitframe, sz);

		RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("write_port, w_sz=%d\n", w_sz));
		//printk("write_port, w_sz=%d, sz=%d, txdesc_sz=%d, tid=%d\n", w_sz, sz, w_sz-sz, pattrib->priority);      

		//mem_addr += w_sz;

		//mem_addr = (u8 *)RND4(((uint)(mem_addr)));

	}
	
	free_xmitframe_ex(pxmitpriv, pxmitframe);
	
}

void rtl8192ce_xmitframe_resume(_adapter *padapter)
{
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	struct xmit_frame *pxmitframe = NULL;
	struct xmit_buf	*pxmitbuf = NULL;
	int res=_SUCCESS, xcnt = 0;

	RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("rtl8192ce_xmitframe_resume()\n"));

	while(1)
	{
		if ((padapter->bDriverStopped == _TRUE)||(padapter->bSurpriseRemoved== _TRUE))
		{
			DBG_8192C("rtl8192ce_xmitframe_resume => bDriverStopped or bSurpriseRemoved \n");
			break;
		}

		pxmitbuf = alloc_xmitbuf(pxmitpriv);
		if(!pxmitbuf)
		{
			break;
		}

		pxmitframe =  dequeue_xframe(pxmitpriv, pxmitpriv->hwxmits, pxmitpriv->hwxmit_entry);
		
		if(pxmitframe)
		{
			pxmitframe->pxmitbuf = pxmitbuf;				

			pxmitframe->buf_addr = pxmitbuf->pbuf;

			pxmitbuf->priv_data = pxmitframe;	

			if((pxmitframe->frame_tag&0x0f) == DATA_FRAMETAG)
			{	
				if(pxmitframe->attrib.priority<=15)//TID0~15
				{
					res = xmitframe_coalesce(padapter, pxmitframe->pkt, pxmitframe);
				}	
							
				os_xmit_complete(padapter, pxmitframe);//always return ndis_packet after xmitframe_coalesce 			
			}	

			RT_TRACE(_module_rtl871x_xmit_c_,_drv_info_,("rtl8192ce_xmitframe_resume(): dump_xframe\n"));

			if(res == _SUCCESS)
			{
				dump_xframe(padapter, pxmitframe);
			}
			else
			{
				free_xmitbuf(pxmitpriv, pxmitbuf);
				free_xmitframe_ex(pxmitpriv, pxmitframe);
			}

			xcnt++;
		}
		else
		{			
			free_xmitbuf(pxmitpriv, pxmitbuf);
			break;
		}
	}
}

static u8 check_nic_enough_desc(_adapter *padapter, struct pkt_attrib *pattrib)
{
	u32 prio;
	struct xmit_priv	*pxmitpriv = &padapter->xmitpriv;
	struct rtw_tx_ring	*ring;

	switch(pattrib->qsel)
	{
		case 0:
		case 3:
			prio = BE_QUEUE_INX;
		 	break;
		case 1:
		case 2:
			prio = BK_QUEUE_INX;
			break;				
		case 4:
		case 5:
			prio = VI_QUEUE_INX;
			break;		
		case 6:
		case 7:
			prio = VO_QUEUE_INX;
			break;
		default:
			prio = BE_QUEUE_INX;
			break;
	}

	ring = &pxmitpriv->tx_ring[prio];

	// for now we reserve two free descriptor as a safety boundary 
	// between the tail and the head 
	//
	if ((ring->entries - ring->qlen) >= 2) {
		return _TRUE;
	} else {
		DBG_8192C("do not have enough desc for Tx \n");
		return _FALSE;
	}
}

static s32 xmitframe_direct(_adapter *padapter, struct xmit_frame *pxmitframe)
{
	s32 res = _SUCCESS;

	res = xmitframe_coalesce(padapter, pxmitframe->pkt, pxmitframe);
	if (res == _SUCCESS) {
		dump_xframe(padapter, pxmitframe);
	}

	return res;
}

/*
 * Return
 *	_TRUE	dump packet directly
 *	_FALSE	enqueue packet
 */
static s32 pre_xmitframe(_adapter *padapter, struct xmit_frame *pxmitframe)
{
        _irqL irqL;
	s32 res;
	struct xmit_buf *pxmitbuf = NULL;
	struct xmit_priv *pxmitpriv = &padapter->xmitpriv;
	struct pkt_attrib *pattrib = &pxmitframe->attrib;
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;


	do_queue_select(padapter, pattrib);
	
	_enter_critical_bh(&pxmitpriv->lock, &irqL);
	
	if ( (txframes_sta_ac_pending(padapter, pattrib) > 0) ||
		(check_nic_enough_desc(padapter, pattrib) == _FALSE))
		goto enqueue;

	if (check_fwstate(pmlmepriv, _FW_UNDER_SURVEY) == _TRUE)
		goto enqueue;

#ifdef CONFIG_AP_MODE
	if(xmitframe_enqueue_for_sleeping_sta(padapter, pxmitframe) == _TRUE)
	{
		_exit_critical_bh(&pxmitpriv->lock, &irqL);
		return _FALSE;
	}
#endif

	pxmitbuf = alloc_xmitbuf(pxmitpriv);
	if (pxmitbuf == NULL)
		goto enqueue;

	_exit_critical_bh(&pxmitpriv->lock, &irqL);

	pxmitframe->pxmitbuf = pxmitbuf;
	pxmitframe->buf_addr = pxmitbuf->pbuf;
	pxmitbuf->priv_data = pxmitframe;

	if (xmitframe_direct(padapter, pxmitframe) != _SUCCESS) {
		free_xmitbuf(pxmitpriv, pxmitbuf);
		free_xmitframe_ex(pxmitpriv, pxmitframe);
	}

	return _TRUE;

enqueue:
	res = xmitframe_enqueue(padapter, pxmitframe);
	_exit_critical_bh(&pxmitpriv->lock, &irqL);

	if (res != _SUCCESS) {
		RT_TRACE(_module_xmit_osdep_c_, _drv_err_, ("pre_xmitframe: enqueue xmitframe fail\n"));
		free_xmitframe_ex(pxmitpriv, pxmitframe);

		// Trick, make the statistics correct
		pxmitpriv->tx_pkts--;
		pxmitpriv->tx_drop++;
		return _TRUE;
	}

	return _FALSE;
}

void rtl8192ce_mgnt_xmit(_adapter *padapter, struct xmit_frame *pmgntframe)
{
	dump_xframe(padapter, pmgntframe);
}

/*
 * Return
 *	_TRUE	dump packet directly ok
 *	_FALSE	temporary can't transmit packets to hardware
 */
s32 rtl8192ce_hal_xmit(_adapter *padapter, struct xmit_frame *pxmitframe)
{
	return pre_xmitframe(padapter, pxmitframe);
}

#ifdef  CONFIG_HOSTAPD_MLME

static void rtl8192ce_hostap_mgnt_xmit_cb(struct urb *urb)
{	
#ifdef PLATFORM_LINUX
	struct sk_buff *skb = (struct sk_buff *)urb->context;

	//printk("%s\n", __FUNCTION__);

	dev_kfree_skb_any(skb);
#endif	
}

s32 rtl8192ce_hostap_mgnt_xmit_entry(_adapter *padapter, _pkt *pkt)
{
#ifdef PLATFORM_LINUX
	u16 fc;
	int rc, len, pipe;	
	unsigned int bmcst, tid, qsel;
	struct sk_buff *skb, *pxmit_skb;
	struct urb *urb;
	unsigned char *pxmitbuf;
	struct tx_desc *ptxdesc;
	struct ieee80211_hdr *tx_hdr;
	struct hostapd_priv *phostapdpriv = padapter->phostapdpriv;	
	struct net_device *pnetdev = padapter->pnetdev;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	struct dvobj_priv *pdvobj = &padapter->dvobjpriv;	

	
	//printk("%s\n", __FUNCTION__);

	skb = pkt;
	
	len = skb->len;
	tx_hdr = (struct ieee80211_hdr *)(skb->data);
	fc = le16_to_cpu(tx_hdr->frame_ctl);
	bmcst = IS_MCAST(tx_hdr->addr1);

	if ((fc & IEEE80211_FCTL_FTYPE) != IEEE80211_FTYPE_MGMT)
		goto _exit;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)) // http://www.mail-archive.com/netdev@vger.kernel.org/msg17214.html
	pxmit_skb = dev_alloc_skb(len + TXDESC_SIZE);			
#else			
	pxmit_skb = netdev_alloc_skb(pnetdev, len + TXDESC_SIZE);
#endif		

	if(!pxmit_skb)
		goto _exit;

	pxmitbuf = pxmit_skb->data;

	urb = usb_alloc_urb(0, GFP_ATOMIC);
	if (!urb) {
		goto _exit;
	}

	// ----- fill tx desc -----	
	ptxdesc = (struct tx_desc *)pxmitbuf;	
	_memset(ptxdesc, 0, sizeof(*ptxdesc));
		
	//offset 0	
	ptxdesc->txdw0 |= cpu_to_le32(len&0x0000ffff); 
	ptxdesc->txdw0 |= cpu_to_le32(((TXDESC_SIZE+OFFSET_SZ)<<OFFSET_SHT)&0x00ff0000);//default = 32 bytes for TX Desc
	ptxdesc->txdw0 |= cpu_to_le32(OWN | FSG | LSG);

	if(bmcst)	
	{
		ptxdesc->txdw0 |= cpu_to_le32(BIT(24));
	}	

	//offset 4	
	ptxdesc->txdw1 |= cpu_to_le32(0x00);//MAC_ID

	ptxdesc->txdw1 |= cpu_to_le32((0x12<<QSEL_SHT)&0x00001f00);

	ptxdesc->txdw1 |= cpu_to_le32((0x06<< 16) & 0x000f0000);//b mode

	//offset 8			

	//offset 12		
	ptxdesc->txdw3 |= cpu_to_le32((le16_to_cpu(tx_hdr->seq_ctl)<<16)&0xffff0000);

	//offset 16		
	ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate
		
	//offset 20

	rtl8192cu_cal_txdesc_chksum(ptxdesc);
	// ----- end of fill tx desc -----

	//
	skb_put(pxmit_skb, len + TXDESC_SIZE);
	pxmitbuf = pxmitbuf + TXDESC_SIZE;
	_memcpy(pxmitbuf, skb->data, len);

	//printk("mgnt_xmit, len=%x\n", pxmit_skb->len);


	// ----- prepare urb for submit -----
	
	//translate DMA FIFO addr to pipehandle
	//pipe = ffaddr2pipehdl(pdvobj, MGT_QUEUE_INX);
	pipe = usb_sndbulkpipe(pdvobj->pusbdev, pHalData->Queue2EPNum[(u8)MGT_QUEUE_INX]&0x0f);
	
	usb_fill_bulk_urb(urb, pdvobj->pusbdev, pipe,
			  pxmit_skb->data, pxmit_skb->len, rtl8192cu_hostap_mgnt_xmit_cb, pxmit_skb);
	
	urb->transfer_flags |= URB_ZERO_PACKET;
	usb_anchor_urb(urb, &phostapdpriv->anchored);
	rc = usb_submit_urb(urb, GFP_ATOMIC);
	if (rc < 0) {
		usb_unanchor_urb(urb);
		kfree_skb(skb);
	}
	usb_free_urb(urb);

	
_exit:	
	
	dev_kfree_skb_any(skb);

#endif

	return 0;

}
#endif

