/******************************************************************************
* pci_halinit.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :                                                          
*
*                                        
*                                                                                                                                       *
* Copyright 2007, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _HCI_HAL_INIT_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_efuse.h>

#include <rtl8192c_hal.h>
#include <rtl8192c_led.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifndef CONFIG_PCI_HCI

#error "CONFIG_PCI_HCI shall be on!\n"

#endif

#include <pci_ops.h>
#include <pci_hal.h>
#include <pci_osintf.h>


// For Two MAC FPGA verify we must disable all MAC/BB/RF setting
#define FPGA_UNKNOWN		0
#define FPGA_2MAC			1
#define FPGA_PHY			2
#define ASIC					3
#define BOARD_TYPE			ASIC

#if BOARD_TYPE == FPGA_2MAC
#define HAL_FW_ENABLE		0
#define HAL_MAC_ENABLE		0
#define HAL_BB_ENABLE		0
#define HAL_RF_ENABLE		0
#else // FPGA_PHY and ASIC
#define HAL_FW_ENABLE		1
#define HAL_MAC_ENABLE		1
#define HAL_BB_ENABLE		1
#define HAL_RF_ENABLE		1

#define FPGA_RF_UNKOWN	0
#define FPGA_RF_8225		1
#define FPGA_RF_0222D		2
#define FPGA_RF				FPGA_RF_0222D
#endif

static u8 getChnlGroup(u8 chnl)
{
	u8	group=0;

	if (chnl < 3)			// Cjanel 1-3
		group = 0;
	else if (chnl < 9)		// Channel 4-9
		group = 1;
	else					// Channel 10-14
		group = 2;
	
	return group;
}

static void ReadTxPowerInfoFromHWPG(
	IN PADAPTER		Adapter,
	BOOLEAN			AutoLoadFail,
	IN u8			*hwinfo
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u8	rf_path, index, tempval;
	u16	i;

	// EEPROM CCK Tx Power
	// EEPROM HT40 1S Tx Power
	for(rf_path=0; rf_path<2; rf_path++)
	{
		for(i=0; i<3; i++)
		{
			if(!AutoLoadFail)
			{
				pHalData->EEPROMChnlAreaTxPwrCCK[rf_path][i] = hwinfo[EEPROM_TxPowerCCK+rf_path*3+i];	
				pHalData->EEPROMChnlAreaTxPwrHT40_1S[rf_path][i] = 	hwinfo[EEPROM_TxPowerHT40_1S+rf_path*3+i];
			}
			else
			{
				pHalData->EEPROMChnlAreaTxPwrCCK[rf_path][i] = EEPROM_Default_TxPowerLevel;
				pHalData->EEPROMChnlAreaTxPwrHT40_1S[rf_path][i] = EEPROM_Default_TxPowerLevel;
			}
		}
	}
	// EEPROM HT40 2S Tx Power Diff
	for(i=0; i<3; i++)
	{
		if(!AutoLoadFail)
			tempval = hwinfo[EEPROM_TxPowerHT40_2SDiff+i];
		else
			tempval = EEPROM_Default_HT40_2SDiff;
		pHalData->EEPROMChnlAreaTxPwrHT40_2SDiff[RF90_PATH_A][i] = (tempval&0xf);
		pHalData->EEPROMChnlAreaTxPwrHT40_2SDiff[RF90_PATH_B][i] = ((tempval&0xf0)>>4);
	}
#if 0
	// Display if needed
	for(rf_path=0; rf_path<2; rf_path++)
		for (i=0; i<3; i++)
			DBG_8192C("RF-%c EEPROM CCK Area(%d) = 0x%x\n", 
				(rf_path==0)? 'A':'B', i, pHalData->EEPROMChnlAreaTxPwrCCK[rf_path][i]);
	for(rf_path=0; rf_path<2; rf_path++)
		for (i=0; i<3; i++)
			DBG_8192C("RF-%c EEPROM HT40 1S Area(%d) = 0x%x\n", 
				(rf_path==0)? 'A':'B', i, pHalData->EEPROMChnlAreaTxPwrHT40_1S[rf_path][i]));
	for(rf_path=0; rf_path<2; rf_path++)
		for (i=0; i<3; i++)
			DBG_8192C("RF-%c EEPROM HT40 2S Diff Area(%d) = 0x%x\n", 
				(rf_path==0)? 'A':'B', i, pHalData->EEPROMChnlAreaTxPwrHT40_2SDiff[rf_path][i]));
#endif
	// Assign eeprom val to hal vars for later use
	for(rf_path=0; rf_path<2; rf_path++)
	{
		// Assign dedicated channel tx power
		for(i=0; i<14; i++)	// channel 1~3 use the same Tx Power Level.
		{
			index = getChnlGroup((u8)i);
			// Record A & B CCK /OFDM - 1T/2T Channel area tx power
			pHalData->TxPwrLevelCck[rf_path][i]  = 
			pHalData->EEPROMChnlAreaTxPwrCCK[rf_path][index];
			pHalData->TxPwrLevelHT40_1S[rf_path][i]  = 
			pHalData->EEPROMChnlAreaTxPwrHT40_1S[rf_path][index];

			if((pHalData->EEPROMChnlAreaTxPwrHT40_1S[rf_path][index] - 
				pHalData->EEPROMChnlAreaTxPwrHT40_2SDiff[rf_path][index]) > 0)
			{
				pHalData->TxPwrLevelHT40_2S[rf_path][i]  = 
					pHalData->EEPROMChnlAreaTxPwrHT40_1S[rf_path][index] - 
					pHalData->EEPROMChnlAreaTxPwrHT40_2SDiff[rf_path][index];
			}
			else
			{
				pHalData->TxPwrLevelHT40_2S[rf_path][i]  = 0;
			}
		}

		/*for(i=0; i<14; i++)
		{
			DBG_8192C("RF-%c, Ch(%d) [CCK = 0x%x / HT40_1S = 0x%x / HT40_2S = 0x%x]\n", 
				(rf_path==0)? 'A':'B', i+1, pHalData->TxPwrLevelCck[rf_path][i], 
				pHalData->TxPwrLevelHT40_1S[rf_path][i], 
				pHalData->TxPwrLevelHT40_2S[rf_path][i]));
		}*/
	}
	
	for(i=0; i<3; i++)
	{
		// Read Power diff limit.
		if(!AutoLoadFail)
		{
			pHalData->EEPROMPwrLimitHT40[i] = hwinfo[EEPROM_TxPWRGroup+i];
			pHalData->EEPROMPwrLimitHT20[i] = hwinfo[EEPROM_TxPWRGroup+3+i];
		}
		else
		{
			pHalData->EEPROMPwrLimitHT40[i] = 0;
			pHalData->EEPROMPwrLimitHT20[i] = 0;
		}
	}

	for(rf_path=0; rf_path<2; rf_path++)
	{
		// Fill Pwr group
		for(i=0; i<14; i++)
		{
			index = getChnlGroup((u8)i);
			if(rf_path == RF90_PATH_A)
			{
				pHalData->PwrGroupHT20[rf_path][i] = (pHalData->EEPROMPwrLimitHT20[index]&0xf);
				pHalData->PwrGroupHT40[rf_path][i] = (pHalData->EEPROMPwrLimitHT40[index]&0xf);
			}
			else if(rf_path == RF90_PATH_B)
			{
				pHalData->PwrGroupHT20[rf_path][i] = ((pHalData->EEPROMPwrLimitHT20[index]&0xf0)>>4);
				pHalData->PwrGroupHT40[rf_path][i] = ((pHalData->EEPROMPwrLimitHT40[index]&0xf0)>>4);
			}
			//DBG_8192C("RF-%c PwrGroupHT20[%d] = 0x%x\n", (rf_path==0)? 'A':'B', i, pHalData->PwrGroupHT20[rf_path][i]);
			//DBG_8192C("RF-%c PwrGroupHT40[%d] = 0x%x\n", (rf_path==0)? 'A':'B', i, pHalData->PwrGroupHT40[rf_path][i]);
		}
	}


	for(i=0; i<14; i++)	// channel 1~3 use the same Tx Power Level.
	{
		// Read tx power difference between HT OFDM 20/40 MHZ
		index = getChnlGroup((u8)i);
		if(!AutoLoadFail)
			tempval = hwinfo[EEPROM_TxPowerHT20Diff+index];
		else
			tempval = EEPROM_Default_HT20_Diff;		
		pHalData->TxPwrHt20Diff[RF90_PATH_A][i] = (tempval&0xF);
		pHalData->TxPwrHt20Diff[RF90_PATH_B][i] = ((tempval>>4)&0xF);

		if(pHalData->TxPwrHt20Diff[RF90_PATH_A][i] & BIT3)	//4bit sign number to 8 bit sign number
			pHalData->TxPwrHt20Diff[RF90_PATH_A][i] |= 0xF0;		

		if(pHalData->TxPwrHt20Diff[RF90_PATH_B][i] & BIT3)	//4bit sign number to 8 bit sign number
			pHalData->TxPwrHt20Diff[RF90_PATH_B][i] |= 0xF0;		

		// Read OFDM<->HT tx power diff
		index = getChnlGroup((u8)i);
		if(!AutoLoadFail)
			tempval = hwinfo[EEPROM_TxPowerOFDMDiff+index];
		else
			tempval = EEPROM_Default_LegacyHTTxPowerDiff;
		pHalData->TxPwrLegacyHtDiff[RF90_PATH_A][i] = (tempval&0xF);
		pHalData->TxPwrLegacyHtDiff[RF90_PATH_B][i] = ((tempval>>4)&0xF);
	}
	//for ccx use
	pHalData->LegacyHTTxPowerDiff = pHalData->TxPwrLegacyHtDiff[RF90_PATH_A][7];
	
	//for(i=0; i<14; i++)
	//	DBG_8192C("RF-A Ht20 to HT40 Diff[%d] = 0x%x\n", i, pHalData->TxPwrHt20Diff[RF90_PATH_A][i]);
	//for(i=0; i<14; i++)
	//	DBG_8192C("RF-B Ht20 to HT40 Diff[%d] = 0x%x\n", i, pHalData->TxPwrHt20Diff[RF90_PATH_B][i]);
	//for(i=0; i<14; i++)
	//	DBG_8192C("RF-A Legacy to Ht40 Diff[%d] = 0x%x\n", i, pHalData->TxPwrLegacyHtDiff[RF90_PATH_A][i]);
	//for(i=0; i<14; i++)
	//	DBG_8192C("RF-B Legacy to HT40 Diff[%d] = 0x%x\n", i, pHalData->TxPwrLegacyHtDiff[RF90_PATH_B][i]);

	if(!AutoLoadFail)
	{
		pHalData->EEPROMRegulatory = (hwinfo[RF_OPTION1]&0x7);	//bit0~2
	}
	else
	{
		pHalData->EEPROMRegulatory = 0;
	}
	DBG_8192C("EEPROMRegulatory = 0x%x\n", pHalData->EEPROMRegulatory);
	//
	// Get TSSI value for each path.
	//
	if(!AutoLoadFail)
	{
		pHalData->EEPROMTSSI[RF90_PATH_A] = hwinfo[EEPROM_TSSI_A];
		pHalData->EEPROMTSSI[RF90_PATH_B] = hwinfo[EEPROM_TSSI_B];
	}
	else
	{
		pHalData->EEPROMTSSI[RF90_PATH_A] = EEPROM_Default_TSSI;
		pHalData->EEPROMTSSI[RF90_PATH_B] = EEPROM_Default_TSSI;
	}
	DBG_8192C("TSSI_A = 0x%x, TSSI_B = 0x%x\n", pHalData->EEPROMTSSI[RF90_PATH_A], pHalData->EEPROMTSSI[RF90_PATH_B]);
		
	//
	// ThermalMeter from EEPROM
	//
	if(!AutoLoadFail)
		tempval = hwinfo[EEPROM_THERMAL_METER];
	else
		tempval = EEPROM_Default_ThermalMeter;
	pHalData->EEPROMThermalMeter = (tempval&0x1f);	//[4:0]

	if(pHalData->EEPROMThermalMeter == 0x1f || AutoLoadFail)
		pdmpriv->bAPKThermalMeterIgnore = _TRUE;

#if 0
	if(pHalData->EEPROMThermalMeter < 0x06 || pHalData->EEPROMThermalMeter > 0x1c)
		pHalData->EEPROMThermalMeter = 0x12;
#endif	
	
	pdmpriv->ThermalMeter[0] = pHalData->EEPROMThermalMeter;	
	DBG_8192C("ThermalMeter = 0x%x\n", pHalData->EEPROMThermalMeter);
}

//
//	Description:
//		Config HW adapter information into initial value.
//
//	Assumption:
//		1. After Auto load fail(i.e, check CR9346 fail)
//
//	Created by Roger, 2008.10.21.
//
static	VOID
ConfigAdapterInfo8192CForAutoLoadFail(
	IN PADAPTER			Adapter
)
{
	EEPROM_EFUSE_PRIV	*pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u16	i;
	u8	sMacAddr[6] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
	u8	*hwinfo = 0;

	DBG_8192C("====> ConfigAdapterInfo8192CForAutoLoadFail\n");
	// Marked by tynli. Suggested by Alfred.
	//write8(Adapter, REG_SYS_ISO_CTRL+1, 0xE8); // Isolation signals from Loader
	//udelay_os(10000);
	//write8(Adapter, REG_APS_FSMCO, 0x02); // Enable Loader Data Keep
	
	// Initialize IC Version && Channel Plan
	pHalData->EEPROMVID = 0;
	pHalData->EEPROMDID = 0;		
	pHalData->EEPROMChannelPlan = 0;
	pHalData->EEPROMCustomerID = 0;
	//cosa pHalData->bIgnoreDiffRateTxPowerOffset = FALSE;

	DBG_8192C("EEPROM VID = 0x%4x\n", pHalData->EEPROMVID);
	DBG_8192C("EEPROM DID = 0x%4x\n", pHalData->EEPROMDID);	
	DBG_8192C("EEPROM Customer ID: 0x%2x\n", pHalData->EEPROMCustomerID);
	DBG_8192C("EEPROM ChannelPlan = 0x%4x\n", pHalData->EEPROMChannelPlan);
	//cosa RT_TRACE(COMP_INIT, DBG_LOUD, ("IgnoreDiffRateTxPowerOffset = %d\n", pHalData->bIgnoreDiffRateTxPowerOffset));
       //
	//<Roger_Notes> In this case, we random assigh MAC address here. 2008.10.15.
	//
	//Initialize Permanent MAC address
	//if(!Adapter->bInHctTest)
       // sMacAddr[5] = (u1Byte)GetRandomNumber(1, 254);	
	for(i = 0; i < 6; i++)
		pEEPROM->mac_addr[i] = sMacAddr[i];
	
	//NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);

	DBG_8192C("Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
	pEEPROM->mac_addr[0], pEEPROM->mac_addr[1], 
	pEEPROM->mac_addr[2], pEEPROM->mac_addr[3], 
	pEEPROM->mac_addr[4], pEEPROM->mac_addr[5]);
	
	//
	// Read tx power index from efuse or eeprom
	//
	ReadTxPowerInfoFromHWPG(Adapter, pEEPROM->bautoload_fail_flag, hwinfo);
	//
	// Read Bluetooth co-exist and initialize
	//
#ifdef CONFIG_BT_COEXIST
	//ReadBluetoothCoexistInfoFromHWPG(Adapter, pEEPROM->bautoload_fail_flag, hwinfo);
	rtl8192c_ReadBluetoothCoexistInfo(Adapter, hwinfo, pEEPROM->bautoload_fail_flag);
#endif
	
	//
	// Read EEPROM Version && Channel Plan
	//
	// Default channel plan  =0
	pHalData->EEPROMChannelPlan = 0;
	pHalData->EEPROMVersion = 1;		// Default version is 1
	pHalData->bTXPowerDataReadFromEEPORM = _FALSE;

	// 20100318 Joseph: These two variable is set in the beginning of the ReadAdapterInfo8192C().
	//pHalData->RF_Type = RTL819X_DEFAULT_RF_TYPE;	// default : 1T2R
	//pHalData->RFChipID = RF_6052;
	
	pHalData->EEPROMCustomerID = 0;

	DBG_8192C("EEPROM Customer ID: 0x%2x\n", pHalData->EEPROMCustomerID);
			
	pHalData->EEPROMBoardType = EEPROM_Default_BoardType;	
	DBG_8192C("BoardType = %#x\n", pHalData->EEPROMBoardType);

//	pHalData->LedStrategy = SW_LED_MODE0;

//1 JOSEPH_REVISE
	//InitRateAdaptive(Adapter);
	
	DBG_8192C("<==== ConfigAdapterInfo8192CForAutoLoadFail\n");
}

