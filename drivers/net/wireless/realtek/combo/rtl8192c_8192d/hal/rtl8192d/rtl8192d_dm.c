//============================================================
// Description:
//
// This file is for 92CE/92CU dynamic mechanism only
//
//
//============================================================

//============================================================
// include files
//============================================================
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_byteorder.h>

#include <hal_init.h>

#include <rtl8192d_hal.h>

//============================================================
// Global var
//============================================================
static u32 EDCAParam[maxAP][3] =
{          // UL			DL
	{0x5ea322, 0x00a630, 0x00a44f}, //atheros AP
	{0x5ea32b, 0x5ea42b, 0x5e4322}, //broadcom AP
	{0x3ea430, 0x00a630, 0xa430}, //cisco AP
	{0x5ea44f, 0x00a44f, 0x5ea42b}, //marvell AP
	{0x5ea422, 0x00a44f, 0x00a44f}, //ralink AP
	//{0x5ea44f, 0x5ea44f, 0x5ea44f}, //realtek AP
	{0xa44f, 0x5ea44f, 0x5e431c}, //realtek AP
	{0x5ea42b, 0xa630, 0x5e431c}, //airgocap AP
	{0x5ea42b, 0x5ea42b, 0x5ea42b}, //unknown AP
//	{0x5e4322, 0x00a44f, 0x5ea44f}, //unknown AP
};

#define	OFDM_TABLE_SIZE 	37
#define	OFDM_TABLE_SIZE_92D 	43
#define	CCK_TABLE_SIZE		33

static u32 OFDMSwingTable[OFDM_TABLE_SIZE_92D] = {
	0x7f8001fe, // 0, +6.0dB
	0x788001e2, // 1, +5.5dB
	0x71c001c7, // 2, +5.0dB
	0x6b8001ae, // 3, +4.5dB
	0x65400195, // 4, +4.0dB
	0x5fc0017f, // 5, +3.5dB
	0x5a400169, // 6, +3.0dB
	0x55400155, // 7, +2.5dB
	0x50800142, // 8, +2.0dB
	0x4c000130, // 9, +1.5dB
	0x47c0011f, // 10, +1.0dB
	0x43c0010f, // 11, +0.5dB
	0x40000100, // 12, +0dB
	0x3c8000f2, // 13, -0.5dB
	0x390000e4, // 14, -1.0dB
	0x35c000d7, // 15, -1.5dB
	0x32c000cb, // 16, -2.0dB
	0x300000c0, // 17, -2.5dB
	0x2d4000b5, // 18, -3.0dB
	0x2ac000ab, // 19, -3.5dB
	0x288000a2, // 20, -4.0dB
	0x26000098, // 21, -4.5dB
	0x24000090, // 22, -5.0dB
	0x22000088, // 23, -5.5dB
	0x20000080, // 24, -6.0dB
	0x1e400079, // 25, -6.5dB
	0x1c800072, // 26, -7.0dB
	0x1b00006c, // 27. -7.5dB
	0x19800066, // 28, -8.0dB
	0x18000060, // 29, -8.5dB
	0x16c0005b, // 30, -9.0dB
	0x15800056, // 31, -9.5dB
	0x14400051, // 32, -10.0dB
	0x1300004c, // 33, -10.5dB
	0x12000048, // 34, -11.0dB
	0x11000044, // 35, -11.5dB
	0x10000040, // 36, -12.0dB
	0x0f00003c,// 37, -12.5dB
	0x0e400039,// 38, -13.0dB    
	0x0d800036,// 39, -13.5dB
    	0x0cc00033,// 40, -14.0dB
	0x0c000030,// 41, -14.5dB
	0x0b40002d,// 42, -15.0dB	
};

static u8 CCKSwingTable_Ch1_Ch13[CCK_TABLE_SIZE][8] = {
{0x36, 0x35, 0x2e, 0x25, 0x1c, 0x12, 0x09, 0x04},	// 0, +0dB
{0x33, 0x32, 0x2b, 0x23, 0x1a, 0x11, 0x08, 0x04},	// 1, -0.5dB
{0x30, 0x2f, 0x29, 0x21, 0x19, 0x10, 0x08, 0x03},	// 2, -1.0dB
{0x2d, 0x2d, 0x27, 0x1f, 0x18, 0x0f, 0x08, 0x03},	// 3, -1.5dB
{0x2b, 0x2a, 0x25, 0x1e, 0x16, 0x0e, 0x07, 0x03},	// 4, -2.0dB 
{0x28, 0x28, 0x22, 0x1c, 0x15, 0x0d, 0x07, 0x03},	// 5, -2.5dB
{0x26, 0x25, 0x21, 0x1b, 0x14, 0x0d, 0x06, 0x03},	// 6, -3.0dB
{0x24, 0x23, 0x1f, 0x19, 0x13, 0x0c, 0x06, 0x03},	// 7, -3.5dB
{0x22, 0x21, 0x1d, 0x18, 0x11, 0x0b, 0x06, 0x02},	// 8, -4.0dB 
{0x20, 0x20, 0x1b, 0x16, 0x11, 0x08, 0x05, 0x02},	// 9, -4.5dB
{0x1f, 0x1e, 0x1a, 0x15, 0x10, 0x0a, 0x05, 0x02},	// 10, -5.0dB 
{0x1d, 0x1c, 0x18, 0x14, 0x0f, 0x0a, 0x05, 0x02},	// 11, -5.5dB
{0x1b, 0x1a, 0x17, 0x13, 0x0e, 0x09, 0x04, 0x02},	// 12, -6.0dB 
{0x1a, 0x19, 0x16, 0x12, 0x0d, 0x09, 0x04, 0x02},	// 13, -6.5dB
{0x18, 0x17, 0x15, 0x11, 0x0c, 0x08, 0x04, 0x02},	// 14, -7.0dB 
{0x17, 0x16, 0x13, 0x10, 0x0c, 0x08, 0x04, 0x02},	// 15, -7.5dB
{0x16, 0x15, 0x12, 0x0f, 0x0b, 0x07, 0x04, 0x01},	// 16, -8.0dB 
{0x14, 0x14, 0x11, 0x0e, 0x0b, 0x07, 0x03, 0x02},	// 17, -8.5dB
{0x13, 0x13, 0x10, 0x0d, 0x0a, 0x06, 0x03, 0x01},	// 18, -9.0dB 
{0x12, 0x12, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},	// 19, -9.5dB
{0x11, 0x11, 0x0f, 0x0c, 0x09, 0x06, 0x03, 0x01},	// 20, -10.0dB
{0x10, 0x10, 0x0e, 0x0b, 0x08, 0x05, 0x03, 0x01},	// 21, -10.5dB
{0x0f, 0x0f, 0x0d, 0x0b, 0x08, 0x05, 0x03, 0x01},	// 22, -11.0dB
{0x0e, 0x0e, 0x0c, 0x0a, 0x08, 0x05, 0x02, 0x01},	// 23, -11.5dB
{0x0d, 0x0d, 0x0c, 0x0a, 0x07, 0x05, 0x02, 0x01},	// 24, -12.0dB
{0x0d, 0x0c, 0x0b, 0x09, 0x07, 0x04, 0x02, 0x01},	// 25, -12.5dB
{0x0c, 0x0c, 0x0a, 0x09, 0x06, 0x04, 0x02, 0x01},	// 26, -13.0dB
{0x0b, 0x0b, 0x0a, 0x08, 0x06, 0x04, 0x02, 0x01},	// 27, -13.5dB
{0x0b, 0x0a, 0x09, 0x08, 0x06, 0x04, 0x02, 0x01},	// 28, -14.0dB
{0x0a, 0x0a, 0x09, 0x07, 0x05, 0x03, 0x02, 0x01},	// 29, -14.5dB
{0x0a, 0x09, 0x08, 0x07, 0x05, 0x03, 0x02, 0x01},	// 30, -15.0dB
{0x09, 0x09, 0x08, 0x06, 0x05, 0x03, 0x01, 0x01},	// 31, -15.5dB
{0x09, 0x08, 0x07, 0x06, 0x04, 0x03, 0x01, 0x01}	// 32, -16.0dB
};

static u8 CCKSwingTable_Ch14 [CCK_TABLE_SIZE][8]= {
{0x36, 0x35, 0x2e, 0x1b, 0x00, 0x00, 0x00, 0x00},	// 0, +0dB	
{0x33, 0x32, 0x2b, 0x19, 0x00, 0x00, 0x00, 0x00},	// 1, -0.5dB 
{0x30, 0x2f, 0x29, 0x18, 0x00, 0x00, 0x00, 0x00},	// 2, -1.0dB  
{0x2d, 0x2d, 0x17, 0x17, 0x00, 0x00, 0x00, 0x00},	// 3, -1.5dB
{0x2b, 0x2a, 0x25, 0x15, 0x00, 0x00, 0x00, 0x00},	// 4, -2.0dB  
{0x28, 0x28, 0x24, 0x14, 0x00, 0x00, 0x00, 0x00},	// 5, -2.5dB
{0x26, 0x25, 0x21, 0x13, 0x00, 0x00, 0x00, 0x00},	// 6, -3.0dB  
{0x24, 0x23, 0x1f, 0x12, 0x00, 0x00, 0x00, 0x00},	// 7, -3.5dB  
{0x22, 0x21, 0x1d, 0x11, 0x00, 0x00, 0x00, 0x00},	// 8, -4.0dB  
{0x20, 0x20, 0x1b, 0x10, 0x00, 0x00, 0x00, 0x00},	// 9, -4.5dB
{0x1f, 0x1e, 0x1a, 0x0f, 0x00, 0x00, 0x00, 0x00},	// 10, -5.0dB  
{0x1d, 0x1c, 0x18, 0x0e, 0x00, 0x00, 0x00, 0x00},	// 11, -5.5dB
{0x1b, 0x1a, 0x17, 0x0e, 0x00, 0x00, 0x00, 0x00},	// 12, -6.0dB  
{0x1a, 0x19, 0x16, 0x0d, 0x00, 0x00, 0x00, 0x00},	// 13, -6.5dB 
{0x18, 0x17, 0x15, 0x0c, 0x00, 0x00, 0x00, 0x00},	// 14, -7.0dB  
{0x17, 0x16, 0x13, 0x0b, 0x00, 0x00, 0x00, 0x00},	// 15, -7.5dB
{0x16, 0x15, 0x12, 0x0b, 0x00, 0x00, 0x00, 0x00},	// 16, -8.0dB  
{0x14, 0x14, 0x11, 0x0a, 0x00, 0x00, 0x00, 0x00},	// 17, -8.5dB
{0x13, 0x13, 0x10, 0x0a, 0x00, 0x00, 0x00, 0x00},	// 18, -9.0dB  
{0x12, 0x12, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},	// 19, -9.5dB
{0x11, 0x11, 0x0f, 0x09, 0x00, 0x00, 0x00, 0x00},	// 20, -10.0dB
{0x10, 0x10, 0x0e, 0x08, 0x00, 0x00, 0x00, 0x00},	// 21, -10.5dB
{0x0f, 0x0f, 0x0d, 0x08, 0x00, 0x00, 0x00, 0x00},	// 22, -11.0dB
{0x0e, 0x0e, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},	// 23, -11.5dB
{0x0d, 0x0d, 0x0c, 0x07, 0x00, 0x00, 0x00, 0x00},	// 24, -12.0dB
{0x0d, 0x0c, 0x0b, 0x06, 0x00, 0x00, 0x00, 0x00},	// 25, -12.5dB
{0x0c, 0x0c, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},	// 26, -13.0dB
{0x0b, 0x0b, 0x0a, 0x06, 0x00, 0x00, 0x00, 0x00},	// 27, -13.5dB
{0x0b, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},	// 28, -14.0dB
{0x0a, 0x0a, 0x09, 0x05, 0x00, 0x00, 0x00, 0x00},	// 29, -14.5dB
{0x0a, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},	// 30, -15.0dB
{0x09, 0x09, 0x08, 0x05, 0x00, 0x00, 0x00, 0x00},	// 31, -15.5dB
{0x09, 0x08, 0x07, 0x04, 0x00, 0x00, 0x00, 0x00}	// 32, -16.0dB
};	

/*-----------------------------------------------------------------------------
 * Function:	dm_DIGInit()
 *
 * Overview:	Set DIG scheme init value.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *
 *---------------------------------------------------------------------------*/
static void dm_DIGInit(
	IN	PADAPTER	pAdapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	

	pDigTable->Dig_Enable_Flag = _TRUE;
	pDigTable->Dig_Ext_Port_Stage = DIG_EXT_PORT_STAGE_MAX;
	
	pDigTable->CurIGValue = 0x20;
	pDigTable->PreIGValue = 0x0;

	pDigTable->CurSTAConnectState = pDigTable->PreSTAConnectState = DIG_STA_DISCONNECT;
	pDigTable->CurMultiSTAConnectState = DIG_MultiSTA_DISCONNECT;

	pDigTable->RssiLowThresh 	= DM_DIG_THRESH_LOW;
	pDigTable->RssiHighThresh 	= DM_DIG_THRESH_HIGH;

	pDigTable->FALowThresh	= DM_FALSEALARM_THRESH_LOW;
	pDigTable->FAHighThresh	= DM_FALSEALARM_THRESH_HIGH;

	
	pDigTable->rx_gain_range_max = DM_DIG_MAX;
	pDigTable->rx_gain_range_min = DM_DIG_MIN;

	pDigTable->BackoffVal = DM_DIG_BACKOFF_DEFAULT;
	pDigTable->BackoffVal_range_max = DM_DIG_BACKOFF_MAX;
	pDigTable->BackoffVal_range_min = DM_DIG_BACKOFF_MIN;

	pDigTable->PreCCKPDState = CCK_PD_STAGE_LowRssi;
	pDigTable->CurCCKPDState = CCK_PD_STAGE_MAX;

	pDigTable->LargeFAHit = 0;
	pDigTable->Recover_cnt = 0;
	pDigTable->ForbiddenIGI = DM_DIG_FA_LOWER;
}


