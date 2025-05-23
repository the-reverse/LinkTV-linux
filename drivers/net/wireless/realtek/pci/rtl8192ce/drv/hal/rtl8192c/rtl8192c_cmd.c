/******************************************************************************
* rtl8192c_cmd.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2010, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _RTL8192C_CMD_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <cmd_osdep.h>
#include <mlme_osdep.h>
#include <rtw_byteorder.h>
#include <circ_buf.h>
#include <rtw_ioctl_set.h>

#include <rtl8192c_hal.h>


#if 0
static BOOLEAN
CheckWriteMSG(
	IN	PADAPTER		Adapter,
	IN	u8		BoxNum
)
{
	u8	valHMETFR;
	BOOLEAN	Result = _FALSE;
	
	valHMETFR = read8(Adapter, REG_HMETFR);

	//DbgPrint("CheckWriteH2C(): Reg[0x%2x] = %x\n",REG_HMETFR, valHMETFR);

	if(((valHMETFR>>BoxNum)&BIT0) == 1)
		Result = _TRUE;
	
	return Result;

}

static BOOLEAN CheckFwReadLastMSG(
	IN	PADAPTER		Adapter,
	IN	u8		BoxNum
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	valHMETFR, valMCUTST_1;
	BOOLEAN	 Result = _FALSE;
	
	valHMETFR = read8(Adapter, REG_HMETFR);
	valMCUTST_1 = read8(Adapter, (REG_MCUTST_1+BoxNum));

	//DbgPrint("REG[%x] = %x, REG[%x] = %x\n", 
	//	REG_HMETFR, valHMETFR, REG_MCUTST_1+BoxNum, valMCUTST_1 );

	// Do not seperate to 91C and 88C, we use the same setting. Suggested by SD4 Filen. 2009.12.03.
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		if(((valHMETFR>>BoxNum)&BIT0) == 0)
			Result = _TRUE;
	}
	else
	{
		if((((valHMETFR>>BoxNum)&BIT0) == 0) && (valMCUTST_1 == 0))
		{
			Result = _TRUE;
		}
	}
	
	return Result;
}
#endif


#define RTL92C_MAX_H2C_BOX_NUMS	4
#define RTL92C_MAX_CMD_LEN	5
#define MESSAGE_BOX_SIZE		4
#define EX_MESSAGE_BOX_SIZE	2


static u8 _is_fw_read_cmd_down(_adapter* padapter, u8 isvern, u8 msgbox_num)
{
	u8	read_down = _FALSE;
	int 	retry_cnts = 100;
	
	u8 valid;

//	DBG_8192C(" _is_fw_read_cmd_down ,isnormal_chip(%x),reg_1cc(%x),msg_box(%d)...\n",isvern,read8(padapter,REG_HMETFR),msgbox_num);
	
	do{
		valid = read8(padapter,REG_HMETFR) & BIT(msgbox_num);	
		if(isvern){
			if(0 == valid ){
				read_down = _TRUE;
			}			
		}
		else{
			if((0 == valid) && (0 == read8(padapter, REG_MCUTST_1+msgbox_num))){
				read_down = _TRUE;	
			}
		}
	}while( (!read_down) && (retry_cnts--));

	return read_down;
	
}


/*****************************************
* H2C Msg format :
*| 31 - 8		|7		| 6 - 0	|	
*| h2c_msg 	|Ext_bit	|CMD_ID	|
*
******************************************/
void FillH2CCmd(_adapter* padapter, u8 ElementID, u32 CmdLen, u8* pCmdBuffer)
{
#if 1
	u8	bcmd_down = _FALSE;
	int 	retry_cnts = 100;
	u8	h2c_box_num;
	u32	msgbox_addr;
	u32  msgbox_ex_addr;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8	isnchip =IS_NORMAL_CHIP(pHalData->VersionID);
	u32	h2c_cmd = 0;
	u16	h2c_cmd_ex = 0;

	_func_enter_;	

	if(!pCmdBuffer){
		return ;
	}
	if(CmdLen > RTL92C_MAX_CMD_LEN){
		return ;
	}
	//pay attention to if  race condition happened in  H2C cmd setting.
	do{
		h2c_box_num = pHalData->LastHMEBoxNum;

		if(!_is_fw_read_cmd_down(padapter, isnchip, h2c_box_num)){
			DBG_8192C(" fw read cmd failed...\n");
			break;
		}

		if(CmdLen<=3)
		{
			_memcpy((u8*)(&h2c_cmd)+1, pCmdBuffer, CmdLen );
		}
		else{
			_memcpy((u8*)(&h2c_cmd_ex), pCmdBuffer, EX_MESSAGE_BOX_SIZE);
			_memcpy((u8*)(&h2c_cmd)+1, pCmdBuffer+2,( CmdLen-EX_MESSAGE_BOX_SIZE));
			*(u8*)(&h2c_cmd) |= BIT(7);
		}

		*(u8*)(&h2c_cmd) |= ElementID;

		if(h2c_cmd & BIT(7)){
			msgbox_ex_addr = REG_HMEBOX_EXT_0 + (h2c_box_num *EX_MESSAGE_BOX_SIZE);
			h2c_cmd_ex = cpu_to_le16( h2c_cmd_ex );
			write16(padapter, msgbox_ex_addr, h2c_cmd_ex);
		}
		msgbox_addr =REG_HMEBOX_0 + (h2c_box_num *MESSAGE_BOX_SIZE);
		h2c_cmd = cpu_to_le32( h2c_cmd );
		write32(padapter,msgbox_addr, h2c_cmd);

		if(!isnchip){//for Test chip
			if(! (read8(padapter, REG_HMETFR) & BIT(h2c_box_num))){
				DBG_8192C("Chip test  - check fw write failed, write again..\n");
				continue;
			}
			// Fill H2C protection register.
			write8(padapter,REG_MCUTST_1+h2c_box_num, 0xFF);
		}
		bcmd_down = _TRUE;

	//	DBG_8192C("MSG_BOX:%d,CmdLen(%d), reg:0x%x =>h2c_cmd:0x%x, reg:0x%x =>h2c_cmd_ex:0x%x ..\n"
	//	 	,pHalData->LastHMEBoxNum ,CmdLen,msgbox_addr,h2c_cmd,msgbox_ex_addr,h2c_cmd_ex);

		 pHalData->LastHMEBoxNum = (h2c_box_num+1) % RTL92C_MAX_H2C_BOX_NUMS ;

	}while((!bcmd_down) && (retry_cnts--));
/*
	if(bcmd_down)
		DBG_8192C("H2C Cmd exe down. \n"	);	
	else
		DBG_8192C("H2C Cmd exe failed. \n"	);
*/
	_func_exit_;

#else
	u8	BoxNum;
	u16	BOXReg, BOXExtReg;
	u8	BoxContent[4], BoxExtContent[2];
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	u8 	BufIndex=0;
	u8	bWriteSucess = _FALSE;
	u8	IsFwRead = _FALSE;
	u8	WaitH2cLimmit = 100;

	u32	h2c_cmd = 0;
	u16	h2c_cmd_ex = 0;

_func_enter_;	

	//printk("FillH2CCmd : ElementID=%d \n",ElementID);

	while(!bWriteSucess)
	{
		// 2. Find the last BOX number which has been writen.
		BoxNum = pHalData->LastHMEBoxNum;
		switch(BoxNum)
		{
			case 0:
				BOXReg = REG_HMEBOX_0;
				BOXExtReg = REG_HMEBOX_EXT_0;
				break;
			case 1:
				BOXReg = REG_HMEBOX_1;
				BOXExtReg = REG_HMEBOX_EXT_1;
				break;
			case 2:
				BOXReg = REG_HMEBOX_2;
				BOXExtReg = REG_HMEBOX_EXT_2;
				break;
			case 3:
				BOXReg = REG_HMEBOX_3;
				BOXExtReg = REG_HMEBOX_EXT_3;
				break;
			default:
				break;
		}

		// 3. Check if the box content is empty.
		IsFwRead = CheckFwReadLastMSG(padapter, BoxNum);
		while(!IsFwRead)
		{
			//wait until Fw read
			WaitH2cLimmit--;
			if(WaitH2cLimmit == 0)
			{
				DBG_8192C("FillH2CCmd92C(): Wating too long for FW read clear HMEBox(%d)!!!\n", BoxNum);
				break;
			}
			msleep_os(10); //us
			IsFwRead = CheckFwReadLastMSG(padapter, BoxNum);
			//U1btmp = PlatformEFIORead1Byte(Adapter, 0x1BF);
			//RT_TRACE(COMP_CMD, DBG_LOUD, ("FillH2CCmd92C(): Wating for FW read clear HMEBox(%d)!!! 0x1BF = %2x\n", BoxNum, U1btmp));
		}

		// If Fw has not read the last H2C cmd, break and give up this H2C.
		if(!IsFwRead)
		{
			DBG_8192C("FillH2CCmd92C():  Write H2C register BOX[%d] fail!!!!! Fw do not read. \n", BoxNum);
			break;
		}

		// 4. Fill the H2C cmd into box		
		_memset(BoxContent, 0, sizeof(BoxContent));
		_memset(BoxExtContent, 0, sizeof(BoxExtContent));
		
		BoxContent[0] = ElementID; // Fill element ID

		//DBG_8192C("FillH2CCmd92C():Write ElementID BOXReg(%4x) = %2x \n", BOXReg, ElementID);

		switch(CmdLen)
		{
			case 1:
				{
					BoxContent[0] &= ~(BIT7);
					_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 1);
					write32(padapter, BOXReg, *((u32*)BoxContent));
					h2c_cmd =  *((u32*)BoxContent);
					break;
				}
			case 2:
				{	
					BoxContent[0] &= ~(BIT7);
					_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 2);
					write32(padapter, BOXReg, *((u32*)BoxContent));
					h2c_cmd =  *((u32*)BoxContent);
					break;
				}
			case 3:
				{
					BoxContent[0] &= ~(BIT7);
					_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex, 3);
					write32(padapter, BOXReg, *((u32*)BoxContent));
					h2c_cmd =  *((u32*)BoxContent);
					break;
				}
			case 4:
				{
					BoxContent[0] |= (BIT7);
					_memcpy((u8*)(BoxExtContent), pCmdBuffer+BufIndex, 2);
					_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex+2, 2);
					write16(padapter, BOXExtReg, *((u16*)BoxExtContent));
					write32(padapter, BOXReg, *((u32*)BoxContent));
					h2c_cmd =  *((u32*)BoxContent);
					h2c_cmd_ex = *((u32*)BoxExtContent);
					break;
				}
			case 5:
				{
					BoxContent[0] |= (BIT7);
					_memcpy((u8*)(BoxExtContent), pCmdBuffer+BufIndex, 2);
					_memcpy((u8*)(BoxContent)+1, pCmdBuffer+BufIndex+2, 3);
					write16(padapter, BOXExtReg, *((u16*)BoxExtContent));
					write32(padapter, BOXReg, *((u32*)BoxContent));
					h2c_cmd =  *((u32*)BoxContent);
					h2c_cmd_ex = *((u32*)BoxExtContent);
					break;
				}
			default:
					break;
					
		}
			

		DBG_8192C("MSG_BOX:%d,CmdLen(%d), reg:0x%x =>h2c_cmd:0x%x, reg:0x%x =>h2c_cmd_ex:0x%x ..\n"
		 	,pHalData->LastHMEBoxNum ,CmdLen,BOXReg,h2c_cmd,BOXExtReg,h2c_cmd_ex);

		//DBG_8192C("FillH2CCmd(): BoxExtContent=0x%x\n", *(u16*)BoxExtContent);		
		//DBG_8192C("FillH2CCmd(): BoxContent=0x%x\n", *(u32*)BoxContent);
			
		if(IS_NORMAL_CHIP(pHalData->VersionID))
		{
			// 5. Normal chip does not need to check if the H2C cmd has be written successfully.
			bWriteSucess = _TRUE;
		}
		else
		{	
			// 5. Check if the H2C cmd has be written successfully.
			bWriteSucess = CheckWriteMSG(padapter, BoxNum);
			if(!bWriteSucess) //If not then write again.
				continue;
			
			//6. Fill H2C protection register.

			write8(padapter, REG_MCUTST_1+BoxNum, 0xFF);
			//RT_TRACE(COMP_CMD, DBG_LOUD, ("FillH2CCmd92C():Write Reg(%4x) = 0xFF \n", REG_MCUTST_1+BoxNum));
		}

		// Record the next BoxNum
		pHalData->LastHMEBoxNum = BoxNum+1;
		if(pHalData->LastHMEBoxNum == 4) // loop to 0
			pHalData->LastHMEBoxNum = 0;
		
		//DBG_8192C("FillH2CCmd92C():pHalData->LastHMEBoxNum  = %d\n", pHalData->LastHMEBoxNum);
		
	}

