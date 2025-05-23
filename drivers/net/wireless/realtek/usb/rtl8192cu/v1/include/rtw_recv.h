#ifndef _RTW_RECV_H_
#define _RTW_RECV_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#ifdef PLATFORM_OS_XP
#define NR_RECVFRAME 256
#else
#define NR_RECVFRAME 256
#endif

#define RXFRAME_ALIGN	8
#define RXFRAME_ALIGN_SZ	(1<<RXFRAME_ALIGN)

#define MAX_RXFRAME_CNT	512
#define MAX_RX_NUMBLKS		(32)
#define RECVFRAME_HDR_ALIGN 128



#define SNAP_SIZE sizeof(struct ieee80211_snap_hdr)

static u8 SNAP_ETH_TYPE_IPX[2] = {0x81, 0x37};

static u8 SNAP_ETH_TYPE_APPLETALK_AARP[2] = {0x80, 0xf3};
static u8 SNAP_ETH_TYPE_APPLETALK_DDP[2] = {0x80, 0x9b};
static u8 SNAP_HDR_APPLETALK_DDP[3] = {0x08, 0x00, 0x07}; // Datagram Delivery Protocol

static u8 oui_8021h[] = {0x00, 0x00, 0xf8};
static u8 oui_rfc1042[]= {0x00,0x00,0x00};

#define MAX_SUBFRAME_COUNT	64
static u8 rfc1042_header[] =
{ 0xaa, 0xaa, 0x03, 0x00, 0x00, 0x00 };
/* Bridge-Tunnel header (for EtherTypes ETH_P_AARP and ETH_P_IPX) */
static u8 bridge_tunnel_header[] =
{ 0xaa, 0xaa, 0x03, 0x00, 0x00, 0xf8 };

//for Rx reordering buffer control
struct recv_reorder_ctrl
{
	_adapter	*padapter;
	u8 enable;
	u16 indicate_seq;//=wstart_b, init_value=0xffff
	u16 wend_b;
       u8 wsize_b;	
	_queue pending_recvframe_queue;
	_timer reordering_ctrl_timer;
};

struct	stainfo_rxcache	{
	u16 	tid_rxseq[16];
/*	
	unsigned short 	tid0_rxseq;
	unsigned short 	tid1_rxseq;
	unsigned short 	tid2_rxseq;
	unsigned short 	tid3_rxseq;
	unsigned short 	tid4_rxseq;
	unsigned short 	tid5_rxseq;
	unsigned short 	tid6_rxseq;
	unsigned short 	tid7_rxseq;
	unsigned short 	tid8_rxseq;
	unsigned short 	tid9_rxseq;
	unsigned short 	tid10_rxseq;
	unsigned short 	tid11_rxseq;
	unsigned short 	tid12_rxseq;
	unsigned short 	tid13_rxseq;
	unsigned short 	tid14_rxseq;
	unsigned short 	tid15_rxseq;
*/
};

#define		PHY_RSSI_SLID_WIN_MAX				100
#define		PHY_LINKQUALITY_SLID_WIN_MAX		20


struct smooth_rssi_data {
	u32	elements[100];	//array to store values
	u32	index;			//index to current array to store
	u32	total_num;		//num of valid elements
	u32	total_val;		//sum of valid elements
};

struct rx_pkt_attrib	{

	u8 	amsdu;
	u8	order;
	u8	qos;
	u8 	to_fr_ds;
	u8	frag_num;
	u16	seq_num;
	u8   pw_save;
	u8    mfrag;
	u8    mdata;	
	u8	privacy; //in frame_ctrl field
	u8	bdecrypted;
	int	hdrlen;		//the WLAN Header Len
	int	encrypt;		//when 0 indicate no encrypt. when non-zero, indicate the encrypt algorith
	int	iv_len;
	int	icv_len;
	int	priority;
	int	ack_policy;
 	u8	crc_err;

	u8 	dst[ETH_ALEN];
	u8 	src[ETH_ALEN];
	u8 	ta[ETH_ALEN];
	u8 	ra[ETH_ALEN];
	u8 	bssid[ETH_ALEN];
#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_RX
	u8	tcpchk_valid; // 0: invalid, 1: valid
	u8	ip_chkrpt; //0: incorrect, 1: correct
	u8	tcp_chkrpt; //0: incorrect, 1: correct
#endif

	u8	mcs_rate;
	u8	rxht;
	u8	signal_qual;
	s8	rx_mimo_signal_qual[2];	
	u8	signal_strength;

	u32 RxPWDBAll;
	s32	RecvSignalPower;
	
};






