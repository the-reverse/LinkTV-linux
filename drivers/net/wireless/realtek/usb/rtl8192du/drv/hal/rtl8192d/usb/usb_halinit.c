/******************************************************************************
* usb_halinit.c                                                                                                                                 *
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
#include <hal_init.h>
#include <rtw_efuse.h>

#include <rtl8192d_hal.h>
#include <rtl8192d_led.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifndef CONFIG_USB_HCI

#error "CONFIG_USB_HCI shall be on!\n"

#endif

#include <usb_ops.h>
#include <usb_hal.h>
#include <usb_osintf.h>


#if DISABLE_BB_RF
	#define		HAL_MAC_ENABLE	0
	#define		HAL_BB_ENABLE		0
	#define		HAL_RF_ENABLE		0
#else
	#define		HAL_MAC_ENABLE	1
	#define		HAL_BB_ENABLE		1
	#define		HAL_RF_ENABLE		1
#endif

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
extern ULONG    GlobalMutexForGlobalAdapterList ;
extern BOOLEAN	GlobalFirstConfigurationForNormalChip;
#endif

//add mutex to solve the problem that reading efuse and power on/fw download do 
//on the same time 
extern atomic_t GlobalMutexForPowerAndEfuse;
extern atomic_t GlobalMutexForPowerOnAndPowerOff;


//endpoint number 1,2,3,4,5
// bult in : 1
// bult out: 2 (High)
// bult out: 3 (Normal) for 3 out_ep, (Low) for 2 out_ep
// interrupt in: 4
// bult out: 5 (Low) for 3 out_ep


static VOID
_OneOutEpMapping(
	IN	HAL_DATA_TYPE	*pHalData
	)
{
	//only endpoint number 0x02

	pHalData->Queue2EPNum[0] = pHalData->RtBulkOutPipe[0];//VO
	pHalData->Queue2EPNum[1] = pHalData->RtBulkOutPipe[0];//VI
	pHalData->Queue2EPNum[2] = pHalData->RtBulkOutPipe[0];//BE
	pHalData->Queue2EPNum[3] = pHalData->RtBulkOutPipe[0];//BK
	
	pHalData->Queue2EPNum[4] = pHalData->RtBulkOutPipe[0];//TS
	pHalData->Queue2EPNum[5] = pHalData->RtBulkOutPipe[0];//MGT
	pHalData->Queue2EPNum[6] = pHalData->RtBulkOutPipe[0];//BMC
	pHalData->Queue2EPNum[7] = pHalData->RtBulkOutPipe[0];//BCN
}


static VOID
_TwoOutEpMapping(
	IN	BOOLEAN			IsTestChip,
	IN	HAL_DATA_TYPE	*pHalData,
	IN	BOOLEAN	 		bWIFICfg
	)
{

	if(IsTestChip && bWIFICfg){ // test chip && wmm
	
		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  1, 	0, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};			
		//0:H(end_number=0x02), 1:L (end_number=0x03)

		pHalData->Queue2EPNum[0] = pHalData->RtBulkOutPipe[0];//VO
		pHalData->Queue2EPNum[1] = pHalData->RtBulkOutPipe[1];//VI
		pHalData->Queue2EPNum[2] = pHalData->RtBulkOutPipe[0];//BE
		pHalData->Queue2EPNum[3] = pHalData->RtBulkOutPipe[1];//BK
		
		pHalData->Queue2EPNum[4] = pHalData->RtBulkOutPipe[0];//TS
		pHalData->Queue2EPNum[5] = pHalData->RtBulkOutPipe[0];//MGT
		pHalData->Queue2EPNum[6] = pHalData->RtBulkOutPipe[0];//BMC
		pHalData->Queue2EPNum[7] = pHalData->RtBulkOutPipe[0];//BCN
	
	}
	else if(!IsTestChip && bWIFICfg){ // Normal chip && wmm
		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  0, 	1, 	0, 	1, 	0, 	0, 	0, 	0, 		0	};
		//0:H(end_number=0x02), 1:L (end_number=0x03)
		
		pHalData->Queue2EPNum[0] = pHalData->RtBulkOutPipe[1];//VO
		pHalData->Queue2EPNum[1] = pHalData->RtBulkOutPipe[0];//VI
		pHalData->Queue2EPNum[2] = pHalData->RtBulkOutPipe[1];//BE
		pHalData->Queue2EPNum[3] = pHalData->RtBulkOutPipe[0];//BK
		
		pHalData->Queue2EPNum[4] = pHalData->RtBulkOutPipe[0];//TS
		pHalData->Queue2EPNum[5] = pHalData->RtBulkOutPipe[0];//MGT
		pHalData->Queue2EPNum[6] = pHalData->RtBulkOutPipe[0];//BMC
		pHalData->Queue2EPNum[7] = pHalData->RtBulkOutPipe[0];//BCN
		
	}
	else{//typical setting

		
		//BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  1, 	1, 	0, 	0, 	0, 	0, 	0, 	0, 		0	};			
		//0:H(end_number=0x02), 1:L (end_number=0x03)
		
		pHalData->Queue2EPNum[0] = pHalData->RtBulkOutPipe[0];//VO
		pHalData->Queue2EPNum[1] = pHalData->RtBulkOutPipe[0];//VI
		pHalData->Queue2EPNum[2] = pHalData->RtBulkOutPipe[1];//BE
		pHalData->Queue2EPNum[3] = pHalData->RtBulkOutPipe[1];//BK
		
		pHalData->Queue2EPNum[4] = pHalData->RtBulkOutPipe[0];//TS
		pHalData->Queue2EPNum[5] = pHalData->RtBulkOutPipe[0];//MGT
		pHalData->Queue2EPNum[6] = pHalData->RtBulkOutPipe[0];//BMC
		pHalData->Queue2EPNum[7] = pHalData->RtBulkOutPipe[0];//BCN	
		
	}
	
}


static VOID _ThreeOutEpMapping(
	IN	HAL_DATA_TYPE	*pHalData,
	IN	BOOLEAN	 		bWIFICfg
	)
{
	if(bWIFICfg){//for WMM
		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  1, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};
		//0:H(end_number=0x02), 1:N(end_number=0x03), 2:L (end_number=0x05)
		
		pHalData->Queue2EPNum[0] = pHalData->RtBulkOutPipe[0];//VO
		pHalData->Queue2EPNum[1] = pHalData->RtBulkOutPipe[1];//VI
		pHalData->Queue2EPNum[2] = pHalData->RtBulkOutPipe[2];//BE
		pHalData->Queue2EPNum[3] = pHalData->RtBulkOutPipe[1];//BK
		
		pHalData->Queue2EPNum[4] = pHalData->RtBulkOutPipe[0];//TS
		pHalData->Queue2EPNum[5] = pHalData->RtBulkOutPipe[0];//MGT
		pHalData->Queue2EPNum[6] = pHalData->RtBulkOutPipe[0];//BMC
		pHalData->Queue2EPNum[7] = pHalData->RtBulkOutPipe[0];//BCN
		
	}
	else{//typical setting

		
		//	BK, 	BE, 	VI, 	VO, 	BCN,	CMD,MGT,HIGH,HCCA 
		//{  2, 	2, 	1, 	0, 	0, 	0, 	0, 	0, 		0	};			
		//0:H(end_number=0x02), 1:N(end_number=0x03), 2:L (end_number=0x05)
		
		pHalData->Queue2EPNum[0] = pHalData->RtBulkOutPipe[0];//VO
		pHalData->Queue2EPNum[1] = pHalData->RtBulkOutPipe[1];//VI
		pHalData->Queue2EPNum[2] = pHalData->RtBulkOutPipe[2];//BE
		pHalData->Queue2EPNum[3] = pHalData->RtBulkOutPipe[2];//BK
		
		pHalData->Queue2EPNum[4] = pHalData->RtBulkOutPipe[0];//TS
		pHalData->Queue2EPNum[5] = pHalData->RtBulkOutPipe[0];//MGT
		pHalData->Queue2EPNum[6] = pHalData->RtBulkOutPipe[0];//BMC
		pHalData->Queue2EPNum[7] = pHalData->RtBulkOutPipe[0];//BCN	
	}

}

static BOOLEAN
_MappingOutEP(
	IN	PADAPTER	pAdapter,
	IN	u8		NumOutPipe,
	IN	BOOLEAN		IsTestChip
	)
{		
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct registry_priv *pregistrypriv = &pAdapter->registrypriv;

	BOOLEAN	 bWIFICfg = (pregistrypriv->wifi_spec) ?_TRUE:_FALSE;
	
	BOOLEAN result = _TRUE;

	switch(NumOutPipe)
	{
		case 2:
			_TwoOutEpMapping(IsTestChip, pHalData, bWIFICfg);
			break;
		case 3:
			_ThreeOutEpMapping(pHalData, bWIFICfg);
			break;
		case 1:
			_OneOutEpMapping(pHalData);
			break;
		default:
			result = _FALSE;
			break;
	}

	return result;
	
}

static VOID
_ConfigTestChipOutEP(
	IN	PADAPTER	pAdapter,
	IN	u8		NumOutPipe
	)
{
	u8			value8,txqsele;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);

	pHalData->OutEpQueueSel = 0;
	pHalData->OutEpNumber	= 0;

	value8 = read8(pAdapter, REG_TEST_SIE_OPTIONAL);
	value8 = (value8 & USB_TEST_EP_MASK) >> USB_TEST_EP_SHIFT;
	
	switch(value8)
	{
		case 0:		// 2 bulk OUT, 1 bulk IN
		case 3:		
			pHalData->OutEpQueueSel = TX_SELE_HQ | TX_SELE_LQ;
			pHalData->OutEpNumber	= 2;
			//RT_TRACE(COMP_INIT,  DBG_LOUD, ("EP Config: 2 bulk OUT, 1 bulk IN\n"));
			break;
		case 1:		// 1 bulk IN/OUT => map all endpoint to Low queue
		case 2:		// 1 bulk IN, 1 bulk OUT => map all endpoint to High queue
			txqsele = read8(pAdapter, REG_TEST_USB_TXQS);
			if(txqsele & 0x0F){//map all endpoint to High queue
				pHalData->OutEpQueueSel  = TX_SELE_HQ;
			}
			else if(txqsele&0xF0){//map all endpoint to Low queue
				pHalData->OutEpQueueSel  =  TX_SELE_LQ;
			}
			pHalData->OutEpNumber	= 1;
			//RT_TRACE(COMP_INIT,  DBG_LOUD, ("%s\n", ((1 == value8) ? "1 bulk IN/OUT" : "1 bulk IN, 1 bulk OUT")));
			break;
		default:
			break;
	}

	// TODO: Error recovery for this case
	//RT_ASSERT((NumOutPipe == pHalData->OutEpNumber), ("Out EP number isn't match! %d(Descriptor) != %d (SIE reg)\n", (u4Byte)NumOutPipe, (u4Byte)pHalData->OutEpNumber));

}



static VOID
_ConfigNormalChipOutEP(
	IN	PADAPTER	pAdapter,
	IN	u8		NumOutPipe
	)
{
	u8			value8;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);

	pHalData->OutEpQueueSel = 0;
	pHalData->OutEpNumber = 0;
		
	// Normal and High queue
	if(pHalData->interfaceIndex==0)
		value8=read8(pAdapter, REG_USB_High_NORMAL_Queue_Select_MAC0);
	else
		value8=read8(pAdapter, REG_USB_High_NORMAL_Queue_Select_MAC1);

	if(value8 & USB_NORMAL_SIE_EP_MASK){
		pHalData->OutEpQueueSel |= TX_SELE_HQ;
		pHalData->OutEpNumber++;
	}
	
	if((value8 >> USB_NORMAL_SIE_EP_SHIFT) & USB_NORMAL_SIE_EP_MASK){
		pHalData->OutEpQueueSel |= TX_SELE_NQ;
		pHalData->OutEpNumber++;
	}
	
	// Low queue
	if(pHalData->interfaceIndex==0)
		value8=read8(pAdapter, (REG_USB_High_NORMAL_Queue_Select_MAC0+1));
	else
		value8=read8(pAdapter, (REG_USB_High_NORMAL_Queue_Select_MAC1+1));
	
	if(value8 & USB_NORMAL_SIE_EP_MASK){
		pHalData->OutEpQueueSel |= TX_SELE_LQ;
		pHalData->OutEpNumber++;
	}

	//add for 0xfe44 0xfe45 0xfe47 0xfe48 not validly 
	switch(NumOutPipe){
		case 3:
			pHalData->OutEpQueueSel=TX_SELE_HQ| TX_SELE_LQ|TX_SELE_NQ;
			pHalData->OutEpNumber=3;
			break;
		case 2:
			pHalData->OutEpQueueSel=TX_SELE_HQ| TX_SELE_NQ;
			pHalData->OutEpNumber=2;
			break;
		case 1:
			pHalData->OutEpQueueSel=TX_SELE_HQ;
			pHalData->OutEpNumber=1;
			break;
		default:				
			break;
	}

	// TODO: Error recovery for this case
	//RT_ASSERT((NumOutPipe == pHalData->OutEpNumber), ("Out EP number isn't match! %d(Descriptor) != %d (SIE reg)\n", (u4Byte)NumOutPipe, (u4Byte)pHalData->OutEpNumber));

}

static BOOLEAN HalUsbSetQueuePipeMapping8192DUsb(
	IN	PADAPTER	pAdapter,
	IN	u8		NumInPipe,
	IN	u8		NumOutPipe
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(pAdapter);
	BOOLEAN			result		= _FALSE;
	BOOLEAN			isNormalChip;

	// ReadAdapterInfo8192C also call _ReadChipVersion too.
	// Since we need dynamic config EP mapping, so we call this function to get chip version.
	// We can remove _ReadChipVersion from ReadAdapterInfo8192C later.
	rtl8192d_ReadChipVersion(pAdapter);

	isNormalChip = IS_NORMAL_CHIP(pHalData->VersionID);

	_ConfigNormalChipOutEP(pAdapter, NumOutPipe);

	// Normal chip with one IN and one OUT doesn't have interrupt IN EP.
	if(isNormalChip && (1 == pHalData->OutEpNumber)){
		if(1 != NumInPipe){
			return result;
		}
	}

	// All config other than above support one Bulk IN and one Interrupt IN.
	//if(2 != NumInPipe){
	//	return result;
	//}

	result = _MappingOutEP(pAdapter, NumOutPipe, !isNormalChip);
	
	return result;

}

void rtl8192du_interface_configure(_adapter *padapter)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(padapter);
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;

	if (pdvobjpriv->ishighspeed == _TRUE)
	{
		pHalData->UsbBulkOutSize = USB_HIGH_SPEED_BULK_SIZE;//512 bytes
	}
	else
	{
		pHalData->UsbBulkOutSize = USB_FULL_SPEED_BULK_SIZE;//64 bytes
	}

	pHalData->interfaceIndex = pdvobjpriv->InterfaceNumber;
	pHalData->RtBulkInPipe = pdvobjpriv->ep_num[0];
	pHalData->RtBulkOutPipe[0] = pdvobjpriv->ep_num[1];
	pHalData->RtBulkOutPipe[1] = pdvobjpriv->ep_num[2];
	pHalData->RtIntInPipe = pdvobjpriv->ep_num[3];
	pHalData->RtBulkOutPipe[2] = pdvobjpriv->ep_num[4];
	//printk("Bulk In = %x, Bulk Out = %x %x %x\n",pHalData->RtBulkInPipe, pHalData->RtBulkOutPipe[0],pHalData->RtBulkOutPipe[1],pHalData->RtBulkOutPipe[2]);
#ifdef USB_TX_AGGREGATION_92C
	pHalData->UsbTxAggMode		= 1;
	pHalData->UsbTxAggDescNum	= 0x6;	// only 4 bits
#endif

#ifdef USB_RX_AGGREGATION_92C
	pHalData->UsbRxAggMode = USB_RX_AGG_DMA_USB;// USB_RX_AGG_DMA;
	pHalData->UsbRxAggBlockCount	= 8; //unit : 512b
	pHalData->UsbRxAggBlockTimeout = 0x6;
	pHalData->UsbRxAggPageCount	= 48; //uint :128 b //0x0A;	// 10 = MAX_RX_DMA_BUFFER_SIZE/2/pHalData->UsbBulkOutSize
	pHalData->UsbRxAggPageTimeout = 0x4; //6, absolute time = 34ms/(2^6)
	//pHalData->UsbRxAggMode = USB_RX_AGG_USB;// USB_RX_AGG_DMA;
	//pHalData->UsbRxAggBlockCount	= 1; //unit : 512b
	//pHalData->UsbRxAggBlockTimeout = 0x5;
	//pHalData->UsbRxAggPageCount	= 1; //uint :128 b //0x0A;	// 10 = MAX_RX_DMA_BUFFER_SIZE/2/pHalData->UsbBulkOutSize
	//pHalData->UsbRxAggPageTimeout = 0x5; //6, absolute time = 34ms/(2^6)
#endif

	HalUsbSetQueuePipeMapping8192DUsb(padapter,
				pdvobjpriv->RtNumInPipes, pdvobjpriv->RtNumOutPipes);

}

static u8 _InitPowerOn(_adapter *padapter)
{
	u8	ret = _SUCCESS;
	u32	value32;
	u16	value16=0;
	u8	value8 = 0;
	HAL_DATA_TYPE    *pHalData = GET_HAL_DATA(padapter);

	// polling autoload done.
	u32	pollingCount = 0;

	if(padapter->bSurpriseRemoved){
		return _FAIL;
	}

#if 0
	if(pHalData->MacPhyMode92D != SINGLEMAC_SINGLEPHY)
	{
		// in DMDP mode,polling power off in process bit 0x17[7]
		value8=read8(padapter, 0x17);
		while((value8&BIT7) && (pollingCount < 200))
		{
			udelay_os(200);
			value8=read8(padapter, 0x17);		
			pollingCount++;
		}
	}
#endif
	pollingCount = 0;
	do
	{
		if(read8(padapter, REG_APS_FSMCO) & PFM_ALDN){
			//RT_TRACE(COMP_INIT,DBG_LOUD,("Autoload Done!\n"));
			break;
		}

		if(pollingCount++ > POLLING_READY_TIMEOUT_COUNT){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling REG_APS_FSMCO[PFM_ALDN] done!\n"));
			return _FAIL;
		}
				
	}while(_TRUE);


	//For hardware power on sequence.

	//0.	RSV_CTRL 0x1C[7:0] = 0x00			// unlock ISO/CLK/Power control register
	write8(padapter, REG_RSV_CTRL, 0x0);	
	// Power on when re-enter from IPS/Radio off/card disable
	write8(padapter, REG_SPS0_CTRL, 0x2b);//enable SPS into PWM mode
	udelay_os(100);//PlatformSleepUs(150);//this is not necessary when initially power on

	value8 = read8(padapter, REG_LDOV12D_CTRL);	
	if(0== (value8 & LDV12_EN) ){
		value8 |= LDV12_EN;
		write8(padapter, REG_LDOV12D_CTRL, value8);	
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" power-on :REG_LDOV12D_CTRL Reg0x21:0x%02x.\n",value8));
		udelay_os(100);//PlatformSleepUs(100);//this is not necessary when initially power on
		value8 = read8(padapter, REG_SYS_ISO_CTRL);
		value8 &= ~ISO_MD2PP;
		write8(padapter, REG_SYS_ISO_CTRL, value8);			
	}	
	
	// auto enable WLAN
	pollingCount = 0;
	value16 = read16(padapter, REG_APS_FSMCO);
	value16 |= APFM_ONMAC;
	write16(padapter, REG_APS_FSMCO, value16);

	do
	{
		if(0 == (read16(padapter, REG_APS_FSMCO) & APFM_ONMAC)){
			//RT_TRACE(COMP_INIT,DBG_LOUD,("MAC auto ON okay!\n"));
			break;
		}

		if(pollingCount++ > POLLING_READY_TIMEOUT_COUNT){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling REG_APS_FSMCO[APFM_ONMAC] done!\n"));
			return _FAIL;
		}
				
	}while(_TRUE);

	// release RF digital isolation
	value16 = read16(padapter, REG_SYS_ISO_CTRL);
	value16 &= ~ISO_DIOR;
	write16(padapter, REG_SYS_ISO_CTRL, value16);

	// Enable MAC DMA/WMAC/SCHEDULE/SEC block
	value16 = read16(padapter, REG_CR);
	value16 |= (HCI_TXDMA_EN | HCI_RXDMA_EN | TXDMA_EN | RXDMA_EN
				| PROTOCOL_EN | SCHEDULE_EN | MACTXEN | MACRXEN | ENSEC);
	write16(padapter, REG_CR, value16);

	return ret;

}


static void _dbg_dump_macreg(_adapter *padapter)
{
	u32 offset = 0;
	u32 val32 = 0;
	u32 index =0 ;
	for(index=0;index<64;index++)
	{
		offset = index*4;
		val32 = read32(padapter,offset);
		printk("offset : 0x%02x ,val:0x%08x\n",offset,val32);
	}
}


static void _InitPABias(_adapter *padapter)
{
	HAL_DATA_TYPE		*pHalData	= GET_HAL_DATA(padapter);
	u8			pa_setting;
	BOOLEAN		isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	BOOLEAN		is92C = IS_92C_SERIAL(pHalData->VersionID);
	
	//FIXED PA current issue	
	//efuse_one_byte_read(padapter, 0x1FA, &pa_setting);
	pa_setting = EFUSE_Read1Byte(padapter, 0x1FA);

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("_InitPABias 0x1FA 0x%x \n",pa_setting));

	if(!(pa_setting & BIT0))
	{
		PHY_SetRFReg(padapter, RF90_PATH_A, 0x15, 0x0FFFFF, 0x0F406);
		PHY_SetRFReg(padapter, RF90_PATH_A, 0x15, 0x0FFFFF, 0x4F406);		
		PHY_SetRFReg(padapter, RF90_PATH_A, 0x15, 0x0FFFFF, 0x8F406);		
		PHY_SetRFReg(padapter, RF90_PATH_A, 0x15, 0x0FFFFF, 0xCF406);		
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("PA BIAS path A\n"));
	}	

	if(!(pa_setting & BIT1) && isNormal && is92C)
	{
		PHY_SetRFReg(padapter,RF90_PATH_B, 0x15, 0x0FFFFF, 0x0F406);
		PHY_SetRFReg(padapter,RF90_PATH_B, 0x15, 0x0FFFFF, 0x4F406);		
		PHY_SetRFReg(padapter,RF90_PATH_B, 0x15, 0x0FFFFF, 0x8F406);		
		PHY_SetRFReg(padapter,RF90_PATH_B, 0x15, 0x0FFFFF, 0xCF406);
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("PA BIAS path B\n"));	
	}

	if(!(pa_setting & BIT4))
	{
		pa_setting = read8(padapter, 0x16);
		pa_setting &= 0x0F; 
		write8(padapter, 0x16, pa_setting | 0x90);		
	}
}

//-------------------------------------------------------------------------
//
// LLT R/W/Init function
//
//-------------------------------------------------------------------------
static u8 _LLTWrite(
	IN  PADAPTER	Adapter,
	IN	u32		address,
	IN	u32		data
	)
{
	u8	status = _SUCCESS;
	int	count = 0;
	u32	value = _LLT_INIT_ADDR(address) | _LLT_INIT_DATA(data) | _LLT_OP(_LLT_WRITE_ACCESS);

	write32(Adapter, REG_LLT_INIT, value);
	
	//polling
	do{
		
		value = read32(Adapter, REG_LLT_INIT);
		if(_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)){
			break;
		}
		
		if(count > POLLING_LLT_THRESHOLD){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling write LLT done at address %d!\n", address));
			status = _FAIL;
			break;
		}
	}while(count++);

	return status;
	
}


static u8 _LLTRead(
	IN  PADAPTER	Adapter,
	IN	u32		address
	)
{
	int		count = 0;
	u32		value = _LLT_INIT_ADDR(address) | _LLT_OP(_LLT_READ_ACCESS);

	write32(Adapter, REG_LLT_INIT, value);

	//polling and get value
	do{
		
		value = read32(Adapter, REG_LLT_INIT);
		if(_LLT_NO_ACTIVE == _LLT_OP_VALUE(value)){
			return (u8)value;
		}
		
		if(count > POLLING_LLT_THRESHOLD){
			//RT_TRACE(COMP_INIT,DBG_SERIOUS,("Failed to polling read LLT done at address %d!\n", address));
			break;
		}
	}while(count++);

	return 0xFF;

}


static u8 InitLLTTable(
	IN  PADAPTER	Adapter,
	IN	u32		boundary
	)
{
	u8		status = _SUCCESS;
	u32		i;
	u32		txpktbuf_bndy = boundary;
	u32		Last_Entry_Of_TxPktBuf = LAST_ENTRY_OF_TX_PKT_BUFFER;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);


	if(pHalData->MacPhyMode92D !=SINGLEMAC_SINGLEPHY){
		//for 92du two mac: The page size is different from 92c and 92s
		txpktbuf_bndy = TX_PAGE_BOUNDARY_DUAL_MAC;
		Last_Entry_Of_TxPktBuf = LAST_ENTRY_OF_TX_PKT_BUFFER_DUAL_MAC;
	}
	else{
		txpktbuf_bndy = boundary;
		Last_Entry_Of_TxPktBuf = LAST_ENTRY_OF_TX_PKT_BUFFER;
		//txpktbuf_bndy =253;
		//Last_Entry_Of_TxPktBuf=255;
	} 

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
	for(i = txpktbuf_bndy ; i < Last_Entry_Of_TxPktBuf ; i++){
		status = _LLTWrite(Adapter, i, (i + 1)); 
		if(_SUCCESS != status){
			return status;
		}
	}
	
	// Let last entry point to the start entry of ring buffer
	status = _LLTWrite(Adapter, Last_Entry_Of_TxPktBuf, txpktbuf_bndy);
	if(_SUCCESS != status){
		return status;
	}

	return status;
	
}


//---------------------------------------------------------------
//
//	MAC init functions
//
//---------------------------------------------------------------
static VOID
_SetMacID(
	IN  PADAPTER Adapter, u8* MacID
	)
{
	u32 i;
	for(i=0 ; i< MAC_ADDR_LEN ; i++){
		write32(Adapter, REG_MACID+i, MacID[i]);
	}
}

static VOID
_SetBSSID(
	IN  PADAPTER Adapter, u8* BSSID
	)
{
	u32 i;
	for(i=0 ; i< MAC_ADDR_LEN ; i++){
		write32(Adapter, REG_BSSID+i, BSSID[i]);
	}
}


// Shall USB interface init this?
static VOID
_InitInterrupt(
	IN  PADAPTER Adapter
	)
{
	u32	value32;

	// HISR - turn all on
	value32 = 0xFFFFFFFF;
	write32(Adapter, REG_HISR, value32);

	// HIMR - turn all on
	write32(Adapter, REG_HIMR, value32);
}


static VOID
_InitQueueReservedPage(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	BOOLEAN			isNormalChip = IS_NORMAL_CHIP(pHalData->VersionID);
	
	u32			outEPNum	= (u32)pHalData->OutEpNumber;
	u32			numHQ		= 0;
	u32			numLQ		= 0;
	u32			numNQ		= 0;
	u32			numPubQ;
	u32			value32;
	u8			value8;
	u32			txQPageNum, txQPageUnit,txQRemainPage;

	if(!pregistrypriv->wifi_spec){
		if(pHalData->MacPhyMode92D !=SINGLEMAC_SINGLEPHY)	
		{
			numPubQ = NORMAL_PAGE_NUM_PUBQ_92D_DUAL_MAC;
			txQPageNum = TX_TOTAL_PAGE_NUMBER_92D_DUAL_MAC- numPubQ;	
		}
		else
		{
			numPubQ =TEST_PAGE_NUM_PUBQ;
			//RT_ASSERT((numPubQ < TX_TOTAL_PAGE_NUMBER), ("Public queue page number is great than total tx page number.\n"));
			txQPageNum = TX_TOTAL_PAGE_NUMBER - numPubQ;
		}

		if((pHalData->MacPhyMode92D !=SINGLEMAC_SINGLEPHY)&&(outEPNum==3))
		{// temply for DMDP/DMSP Page allocate
			numHQ=NORMAL_PAGE_NUM_HPQ_92D_DUAL_MAC;
			numLQ=NORMAL_PAGE_NUM_LPQ_92D_DUAL_MAC;
			numNQ=NORMAL_PAGE_NUM_NORMALQ_92D_DUAL_MAC;
		}
		else
		{
			txQPageUnit = txQPageNum/outEPNum;
			txQRemainPage = txQPageNum % outEPNum;

			if(pHalData->OutEpQueueSel & TX_SELE_HQ){
				numHQ = txQPageUnit;
			}
			if(pHalData->OutEpQueueSel & TX_SELE_LQ){
				numLQ = txQPageUnit;
			}
			// HIGH priority queue always present in the configuration of 2 or 3 out-ep 
			// so ,remainder pages have assigned to High queue
			if((outEPNum>1) && (txQRemainPage)){			
				numHQ += txQRemainPage;
			}

			// NOTE: This step shall be proceed before writting REG_RQPN.
			if(pHalData->OutEpQueueSel & TX_SELE_NQ){
				numNQ = txQPageUnit;
			}
			value8 = (u8)_NPQ(numNQ);
			write8(Adapter, REG_RQPN_NPQ, value8);
		}
	}
	else{ //for WMM 
		//RT_ASSERT((outEPNum>=2), ("for WMM ,number of out-ep must more than or equal to 2!\n"));

		// 92du wifi config only for SMSP
		//RT_ASSERT((pHalData->MacPhyMode92D==SINGLEMAC_SINGLEPHY), ("for WMM ,only SMSP come here!\n"));

		numPubQ =WMM_NORMAL_PAGE_NUM_PUBQ_92D;		

		if(pHalData->OutEpQueueSel & TX_SELE_HQ){
			numHQ = WMM_NORMAL_PAGE_NUM_HPQ_92D;									
		}

		if(pHalData->OutEpQueueSel & TX_SELE_LQ){
			numLQ = WMM_NORMAL_PAGE_NUM_LPQ_92D;							
		}

		if(pHalData->OutEpQueueSel & TX_SELE_NQ){
			numNQ = WMM_NORMAL_PAGE_NUM_NPQ_92D;
			value8 = (u8)_NPQ(numNQ);
			write8(Adapter, REG_RQPN_NPQ, value8);
		}
	}

	// TX DMA
	value32 = _HPQ(numHQ) | _LPQ(numLQ) | _PUBQ(numPubQ) | LD_RQPN;	
	write32(Adapter, REG_RQPN, value32);	
}

static void _InitID(IN  PADAPTER Adapter)
{
	int i;	 
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	
	for(i=0; i<6; i++)
	{
		write8(Adapter, (REG_MACID+i), pEEPROM->mac_addr[i]);		 	
	}

/*
	NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);
	//Ziv test
#if 1
	{
		u1Byte sMacAddr[6] = {0};
		u4Byte i;
		
		for(i = 0 ; i < MAC_ADDR_LEN ; i++){
			sMacAddr[i] = PlatformIORead1Byte(Adapter, (REG_MACID + i));
		}
		RT_PRINT_ADDR(COMP_INIT|COMP_EFUSE, DBG_LOUD, "Read back MAC Addr: ", sMacAddr);
	}
#endif

#if 0
	u4Byte nMAR 	= 0xFFFFFFFF;
	u8 m_MacID[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
	u8 m_BSSID[] = {0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
	int i;
	
	_SetMacID(Adapter, Adapter->PermanentAddress);
	_SetBSSID(Adapter, m_BSSID);

	//set MAR
	PlatformIOWrite4Byte(Adapter, REG_MAR, nMAR);
	PlatformIOWrite4Byte(Adapter, REG_MAR+4, nMAR);
#endif
*/
}


