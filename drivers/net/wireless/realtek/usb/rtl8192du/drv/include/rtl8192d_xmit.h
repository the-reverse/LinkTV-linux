#ifndef _RTL8192D_XMIT_H_
#define _RTL8192D_XMIT_H_

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

//Because we open EM for normal case, we just always insert 2*8 bytes.by wl
#define USB_92D_DUMMY_OFFSET		2
#define USB_92D_DUMMY_LENGTH		(USB_92D_DUMMY_OFFSET * PACKET_OFFSET_SZ)
#define USB_HWDESC_HEADER_LEN	(TXDESC_SIZE + USB_92D_DUMMY_LENGTH)

//For 92D early mode
#define SET_EARLYMODE_PKTNUM(__pAddr, __Value) SET_BITS_TO_LE_4BYTE(__pAddr, 0, 3, __Value)
#define SET_EARLYMODE_LEN0(__pAddr, __Value) SET_BITS_TO_LE_4BYTE(__pAddr, 4, 12, __Value)
#define SET_EARLYMODE_LEN1(__pAddr, __Value) SET_BITS_TO_LE_4BYTE(__pAddr, 16, 12, __Value)
#define SET_EARLYMODE_LEN2_1(__pAddr, __Value) SET_BITS_TO_LE_4BYTE(__pAddr, 28, 4, __Value)
#define SET_EARLYMODE_LEN2_2(__pAddr, __Value) SET_BITS_TO_LE_4BYTE(__pAddr+4, 0, 8, __Value)
#define SET_EARLYMODE_LEN3(__pAddr, __Value) SET_BITS_TO_LE_4BYTE(__pAddr+4, 8, 12, __Value)
#define SET_EARLYMODE_LEN4(__pAddr, __Value) SET_BITS_TO_LE_4BYTE(__pAddr+4, 20, 12, __Value)

#ifdef CONFIG_USB_HCI

#ifdef USB_TX_AGGREGATION_92C
#define MAX_TX_AGG_PACKET_NUMBER 0xFF
#endif

s32	rtl8192du_init_xmit_priv(_adapter * padapter);

void	rtl8192du_free_xmit_priv(_adapter * padapter);

void rtl8192du_cal_txdesc_chksum(struct tx_desc	*ptxdesc);

s32 rtl8192du_xmitframe_complete(_adapter *padapter, struct xmit_priv *pxmitpriv, struct xmit_buf *pxmitbuf);

void rtl8192du_mgnt_xmit(_adapter *padapter, struct xmit_frame *pmgntframe);

s32 rtl8192du_hal_xmit(_adapter *padapter, struct xmit_frame *pxmitframe);
#endif

#endif