static BOOLEAN
Check11nProductID(
	IN	PADAPTER	Adapter)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);

	if(((pHalData->EEPROMDID==0x8191)) ||
		((pHalData->EEPROMDID==0x8176) && (
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8176) || // <= Start of 88CE Solo
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8181) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8182) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8184) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8185) ||
			(pHalData->EEPROMSVID == 0x1025 && pHalData->EEPROMSMID == 0x8186) ||
			(pHalData->EEPROMSVID == 0x1025 && pHalData->EEPROMSMID == 0x8187) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8191) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8192) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8193) ||
			(pHalData->EEPROMSVID == 0x1A3B && pHalData->EEPROMSMID == 0x1139) ||
			(pHalData->EEPROMSVID == 0x1028 && pHalData->EEPROMSMID == 0x8194) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8197) ||
			(pHalData->EEPROMSVID == 0x1462 && pHalData->EEPROMSMID == 0x3824) ||
			(pHalData->EEPROMSVID == 0x1028 && pHalData->EEPROMSMID == 0x8198) ||
			(pHalData->EEPROMSVID == 0x1A32 && pHalData->EEPROMSMID == 0x0315) ||
			(pHalData->EEPROMSVID == 0x144F && pHalData->EEPROMSMID == 0x7185) ||
			(pHalData->EEPROMSVID == 0x103C && pHalData->EEPROMSMID == 0x1629) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8199) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8203) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8204) ||
			(pHalData->EEPROMSVID == 0x1A32 && pHalData->EEPROMSMID == 0x2315) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7611) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8150) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9176) || // <= Start of 88CE Combo
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9181) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9182) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9184) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9185) ||
			(pHalData->EEPROMSVID == 0x103C && pHalData->EEPROMSMID == 0x196F) ||
			(pHalData->EEPROMSVID == 0x10CF && pHalData->EEPROMSMID == 0x16B3) ||
			(pHalData->EEPROMSVID == 0x1025 && pHalData->EEPROMSMID == 0x9186) ||
			(pHalData->EEPROMSVID == 0x1025 && pHalData->EEPROMSMID == 0x9187) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9191) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9192) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9193) ||
			(pHalData->EEPROMSVID == 0x1A3B && pHalData->EEPROMSMID == 0x2057) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9194) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9196) ||
			(pHalData->EEPROMSVID == 0x1028 && pHalData->EEPROMSMID == 0x9197) ||
			(pHalData->EEPROMSVID == 0x1462 && pHalData->EEPROMSMID == 0x3874) ||
			(pHalData->EEPROMSVID == 0x1028 && pHalData->EEPROMSMID == 0x9198) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9201) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9150) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8195) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8200) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9195) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9199) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8201) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8202) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9200)))	||
		((pHalData->EEPROMDID==0x8177)) ||
		((pHalData->EEPROMDID==0x8178) && (
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8178) ||
			(pHalData->EEPROMSVID == 0x1025 && pHalData->EEPROMSMID == 0x8179) ||
			(pHalData->EEPROMSVID == 0x1025 && pHalData->EEPROMSMID == 0x8180) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8186) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8189) ||
			(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9178) ||
			(pHalData->EEPROMSVID == 0x1025 && pHalData->EEPROMSMID == 0x9179) ||
			(pHalData->EEPROMSVID == 0x1025 && pHalData->EEPROMSMID == 0x9180))))
	{
		return _TRUE;
	}
	else
	{
		return _FALSE;
	}
	
}

static VOID
HalCustomizedBehavior8192C(
	PADAPTER			Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	EEPROM_EFUSE_PRIV	*pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	struct led_priv	*pledpriv = &(Adapter->ledpriv);
	
	pledpriv->LedStrategy = SW_LED_MODE7; //Default LED strategy.

	switch(pHalData->CustomerID)
	{

		case RT_CID_DEFAULT:
			break;

		case RT_CID_819x_SAMSUNG:
			//pMgntInfo->bAutoConnectEnable = _FALSE;
			//pMgntInfo->bForcedShowRateStill = _TRUE;
			break;

		case RT_CID_TOSHIBA:
			pHalData->CurrentChannel = 10;
			//pHalData->EEPROMRegulatory = 1;
			break;

		case RT_CID_CCX:
			//pMgntInfo->IndicateByDeauth = _TRUE;
			break;

		case RT_CID_819x_Lenovo:
			// Customize Led mode	
			pledpriv->LedStrategy = SW_LED_MODE7;
			// Customize  Link any for auto connect
			// This Value should be set after InitializeMgntVariables
			//pMgntInfo->bAutoConnectEnable = FALSE;
			//pMgntInfo->pHTInfo->RxReorderPendingTime = 50;
			DBG_8192C("RT_CID_819x_Lenovo \n");

			if((pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8200) ||
				(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9199) )
			{
				pledpriv->LedStrategy = SW_LED_MODE9;
			}
			else if((pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8201) ||
					(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8202) ||
					(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9200))
			{
				pledpriv->LedStrategy = SW_LED_MODE7;
			}
			else
			{
				pledpriv->LedStrategy = SW_LED_MODE7;
			}
			break;

		case RT_CID_819x_QMI:
			pledpriv->LedStrategy = SW_LED_MODE8; // Customize Led mode	
			break;			

		case RT_CID_819x_HP:
			pledpriv->LedStrategy = SW_LED_MODE7; // Customize Led mode	
			pHalData->bLedOpenDrain = _TRUE;// Support Open-drain arrangement for controlling the LED. Added by Roger, 2009.10.16.
			break;

		case RT_CID_819x_Acer:
			break;			

		case RT_CID_WHQL:
			//Adapter->bInHctTest = TRUE;
			break;

		case RT_CID_819x_PRONETS:
			pledpriv->LedStrategy = SW_LED_MODE9; // Customize Led mode	
			break;			
	
		default:
			DBG_8192C("Unkown hardware Type \n");
			break;
	}
	DBG_8192C("HalCustomizedBehavior8192C(): RT Customized ID: 0x%02X\n", pHalData->CustomerID);

	pHalData->bLedOpenDrain = _TRUE;// Support Open-drain arrangement for controlling the LED. Added by Roger, 2009.10.16.

	if((pEEPROM->bautoload_fail_flag) || (!Check11nProductID(Adapter)))
	{
		if(pHalData->CurrentWirelessMode != WIRELESS_MODE_B)
			pHalData->CurrentWirelessMode = WIRELESS_MODE_G;
	}

	// 20100707 Joseph: Add for Edimax HW WPS PBC function.
	if(pHalData->EEPROMDID==0x8176 && pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7611)
	{
		pHalData->bGpioHwWpsPbc = _TRUE;
		//pMgntInfo->PowerSaveControl.bGpioRfSw = _FALSE;
	}

}

static	VOID
_ReadAdapterInfo8192CE(
	IN PADAPTER			Adapter
	)
{
	EEPROM_EFUSE_PRIV	*pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u16			i,usValue;
	u16			EEPROMId;
	u8			tempval;
	u8			hwinfo[HWSET_MAX_SIZE];

	//DBG_8192C("====> ReadAdapterInfo8192CE\n");

	if (pEEPROM->EepromOrEfuse == EEPROM_93C46)
	{	// Read frin EEPROM
		//DBG_8192C("EEPROM\n");
	}
	else if (pEEPROM->EepromOrEfuse == EEPROM_BOOT_EFUSE)
	{	// Read from EFUSE	
		//DBG_8192C("EFUSE\n");

		// Read EFUSE real map to shadow!!
		EFUSE_ShadowMapUpdate(Adapter);

		_memcpy((void*)hwinfo, (void*)pEEPROM->efuse_eeprom_data, HWSET_MAX_SIZE);
	}

	// Print current HW setting map!!
	//RT_PRINT_DATA(COMP_INIT, DBG_LOUD, ("MAP \n"), hwinfo, HWSET_MAX_SIZE);			

	// Checl 0x8129 again for making sure autoload status!!
	EEPROMId = *((u16 *)&hwinfo[0]);
	if( EEPROMId != RTL8190_EEPROM_ID )
	{
		DBG_8192C("EEPROM ID(%#x) is invalid!!\n", EEPROMId);
		pEEPROM->bautoload_fail_flag = _TRUE;
	}
	else
	{
		//DBG_8192C("Autoload OK\n");
		pEEPROM->bautoload_fail_flag = _FALSE;
	}	

	//
	if (pEEPROM->bautoload_fail_flag == _TRUE)
	{
		ConfigAdapterInfo8192CForAutoLoadFail(Adapter);
		return;
	}

	// Read IC Version && Channel Plan	
	// 20100318 Joseph: These variable are set in the beginning of the ReadAdapterInfo8192C().
	//pHalData->VersionID = ReadChipVersion(Adapter);
	//pHalData->RF_Type = (version & BIT0)?RF_2T2R:RF_1T1R;
	//pHalData->RFChipID = RF_6052;
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("VersionID = 0x%4x\n", pHalData->VersionID));
	   
	// VID, DID	 SE	0xA-D
	pHalData->EEPROMVID		= *(u16 *)&hwinfo[EEPROM_VID];
	pHalData->EEPROMDID		= *(u16 *)&hwinfo[EEPROM_DID];
	pHalData->EEPROMSVID	= *(u16 *)&hwinfo[EEPROM_SVID];
	pHalData->EEPROMSMID	= *(u16 *)&hwinfo[EEPROM_SMID];

	DBG_8192C("EEPROMId = 0x%4x\n", EEPROMId);
	DBG_8192C("EEPROM VID = 0x%4x\n", pHalData->EEPROMVID);
	DBG_8192C("EEPROM DID = 0x%4x\n", pHalData->EEPROMDID);
	DBG_8192C("EEPROM SVID = 0x%4x\n", pHalData->EEPROMSVID);
	DBG_8192C("EEPROM SMID = 0x%4x\n", pHalData->EEPROMSMID);


	//Read Permanent MAC address
	for(i = 0; i < 6; i += 2)
	{
		usValue = *(u16 *)&hwinfo[EEPROM_MAC_ADDR+i];
		*((u16 *)(&pEEPROM->mac_addr[i])) = usValue;
	}

	//NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);

	//DBG_8192C("ReadAdapterInfo8192CE(), Permanent Address = %02x-%02x-%02x-%02x-%02x-%02x\n", 
	//pEEPROM->mac_addr[0], pEEPROM->mac_addr[1], 
	//pEEPROM->mac_addr[2], pEEPROM->mac_addr[3], 
	//pEEPROM->mac_addr[4], pEEPROM->mac_addr[5]); 

	//
	// Read tx power index from efuse or eeprom
	//
	ReadTxPowerInfoFromHWPG(Adapter, pEEPROM->bautoload_fail_flag, hwinfo);
	//
	// Read Bluetooth co-exist and initialize
	//
#ifdef CONFIG_BT_COEXIST
	//ReadBluetoothCoexistInfoFromHWPG(Adapter, pEEPROM->bautoload_fail_flag, hwinfo);
	rtl8192c_ReadBluetoothCoexistInfo(Adapter, hwinfo, pEEPROM->bautoload_fail_flag);
#endif
	
	//
	// Read IC Version && Channel Plan
	//
	// Version ID, Channel plan
	pHalData->EEPROMChannelPlan = *(u8 *)&hwinfo[EEPROM_ChannelPlan];
	pHalData->EEPROMVersion  = *(u16 *)&hwinfo[EEPROM_Version];
	pHalData->bTXPowerDataReadFromEEPORM = _TRUE;
	//DBG_8192C("EEPROM ChannelPlan = 0x%02x\n", pHalData->EEPROMChannelPlan);

#if 0	
	//
	// Read Customer ID or Board Type!!!
	//
	tempval = 0; 
	// 2008/11/14 MH RTL8192SE_PCIE_EEPROM_SPEC_V0.6_20081111.doc
	// 2008/12/09 MH RTL8192SE_PCIE_EEPROM_SPEC_V0.7_20081201.doc
	// Change RF type definition
	if (tempval == 0)	// 2x2           RTL8192SE (QFN68)
		pHalData->RF_Type = RF_2T2R;
	else if (tempval == 1)	 // 1x2           RTL8191RE (QFN68)
		pHalData->RF_Type = RF_1T2R;
	else if (tempval == 2)	 //  1x2		RTL8191SE (QFN64) Crab
		pHalData->RF_Type = RF_1T2R;
	else if (tempval == 3)	 //  1x1 		RTL8191SE (QFN64) Unicorn
		pHalData->RF_Type = RF_1T1R;
#endif

	pHalData->EEPROMCustomerID = *(u8 *)&hwinfo[EEPROM_CustomID]; 	
	DBG_8192C("EEPROM Customer ID: 0x%02x\n", pHalData->EEPROMCustomerID);

	// If the customer ID had been changed by registry, do not cover up by EEPROM.
	if(pHalData->CustomerID == RT_CID_DEFAULT)
	{
		switch(pHalData->EEPROMCustomerID)
		{	
			case EEPROM_CID_DEFAULT:
				if(pHalData->EEPROMDID==0x8176)
				{
					if((pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6151) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6152) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6154) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6155) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6177) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6178) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6179) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6180) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7151) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7152) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7154) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7155) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7177) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7178) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7179) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7180) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8151) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8152) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8154) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8155) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8181) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8182) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8184) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8185) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9151) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9152) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9154) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9155) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9181) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9182) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9184) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9185) )
					{
						pHalData->CustomerID = RT_CID_TOSHIBA;
					}
					else if(pHalData->EEPROMSVID == 0x1025)
					{
						pHalData->CustomerID = RT_CID_819x_Acer;	
					}
					else if((pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6191) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6192) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6193) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7191) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7192) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7193) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8191) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8192) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8193) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9191) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9192) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9193) )
					{
						pHalData->CustomerID = RT_CID_819x_SAMSUNG;
					}
					else if((pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8195) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9195) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7194) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8200) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8201) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8202) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9199) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9200))
					{
						pHalData->CustomerID = RT_CID_819x_Lenovo;
					}
					else if((pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8197) ||
							(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9196) )
					{
						pHalData->CustomerID = RT_CID_819x_CLEVO;
					}
					else if((pHalData->EEPROMSVID == 0x1028 && pHalData->EEPROMSMID == 0x8194) ||
							(pHalData->EEPROMSVID == 0x1028 && pHalData->EEPROMSMID == 0x8198) ||
							(pHalData->EEPROMSVID == 0x1028 && pHalData->EEPROMSMID == 0x9197) ||
							(pHalData->EEPROMSVID == 0x1028 && pHalData->EEPROMSMID == 0x9198))
					{
						pHalData->CustomerID = RT_CID_819x_DELL;
					}
					else if((pHalData->EEPROMSVID == 0x103C && pHalData->EEPROMSMID == 0x1629))// HP LiteOn
					{
			 			pHalData->CustomerID = RT_CID_819x_HP;
					}
					else if((pHalData->EEPROMSVID == 0x1A32 && pHalData->EEPROMSMID == 0x2315))// QMI
					{
						pHalData->CustomerID = RT_CID_819x_QMI;
					}
					else if((pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8203))// QMI
					{
						pHalData->CustomerID = RT_CID_819x_PRONETS;
					}
					else
					{
						pHalData->CustomerID = RT_CID_DEFAULT;
					}
				}
				else if(pHalData->EEPROMDID==0x8178)
				{
					if((pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6181) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6182) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6184) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x6185) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7181) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7182) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7184) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x7185) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8181) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8182) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8184) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8185) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9181) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9182) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9184) ||
						(pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x9185) )
							pHalData->CustomerID = RT_CID_TOSHIBA;
					else if(pHalData->EEPROMSVID == 0x1025)
						pHalData->CustomerID = RT_CID_819x_Acer;
					else if((pHalData->EEPROMSVID == 0x10EC && pHalData->EEPROMSMID == 0x8186))// PRONETS					
						pHalData->CustomerID = RT_CID_819x_PRONETS;
					else
						pHalData->CustomerID = RT_CID_DEFAULT;
				}
				else
				{
					pHalData->CustomerID = RT_CID_DEFAULT;
				}
				break;
				
			case EEPROM_CID_TOSHIBA:       
				pHalData->CustomerID = RT_CID_TOSHIBA;
				break;

			case EEPROM_CID_QMI:
				pHalData->CustomerID = RT_CID_819x_QMI;
				break;
				
			case EEPROM_CID_WHQL:
				/*Adapter->bInHctTest = TRUE;
				
				pMgntInfo->bSupportTurboMode = FALSE;
				pMgntInfo->bAutoTurboBy8186 = FALSE;
				pMgntInfo->PowerSaveControl.bInactivePs = FALSE;
				pMgntInfo->PowerSaveControl.bIPSModeBackup = FALSE;
				pMgntInfo->PowerSaveControl.bLeisurePs = FALSE;
				pMgntInfo->keepAliveLevel = 0;	
				Adapter->bUnloadDriverwhenS3S4 = FALSE;*/
				break;
					
			default:
				pHalData->CustomerID = RT_CID_DEFAULT;
				break;
					
		}
	}

	//DBG_8192C("MGNT Customer ID: 0x%02x\n", pHalData->CustomerID);

#ifdef CONFIG_ANTENNA_DIVERSITY
	// Antenna Diversity setting. 
	if(Adapter->registrypriv.antdiv_cfg == 2) // 2: From Efuse
		pHalData->AntDivCfg = (hwinfo[RF_OPTION1]&0x18)>>3;
	else
		pHalData->AntDivCfg = Adapter->registrypriv.antdiv_cfg;  // 0:OFF , 1:ON,

	DBG_8192C("SWAS: bHwAntDiv = %x\n", pHalData->AntDivCfg);
#endif

	pHalData->InterfaceSel = (hwinfo[RF_OPTION1]&0xE0)>>5;
	DBG_8192C("Board Type: 0x%x\n", pHalData->InterfaceSel);

	// 2008/01/16 MH We can only know RF type in the function. So we have to init
	// DIG RATR table again.
	//InitRateAdaptive(Adapter);
	
	//DBG_8192C("<==== ReadAdapterInfo8192CE\n");
}

