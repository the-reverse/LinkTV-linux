#ifndef __RTW_IOCTL_SET_H_
#define __RTW_IOCTL_SET_H_

#include <drv_conf.h>
#include <drv_types.h>


typedef u8 NDIS_802_11_PMKID_VALUE[16];

typedef struct _BSSIDInfo {
	NDIS_802_11_MAC_ADDRESS  BSSID;
	NDIS_802_11_PMKID_VALUE  PMKID;
} BSSIDInfo, *PBSSIDInfo;


#ifdef PLATFORM_OS_XP
typedef struct _NDIS_802_11_PMKID {
	u32	Length;
	u32	BSSIDInfoCount;
	BSSIDInfo BSSIDInfo[1];
} NDIS_802_11_PMKID, *PNDIS_802_11_PMKID;
#endif


#ifdef PLATFORM_WINDOWS
u8 set_802_11_reload_defaults(_adapter * padapter, NDIS_802_11_RELOAD_DEFAULTS reloadDefaults);
u8 set_802_11_test(_adapter * padapter, NDIS_802_11_TEST * test);
u8 set_802_11_pmkid(_adapter *pdapter, NDIS_802_11_PMKID *pmkid);

u8 pnp_set_power_sleep(_adapter* padapter);
u8 pnp_set_power_wakeup(_adapter* padapter);

void pnp_resume_wk(void *context);
void pnp_sleep_wk(void * context);

#endif

u8 set_802_11_add_key(_adapter * padapter, NDIS_802_11_KEY * key);
u8 set_802_11_authentication_mode(_adapter *pdapter, NDIS_802_11_AUTHENTICATION_MODE authmode);
u8 set_802_11_bssid(_adapter* padapter, u8 *bssid);
u8 set_802_11_add_wep(_adapter * padapter, NDIS_802_11_WEP * wep);
u8 set_802_11_disassociate(_adapter * padapter);
u8 set_802_11_bssid_list_scan(_adapter* padapter);
u8 set_802_11_infrastructure_mode(_adapter * padapter, NDIS_802_11_NETWORK_INFRASTRUCTURE networktype);
u8 set_802_11_remove_wep(_adapter * padapter, u32 keyindex);
u8 set_802_11_ssid(_adapter * padapter, NDIS_802_11_SSID * ssid);
u8 set_802_11_remove_key(_adapter * padapter, NDIS_802_11_REMOVE_KEY * key);


u8 validate_ssid(NDIS_802_11_SSID *ssid);


#endif

