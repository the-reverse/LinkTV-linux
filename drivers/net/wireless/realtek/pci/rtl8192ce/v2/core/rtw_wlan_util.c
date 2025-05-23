/******************************************************************************
* rtw_wlan_util.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2008, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _RTW_WLAN_UTIL_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <wifi.h>


unsigned char ARTHEROS_OUI1[] = {0x00, 0x03, 0x7f};
unsigned char ARTHEROS_OUI2[] = {0x00, 0x13, 0x74};

unsigned char BROADCOM_OUI1[] = {0x00, 0x10, 0x18};
unsigned char BROADCOM_OUI2[] = {0x00, 0x0a, 0xf7};
unsigned char BROADCOM_OUI3[] = {0x00, 0x05, 0xb5};

unsigned char CISCO_OUI[] = {0x00, 0x40, 0x96};
unsigned char MARVELL_OUI[] = {0x00, 0x50, 0x43};
unsigned char RALINK_OUI[] = {0x00, 0x0c, 0x43};
unsigned char REALTEK_OUI[] = {0x00, 0xe0, 0x4c};
unsigned char AIRGOCAP_OUI[] = {0x00, 0x0a, 0xf5};

unsigned char REALTEK_96B_IE[] = {0x00, 0xe0, 0x4c, 0x02, 0x01, 0x20};

extern unsigned char	MCS_rate_2R[16];
extern unsigned char	MCS_rate_1R[16];
extern unsigned char WPA_OUI[];
extern unsigned char WPA_TKIP_CIPHER[4];
extern unsigned char RSN_TKIP_CIPHER[4];

#define R2T_PHY_DELAY	(0)

//#define WAIT_FOR_BCN_TO_MIN	(3000)
#define WAIT_FOR_BCN_TO_MIN	(6000)
#define WAIT_FOR_BCN_TO_MAX	(20000)


int cckrates_included(unsigned char *rate, int ratelen)
{
	int	i;
	
	for(i = 0; i < ratelen; i++)
	{
		if  (  (((rate[i]) & 0x7f) == 2)	|| (((rate[i]) & 0x7f) == 4) ||
			   (((rate[i]) & 0x7f) == 11)  || (((rate[i]) & 0x7f) == 22) )
		return _TRUE;	
	}

	return _FALSE;

}

int cckratesonly_included(unsigned char *rate, int ratelen)
{
	int	i;
	
	for(i = 0; i < ratelen; i++)
	{
		if  ( (((rate[i]) & 0x7f) != 2) && (((rate[i]) & 0x7f) != 4) &&
			   (((rate[i]) & 0x7f) != 11)  && (((rate[i]) & 0x7f) != 22) )
		return _FALSE;	
	}
	
	return _TRUE;
}

unsigned char networktype_to_raid(unsigned char network_type)
{
	unsigned char raid;

	switch(network_type)
	{
		case WIRELESS_11B:
			raid = 6;
			break;
		case WIRELESS_11G:
			raid = 5;
			break;
		case WIRELESS_11BG:
			raid = 4;
			break;
		case WIRELESS_11N:
			raid = 3;
			break;
		case WIRELESS_11GN:
			raid = 1;
			break;
		case WIRELESS_11BGN:
			raid = 0;
			break;
		default:
			raid = 4;
			break;	

	}

	return raid;
	
}

int judge_network_type(_adapter *padapter, unsigned char *rate, int ratelen)
{
	int network_type = 0;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	if (pmlmeinfo->HT_enable)
	{
		network_type = WIRELESS_11N;
	}
	
	if ((cckratesonly_included(rate, ratelen)) == _TRUE)
	{
		network_type |= WIRELESS_11B;
	}
	else if((cckrates_included(rate, ratelen)) == _TRUE)
	{
		network_type |= WIRELESS_11BG;
	}
	else
	{
		network_type |= WIRELESS_11G;
	}
		
	return 	network_type;
}

unsigned char ratetbl_val_2wifirate(unsigned char rate)
{
	unsigned char val = 0;

	switch (rate & 0x7f) 
	{
		case 0:
			val = IEEE80211_CCK_RATE_1MB;
			break;

		case 1:
			val = IEEE80211_CCK_RATE_2MB;
			break;

		case 2:
			val = IEEE80211_CCK_RATE_5MB;
			break;

		case 3:
			val = IEEE80211_CCK_RATE_11MB;
			break;
			
		case 4:
			val = IEEE80211_OFDM_RATE_6MB;
			break;

		case 5:
			val = IEEE80211_OFDM_RATE_9MB;
			break;

		case 6:
			val = IEEE80211_OFDM_RATE_12MB;
			break;
			
		case 7:
			val = IEEE80211_OFDM_RATE_18MB;
			break;

		case 8:
			val = IEEE80211_OFDM_RATE_24MB;
			break;
			
		case 9:
			val = IEEE80211_OFDM_RATE_36MB;
			break;

		case 10:
			val = IEEE80211_OFDM_RATE_48MB;
			break;
		
		case 11:
			val = IEEE80211_OFDM_RATE_54MB;
			break;

	}

	return val;

}

int is_basicrate(_adapter *padapter, unsigned char rate)
{
	int i;
	unsigned char val;
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	
	for(i = 0; i < NumRates; i++)
	{
		val = pmlmeext->basicrate[i];

		if ((val != 0xff) && (val != 0xfe))
		{
			if (rate == ratetbl_val_2wifirate(val))
			{
				return _TRUE;
			}
		}
	}
	
	return _FALSE;
}


unsigned int ratetbl2rateset(_adapter *padapter, unsigned char *rateset)
{
	int i;
	unsigned char rate;
	unsigned int	len = 0;
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;

	for (i = 0; i < NumRates; i++)
	{
		rate = pmlmeext->datarate[i];

		switch (rate)
		{
			case 0xff:
				return len;
				
			case 0xfe:
				continue;
				
			default:
				rate = ratetbl_val_2wifirate(rate);

				if (is_basicrate(padapter, rate) == _TRUE)
				{
					rate |= IEEE80211_BASIC_RATE_MASK;
				}
				
				rateset[len] = rate;
				len++;
				break;
		}
	}
	return len;
}


void get_rate_set(_adapter *padapter, unsigned char *pbssrate, int *bssrate_len)
{
	unsigned char supportedrates[NumRates];

	_memset(supportedrates, 0, NumRates);
	*bssrate_len = ratetbl2rateset(padapter, supportedrates);
	_memcpy(pbssrate, supportedrates, *bssrate_len);
}

void Save_DM_Func_Flag(_adapter *padapter)
{
	u8	bSaveFlag = _TRUE;

	padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_DM_FUNC_OP, (u8 *)(&bSaveFlag));
}

void Restore_DM_Func_Flag(_adapter *padapter)
{
	u8	bSaveFlag = _FALSE;

	padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_DM_FUNC_OP, (u8 *)(&bSaveFlag));
}

void Switch_DM_Func(_adapter *padapter, u8 mode, u8 enable)
{
	if(enable == _TRUE)
	{
		padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_DM_FUNC_SET, (u8 *)(&mode));
	}
	else
	{
		padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_DM_FUNC_CLR, (u8 *)(&mode));
	}

#if 0
	u8 val8;

	val8 = read8(padapter, FW_DYNAMIC_FUN_SWITCH);

	if(enable == _TRUE)
	{
		write8(padapter, FW_DYNAMIC_FUN_SWITCH, (val8 | mode));
	}
	else
	{
		write8(padapter, FW_DYNAMIC_FUN_SWITCH, (val8 & mode));
	}
#endif

}

void Set_NETYPE1_MSR(_adapter *padapter, u8 type)
{
	padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_MEDIA_STATUS1, (u8 *)(&type));
}

void Set_NETYPE0_MSR(_adapter *padapter, u8 type)
{
	padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_MEDIA_STATUS, (u8 *)(&type));
}

void SelectChannel(_adapter *padapter, unsigned char channel)
{
	unsigned int scanMode;
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	
	scanMode = (pmlmeext->sitesurvey_res.state == _TRUE)? 1: 0;//todo:

	if(padapter->HalFunc.set_channel_handler)
		padapter->HalFunc.set_channel_handler(padapter, channel);
	
}

void SetBWMode(_adapter *padapter, unsigned short bwmode, unsigned char channel_offset)
{
	if(padapter->HalFunc.set_bwmode_handler)
		padapter->HalFunc.set_bwmode_handler(padapter, (HT_CHANNEL_WIDTH)bwmode, channel_offset);
}

void set_channel_bwmode(_adapter *padapter, unsigned char channel, unsigned char channel_offset, unsigned short bwmode)
{
	if((bwmode == HT_CHANNEL_WIDTH_20)||(channel_offset == HAL_PRIME_CHNL_OFFSET_DONT_CARE))
	{
		SelectChannel(padapter, channel);
	}
	else		
	{
		//switch to the proper channel
		if (channel_offset == HAL_PRIME_CHNL_OFFSET_LOWER)
		{
			SelectChannel(padapter, channel + 2);
		}
		else
		{
			SelectChannel(padapter, channel - 2);
		}
	}	

	
	SetBWMode(padapter, bwmode, channel_offset);
	
}

int get_bsstype(unsigned short capability)
{
	if (capability & BIT(0))
	{
		return WIFI_FW_AP_STATE;
	}
	else if (capability & BIT(1))
	{
		return WIFI_FW_ADHOC_STATE;
	}
	else
	{
		return 0;		
	}
}

__inline u8 *get_my_bssid(WLAN_BSSID_EX *pnetwork)
{	
	return (pnetwork->MacAddress); 
}

u16 get_beacon_interval(WLAN_BSSID_EX *bss)
{
	unsigned short val;
	_memcpy((unsigned char *)&val, get_beacon_interval_from_ie(bss->IEs), 2);
	return le16_to_cpu(val);	

}

int is_client_associated_to_ap(_adapter *padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	if ((pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS) && ((pmlmeinfo->state&0x03) == WIFI_FW_STATION_STATE))
	{
		return _TRUE;
	}
	else
	{
		return _FAIL;
	}
}

int is_client_associated_to_ibss(_adapter *padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	if ((pmlmeinfo->state & WIFI_FW_ASSOC_SUCCESS) && ((pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE))
	{
		return _TRUE;
	}
	else
	{
		return _FAIL;
	}
}

int is_IBSS_empty(_adapter *padapter)
{
	unsigned int i;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	for (i = 6; i < NUM_STA; i++)
	{
		if (pmlmeinfo->FW_sta_info[i].status == 1)
		{
			return _FAIL;
		}
	}
	
	return _TRUE;
	
}

unsigned int decide_wait_for_beacon_timeout(unsigned int bcn_interval)
{
	if ((bcn_interval << 2) < WAIT_FOR_BCN_TO_MIN)
	{
		return WAIT_FOR_BCN_TO_MIN;
	} 
	else if ((bcn_interval << 2) > WAIT_FOR_BCN_TO_MAX)
	{
		return WAIT_FOR_BCN_TO_MAX;
	}	
	else
	{
		return ((bcn_interval << 2));
	}
}

void CAM_mark_invalid(
	PADAPTER     	Adapter,	
	u8 			ucIndex
)
{
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CAM_MARK_INVALID, (u8 *)(&ucIndex));
}

void CAM_empty_entry(
	PADAPTER     	Adapter,	
	u8 			ucIndex
)
{
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_CAM_EMPTY_ENTRY, (u8 *)(&ucIndex));
}

void invalidate_cam_all(_adapter *padapter)
{
	padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_CAM_INVALID_ALL, 0);
}

void write_cam(_adapter *padapter, u8 entry, u16 ctrl, u8 *mac, u8 *key)
{
	unsigned int	i, j, val, addr, cmd;
	u32	cam_val[2];

	addr = entry << 3;

	for (j = 0; j < 6; j++)
	{	
		switch (j)
		{
			case 0:
				val = (ctrl | (mac[0] << 16) | (mac[1] << 24) );
				break;
				
			case 1:
				val = (mac[2] | ( mac[3] << 8) | (mac[4] << 16) | (mac[5] << 24));
				break;
			
			default:
				i = (j - 2) << 2;
				val = (key[i] | (key[i+1] << 8) | (key[i+2] << 16) | (key[i+3] << 24));
				break;
				
		}

		cam_val[0] = val;
		cam_val[1] = addr + j;

		padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_CAM_WRITE, (u8 *)cam_val);
		
		//write32(padapter, WCAMI, val);
		
		//cmd = CAM_POLLINIG | CAM_WRITE | (addr + j);
		//write32(padapter, RWCAM, cmd);
		
		//printk("%s=> cam write: %x, %x\n",__FUNCTION__, cmd, val);
		
	}

}

int allocate_cam_entry(_adapter *padapter)
{
	unsigned int cam_idx;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	for (cam_idx = 6; cam_idx < NUM_STA; cam_idx++)
	{
		if (pmlmeinfo->FW_sta_info[cam_idx].status == 0)
		{
			pmlmeinfo->FW_sta_info[cam_idx].status = 1;
			pmlmeinfo->FW_sta_info[cam_idx].retry = 0;
			break;
		}
	}
	
	return cam_idx;
}

void flush_all_cam_entry(_adapter *padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
#if 0
	unsigned char null_sta[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	unsigned char null_key[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00,0x00, 0x00, 0x00, 0x00};

	for (i = 0; i < NUM_STA; i++)
	{
		write_cam(padapter, i, 0, null_sta, null_key);
	}
#else
	padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_CAM_FLUSH_ALL, 0);
#endif
	_memset((u8 *)(pmlmeinfo->FW_sta_info), 0, sizeof(pmlmeinfo->FW_sta_info));
}

int WMM_param_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs	pIE)
{
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	struct mlme_priv	*pmlmepriv = &(padapter->mlmepriv);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);	
	
	if(pmlmepriv->qospriv.qos_option==0)
	{
		pmlmeinfo->WMM_enable = 0;
		return _FAIL;
	}	
	
	if (pregpriv->wifi_spec == 1)
	{
		if (pmlmeinfo->WMM_enable == 1)
		{
			//todo: compare the parameter set count & decide wheher to update or not
			return _FAIL;
		}
		else
		{
			pmlmeinfo->WMM_enable = 1;
			_memcpy(&(pmlmeinfo->WMM_param), (pIE->data + 6), sizeof(struct WMM_para_element));
			return _TRUE;
		}
	}
	else
	{
		pmlmeinfo->WMM_enable = 0;
		return _FAIL;
	}
	
}

void WMMOnAssocRsp(_adapter *padapter)
{	
	unsigned char		ACI, ACM, AIFS, ECWMin, ECWMax;
	unsigned short	TXOP;
	unsigned int		acParm, i;
	struct registry_priv	*pregpriv = &padapter->registrypriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	if (pmlmeinfo->WMM_enable == 0)
		return;
	
	for (i = 0; i < 4; i++)  
	{
		ACI = (pmlmeinfo->WMM_param.ac_param[i].ACI_AIFSN >> 5) & 0x03;
		ACM = (pmlmeinfo->WMM_param.ac_param[i].ACI_AIFSN >> 4) & 0x01;

		//AIFS = AIFSN * slot time + SIFS - r2t phy delay
		AIFS = (pmlmeinfo->WMM_param.ac_param[i].ACI_AIFSN & 0x0f) * (pmlmeinfo->slotTime) + 10 - R2T_PHY_DELAY;
		
		ECWMin = (pmlmeinfo->WMM_param.ac_param[i].CW & 0x0f);
		ECWMax = (pmlmeinfo->WMM_param.ac_param[i].CW & 0xf0) >> 4;
		TXOP = le16_to_cpu(pmlmeinfo->WMM_param.ac_param[i].TXOP_limit);
		
		acParm = AIFS | (ECWMin << 8) | (ECWMax << 12) | (TXOP << 16);

		switch (ACI)
		{
			case 0x0:
				padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_AC_PARAM_BE, (u8 *)(&acParm));
				break;
								
			case 0x1:
				padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_AC_PARAM_BK, (u8 *)(&acParm));
				break;
								
			case 0x2:
				padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_AC_PARAM_VI, (u8 *)(&acParm));
				break;
								
			case 0x3:
				padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_AC_PARAM_VO, (u8 *)(&acParm));
				break;							
		}
		
		DBG_871X("WMM(%x): %x, %x\n", ACI, ACM, acParm);
	}
	
	return;	
}

void HT_caps_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs pIE)
{
	unsigned int	i;
	u8	rf_type;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct mlme_priv 		*pmlmepriv = &padapter->mlmepriv;	
	struct ht_priv			*phtpriv = &pmlmepriv->htpriv;

	if(phtpriv->ht_option == 0)	return;

	pmlmeinfo->HT_caps_enable = 1;
	
	for (i = 0; i < (pIE->Length); i++)
	{
		if (i != 2)
		{
			//	Commented by Albert 2010/07/12
			//	Got the endian issue here.
			pmlmeinfo->HT_caps.HT_cap[i] &= (pIE->data[i]);
		}
		else
		{
			if ((pmlmeinfo->HT_caps.HT_cap_element.AMPDU_para & 0x3) > (pIE->data[i] & 0x3))
			{
				pmlmeinfo->HT_caps.HT_cap_element.AMPDU_para &= 0x03;
				pmlmeinfo->HT_caps.HT_cap_element.AMPDU_para |= (pIE->data[i] & 0x3);
			}
					
			if ((pmlmeinfo->HT_caps.HT_cap_element.AMPDU_para & 0x1c) > (pIE->data[i] & 0x1c))
			{
				pmlmeinfo->HT_caps.HT_cap_element.AMPDU_para &= 0x1c;
				pmlmeinfo->HT_caps.HT_cap_element.AMPDU_para |= (pIE->data[i] & 0x1c);
			}
		}
	}

	//	Commented by Albert 2010/07/12
	//	Have to handle the endian issue after copying.
	//	HT_ext_caps didn't be used yet.	
	pmlmeinfo->HT_caps.HT_cap_element.HT_caps_info = le16_to_cpu( pmlmeinfo->HT_caps.HT_cap_element.HT_caps_info );
	pmlmeinfo->HT_caps.HT_cap_element.HT_ext_caps = le16_to_cpu( pmlmeinfo->HT_caps.HT_cap_element.HT_ext_caps );

	padapter->HalFunc.GetHwRegHandler(padapter, HW_VAR_RF_TYPE, (u8 *)(&rf_type));

	//update the MCS rates
	for (i = 0; i < 16; i++)
	{
		if((rf_type == RF_1T1R) || (rf_type == RF_1T2R))
		{
			pmlmeinfo->HT_caps.HT_cap_element.MCS_rate[i] &= MCS_rate_1R[i];
		}
		else
		{
			pmlmeinfo->HT_caps.HT_cap_element.MCS_rate[i] &= MCS_rate_2R[i];
		}
	}
	
	return;
}

void HT_info_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs pIE)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	struct mlme_priv 		*pmlmepriv = &padapter->mlmepriv;	
	struct ht_priv			*phtpriv = &pmlmepriv->htpriv;

	if(phtpriv->ht_option == 0)	return;


	if(pIE->Length > sizeof(struct HT_info_element))
		return;
	
	pmlmeinfo->HT_info_enable = 1;
	_memcpy(&(pmlmeinfo->HT_info), pIE->data, pIE->Length);
	
	return;
}

void HTOnAssocRsp(_adapter *padapter)
{
	unsigned char		max_AMPDU_len;
	unsigned char		min_MPDU_spacing;
	unsigned char		FactorLevel[18] = {2, 4, 4, 7, 7, 13, 13, 13, 2, 7, 7, 13, 13, 15, 15, 15, 15, 0};
	struct registry_priv	 *pregpriv = &padapter->registrypriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	
	DBG_871X("%s\n", __FUNCTION__);

	if ((pmlmeinfo->HT_info_enable) && (pmlmeinfo->HT_caps_enable))
	{
		pmlmeinfo->HT_enable = 1;
	}
	else
	{
		pmlmeinfo->HT_enable = 0;
		return;
	}
	
	//handle A-MPDU parameter field
	/* 	
		AMPDU_para [1:0]:Max AMPDU Len => 0:8k , 1:16k, 2:32k, 3:64k
		AMPDU_para [4:2]:Min MPDU Start Spacing	
	*/
	max_AMPDU_len = pmlmeinfo->HT_caps.HT_cap_element.AMPDU_para & 0x03;	
	
	min_MPDU_spacing = (pmlmeinfo->HT_caps.HT_cap_element.AMPDU_para & 0x1c) >> 2;	

	padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_AMPDU_MIN_SPACE, (u8 *)(&min_MPDU_spacing));

	padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_AMPDU_FACTOR, (u8 *)(&max_AMPDU_len));

	if ((pregpriv->cbw40_enable) &&
		(pmlmeinfo->HT_caps.HT_cap_element.HT_caps_info & BIT(1)) && 
		(pmlmeinfo->HT_info.infos[0] & BIT(2)))
	{
		//switch to the 40M Hz mode accoring to the AP
		pmlmeext->cur_bwmode = HT_CHANNEL_WIDTH_40;
		switch ((pmlmeinfo->HT_info.infos[0] & 0x3))
		{
			case 1:
				pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_LOWER;
				break;
			
			case 3:
				pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_UPPER;
				break;
				
			default:
				pmlmeext->cur_ch_offset = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
				break;
		}
		
		//SelectChannel(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset);
		set_channel_bwmode(padapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
	}

	//
	// Config SM Power Save setting
	//
	pmlmeinfo->SM_PS = (pmlmeinfo->HT_caps.HT_cap_element.HT_caps_info & 0x0C) >> 2;
	if(pmlmeinfo->SM_PS == WLAN_HT_CAP_SM_PS_STATIC)
	{
		/*u8 i;
		//update the MCS rates
		for (i = 0; i < 16; i++)
		{
			pmlmeinfo->HT_caps.HT_cap_element.MCS_rate[i] &= MCS_rate_1R[i];
		}*/
		DBG_8192C("%s(): WLAN_HT_CAP_SM_PS_STATIC\n",__FUNCTION__);
	}

	//
	// Config current HT Protection mode.
	//
	pmlmeinfo->HT_protection = pmlmeinfo->HT_info.infos[1] & 0x3;
}