static int _ReadAdapterInfo8192C(PADAPTER Adapter)
{
	EEPROM_EFUSE_PRIV	*pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv		*pmlmepriv = &(Adapter->mlmepriv);
	struct registry_priv	*pregistrypriv = &Adapter->registrypriv;
	u8	tmpU1b;

	//DBG_8192C("====> ReadAdapterInfo8192C\n");

	// For debug test now!!!!!
	//PHY_RFShadowRefresh(Adapter);

	// Read IC Version && Channel Plan	
	pHalData->VersionID = rtl8192c_ReadChipVersion(Adapter);
	pHalData->rf_chip = RF_6052;
	if (pHalData->rf_type == RF_1T1R)
		pHalData->bRFPathRxEnable[0] = _TRUE;
	else
		pHalData->bRFPathRxEnable[0] = pHalData->bRFPathRxEnable[1] = _TRUE;

	DBG_8192C("VersionID = 0x%4x\n", pHalData->VersionID);

	tmpU1b = read8(Adapter, REG_9346CR);

	if (tmpU1b & BIT4)
	{
		DBG_8192C("Boot from EEPROM\n");
		pEEPROM->EepromOrEfuse = EEPROM_93C46;
	}
	else 
	{
		DBG_8192C("Boot from EFUSE\n");
		pEEPROM->EepromOrEfuse = EEPROM_BOOT_EFUSE;
	}

	// Autoload OK
	if (tmpU1b & BIT5)
	{
		DBG_8192C("Autoload OK\n");
		pEEPROM->bautoload_fail_flag = _FALSE;
		_ReadAdapterInfo8192CE(Adapter);
	}	
	else
	{ 	// Auto load fail.		
		DBG_8192C("AutoLoad Fail reported from CR9346!!\n");
		pEEPROM->bautoload_fail_flag = _TRUE;
		ConfigAdapterInfo8192CForAutoLoadFail(Adapter);		

		if (pEEPROM->EepromOrEfuse == EEPROM_BOOT_EFUSE)
		{
			EFUSE_ShadowMapUpdate(Adapter);
		}
	}	

	if(pHalData->CustomerID == RT_CID_DEFAULT)
	{ // If we have not yet change pMgntInfo->CustomerID in register, 
		switch(pHalData->EEPROMCustomerID)
		{
			case EEPROM_CID_DEFAULT:
				pHalData->CustomerID = RT_CID_DEFAULT;
				break;
			case EEPROM_CID_TOSHIBA:       
				pHalData->CustomerID = RT_CID_TOSHIBA;
				break;

			case EEPROM_CID_CCX:
				pHalData->CustomerID = RT_CID_CCX;
				break;
				
			default:
				// value from RegCustomerID
				break;
		}
	}

	if((pregistrypriv->channel_plan>= RT_CHANNEL_DOMAIN_MAX) || (pHalData->EEPROMChannelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK))
	{
		pmlmepriv->ChannelPlan = _HalMapChannelPlan8192C(Adapter, (pHalData->EEPROMChannelPlan & (~(EEPROM_CHANNEL_PLAN_BY_HW_MASK))));
		//pMgntInfo->bChnlPlanFromHW = (channelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK) ? _TRUE : _FALSE; // User cannot change  channel plan.
	}
	else
	{
		pmlmepriv->ChannelPlan = (RT_CHANNEL_DOMAIN)pregistrypriv->channel_plan;
	}

#if 0 //todo:
	switch(pMgntInfo->ChannelPlan)
	{
		case RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN:
		{
			PRT_DOT11D_INFO	pDot11dInfo = GET_DOT11D_INFO(pMgntInfo);
	
			pDot11dInfo->bEnabled = TRUE;
		}
		RT_TRACE(COMP_INIT, DBG_LOUD, ("ReadAdapterInfo8187(): Enable dot11d when RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN!\n"));
		break;
	}
#endif

	//MSG_8192C("RegChannelPlan(%d) EEPROMChannelPlan(%d)", pregistrypriv->channel_plan, pHalData->EEPROMChannelPlan);
	//MSG_8192C("ChannelPlan = %d\n" , pmlmepriv->ChannelPlan);

	HalCustomizedBehavior8192C(Adapter);	

	//DBG_8192C("<==== ReadAdapterInfo8192C\n");

	return _SUCCESS;
}

static void ReadAdapterInfo8192CE(PADAPTER Adapter)
{
	// Read EEPROM size before call any EEPROM function
	//Adapter->EepromAddressSize=Adapter->HalFunc.GetEEPROMSizeHandler(Adapter);
	Adapter->EepromAddressSize = GetEEPROMSize8192C(Adapter);
	
	_ReadAdapterInfo8192C(Adapter);
}


void rtl8192ce_interface_configure(_adapter *padapter)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;

_func_enter_;

	////close ASPM for AMD defaultly
	pdvobjpriv->const_amdpci_aspm = 0;

	//// ASPM PS mode.
	//// 0 - Disable ASPM, 1 - Enable ASPM without Clock Req, 
	//// 2 - Enable ASPM with Clock Req, 3- Alwyas Enable ASPM with Clock Req,
	//// 4-  Always Enable ASPM without Clock Req.
	//// set defult to RTL8192CE:3 RTL8192E:2
	pdvobjpriv->const_pci_aspm = 3;

	//// Setting for PCI-E device */
	pdvobjpriv->const_devicepci_aspm_setting = 0x03;

	//// Setting for PCI-E bridge */
	pdvobjpriv->const_hostpci_aspm_setting = 0x02;

	//// In Hw/Sw Radio Off situation.
	//// 0 - Default, 1 - From ASPM setting without low Mac Pwr, 
	//// 2 - From ASPM setting with low Mac Pwr, 3 - Bus D3
	//// set default to RTL8192CE:0 RTL8192SE:2
	pdvobjpriv->const_hwsw_rfoff_d3 = 0;

	//// This setting works for those device with backdoor ASPM setting such as EPHY setting.
	//// 0: Not support ASPM, 1: Support ASPM, 2: According to chipset.
	pdvobjpriv->const_support_pciaspm = 1;

	pwrpriv->reg_rfoff = 0;
	pwrpriv->rfoff_reason = 0;

_func_exit_;
}

VOID
DisableInterrupt8192CE (
	IN PADAPTER			Adapter
	)
{
	struct dvobj_priv	*pdvobjpriv=&Adapter->dvobjpriv;

	// Because 92SE now contain two DW IMR register range.
	write32(Adapter, REG_HIMR, IMR8190_DISABLED);	
	//RT_TRACE(COMP_INIT,DBG_LOUD,("***DisableInterrupt8192CE.\n"));
	// From WMAC code
	//PlatformEFIOWrite4Byte(Adapter, REG_HIMR+4,IMR, IMR8190_DISABLED);
	//RTPRINT(FISR, ISR_CHK, ("Disable IMR=%x"));

	write32(Adapter, REG_HIMRE, IMR8190_DISABLED);	// by tynli

	pdvobjpriv->irq_enabled = 0;

}

VOID
ClearInterrupt8192CE(
	IN PADAPTER			Adapter
	)
{
	u32	tmp = 0;
	tmp = read32(Adapter, REG_HISR);	
	write32(Adapter, REG_HISR, tmp);	

	tmp = 0;
	tmp = read32(Adapter, REG_HISRE);	
	write32(Adapter, REG_HISRE, tmp);	
}


VOID
EnableInterrupt8192CE(
	IN PADAPTER			Adapter
	)
{
	HAL_DATA_TYPE	*pHalData=GET_HAL_DATA(Adapter);
	struct dvobj_priv	*pdvobjpriv=&Adapter->dvobjpriv;

	pdvobjpriv->irq_enabled = 1;

	pHalData->IntrMask[0] = pHalData->IntrMaskToSet[0];
	pHalData->IntrMask[1] = pHalData->IntrMaskToSet[1];

	write32(Adapter, REG_HIMR, pHalData->IntrMask[0]&0xFFFFFFFF);

	write32(Adapter, REG_HIMRE, pHalData->IntrMask[1]&0xFFFFFFFF);

}

void
InterruptRecognized8192CE(
	IN	PADAPTER			Adapter,
	OUT	PRT_ISR_CONTENT	pIsrContent
	)
{
	HAL_DATA_TYPE	*pHalData=GET_HAL_DATA(Adapter);

	pIsrContent->IntArray[0] = read32(Adapter, REG_HISR);	
	pIsrContent->IntArray[0] &= pHalData->IntrMask[0];
	write32(Adapter, REG_HISR, pIsrContent->IntArray[0]);

	//For HISR extension. Added by tynli. 2009.10.07.
	pIsrContent->IntArray[1] = read32(Adapter, REG_HISRE);	
	pIsrContent->IntArray[1] &= pHalData->IntrMask[1];
	write32(Adapter, REG_HISRE, pIsrContent->IntArray[1]);

}

VOID
UpdateInterruptMask8192CE(
	IN	PADAPTER		Adapter,
	IN	u32		AddMSR,
	IN	u32		RemoveMSR
	)
{
	HAL_DATA_TYPE	*pHalData=GET_HAL_DATA(Adapter);

	if( AddMSR )
	{
		pHalData->IntrMaskToSet[0] |= AddMSR;
	}

	if( RemoveMSR )
	{
		pHalData->IntrMaskToSet[0] &= (~RemoveMSR);
	}

	DisableInterrupt8192CE( Adapter );
	EnableInterrupt8192CE( Adapter );
}


static VOID
HwConfigureRTL8192CE(
		IN	PADAPTER			Adapter
		)
{

	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	regBwOpMode = 0;
	u32	regRATR = 0, regRRSR = 0;


	//1 This part need to modified according to the rate set we filtered!!
	//
	// Set RRSR, RATR, and BW_OPMODE registers
	//
	switch(pHalData->CurrentWirelessMode)
	{
	case WIRELESS_MODE_B:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK;
		regRRSR = RATE_ALL_CCK;
		break;
	case WIRELESS_MODE_A:
		regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_G:
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_UNKNOWN:
	case WIRELESS_MODE_AUTO:
	case WIRELESS_MODE_N_24G:
		// It support CCK rate by default.
		// CCK rate will be filtered out only when associated AP does not support it.
		regBwOpMode = BW_OPMODE_20MHZ;
		regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
		break;
	case WIRELESS_MODE_N_5G:
		regBwOpMode = BW_OPMODE_5G;
		regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
		regRRSR = RATE_ALL_OFDM_AG;
		break;
	}

	write8(Adapter, REG_INIRTS_RATE_SEL, 0x8);

	// 2007/02/07 Mark by Emily becasue we have not verify whether this register works	
	//For 92C,which reg?
	write8(Adapter, REG_BWOPMODE, regBwOpMode);


	// Init value for RRSR.
	write32(Adapter, REG_RRSR, regRRSR);

	// Set SLOT time
	write8(Adapter,REG_SLOT, 0x09);

	// Set AMPDU min space
	write8(Adapter,REG_AMPDU_MIN_SPACE, 0x0);

	// CF-End setting.
	write16(Adapter,REG_FWHW_TXQ_CTRL, 0x1F80);

	// Set retry limit
	write16(Adapter,REG_RL, 0x0707);

	// BAR settings
	write32(Adapter, REG_BAR_MODE_CTRL, 0x0201ffff);

	// HW SEQ CTRL
	write8(Adapter,REG_HWSEQ_CTRL, 0xFF); //set 0x0 to 0xFF by tynli. Default enable HW SEQ NUM.

	// Set Data / Response auto rate fallack retry count
	write32(Adapter, REG_DARFRC, 0x01000000);
	write32(Adapter, REG_DARFRC+4, 0x07060504);
	write32(Adapter, REG_RARFRC, 0x01000000);
	write32(Adapter, REG_RARFRC+4, 0x07060504);

#if 0//cosa, for 92s
	if(	(pHalData->bt_coexist.BluetoothCoexist) &&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC4) )
	{
		PlatformEFIOWrite4Byte(Adapter, REG_AGGLEN_LMT, 0x97427431);
		RTPRINT(FBT, BT_TRACE, ("BT write 0x%x = 0x97427431\n", REG_AGGLEN_LMT));
	}
	else
#endif
	{
		// Aggregation threshold
		write32(Adapter, REG_AGGLEN_LMT, 0xb972a841);
	}
	
	// Beacon related, for rate adaptive
	write8(Adapter, REG_ATIMWND, 0x2);
#if 0 // Just set 0x55D to default 0xff. Suggested by TimChen. Marked by tynli.
	if(IS_NORMAL_CHIP(pHalData->VersionID) )
	{
		// Change  0xff to 0x0A. Advised by TimChen. 2009.01.25. by tynli.
		PlatformEFIOWrite1Byte(Adapter, REG_BCN_MAX_ERR, 0x0a);
	}
	else
#endif
	{
		// For client mode and ad hoc mode TSF setting
		write8(Adapter, REG_BCN_MAX_ERR, 0xff);
	}

	// 20100211 Joseph: Change original setting of BCN_CTRL(0x550) from 
	// 0x1e(0x2c for test chip) ro 0x1f(0x2d for test chip). Set BIT0 of this register disable ATIM
	// function. Since we do not use HIGH_QUEUE anymore, ATIM function is no longer used.
	// Also, enable ATIM function may invoke HW Tx stop operation. This may cause ping failed
	// sometimes in long run test. So just disable it now.
	if(IS_NORMAL_CHIP( pHalData->VersionID))
		pHalData->RegBcnCtrlVal = 0x1d;
		//PlatformAtomicExchange((pu4Byte)(&pHalData->RegBcnCtrlVal), 0x1d);
	else
		pHalData->RegBcnCtrlVal = 0x2d;
		//PlatformAtomicExchange((pu4Byte)(&pHalData->RegBcnCtrlVal), 0x2d);
	write8(Adapter, REG_BCN_CTRL, (u8)(pHalData->RegBcnCtrlVal));
	// Marked out by Bruce, 2010-09-09.
	// This register is configured for the 2nd Beacon (multiple BSSID). 
	// We shall disable this register if we only support 1 BSSID.
	write8(Adapter, REG_BCN_CTRL_1, 0);

	// TBTT prohibit hold time. Suggested by designer TimChen.
	write8(Adapter, REG_TBTT_PROHIBIT+1,0xff); // 8 ms

	// 20091211 Joseph: Do not set 0x551[1] suggested by Scott.
	// 
	// Disable BCNQ SUB1 0x551[1]. Suggested by TimChen. 2009.12.04. by tynli.
	// For protecting HW to decrease the TSF value when temporarily the real TSF value 
	// is smaller than the TSF counter.
	//regTmp = PlatformEFIORead1Byte(Adapter, REG_USTIME_TSF);
	//PlatformEFIOWrite1Byte(Adapter, REG_USTIME_TSF, (regTmp|BIT1)); // 8 ms

	write8(Adapter, REG_PIFS, 0x1C);
	write8(Adapter, REG_AGGR_BREAK_TIME, 0x16);

#if 0//cosa, for 92s
	if(	(pHalData->bt_coexist.BluetoothCoexist) &&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC4) )
	{
		PlatformEFIOWrite2Byte(Adapter, REG_NAV_PROT_LEN, 0x0020);
		RTPRINT(FBT, BT_TRACE, ("BT write 0x%x = 0x0020\n", REG_NAV_PROT_LEN));
		PlatformEFIOWrite2Byte(Adapter, REG_PROT_MODE_CTRL, 0x0402);
		RTPRINT(FBT, BT_TRACE, ("BT write 0x%x = 0x0402\n", REG_PROT_MODE_CTRL));
	}
	else
#endif
	{
		write16(Adapter, REG_NAV_PROT_LEN, 0x0040);
		write16(Adapter, REG_PROT_MODE_CTRL, 0x08ff);
	}

	if(!Adapter->registrypriv.wifi_spec)
	{
		//For Rx TP. Suggested by SD1 Richard. Added by tynli. 2010.04.12.
		write32(Adapter, REG_FAST_EDCA_CTRL, 0x03086666);
	}
	else
	{
		//For WiFi WMM. suggested by timchen. Added by tynli.	
		write16(Adapter, REG_FAST_EDCA_CTRL, 0x0);
	}

#if (BOARD_TYPE == FPGA_2MAC)
	// ACKTO for IOT issue.
	write8(Adapter, REG_ACKTO, 0x40);

	// Set Spec SIFS (used in NAV)
	write16(Adapter,REG_SPEC_SIFS, 0x1010);
	write16(Adapter,REG_MAC_SPEC_SIFS, 0x1010);

	// Set SIFS for CCK
	write16(Adapter,REG_SIFS_CTX, 0x1010);	

	// Set SIFS for OFDM
	write16(Adapter,REG_SIFS_TRX, 0x1010);

#else
	// ACKTO for IOT issue.
	write8(Adapter, REG_ACKTO, 0x40);

	// Set Spec SIFS (used in NAV)
	write16(Adapter,REG_SPEC_SIFS, 0x100a);
	write16(Adapter,REG_MAC_SPEC_SIFS, 0x100a);

	// Set SIFS for CCK
	write16(Adapter,REG_SIFS_CTX, 0x100a);	

	// Set SIFS for OFDM
	write16(Adapter,REG_SIFS_TRX, 0x100a);
#endif

	// Set Multicast Address. 2009.01.07. by tynli.
	write32(Adapter, REG_MAR, 0xffffffff);
	write32(Adapter, REG_MAR+4, 0xffffffff);

{
	u16	value16;
	// Accept all management frames
	value16 = 0xFFFF;
	write16(Adapter, REG_RXFLTMAP0, value16);

	//Reject all control frame - default value is 0
	write16(Adapter,REG_RXFLTMAP1,0x0);

	// Accept all data frames
	value16 = 0xFFFF;
	write16(Adapter, REG_RXFLTMAP2, value16);
}

	//For 92C,how to??
	//PlatformEFIOWrite1Byte(Adapter, MLT, 0x8f);

	// Set Contention Window here		

	// Set Tx AGC

	// Set Tx Antenna including Feedback control
		
	// Set Auto Rate fallback control
				
	//
	// For Min Spacing configuration.
	//
//1 JOSEPH_REVISE
#if 0
	Adapter->MgntInfo.MinSpaceCfg = 0x90;	//cosa, asked by scott, for MCS15 short GI, padding patch, 0x237[7:3] = 0x12.
	Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_AMPDU_MIN_SPACE, (pu1Byte)(&Adapter->MgntInfo.MinSpaceCfg));	
        switch(pHalData->RF_Type)
	{
		case RF_1T2R:
		case RF_1T1R:
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Initializeadapter: RF_Type%s\n", (pHalData->RF_Type==RF_1T1R? "(1T1R)":"(1T2R)")));
			Adapter->MgntInfo.MinSpaceCfg = (MAX_MSS_DENSITY_1T<<3);						
			break;
		case RF_2T2R:
		case RF_2T2R_GREEN:
			RT_TRACE(COMP_INIT, DBG_LOUD, ("Initializeadapter:RF_Type(2T2R)\n"));
			Adapter->MgntInfo.MinSpaceCfg = (MAX_MSS_DENSITY_2T<<3);			
			break;
	}
	PlatformEFIOWrite1Byte(Adapter, AMPDU_MIN_SPACE, Adapter->MgntInfo.MinSpaceCfg);
#endif 
}

static u32
_LLTWrite(
	IN  PADAPTER	Adapter,
	IN	u32		address,
	IN	u32		data
	)
{
	u32	status = _SUCCESS;
	s32	count = 0;
	u32	value = _LLT_INIT_ADDR(address) | _LLT_INIT_DATA(data) | _LLT_OP(_LLT_WRITE_ACCESS);

	write32(Adapter, REG_LLT_INIT, value);

	//polling
	do{

		value = read32(Adapter, REG_LLT_INIT	);
		if(_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)){
			break;
		}

		if(count > POLLING_LLT_THRESHOLD){
			DBG_8192C("Failed to polling write LLT done at address %x!\n", address);
			status = _FAIL;
			break;
		}
	}while(++count);

	return status;

}

