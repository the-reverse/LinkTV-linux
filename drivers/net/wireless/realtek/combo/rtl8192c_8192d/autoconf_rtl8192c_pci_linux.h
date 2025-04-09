/*
 * Automatically generated C config: don't edit
 */
#define AUTOCONF_INCLUDED
#define RTL871X_MODULE_NAME "92CE"

//#define CONFIG_DEBUG_RTL871X 1

#undef CONFIG_USB_HCI
#undef  CONFIG_SDIO_HCI
#define CONFIG_PCI_HCI	1

#undef CONFIG_RTL8711
#undef  CONFIG_RTL8712
#define	CONFIG_RTL8192C 1
//#define	CONFIG_RTL8192D 1


//#define CONFIG_LITTLE_ENDIAN 1 //move to Makefile depends on platforms
//#undef CONFIG_BIG_ENDIAN

#undef PLATFORM_WINDOWS
#undef PLATFORM_OS_XP 
#undef PLATFORM_OS_CE


#define PLATFORM_LINUX 1

//#define CONFIG_PWRCTRL	1
//#define CONFIG_H2CLBK 1

//#define CONFIG_MP_INCLUDED 1

//#undef CONFIG_EMBEDDED_FWIMG
#define CONFIG_EMBEDDED_FWIMG 1

#define CONFIG_R871X_TEST 1

#define CONFIG_80211N_HT 1

#define CONFIG_RECV_REORDERING_CTRL 1

//#define CONFIG_RTL8712_TCP_CSUM_OFFLOAD_RX 1

//#define CONFIG_DRVEXT_MODULE 1

//#define CONFIG_IPS	1
//#define CONFIG_LPS	1
//#define CONFIG_PM 	1
//#define CONFIG_BT_COEXIST  	1
//#define CONFIG_ANTENNA_DIVERSITY	1


#ifdef PLATFORM_LINUX
	#define CONFIG_PROC_DEBUG 1
#endif

#ifdef CONFIG_RTL8192C

	#define DBG 0

	#define CONFIG_DEBUG_RTL8192C		1

	#define DEV_BUS_PCI_INTERFACE				1
	#define DEV_BUS_USB_INTERFACE				2	

	#define RTL8192C_RX_PACKET_NO_INCLUDE_CRC	1

	#ifdef CONFIG_PCI_HCI

		#define DEV_BUS_TYPE	DEV_BUS_PCI_INTERFACE

		#ifdef PLATFORM_LINUX
			#define CONFIG_SKB_COPY //for amsdu
		#endif

	#endif

	
	#define DISABLE_BB_RF	0	

	#define RTL8191C_FPGA_NETWORKTYPE_ADHOC 0

	#ifdef CONFIG_MP_INCLUDED
		#define MP_DRIVER 1
	#else
		#define MP_DRIVER 0
	#endif

#endif