static VOID 
dm_FalseAlarmCounterStatistics(
	IN	PADAPTER	Adapter
	)
{
	u32 ret_value;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PFALSE_ALARM_STATISTICS FalseAlmCnt = &(pdmpriv->FalseAlmCnt);
	DIG_T	*pDM_DigTable = &pdmpriv->DM_DigTable;

	//hold ofdm counter
	PHY_SetBBReg(Adapter, rOFDM0_LSTF, BIT31, 1); //hold page C counter
	PHY_SetBBReg(Adapter, rOFDM1_LSTF, BIT31, 1); //hold page D counter

	ret_value = PHY_QueryBBReg(Adapter, rOFDM0_FrameSync, bMaskDWord);
	FalseAlmCnt->Cnt_Fast_Fsync_fail = (ret_value&0xffff);
	FalseAlmCnt->Cnt_SB_Search_fail = ((ret_value&0xffff0000)>>16);		
	ret_value = PHY_QueryBBReg(Adapter, rOFDM_PHYCounter1, bMaskDWord);
	FalseAlmCnt->Cnt_Parity_Fail = ((ret_value&0xffff0000)>>16);	
	ret_value = PHY_QueryBBReg(Adapter, rOFDM_PHYCounter2, bMaskDWord);
	FalseAlmCnt->Cnt_Rate_Illegal = (ret_value&0xffff);
	FalseAlmCnt->Cnt_Crc8_fail = ((ret_value&0xffff0000)>>16);
	ret_value = PHY_QueryBBReg(Adapter, rOFDM_PHYCounter3, bMaskDWord);
	FalseAlmCnt->Cnt_Mcs_fail = (ret_value&0xffff);

	FalseAlmCnt->Cnt_Ofdm_fail = 	FalseAlmCnt->Cnt_Parity_Fail + FalseAlmCnt->Cnt_Rate_Illegal +
								FalseAlmCnt->Cnt_Crc8_fail + FalseAlmCnt->Cnt_Mcs_fail +
								FalseAlmCnt->Cnt_Fast_Fsync_fail + FalseAlmCnt->Cnt_SB_Search_fail;

	if(pHalData->CurrentBandType92D != BAND_ON_5G)
	{
		//hold cck counter
		//AcquireCCKAndRWPageAControl(Adapter);
		//PHY_SetBBReg(Adapter, rCCK0_FalseAlarmReport, BIT14, 1);
		
		ret_value = PHY_QueryBBReg(Adapter, rCCK0_FACounterLower, bMaskByte0);
		FalseAlmCnt->Cnt_Cck_fail = ret_value;

		ret_value = PHY_QueryBBReg(Adapter, rCCK0_FACounterUpper, bMaskByte3);
		FalseAlmCnt->Cnt_Cck_fail +=  (ret_value& 0xff)<<8;
		//ReleaseCCKAndRWPageAControl(Adapter);
	}
	else
	{
		FalseAlmCnt->Cnt_Cck_fail = 0;
	}

#if 1	
	FalseAlmCnt->Cnt_all = (	FalseAlmCnt->Cnt_Fast_Fsync_fail + 
						FalseAlmCnt->Cnt_SB_Search_fail +
						FalseAlmCnt->Cnt_Parity_Fail +
						FalseAlmCnt->Cnt_Rate_Illegal +
						FalseAlmCnt->Cnt_Crc8_fail +
						FalseAlmCnt->Cnt_Mcs_fail +
						FalseAlmCnt->Cnt_Cck_fail);	
#endif
#if 0 //Just for debug
	if(pDM_DigTable->CurIGValue < 0x25)
		FalseAlmCnt->Cnt_all = 12000;
	else if(pDM_DigTable->CurIGValue < 0x2A)
		FalseAlmCnt->Cnt_all = 20;
	else if(pDM_DigTable->CurIGValue < 0x2D)
		FalseAlmCnt->Cnt_all = 0;
#endif

	//reset false alarm counter registers
	PHY_SetBBReg(Adapter, rOFDM1_LSTF, 0x08000000, 1);
	PHY_SetBBReg(Adapter, rOFDM1_LSTF, 0x08000000, 0);
	//update ofdm counter
	PHY_SetBBReg(Adapter, rOFDM0_LSTF, BIT31, 0); //update page C counter
	PHY_SetBBReg(Adapter, rOFDM1_LSTF, BIT31, 0); //update page D counter
	if(pHalData->CurrentBandType92D != BAND_ON_5G)
	{
		//reset cck counter
		//AcquireCCKAndRWPageAControl(Adapter);
		PHY_SetBBReg(Adapter, rCCK0_FalseAlarmReport, 0x0000c000, 0);
		//enable cck counter
		PHY_SetBBReg(Adapter, rCCK0_FalseAlarmReport, 0x0000c000, 2);
		//ReleaseCCKAndRWPageAControl(Adapter);
	}

	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("Cnt_Fast_Fsync_fail = %ld, Cnt_SB_Search_fail = %ld\n", 
	//			FalseAlmCnt->Cnt_Fast_Fsync_fail , FalseAlmCnt->Cnt_SB_Search_fail) );	
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("Cnt_Parity_Fail = %ld, Cnt_Rate_Illegal = %ld, Cnt_Crc8_fail = %ld, Cnt_Mcs_fail = %ld\n", 
	//			FalseAlmCnt->Cnt_Parity_Fail, FalseAlmCnt->Cnt_Rate_Illegal, FalseAlmCnt->Cnt_Crc8_fail, FalseAlmCnt->Cnt_Mcs_fail) );	
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("Cnt_Ofdm_fail = %ld, Cnt_Cck_fail = %ld, Cnt_all = %ld\n", 
	//			FalseAlmCnt->Cnt_Ofdm_fail, FalseAlmCnt->Cnt_Cck_fail, FalseAlmCnt->Cnt_all) );
}


static VOID
DM_Write_DIG(
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("CurIGValue = 0x%lx, PreIGValue = 0x%lx, BackoffVal = %d\n", 
	//			DM_DigTable.CurIGValue, DM_DigTable.PreIGValue, DM_DigTable.BackoffVal));
	
	if(pDigTable->PreIGValue != pDigTable->CurIGValue)
	{
		// Set initial gain.
		// 20100211 Joseph: Set only BIT0~BIT6 for DIG. BIT7 is the function switch of Antenna diversity.
		// Just not to modified it for SD3 testing.
		//PHY_SetBBReg(pAdapter, rOFDM0_XAAGCCore1, bMaskByte0, DM_DigTable.CurIGValue);
		//PHY_SetBBReg(pAdapter, rOFDM0_XBAGCCore1, bMaskByte0, DM_DigTable.CurIGValue);
		PHY_SetBBReg(pAdapter, rOFDM0_XAAGCCore1, 0x7f, pDigTable->CurIGValue);
		PHY_SetBBReg(pAdapter, rOFDM0_XBAGCCore1, 0x7f, pDigTable->CurIGValue);	
		pDigTable->PreIGValue = pDigTable->CurIGValue;
	}
}


static void dm_1R_CCA(
	IN	PADAPTER	pAdapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	struct mlme_priv	*pmlmepriv = &pAdapter->mlmepriv;
	pPS_T pDM_PSTable = &pdmpriv->DM_PSTable;

	if(pdmpriv->MinUndecoratedPWDBForDM != 0)
	{		 
		if(PHY_QueryBBReg(pAdapter, rOFDM0_TRxPathEnable, bMaskByte0) == 0x33)
			pDM_PSTable->PreCCAState = CCA_2R;
		if(pDM_PSTable->PreCCAState == CCA_2R || pDM_PSTable->PreCCAState == CCA_MAX)
		{
			if(pdmpriv->MinUndecoratedPWDBForDM >= 35)
				pDM_PSTable->CurCCAState = CCA_1R;
			else
				pDM_PSTable->CurCCAState = CCA_2R;
			
		}
		else{
			if(pdmpriv->MinUndecoratedPWDBForDM <= 30)
				pDM_PSTable->CurCCAState = CCA_2R;
			else
				pDM_PSTable->CurCCAState = CCA_1R;
		}
	}
	else	//disconnect
	{
		pDM_PSTable->CurCCAState=CCA_MAX;
	}
	
	if(pDM_PSTable->PreCCAState != pDM_PSTable->CurCCAState)
	{
		if(pDM_PSTable->CurCCAState == CCA_1R)
		{
			if(pHalData->rf_type == RF_2T2R)
			{
				PHY_SetBBReg(pAdapter, rOFDM0_TRxPathEnable  , bMaskByte0, 0x13);
				PHY_SetBBReg(pAdapter, 0xe70, bMaskByte3, 0x20);
			}
			else
			{
				PHY_SetBBReg(pAdapter, rOFDM0_TRxPathEnable  , bMaskByte0, 0x23);
				PHY_SetBBReg(pAdapter, 0xe70, 0x7fc00000, 0x10c); // Set RegE70[30:22] = 9b'100001100
			}
		}
		else if (pDM_PSTable->CurCCAState == CCA_2R || pDM_PSTable->CurCCAState == CCA_MAX)		
		{
			PHY_SetBBReg(pAdapter, rOFDM0_TRxPathEnable  , bMaskByte0, 0x33);
			PHY_SetBBReg(pAdapter,0xe70, bMaskByte3, 0x63);
		}
		else if (pDM_PSTable->CurCCAState == CCA_MIN)//if disconnet and not scaning,close rx.
		{
			PHY_SetBBReg(pAdapter, rOFDM0_TRxPathEnable  , bMaskByte0, 0x03);
			//PHY_SetBBReg(pAdapter,0xe70, 0x7fc00000, 0);
			PHY_SetBBReg(pAdapter,0xe70, bMaskByte3, 0x0);
		}
			
		pDM_PSTable->PreCCAState = pDM_PSTable->CurCCAState;
	}
	//RT_TRACE(	COMP_BB_POWERSAVING|COMP_INIT, DBG_LOUD, ("CCAStage = %d\n",pDM_PSTable->CurCCAState));
}


static void dm_CCK_PacketDetectionThresh(
	IN	PADAPTER	pAdapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PFALSE_ALARM_STATISTICS FalseAlmCnt = &(pdmpriv->FalseAlmCnt);
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;

	if(pDigTable->CurSTAConnectState == DIG_STA_CONNECT)
	{
		if(pDigTable->PreCCKPDState == CCK_PD_STAGE_LowRssi)
		{
			if(pdmpriv->MinUndecoratedPWDBForDM <= 25)
				pDigTable->CurCCKPDState = CCK_PD_STAGE_LowRssi;
			else
				pDigTable->CurCCKPDState = CCK_PD_STAGE_HighRssi;
		}
		else{
			if(pdmpriv->MinUndecoratedPWDBForDM <= 20)
				pDigTable->CurCCKPDState = CCK_PD_STAGE_LowRssi;
			else
				pDigTable->CurCCKPDState = CCK_PD_STAGE_HighRssi;
		}
	}
	else
		pDigTable->CurCCKPDState=CCK_PD_STAGE_LowRssi;
	
	if(pDigTable->PreCCKPDState != pDigTable->CurCCKPDState)
	{
		if(pDigTable->CurCCKPDState == CCK_PD_STAGE_LowRssi)
		{
			PHY_SetBBReg(pAdapter, rCCK0_CCA, bMaskByte2, 0x83);
			//PHY_SetBBReg(pAdapter, rCCK0_System, bMaskByte1, 0x40);
			//if(IS_92C_SERIAL(pHalData->VersionID) || IS_92D_SINGLEPHY(pHalData->VersionID))
			//	PHY_SetBBReg(pAdapter, rCCK0_FalseAlarmReport , bMaskByte2, 0xd7);
		}
		else
		{
			PHY_SetBBReg(pAdapter, rCCK0_CCA, bMaskByte2, 0xcd);
			//PHY_SetBBReg(pAdapter,rCCK0_System, bMaskByte1, 0x47);
			//if(IS_92C_SERIAL(pHalData->VersionID) || IS_92D_SINGLEPHY(pHalData->VersionID))
			//	PHY_SetBBReg(pAdapter, rCCK0_FalseAlarmReport , bMaskByte2, 0xd3);
		}
		pDigTable->PreCCKPDState = pDigTable->CurCCKPDState;	
	}
	
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("CurSTAConnectState=%s\n",(pDigTable->CurSTAConnectState == DIG_STA_CONNECT?"DIG_STA_CONNECT ":"DIG_STA_DISCONNECT")));
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("CCKPDStage=%s\n",(pDigTable->CurCCKPDState==CCK_PD_STAGE_LowRssi?"Low RSSI ":"High RSSI ")));
	//RT_TRACE(	COMP_DIG, DBG_LOUD, ("is92d single phy =%x\n",IS_92D_SINGLEPHY(pHalData->VersionID)));
}


//For 8192D DIG mechanism,by luke.
static void dm_DIG_8192D(
	IN	PADAPTER	pAdapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct mlme_priv	*pmlmepriv = &(pAdapter->mlmepriv);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PFALSE_ALARM_STATISTICS FalseAlmCnt = &(pdmpriv->FalseAlmCnt);
	DIG_T	*pDigTable = &pdmpriv->DM_DigTable;
	u8	value_IGI = pDigTable->CurIGValue;
	
	//RT_TRACE(COMP_DIG, DBG_LOUD, ("dm_DIG() ==>\n"));
	
	if(pdmpriv->bDMInitialGainEnable == _FALSE)
		return;
	if (pDigTable->Dig_Enable_Flag == _FALSE)
		return;
	if(!(pdmpriv->DMFlag & DYNAMIC_FUNC_DIG))
		return;
	if (check_fwstate(pmlmepriv, _FW_UNDER_SURVEY) == _TRUE)
		return;

	//RT_TRACE(COMP_DIG, DBG_LOUD, ("dm_DIG() progress \n"));

	// Decide the current status and if modify initial gain or not
	if(check_fwstate(pmlmepriv, _FW_UNDER_LINKING) == _TRUE)
		pDigTable->CurSTAConnectState = DIG_STA_BEFORE_CONNECT;
	//else if(MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_NORMAL || MgntActQuery_ApType(pAdapter) == RT_AP_TYPE_IBSS_EMULATED ) 
	//	pDigTable->CurSTAConnectState = DIG_STA_DISCONNECT;
	else if((check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)&&(pmlmepriv->fw_state & WIFI_STATION_STATE))
		pDigTable->CurSTAConnectState = DIG_STA_CONNECT;
	else
		pDigTable->CurSTAConnectState = DIG_STA_DISCONNECT;

	//adjust initial gain according to false alarm counter
	if(FalseAlmCnt->Cnt_all < DM_DIG_FA_TH0)	
		value_IGI --;
	else if(FalseAlmCnt->Cnt_all < DM_DIG_FA_TH1)	
		value_IGI += 0;
	else if(FalseAlmCnt->Cnt_all < DM_DIG_FA_TH2)	
		value_IGI ++;
	else if(FalseAlmCnt->Cnt_all >= DM_DIG_FA_TH2)	
		value_IGI +=2;
	
	//DBG Report
	//RT_TRACE(COMP_DIG, DBG_LOUD, ("dm_DIG() Before: LargeFAHit=%d, ForbiddenIGI=%x \n", 
	//			pDigTable->LargeFAHit, pDigTable->ForbiddenIGI));
	//RT_TRACE(COMP_DIG, DBG_LOUD, ("dm_DIG() Before: Recover_cnt=%d, rx_gain_range_min=%x \n", 
	//			pDigTable->Recover_cnt, pDigTable->rx_gain_range_min));

	//deal with abnorally large false alarm
	if(FalseAlmCnt->Cnt_all > 10000)
	{
		//RT_TRACE(COMP_DIG, DBG_LOUD, ("dm_DIG(): Abnornally false alarm case. \n"));

		pDigTable->LargeFAHit++;
		if(pDigTable->ForbiddenIGI < pDigTable->CurIGValue)
		{
			pDigTable->ForbiddenIGI = pDigTable->CurIGValue;
			pDigTable->LargeFAHit = 1;
		}
		if(pDigTable->LargeFAHit >= 3)
		{
			if((pDigTable->ForbiddenIGI+1) > DM_DIG_MAX)
				pDigTable->rx_gain_range_min = DM_DIG_MAX;
			else
				pDigTable->rx_gain_range_min = (pDigTable->ForbiddenIGI + 1);
			pDigTable->Recover_cnt = 300; //300=10min
		}
	}
	else
	{
		//Recovery mechanism for IGI lower bound
		if(pDigTable->Recover_cnt != 0)
			pDigTable->Recover_cnt --;
		else
		{
			if(pDigTable->LargeFAHit == 0 )
			{
				if((pDigTable->ForbiddenIGI -1) < DM_DIG_FA_LOWER)
				{
					pDigTable->ForbiddenIGI = DM_DIG_FA_LOWER;
					pDigTable->rx_gain_range_min = DM_DIG_FA_LOWER;
				}
				else
				{
					pDigTable->ForbiddenIGI --;
					pDigTable->rx_gain_range_min = (pDigTable->ForbiddenIGI + 1);
				}
			}
			else if(pDigTable->LargeFAHit == 3 )
			{
				pDigTable->LargeFAHit = 0;
			}
		}
	}

	//DBG Report
	//RT_TRACE(COMP_DIG, DBG_LOUD, ("dm_DIG() After: LargeFAHit=%d, ForbiddenIGI=%x \n", 
	//			pDigTable->LargeFAHit, pDigTable->ForbiddenIGI));
	//RT_TRACE(COMP_DIG, DBG_LOUD, ("dm_DIG() After: Recover_cnt=%d, rx_gain_range_min=%x \n", 
	//			pDigTable->Recover_cnt, pDigTable->rx_gain_range_min));

	if(value_IGI > DM_DIG_MAX)			
		value_IGI = DM_DIG_MAX;
	else if(value_IGI < pDigTable->rx_gain_range_min)		
		value_IGI = pDigTable->rx_gain_range_min;

	pDigTable->CurIGValue = value_IGI;

#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
	if(pHalData->MacPhyMode92D == DUALMAC_SINGLEPHY)
	{
		DM_Write_DIG_DMSP(pAdapter);
		if(pHalData->CurrentBandType92D != BAND_ON_5G)
			dm_CCK_PacketDetectionThresh_DMSP(pAdapter);
	}
	else
	{
		DM_Write_DIG(pAdapter);
		if(pHalData->CurrentBandType92D != BAND_ON_5G)
			dm_CCK_PacketDetectionThresh(pAdapter);
	}
#else
	DM_Write_DIG(pAdapter);
	if(pHalData->CurrentBandType92D != BAND_ON_5G)
		dm_CCK_PacketDetectionThresh(pAdapter);
#endif

	//RT_TRACE(COMP_DIG, DBG_LOUD, ("dm_DIG() <<==\n"));
}


static void dm_InitDynamicTxPower(IN	PADAPTER	Adapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;

	pdmpriv->bDynamicTxPowerEnable = _TRUE;

	pdmpriv->LastDTPLvl = TxHighPwrLevel_Normal;
	pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Normal;
}

static void dm_DynamicTxPower(IN	PADAPTER	Adapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
	struct mlme_ext_priv *pmlmeext = &Adapter->mlmeextpriv;
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	int	UndecoratedSmoothedPWDB;


	// If dynamic high power is disabled.
	if( (pdmpriv->bDynamicTxPowerEnable != _TRUE) ||
		(!(pdmpriv->DMFlag & DYNAMIC_FUNC_HP)) )
	{
		pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Normal;
		return;
	}

	// STA not connected and AP not connected
	if((check_fwstate(pmlmepriv, _FW_LINKED) != _TRUE) &&	
		(pdmpriv->EntryMinUndecoratedSmoothedPWDB == 0))
	{
		//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("Not connected to any \n"));
		pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Normal;

		//the LastDTPlvl should reset when disconnect, 
		//otherwise the tx power level wouldn't change when disconnect and connect again.
		// Maddest 20091220.
		pdmpriv->LastDTPLvl=TxHighPwrLevel_Normal;
		return;
	}
	
	if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)	// Default port
	{
		//todo: AP Mode
		if ((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
		       (check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE))
		{
			UndecoratedSmoothedPWDB = pdmpriv->EntryMinUndecoratedSmoothedPWDB;
			//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("AP Client PWDB = 0x%x \n", UndecoratedSmoothedPWDB));
		}
		else
		{
			UndecoratedSmoothedPWDB = pdmpriv->UndecoratedSmoothedPWDB;
			//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("STA Default Port PWDB = 0x%x \n", UndecoratedSmoothedPWDB));
		}
	}
	else // associated entry pwdb
	{	
		UndecoratedSmoothedPWDB = pdmpriv->EntryMinUndecoratedSmoothedPWDB;
		//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("AP Ext Port PWDB = 0x%x \n", UndecoratedSmoothedPWDB));
	}
#if TX_POWER_FOR_5G_BAND == 1
	if(pHalData->CurrentBandType92D == 1){
		if(UndecoratedSmoothedPWDB >= 0x33)
		{
			pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Level2;
			//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("5G:TxHighPwrLevel_Level2 (TxPwr=0x0)\n"));
		}
		else if((UndecoratedSmoothedPWDB <0x33) &&
			(UndecoratedSmoothedPWDB >= 0x2b) )
		{
			pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Level1;
			//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("5G:TxHighPwrLevel_Level1 (TxPwr=0x10)\n"));
		}
		else if(UndecoratedSmoothedPWDB < 0x2b)
		{
			pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Normal;
			//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("5G:TxHighPwrLevel_Normal\n"));
		}
	}
	else
#endif
	{
		if(UndecoratedSmoothedPWDB >= TX_POWER_NEAR_FIELD_THRESH_LVL2)
		{
			pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Level2;
			//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("TxHighPwrLevel_Level1 (TxPwr=0x0)\n"));
		}
		else if((UndecoratedSmoothedPWDB < (TX_POWER_NEAR_FIELD_THRESH_LVL2-3)) &&
			(UndecoratedSmoothedPWDB >= TX_POWER_NEAR_FIELD_THRESH_LVL1) )
		{
			pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Level1;
			//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("TxHighPwrLevel_Level1 (TxPwr=0x10)\n"));
		}
		else if(UndecoratedSmoothedPWDB < (TX_POWER_NEAR_FIELD_THRESH_LVL1-5))
		{
			pdmpriv->DynamicTxHighPowerLvl = TxHighPwrLevel_Normal;
			//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("TxHighPwrLevel_Normal\n"));
		}
	}

	if( (pdmpriv->DynamicTxHighPowerLvl != pdmpriv->LastDTPLvl) )
	{
		//RT_TRACE(COMP_HIPWR, DBG_LOUD, ("PHY_SetTxPowerLevel8192S() Channel = %d \n" , pHalData->CurrentChannel));
		PHY_SetTxPowerLevel8192D(Adapter, pHalData->CurrentChannel);
		//set_channel_bwmode(Adapter, pmlmeext->cur_channel, pmlmeext->cur_ch_offset, pmlmeext->cur_bwmode);
	}
	pdmpriv->LastDTPLvl = pdmpriv->DynamicTxHighPowerLvl;
	
}


static VOID PWDB_Monitor(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	//u8	i;
	int	tmpEntryMaxPWDB=0, tmpEntryMinPWDB=0xff;

	//if(check_fwstate(&Adapter->mlmepriv, _FW_LINKED) != _TRUE)
	//	return;

#if 0 //todo:
	for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
	{
		if(Adapter->MgntInfo.NdisVersion == RT_NDIS_VERSION_6_2)
		{
			if(ACTING_AS_AP(ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, FALSE)))
				pEntry = AsocEntry_EnumStation(ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, FALSE), i);
			else
				pEntry = AsocEntry_EnumStation(ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, TRUE), i);
		}
		else
		{
			pEntry = AsocEntry_EnumStation(ADJUST_TO_ADAPTIVE_ADAPTER(Adapter, TRUE), i);	
		}

		if(pEntry!=NULL)
		{
			if(pEntry->bAssociated)
			{
				RTPRINT_ADDR(FDM, DM_PWDB, ("pEntry->MacAddr ="), pEntry->MacAddr);
				RTPRINT(FDM, DM_PWDB, ("pEntry->rssi = 0x%x(%d)\n", 
					pEntry->rssi_stat.UndecoratedSmoothedPWDB,
					pEntry->rssi_stat.UndecoratedSmoothedPWDB));
				if(pEntry->rssi_stat.UndecoratedSmoothedPWDB < tmpEntryMinPWDB)
					tmpEntryMinPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;
				if(pEntry->rssi_stat.UndecoratedSmoothedPWDB > tmpEntryMaxPWDB)
					tmpEntryMaxPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;
			}
		}
		else
		{
			break;
		}
	}