#define LLT_CONFIG	5
static u32
LLT_table_init(
	IN PADAPTER	Adapter
	)
{
	u16	i;
	HAL_DATA_TYPE* pHalData = GET_HAL_DATA(Adapter);
	u8	txpktbuf_bndy;
	u8	maxPage;
	u32	status;

	//UCHAR	txpktbufSz = 252; //174(0xAE) 120(0x78) 252(0xFC)
#if LLT_CONFIG == 1	// Normal
	maxPage = 255;
	txpktbuf_bndy = 252;
#elif LLT_CONFIG == 2 // 2MAC FPGA
	maxPage = 127;
	txpktbuf_bndy = 124;
#elif LLT_CONFIG == 3 // WMAC
	maxPage = 255;
	txpktbuf_bndy = 174;
#elif LLT_CONFIG == 4 // WMAC
	maxPage = 255;
	txpktbuf_bndy = 246;
#elif LLT_CONFIG == 5 // tynli_test for WiFi.
	maxPage = 255;
	txpktbuf_bndy = 246;
#endif

	// Set reserved page for each queue
	// 11.	RQPN 0x200[31:0]	= 0x80BD1C1C				// load RQPN
#if LLT_CONFIG == 1
	write8(Adapter,REG_RQPN_NPQ, 0x1c);
	write32(Adapter,REG_RQPN, 0x80a71c1c);
#elif LLT_CONFIG == 2
	write32(Adapter,REG_RQPN, 0x845B1010);
#elif LLT_CONFIG == 3
	write32(Adapter,REG_RQPN, 0x84838484);
#elif LLT_CONFIG == 4
	write32(Adapter,REG_RQPN, 0x80bd1c1c);
	//write8(Adapter,REG_RQPN_NPQ, 0x1c);
#elif LLT_CONFIG == 5 // tynli_test for WiFi WMM. Suggested by TimChen
	if(IS_NORMAL_CHIP(pHalData->VersionID))
		write16(Adapter, REG_RQPN_NPQ, 0x0000);

	if(Adapter->registrypriv.wifi_spec)
		write32(Adapter,REG_RQPN, 0x80b01c29);
	else
		write32(Adapter,REG_RQPN, 0x80bf0d29);
#endif

	// 12.	TXRKTBUG_PG_BNDY 0x114[31:0] = 0x27FF00F6	//TXRKTBUG_PG_BNDY	
	write32(Adapter,REG_TRXFF_BNDY, (0x27FF0000 |txpktbuf_bndy));

	// 13.	TDECTRL[15:8] 0x209[7:0] = 0xF6				// Beacon Head for TXDMA
	write8(Adapter,REG_TDECTRL+1, txpktbuf_bndy);

	// 14.	BCNQ_PGBNDY 0x424[7:0] =  0xF6				//BCNQ_PGBNDY
	// 2009/12/03 Why do we set so large boundary. confilct with document V11.
	write8(Adapter,REG_TXPKTBUF_BCNQ_BDNY, txpktbuf_bndy);
	write8(Adapter,REG_TXPKTBUF_MGQ_BDNY, txpktbuf_bndy);

	// 15.	WMAC_LBK_BF_HD 0x45D[7:0] =  0xF6			//WMAC_LBK_BF_HD
	write8(Adapter,0x45D, txpktbuf_bndy);
	
	// Set Tx/Rx page size (Tx must be 128 Bytes, Rx can be 64,128,256,512,1024 bytes)
	// 16.	PBP [7:0] = 0x11								// TRX page size
	write8(Adapter,REG_PBP, 0x11);		

	// 17.	DRV_INFO_SZ = 0x04
	write8(Adapter,REG_RX_DRVINFO_SZ, 0x4);	

	// 18.	LLT_table_init(Adapter); 
	for(i = 0 ; i < (txpktbuf_bndy - 1) ; i++){
		status = _LLTWrite(Adapter, i , i + 1);
		if(_SUCCESS != status){
			return status;
		}
	}

	// end of list
	status = _LLTWrite(Adapter, (txpktbuf_bndy - 1), 0xFF); 
	if(_SUCCESS != status){
		return status;
	}

	// Make the other pages as ring buffer
	// This ring buffer is used as beacon buffer if we config this MAC as two MAC transfer.
	// Otherwise used as local loopback buffer. 
	for(i = txpktbuf_bndy ; i < maxPage ; i++){
		status = _LLTWrite(Adapter, i, (i + 1)); 
		if(_SUCCESS != status){
			return status;
		}
	}

	// Let last entry point to the start entry of ring buffer
	status = _LLTWrite(Adapter, maxPage, txpktbuf_bndy);
	if(_SUCCESS != status)
	{
		return status;
	}	
	
	return _SUCCESS;
	
}

//I don't kown why udelay is not enough for REG_APSD_CTRL+1
//so I add more 200 us for every udelay.
#define MORE_DELAY_VS_WIN

static u32 InitMAC(IN	PADAPTER Adapter)
{
	u8	bytetmp;
	u16	wordtmp;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct recv_priv	*precvpriv = &Adapter->recvpriv;
	struct xmit_priv	*pxmitpriv = &Adapter->xmitpriv;
	u16	retry = 0;
	u16	tmpU2b = 0;

	DBG_8192C("=======>InitMAC()\n");
	
	//2009.12.23. Added by tynli. We should disable PCI host L0s. Suggested by SD1 victorh.
	// 20100422 Joseph: Marked out. This is suggested by SD1 Glayrainx.
	// Lenovo does not like driver to control ASPM setting. All ASPM setting depends on ROOT.
	//RT_DISABLE_HOST_L0S(Adapter);
	
	// 2009/10/13 MH Enable backdoor.
	//PlatformEnable92CEBackDoor(Adapter);

	//
	// 2009/10/13 MH Add for resume sequence of power domain from document of Afred...
	// 2009/12/03 MH Modify according to power document V11. Chapter V.11.
	//	
	// 0.	RSV_CTRL 0x1C[7:0] = 0x00			// unlock ISO/CLK/Power control register
	write8(Adapter,REG_RSV_CTRL, 0x00);

#ifdef CONFIG_BT_COEXIST
	// For Bluetooth Power save
	// 209/12/03 MH The setting is not written power document now. ?????
	if(	(pHalData->bt_coexist.BT_Coexist) /*&&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC4)*/	)
	{
		u32	value32;
		value32 = read32(Adapter, REG_APS_FSMCO);
		value32 |= (SOP_ABG|SOP_AMB|XOP_BTCK);
		write32(Adapter, REG_APS_FSMCO, value32);
	}
#endif

	// 1.	AFE_XTAL_CTRL [7:0] = 0x0F		//enable XTAL
	// 2.	SPS0_CTRL 0x11[7:0] = 0x2b		//enable SPS into PWM mode
	// 3.	delay (1ms) 			//this is not necessary when initially power on
	
	// C.	Resume Sequence
	// a.	SPS0_CTRL 0x11[7:0] = 0x2b
	write8(Adapter,REG_SPS0_CTRL, 0x2b);
	
	// b.	AFE_XTAL_CTRL [7:0] = 0x0F
	write8(Adapter, REG_AFE_XTAL_CTRL, 0x0F);
	
	// c.	DRV runs power on init flow

#ifdef CONFIG_BT_COEXIST
	// Temporarily fix system hang problem.
	if(	(pHalData->bt_coexist.BT_Coexist) /*&&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC4)*/	)
	{
		u32 u4bTmp = read32(Adapter, REG_AFE_XTAL_CTRL);
	
		//AFE_XTAL_CTRL+2 0x26[1] = 1
		u4bTmp &= (~0x00024800);
		write32(Adapter, REG_AFE_XTAL_CTRL, u4bTmp);
	}
#endif

	// auto enable WLAN
	// 4.	APS_FSMCO 0x04[8] = 1; //wait till 0x04[8] = 0	
	//Power On Reset for MAC Block
	bytetmp = read8(Adapter,REG_APS_FSMCO+1) | BIT0;
	udelay_os(2);

//#ifdef MORE_DELAY_VS_WIN
//	udelay_os(200);
//#endif

	write8(Adapter,REG_APS_FSMCO+1, bytetmp);
	//DbgPrint("Reg0xEC = %x\n", read32(Adapter, 0xEC));
	udelay_os(2);

//#ifdef MORE_DELAY_VS_WIN
//	udelay_os(200);
//#endif

	// 5.	Wait while 0x04[8] == 0 goto 2, otherwise goto 1
	// 2009/12/03 MH The document V11 loop is not the same current code.
	bytetmp = read8(Adapter,REG_APS_FSMCO+1);
	//printk("Reg0xEC = %x\n", read32(Adapter, 0xEC));
	udelay_os(2);

//#ifdef MORE_DELAY_VS_WIN
//	mdelay_os(1);
//#endif

	retry = 0;
	//DBG_8192C("################>%s()Reg0xEC:%x:%x\n", __FUNCTION__,read32(Adapter, 0xEC),bytetmp);
	while((bytetmp & BIT0) && retry < 1000){
		retry++;
		udelay_os(50);
		bytetmp = read8(Adapter,REG_APS_FSMCO+1);
		//DBG_8192C("################>%s()Reg0xEC:%x:%x\n", __FUNCTION__,read32(Adapter, 0xEC),bytetmp);
		udelay_os(50);
	}

	// Enable Radio off, GPIO, and LED function
	// 6.	APS_FSMCO 0x04[15:0] = 0x0012		//when enable HWPDN
	write16(Adapter, REG_APS_FSMCO, 0x1012); //tynli_test to 0x1012. SD1. 2009.12.08.

	 // release RF digital isolation
	 // 7.	SYS_ISO_CTRL 0x01[1]	= 0x0;
	//bytetmp = PlatformEFIORead1Byte(Adapter, REG_SYS_ISO_CTRL+1) & ~BIT1; //marked by tynli.
	//PlatformSleepUs(2);
	//Set REG_SYS_ISO_CTRL 0x1=0x82 to prevent wake# problem. Suggested by Alfred.
	// 2009/12/03 MH We need to check this modification

	tmpU2b = read16(Adapter,REG_SYS_ISO_CTRL);

	//if(IS_HARDWARE_TYPE_8723E(Adapter))
	//	write8(Adapter, REG_SYS_ISO_CTRL, (tmpU2b|ISO_DIOR|PWC_EV12V));
	//else
		write8(Adapter, REG_SYS_ISO_CTRL+1, 0x82);

	udelay_os(2);		// By experience!!??

#ifdef CONFIG_BT_COEXIST
	// Enable MAC DMA/WMAC/SCHEDULE block
	// 8.	AFE_XTAL_CTRL [17] = 0;			//with BT, driver will set in card disable
	if(	(pHalData->bt_coexist.BT_Coexist) /*&&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC4)	*/) //tynli_test 2009.12.16.
	{
		bytetmp = read8(Adapter, REG_AFE_XTAL_CTRL+2) & 0xfd;
		write8(Adapter, REG_AFE_XTAL_CTRL+2, bytetmp);
	}
#endif

	// Release MAC IO register reset
	// 9.	CR 0x100[7:0]	= 0xFF;
	// 10.	CR 0x101[1]	= 0x01; // Enable SEC block
	write16(Adapter,REG_CR, 0x2ff);

	//WritePortUchar(0x553, 0xff);  //reset ??? richard 0507
	// What is the meaning???
	//PlatformEFIOWrite1Byte(Adapter,0x553, 0xFF);	
	
	//PlatformSleepUs(1500);
	//bytetmp = PlatformEFIORead1Byte(Adapter,REG_CR);
	//bytetmp |= (BIT6 | BIT7);//(CmdTxEnb|CmdRxEnb);
	//PlatformEFIOWrite1Byte(Adapter,REG_CR, bytetmp);

	// 2009/12/03 MH The section of initialize code does not exist in the function~~~!
	// These par is inserted into function LLT_table_init
	// 11.	RQPN 0x200[31:0]	= 0x80BD1C1C				// load RQPN
	// 12.	TXRKTBUG_PG_BNDY 0x114[31:0] = 0x27FF00F6	//TXRKTBUG_PG_BNDY
	// 13.	TDECTRL[15:8] 0x209[7:0] = 0xF6				// Beacon Head for TXDMA
	// 14.	BCNQ_PGBNDY 0x424[7:0] =  0xF6				//BCNQ_PGBNDY
	// 15.	WMAC_LBK_BF_HD 0x45D[7:0] =  0xF6			//WMAC_LBK_BF_HD
	// 16.	PBP [7:0] = 0x11								// TRX page size
	// 17.	DRV_INFO_SZ = 0x04
	
	//System init
	// 18.	LLT_table_init(Adapter); 
	if(LLT_table_init(Adapter) == _FAIL)
	{
		return _FAIL;
	}

	// Clear interrupt and enable interrupt
	// 19.	HISR 0x124[31:0] = 0xffffffff; 
	//	   	HISRE 0x12C[7:0] = 0xFF
	// NO 0x12c now!!!!!
	write32(Adapter,REG_HISR, 0xffffffff);
	write8(Adapter,REG_HISRE, 0xff);
	
	// 20.	HIMR 0x120[31:0] |= [enable INT mask bit map]; 
	// 21.	HIMRE 0x128[7:0] = [enable INT mask bit map]
	// The IMR should be enabled later after all init sequence is finished.

	// ========= PCIE related register setting =======
	// 22.	PCIE configuration space configuration
	// 23.	Ensure PCIe Device 0x80[15:0] = 0x0143 (ASPM+CLKREQ), 
	//          and PCIe gated clock function is enabled.	
	// PCIE configuration space will be written after all init sequence.(Or by BIOS)


	// Set Rx FF0 boundary : 9K/10K
	// 2009/12/03 MH This should be relative to HW setting. But he power document does
	// not contain the description.
	write16(Adapter,REG_TRXFF_BNDY+2, 0x27ff);

	//
	// 2009/12/03 MH THe below section is not related to power document Vxx .
	// This is only useful for driver and OS setting.
	//
	// -------------------Software Relative Setting----------------------
	//
	
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		wordtmp = read16(Adapter,REG_TRXDMA_CTRL);
		wordtmp &= 0xf;
		wordtmp |= 0xF771;
		write16(Adapter,REG_TRXDMA_CTRL, wordtmp);
	}
	else
	{
		// Set High priority queue select : HPQ:BC/H/VO/VI/MG, LPQ:BE/BK
		// [5]:H, [4]:MG, [3]:BK, [2]:BE, [1]:VI, [0]:VO
		write16(Adapter,REG_TRXDMA_CTRL, 0x3501);
	}
	
	// Reported Tx status from HW for rate adaptive.
	// 2009/12/03 MH This should be realtive to power on step 14. But in document V11  
	// still not contain the description.!!!
	write8(Adapter,REG_FWHW_TXQ_CTRL+1, 0x1F);

	// Set RCR register
	write32(Adapter,REG_RCR, pHalData->ReceiveConfig);
	// Set TCR register
	write32(Adapter,REG_TCR, pHalData->TransmitConfig);

	// disable earlymode
	write8(Adapter,0x4d0, 0x0);
		
	//
	// Set TX/RX descriptor physical address(from OS API).
	//
	write32(Adapter, REG_BCNQ_DESA, (u64)pxmitpriv->tx_ring[BCN_QUEUE_INX].dma & DMA_BIT_MASK(32));
	write32(Adapter, REG_MGQ_DESA, (u64)pxmitpriv->tx_ring[MGT_QUEUE_INX].dma & DMA_BIT_MASK(32));
	write32(Adapter, REG_VOQ_DESA, (u64)pxmitpriv->tx_ring[VO_QUEUE_INX].dma & DMA_BIT_MASK(32));
	write32(Adapter, REG_VIQ_DESA, (u64)pxmitpriv->tx_ring[VI_QUEUE_INX].dma & DMA_BIT_MASK(32));
	write32(Adapter, REG_BEQ_DESA, (u64)pxmitpriv->tx_ring[BE_QUEUE_INX].dma & DMA_BIT_MASK(32));
	write32(Adapter, REG_BKQ_DESA, (u64)pxmitpriv->tx_ring[BK_QUEUE_INX].dma & DMA_BIT_MASK(32));
	write32(Adapter, REG_HQ_DESA, (u64)pxmitpriv->tx_ring[HIGH_QUEUE_INX].dma & DMA_BIT_MASK(32));
	write32(Adapter, REG_RX_DESA, (u64)precvpriv->rx_ring[RX_MPDU_QUEUE].dma & DMA_BIT_MASK(32));

#ifdef CONFIG_64BIT_DMA
	// 2009/10/28 MH For DMA 64 bits. We need to assign the high 32 bit address
	// for NIC HW to transmit data to correct path.
	write32(Adapter, REG_BCNQ_DESA+4, 
		((u64)pxmitpriv->tx_ring[BCN_QUEUE_INX].dma)>>32);
	write32(Adapter, REG_MGQ_DESA+4, 
		((u64)pxmitpriv->tx_ring[MGT_QUEUE_INX].dma)>>32);  
	write32(Adapter, REG_VOQ_DESA+4, 
		((u64)pxmitpriv->tx_ring[VO_QUEUE_INX].dma)>>32);
	write32(Adapter, REG_VIQ_DESA+4, 
		((u64)pxmitpriv->tx_ring[VI_QUEUE_INX].dma)>>32);
	write32(Adapter, REG_BEQ_DESA+4, 
		((u64)pxmitpriv->tx_ring[BE_QUEUE_INX].dma)>>32);
	write32(Adapter, REG_BKQ_DESA+4,
		((u64)pxmitpriv->tx_ring[BK_QUEUE_INX].dma)>>32);
	write32(Adapter,REG_HQ_DESA+4,
		((u64)pxmitpriv->tx_ring[HIGH_QUEUE_INX].dma)>>32);
	write32(Adapter, REG_RX_DESA+4, 
		((u64)precvpriv->rx_ring[RX_MPDU_QUEUE].dma)>>32);


	// 2009/10/28 MH If RX descriptor address is not equal to zero. We will enable
	// DMA 64 bit functuion.
	// Note: We never saw thd consition which the descripto address are divided into
	// 4G down and 4G upper seperate area.
	if (((u64)precvpriv->rx_ring[RX_MPDU_QUEUE].dma)>>32 != 0)
	{
		//DBG_8192C("RX_DESC_HA=%08lx\n", ((u64)priv->rx_ring_dma[RX_MPDU_QUEUE])>>32);
		DBG_8192C("Enable DMA64 bit\n");

		// Check if other descriptor address is zero and abnormally be in 4G lower area.
		if (((u64)pxmitpriv->tx_ring[MGT_QUEUE_INX].dma)>>32)
		{
			DBG_8192C("MGNT_QUEUE HA=0\n");
		}
		
		PlatformEnable92CEDMA64(Adapter);
	}
	else
	{
		DBG_8192C("Enable DMA32 bit\n");
	}