void ERP_IE_handler(_adapter *padapter, PNDIS_802_11_VARIABLE_IEs pIE)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	if(pIE->Length>1)
		return;
	
	pmlmeinfo->ERP_enable = 1;
	_memcpy(&(pmlmeinfo->ERP_IE), pIE->data, pIE->Length);
}

void VCS_update(_adapter *padapter, struct sta_info *psta)
{
	struct registry_priv	 *pregpriv = &padapter->registrypriv;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	switch (pregpriv->vrtl_carrier_sense)/* 0:off 1:on 2:auto */
	{
		case 0: //off
			psta->rtsen = 0;
			psta->cts2self = 0;
			break;
			
		case 1: //on
			if (pregpriv->vcs_type == 1) /* 1:RTS/CTS 2:CTS to self */
			{
				psta->rtsen = 1;
				psta->cts2self = 0;
			}
			else
			{
				psta->rtsen = 0;
				psta->cts2self = 1;
			}
			break;
			
		case 2: //auto
		default:
			if ((pmlmeinfo->ERP_enable) && (pmlmeinfo->ERP_IE & BIT(1)))
			{
				if (pregpriv->vcs_type == 1)
				{
					psta->rtsen = 1;
					psta->cts2self = 0;
				}
				else
				{
					psta->rtsen = 0;
					psta->cts2self = 1;
				}
			}
			else
			{
				psta->rtsen = 0;
				psta->cts2self = 0;
			}	
			break;
	}
}