_func_exit_;

#endif

}

u8 rtl8192c_h2c_msg_hdl(_adapter *padapter, unsigned char *pbuf)
{	
	u8 ElementID, CmdLen;
	u8 *pCmdBuffer;
	struct cmd_msg_parm  *pcmdmsg;
	
	if(!pbuf)
		return H2C_PARAMETERS_ERROR;

	pcmdmsg = (struct cmd_msg_parm*)pbuf;
	ElementID = pcmdmsg->eid;
	CmdLen = pcmdmsg->sz;
	pCmdBuffer = pcmdmsg->buf;

	FillH2CCmd(padapter, ElementID, CmdLen, pCmdBuffer);

	return H2C_SUCCESS;
}

u8 rtl8192c_set_rssi_cmd(_adapter*padapter, u8 *param)
{	
	u8	res=_SUCCESS;

_func_enter_;

	*((u32*) param ) = cpu_to_le32( *((u32*) param ) );

	FillH2CCmd(padapter, RSSI_SETTING_EID, 3, param);

_func_exit_;

	return res;
}

u8 rtl8192c_set_raid_cmd(_adapter*padapter, u32 mask, u8 arg)
{	
	u8	buf[5];
	u8	res=_SUCCESS;
	
_func_enter_;	
	
	_memset(buf, 0, 5);
	mask = cpu_to_le32( mask );
	_memcpy(buf, &mask, 4);
	buf[4]  = arg;

	FillH2CCmd(padapter, MACID_CONFIG_EID, 5, buf);
	
_func_exit_;

	return res;

}