#endif

	if(tmpEntryMaxPWDB != 0)	// If associated entry is found
	{
		pdmpriv->EntryMaxUndecoratedSmoothedPWDB = tmpEntryMaxPWDB;		
	}
	else
	{
		pdmpriv->EntryMaxUndecoratedSmoothedPWDB = 0;
	}
	
	if(tmpEntryMinPWDB != 0xff) // If associated entry is found
	{
		pdmpriv->EntryMinUndecoratedSmoothedPWDB = tmpEntryMinPWDB;		
	}
	else
	{
		pdmpriv->EntryMinUndecoratedSmoothedPWDB = 0;
	}

	// Indicate Rx signal strength to FW.
	if(pHalData->fw_ractrl == _TRUE)
	{
		u32	temp = 0;
		//DBG_8192C("RxSS: %lx =%ld\n", pdmpriv->UndecoratedSmoothedPWDB, pdmpriv->UndecoratedSmoothedPWDB);
		temp = pdmpriv->UndecoratedSmoothedPWDB;
		temp = temp << 16;
		//temp = temp | 0x800000;
		temp = temp | 0x100 ;   // fw v12 cmdid 5:use max macid ,for nic ,default macid is 0 ,max macid is 1
		FillH2CCmd92D(Adapter, H2C_RSSI_REPORT, 3, (u8 *)(&temp));
	}
	else
	{
		write8(Adapter, 0x4fe, (u8)pdmpriv->UndecoratedSmoothedPWDB);
		//DBG_8192C("0x4fe write %x %d\n", pdmpriv->UndecoratedSmoothedPWDB, pdmpriv->UndecoratedSmoothedPWDB);
	}
}