/*
accesser of recv_priv: recv_entry(dispatch / passive level); recv_thread(passive) ; returnpkt(dispatch)
; halt(passive) ;

using enter_critical section to protect
*/
struct recv_priv {

  	  _lock	lock;

#ifdef CONFIG_RECV_THREAD_MODE	
	_sema	recv_sema;
	_sema	terminate_recvthread_sema;
#endif
	
	//_queue	blk_strms[MAX_RX_NUMBLKS];    // keeping the block ack frame until return ack
	_queue	free_recv_queue;
	_queue	recv_pending_queue;
	

	u8 *pallocated_frame_buf;
	u8 *precv_frame_buf; 
	
	uint free_recvframe_cnt;
	
	_adapter	*adapter;
	
#ifdef PLATFORM_WINDOWS
	_nic_hdl  RxPktPoolHdl;
	_nic_hdl  RxBufPoolHdl;

#ifdef PLATFORM_OS_XP
	PMDL	pbytecnt_mdl;
#endif
	uint	counter; //record the number that up-layer will return to drv; only when counter==0 can we  release recv_priv 
	NDIS_EVENT 	recv_resource_evt ;
#endif	

	u32	bIsAnyNonBEPkts;
	u32	NumRxUnicastOkInPeriod;
	u64	rx_bytes;
	u64	rx_pkts;
	u64	rx_drop;
	u64	last_rx_bytes;

	uint  rx_icv_err;
	uint  rx_largepacket_crcerr;
	uint  rx_smallpacket_crcerr;
	uint  rx_middlepacket_crcerr;

#ifdef CONFIG_USB_HCI	
	//u8 *pallocated_urb_buf;	
	_sema allrxreturnevt;
	u8  rx_pending_cnt;
	uint	ff_hwaddr;
#endif	
#ifdef PLATFORM_LINUX
	struct tasklet_struct recv_tasklet;
	struct sk_buff_head free_recv_skb_queue;
	struct sk_buff_head rx_skb_queue;
#endif

  
	u8 *pallocated_recv_buf;
	u8 *precv_buf;    // 4 alignment	
	_queue	free_recv_buf_queue;
	u32	free_recv_buf_queue_cnt;
	u32	max_recvbuf_sz;
	u32	nr_recvbuf;

#ifdef CONFIG_SDIO_HCI
        u8 bytecnt_buf[512];
//	u8 * recvbuf_drop_ori;
	//u8 * recvbuf_drop;
	struct recv_buf *recvbuf_drop;
#endif

	//For display the phy informatiom
	s8 rssi;
	u8 signal_strength;
	u8 signal_qual;
	u8 noise;
	struct smooth_rssi_data signal_qual_data;
	struct smooth_rssi_data signal_strength_data;
	
};


struct sta_recv_priv {
    
    _lock	lock;
	sint	option;	
	
	//_queue	blk_strms[MAX_RX_NUMBLKS];
	_queue defrag_q;	 //keeping the fragment frame until defrag
	
	struct	stainfo_rxcache rxcache;  
	
	//uint	sta_rx_bytes;
	//uint	sta_rx_pkts;
	//uint	sta_rx_fail;

};

//These definition is used for Rx packet reordering.
#define SN_LESS(a, b)		(((a-b)&0x800)!=0)
#define SN_EQUAL(a, b)	(a == b)
//#define REORDER_WIN_SIZE	128
//#define REORDER_ENTRY_NUM	128
#define REORDER_WAIT_TIME	(30) // (ms)

#define RECVBUFF_ALIGN_SZ 512

#define RXDESC_SIZE	24
#define RXDESC_OFFSET RXDESC_SIZE

struct recv_stat
{
	unsigned int rxdw0;

	unsigned int rxdw1;

	unsigned int rxdw2;

	unsigned int rxdw3;

	unsigned int rxdw4;

	unsigned int rxdw5;
};

struct phy_cck_rx_status
{
	/* For CCK rate descriptor. This is a unsigned 8:1 variable. LSB bit presend
	   0.5. And MSB 7 bts presend a signed value. Range from -64~+63.5. */
	u8	adc_pwdb_X[4];
	u8	sq_rpt;
	u8	cck_agc_rpt;
};

struct phy_stat
{
	unsigned int phydw0;

	unsigned int phydw1;

	unsigned int phydw2;

	unsigned int phydw3;

	unsigned int phydw4;

	unsigned int phydw5;

	unsigned int phydw6;

	unsigned int phydw7;
};

// Rx smooth factor
#define	Rx_Smooth_Factor (20)