static VOID
_InitTxBufferBoundary(
	IN  PADAPTER Adapter
	)
{	
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	//u16	txdmactrl;	
	u8	txpktbuf_bndy; 

	if(!pregistrypriv->wifi_spec){
		txpktbuf_bndy = TX_PAGE_BOUNDARY;
	}
	else{//for WMM
		txpktbuf_bndy = ( IS_NORMAL_CHIP( pHalData->VersionID))?WMM_NORMAL_TX_PAGE_BOUNDARY
															:WMM_TEST_TX_PAGE_BOUNDARY;
	}

	if(pHalData->MacPhyMode92D !=SINGLEMAC_SINGLEPHY)
		txpktbuf_bndy = TX_PAGE_BOUNDARY_DUAL_MAC;

	write8(Adapter, REG_TXPKTBUF_BCNQ_BDNY, txpktbuf_bndy);
	write8(Adapter, REG_TXPKTBUF_MGQ_BDNY, txpktbuf_bndy);
	write8(Adapter, REG_TXPKTBUF_WMAC_LBK_BF_HD, txpktbuf_bndy);
	write8(Adapter, REG_TRXFF_BNDY, txpktbuf_bndy);	
#if 1
	write8(Adapter, REG_TDECTRL+1, txpktbuf_bndy);
#else
	txdmactrl = PlatformIORead2Byte(Adapter, REG_TDECTRL);
	txdmactrl &= ~BCN_HEAD_MASK;
	txdmactrl |= BCN_HEAD(txpktbuf_bndy);
	PlatformIOWrite2Byte(Adapter, REG_TDECTRL, txdmactrl);
#endif
}


static VOID
_InitNormalChipRegPriority(
	IN	PADAPTER	Adapter,
	IN	u16		beQ,
	IN	u16		bkQ,
	IN	u16		viQ,
	IN	u16		voQ,
	IN	u16		mgtQ,
	IN	u16		hiQ
	)
{
	u16 value16		= (read16(Adapter, REG_TRXDMA_CTRL) & 0x7);

	value16 |=	_TXDMA_BEQ_MAP(beQ) 	| _TXDMA_BKQ_MAP(bkQ) |
				_TXDMA_VIQ_MAP(viQ) 	| _TXDMA_VOQ_MAP(voQ) |
				_TXDMA_MGQ_MAP(mgtQ)| _TXDMA_HIQ_MAP(hiQ);
	
	write16(Adapter, REG_TRXDMA_CTRL, value16);
}

static VOID
_InitNormalChipOneOutEpPriority(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	u16	value = 0;
	switch(pHalData->OutEpQueueSel)
	{
		case TX_SELE_HQ:
			value = QUEUE_HIGH;
			break;
		case TX_SELE_LQ:
			value = QUEUE_LOW;
			break;
		case TX_SELE_NQ:
			value = QUEUE_NORMAL;
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}
	
	_InitNormalChipRegPriority(Adapter,
								value,
								value,
								value,
								value,
								value,
								value
								);

}

static VOID
_InitNormalChipTwoOutEpPriority(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	u16			beQ,bkQ,viQ,voQ,mgtQ,hiQ;
	

	u16	valueHi = 0;
	u16	valueLow = 0;
	
	switch(pHalData->OutEpQueueSel)
	{
		case (TX_SELE_HQ | TX_SELE_LQ):
			valueHi = QUEUE_HIGH;
			valueLow = QUEUE_LOW;
			break;
		case (TX_SELE_NQ | TX_SELE_LQ):
			valueHi = QUEUE_NORMAL;
			valueLow = QUEUE_LOW;
			break;
		case (TX_SELE_HQ | TX_SELE_NQ):
			valueHi = QUEUE_HIGH;
			valueLow = QUEUE_NORMAL;
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}

	if(!pregistrypriv->wifi_spec ){
		beQ 		= valueLow;
		bkQ 		= valueLow;
		viQ		= valueHi;
		voQ 		= valueHi;
		mgtQ 	= valueHi; 
		hiQ 		= valueHi;								
	}
	else{//for WMM ,CONFIG_OUT_EP_WIFI_MODE
		beQ		= valueLow;
		bkQ 		= valueHi;		
		viQ 		= valueHi;
		voQ 		= valueLow;
		mgtQ 	= valueHi;
		hiQ 		= valueHi;							
	}
	
	_InitNormalChipRegPriority(Adapter,beQ,bkQ,viQ,voQ,mgtQ,hiQ);

}

static VOID
_InitNormalChipThreeOutEpPriority(
	IN	PADAPTER Adapter
	)
{
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	u16			beQ,bkQ,viQ,voQ,mgtQ,hiQ;

	if(!pregistrypriv->wifi_spec ){// typical setting
		beQ		= QUEUE_LOW;
		bkQ 		= QUEUE_LOW;
		viQ 		= QUEUE_NORMAL;
		voQ 		= QUEUE_HIGH;
		mgtQ 	= QUEUE_HIGH;
		hiQ 		= QUEUE_HIGH;			
	}
	else{// for WMM
		beQ		= QUEUE_LOW;
		bkQ 		= QUEUE_NORMAL;
		viQ 		= QUEUE_NORMAL;
		voQ 		= QUEUE_HIGH;
		mgtQ 	= QUEUE_HIGH;
		hiQ 		= QUEUE_HIGH;			
	}
	_InitNormalChipRegPriority(Adapter,beQ,bkQ,viQ,voQ,mgtQ,hiQ);
}

static VOID
_InitNormalChipQueuePriority(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	switch(pHalData->OutEpNumber)
	{
		case 1:
			_InitNormalChipOneOutEpPriority(Adapter);
			break;
		case 2:
			_InitNormalChipTwoOutEpPriority(Adapter);
			break;
		case 3:
			_InitNormalChipThreeOutEpPriority(Adapter);
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}


}

static VOID
_InitTestChipQueuePriority(
	IN	PADAPTER Adapter
	)
{
	u8	hq_sele ;
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	
	switch(pHalData->OutEpNumber)
	{
		case 2:	// (TX_SELE_HQ|TX_SELE_LQ)
			if(!pregistrypriv->wifi_spec)//typical setting			
				hq_sele =  HQSEL_VOQ | HQSEL_VIQ | HQSEL_MGTQ | HQSEL_HIQ ;
			else	//for WMM
				hq_sele = HQSEL_VOQ | HQSEL_BEQ | HQSEL_MGTQ | HQSEL_HIQ ;
			break;
		case 1:
			if(TX_SELE_LQ == pHalData->OutEpQueueSel ){//map all endpoint to Low queue
				 hq_sele = 0;
			}
			else if(TX_SELE_HQ == pHalData->OutEpQueueSel){//map all endpoint to High queue
				hq_sele =  HQSEL_VOQ | HQSEL_VIQ | HQSEL_BEQ | HQSEL_BKQ | HQSEL_MGTQ | HQSEL_HIQ ;
			}		
			break;
		default:
			//RT_ASSERT(FALSE,("Shall not reach here!\n"));
			break;
	}
	write8(Adapter, (REG_TRXDMA_CTRL+1), hq_sele);
}


static VOID
_InitQueuePriority(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);

	//if(IS_NORMAL_CHIP( pHalData->VersionID)||IS_HARDWARE_TYPE_8192DU(Adapter)){
		_InitNormalChipQueuePriority(Adapter);
	//}
	//else{
	//	_InitTestChipQueuePriority(Adapter);
	//}
}

static VOID
_InitHardwareDropIncorrectBulkOut(
	IN  PADAPTER Adapter
	)
{
	u32	value32 = read32(Adapter, REG_TXDMA_OFFSET_CHK);
	value32 |= DROP_DATA_EN;
	write32(Adapter, REG_TXDMA_OFFSET_CHK, value32);
}

static VOID
_InitNetworkType(
	IN  PADAPTER Adapter
	)
{
	u32	value32;

	value32 = read32(Adapter, REG_CR);

	// TODO: use the other function to set network type
#if RTL8191C_FPGA_NETWORKTYPE_ADHOC
	value32 = (value32 & ~MASK_NETTYPE) | _NETTYPE(NT_LINK_AD_HOC);
#else
	value32 = (value32 & ~MASK_NETTYPE) | _NETTYPE(NT_LINK_AP);
#endif
	write32(Adapter, REG_CR, value32);
//	RASSERT(pIoBase->read8(REG_CR + 2) == 0x2);
}

static VOID
_InitTransferPageSize(
	IN  PADAPTER Adapter
	)
{
	// Tx page size is always 128.
	
	u8	value8;
	value8 = _PSRX(PBP_128) | _PSTX(PBP_128);
	write8(Adapter, REG_PBP, value8);
}

static VOID
_InitDriverInfoSize(
	IN  PADAPTER	Adapter,
	IN	u8		drvInfoSize
	)
{
	write8(Adapter,REG_RX_DRVINFO_SZ, drvInfoSize);
}

static VOID
_InitWMACSetting(
	IN  PADAPTER Adapter
	)
{
	//u4Byte			value32;
	u16			value16;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	//pHalData->ReceiveConfig = AAP | APM | AM | AB | APP_ICV | ADF | AMF | APP_FCS | HTC_LOC_CTRL | APP_MIC | APP_PHYSTS;
	pHalData->ReceiveConfig = AAP | APM | AM | AB | APP_ICV | AMF | HTC_LOC_CTRL | APP_MIC | APP_PHYSTS;
#if (0 == RTL8192C_RX_PACKET_NO_INCLUDE_CRC)
	pHalData->ReceiveConfig |= ACRC32;
#endif

	write32(Adapter, REG_RCR, pHalData->ReceiveConfig);

	// Accept all data frames
	value16 = 0xFFFF;
	write16(Adapter, REG_RXFLTMAP2, value16);

	// Accept all management frames
	value16 = 0xFFFF;
	write16(Adapter, REG_RXFLTMAP0, value16);

	//Reject all control frame - default value is 0
	write16(Adapter,REG_RXFLTMAP1,0x0);
}

static VOID
_InitAdaptiveCtrl(
	IN  PADAPTER Adapter
	)
{
	u16	value16;
	u32	value32;

	// Response Rate Set
	value32 = read32(Adapter, REG_RRSR);
	value32 &= ~RATE_BITMAP_ALL;
	value32 |= RATE_RRSR_CCK_ONLY_1M;
	write32(Adapter, REG_RRSR, value32);

	// CF-END Threshold
	//m_spIoBase->write8(REG_CFEND_TH, 0x1);

	// SIFS (used in NAV)
	value16 = _SPEC_SIFS_CCK(0x10) | _SPEC_SIFS_OFDM(0x10);
	write16(Adapter, REG_SPEC_SIFS, value16);

	// Retry Limit
	value16 = _LRL(0x30) | _SRL(0x30);
	write16(Adapter, REG_RL, value16);
	
}

