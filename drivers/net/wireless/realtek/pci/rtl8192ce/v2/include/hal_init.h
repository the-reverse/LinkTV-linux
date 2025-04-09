
#ifndef __HAL_INIT_H__
#define __HAL_INIT_H__

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#ifdef CONFIG_PCI_HCI
#include <pci_hal.h>
#endif


enum RTL871X_HCI_TYPE {

	RTL8192C_SDIO,
	RTL8192C_USB,
	RTL8192C_PCIE
};

enum _CHIP_TYPE {

	NULL_CHIP_TYPE,
	RTL8712_8188S_8191S_8192S,
	RTL8188C_8192C,
	RTL8192D,
	MAX_CHIP_TYPE
};


struct hal_ops {
	u32	(*hal_init)(PADAPTER Adapter);
	u32	(*hal_deinit)(PADAPTER Adapter);

	void	(*free_hal_data)(PADAPTER Adapter);

	u32	(*inirp_init)(PADAPTER Adapter);
	u32	(*inirp_deinit)(PADAPTER Adapter);

	s32	(*init_xmit_priv)(PADAPTER Adapter);
	void	(*free_xmit_priv)(PADAPTER Adapter);

	s32	(*init_recv_priv)(PADAPTER Adapter);
	void	(*free_recv_priv)(PADAPTER Adapter);

	void	(*InitSwLeds)(PADAPTER Adapter);
	void	(*DeInitSwLeds)(PADAPTER Adapter);

	void	(*dm_init)(PADAPTER Adapter);
	void	(*dm_deinit)(PADAPTER Adapter);

	void	(*init_default_value)(PADAPTER Adapter);

	void	(*intf_chip_configure)(PADAPTER Adapter);

	void	(*read_adapter_info)(PADAPTER Adapter);

	void	(*enable_interrupt)(PADAPTER Adapter);
	void	(*disable_interrupt)(PADAPTER Adapter);
	s32	(*interrupt_handler)(PADAPTER Adapter);

	void	(*set_bwmode_handler)(PADAPTER Adapter, HT_CHANNEL_WIDTH Bandwidth, u8 Offset);
	void	(*set_channel_handler)(PADAPTER Adapter, u8 channel);

	void	(*process_phy_info)(PADAPTER Adapter, void *prframe);
	void	(*hal_dm_watchdog)(PADAPTER Adapter);

	void	(*SetHwRegHandler)(PADAPTER Adapter, u8	variable,u8* val);
	void	(*GetHwRegHandler)(PADAPTER Adapter, u8	variable,u8* val);

	void	(*UpdateRAMaskHandler)(PADAPTER Adapter, u32 cam_idx);
	void	(*SetBeaconRelatedRegistersHandler)(PADAPTER Adapter);

	void	(*Add_RateATid)(PADAPTER Adapter, u32 bitmap, u8 arg);

#ifdef CONFIG_ANTENNA_DIVERSITY
	u8	(*SwAntDivBeforeLinkHandler)(PADAPTER Adapter);
	void	(*SwAntDivCompareHandler)(PADAPTER Adapter, WLAN_BSSID_EX *dst, WLAN_BSSID_EX *src);
#endif

	s32	(*hal_xmit)(PADAPTER Adapter, struct xmit_frame *pxmitframe);
	void	(*mgnt_xmit)(PADAPTER Adapter, struct xmit_frame *pmgntframe);

	u32	(*read_bbreg)(PADAPTER Adapter, u32 RegAddr, u32 BitMask);
	void	(*write_bbreg)(PADAPTER Adapter, u32 RegAddr, u32 BitMask, u32 Data);
	u32	(*read_rfreg)(PADAPTER Adapter, u32 eRFPath, u32 RegAddr, u32 BitMask);
	void	(*write_rfreg)(PADAPTER Adapter, u32 eRFPath, u32 RegAddr, u32 BitMask, u32 Data);

#ifdef CONFIG_HOSTAPD_MLME
	s32	(*hostap_mgnt_xmit_entry)(PADAPTER Adapter, _pkt *pkt);
#endif

	
};

typedef enum _HW_VARIABLES{
	HW_VAR_MEDIA_STATUS,
	HW_VAR_MEDIA_STATUS1,
	HW_VAR_SET_OPMODE,
	HW_VAR_BSSID,
	HW_VAR_TXPAUSE,
	HW_VAR_BCN_FUNC,
	HW_VAR_CORRECT_TSF,
	HW_VAR_CHECK_BSSID,
	HW_VAR_MLME_DISCONNECT,
	HW_VAR_MLME_SITESURVEY,
	HW_VAR_MLME_JOIN,
	HW_VAR_BEACON_INTERVAL,
	HW_VAR_INIT_RTS_RATE,
	HW_VAR_SEC_CFG,
	HW_VAR_TX_BCN_DONE,
	HW_VAR_RF_TYPE,
	HW_VAR_DM_FLAG,
	HW_VAR_DM_FUNC_OP,
	HW_VAR_DM_FUNC_SET,
	HW_VAR_DM_FUNC_CLR,
	HW_VAR_CAM_MARK_INVALID,
	HW_VAR_CAM_EMPTY_ENTRY,
	HW_VAR_CAM_INVALID_ALL,
	HW_VAR_CAM_WRITE,
	HW_VAR_CAM_FLUSH_ALL,
	HW_VAR_AC_PARAM_VO,
	HW_VAR_AC_PARAM_VI,
	HW_VAR_AC_PARAM_BE,
	HW_VAR_AC_PARAM_BK,
	HW_VAR_AMPDU_MIN_SPACE,
	HW_VAR_AMPDU_FACTOR,
	HW_VAR_RXDMA_AGG_PG_TH,
	HW_VAR_SET_RPWM,
	HW_VAR_H2C_FW_PWRMODE,
	HW_VAR_H2C_FW_JOINBSSRPT,
	HW_VAR_FWLPS_RF_ON,
	HW_VAR_INITIAL_GAIN,
	HW_VAR_TRIGGER_GPIO_0,
	HW_VAR_BT_SET_COEXIST,
	HW_VAR_BT_ISSUE_DELBA,	
	HW_VAR_CURRENT_ANTENNA,
	HW_VAR_ANTENNA_DIVERSITY_JOIN,
	HW_VAR_ANTENNA_DIVERSITY_LINK,
	HW_VAR_ANTENNA_DIVERSITY_SELECT,
	HW_VAR_SWITCH_EPHY_WoWLAN,
}HW_VARIABLES;

typedef	enum _RT_EEPROM_TYPE{
	EEPROM_93C46,
	EEPROM_93C56,
	EEPROM_BOOT_EFUSE,
}RT_EEPROM_TYPE,*PRT_EEPROM_TYPE;

#define USB_HIGH_SPEED_BULK_SIZE	512
#define USB_FULL_SPEED_BULK_SIZE	64

#define RF_CHANGE_BY_INIT	0
#define RF_CHANGE_BY_IPS 	BIT28
#define RF_CHANGE_BY_PS 	BIT29
#define RF_CHANGE_BY_HW 	BIT30
#define RF_CHANGE_BY_SW 	BIT31

typedef struct eeprom_priv EEPROM_EFUSE_PRIV, *PEEPROM_EFUSE_PRIV;
#define GET_EEPROM_EFUSE_PRIV(priv)	(&priv->eeprompriv)

uint rtw_hal_init(_adapter *padapter);
uint rtw_hal_deinit(_adapter *padapter);
void rtw_hal_stop(_adapter *padapter);

#endif //__HAL_INIT_H__