static void
DM_InitEdcaTurbo(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	pHalData->bCurrentTurboEDCA = _FALSE;
	Adapter->recvpriv.bIsAnyNonBEPkts = _FALSE;
}	// DM_InitEdcaTurbo

static void
dm_CheckEdcaTurbo(
	IN	PADAPTER	Adapter
	)
{
	u32 	trafficIndex;
	u32	edca_param;
	u64	cur_tx_bytes = 0;
	u64	cur_rx_bytes = 0;
	u8	bbtchange = _FALSE;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv		*pdmpriv = &pHalData->dmpriv;
	struct xmit_priv		*pxmitpriv = &(Adapter->xmitpriv);
	struct recv_priv		*precvpriv = &(Adapter->recvpriv);
	struct registry_priv	*pregpriv = &Adapter->registrypriv;
	struct mlme_ext_priv	*pmlmeext = &(Adapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);


	if ((pregpriv->wifi_spec == 1) || (pmlmeext->cur_wireless_mode == WIRELESS_11B))
	{
		goto dm_CheckEdcaTurbo_EXIT;
	}

	if (pmlmeinfo->assoc_AP_vendor >= maxAP)
	{
		goto dm_CheckEdcaTurbo_EXIT;
	}

	// Check if the status needs to be changed.
	if((bbtchange) || (!precvpriv->bIsAnyNonBEPkts) )
	{
		cur_tx_bytes = pxmitpriv->tx_bytes - pxmitpriv->last_tx_bytes;
		cur_rx_bytes = precvpriv->rx_bytes - precvpriv->last_rx_bytes;

		//traffic, TX or RX
		if((pmlmeinfo->assoc_AP_vendor == ralinkAP)||(pmlmeinfo->assoc_AP_vendor == atherosAP))
		{
			if (cur_tx_bytes > (cur_rx_bytes << 2))
			{ // Uplink TP is present.
				trafficIndex = UP_LINK; 
			}
			else
			{ // Balance TP is present.
				trafficIndex = DOWN_LINK;
			}
		}
		else
		{
			if (cur_rx_bytes > (cur_tx_bytes << 2))
			{ // Downlink TP is present.
				trafficIndex = DOWN_LINK;
			}
			else
			{ // Balance TP is present.
				trafficIndex = UP_LINK;
			}
		}

		if ((pdmpriv->prv_traffic_idx != trafficIndex) || (!pHalData->bCurrentTurboEDCA))
		{
			{
				if((pmlmeinfo->assoc_AP_vendor == ciscoAP) && (pmlmeext->cur_wireless_mode & (WIRELESS_11_24N|WIRELESS_11_5N)))
				{
					edca_param = EDCAParam[pmlmeinfo->assoc_AP_vendor][trafficIndex];
				}
				else if((pmlmeinfo->assoc_AP_vendor == airgocapAP) &&
					((pmlmeext->cur_wireless_mode == WIRELESS_11G) ||(pmlmeext->cur_wireless_mode == WIRELESS_11BG)))
				{
					edca_param = EDCAParam[pmlmeinfo->assoc_AP_vendor][trafficIndex];
				}
				else
				{
					edca_param = EDCAParam[unknownAP][trafficIndex];
				}
			}

			write32(Adapter, REG_EDCA_BE_PARAM, 0x5ea42b);
			//write32(Adapter, REG_EDCA_BE_PARAM, edca_param);

			pdmpriv->prv_traffic_idx = trafficIndex;
		}
		
		pHalData->bCurrentTurboEDCA = _TRUE;
	}
	else
	{
		//
		// Turn Off EDCA turbo here.
		// Restore original EDCA according to the declaration of AP.
		//
		 if(pHalData->bCurrentTurboEDCA)
		{
			write32(Adapter, REG_EDCA_BE_PARAM, pHalData->AcParam_BE);
			pHalData->bCurrentTurboEDCA = _FALSE;
		}
	}

dm_CheckEdcaTurbo_EXIT:
	// Set variables for next time.
	precvpriv->bIsAnyNonBEPkts = _FALSE;
	pxmitpriv->last_tx_bytes = pxmitpriv->tx_bytes;
	precvpriv->last_rx_bytes = precvpriv->rx_bytes;

}	// dm_CheckEdcaTurbo


static void
FindMinimumRSSI(
IN	PADAPTER	pAdapter
	)
{	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	struct mlme_priv	*pmlmepriv = &pAdapter->mlmepriv;
	//pPS_T pDM_PSTable = &pdmpriv->DM_PSTable;
#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
	s32			Rssi_val_min_back_for_mac0;
	BOOLEAN		bGetValueFromBuddyAdapter = DualMacGetParameterFromBuddyAdapter(pAdapter);
	BOOLEAN		bRestoreRssi = _FALSE;
	PADAPTER	BuddyAdapter = pAdapter->BuddyAdapter;
#endif

	//1 1.Determine the minimum RSSI 
	if((check_fwstate(pmlmepriv, _FW_LINKED) == _FALSE) &&
		(pdmpriv->EntryMinUndecoratedSmoothedPWDB == 0))
	{
		pdmpriv->MinUndecoratedPWDBForDM = 0;
		//RT_TRACE(COMP_BB_POWERSAVING, DBG_LOUD, ("Not connected to any \n"));
	}
	if(check_fwstate(pmlmepriv, _FW_LINKED) == _TRUE)	// Default port
	{
		//if(ACTING_AS_AP(pAdapter) || pMgntInfo->mIbss)
		if ((check_fwstate(pmlmepriv, WIFI_ADHOC_MASTER_STATE) == _TRUE) ||
			(check_fwstate(pmlmepriv, WIFI_ADHOC_STATE) == _TRUE))	//todo: AP Mode
		{
			pdmpriv->MinUndecoratedPWDBForDM = pdmpriv->EntryMinUndecoratedSmoothedPWDB;
			//RT_TRACE(COMP_BB_POWERSAVING, DBG_LOUD, ("AP Client PWDB = 0x%lx \n", pdmpriv->MinUndecoratedPWDBForDM));
		}
		else
		{
			pdmpriv->MinUndecoratedPWDBForDM = pdmpriv->UndecoratedSmoothedPWDB;
			//RT_TRACE(COMP_BB_POWERSAVING, DBG_LOUD, ("STA Default Port PWDB = 0x%lx \n", pdmpriv->MinUndecoratedPWDBForDM));
		}
	}
	else // associated entry pwdb
	{	
		pdmpriv->MinUndecoratedPWDBForDM = pdmpriv->EntryMinUndecoratedSmoothedPWDB;
		//RT_TRACE(COMP_BB_POWERSAVING, DBG_LOUD, ("AP Ext Port or disconnet PWDB = 0x%lx \n", pdmpriv->MinUndecoratedPWDBForDM));
	}

#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
	if(pHalData->MacPhyMode92D == DUALMAC_SINGLEPHY)
	{
		if(BuddyAdapter!= NULL)
		{
			if(pAdapter->bSlaveOfDMSP)
			{
				//RT_TRACE(COMP_EASY_CONCURRENT,DBG_LOUD,("bSlavecase of dmsp\n"));
				BuddyAdapter->DualMacDMSPControl.RssiValMinForAnotherMacOfDMSP = pdmpriv->MinUndecoratedPWDBForDM;
			}
			else
			{
				if(bGetValueFromBuddyAdapter)
				{
					//RT_TRACE(COMP_EASY_CONCURRENT,DBG_LOUD,("get new RSSI\n"));
					bRestoreRssi = _TRUE;
					Rssi_val_min_back_for_mac0 = pdmpriv->MinUndecoratedPWDBForDM;
					pdmpriv->MinUndecoratedPWDBForDM = pAdapter->DualMacDMSPControl.RssiValMinForAnotherMacOfDMSP;
				}
			}
		}
		
	}

	if(bRestoreRssi)
	{
		bRestoreRssi = _FALSE;
		pdmpriv->MinUndecoratedPWDBForDM = Rssi_val_min_back_for_mac0;
	}
	
#endif
	//RT_TRACE(COMP_DIG, DBG_LOUD, ("MinUndecoratedPWDBForDM =%d\n",pdmpriv->MinUndecoratedPWDBForDM));
}


static void dm_InitDynamicBBPowerSaving(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	pPS_T pDM_PSTable = &pdmpriv->DM_PSTable;
	
	pDM_PSTable->PreCCAState = CCA_MAX;
	pDM_PSTable->CurCCAState = CCA_MAX;
	pDM_PSTable->PreRFState = RF_MAX;
	pDM_PSTable->CurRFState = RF_MAX;
}


static void
dm_DynamicBBPowerSaving(
IN	PADAPTER	pAdapter
	)
{	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct mlme_priv	*pmlmepriv = &pAdapter->mlmepriv;
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	pPS_T			pDM_PSTable = &pdmpriv->DM_PSTable;
#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
	s32			Rssi_val_min_back_for_mac0;
	BOOLEAN		bGetValueFromBuddyAdapter = DualMacGetParameterFromBuddyAdapter(pAdapter);
	BOOLEAN		bRestoreRssi = _FALSE;
	PADAPTER	BuddyAdapter = pAdapter->BuddyAdapter;
#endif

	
	//1 Power Saving for 92C
	if(IS_92D_SINGLEPHY(pHalData->VersionID))
	{
#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
		if(!pAdapter->bSlaveOfDMSP)
#endif
			dm_1R_CCA(pAdapter);
	}

#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
	if(bRestoreRssi)
	{
		bRestoreRssi = _FALSE;
		pdmpriv->MinUndecoratedPWDBForDM = Rssi_val_min_back_for_mac0;
	}
#endif

// 20100628 Joseph: Turn off BB power save for 88CE because it makesthroughput unstable.
#if 0
	//1 3.Power Saving for 88C
	if(!IS_92C_SERIAL(pHalData->VersionID))
	{
		dm_RF_Saving(pAdapter, FALSE);
	}
#endif
}