union recvstat {
	struct recv_stat recv_stat;
	unsigned int value[RXDESC_SIZE>>2];
};


struct recv_buf{

	_list list;

	_lock recvbuf_lock;

	u32	ref_cnt;

	_adapter  *adapter;

#ifdef CONFIG_SDIO_HCI
#ifdef PLATFORM_OS_XP
	PMDL mdl_ptr;
#endif
	u8	cmd_fail;
#endif

#ifdef CONFIG_USB_HCI

	#if defined(PLATFORM_OS_XP)||defined(PLATFORM_LINUX)
	PURB	purb;
	#endif

	#ifdef PLATFORM_OS_XP
	PIRP		pirp;
	#endif

	#ifdef PLATFORM_OS_CE
	USB_TRANSFER	usb_transfer_read_port;
	#endif

	u8  irp_pending;
	int  transfer_len;

#endif

#ifdef PLATFORM_LINUX
	_pkt *pskb;
	u8	reuse;
#endif

	uint	len;
	u8	*phead;
	u8	*pdata;
	u8	*ptail;
	u8	*pend;

	u8	*pbuf;
	u8	*pallocated_buf;

};


/*
	head  ----->

		data  ----->

			payload

		tail  ----->


	end   ----->

	len = (unsigned int )(tail - data);

*/
struct recv_frame_hdr{

	_list	list;
	_pkt	*pkt;
	_pkt *pkt_newalloc;

	_adapter  *adapter;
	
	u8 fragcnt;

	int frame_tag;

	struct rx_pkt_attrib attrib;

	uint  len;
	u8 *rx_head;
	u8 *rx_data;
	u8 *rx_tail;
	u8 *rx_end;

	void *precvbuf;


	//
	struct sta_info *psta;

	//for A-MPDU Rx reordering buffer control
	struct recv_reorder_ctrl *preorder_ctrl;

};


union recv_frame{

	union{
		_list list;
		struct recv_frame_hdr hdr;
		uint mem[RECVFRAME_HDR_ALIGN>>2];
	}u;

	//uint mem[MAX_RXSZ>>2];

};


int init_recvbuf(_adapter *padapter, struct recv_buf *precvbuf);
extern union recv_frame *alloc_recvframe (_queue *pfree_recv_queue);  //get a free recv_frame from pfree_recv_queue
extern void init_recvframe(union recv_frame *precvframe ,struct recv_priv *precvpriv);
extern int	 free_recvframe(union recv_frame *precvframe, _queue *pfree_recv_queue);  
extern union recv_frame *dequeue_recvframe (_queue *queue);
extern int	enqueue_recvframe(union recv_frame *precvframe, _queue *queue);
extern void free_recvframe_queue(_queue *pframequeue,  _queue *pfree_recv_queue);  

void reordering_ctrl_timeout_handler(void *pcontext);

__inline static u8 *get_rxmem(union recv_frame *precvframe)
{
	//always return rx_head...
	if(precvframe==NULL)
		return NULL;

	return precvframe->u.hdr.rx_head;
}

__inline static u8 *get_rx_status(union recv_frame *precvframe)
{
	
	return get_rxmem(precvframe);
	
}

__inline static u8 *get_recvframe_data(union recv_frame *precvframe)
{
	
	//alwasy return rx_data	
	if(precvframe==NULL)
		return NULL;

	return precvframe->u.hdr.rx_data;
	
}

__inline static u8 *recvframe_push(union recv_frame *precvframe, sint sz)
{	
	// append data before rx_data 

	/* add data to the start of recv_frame
 *
 *      This function extends the used data area of the recv_frame at the buffer
 *      start. rx_data must be still larger than rx_head, after pushing.
 */
 
	if(precvframe==NULL)
		return NULL;


	precvframe->u.hdr.rx_data -= sz ;
	if( precvframe->u.hdr.rx_data < precvframe->u.hdr.rx_head )
	{
		precvframe->u.hdr.rx_data += sz ;
		return NULL;
	}

	precvframe->u.hdr.len +=sz;

	return precvframe->u.hdr.rx_data;
	
}


__inline static u8 *recvframe_pull(union recv_frame *precvframe, sint sz)
{
	// rx_data += sz; move rx_data sz bytes  hereafter

	//used for extract sz bytes from rx_data, update rx_data and return the updated rx_data to the caller


	if(precvframe==NULL)
		return NULL;

	
	precvframe->u.hdr.rx_data += sz;

	if(precvframe->u.hdr.rx_data > precvframe->u.hdr.rx_tail)
	{
		precvframe->u.hdr.rx_data -= sz;
		return NULL;
	}

	precvframe->u.hdr.len -=sz;
	
	return precvframe->u.hdr.rx_data;
	
}