static VOID
_InitRateFallback(
	IN  PADAPTER Adapter
	)
{
	// Set Data Auto Rate Fallback Retry Count register.
	write32(Adapter, REG_DARFRC, 0x00000000);
	write32(Adapter, REG_DARFRC+4, 0x10080404);
	write32(Adapter, REG_RARFRC, 0x04030201);
	write32(Adapter, REG_RARFRC+4, 0x08070605);

}


static VOID
_InitEDCA(
	IN  PADAPTER Adapter
	)
{
	//PHAL_DATA_8192CUSB	pHalData = GetHalData8192CUsb(Adapter);
	u16				value16;

#if 1
	//disable EDCCA count down, to reduce collison and retry
	value16 = read16(Adapter, REG_RD_CTRL);
	value16 |= DIS_EDCA_CNT_DWN;
	write16(Adapter, REG_RD_CTRL, value16);	


	// Update SIFS timing.  ??????????
	//pHalData->SifsTime = 0x0e0e0a0a;
	//Adapter->HalFunc.SetHwRegHandler( Adapter, HW_VAR_SIFS,  (pu1Byte)&pHalData->SifsTime);
	// SIFS for CCK Data ACK
	write8(Adapter, REG_SIFS_CTX, 0xa);
	// SIFS for CCK consecutive tx like CTS data!
	write8(Adapter, REG_SIFS_CTX+1, 0xa);
	
	// SIFS for OFDM Data ACK
	write8(Adapter, REG_SIFS_TRX, 0xe);
	// SIFS for OFDM consecutive tx like CTS data!
	write8(Adapter, REG_SIFS_TRX+1, 0xe);

	// Set CCK/OFDM SIFS
	write16(Adapter, REG_SIFS_CTX, 0x0a0a); // CCK SIFS shall always be 10us.
	write16(Adapter, REG_SIFS_TRX, 0x1010);
#endif

	write16(Adapter, REG_PROT_MODE_CTRL, 0x0204);

	write32(Adapter, REG_BAR_MODE_CTRL, 0x014004);


	// TXOP
	write32(Adapter, REG_EDCA_BE_PARAM, 0x005EA42B);
	write32(Adapter, REG_EDCA_BK_PARAM, 0x0000A44F);
	write32(Adapter, REG_EDCA_VI_PARAM, 0x005EA324);
	write32(Adapter, REG_EDCA_VO_PARAM, 0x002FA226);

	// PIFS
	write8(Adapter, REG_PIFS, 0x1C);
		
	//AGGR BREAK TIME Register
	write8(Adapter, REG_AGGR_BREAK_TIME, 0x16);

	write16(Adapter, REG_NAV_PROT_LEN, 0x0040);
	
	write8(Adapter, REG_BCNDMATIM, 0x02);

	write8(Adapter, REG_ATIMWND, 0x02);
	
}


static VOID
_InitAMPDUAggregation(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);

	write32(Adapter, REG_AGGLEN_LMT, 0x99997631);

	if(pHalData->MacPhyMode92D != SINGLEMAC_SINGLEPHY)
		write32(Adapter, REG_AGGLEN_LMT, 0x88888888);
	else
		write32(Adapter, REG_AGGLEN_LMT, 0xffffffff);
	
	write8(Adapter, REG_AGGR_BREAK_TIME, 0x16);
}

static VOID
_InitBeaconMaxError(
	IN  PADAPTER	Adapter,
	IN	BOOLEAN		InfraMode
	)
{
#ifdef RTL8192CU_ADHOC_WORKAROUND_SETTING
	write8(Adapter, REG_BCN_MAX_ERR,  0xFF );
#else
	//write8(Adapter, REG_BCN_MAX_ERR, (InfraMode ? 0xFF : 0x10));
#endif
}

static VOID
_InitRDGSetting(
	IN	PADAPTER Adapter
	)
{
	write8(Adapter,REG_RD_CTRL,0xFF);
	write16(Adapter, REG_RD_NAV_NXT, 0x200);
	write8(Adapter,REG_RD_RESP_PKT_TH,0x05);
}

VOID
_InitRxSetting(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(pHalData->interfaceIndex == 0)
	{
		write32(Adapter, REG_MACID, 0x87654321);
		write32(Adapter, 0x0700, 0x87654321);
	}
	else
	{
		write32(Adapter, REG_MACID, 0x12345678);
		write32(Adapter, 0x0700, 0x12345678); 	
	}	
}

static VOID
_InitRetryFunction(
	IN  PADAPTER Adapter
	)
{
	u8	value8;
	
	value8 = read8(Adapter, REG_FWHW_TXQ_CTRL);
	value8 |= EN_AMPDU_RTY_NEW;
	write8(Adapter, REG_FWHW_TXQ_CTRL, value8);

	// Set ACK timeout
	write8(Adapter, REG_ACKTO, 0x40);
}


static VOID
_InitUsbAggregationSetting(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

#ifdef USB_TX_AGGREGATION_92C
{
	u32			value32;

	if(Adapter->registrypriv.wifi_spec)
		pHalData->UsbTxAggMode = _FALSE;

	if(pHalData->MacPhyMode92D!=SINGLEMAC_SINGLEPHY)
		pHalData->UsbTxAggDescNum = 3;

	if(pHalData->UsbTxAggMode){
		value32 = read32(Adapter, REG_TDECTRL);
		value32 = value32 & ~(BLK_DESC_NUM_MASK << BLK_DESC_NUM_SHIFT);
		value32 |= ((pHalData->UsbTxAggDescNum & BLK_DESC_NUM_MASK) << BLK_DESC_NUM_SHIFT);
		
		write32(Adapter, REG_TDECTRL, value32);
	}
}
#endif

	// Rx aggregation setting
#ifdef USB_RX_AGGREGATION_92C
{
	u8		valueDMA;
	u8		valueUSB;

	if(pHalData->MacPhyMode92D!=SINGLEMAC_SINGLEPHY)
		pHalData->UsbRxAggPageCount	= 24;

	valueDMA = read8(Adapter, REG_TRXDMA_CTRL);
	valueUSB = read8(Adapter, REG_USB_SPECIAL_OPTION);

	switch(pHalData->UsbRxAggMode)
	{
		case USB_RX_AGG_DMA:
			valueDMA |= RXDMA_AGG_EN;
			valueUSB &= ~USB_AGG_EN;
			break;
		case USB_RX_AGG_USB:
			valueDMA &= ~RXDMA_AGG_EN;
			valueUSB |= USB_AGG_EN;
			break;
		case USB_RX_AGG_DMA_USB:
			valueDMA |= RXDMA_AGG_EN;
			valueUSB |= USB_AGG_EN;
			break;
		case USB_RX_AGG_DISABLE:
		default:
			valueDMA &= ~RXDMA_AGG_EN;
			valueUSB &= ~USB_AGG_EN;
			break;
	}

	write8(Adapter, REG_TRXDMA_CTRL, valueDMA);
	write8(Adapter, REG_USB_SPECIAL_OPTION, valueUSB);
#if 1
	switch(pHalData->UsbRxAggMode)
	{
		case USB_RX_AGG_DMA:
			write8(Adapter, REG_RXDMA_AGG_PG_TH, pHalData->UsbRxAggPageCount);
			write8(Adapter, REG_USB_DMA_AGG_TO, pHalData->UsbRxAggPageTimeout);
			break;
		case USB_RX_AGG_USB:
			write8(Adapter, REG_USB_AGG_TH, pHalData->UsbRxAggBlockCount);
			write8(Adapter, REG_USB_AGG_TO, pHalData->UsbRxAggBlockTimeout);
			break;
		case USB_RX_AGG_DMA_USB:
			write8(Adapter, REG_RXDMA_AGG_PG_TH, pHalData->UsbRxAggPageCount);
			write8(Adapter, REG_USB_DMA_AGG_TO, pHalData->UsbRxAggPageTimeout);
			write8(Adapter, REG_USB_AGG_TH, pHalData->UsbRxAggBlockCount);
			write8(Adapter, REG_USB_AGG_TO, pHalData->UsbRxAggBlockTimeout);
			break;
		case USB_RX_AGG_DISABLE:
		default:
			// TODO: 
			break;
	}
#endif
	switch(PBP_128)
	{
		case PBP_128:
			pHalData->HwRxPageSize = 128;
			break;
		case PBP_64:
			pHalData->HwRxPageSize = 64;
			break;
		case PBP_256:
			pHalData->HwRxPageSize = 256;
			break;
		case PBP_512:
			pHalData->HwRxPageSize = 512;
			break;
		case PBP_1024:
			pHalData->HwRxPageSize = 1024;
			break;
		default:
			//RT_ASSERT(FALSE, ("RX_PAGE_SIZE_REG_VALUE definition is incorrect!\n"));
			break;
	}

}
#endif

}


static VOID
_InitOperationMode(
	IN	PADAPTER			Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8				regBwOpMode = 0, MinSpaceCfg;
	u32				regRATR = 0, regRRSR = 0;

	//1 This part need to modified according to the rate set we filtered!!
	//
	// Set RRSR, RATR, and REG_BWOPMODE registers
	//
	switch(pHalData->CurrentWirelessMode)
	{
		case WIRELESS_MODE_B:
			regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK;
			regRRSR = RATE_ALL_CCK;
			break;
		case WIRELESS_MODE_A:
			//RT_ASSERT(FALSE,("Error wireless a mode\n"));
#if 1
			regBwOpMode = BW_OPMODE_5G |BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_OFDM_AG;
			regRRSR = RATE_ALL_OFDM_AG;
#endif
			break;
		case WIRELESS_MODE_G:
			regBwOpMode = BW_OPMODE_20MHZ;
			regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			break;
		case WIRELESS_MODE_UNKNOWN:
		case WIRELESS_MODE_AUTO:
			//if (pHalData->bInHctTest)
			if (0)
			{
			    regBwOpMode = BW_OPMODE_20MHZ;
			    regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			    regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			}
			else
			{
			    regBwOpMode = BW_OPMODE_20MHZ;
			    regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			    regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			}
			break;
		case WIRELESS_MODE_N_24G:
			// It support CCK rate by default.
			// CCK rate will be filtered out only when associated AP does not support it.
			regBwOpMode = BW_OPMODE_20MHZ;
				regRATR = RATE_ALL_CCK | RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
				regRRSR = RATE_ALL_CCK | RATE_ALL_OFDM_AG;
			break;
		case WIRELESS_MODE_N_5G:
			//RT_ASSERT(FALSE,("Error wireless mode"));
#if 1
			regBwOpMode = BW_OPMODE_5G;
			regRATR = RATE_ALL_OFDM_AG | RATE_ALL_OFDM_1SS | RATE_ALL_OFDM_2SS;
			regRRSR = RATE_ALL_OFDM_AG;
#endif
			break;
	}

	// Ziv ????????
	//write32(Adapter, REG_INIRTS_RATE_SEL, regRRSR);
	write8(Adapter, REG_BWOPMODE, regBwOpMode);

	// For Min Spacing configuration.
	switch(pHalData->rf_type)
	{
		case RF_1T2R:
		case RF_1T1R:
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("Initializeadapter: RF_Type%s\n", (pHalData->RF_Type==RF_1T1R? "(1T1R)":"(1T2R)")));
			MinSpaceCfg = (MAX_MSS_DENSITY_1T<<3);						
			break;
		case RF_2T2R:
		case RF_2T2R_GREEN:
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("Initializeadapter:RF_Type(2T2R)\n"));
			MinSpaceCfg = (MAX_MSS_DENSITY_2T<<3);			
			break;
	}
	
	write8(Adapter, REG_AMPDU_MIN_SPACE, MinSpaceCfg);

}


static VOID
_InitSecuritySetting(
	IN  PADAPTER Adapter
	)
{
	invalidate_cam_all(Adapter);
}

 static VOID
_InitBeaconParameters(
	IN  PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	 //default value  for register 0x558 and 0x559 is  0x05 0x03 (92DU before bitfile0821) zhiyuan 2009/08/26
	write16(Adapter, REG_TBTT_PROHIBIT,0x3c02);// ms
	write8(Adapter, REG_DRVERLYINT, 0x05);//ms
	write8(Adapter, REG_BCNDMATIM, 0x03);

	// TODO: Remove these magic number
	//write16(Adapter, REG_TBTT_PROHIBIT,0x6404);// ms
	//write8(Adapter, REG_DRVERLYINT, DRIVER_EARLY_INT_TIME);//ms
	//write8(Adapter, REG_BCNDMATIM, BCN_DMA_ATIME_INT_TIME);

	// Suggested by designer timchen. Change beacon AIFS to the largest number
	// beacause test chip does not contension before sending beacon. by tynli. 2009.11.03
	//if(IS_NORMAL_CHIP( pHalData->VersionID)){
		write16(Adapter, REG_BCNTCFG, 0x660F);
	//}
	//else{		
	//	write16(Adapter, REG_BCNTCFG, 0x66FF);
	//}

}

static VOID
_InitRFType(
	IN	PADAPTER Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

#if (DISABLE_BB_RF==1)
	pHalData->rf_chip	= RF_PSEUDO_11N;
	pHalData->rf_type	= RF_1T1R;// RF_2T2R;
#else

	pHalData->rf_chip	= RF_6052;

	if(pHalData->MacPhyMode92D==DUALMAC_DUALPHY)
	{
		pHalData->rf_type = RF_1T1R;	
	}
	else{// SMSP OR DMSP
		pHalData->rf_type = RF_2T2R;
	}
#endif
}

#if RTL8192CU_ADHOC_WORKAROUND_SETTING
static VOID _InitAdhocWorkaroundParams(IN PADAPTER Adapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	pHalData->RegBcnCtrlVal = read8(Adapter, REG_BCN_CTRL);
	pHalData->RegTxPause = read8(Adapter, REG_TXPAUSE); 
	pHalData->RegFwHwTxQCtrl = read8(Adapter, REG_FWHW_TXQ_CTRL+2);
}
#endif

static VOID
_BeaconFunctionEnable(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			Enable,
	IN	BOOLEAN			Linked
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			value8 = 0;
#if 0
	value8 = Enable ? (EN_BCN_FUNCTION | EN_TXBCN_RPT) : EN_BCN_FUNCTION;

	if(_FALSE == Linked){		
		if(IS_NORMAL_CHIP( pHalData->VersionID)){
			value8 |= DIS_TSF_UDT0_NORMAL_CHIP;
		}
		else{
			value8 |= DIS_TSF_UDT0_TEST_CHIP;
		}
	}

	write8(Adapter, REG_BCN_CTRL, value8);
#else
	// 20100901 zhiyuan: Change original setting of BCN_CTRL(0x550) from 
	// 0x1a to 0x1b. Set BIT0 of this register disable ATIM  function. 
	//  enable ATIM function may invoke HW Tx stop operation. This may cause ping failed 
	// sometimes in long run test. So just disable it now.
	// When ATIM function is disabled, High Queue should not use anymore.
	write8(Adapter, REG_BCN_CTRL, 0x1b);
	write8(Adapter, REG_RD_CTRL+1, 0x6F);
#endif
}


// Set CCK and OFDM Block "ON"
static VOID _BBTurnOnBlock(
	IN	PADAPTER		Adapter
	)
{
	HAL_DATA_TYPE		*pHalData	= GET_HAL_DATA(Adapter);
#if (DISABLE_BB_RF)
	return;
#endif

	if(pHalData->CurrentBandType92D == BAND_ON_5G)
		PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bCCKEn, 0);
	else
		PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bCCKEn, 0x1);

	PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bOFDMEn, 0x1);
}

static VOID _RfPowerSave(
	IN	PADAPTER		Adapter
	)
{
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct pwrctrl_priv		*pwrctrlpriv = &Adapter->pwrctrlpriv;
	u8			eRFPath;

#if (DISABLE_BB_RF)
	return;
#endif

	if(pwrctrlpriv->reg_rfoff == _TRUE){ // User disable RF via registry.
		//RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8192CUsb(): Turn off RF for RegRfOff.\n"));
		//MgntActSet_RF_State(Adapter, rf_off, RF_CHANGE_BY_SW, _TRUE);
		// Those action will be discard in MgntActSet_RF_State because off the same state
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
		if(pHalData->bSlaveOfDMSP)
			return;
#endif
		for(eRFPath = 0; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x4, 0xC00, 0x0);
	}
	else if(pwrctrlpriv->rfoff_reason > RF_CHANGE_BY_PS){ // H/W or S/W RF OFF before sleep.
		//RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8192CUsb(): Turn off RF for RfOffReason(%ld).\n", pMgntInfo->RfOffReason));
		//MgntActSet_RF_State(Adapter, rf_off, pMgntInfo->RfOffReason, _TRUE);
	}
	else{
		pwrctrlpriv->rf_pwrstate = rf_on;
		pwrctrlpriv->rfoff_reason = 0; 
		//if(Adapter->bInSetPower || Adapter->bResetInProgress)
		//	PlatformUsbEnableInPipes(Adapter);
		//RT_TRACE((COMP_INIT|COMP_RF), DBG_LOUD, ("InitializeAdapter8192CUsb(): RF is on.\n"));
	}
}

#if ANTENNA_SELECTION_STATIC_SETTING
enum {
	Antenna_Lfet = 1,
	Antenna_Right = 2,	
};

static VOID
_InitAntenna_Selection	(IN	PADAPTER Adapter)
{
	write8(Adapter, REG_LEDCFG2, 0x82);	
	PHY_SetBBReg(Adapter, rFPGA0_XA_RFInterfaceOE, bAntennaSelect, Antenna_Right);
}
#endif

//
//	Description:
//	set RX packet buffer and other setting acording to dual mac mode
//
//	Assumption:
//		1. Boot from EEPROM and CR9346 regiser has verified.
//		2. PASSIVE_LEVEL (USB interface)
//
static VOID PHY_ConfigMacCoexist_RFPage92D(
		IN PADAPTER			Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	offset = IS_NORMAL_CHIP(pHalData->VersionID)?REG_MAC_PHY_CTRL_NORMAL:REG_MAC_PHY_CTRL;

	switch(pHalData->MacPhyMode92D)
	{
		case DUALMAC_DUALPHY:
			write8(Adapter,REG_DMC, 0x0);
			write8(Adapter,REG_RX_PKT_LIMIT,0x08);
			write16(Adapter,REG_TRXFF_BNDY+2, 0x13ff);
			break;
		case DUALMAC_SINGLEPHY:
			write8(Adapter,REG_DMC, 0xf8);
			write8(Adapter,REG_RX_PKT_LIMIT,0x08);
			write16(Adapter,REG_TRXFF_BNDY+2, 0x13ff);
			break;
		case SINGLEMAC_SINGLEPHY:
			write8(Adapter,REG_DMC, 0x0);
			write8(Adapter,REG_RX_PKT_LIMIT,0x10);
			if(IS_NORMAL_CHIP( pHalData->VersionID))
				write16(Adapter, (REG_TRXFF_BNDY + 2), 0x27FF);
			else
				write16(Adapter,REG_TRXFF_BNDY+2, 0x13ff);
			break;
		default:
			break;
	}		
}

u32 rtl8192du_hal_init(_adapter *padapter)
{
	u8	val8 = 0, tmpU1b;
	u16	val16;
	u32	boundary, i = 0, j, status = _SUCCESS;
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(padapter);
	struct pwrctrl_priv		*pwrctrlpriv = &padapter->pwrctrlpriv;
	struct registry_priv	*pregistrypriv = &padapter->registrypriv;
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	PADAPTER	BuddyAdapter = padapter->BuddyAdapter;
#endif

_func_enter_;

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(BuddyAdapter != NULL)
	{
		if(BuddyAdapter->MgntInfo.bHaltInProgress)
		{
			for(i=0;i<100;i++)
			{
				udelay_os(1000);
				if(!BuddyAdapter->MgntInfo.bHaltInProgress)
					break;
			}

			if(i==100)
			{
				RT_TRACE(COMP_INIT,DBG_LOUD,("fail to initialization due to another adapter is in halt \n"));
				return RT_STATUS_FAILURE;
			}
		}
	}
#endif
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("--->InitializeAdapter8192CUsb()\n"));

	if(padapter->bSurpriseRemoved)
		return _FAIL;

	pHalData->bCardDisableHWSM = _TRUE;

	ACQUIRE_GLOBAL_MUTEX(GlobalMutexForPowerAndEfuse);

	write8(padapter, REG_RSV_CTRL, 0x0);
	val8 = read8(padapter, 0x0003);
	val8 &= (~BIT7);
	write8(padapter, 0x0003, val8);

	//mac status:
	//0x81[4]:0 mac0 off, 1:mac0 on
	//0x82[4]:0 mac1 off, 1: mac1 on.
#if 0
	if(pHalData->interfaceIndex == 0)
	{
		val8 = read8(padapter, REG_MAC0);
		write8(padapter, REG_MAC0, val8|MAC0_ON);
	}
	else
	{
		val8 = read8(padapter, REG_MAC1);
		write8(padapter, REG_MAC1, val8|MAC1_ON);
	}