//091212 chiyokolin
static	VOID
dm_TXPowerTrackingCallback_ThermalMeter_92D(
            IN PADAPTER	Adapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u8			ThermalValue = 0, delta, delta_LCK, delta_IQK, index, offset, ThermalValue_AVG_count = 0;
	u32			ThermalValue_AVG = 0;
	s32 			ele_A, ele_D, TempCCk, X, value32;
	s32			Y, ele_C;
	s8			OFDM_index[2], CCK_index, OFDM_index_old[2], CCK_index_old, delta_APK;
	int	    		i = 0, CCKSwingNeedUpdate = 0;
	BOOLEAN		is2T = (IS_92C_SERIAL(pHalData->VersionID) || IS_92D_SINGLEPHY(pHalData->VersionID)) ;
	//PMPT_CONTEXT	pMptCtx = &(Adapter->MptCtx);	
	//u8*			TxPwrLevel = pMptCtx->TxPwrLevel;
	u8			OFDM_min_index = 6, rf; //OFDM BB Swing should be less than +3.0dB, which is required by Arthur
	u8			index_mapping[5][index_mapping_NUM] = {	
					{0,	1,	3,	6,	8,	9,				//5G, path A/MAC 0, decrease power 
					11,	13,	14,	16,	17,	18, 18},	
					{0,	2,	4,	5,	7,	10,				//5G, path A/MAC 0, increase power 
					12,	14,	16,	18,	18,	18,	18},					
					{0,	2,	3,	6,	8,	9,				//5G, path B/MAC 1, decrease power
					11,	13,	14,	16,	17,	18,	18},		
					{0,	2,	4,	5,	7,	10,				//5G, path B/MAC 1, increase power
					13,	16,	16,	18,	18,	18,	18},					
					{0,	1,	2,	3,	4,	5,				//2.4G, for decreas power
					6,	7,	7,	8,	9,	10,	10},												
					};


//#if MP_DRIVER != 1
//	return;
//#endif

	pdmpriv->TXPowerTrackingCallbackCnt++;	//cosa add for debug
	pdmpriv->bTXPowerTrackingInit = _TRUE;

	if(pHalData->CurrentChannel == 14 && !pdmpriv->bCCKinCH14)
		pdmpriv->bCCKinCH14 = _TRUE;
	else if(pHalData->CurrentChannel != 14 && pdmpriv->bCCKinCH14)
		pdmpriv->bCCKinCH14 = _FALSE;

	//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("===>dm_TXPowerTrackingCallback_ThermalMeter_92D interface %d txpowercontrol %d\n", pHalData->interfaceIndex, pdmpriv->TxPowerTrackControl));

	ThermalValue = (u8)PHY_QueryRFReg(Adapter, RF90_PATH_A, RF_T_METER, 0xf800);	//0x42: RF Reg[15:11] 92D

	//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Readback Thermal Meter = 0x%lx pre thermal meter 0x%lx EEPROMthermalmeter 0x%lx\n", ThermalValue, pHalData->ThermalValue, pHalData->EEPROMThermalMeter));

	rtl8192d_PHY_APCalibrate(Adapter, (ThermalValue - pHalData->EEPROMThermalMeter));//notes:EEPROMThermalMeter is a fixed value from efuse/eeprom

//	if(!pHalData->TxPowerTrackControl)
//		return;

	if(is2T)
		rf = 2;
	else
		rf = 1;

	if(ThermalValue)
	{
//		if(!pHalData->ThermalValue)
		{
			//Query OFDM path A default setting 		
			ele_D = PHY_QueryBBReg(Adapter, rOFDM0_XATxIQImbalance, bMaskDWord)&bMaskOFDM_D;
			for(i=0; i<OFDM_TABLE_SIZE_92D; i++)	//find the index
			{
				if(ele_D == (OFDMSwingTable[i]&bMaskOFDM_D))
				{
					OFDM_index_old[0] = (u8)i;
					//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Initial pathA ele_D reg0x%x = 0x%lx, OFDM_index=0x%x\n", 
					//	rOFDM0_XATxIQImbalance, ele_D, OFDM_index_old[0]));
					break;
				}
			}

			//Query OFDM path B default setting 
			if(is2T)
			{
				ele_D = PHY_QueryBBReg(Adapter, rOFDM0_XBTxIQImbalance, bMaskDWord)&bMaskOFDM_D;
				for(i=0; i<OFDM_TABLE_SIZE_92D; i++)	//find the index
				{
					if(ele_D == (OFDMSwingTable[i]&bMaskOFDM_D))
					{
						OFDM_index_old[1] = (u8)i;
						//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Initial pathB ele_D reg0x%x = 0x%lx, OFDM_index=0x%x\n", 
						//	rOFDM0_XBTxIQImbalance, ele_D, OFDM_index_old[1]));
						break;
					}
				}
			}

			if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
			{
				//Query CCK default setting From 0xa24
				TempCCk = PHY_QueryBBReg(Adapter, rCCK0_TxFilter2, bMaskDWord)&bMaskCCK;
				for(i=0 ; i<CCK_TABLE_SIZE ; i++)
				{
					if(pdmpriv->bCCKinCH14)
					{
						if(_memcmp((void*)&TempCCk, (void*)&CCKSwingTable_Ch14[i][2], 4)==_TRUE)
						{
							CCK_index_old =(u8)i;
							//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Initial reg0x%x = 0x%lx, CCK_index=0x%x, ch 14 %d\n", 
							//	rCCK0_TxFilter2, TempCCk, CCK_index_old, pdmpriv->bCCKinCH14));
							break;
						}
					}
					else
					{
						if(_memcmp((void*)&TempCCk, (void*)&CCKSwingTable_Ch1_Ch13[i][2], 4)==_TRUE)
						{
							CCK_index_old =(u8)i;
							//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Initial reg0x%x = 0x%lx, CCK_index=0x%x, ch14 %d\n", 
							//	rCCK0_TxFilter2, TempCCk, CCK_index_old, pdmpriv->bCCKinCH14));
							break;
						}			
					}
				}
			}
			else
			{
				TempCCk = 0x090e1317;
				CCK_index_old = 12;
			}

			if(!pdmpriv->ThermalValue)
			{
				pdmpriv->ThermalValue = pHalData->EEPROMThermalMeter;
				pdmpriv->ThermalValue_LCK = ThermalValue;				
				pdmpriv->ThermalValue_IQK = ThermalValue;								
				
				for(i = 0; i < rf; i++)
					pdmpriv->OFDM_index[i] = OFDM_index_old[i];
				pdmpriv->CCK_index = CCK_index_old;
			}

			if(pdmpriv->bReloadtxpowerindex)
			{
				//pdmpriv->bReloadtxpowerindex = FALSE;
				for(i = 0; i < rf; i++)
					pdmpriv->OFDM_index[i] = OFDM_index_old[i];
				pdmpriv->CCK_index = CCK_index_old;	
				//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("reload ofdm index for band switch\n"));				
			}

			//calculate average thermal meter
			{
				pdmpriv->ThermalValue_AVG[pdmpriv->ThermalValue_AVG_index] = ThermalValue;
				pdmpriv->ThermalValue_AVG_index++;
				if(pdmpriv->ThermalValue_AVG_index == AVG_THERMAL_NUM)
					pdmpriv->ThermalValue_AVG_index = 0;

				for(i = 0; i < AVG_THERMAL_NUM; i++)
				{
					if(pdmpriv->ThermalValue_AVG[i])
					{
						ThermalValue_AVG += pdmpriv->ThermalValue_AVG[i];
						ThermalValue_AVG_count++;
					}
				}

				if(ThermalValue_AVG_count)
					ThermalValue = (u8)(ThermalValue_AVG / ThermalValue_AVG_count);
			}
		}

		if(pdmpriv->bReloadtxpowerindex)
		{
			delta = ThermalValue > pHalData->EEPROMThermalMeter?(ThermalValue - pHalData->EEPROMThermalMeter):(pHalData->EEPROMThermalMeter - ThermalValue);				
			pdmpriv->bReloadtxpowerindex = _FALSE;	
			pdmpriv->bDoneTxpower = _FALSE;
		}
		else if(pdmpriv->bDoneTxpower)
		{
			delta = (ThermalValue > pdmpriv->ThermalValue)?(ThermalValue - pdmpriv->ThermalValue):(pdmpriv->ThermalValue - ThermalValue);
		}
		else
		{
			delta = ThermalValue > pHalData->EEPROMThermalMeter?(ThermalValue - pHalData->EEPROMThermalMeter):(pHalData->EEPROMThermalMeter - ThermalValue);		
		}
		delta_LCK = (ThermalValue > pdmpriv->ThermalValue_LCK)?(ThermalValue - pdmpriv->ThermalValue_LCK):(pdmpriv->ThermalValue_LCK - ThermalValue);
		delta_IQK = (ThermalValue > pdmpriv->ThermalValue_IQK)?(ThermalValue - pdmpriv->ThermalValue_IQK):(pdmpriv->ThermalValue_IQK - ThermalValue);

		//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("interface %d Readback Thermal Meter = 0x%lx pre thermal meter 0x%lx EEPROMthermalmeter 0x%lx delta 0x%lx delta_LCK 0x%lx delta_IQK 0x%lx\n",  pHalData->interfaceIndex, ThermalValue, pdmpriv->ThermalValue, pHalData->EEPROMThermalMeter, delta, delta_LCK, delta_IQK));

		if((delta_LCK > pdmpriv->Delta_LCK) && (pdmpriv->Delta_LCK != 0))
		{
			pdmpriv->ThermalValue_LCK = ThermalValue;
			rtl8192d_PHY_LCCalibrate(Adapter);
		}
		
		if(delta > 0 && pdmpriv->TxPowerTrackControl)
		{
			pdmpriv->bDoneTxpower = _TRUE;
			delta = ThermalValue > pHalData->EEPROMThermalMeter?(ThermalValue - pHalData->EEPROMThermalMeter):(pHalData->EEPROMThermalMeter - ThermalValue);

			//calculate new OFDM / CCK offset	
			{
				if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
				{
					offset = 4;
				
					if(delta > index_mapping_NUM-1)					
						index = index_mapping[offset][index_mapping_NUM-1];
					else
						index = index_mapping[offset][delta];
				
					if(ThermalValue > pHalData->EEPROMThermalMeter)
					{ 
						for(i = 0; i < rf; i++)
						 	OFDM_index[i] = pdmpriv->OFDM_index[i] -delta;
						CCK_index = pdmpriv->CCK_index -delta;
					}
					else
					{
						for(i = 0; i < rf; i++)			
							OFDM_index[i] = pdmpriv->OFDM_index[i] + index;
						CCK_index = pdmpriv->CCK_index + index;
					}
				}
				else if(pHalData->CurrentBandType92D == BAND_ON_5G)
				{
					for(i = 0; i < rf; i++)
					{
						if(pHalData->interfaceIndex == 1 || i == rf)
							offset = 2;
						else
							offset = 0;

						if(ThermalValue > pHalData->EEPROMThermalMeter)	//set larger Tx power
							offset++;		
						
						if(delta > index_mapping_NUM-1)					
							index = index_mapping[offset][index_mapping_NUM-1];
						else
							index = index_mapping[offset][delta];
						
						if(ThermalValue > pHalData->EEPROMThermalMeter)	//set larger Tx power
						{
							 OFDM_index[i] = pdmpriv->OFDM_index[i] -index;
						}
						else
						{				
							OFDM_index[i] = pdmpriv->OFDM_index[i] + index;
						}
					}
				}
				
				if(is2T)
				{
					//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("temp OFDM_A_index=0x%x, OFDM_B_index=0x%x, CCK_index=0x%x\n", 
					//	pdmpriv->OFDM_index[0], pdmpriv->OFDM_index[1], pdmpriv->CCK_index));			
				}
				else
				{
					//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("temp OFDM_A_index=0x%x, CCK_index=0x%x\n", 
					//	pdmpriv->OFDM_index[0], pdmpriv->CCK_index));			
				}
				
				for(i = 0; i < rf; i++)
				{
					if(OFDM_index[i] > OFDM_TABLE_SIZE_92D-1)
						OFDM_index[i] = OFDM_TABLE_SIZE_92D-1;
					else if (OFDM_index[i] < OFDM_min_index)
						OFDM_index[i] = OFDM_min_index;
				}

				if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
				{
					if(CCK_index > CCK_TABLE_SIZE-1)
						CCK_index = CCK_TABLE_SIZE-1;
					else if (CCK_index < 0)
						CCK_index = 0;
				}

				if(is2T)
				{
					//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("new OFDM_A_index=0x%x, OFDM_B_index=0x%x, CCK_index=0x%x\n", 
					//	OFDM_index[0], OFDM_index[1], CCK_index));
				}
				else
				{
					//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("new OFDM_A_index=0x%x, CCK_index=0x%x\n", 
					//	OFDM_index[0], CCK_index));	
				}
			}

			//Config by SwingTable
			{
				//Adujst OFDM Ant_A according to IQK result
				ele_D = (OFDMSwingTable[OFDM_index[0]] & 0xFFC00000)>>22;
				X = pdmpriv->RegE94;
				Y = pdmpriv->RegE9C;
				//X = pHalData->IQKMatrixRegSetting[Indexforchannel].Value[9];
				//Y = pHalData->IQKMatrixRegSetting[Indexforchannel].Value[10];

				if(X != 0)
				{
					if ((X & 0x00000200) != 0)
						X = X | 0xFFFFFC00;
					ele_A = ((X * ele_D)>>8)&0x000003FF;
						
					//new element C = element D x Y
					if ((Y & 0x00000200) != 0)
						Y = Y | 0xFFFFFC00;
					ele_C = ((Y * ele_D)>>8)&0x000003FF;
					
					//wirte new elements A, C, D to regC80 and regC94, element B is always 0
					value32 = (ele_D<<22)|((ele_C&0x3F)<<16)|ele_A;
					PHY_SetBBReg(Adapter, rOFDM0_XATxIQImbalance, bMaskDWord, value32);

					//if(pHalData->IQKMatrixRegSetting[Indexforchannel].bIQKDone)
					//	pHalData->IQKMatrixRegSetting[Indexforchannel].Value[4] = value32;

					value32 = (ele_C&0x000003C0)>>6;
					PHY_SetBBReg(Adapter, rOFDM0_XCTxAFE, bMaskH4Bits, value32);

					value32 = ((X * ele_D)>>7)&0x01;
					PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold, BIT24, value32);
					
				}
				else
				{
					PHY_SetBBReg(Adapter, rOFDM0_XATxIQImbalance, bMaskDWord, OFDMSwingTable[OFDM_index[0]]);				
					PHY_SetBBReg(Adapter, rOFDM0_XCTxAFE, bMaskH4Bits, 0x00);
					PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold, BIT24, 0x00);			
				}

				
				if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
				{
					//Adjust CCK according to IQK result
					if(!pdmpriv->bCCKinCH14){
						write8(Adapter, 0xa22, CCKSwingTable_Ch1_Ch13[CCK_index][0]);
						write8(Adapter, 0xa23, CCKSwingTable_Ch1_Ch13[CCK_index][1]);
						write8(Adapter, 0xa24, CCKSwingTable_Ch1_Ch13[CCK_index][2]);
						write8(Adapter, 0xa25, CCKSwingTable_Ch1_Ch13[CCK_index][3]);
						write8(Adapter, 0xa26, CCKSwingTable_Ch1_Ch13[CCK_index][4]);
						write8(Adapter, 0xa27, CCKSwingTable_Ch1_Ch13[CCK_index][5]);
						write8(Adapter, 0xa28, CCKSwingTable_Ch1_Ch13[CCK_index][6]);
						write8(Adapter, 0xa29, CCKSwingTable_Ch1_Ch13[CCK_index][7]);		
					}
					else{
						write8(Adapter, 0xa22, CCKSwingTable_Ch14[CCK_index][0]);
						write8(Adapter, 0xa23, CCKSwingTable_Ch14[CCK_index][1]);
						write8(Adapter, 0xa24, CCKSwingTable_Ch14[CCK_index][2]);
						write8(Adapter, 0xa25, CCKSwingTable_Ch14[CCK_index][3]);
						write8(Adapter, 0xa26, CCKSwingTable_Ch14[CCK_index][4]);
						write8(Adapter, 0xa27, CCKSwingTable_Ch14[CCK_index][5]);
						write8(Adapter, 0xa28, CCKSwingTable_Ch14[CCK_index][6]);
						write8(Adapter, 0xa29, CCKSwingTable_Ch14[CCK_index][7]);	
					}		
				}
				
				if(is2T)
				{						
					ele_D = (OFDMSwingTable[OFDM_index[1]] & 0xFFC00000)>>22;
					
					//new element A = element D x X
					X = pdmpriv->RegEB4;
					Y = pdmpriv->RegEBC;
					//X = pHalData->IQKMatrixRegSetting[Indexforchannel].Value[11];
					//Y = pHalData->IQKMatrixRegSetting[Indexforchannel].Value[12];
					
					if(X != 0){
						if ((X & 0x00000200) != 0)	//consider minus
							X = X | 0xFFFFFC00;
						ele_A = ((X * ele_D)>>8)&0x000003FF;
						
						//new element C = element D x Y
						if ((Y & 0x00000200) != 0)
							Y = Y | 0xFFFFFC00;
						ele_C = ((Y * ele_D)>>8)&0x00003FF;
						
						//wirte new elements A, C, D to regC88 and regC9C, element B is always 0
						value32=(ele_D<<22)|((ele_C&0x3F)<<16) |ele_A;
						PHY_SetBBReg(Adapter, rOFDM0_XBTxIQImbalance, bMaskDWord, value32);

						//if(pHalData->IQKMatrixRegSetting[Indexforchannel].bIQKDone)
						//	pHalData->IQKMatrixRegSetting[Indexforchannel].Value[5] = value32;

						value32 = (ele_C&0x000003C0)>>6;
						PHY_SetBBReg(Adapter, rOFDM0_XDTxAFE, bMaskH4Bits, value32);	
						
						value32 = ((X * ele_D)>>7)&0x01;
						PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold, BIT28, value32);

					}
					else{
						PHY_SetBBReg(Adapter, rOFDM0_XBTxIQImbalance, bMaskDWord, OFDMSwingTable[OFDM_index[1]]);										
						PHY_SetBBReg(Adapter, rOFDM0_XDTxAFE, bMaskH4Bits, 0x00);	
						PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold, BIT28, 0x00);				
					}

				}

				//rtl8192d_PHY_ResetIQKResult(Adapter);
				//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD, ("TxPwrTracking 0xc80 = 0x%x, 0xc94 = 0x%x RF 0x24 = 0x%x\n", PHY_QueryBBReg(Adapter, 0xc80, bMaskDWord), PHY_QueryBBReg(Adapter, 0xc94, bMaskDWord), PHY_QueryRFReg(Adapter, RF90_PATH_A, 0x24, bRFRegOffsetMask)));
			}
		}

		if((delta_IQK > pdmpriv->Delta_IQK) && (pdmpriv->Delta_IQK != 0))
		{
			pdmpriv->ThermalValue_IQK = ThermalValue;
			rtl8192d_PHY_IQCalibrate(Adapter);
		}

		//update thermal meter value
		//if(pHalData->TxPowerTrackControl)
		//{
		//	Adapter->HalFunc.SetHalDefVarHandler(Adapter, HAL_DEF_THERMAL_VALUE, &ThermalValue);
		//}
	
	}

	//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("<===dm_TXPowerTrackingCallback_ThermalMeter_92D\n"));
	
	pdmpriv->TXPowercount = 0;

}