__inline static u8 *recvframe_put(union recv_frame *precvframe, sint sz)
{
	// rx_tai += sz; move rx_tail sz bytes  hereafter

	//used for append sz bytes from ptr to rx_tail, update rx_tail and return the updated rx_tail to the caller
	//after putting, rx_tail must be still larger than rx_end. 
 	unsigned char * prev_rx_tail;

	if(precvframe==NULL)
		return NULL;

	prev_rx_tail = precvframe->u.hdr.rx_tail;
	
	precvframe->u.hdr.rx_tail += sz;
	
	if(precvframe->u.hdr.rx_tail > precvframe->u.hdr.rx_end)
	{
		precvframe->u.hdr.rx_tail -= sz;
		return NULL;
	}

	precvframe->u.hdr.len +=sz;

	return precvframe->u.hdr.rx_tail;

}



__inline static u8 *recvframe_pull_tail(union recv_frame *precvframe, sint sz)
{
	// rmv data from rx_tail (by yitsen)
	
	//used for extract sz bytes from rx_end, update rx_end and return the updated rx_end to the caller
	//after pulling, rx_end must be still larger than rx_data.

	if(precvframe==NULL)
		return NULL;

	precvframe->u.hdr.rx_tail -= sz;

	if(precvframe->u.hdr.rx_tail < precvframe->u.hdr.rx_data)
	{
		precvframe->u.hdr.rx_tail += sz;
		return NULL;
	}

	precvframe->u.hdr.len -=sz;

	return precvframe->u.hdr.rx_tail;

}



__inline static _buffer * get_rxbuf_desc(union recv_frame *precvframe)
{
	_buffer * buf_desc;
	
	if(precvframe==NULL)
		return NULL;
#ifdef PLATFORM_WINDOWS	
	NdisQueryPacket(precvframe->u.hdr.pkt, NULL, NULL, &buf_desc, NULL);
#endif

	return buf_desc;
}


__inline static union recv_frame *rxmem_to_recvframe(u8 *rxmem)
{
	//due to the design of 2048 bytes alignment of recv_frame, we can reference the union recv_frame 
	//from any given member of recv_frame.
	// rxmem indicates the any member/address in recv_frame
	
	return (union recv_frame*)(((uint)rxmem>>RXFRAME_ALIGN) <<RXFRAME_ALIGN) ;
	
}

__inline static union recv_frame *pkt_to_recvframe(_pkt *pkt)
{
	
	u8 * buf_star;
	union recv_frame * precv_frame;
#ifdef PLATFORM_WINDOWS
	_buffer * buf_desc;
	uint len;

	NdisQueryPacket(pkt, NULL, NULL, &buf_desc, &len);
	NdisQueryBufferSafe(buf_desc, &buf_star, &len, HighPagePriority);
#endif
	precv_frame = rxmem_to_recvframe((unsigned char*)buf_star);

	return precv_frame;
}

__inline static u8 *pkt_to_recvmem(_pkt *pkt)
{
	// return the rx_head
	
	union recv_frame * precv_frame = pkt_to_recvframe(pkt);

	return 	precv_frame->u.hdr.rx_head;

}

__inline static u8 *pkt_to_recvdata(_pkt *pkt)
{
	// return the rx_data

	union recv_frame * precv_frame =pkt_to_recvframe(pkt);

	return 	precv_frame->u.hdr.rx_data;
	
}


__inline static sint get_recvframe_len(union recv_frame *precvframe)
{
	return precvframe->u.hdr.len;
}

__inline static u8 query_rx_pwr_percentage(s8 antpower )
{
	if ((antpower <= -100) || (antpower >= 20))
	{
		return	0;
	}
	else if (antpower >= 0)
	{
		return	100;
	}
	else
	{
		return	(100+antpower);
	}
}
__inline static s32 translate_percentage_to_dbm(u32 SignalStrengthIndex)
{
	s32	SignalPower; // in dBm.

	// Translate to dBm (x=0.5y-95).
	SignalPower = (s32)((SignalStrengthIndex + 1) >> 1); 
	SignalPower -= 95; 

	return SignalPower;
}


struct sta_info;

extern void _init_sta_recv_priv(struct sta_recv_priv *psta_recvpriv);

extern void  mgt_dispatcher(_adapter *padapter, union recv_frame *precv_frame);

#endif