#endif
	//For s3/s4 may reset mac,Reg0xf8 may be set to 0, so reset macphy control reg here.
	PHY_ConfigMacPhyMode92D(padapter);

	PHY_SetPowerOnFor8192D(padapter);

	status = _InitPowerOn(padapter);
	if(status == _FAIL){
		RT_TRACE(_module_hci_hal_init_c_, _drv_err_, ("Failed to init power on!\n"));
		RELEASE_GLOBAL_MUTEX(GlobalMutexForPowerAndEfuse);
		goto exit;
	}

	if(!pregistrypriv->wifi_spec){
		boundary = TX_PAGE_BOUNDARY;
	}
	else{// for WMM
		boundary = (IS_NORMAL_CHIP(pHalData->VersionID))	?WMM_NORMAL_TX_PAGE_BOUNDARY
													:WMM_TEST_TX_PAGE_BOUNDARY;
	}

	PHY_ConfigMacCoexist_RFPage92D(padapter);

	status =  InitLLTTable(padapter, boundary);
	if(status == _FAIL){
		RT_TRACE(_module_hci_hal_init_c_, _drv_err_, ("Failed to init power on!\n"));
		RELEASE_GLOBAL_MUTEX(GlobalMutexForPowerAndEfuse);
		return status;
	}
	_InitQueueReservedPage(padapter);
	_InitTxBufferBoundary(padapter);		
	_InitQueuePriority(padapter);
	//_InitPageBoundary(padapter);	
	_InitTransferPageSize(padapter);	

	// Get Rx PHY status in order to report RSSI and others.
	_InitDriverInfoSize(padapter, 4);

	_InitInterrupt(padapter);	
	_InitID(padapter);//set mac_address
	_InitNetworkType(padapter);//set msr	
	_InitWMACSetting(padapter);
	_InitAdaptiveCtrl(padapter);
	_InitEDCA(padapter);
	_InitRateFallback(padapter);
	_InitRetryFunction(padapter);
	_InitUsbAggregationSetting(padapter);
	_InitOperationMode(padapter);//todo
	_InitBeaconParameters(padapter);
	_InitAMPDUAggregation(padapter);
	_InitBeaconMaxError(padapter, _TRUE);
	_BeaconFunctionEnable(padapter, _FALSE, _FALSE);

#if ENABLE_USB_DROP_INCORRECT_OUT
	_InitHardwareDropIncorrectBulkOut(padapter);
#endif
	if(pHalData->bRDGEnable){
		_InitRDGSetting(padapter);
	}

	// Set Data Auto Rate Fallback Reg.
	for(i = 0 ; i < 4 ; i++){
		write32(padapter, REG_ARFR0+i*4, 0x1f0ffff0);
	}

	if(pregistrypriv->wifi_spec)
	{
		write16(padapter, REG_FAST_EDCA_CTRL, 0);
	}
	else{
		if(pHalData->MacPhyMode92D==SINGLEMAC_SINGLEPHY){
			write16(padapter, REG_FAST_EDCA_CTRL, 0x8888);
		}
		else{
			write16(padapter, REG_FAST_EDCA_CTRL, 0x4444);
		}
	}

	tmpU1b=read8(padapter, 0x605);
	tmpU1b|=0xf0;
	write8(padapter, 0x605,tmpU1b);	
	write8(padapter, 0x55e,0x30);
	write8(padapter, 0x55f,0x30);
	write8(padapter, 0x606,0x30);
	//PlatformEFIOWrite1Byte(Adapter, 0x5e4,0x38);	  // masked for new bitfile

	//for bitfile 0912/0923 zhiyuan 2009/09/23
	// temp for high queue and mgnt Queue corrupt in time;
	//it may cause hang when sw beacon use high_Q,other frame use mgnt_Q; or ,sw beacon use mgnt_Q ,other frame use high_Q;
	write8(padapter, 0x523, 0x10);
	val16 = read16(padapter, 0x524);
	val16|=BIT12;
	write16(padapter,0x524 , val16);

	write8(padapter,REG_TXPAUSE, 0);

	// suggested by zhouzhou   usb suspend  idle time count for bitfile0927  2009/10/09 zhiyuan
	val8=read8(padapter, 0xfe56);
	val8 |=(BIT0|BIT1);
	write8(padapter, 0xfe56, val8);

	// led control for 92du
	write16(padapter, REG_LEDCFG0, 0x8282);

	if(pHalData->bEarlyModeEanble)
	{
		DBG_8192C("EarlyMode Enabled!!!\n");

		tmpU1b = read8(padapter,0x4d0);
		tmpU1b = tmpU1b|0x1f;
		write8(padapter,0x4d0,tmpU1b);

		write8(padapter,0x4d3,0x80);

		tmpU1b = read8(padapter,0x605);
		tmpU1b = tmpU1b|0x40;
		write8(padapter,0x605,tmpU1b);
	}
	else
	{
		write8(padapter,0x4d0,0);
	}

#if ((1 == MP_DRIVER) ||  (0 == FW_PROCESS_VENDOR_CMD))

	_InitRxSetting(padapter);
	DBG_8192C("%s(): Don't Download Firmware !!\n",__FUNCTION__);
	padapter->bFWReady = _FALSE;
	pHalData->fw_ractrl = _FALSE;

#else

	status = FirmwareDownload92D(padapter);
	RELEASE_GLOBAL_MUTEX(GlobalMutexForPowerAndEfuse);
	if(status == _FAIL){
		padapter->bFWReady = _FALSE;
		pHalData->fw_ractrl = _FALSE;
		DBG_8192C("fw download fail!\n");
		goto exit;
	}
	else	{
		padapter->bFWReady = _TRUE;
		pHalData->fw_ractrl = _TRUE;
		DBG_8192C("fw download ok!\n");
	}

#endif

	if(pwrctrlpriv->reg_rfoff == _TRUE){
		pwrctrlpriv->rf_pwrstate = rf_off;
	}

	// Set RF type for BB/RF configuration	
	_InitRFType(padapter);//->_ReadRFType()

	// Save target channel
	// <Roger_Notes> Current Channel will be updated again later.
	//pHalData->CurrentChannel = 6;//default set to 6

#if (HAL_MAC_ENABLE == 1)
	status = PHY_MACConfig8192D(padapter);
	if(status == _FAIL)
	{
		goto exit;
	}

	//CLEAR ADF , for using RX_FILTER_MAP.
	write32(padapter, REG_RCR, read32(padapter, REG_RCR) & ~RCR_ADF);
#endif

	//
	//d. Initialize BB related configurations.
	//
#if (HAL_BB_ENABLE == 1)
	status = PHY_BBConfig8192D(padapter);
	if(status == _FAIL)
	{
		goto exit;
	}
#endif

	// 92CU use 3-wire to r/w RF
	//pHalData->Rf_Mode = RF_OP_By_SW_3wire;

#if (HAL_RF_ENABLE == 1)
	// set before initialize RF, 
	PHY_SetBBReg(padapter, rFPGA0_AnalogParameter4, 0x00f00000,  0xf);

	status = PHY_RFConfig8192D(padapter);	
	if(status == _FAIL)
	{
		goto exit;
	}

	// set default value after initialize RF, 
	PHY_SetBBReg(padapter, rFPGA0_AnalogParameter4, 0x00f00000,  0);

#if(RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(!pHalData->bSlaveOfDMSP)
#endif
	//_Update_BB_RF_FOR_92DU(padapter);
	PHY_UpdateBBRFConfiguration8192D(padapter);

#endif

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(!pHalData->bSlaveOfDMSP)
#endif
	_BBTurnOnBlock(padapter);

	_InitID(padapter);
	//NicIFSetMacAddress(padapter, padapter->PermanentAddress);

	if(pHalData->CurrentBandType92D == BAND_ON_5G)
	{
		pHalData->CurrentWirelessMode = WIRELESS_MODE_N_5G;
	}
	else
	{
		pHalData->CurrentWirelessMode = WIRELESS_MODE_N_24G;
	}

	_InitSecuritySetting(padapter);
	_RfPowerSave(padapter);

#if ANTENNA_SELECTION_STATIC_SETTING
	if (!(IS_92C_SERIAL(pHalData->VersionID) || IS_92D_SINGLEPHY(pHalData->VersionID))){ //for 88CU ,1T1R
		_InitAntenna_Selection(padapter);
	}
#endif

	// HW SEQ CTRL
	//set 0x0 to 0xFF by tynli. Default enable HW SEQ NUM.
	write8(padapter,REG_HWSEQ_CTRL, 0xFF); 

	//
	// f. Start to BulkIn transfer.
	//


#if (MP_DRIVER == 1)
	//MPT_InitializeAdapter(padapter, Channel);
#else // temply marked this for RF
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(!pHalData->bSlaveOfDMSP)
#endif
	{
		//rtl8192d_PHY_IQCalibrate(padapter);
		rtl8192d_dm_CheckTXPowerTracking(padapter);
		rtl8192d_PHY_LCCalibrate(padapter);

		//5G and 2.4G must wait sometime to let RF LO ready
		//by sherry 2010.06.28
		if(!IS_NORMAL_CHIP(pHalData->VersionID))
		{
#if 0
			if(pHalData->rf_type == RF_1T1R)
			{
				for(j=0;j<3000;j++)
					udelay_os(MAX_STALL_TIME);
			}
			else
			{
				for(j=0;j<5000;j++)
					udelay_os(MAX_STALL_TIME);
			}
#endif
			u32 tmpRega, tmpRegb;
			for(j=0;j<10000;j++)
			{
				udelay_os(MAX_STALL_TIME);
                           	if(pHalData->rf_type == RF_1T1R)
				{
					tmpRega = PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)RF90_PATH_A, 0x2a, bMaskDWord);
					if((tmpRega&BIT11)==BIT11)
						break;	
				}
				else
				{
					tmpRega = PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)RF90_PATH_A, 0x2a, bMaskDWord);
					tmpRegb = PHY_QueryRFReg(padapter, (RF90_RADIO_PATH_E)RF90_PATH_B, 0x2a, bMaskDWord);				
					if(((tmpRega&BIT11)==BIT11)&&((tmpRegb&BIT11)==BIT11))
						break;
					// temply add for DMSP
					if(pHalData->MacPhyMode92D==DUALMAC_SINGLEPHY&&(pHalData->interfaceIndex!=0))
						break;
				}
			}
		}
	}
#endif

#if RTL8192CU_ADHOC_WORKAROUND_SETTING
	_InitAdhocWorkaroundParams(padapter);
#endif

	rtl8192d_InitHalDm(padapter);

	//misc
	{
		int i;		
		u8 mac_addr[6];
		for(i=0; i<6; i++)
		{			
			mac_addr[i] = read8(padapter, REG_MACID+i);		
		}
		
		DBG_8192C("MAC Address from REG = %x-%x-%x-%x-%x-%x\n", 
			mac_addr[0],	mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
	}

	{	
		//pHalData->LoopbackMode=LOOPBACK_MODE;
		//if(padapter->ResetProgress == RESET_TYPE_NORESET)
		//{
		       u32					ulRegRead;
			//3//
			//3//Set Loopback mode or Normal mode
			//3//
			//2006.12.13 by emily. Note!We should not merge these two CPU_GEN register writings 
			//	because setting of System_Reset bit reset MAC to default transmission mode.
			ulRegRead = read32(padapter, 0x100);	//CPU_GEN  0x100

			//if(pHalData->LoopbackMode == RTL8192SU_NO_LOOPBACK)
			{
				ulRegRead |= ulRegRead ;
			}
			//else if (pHalData->LoopbackMode == RTL8192SU_MAC_LOOPBACK )
			//{
				//RT_TRACE(COMP_INIT, DBG_LOUD, ("==>start loop back mode %x\n",ulRegRead));
			//	ulRegRead |= 0x0b000000; //0x0b000000 CPU_CCK_LOOPBACK;
			//}
			//else if(pHalData->LoopbackMode == RTL8192SU_DMA_LOOPBACK)
			//{
				//RT_TRACE(COMP_INIT, DBG_LOUD, ("==>start dule mac loop back mode %x\n",ulRegRead));
			//	ulRegRead |= 0x07000000; //0x07000000 CPU_CCK_LOOPBACK;
			//}
			//else
			//{
				//RT_ASSERT(FALSE, ("Serious error: wrong loopback mode setting\n"));	
			//}

			write32(padapter, 0x100, ulRegRead);
	              //RT_TRACE(COMP_INIT,DBG_LOUD,("==>loop back mode CPU GEN value:%x\n",ulRegRead));

			// 2006.11.29. After reset cpu, we sholud wait for a second, otherwise, it may fail to write registers. Emily
			udelay_os(500);
		//}
	}

	RT_CLEAR_PS_LEVEL(pwrctrlpriv, RT_RF_OFF_LEVL_HALT_NIC);

exit:

_func_exit_;

	return status;
}


static VOID 
_DisableGPIO(
	IN	PADAPTER	Adapter
	)
{
/***************************************
j. GPIO_PIN_CTRL 0x44[31:0]=0x000		// 
k. Value = GPIO_PIN_CTRL[7:0]
l.  GPIO_PIN_CTRL 0x44[31:0] = 0x00FF0000 | (value <<8); //write external PIN level
m. GPIO_MUXCFG 0x42 [15:0] = 0x0780
n. LEDCFG 0x4C[15:0] = 0x8080
***************************************/
	u8	value8;
	u16	value16;
	u32	value32;

	//1. Disable GPIO[7:0]
	write16(Adapter, REG_GPIO_PIN_CTRL+2, 0x0000);
    	value32 = read32(Adapter, REG_GPIO_PIN_CTRL) & 0xFFFF00FF;  
	value8 = (u8) (value32&0x000000FF);
	value32 |= ((value8<<8) | 0x00FF0000);
	write32(Adapter, REG_GPIO_PIN_CTRL, value32);
	      
	//2. Disable GPIO[10:8]          
	write8(Adapter, REG_GPIO_MUXCFG+3, 0x00);
	value16 = read16(Adapter, REG_GPIO_MUXCFG+2) & 0xFF0F;  
	value8 = (u8) (value16&0x000F);
	value16 |= ((value8<<4) | 0x0780);
	write16(Adapter, REG_GPIO_MUXCFG+2, value16);

	//3. Disable LED0 & 1
	write16(Adapter, REG_LEDCFG0, 0x8888);

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Disable GPIO and LED.\n"));
 
} //end of _DisableGPIO()

static VOID
_ResetFWDownloadRegister(
	IN PADAPTER			Adapter
	)
{
	u32	value32;

	value32 = read32(Adapter, REG_MCUFWDL);
	value32 &= ~(MCUFWDL_EN | MCUFWDL_RDY);
	write32(Adapter, REG_MCUFWDL, value32);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reset FW download register.\n"));
}


static int
_DisableRF_AFE(
	IN PADAPTER			Adapter
	)
{
	int		rtStatus = _SUCCESS;
	u32			pollingCount = 0;
	u8			value8;
	
	//disable RF/ AFE AD/DA
	value8 = APSDOFF;
	write8(Adapter, REG_APSD_CTRL, value8);


#if (RTL8192CU_ASIC_VERIFICATION)

	do
	{
		if(read8(Adapter, REG_APSD_CTRL) & APSDOFF_STATUS){
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("Disable RF, AFE, AD, DA Done!\n"));
			break;
		}

		if(pollingCount++ > POLLING_READY_TIMEOUT_COUNT){
			//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Failed to polling APSDOFF_STATUS done!\n"));
			return _FAIL;
		}
				
	}while(_TRUE);
	
#endif

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Disable RF, AFE,AD, DA.\n"));
	return rtStatus;

}

static VOID
_ResetBB(
	IN PADAPTER			Adapter
	)
{
	u16	value16;

	//reset BB
	value16 = read16(Adapter, REG_SYS_FUNC_EN);
	value16 &= ~(FEN_BBRSTB | FEN_BB_GLB_RSTn);
	write16(Adapter, REG_SYS_FUNC_EN, value16);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reset BB.\n"));
}

static VOID
_ResetMCU(
	IN PADAPTER			Adapter
	)
{
	u16	value16;
	
	// reset MCU
	value16 = read16(Adapter, REG_SYS_FUNC_EN);
	value16 &= ~FEN_CPUEN;
	write16(Adapter, REG_SYS_FUNC_EN, value16);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Reset MCU.\n"));
}

static VOID
_DisableMAC_AFE_PLL(
	IN PADAPTER			Adapter
	)
{
	u32	value32;
	
	//disable MAC/ AFE PLL
	value32 = read32(Adapter, REG_APS_FSMCO);
	value32 |= APDM_MAC;
	write32(Adapter, REG_APS_FSMCO, value32);
	
	value32 |= APFM_OFF;
	write32(Adapter, REG_APS_FSMCO, value32);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Disable MAC, AFE PLL.\n"));
}

static VOID
_AutoPowerDownToHostOff(
	IN	PADAPTER		Adapter
	)
{
	u32			value32;
	write8(Adapter, REG_SPS0_CTRL, 0x22);

	value32 = read32(Adapter, REG_APS_FSMCO);	
	
	value32 |= APDM_HOST;//card disable
	write32(Adapter, REG_APS_FSMCO, value32);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Auto Power Down to Host-off state.\n"));

	// set USB suspend
	value32 = read32(Adapter, REG_APS_FSMCO);
	value32 &= ~AFSM_PCIE;
	write32(Adapter, REG_APS_FSMCO, value32);

}

static VOID
_SetUsbSuspend(
	IN PADAPTER			Adapter
	)
{
	u32			value32;

	value32 = read32(Adapter, REG_APS_FSMCO);
	
	// set USB suspend
	value32 |= AFSM_HSUS;
	write32(Adapter, REG_APS_FSMCO, value32);

	//RT_ASSERT(0 == (read32(Adapter, REG_APS_FSMCO) & BIT(12)),(""));
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Set USB suspend.\n"));
	
}

static VOID
_DisableRFAFEAndResetBB8192D(
	IN PADAPTER			Adapter
	)
{
/**************************************
a.	TXPAUSE 0x522[7:0] = 0xFF             //Pause MAC TX queue
b.	RF path 0 offset 0x00 = 0x00            // disable RF
c. 	APSD_CTRL 0x600[7:0] = 0x40
d.	SYS_FUNC_EN 0x02[7:0] = 0x16		//reset BB state machine
e.	SYS_FUNC_EN 0x02[7:0] = 0x14		//reset BB state machine
***************************************/
       HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	eRFPath = 0,value8 = 0;

	PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, 0x00f00000,  0xf);
	PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x0,bRFRegOffsetMask, 0x0);

	value8 |= APSDOFF;
	write8(Adapter, REG_APSD_CTRL, value8);//0x40

	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{			
		//testchip  should not do BB reset if another mac is alive;
		value8 = 0 ; 
		value8 |=( FEN_USBD | FEN_USBA | FEN_BB_GLB_RSTn);
		write8(Adapter, REG_SYS_FUNC_EN,value8 );//0x16
	}

	if(pHalData->MacPhyMode92D!=SINGLEMAC_SINGLEPHY)
	{
		if(pHalData->interfaceIndex!=0){
			value8 &=( ~FEN_BB_GLB_RSTn );
			write8(Adapter, REG_SYS_FUNC_EN, value8); //0x14	
		}
	}
	else{
		value8 &=( ~FEN_BB_GLB_RSTn );
		write8(Adapter, REG_SYS_FUNC_EN, value8); //0x14	
	}

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> RF off and reset BB.\n"));
}

static VOID
_DisableRFAFEAndResetBB(
	IN PADAPTER			Adapter
	)
{
	_DisableRFAFEAndResetBB8192D(Adapter);
}

