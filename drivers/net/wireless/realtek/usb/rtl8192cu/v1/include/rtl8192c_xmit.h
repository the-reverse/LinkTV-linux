#ifndef _RTL8192C_XMIT_H_
#define _RTL8192C_XMIT_H_

#define VO_QUEUE_INX	0
#define VI_QUEUE_INX	1
#define BE_QUEUE_INX	2
#define BK_QUEUE_INX	3
#define TS_QUEUE_INX	4
#define MGT_QUEUE_INX	5
#define BMC_QUEUE_INX	6
#define BCN_QUEUE_INX	7

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


#if USB_TX_AGGREGATION_92C
#define MAX_TX_AGG_PACKET_NUMBER 0xFF
#endif

#if (DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
void rtl8192cu_cal_txdesc_chksum(struct tx_desc	*ptxdesc);

s32 rtl8192cu_xmitframe_complete(_adapter *padapter, struct xmit_priv *pxmitpriv, struct xmit_buf *pxmitbuf);

void rtl8192cu_mgnt_xmit(_adapter *padapter, struct xmit_frame *pmgntframe);

s32 rtl8192cu_hal_xmit(_adapter *padapter, struct xmit_frame *pxmitframe);
#endif

#ifdef CONFIG_HOSTAPD_MLME
s32 rtl8192cu_hostap_mgnt_xmit_entry(_adapter *padapter, _pkt *pkt);
#endif

#endif