void update_beacon_info(_adapter *padapter, u8 *pframe, uint pkt_len, struct sta_info *psta)
{
	unsigned int i;
	unsigned int len;
	PNDIS_802_11_VARIABLE_IEs	pIE;
		
	len = pkt_len - (_BEACON_IE_OFFSET_ + WLAN_HDR_A3_LEN);
	
	for (i = 0; i < len;)
	{
		pIE = (PNDIS_802_11_VARIABLE_IEs)(pframe + (_BEACON_IE_OFFSET_ + WLAN_HDR_A3_LEN) + i);
		
		switch (pIE->ElementID)
		{
#if 0			
			case _VENDOR_SPECIFIC_IE_:		
				//todo: to update WMM paramter set while receiving beacon			
				if (_memcmp(pIE->data, WMM_PARA_OUI, 6))	//WMM
				{
					(WMM_param_handler(padapter, pIE))? WMMOnAssocRsp(padapter): 0;
				}				
				break;
#endif								
			case _ERPINFO_IE_:
				ERP_IE_handler(padapter, pIE);
				VCS_update(padapter, psta);
				break;
				
			default:
				break;
		}
		
		i += (pIE->Length + 2);
	}
}

unsigned int is_ap_in_tkip(_adapter *padapter)
{
	u32 i;
	PNDIS_802_11_VARIABLE_IEs	pIE;
	struct mlme_ext_priv *pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);

	if (get_capability((WLAN_BSSID_EX *)cur_network) & WLAN_CAPABILITY_PRIVACY)
	{
		for (i = sizeof(NDIS_802_11_FIXED_IEs); i < pmlmeinfo->network.IELength;)
		{
			pIE = (PNDIS_802_11_VARIABLE_IEs)(pmlmeinfo->network.IEs + i);
		
			switch (pIE->ElementID)
			{
				case _VENDOR_SPECIFIC_IE_:
					if ((_memcmp(pIE->data, WPA_OUI, 4)) && (_memcmp((pIE->data + 12), WPA_TKIP_CIPHER, 4))) 
					{
						return _TRUE;
					}
					break;
				
				case _RSN_IE_2_:
					if (_memcmp((pIE->data + 8), RSN_TKIP_CIPHER, 4)) 
					{
						return _TRUE;
					}
					
				default:
					break;
			}
		
			i += (pIE->Length + 2);
		}
		
		return _FALSE;
	}
	else
	{
		return _FALSE;
	}
	
}