static VOID
_ResetDigitalProcedure1(
	IN 	PADAPTER			Adapter,
	IN	BOOLEAN				bWithoutHWSM	
	)
{

	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);

	if((pHalData->FirmwareVersion <=  0x20)&&(_FALSE)){
		#if 0
		/*****************************
		f.	SYS_FUNC_EN 0x03[7:0]=0x54		// reset MAC register, DCORE
		g.	MCUFWDL 0x80[7:0]=0				// reset MCU ready status
		******************************/
		u4Byte	value32 = 0;
		PlatformIOWrite1Byte(Adapter, REG_SYS_FUNC_EN+1, 0x54);
		PlatformIOWrite1Byte(Adapter, REG_MCUFWDL, 0);	
		#else
		/*****************************
		f.	MCUFWDL 0x80[7:0]=0				// reset MCU ready status
		g.	SYS_FUNC_EN 0x02[10]= 0			// reset MCU register, (8051 reset)
		h.	SYS_FUNC_EN 0x02[15-12]= 5		// reset MAC register, DCORE
		i.     SYS_FUNC_EN 0x02[10]= 1			// enable MCU register, (8051 enable)
		******************************/
			u16 valu16 = 0;
			write8(Adapter, REG_MCUFWDL, 0);

			valu16 = read16(Adapter, REG_SYS_FUNC_EN);	
			write16(Adapter, REG_SYS_FUNC_EN, (valu16 & (~FEN_CPUEN)));//reset MCU ,8051

			valu16 = read16(Adapter, REG_SYS_FUNC_EN)&0x0FFF;	
			write16(Adapter, REG_SYS_FUNC_EN, (valu16 |(FEN_HWPDN|FEN_ELDR)));//reset MAC
			
			valu16 = read16(Adapter, REG_SYS_FUNC_EN);	
			write16(Adapter, REG_SYS_FUNC_EN, (valu16 | FEN_CPUEN));//enable MCU ,8051	

		
		#endif
	}
	else{
		u8 retry_cnts = 0;	
		if(IS_NORMAL_CHIP(pHalData->VersionID) || (_FALSE)){
			if(read8(Adapter, REG_MCUFWDL) & BIT1){ //IF fw in RAM code, do reset 

				write8(Adapter, REG_MCUFWDL, 0);
				if(Adapter->bFWReady)
				{
					write8(Adapter, REG_HMETFR+3, 0x20);//8051 reset by self
					while( (retry_cnts++ <100) && (FEN_CPUEN &read16(Adapter, REG_SYS_FUNC_EN)))
					{					
						udelay_os(50);//PlatformStallExecution(50);//us
					}
					if(retry_cnts >= 100){				
						DBG_8192C("#####=> 8051 reset failed!.........................\n");
					}
					
					//RT_ASSERT((retry_cnts < 100), );			
					//RT_TRACE(COMP_INIT, DBG_LOUD, ("=====> 8051 reset success (%d) .\n",retry_cnts));
				}
			}
			else{
				//RT_TRACE(COMP_INIT, DBG_LOUD, ("=====> 8051 in ROM.\n"));
			}	
		}

	   	if(pHalData->bInSetPower&&(!IS_NORMAL_CHIP(pHalData->VersionID)))
		{		
			// S3 or s4   FW 8051 go to rom
			u8  value8;
			retry_cnts = 0;
			write32(Adapter, 0x130, 0x20);
			value8=read8(Adapter, REG_MCUFWDL);
			value8 &=~(BIT1);
			write8(Adapter, REG_MCUFWDL,value8);
			write8(Adapter,REG_HMETFR+3,0x40);
			while( (retry_cnts++ <10) && (BIT7&read8(Adapter, REG_MCUFWDL)))
			{
				udelay_os(50);//us
			}
			udelay_os(50);//us
		}

		write8(Adapter, REG_SYS_FUNC_EN+1, 0x54);	//Reset MAC and Enable 8051
	}

	if(bWithoutHWSM){
	/*****************************
		Without HW auto state machine
	g.	SYS_CLKR 0x08[15:0] = 0x30A3			//disable MAC clock
	h.	AFE_PLL_CTRL 0x28[7:0] = 0x80			//disable AFE PLL
	i.	AFE_XTAL_CTRL 0x24[15:0] = 0x880F		//gated AFE DIG_CLOCK
	j.	SYS_ISO_CTRL 0x00[7:0] = 0xF9			// isolated digital to PON
	******************************/	
		//write16(Adapter, REG_SYS_CLKR, 0x30A3);
		write16(Adapter, REG_SYS_CLKR, 0x70A3);//modify to 0x70A3 by Scott.
		write8(Adapter, REG_AFE_PLL_CTRL, 0x80);		
		write16(Adapter, REG_AFE_XTAL_CTRL, 0x880F);
		write8(Adapter, REG_SYS_ISO_CTRL, 0xF9);		
	}
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Reset Digital.\n"));

}

static VOID
_ResetDigitalProcedure2(
	IN 	PADAPTER			Adapter
)
{
/*****************************
k.	SYS_FUNC_EN 0x03[7:0] = 0x44			// disable ELDR runction
l.	SYS_CLKR 0x08[15:0] = 0x3083			// disable ELDR clock
m.	SYS_ISO_CTRL 0x01[7:0] = 0x83			// isolated ELDR to PON
******************************/
	//write8(Adapter, REG_SYS_FUNC_EN+1, 0x44);//marked by Scott.
	//write16(Adapter, REG_SYS_CLKR, 0x3083);
	//write8(Adapter, REG_SYS_ISO_CTRL+1, 0x83);

 	write16(Adapter, REG_SYS_CLKR, 0x70a3); //modify to 0x70a3 by Scott.
 	write8(Adapter, REG_SYS_ISO_CTRL+1, 0x82); //modify to 0x82 by Scott.
}

static VOID
_DisableAnalog(
	IN PADAPTER			Adapter,
	IN BOOLEAN			bWithoutHWSM	
	)
{
	u32 value16 = 0;
	u8 value8=0;
	
	if(bWithoutHWSM){
	/*****************************
	n.	LDOA15_CTRL 0x20[7:0] = 0x04		// disable A15 power
	o.	LDOV12D_CTRL 0x21[7:0] = 0x54		// disable digital core power
	r.	When driver call disable, the ASIC will turn off remaining clock automatically 
	******************************/
	
		write8(Adapter, REG_LDOA15_CTRL, 0x04);
		//PlatformIOWrite1Byte(Adapter, REG_LDOV12D_CTRL, 0x54);		
		
		value8 = read8(Adapter, REG_LDOV12D_CTRL);		
		value8 &= (~LDV12_EN);
		write8(Adapter, REG_LDOV12D_CTRL, value8);	
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" REG_LDOV12D_CTRL Reg0x21:0x%02x.\n",value8));
	}
	
/*****************************
h.	SPS0_CTRL 0x11[7:0] = 0x23			//enter PFM mode
i.	APS_FSMCO 0x04[15:0] = 0x4802		// set USB suspend 
******************************/	
	write8(Adapter, REG_SPS0_CTRL, 0x23);
	
	value16 |= (APDM_HOST | AFSM_HSUS |PFM_ALDN);
	write16(Adapter, REG_APS_FSMCO,value16 );//0x4802 

	write8(Adapter, REG_RSV_CTRL, 0x0e);

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("======> Disable Analog Reg0x04:0x%04x.\n",value16));
}


static BOOLEAN
CanGotoPowerOff92D(
	IN	PADAPTER			Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8 u1bTmp;
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	PADAPTER	BuddyAdapter = Adapter->BuddyAdapter;
#endif

	if(pHalData->MacPhyMode92D==SINGLEMAC_SINGLEPHY)
		return _TRUE;	  
	
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(BuddyAdapter != NULL)
	{
		if(BuddyAdapter->MgntInfo.init_adpt_in_progress)
		{
			RT_TRACE(COMP_DUALMACSWITCH,DBG_LOUD,("do not power off during another adapter is initialization \n"));
			return _FALSE;
		}
	}

#endif

	if(pHalData->interfaceIndex==0)
	{	// query another mac status;
		u1bTmp = read8(Adapter, REG_MAC1);
		u1bTmp&=MAC1_ON;
	}
	else
	{
		u1bTmp = read8(Adapter, REG_MAC0);
		u1bTmp&=MAC0_ON;
	}

	//0x17[7]:1b' power off in process
	u1bTmp=read8(Adapter, 0x17);
	u1bTmp|=BIT7;
	write8(Adapter, 0x17, u1bTmp);

	udelay_os(500);
	// query another mac status;
	if(pHalData->interfaceIndex==0)
	{	// query another mac status;
		u1bTmp = read8(Adapter, REG_MAC1);
		u1bTmp&=MAC1_ON;
	}
	else
	{
		u1bTmp = read8(Adapter, REG_MAC0);
		u1bTmp&=MAC0_ON;
	}
	//if another mac is alive,do not do power off 
	if(u1bTmp)
	{
		u1bTmp=read8(Adapter, 0x17);
		u1bTmp&=(~BIT7);
		write8(Adapter, 0x17, u1bTmp);
		return _FALSE;
	}
	return _TRUE;
}


static int	
CardDisableHWSM( // HW Auto state machine
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			resetMCU
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	int		rtStatus = _SUCCESS;
	u8		value;
	
	if(Adapter->bSurpriseRemoved){
		return rtStatus;
	}

	write8(Adapter, REG_TXPAUSE, 0xFF);

#if 1
	//==== RF Off Sequence ====
#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
	if(!Adapter->bSlaveOfDMSP)
#endif
		_DisableRFAFEAndResetBB(Adapter);

	if(!PHY_CheckPowerOffFor8192D(Adapter))
		return rtStatus;

	if(!IS_NORMAL_CHIP(pHalData->VersionID))
	{			
		value = 0 ; 
		value |=( FEN_USBD | FEN_USBA | FEN_BB_GLB_RSTn);
		write8(Adapter, REG_SYS_FUNC_EN,value);//0x16		
	}
	//0x20:value 05-->04
	write8(Adapter, REG_LDOA15_CTRL,0x04);
      	//RF Control 
	write8(Adapter, REG_RF_CTRL,0);

	//  ==== Reset digital sequence   ======
	_ResetDigitalProcedure1(Adapter, _FALSE);
	
	//  ==== Pull GPIO PIN to balance level and LED control ======
	_DisableGPIO(Adapter);

	//  ==== Disable analog sequence ===
	_DisableAnalog(Adapter, _FALSE);

	ACQUIRE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);
	value=read8(Adapter, REG_POWER_OFF_IN_PROCESS);
	value&=(~BIT7);
	write8(Adapter, REG_POWER_OFF_IN_PROCESS, value);
	RELEASE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);

	RT_TRACE(_module_hci_hal_init_c_, _drv_info_, ("======> Card disable finished.\n"));
#else
	_DisableGPIO(Adapter);
	
	//reset FW download register
	_ResetFWDownloadRegister(Adapter);


	//disable RF/ AFE AD/DA
	rtStatus = _DisableRF_AFE(Adapter);
	if(RT_STATUS_SUCCESS != rtStatus){
		RT_TRACE(COMP_INIT, DBG_SERIOUS, ("_DisableRF_AFE failed!\n"));
		goto Exit;
	}
	_ResetBB(Adapter);

	if(resetMCU){
		_ResetMCU(Adapter);
	}

	_AutoPowerDownToHostOff(Adapter);
	//_DisableMAC_AFE_PLL(Adapter);
	
	_SetUsbSuspend(Adapter);
Exit:
#endif
	return rtStatus;
	
}

static int	
CardDisableWithoutHWSM( // without HW Auto state machine
	IN	PADAPTER		Adapter	
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	int		rtStatus = _SUCCESS;
	u8		value, u1bTmp;

	if(Adapter->bSurpriseRemoved){
		return rtStatus;
	}

	write8(Adapter, REG_TXPAUSE, 0xFF);

	//==== RF Off Sequence ====
#if (RTL8192D_EASY_SMART_CONCURRENT == 1)
	if(!Adapter->bSlaveOfDMSP)
#endif	
		_DisableRFAFEAndResetBB(Adapter);

#if 0
	if(pHalData->interfaceIndex == 0){
		u1bTmp = read8(Adapter, REG_MAC0);
		write8(Adapter, REG_MAC0, u1bTmp&(~MAC0_ON));
	}
	else{
		u1bTmp = read8(Adapter, REG_MAC1);
		write8(Adapter, REG_MAC1, u1bTmp&(~MAC1_ON));
	}
#endif
	// stop tx/rx 
	write8(Adapter, REG_TXPAUSE, 0xFF);
	udelay_os(500); 
	write8(Adapter,	REG_CR, 0x0);

	if(!PHY_CheckPowerOffFor8192D(Adapter))
	{
		return rtStatus;
	}

	if(!IS_NORMAL_CHIP(pHalData->VersionID))
	{			
		value = 0 ; 
		value |=( FEN_USBD | FEN_USBA | FEN_BB_GLB_RSTn);
		write8(Adapter, REG_SYS_FUNC_EN,value);//0x16		
	}
	//0x20:value 05-->04
	write8(Adapter, REG_LDOA15_CTRL,0x04);
      	//RF Control 
	write8(Adapter, REG_RF_CTRL,0);

	//  ==== Reset digital sequence   ======
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	_ResetDigitalProcedure1(Adapter, _FALSE);
#else
	_ResetDigitalProcedure1(Adapter,_TRUE);
#endif

	//  ==== Pull GPIO PIN to balance level and LED control ======
	_DisableGPIO(Adapter);

	//  ==== Reset digital sequence   ======
	_ResetDigitalProcedure2(Adapter);

	//  ==== Disable analog sequence ===
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	_DisableAnalog(Adapter,_FALSE);
#else
	_DisableAnalog(Adapter,_TRUE);
#endif

	ACQUIRE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);
	value=read8(Adapter, REG_POWER_OFF_IN_PROCESS);
	value&=(~BIT7);
	write8(Adapter, REG_POWER_OFF_IN_PROCESS, value);
	RELEASE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("<====== Card Disable Without HWSM .\n"));
	return rtStatus;
}


u32 rtl8192du_hal_deinit(_adapter *padapter)
 {
	u8	u1bTmp;
	u8	OpMode;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;

_func_enter_;

	if(RT_IN_PS_LEVEL(pwrpriv, RT_RF_OFF_LEVL_HALT_NIC))
	{
		DBG_8192C("HaltAdapter8192DUsb(): Not to haltadapter if HW already halt\n");
		return _FAIL;
	}

	OpMode = 0;
	padapter->HalFunc.SetHwRegHandler(padapter, HW_VAR_MEDIA_STATUS, (u8 *)(&OpMode));

#if 0
	if(pHalData->interfaceIndex == 0){
		u1bTmp = read8(padapter, REG_MAC0);
		write8(padapter, REG_MAC0, u1bTmp&(~MAC0_ON));
	}
	else{
		u1bTmp = read8(padapter, REG_MAC1);
		write8(padapter, REG_MAC1, u1bTmp&(~MAC1_ON));
	}
#endif

	pHalData->bCardDisableHWSM = !padapter->bCardDisableWOHSM;

	if(/*Adapter->bInUsbIfTest ||*/ !pHalData->bSupportRemoteWakeUp){
		if(pHalData->bCardDisableHWSM)
			CardDisableHWSM(padapter, _FALSE);
		else
			CardDisableWithoutHWSM(padapter);
		pHalData->bCardDisableHWSM = _TRUE;
	}
	else{

		// Wake on WLAN
	}

	if(pHalData->bInSetPower)
	{
		//0xFE10[4] clear before suspend	 suggested by zhouzhou
		u1bTmp=read8(padapter,0xfe10);
		u1bTmp&=(~BIT4);
		write8(padapter,0xfe10,u1bTmp);
	}

	RT_SET_PS_LEVEL(pwrpriv, RT_RF_OFF_LEVL_HALT_NIC);

_func_exit_;
	
	return _SUCCESS;
 }


unsigned int rtl8192du_inirp_init(_adapter * padapter)
{	
	u8 i;	
	struct recv_buf *precvbuf;
	uint	status;
	struct dvobj_priv *pdev=&padapter->dvobjpriv;
	struct intf_hdl * pintfhdl=&padapter->iopriv.intf;
	struct recv_priv *precvpriv = &(padapter->recvpriv);
	u32 (*_read_port)(struct intf_hdl *pintfhdl, u32 addr, u32 cnt, u8 *pmem);
#ifdef CONFIG_USB_INTERRUPT_IN_PIPE
	u32 (*_read_interrupt)(struct intf_hdl *pintfhdl, u32 addr);
#endif

_func_enter_;

	_read_port = pintfhdl->io_ops._read_port;

	status = _SUCCESS;

	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("===> usb_inirp_init \n"));	
		
	precvpriv->ff_hwaddr = RECV_BULK_IN_ADDR;

	//issue Rx irp to receive data	
	precvbuf = (struct recv_buf *)precvpriv->precv_buf;	
	for(i=0; i<NR_RECVBUFF; i++)
	{
		if(_read_port(pintfhdl, precvpriv->ff_hwaddr, 0, (unsigned char *)precvbuf) == _FALSE )
		{
			RT_TRACE(_module_hci_hal_init_c_,_drv_err_,("usb_rx_init: usb_read_port error \n"));
			status = _FAIL;
			goto exit;
		}
		
		precvbuf++;		
		precvpriv->free_recv_buf_queue_cnt--;
	}

#ifdef CONFIG_USB_INTERRUPT_IN_PIPE
	_read_interrupt = pintfhdl->io_ops._read_interrupt;
	if(_read_interrupt(pintfhdl, RECV_INT_IN_ADDR) == _FALSE )
	{
		RT_TRACE(_module_hci_hal_init_c_,_drv_err_,("usb_rx_init: usb_read_interrupt error \n"));
		status = _FAIL;
	}
#endif

exit:
	
	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("<=== usb_inirp_init \n"));

_func_exit_;

	return status;

}

unsigned int rtl8192du_inirp_deinit(_adapter * padapter)
{	
	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("\n ===> usb_rx_deinit \n"));
	
	read_port_cancel(padapter);


	RT_TRACE(_module_hci_hal_init_c_,_drv_info_,("\n <=== usb_rx_deinit \n"));

	return _SUCCESS;
}

//-------------------------------------------------------------------
//
//	EEPROM/EFUSE Content Parsing
//
//-------------------------------------------------------------------

//-------------------------------------------------------------------------
//
//	Channel Plan
//
//-------------------------------------------------------------------------

static RT_CHANNEL_DOMAIN
_HalMapChannelPlan8192D(
	IN	PADAPTER	Adapter,
	IN	u8		HalChannelPlan
	)
{
	RT_CHANNEL_DOMAIN	rtChannelDomain;

	switch(HalChannelPlan)
	{
		case EEPROM_CHANNEL_PLAN_GLOBAL_DOMAIN:
			rtChannelDomain = RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN;
			break;
		case EEPROM_CHANNEL_PLAN_WORLD_WIDE_13:
			rtChannelDomain = RT_CHANNEL_DOMAIN_WORLD_WIDE_13;
			break;			
		default:
			rtChannelDomain = (RT_CHANNEL_DOMAIN)HalChannelPlan;
			break;
	}
	
	return 	rtChannelDomain;

}

static VOID
ReadChannelPlan(
	IN	PADAPTER 		Adapter,
	IN	u8*			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	)
{
	struct mlme_priv	*pmlmepriv = &(Adapter->mlmepriv);
	struct registry_priv *pregistrypriv = &Adapter->registrypriv;
	u8			channelPlan;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(AutoLoadFail){
		channelPlan = CHPL_FCC;
	}
	else{
		channelPlan = PROMContent[EEPROM_CHANNEL_PLAN];
	}

	if((pregistrypriv->channel_plan>= RT_CHANNEL_DOMAIN_MAX) || (channelPlan & EEPROM_CHANNEL_PLAN_BY_HW_MASK))
	{
		pmlmepriv->ChannelPlan = _HalMapChannelPlan8192D(Adapter, (channelPlan & (~(EEPROM_CHANNEL_PLAN_BY_HW_MASK))));
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
	
			pDot11dInfo->bEnabled = _TRUE;
		}
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("Enable dot11d when RT_CHANNEL_DOMAIN_GLOBAL_DOAMIN!\n"));
		break;
	}
#endif

	switch(pHalData->BandSet92D)
	{
		case BAND_ON_2_4G:
			pmlmepriv->ChannelPlan = RT_CHANNEL_DOMAIN_TELEC;
			break;
		case BAND_ON_5G:
			pmlmepriv->ChannelPlan = RT_CHANNEL_DOMAIN_5G;
			break;
		case BAND_ON_BOTH:
			pmlmepriv->ChannelPlan = RT_CHANNEL_DOMAIN_FCC;
			break;
		default:
			break;
	}

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	pmlmepriv->ChannelPlan = (RT_CHANNEL_DOMAIN)RT_CHANNEL_DOMAIN_FCC;
#endif

	//RT_TRACE(COMP_INIT, DBG_LOUD, ("RegChannelPlan(%d) EEPROMChannelPlan(%ld)", pMgntInfo->RegChannelPlan, (u4Byte)channelPlan));
	MSG_8192C("ChannelPlan = %d\n" , pmlmepriv->ChannelPlan);

}

static void
_ReadPROMVersion(
	IN	PADAPTER	Adapter,	
	IN	u8* 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	if(AutoloadFail){
		pHalData->EEPROMVersion = EEPROM_Default_Version;		
	}
	else{
		pHalData->EEPROMVersion = *(u8 *)&PROMContent[EEPROM_VERSION];
	}
}

//-------------------------------------------------------------------------
//
//	EEPROM Power index mapping
//
//-------------------------------------------------------------------------