#endif

	// 2009/12/03 MH This should be included in power
	// Set to 0x22. Suggested to SD1 Alan. by tynli. 2009.12.10.
	// 20100317 Joseph: Resume the setting of Tx/Rx DMA burst size(Reg0x303) to 0x77 suggested by SD1.
	// 20100324 Joseph: Set different value for Tx/Rx DMA burst size(Reg0x303) suggested by SD1.
	// 92CE set to 0x77 and 88CE set to 0x22.
	if(IS_92C_SERIAL(pHalData->VersionID))
		write8(Adapter,REG_PCIE_CTRL_REG+3, 0x77);
	else
		write8(Adapter,REG_PCIE_CTRL_REG+3, 0x22);

	// 20100318 Joseph: Reset interrupt migration setting when initialization. Suggested by SD1.
	write32(Adapter, REG_INT_MIG, 0);	
	pHalData->bInterruptMigration = _FALSE;

	// 20090928 Joseph: Add temporarily.
	// Reconsider when to do this operation after asking HWSD.
	bytetmp = read8(Adapter, REG_APSD_CTRL);
	write8(Adapter, REG_APSD_CTRL, bytetmp & ~BIT6);
	do{
		retry++;
		bytetmp = read8(Adapter, REG_APSD_CTRL);
	}while((retry<200) && (bytetmp&BIT7)); //polling until BIT7 is 0. by tynli

	// 2009/10/26 MH For led test.
	// After MACIO reset,we must refresh LED state.
	rtl8192ce_gen_RefreshLedState(Adapter);

	//2009.10.19. Reset H2C protection register. by tynli.
	write32(Adapter, REG_MCUTST_1, 0x0);

#if MP_DRIVER == 1
	write32(Adapter, REG_MACID, 0x87654321);
	write32(Adapter, 0x0700, 0x87654321);
#endif

	//
	// -------------------Software Relative Setting----------------------
	//

	DBG_8192C("<=======InitMAC()\n");

	return _SUCCESS;
	
}

static VOID
EnableAspmBackDoor92CE(IN	PADAPTER Adapter)
{
	struct pwrctrl_priv		*pwrpriv = &Adapter->pwrctrlpriv;

	// 0x70f BIT7 is used to control L0S
	// 20100212 Tynli: Set register offset 0x70f in PCI configuration space to the value 0x23 
	// for all bridge suggested by SD1. Origianally this is only for INTEL.
	// 20100422 Joseph: Set PCI configuration space offset 0x70F to 0x93 to Enable L0s for all platform.
	// This is suggested by SD1 Glayrainx and for Lenovo's request.
	//if(GetPciBridgeVendor(Adapter) == PCI_BRIDGE_VENDOR_INTEL)
		write8(Adapter, 0x34b, 0x93);
	//else
	//	PlatformEFIOWrite1Byte(Adapter, 0x34b, 0x23);
	write16(Adapter, 0x350, 0x870c);
	write8(Adapter, 0x352, 0x1);

	// 0x719 Bit3 is for L1 BIT4 is for clock request 
	// 20100427 Joseph: Disable L1 for Toshiba AMD platform. If AMD platform do not contain
	// L1 patch, driver shall disable L1 backdoor.
	if(pwrpriv->b_support_backdoor)
		write8(Adapter, 0x349, 0x1b);
	else
		write8(Adapter, 0x349, 0x03);
	write16(Adapter, 0x350, 0x2718);
	write8(Adapter, 0x352, 0x1);
}

static u32 rtl8192ce_hal_init(PADAPTER Adapter)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	EEPROM_EFUSE_PRIV	*pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	struct pwrctrl_priv		*pwrpriv = &Adapter->pwrctrlpriv;
	u32	rtStatus = _SUCCESS;
	u8	tmpU1b;
	u8	eRFPath;
	u32	i;
	BOOLEAN bSupportRemoteWakeUp;
	BOOLEAN	 isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	BOOLEAN is92C = IS_92C_SERIAL(pHalData->VersionID);
_func_enter_;

	//
	// No I/O if device has been surprise removed
	//
	if (Adapter->bSurpriseRemoved)
	{
		DBG_8192C("rtl8192ce_hal_init(): bSurpriseRemoved!\n");
		return _SUCCESS;
	}

	DBG_8192C("=======>rtl8192ce_hal_init()\n");

	rtl8192ce_reset_desc_ring(Adapter);

	//
	// 1. MAC Initialize
	//
	rtStatus = InitMAC(Adapter);
	if(rtStatus != _SUCCESS)
	{
		DBG_8192C("Init MAC failed\n");
		return rtStatus;
	}

#if (MP_DRIVER != 1)
#if HAL_FW_ENABLE
	rtStatus = FirmwareDownload92C(Adapter);
	if(rtStatus != _SUCCESS)
	{
		DBG_8192C("FwLoad failed\n");
		rtStatus = _SUCCESS;
		Adapter->bFWReady = _FALSE;
		pHalData->fw_ractrl = _FALSE;
	}
	else
	{
		DBG_8192C("FwLoad SUCCESSFULLY!!!\n");
		Adapter->bFWReady = _TRUE;
		pHalData->fw_ractrl = _TRUE;
	}

	// Init Fw LPS related.
	Adapter->pwrctrlpriv.bFwCurrentInPSMode = _FALSE;

	//Init H2C counter. by tynli. 2009.12.09.
	pHalData->LastHMEBoxNum = 0;
#endif
#endif

// 20100318 Joseph: These setting only for FPGA.
// Add new type "ASIC" and set RFChipID and RF_Type in ReadAdapter function.
#if BOARD_TYPE==FPGA_2MAC
	pHalData->rf_chip = RF_PSEUDO_11N;
	pHalData->rf_type = RF_2T2R;
#elif BOARD_TYPE==FPGA_PHY
	#if FPGA_RF==FPGA_RF_8225
		pHalData->rf_chip = RF_8225;
		pHalData->rf_type = RF_2T2R;
	#elif FPGA_RF==FPGA_RF_0222D
		pHalData->rf_chip = RF_6052;
		pHalData->rf_type = RF_2T2R;
	#endif
#endif

	//
	// 2. Initialize MAC/PHY Config by MACPHY_reg.txt
	//
#if (HAL_MAC_ENABLE == 1)
	DBG_8192C("MAC Config Start!\n");
	rtStatus = PHY_MACConfig8192C(Adapter);
	if (rtStatus != _SUCCESS)
	{
		DBG_8192C("MAC Config failed\n");
		return rtStatus;
	}
	DBG_8192C("MAC Config Finished!\n");

	//improve 2-stream TX EVM by Jenyu
	if(isNormal && is92C)
		write8(Adapter, 0x14,0x71);

	//CLEAR ADF , for using RX_FILTER_MAP.
	write32(Adapter, REG_RCR, read32(Adapter, REG_RCR) & ~RCR_ADF);
#endif	// #if (HAL_MAC_ENABLE == 1)

	//
	// 3. Initialize BB After MAC Config PHY_reg.txt, AGC_Tab.txt
	//
#if (HAL_BB_ENABLE == 1)
	DBG_8192C("BB Config Start!\n");
	rtStatus = PHY_BBConfig8192C(Adapter);
	if (rtStatus!= _SUCCESS)
	{
		DBG_8192C("BB Config failed\n");
		return rtStatus;
	}
	DBG_8192C("BB Config Finished!\n");
#endif	// #if (HAL_BB_ENABLE == 1)

	//
	// 4. Initiailze RF RAIO_A.txt RF RAIO_B.txt
	//
	// 2007/11/02 MH Before initalizing RF. We can not use FW to do RF-R/W.
	//pHalData->Rf_Mode = RF_OP_By_SW_3wire;
#if (HAL_RF_ENABLE == 1)		
	DBG_8192C("RF Config started!\n");
	rtStatus = PHY_RFConfig8192C(Adapter);
	if(rtStatus != _SUCCESS)
	{
		DBG_8192C("RF Config failed\n");
		return rtStatus;
	}
	DBG_8192C("RF Config Finished!\n");

	if(IS_VENDOR_UMC_A_CUT(pHalData->VersionID) && !IS_92C_SERIAL(pHalData->VersionID))
	{
		PHY_SetRFReg(Adapter, RF90_PATH_A, RF_RX_G1, bMaskDWord, 0x30255);
		PHY_SetRFReg(Adapter, RF90_PATH_A, RF_RX_G2, bMaskDWord, 0x50a00);		
	}
	

	// 20100329 Joseph: Restore RF register value for later use in channel switching.
	pHalData->RfRegChnlVal[0] = PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E)0, RF_CHNLBW, bRFRegOffsetMask);
	pHalData->RfRegChnlVal[1] = PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E)1, RF_CHNLBW, bRFRegOffsetMask);
#endif	// #if (HAL_RF_ENABLE == 1)	

	// After read predefined TXT, we must set BB/MAC/RF register as our requirement
	/*---- Set CCK and OFDM Block "ON"----*/
	PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bCCKEn, 0x1);
	PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bOFDMEn, 0x1);
#if (MP_DRIVER == 0)
	// Set to 20MHz by default
	PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10, 1);
#endif

	pHalData->CurrentWirelessMode = WIRELESS_MODE_AUTO;

	//3 Set Hardware(MAC default setting.)
	HwConfigureRTL8192CE(Adapter);

	//3 Set Wireless Mode
	// TODO: Emily 2006.07.13. Wireless mode should be set according to registry setting and RF type
	//Default wireless mode is set to "WIRELESS_MODE_N_24G|WIRELESS_MODE_G", 
	//and the RRSR is set to Legacy OFDM rate sets. We do not include the bit mask 
	//of WIRELESS_MODE_B currently. Emily, 2006.11.13
	//For wireless mode setting from mass. 
	//if(Adapter->ResetProgress == RESET_TYPE_NORESET)
	//	Adapter->HalFunc.SetWirelessModeHandler(Adapter, Adapter->RegWirelessMode);

	//3Security related
	//-----------------------------------------------------------------------------
	// Set up security related. 070106, by rcnjko:
	// 1. Clear all H/W keys.
	// 2. Enable H/W encryption/decryption.
	//-----------------------------------------------------------------------------
	// 92SE not enable security now
	{
		u8 SECR_value = 0x0;

		invalidate_cam_all(Adapter);

		if(IS_NORMAL_CHIP(pHalData->VersionID))
			SECR_value |= (BIT6|BIT7);

		// Joseph debug: MAC_SEC_EN need to be set
		write8(Adapter, REG_CR+1, 0x02);

		write8(Adapter, REG_SECCFG, SECR_value);
	}

	pHalData->CurrentChannel = 6;//default set to 6

	/* Write correct tx power index */
	PHY_SetTxPowerLevel8192C(Adapter, pHalData->CurrentChannel);

	//2=======================================================
	// RF Power Save
	//2=======================================================
#if 1
	// Fix the bug that Hw/Sw radio off before S3/S4, the RF off action will not be executed 
	// in MgntActSet_RF_State() after wake up, because the value of pHalData->eRFPowerState 
	// is the same as eRfOff, we should change it to eRfOn after we config RF parameters.
	// Added by tynli. 2010.03.30.
	pwrpriv->rf_pwrstate = rf_on;

	// 20100326 Joseph: Copy from GPIOChangeRFWorkItemCallBack() function to check HW radio on/off.
	// 20100329 Joseph: Revise and integrate the HW/SW radio off code in initialization.
	tmpU1b = read8(Adapter, REG_MAC_PINMUX_CFG)&(~BIT3);
	write8(Adapter, REG_MAC_PINMUX_CFG, tmpU1b);
	tmpU1b = read8(Adapter, REG_GPIO_IO_SEL);
	DBG_8192C("GPIO_IN=%02x\n", tmpU1b);
	pwrpriv->rfoff_reason |= (tmpU1b & BIT3) ? 0 : RF_CHANGE_BY_HW;
	pwrpriv->rfoff_reason |= (pwrpriv->reg_rfoff) ? RF_CHANGE_BY_SW : 0;

	if(pwrpriv->rfoff_reason & RF_CHANGE_BY_HW)
		pwrpriv->b_hw_radio_off = _TRUE;

	if(pwrpriv->rfoff_reason > RF_CHANGE_BY_PS)
	{ // H/W or S/W RF OFF before sleep.
		DBG_8192C("InitializeAdapter8192CE(): Turn off RF for RfOffReason(%d) ----------\n", pwrpriv->rfoff_reason);
		//MgntActSet_RF_State(Adapter, rf_off, pwrpriv->rfoff_reason, _TRUE);
	}
	else
	{
		pwrpriv->rf_pwrstate = rf_on;
		pwrpriv->rfoff_reason = 0;

		DBG_8192C("InitializeAdapter8192CE(): Turn on  ----------\n");

		// LED control
		Adapter->ledpriv.LedControlHandler(Adapter, LED_CTL_POWER_ON);

		//
		// If inactive power mode is enabled, disable rf while in disconnected state.
		// But we should still tell upper layer we are in rf on state.
		// 2007.07.16, by shien chang.
		//
		//if(!Adapter->bInHctTest)
			//IPSEnter(Adapter);
	}
#endif

	// Fix the bug that when the system enters S3/S4 then tirgger HW radio off, after system
	// wakes up, the scan OID will be set from upper layer, but we still in RF OFF state and scan
	// list is empty, such that the system might consider the NIC is in RF off state and will wait 
	// for several seconds (during this time the scan OID will not be set from upper layer anymore)
	// even though we have already HW RF ON, so we tell the upper layer our RF state here.
	// Added by tynli. 2010.04.01.
	//DrvIFIndicateCurrentPhyStatus(Adapter);

	//
	// Execute TX power tracking later
	//

	// We must set MAC address after firmware download. HW do not support MAC addr
	// autoload now.
	for(i=0; i<6; i++)
	{
		write8(Adapter, (REG_MACID+i), pEEPROM->mac_addr[i]);		 	
	}

	// Joseph. Turn on the secret lock of ASPM.
	EnableAspmBackDoor92CE(Adapter);

#ifdef CONFIG_BT_COEXIST
	_InitBTCoexist(Adapter);
#endif

	rtl8192c_InitHalDm(Adapter);

	//EnableInterrupt8192CE(Adapter);

#if (MP_DRIVER == 1)
	//MPT_InitializeAdapter(Adapter, Channel);
#else
	// 20100329 Joseph: Disable te Caliberation operation when Radio off.
	// This prevent from outputing signal when initialization in Radio-off state.
	if(pwrpriv->rf_pwrstate == rf_on)
	{
		rtl8192c_PHY_SetRFPathSwitch(Adapter, pHalData->bDefaultAntenna);	//Wifi default use Main

		if(pHalData->bIQKInitialized )
			rtl8192c_PHY_IQCalibrate(Adapter, _TRUE);
		else
		{
			rtl8192c_PHY_IQCalibrate(Adapter, _FALSE);
			pHalData->bIQKInitialized = _TRUE;
		}		
		
		rtl8192c_dm_CheckTXPowerTracking(Adapter);
		rtl8192c_PHY_LCCalibrate(Adapter);
	}
#endif

#if 0
	//WoWLAN setting. by tynli.
	Adapter->HalFunc.GetHalDefVarHandler(Adapter, HAL_DEF_WOWLAN , &bSupportRemoteWakeUp);
	if(bSupportRemoteWakeUp) // WoWLAN setting. by tynli.
	{
		u8	u1bTmp;
		u8	i;
#if 0
		u4Byte	u4bTmp;

		//Disable L2 support
		u4bTmp = PlatformEFIORead4Byte(Adapter, REG_PCIE_CTRL_REG);
		u4bTmp &= ~(BIT17);
		 PlatformEFIOWrite4Byte(Adapter, REG_PCIE_CTRL_REG, u4bTmp);
#endif

		// enable Rx DMA. by tynli.
		u1bTmp = read8(Adapter, REG_RXPKT_NUM+2);
		u1bTmp &= ~(BIT2);
		write8(Adapter, REG_RXPKT_NUM+2, u1bTmp);

		if(pPSC->WoWLANMode == eWakeOnMagicPacketOnly)
		{
			//Enable magic packet and WoWLAN function in HW.
			write8(Adapter, REG_WOW_CTRL, WOW_MAGIC);
		}
		else if (pPSC->WoWLANMode == eWakeOnPatternMatchOnly)
		{
			//Enable pattern match and WoWLAN function in HW.
			write8(Adapter, REG_WOW_CTRL, WOW_WOMEN);
		}
		else if (pPSC->WoWLANMode == eWakeOnBothTypePacket)
		{
			//Enable magic packet, pattern match, and WoWLAN function in HW.
			write8(Adapter, REG_WOW_CTRL, WOW_MAGIC|WOW_WOMEN); 
		}
		
		PlatformClearPciPMEStatus(Adapter);

		if(ADAPTER_TEST_STATUS_FLAG(Adapter, ADAPTER_STATUS_FIRST_INIT))
		{
			//Reset WoWLAN register and related data structure at the first init. 2009.06.18. by tynli.
			ResetWoLPara(Adapter);
		}
		else
		{
			if(pPSC->WoWLANMode > eWakeOnMagicPacketOnly)
			{
				//Rewrite WOL pattern and mask to HW.
				for(i=0; i<(MAX_SUPPORT_WOL_PATTERN_NUM-2); i++)
				{
					Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_WF_MASK, (pu1Byte)(&i)); 
					Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_WF_CRC, (pu1Byte)(&i)); 
				}
			}
		}
	}
