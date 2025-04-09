/******************************************************************************
* rtl8192d_hal_init.c                                                                                                                                 *
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

#define _RTL8192D_HAL_INIT_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_byteorder.h>
#include <rtw_efuse.h>

#include <hal_init.h>

#ifdef CONFIG_USB_HCI
#include <usb_hal.h>
#endif

#include <rtl8192d_hal.h>

atomic_t GlobalMutexForPowerAndEfuse = ATOMIC_INIT(0);
atomic_t GlobalMutexForPowerOnAndPowerOff = ATOMIC_INIT(0);
atomic_t GlobalMutexForFwDownload = ATOMIC_INIT(0);

static BOOLEAN
_IsFWDownloaded(
	IN	PADAPTER			Adapter
	)
{
	return ((read32(Adapter, REG_MCUFWDL) & MCUFWDL_RDY) ? _TRUE : _FALSE);
}

static VOID
_FWDownloadEnable(
	IN	PADAPTER		Adapter,
	IN	BOOLEAN			enable
	)
{
#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
	u32	value32 = read32(Adapter, REG_MCUFWDL);

	if(enable){
		value32 |= MCUFWDL_EN;
	}
	else{
		value32 &= ~MCUFWDL_EN;
	}

	write32(Adapter, REG_MCUFWDL, value32);

#else
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

		// Reserved for fw extension.   0x81[7] is used for mac0 status ,so don't write this reg here		 
		//write8(Adapter, REG_MCUFWDL+1, 0x00);
	}
#endif
}

#if (DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
static VOID
_BlockWrite_92d(
	IN		PADAPTER		Adapter,
	IN		PVOID			buffer,
	IN		u32				size
	)
{
	u32			blockSize8 = sizeof(u64);	
	u32			blocksize4 = sizeof(u32);
	u32			blockSize = 64;
	u8*			bufferPtr = (u8*)buffer;
	u32*		pu4BytePtr = (u32*)buffer;
	u32			i, offset, blockCount, remainSize,remain8,remain4,blockCount8,blockCount4;

	blockCount = size / blockSize;
	remain8 = size % blockSize;
	for(i = 0 ; i < blockCount ; i++){
		offset = i * blockSize;
		writeN(Adapter, (FW_8192C_START_ADDRESS + offset), 64,(bufferPtr + offset));
	}


	if(remain8){
		offset = blockCount * blockSize;

		blockCount8=remain8/blockSize8;
		remain4=remain8%blockSize8;
		//RT_TRACE(COMP_INIT,DBG_LOUD,("remain4 size %x blockcount %x blockCount8 %x\n",remain4,blockCount,blockCount8));		 
		for(i = 0 ; i < blockCount8 ; i++){	
			writeN(Adapter, (FW_8192C_START_ADDRESS + offset+i*blockSize8), 8,(bufferPtr + offset+i*blockSize8));
		}

		if(remain4){
			offset=blockCount * blockSize+blockCount8*blockSize8;		
			blockCount4=remain4/blocksize4;
			remainSize=remain8%blocksize4;
			 
			for(i = 0 ; i < blockCount4 ; i++){				
				write32(Adapter, (FW_8192C_START_ADDRESS + offset+i*blocksize4), *(pu4BytePtr+ offset/4+i));		
			}	
			
			if(remainSize){
				offset=blockCount * blockSize+blockCount8*blockSize8+blockCount4*blocksize4;
				for(i = 0 ; i < remainSize ; i++){
					write8(Adapter, (FW_8192C_START_ADDRESS + offset + i), *(bufferPtr +offset+ i));
				}	
			}
			
		}
		
	}
	
}
#endif

static VOID
_BlockWrite(
	IN		PADAPTER		Adapter,
	IN		PVOID			buffer,
	IN		u32			size
	)
{
	u32			blockSize	= sizeof(u32);	// Use 4-byte write to download FW
	u8*			bufferPtr	= (u8*)buffer;
	u32*			pu4BytePtr	= (u32*)buffer;
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

}

static VOID
_PageWrite(
	IN		PADAPTER		Adapter,
	IN		u32			page,
	IN		PVOID			buffer,
	IN		u32			size
	)
{
	u8 value8;
	u8 u8Page = (u8) (page & 0x07) ;

	value8 = (read8(Adapter, REG_MCUFWDL+2)& 0xF8 ) | u8Page ;
	write8(Adapter, REG_MCUFWDL+2,value8);
#if (DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)	
	_BlockWrite_92d(Adapter,buffer,size);
#else
	_BlockWrite(Adapter,buffer,size);
#endif
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
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
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
	DBG_8192C("_WriteFW Done- for Normal chip.\n");

}

int _FWFreeToGo_92D(
	IN		PADAPTER		Adapter
	)
{
	u32			counter = 0;
	u32			value32;
	// polling CheckSum report
	do{
		value32 = read32(Adapter, REG_MCUFWDL);
	}while((counter ++ < POLLING_READY_TIMEOUT_COUNT) && (!(value32 & FWDL_ChkSum_rpt  )));	

	if(counter >= POLLING_READY_TIMEOUT_COUNT){	
		DBG_8192C("chksum report faill ! REG_MCUFWDL:0x%08x .\n",value32);
		return _FAIL;
	}
	DBG_8192C("Checksum report OK ! REG_MCUFWDL:0x%08x .\n",value32);

	value32 = read32(Adapter, REG_MCUFWDL);
	value32 |= MCUFWDL_RDY;
	write32(Adapter, REG_MCUFWDL, value32);
	return _SUCCESS;
	
}

VOID
FirmwareSelfReset(
	IN	PADAPTER		Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	u1bTmp;
	u8	Delay = 100;
		
	//if((pHalData->FirmwareVersion > 0x21) ||
	//	(pHalData->FirmwareVersion == 0x21 &&
	//	pHalData->FirmwareSubVersion >= 0x01))
	{
		//0x1cf=0x20. Inform 8051 to reset. 2009.12.25. tynli_test
		write8(Adapter, REG_HMETFR+3, 0x20);
	
		u1bTmp = read8(Adapter, REG_SYS_FUNC_EN+1);
		while(u1bTmp&BIT2)
		{
			Delay--;
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("PowerOffAdapter8192CE(): polling 0x03[2] Delay = %d \n", Delay));
			if(Delay == 0)
				break;
			udelay_os(50);
			u1bTmp = read8(Adapter, REG_SYS_FUNC_EN+1);
		}
	
		if((u1bTmp&BIT2) && (Delay == 0))
		{
			//DbgPrint("FirmwareDownload92C(): Fail!!!!!! 0x03 = %x\n", u1bTmp);
			//RT_ASSERT(FALSE, ("PowerOffAdapter8192CE(): 0x03 = %x\n", u1bTmp));
		}
	}
}

//
// description :polling fw ready
//
int _FWInit(
	IN PADAPTER			  Adapter     
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u32			value[2];
	u32			counter = 0;
	u32			value32;


	DBG_8192C("FW already have download ; \n");

	// polling for FW ready
	counter = 0;
	do
	{
		if(pHalData->interfaceIndex==0){
			if(read8(Adapter, FW_MAC0_ready) & mac0_ready){
				DBG_8192C("Polling FW ready success!! REG_MCUFWDL:0x%x .\n",read8(Adapter, FW_MAC0_ready));
				return _SUCCESS;
			}
			udelay_os(5);
		}
		else{
			if(read8(Adapter, FW_MAC1_ready) &mac1_ready){
				DBG_8192C("Polling FW ready success!! REG_MCUFWDL:0x%x .\n",read8(Adapter, FW_MAC1_ready));
				return _SUCCESS;
			}
			udelay_os(5);					
		}

	}while(counter++ < POLLING_READY_TIMEOUT_COUNT);

	if(pHalData->interfaceIndex==0){
		DBG_8192C("Polling FW ready fail!! MAC0 FW init not ready:0x%x .\n",read8(Adapter, FW_MAC0_ready) );
	}
	else{
		DBG_8192C("Polling FW ready fail!! MAC1 FW init not ready:0x%x .\n",read8(Adapter, FW_MAC1_ready) );
	}
	
	DBG_8192C("Polling FW ready fail!! REG_MCUFWDL:0x%x .\n",read32(Adapter, REG_MCUFWDL));
	return _FAIL;

}

//
//	Description:
//		Download 8192C firmware code.
//
//
int FirmwareDownload92D(
	IN	PADAPTER			Adapter
)
{	
	int	rtStatus = _SUCCESS;	
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	char 			R92DFwImageFileName[] ={RTL8192D_FW_IMG};
	char				TestChipFwFile92D[] = RTL8192D_TEST_FW_IMG_FILE;
	u8*				FwImage;
	u32				FwImageLen;
	char*			pFwImageFileName;
	PRT_FIRMWARE_92D	pFirmware = NULL;
	PRT_8192D_FIRMWARE_HDR		pFwHdr = NULL;
	u8		*pFirmwareBuf;
	u32		FirmwareLen;
	u8		value;
	u32		count;
	BOOLEAN	 bFwDownloaded = _FALSE,bFwDownloadInProcess = _FALSE;

	if(Adapter->bSurpriseRemoved){
		return _FAIL;
	}

	pFirmware = (PRT_FIRMWARE_92D)_malloc(sizeof(RT_FIRMWARE_92D));
	if(!pFirmware)
	{
		rtStatus = _FAIL;
		goto Exit;
	}

	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		pFwImageFileName = R92DFwImageFileName;

		FwImage = Rtl819XFwImageArray;
		FwImageLen = ImgArrayLength;
		DBG_8192C(" ===> FirmwareDownload92D() fw:Rtl819XFwImageArray\n");
	}
	else //testchip
	{
		pFwImageFileName =TestChipFwFile92D;

		FwImage = Rtl8192DTestFwImgArray;
		FwImageLen = Rtl8192DTestImgArrayLength;
		DBG_8192C(" ===> FirmwareDownload92D() fw:Rtl8192CTestFwImg\n");
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
#if 0
			if(ImgArrayLength > FW_8192C_SIZE){
				rtStatus = _FAIL;
				//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("Firmware size exceed 0x%X. Check it.\n", FW_8192C_SIZE) );
				goto Exit;
			}
#endif
			_memcpy(pFirmware->szFwBuffer, FwImage, FwImageLen);
			pFirmware->ulFwLength = FwImageLen;
			break;
	}

	pFirmwareBuf = pFirmware->szFwBuffer;
	FirmwareLen = pFirmware->ulFwLength;

	// To Check Fw header. Added by tynli. 2009.12.04.
	pFwHdr = (PRT_8192D_FIRMWARE_HDR)pFirmware->szFwBuffer;

	pHalData->FirmwareVersion =  le16_to_cpu(pFwHdr->Version); 
	pHalData->FirmwareSubVersion = le16_to_cpu(pFwHdr->Subversion); 

	DBG_8192C(" FirmwareVersion(%#x), Signature(%#x)\n", pHalData->FirmwareVersion, pFwHdr->Signature);

	if(IS_FW_HEADER_EXIST(pFwHdr))
	{
		//DBG_8192C("Shift 32 bytes for FW header!!\n");
		pFirmwareBuf = pFirmwareBuf + 32;
		FirmwareLen = FirmwareLen -32;
	}

  	ACQUIRE_GLOBAL_MUTEX(GlobalMutexForFwDownload);
	bFwDownloaded = _IsFWDownloaded(Adapter);
	if((read8(Adapter, 0x1f)&BIT5) == BIT5)
		bFwDownloadInProcess = _TRUE;
	else
		bFwDownloadInProcess = _FALSE;

	if(bFwDownloaded)
	{
		RELEASE_GLOBAL_MUTEX(GlobalMutexForFwDownload);
		goto Exit;
	}
	else if(bFwDownloadInProcess)
	{
		RELEASE_GLOBAL_MUTEX(GlobalMutexForFwDownload);
		for(count=0;count<5000;count++)
		{
			udelay_os(500);
			ACQUIRE_GLOBAL_MUTEX(GlobalMutexForFwDownload);
			bFwDownloaded = _IsFWDownloaded(Adapter);
			if((read8(Adapter, 0x1f)&BIT5) == BIT5)
				bFwDownloadInProcess = _TRUE;
			else
				bFwDownloadInProcess = _FALSE;
			RELEASE_GLOBAL_MUTEX(GlobalMutexForFwDownload);
			if(bFwDownloaded)
				goto Exit;
			else if(!bFwDownloadInProcess)
				break;
			else
				DBG_8192C("Wait for another mac download fw \n");
		}
		ACQUIRE_GLOBAL_MUTEX(GlobalMutexForFwDownload);
		value=read8(Adapter, 0x1f);
		value|=BIT5;
		write8(Adapter, 0x1f,value);
		RELEASE_GLOBAL_MUTEX(GlobalMutexForFwDownload);
	}
	else
	{
		value=read8(Adapter, 0x1f);
		value|=BIT5;
		write8(Adapter, 0x1f,value);
		RELEASE_GLOBAL_MUTEX(GlobalMutexForFwDownload);
	}

	// Suggested by Filen. If 8051 is running in RAM code, driver should inform Fw to reset by itself,
	// or it will cause download Fw fail. 2010.02.01. by tynli.
	if(read8(Adapter, REG_MCUFWDL)&BIT7) //8051 RAM code
	{	
		FirmwareSelfReset(Adapter);
		write8(Adapter, REG_MCUFWDL, 0x00);		
	}

	_FWDownloadEnable(Adapter, _TRUE);
	_WriteFW(Adapter, pFirmwareBuf, FirmwareLen);
	_FWDownloadEnable(Adapter, _FALSE);

	ACQUIRE_GLOBAL_MUTEX(GlobalMutexForFwDownload);
	rtStatus=_FWFreeToGo_92D(Adapter);	
	// download fw over,clear 0x1f[5]
	value=read8(Adapter, 0x1f);
	value&=(~BIT5);
	write8(Adapter, 0x1f,value);		
	RELEASE_GLOBAL_MUTEX(GlobalMutexForFwDownload);

	if(_SUCCESS != rtStatus){
		DBG_8192C("Firmware is not ready to run!\n");
		goto Exit;
	}

Exit:

	rtStatus =_FWInit(Adapter);

	if(pFirmware)
		_mfree((u8*)pFirmware, sizeof(RT_FIRMWARE_92D));

	//RT_TRACE(COMP_INIT, DBG_LOUD, (" <=== FirmwareDownload91C()\n"));
	return rtStatus;

}


static VERSION_8192D
ReadChipVersion8192D(
	IN	PADAPTER	Adapter
	)
{
	u32			value32;
	u8 			value8;
	VERSION_8192D	version = VERSION_NORMAL_CHIP_88C;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	value32 = read32(Adapter, REG_SYS_CFG);

	//Decide TestChip or NormalChip here.
	//92D's RF_type will be decided when the reg0x2c is filled.
	if (!(value32 & 0x000f0000)){ //Test or Normal Chip:  hardward id 0xf0[19:16] =0 test chip
		version = VERSION_TEST_CHIP_92D_SINGLEPHY;
		DBG_8192C("TEST CHIP!!!\n");
	}
	else{
		version = VERSION_NORMAL_CHIP_92D_SINGLEPHY;
		DBG_8192C("Normal CHIP!!!\n");
	}	

	//Default set. The version will be decide after get the value of reg0x2c.

	return version;
}

static VERSION_8192D
ReadChipVersion(
	IN	PADAPTER	Adapter
	)
{
	VERSION_8192D	version = VERSION_NORMAL_CHIP_88C;

	version = ReadChipVersion8192D(Adapter);

	return version;
}

VOID
rtl8192d_ReadChipVersion(
	IN PADAPTER			Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
	pHalData->VersionID = ReadChipVersion(Adapter);
}

u8 GetEEPROMSize8192D(PADAPTER Adapter)
{
	u8	size = 0;
	u32	curRCR;

	curRCR = read16(Adapter, REG_9346CR);
	size = (curRCR & BOOT_FROM_EEPROM) ? 6 : 4; // 6: EEPROM used is 93C46, 4: boot from E-Fuse.
	
	MSG_8192C("EEPROM type is %s\n", size==4 ? "E-FUSE" : "93C46");
	
	return size;
}

/************************************************************
Function: Synchrosize for power off with dual mac
*************************************************************/
BOOLEAN
PHY_CheckPowerOffFor8192D(
	PADAPTER   Adapter
)
{
	u8 u1bTmp;
	PHAL_DATA_TYPE	pHalData = GET_HAL_DATA(Adapter);

	if(pHalData->MacPhyMode92D==SINGLEMAC_SINGLEPHY)
	{
		u1bTmp = read8(Adapter, REG_MAC0);
		write8(Adapter, REG_MAC0, u1bTmp&(~MAC0_ON));	
		return _TRUE;	  
	}

	ACQUIRE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);
	if(pHalData->interfaceIndex == 0){
		u1bTmp = read8(Adapter, REG_MAC0);
		write8(Adapter, REG_MAC0, u1bTmp&(~MAC0_ON));	
		u1bTmp = read8(Adapter, REG_MAC1);
		u1bTmp &=MAC1_ON;

	}
	else{
		u1bTmp = read8(Adapter, REG_MAC1);
		write8(Adapter, REG_MAC1, u1bTmp&(~MAC1_ON));
		u1bTmp = read8(Adapter, REG_MAC0);
		u1bTmp &=MAC0_ON;
	}

	if(u1bTmp)
	{
		RELEASE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);
		return _FALSE;	
	}

	u1bTmp=read8(Adapter, REG_POWER_OFF_IN_PROCESS);
	u1bTmp|=BIT7;
	write8(Adapter, REG_POWER_OFF_IN_PROCESS, u1bTmp);

	RELEASE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);
	return _TRUE;	
}