static VOID
_ReadPowerValueFromPROM(
	IN	PTxPowerInfo	pwrInfo,
	IN	u8*			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	)
{
	u32	rfPath, eeAddr, group, offset1,offset2=0;
	u8	i = 0;

	_memset(pwrInfo, 0, sizeof(TxPowerInfo));

	if(AutoLoadFail){		
		for(group = 0 ; group < CHANNEL_GROUP_MAX ; group++){
			for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
#if(TX_POWER_FOR_5G_BAND == 1)				
				if(group< CHANNEL_GROUP_MAX_2G)
#endif					
					pwrInfo->CCKIndex[rfPath][group]		= EEPROM_Default_TxPowerLevel_2G;	
				pwrInfo->HT40_1SIndex[rfPath][group]		= EEPROM_Default_TxPowerLevel_2G;
				pwrInfo->HT40_2SIndexDiff[rfPath][group]	= EEPROM_Default_HT40_2SDiff;
				pwrInfo->HT20IndexDiff[rfPath][group]		= EEPROM_Default_HT20_Diff;
				pwrInfo->OFDMIndexDiff[rfPath][group]	= EEPROM_Default_LegacyHTTxPowerDiff;
				pwrInfo->HT40MaxOffset[rfPath][group]	= EEPROM_Default_HT40_PwrMaxOffset;		
				pwrInfo->HT20MaxOffset[rfPath][group]	= EEPROM_Default_HT20_PwrMaxOffset;
			}
		}

		for(i = 0; i < 3; i++)
		{
			pwrInfo->TSSI_A[i] = EEPROM_Default_TSSI;
			pwrInfo->TSSI_B[i] = EEPROM_Default_TSSI;
		}
		return;
	}

	//Maybe autoload OK,buf the tx power index vlaue is not filled.
	//If we find it,we set it default value.
	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(group = 0 ; group < CHANNEL_GROUP_MAX_2G; group++){
			eeAddr = EEPROM_CCK_TX_PWR_INX_2G + (rfPath * 3) + group;
			pwrInfo->CCKIndex[rfPath][group] = 
				(PROMContent[eeAddr] == 0xFF)?(eeAddr>0x7B?EEPROM_Default_TxPowerLevel_5G:EEPROM_Default_TxPowerLevel_2G):PROMContent[eeAddr];
		}
	}
	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(group = 0 ; group < CHANNEL_GROUP_MAX ; group++){
			offset1 = group / 3;
			offset2 = group % 3;
			eeAddr = EEPROM_HT40_1S_TX_PWR_INX_2G+ (rfPath * 3) + offset2 + offset1*21;
			pwrInfo->HT40_1SIndex[rfPath][group] = 
				(PROMContent[eeAddr] == 0xFF)?(eeAddr>0x7B?EEPROM_Default_TxPowerLevel_5G:EEPROM_Default_TxPowerLevel_2G):PROMContent[eeAddr];
		}
	}

	//These just for 92D efuse offset.
	for(group = 0 ; group < CHANNEL_GROUP_MAX ; group++){
		for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
			offset1 = group / 3;
			offset2 = group % 3;

			if(PROMContent[EEPROM_HT40_2S_TX_PWR_INX_DIFF_2G+ offset2 + offset1*21] != 0xFF)
				pwrInfo->HT40_2SIndexDiff[rfPath][group] = 
					(PROMContent[EEPROM_HT40_2S_TX_PWR_INX_DIFF_2G+ offset2 + offset1*21] >> (rfPath * 4)) & 0xF;
					//RT_TRACE(COMP_INIT,DBG_LOUD,
					//	("ht40_2sdiff:group:%d,%x:0x%x.\n",group,EEPROM_HT40_2S_TX_PWR_INX_DIFF_2G+ offset2 + offset1*21,PROMContent[EEPROM_HT40_2S_TX_PWR_INX_DIFF_2G+ offset2 + offset1*21]));
			else
				pwrInfo->HT40_2SIndexDiff[rfPath][group]	= EEPROM_Default_HT40_2SDiff;

			if(PROMContent[EEPROM_HT20_TX_PWR_INX_DIFF_2G + offset2 + offset1*21] != 0xFF)
				pwrInfo->HT20IndexDiff[rfPath][group] =
					(PROMContent[EEPROM_HT20_TX_PWR_INX_DIFF_2G + offset2 + offset1*21] >> (rfPath * 4)) & 0xF;
			else
				pwrInfo->HT20IndexDiff[rfPath][group]		= EEPROM_Default_HT20_Diff;

			if(PROMContent[EEPROM_OFDM_TX_PWR_INX_DIFF_2G + offset2 + offset1*21] != 0xFF)
				pwrInfo->OFDMIndexDiff[rfPath][group] =
					(PROMContent[EEPROM_OFDM_TX_PWR_INX_DIFF_2G + offset2 + offset1*21] >> (rfPath * 4)) & 0xF;
			else
				pwrInfo->OFDMIndexDiff[rfPath][group]	= EEPROM_Default_LegacyHTTxPowerDiff;

			if(PROMContent[EEPROM_HT40_MAX_PWR_OFFSET_2G + offset2 + offset1*21] != 0xFF)
				pwrInfo->HT40MaxOffset[rfPath][group] =
					(PROMContent[EEPROM_HT40_MAX_PWR_OFFSET_2G + offset2 + offset1*21] >> (rfPath * 4)) & 0xF;
			else
				pwrInfo->HT40MaxOffset[rfPath][group]	= EEPROM_Default_HT40_PwrMaxOffset;

			if(PROMContent[EEPROM_HT20_MAX_PWR_OFFSET_2G + offset2 + offset1*21] != 0xFF)
				pwrInfo->HT20MaxOffset[rfPath][group] =
					(PROMContent[EEPROM_HT20_MAX_PWR_OFFSET_2G + offset2 + offset1*21] >> (rfPath * 4)) & 0xF;
			else	
				pwrInfo->HT20MaxOffset[rfPath][group]	= EEPROM_Default_HT20_PwrMaxOffset;			
			
		}
	}

	if(PROMContent[EEPROM_TSSI_A_5G] != 0xFF){
		//5GL
		pwrInfo->TSSI_A[0] = PROMContent[EEPROM_TSSI_A_5G] & 0x3F;	//[0:5]
		pwrInfo->TSSI_B[0] = PROMContent[EEPROM_TSSI_B_5G] & 0x3F;

		//5GM
		pwrInfo->TSSI_A[1] = PROMContent[EEPROM_TSSI_AB_5G] & 0x3F;
		pwrInfo->TSSI_B[1] = (PROMContent[EEPROM_TSSI_AB_5G] & 0xC0) >> 6 | 
							(PROMContent[EEPROM_TSSI_AB_5G+1] & 0x0F) << 2;

		//5GH
		pwrInfo->TSSI_A[2] = (PROMContent[EEPROM_TSSI_AB_5G+1] & 0xF0) >> 4 | 
							(PROMContent[EEPROM_TSSI_AB_5G+2] & 0x03) << 4;
		pwrInfo->TSSI_B[2] = (PROMContent[EEPROM_TSSI_AB_5G+2] & 0xFC) >> 2 ;
	}
	else
	{
		for(i = 0; i < 3; i++)
		{
			pwrInfo->TSSI_A[i] = EEPROM_Default_TSSI;
			pwrInfo->TSSI_B[i] = EEPROM_Default_TSSI;
		}
	}

}


u32
_GetChannelGroup(
	IN	u32	channel
	)
{
	//RT_ASSERT((channel < 14), ("Channel %d no is supported!\n"));

	if(channel < 3){ 	// Channel 1~3
		return 0;
	}
	else if(channel < 9){ // Channel 4~9
		return 1;
	}

	return 2;				// Channel 10~14	
}

static VOID
ReadTxPowerInfo(
	IN	PADAPTER 		Adapter,
	IN	u8*			PROMContent,
	IN	BOOLEAN			AutoLoadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	TxPowerInfo	pwrInfo;
	u32			rfPath, ch, group;
	u8			pwr, diff,tempval[2], i;

	_ReadPowerValueFromPROM(&pwrInfo, PROMContent, AutoLoadFail);

	if(!AutoLoadFail)
	{
		pHalData->EEPROMRegulatory = (PROMContent[EEPROM_RF_OPT1]&0x7);	//bit0~2
		pHalData->EEPROMThermalMeter = PROMContent[EEPROM_THERMAL_METER]&0x1f;
		pHalData->CrystalCap = PROMContent[EEPROM_XTAL_K];
		tempval[0] = PROMContent[EEPROM_IQK_DELTA]&0x03;
		tempval[1] = (PROMContent[EEPROM_LCK_DELTA]&0x0C) >> 2;
		pHalData->bTXPowerDataReadFromEEPORM = _TRUE;
	}
	else
	{
		pHalData->EEPROMRegulatory = 0;
		pHalData->EEPROMThermalMeter = EEPROM_Default_ThermalMeter;
		pHalData->CrystalCap = EEPROM_Default_CrystalCap;		
		//tempval[0] = tempval[1] = 3;
	}

	//Use default value to fill parameters if efuse is not filled on some place.	

	// ThermalMeter from EEPROM
	if(pHalData->EEPROMThermalMeter < 0x06 || pHalData->EEPROMThermalMeter > 0x1c)
		pHalData->EEPROMThermalMeter = 0x12;
	
	pdmpriv->ThermalMeter[0] = pHalData->EEPROMThermalMeter;	

	//check XTAL_K
	if(pHalData->CrystalCap == 0xFF)
		pHalData->CrystalCap = 0;

	if(pHalData->EEPROMRegulatory >3)
		pHalData->EEPROMRegulatory = 0;

	for(i = 0; i < 2; i++)
	{
		switch(tempval[i])
		{
			case 0: 
				tempval[i] = 5;
				break;
				
			case 1: 
				tempval[i] = 3;				
				break;
				
			case 2:
				tempval[i] = 2;				
				break;
				
			case 3:
			default:				
				tempval[i] = 0;
				break;			
		}
	}

#if 0
	pdmpriv->Delta_IQK = tempval[0];
	pdmpriv->Delta_LCK = tempval[1];	
#else
	//temporarily close 92D re-IQK&LCK by thermal meter,advised by allen.2010-11-03
	pdmpriv->Delta_IQK = 0;
	pdmpriv->Delta_LCK = 0;
#endif
	//RTPRINT(FINIT, INIT_TxPower, ("EEPROMRegulatory = 0x%x\n", pHalData->EEPROMRegulatory));		
	//RTPRINT(FINIT, INIT_TxPower, ("ThermalMeter = 0x%x\n", pHalData->EEPROMThermalMeter));	
	//RTPRINT(FINIT, INIT_TxPower, ("CrystalCap = 0x%x\n", pHalData->CrystalCap));	
	//RTPRINT(FINIT, INIT_TxPower, ("Delta_IQK = 0x%x Delta_LCK = 0x%x\n", pHalData->Delta_IQK, pHalData->Delta_LCK));	
	
	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
			group = rtl8192d_getChnlGroupfromArray((u8)ch);

			if(ch < CHANNEL_MAX_NUMBER_2G)
				pHalData->TxPwrLevelCck[rfPath][ch]		= pwrInfo.CCKIndex[rfPath][group];
			pHalData->TxPwrLevelHT40_1S[rfPath][ch]	= pwrInfo.HT40_1SIndex[rfPath][group];

			pHalData->TxPwrHt20Diff[rfPath][ch]		= pwrInfo.HT20IndexDiff[rfPath][group];
			pHalData->TxPwrLegacyHtDiff[rfPath][ch]	= pwrInfo.OFDMIndexDiff[rfPath][group];
			pHalData->PwrGroupHT20[rfPath][ch]		= pwrInfo.HT20MaxOffset[rfPath][group];
			pHalData->PwrGroupHT40[rfPath][ch]		= pwrInfo.HT40MaxOffset[rfPath][group];

			pwr		= pwrInfo.HT40_1SIndex[rfPath][group];
			diff	= pwrInfo.HT40_2SIndexDiff[rfPath][group];

			pHalData->TxPwrLevelHT40_2S[rfPath][ch]  = (pwr > diff) ? (pwr - diff) : 0;
		}
	}

#if DBG

	for(rfPath = 0 ; rfPath < RF90_PATH_MAX ; rfPath++){
		for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
			if(ch < CHANNEL_MAX_NUMBER_2G)
			{
				RTPRINT(FINIT, INIT_TxPower, 
					("RF(%ld)-Ch(%ld) [CCK / HT40_1S / HT40_2S] = [0x%x / 0x%x / 0x%x]\n", 
					rfPath, ch, 
					pHalData->TxPwrLevelCck[rfPath][ch], 
					pHalData->TxPwrLevelHT40_1S[rfPath][ch], 
					pHalData->TxPwrLevelHT40_2S[rfPath][ch]));
			}
			else
			{
				RTPRINT(FINIT, INIT_TxPower, 
					("RF(%ld)-Ch(%ld) [HT40_1S / HT40_2S] = [0x%x / 0x%x]\n", 
					rfPath, ch, 
					pHalData->TxPwrLevelHT40_1S[rfPath][ch], 
					pHalData->TxPwrLevelHT40_2S[rfPath][ch]));				
			}
		}
	}

	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-A Ht20 to HT40 Diff[%ld] = 0x%x\n", ch, pHalData->TxPwrHt20Diff[RF90_PATH_A][ch]));
	}

	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-A Legacy to Ht40 Diff[%ld] = 0x%x\n", ch, pHalData->TxPwrLegacyHtDiff[RF90_PATH_A][ch]));
	}
	
	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-B Ht20 to HT40 Diff[%ld] = 0x%x\n", ch, pHalData->TxPwrHt20Diff[RF90_PATH_B][ch]));
	}
	
	for(ch = 0 ; ch < CHANNEL_MAX_NUMBER ; ch++){
		RTPRINT(FINIT, INIT_TxPower, ("RF-B Legacy to HT40 Diff[%ld] = 0x%x\n", ch, pHalData->TxPwrLegacyHtDiff[RF90_PATH_B][ch]));
	}
	
#endif

}

static void
_ReadIDs(
	IN	PADAPTER	Adapter,
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);


	if(_FALSE == AutoloadFail){
		// VID, PID 
		pHalData->EEPROMVID = le16_to_cpu( *(u16 *)&PROMContent[EEPROM_VID]);
		pHalData->EEPROMPID = le16_to_cpu( *(u16 *)&PROMContent[EEPROM_PID]);
		
		// Customer ID, 0x00 and 0xff are reserved for Realtek. 		
		pHalData->EEPROMCustomerID = *(u8 *)&PROMContent[EEPROM_CUSTOMER_ID];
		pHalData->EEPROMSubCustomerID = *(u8 *)&PROMContent[EEPROM_SUBCUSTOMER_ID];

	}
	else{
		pHalData->EEPROMVID	 = EEPROM_Default_VID;
		pHalData->EEPROMPID	 = EEPROM_Default_PID;

		// Customer ID, 0x00 and 0xff are reserved for Realtek. 		
		pHalData->EEPROMCustomerID	= EEPROM_Default_CustomerID;
		pHalData->EEPROMSubCustomerID = EEPROM_Default_SubCustomerID;

	}

	//	Decide CustomerID according to VID/DID or EEPROM
	switch(pHalData->EEPROMCustomerID)
	{
		case EEPROM_CID_WHQL:
			//Adapter->bInHctTest = _TRUE;

			//pMgntInfo->bSupportTurboMode = _FALSE;
			//pMgntInfo->bAutoTurboBy8186 = _FALSE;

			//pMgntInfo->PowerSaveControl.bInactivePs = _FALSE;
			//pMgntInfo->PowerSaveControl.bIPSModeBackup = _FALSE;
			//pMgntInfo->PowerSaveControl.bLeisurePs = _FALSE;
			//pMgntInfo->keepAliveLevel = 0;

			//Adapter->bUnloadDriverwhenS3S4 = _FALSE;
			break;			
		default:
			pHalData->CustomerID = RT_CID_DEFAULT;
			break;
			
	}

	MSG_8192C("EEPROMVID = 0x%04x\n", pHalData->EEPROMVID);
	MSG_8192C("EEPROMPID = 0x%04x\n", pHalData->EEPROMPID);
	MSG_8192C("EEPROMCustomerID : 0x%02x\n", pHalData->EEPROMCustomerID);
	MSG_8192C("EEPROMSubCustomerID: 0x%02x\n", pHalData->EEPROMSubCustomerID);
}

static VOID
_ReadMACAddress(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	EEPROM_EFUSE_PRIV	*pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	u16	usValue=0;
	u8	i=0;

	// Dual MAC should assign diffrent MAC address ,or, it is wil cause hang in single phy mode  zhiyuan 04/07/2010
	//Temply random assigh mac address for  efuse mac address not ready now
	if(AutoloadFail == _FALSE  ){
		if(pHalData->interfaceIndex == 0){
			for(i = 0; i < 6; i += 2)
			{
				usValue = *(u16 *)&PROMContent[EEPROM_MAC_ADDR_MAC0_92D+i];
				*((u16 *)(&pEEPROM->mac_addr[i])) = usValue;
			}
		}
		else{
			for(i = 0; i < 6; i += 2)
			{
				usValue = *(u16 *)&PROMContent[EEPROM_MAC_ADDR_MAC1_92D+i];
				*((u16 *)(&pEEPROM->mac_addr[i])) = usValue;
			}
		}

		if(is_broadcast_mac_addr(pEEPROM->mac_addr) || is_multicast_mac_addr(pEEPROM->mac_addr))
		{
			//Random assigh MAC address
			u8 sMacAddr[MAC_ADDR_LEN] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
			//u32	curtime = get_current_time();
			if(pHalData->interfaceIndex == 1){
				sMacAddr[5] = 0x01;
				//sMacAddr[5] = (u8)(curtime & 0xff);
				//sMacAddr[5] = (u8)GetRandomNumber(1, 254);
			}
			_memcpy(pEEPROM->mac_addr, sMacAddr, ETH_ALEN);
		}
	}
	else
	{
		//Random assigh MAC address
		u8 sMacAddr[MAC_ADDR_LEN] = {0x00, 0xE0, 0x4C, 0x81, 0x92, 0x00};
		//u32	curtime = get_current_time();
		if(pHalData->interfaceIndex == 1){
			sMacAddr[5] = 0x01;
			//sMacAddr[5] = (u8)(curtime & 0xff);
			//sMacAddr[5] = (u8)GetRandomNumber(1, 254);
		}
		_memcpy(pEEPROM->mac_addr, sMacAddr, ETH_ALEN);
	}
	
	//NicIFSetMacAddress(Adapter, Adapter->PermanentAddress);
	//RT_PRINT_ADDR(COMP_INIT|COMP_EFUSE, DBG_LOUD, "MAC Addr: %s", Adapter->PermanentAddress);

}


//
//	Description:
//		Read HW adapter information through EEPROM 93C46.
//		Or For EFUSE 92S .And Get and Set 92D MACPHY mode and Band Type.
//		MacPhyMode:DMDP,SMSP.
//		BandType:2.4G,5G.
//
//	Assumption:
//		1. Boot from EEPROM and CR9346 regiser has verified.
//		2. PASSIVE_LEVEL (USB interface)
//
VOID PHY_ReadMacPhyMode92D(
		IN PADAPTER			Adapter,
		IN	BOOLEAN		AutoloadFail		
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			MacPhyCrValue = 0;

	if(AutoloadFail)
	{
		pHalData->MacPhyMode92D = SINGLEMAC_SINGLEPHY;
		return;	
	}

	if(IS_NORMAL_CHIP(pHalData->VersionID))				
		MacPhyCrValue = read8(Adapter, REG_MAC_PHY_CTRL_NORMAL);
	else
		MacPhyCrValue = read8(Adapter, REG_MAC_PHY_CTRL);					
	DBG_8192C("PHY_ReadMacPhyMode92D():   MAC_PHY_CTRL Value %x \n",MacPhyCrValue);
	
	if((MacPhyCrValue&0x03) == 0x03)
	{
		pHalData->MacPhyMode92D = DUALMAC_DUALPHY;
	}
	else if((MacPhyCrValue&0x03) == 0x01)
	{
		pHalData->MacPhyMode92D = DUALMAC_SINGLEPHY;
	}
	else
	{
		pHalData->MacPhyMode92D = SINGLEMAC_SINGLEPHY;
	}		
}

//
//	Description:
//		Read HW adapter information through EEPROM 93C46.
//		Or For EFUSE 92S .And Get and Set 92D MACPHY mode and Band Type.
//		MacPhyMode:DMDP,SMSP.
//		BandType:2.4G,5G.
//
//	Assumption:
//		1. Boot from EEPROM and CR9346 regiser has verified.
//		2. PASSIVE_LEVEL (USB interface)
//
VOID PHY_ConfigMacPhyMode92D(
		IN PADAPTER			Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	offset = IS_NORMAL_CHIP(pHalData->VersionID)?REG_MAC_PHY_CTRL_NORMAL:REG_MAC_PHY_CTRL;
			
	switch(pHalData->MacPhyMode92D){
		case DUALMAC_DUALPHY:
			DBG_8192C("MacPhyMode: DUALMAC_DUALPHY \n");
			write8(Adapter, offset, 0xF3);
			break;
		case SINGLEMAC_SINGLEPHY:
			DBG_8192C("MacPhyMode: SINGLEMAC_SINGLEPHY \n");
			write8(Adapter, offset, 0xF4);
			break;
		case DUALMAC_SINGLEPHY:
			DBG_8192C("MacPhyMode: DUALMAC_SINGLEPHY \n");
			write8(Adapter, offset, 0xF1);
			break;
	}		
}

