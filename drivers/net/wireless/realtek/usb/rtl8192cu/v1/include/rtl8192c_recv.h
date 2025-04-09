#ifndef _RTL8192C_RECV_H_
#define _RTL8192C_RECV_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


#ifdef PLATFORM_OS_XP
	#ifdef CONFIG_SDIO_HCI
		#define NR_RECVBUFF 1024//512//128
	#else
		#define NR_RECVBUFF (16)
	#endif
#elif defined(PLATFORM_OS_CE)
	#ifdef CONFIG_SDIO_HCI
		#define NR_RECVBUFF (128)
	#else
		#define NR_RECVBUFF (4)
	#endif
#else
	#define NR_RECVBUFF (4)
	#define NR_PREALLOC_RECV_SKB (8)
#endif



#define RECV_BLK_SZ 512
#define RECV_BLK_CNT 16
#define RECV_BLK_TH RECV_BLK_CNT

//#define MAX_RECVBUF_SZ 2048 // 2k
//#define MAX_RECVBUF_SZ (8192) // 8K
//#define MAX_RECVBUF_SZ (16384) //16K
//#define MAX_RECVBUF_SZ (16384 + 1024) //16K + 1k
//#define MAX_RECVBUF_SZ (30720) //30k
//#define MAX_RECVBUF_SZ (30720 + 1024) //30k+1k
//#define MAX_RECVBUF_SZ (32768) // 32k

#if defined(CONFIG_SDIO_HCI)

#define MAX_RECVBUF_SZ (50000) //30k //(2048)//(30720) //30k

#elif defined(CONFIG_USB_HCI)

#ifdef PLATFORM_OS_CE
#define MAX_RECVBUF_SZ (8192+1024) // 8K+1k
#else
//#define MAX_RECVBUF_SZ (32768) // 32k
//#define MAX_RECVBUF_SZ (16384) //16K
//#define MAX_RECVBUF_SZ (10240) //10K
#define MAX_RECVBUF_SZ (15360) // 15k < 16k
#endif

#endif

#define RECV_BULK_IN_ADDR		0x80
#define RECV_INT_IN_ADDR		0x81


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

void rtl8192cu_init_recv_priv(_adapter * padapter);

void rtl8192cu_update_recvframe_attrib_from_recvstat(union recv_frame *precvframe, struct recv_stat *prxstat);

void rtl8192c_query_rx_phy_status(union recv_frame *prframe, struct recv_stat *prxstat);

void rtl8192c_process_phy_info(_adapter *padapter, void *prframe);

#endif