static	VOID
dm_InitializeTXPowerTracking_ThermalMeter(
	IN	PADAPTER		Adapter)
{	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;

	//if(IS_HARDWARE_TYPE_8192C(pHalData))
	{
		pdmpriv->bTXPowerTracking = _TRUE;
		pdmpriv->TXPowercount = 0;
		pdmpriv->bTXPowerTrackingInit = _FALSE;
		
#if	(MP_DRIVER != 1)	//for mp driver, turn off txpwrtracking as default
		pdmpriv->TxPowerTrackControl = _TRUE;
#endif
	}
	MSG_8192C("pdmpriv->TxPowerTrackControl = %d\n", pdmpriv->TxPowerTrackControl);
}


static VOID
DM_InitializeTXPowerTracking(
	IN	PADAPTER		Adapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	//if(IS_HARDWARE_TYPE_8192C(pHalData))
	{
		dm_InitializeTXPowerTracking_ThermalMeter(Adapter);
	}
}	

//
//	Description:
//		- Dispatch TxPower Tracking direct call ONLY for 92s.
//		- We shall NOT schedule Workitem within PASSIVE LEVEL, which will cause system resource
//		   leakage under some platform.
//
//	Assumption:
//		PASSIVE_LEVEL when this routine is called.
//
//	Added by Roger, 2009.06.18.
//
static VOID
DM_TXPowerTracking92CDirectCall(
            IN	PADAPTER		Adapter)
{	
	dm_TXPowerTrackingCallback_ThermalMeter_92D(Adapter);
}

static VOID
dm_CheckTXPowerTracking_ThermalMeter(
	IN	PADAPTER		Adapter)
{	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	//u1Byte					TxPowerCheckCnt = 5;	//10 sec

	if(!(pdmpriv->DMFlag & DYNAMIC_FUNC_SS))
	{
		return;
	}

	if(!pdmpriv->bTXPowerTracking /*|| (!pHalData->TxPowerTrackControl && pHalData->bAPKdone)*/)
	{
		return;
	}

	if(!pdmpriv->TM_Trigger)		//at least delay 1 sec
	{
		//pHalData->TxPowerCheckCnt++;	//cosa add for debug

		PHY_SetRFReg(Adapter, RF90_PATH_A, RF_T_METER, BIT17 | BIT16, 0x03);
		//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Trigger 92C Thermal Meter!!\n"));
		pdmpriv->TM_Trigger = 1;
		return;
	}
	else
	{
		//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("Schedule TxPowerTracking direct call!!\n"));		
		DM_TXPowerTracking92CDirectCall(Adapter); //Using direct call is instead, added by Roger, 2009.06.18.
		pdmpriv->TM_Trigger = 0;
	}

}


VOID
rtl8192d_dm_CheckTXPowerTracking(
	IN	PADAPTER		Adapter)
{
	//RT_TRACE(COMP_POWER_TRACKING, DBG_LOUD,("dm_CheckTXPowerTracking!!\n"));

#if DISABLE_BB_RF
	return;
#endif
	dm_CheckTXPowerTracking_ThermalMeter(Adapter);
}


#if (DEV_BUS_TYPE==DEV_BUS_PCI_INTERFACE)

BOOLEAN
BT_BTStateChange(
	IN	PADAPTER	Adapter
	)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	
	u4Byte 			temp, Polling, Ratio_Tx, Ratio_PRI;
	u4Byte 			BT_Tx, BT_PRI;
	u1Byte			BT_State;
	static u1Byte		ServiceTypeCnt = 0;
	u1Byte			CurServiceType;
	static u1Byte		LastServiceType = BT_Idle;

	if(!pMgntInfo->bMediaConnect)
		return FALSE;
	
	BT_State = PlatformEFIORead1Byte(Adapter, 0x4fd);
/*
	temp = PlatformEFIORead4Byte(Adapter, 0x488);
	BT_Tx = (u2Byte)(((temp<<8)&0xff00)+((temp>>8)&0xff));
	BT_PRI = (u2Byte)(((temp>>8)&0xff00)+((temp>>24)&0xff));

	temp = PlatformEFIORead4Byte(Adapter, 0x48c);
	Polling = ((temp<<8)&0xff000000) + ((temp>>8)&0x00ff0000) + 
			((temp<<8)&0x0000ff00) + ((temp>>8)&0x000000ff);
	
*/
	BT_Tx = PlatformEFIORead4Byte(Adapter, 0x488);
	
	RTPRINT(FBT, BT_TRACE, ("Ratio 0x488  =%x\n", BT_Tx));
	BT_Tx =BT_Tx & 0x00ffffff;
	//RTPRINT(FBT, BT_TRACE, ("Ratio BT_Tx  =%x\n", BT_Tx));

	BT_PRI = PlatformEFIORead4Byte(Adapter, 0x48c);
	
	RTPRINT(FBT, BT_TRACE, ("Ratio Ratio 0x48c  =%x\n", BT_PRI));
	BT_PRI =BT_PRI & 0x00ffffff;
	//RTPRINT(FBT, BT_TRACE, ("Ratio BT_PRI  =%x\n", BT_PRI));


	Polling = PlatformEFIORead4Byte(Adapter, 0x490);
	//RTPRINT(FBT, BT_TRACE, ("Ratio 0x490  =%x\n", Polling));


	if(BT_Tx==0xffffffff && BT_PRI==0xffffffff && Polling==0xffffffffff && BT_State==0xff)
		return FALSE;

	BT_State &= BIT0;

	if(BT_State != pHalData->bt_coexist.BT_CUR_State)
	{
		pHalData->bt_coexist.BT_CUR_State = BT_State;
	
		if(pMgntInfo->bRegBT_Sco == 3)
		{
			ServiceTypeCnt = 0;
		
			pHalData->bt_coexist.BT_Service = BT_Idle;

			RTPRINT(FBT, BT_TRACE, ("BT_%s\n", BT_State?"ON":"OFF"));

			BT_State = BT_State | 
					((pHalData->bt_coexist.BT_Ant_isolation==1)?0:BIT1) |BIT2;

			PlatformEFIOWrite1Byte(Adapter, 0x4fd, BT_State);
			RTPRINT(FBT, BT_TRACE, ("BT set 0x4fd to %x\n", BT_State));
		}
		
		return TRUE;
	}
	RTPRINT(FBT, BT_TRACE, ("bRegBT_Sco   %d\n", pMgntInfo->bRegBT_Sco));

	Ratio_Tx = BT_Tx*1000/Polling;
	Ratio_PRI = BT_PRI*1000/Polling;

	pHalData->bt_coexist.Ratio_Tx=Ratio_Tx;
	pHalData->bt_coexist.Ratio_PRI=Ratio_PRI;
	
	RTPRINT(FBT, BT_TRACE, ("Ratio_Tx=%d\n", Ratio_Tx));
	RTPRINT(FBT, BT_TRACE, ("Ratio_PRI=%d\n", Ratio_PRI));

	
	if(BT_State && pMgntInfo->bRegBT_Sco==3)
	{
		RTPRINT(FBT, BT_TRACE, ("bRegBT_Sco  ==3 Follow Counter\n"));
//		if(BT_Tx==0xffff && BT_PRI==0xffff && Polling==0xffffffff)
//		{
//			ServiceTypeCnt = 0;
//			return FALSE;
//		}
//		else
		{
		/*
			Ratio_Tx = BT_Tx*1000/Polling;
			Ratio_PRI = BT_PRI*1000/Polling;

			pHalData->bt_coexist.Ratio_Tx=Ratio_Tx;
			pHalData->bt_coexist.Ratio_PRI=Ratio_PRI;
			
			RTPRINT(FBT, BT_TRACE, ("Ratio_Tx=%d\n", Ratio_Tx));
			RTPRINT(FBT, BT_TRACE, ("Ratio_PRI=%d\n", Ratio_PRI));

		*/	
			if((Ratio_Tx <= 50)  && (Ratio_PRI <= 50)) 
			  	CurServiceType = BT_Idle;
			else if((Ratio_PRI > 150) && (Ratio_PRI < 200))
				CurServiceType = BT_SCO;
			else if((Ratio_Tx >= 200)&&(Ratio_PRI >= 200))
				CurServiceType = BT_Busy;
			else if(Ratio_Tx >= 350)
				CurServiceType = BT_OtherBusy;
			else
				CurServiceType=BT_OtherAction;

		}
/*		if(pHalData->bt_coexist.bStopCount)
		{
			ServiceTypeCnt=0;
			pHalData->bt_coexist.bStopCount=FALSE;
		}
*/
		if(CurServiceType == BT_OtherBusy)
		{
			ServiceTypeCnt=2;
			LastServiceType=CurServiceType;
		}
		else if(CurServiceType == LastServiceType)
		{
			if(ServiceTypeCnt<3)
				ServiceTypeCnt++;
		}
		else
		{
			ServiceTypeCnt = 0;
			LastServiceType = CurServiceType;
		}

		if(ServiceTypeCnt==2)
		{
			pHalData->bt_coexist.BT_Service = LastServiceType;
			BT_State = BT_State | 
					((pHalData->bt_coexist.BT_Ant_isolation==1)?0:BIT1) |
					((pHalData->bt_coexist.BT_Service==BT_SCO)?0:BIT2);

			if(pHalData->bt_coexist.BT_Service==BT_Busy)
				BT_State&= ~(BIT2);

			if(pHalData->bt_coexist.BT_Service==BT_SCO)
			{
				RTPRINT(FBT, BT_TRACE, ("BT TYPE Set to  ==> BT_SCO\n"));
			}
			else if(pHalData->bt_coexist.BT_Service==BT_Idle)
			{
				RTPRINT(FBT, BT_TRACE, ("BT TYPE Set to  ==> BT_Idle\n"));
			}
			else if(pHalData->bt_coexist.BT_Service==BT_OtherAction)
			{
				RTPRINT(FBT, BT_TRACE, ("BT TYPE Set to  ==> BT_OtherAction\n"));
			}
			else if(pHalData->bt_coexist.BT_Service==BT_Busy)
			{
				RTPRINT(FBT, BT_TRACE, ("BT TYPE Set to  ==> BT_Busy\n"));
			}
			else
			{
				RTPRINT(FBT, BT_TRACE, ("BT TYPE Set to ==> BT_OtherBusy\n"));
			}
				
			//Add interrupt migration when bt is not in idel state (no traffic).
			//suggestion by Victor.
			if(pHalData->bt_coexist.BT_Service!=BT_Idle)
			{
			
				PlatformEFIOWrite2Byte(Adapter, 0x504, 0x0ccc);
				PlatformEFIOWrite1Byte(Adapter, 0x506, 0x54);
				PlatformEFIOWrite1Byte(Adapter, 0x507, 0x54);
			
			}
			else
			{
				PlatformEFIOWrite1Byte(Adapter, 0x506, 0x00);
				PlatformEFIOWrite1Byte(Adapter, 0x507, 0x00);			
			}
				
			PlatformEFIOWrite1Byte(Adapter, 0x4fd, BT_State);
			RTPRINT(FBT, BT_TRACE, ("BT_SCO set 0x4fd to %x\n", BT_State));
			return TRUE;
		}
	}

	return FALSE;

}