//
//	Description:
//		Read HW adapter information through EEPROM 93C46.
//		Or For EFUSE 92S .And Get and Set 92D MACPHY mode and Band Type.
//		MacPhyMode:DMDP,SMSP.
//		BandType:2.4G,5G.
//
//	Assumption:
//		1. Boot from EEPROM and CR9346 regiser has verified.
//		2. PASSIVE_LEVEL (USB interface)
//
VOID PHY_ConfigMacPhyModeInfo92D(
		IN PADAPTER			Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	PADAPTER		BuddyAdapter = Adapter->BuddyAdapter;
	u8				MacPhyCrValue = 0;
	HAL_DATA_TYPE	*pHalDataBuddyAdapter;
	int				i;
#endif

	switch(pHalData->MacPhyMode92D){
		case DUALMAC_SINGLEPHY:
			pHalData->rf_type = RF_2T2R;
			pHalData->VersionID |= CHIP_92D_SINGLEPHY;
			pHalData->BandSet92D = BAND_ON_BOTH;
			pHalData->CurrentBandType92D = BAND_ON_2_4G;
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
//get bMasetOfDMSP and bSlaveOfDMSP sync with buddy adapter
			ACQUIRE_GLOBAL_MUTEX(GlobalCounterForMutex);
			if(BuddyAdapter != NULL)
			{
				Adapter->bMasterOfDMSP = !BuddyAdapter->bMasterOfDMSP;
				Adapter->bSlaveOfDMSP = !BuddyAdapter->bSlaveOfDMSP;
				pHalDataBuddyAdapter = GET_HAL_DATA(BuddyAdapter);
				pHalData->CurrentBandType92D = pHalDataBuddyAdapter->CurrentBandType92D;				
			}
			else
			{
				if(pHalData->interfaceIndex == 0)
				{
					Adapter->bMasterOfDMSP = TRUE;
					Adapter->bSlaveOfDMSP = FALSE;
				}
				else if(pHalData->interfaceIndex == 1)
				{
					Adapter->bMasterOfDMSP = FALSE;
					Adapter->bSlaveOfDMSP = TRUE;
				}
			}
			RELEASE_GLOBAL_MUTEX(GlobalCounterForMutex);

#endif
			break;

		case SINGLEMAC_SINGLEPHY:
			pHalData->rf_type = RF_2T2R;
			pHalData->VersionID |= CHIP_92D_SINGLEPHY;
			pHalData->BandSet92D = BAND_ON_BOTH;
			pHalData->CurrentBandType92D = BAND_ON_2_4G;
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
			Adapter->bMasterOfDMSP = _FALSE;
			Adapter->bSlaveOfDMSP = _FALSE;
#endif
			break;

		case DUALMAC_DUALPHY:
			pHalData->rf_type = RF_1T1R;
			pHalData->VersionID &= (~CHIP_92D_SINGLEPHY);
			if(pHalData->interfaceIndex == 0){
				pHalData->BandSet92D = BAND_ON_5G;
				pHalData->CurrentBandType92D = BAND_ON_5G;//Now we let MAC0 run on 5G band.
			}
			else{
				pHalData->BandSet92D = BAND_ON_2_4G;
				pHalData->CurrentBandType92D = BAND_ON_2_4G;//
			}
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
			Adapter->bMasterOfDMSP = _FALSE;
			Adapter->bSlaveOfDMSP = _FALSE;
#endif
			break;

		default:
			break;	
	}

	/*if(pHalData->MacPhyMode92D == SINGLEMAC_SINGLEPHY)
	{
		pHalData->CurrentBandType92D=BAND_ON_2_4G;
		pHalData->BandSet92D = BAND_ON_2_4G;
	}*/

	if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
		pHalData->CurrentChannel = 6;
	else
		pHalData->CurrentChannel = 44;

	Adapter->registrypriv.channel = pHalData->CurrentChannel;

	switch(pHalData->BandSet92D)
	{
		case BAND_ON_2_4G:
			Adapter->registrypriv.wireless_mode = WIRELESS_11BG_24N;
			break;

		case BAND_ON_5G:
			Adapter->registrypriv.wireless_mode = WIRELESS_11A_5N;
			break;

		case BAND_ON_BOTH:
			Adapter->registrypriv.wireless_mode = WIRELESS_11ABGN;
			break;

		default:
			Adapter->registrypriv.wireless_mode = WIRELESS_11ABGN;
			break;
	}
	DBG_8192C("%s(): wireless_mode = %x\n",__FUNCTION__,Adapter->registrypriv.wireless_mode);
}

VOID
_ReadMacPhyModeFromPROM92DU(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	BOOLEAN			isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	u8				MacPhyCrValue = 0;
#if (RTL8192D_DUAL_MAC_MODE_SWITCH)
	PHAL_DATA_TYPE  	pHalDataBuddyAdapter;
	PADAPTER		BuddyAdapter = Adapter->BuddyAdapter;
	BOOLEAN			bMac1Enable = _FALSE;
	int				i;
#endif

	
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		MacPhyCrValue=PROMContent[EEPROM_ENDPOINT_SETTING];
		if(MacPhyCrValue & BIT0)
			pHalData->MacPhyMode92D = DUALMAC_DUALPHY;
		else				
			pHalData->MacPhyMode92D = SINGLEMAC_SINGLEPHY;
	}
	else
	{
		MacPhyCrValue=PROMContent[EEPROM_MAC_FUNCTION];
		if((MacPhyCrValue&0x03) == 0x03)
		{
			pHalData->MacPhyMode92D = DUALMAC_DUALPHY;
		}
		else if((MacPhyCrValue&0x03) == 0x01)
		{
			pHalData->MacPhyMode92D = DUALMAC_SINGLEPHY;
		}
		else
		{
			pHalData->MacPhyMode92D = SINGLEMAC_SINGLEPHY;
		}			
	}
	DBG_8192C("_ReadMacPhyModeFromPROM92DU(): MacPhyCrValue %d \n", MacPhyCrValue);
	
}

//
//	Description:
//		Reset Dual Mac Mode Switch related settings
//
//	Assumption:
//
VOID ResetDualMacSwitchVariables(
		IN PADAPTER			Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	PADAPTER		BuddyAdapter = Adapter->BuddyAdapter;
	u8				MacPhyCrValue = 0;
	HAL_DATA_TYPE	*pHalDataBuddyAdapter;
	int				i;
#endif

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
		Adapter->bNeedReConfigMac = _FALSE;
		Adapter->bNeedReConfigPhyRf = _FALSE;
		Adapter->bNeedTurnOffAdapterInModeSwitchProcess = _FALSE;
		Adapter->bNeedRecoveryAfterModeSwitch = _FALSE;
		Adapter->bInModeSwitchProcess = _FALSE;
		Adapter->bDoTurnOffPhyRf  = _FALSE;
		
		if(BuddyAdapter != NULL)
		{
			Adapter->PreChangeAction = BuddyAdapter->PreChangeAction;
		}
		else
		{
			Adapter->PreChangeAction = MAXACTION;
		}

//set dual mac role to set
		Adapter->DualMacRoleToSet.BandType = pHalData->CurrentBandType92D;
		Adapter->DualMacRoleToSet.BandSet = pHalData->BandSet92D;
		Adapter->DualMacRoleToSet.RFType = pHalData->RF_Type;
		Adapter->DualMacRoleToSet.macPhyMode = pHalData->MacPhyMode92D ;
		Adapter->DualMacRoleToSet.bMasterOfDMSP = Adapter->bMasterOfDMSP;
		Adapter->DualMacRoleToSet.bSlaveOfDMSP = Adapter->bSlaveOfDMSP;
#endif

}

static VOID
_ReadMacPhyMode(
	IN	PADAPTER	Adapter,	
	IN	u8			*PROMContent,
	IN	BOOLEAN		AutoloadFail
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	//u8			MacPhyMode,Dual_Phymode,Dual_Macmode;
	BOOLEAN		isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	u8			MacPhyCrValue = 0, Mac1EnableValue = 0;
#if (RTL8192D_DUAL_MAC_MODE_SWITCH)
	PHAL_DATA_TYPE  	pHalDataBuddyAdapter;
	PADAPTER		BuddyAdapter = Adapter->BuddyAdapter;
	BOOLEAN			bMac1Enable = _FALSE;
	int				i;
#endif

	
	if(AutoloadFail==_TRUE){
		Mac1EnableValue = read8(Adapter,0xFE64);
		PHY_ReadMacPhyMode92D(Adapter, AutoloadFail);

		DBG_8192C("_ReadMacPhyMode(): AutoloadFail %d 0xFE64 = 0x%x \n",AutoloadFail, Mac1EnableValue);
	}
	else{
		_ReadMacPhyModeFromPROM92DU(Adapter, PROMContent);
	}

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(IS_HARDWARE_TYPE_8192DU(Adapter))
	{
//SMSP-->DMDP/DMSP wait for another adapter compeletes mode switc
		CheckInModeSwitchProcess(Adapter);

//get Dual Mac Mode from 0x2C for test chip and 0xF8 for normal chip
		if(IS_NORMAL_CHIP(pHalData->VersionID))
		{
			ACQUIRE_GLOBAL_MUTEX(GlobalCounterForMutex);
			if(GlobalFirstConfigurationForNormalChip)
			{
				RELEASE_GLOBAL_MUTEX(GlobalCounterForMutex);			
				PHY_ConfigMacPhyMode92D(Adapter);
				ACQUIRE_GLOBAL_MUTEX(GlobalCounterForMutex);
				GlobalFirstConfigurationForNormalChip = FALSE;
				RELEASE_GLOBAL_MUTEX(GlobalCounterForMutex);				
			}
			else
			{
				RELEASE_GLOBAL_MUTEX(GlobalCounterForMutex);			
				PHY_ReadMacPhyMode92D(Adapter, AutoloadFail);
			}
		}
		else
		{			
			PHY_ReadMacPhyMode92D(Adapter, AutoloadFail);
		}
	}
#else
	PHY_ConfigMacPhyMode92D(Adapter);      
#endif

	PHY_ConfigMacPhyModeInfo92D(Adapter);
	ResetDualMacSwitchVariables(Adapter);

}

static VOID
_ReadBoardType(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			boardType;

	if(AutoloadFail){
		pHalData->rf_type = RF_2T2R;
		pHalData->BluetoothCoexist = _FALSE;
		return;
	}

	boardType = PROMContent[EEPROM_NORMAL_BoardType];
	boardType &= BOARD_TYPE_NORMAL_MASK;
	boardType >>= 5;

#if 0
	switch(boardType & 0xF)
	{
		case 0:
		case 1:
		case 2:
		case 3:
		case 4:
			pHalData->rf_type = RF_2T2R;
			break;
		case 5:
			pHalData->rf_type = RF_1T2R;
			break;
		default:
			pHalData->rf_type = RF_1T1R;
			break;
	}

	pHalData->BluetoothCoexist = (boardType >> 4) ? _TRUE : _FALSE;
#endif
}


static VOID
_ReadLEDSetting(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct led_priv 	*pledpriv = &(Adapter->ledpriv);

	// Led mode
	switch(pHalData->CustomerID)
	{
		case RT_CID_DEFAULT:
			pledpriv->LedStrategy = SW_LED_MODE1;
			pledpriv->bRegUseLed = _TRUE;			
		break;
		default:
			pledpriv->LedStrategy = SW_LED_MODE0;
		break;			
	}

}

static VOID
_ReadThermalMeter(
	IN	PADAPTER	Adapter,	
	IN	u8* 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u8	tempval;

	//
	// ThermalMeter from EEPROM
	//
	if(!AutoloadFail)	
		tempval = PROMContent[EEPROM_THERMAL_METER];
	else
		tempval = EEPROM_Default_ThermalMeter;
	pHalData->EEPROMThermalMeter = (tempval&0x1f);	//[4:0]

	if(pHalData->EEPROMThermalMeter < 0x06 || pHalData->EEPROMThermalMeter > 0x1c)
		pHalData->EEPROMThermalMeter = 0x12;
	
	pdmpriv->ThermalMeter[0] = pHalData->EEPROMThermalMeter;
	//RTPRINT(FINIT, INIT_TxPower, ("ThermalMeter = 0x%x\n", pHalData->EEPROMThermalMeter));
	
}

static VOID
_ReadRFSetting(
	IN	PADAPTER	Adapter,	
	IN	u8* 	PROMContent,
	IN	BOOLEAN 	AutoloadFail
	)
{
}

static void _InitAdapterVariablesByPROM(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	unsigned char AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	_ReadPROMVersion(Adapter, PROMContent, AutoloadFail);
	_ReadIDs(Adapter, PROMContent, AutoloadFail);
	_ReadMACAddress(Adapter, PROMContent, AutoloadFail);
	ReadTxPowerInfo(Adapter, PROMContent, AutoloadFail);
	_ReadMacPhyMode(Adapter, PROMContent, AutoloadFail);
	ReadChannelPlan(Adapter, PROMContent, AutoloadFail);
	_ReadBoardType(Adapter, PROMContent, AutoloadFail);
	//_ReadThermalMeter(Adapter, PROMContent, AutoloadFail);
	_ReadLEDSetting(Adapter, PROMContent, AutoloadFail);	
	_ReadRFSetting(Adapter, PROMContent, AutoloadFail);
}

static void _ReadPROMContent(
	IN PADAPTER 		Adapter
	)
{	
	EEPROM_EFUSE_PRIV	*pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u8			PROMContent[HWSET_MAX_SIZE]={0};
	u8			eeValue;
	u32			i;
	u16			value16;

	eeValue = read8(Adapter, REG_9346CR);
	// To check system boot selection.
	pEEPROM->EepromOrEfuse		= (eeValue & BOOT_FROM_EEPROM) ? _TRUE : _FALSE;
	pEEPROM->bautoload_fail_flag	= (eeValue & EEPROM_EN) ? _FALSE : _TRUE;


	DBG_8192C("Boot from %s, Autoload %s !\n", (pEEPROM->EepromOrEfuse ? "EEPROM" : "EFUSE"),
				(pEEPROM->bautoload_fail_flag ? "Fail" : "OK") );

	//pHalData->EEType = (pEEPROM->EepromOrEfuse == _TRUE) ? EEPROM_93C46 : EEPROM_BOOT_EFUSE;

	if (pEEPROM->bautoload_fail_flag == _FALSE)
	{
		if ( pEEPROM->EepromOrEfuse == _TRUE)
		{
			// Read all Content from EEPROM or EFUSE.
			for(i = 0; i < HWSET_MAX_SIZE; i += 2)
			{
				//todo:
				//value16 = EF2Byte(ReadEEprom(Adapter, (u16) (i>>1)));
				//*((u16*)(&PROMContent[i])) = value16; 				
			}
		}
		else
		{
			// Read EFUSE real map to shadow.
			ACQUIRE_GLOBAL_MUTEX(GlobalMutexForPowerAndEfuse);
			EFUSE_ShadowMapUpdate(Adapter);
			RELEASE_GLOBAL_MUTEX(GlobalMutexForPowerAndEfuse);
			_memcpy((void*)PROMContent, (void*)pEEPROM->efuse_eeprom_data, HWSET_MAX_SIZE);		
		}

		//Double check 0x8192 autoload status again
		if(RTL8192_EEPROM_ID != *((u16 *)PROMContent))
		{
			pEEPROM->bautoload_fail_flag = _TRUE;
			DBG_8192C("Autoload OK but EEPROM ID content is incorrect!!\n");
		}
		
	}
	else if ( pEEPROM->EepromOrEfuse == _FALSE)//auto load fail
	{
		_memset(pEEPROM->efuse_eeprom_data, 0xff, HWSET_MAX_SIZE);
		_memcpy((void*)PROMContent, (void*)pEEPROM->efuse_eeprom_data, HWSET_MAX_SIZE);		
	}

	
	_InitAdapterVariablesByPROM(Adapter, PROMContent, pEEPROM->bautoload_fail_flag);
	
}


static VOID
_InitOtherVariable(
	IN PADAPTER 		Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	

	//if(Adapter->bInHctTest){
	//	pMgntInfo->PowerSaveControl.bInactivePs = _FALSE;
	//	pMgntInfo->PowerSaveControl.bIPSModeBackup = _FALSE;
	//	pMgntInfo->PowerSaveControl.bLeisurePs = _FALSE;
	//	pMgntInfo->keepAliveLevel = 0;
	//}

	// 2009/06/10 MH For 92S 1*1=1R/ 1*2&2*2 use 2R. We default set 1*1 use radio A
	// So if you want to use radio B. Please modify RF path enable bit for correct signal
	// strength calculate.
	if (pHalData->rf_type == RF_1T1R){
		pHalData->bRFPathRxEnable[0] = _TRUE;
	}
	else{
		pHalData->bRFPathRxEnable[0] = pHalData->bRFPathRxEnable[1] = _TRUE;
	}

}

static VOID
_ReadRFType(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

#if DISABLE_BB_RF
	pHalData->rf_chip = RF_PSEUDO_11N;
#else
	pHalData->rf_chip = RF_6052;
#endif
}

static int _ReadAdapterInfo8192DU(PADAPTER	Adapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	PADAPTER		BuddyAdapter = Adapter->BuddyAdapter;
	int				i;
	u8				MacPhyCrValue = 0;
	PHAL_DATA_TYPE	pHalDataBuddyAdapter;
#endif

	
	MSG_8192C("====> ReadAdapterInfo8192D\n");

	rtl8192d_ReadChipVersion(Adapter);
	_ReadRFType(Adapter);
	_ReadPROMContent(Adapter);

	_InitOtherVariable(Adapter);

	//For 92DU Phy and Mac mode set ,will initialize by EFUSE/EPPROM     zhiyuan 2010/03/25
	MSG_8192C("<==== ReadAdapterInfo8192D\n");

	return _SUCCESS;
}

static void ReadAdapterInfo8192DU(PADAPTER Adapter)
{
	// Read EEPROM size before call any EEPROM function
	//Adapter->EepromAddressSize=Adapter->HalFunc.GetEEPROMSizeHandler(Adapter);
	Adapter->EepromAddressSize = GetEEPROMSize8192D(Adapter);
	
	_ReadAdapterInfo8192DU(Adapter);
}

#define GPIO_DEBUG_PORT_NUM 0
static void rtl8192du_trigger_gpio_0(_adapter *padapter)
{

	u32 gpioctrl;
	DBG_8192C("==> trigger_gpio_0...\n");
	write16_async(padapter,REG_GPIO_PIN_CTRL,0);
	write8_async(padapter,REG_GPIO_PIN_CTRL+2,0xFF);
	gpioctrl = (BIT(GPIO_DEBUG_PORT_NUM)<<24 )|(BIT(GPIO_DEBUG_PORT_NUM)<<16);
	write32_async(padapter,REG_GPIO_PIN_CTRL,gpioctrl);
	gpioctrl |= (BIT(GPIO_DEBUG_PORT_NUM)<<8);
	write32_async(padapter,REG_GPIO_PIN_CTRL,gpioctrl);
	DBG_8192C("<=== trigger_gpio_0...\n");

}

static VOID
StopTxBeaconWorkItemCallback(
		IN PVOID			pContext
		)
{
	PADAPTER 	Adapter = (PADAPTER)pContext;
	HAL_DATA_TYPE*	pHalData = GET_HAL_DATA(Adapter);
	u8	tmp1Byte = 0;

 	if(IS_NORMAL_CHIP(pHalData->VersionID))
	 {
		tmp1Byte = read8(Adapter, REG_FWHW_TXQ_CTRL+2);
		write8(Adapter, REG_FWHW_TXQ_CTRL+2, tmp1Byte & (~BIT6));
		write8(Adapter, REG_BCN_MAX_ERR, 0xff);
		write8(Adapter, REG_TBTT_PROHIBIT+1, 0x64);
	 	//CheckFwRsvdPageContent(Adapter);  // 2010.06.23. Added by tynli.
	 }
	 else
	 {
		tmp1Byte = read8(Adapter, REG_TXPAUSE);
		write8(Adapter, REG_TXPAUSE, tmp1Byte | BIT6);
	 }
}

static VOID 
StopTxBeacon(	
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE*	pHalData = GET_HAL_DATA(Adapter);

	StopTxBeaconWorkItemCallback((PVOID)Adapter);
	//PlatformScheduleWorkItem(&pHalData->StopTxBeaconWorkItem);

}

static VOID
ResumeTxBeaconWorkItemCallback(
	IN	PVOID	pContext
	)
{
	PADAPTER 	Adapter = (PADAPTER)pContext;
	HAL_DATA_TYPE*	pHalData = GET_HAL_DATA(Adapter);
	u8	tmp1Byte = 0;

	if(IS_NORMAL_CHIP(pHalData->VersionID))
	 {
		tmp1Byte = read8(Adapter, REG_FWHW_TXQ_CTRL+2);
		write8(Adapter, REG_FWHW_TXQ_CTRL+2, tmp1Byte | BIT6);
		write8(Adapter, REG_BCN_MAX_ERR, 0x0a);
		write8(Adapter, REG_TBTT_PROHIBIT+1, 0x64);
	 }
	 else
	 {
		tmp1Byte = read8(Adapter, REG_TXPAUSE);
		write8(Adapter, REG_TXPAUSE, tmp1Byte & (~BIT6));
	 }
}

