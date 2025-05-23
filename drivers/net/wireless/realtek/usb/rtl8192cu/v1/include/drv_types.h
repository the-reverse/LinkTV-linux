/*-------------------------------------------------------------------------------
	
	For type defines and data structure defines

--------------------------------------------------------------------------------*/


#ifndef __DRV_TYPES_H__
#define __DRV_TYPES_H__

#include <drv_conf.h>
#include <osdep_service.h>
#include <wlan_bssdef.h>


#ifdef PLATFORM_OS_XP
#include <drv_types_xp.h>
#endif

#ifdef PLATFORM_OS_CE
#include <drv_types_ce.h>
#endif

#ifdef PLATFORM_LINUX
#include <drv_types_linux.h>
#endif

enum _NIC_VERSION {
	
	RTL8711_NIC,
	RTL8712_NIC,
	RTL8713_NIC,
	RTL8716_NIC
		
};

enum{
	UP_LINK,
	DOWN_LINK,	
};
typedef struct _ADAPTER _adapter, ADAPTER,*PADAPTER;

#ifdef CONFIG_80211N_HT
#include <rtw_ht.h>
#endif

#include <rtw_cmd.h>
#include <wlan_bssdef.h>
#include <rtw_xmit.h>
#include <rtw_recv.h>
#include <hal_init.h>
#include <rtw_qos.h>
#include <rtw_security.h>
#include <rtw_pwrctrl.h>
#include <rtw_io.h>
#include <rtw_eeprom.h>
#include <sta_info.h>
#include <rtw_mlme.h>
#include <rtw_mp.h>
#include <rtw_debug.h>
#include <rtw_rf.h>
#include <rtw_event.h>
#include <rtw_led.h>
#include <rtw_mlme_ext.h>

#ifdef CONFIG_DRVEXT_MODULE
#include <rtl871x_drvext.h>
#include <wsc_api.h>
#endif


#define SPEC_DEV_ID_NONE BIT(0)
#define SPEC_DEV_ID_DISABLE_HT BIT(1)
#define SPEC_DEV_ID_ENABLE_PS BIT(2)
#define SPEC_DEV_ID_RF_CONFIG_1T1R BIT(3)
#define SPEC_DEV_ID_RF_CONFIG_2T2R BIT(4)

struct specific_device_id{
	
	u32		flags;
	
	u16		idVendor;
	u16		idProduct;

};

struct registry_priv
{    
	u8	chip_version;
	u8	rfintfs;
	u8	lbkmode;
	u8	hci;
	u8	network_mode;	//infra, ad-hoc, auto	  
	NDIS_802_11_SSID	ssid;
	u8	channel;//ad-hoc support requirement 
	u8	wireless_mode;//A, B, G, auto
	u8	vrtl_carrier_sense;//Enable, Disable, Auto
	u8	vcs_type;//RTS/CTS, CTS-to-self
	u16	rts_thresh;
	u16  frag_thresh;	
	u8	preamble;//long, short, auto
	u8  scan_mode;//active, passive
	u8  adhoc_tx_pwr;
	u8      	     soft_ap;
	u8      	     smart_ps;  
	 u8                  power_mgnt;
	 u8                  radio_enable;
	 u8                  long_retry_lmt;
	 u8                  short_retry_lmt;
  	 u16                 busy_thresh;
    	 u8                  ack_policy;
	 u8		     mp_mode;	
	 u8 		     software_encrypt;
	 u8 		     software_decrypt;	  

	  //UAPSD
	  u8		     wmm_enable;
	  u8		     uapsd_enable;	  
	  u8		     uapsd_max_sp;
	  u8		     uapsd_acbk_en;
	  u8		     uapsd_acbe_en;
	  u8		     uapsd_acvi_en;
	  u8		     uapsd_acvo_en;	  

	  WLAN_BSSID_EX    dev_network;

#ifdef CONFIG_80211N_HT

	u8		ht_enable;
	u8		cbw40_enable;
	u8		ampdu_enable;//for tx
	

#endif
	u8		rf_config ;
	u8		low_power ;

	u8 		wifi_spec;// !turbo_mode	  
	  
	u8 		channel_plan;
#ifdef CONFIG_BT_COEXIST
	u8		bt_iso;
	u8		bt_sco;
	u8		bt_ampdu;
#endif
	BOOLEAN	bAcceptAddbaReq;	

#ifdef CONFIG_ANTENNA_DIVERSITY
	u8		antdiv_cfg;
#endif
};


//For registry parameters
#define RGTRY_OFT(field) ((ULONG)FIELD_OFFSET(struct registry_priv,field))
#define RGTRY_SZ(field)   sizeof(((struct registry_priv*) 0)->field)
#define BSSID_OFT(field) ((ULONG)FIELD_OFFSET(WLAN_BSSID_EX,field))
#define BSSID_SZ(field)   sizeof(((PWLAN_BSSID_EX) 0)->field)


struct dvobj_priv {

	_adapter * padapter;

/*-------- below is for SDIO INTERFACE --------*/

#ifdef CONFIG_SDIO_HCI

#ifdef PLATFORM_OS_XP
	PDEVICE_OBJECT	pphysdevobj;//pPhysDevObj;
	PDEVICE_OBJECT	pfuncdevobj;//pFuncDevObj;
	PDEVICE_OBJECT	pnextdevobj;//pNextDevObj;
	SDBUS_INTERFACE_STANDARD	sdbusinft;//SdBusInterface;
	u8	nextdevstacksz;//unsigned char			 NextDeviceStackSize;
#endif//PLATFORM_OS_XP

#ifdef PLATFORM_OS_CE
	SD_DEVICE_HANDLE hDevice;     
	SD_CARD_RCA                 sd_rca;
	SD_CARD_INTERFACE           card_intf;
	BOOLEAN                     enableIsarWithStatus;
	WCHAR	active_path[MAX_ACTIVE_REG_PATH];
	SD_HOST_BLOCK_CAPABILITY    sd_host_blk_cap;
#endif//PLATFORM_OS_CE

#ifdef PLATFORM_LINUX
	struct sdio_func	*func;	
#endif//PLATFORM_LINUX

