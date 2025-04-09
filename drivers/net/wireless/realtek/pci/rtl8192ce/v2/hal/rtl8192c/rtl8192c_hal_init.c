/******************************************************************************
* rtl8192c_hal_init.c                                                                                                                                 *
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

#define _RTL8192C_HAL_INIT_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_byteorder.h>
#include <rtw_efuse.h>

#include <rtl8192c_hal.h>

#ifdef CONFIG_USB_HCI
#include <usb_hal.h>
#endif

#ifdef CONFIG_PCI_HCI
#include <pci_hal.h>
#endif

static VOID
_FWDownloadEnable(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			enable
	)
{
	u8	tmp;

	if(enable)
	{
		// 8051 enable
		tmp = read8(Adapter, REG_SYS_FUNC_EN+1);
		write8(Adapter, REG_SYS_FUNC_EN+1, tmp|0x04);

		// MCU firmware download enable.
		tmp = read8(Adapter, REG_MCUFWDL);
		write8(Adapter, REG_MCUFWDL, tmp|0x01);

		// 8051 reset
		tmp = read8(Adapter, REG_MCUFWDL+2);
		write8(Adapter, REG_MCUFWDL+2, tmp&0xf7);
	}
	else
	{
		// MCU firmware download enable.
		tmp = read8(Adapter, REG_MCUFWDL);
		write8(Adapter, REG_MCUFWDL, tmp&0xfe);

		// Reserved for fw extension.
		write8(Adapter, REG_MCUFWDL+1, 0x00);
	}
}


#define MAX_REG_BOLCK_SIZE	196
#define MIN_REG_BOLCK_SIZE	8

static VOID
_BlockWrite(
	IN		PADAPTER		Adapter,
	IN		PVOID		buffer,
	IN		u32			size
	)
{
#if DEV_BUS_TYPE==DEV_BUS_PCI_INTERFACE
	u32			blockSize	= sizeof(u32);	// Use 4-byte write to download FW
	u8			*bufferPtr	= (u8 *)buffer;
	u32			*pu4BytePtr	= (u32 *)buffer;
	u32			i, offset, blockCount, remainSize;

	blockCount = size / blockSize;
	remainSize = size % blockSize;

	for(i = 0 ; i < blockCount ; i++){
		offset = i * blockSize;
		write32(Adapter, (FW_8192C_START_ADDRESS + offset), *(pu4BytePtr + i));
	}

	if(remainSize){
		offset = blockCount * blockSize;
		bufferPtr += offset;
		
		for(i = 0 ; i < remainSize ; i++){
			write8(Adapter, (FW_8192C_START_ADDRESS + offset + i), *(bufferPtr + i));
		}
	}
#else
  
#ifdef SUPPORTED_BLOCK_IO
	u32			blockSize	= MAX_REG_BOLCK_SIZE;	// Use 196-byte write to download FW	
	u32			blockSize2  	= MIN_REG_BOLCK_SIZE;	
#else
	u32			blockSize	= sizeof(u32);	// Use 4-byte write to download FW
	u32*		pu4BytePtr	= (u32*)buffer;
	u32			blockSize2  	= sizeof(u8);
#endif
	u8*			bufferPtr	= (u8*)buffer;
	u32			i, offset, offset2, blockCount, remainSize, remainSize2;

	blockCount = size / blockSize;
	remainSize = size % blockSize;

	for(i = 0 ; i < blockCount ; i++){
		offset = i * blockSize;
		#ifdef SUPPORTED_BLOCK_IO
		writeN(Adapter, (FW_8192C_START_ADDRESS + offset), blockSize, (bufferPtr + offset));
		#else
		write32(Adapter, (FW_8192C_START_ADDRESS + offset), le32_to_cpu(*(pu4BytePtr + i)));
		#endif
	}

	if(remainSize){
		offset2 = blockCount * blockSize;		
		blockCount = remainSize / blockSize2;
		remainSize2 = remainSize % blockSize2;

		for(i = 0 ; i < blockCount ; i++){
			offset = offset2 + i * blockSize2;
			#ifdef SUPPORTED_BLOCK_IO
			writeN(Adapter, (FW_8192C_START_ADDRESS + offset), blockSize2, (bufferPtr + offset));
			#else
			write8(Adapter, (FW_8192C_START_ADDRESS + offset ), *(bufferPtr + offset));
			#endif
		}		

		if(remainSize2)
		{
			offset += blockSize2;
			bufferPtr += offset;
			
			for(i = 0 ; i < remainSize2 ; i++){
				write8(Adapter, (FW_8192C_START_ADDRESS + offset + i), *(bufferPtr + i));
			}
		}
	}
#endif
}

static VOID
_PageWrite(
	IN		PADAPTER	Adapter,
	IN		u32			page,
	IN		PVOID		buffer,
	IN		u32			size
	)
{
	u8 value8;
	u8 u8Page = (u8) (page & 0x07) ;

	value8 = (read8(Adapter, REG_MCUFWDL+2)& 0xF8 ) | u8Page ;
	write8(Adapter, REG_MCUFWDL+2,value8);
	_BlockWrite(Adapter,buffer,size);
}

static VOID
_FillDummy(
	u8*		pFwBuf,
	u32*	pFwLen
	)
{
	u32	FwLen = *pFwLen;
	u8	remain = (u8)(FwLen%4);
	remain = (remain==0)?0:(4-remain);

	while(remain>0)
	{
		pFwBuf[FwLen] = 0;
		FwLen++;
		remain--;
	}

	*pFwLen = FwLen;
}

static VOID
_WriteFW(
	IN		PADAPTER		Adapter,
	IN		PVOID			buffer,
	IN		u32			size
	)
{
	// Since we need dynamic decide method of dwonload fw, so we call this function to get chip version.
	// We can remove _ReadChipVersion from ReadAdapterInfo8192C later.

	BOOLEAN			isNormalChip;	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	
	isNormalChip = IS_NORMAL_CHIP(pHalData->VersionID);

	if(isNormalChip){
		u32 	pageNums,remainSize ;
		u32 	page,offset;
		u8*	bufferPtr = (u8*)buffer;

#if DEV_BUS_TYPE==DEV_BUS_PCI_INTERFACE
		// 20100120 Joseph: Add for 88CE normal chip. 
		// Fill in zero to make firmware image to dword alignment.
		_FillDummy(bufferPtr, &size);
#endif

		pageNums = size / MAX_PAGE_SIZE ;		
		//RT_ASSERT((pageNums <= 4), ("Page numbers should not greater then 4 \n"));			
		remainSize = size % MAX_PAGE_SIZE;		
		
		for(page = 0; page < pageNums;  page++){
			offset = page *MAX_PAGE_SIZE;
			_PageWrite(Adapter,page, (bufferPtr+offset),MAX_PAGE_SIZE);			
		}
		if(remainSize){
			offset = pageNums *MAX_PAGE_SIZE;
			page = pageNums;
			_PageWrite(Adapter,page, (bufferPtr+offset),remainSize);
		}	
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("_WriteFW Done- for Normal chip.\n"));
	}
	else	{
		_BlockWrite(Adapter,buffer,size);
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("_WriteFW Done- for Test chip.\n"));
	}
}

static int _FWFreeToGo(
	IN		PADAPTER		Adapter
	)
{
	u32			counter = 0;
	u32			value32;
	
	// polling CheckSum report
	do{
		value32 = read32(Adapter, REG_MCUFWDL);
	}while((counter ++ < POLLING_READY_TIMEOUT_COUNT) && (!(value32 & FWDL_ChkSum_rpt)));	

	if(counter >= POLLING_READY_TIMEOUT_COUNT){	
		DBG_8192C("chksum report faill ! REG_MCUFWDL:0x%08x .\n",value32);
		return _FAIL;
	}
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Checksum report OK ! REG_MCUFWDL:0x%08x .\n",value32));


	value32 = read32(Adapter, REG_MCUFWDL);
	value32 |= MCUFWDL_RDY;
	value32 &= ~WINTINI_RDY;
	write32(Adapter, REG_MCUFWDL, value32);
	
	// polling for FW ready
	counter = 0;
	do
	{
		if(read32(Adapter, REG_MCUFWDL) & WINTINI_RDY){
			//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Polling FW ready success!! REG_MCUFWDL:0x%08x .\n",PlatformIORead4Byte(Adapter, REG_MCUFWDL)) );
			return _SUCCESS;
		}
		udelay_os(5);
	}while(counter++ < POLLING_READY_TIMEOUT_COUNT);

	DBG_8192C("Polling FW ready fail!! REG_MCUFWDL:0x%08x .\n", read32(Adapter, REG_MCUFWDL));
	return _FAIL;
	
}


VOID
rtl8192c_FirmwareSelfReset(
	IN		PADAPTER		Adapter
)
{
	u8	u1bTmp;
	u8	Delay = 100;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
		
	if((pHalData->FirmwareVersion > 0x21) ||
		(pHalData->FirmwareVersion == 0x21 &&
		pHalData->FirmwareSubVersion >= 0x01))
	{
		//0x1cf=0x20. Inform 8051 to reset. 2009.12.25. tynli_test
		write8(Adapter, REG_HMETFR+3, 0x20);
	
		u1bTmp = read8(Adapter, REG_SYS_FUNC_EN+1);
		while(u1bTmp&BIT2)
		{
			Delay--;
			if(Delay == 0)
				break;
			udelay_os(50);
			u1bTmp = read8(Adapter, REG_SYS_FUNC_EN+1);
		}

		DBG_8192C("=====> 8051 reset success (%d) .\n", Delay);
	}
}

//
//	Description:
//		Download 8192C firmware code.
//
//
int FirmwareDownload92C(
	IN	PADAPTER			Adapter
)
{	
	int	rtStatus = _SUCCESS;	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	s8 			R92CFwImgFileName_TSMC[] ={RTL8192C_FW_TSMC_IMG};
	s8 			R92CFwImageFileName_UMC[] ={RTL8192C_FW_UMC_IMG};
	u8*			FwImage;
	u32			FwImageLen;
	char*		pFwImageFileName;	
	u8*			pucMappedFile = NULL;
	//vivi, merge 92c and 92s into one driver, 20090817
	//vivi modify this temply, consider it later!!!!!!!!
	//PRT_FIRMWARE	pFirmware = GET_FIRMWARE_819X(Adapter);	
	//PRT_FIRMWARE_92C	pFirmware = GET_FIRMWARE_8192C(Adapter);
	PRT_FIRMWARE_92C	pFirmware = NULL;
	PRT_8192C_FIRMWARE_HDR		pFwHdr = NULL;
	u8		*pFirmwareBuf;
	u32		FirmwareLen;

	pFirmware = (PRT_FIRMWARE_92C)_malloc(sizeof(RT_FIRMWARE_92C));
	if(!pFirmware)
	{
		rtStatus = _FAIL;
		goto Exit;
	}

	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		if(IS_VENDOR_UMC_A_CUT(pHalData->VersionID) && !IS_92C_SERIAL(pHalData->VersionID))
		{
			pFwImageFileName = R92CFwImageFileName_UMC;
			FwImage = Rtl819XFwUMCImageArray;
			FwImageLen = UMCImgArrayLength;
			DBG_8192C(" ===> FirmwareDownload91C() fw:Rtl819XFwImageArray_UMC\n");
		}
		else
		{
			pFwImageFileName = R92CFwImgFileName_TSMC;
			FwImage = Rtl819XFwTSMCImageArray;
			FwImageLen = TSMCImgArrayLength;
			DBG_8192C(" ===> FirmwareDownload91C() fw:Rtl819XFwImageArray_TSMC\n");
		}
	}
	else
	{
		DBG_8192C(" ===> FirmwareDownload91C() fw:Rtl8192CTestFwImg\n");
		rtStatus = _FAIL;
		goto Exit;
	}

	//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> FirmwareDownload91C() fw:%s\n", pFwImageFileName));

#ifdef CONFIG_EMBEDDED_FWIMG
	pFirmware->eFWSource = FW_SOURCE_HEADER_FILE;
#else
	pFirmware->eFWSource = FW_SOURCE_IMG_FILE; // We should decided by Reg.
#endif

	switch(pFirmware->eFWSource)
	{
		case FW_SOURCE_IMG_FILE:
			//TODO:
			break;
		case FW_SOURCE_HEADER_FILE:
			if(FwImageLen > FW_8192C_SIZE){
				rtStatus = _FAIL;
				//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Firmware size exceed 0x%X. Check it.\n", FW_8192C_SIZE) );
				DBG_871X("Firmware size exceed 0x%X. Check it.\n", FW_8192C_SIZE);
				goto Exit;
			}

			_memcpy(pFirmware->szFwBuffer, FwImage, FwImageLen);
			pFirmware->ulFwLength = FwImageLen;
			break;
	}

	pFirmwareBuf = pFirmware->szFwBuffer;
	FirmwareLen = pFirmware->ulFwLength;

	// To Check Fw header. Added by tynli. 2009.12.04.
	pFwHdr = (PRT_8192C_FIRMWARE_HDR)pFirmware->szFwBuffer;

	pHalData->FirmwareVersion =  le16_to_cpu(pFwHdr->Version); 
	pHalData->FirmwareSubVersion = le16_to_cpu(pFwHdr->Subversion); 

	//RT_TRACE(COMP_INIT, DBG_LOUD, (" FirmwareVersion(%#x), Signature(%#x)\n", 
	//	Adapter->MgntInfo.FirmwareVersion, pFwHdr->Signature));

	DBG_8192C("fw_ver=v%d, fw_subver=%d, sig=0x%x\n", 
		pHalData->FirmwareVersion, pHalData->FirmwareSubVersion, le16_to_cpu(pFwHdr->Signature)&0xFFF0);

	if(IS_FW_HEADER_EXIST(pFwHdr))
	{
		//RT_TRACE(COMP_INIT, DBG_LOUD,("Shift 32 bytes for FW header!!\n"));
		pFirmwareBuf = pFirmwareBuf + 32;
		FirmwareLen = FirmwareLen -32;
	}
		
	// Suggested by Filen. If 8051 is running in RAM code, driver should inform Fw to reset by itself,
	// or it will cause download Fw fail. 2010.02.01. by tynli.
	if(read8(Adapter, REG_MCUFWDL)&BIT7) //8051 RAM code
	{	
		rtl8192c_FirmwareSelfReset(Adapter);
		write8(Adapter, REG_MCUFWDL, 0x00);		
	}

		
	_FWDownloadEnable(Adapter, _TRUE);
	_WriteFW(Adapter, pFirmwareBuf, FirmwareLen);
	_FWDownloadEnable(Adapter, _FALSE);

	rtStatus = _FWFreeToGo(Adapter);
	if(_SUCCESS != rtStatus){
		DBG_8192C("DL Firmware failed!\n");
		goto Exit;
	}	
	//RT_TRACE(COMP_INIT, DBG_LOUD, (" Firmware is ready to run!\n"));

Exit:

	if(pFirmware)
		_mfree((u8*)pFirmware, sizeof(RT_FIRMWARE_92C));

	//RT_TRACE(COMP_INIT, DBG_LOUD, (" <=== FirmwareDownload91C()\n"));
	return rtStatus;

}

#ifdef CONFIG_BT_COEXIST
static void _update_bt_param(_adapter *padapter)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(padapter);
	struct btcoexist_priv	*pbtpriv = &(pHalData->bt_coexist);
	struct registry_priv	*registry_par = &padapter->registrypriv;

	if(2 != registry_par->bt_iso)
		pbtpriv->BT_Ant_isolation = registry_par->bt_iso;// 0:Low, 1:High, 2:From Efuse

	if(registry_par->bt_sco == 1) // 0:Idle, 1:None-SCO, 2:SCO, 3:From Counter, 4.Busy, 5.OtherBusy
		pbtpriv->BT_Service = BT_OtherAction;
	else if(registry_par->bt_sco==2)
		pbtpriv->BT_Service = BT_SCO;
	else if(registry_par->bt_sco==4)
		pbtpriv->BT_Service = BT_Busy;
	else if(registry_par->bt_sco==5)
		pbtpriv->BT_Service = BT_OtherBusy;		
	else
		pbtpriv->BT_Service = BT_Idle;

	pbtpriv->BT_Ampdu = registry_par->bt_ampdu;
	pbtpriv->bCOBT = _TRUE;
#if 1
	printk("BT Coexistance = %s\n", (pbtpriv->BT_Coexist==_TRUE)?"enable":"disable");
	if(pbtpriv->BT_Coexist)
	{
		if(pbtpriv->BT_Ant_Num == Ant_x2)
		{
			printk("BlueTooth BT_Ant_Num = Antx2\n");
		}
		else if(pbtpriv->BT_Ant_Num == Ant_x1)
		{
			printk("BlueTooth BT_Ant_Num = Antx1\n");
		}
		switch(pbtpriv->BT_CoexistType)
		{
			case BT_2Wire:
				printk("BlueTooth BT_CoexistType = BT_2Wire\n");
				break;
			case BT_ISSC_3Wire:
				printk("BlueTooth BT_CoexistType = BT_ISSC_3Wire\n");
				break;
			case BT_Accel:
				printk("BlueTooth BT_CoexistType = BT_Accel\n");
				break;
			case BT_CSR_BC4:
				printk("BlueTooth BT_CoexistType = BT_CSR_BC4\n");
				break;
			case BT_RTL8756:
				printk("BlueTooth BT_CoexistType = BT_RTL8756\n");
				break;
			default:
				printk("BlueTooth BT_CoexistType = Unknown\n");
				break;
		}
		printk("BlueTooth BT_Ant_isolation = %d\n", pbtpriv->BT_Ant_isolation);


		switch(pbtpriv->BT_Service)
		{
			case BT_OtherAction:
				printk("BlueTooth BT_Service = BT_OtherAction\n");
				break;
			case BT_SCO:
				printk("BlueTooth BT_Service = BT_SCO\n");
				break;
			case BT_Busy:
				printk("BlueTooth BT_Service = BT_Busy\n");
				break;
			case BT_OtherBusy:
				printk("BlueTooth BT_Service = BT_OtherBusy\n");
				break;			
			default:
				printk("BlueTooth BT_Service = BT_Idle\n");
				break;
		}

		printk("BT_RadioSharedType = 0x%x\n", pbtpriv->BT_RadioSharedType);
	}
#endif

}


#define GET_BT_COEXIST(priv) (&priv->bt_coexist)

void rtl8192c_ReadBluetoothCoexistInfo(
	IN	PADAPTER	Adapter,	
	IN	u8*		PROMContent,
	IN	BOOLEAN		AutoloadFail
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	BOOLEAN			isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	struct btcoexist_priv	*pbtpriv = &(pHalData->bt_coexist);
	u8	rf_opt4;

	if(AutoloadFail){
		pbtpriv->BT_Coexist = _FALSE;
		pbtpriv->BT_CoexistType= BT_2Wire;
		pbtpriv->BT_Ant_Num = Ant_x2;
		pbtpriv->BT_Ant_isolation= 0;
		pbtpriv->BT_RadioSharedType = BT_Radio_Shared;		
		return;
	}

	if(isNormal)
	{
		pbtpriv->BT_Coexist = (((PROMContent[EEPROM_RF_OPT1]&BOARD_TYPE_NORMAL_MASK)>>5) == BOARD_USB_COMBO)?_TRUE:_FALSE;	// bit [7:5]
		rf_opt4 = PROMContent[EEPROM_RF_OPT4];
		pbtpriv->BT_CoexistType 		= ((rf_opt4&0xe)>>1);			// bit [3:1]
		pbtpriv->BT_Ant_Num 		= (rf_opt4&0x1);				// bit [0]
		pbtpriv->BT_Ant_isolation 	= ((rf_opt4&0x10)>>4);			// bit [4]
		pbtpriv->BT_RadioSharedType 	= ((rf_opt4&0x20)>>5);			// bit [5]
	}
	else
	{
		pbtpriv->BT_Coexist = (PROMContent[EEPROM_RF_OPT4] >> 4) ? _TRUE : _FALSE;	
	}
	_update_bt_param(Adapter);

}
#endif

VERSION_8192C
rtl8192c_ReadChipVersion(
	IN	PADAPTER	Adapter
	)
{
	u32			value32;
	VERSION_8192C	version;
	u8			ChipVersion=0;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	value32 = read32(Adapter, REG_SYS_CFG);
#if 0
	if(value32 & TRP_VAUX_EN){		
		//Test chip
		switch(((value32 & CHIP_VER_RTL_MASK) >> CHIP_VER_RTL_SHIFT))
		{
			case 0: //8191C
				version = VERSION_TEST_CHIP_91C;
				break;
			case 1: //8188C
				version = VERSION_TEST_CHIP_88C;
				break;
			default:
				// TODO: set default to 1T1R?
				RT_ASSERT(FALSE,("Chip Version can't be recognized.\n"));
				break;
		}
		
	}
	else{		
		//Normal chip
		version = VERSION_8192C_NORMAL_CHIP;

	}
#else
	if (value32 & TRP_VAUX_EN)
	{
		// Test chip.
		version = (value32 & TYPE_ID) ?VERSION_TEST_CHIP_92C :VERSION_TEST_CHIP_88C;		
	}
	else
	{
		// Normal mass production chip.
		ChipVersion = NORMAL_CHIP;
		ChipVersion |= ((value32 & TYPE_ID) ? CHIP_92C : 0);
		ChipVersion |= ((value32 & VENDOR_ID) ? CHIP_VENDOR_UMC : 0);
		if(IS_VENDOR_UMC(ChipVersion))
			ChipVersion |= ((value32 & CHIP_VER_RTL_MASK) ? CHIP_VENDOR_UMC_B_CUT : 0);

		if(IS_92C_SERIAL(ChipVersion))
		{
			value32 = read32(Adapter, REG_HPON_FSM);
			ChipVersion |= ((value32 & BOND92CE_1T2R_CFG) ? CHIP_92C_1T2R : 0);
		}
		version = (VERSION_8192C)ChipVersion;
	}
#endif
//#if DBG
#if 1
	switch(version)
	{
		case VERSION_NORMAL_TSMC_CHIP_92C_1T2R:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_TSMC_CHIP_92C_1T2R.\n");
			break;
		case VERSION_NORMAL_TSMC_CHIP_92C:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_TSMC_CHIP_92C.\n");
			break;
		case VERSION_NORMAL_TSMC_CHIP_88C:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_TSMC_CHIP_88C.\n");
			break;
		case VERSION_NORMAL_UMC_CHIP_92C_1T2R_A_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_92C_1T2R_A_CUT.\n");
			break;
		case VERSION_NORMAL_UMC_CHIP_92C_A_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_92C_A_CUT.\n");
			break;
		case VERSION_NORMAL_UMC_CHIP_88C_A_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_88C_A_CUT.\n");
			break;			
		case VERSION_NORMAL_UMC_CHIP_92C_1T2R_B_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_92C_1T2R_B_CUT.\n");
			break;
		case VERSION_NORMAL_UMC_CHIP_92C_B_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_92C_B_CUT.\n");
			break;
		case VERSION_NORMAL_UMC_CHIP_88C_B_CUT:
			MSG_8192C("Chip Version ID: VERSION_NORMAL_UMC_CHIP_88C_B_CUT.\n");
			break;
		case VERSION_TEST_CHIP_92C:
			MSG_8192C("Chip Version ID: VERSION_TEST_CHIP_92C.\n");
			break;
		case VERSION_TEST_CHIP_88C:
			MSG_8192C("Chip Version ID: VERSION_TEST_CHIP_88C.\n");
			break;
		default:
			MSG_8192C("Chip Version ID: ???????????????.\n");
			break;
	}
#endif

	pHalData->VersionID = version;

	if(IS_92C_1T2R(version))
		pHalData->rf_type = RF_1T2R;
	else if(IS_92C_SERIAL(version))
		pHalData->rf_type = RF_2T2R;
	else
		pHalData->rf_type = RF_1T1R;

	MSG_8192C("RF_Type is %x!!\n", pHalData->rf_type);

	return version;
}


RT_CHANNEL_DOMAIN
_HalMapChannelPlan8192C(
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

u8 GetEEPROMSize8192C(PADAPTER Adapter)
{
	u8	size = 0;
	u32	curRCR;

	curRCR = read16(Adapter, REG_9346CR);
	size = (curRCR & BOOT_FROM_EEPROM) ? 6 : 4; // 6: EEPROM used is 93C46, 4: boot from E-Fuse.
	
	MSG_8192C("EEPROM type is %s\n", size==4 ? "E-FUSE" : "93C46");
	
	return size;
}

void rtl8192c_free_hal_data(_adapter * padapter)
{
_func_enter_;

	DBG_8192C("===== rtl8192c_free_hal_data =====\n");

	if(padapter->HalData)
		_mfree(padapter->HalData, sizeof(HAL_DATA_TYPE));

_func_exit_;
}