#endif

	tmpU1b = EFUSE_Read1Byte(Adapter, 0x1FA);

	if(!(tmpU1b & BIT0))
	{
		PHY_SetRFReg(Adapter, RF90_PATH_A, 0x15, 0x0F, 0x05);
		DBG_8192C("PA BIAS path A\n");
	}	

	if(!(tmpU1b & BIT1) && isNormal && is92C)
	{
		PHY_SetRFReg(Adapter, RF90_PATH_B, 0x15, 0x0F, 0x05);
		DBG_8192C("PA BIAS path B\n");
	}

	if(!(tmpU1b & BIT4))
	{
		tmpU1b = read8(Adapter, 0x16);
		tmpU1b &= 0x0F; 
		write8(Adapter, 0x16, tmpU1b | 0x80);
		udelay_os(10);
		write8(Adapter, 0x16, tmpU1b | 0x90);
		DBG_8192C("under 1.5V\n");
	}

/*{
	printk("===== Start Dump Reg =====");
	for(i = 0 ; i <= 0xeff ; i+=4)
	{
		if(i%16==0)
			printk("\n%04x: ",i);
		printk("0x%08x ",read32(Adapter, i));
	}
	printk("\n ===== End Dump Reg =====\n");
}*/

_func_exit_;

	return rtStatus;
}

//
// 2009/10/13 MH Acoording to documetn form Scott/Alfred....
// This is based on version 8.1.
//
static VOID
PowerOffAdapter8192CE(
	IN	PADAPTER			Adapter	
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	u1bTmp;
	u32	u4bTmp;
	
	DBG_8192C("=======>PowerOffAdapter8192CE()\n");
	// XVI.1.1 PCIe Card Disable
	
	// A.	Target 

	// a.	WLAN all disable (RF off, A15 off, DCORE reset)
	// b.	MCU reset
	// c.	A33 disable (AFE bandgap and m-bias on, others disable)
	// d.	XTAL off
	// e.	PON on
	// f.		HCI D3 mode (the same as S3 state)
	// g.	REG can be accessed by host. Resume by register control.


	// B.	Off Sequence 

	//
	// 2009/10/13 MH Refer to document RTL8191C power sec V8.1 sequence.
	// Chapter 6.1 for power card disable.
	//
	// A. Ensure PCIe Device 0x80[15:0] = 0x0143 (ASPM+CLKREQ), and PCIe gated 
	// clock function is enabled.
	rtw_pci_enable_aspm(Adapter);

	//==== RF Off Sequence ====
	// a.	TXPAUSE 0x522[7:0] = 0xFF			//Pause MAC TX queue
	write8(Adapter, REG_TXPAUSE, 0xFF);
	
	// b.	RF path 0 offset 0x00 = 0x00		// disable RF
	//PlatformEFIOWrite1Byte(Adapter, 0x00, 0x00);
	PHY_SetRFReg(Adapter, RF90_PATH_A, 0x00, bRFRegOffsetMask, 0x00);
	//tynli_test. by sd1 Jonbon. Turn off then resume, system will restart.
	write8(Adapter, REG_RF_CTRL, 0x00);

	// c.	APSD_CTRL 0x600[7:0] = 0x40
	write8(Adapter, REG_APSD_CTRL, 0x40);

	// d.	SYS_FUNC_EN 0x02[7:0] = 0xE2		//reset BB state machine
	write8(Adapter, REG_SYS_FUNC_EN, 0xE2);

	// e.		SYS_FUNC_EN 0x02[7:0] = 0xE0		//reset BB state machine
	write8(Adapter, REG_SYS_FUNC_EN, 0xE0);

	//  ==== Reset digital sequence   ======
	
	if((read8(Adapter, REG_MCUFWDL)&BIT7) && 
		Adapter->bFWReady) //8051 RAM code
	{	
		rtl8192c_FirmwareSelfReset(Adapter);
	}
	
	// f.	SYS_FUNC_EN 0x03[7:0]=0x51		// reset MCU, MAC register, DCORE
	write8(Adapter, REG_SYS_FUNC_EN+1, 0x51);

	// g.	MCUFWDL 0x80[1:0]=0				// reset MCU ready status
	write8(Adapter, REG_MCUFWDL, 0x00);
	
	//  ==== Pull GPIO PIN to balance level and LED control ======

	// h.		GPIO_PIN_CTRL 0x44[31:0]=0x000		// 
	write32(Adapter, REG_GPIO_PIN_CTRL, 0x00000000);

	// i.		Value = GPIO_PIN_CTRL[7:0]
	u1bTmp = read8(Adapter, REG_GPIO_PIN_CTRL);

#ifdef CONFIG_BT_COEXIST
	if((pHalData->bt_coexist.BT_Coexist) &&
		((pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC4)||
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC8)))
	{ // when BT COEX exist
		//j. GPIO_PIN_CTRL 0x44[31:0] = 0x00F30000 | (value <<8); //write external PIN level 
		write32(Adapter, REG_GPIO_PIN_CTRL, 0x00F30000| (u1bTmp <<8));
	}
	else
#endif
	{ //Without BT COEX
		//j.	GPIO_PIN_CTRL 0x44[31:0] = 0x00FF0000 | (value <<8); //write external PIN level 
		write32(Adapter, REG_GPIO_PIN_CTRL, 0x00FF0000| (u1bTmp <<8));
	}


	// k.	GPIO_MUXCFG 0x42 [15:0] = 0x0780
	write16(Adapter, REG_GPIO_IO_SEL, 0x0790);

	// l.	LEDCFG 0x4C[15:0] = 0x8080
	if(pHalData->InterfaceSel == INTF_SEL1_BT_COMBO_MINICARD)
	{
		write32(Adapter, REG_LEDCFG0, 0x00028080);	
		DBG_8192C("poweroffadapter 0x4c 0x%x\n", read32(Adapter, REG_LEDCFG0));
	}
	else
		write16(Adapter, REG_LEDCFG0, 0x8080);

	//  ==== Disable analog sequence ===

	// m.	AFE_PLL_CTRL[7:0] = 0x80			//disable PLL
	write8(Adapter, REG_AFE_PLL_CTRL, 0x80);

	// n.	SPS0_CTRL 0x11[7:0] = 0x22			//enter PFM mode
	write8(Adapter, REG_SPS0_CTRL, 0x23);

#ifdef CONFIG_BT_COEXIST
	if(	(pHalData->bt_coexist.BT_Coexist) /*&&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC4)	*/)
	{ // when BT COEX exist
		u4bTmp = read32(Adapter, REG_AFE_XTAL_CTRL);

		//AFE_XTAL_CTRL+2 0x26[9:7] = 3b'111
		u4bTmp |= 0x03800000;
		write32(Adapter, REG_AFE_XTAL_CTRL, u4bTmp);

		//AFE_XTAL_CTRL+2 0x26[1] = 1
		u4bTmp |= 0x00020000;
		write32(Adapter, REG_AFE_XTAL_CTRL, u4bTmp);

		//AFE_XTAL_CTRL 0x24[14] = 1
		u4bTmp |= 0x00004000;
		write32(Adapter, REG_AFE_XTAL_CTRL, u4bTmp);

		//AFE_XTAL_CTRL 0x24[11] = 1
		u4bTmp |= 0x00000800;
		write32(Adapter, REG_AFE_XTAL_CTRL, u4bTmp);
	}
	else
#endif
	{
		// o.	AFE_XTAL_CTRL 0x24[7:0] = 0x0E		// disable XTAL, if No BT COEX
		write8(Adapter, REG_AFE_XTAL_CTRL, 0x0e);
	}
	
	// p.	RSV_CTRL 0x1C[7:0] = 0x0E			// lock ISO/CLK/Power control register
	write8(Adapter, REG_RSV_CTRL, 0x0e);
	
	//  ==== interface into suspend ===

	// q.	APS_FSMCO[15:8] = 0x58				// PCIe suspend mode
	//PlatformEFIOWrite1Byte(Adapter, REG_APS_FSMCO+1, 0x58);
	// by tynli. Suggested by SD1.
	// According to power document V11, we need to set this value as 0x18. Otherwise, we
	// may not L0s sometimes. This indluences power consumption. Bases on SD1's test,
	// set as 0x00 do not affect power current. And if it is set as 0x18, they had ever
	// met auto load fail problem. 2009/12/03 MH/Tylin/Alan add the description.
	write8(Adapter, REG_APS_FSMCO+1, 0x10); //tynli_test. SD1 2009.12.08.


	// r.	Note: for PCIe interface, PON will not turn off m-bias and BandGap 
	// in PCIe suspend mode. 


	// 2009/10/16 MH SD1 Victor need the test for isolation.
	// tynli_test set BIT0 to 1. moved to shutdown
	//write8(Adapter, 0x0, PlatformEFIORead1Byte(Adapter, 0x0)|BIT0);
	

	// 2009/10/13 MH Disable 92SE card disable sequence.
	DBG_8192C("<=======PowerOffAdapter8192CE()\n");
	
}	// PowerOffAdapter8192CE

//
// Description: For WoWLAN, when D0 support PME, we should clear PME status from 0x81
//			to 0x01 to prevent S3/S4 hang. Suggested by SD1 Jonbon/Isaac.
//
//	2009.04. by tynli.
VOID
PlatformClearPciPMEStatus(
	IN		PADAPTER		Adapter	
)
{
	struct dvobj_priv	*pdvobjpriv = &Adapter->dvobjpriv;
	struct pci_dev 	*pdev = pdvobjpriv->ppcidev;
	u8	value_offset, value, tmp;

	pci_read_config_byte(pdev, 0x34, &value_offset);

	DBG_8192C("PlatformClearPciPMEStatus(): PCI configration 0x34 = 0x%2x\n", value_offset);

	do
	{
		if(value_offset == 0x00) //end of pci capability
		{
			value = 0xff;
			break;
		}

		pci_read_config_byte(pdev, value_offset, &value);

		DBG_8192C("PlatformClearPciPMEStatus(): in pci configration1, value_offset%x = %x\n", value_offset, value);
		
		if(value == 0x01)
			break;
		else
		{
			value_offset = value_offset + 0x1;
			pci_read_config_byte(pdev, value_offset, &value);
			value_offset = value;
		}
	}while(_TRUE);
	
	if(value == 0x01)
	{
		value_offset = value_offset + 0x05;
		pci_read_config_byte(pdev, value_offset, &value);
		//printk("*** 1 PME value = %x \n", value);
		if(value & BIT7)
		{
			tmp = value | BIT7;
			pci_write_config_byte(pdev, value_offset, tmp);

			pci_read_config_byte(pdev, value_offset, &tmp);
			//printk("*** 2 PME value = %x \n", tmp);
			DBG_8192C("PlatformClearPciPMEStatus(): Clear PME status 0x%2x to 0x%2x\n", value_offset, tmp);
		}
		else
			DBG_8192C("PlatformClearPciPMEStatus(): PME status(0x%2x) = 0x%2x\n", value_offset, value);
	}

	//DBG_8192C("PlatformClearPciPMEStatus(): PME_status offset = %x, EN = %x\n", value_offset, PCIClkReq);

}

static u32 rtl8192ce_hal_deinit(PADAPTER Adapter)
{
	u8	u1bTmp = 0;
	u8	bSupportRemoteWakeUp = _FALSE;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	struct pwrctrl_priv		*pwrpriv = &Adapter->pwrctrlpriv;
	
_func_enter_;

	RT_SET_PS_LEVEL(pwrpriv, RT_RF_OFF_LEVL_HALT_NIC);

	// Without supporting WoWLAN or the driver is in awake (D0) state, we should 
	// call PowerOffAdapter8192CE() to run the power sequence. 2009.04.23. by tynli.
	if(!bSupportRemoteWakeUp )//||!pMgntInfo->bPwrSaveState) 
	{
		// 2009/10/13 MH For power off test.
		PowerOffAdapter8192CE(Adapter);	
	}
	else
	{
		u8	bSleep = _TRUE;

		//RxDMA
		//tynli_test 2009.12.16.
		u1bTmp = read8(Adapter, REG_RXPKT_NUM+2);
		write8(Adapter, REG_RXPKT_NUM+2, u1bTmp|BIT2);	

		//PlatformDisableASPM(Adapter);
		Adapter->HalFunc.SetHwRegHandler(Adapter, HW_VAR_SWITCH_EPHY_WoWLAN, (u8 *)&bSleep);

		//tynli_test. 2009.12.17.
		u1bTmp = read8(Adapter, REG_SPS0_CTRL);
		write8(Adapter, REG_SPS0_CTRL, (u1bTmp|BIT1));

		//
		write8(Adapter, REG_APS_FSMCO+1, 0x0);

		PlatformClearPciPMEStatus(Adapter);

		if(IS_NORMAL_CHIP(pHalData->VersionID))
		{
			// tynli_test for normal chip wowlan. 2010.01.26. Suggested by Sd1 Isaac and designer Alfred.
			write8(Adapter, REG_SYS_CLKR, (read8(Adapter, REG_SYS_CLKR)|BIT3));

			//prevent 8051 to be reset by PERST#
			write8(Adapter, REG_RSV_CTRL, 0x20);
			write8(Adapter, REG_RSV_CTRL, 0x60);
		}	
	}

_func_exit_;

	return _SUCCESS;
}

void StopTxBeacon(_adapter *padapter)
{
	HAL_DATA_TYPE*	pHalData = GET_HAL_DATA(padapter);
	u8 			tmp1Byte = 0;

 	if(IS_NORMAL_CHIP(pHalData->VersionID))
	 {
		tmp1Byte = read8(padapter, REG_FWHW_TXQ_CTRL+2);
		write8(padapter, REG_FWHW_TXQ_CTRL+2, tmp1Byte & (~BIT6));
		//write8(padapter, REG_BCN_MAX_ERR, 0xff); //Marked by tynli. Suggested by designer TimChen.
		write8(padapter, REG_TBTT_PROHIBIT+1, 0x64);
		tmp1Byte = read8(padapter, REG_TBTT_PROHIBIT+2);
		tmp1Byte &= ~(BIT0);
		write8(padapter, REG_TBTT_PROHIBIT+2, tmp1Byte);
	 }
	 else
	 {
		pHalData->RegTxPause = read8(padapter, REG_TXPAUSE);
		write8(padapter, REG_TXPAUSE, pHalData->RegTxPause | BIT6);
	 }

	 //CheckFwRsvdPageContent(Adapter);  // 2010.06.23. Added by tynli.

}

void ResumeTxBeacon(_adapter *padapter)
{
	HAL_DATA_TYPE*	pHalData = GET_HAL_DATA(padapter);	
	u8 			tmp1Byte = 0;

	 if(IS_NORMAL_CHIP(pHalData->VersionID))
	 {
		tmp1Byte = read8(padapter, REG_FWHW_TXQ_CTRL+2);
		write8(padapter, REG_FWHW_TXQ_CTRL+2, tmp1Byte | BIT6);
		//write8(padapter, REG_BCN_MAX_ERR, 0x0a);
		write8(padapter, REG_TBTT_PROHIBIT+1, 0xff);
		tmp1Byte = read8(padapter, REG_TBTT_PROHIBIT+2);
		tmp1Byte |= BIT0;
		write8(padapter, REG_TBTT_PROHIBIT+2, tmp1Byte);
	 }
	 else
	 {
		pHalData->RegTxPause = read8(padapter, REG_TXPAUSE);
		write8(padapter, REG_TXPAUSE, pHalData->RegTxPause & (~BIT6));
	 }
	 
}

static VOID
HalSetBrateCfg(
	IN PADAPTER		Adapter,
	IN u8			*mBratesOS,
	OUT u16			*pBrateCfg
)
{
	u8	is_brate;
	u8	i;
	u8	brate;

	for(i=0;i<NDIS_802_11_LENGTH_RATES_EX;i++)
	{
		is_brate = mBratesOS[i] & IEEE80211_BASIC_RATE_MASK;
		brate = mBratesOS[i] & 0x7f;
		if( is_brate )
		{
			switch(brate)
			{
				case IEEE80211_CCK_RATE_1MB:	*pBrateCfg |= RATE_1M;	break;
				case IEEE80211_CCK_RATE_2MB:	*pBrateCfg |= RATE_2M;	break;
				case IEEE80211_CCK_RATE_5MB:	*pBrateCfg |= RATE_5_5M;break;
				case IEEE80211_CCK_RATE_11MB:	*pBrateCfg |= RATE_11M;	break;
				case IEEE80211_OFDM_RATE_6MB:	*pBrateCfg |= RATE_6M;	break;
				case IEEE80211_OFDM_RATE_9MB:	*pBrateCfg |= RATE_9M;	break;
				case IEEE80211_OFDM_RATE_12MB:	*pBrateCfg |= RATE_12M;	break;
				case IEEE80211_OFDM_RATE_18MB:	*pBrateCfg |= RATE_18M;	break;
				case IEEE80211_OFDM_RATE_24MB:	*pBrateCfg |= RATE_24M;	break;
				case IEEE80211_OFDM_RATE_36MB:	*pBrateCfg |= RATE_36M;	break;
				case IEEE80211_OFDM_RATE_48MB:	*pBrateCfg |= RATE_48M;	break;
				case IEEE80211_OFDM_RATE_54MB:	*pBrateCfg |= RATE_54M;	break;			
			}
		}

	}
}

void SetHwReg8192CE(PADAPTER Adapter, u8 variable, u8* val)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;