int wifirate2_ratetbl_inx(unsigned char rate)
{
	int	inx = 0;
	rate = rate & 0x7f;

	switch (rate) 
	{
		case 54*2:
			inx = 11;
			break;

		case 48*2:
			inx = 10;
			break;

		case 36*2:
			inx = 9;
			break;

		case 24*2:
			inx = 8;
			break;
			
		case 18*2:
			inx = 7;
			break;

		case 12*2:
			inx = 6;
			break;

		case 9*2:
			inx = 5;
			break;
			
		case 6*2:
			inx = 4;
			break;

		case 11*2:
			inx = 3;
			break;
		case 11:
			inx = 2;
			break;

		case 2*2:
			inx = 1;
			break;
		
		case 1*2:
			inx = 0;
			break;

	}
	return inx;	
}

unsigned int update_basic_rate(unsigned char *ptn, unsigned int ptn_sz)
{
	unsigned int i, num_of_rate;
	unsigned int mask = 0;
	
	num_of_rate = (ptn_sz > NumRates)? NumRates: ptn_sz;
		
	for (i = 0; i < num_of_rate; i++)
	{
		if ((*(ptn + i)) & 0x80)
		{
			mask |= 0x1 << wifirate2_ratetbl_inx(*(ptn + i));
		}
	}
	return mask;
}