/************************************************************
Function: Synchrosize for power off/on with dual mac
*************************************************************/
VOID
PHY_SetPowerOnFor8192D(
	PADAPTER	Adapter
)
{
	PHAL_DATA_TYPE		pHalData = GET_HAL_DATA(Adapter);
	u8	value8;
	u16	i;

	if(pHalData->MacPhyMode92D ==SINGLEMAC_SINGLEPHY)
	{
		value8 = read8(Adapter, REG_MAC0);		
		write8(Adapter, REG_MAC0, value8|MAC0_ON);
	}
	else
	{
		ACQUIRE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);
		if(pHalData->interfaceIndex == 0)
		{
			value8 = read8(Adapter, REG_MAC0);		
			write8(Adapter, REG_MAC0, value8|MAC0_ON);
		}
		else	
		{
			value8 = read8(Adapter, REG_MAC1);
			write8(Adapter, REG_MAC1, value8|MAC1_ON);
		}
		value8 = read8(Adapter, REG_POWER_OFF_IN_PROCESS);
		RELEASE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);

		for(i=0;i<200;i++)
		{
			if((value8&BIT7) == 0)
			{
				break;
			}
			else
			{
				udelay_os(500);
				ACQUIRE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);
				value8 = read8(Adapter, REG_POWER_OFF_IN_PROCESS);
				RELEASE_GLOBAL_MUTEX(GlobalMutexForPowerOnAndPowerOff);
			}
		}

		if(i==200)
			DBG_8192C("Another mac power off over time \n");
	}
}