_func_enter_;

	switch(variable)
	{
		case HW_VAR_MEDIA_STATUS:
			{
				u8 val8;

				val8 = read8(Adapter, MSR)&0x0c;
				val8 |= *((u8 *)val);
				write8(Adapter, MSR, val8);
			}
			break;
		case HW_VAR_MEDIA_STATUS1:
			{
				u8 val8;
				
				val8 = read8(Adapter, MSR)&0x03;
				val8 |= *((u8 *)val) <<2;
				write8(Adapter, MSR, val8);
			}
			break;
		case HW_VAR_SET_OPMODE:
			{
				u8	val8;
				u8	mode = *((u8 *)val);

				if((mode == _HW_STATE_STATION_) || (mode == _HW_STATE_NOLINK_))
				{
					StopTxBeacon(Adapter);
					write8(Adapter,REG_BCN_CTRL, 0x18);
				}
				else if((mode == _HW_STATE_ADHOC_) /*|| (mode == _HW_STATE_AP_)*/)
				{
					ResumeTxBeacon(Adapter);
					write8(Adapter,REG_BCN_CTRL, 0x1a);
				}
				else if(mode == _HW_STATE_AP_)
				{
					ResumeTxBeacon(Adapter);
					
					write8(Adapter, REG_BCN_CTRL, 0x12);

					
					//Set RCR
					//write32(padapter, REG_RCR, 0x70002a8e);//CBSSID_DATA must set to 0
					write32(Adapter, REG_RCR, 0x7000228e);//CBSSID_DATA must set to 0
					//enable to rx data frame				
					write16(Adapter, REG_RXFLTMAP2, 0xFFFF);
					//enable to rx ps-poll
					write16(Adapter, REG_RXFLTMAP1, 0x0400);

					//Beacon Control related register for first time 
					write8(Adapter, REG_BCNDMATIM, 0x02); // 2ms		
					write8(Adapter, REG_DRVERLYINT, 0x05);// 5ms
					//write8(Adapter, REG_BCN_MAX_ERR, 0xFF);
					write8(Adapter, REG_ATIMWND, 0x0a); // 10ms
					write16(Adapter, REG_BCNTCFG, 0x00);
					write16(Adapter, REG_TBTT_PROHIBIT, 0x6404);		
	
					//reset TSF		
					write8(Adapter, REG_DUAL_TSF_RST, BIT(0));

					//enable TSF Function for if1
					write8(Adapter, REG_BCN_CTRL, (EN_BCN_FUNCTION | EN_TXBCN_RPT));
			
					//enable update TSF for if1
					if(IS_NORMAL_CHIP(pHalData->VersionID))
					{			
						write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~BIT(4)));			
					}
					else
					{
						write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~(BIT(4)|BIT(5))));
					}
					
				}

				val8 = read8(Adapter, MSR)&0x0c;
				val8 |= mode;
				write8(Adapter, MSR, val8);
			}
			break;
		case HW_VAR_BSSID:
			{
				u8	idx = 0;
				for(idx = 0 ; idx < 6; idx++)
				{
					write8(Adapter, (REG_BSSID+idx), val[idx]);
				}
			}
			break;
		case HW_VAR_BASIC_RATE:
			{
				u16			BrateCfg = 0;
				u8			RateIndex = 0;
				
				// 2007.01.16, by Emily
				// Select RRSR (in Legacy-OFDM and CCK)
				// For 8190, we select only 24M, 12M, 6M, 11M, 5.5M, 2M, and 1M from the Basic rate.
				// We do not use other rates.
				HalSetBrateCfg( Adapter, val, &BrateCfg );

				pHalData->BasicRateSet = BrateCfg = BrateCfg & 0x15f;

				if(Adapter->mlmeextpriv.mlmext_info.assoc_AP_vendor == ciscoAP && ((BrateCfg &0x150)==0))
				{
					// if peer is cisco and didn't use ofdm rate,
					// we use 6M for ack.
					BrateCfg |=0x010;
				}

				BrateCfg |= 0x01; // default enable 1M ACK rate
				// Set RRSR rate table.
				write8(Adapter, REG_RRSR, BrateCfg&0xff);
				write8(Adapter, REG_RRSR+1, (BrateCfg>>8)&0xff);

				// Set RTS initial rate
				while(BrateCfg > 0x1)
				{
					BrateCfg = (BrateCfg>> 1);
					RateIndex++;
				}
				write8(Adapter, REG_INIRTS_RATE_SEL, RateIndex);
			}
			break;
		case HW_VAR_TXPAUSE:
			write8(Adapter, REG_TXPAUSE, *((u8 *)val));	
			break;
		case HW_VAR_BCN_FUNC:
			if(*((u8 *)val))
			{
				write8(Adapter, REG_BCN_CTRL, (EN_BCN_FUNCTION | EN_TXBCN_RPT));
			}
			else
			{
				write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~(EN_BCN_FUNCTION | EN_TXBCN_RPT)));
			}
			break;
		case HW_VAR_CORRECT_TSF:
			{
				u64	tsf;
				struct mlme_ext_priv	*pmlmeext = &Adapter->mlmeextpriv;
				struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

				tsf = pmlmeext->TSFValue - ((u32)pmlmeext->TSFValue % (pmlmeinfo->bcn_interval*1024)) -1024; //us

				if(((pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE) || ((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE))
				{				
					//pHalData->RegTxPause |= STOP_BCNQ;BIT(6)
					//write8(Adapter, REG_TXPAUSE, (read8(Adapter, REG_TXPAUSE)|BIT(6)));
					StopTxBeacon(Adapter);
				}

				//disable related TSF function
				write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~BIT(3)));
							
				write32(Adapter, REG_TSFTR, tsf);
				write32(Adapter, REG_TSFTR+4, tsf>>32);

				//enable related TSF function
				write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)|BIT(3));
				
							
				if(((pmlmeinfo->state&0x03) == WIFI_FW_ADHOC_STATE) || ((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE))
				{
					//pHalData->RegTxPause  &= (~STOP_BCNQ);
					//write8(Adapter, REG_TXPAUSE, (read8(Adapter, REG_TXPAUSE)&(~BIT(6))));
					ResumeTxBeacon(Adapter);
				}
			}
			break;
		case HW_VAR_CHECK_BSSID:
			if(*((u8 *)val))
			{
				if(IS_NORMAL_CHIP(pHalData->VersionID))
				{
					write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA|RCR_CBSSID_BCN);
				}
				else
				{
					write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA);
				}
			}
			else
			{
				u32	val32;

				val32 = read32(Adapter, REG_RCR);

				if(IS_NORMAL_CHIP(pHalData->VersionID))
				{
					val32 &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN);
				}
				else
				{
					val32 &= 0xfffff7bf;
				}

				write32(Adapter, REG_RCR, val32);
			}
			break;
		case HW_VAR_MLME_DISCONNECT:
			{
				//Set RCR to not to receive data frame when NO LINK state
				//write32(Adapter, REG_RCR, read32(padapter, REG_RCR) & ~RCR_ADF);
				write16(Adapter, REG_RXFLTMAP2,0x00);

				//reset TSF
				write8(Adapter, REG_DUAL_TSF_RST, (BIT(0)|BIT(1)));

				//disable update TSF
				if(IS_NORMAL_CHIP(pHalData->VersionID))
				{
					write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)|BIT(4));	
				}
				else
				{
					write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)|BIT(4)|BIT(5));
				}
			}
			break;
		case HW_VAR_MLME_SITESURVEY:
			if(*((u8 *)val))//under sitesurvey
			{
				if(IS_NORMAL_CHIP(pHalData->VersionID))
				{
					//config RCR to receive different BSSID & not to receive data frame
					//pHalData->ReceiveConfig &= (~(RCR_CBSSID_DATA | RCR_CBSSID_BCN));			
					u32 v = read32(Adapter, REG_RCR);
					v &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN );//| RCR_ADF
					write32(Adapter, REG_RCR, v);
					write16(Adapter, REG_RXFLTMAP2,0x00);

					//disable update TSF
					write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)|BIT(4));
				}	
				else
				{
					//config RCR to receive different BSSID & not to receive data frame			
					write32(Adapter, REG_RCR, read32(Adapter, REG_RCR) & 0xfffff7bf);


					//disable update TSF
					write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)|BIT(4)|BIT(5));
				}
			}
			else//sitesurvey done
			{
				struct mlme_ext_priv	*pmlmeext = &Adapter->mlmeextpriv;
				struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

				if (is_client_associated_to_ap(Adapter) == _TRUE)
				{
					//enable to rx data frame
					//write32(Adapter, REG_RCR, read32(padapter, REG_RCR)|RCR_ADF);
					write16(Adapter, REG_RXFLTMAP2,0xFFFF);

					//enable update TSF
					if(IS_NORMAL_CHIP(pHalData->VersionID))
					{
						write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~BIT(4)));
					}
					else
					{
						write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~(BIT(4)|BIT(5))));
					}
				}
				else if((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
				{
					write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_ADF);
					
					//enable update TSF
					if(IS_NORMAL_CHIP(pHalData->VersionID))			
						write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~BIT(4)));			
					else			
						write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~(BIT(4)|BIT(5))));	
				}


				if(IS_NORMAL_CHIP(pHalData->VersionID))
				{
					if((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
						write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_BCN);
					else			
						write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA|RCR_CBSSID_BCN);
				}
				else
				{
					write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA);
				}
			}
			break;
		case HW_VAR_MLME_JOIN:
			{
				u8	RetryLimit = 0x30;
				u8	type = *((u8 *)val);
				if(type == 0) // prepare to join
				{
					if(IS_NORMAL_CHIP(pHalData->VersionID))
					{
						//config RCR to receive different BSSID & not to receive data frame during linking				
						u32 v = read32(Adapter, REG_RCR);
						v &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN );//| RCR_ADF
						write32(Adapter, REG_RCR, v);
						write16(Adapter, REG_RXFLTMAP2,0x00);
					}	
					else
					{
						//config RCR to receive different BSSID & not to receive data frame during linking	
						write32(Adapter, REG_RCR, read32(Adapter, REG_RCR) & 0xfffff7bf);
					}
				}
				else if(type == 1) //joinbss_event call back
				{
					struct mlme_ext_priv	*pmlmeext = &Adapter->mlmeextpriv;
					struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

					//enable to rx data frame.Accept all data frame
					//write32(padapter, REG_RCR, read32(padapter, REG_RCR)|RCR_ADF);
					write16(Adapter, REG_RXFLTMAP2,0xFFFF);

					if((pmlmeinfo->state&0x03) == WIFI_FW_STATION_STATE)
					{
						if(IS_NORMAL_CHIP(pHalData->VersionID))
						{
							write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA|RCR_CBSSID_BCN);

							//enable update TSF
							write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~BIT(4)));
						}
						else
						{
							write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA);

							//enable update TSF
							write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~(BIT(4)|BIT(5))));
						}

						RetryLimit = (pHalData->CustomerID == RT_CID_CCX) ? 7 : 48;
					}
					else
					{
						RetryLimit = 0x7;
					}
				}
				else if(type == 2) //sta add event call back
				{				
					write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA|RCR_CBSSID_BCN);

					//accept all data frame
					write16(Adapter, REG_RXFLTMAP2, 0xff);

					//enable update TSF
					write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~BIT(4)));

					RetryLimit = 0x7;
				}

				write16(Adapter, REG_RL, RetryLimit << RETRY_LIMIT_SHORT_SHIFT | RetryLimit << RETRY_LIMIT_LONG_SHIFT);
			}
			break;
		case HW_VAR_BEACON_INTERVAL:
			write16(Adapter, REG_BCN_INTERVAL, *((u16 *)val));
			break;
		case HW_VAR_SLOT_TIME:
			{
				u8	u1bAIFS, aSifsTime;
				struct mlme_ext_priv	*pmlmeext = &Adapter->mlmeextpriv;
				struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

				DBG_8192C("Set HW_VAR_SLOT_TIME: SlotTime(%#x)\n", val[0]);
				write8(Adapter, REG_SLOT, val[0]);

				if(pmlmeinfo->WMM_enable == 0)
				{
					if(pmlmeext->cur_wireless_mode == WIRELESS_11B)
						aSifsTime = 10;
					else
						aSifsTime = 16;
					
					u1bAIFS = aSifsTime + (2 * pmlmeinfo->slotTime);
					
					// <Roger_EXP> Temporary removed, 2008.06.20.
					write8(Adapter, REG_EDCA_VO_PARAM, u1bAIFS);
					write8(Adapter, REG_EDCA_VI_PARAM, u1bAIFS);
					write8(Adapter, REG_EDCA_BE_PARAM, u1bAIFS);
					write8(Adapter, REG_EDCA_BK_PARAM, u1bAIFS);
				}
			}
			break;
		case HW_VAR_ACK_PREAMBLE:
			{
				u8	regTmp;
				u8	bShortPreamble = *( (PBOOLEAN)val );
				// Joseph marked out for Netgear 3500 TKIP channel 7 issue.(Temporarily)
				//regTmp = (pHalData->nCur40MhzPrimeSC)<<5;
				regTmp = 0;
				if(bShortPreamble)
					regTmp |= 0x80;

				write8(Adapter, REG_RRSR+2, regTmp);
			}
			break;
		case HW_VAR_SEC_CFG:
			write8(Adapter, REG_SECCFG, *((u8 *)val));
			break;
		case HW_VAR_DM_FLAG:
			pdmpriv->DMFlag = *((u8 *)val);
			break;
		case HW_VAR_DM_FUNC_OP:
			if(val[0])
			{// save dm flag
				pdmpriv->DMFlag_tmp = pdmpriv->DMFlag;
			}
			else
			{// restore dm flag
				DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
				
				pdmpriv->DMFlag = pdmpriv->DMFlag_tmp;

				if(pdmpriv->DMFlag&DYNAMIC_FUNC_DIG)
				{
					PHY_SetBBReg(Adapter, rOFDM0_XAAGCCore1, 0x7f, pDigTable->CurIGValue);
					PHY_SetBBReg(Adapter, rOFDM0_XBAGCCore1, 0x7f, pDigTable->CurIGValue);
				}
			}
			break;
		case HW_VAR_DM_FUNC_SET:
			pdmpriv->DMFlag |= *((u8 *)val);
			break;
		case HW_VAR_DM_FUNC_CLR:
			pdmpriv->DMFlag &= *((u8 *)val);
			break;
		case HW_VAR_CAM_EMPTY_ENTRY:
			{
				u8	ucIndex = *((u8 *)val);
				u8	i;
				u32	ulCommand=0;
				u32	ulContent=0;
				u32	ulEncAlgo=CAM_AES;

				for(i=0;i<CAM_CONTENT_COUNT;i++)
				{
					// filled id in CAM config 2 byte
					if( i == 0)
					{
						ulContent |=(ucIndex & 0x03) | ((u16)(ulEncAlgo)<<2);
						//ulContent |= CAM_VALID;
					}
					else
					{
						ulContent = 0;
					}
					// polling bit, and No Write enable, and address
					ulCommand= CAM_CONTENT_COUNT*ucIndex+i;
					ulCommand= ulCommand | CAM_POLLINIG|CAM_WRITE;
					// write content 0 is equall to mark invalid
					write32(Adapter, WCAMI, ulContent);  //delay_ms(40);
					//RT_TRACE(COMP_SEC, DBG_LOUD, ("CAM_empty_entry(): WRITE A4: %lx \n",ulContent));
					write32(Adapter, RWCAM, ulCommand);  //delay_ms(40);
					//RT_TRACE(COMP_SEC, DBG_LOUD, ("CAM_empty_entry(): WRITE A0: %lx \n",ulCommand));
				}
			}
			break;
		case HW_VAR_CAM_INVALID_ALL:
			write32(Adapter, RWCAM, BIT(31)|BIT(30));
			break;
		case HW_VAR_CAM_WRITE:
			{
				u32	cmd;
				u32	*cam_val = (u32 *)val;
				write32(Adapter, WCAMI, cam_val[0]);
				
				cmd = CAM_POLLINIG | CAM_WRITE | cam_val[1];
				write32(Adapter, RWCAM, cmd);
			}
			break;
		case HW_VAR_AC_PARAM_VO:
			write32(Adapter, REG_EDCA_VO_PARAM, ((u32 *)(val))[0]);
			break;
		case HW_VAR_AC_PARAM_VI:
			write32(Adapter, REG_EDCA_VI_PARAM, ((u32 *)(val))[0]);
			break;
		case HW_VAR_AC_PARAM_BE:
			pHalData->AcParam_BE = ((u32 *)(val))[0];
			write32(Adapter, REG_EDCA_BE_PARAM, ((u32 *)(val))[0]);
			break;
		case HW_VAR_AC_PARAM_BK:
			write32(Adapter, REG_EDCA_BK_PARAM, ((u32 *)(val))[0]);
			break;
		case HW_VAR_AMPDU_MIN_SPACE:
			{
				u8	MinSpacingToSet;
				u8	SecMinSpace;

				MinSpacingToSet = *((u8 *)val);
				if(MinSpacingToSet <= 7)
				{
					switch(Adapter->securitypriv.dot11PrivacyAlgrthm)
					{
						case _NO_PRIVACY_:
						case _AES_:
							SecMinSpace = 0;
							break;

						case _WEP40_:
						case _WEP104_:
						case _TKIP_:
						case _TKIP_WTMIC_:
						default:
							SecMinSpace = 7;
							break;
					}

					if(MinSpacingToSet < SecMinSpace){
						MinSpacingToSet = SecMinSpace;
					}

					//RT_TRACE(COMP_MLME, DBG_LOUD, ("Set HW_VAR_AMPDU_MIN_SPACE: %#x\n", Adapter->MgntInfo.MinSpaceCfg));
					write8(Adapter, REG_AMPDU_MIN_SPACE, (read8(Adapter, REG_AMPDU_MIN_SPACE) & 0xf8) | MinSpacingToSet);
				}
			}
			break;
		case HW_VAR_AMPDU_FACTOR:
			{
				u8	RegToSet_Normal[4]={0x41,0xa8,0x72, 0xb9};
				u8	RegToSet_BT[4]={0x31,0x74,0x42, 0x97};
				u8	FactorToSet;
				u8	*pRegToSet;
				u8	index = 0;

#if 0//cosa, for 92s
				if(	(pHalData->bt_coexist.BT_Coexist) &&
					(pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC4) )
					pRegToSet = RegToSet_BT; // 0x97427431;
				else
#endif
					pRegToSet = RegToSet_Normal; // 0xb972a841;

				FactorToSet = *((u8 *)val);
				if(FactorToSet <= 3)
				{
					FactorToSet = (1<<(FactorToSet + 2));
					if(FactorToSet>0xf)
						FactorToSet = 0xf;

					for(index=0; index<4; index++)
					{
						if((pRegToSet[index] & 0xf0) > (FactorToSet<<4))
							pRegToSet[index] = (pRegToSet[index] & 0x0f) | (FactorToSet<<4);
					
						if((pRegToSet[index] & 0x0f) > FactorToSet)
							pRegToSet[index] = (pRegToSet[index] & 0xf0) | (FactorToSet);
						
						write8(Adapter, (REG_AGGLEN_LMT+index), pRegToSet[index]);
					}

					//RT_TRACE(COMP_MLME, DBG_LOUD, ("Set HW_VAR_AMPDU_FACTOR: %#x\n", FactorToSet));
				}
			}
			break;
		case HW_VAR_SET_RPWM:
			write8(Adapter, REG_USB_HRPWM, *((u8 *)val));
			break;
		case HW_VAR_H2C_FW_PWRMODE:
			{
				u8	psmode = (*(u8 *)val);
			
				// Forece leave RF low power mode for 1T1R to prevent conficting setting in Fw power
				// saving sequence. 2010.06.07. Added by tynli. Suggested by SD3 yschang.
				if( (psmode != PS_MODE_ACTIVE) && (!IS_92C_SERIAL(pHalData->VersionID)))
				{
					rtl8192c_dm_RF_Saving(Adapter, _TRUE);
				}
				rtl8192c_set_FwPwrMode_cmd(Adapter, psmode);
			}
			break;
		case HW_VAR_H2C_FW_JOINBSSRPT:
			{
				u8	mstatus = (*(u8 *)val);
				rtl8192c_set_FwJoinBssReport_cmd(Adapter, mstatus);
			}
			break;
		case HW_VAR_INITIAL_GAIN:
			PHY_SetBBReg(Adapter, rOFDM0_XAAGCCore1, 0x7f, ((u32 *)(val))[0]);
			PHY_SetBBReg(Adapter, rOFDM0_XBAGCCore1, 0x7f, ((u32 *)(val))[0]);
			break;