BOOLEAN
BT_WifiConnectChange(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	static BOOLEAN	bMediaConnect = FALSE;

	if(!pMgntInfo->bMediaConnect || MgntRoamingInProgress(pMgntInfo))
	{
		bMediaConnect = FALSE;
	}
	else
	{
		if(!bMediaConnect)
		{
			bMediaConnect = TRUE;
			return TRUE;
		}
		bMediaConnect = TRUE;
	}

	return FALSE;
}

BOOLEAN
BT_RSSIChangeWithAMPDU(
	IN	PADAPTER	Adapter
	)
{
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(!Adapter->pNdisCommon->bRegBT_Ampdu || !Adapter->pNdisCommon->bRegAcceptAddbaReq)
		return FALSE;

	RTPRINT(FBT, BT_TRACE, ("RSSI is %d\n",pHalData->UndecoratedSmoothedPWDB));
	
	if((pHalData->UndecoratedSmoothedPWDB<=32) && pMgntInfo->pHTInfo->bAcceptAddbaReq)
	{
		RTPRINT(FBT, BT_TRACE, ("BT_Disallow AMPDU RSSI <=32  Need change\n"));				
		return TRUE;

	}
	else if((pHalData->UndecoratedSmoothedPWDB>=40) && !pMgntInfo->pHTInfo->bAcceptAddbaReq )
	{
		RTPRINT(FBT, BT_TRACE, ("BT_Allow AMPDU RSSI >=40, Need change\n"));
		return TRUE;
	}
	else 
		return FALSE;

}


VOID
dm_BTCoexist(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	PMGNT_INFO		pMgntInfo = &Adapter->MgntInfo;
	static u1Byte		LastTxPowerLvl = 0xff;
	PRX_TS_RECORD	pRxTs = NULL;

	BOOLEAN			bWifiConnectChange, bBtStateChange,bRSSIChangeWithAMPDU;

	if( (pHalData->bt_coexist.BluetoothCoexist) &&
		(pHalData->bt_coexist.BT_CoexistType == BT_CSR_BC4) && 
		(!ACTING_AS_AP(Adapter))	)
	{
		bWifiConnectChange = BT_WifiConnectChange(Adapter);
		bBtStateChange = BT_BTStateChange(Adapter);
		bRSSIChangeWithAMPDU = BT_RSSIChangeWithAMPDU(Adapter);
		RTPRINT(FBT, BT_TRACE, ("bWifiConnectChange %d, bBtStateChange  %d,LastTxPowerLvl  %x,  DynamicTxHighPowerLvl  %x\n",
			bWifiConnectChange,bBtStateChange,LastTxPowerLvl,pHalData->DynamicTxHighPowerLvl));
		if( bWifiConnectChange ||bBtStateChange  ||
			(LastTxPowerLvl != pHalData->DynamicTxHighPowerLvl)	||bRSSIChangeWithAMPDU)
		{
			LastTxPowerLvl = pHalData->DynamicTxHighPowerLvl;

			if(pHalData->bt_coexist.BT_CUR_State)
			{
				// Do not allow receiving A-MPDU aggregation.
				if((pHalData->bt_coexist.BT_Service==BT_SCO) || (pHalData->bt_coexist.BT_Service==BT_Busy))
				{
					if(pHalData->UndecoratedSmoothedPWDB<=32)
					{
						if(Adapter->pNdisCommon->bRegBT_Ampdu && Adapter->pNdisCommon->bRegAcceptAddbaReq)
						{
							RTPRINT(FBT, BT_TRACE, ("BT_Disallow AMPDU RSSI <=32\n"));	
							pMgntInfo->pHTInfo->bAcceptAddbaReq = FALSE;
							if(GetTs(Adapter, (PTS_COMMON_INFO*)(&pRxTs), pMgntInfo->Bssid, 0, RX_DIR, FALSE))
								TsInitDelBA(Adapter, (PTS_COMMON_INFO)pRxTs, RX_DIR);
						}
					}
					else if(pHalData->UndecoratedSmoothedPWDB>=40)
					{
						if(Adapter->pNdisCommon->bRegBT_Ampdu && Adapter->pNdisCommon->bRegAcceptAddbaReq)
						{
							RTPRINT(FBT, BT_TRACE, ("BT_Allow AMPDU  RSSI >=40\n"));	
							pMgntInfo->pHTInfo->bAcceptAddbaReq = TRUE;
						}
					}
				}
				else
				{
					if(Adapter->pNdisCommon->bRegBT_Ampdu && Adapter->pNdisCommon->bRegAcceptAddbaReq)
					{
						RTPRINT(FBT, BT_TRACE, ("BT_Allow AMPDU BT not in SCO or BUSY\n"));	
						pMgntInfo->pHTInfo->bAcceptAddbaReq = TRUE;
					}
				}

				if(pHalData->bt_coexist.BT_Ant_isolation)
				{			
					RTPRINT(FBT, BT_TRACE, ("BT_IsolationLow\n"));
					RTPRINT(FBT, BT_TRACE, ("BT_Update Rate table\n"));
					Adapter->HalFunc.UpdateHalRATRTableHandler(
								Adapter, 
								&pMgntInfo->dot11OperationalRateSet,
								pMgntInfo->dot11HTOperationalRateSet,NULL);
					
					if(pHalData->bt_coexist.BT_Service==BT_SCO)
					{

						RTPRINT(FBT, BT_TRACE, ("BT_Turn OFF Coexist with SCO \n"));
						PlatformEFIOWrite1Byte(Adapter, REG_GPIO_MUXCFG, 0x14);					
					}
					else if(pHalData->DynamicTxHighPowerLvl == TxHighPwrLevel_Normal)
					{
						RTPRINT(FBT, BT_TRACE, ("BT_Turn ON Coexist\n"));
						PlatformEFIOWrite1Byte(Adapter, REG_GPIO_MUXCFG, 0xb4);
					}
					else
					{
						RTPRINT(FBT, BT_TRACE, ("BT_Turn OFF Coexist\n"));
						PlatformEFIOWrite1Byte(Adapter, REG_GPIO_MUXCFG, 0x14);
					}
				}
				else
				{
					RTPRINT(FBT, BT_TRACE, ("BT_IsolationHigh\n"));
					// Do nothing.
				}
			}
			else
			{
				if(Adapter->pNdisCommon->bRegBT_Ampdu && Adapter->pNdisCommon->bRegAcceptAddbaReq)
				{
					RTPRINT(FBT, BT_TRACE, ("BT_Allow AMPDU bt is off\n"));	
					pMgntInfo->pHTInfo->bAcceptAddbaReq = TRUE;
				}

				RTPRINT(FBT, BT_TRACE, ("BT_Turn OFF Coexist bt is off \n"));
				PlatformEFIOWrite1Byte(Adapter, REG_GPIO_MUXCFG, 0x14);

				RTPRINT(FBT, BT_TRACE, ("BT_Update Rate table\n"));
				Adapter->HalFunc.UpdateHalRATRTableHandler(
							Adapter, 
							&pMgntInfo->dot11OperationalRateSet,
							pMgntInfo->dot11HTOperationalRateSet,NULL);
			}
		}
	}
}
#endif

/*-----------------------------------------------------------------------------
 * Function:	dm_CheckRfCtrlGPIO()
 *
 * Overview:	Copy 8187B template for 9xseries.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/10/2008	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
static VOID
dm_CheckRfCtrlGPIO(
	IN	PADAPTER	Adapter
	)
{
#if 0
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);

#if ( (HAL_CODE_BASE == RTL8190) || \
 	 ((HAL_CODE_BASE == RTL8192) && (DEV_BUS_TYPE==DEV_BUS_USB_INTERFACE)))
	//return;
#endif
	
	// Walk around for DTM test, we will not enable HW - radio on/off because r/w
	// page 1 register before Lextra bus is enabled cause system fails when resuming
	// from S4. 20080218, Emily
	if(Adapter->bInHctTest)
		return;

//#if ((HAL_CODE_BASE == RTL8192_S) )
	//Adapter->HalFunc.GPIOChangeRFHandler(Adapter, GPIORF_POLLING);
//#else
	PlatformScheduleWorkItem( &(pHalData->GPIOChangeRFWorkItem) );
//#endif

#endif
}	/* dm_CheckRfCtrlGPIO */

static VOID
dm_InitRateAdaptiveMask(
	IN	PADAPTER	Adapter	
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	PRATE_ADAPTIVE	pRA = (PRATE_ADAPTIVE)&pdmpriv->RateAdaptive;
	
	pRA->RATRState = DM_RATR_STA_INIT;
	pRA->PreRATRState = DM_RATR_STA_INIT;

	if (pdmpriv->DM_Type == DM_Type_ByDriver)
		pdmpriv->bUseRAMask = _TRUE;
	else
		pdmpriv->bUseRAMask = _FALSE;	
}