//bitmap[0:27] = tx_rate_bitmap
//bitmap[28:31]= Rate Adaptive id
//arg[0:4] = macid
//arg[5] = Short GI
void rtl8192c_Add_RateATid(PADAPTER pAdapter, u32 bitmap, u8 arg)
{	
	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
		
	if(pHalData->fw_ractrl == _TRUE)
	{
		rtl8192c_set_raid_cmd(pAdapter, bitmap, arg);
	}
	else
	{
		u8 macid, init_rate, shortGIrate=_FALSE;

		init_rate = get_highest_rate_idx(bitmap&0x0fffffff)&0x3f;
		
		macid = arg&0x1f;
		
		shortGIrate = (arg&BIT(5)) ? _TRUE:_FALSE;
		
		if (shortGIrate==_TRUE)
			init_rate |= BIT(6);

		write8(pAdapter, (REG_INIDATA_RATE_SEL+macid), (u8)init_rate);		
	}

}

void rtl8192c_set_FwPwrMode_cmd(_adapter*padapter, u8 Mode)
{
	SETPWRMODE_PARM H2CSetPwrMode;
	
_func_enter_;

	DBG_871X("%s(): Mode = %d\n", __FUNCTION__,Mode);

	H2CSetPwrMode.Mode = Mode;
	H2CSetPwrMode.SmartPS = 1;
	H2CSetPwrMode.BcnPassTime = 1;//pPSC->RegMaxLPSAwakeIntvl;

	FillH2CCmd(padapter, SET_PWRMODE_EID, sizeof(H2CSetPwrMode), (u8 *)&H2CSetPwrMode);
	
_func_exit_;
}

