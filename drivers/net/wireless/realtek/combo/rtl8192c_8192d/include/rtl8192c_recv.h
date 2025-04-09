#ifndef _RTL8192C_RECV_H_
#define _RTL8192C_RECV_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


#ifdef PLATFORM_OS_XP
	#define NR_RECVBUFF (16)
#elif defined(PLATFORM_OS_CE)
	#define NR_RECVBUFF (4)
#else
	#define NR_RECVBUFF (4)
	#define NR_PREALLOC_RECV_SKB (8)
#endif


#define RECV_BLK_SZ 512
#define RECV_BLK_CNT 16
#define RECV_BLK_TH RECV_BLK_CNT

#if defined(CONFIG_USB_HCI)

#ifdef PLATFORM_OS_CE
#define MAX_RECVBUF_SZ (8192+1024) // 8K+1k
#else
//#define MAX_RECVBUF_SZ (32768) // 32k
//#define MAX_RECVBUF_SZ (16384) //16K
//#define MAX_RECVBUF_SZ (10240) //10K
#define MAX_RECVBUF_SZ (15360) // 15k < 16k
#endif

#elif defined(CONFIG_PCI_HCI)
#define MAX_RECVBUF_SZ (9100)

#define RX_MPDU_QUEUE				0
#define RX_CMD_QUEUE				1
#define RX_MAX_QUEUE				2
#endif


#define RECV_BULK_IN_ADDR		0x80
#define RECV_INT_IN_ADDR		0x81

#define PHY_RSSI_SLID_WIN_MAX				100
#define PHY_LINKQUALITY_SLID_WIN_MAX		20

#define PHY_STAT_GAIN_TRSW_SHT 0
#define PHY_STAT_PWDB_ALL_SHT 4
#define PHY_STAT_CFOSHO_SHT 5
#define PHY_STAT_CCK_AGC_RPT_SHT 5
#define PHY_STAT_CFOTAIL_SHT 9
#define PHY_STAT_RXEVM_SHT 13
#define PHY_STAT_RXSNR_SHT 15
#define PHY_STAT_PDSNR_SHT 19
#define PHY_STAT_CSI_CURRENT_SHT 21
#define PHY_STAT_CSI_TARGET_SHT 23
#define PHY_STAT_SIGEVM_SHT 25
#define PHY_STAT_MAX_EX_PWR_SHT 26

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


#ifdef CONFIG_USB_HCI
typedef struct _INTERRUPT_MSG_FORMAT_EX{
	unsigned int C2H_MSG0;
	unsigned int C2H_MSG1;
	unsigned int C2H_MSG2;
	unsigned int C2H_MSG3;
	unsigned int HISR; // from HISR Reg0x124, read to clear
	unsigned int HISRE;// from HISRE Reg0x12c, read to clear
	unsigned int  MSG_EX;
}INTERRUPT_MSG_FORMAT_EX,*PINTERRUPT_MSG_FORMAT_EX;

void rtl8192cu_init_recvbuf(_adapter *padapter, struct recv_buf *precvbuf);
int	rtl8192cu_init_recv_priv(_adapter * padapter);
void rtl8192cu_free_recv_priv(_adapter * padapter);
void rtl8192cu_update_recvframe_attrib_from_recvstat(union recv_frame *precvframe, struct recv_stat *prxstat);
#endif

#ifdef CONFIG_PCI_HCI
int	rtl8192ce_init_recv_priv(_adapter * padapter);
void rtl8192ce_free_recv_priv(_adapter * padapter);
void rtl8192ce_update_recvframe_attrib_from_recvstat(union recv_frame *precvframe, struct recv_stat *prxstat);
#endif

void rtl8192c_query_rx_phy_status(union recv_frame *prframe, struct phy_stat *pphy_stat);
void rtl8192c_process_phy_info(_adapter *padapter, void *prframe);

#endif