#ifdef CONFIG_ANTENNA_DIVERSITY
		case HW_VAR_ANTENNA_DIVERSITY_JOIN:
			{
				u8	Optimum_antenna = (*(u8 *)val);
				//switch antenna to Optimum_antenna
				DBG_8192C("HW_VAR_ANTENNA_DIVERSITY_JOIN cur_ant(%d),opt_ant(%d)\n", pHalData->CurAntenna, Optimum_antenna);
				if(pHalData->CurAntenna !=  Optimum_antenna)		
				{
					PHY_SetBBReg(Adapter, rFPGA0_XA_RFInterfaceOE, 0x300, Optimum_antenna);
					DBG_8192C("#### Change to Optimum_antenna(%s)\n",(2==Optimum_antenna)?"A":"B");
				}
			}
			break;
		case HW_VAR_ANTENNA_DIVERSITY_LINK:
			SwAntDivRestAfterLink8192C(Adapter);
			break;
		case HW_VAR_ANTENNA_DIVERSITY_SELECT:
			{
				u8	Optimum_antenna = (*(u8 *)val);

				DBG_8192C("==> HW_VAR_ANTENNA_DIVERSITY_SELECT , Ant_(%s)\n",(Optimum_antenna==2)?"A":"B");

				PHY_SetBBReg(Adapter, rFPGA0_XA_RFInterfaceOE, 0x300, Optimum_antenna);
			}
			break;
#endif
		default:
			break;
	}

_func_exit_;
}

void GetHwReg8192CE(PADAPTER Adapter, u8 variable, u8* val)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

_func_enter_;

	switch(variable)
	{
		case HW_VAR_BASIC_RATE:
			*((u16 *)(val)) = pHalData->BasicRateSet;
		case HW_VAR_TXPAUSE:
			val[0] = read8(Adapter, REG_TXPAUSE);
			break;
		case HW_VAR_TX_BCN_DONE:
			{
				u32 xmitbcnDown;
				xmitbcnDown= read32(Adapter, REG_TDECTRL);
				if(xmitbcnDown & BCN_VALID  ){
					write32(Adapter,REG_TDECTRL, xmitbcnDown | BCN_VALID  ); // write 1 to clear, Clear by sw
					val[0] = _TRUE;
				}
			}
			break;
		case HW_VAR_DM_FLAG:
			val[0] = pHalData->dmpriv.DMFlag;
			break;
		case HW_VAR_RF_TYPE:
			val[0] = pHalData->rf_type;
			break;
		case HW_VAR_FWLPS_RF_ON:
			{
				//When we halt NIC, we should check if FW LPS is leave.
				u32	valRCR;
				
				if(Adapter->pwrctrlpriv.inactive_pwrstate == rf_off)
				{
					// If it is in HW/SW Radio OFF or IPS state, we do not check Fw LPS Leave,
					// because Fw is unload.
					val[0] = _TRUE;
				}
				else
				{
					valRCR = read32(Adapter, REG_RCR);
					valRCR &= 0x00070000;
					if(valRCR)
						val[0] = _FALSE;
					else
						val[0] = _TRUE;
				}
			}
			break;
#ifdef CONFIG_ANTENNA_DIVERSITY
		case HW_VAR_CURRENT_ANTENNA:
			val[0] = pHalData->CurAntenna;
			break;
#endif
		default:
			break;
	}

_func_exit_;
}

void UpdateHalRAMask8192CE(PADAPTER padapter, unsigned int cam_idx)
{
	//volatile unsigned int result;
	u8	init_rate=0;
	u8	networkType, raid;	
	u32	mask;
	u8	shortGIrate = _FALSE;
	int	supportRateNum = 0;
	struct sta_info	*psta;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &padapter->mlmeextpriv;
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);
	WLAN_BSSID_EX 		*cur_network = &(pmlmeinfo->network);
#ifdef CONFIG_BT_COEXIST
	struct btcoexist_priv	*pbtpriv = &(pHalData->bt_coexist);
#endif

	if (cam_idx >= NUM_STA) //CAM_SIZE
	{
		return;
	}
		
	switch (cam_idx)
	{
		case 4: //for broadcast/multicast
			supportRateNum = get_rateset_len(cur_network->SupportedRates);
			networkType = judge_network_type(padapter, cur_network->SupportedRates, supportRateNum) & 0xf;
			raid = networktype_to_raid(networkType);

			mask = update_basic_rate(cur_network->SupportedRates, supportRateNum);
			mask |= ((raid<<28)&0xf0000000);

			break;

		//case 5: //for AP 
		case 0: //for AP 
			supportRateNum = get_rateset_len(cur_network->SupportedRates);
			networkType = judge_network_type(padapter, cur_network->SupportedRates, supportRateNum) & 0xf;
			pmlmeext->cur_wireless_mode = networkType;
			raid = networktype_to_raid(networkType);

			mask = update_supported_rate(cur_network->SupportedRates, supportRateNum);
			mask |= (pmlmeinfo->HT_enable)? update_MSC_rate(&(pmlmeinfo->HT_caps)): 0;
			mask |= ((raid<<28)&0xf0000000);

			if (support_short_GI(padapter, &(pmlmeinfo->HT_caps)))
			{
				shortGIrate = _TRUE;
			}

			break;

		default: //for each sta in IBSS
			supportRateNum = get_rateset_len(pmlmeinfo->FW_sta_info[cam_idx].SupportedRates);
			networkType = judge_network_type(padapter, pmlmeinfo->FW_sta_info[cam_idx].SupportedRates, supportRateNum) & 0xf;
			pmlmeext->cur_wireless_mode = networkType;
			raid = networktype_to_raid(networkType);

			mask = update_supported_rate(cur_network->SupportedRates, supportRateNum);
			mask |= ((raid<<28)&0xf0000000);

			//todo: support HT in IBSS

			break;
	}

#if 0
	//
	// Modify rate adaptive bitmap for BT coexist.
	//
	if( (pHalData->bt_coexist.BluetoothCoexist) &&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR) &&
		(pHalData->bt_coexist.BT_CUR_State) &&
		(pHalData->bt_coexist.BT_Ant_isolation) &&
		((pHalData->bt_coexist.BT_Service==BT_SCO)||
		(pHalData->bt_coexist.BT_Service==BT_Busy)) )
		mask &= 0x0fffcfc0;
	else
		mask &= 0x0FFFFFFF;
#endif

	init_rate = get_highest_rate_idx(mask)&0x3f;

	if(pHalData->fw_ractrl == _TRUE)
	{
		u8 arg = 0;

		//arg = (cam_idx-4)&0x1f;//MACID
		arg = cam_idx&0x1f;//MACID
		
		arg |= BIT(7);
		
		if (shortGIrate==_TRUE)
			arg |= BIT(5);

		DBG_871X("update raid entry, mask=0x%x, arg=0x%x\n", mask, arg);

		rtl8192c_set_raid_cmd(padapter, mask, arg);	
		
	}
	else
	{
		if (shortGIrate==_TRUE)
			init_rate |= BIT(6);

		//write8(padapter, (REG_INIDATA_RATE_SEL+(cam_idx-4)), init_rate);
		write8(padapter, (REG_INIDATA_RATE_SEL+cam_idx), init_rate);
	}


	//set ra_id
	psta = pmlmeinfo->FW_sta_info[cam_idx].psta;
	if(psta)
	{
		psta->raid = raid;
		psta->init_rate = init_rate;
	}
}

void SetBeaconRelatedRegisters8192CE(PADAPTER padapter)
{
	u32	value32;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

	//
	// ATIM window
	//
	write16(padapter, REG_ATIMWND, 0x2);
	
	//
	// Beacon interval (in unit of TU).
	//
	write16(padapter, REG_BCN_INTERVAL, pmlmeinfo->bcn_interval);
	//2008.10.24 added by tynli for beacon changed.
	//PHY_SetBeaconHwReg( Adapter, BcnInterval );

	//
	// DrvErlyInt (in unit of TU). (Time to send interrupt to notify driver to change beacon content)
	//
	//PlatformEFIOWrite1Byte(Adapter, BCN_DMA_INT_92C+1, 0xC);

	//
	// BcnDMATIM(in unit of us). Indicates the time before TBTT to perform beacon queue DMA 
	//
	//PlatformEFIOWrite2Byte(Adapter, BCN_DMATIM_92C, 256); // HWSD suggest this value 2006.11.14

	//
	// Force beacon frame transmission even after receiving beacon frame from other ad hoc STA
	//
	//PlatformEFIOWrite2Byte(Adapter, BCN_ERRTH_92C, 100); // Reference from WMAC code 2006.11.14
	//suggest by wl, 20090902
	if(IS_NORMAL_CHIP(GET_HAL_DATA(padapter)->VersionID))
	{
		write16(padapter, REG_BCNTCFG, 0x660f);
	}
	else
	{
		// Suggested by designer timchen. Change beacon AIFS to the largest number
		// beacause test chip does not contension before sending beacon. by tynli. 2009.11.03
		write16(padapter, REG_BCNTCFG, 0x66ff);
	}

	//For throughput
	//PlatformEFIOWrite2Byte(Adapter,TBTT_PROHIBIT_92C,0x0202);
	//suggest by wl, 20090902
	//PlatformEFIOWrite1Byte(Adapter,REG_RXTSF_OFFSET_CCK, 0x30);	
	//PlatformEFIOWrite1Byte(Adapter,REG_RXTSF_OFFSET_OFDM, 0x30);
	
	// Suggested by TimChen. 2009.01.25.
	// Rx RF to MAC data path time.
	write8(padapter,REG_RXTSF_OFFSET_CCK, 0x18);	
	write8(padapter,REG_RXTSF_OFFSET_OFDM, 0x18);

	write8(padapter,0x606, 0x30);

	//
	// Update interrupt mask for IBSS.
	//
	UpdateInterruptMask8192CE( padapter, RT_IBSS_INT_MASKS, 0 );

	ResumeTxBeacon(padapter);

	//write8(padapter, 0x422, read8(padapter, 0x422)|BIT(6));
	
	//write8(padapter, 0x541, 0xff);

	//write8(padapter, 0x542, read8(padapter, 0x541)|BIT(0));

	write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)|BIT(1));

}

static void rtl8192ce_init_default_value(_adapter * padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;

	//init default value
	pHalData->fw_ractrl = _FALSE;
	pHalData->LastHMEBoxNum = 0;
	pHalData->bIQKInitialized = _FALSE;
	pHalData->bDefaultAntenna = 1;

	//
	// Set TCR-Transmit Control Register. The value is set in InitializeAdapter8190Pci()
	//
	pHalData->TransmitConfig = CFENDFORM | BIT12 | BIT13;

	//
	// Set RCR-Receive Control Register . The value is set in InitializeAdapter8190Pci().
	//
	pHalData->ReceiveConfig = (\
		//RCR_APPFCS	
		// | RCR_APWRMGT
		// |RCR_ADD3
		 RCR_AMF | RCR_ADF| RCR_APP_MIC| RCR_APP_ICV
		| RCR_AICV | RCR_ACRC32	// Accept ICV error, CRC32 Error
		| RCR_AB | RCR_AM			// Accept Broadcast, Multicast	 
     		| RCR_APM 					// Accept Physical match
     		//| RCR_AAP					// Accept Destination Address packets
     		| RCR_APP_PHYST_RXFF		// Accept PHY status
     		| RCR_HTC_LOC_CTRL
		//(pHalData->EarlyRxThreshold<<RCR_FIFO_OFFSET)	);
		);

	//
	// Set Interrupt Mask Register
	//
	// Make reference from WMAC code 2006.10.02, maybe we should disable some of the interrupt. by Emily
	pHalData->IntrMask[0]	= (u32)(			\
								IMR_ROK			|
								IMR_VODOK		|
								IMR_VIDOK 		|
								IMR_BEDOK 		|
								IMR_BKDOK		|
//								IMR_TBDER		|
								IMR_MGNTDOK 	|
//								IMR_TBDOK		|
								IMR_HIGHDOK 	|
								IMR_BDOK		|
//								IMR_ATIMEND		|
								IMR_RDU			|
								IMR_RXFOVW		|
								IMR_BcnInt		|
								IMR_PSTIMEOUT	| // P2P PS Timeout
//								IMR_TXFOVW		|
//								IMR_TIMEOUT1		|
//								IMR_TIMEOUT2		|
//								IMR_BCNDOK1		|
//								IMR_BCNDOK2		|
//								IMR_BCNDOK3		|
//								IMR_BCNDOK4		|
//								IMR_BCNDOK5		|
//								IMR_BCNDOK6		|
//								IMR_BCNDOK7		|
//								IMR_BCNDOK8		|
//								IMR_BCNDMAINT1	|
//								IMR_BCNDMAINT2	|
//								IMR_BCNDMAINT3	|
//								IMR_BCNDMAINT4	|
//								IMR_BCNDMAINT5	|
//								IMR_BCNDMAINT6	|
								0);
	pHalData->IntrMask[1] 	= (u32)(\
//								IMR_WLANOFF		|
//								IMR_OCPINT		|
//								IMR_CPWM		|
								IMR_C2HCMD		|
//								IMR_RXERR		|
//								IMR_TXERR		|
								0);

	pHalData->IntrMaskToSet[0] = pHalData->IntrMask[0];
	pHalData->IntrMaskToSet[1] = pHalData->IntrMask[1];

	//init dm default value
	pdmpriv->TM_Trigger = 0;
	pdmpriv->binitialized = _FALSE;
	pdmpriv->prv_traffic_idx = 3;
	pdmpriv->initialize = 0;
}

void rtl8192ce_set_hal_ops(_adapter * padapter)
{
	struct hal_ops	*pHalFunc = &padapter->HalFunc;

_func_enter_;

	padapter->HalData = _malloc(sizeof(HAL_DATA_TYPE));
	if(padapter->HalData == NULL){
		DBG_8192C("cant not alloc memory for HAL DATA \n");
	}
	_memset(padapter->HalData, 0, sizeof(HAL_DATA_TYPE));

	pHalFunc->hal_init = &rtl8192ce_hal_init;
	pHalFunc->hal_deinit = &rtl8192ce_hal_deinit;

	pHalFunc->free_hal_data = &rtl8192c_free_hal_data;

	pHalFunc->inirp_init = &rtl8192ce_init_desc_ring;
	pHalFunc->inirp_deinit = &rtl8192ce_free_desc_ring;

	pHalFunc->init_xmit_priv = &rtl8192ce_init_xmit_priv;
	pHalFunc->free_xmit_priv = &rtl8192ce_free_xmit_priv;

	pHalFunc->init_recv_priv = &rtl8192ce_init_recv_priv;
	pHalFunc->free_recv_priv = &rtl8192ce_free_recv_priv;

	pHalFunc->InitSwLeds = &rtl8192ce_InitSwLeds;
	pHalFunc->DeInitSwLeds = &rtl8192ce_DeInitSwLeds;

	pHalFunc->dm_init = &rtl8192c_init_dm_priv;
	pHalFunc->dm_deinit = &rtl8192c_deinit_dm_priv;

	pHalFunc->init_default_value = &rtl8192ce_init_default_value;
	pHalFunc->intf_chip_configure = &rtl8192ce_interface_configure;
	pHalFunc->read_adapter_info = &ReadAdapterInfo8192CE;

	pHalFunc->enable_interrupt = &EnableInterrupt8192CE;
	pHalFunc->disable_interrupt = &DisableInterrupt8192CE;
	pHalFunc->interrupt_handler = &rtl8192ce_interrupt;

	pHalFunc->set_bwmode_handler = &PHY_SetBWMode8192C;
	pHalFunc->set_channel_handler = &PHY_SwChnl8192C;

	pHalFunc->hal_dm_watchdog = &rtl8192c_HalDmWatchDog;

	pHalFunc->SetHwRegHandler = &SetHwReg8192CE;
	pHalFunc->GetHwRegHandler = &GetHwReg8192CE;

	pHalFunc->UpdateRAMaskHandler = &UpdateHalRAMask8192CE;
	pHalFunc->SetBeaconRelatedRegistersHandler = &SetBeaconRelatedRegisters8192CE;

	pHalFunc->Add_RateATid = &rtl8192c_Add_RateATid;

#ifdef CONFIG_ANTENNA_DIVERSITY
	pHalFunc->SwAntDivBeforeLinkHandler = &SwAntDivBeforeLink8192C;
	pHalFunc->SwAntDivCompareHandler = &SwAntDivCompare8192C;
#endif

	pHalFunc->hal_xmit = &rtl8192ce_hal_xmit;
	pHalFunc->mgnt_xmit = &rtl8192ce_mgnt_xmit;

	pHalFunc->read_bbreg = &rtl8192c_PHY_QueryBBReg;
	pHalFunc->write_bbreg = &rtl8192c_PHY_SetBBReg;
	pHalFunc->read_rfreg = &rtl8192c_PHY_QueryRFReg;
	pHalFunc->write_rfreg = &rtl8192c_PHY_SetRFReg;

#ifdef CONFIG_HOSTAPD_MLME
	pHalFunc->hostap_mgnt_xmit_entry = &rtl8192ce_hostap_mgnt_xmit_entry;
#endif
	
_func_exit_;

}