void ConstructBeacon(_adapter *padapter, u8 *pframe, u32 *pLength)
{
	struct ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	u32					rate_len, pktlen;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	u8	bc_addr[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};


	DBG_871X("%s\n", __FUNCTION__);

	pwlanhdr = (struct ieee80211_hdr *)pframe;	

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	
	_memcpy(pwlanhdr->addr1, bc_addr, ETH_ALEN);
	_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
	_memcpy(pwlanhdr->addr3, get_my_bssid(cur_network), ETH_ALEN);

	SetSeqNum(pwlanhdr, 0/*pmlmeext->mgnt_seq*/);
	//pmlmeext->mgnt_seq++;
	SetFrameSubType(pframe, WIFI_BEACON);
	
	pframe += sizeof(struct ieee80211_hdr_3addr);	
	pktlen = sizeof (struct ieee80211_hdr_3addr);
	
	//timestamp will be inserted by hardware
	pframe += 8;
	pktlen += 8;

	// beacon interval: 2 bytes
	_memcpy(pframe, (unsigned char *)(get_beacon_interval_from_ie(cur_network->IEs)), 2); 
	pframe += 2;
	pktlen += 2;

	// capability info: 2 bytes
	_memcpy(pframe, (unsigned char *)(get_capability_from_ie(cur_network->IEs)), 2);
	pframe += 2;
	pktlen += 2;

	if( (pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
	{
		DBG_871X("ie len=%d\n", cur_network->IELength);
		pktlen += cur_network->IELength - sizeof(NDIS_802_11_FIXED_IEs);
		_memcpy(pframe, cur_network->IEs+sizeof(NDIS_802_11_FIXED_IEs), pktlen);
		
		goto _ConstructBeacon;
	}

	//below for ad-hoc mode

	// SSID
	pframe = set_ie(pframe, _SSID_IE_, cur_network->Ssid.SsidLength, cur_network->Ssid.Ssid, &pktlen);

	// supported rates...
	rate_len = get_rateset_len(cur_network->SupportedRates);
	pframe = set_ie(pframe, _SUPPORTEDRATES_IE_, ((rate_len > 8)? 8: rate_len), cur_network->SupportedRates, &pktlen);

	// DS parameter set
	pframe = set_ie(pframe, _DSSET_IE_, 1, (unsigned char *)&(cur_network->Configuration.DSConfig), &pktlen);

	if( (pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE)
	{
		u32 ATIMWindow;
		// IBSS Parameter Set...
		//ATIMWindow = cur->Configuration.ATIMWindow;
		ATIMWindow = 0;
		pframe = set_ie(pframe, _IBSS_PARA_IE_, 2, (unsigned char *)(&ATIMWindow), &pktlen);
	}	


	//todo: ERP IE
	
	
	// EXTERNDED SUPPORTED RATE
	if (rate_len > 8)
	{
		pframe = set_ie(pframe, _EXT_SUPPORTEDRATES_IE_, (rate_len - 8), (cur_network->SupportedRates + 8), &pktlen);
	}


	//todo:HT for adhoc

_ConstructBeacon:

	if ((pktlen + TXDESC_SIZE) > 512)
	{
		DBG_871X("beacon frame too large\n");
		return;
	}

	*pLength = pktlen;

	DBG_871X("%s bcn_sz=%d\n", __FUNCTION__, pktlen);

}

void ConstructPSPoll(_adapter *padapter, u8 *pframe, u32 *pLength)
{
	struct ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	u32					pktlen;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	DBG_871X("%s\n", __FUNCTION__);

	pwlanhdr = (struct ieee80211_hdr *)pframe;

	// Frame control.
	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	SetPwrMgt(fctrl);
	SetFrameSubType(pframe, WIFI_PSPOLL);

	// AID.
	SetDuration(pframe, (pmlmeinfo->aid | 0xc000));

	// BSSID.
	_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);

	// TA.
	_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);

	*pLength = 16;
}

void ConstructNullFunctionData(_adapter *padapter, u8 *pframe, u32 *pLength, u8 *StaAddr, BOOLEAN bForcePowerSave)
{
	struct ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;
	u32					pktlen;
	struct mlme_priv		*pmlmepriv = &padapter->mlmepriv;
	struct wlan_network	*cur_network = &pmlmepriv->cur_network;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	DBG_871X("%s:%d\n", __FUNCTION__, bForcePowerSave);

	pwlanhdr = (struct ieee80211_hdr *)pframe;

	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	if (bForcePowerSave)
	{
		SetPwrMgt(fctrl);
	}

	switch(cur_network->network.InfrastructureMode)
	{			
		case Ndis802_11Infrastructure:
			SetToDs(fctrl);
			_memcpy(pwlanhdr->addr1, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
			_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
			_memcpy(pwlanhdr->addr3, StaAddr, ETH_ALEN);
			break;
		case Ndis802_11APMode:
			SetFrDs(fctrl);
			_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
			_memcpy(pwlanhdr->addr2, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
			_memcpy(pwlanhdr->addr3, myid(&(padapter->eeprompriv)), ETH_ALEN);
			break;
		case Ndis802_11IBSS:
		default:
			_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
			_memcpy(pwlanhdr->addr2, myid(&(padapter->eeprompriv)), ETH_ALEN);
			_memcpy(pwlanhdr->addr3, get_my_bssid(&(pmlmeinfo->network)), ETH_ALEN);
			break;
	}

	SetSeqNum(pwlanhdr, 0);

	SetFrameSubType(pframe, WIFI_DATA_NULL);

	pframe += sizeof(struct ieee80211_hdr_3addr);
	pktlen = sizeof(struct ieee80211_hdr_3addr);

	*pLength = pktlen;
}

void ConstructProbeRsp(_adapter *padapter, u8 *pframe, u32 *pLength, u8 *StaAddr, BOOLEAN bHideSSID)
{
	struct ieee80211_hdr	*pwlanhdr;
	u16					*fctrl;	
	u8					*mac, *bssid;
	u32					pktlen;
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
	
	
	DBG_871X("%s\n", __FUNCTION__);
	
	pwlanhdr = (struct ieee80211_hdr *)pframe;	
	
	mac = myid(&(padapter->eeprompriv));
	bssid = cur_network->MacAddress;
	
	fctrl = &(pwlanhdr->frame_ctl);
	*(fctrl) = 0;
	_memcpy(pwlanhdr->addr1, StaAddr, ETH_ALEN);
	_memcpy(pwlanhdr->addr2, mac, ETH_ALEN);
	_memcpy(pwlanhdr->addr3, bssid, ETH_ALEN);

	SetSeqNum(pwlanhdr, 0);
	SetFrameSubType(fctrl, WIFI_PROBERSP);
	
	pktlen = sizeof(struct ieee80211_hdr_3addr);
	pframe += pktlen;

	if(cur_network->IELength>MAX_IE_SZ)
		return;

	_memcpy(pframe, cur_network->IEs, cur_network->IELength);
	pframe += cur_network->IELength;
	pktlen += cur_network->IELength;
	
	*pLength = pktlen;
}

//
// Description: In normal chip, we should send some packet to Hw which will be used by Fw
//			in FW LPS mode. The function is to fill the Tx descriptor of this packets, then 
//			Fw can tell Hw to send these packet derectly.
// Added by tynli. 2009.10.15.
//
static VOID
FillFakeTxDescriptor92C(
	IN PADAPTER		Adapter,
	IN u8*			pDesc,
	IN u32			BufferLen,
	IN BOOLEAN		IsPsPoll
)
{
	struct tx_desc	*ptxdesc = (struct tx_desc *)pDesc;

	// Clear all status
	_memset(pDesc, 0, 32);

	//offset 0
	ptxdesc->txdw0 |= cpu_to_le32( OWN | FSG | LSG); //own, bFirstSeg, bLastSeg;

	ptxdesc->txdw0 |= cpu_to_le32(((TXDESC_SIZE+OFFSET_SZ)<<OFFSET_SHT)&0x00ff0000); //32 bytes for TX Desc

	ptxdesc->txdw0 |= cpu_to_le32(BufferLen&0x0000ffff); // Buffer size + command header

	//offset 4
	ptxdesc->txdw1 |= cpu_to_le32((QSLT_MGNT<<QSEL_SHT)&0x00001f00); // Fixed queue of Mgnt queue

	//Set NAVUSEHDR to prevent Ps-poll AId filed to be changed to error vlaue by Hw.
	if(IsPsPoll)
	{
		ptxdesc->txdw1 |= cpu_to_le32(NAVUSEHDR);
	}
	else
	{
		ptxdesc->txdw4 |= cpu_to_le32(BIT(7)); // Hw set sequence number
		ptxdesc->txdw3 |= cpu_to_le32((8 <<28)); //set bit3 to 1. Suugested by TimChen. 2009.12.29.
	}

	//offset 16
	ptxdesc->txdw4 |= cpu_to_le32(BIT(8));//driver uses rate

#if (DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
	// USB interface drop packet if the checksum of descriptor isn't correct.
	// Using this checksum can let hardware recovery from packet bulk out error (e.g. Cancel URC, Bulk out error.).
	rtl8192cu_cal_txdesc_chksum(ptxdesc);
#endif
	
	//RT_PRINT_DATA(COMP_CMD, DBG_TRACE, "TxFillCmdDesc8192C(): H2C Tx Cmd Content ----->\n", pDesc, TX_DESC_SIZE);
}

// To check if reserved page content is destroyed by beacon beacuse beacon is too large.
// 2010.06.23. Added by tynli.
VOID
CheckFwRsvdPageContent(
	IN	PADAPTER		Adapter
)
{
	HAL_DATA_TYPE*	pHalData = GET_HAL_DATA(Adapter);
	u32	MaxBcnPageNum;	
	
 	if(pHalData->FwRsvdPageStartOffset != 0)
 	{
 		/*MaxBcnPageNum = PageNum_128(pMgntInfo->MaxBeaconSize);
		RT_ASSERT((MaxBcnPageNum <= pHalData->FwRsvdPageStartOffset), 
			("CheckFwRsvdPageContent(): The reserved page content has been"\
			"destroyed by beacon!!! MaxBcnPageNum(%d) FwRsvdPageStartOffset(%d)\n!",
			MaxBcnPageNum, pHalData->FwRsvdPageStartOffset));*/
 	}
}

//
// Description: Fill the reserved packets that FW will use to RSVD page. 
//			Now we just send 4 types packet to rsvd page.
//			(1)Beacon, (2)Ps-poll, (3)Null data, (4)ProbeRsp.
//	Input: 
//	    bDLFinished - FALSE: At the first time we will send all the packets as a large packet to Hw,
//				 		so we need to set the packet length to total lengh.
//			      TRUE: At the second time, we should send the first packet (default:beacon)
//						to Hw again and set the lengh in descriptor to the real beacon lengh.
// 2009.10.15 by tynli.
void SetFwRsvdPagePkt(PADAPTER Adapter, BOOLEAN bDLFinished)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	struct xmit_frame	*pmgntframe;
	struct pkt_attrib	*pattrib;
	struct xmit_priv	*pxmitpriv = &(Adapter->xmitpriv);
	struct mlme_ext_priv	*pmlmeext = &(Adapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	u32	BeaconLength, ProbeRspLength, PSPollLength, NullFunctionDataLength;
	u8	*ReservedPagePacket;
	u8	PageNum=0, U1bTmp, TxDescLen=0;
	u16	BufIndex=0;
	u32	TotalPacketLen;
	RSVDPAGE_LOC	RsvdPageLoc;
	BOOLEAN	bDLOK = _FALSE;

	DBG_871X("%s\n", __FUNCTION__);

	ReservedPagePacket = (u8*)_malloc(1000);
	if(ReservedPagePacket == NULL){
		DBG_871X("%s(): alloc ReservedPagePacket fail !!!\n", __FUNCTION__);
		return;
	}

	_memset(ReservedPagePacket, 0, 1000);

	TxDescLen = 32;//TX_DESC_SIZE;

	if(DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
	{
		BufIndex = TXDESC_OFFSET;
	}
	else
	{
		BufIndex = 0;
	}

	//(1) beacon
	ConstructBeacon(Adapter,&ReservedPagePacket[BufIndex],&BeaconLength);

	//printk("SetFwRsvdPagePkt(): HW_VAR_SET_TX_CMD: BCN\n", &ReservedPagePacket[BufIndex], (BeaconLength+BufIndex));

//--------------------------------------------------------------------

	// When we count the first page size, we need to reserve description size for the RSVD 
	// packet, it will be filled in front of the packet in TXPKTBUF.
	U1bTmp = (u8)PageNum_128(BeaconLength+TxDescLen);
	PageNum += U1bTmp;
	// To reserved 2 pages for beacon buffer. 2010.06.24.
	if(PageNum == 1)
		PageNum+=1;
	pHalData->FwRsvdPageStartOffset = PageNum;

	if(DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
		BufIndex = (PageNum*128) + TxDescLen+8; //Shift index for 8 bytes because the dummy bytes in the first descipstor.
	else
		BufIndex = (PageNum*128);
		
	//(2) ps-poll
	ConstructPSPoll(Adapter, &ReservedPagePacket[BufIndex],&PSPollLength);
	
	FillFakeTxDescriptor92C(Adapter, &ReservedPagePacket[BufIndex-TxDescLen], PSPollLength, _TRUE);

	//printk("SetFwRsvdPagePkt(): HW_VAR_SET_TX_CMD: PS-POLL\n", &ReservedPagePacket[BufIndex-TxDescLen], (PSPollLength+TxDescLen));

	RsvdPageLoc.LocPsPoll = PageNum;

//------------------------------------------------------------------
			
	U1bTmp = (u8)PageNum_128(PSPollLength+TxDescLen);
	PageNum += U1bTmp;

	if(DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
		BufIndex = (PageNum*128) + TxDescLen+8;
	else
		BufIndex = (PageNum*128);

	//(3) null data
	ConstructNullFunctionData(
		Adapter, 
		&ReservedPagePacket[BufIndex],
		&NullFunctionDataLength,
		get_my_bssid(&(pmlmeinfo->network)),
		_FALSE);
	
	FillFakeTxDescriptor92C(Adapter, &ReservedPagePacket[BufIndex-TxDescLen], NullFunctionDataLength, _FALSE);

	RsvdPageLoc.LocNullData = PageNum;
	
	//printk("SetFwRsvdPagePkt(): HW_VAR_SET_TX_CMD: NULL DATA \n", &ReservedPagePacket[BufIndex-TxDescLen], (NullFunctionDataLength+TxDescLen));
//------------------------------------------------------------------

	U1bTmp = (u8)PageNum_128(NullFunctionDataLength+TxDescLen);
	PageNum += U1bTmp;
	
	if(DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
		BufIndex = (PageNum*128) + TxDescLen+8;
	else
		BufIndex = (PageNum*128);
	
	//(4) probe response
	ConstructProbeRsp(
		Adapter, 
		&ReservedPagePacket[BufIndex],
		&ProbeRspLength,
		get_my_bssid(&(pmlmeinfo->network)),
		_FALSE);
	
	FillFakeTxDescriptor92C(Adapter, &ReservedPagePacket[BufIndex-TxDescLen], ProbeRspLength, _FALSE);

	RsvdPageLoc.LocProbeRsp = PageNum;

	//printk("SetFwRsvdPagePkt(): HW_VAR_SET_TX_CMD: PROBE RSP \n", &ReservedPagePacket[BufIndex-TxDescLen], (ProbeRspLength-TxDescLen));

//------------------------------------------------------------------

	U1bTmp = (u8)PageNum_128(ProbeRspLength+TxDescLen);

	PageNum += U1bTmp;

	TotalPacketLen = (PageNum*128);

	if ((pmgntframe = alloc_mgtxmitframe(pxmitpriv)) == NULL)
	{
		return;
	}

	//update attribute
	pattrib = &pmgntframe->attrib;
	update_mgntframe_attrib(Adapter, pattrib);
	pattrib->qsel = 0x10;
	pattrib->pktlen = pattrib->last_txcmdsz = TotalPacketLen - TxDescLen;
	_memcpy(pmgntframe->buf_addr, ReservedPagePacket, TotalPacketLen);

	Adapter->HalFunc.mgnt_xmit(Adapter, pmgntframe);

	bDLOK = _TRUE;

	if(bDLOK)
	{
		DBG_871X("Set RSVD page location to Fw.\n");
		FillH2CCmd(Adapter, RSVD_PAGE_EID, sizeof(RsvdPageLoc), (u8 *)&RsvdPageLoc);
	}

	_mfree(ReservedPagePacket,1000);

}

void rtl8192c_set_FwJoinBssReport_cmd(_adapter* padapter, u8 mstatus)
{
	JOINBSSRPT_PARM	JoinBssRptParm;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	
_func_enter_;

	DBG_871X("%s\n", __FUNCTION__);

	if(mstatus == 1)
	{
		// We should set AID, correct TSF, HW seq enable before set JoinBssReport to Fw in 88/92C.
		// Suggested by filen. Added by tynli.
		write16(padapter, REG_BCN_PSR_RPT, (0xC000|pmlmeinfo->aid));
		// Do not set TSF again here or vWiFi beacon DMA INT will not work.
		//correct_TSF(padapter, pmlmeext);
		// Hw sequende enable by dedault. 2010.06.23. by tynli.
		//write16(padapter, REG_NQOS_SEQ, ((pmlmeext->mgnt_seq+100)&0xFFF));
		//write8(padapter, REG_HWSEQ_CTRL, 0xFF);

		if(IS_NORMAL_CHIP(pHalData->VersionID))
		{
			BOOLEAN bRecover = _FALSE;

			//set REG_CR bit 8
			//U1bTmp = read8(padapter, REG_CR+1);
			write8(padapter,  REG_CR+1, 0x03);

			// Disable Hw protection for a time which revserd for Hw sending beacon.
			// Fix download reserved page packet fail that access collision with the protection time.
			// 2010.05.11. Added by tynli.
			//SetBcnCtrlReg(padapter, 0, BIT3);
			//SetBcnCtrlReg(padapter, BIT4, 0);
			write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)&(~BIT(3)));
			write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)|BIT(4));

			// Set FWHW_TXQ_CTRL 0x422[6]=0 to tell Hw the packet is not a real beacon frame.
			if(pHalData->RegFwHwTxQCtrl&BIT6)
				bRecover = _TRUE;

			// To tell Hw the packet is not a real beacon frame.
			//U1bTmp = read8(padapter, REG_FWHW_TXQ_CTRL+2);
			write8(padapter, REG_FWHW_TXQ_CTRL+2, (pHalData->RegFwHwTxQCtrl&(~BIT6)));
			pHalData->RegFwHwTxQCtrl &= (~BIT6);
			SetFwRsvdPagePkt(padapter, 0);

			// 2010.05.11. Added by tynli.
			//SetBcnCtrlReg(padapter, BIT3, 0);
			//SetBcnCtrlReg(padapter, 0, BIT4);
			write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)|BIT(3));
			write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)&(~BIT(4)));

			// To make sure that if there exists an adapter which would like to send beacon.
			// If exists, the origianl value of 0x422[6] will be 1, we should check this to
			// prevent from setting 0x422[6] to 0 after download reserved page, or it will cause 
			// the beacon cannot be sent by HW.
			// 2010.06.23. Added by tynli.
			if(bRecover)
			{
				write8(padapter, REG_FWHW_TXQ_CTRL+2, (pHalData->RegFwHwTxQCtrl|BIT6));
				pHalData->RegFwHwTxQCtrl |= BIT6;
			}

			// Clear CR[8] or beacon packet will not be send to TxBuf anymore.
			write8(padapter, REG_CR+1, 0x02);
		}
	}

	JoinBssRptParm.OpMode = mstatus;

	FillH2CCmd(padapter, JOINBSS_RPT_EID, sizeof(JoinBssRptParm), (u8 *)&JoinBssRptParm);
	
_func_exit_;
}

