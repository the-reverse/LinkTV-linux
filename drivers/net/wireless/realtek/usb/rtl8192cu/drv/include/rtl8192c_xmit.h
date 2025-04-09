#ifndef _RTL8192C_XMIT_H_
#define _RTL8192C_XMIT_H_

#define VO_QUEUE_INX		0
#define VI_QUEUE_INX		1
#define BE_QUEUE_INX		2
#define BK_QUEUE_INX		3
#define BCN_QUEUE_INX		4
#define MGT_QUEUE_INX		5
#define HIGH_QUEUE_INX		6
#define TXCMD_QUEUE_INX	7

#define HW_QUEUE_ENTRY	8

//
// Queue Select Value in TxDesc
//
#define QSLT_BK							0x2//0x01
#define QSLT_BE							0x0
#define QSLT_VI							0x5//0x4
#define QSLT_VO							0x7//0x6
#define QSLT_BEACON						0x10
#define QSLT_HIGH						0x11
#define QSLT_MGNT						0x12
#define QSLT_CMD						0x13

#ifdef CONFIG_PCI_HCI
#define TXDESC_NUM						64
//#define TXDESC_NUM						128
#define TXDESC_NUM_BE_QUEUE			256
#endif

#ifdef CONFIG_USB_HCI

#ifdef USB_TX_AGGREGATION_92C
#define MAX_TX_AGG_PACKET_NUMBER 0xFF
#endif

s32	rtl8192cu_init_xmit_priv(_adapter * padapter);

void	rtl8192cu_free_xmit_priv(_adapter * padapter);

void rtl8192cu_cal_txdesc_chksum(struct tx_desc	*ptxdesc);

s32 rtl8192cu_xmitframe_complete(_adapter *padapter, struct xmit_priv *pxmitpriv, struct xmit_buf *pxmitbuf);

void rtl8192cu_mgnt_xmit(_adapter *padapter, struct xmit_frame *pmgntframe);

s32 rtl8192cu_hal_xmit(_adapter *padapter, struct xmit_frame *pxmitframe);

#ifdef CONFIG_HOSTAPD_MLME
s32 rtl8192cu_hostap_mgnt_xmit_entry(_adapter *padapter, _pkt *pkt);
#endif

#endif

#ifdef CONFIG_PCI_HCI
s32	rtl8192ce_init_xmit_priv(_adapter * padapter);
void	rtl8192ce_free_xmit_priv(_adapter * padapter);

s32	rtl8192ce_enqueue_xmitbuf(struct rtw_tx_ring *ring, struct xmit_buf *pxmitbuf);
struct xmit_buf *rtl8192ce_dequeue_xmitbuf(struct rtw_tx_ring *ring);

void	rtl8192ce_xmitframe_resume(_adapter *padapter);

void	rtl8192ce_mgnt_xmit(_adapter *padapter, struct xmit_frame *pmgntframe);

s32	rtl8192ce_hal_xmit(_adapter *padapter, struct xmit_frame *pxmitframe);

#ifdef CONFIG_HOSTAPD_MLME
s32	rtl8192ce_hostap_mgnt_xmit_entry(_adapter *padapter, _pkt *pkt);
#endif

#endif

#endif