static VOID
ResumeTxBeacon(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE*	pHalData = GET_HAL_DATA(Adapter);

	ResumeTxBeaconWorkItemCallback((PVOID)Adapter);
	//PlatformScheduleWorkItem(&pHalData->ResumeTxBeaconWorkItem);
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

void SetHwReg8192DU(PADAPTER Adapter, u8 variable, u8* val)
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
				DBG_8192C("HW_VAR_BASIC_RATE: BrateCfg(%#x)\n", BrateCfg);

				pHalData->BasicRateSet = BrateCfg = BrateCfg & 0x15f;

				//if(Adapter->MgntInfo.IOTAction & HT_IOT_ACT_DISABLE_CCK_RATE){
				//	pHalData->BasicRateSet = BrateCfg = BrateCfg & 0x150; // Disable CCK 11M, 5.5M, 2M, and 1M rates.
				//}
				//else
				//{
				//	u16 BRateMask = (pHalData->VersionID ==VERSION_TEST_CHIP_88C )?0x159:0x15F;
					//for 88CU 46PING setting, Disable CCK 2M, 5.5M, Others must tuning
				//	pHalData->BasicRateSet = BrateCfg = BrateCfg & BRateMask;
				//}

				// Set RRSR rate table.
				write8(Adapter, REG_RRSR, BrateCfg&0xff);
				write8(Adapter, REG_RRSR+1, (BrateCfg>>8)&0xff);

				// Set RTS initial rate
				while(BrateCfg > 0x1)
				{
					BrateCfg = (BrateCfg>> 1);
					RateIndex++;
				}
				// Ziv - Check
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
				//if(IS_NORMAL_CHIP(pHalData->VersionID))
				//{
					write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA|RCR_CBSSID_BCN);
				//}
				//else
				//{
				//	write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA);
				//}
			}
			else
			{
				u32	val32;

				val32 = read32(Adapter, REG_RCR);

				//if(IS_NORMAL_CHIP(pHalData->VersionID))
				//{
					val32 &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN);
				//}
				//else
				//{
				//	val32 &= 0xfffff7bf;
				//}

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
				//if(IS_NORMAL_CHIP(pHalData->VersionID))
				//{
					write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)|BIT(4));	
				//}
				//else
				//{
				//	write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)|BIT(4)|BIT(5));
				//}
			}
			break;
		case HW_VAR_MLME_SITESURVEY:
			if(*((u8 *)val))//under sitesurvey
			{
				pHalData->bLoadIMRandIQKSettingFor2G = _FALSE;
				//if(IS_NORMAL_CHIP(pHalData->VersionID))
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
				//else
				//{
					//config RCR to receive different BSSID & not to receive data frame			
				//	write32(Adapter, REG_RCR, read32(Adapter, REG_RCR) & 0xfffff7bf);

					//disable update TSF
				//	write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)|BIT(4)|BIT(5));
				//}
			}
			else//sitesurvey done
			{
				struct mlme_ext_priv	*pmlmeext = &Adapter->mlmeextpriv;
				struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);

				pHalData->bLoadIMRandIQKSettingFor2G = _FALSE;
				if (is_client_associated_to_ap(Adapter) == _TRUE)
				{
					//enable to rx data frame
					//write32(Adapter, REG_RCR, read32(padapter, REG_RCR)|RCR_ADF);
					write16(Adapter, REG_RXFLTMAP2,0xFFFF);

					//enable update TSF
					//if(IS_NORMAL_CHIP(pHalData->VersionID))
					//{
						write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~BIT(4)));
					//}
					//else
					//{
					//	write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~(BIT(4)|BIT(5))));
					//}
				}
				else if((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
				{
					write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_ADF);
					
					//enable update TSF
					//if(IS_NORMAL_CHIP(pHalData->VersionID))			
						write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~BIT(4)));			
					//else			
					//	write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~(BIT(4)|BIT(5))));	
				}


				//if(IS_NORMAL_CHIP(pHalData->VersionID))
				//{
					if((pmlmeinfo->state&0x03) == WIFI_FW_AP_STATE)
						write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_BCN);
					else			
						write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA|RCR_CBSSID_BCN);
				//}
				//else
				//{
				//	write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA);
				//}
			}
			break;
		case HW_VAR_MLME_JOIN:
			{
				u8	RetryLimit = 0x30;
				u8	type = *((u8 *)val);
				if(type == 0) // prepare to join
				{
					//if(IS_NORMAL_CHIP(pHalData->VersionID))
					{
						//config RCR to receive different BSSID & not to receive data frame during linking				
						u32 v = read32(Adapter, REG_RCR);
						v &= ~(RCR_CBSSID_DATA | RCR_CBSSID_BCN );//| RCR_ADF
						write32(Adapter, REG_RCR, v);
						write16(Adapter, REG_RXFLTMAP2,0x00);
					}	
					//else
					//{
						//config RCR to receive different BSSID & not to receive data frame during linking	
					//	write32(Adapter, REG_RCR, read32(Adapter, REG_RCR) & 0xfffff7bf);
					//}
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
						//if(IS_NORMAL_CHIP(pHalData->VersionID))
						//{
							write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA|RCR_CBSSID_BCN);

							//enable update TSF
							write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~BIT(4)));
						//}
						//else
						//{
						//	write32(Adapter, REG_RCR, read32(Adapter, REG_RCR)|RCR_CBSSID_DATA);

							//enable update TSF
						//	write8(Adapter, REG_BCN_CTRL, read8(Adapter, REG_BCN_CTRL)&(~(BIT(4)|BIT(5))));
						//}

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
					write16(Adapter, REG_RXFLTMAP2, 0xFFFF);

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
					PHY_SetBBReg(Adapter, rOFDM0_XAAGCCore1, bMaskByte0, pDigTable->CurIGValue);
					PHY_SetBBReg(Adapter, rOFDM0_XBAGCCore1, bMaskByte0, pDigTable->CurIGValue);
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

				if(check_fwstate(&Adapter->mlmepriv, _FW_LINKED)==_TRUE){
					//printk("%s(): pHalData->bNeedIQK\n",__FUNCTION__);
					pHalData->bNeedIQK = _TRUE; //for 92D IQK
				}

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
							SecMinSpace = 6;
							break;
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
				u8	FactorToSet;
				u32	RegToSet;
				u8	*pTmpByte = NULL;
				u8	index = 0;

				RegToSet = 0xb972a841;

				if(pHalData->MacPhyMode92D==SINGLEMAC_SINGLEPHY){
					RegToSet = 0x88728841;					 
				}
				else{
					RegToSet = 0x66524441;
				}

				FactorToSet = *((u8 *)val);
				if(FactorToSet <= 3)
				{
					FactorToSet = (1<<(FactorToSet + 2));
					if(FactorToSet>0xf)
						FactorToSet = 0xf;

					for(index=0; index<4; index++)
					{
						pTmpByte = (u8 *)(&RegToSet) + index;

						if((*pTmpByte & 0xf0) > (FactorToSet<<4))
							*pTmpByte = (*pTmpByte & 0x0f) | (FactorToSet<<4);
					
						if((*pTmpByte & 0x0f) > FactorToSet)
							*pTmpByte = (*pTmpByte & 0xf0) | (FactorToSet);
					}

					write32(Adapter, REG_AGGLEN_LMT, RegToSet);
					//RT_TRACE(COMP_MLME, DBG_LOUD, ("Set HW_VAR_AMPDU_FACTOR: %#x\n", FactorToSet));
				}
			}
			break;
		case HW_VAR_RXDMA_AGG_PG_TH:
			//write8(Adapter, REG_RXDMA_AGG_PG_TH, *((u8 *)val));
			break;
		case HW_VAR_SET_RPWM:
			{
				u8	RpwmVal = (*(u8 *)val);
				RpwmVal = RpwmVal & 0xf;

				/*if(pHalData->PreRpwmVal & BIT7) //bit7: 1
				{
					PlatformEFIOWrite1Byte(Adapter, REG_USB_HRPWM, (*(pu1Byte)val));
					pHalData->PreRpwmVal = (*(pu1Byte)val);
				}
				else //bit7: 0
				{
					PlatformEFIOWrite1Byte(Adapter, REG_USB_HRPWM, ((*(pu1Byte)val)|BIT7));
					pHalData->PreRpwmVal = ((*(pu1Byte)val)|BIT7);
				}*/
				FillH2CCmd92D(Adapter, H2C_PWRM, 1, (u8 *)(&RpwmVal));
			}
			break;
		case HW_VAR_H2C_FW_PWRMODE:
			rtl8192d_set_FwPwrMode_cmd(Adapter, (*(u8 *)val));
			break;
		case HW_VAR_H2C_FW_JOINBSSRPT:
			rtl8192d_set_FwJoinBssReport_cmd(Adapter, (*(u8 *)val));
			break;
		case HW_VAR_INITIAL_GAIN:
			PHY_SetBBReg(Adapter, rOFDM0_XAAGCCore1, 0x7f, ((u32 *)(val))[0]);
			PHY_SetBBReg(Adapter, rOFDM0_XBAGCCore1, 0x7f, ((u32 *)(val))[0]);
			break;
		case HW_VAR_TRIGGER_GPIO_0:
			rtl8192du_trigger_gpio_0(Adapter);
			break;
		default:
			break;
	}

_func_exit_;
}

void GetHwReg8192DU(PADAPTER Adapter, u8 variable, u8* val)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

_func_enter_;

	switch(variable)
	{
		case HW_VAR_BASIC_RATE:
			*((u16 *)(val)) = pHalData->BasicRateSet;
			break;
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
		default:
			break;
	}

_func_exit_;
}

u32  _update_92cu_basic_rate(_adapter *padapter, unsigned int mask)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(padapter);
#ifdef CONFIG_BT_COEXIST
	struct btcoexist_priv	*pbtpriv = &(pHalData->bt_coexist);
#endif
	unsigned int BrateCfg = 0;

#ifdef CONFIG_BT_COEXIST
	if(	(pbtpriv->BT_Coexist) &&	(pbtpriv->BT_CoexistType == BT_CSR_BC4)	)
	{
		BrateCfg = mask  & 0x151;
		//printk("BT temp disable cck 2/5.5/11M, (0x%x = 0x%x)\n", REG_RRSR, BrateCfg & 0x151);
	}
	else
#endif
	{
		if(pHalData->VersionID != VERSION_TEST_CHIP_88C)
			BrateCfg = mask  & 0x15F;
		else	//for 88CU 46PING setting, Disable CCK 2M, 5.5M, Others must tuning
			BrateCfg = mask  & 0x159;
	}

	BrateCfg |= 0x01; // default enable 1M ACK rate					

	return BrateCfg;
}

void _update_response_rate(_adapter *padapter,unsigned int mask)
{
	u8	RateIndex = 0;
	// Set RRSR rate table.
	write8(padapter, REG_RRSR, mask&0xff);
	write8(padapter,REG_RRSR+1, (mask>>8)&0xff);

	// Set RTS initial rate
	while(mask > 0x1)
	{
		mask = (mask>> 1);
		RateIndex++;
	}
	write8(padapter, REG_INIRTS_RATE_SEL, RateIndex);
}

void UpdateHalRAMask8192DUsb(PADAPTER padapter, unsigned int cam_idx)
{
	u32	value[2];
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
			networkType = judge_network_type(padapter, cur_network->SupportedRates, supportRateNum);
			raid = networktype_to_raid(networkType);
			
			mask = update_basic_rate(cur_network->SupportedRates, supportRateNum);

			//mask = _update_92cu_basic_rate(padapter,mask);
			//_update_response_rate(padapter,mask);
			
			mask |= ((raid<<28)&0xf0000000);
			
			break;
			
		case 0://5: //for AP 
			supportRateNum = get_rateset_len(cur_network->SupportedRates);
			networkType = judge_network_type(padapter, cur_network->SupportedRates, supportRateNum);
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
	
#ifdef CONFIG_BT_COEXIST
	if( (pbtpriv->BT_Coexist) &&
		(pbtpriv->BT_CoexistType == BT_CSR_BC4) &&
		(pbtpriv->BT_CUR_State) &&
		(pbtpriv->BT_Ant_isolation) &&
		((pbtpriv->BT_Service==BT_SCO)||
		(pbtpriv->BT_Service==BT_Busy)) )
		mask &= 0xffffcfc0;
	else		
#endif
		mask &=0xffffffff;
	
	
	init_rate = get_highest_rate_idx(mask)&0x3f;

	if(pHalData->fw_ractrl == _TRUE)
	{
		/*u8 arg = 0;

		//arg = (cam_idx-4)&0x1f;//MACID
		arg = cam_idx&0x1f;//MACID
		
		arg |= BIT(7);
		
		if (shortGIrate==_TRUE)
			arg |= BIT(5);

		DBG_871X("update raid entry, mask=0x%x, arg=0x%x\n", mask, arg);

		rtl8192d_set_raid_cmd(padapter, mask, arg);*/

		value[0] = mask;
		value[1] = cam_idx | (shortGIrate?0x20:0x00) | 0x80;

		FillH2CCmd92D(padapter, H2C_RA_MASK, 5, (u8 *)(&value));
	}
	else
	{
		if (shortGIrate==_TRUE)
			init_rate |= BIT(6);

		write8(padapter, (REG_INIDATA_RATE_SEL+(cam_idx-4)), init_rate);		
	}


	//set ra_id
	psta = pmlmeinfo->FW_sta_info[cam_idx].psta;
	if(psta)
	{
		psta->raid = raid;
		psta->init_rate = init_rate;
	}
}

void SetBeaconRelatedRegisters8192DUsb(PADAPTER padapter)
{
	u32	value32;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct mlme_ext_priv	*pmlmeext = &(padapter->mlmeextpriv);
	struct mlme_ext_info	*pmlmeinfo = &(pmlmeext->mlmext_info);


	write8(padapter, REG_ATIMWND, 0x02);

	write16(padapter, REG_BCN_INTERVAL, pmlmeinfo->bcn_interval);

	_InitBeaconParameters(padapter);

	write8(padapter, REG_SLOT, 0x09);

	//
	// Reset TSF Timer to zero, added by Roger. 2008.06.24
	//
	//pHalData->TransmitConfig &= (~TSFRST);
	value32 = read32(padapter, REG_TCR); 
	value32 &= ~TSFRST;
	write32(padapter, REG_TCR, value32); 

	value32 |= TSFRST;
	write32(padapter, REG_TCR, value32);

	// NOTE: Fix test chip's bug (about contention windows's randomness)
	write8(padapter,  REG_RXTSF_OFFSET_CCK, 0x50);
	write8(padapter, REG_RXTSF_OFFSET_OFDM, 0x50);

	_BeaconFunctionEnable(padapter, _TRUE, _TRUE);

	ResumeTxBeacon(padapter);

	//write8(padapter, 0x422, read8(padapter, 0x422)|BIT(6));
	
	//write8(padapter, 0x541, 0xff);

	//write8(padapter, 0x542, read8(padapter, 0x541)|BIT(0));

	write8(padapter, REG_BCN_CTRL, read8(padapter, REG_BCN_CTRL)|BIT(1));

}

static void rtl8192du_init_default_value(_adapter * padapter)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u8	i;

	pHalData->CurrentWirelessMode = WIRELESS_MODE_AUTO;

	//init default value
	pHalData->fw_ractrl = _FALSE;
	pHalData->LastHMEBoxNum = 0;
	pHalData->bEarlyModeEanble = 0;
	pHalData->pwrGroupCnt = 0;

	//init dm default value
	pdmpriv->TM_Trigger = 0;
	//pdmpriv->binitialized = _FALSE;
	pdmpriv->prv_traffic_idx = 3;

	//keep pHalData->IQKMatrixReg and pHalData->IQKMatrixRegSetting consistent.
	//IQK Matrix Reg:0xc14,0xc1c,0xc4c,oxc78,0xc80,0xc88,0xc94,0xc9c,0xca0
	pHalData->IQKMatrixReg[0] = rOFDM0_XARxIQImbalance; 
	pHalData->IQKMatrixReg[1] = rOFDM0_XBRxIQImbalance; 
	pHalData->IQKMatrixReg[2] = rOFDM0_ECCAThreshold; 
	pHalData->IQKMatrixReg[3] = rOFDM0_AGCRSSITable; 
	pHalData->IQKMatrixReg[4] = rOFDM0_XATxIQImbalance; 
	pHalData->IQKMatrixReg[5] = rOFDM0_XBTxIQImbalance; 
	pHalData->IQKMatrixReg[6] = rOFDM0_XCTxAFE;
	pHalData->IQKMatrixReg[7] = rOFDM0_XDTxAFE;
	pHalData->IQKMatrixReg[8] = 0xca0;

	for(i=0;i<sizeof(pHalData->IQKMatrixRegSetting)/sizeof(IQK_MATRIX_REGS_SETTING);i++){
  		pHalData->IQKMatrixRegSetting[i].bIQKDone = 0;
		//0xc14
		pHalData->IQKMatrixRegSetting[i].Mark[0] = bMaskDWord;
		pHalData->IQKMatrixRegSetting[i].Value[0] = 0x40000100;
		//0xc1c
		pHalData->IQKMatrixRegSetting[i].Mark[1] = bMaskDWord;
		pHalData->IQKMatrixRegSetting[i].Value[1] = 0x40000100;
		//0xc4c
		pHalData->IQKMatrixRegSetting[i].Mark[2] = bMaskByte3;
		pHalData->IQKMatrixRegSetting[i].Value[2] = 0x0;
		//0xc78
		pHalData->IQKMatrixRegSetting[i].Mark[3] = bMaskByte1;//bMaskByte1;
		pHalData->IQKMatrixRegSetting[i].Value[3] = 0;
		
		pHalData->IQKMatrixRegSetting[i].Mark[4] = bMaskDWord;
		pHalData->IQKMatrixRegSetting[i].Mark[5] = bMaskDWord;
		if(i == 0){ //G band
			
			pHalData->IQKMatrixRegSetting[i].Value[4] = 0x40000100;
			pHalData->IQKMatrixRegSetting[i].Value[5] = 0x40000100;
		}else //A band
		{
			pHalData->IQKMatrixRegSetting[i].Value[4] = 0x20000080;
			pHalData->IQKMatrixRegSetting[i].Value[5] = 0x20000080;
		}
		//0xc94
		pHalData->IQKMatrixRegSetting[i].Mark[6] = bMaskByte3;
		pHalData->IQKMatrixRegSetting[i].Value[6] = 0x0;
		//0xc9c
		pHalData->IQKMatrixRegSetting[i].Mark[7] = bMaskByte3;
		pHalData->IQKMatrixRegSetting[i].Value[7] = 0x0;
		//0xca0
		pHalData->IQKMatrixRegSetting[i].Mark[8] = bMaskByte3;
		pHalData->IQKMatrixRegSetting[i].Value[8] = 0x0;

	}

}

static void rtl8192du_free_hal_data(_adapter * padapter)
{
_func_enter_;

	DBG_8192C("===== rtl8192du_free_hal_data =====\n");

	if(padapter->HalData)
		_mfree(padapter->HalData, sizeof(HAL_DATA_TYPE));

_func_exit_;
}

void rtl8192du_set_hal_ops(_adapter * padapter)
{
	struct hal_ops	*pHalFunc = &padapter->HalFunc;

_func_enter_;

	padapter->HalData = _malloc(sizeof(HAL_DATA_TYPE));
	if(padapter->HalData == NULL){
		DBG_8192C("cant not alloc memory for HAL DATA \n");
	}
	_memset(padapter->HalData, 0, sizeof(HAL_DATA_TYPE));

	pHalFunc->hal_init = &rtl8192du_hal_init;
	pHalFunc->hal_deinit = &rtl8192du_hal_deinit;

	pHalFunc->free_hal_data = &rtl8192du_free_hal_data;

	pHalFunc->inirp_init = &rtl8192du_inirp_init;
	pHalFunc->inirp_deinit = &rtl8192du_inirp_deinit;

	pHalFunc->init_xmit_priv = &rtl8192du_init_xmit_priv;
	pHalFunc->free_xmit_priv = &rtl8192du_free_xmit_priv;

	pHalFunc->init_recv_priv = &rtl8192du_init_recv_priv;
	pHalFunc->free_recv_priv = &rtl8192du_free_recv_priv;

	pHalFunc->InitSwLeds = &rtl8192du_InitSwLeds;
	pHalFunc->DeInitSwLeds = &rtl8192du_DeInitSwLeds;

	pHalFunc->dm_init = &rtl8192d_init_dm_priv;
	pHalFunc->dm_deinit = &rtl8192d_deinit_dm_priv;

	pHalFunc->init_default_value = &rtl8192du_init_default_value;
	pHalFunc->intf_chip_configure = &rtl8192du_interface_configure;
	pHalFunc->read_adapter_info = &ReadAdapterInfo8192DU;

	pHalFunc->set_bwmode_handler = &PHY_SetBWMode8192D;
	pHalFunc->set_channel_handler = &PHY_SwChnl8192D;

	pHalFunc->hal_dm_watchdog = &rtl8192d_HalDmWatchDog;

	pHalFunc->SetHwRegHandler = &SetHwReg8192DU;
	pHalFunc->GetHwRegHandler = &GetHwReg8192DU;

	pHalFunc->UpdateRAMaskHandler = &UpdateHalRAMask8192DUsb;
	pHalFunc->SetBeaconRelatedRegistersHandler = &SetBeaconRelatedRegisters8192DUsb;

	pHalFunc->hal_xmit = &rtl8192du_hal_xmit;
	pHalFunc->mgnt_xmit = &rtl8192du_mgnt_xmit;

	pHalFunc->read_bbreg = &rtl8192d_PHY_QueryBBReg;
	pHalFunc->write_bbreg = &rtl8192d_PHY_SetBBReg;
	pHalFunc->read_rfreg = &rtl8192d_PHY_QueryRFReg;
	pHalFunc->write_rfreg = &rtl8192d_PHY_SetRFReg;

#ifdef CONFIG_HOSTAPD_MLME
	pHalFunc->hostap_mgnt_xmit_entry = &rtl8192du_hostap_mgnt_xmit_entry;
#endif

_func_exit_;
}