unsigned int update_supported_rate(unsigned char *ptn, unsigned int ptn_sz)
{
	unsigned int i, num_of_rate;
	unsigned int mask = 0;
	
	num_of_rate = (ptn_sz > NumRates)? NumRates: ptn_sz;
		
	for (i = 0; i < num_of_rate; i++)
	{
		mask |= 0x1 << wifirate2_ratetbl_inx(*(ptn + i));
	}

	return mask;
}

unsigned int update_MSC_rate(struct HT_caps_element *pHT_caps)
{
	unsigned int mask = 0;
	
	mask = ((pHT_caps->HT_cap_element.MCS_rate[0] << 12) | (pHT_caps->HT_cap_element.MCS_rate[1] << 20));
						
	return mask;
}

int support_short_GI(_adapter *padapter, struct HT_caps_element *pHT_caps)
{
	unsigned char					bit_offset;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	if (!(pmlmeinfo->HT_enable))
		return _FAIL;
	
	if ((pmlmeinfo->assoc_AP_vendor == ralinkAP))
		return _FAIL;
		
	bit_offset = (pmlmeext->cur_bwmode & HT_CHANNEL_WIDTH_40)? 6: 5;
	
	if (pHT_caps->HT_cap_element.HT_caps_info & (0x1 << bit_offset))
	{
		return _SUCCESS;
	}
	else
	{
		return _FAIL;
	}		
}