/*-----------------------------------------------------------------------------
 * Function:	dm_RefreshRateAdaptiveMask()
 *
 * Overview:	Update rate table mask according to rssi
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	05/27/2009	hpfan	Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
static VOID
dm_RefreshRateAdaptiveMask(	IN	PADAPTER	pAdapter)
{
#if 0
	PADAPTER 				pTargetAdapter;
	HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(pAdapter);
	PMGNT_INFO				pMgntInfo = &(ADJUST_TO_ADAPTIVE_ADAPTER(pAdapter, TRUE)->MgntInfo);
	PRATE_ADAPTIVE			pRA = (PRATE_ADAPTIVE)&pMgntInfo->RateAdaptive;
	u4Byte					LowRSSIThreshForRA = 0, HighRSSIThreshForRA = 0;

	if(pAdapter->bDriverStopped)
	{
		RT_TRACE(COMP_RATR, DBG_TRACE, ("<---- dm_RefreshRateAdaptiveMask(): driver is going to unload\n"));
		return;
	}

	if(!pMgntInfo->bUseRAMask)
	{
		RT_TRACE(COMP_RATR, DBG_LOUD, ("<---- dm_RefreshRateAdaptiveMask(): driver does not control rate adaptive mask\n"));
		return;
	}

	// if default port is connected, update RA table for default port (infrastructure mode only)
	if(pAdapter->MgntInfo.mAssoc && (!ACTING_AS_AP(pAdapter)))
	{
		
		// decide rastate according to rssi
		switch (pRA->PreRATRState)
		{
			case DM_RATR_STA_HIGH:
				HighRSSIThreshForRA = 50;
				LowRSSIThreshForRA = 20;
				break;
			
			case DM_RATR_STA_MIDDLE:
				HighRSSIThreshForRA = 55;
				LowRSSIThreshForRA = 20;
				break;
			
			case DM_RATR_STA_LOW:
				HighRSSIThreshForRA = 50;
				LowRSSIThreshForRA = 25;
				break;

			default:
				HighRSSIThreshForRA = 50;
				LowRSSIThreshForRA = 20;
				break;
		}

		if(pHalData->UndecoratedSmoothedPWDB > (s4Byte)HighRSSIThreshForRA)
			pRA->RATRState = DM_RATR_STA_HIGH;
		else if(pHalData->UndecoratedSmoothedPWDB > (s4Byte)LowRSSIThreshForRA)
			pRA->RATRState = DM_RATR_STA_MIDDLE;
		else
			pRA->RATRState = DM_RATR_STA_LOW;

		if(pRA->PreRATRState != pRA->RATRState)
		{
			RT_PRINT_ADDR(COMP_RATR, DBG_LOUD, ("Target AP addr : "), pMgntInfo->Bssid);
			RT_TRACE(COMP_RATR, DBG_LOUD, ("RSSI = %ld\n", pHalData->UndecoratedSmoothedPWDB));
			RT_TRACE(COMP_RATR, DBG_LOUD, ("RSSI_LEVEL = %d\n", pRA->RATRState));
			RT_TRACE(COMP_RATR, DBG_LOUD, ("PreState = %d, CurState = %d\n", pRA->PreRATRState, pRA->RATRState));
			pAdapter->HalFunc.UpdateHalRAMaskHandler(
									pAdapter,
									FALSE,
									0,
									NULL,
									NULL,
									pRA->RATRState);
			pRA->PreRATRState = pRA->RATRState;
		}
	}

	//
	// The following part configure AP/VWifi/IBSS rate adaptive mask.
	//
	if(ACTING_AS_AP(pAdapter) || ACTING_AS_IBSS(pAdapter))
	{
		pTargetAdapter = pAdapter;
	}
	else
	{
		pTargetAdapter = ADJUST_TO_ADAPTIVE_ADAPTER(pAdapter, FALSE);
		if(!ACTING_AS_AP(pTargetAdapter))
			pTargetAdapter = NULL;
	}

	// if extension port (softap) is started, updaet RA table for more than one clients associate
	if(pTargetAdapter != NULL)
	{
		int	i;
		PRT_WLAN_STA	pEntry;
		PRATE_ADAPTIVE     pEntryRA;

		for(i = 0; i < ASSOCIATE_ENTRY_NUM; i++)
		{
			if(	pTargetAdapter->MgntInfo.AsocEntry[i].bUsed && pTargetAdapter->MgntInfo.AsocEntry[i].bAssociated)
			{
				pEntry = pTargetAdapter->MgntInfo.AsocEntry+i;
				pEntryRA = &pEntry->RateAdaptive;

				switch (pEntryRA->PreRATRState)
				{
					case DM_RATR_STA_HIGH:
					{
						HighRSSIThreshForRA = 50;
						LowRSSIThreshForRA = 20;
					}
					break;
					
					case DM_RATR_STA_MIDDLE:
					{
						HighRSSIThreshForRA = 55;
						LowRSSIThreshForRA = 20;
					}
					break;
					
					case DM_RATR_STA_LOW:
					{
						HighRSSIThreshForRA = 50;
						LowRSSIThreshForRA = 25;
					}
					break;

					default:
					{
						HighRSSIThreshForRA = 50;
						LowRSSIThreshForRA = 20;
					}
				}

				if(pEntry->rssi_stat.UndecoratedSmoothedPWDB > (s4Byte)HighRSSIThreshForRA)
					pEntryRA->RATRState = DM_RATR_STA_HIGH;
				else if(pEntry->rssi_stat.UndecoratedSmoothedPWDB > (s4Byte)LowRSSIThreshForRA)
					pEntryRA->RATRState = DM_RATR_STA_MIDDLE;
				else
					pEntryRA->RATRState = DM_RATR_STA_LOW;

				if(pEntryRA->PreRATRState != pEntryRA->RATRState)
				{
					RT_PRINT_ADDR(COMP_RATR, DBG_LOUD, ("AsocEntry addr : "), pEntry->MacAddr);
					RT_TRACE(COMP_RATR, DBG_LOUD, ("RSSI = %ld\n", pEntry->rssi_stat.UndecoratedSmoothedPWDB));
					RT_TRACE(COMP_RATR, DBG_LOUD, ("RSSI_LEVEL = %d\n", pEntryRA->RATRState));
					RT_TRACE(COMP_RATR, DBG_LOUD, ("PreState = %d, CurState = %d\n", pEntryRA->PreRATRState, pEntryRA->RATRState));
					pAdapter->HalFunc.UpdateHalRAMaskHandler(
											pTargetAdapter,
											FALSE,
											pEntry->AID+1,
											pEntry->MacAddr,
											pEntry,
											pEntryRA->RATRState);
					pEntryRA->PreRATRState = pEntryRA->RATRState;
				}

			}
		}
	}
#endif	
}

static VOID
dm_CheckProtection(
	IN	PADAPTER	Adapter
	)
{
#if 0
	PMGNT_INFO		pMgntInfo = &(Adapter->MgntInfo);
	u1Byte			CurRate, RateThreshold;

	if(pMgntInfo->pHTInfo->bCurBW40MHz)
		RateThreshold = MGN_MCS1;
	else
		RateThreshold = MGN_MCS3;

	if(Adapter->TxStats.CurrentInitTxRate <= RateThreshold)
	{
		pMgntInfo->bDmDisableProtect = TRUE;
		DbgPrint("Forced disable protect: %x\n", Adapter->TxStats.CurrentInitTxRate);
	}
	else
	{
		pMgntInfo->bDmDisableProtect = FALSE;
		DbgPrint("Enable protect: %x\n", Adapter->TxStats.CurrentInitTxRate);
	}
#endif
}

static VOID
dm_CheckStatistics(
	IN	PADAPTER	Adapter
	)
{
#if 0
	if(!Adapter->MgntInfo.bMediaConnect)
		return;

	//2008.12.10 tynli Add for getting Current_Tx_Rate_Reg flexibly.
	Adapter->HalFunc.GetHwRegHandler( Adapter, HW_VAR_INIT_TX_RATE, (pu1Byte)(&Adapter->TxStats.CurrentInitTxRate) );

	// Calculate current Tx Rate(Successful transmited!!)

	// Calculate current Rx Rate(Successful received!!)
	
	//for tx tx retry count
	Adapter->HalFunc.GetHwRegHandler( Adapter, HW_VAR_RETRY_COUNT, (pu1Byte)(&Adapter->TxStats.NumTxRetryCount) );
#endif	
}

//
// Initialize GPIO setting registers
//
static void
dm_InitGPIOSetting(
	IN	PADAPTER	Adapter
	)
{
	u8	tmp1byte;
	
	tmp1byte = read8(Adapter, REG_GPIO_MUXCFG);
	tmp1byte &= (GPIOSEL_GPIO | ~GPIOSEL_ENBT);
	write8(Adapter, REG_GPIO_MUXCFG, tmp1byte);

}

//============================================================
// functions
//============================================================
void rtl8192d_init_dm_priv(IN PADAPTER Adapter)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;

	_memset(pdmpriv, 0, sizeof(struct dm_priv));

}

void rtl8192d_deinit_dm_priv(IN PADAPTER Adapter)
{
	//PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	//struct dm_priv	*pdmpriv = &pHalData->dmpriv;

}

void
rtl8192d_InitHalDm(
	IN	PADAPTER	Adapter
	)
{
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;

	_memset(pdmpriv, 0, sizeof(struct dm_priv));
	

	pdmpriv->DM_Type = DM_Type_ByDriver;	
	pdmpriv->DMFlag = DYNAMIC_FUNC_DISABLE;
	pdmpriv->UndecoratedSmoothedPWDB = (-1);
	
	//.1 DIG INIT
	pdmpriv->bDMInitialGainEnable = _TRUE;
	pdmpriv->DMFlag |= DYNAMIC_FUNC_DIG;
	dm_DIGInit(Adapter);

	//.2 DynamicTxPower INIT
	pdmpriv->DMFlag |= DYNAMIC_FUNC_HP;
	dm_InitDynamicTxPower(Adapter);

	//.3
	DM_InitEdcaTurbo(Adapter);//moved to  linked_status_chk()

	//.4 RateAdaptive INIT
	dm_InitRateAdaptiveMask(Adapter);

	//.5 Tx Power Tracking Init.
	pdmpriv->DMFlag |= DYNAMIC_FUNC_SS;
	DM_InitializeTXPowerTracking(Adapter);

	dm_InitGPIOSetting(Adapter);

	pdmpriv->DMFlag_tmp = pdmpriv->DMFlag;

}

VOID
rtl8192d_HalDmWatchDog(
	IN	PADAPTER	Adapter
	)
{
	BOOLEAN		bFwCurrentInPSMode = _FALSE;
	BOOLEAN		bFwPSAwake = _TRUE;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);


#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if((Adapter->bInModeSwitchProcess))
	{
		RT_TRACE(COMP_DUALMACSWITCH,DBG_LOUD,("HalDmWatchDog(): During dual mac mode switch or slave mac \n"));
		return;
	}
#endif

#ifdef CONFIG_LPS
	bFwCurrentInPSMode = Adapter->pwrctrlpriv.bFwCurrentInPSMode;
	Adapter->HalFunc.GetHwRegHandler(Adapter, HW_VAR_FWLPS_RF_ON, (u8 *)(&bFwPSAwake));
#endif

	// Stop dynamic mechanism when:
	// 1. RF is OFF. (No need to do DM.)
	// 2. Fw is under power saving mode for FwLPS. (Prevent from SW/FW I/O racing.)
	// 3. IPS workitem is scheduled. (Prevent from IPS sequence to be swapped with DM.
	//     Sometimes DM execution time is longer than 100ms such that the assertion
	//     in MgntActSet_RF_State() called by InactivePsWorkItem will be triggered by 
	//     wating to long for RFChangeInProgress.)
	// 4. RFChangeInProgress is TRUE. (Prevent from broken by IPS/HW/SW Rf off.)
	// Noted by tynli. 2010.06.01.
	//if(rfState == eRfOn)
	if( (Adapter->hw_init_completed == _TRUE) 
		&& ((!bFwCurrentInPSMode) && bFwPSAwake))
	{
		//
		// Calculate Tx/Rx statistics.
		//
		dm_CheckStatistics(Adapter);

		//
		// For PWDB monitor and record some value for later use.
		//
		PWDB_Monitor(Adapter);

		//
		// Dynamic Initial Gain mechanism.
		//
#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
#if (DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE)
		ACQUIRE_GLOBAL_SPINLOCK(&GlobalSpinlockForGlobalAdapterList);
#else
		ACQUIRE_GLOBAL_MUTEX(GlobalMutexForGlobalAdapterList);
#endif
		if(Adapter->bSlaveOfDMSP)
		{
			dm_FalseAlarmCounterStatistics_ForSlaveOfDMSP(Adapter);
		}
		else
		{
			dm_FalseAlarmCounterStatistics(Adapter);
		}
#if (DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE)
		RELEASE_GLOBAL_SPINLOCK(&GlobalSpinlockForGlobalAdapterList);
#else
		RELEASE_GLOBAL_MUTEX(GlobalMutexForGlobalAdapterList);
#endif
#else
		dm_FalseAlarmCounterStatistics(Adapter);
#endif
		FindMinimumRSSI(Adapter);
		dm_DIG_8192D(Adapter); //92D DIG,add by luke

		//
		// Dynamic Tx Power mechanism.
		//
		dm_DynamicTxPower(Adapter);

		//
		// Tx Power Tracking.
		//
		//TX power tracking will make 92de DMDP MAC0's throughput bad.
#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
		if(!Adapter->bSlaveOfDMSP)
#endif
			rtl8192d_dm_CheckTXPowerTracking(Adapter);

		//
		// Rate Adaptive by Rx Signal Strength mechanism.
		//
		dm_RefreshRateAdaptiveMask(Adapter);

#if DEV_BUS_TYPE==DEV_BUS_PCI_INTERFACE
		//BT-Coexist
		dm_BTCoexist(Adapter);
#endif

#if DEV_BUS_TYPE==DEV_BUS_PCI_INTERFACE
		// Tx Migration settings.
		//migration, 92d just for normal chip, vivi, 20100708
		//if(IS_NORMAL_CHIP(pHalData->VersionID))
			//dm_InterruptMigration(Adapter);

#endif

		// EDCA turbo
		//update the EDCA paramter according to the Tx/RX mode
		//update_EDCA_param(Adapter);
		dm_CheckEdcaTurbo(Adapter);

		//
		// Dynamically switch RTS/CTS protection.
		//
		//dm_CheckProtection(Adapter);

		//
		//Dynamic BB Power Saving Mechanism
		//vivi, 20101014, to pass DTM item: softap_excludeunencrypted_ext.htm 
		//temporarily disable it advised for performance test by yn,2010-11-03.
		//if(!Adapter->bInHctTest)
		//	dm_DynamicBBPowerSaving(Adapter);
	}

#if 0
	// Check GPIO to determine current RF on/off and Pbc status.
	// Not enable for 92CU now!!!
#if 0// DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
	if(Adapter->HalFunc.GetInterfaceSelectionHandler(Adapter)==INTF_SEL1_MINICARD)

	{
#endif
		// Check Hardware Radio ON/OFF or not	
		if(Adapter->MgntInfo.PowerSaveControl.bGpioRfSw)
		{
			RTPRINT(FPWR, PWRHW, ("dm_CheckRfCtrlGPIO \n"));
			dm_CheckRfCtrlGPIO(Adapter);
		}
#if 0//DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
	}
	else if(Adapter->HalFunc.GetInterfaceSelectionHandler(Adapter)==INTF_SEL0_USB)
	{
		dm_CheckPbcGPIO(Adapter);				// Add by hpfan 2008-03-11
	}
#endif
#endif

}