	u8	func_number;//unsigned char			FunctionNumber;
	u32	block_transfer_len;//unsigned long			BLOCK_TRANSFER_LEN;
	u32	blk_shiftbits;
	u16	driver_version;
	u16	rxblknum;
	u16	rxblknum_rd;
	u16	c2hblknum; 
	u8  tx_block_mode;
	u8  rx_block_mode;
	u8 cmdfifo_cnt;
	u8 rxfifo_cnt;
	u16	sdio_hisr;
	u16	sdio_himr;	
#endif//	CONFIG_SDIO_HCI

/*-------- below is for USB INTERFACE --------*/
 
#ifdef CONFIG_USB_HCI

	u8	nr_endpoint;
	u8	InterfaceNumber;
	u8	RtNumInPipes;
	u8	RtNumOutPipes;
	u8	ishighspeed;
	
	_sema	usb_suspend_sema;
	
#ifdef PLATFORM_WINDOWS
	//related device objects
	PDEVICE_OBJECT	pphysdevobj;//pPhysDevObj;
	PDEVICE_OBJECT	pfuncdevobj;//pFuncDevObj;
	PDEVICE_OBJECT	pnextdevobj;//pNextDevObj;

	u8	nextdevstacksz;//unsigned char NextDeviceStackSize;	//= (CHAR)CEdevice->pUsbDevObj->StackSize + 1; 

	//urb for control diescriptor request

#ifdef PLATFORM_OS_XP
	struct _URB_CONTROL_DESCRIPTOR_REQUEST descriptor_urb;
	PUSB_CONFIGURATION_DESCRIPTOR	pconfig_descriptor;//UsbConfigurationDescriptor;
#endif

#ifdef PLATFORM_OS_CE
	WCHAR			active_path[MAX_ACTIVE_REG_PATH];	// adapter regpath
	USB_EXTENSION	usb_extension;

	_nic_hdl		pipehdls_r8192c[0x10];
#endif

	u32	config_descriptor_len;//ULONG UsbConfigurationDescriptorLength;	
#endif//PLATFORM_WINDOWS

#ifdef PLATFORM_LINUX
	struct usb_device *pusbdev;
#endif//PLATFORM_LINUX

#endif//CONFIG_USB_HCI

};


struct _ADAPTER{	
 	
	struct 	dvobj_priv dvobjpriv;
	struct	mlme_priv mlmepriv;
	struct	mlme_ext_priv mlmeextpriv;
	struct	cmd_priv	cmdpriv;
	struct	evt_priv	evtpriv;
	//struct	io_queue	*pio_queue;
	struct 	io_priv	iopriv;
	struct	xmit_priv	xmitpriv;
	struct	recv_priv	recvpriv;
	struct	sta_priv	stapriv;
	struct	security_priv	securitypriv;	
	struct	registry_priv	registrypriv;
	struct	wlan_acl_pool	acl_list;
	struct	pwrctrl_priv	pwrctrlpriv;
	struct 	eeprom_priv eeprompriv;
	//struct	hal_priv	halpriv;
	struct	led_priv	ledpriv;
	//struct 	dm_priv	dmpriv;
	
#ifdef CONFIG_MP_INCLUDED
       struct	mp_priv	mppriv;
#endif

#ifdef CONFIG_DRVEXT_MODULE
	struct	drvext_priv	drvextpriv;
#endif
	
#ifdef CONFIG_AP_MODE
	struct	hostapd_priv	*phostapdpriv;		
#endif

	PVOID			HalData;
	struct hal_ops	HalFunc;

	int 	chip_type;
#ifdef CONFIG_BT_COEXIST
	//struct	btcoexist_priv	bt_coexist;
#endif
	s32	bDriverStopped; 
	s32	bSurpriseRemoved;
	s32  bCardDisableWOHSM;

	u32	IsrContent;
	u32	ImrContent;	
	
	u8	EepromAddressSize;		
	u8	hw_init_completed;	
	
	_thread_hdl_	cmdThread;
	_thread_hdl_	evtThread;
	_thread_hdl_	xmitThread;
	_thread_hdl_	recvThread;


	NDIS_STATUS (*dvobj_init)(_adapter * adapter);
	void  (*dvobj_deinit)(_adapter * adapter);
	

#ifdef PLATFORM_WINDOWS
	_nic_hdl		hndis_adapter;//hNdisAdapter(NDISMiniportAdapterHandle);
	_nic_hdl		hndis_config;//hNdisConfiguration;
	NDIS_STRING fw_img;

	u32	NdisPacketFilter;	
	u8	MCList[MAX_MCAST_LIST_NUM][6];
	u32	MCAddrCount;	
#endif //end of PLATFORM_WINDOWS


#ifdef PLATFORM_LINUX	
	_nic_hdl pnetdev;
	int bup;
	struct net_device_stats stats;
	struct iw_statistics iwstats;
#endif //end of PLATFORM_LINUX

	int pid;//process id from UI

	int net_closed;

	u8 bFWReady;
	u8 bReadPortCancel;
	u8 bWritePortCancel;
};	
  
__inline static u8 *myid(struct eeprom_priv *peepriv)
{
	return (peepriv->mac_addr);
}


#endif //__DRV_TYPES_H__