unsigned char get_highest_rate_idx(u32 mask)
{
	int i;
	unsigned char rate_idx=0;

	for(i=27; i>=0; i--)
	{
		if(mask & BIT(i))
		{
			rate_idx = i;
			break;
		}
	}

	return rate_idx;
}

unsigned char get_highest_mcs_rate(struct HT_caps_element *pHT_caps)
{
	int i, mcs_rate;
	
	mcs_rate = (pHT_caps->HT_cap_element.MCS_rate[0] | (pHT_caps->HT_cap_element.MCS_rate[1] << 8));
	
	for (i = 15; i >= 0; i--)
	{
		if (mcs_rate & (0x1 << i))
		{
			break;
		}
	}
	
	return i;
}

void Update_RA_Entry(_adapter *padapter, unsigned int cam_idx)
{
	padapter->HalFunc.UpdateRAMaskHandler(padapter, cam_idx);
}

void enable_rate_adaptive(_adapter *padapter)
{
	Update_RA_Entry(padapter, 4);
	//Update_RA_Entry(padapter, 5);
	Update_RA_Entry(padapter, 0);
	
	return;
}

void set_sta_rate(_adapter *padapter)
{
	//rate adaptive	
	enable_rate_adaptive(padapter);
}

unsigned char check_assoc_AP(u8 *pframe, uint len)
{
	unsigned int	i;
	PNDIS_802_11_VARIABLE_IEs	pIE;

	for (i = sizeof(NDIS_802_11_FIXED_IEs); i < len;)
	{
		pIE = (PNDIS_802_11_VARIABLE_IEs)(pframe + i);
		
		switch (pIE->ElementID)
		{
			case _VENDOR_SPECIFIC_IE_:
				if ((_memcmp(pIE->data, ARTHEROS_OUI1, 3)) || (_memcmp(pIE->data, ARTHEROS_OUI2, 3)))
				{
					DBG_871X("link to Artheros AP\n");
					return atherosAP;
				}
				else if ((_memcmp(pIE->data, BROADCOM_OUI1, 3))
							|| (_memcmp(pIE->data, BROADCOM_OUI2, 3))
							|| (_memcmp(pIE->data, BROADCOM_OUI2, 3)))
				{
					DBG_871X("link to Broadcom AP\n");
					return broadcomAP;
				}
				else if (_memcmp(pIE->data, MARVELL_OUI, 3))
				{
					DBG_871X("link to Marvell AP\n");
					return marvellAP;
				}
				else if (_memcmp(pIE->data, RALINK_OUI, 3))
				{
					DBG_871X("link to Ralink AP\n");
					return ralinkAP;
				}
				else if (_memcmp(pIE->data, CISCO_OUI, 3))
				{
					DBG_871X("link to Cisco AP\n");
					return ciscoAP;
				}
				else if (_memcmp(pIE->data, REALTEK_OUI, 3))
				{
					DBG_871X("link to Realtek 96B\n");
					return realtekAP;
				}
				else if (_memcmp(pIE->data, AIRGOCAP_OUI,3))
				{
					DBG_871X("link to Airgo Cap\n");
					return airgocapAP;
				}
				else
				{
					break;
				}
						
			default:
				break;
		}
				
		i += (pIE->Length + 2);
	}
	
	DBG_871X("link to new AP\n");
	return unknownAP;
}

void update_IOT_info(_adapter *padapter)
{
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	switch (pmlmeinfo->assoc_AP_vendor)
	{
		case marvellAP:
			pmlmeinfo->turboMode_cts2self = 1;
			pmlmeinfo->turboMode_rtsen = 0;
			break;
		
		case ralinkAP:
			pmlmeinfo->turboMode_cts2self = 0;
			pmlmeinfo->turboMode_rtsen = 1;
			//disable high power			
			Switch_DM_Func(padapter, (~DYNAMIC_FUNC_HP), _FALSE);
			break;
		case realtekAP:
			write16(padapter, 0x4cc, 0xffff);
			write16(padapter, 0x546, 0x01c0);			
			break;
		default:
			pmlmeinfo->turboMode_cts2self = 0;
			pmlmeinfo->turboMode_rtsen = 1;
			break;	
	}
	
}


void fire_write_MAC_cmd(_adapter *padapter, unsigned int addr, unsigned int value)
{
#if 0
	struct cmd_obj					*ph2c;
	struct reg_rw_parm			*pwriteMacPara;
	struct cmd_priv					*pcmdpriv = &(padapter->cmdpriv);

	if ((ph2c = (struct cmd_obj*)_malloc(sizeof(struct cmd_obj))) == NULL)
	{
		return;
	}	

	if ((pwriteMacPara = (struct reg_rw_parm*)_malloc(sizeof(struct reg_rw_parm))) == NULL) 
	{		
		_mfree((unsigned char *)ph2c, sizeof(struct cmd_obj));
		return;
	}
	
	pwriteMacPara->rw = 1;
	pwriteMacPara->addr = addr;
	pwriteMacPara->value = value;
	
	init_h2fwcmd_w_parm_no_rsp(ph2c, pwriteMacPara, GEN_CMD_CODE(_Write_MACREG));
	enqueue_cmd(pcmdpriv, ph2c);
#endif	
}

int update_sta_support_rate(_adapter *padapter, u8* pvar_ie, uint var_ie_len, int cam_idx)
{
	unsigned int	ie_len;
	PNDIS_802_11_VARIABLE_IEs	pIE;
	int	supportRateNum = 0;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
	pIE = (PNDIS_802_11_VARIABLE_IEs)get_ie(pvar_ie, _SUPPORTEDRATES_IE_, &ie_len, var_ie_len);
	if (pIE == NULL)
	{
		return _FAIL;
	}
	
	_memcpy(pmlmeinfo->FW_sta_info[cam_idx].SupportedRates, pIE->data, ie_len);
	supportRateNum = ie_len;
				
	pIE = (PNDIS_802_11_VARIABLE_IEs)get_ie(pvar_ie, _EXT_SUPPORTEDRATES_IE_, &ie_len, var_ie_len);
	if (pIE)
	{
		_memcpy((pmlmeinfo->FW_sta_info[cam_idx].SupportedRates + supportRateNum), pIE->data, ie_len);
	}

	return _SUCCESS;
	
}

void process_addba_req(_adapter *padapter, u8 *paddba_req, u8 *addr)
{
	struct sta_info *psta;
	u16 tid, start_seq, param;	
	struct recv_reorder_ctrl *preorder_ctrl;
	struct sta_priv *pstapriv = &padapter->stapriv;	
	struct ADDBA_request	*preq = (struct ADDBA_request*)paddba_req;
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	psta = get_stainfo(pstapriv, addr);

	if(psta)
	{
		start_seq = le16_to_cpu(preq->BA_starting_seqctrl) >> 4;
			
		param = le16_to_cpu(preq->BA_para_set);
		tid = (param>>2)&0x0f;
		
		preorder_ctrl = &psta->recvreorder_ctrl[tid];

		preorder_ctrl->indicate_seq = start_seq;
		
		preorder_ctrl->enable =(pmlmeinfo->bAcceptAddbaReq == _TRUE)? _TRUE :_FALSE;
	}

}

void update_TSF(struct mlme_ext_priv *pmlmeext, u8 *pframe, uint len)
{	
	u8* pIE;
	u32 *pbuf;
		
	pIE = pframe + sizeof(struct ieee80211_hdr_3addr);
	pbuf = (u32*)pIE;

	pmlmeext->TSFValue = le32_to_cpu(*(pbuf+1));
	
	pmlmeext->TSFValue = pmlmeext->TSFValue << 32;

	pmlmeext->TSFValue |= le32_to_cpu(*pbuf);
}

void correct_TSF(_adapter *padapter, struct mlme_ext_priv *pmlmeext)
{
	padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_CORRECT_TSF, 0);
}

void beacon_timing_control(_adapter *padapter)
{
	padapter->HalFunc.SetBeaconRelatedRegistersHandler(padapter);
}

#if 0
unsigned int setup_beacon_frame(_adapter *padapter, unsigned char *beacon_frame)
{
	unsigned short				ATIMWindow;
	unsigned char					*pframe;
	struct tx_desc 				*ptxdesc;
	struct ieee80211_hdr 	*pwlanhdr;
	unsigned short				*fctrl;
	unsigned int					rate_len, len = 0;
	struct xmit_priv			*pxmitpriv = &(padapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	u8	bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	
	_memset(beacon_frame, 0, 256);
	
	pframe = beacon_frame + TXDESC_SIZE;
	
	pwlanhdr = (struct ieee80211_hdr *)pframe;	
	
	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	
	_memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
	_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_memcpy(pwlanhdr->addr3, get_my_bssid(cur_network), ETH_ALEN);
	
	SetFrameSubType(pframe, WIFI_BEACON);
	
	pframe += sizeof(struct ieee80211_hdr_3addr);	
	len = sizeof(struct ieee80211_hdr_3addr);

	//timestamp will be inserted by hardware
	pframe += 8;
	len += 8;

	// beacon interval: 2 bytes
	_memcpy(pframe, (unsigned char *)(get_beacon_interval_from_ie(cur_network->IEs)), 2); 
	pframe += 2;
	len += 2;

	// capability info: 2 bytes
	_memcpy(pframe, (unsigned char *)(get_capability_from_ie(cur_network->IEs)), 2);
	pframe += 2;
	len += 2;

	// SSID
	pframe = set_ie(pframe, _SSID_IE_, cur_network->Ssid.SsidLength, cur_network->Ssid.Ssid, &len);

	// supported rates...
	rate_len = get_rateset_len(cur_network->SupportedRates);
	pframe = set_ie(pframe, _SUPPORTEDRATES_IE_, ((rate_len > 8)? 8: rate_len), cur_network->SupportedRates, &len);

	// DS parameter set
	pframe = set_ie(pframe, _DSSET_IE_, 1, (unsigned char *)&(cur_network->Configuration.DSConfig), &len);

	// IBSS Parameter Set...
	//ATIMWindow = cur->Configuration.ATIMWindow;
	ATIMWindow = 0;
	pframe = set_ie(pframe, _IBSS_PARA_IE_, 2, (unsigned char *)(&ATIMWindow), &len);

	//todo: ERP IE
	
	// EXTERNDED SUPPORTED RATE
	if (rate_len > 8)
	{
		pframe = set_ie(pframe, _EXT_SUPPORTEDRATES_IE_, (rate_len - 8), (cur_network->SupportedRates + 8), &len);
	}

	if ((len + TXDESC_SIZE) > 256)
	{
		//printk("marc: beacon frame too large\n");
		return 0;
	}

	//fill the tx descriptor
	ptxdesc = (struct tx_desc *)beacon_frame;
	
	//offset 0	
	ptxdesc->txdw0 |= cpu_to_le32(len & 0x0000ffff); 
	ptxdesc->txdw0 |= cpu_to_le32(((TXDESC_SIZE + OFFSET_SZ) << OFFSET_SHT) & 0x00ff0000); //default = 32 bytes for TX Desc
	
	//offset 4	
	ptxdesc->txdw1 |= cpu_to_le32((0x10 << QSEL_SHT) & 0x00001f00);
	
	//offset 8		
	ptxdesc->txdw2 |= cpu_to_le32(BMC);
	ptxdesc->txdw2 |= cpu_to_le32(BK);

	//offset 16		
	ptxdesc->txdw4 = 0x80000000;
	
	//offset 20
	ptxdesc->txdw5 = 0x00000000; //1M	
	
	return (len + TXDESC_SIZE);
}
#endif

