/**

@file

@brief   TDA18272 tuner module definition

One can manipulate TDA18272 tuner through TDA18272 module.
TDA18272 module is derived from tuner module.

*/


#include "tuner_tda18272.h"





/**

@brief   TDA18272 tuner module builder

Use BuildTda18272Module() to build TDA18272 module, set all module function pointers with the corresponding functions,
and initialize module private variables.


@param [in]   ppTuner                      Pointer to TDA18272 tuner module pointer
@param [in]   pTunerModuleMemory           Pointer to an allocated tuner module memory
@param [in]   pBaseInterfaceModuleMemory   Pointer to an allocated base interface module memory
@param [in]   pI2cBridgeModuleMemory       Pointer to an allocated I2C bridge module memory
@param [in]   DeviceAddr                   TDA18272 I2C device address
@param [in]   CrystalFreqHz                TDA18272 crystal frequency in Hz
@param [in]   UnitNo                       TDA18272 unit number
@param [in]   IfOutputVppMode              TDA18272 IF output Vp-p mode


@note
	-# One should call BuildTda18272Module() to build TDA18272 module before using it.

*/
void
BuildTda18272Module(
	TUNER_MODULE **ppTuner,
	TUNER_MODULE *pTunerModuleMemory,
	BASE_INTERFACE_MODULE *pBaseInterfaceModuleMemory,
	I2C_BRIDGE_MODULE *pI2cBridgeModuleMemory,
	unsigned char DeviceAddr,
	unsigned long CrystalFreqHz,
	int UnitNo,
	int IfOutputVppMode
	)
{
	TUNER_MODULE *pTuner;
	TDA18272_EXTRA_MODULE *pExtra;



	// Set tuner module pointer.
	*ppTuner = pTunerModuleMemory;

	// Get tuner module.
	pTuner = *ppTuner;

	// Set base interface module pointer and I2C bridge module pointer.
	pTuner->pBaseInterface = pBaseInterfaceModuleMemory;
	pTuner->pI2cBridge = pI2cBridgeModuleMemory;

	// Get tuner extra module.
	pExtra = &(pTuner->Extra.Tda18272);



	// Set tuner type.
	pTuner->TunerType = TUNER_TYPE_TDA18272;

	// Set tuner I2C device address.
	pTuner->DeviceAddr = DeviceAddr;


	// Initialize tuner parameter setting status.
	pTuner->IsRfFreqHzSet = NO;


	// Set tuner module manipulating function pointers.
	pTuner->GetTunerType  = tda18272_GetTunerType;
	pTuner->GetDeviceAddr = tda18272_GetDeviceAddr;

	pTuner->Initialize  = tda18272_Initialize;
	pTuner->SetRfFreqHz = tda18272_SetRfFreqHz;
	pTuner->GetRfFreqHz = tda18272_GetRfFreqHz;


	// Initialize tuner extra module variables.
	pExtra->CrystalFreqHz              = CrystalFreqHz;
	pExtra->UnitNo                     = UnitNo;
	pExtra->IfOutputVppMode            = IfOutputVppMode;
	pExtra->IsStandardBandwidthModeSet = NO;
	pExtra->IsPowerModeSet             = NO;

	// Set tuner extra module function pointers.
	pExtra->SetStandardBandwidthMode = tda18272_SetStandardBandwidthMode;
	pExtra->GetStandardBandwidthMode = tda18272_GetStandardBandwidthMode;
	pExtra->SetPowerMode             = tda18272_SetPowerMode;
	pExtra->GetPowerMode             = tda18272_GetPowerMode;
	pExtra->GetIfFreqHz              = tda18272_GetIfFreqHz;


	return;
}





/**

@see   TUNER_FP_GET_TUNER_TYPE

*/
void
tda18272_GetTunerType(
	TUNER_MODULE *pTuner,
	int *pTunerType
	)
{
	// Get tuner type from tuner module.
	*pTunerType = pTuner->TunerType;


	return;
}





/**

@see   TUNER_FP_GET_DEVICE_ADDR

*/
void
tda18272_GetDeviceAddr(
	TUNER_MODULE *pTuner,
	unsigned char *pDeviceAddr
	)
{
	// Get tuner I2C device address from tuner module.
	*pDeviceAddr = pTuner->DeviceAddr;


	return;
}





/**

@see   TUNER_FP_INITIALIZE

*/
int
tda18272_Initialize(
	TUNER_MODULE *pTuner
	)
{
	TDA18272_EXTRA_MODULE *pExtra;

	int UnitNo;
	tmbslFrontEndDependency_t  sSrvTunerFunc;



	// Get tuner extra module.
	pExtra = &(pTuner->Extra.Tda18272);

	// Get unit number.
	UnitNo = pExtra->UnitNo;


	// Set user-defined function to tuner structure.
	sSrvTunerFunc.sIo.Write             = tda18272_Write;
	sSrvTunerFunc.sIo.Read              = tda18272_Read;
	sSrvTunerFunc.sTime.Get             = Null;
	sSrvTunerFunc.sTime.Wait            = tda18272_Wait;
	sSrvTunerFunc.sDebug.Print          = Null;
	sSrvTunerFunc.sMutex.Init           = Null;
	sSrvTunerFunc.sMutex.DeInit         = Null;
	sSrvTunerFunc.sMutex.Acquire        = Null;
	sSrvTunerFunc.sMutex.Release        = Null;
	sSrvTunerFunc.dwAdditionalDataSize  = 0;
	sSrvTunerFunc.pAdditionalData       = Null;

	// De-initialize tuner.
	// Note: 1. tmbslTDA182I2DeInit() doesn't access hardware.
	//       2. Doesn't need to check tmbslTDA182I2DeInit() return, because we will get error return when the module is
	//          un-initialized.
	tmbslTDA182I2DeInit(UnitNo);

	// Initialize tuner.
	// Note: tmbslTDA182I2Init() doesn't access hardware.
	if(tmbslTDA182I2Init(UnitNo, &sSrvTunerFunc, pTuner) != TM_OK)
		goto error_status_initialize_tuner;

	// Reset tuner.
	if(tmbslTDA182I2Reset(UnitNo) != TM_OK)
		goto error_status_initialize_tuner;


	return FUNCTION_SUCCESS;


error_status_initialize_tuner:
	return FUNCTION_ERROR;
}





/**

@see   TUNER_FP_SET_RF_FREQ_HZ

*/
int
tda18272_SetRfFreqHz(
	TUNER_MODULE *pTuner,
	unsigned long RfFreqHz
	)
{
	TDA18272_EXTRA_MODULE *pExtra;
	tmUnitSelect_t UnitNo;
	tmTDA182I2IF_AGC_Gain_t IfOutputVppMode;



	// Get tuner extra module.
	pExtra = &(pTuner->Extra.Tda18272);

	// Get unit number.
	UnitNo = (tmUnitSelect_t)pExtra->UnitNo;

	// Get IF output Vp-p mode.
	IfOutputVppMode = (tmTDA182I2IF_AGC_Gain_t)pExtra->IfOutputVppMode;


	// Set tuner RF frequency.
	if(tmbslTDA182I2SetRf(UnitNo, RfFreqHz, IfOutputVppMode) != TM_OK)
		goto error_status_set_tuner_rf_frequency;


	// Set tuner RF frequency parameter.
	pTuner->RfFreqHz      = RfFreqHz;
	pTuner->IsRfFreqHzSet = YES;


	return FUNCTION_SUCCESS;


error_status_set_tuner_rf_frequency:
	return FUNCTION_ERROR;
}





/**

@see   TUNER_FP_GET_RF_FREQ_HZ

*/
int
tda18272_GetRfFreqHz(
	TUNER_MODULE *pTuner,
	unsigned long *pRfFreqHz
	)
{
	// Get tuner RF frequency in Hz from tuner module.
	if(pTuner->IsRfFreqHzSet != YES)
		goto error_status_get_tuner_rf_frequency;

	*pRfFreqHz = pTuner->RfFreqHz;


	return FUNCTION_SUCCESS;


error_status_get_tuner_rf_frequency:
	return FUNCTION_ERROR;
}





/**

@brief   Set TDA18272 tuner standard and bandwidth mode.

*/
int
tda18272_SetStandardBandwidthMode(
	TUNER_MODULE *pTuner,
	int StandardBandwidthMode
	)
{
	TDA18272_EXTRA_MODULE *pExtra;
	int UnitNo;



	// Get tuner extra module.
	pExtra = &(pTuner->Extra.Tda18272);

	// Get unit number.
	UnitNo = pExtra->UnitNo;


	// Set tuner standard and bandwidth mode.
	if(tmbslTDA182I2SetStandardMode(UnitNo, StandardBandwidthMode) != TM_OK)
		goto error_status_set_tuner_standard_bandwidth_mode;


	// Set tuner bandwidth mode parameter.
	pExtra->StandardBandwidthMode      = StandardBandwidthMode;
	pExtra->IsStandardBandwidthModeSet = YES;


	return FUNCTION_SUCCESS;


error_status_set_tuner_standard_bandwidth_mode:
	return FUNCTION_ERROR;
}





/**

@brief   Get TDA18272 tuner standard and bandwidth mode.

*/
int
tda18272_GetStandardBandwidthMode(
	TUNER_MODULE *pTuner,
	int *pStandardBandwidthMode
	)
{
	TDA18272_EXTRA_MODULE *pExtra;



	// Get tuner extra module.
	pExtra = &(pTuner->Extra.Tda18272);


	// Get tuner bandwidth mode from tuner module.
	if(pExtra->IsStandardBandwidthModeSet != YES)
		goto error_status_get_tuner_standard_bandwidth_mode;

	*pStandardBandwidthMode = pExtra->StandardBandwidthMode;


	return FUNCTION_SUCCESS;


error_status_get_tuner_standard_bandwidth_mode:
	return FUNCTION_ERROR;
}





/**

@brief   Set TDA18272 tuner power mode.

*/
int
tda18272_SetPowerMode(
	TUNER_MODULE *pTuner,
	int PowerMode
	)
{
	TDA18272_EXTRA_MODULE *pExtra;
	int UnitNo;



	// Get tuner extra module.
	pExtra = &(pTuner->Extra.Tda18272);

	// Get unit number.
	UnitNo = pExtra->UnitNo;


	// Set tuner power mode.
	if(tmbslTDA182I2SetPowerState(UnitNo, PowerMode) != TM_OK)
		goto error_status_set_tuner_power_mode;


	// Set tuner power mode parameter.
	pExtra->PowerMode      = PowerMode;
	pExtra->IsPowerModeSet = YES;


	return FUNCTION_SUCCESS;


error_status_set_tuner_power_mode:
	return FUNCTION_ERROR;
}





/**

@brief   Get TDA18272 tuner power mode.

*/
int
tda18272_GetPowerMode(
	TUNER_MODULE *pTuner,
	int *pPowerMode
	)
{
	TDA18272_EXTRA_MODULE *pExtra;



	// Get tuner extra module.
	pExtra = &(pTuner->Extra.Tda18272);


	// Get tuner power mode from tuner module.
	if(pExtra->IsPowerModeSet != YES)
		goto error_status_get_tuner_power_mode;

	*pPowerMode = pExtra->PowerMode;


	return FUNCTION_SUCCESS;


error_status_get_tuner_power_mode:
	return FUNCTION_ERROR;
}





/**

@brief   Get TDA18272 tuner IF frequency in Hz.

*/
int
tda18272_GetIfFreqHz(
	TUNER_MODULE *pTuner,
	unsigned long *pIfFreqHz
	)
{
	TDA18272_EXTRA_MODULE *pExtra;
	int UnitNo;



	// Get tuner extra module.
	pExtra = &(pTuner->Extra.Tda18272);

	// Get unit number.
	UnitNo = pExtra->UnitNo;


	// Get tuner IF frequency in Hz.
	if(tmbslTDA182I2GetIF(UnitNo, pIfFreqHz) != TM_OK)
		goto error_status_get_tuner_if_frequency;


	return FUNCTION_SUCCESS;


error_status_get_tuner_if_frequency:
	return FUNCTION_ERROR;
}























// The following context is implemented for TDA18272 source code.


// Read TDA18272 registers.
tmErrorCode_t
tda18272_Read(
	tmUnitSelect_t tUnit,
	UInt32 AddrSize,
	UInt8* pAddr,
	UInt32 ReadLen,
	UInt8* pData
	)
{
	ptmTDA182I2Object_t pObj;

	TUNER_MODULE *pTuner;
	I2C_BRIDGE_MODULE *pI2cBridge;
	unsigned char DeviceAddr;
	struct dvb_usb_device	*d;
	BASE_INTERFACE_MODULE *pBaseInterface;
	unsigned char RegAddr=*pAddr;


	// Get NXP object.
	pObj = Null;
	if(TDA182I2GetInstance(tUnit, &pObj) != TM_OK)
		goto error_status_get_nxp_object;


	// Get tuner.
	pTuner = pObj->pTuner;

	// Get I2C bridge.
	pI2cBridge = pTuner->pI2cBridge;

	// Get tuner device address.
	pTuner->GetDeviceAddr(pTuner, &DeviceAddr);
	
	

#if 0
	// Set tuner register reading address.
	// Note: The I2C format of tuner register reading address setting is as follows:
	//       start_bit + (DeviceAddr | writing_bit) + addr * N + stop_bit
	if(pI2cBridge->ForwardI2cWritingCmd(pI2cBridge, DeviceAddr, pAddr, (unsigned long)AddrSize) != FUNCTION_SUCCESS)
		goto error_status_set_tuner_register_reading_address;

	// Get tuner register byte.
	// Note: The I2C format of tuner register byte getting is as follows:
	//       start_bit + (DeviceAddr | reading_bit) + read_data * N + stop_bit
	if(pI2cBridge->ForwardI2cReadingCmd(pI2cBridge, DeviceAddr, pData, (unsigned long)ReadLen) != FUNCTION_SUCCESS)
		goto error_status_get_tuner_registers;
#endif
	pBaseInterface = pTuner->pBaseInterface;
	pBaseInterface->GetUserDefinedDataPointer(pBaseInterface, (void **)&d);	
	
	
	if( read_rtl2832_tuner_register( d, DeviceAddr, RegAddr, pData, (unsigned char)ReadLen ) )
		goto error_status_get_tuner_registers;

	
	return TM_OK;


error_status_get_tuner_registers:
//error_status_set_tuner_register_reading_address:
error_status_get_nxp_object:
	return TM_ERR_READ;
}





// Write TDA18272 registers.
tmErrorCode_t
tda18272_Write(
	tmUnitSelect_t tUnit,
	UInt32 AddrSize,
	UInt8* pAddr,
	UInt32 WriteLen,
	UInt8* pData
	)
{
	ptmTDA182I2Object_t pObj;

	TUNER_MODULE *pTuner;
	I2C_BRIDGE_MODULE *pI2cBridge;
	unsigned char DeviceAddr;
	unsigned char WritingBuffer[I2C_BUFFER_LEN];

	unsigned long i;
	unsigned long WritingByteNum;

//	struct dvb_usb_device	*d;
//	BASE_INTERFACE_MODULE *pBaseInterface;
//	unsigned char RegAddr=*pAddr;

	// Get NXP object.
	pObj = Null;
	if(TDA182I2GetInstance(tUnit, &pObj) != TM_OK)
		goto error_status_get_nxp_object;


	// Get tuner.
	pTuner = pObj->pTuner;

	// Get I2C bridge.
	pI2cBridge = pTuner->pI2cBridge;

	// Get tuner device address.
	pTuner->GetDeviceAddr(pTuner, &DeviceAddr);


	// Calculate writing byte number.
	WritingByteNum = (unsigned char)(AddrSize + WriteLen);


	// Set writing bytes.
	// Note: The I2C format of tuner register byte setting is as follows:
	//       start_bit + (DeviceAddr | writing_bit) + addr * N + data * N + stop_bit
	for(i = 0; i < AddrSize; i++)
		WritingBuffer[i] = pAddr[i];

	for(i = 0; i < WriteLen; i++)
		WritingBuffer[AddrSize + i] = pData[i];


	// Set tuner register bytes with writing buffer.
	if(pI2cBridge->ForwardI2cWritingCmd(pI2cBridge, DeviceAddr, WritingBuffer, WritingByteNum) != FUNCTION_SUCCESS)
		goto error_status_set_tuner_registers;

#if 0
	pBaseInterface = pTuner->pBaseInterface;
	pBaseInterface->GetUserDefinedDataPointer(pBaseInterface, (void **)&d);	
	if( write_rtl2832_tuner_register( d, DeviceAddr, RegAddr, pData, (unsigned char)WriteLen) )
		goto error_status_set_tuner_registers;
#endif	
	return TM_OK;


error_status_set_tuner_registers:
error_status_get_nxp_object:
	return TM_ERR_WRITE;
}





// Wait a time.
tmErrorCode_t
tda18272_Wait(
	tmUnitSelect_t tUnit,
	UInt32 tms
	)
{
	ptmTDA182I2Object_t pObj;

	TUNER_MODULE *pTuner;
	BASE_INTERFACE_MODULE *pBaseInterface;


	// Get NXP object.
	pObj = Null;
	if(TDA182I2GetInstance(tUnit, &pObj) != TM_OK)
		goto error_status_get_nxp_object;


	// Get tuner.
	pTuner = pObj->pTuner;

	// Get I2C bridge.
	pBaseInterface = pTuner->pBaseInterface;

	// Wait a time.
	pBaseInterface->WaitMs(pBaseInterface, tms);


	return TM_OK;


error_status_get_nxp_object:
	return TM_ERR_BAD_UNIT_NUMBER;
}























// The following context is source code provided by NXP.





// NXP source code - .\tmbslTDA182I2\src\tmbslTDA182I2.c


//-----------------------------------------------------------------------------
// $Header: 
// (C) Copyright 2001 NXP Semiconductors, All rights reserved
//
// This source code and any compilation or derivative thereof is the sole
// property of NXP Corporation and is provided pursuant to a Software
// License Agreement.  This code is the proprietary information of NXP
// Corporation and is confidential in nature.  Its use and dissemination by
// any party other than NXP Corporation is strictly limited by the
// confidential information provisions of the Agreement referenced above.
//-----------------------------------------------------------------------------
// FILE NAME:    tmbslTDA182I2.c
//
// %version: 25 %
//
// DESCRIPTION:  Function for the Hybrid silicon tuner TDA182I2
//
// DOCUMENT REF: 
//
// NOTES:
//-----------------------------------------------------------------------------
//

//-----------------------------------------------------------------------------
// Standard include files:
//-----------------------------------------------------------------------------
//

//#include "tmNxTypes.h"
//#include "tmCompId.h"
//#include "tmFrontEnd.h"
//#include "tmbslFrontEndTypes.h"

//#include "tmddTDA182I2.h"

//#ifdef TMBSL_TDA18272
// #include "tmbslTDA18272.h"
//#else /* TMBSL_TDA18272 */
// #include "tmbslTDA18212.h"
//#endif /* TMBSL_TDA18272 */

//-----------------------------------------------------------------------------
// Project include files:
//-----------------------------------------------------------------------------
//
//#include "tmbslTDA182I2local.h"
//#include "tmbslTDA182I2Instance.h"

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
//

//-----------------------------------------------------------------------------
// Global data:
//-----------------------------------------------------------------------------
//

//-----------------------------------------------------------------------------
// Exported functions:
//-----------------------------------------------------------------------------
//

//-----------------------------------------------------------------------------
// FUNCTION:    tmbslTDA18211Init:
//
// DESCRIPTION: create an instance of a TDA182I2 Tuner
//
// RETURN:      TMBSL_ERR_TUNER_BAD_UNIT_NUMBER
//              TM_OK
//  
// NOTES:
//-----------------------------------------------------------------------------
//
tmErrorCode_t
tmbslTDA182I2Init
(
    tmUnitSelect_t              tUnit,      /* I: Unit number */
    tmbslFrontEndDependency_t*  psSrvFunc,   /*  I: setup parameters */
    TUNER_MODULE                *pTuner	    // Added by Realtek
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err  = TM_OK;

    if (psSrvFunc == Null)
    {
        err = TDA182I2_ERR_BAD_PARAMETER;
    }

    if(err == TM_OK)
    {
        //----------------------
        // initialize the Object
        //----------------------
        // pObj initialization
        err = TDA182I2GetInstance(tUnit, &pObj);
    }

    /* check driver state */
    if (err == TM_OK || err == TDA182I2_ERR_NOT_INITIALIZED)
    {
        if (pObj != Null && pObj->init == True)
        {
            err = TDA182I2_ERR_NOT_INITIALIZED;
        }
        else 
        {
            /* initialize the Object */
            if (pObj == Null)
            {
                err = TDA182I2AllocInstance(tUnit, &pObj);
                if (err != TM_OK || pObj == Null)
                {
                    err = TDA182I2_ERR_NOT_INITIALIZED;        
                }
            }

            if (err == TM_OK)
            {
                // initialize the Object by default values
                pObj->sRWFunc = psSrvFunc->sIo;
                pObj->sTime = psSrvFunc->sTime;
                pObj->sDebug = psSrvFunc->sDebug;

                if(  psSrvFunc->sMutex.Init != Null
                    && psSrvFunc->sMutex.DeInit != Null
                    && psSrvFunc->sMutex.Acquire != Null
                    && psSrvFunc->sMutex.Release != Null)
                {
                    pObj->sMutex = psSrvFunc->sMutex;

                    err = pObj->sMutex.Init(&pObj->pMutex);
                }

                pObj->init = True;
                err = TM_OK;

                err = tmddTDA182I2Init(tUnit, psSrvFunc);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Init(0x%08X) failed.", pObj->tUnit));
            }
        }
    }

    // Added by Realtek
    pObj->pTuner = pTuner;

    return err;
}

//-----------------------------------------------------------------------------
// FUNCTION:    tmbslTDA182I2DeInit:
//
// DESCRIPTION: destroy an instance of a TDA182I2 Tuner
//
// RETURN:      TMBSL_ERR_TUNER_BAD_UNIT_NUMBER
//              TDA182I2_ERR_NOT_INITIALIZED
//              TM_OK
//
// NOTES:
//-----------------------------------------------------------------------------
//
tmErrorCode_t 
tmbslTDA182I2DeInit
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    /* check input parameters */
    err = TDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = tmddTDA182I2DeInit(tUnit);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2DeInit(0x%08X) failed.", pObj->tUnit));

        (void)TDA182I2MutexRelease(pObj);

        if(pObj->sMutex.DeInit != Null)
        {
            if(pObj->pMutex != Null)
            {
                err = pObj->sMutex.DeInit(pObj->pMutex);
            }

            pObj->sMutex.Init = Null;
            pObj->sMutex.DeInit = Null;
            pObj->sMutex.Acquire = Null;
            pObj->sMutex.Release = Null;

            pObj->pMutex = Null;
        }
    }

    err = TDA182I2DeAllocInstance(tUnit);

    return err;
}

//-----------------------------------------------------------------------------
// FUNCTION:    tmbslTDA182I2GetSWVersion:
//
// DESCRIPTION: Return the version of this device
//
// RETURN:      TM_OK
//
// NOTES:       Values defined in the tmTDA182I2local.h file
//-----------------------------------------------------------------------------
//
tmErrorCode_t
tmbslTDA182I2GetSWVersion
(
    ptmSWVersion_t  pSWVersion  /* I: Receives SW Version */
)
{
    tmErrorCode_t   err = TDA182I2_ERR_NOT_INITIALIZED;
    
    err = tmddTDA182I2GetSWVersion(pSWVersion);
    
    return err;
}

//-----------------------------------------------------------------------------
// FUNCTION:    tmbslTDA182I2CheckHWVersion:
//
// DESCRIPTION: Check HW version
//
// RETURN:      TM_OK if no error
//
// NOTES:       Values defined in the tmTDA182I2local.h file
//-----------------------------------------------------------------------------
tmErrorCode_t
tmbslTDA182I2CheckHWVersion
(
    tmUnitSelect_t tUnit    /* I: Unit number */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TDA182I2_ERR_NOT_INITIALIZED;
    UInt16              uIdentity = 0;
    UInt8               majorRevision = 0;

    err = TDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = tmddTDA182I2GetIdentity(tUnit, &uIdentity);

        if(err == TM_OK)
        {
            if(uIdentity == 18272 || uIdentity == 18212)
            {
                /* TDA18272/12 found. Check Major Revision*/
                err = tmddTDA182I2GetMajorRevision(tUnit, &majorRevision);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetMajorRevision(0x%08X) failed.", tUnit));

                if(err == TM_OK && majorRevision != 1)
                {
                    /* Only TDA18272/12 ES2 are supported */
                    err = TDA182I2_ERR_BAD_VERSION;
                }
            }
            else
            {
                err = TDA182I2_ERR_BAD_VERSION;
            }
        }

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmbslTDA182I2SetPowerState:
//
// DESCRIPTION: Set the power state of this device.
//
// RETURN:      TMBSL_ERR_TUNER_BAD_UNIT_NUMBER
//              TDA182I2_ERR_NOT_INITIALIZED
//              TM_OK
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmbslTDA182I2SetPowerState
(
    tmUnitSelect_t          tUnit,      /* I: Unit number */
    tmTDA182I2PowerState_t  powerState  /* I: Power state of this device */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err  = TM_OK;

    // pObj initialization
    err = TDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // Call tmddTDA182I2SetPowerState
        err = tmddTDA182I2SetPowerState(tUnit, powerState);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetPowerState(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // set power state
            pObj->curPowerState = powerState;
        }

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmbslTDA182I2GetPowerState:
//
// DESCRIPTION: Get the power state of this device.
//
// RETURN:      TMBSL_ERR_TUNER_BAD_UNIT_NUMBER
//              TDA182I2_ERR_NOT_INITIALIZED
//              TM_OK
//
// NOTES:
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmbslTDA182I2GetPowerState
(
    tmUnitSelect_t          tUnit,          /* I: Unit number */
    tmTDA182I2PowerState_t  *pPowerState    /* O: Power state of this device */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err  = TM_OK;

    if(pPowerState == Null)
        err = TDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        // pObj initialization
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // get power state
        *pPowerState = pObj->curPowerState;

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmbslTDA182I2SetStandardMode:
//
// DESCRIPTION: Set the standard mode of this device.
//
// RETURN:      TMBSL_ERR_TUNER_BAD_UNIT_NUMBER
//              TDA182I2_ERR_NOT_INITIALIZED
//              TM_OK
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmbslTDA182I2SetStandardMode
(
    tmUnitSelect_t              tUnit,          /* I: Unit number */
    tmTDA182I2StandardMode_t    StandardMode    /* I: Standard mode of this device */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err  = TM_OK;

    // pObj initialization
    err = TDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // store standard mode 
        pObj->StandardMode = StandardMode;

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmbslTDA182I2GetStandardMode:
//
// DESCRIPTION: Get the standard mode of this device.
//
// RETURN:      TMBSL_ERR_TUNER_BAD_UNIT_NUMBER
//              TDA182I2_ERR_NOT_INITIALIZED
//              TM_OK
//
// NOTES:
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmbslTDA182I2GetStandardMode
(
    tmUnitSelect_t              tUnit,          /* I: Unit number */
    tmTDA182I2StandardMode_t    *pStandardMode  /* O: Standard mode of this device */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err  = TM_OK;

    if(pStandardMode == Null)
        err = TDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        // pObj initialization
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Get standard mode */
        *pStandardMode = pObj->StandardMode;

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmbslTDA182I2SetRf:
//
// DESCRIPTION: Calculate i2c I2CMap & write in TDA182I2
//
// RETURN:      TMBSL_ERR_TUNER_BAD_UNIT_NUMBER
//              TDA182I2_ERR_NOT_INITIALIZED
//              TDA182I2_ERR_BAD_PARAMETER
//              TMBSL_ERR_IIC_ERR
//              TM_OK
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmbslTDA182I2SetRf
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32          uRF,    /* I: RF frequency in hertz */
    tmTDA182I2IF_AGC_Gain_t            IF_Gain     // Added by realtek
)
{    
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err  = TM_OK;
    Bool                bIRQWait = True;
    /* ---------- ES2 PATCH BEGIN ---------- */
    UInt8               majorRevision = 0;
    UInt8               minorRevision = 0;
    /* ---------- ES2 PATCH END ---------- */
    UInt8   ratioL, ratioH;  /* patch Spurious */
    UInt32  DeltaL, DeltaH; /* patch Spurious */

    //------------------------------
    // test input parameters
    //------------------------------
    // pObj initialization
    err = TDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = tmddTDA182I2GetIRQWait(tUnit, &bIRQWait);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetIRQWait(0x%08X) failed.", tUnit));

        pObj->uRF = uRF;

        if(err == TM_OK)
        {
            /* Set LPF */
            err = tmddTDA182I2SetLP_FC(tUnit, pObj->Std_Array[pObj->StandardMode].LPF);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetLP_FC(0x%08X) failed.", tUnit));
        }
 
        if(err == TM_OK)
        {
            /* Set LPF Offset */
            err = tmddTDA182I2SetLP_FC_Offset(tUnit, pObj->Std_Array[pObj->StandardMode].LPF_Offset);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetLP_FC_Offset(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set IF Gain */
//            err = tmddTDA182I2SetIF_Level(tUnit, pObj->Std_Array[pObj->StandardMode].IF_Gain);
            err = tmddTDA182I2SetIF_Level(tUnit, IF_Gain);		// Modified by Realtek
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIF_Level(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set IF Notch */
            err = tmddTDA182I2SetIF_ATSC_Notch(tUnit, pObj->Std_Array[pObj->StandardMode].IF_Notch);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIF_ATSC_Notch(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Enable/disable HPF */
            if ( pObj->Std_Array[pObj->StandardMode].IF_HPF == tmTDA182I2_IF_HPF_Disabled )
            {
                err = tmddTDA182I2SetHi_Pass(tUnit, 0);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetHi_Pass(0x%08X, 0) failed.", tUnit));
            }
            else
            {
                err = tmddTDA182I2SetHi_Pass(tUnit, 1);     
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetHi_Pass(0x%08X, 1) failed.", tUnit));

                if(err == TM_OK)
                {
                    /* Set IF HPF */
                    err = tmddTDA182I2SetIF_HP_Fc(tUnit, (UInt8)(pObj->Std_Array[pObj->StandardMode].IF_HPF - 1));
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIF_HP_Fc(0x%08X) failed.", tUnit));
                }
            }
        }

        if(err == TM_OK)
        {
            /* Set DC Notch */
            err = tmddTDA182I2SetIF_Notch(tUnit, pObj->Std_Array[pObj->StandardMode].DC_Notch);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIF_Notch(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set AGC1 LNA Top */
            err = tmddTDA182I2SetAGC1_TOP(tUnit, pObj->Std_Array[pObj->StandardMode].AGC1_LNA_TOP);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC1_TOP(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set AGC2 RF Top */
            err = tmddTDA182I2SetAGC2_TOP(tUnit, pObj->Std_Array[pObj->StandardMode].AGC2_RF_Attenuator_TOP);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC2_TOP(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set AGC3 RF AGC Top */
            if (pObj->uRF < tmTDA182I2_AGC3_RF_AGC_TOP_FREQ_LIM)
            {
                err = tmddTDA182I2SetRFAGC_Top(tUnit, pObj->Std_Array[pObj->StandardMode].AGC3_RF_AGC_TOP_Low_band);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRFAGC_Top(0x%08X) failed.", tUnit));
            }
            else
            {
                err = tmddTDA182I2SetRFAGC_Top(tUnit, pObj->Std_Array[pObj->StandardMode].AGC3_RF_AGC_TOP_High_band);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRFAGC_Top(0x%08X) failed.", tUnit));
            }
        }

        if(err == TM_OK)
        {
            /* Set AGC4 IR Mixer Top */
            err = tmddTDA182I2SetIR_Mixer_Top(tUnit, pObj->Std_Array[pObj->StandardMode].AGC4_IR_Mixer_TOP);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIR_Mixer_Top(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set AGC5 IF AGC Top */
            err = tmddTDA182I2SetAGC5_TOP(tUnit, pObj->Std_Array[pObj->StandardMode].AGC5_IF_AGC_TOP);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC5_TOP(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set AGC3 Adapt */
            err = tmddTDA182I2SetPD_RFAGC_Adapt(tUnit, pObj->Std_Array[pObj->StandardMode].AGC3_Adapt);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetPD_RFAGC_Adapt(0x%08X) failed.", tUnit));
        }
    
        if(err == TM_OK)
        {
            /* Set AGC3 Adapt TOP */
            err = tmddTDA182I2SetRFAGC_Adapt_TOP(tUnit, pObj->Std_Array[pObj->StandardMode].AGC3_Adapt_TOP);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRFAGC_Adapt_TOP(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set AGC5 Atten 3dB */
            err = tmddTDA182I2SetRF_Atten_3dB(tUnit, pObj->Std_Array[pObj->StandardMode].AGC5_Atten_3dB);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRF_Atten_3dB(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set AGC5 Detector HPF */
            err = tmddTDA182I2SetAGC5_Ana(tUnit, pObj->Std_Array[pObj->StandardMode].AGC5_Detector_HPF);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC5_Ana(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set AGCK Mode */
            err = tmddTDA182I2SetAGCK_Mode(tUnit, pObj->Std_Array[pObj->StandardMode].GSK&0x03);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGCK_Mode(0x%08X) failed.", tUnit));

            err = tmddTDA182I2SetAGCK_Step(tUnit, (pObj->Std_Array[pObj->StandardMode].GSK&0x0C)>>2);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGCK_Step(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set H3H5 VHF Filter 6 */
            err = tmddTDA182I2SetPSM_StoB(tUnit, pObj->Std_Array[pObj->StandardMode].H3H5_VHF_Filter6);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetPSM_StoB(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set IF */
            err = tmddTDA182I2SetIF_Freq(tUnit, pObj->Std_Array[pObj->StandardMode].IF - pObj->Std_Array[pObj->StandardMode].CF_Offset);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIF_Freq(0x%08X) failed.", tUnit));
        }

        /* ---------- ES2 PATCH BEGIN ---------- */
        if(err == TM_OK)
        {
            err = tmddTDA182I2GetMajorRevision(tUnit, &majorRevision);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetMajorRevision(0x%08X) failed.", tUnit));

            err = tmddTDA182I2GetMinorRevision(tUnit, &minorRevision);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetMinorRevision(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK && majorRevision == 1 && minorRevision == 0)
        {
            /* Set LPF Gain */
            err = tmddTDA182I2SetLPF_Gain_Mode(tUnit, pObj->Std_Array[pObj->StandardMode].LPF_Gain);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetLPF_Gain_Mode(0x%08X) failed.", tUnit));
        }
        /* ---------- ES2 PATCH END ---------- */

        if(err == TM_OK)
        {
            /* Set RF */
            err = tmddTDA182I2SetRF_Freq(tUnit, uRF + pObj->Std_Array[pObj->StandardMode].CF_Offset);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRF_Freq(0x%08X) failed.", tUnit));
            
            /* PATCH ES2 begin : in case of timeout WAIT IRQ */
            if(err == ddTDA182I2_ERR_NOT_READY  && bIRQWait && majorRevision == 1 && minorRevision == 0)
            {
                err = tmddTDA182I2Reset(tUnit);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Reset(0x%08X) failed.", tUnit));

                err = tmddTDA182I2SetRF_Freq(tUnit, uRF + pObj->Std_Array[pObj->StandardMode].CF_Offset);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRF_Freq(0x%08X) failed.", tUnit));
            }
            /* PATCH ES2 end */
        }
        if(err == TM_OK)
        {
            /*  patch Spurious begin */
            ratioL = (UInt8)(uRF / 16000000);
            ratioH = (UInt8)(uRF / 16000000) + 1;
            DeltaL = (uRF - (ratioL * 16000000));
            DeltaH = ((ratioH * 16000000) - uRF);

            if (uRF < 72000000 )
            {
                /* Set sigma delta clock*/
                err = tmddTDA182I2SetDigital_Clock_Mode(tUnit, 1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetDigital_Clock_Mode(0x%08X, sigma delta clock) failed.", tUnit));                    
            }
            else if (uRF < 104000000 )
            {
                /* Set 16 Mhz Xtal clock */
                err = tmddTDA182I2SetDigital_Clock_Mode(tUnit, 0);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetDigital_Clock_Mode(0x%08X, 16 Mhz xtal clock) failed.", tUnit));
            }
            else if (uRF <= 120000000 )
            {
                /* Set sigma delta clock*/
                err = tmddTDA182I2SetDigital_Clock_Mode(tUnit, 1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetDigital_Clock_Mode(0x%08X, sigma delta clock) failed.", tUnit));                    
            }
            else  /* RF above 120 MHz */
            {
                if  (DeltaL <= DeltaH )  
                {
                    if (ratioL & 0x000001 )  /* ratioL odd */
                    {
                        /* Set 16 Mhz Xtal clock */
                        err = tmddTDA182I2SetDigital_Clock_Mode(tUnit, 0);
                        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetDigital_Clock_Mode(0x%08X, 16 Mhz xtal clock) failed.", tUnit));
                    }
                    else /* ratioL even */
                    {
                        /* Set sigma delta clock*/
                        err = tmddTDA182I2SetDigital_Clock_Mode(tUnit, 1);
                        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetDigital_Clock_Mode(0x%08X, sigma delta clock) failed.", tUnit));
                    }
                    
                }
                else  /* (DeltaL > DeltaH ) */
                {
                    if (ratioL & 0x000001 )  /*(ratioL odd)*/
                    {
                        /* Set sigma delta clock*/
                        err = tmddTDA182I2SetDigital_Clock_Mode(tUnit, 1);
                        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetDigital_Clock_Mode(0x%08X, sigma delta clock) failed.", tUnit));
                    }
                    else
                    {
                        /* Set 16 Mhz Xtal clock */
                        err = tmddTDA182I2SetDigital_Clock_Mode(tUnit, 0);
                        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetDigital_Clock_Mode(0x%08X, 16 Mhz xtal clock) failed.", tUnit));
                    }
                }
            }
        }
        /*  patch Spurious end */
        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}


//-------------------------------------------------------------------------------------
// FUNCTION:    tmbslTDA182I2GetRf:
//
// DESCRIPTION: Get the frequency programmed in the tuner
//
// RETURN:      TMBSL_ERR_TUNER_BAD_UNIT_NUMBER
//              TDA182I2_ERR_NOT_INITIALIZED
//              TM_OK
//
// NOTES:       The value returned is the one stored in the Object
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmbslTDA182I2GetRf
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32*         puRF    /* O: RF frequency in hertz */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err  = TM_OK;

    if(puRF == Null)
        err = TDA182I2_ERR_BAD_PARAMETER;

    //------------------------------
    // test input parameters
    //------------------------------
    // pObj initialization
    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Get RF */
        *puRF = pObj->uRF/* - pObj->Std_Array[pObj->StandardMode].CF_Offset*/;

        (void)TDA182I2MutexRelease(pObj);
    }
    return err;
}

/*============================================================================*/
/* tmbslTDA182I2Reset                                                         */
/*============================================================================*/
tmErrorCode_t
tmbslTDA182I2Reset
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;
    Bool                bIRQWait = True;
    /* ---------- ES2 PATCH BEGIN ---------- */
    UInt8               majorRevision = 0;
    UInt8               minorRevision = 0;
    /* ---------- ES2 PATCH END ---------- */

    //------------------------------
    // test input parameters
    //------------------------------
    // pObj initialization
    err = TDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = tmddTDA182I2GetIRQWait(tUnit, &bIRQWait);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetIRQWait(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2Init(tUnit);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2Init(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = tmddTDA182I2GetMajorRevision(tUnit, &majorRevision);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetMajorRevision(0x%08X) failed.", tUnit));

        err = tmddTDA182I2GetMinorRevision(tUnit, &minorRevision);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetMinorRevision(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        // initialize tuner
        err =  tmddTDA182I2Reset(tUnit);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Reset(0x%08X) failed.", tUnit));


        /* PATCH ES2 begin : in case of timeout WAIT IRQ */
        if(err == ddTDA182I2_ERR_NOT_READY && bIRQWait && majorRevision == 1 && minorRevision == 0)
        {
            err = tmddTDA182I2Reset(tUnit);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Reset(0x%08X) failed.", tUnit));
        }
        /* PATCH ES2 end */

        /****** Begin settings V3.0.1 *********/
        
#ifdef TMBSL_TDA18272
        /* LT_Enable  N     0*/
        if (err == TM_OK)
        {
            err = tmddTDA182I2SetLT_Enable(tUnit, 0 );
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetLT_Enable(0x%08X, 0) failed.", tUnit));
        }
        /* PSM AGC1   -5,9 mA     1*/
        if (err == TM_OK)
        {
            err = tmddTDA182I2SetPSM_AGC1(tUnit, 1 );
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetPSM_AGC1(0x%08X, 1) failed.", tUnit));
        }

        /* AGC1 gain range  [-12, +15dB]      0*/
        if (err == TM_OK)
        {
            err = tmddTDA182I2SetAGC1_6_15dB(tUnit, 0 );
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC1_6_15dB(0x%08X, 0) failed.", tUnit));
        }
        /****** End settings V3.0.1 *********/
#endif


    }
    (void)TDA182I2MutexRelease(pObj);

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2GetIF                                                         */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2GetIF
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32*         puIF    /* O: IF Frequency in hertz */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    if(puIF == Null)
        err = TDA182I2_ERR_BAD_PARAMETER;
    
    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        *puIF = pObj->Std_Array[pObj->StandardMode].IF - pObj->Std_Array[pObj->StandardMode].CF_Offset;

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2GetCF_Offset                                                  */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2GetCF_Offset(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32*         puOffset    /* O: Center frequency offset in hertz */
    )
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    if(puOffset == Null)
        err = TDA182I2_ERR_BAD_PARAMETER;
    
    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        *puOffset = pObj->Std_Array[pObj->StandardMode].CF_Offset;

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2GetLockStatus                                                 */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2GetLockStatus
(
    tmUnitSelect_t          tUnit,      /* I: Unit number */
    tmbslFrontEndState_t*   pLockStatus /* O: PLL Lock status */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;
    UInt8 uValue, uValueLO;

    if(pLockStatus == Null)
        err = TDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }
    if(err == TM_OK)
    {
        err =  tmddTDA182I2GetLO_Lock(tUnit, &uValueLO);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetLO_Lock(0x%08X) failed.", tUnit));
    }
    if(err == TM_OK)
    {
        err =  tmddTDA182I2GetIRQ_status(tUnit, &uValue);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetIRQ_status(0x%08X) failed.", tUnit));

        uValue = uValue & uValueLO;
    }
    if(err == TM_OK)
    {
        *pLockStatus =  (uValue)? tmbslFrontEndStateLocked : tmbslFrontEndStateNotLocked;
    }
    else
    {
        *pLockStatus = tmbslFrontEndStateUnknown;  
    }

    (void)TDA182I2MutexRelease(pObj);

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2GetPowerLevel                                                 */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2GetPowerLevel
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32*         pPowerLevel /* O: Power Level in dBµV */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    if(pPowerLevel == Null)
        err = TDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        *pPowerLevel = 0;

        err = tmddTDA182I2GetPower_Level(tUnit, (UInt8 *)pPowerLevel);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetPower_Level(0x%08X) failed.", tUnit));

        (void)TDA182I2MutexRelease(pObj);
    }
    return err;
}

/*============================================================================*/
/* tmbslTDA182I2SetIRQWait                                                  */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2SetIRQWait
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool            bWait   /* I: Determine if we need to wait IRQ in driver functions */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    err = TDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = tmddTDA182I2SetIRQWait(tUnit, bWait);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIRQWait(0x%08X) failed.", tUnit));

        (void)TDA182I2MutexRelease(pObj);
    }
    return err;
}

/*============================================================================*/
/* tmbslTDA182I2GetIRQWait                                                  */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2GetIRQWait
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool*           pbWait  /* O: Determine if we need to wait IRQ in driver functions */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    if(pbWait == Null)
        err = TDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = tmddTDA182I2GetIRQWait(tUnit, pbWait);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetIRQWait(0x%08X) failed.", tUnit));

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2GetIRQ                                                        */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2GetIRQ
(
    tmUnitSelect_t  tUnit  /* I: Unit number */,
    Bool*           pbIRQ  /* O: IRQ triggered */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    if(pbIRQ == Null)
        err = TDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        *pbIRQ = 0;

        err = tmddTDA182I2GetIRQ_status(tUnit, (UInt8 *)pbIRQ);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetIRQ_status(0x%08X) failed.", tUnit));

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2WaitIRQ                                                       */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2WaitIRQ
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32          timeOut,    /* I: timeOut for IRQ wait */
    UInt32          waitStep,   /* I: wait step */
    UInt8           irqStatus   /* I: IRQs to wait */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = tmddTDA182I2WaitIRQ(tUnit, timeOut, waitStep, irqStatus);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2WaitIRQ(0x%08X) failed.", tUnit));

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2GetXtalCal_End                                                */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2GetXtalCal_End
(
    tmUnitSelect_t  tUnit  /* I: Unit number */,
    Bool*           pbXtalCal_End  /* O: XtalCal_End triggered */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    if(pbXtalCal_End == Null)
        err = TDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        *pbXtalCal_End = 0;

        err = tmddTDA182I2GetMSM_XtalCal_End(tUnit, (UInt8 *)pbXtalCal_End);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetMSM_XtalCal_End(0x%08X) failed.", tUnit));

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2WaitXtalCal_End                                               */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2WaitXtalCal_End
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32          timeOut,    /* I: timeOut for IRQ wait */
    UInt32          waitStep   /* I: wait step */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = tmddTDA182I2WaitXtalCal_End(tUnit, timeOut, waitStep);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2WaitXtalCal_End(0x%08X) failed.", tUnit));

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2SoftReset                                                     */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2SoftReset
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = tmddTDA182I2SoftReset(tUnit);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SoftReset(0x%08X) failed.", tUnit));

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2CheckRFFilterRobustness                                       */
/*============================================================================*/
/*
tmErrorCode_t
tmbslTDA182I2CheckRFFilterRobustness 
(
 tmUnitSelect_t                         tUnit,      // I: Unit number
 ptmTDA182I2RFFilterRating              rating      // O: RF Filter rating
 )
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err = TM_OK;

    if(err == TM_OK)
    {
        err = TDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        UInt8  rfcal_log_0 = 0;
        UInt8  rfcal_log_2 = 0;
        UInt8  rfcal_log_3 = 0;
        UInt8  rfcal_log_5 = 0;
        UInt8  rfcal_log_6 = 0;
        UInt8  rfcal_log_8 = 0;
        UInt8  rfcal_log_9 = 0;
        UInt8  rfcal_log_11 = 0;

        double   VHFLow_0 = 0;
        double   VHFLow_1 = 0;
        double   VHFHigh_0 = 0;
        double   VHFHigh_1 = 0;
        double   UHFLow_0 = 0;
        double   UHFLow_1 = 0;
        double   UHFHigh_0 = 0;
        double   UHFHigh_1 = 0;

        err = tmddTDA182I2Getrfcal_log_0(tUnit, &rfcal_log_0);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Getrfcal_log_0(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            err = tmddTDA182I2Getrfcal_log_2(tUnit, &rfcal_log_2);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Getrfcal_log_2(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = tmddTDA182I2Getrfcal_log_3(tUnit, &rfcal_log_3);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Getrfcal_log_3(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = tmddTDA182I2Getrfcal_log_5(tUnit, &rfcal_log_5);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Getrfcal_log_5(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = tmddTDA182I2Getrfcal_log_6(tUnit, &rfcal_log_6);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Getrfcal_log_6(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = tmddTDA182I2Getrfcal_log_8(tUnit, &rfcal_log_8);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Getrfcal_log_8(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = tmddTDA182I2Getrfcal_log_9(tUnit, &rfcal_log_9);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Getrfcal_log_9(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = tmddTDA182I2Getrfcal_log_11(tUnit, &rfcal_log_11);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Getrfcal_log_11(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        { 
            // VHFLow_0
            VHFLow_0 = 100 * (45 - 39.8225 * (1 + (0.31 * (rfcal_log_0 < 64 ? rfcal_log_0 : rfcal_log_0 - 128)) / 1.0 / 100.0)) / 45.0;
            rating->VHFLow_0_Margin = 0.0024 * VHFLow_0 * VHFLow_0 * VHFLow_0 - 0.101 * VHFLow_0 * VHFLow_0 + 1.629 * VHFLow_0 + 1.8266;
            if (rating->VHFLow_0_Margin >= 0) 
            {
                rating->VHFLow_0_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_High;
            }
            else
            {
                rating->VHFLow_0_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_Low;
            }

            // VHFLow_1
            VHFLow_1 = 100 * (152.1828 * (1 + (1.53 * (rfcal_log_2 < 64 ? rfcal_log_2 : rfcal_log_2 - 128)) / 1.0 / 100.0) - (144.896 - 6)) / (144.896 - 6); 
            rating->VHFLow_1_Margin = 0.0024 * VHFLow_1 * VHFLow_1 * VHFLow_1 - 0.101 * VHFLow_1 * VHFLow_1 + 1.629 * VHFLow_1 + 1.8266;
            if (rating->VHFLow_1_Margin >= 0)
            {
                rating->VHFLow_1_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_High;
            }
            else
            {
                rating->VHFLow_1_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_Low;
            }

            // VHFHigh_0    
            VHFHigh_0 = 100 * ((144.896 + 6) - 135.4063 * (1 + (0.27 * (rfcal_log_3 < 64 ? rfcal_log_3 : rfcal_log_3 - 128)) / 1.0 / 100.0)) / (144.896 + 6);
            rating->VHFHigh_0_Margin = 0.0024 * VHFHigh_0 * VHFHigh_0 * VHFHigh_0 - 0.101 * VHFHigh_0 * VHFHigh_0 + 1.629 * VHFHigh_0 + 1.8266;
            if (rating->VHFHigh_0_Margin >= 0)
            {
                rating->VHFHigh_0_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_High;
            }
            else
            {
                rating->VHFHigh_0_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_Low;
            }

            // VHFHigh_1
            VHFHigh_1 = 100 * (383.1455 * (1 + (0.91 * (rfcal_log_5 < 64 ? rfcal_log_5 : rfcal_log_5 - 128)) / 1.0 / 100.0) - (367.104 - 8)) / (367.104 - 8);
            rating->VHFHigh_1_Margin = 0.0024 * VHFHigh_1 * VHFHigh_1 * VHFHigh_1 - 0.101 * VHFHigh_1 * VHFHigh_1 + 1.629 * VHFHigh_1 + 1.8266;
            if (rating->VHFHigh_1_Margin >= 0)
            {
                rating->VHFHigh_1_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_High;
            }
            else
            {
                rating->VHFHigh_1_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_Low;
            }

            // UHFLow_0
            UHFLow_0 = 100 * ((367.104 + 8) - 342.6224 * (1 + (0.21 * (rfcal_log_6 < 64 ? rfcal_log_6 : rfcal_log_6 - 128)) / 1.0 / 100.0)) / (367.104 + 8);
            rating->UHFLow_0_Margin = 0.0024 * UHFLow_0 * UHFLow_0 * UHFLow_0 - 0.101 * UHFLow_0 * UHFLow_0 + 1.629 * UHFLow_0 + 1.8266;
            if (rating->UHFLow_0_Margin >= 0)
            {
                rating->UHFLow_0_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_High;
            }
            else
            {
                rating->UHFLow_0_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_Low;
            }

            // UHFLow_1
            UHFLow_1 = 100 * (662.5595 * (1 + (0.33 * (rfcal_log_8 < 64 ? rfcal_log_8 : rfcal_log_8 - 128)) / 1.0 / 100.0) - (624.128 - 2)) / (624.128 - 2);
            rating->UHFLow_1_Margin = 0.0024 * UHFLow_1 * UHFLow_1 * UHFLow_1 - 0.101 * UHFLow_1 * UHFLow_1 + 1.629 * UHFLow_1 + 1.8266;
            if (rating->UHFLow_1_Margin >= 0)
            {
                rating->UHFLow_1_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_High;
            }
            else
            {
                rating->UHFLow_1_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_Low;
            }

            // UHFHigh_0
            UHFHigh_0 = 100 * ((624.128 + 2) - 508.2747 * (1 + (0.23 * (rfcal_log_9 < 64 ? rfcal_log_9 : rfcal_log_9 - 128)) / 1.0 / 100.0)) / (624.128 + 2);
            rating->UHFHigh_0_Margin = 0.0024 * UHFHigh_0 * UHFHigh_0 * UHFHigh_0 - 0.101 * UHFHigh_0 * UHFHigh_0 + 1.629 * UHFHigh_0 + 1.8266;
            if (rating->UHFHigh_0_Margin >= 0)
            {
                rating->UHFHigh_0_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_High;
            }
            else
            {
                rating->UHFHigh_0_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_Low;
            }

            // UHFHigh_1
            UHFHigh_1 = 100 * (947.8913 * (1 + (0.3 * (rfcal_log_11 < 64 ? rfcal_log_11 : rfcal_log_11 - 128)) / 1.0 / 100.0) - (866 - 14)) / (866 - 14);
            rating->UHFHigh_1_Margin = 0.0024 * UHFHigh_1 * UHFHigh_1 * UHFHigh_1 - 0.101 * UHFHigh_1 * UHFHigh_1 + 1.629 * UHFHigh_1 + 1.8266;            
            if (rating->UHFHigh_1_Margin >= 0)
            {
                rating->UHFHigh_1_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_High;
            }
            else
            {
                rating->UHFHigh_1_RFFilterRobustness =  tmTDA182I2RFFilterRobustness_Low;
            }
        }

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}
*/

/*============================================================================*/
/* tmbslTDA182I2Write                                                         */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2Write
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32          uIndex,     /* I: Start index to write */
    UInt32          WriteLen,   /* I: Number of bytes to write */
    UInt8*          pData       /* I: Data to write */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err  = TM_OK;

    // pObj initialization
    err = TDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // Call tmddTDA182I2Write

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* tmbslTDA182I2Read                                                          */
/*============================================================================*/

tmErrorCode_t
tmbslTDA182I2Read
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32          uIndex,     /* I: Start index to read */
    UInt32          ReadLen,    /* I: Number of bytes to read */
    UInt8*          pData       /* I: Data to read */
)
{
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err  = TM_OK;

    // pObj initialization
    err = TDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = TDA182I2MutexAcquire(pObj, TDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // Call tmddTDA182I2Read

        (void)TDA182I2MutexRelease(pObj);
    }

    return err;
}

//-----------------------------------------------------------------------------
// Internal functions:
//-----------------------------------------------------------------------------
//

//-----------------------------------------------------------------------------
// FUNCTION:    TDA182I2Init:
//
// DESCRIPTION: initialization of the Tuner
//
// RETURN:      always True
//
// NOTES:       
//-----------------------------------------------------------------------------
//
static tmErrorCode_t
TDA182I2Init
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{     
    ptmTDA182I2Object_t pObj = Null;
    tmErrorCode_t       err  = TM_OK;

    //------------------------------
    // test input parameters
    //------------------------------
    // pObj initialization
    err = TDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "TDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        //err = tmddTDA182I2SetIRQWait(tUnit, True);

        //if(pObj->bIRQWait)
        //{
        //    err = TDA182I2WaitIRQ(pObj);
        //}
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    TDA182I2Wait
//
// DESCRIPTION: This function waits for requested time
//
// RETURN:      True or False
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
static tmErrorCode_t 
TDA182I2Wait
(
    ptmTDA182I2Object_t pObj,   /* I: Driver object */
    UInt32              Time   /*  I: Time to wait for */
)
{
    tmErrorCode_t   err  = TM_OK;

    // wait Time ms
    err = POBJ_SRVFUNC_STIME.Wait(pObj->tUnit, Time);

    // Return value
    return err;
}

/*============================================================================*/
/* TDA182I2MutexAcquire                                                       */
/*============================================================================*/
extern tmErrorCode_t
TDA182I2MutexAcquire
(
 ptmTDA182I2Object_t    pObj,
 UInt32                 timeOut
 )
{
    tmErrorCode_t       err = TM_OK;

    if(pObj->sMutex.Acquire != Null && pObj->pMutex != Null)
    {
        err = pObj->sMutex.Acquire(pObj->pMutex, timeOut);
    }

    return err;
}

/*============================================================================*/
/* TDA182I2MutexRelease                                                       */
/*============================================================================*/
extern tmErrorCode_t
TDA182I2MutexRelease
(
 ptmTDA182I2Object_t    pObj
 )
{
    tmErrorCode_t       err = TM_OK;

    if(pObj->sMutex.Release != Null && pObj->pMutex != Null)
    {
        err = pObj->sMutex.Release(pObj->pMutex);
    }

    return err;
}























// NXP source code - .\tmbslTDA182I2\src\tmbslTDA182I2Instance.c


//-----------------------------------------------------------------------------
// $Header: 
// (C) Copyright 2001 NXP Semiconductors, All rights reserved
//
// This source code and any compilation or derivative thereof is the sole
// property of NXP Corporation and is provided pursuant to a Software
// License Agreement.  This code is the proprietary information of NXP
// Corporation and is confidential in nature.  Its use and dissemination by
// any party other than NXP Corporation is strictly limited by the
// confidential information provisions of the Agreement referenced above.
//-----------------------------------------------------------------------------
// FILE NAME:    tmbslTDA182I2Instance.c
//
// DESCRIPTION:  define the static Objects
//
// DOCUMENT REF: DVP Software Coding Guidelines v1.14
//               DVP Board Support Library Architecture Specification v0.5
//
// NOTES:        
//-----------------------------------------------------------------------------
//

//#include "tmNxTypes.h"
//#include "tmCompId.h"
//#include "tmFrontEnd.h"
//#include "tmUnitParams.h"
//#include "tmbslFrontEndTypes.h"

//#ifdef TMBSL_TDA18272
// #include "tmbslTDA18272.h"
//#else /* TMBSL_TDA18272 */
// #include "tmbslTDA18212.h"
//#endif /* TMBSL_TDA18272 */

//#include "tmbslTDA182I2local.h"
//#include "tmbslTDA182I2Instance.h"

//-----------------------------------------------------------------------------
// Global data:
//-----------------------------------------------------------------------------
//
//
// default instance
tmTDA182I2Object_t gTDA182I2Instance[] = 
{
    {
        (tmUnitSelect_t)(-1),                                   /* tUnit */
        (tmUnitSelect_t)(-1),                                   /* tUnit temporary */
        Null,                                                   /* pMutex */
        False,                                                  /* init (instance initialization default) */
        {                                                       /* sRWFunc */
            Null,
            Null
        },
        {                                                       /* sTime */
            Null,
            Null
        },
        {                                                       /* sDebug */
            Null
        },
        {                                                       /* sMutex */
            Null,
            Null,
            Null,
            Null
        },
        tmTDA182I2_PowerStandbyWithXtalOn,                      /* Power state */
        0,                                                      /* RF */
        tmTDA182I2_DVBT_8MHz,                                   /* Standard mode */
        {
            {   /* Std_Array */                                 /* DVB-T 6MHz */                               
                3250000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_6MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_0_4MHz,                       /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_102dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_98dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   // DVB-T 7MHz */
                3500000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_7MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_min_8pc,                   /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_102dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_98dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   /* DVB-T 8MHz */
                4000000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_102dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_98dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
#ifdef TMBSL_TDA18272
            {                                                   /* Analog M/N */
                5400000,                                        /* IF */
                1750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_6MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */

                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog B */
                6400000,                                        /* IF */
                2250000,                                        /* CF_Offset */
                tmTDA182I2_LPF_7MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band*/
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog G/H */
                6750000,                                        /* IF */
                2750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog I */
                7250000,                                        /* IF */
                2750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog D/K */
                6850000,                                        /* IF */
                2750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog L */
                6750000,                                        /* IF */
                2750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog L' */
                1250000,                                        /* IF */
                -2750000,                                       /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog FM Radio */
                1250000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_1_5MHz,                          /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_0_85MHz,                      /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Blind Scanning copy of PAL-I */
                7250000,                                        /* IF */
                2750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
#endif
            {                                                   /* QAM 6MHz */
                3600000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_6MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_min_8pc,                   /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   /* QAM 8MHz */
                5000000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_9MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_min_8pc,                   /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_0_85MHz,                      /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   /* ISDBT 6MHz */
                3250000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_6MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_6Vpp_min_10_3_19_7dB,  /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_0_4MHz,                       /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_102dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_98dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   /* ATSC 6MHz */
                3250000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_6MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_6Vpp_min_10_3_19_7dB,  /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_0_4MHz,                       /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d100_u94dBuV,           /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_104dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_104dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d112_u107dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d112_u107dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_96dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   /* DMB-T 8MHz */
                4000000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_102dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_98dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            }
        }
    },
    {
        (tmUnitSelect_t)(-1),                                   /* tUnit */
        (tmUnitSelect_t)(-1),                                   /* tUnit temporary */
        Null,                                                   /* pMutex */
        False,                                                  /* init (instance initialization default) */
        {                                                       /* sRWFunc */
            Null,
            Null
        },
        {                                                       /* sTime */
            Null,
            Null
        },
        {                                                       /* sDebug */
            Null
        },
        {                                                       /* sMutex */
            Null,
            Null,
            Null,
            Null
        },
        tmTDA182I2_PowerStandbyWithXtalOn,                      /* Power state */
        0,                                                      /* RF */
        tmTDA182I2_DVBT_8MHz,                                   /* Standard mode */
        {
            {   /* Std_Array */                                 /* DVB-T 6MHz */                               
                3250000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_6MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_0_4MHz,                       /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_102dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_98dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   // DVB-T 7MHz */
                3500000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_7MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_min_8pc,                   /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_102dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_98dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   /* DVB-T 8MHz */
                4000000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_102dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_98dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
#ifdef TMBSL_TDA18272
            {                                                   /* Analog M/N */
                5400000,                                        /* IF */
                1750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_6MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog B */
                6400000,                                        /* IF */
                2250000,                                        /* CF_Offset */
                tmTDA182I2_LPF_7MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog G/H */
                6750000,                                        /* IF */
                2750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog I */
                7250000,                                        /* IF */
                2750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog D/K */
                6850000,                                        /* IF */
                2750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog L */
                6750000,                                        /* IF */
                2750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog L' */
                1250000,                                        /* IF */
                -2750000,                                       /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Analog FM Radio */
                1250000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_1_5MHz,                          /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_0_85MHz,                      /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
            {                                                   /* Blind Scanning copy of PAL-I */
                7250000,                                        /* IF */
                2750000,                                        /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_7Vpp_min_9_21dB,       /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Disabled,                   /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_96dBuV,              /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d105_u100dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d105_u100dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Enabled,           /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x01,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Frozen                      /* LPF_Gain */
            },
#endif
            {                                                   /* QAM 6MHz */
                3600000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_6MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_min_8pc,                   /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   /* QAM 8MHz */
                5000000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_9MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_min_8pc,                   /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Disabled,                   /* IF_Notch */
                tmTDA182I2_IF_HPF_0_85MHz,                      /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Disabled,                 /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_102dBuV,              /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Disabled,             /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Disabled,           /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   /* ISDBT 6MHz */
                3250000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_6MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_6Vpp_min_10_3_19_7dB,  /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_0_4MHz,                       /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_102dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_98dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   /* ATSC 6MHz */
                3250000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_6MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_0_6Vpp_min_10_3_19_7dB,  /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_0_4MHz,                       /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d100_u94dBuV,           /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_104dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_104dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d112_u107dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d112_u107dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_96dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            },
            {                                                   /* DMB-T 8MHz */
                4000000,                                        /* IF */
                0,                                              /* CF_Offset */
                tmTDA182I2_LPF_8MHz,                            /* LPF */
                tmTDA182I2_LPFOffset_0pc,                       /* LPF_Offset */
                tmTDA182I2_IF_AGC_Gain_1Vpp_min_6_24dB,         /* IF_Gain */
                tmTDA182I2_IF_Notch_Enabled,                    /* IF_Notch */
                tmTDA182I2_IF_HPF_Disabled,                     /* IF_HPF */
                tmTDA182I2_DC_Notch_Enabled,                    /* DC_Notch */
                tmTDA182I2_AGC1_LNA_TOP_d95_u89dBuV,            /* AGC1_LNA_TOP */
                tmTDA182I2_AGC2_RF_Attenuator_TOP_d90_u84dBuV,  /* AGC2_RF_Attenuator_TOP */
                tmTDA182I2_AGC3_RF_AGC_TOP_100dBuV,             /* AGC3_RF_AGC_TOP_Low_band */
                tmTDA182I2_AGC3_RF_AGC_TOP_102dBuV,             /* AGC3_RF_AGC_TOP_High_band */
                tmTDA182I2_AGC4_IR_Mixer_TOP_d110_u105dBuV,     /* AGC4_IR_Mixer_TOP */
                tmTDA182I2_AGC5_IF_AGC_TOP_d110_u105dBuV,       /* AGC5_IF_AGC_TOP */
                tmTDA182I2_AGC5_Detector_HPF_Disabled,          /* AGC5_Detector_HPF */
                tmTDA182I2_AGC3_Adapt_Enabled,                  /* AGC3_Adapt */
                tmTDA182I2_AGC3_Adapt_TOP_98dBuV,               /* AGC3_Adapt_TOP */
                tmTDA182I2_AGC5_Atten_3dB_Enabled,              /* AGC5_Atten_3dB */
                0x02,                                           /* GSK : settings V2.0.0  */
                tmTDA182I2_H3H5_VHF_Filter6_Enabled,            /* H3H5_VHF_Filter6 */
                tmTDA182I2_LPF_Gain_Free                        /* LPF_Gain */
            }
        }
    }
};


//-----------------------------------------------------------------------------
// FUNCTION:    TDA182I2AllocInstance:
//
// DESCRIPTION: allocate new instance
//
// RETURN:      
//
// NOTES:       
//-----------------------------------------------------------------------------
//
tmErrorCode_t
TDA182I2AllocInstance
(
    tmUnitSelect_t          tUnit,      /* I: Unit number */
    pptmTDA182I2Object_t    ppDrvObject /* I: Device Object */
)
{ 
    tmErrorCode_t       err = TDA182I2_ERR_BAD_UNIT_NUMBER;
    ptmTDA182I2Object_t pObj = Null;
    UInt32              uLoopCounter = 0;    

    /* Find a free instance */
    for(uLoopCounter = 0; uLoopCounter<TDA182I2_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gTDA182I2Instance[uLoopCounter];
        if(pObj->init == False)
        {
            pObj->tUnit = tUnit;
            pObj->tUnitW = tUnit;

            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    /* return value */
    return err;
}

//-----------------------------------------------------------------------------
// FUNCTION:    TDA182I2DeAllocInstance:
//
// DESCRIPTION: deallocate instance
//
// RETURN:      always TM_OK
//
// NOTES:       
//-----------------------------------------------------------------------------
//
tmErrorCode_t
TDA182I2DeAllocInstance
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{     
    tmErrorCode_t       err = TDA182I2_ERR_BAD_UNIT_NUMBER;
    ptmTDA182I2Object_t pObj = Null;

    /* check input parameters */
    err = TDA182I2GetInstance(tUnit, &pObj);

    /* check driver state */
    if (err == TM_OK)
    {
        if (pObj == Null || pObj->init == False)
        {
            err = TDA182I2_ERR_NOT_INITIALIZED;
        }
    }

    if ((err == TM_OK) && (pObj != Null)) 
    {
        pObj->init = False;
    }

    /* return value */
    return err;
}

//-----------------------------------------------------------------------------
// FUNCTION:    TDA182I2GetInstance:
//
// DESCRIPTION: get the instance
//
// RETURN:      always True
//
// NOTES:       
//-----------------------------------------------------------------------------
//
tmErrorCode_t
TDA182I2GetInstance
(
    tmUnitSelect_t          tUnit,      /* I: Unit number */
    pptmTDA182I2Object_t    ppDrvObject /* I: Device Object */
)
{     
    tmErrorCode_t       err = TDA182I2_ERR_NOT_INITIALIZED;
    ptmTDA182I2Object_t pObj = Null;
    UInt32              uLoopCounter = 0;    

    /* get instance */
    for(uLoopCounter = 0; uLoopCounter<TDA182I2_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gTDA182I2Instance[uLoopCounter];
        if(pObj->init == True && pObj->tUnit == GET_INDEX_TYPE_TUNIT(tUnit))
        {
            pObj->tUnitW = tUnit;
            
            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    /* return value */
    return err;
}























// NXP source code - .\tmddTDA182I2\src\tmddTDA182I2.c


/*
  Copyright (C) 2006-2009 NXP B.V., All Rights Reserved.
  This source code and any compilation or derivative thereof is the proprietary
  information of NXP B.V. and is confidential in nature. Under no circumstances
  is this software to be  exposed to or placed under an Open Source License of
  any type without the expressed written permission of NXP B.V.
 *
 * \file          tmddTDA182I2.c
 *
 *                3
 *
 * \date          %modify_time%
 *
 * \brief         Describe briefly the purpose of this file.
 *
 * REFERENCE DOCUMENTS :
 *                TDA18254_Driver_User_Guide.pdf
 *
 * Detailed description may be added here.
 *
 * \section info Change Information
 *
*/

/*============================================================================*/
/* Standard include files:                                                    */
/*============================================================================*/
//#include "tmNxTypes.h"
//#include "tmCompId.h"
//#include "tmFrontEnd.h"
//#include "tmbslFrontEndTypes.h"
//#include "tmUnitParams.h"

/*============================================================================*/
/* Project include files:                                                     */
/*============================================================================*/
//#include "tmddTDA182I2.h"
//#include <tmddTDA182I2local.h>

//#include "tmddTDA182I2Instance.h"

/*============================================================================*/
/* Types and defines:                                                         */
/*============================================================================*/

/*============================================================================*/
/* Global data:                                                               */
/*============================================================================*/

typedef struct _tmTDA182I2_CAL_Map_t {
    UInt8   Cal_number;
    UInt32  Cal_Freq[4];
    Int8    CprogOffset_Val[4];
} tmTDA182I2_CAL_Map_t, *ptmTDA182I2_CAL_Map_t;

typedef struct _tmTDA182I2_Freq_ROM_Map_t {
    UInt32  uRF_Max;
    UInt8   uCprog;
    UInt8   uGainTapper;
    UInt8   uRFBand;
} tmTDA182I2_Freq_ROM_Map_t, *ptmTDA182I2_Freq_ROM_Map_t;


#define TDA182I2_FREQ_ROM_MAP_ROW (565)

/* cal number / freq for index = 0 / freq for index = 1 / freq for index = 2 / freq for index = 3 / cprog_delta / cprog_offset / cprog_offset_0 / cprog_offset_1 / cprog_offset_2 / cprog_offset_3 */
static tmTDA182I2_CAL_Map_t TDA182I2_Cal_Map[TDA182I2_RFCAL_PROG_ROW] =
{
    { 0, { 52224000, 56320000, 44032000, 48128000 }, { 15, 0, 10, 17 } },
    { 1, { 93184000, 97280000, 84992000, 89088000 }, { 1, 0, -2, 3 } },
    { 2, { 115712000, 123904000, 106496000, 111616000 }, { 0, -1, 1, 2 } },
    { 3, { 169984000, 174080000, 161792000, 165888000 }, { 3, 0, 1, 2 } },
    { 4, { 232448000, 235520000, 224256000, 228352000 }, { 3, 0, 1, 2 } },
    { 5, { 322560000, 335872000, 301056000, 312320000 }, { 0, -1, 1, 2 } },
    { 6, { 397312000, 401408000, 389120000, 393216000 }, { -2, 0, -1, 1 } },
    { 7, { 465920000, 471040000, 455680000, 460800000 }, { 0, -2, -3, 1 } },
    { 8, { 570368000, 577536000, 555008000, 563200000 }, { -1, 0, -3, -2 } },
    { 9, { 658432000, 662528000, 647168000, 652288000 }, { -6, -3, 0, -5 } },
    { 10, { 762880000, 770048000, 748544000, 755712000 }, { -6, -3, 0, -5 } },
    { 11, { 809984000, 818176000, 792576000, 801792000 }, { -5, -2, 0, -4 } }
};

static tmTDA182I2_Freq_ROM_Map_t TDA182I2_Freq_ROM_Map[TDA182I2_FREQ_ROM_MAP_ROW] =
{
    { 39936000, 0xFF, 0x17, 0 },
    { 40960000, 0xFD, 0x17, 0 },
    { 41984000, 0xF1, 0x15, 0 },
    { 43008000, 0xE5, 0x13, 0 },
    { 44032000, 0xDB, 0x13, 0 },
    { 45056000, 0xD1, 0x12, 0 },
    { 46080000, 0xC7, 0x10, 0 },
    { 47104000, 0xBE, 0x0F, 0 },
    { 48128000, 0xB5, 0x0F, 0 },
    { 49152000, 0xAD, 0x0F, 0 },
    { 50176000, 0xA6, 0x0F, 0 },
    { 51200000, 0x9F, 0x0F, 0 },
    { 52224000, 0x98, 0x0F, 0 },
    { 53248000, 0x91, 0x0F, 0 },
    { 54272000, 0x8B, 0x0F, 0 },
    { 55296000, 0x86, 0x0F, 0 },
    { 56320000, 0x80, 0x0F, 0 },
    { 57344000, 0x7B, 0x0E, 0 },
    { 58368000, 0x76, 0x0E, 0 },
    { 59392000, 0x72, 0x0D, 0 },
    { 60416000, 0x6D, 0x0D, 0 },
    { 61440000, 0x69, 0x0C, 0 },
    { 62464000, 0x65, 0x0C, 0 },
    { 63488000, 0x61, 0x0B, 0 },
    { 64512000, 0x5E, 0x0B, 0 },
    { 64512000, 0x5A, 0x0B, 0 },
    { 65536000, 0x57, 0x0A, 0 },
    { 66560000, 0x54, 0x0A, 0 },
    { 67584000, 0x51, 0x09, 0 },
    { 68608000, 0x4E, 0x09, 0 },
    { 69632000, 0x4B, 0x09, 0 },
    { 70656000, 0x49, 0x08, 0 },
    { 71680000, 0x46, 0x08, 0 },
    { 72704000, 0x44, 0x08, 0 },
    { 73728000, 0x41, 0x07, 0 },
    { 74752000, 0x3F, 0x07, 0 },
    { 75776000, 0x3D, 0x07, 0 },
    { 76800000, 0x3B, 0x07, 0 },
    { 77824000, 0x39, 0x07, 0 },
    { 78848000, 0x37, 0x07, 0 },
    { 79872000, 0x35, 0x07, 0 },
    { 80896000, 0x33, 0x07, 0 },
    { 81920000, 0x32, 0x07, 0 },
    { 82944000, 0x30, 0x07, 0 },
    { 83968000, 0x2F, 0x07, 0 },
    { 84992000, 0x2D, 0x07, 0 },
    { 86016000, 0x2C, 0x07, 0 },
    { 87040000, 0x2A, 0x07, 0 },
    { 88064000, 0x29, 0x06, 0 },
    { 89088000, 0x27, 0x06, 0 },
    { 90112000, 0x26, 0x06, 0 },
    { 91136000, 0x25, 0x06, 0 },
    { 92160000, 0x24, 0x06, 0 },
    { 93184000, 0x22, 0x05, 0 },
    { 94208000, 0x21, 0x05, 0 },
    { 95232000, 0x20, 0x05, 0 },
    { 96256000, 0x1F, 0x05, 0 },
    { 97280000, 0x1E, 0x05, 0 },
    { 98304000, 0x1D, 0x05, 0 },
    { 99328000, 0x1C, 0x04, 0 },
    { 100352000, 0x1B, 0x04, 0 },
    { 101376000, 0x1A, 0x04, 0 },
    { 103424000, 0x19, 0x04, 0 },
    { 104448000, 0x18, 0x04, 0 },
    { 105472000, 0x17, 0x04, 0 },
    { 106496000, 0x16, 0x03, 0 },
    { 106496000, 0x15, 0x03, 0 },
    { 108544000, 0x14, 0x03, 0 },
    { 109568000, 0x13, 0x03, 0 },
    { 111616000, 0x12, 0x03, 0 },
    { 112640000, 0x11, 0x03, 0 },
    { 113664000, 0x11, 0x07, 0 },
    { 114688000, 0x10, 0x07, 0 },
    { 115712000, 0x0F, 0x07, 0 },
    { 117760000, 0x0E, 0x07, 0 },
    { 119808000, 0x0D, 0x06, 0 },
    { 121856000, 0x0C, 0x06, 0 },
    { 123904000, 0x0B, 0x06, 0 },
    { 125952000, 0x0A, 0x05, 0 },
    { 128000000, 0x09, 0x05, 0 },
    { 130048000, 0x08, 0x05, 0 },
    { 133120000, 0x07, 0x04, 0 },
    { 135168000, 0x06, 0x04, 0 },
    { 138240000, 0x05, 0x04, 0 },
    { 141312000, 0x04, 0x04, 0 },
    { 144384000, 0x03, 0x03, 0 },
    { 145408000, 0xE0, 0x3F, 1 },
    { 147456000, 0xDC, 0x37, 1 },
    { 148480000, 0xD9, 0x32, 1 },
    { 149504000, 0xD6, 0x2F, 1 },
    { 149504000, 0xD2, 0x2F, 1 },
    { 150528000, 0xCF, 0x2F, 1 },
    { 151552000, 0xCC, 0x2B, 1 },
    { 152576000, 0xC9, 0x27, 1 },
    { 153600000, 0xC5, 0x27, 1 },
    { 154624000, 0xC2, 0x25, 1 },
    { 155648000, 0xBF, 0x23, 1 },
    { 156672000, 0xBD, 0x20, 1 },
    { 157696000, 0xBA, 0x1F, 1 },
    { 158720000, 0xB7, 0x1F, 1 },
    { 159744000, 0xB4, 0x1F, 1 },
    { 160768000, 0xB1, 0x1F, 1 },
    { 161792000, 0xAF, 0x1F, 1 },
    { 162816000, 0xAC, 0x1F, 1 },
    { 163840000, 0xAA, 0x1F, 1 },
    { 164864000, 0xA7, 0x1F, 1 },
    { 165888000, 0xA5, 0x1F, 1 },
    { 166912000, 0xA2, 0x1F, 1 },
    { 167936000, 0xA0, 0x1F, 1 },
    { 168960000, 0x9D, 0x1F, 1 },
    { 169984000, 0x9B, 0x1F, 1 },
    { 171008000, 0x99, 0x1F, 1 },
    { 172032000, 0x97, 0x1E, 1 },
    { 173056000, 0x95, 0x1D, 1 },
    { 174080000, 0x92, 0x1C, 1 },
    { 175104000, 0x90, 0x1B, 1 },
    { 176128000, 0x8E, 0x1A, 1 },
    { 177152000, 0x8C, 0x19, 1 },
    { 178176000, 0x8A, 0x18, 1 },
    { 179200000, 0x88, 0x17, 1 },
    { 180224000, 0x86, 0x17, 1 },
    { 181248000, 0x84, 0x17, 1 },
    { 182272000, 0x82, 0x17, 1 },
    { 183296000, 0x81, 0x17, 1 },
    { 184320000, 0x7F, 0x17, 1 },
    { 185344000, 0x7D, 0x16, 1 },
    { 186368000, 0x7B, 0x15, 1 },
    { 187392000, 0x7A, 0x14, 1 },
    { 188416000, 0x78, 0x14, 1 },
    { 189440000, 0x76, 0x13, 1 },
    { 190464000, 0x75, 0x13, 1 },
    { 191488000, 0x73, 0x13, 1 },
    { 192512000, 0x71, 0x12, 1 },
    { 192512000, 0x70, 0x11, 1 },
    { 193536000, 0x6E, 0x11, 1 },
    { 194560000, 0x6D, 0x10, 1 },
    { 195584000, 0x6B, 0x10, 1 },
    { 196608000, 0x6A, 0x0F, 1 },
    { 197632000, 0x68, 0x0F, 1 },
    { 198656000, 0x67, 0x0F, 1 },
    { 199680000, 0x65, 0x0F, 1 },
    { 200704000, 0x64, 0x0F, 1 },
    { 201728000, 0x63, 0x0F, 1 },
    { 202752000, 0x61, 0x0F, 1 },
    { 203776000, 0x60, 0x0F, 1 },
    { 204800000, 0x5F, 0x0F, 1 },
    { 205824000, 0x5D, 0x0F, 1 },
    { 206848000, 0x5C, 0x0F, 1 },
    { 207872000, 0x5B, 0x0F, 1 },
    { 208896000, 0x5A, 0x0F, 1 },
    { 209920000, 0x58, 0x0F, 1 },
    { 210944000, 0x57, 0x0F, 1 },
    { 211968000, 0x56, 0x0F, 1 },
    { 212992000, 0x55, 0x0F, 1 },
    { 214016000, 0x54, 0x0F, 1 },
    { 215040000, 0x53, 0x0F, 1 },
    { 216064000, 0x52, 0x0F, 1 },
    { 217088000, 0x50, 0x0F, 1 },
    { 218112000, 0x4F, 0x0F, 1 },
    { 219136000, 0x4E, 0x0F, 1 },
    { 220160000, 0x4D, 0x0E, 1 },
    { 221184000, 0x4C, 0x0E, 1 },
    { 222208000, 0x4B, 0x0E, 1 },
    { 223232000, 0x4A, 0x0E, 1 },
    { 224256000, 0x49, 0x0D, 1 },
    { 225280000, 0x48, 0x0D, 1 },
    { 226304000, 0x47, 0x0D, 1 },
    { 227328000, 0x46, 0x0D, 1 },
    { 228352000, 0x45, 0x0C, 1 },
    { 229376000, 0x44, 0x0C, 1 },
    { 230400000, 0x43, 0x0C, 1 },
    { 231424000, 0x42, 0x0C, 1 },
    { 232448000, 0x42, 0x0B, 1 },
    { 233472000, 0x41, 0x0B, 1 },
    { 234496000, 0x40, 0x0B, 1 },
    { 234496000, 0x3F, 0x0B, 1 },
    { 235520000, 0x3E, 0x0B, 1 },
    { 236544000, 0x3D, 0x0B, 1 },
    { 237568000, 0x3C, 0x0B, 1 },
    { 239616000, 0x3B, 0x0A, 1 },
    { 240640000, 0x3A, 0x0A, 1 },
    { 241664000, 0x39, 0x0A, 1 },
    { 242688000, 0x38, 0x0A, 1 },
    { 244736000, 0x37, 0x09, 1 },
    { 245760000, 0x36, 0x09, 1 },
    { 246784000, 0x35, 0x09, 1 },
    { 248832000, 0x34, 0x09, 1 },
    { 249856000, 0x33, 0x09, 1 },
    { 250880000, 0x32, 0x08, 1 },
    { 252928000, 0x31, 0x08, 1 },
    { 253952000, 0x30, 0x08, 1 },
    { 256000000, 0x2F, 0x08, 1 },
    { 257024000, 0x2E, 0x08, 1 },
    { 259072000, 0x2D, 0x07, 1 },
    { 260096000, 0x2C, 0x07, 1 },
    { 262144000, 0x2B, 0x07, 1 },
    { 264192000, 0x2A, 0x07, 1 },
    { 265216000, 0x29, 0x07, 1 },
    { 267264000, 0x28, 0x07, 1 },
    { 269312000, 0x27, 0x07, 1 },
    { 270336000, 0x26, 0x07, 1 },
    { 272384000, 0x25, 0x07, 1 },
    { 274432000, 0x24, 0x07, 1 },
    { 276480000, 0x23, 0x07, 1 },
    { 277504000, 0x22, 0x07, 1 },
    { 279552000, 0x21, 0x07, 1 },
    { 281600000, 0x20, 0x07, 1 },
    { 283648000, 0x1F, 0x07, 1 },
    { 285696000, 0x1E, 0x0F, 1 },
    { 287744000, 0x1D, 0x0F, 1 },
    { 289792000, 0x1C, 0x0E, 1 },
    { 291840000, 0x1B, 0x0E, 1 },
    { 293888000, 0x1A, 0x0D, 1 },
    { 296960000, 0x19, 0x0D, 1 },
    { 299008000, 0x18, 0x0C, 1 },
    { 301056000, 0x17, 0x0C, 1 },
    { 304128000, 0x16, 0x0C, 1 },
    { 306176000, 0x15, 0x0B, 1 },
    { 309248000, 0x14, 0x0B, 1 },
    { 312320000, 0x13, 0x0B, 1 },
    { 314368000, 0x12, 0x0B, 1 },
    { 317440000, 0x11, 0x0A, 1 },
    { 320512000, 0x10, 0x0A, 1 },
    { 322560000, 0x0F, 0x0A, 1 },
    { 325632000, 0x0E, 0x09, 1 },
    { 328704000, 0x0D, 0x09, 1 },
    { 331776000, 0x0C, 0x08, 1 },
    { 335872000, 0x0B, 0x08, 1 },
    { 338944000, 0x0A, 0x08, 1 },
    { 343040000, 0x09, 0x07, 1 },
    { 346112000, 0x08, 0x07, 1 },
    { 350208000, 0x07, 0x07, 1 },
    { 354304000, 0x06, 0x07, 1 },
    { 358400000, 0x05, 0x07, 1 },
    { 362496000, 0x04, 0x07, 1 },
    { 365568000, 0x04, 0x07, 1 },
    { 367616000, 0xDA, 0x2A, 2 },
    { 367616000, 0xD9, 0x27, 2 },
    { 368640000, 0xD8, 0x27, 2 },
    { 369664000, 0xD6, 0x27, 2 },
    { 370688000, 0xD5, 0x27, 2 },
    { 371712000, 0xD3, 0x25, 2 },
    { 372736000, 0xD2, 0x23, 2 },
    { 373760000, 0xD0, 0x23, 2 },
    { 374784000, 0xCF, 0x21, 2 },
    { 375808000, 0xCD, 0x1F, 2 },
    { 376832000, 0xCC, 0x1F, 2 },
    { 377856000, 0xCA, 0x1F, 2 },
    { 378880000, 0xC9, 0x1F, 2 },
    { 379904000, 0xC7, 0x1F, 2 },
    { 380928000, 0xC6, 0x1F, 2 },
    { 381952000, 0xC4, 0x1F, 2 },
    { 382976000, 0xC3, 0x1F, 2 },
    { 384000000, 0xC1, 0x1F, 2 },
    { 385024000, 0xC0, 0x1F, 2 },
    { 386048000, 0xBF, 0x1F, 2 },
    { 387072000, 0xBD, 0x1F, 2 },
    { 388096000, 0xBC, 0x1F, 2 },
    { 389120000, 0xBB, 0x1F, 2 },
    { 390144000, 0xB9, 0x1F, 2 },
    { 391168000, 0xB8, 0x1F, 2 },
    { 392192000, 0xB7, 0x1F, 2 },
    { 393216000, 0xB5, 0x1F, 2 },
    { 394240000, 0xB4, 0x1F, 2 },
    { 395264000, 0xB3, 0x1F, 2 },
    { 396288000, 0xB1, 0x1F, 2 },
    { 397312000, 0xB0, 0x1F, 2 },
    { 398336000, 0xAF, 0x1F, 2 },
    { 399360000, 0xAD, 0x1F, 2 },
    { 400384000, 0xAC, 0x1F, 2 },
    { 401408000, 0xAB, 0x1F, 2 },
    { 402432000, 0xAA, 0x1F, 2 },
    { 403456000, 0xA8, 0x1E, 2 },
    { 404480000, 0xA7, 0x1D, 2 },
    { 405504000, 0xA6, 0x1D, 2 },
    { 405504000, 0xA5, 0x1C, 2 },
    { 406528000, 0xA3, 0x1C, 2 },
    { 407552000, 0xA2, 0x1B, 2 },
    { 408576000, 0xA1, 0x1B, 2 },
    { 409600000, 0xA0, 0x1B, 2 },
    { 410624000, 0x9F, 0x1A, 2 },
    { 411648000, 0x9D, 0x1A, 2 },
    { 412672000, 0x9C, 0x19, 2 },
    { 413696000, 0x9B, 0x18, 2 },
    { 414720000, 0x9A, 0x18, 2 },
    { 415744000, 0x99, 0x17, 2 },
    { 416768000, 0x98, 0x17, 2 },
    { 417792000, 0x97, 0x17, 2 },
    { 418816000, 0x95, 0x17, 2 },
    { 419840000, 0x94, 0x17, 2 },
    { 420864000, 0x93, 0x17, 2 },
    { 421888000, 0x92, 0x17, 2 },
    { 422912000, 0x91, 0x17, 2 },
    { 423936000, 0x90, 0x17, 2 },
    { 424960000, 0x8F, 0x17, 2 },
    { 425984000, 0x8E, 0x16, 2 },
    { 427008000, 0x8D, 0x16, 2 },
    { 428032000, 0x8C, 0x15, 2 },
    { 429056000, 0x8B, 0x15, 2 },
    { 430080000, 0x8A, 0x15, 2 },
    { 431104000, 0x88, 0x14, 2 },
    { 432128000, 0x87, 0x14, 2 },
    { 433152000, 0x86, 0x14, 2 },
    { 434176000, 0x85, 0x13, 2 },
    { 435200000, 0x84, 0x13, 2 },
    { 436224000, 0x83, 0x13, 2 },
    { 437248000, 0x82, 0x13, 2 },
    { 438272000, 0x81, 0x13, 2 },
    { 439296000, 0x80, 0x12, 2 },
    { 440320000, 0x7F, 0x12, 2 },
    { 441344000, 0x7E, 0x12, 2 },
    { 442368000, 0x7D, 0x11, 2 },
    { 444416000, 0x7C, 0x11, 2 },
    { 445440000, 0x7B, 0x10, 2 },
    { 446464000, 0x7A, 0x10, 2 },
    { 447488000, 0x79, 0x10, 2 },
    { 448512000, 0x78, 0x10, 2 },
    { 448512000, 0x77, 0x0F, 2 },
    { 449536000, 0x76, 0x0F, 2 },
    { 450560000, 0x75, 0x0F, 2 },
    { 451584000, 0x74, 0x0F, 2 },
    { 452608000, 0x73, 0x0F, 2 },
    { 453632000, 0x72, 0x0F, 2 },
    { 454656000, 0x71, 0x0F, 2 },
    { 455680000, 0x70, 0x0F, 2 },
    { 457728000, 0x6F, 0x0F, 2 },
    { 458752000, 0x6E, 0x0F, 2 },
    { 459776000, 0x6D, 0x0F, 2 },
    { 460800000, 0x6C, 0x0F, 2 },
    { 461824000, 0x6B, 0x0F, 2 },
    { 462848000, 0x6A, 0x0F, 2 },
    { 464896000, 0x69, 0x0F, 2 },
    { 465920000, 0x68, 0x0F, 2 },
    { 466944000, 0x67, 0x0F, 2 },
    { 467968000, 0x66, 0x0F, 2 },
    { 468992000, 0x65, 0x0F, 2 },
    { 471040000, 0x64, 0x0F, 2 },
    { 472064000, 0x63, 0x0F, 2 },
    { 473088000, 0x62, 0x0F, 2 },
    { 474112000, 0x61, 0x0F, 2 },
    { 476160000, 0x60, 0x0F, 2 },
    { 477184000, 0x5F, 0x0F, 2 },
    { 478208000, 0x5E, 0x0F, 2 },
    { 479232000, 0x5D, 0x0F, 2 },
    { 481280000, 0x5C, 0x0F, 2 },
    { 482304000, 0x5B, 0x0F, 2 },
    { 483328000, 0x5A, 0x0F, 2 },
    { 485376000, 0x59, 0x0F, 2 },
    { 486400000, 0x58, 0x0F, 2 },
    { 487424000, 0x57, 0x0F, 2 },
    { 489472000, 0x56, 0x0F, 2 },
    { 490496000, 0x55, 0x0F, 2 },
    { 490496000, 0x54, 0x0F, 2 },
    { 492544000, 0x53, 0x0E, 2 },
    { 493568000, 0x52, 0x0E, 2 },
    { 495616000, 0x51, 0x0E, 2 },
    { 496640000, 0x50, 0x0E, 2 },
    { 497664000, 0x4F, 0x0E, 2 },
    { 499712000, 0x4E, 0x0D, 2 },
    { 500736000, 0x4D, 0x0D, 2 },
    { 502784000, 0x4C, 0x0D, 2 },
    { 503808000, 0x4B, 0x0D, 2 },
    { 505856000, 0x4A, 0x0C, 2 },
    { 506880000, 0x49, 0x0C, 2 },
    { 508928000, 0x48, 0x0C, 2 },
    { 509952000, 0x47, 0x0C, 2 },
    { 512000000, 0x46, 0x0C, 2 },
    { 513024000, 0x45, 0x0B, 2 },
    { 515072000, 0x44, 0x0B, 2 },
    { 517120000, 0x43, 0x0B, 2 },
    { 518144000, 0x42, 0x0B, 2 },
    { 520192000, 0x41, 0x0B, 2 },
    { 521216000, 0x40, 0x0B, 2 },
    { 523264000, 0x3F, 0x0B, 2 },
    { 525312000, 0x3E, 0x0B, 2 },
    { 526336000, 0x3D, 0x0B, 2 },
    { 528384000, 0x3C, 0x0A, 2 },
    { 530432000, 0x3B, 0x0A, 2 },
    { 531456000, 0x3A, 0x0A, 2 },
    { 533504000, 0x39, 0x0A, 2 },
    { 534528000, 0x38, 0x0A, 2 },
    { 536576000, 0x37, 0x0A, 2 },
    { 537600000, 0x36, 0x09, 2 },
    { 539648000, 0x35, 0x09, 2 },
    { 541696000, 0x34, 0x09, 2 },
    { 543744000, 0x33, 0x09, 2 },
    { 544768000, 0x32, 0x09, 2 },
    { 546816000, 0x31, 0x09, 2 },
    { 548864000, 0x30, 0x08, 2 },
    { 550912000, 0x2F, 0x08, 2 },
    { 552960000, 0x2E, 0x08, 2 },
    { 555008000, 0x2D, 0x08, 2 },
    { 557056000, 0x2C, 0x08, 2 },
    { 559104000, 0x2B, 0x08, 2 },
    { 561152000, 0x2A, 0x07, 2 },
    { 563200000, 0x29, 0x07, 2 },
    { 565248000, 0x28, 0x07, 2 },
    { 567296000, 0x27, 0x07, 2 },
    { 569344000, 0x26, 0x07, 2 },
    { 570368000, 0x26, 0x07, 2 },
    { 571392000, 0x25, 0x07, 2 },
    { 573440000, 0x24, 0x07, 2 },
    { 575488000, 0x23, 0x07, 2 },
    { 577536000, 0x22, 0x0F, 2 },
    { 578560000, 0x21, 0x0F, 2 },
    { 580608000, 0x20, 0x0F, 2 },
    { 583680000, 0x1F, 0x0F, 2 },
    { 585728000, 0x1E, 0x0F, 2 },
    { 587776000, 0x1D, 0x0F, 2 },
    { 589824000, 0x1C, 0x0F, 2 },
    { 592896000, 0x1B, 0x0F, 2 },
    { 594944000, 0x1A, 0x0F, 2 },
    { 596992000, 0x19, 0x0F, 2 },
    { 600064000, 0x18, 0x0F, 2 },
    { 602112000, 0x17, 0x0F, 2 },
    { 604160000, 0x16, 0x0F, 2 },
    { 607232000, 0x15, 0x0F, 2 },
    { 609280000, 0x14, 0x0F, 2 },
    { 612352000, 0x13, 0x0F, 2 },
    { 615424000, 0x12, 0x0F, 2 },
    { 617472000, 0x11, 0x0F, 2 },
    { 619520000, 0x10, 0x0E, 2 },
    { 621568000, 0x0F, 0x0E, 2 },
    { 623616000, 0x0F, 0x0E, 2 },
    { 624640000, 0xA3, 0x1F, 3 },
    { 625664000, 0xA2, 0x1F, 3 },
    { 626688000, 0xA1, 0x1F, 3 },
    { 627712000, 0xA0, 0x1F, 3 },
    { 628736000, 0x9F, 0x1F, 3 },
    { 630784000, 0x9E, 0x1F, 3 },
    { 631808000, 0x9D, 0x1F, 3 },
    { 632832000, 0x9C, 0x1F, 3 },
    { 633856000, 0x9B, 0x1F, 3 },
    { 635904000, 0x9A, 0x1F, 3 },
    { 636928000, 0x99, 0x1F, 3 },
    { 637952000, 0x98, 0x1F, 3 },
    { 638976000, 0x97, 0x1F, 3 },
    { 641024000, 0x96, 0x1E, 3 },
    { 642048000, 0x95, 0x1E, 3 },
    { 643072000, 0x94, 0x1E, 3 },
    { 644096000, 0x93, 0x1D, 3 },
    { 646144000, 0x92, 0x1D, 3 },
    { 647168000, 0x91, 0x1C, 3 },
    { 648192000, 0x90, 0x1C, 3 },
    { 650240000, 0x8F, 0x1B, 3 },
    { 651264000, 0x8E, 0x1B, 3 },
    { 652288000, 0x8D, 0x1B, 3 },
    { 654336000, 0x8C, 0x1B, 3 },
    { 655360000, 0x8B, 0x1B, 3 },
    { 656384000, 0x8A, 0x1B, 3 },
    { 658432000, 0x89, 0x1A, 3 },
    { 659456000, 0x88, 0x1A, 3 },
    { 660480000, 0x87, 0x1A, 3 },
    { 661504000, 0x86, 0x19, 3 },
    { 662528000, 0x85, 0x19, 3 },
    { 664576000, 0x84, 0x18, 3 },
    { 665600000, 0x83, 0x18, 3 },
    { 666624000, 0x82, 0x18, 3 },
    { 668672000, 0x81, 0x18, 3 },
    { 669696000, 0x80, 0x17, 3 },
    { 671744000, 0x7F, 0x17, 3 },
    { 672768000, 0x7E, 0x17, 3 },
    { 674816000, 0x7D, 0x17, 3 },
    { 675840000, 0x7C, 0x17, 3 },
    { 676864000, 0x7B, 0x17, 3 },
    { 678912000, 0x7A, 0x17, 3 },
    { 679936000, 0x79, 0x17, 3 },
    { 681984000, 0x78, 0x17, 3 },
    { 683008000, 0x77, 0x17, 3 },
    { 685056000, 0x76, 0x17, 3 },
    { 686080000, 0x75, 0x17, 3 },
    { 688128000, 0x74, 0x17, 3 },
    { 689152000, 0x73, 0x17, 3 },
    { 691200000, 0x72, 0x16, 3 },
    { 693248000, 0x71, 0x16, 3 },
    { 694272000, 0x70, 0x16, 3 },
    { 696320000, 0x6F, 0x15, 3 },
    { 697344000, 0x6E, 0x15, 3 },
    { 699392000, 0x6D, 0x15, 3 },
    { 700416000, 0x6C, 0x15, 3 },
    { 702464000, 0x6B, 0x14, 3 },
    { 704512000, 0x6A, 0x14, 3 },
    { 704512000, 0x69, 0x14, 3 },
    { 706560000, 0x68, 0x14, 3 },
    { 707584000, 0x67, 0x13, 3 },
    { 709632000, 0x66, 0x13, 3 },
    { 711680000, 0x65, 0x13, 3 },
    { 712704000, 0x64, 0x13, 3 },
    { 714752000, 0x63, 0x13, 3 },
    { 716800000, 0x62, 0x13, 3 },
    { 717824000, 0x61, 0x13, 3 },
    { 719872000, 0x60, 0x13, 3 },
    { 721920000, 0x5F, 0x12, 3 },
    { 723968000, 0x5E, 0x12, 3 },
    { 724992000, 0x5D, 0x12, 3 },
    { 727040000, 0x5C, 0x12, 3 },
    { 729088000, 0x5B, 0x11, 3 },
    { 731136000, 0x5A, 0x11, 3 },
    { 732160000, 0x59, 0x11, 3 },
    { 734208000, 0x58, 0x11, 3 },
    { 736256000, 0x57, 0x10, 3 },
    { 738304000, 0x56, 0x10, 3 },
    { 740352000, 0x55, 0x10, 3 },
    { 741376000, 0x54, 0x10, 3 },
    { 743424000, 0x53, 0x10, 3 },
    { 745472000, 0x52, 0x0F, 3 },
    { 746496000, 0x51, 0x0F, 3 },
    { 748544000, 0x50, 0x0F, 3 },
    { 750592000, 0x4F, 0x0F, 3 },
    { 752640000, 0x4E, 0x0F, 3 },
    { 753664000, 0x4D, 0x0F, 3 },
    { 755712000, 0x4C, 0x0F, 3 },
    { 757760000, 0x4B, 0x0F, 3 },
    { 759808000, 0x4A, 0x0F, 3 },
    { 761856000, 0x49, 0x0F, 3 },
    { 762880000, 0x49, 0x0F, 3 },
    { 763904000, 0x48, 0x0F, 3 },
    { 765952000, 0x47, 0x0F, 3 },
    { 768000000, 0x46, 0x0F, 3 },
    { 770048000, 0x45, 0x0F, 3 },
    { 772096000, 0x44, 0x0F, 3 },
    { 774144000, 0x43, 0x0F, 3 },
    { 776192000, 0x42, 0x0F, 3 },
    { 778240000, 0x41, 0x0F, 3 },
    { 780288000, 0x40, 0x0F, 3 },
    { 783360000, 0x3F, 0x0F, 3 },
    { 785408000, 0x3E, 0x0F, 3 },
    { 787456000, 0x3D, 0x0F, 3 },
    { 789504000, 0x3C, 0x0F, 3 },
    { 790528000, 0x3B, 0x0F, 3 },
    { 792576000, 0x3A, 0x0F, 3 },
    { 794624000, 0x39, 0x0F, 3 },
    { 797696000, 0x38, 0x0F, 3 },
    { 799744000, 0x37, 0x0F, 3 },
    { 801792000, 0x36, 0x0F, 3 },
    { 803840000, 0x35, 0x0F, 3 },
    { 806912000, 0x34, 0x0F, 3 },
    { 808960000, 0x33, 0x0F, 3 },
    { 809984000, 0x33, 0x0F, 3 },
    { 811008000, 0x32, 0x0F, 3 },
    { 813056000, 0x31, 0x0F, 3 },
    { 816128000, 0x30, 0x0F, 3 },
    { 818176000, 0x2F, 0x0F, 3 },
    { 820224000, 0x2E, 0x0F, 3 },
    { 823296000, 0x2D, 0x0F, 3 },
    { 825344000, 0x2C, 0x0F, 3 },
    { 828416000, 0x2B, 0x0F, 3 },
    { 830464000, 0x2A, 0x0F, 3 },
    { 832512000, 0x29, 0x0F, 3 },
    { 834560000, 0x28, 0x0F, 3 },
    { 836608000, 0x27, 0x0F, 3 },
    { 839680000, 0x26, 0x0F, 3 },
    { 841728000, 0x25, 0x0F, 3 },
    { 844800000, 0x24, 0x0F, 3 },
    { 847872000, 0x23, 0x0F, 3 },
    { 849920000, 0x22, 0x0F, 3 },
    { 852992000, 0x21, 0x0E, 3 },
    { 855040000, 0x20, 0x0E, 3 },
    { 858112000, 0x1F, 0x0E, 3 },
    { 861184000, 0x1E, 0x0E, 3 },
    { 863232000, 0x1D, 0x0E, 3 },
    { 866304000, 0x1C, 0x0E, 3 },
    { 900096000, 0x10, 0x0C, 3 },
    { 929792000, 0x07, 0x0B, 3 },
    { 969728000, 0x00, 0x0A, 3 }
};

static tmErrorCode_t
ddTDA182I2GetRFCalFreq(ptmddTDA182I2Object_t pObj, UInt32 idx, UInt32 *puRF0, UInt32 *puRF1)
{
    switch(idx)
    {
    case 0:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx26.bF.RFCAL_Freq0];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx26.bF.RFCAL_Freq1];
        break;

    case 1:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx26.bF.RFCAL_Freq1];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx27.bF.RFCAL_Freq2];
        break;

    case 2:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx27.bF.RFCAL_Freq2];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx27.bF.RFCAL_Freq3];
        break;

    case 3:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx27.bF.RFCAL_Freq3];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx28.bF.RFCAL_Freq4];
        break;

    case 4:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx28.bF.RFCAL_Freq4];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx28.bF.RFCAL_Freq5];
        break;

    case 5:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx28.bF.RFCAL_Freq5];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx29.bF.RFCAL_Freq6];
        break;

    case 6:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx29.bF.RFCAL_Freq6];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx29.bF.RFCAL_Freq7];
        break;

    case 7:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx29.bF.RFCAL_Freq7];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx2A.bF.RFCAL_Freq8];
        break;

    case 8:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx2A.bF.RFCAL_Freq8];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx2A.bF.RFCAL_Freq9];
        break;

    case 9:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx2A.bF.RFCAL_Freq9];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx2B.bF.RFCAL_Freq10];
        break;

    case 10:
        if(puRF0!=Null) *puRF0 = TDA182I2_Cal_Map[idx].Cal_Freq[pObj->I2CMap.uBx2B.bF.RFCAL_Freq10];
        if(puRF1!=Null) *puRF1 = TDA182I2_Cal_Map[idx+1].Cal_Freq[pObj->I2CMap.uBx2B.bF.RFCAL_Freq11];
        break;

    default:
        if(puRF0!=Null) *puRF0 = 0;
        if(puRF1!=Null) *puRF1 = 0;
        break;
    }

    return TM_OK;
}

/*============================================================================*/
/* Internal Prototypes:                                                       */
/*============================================================================*/

/*============================================================================*/
/* Exported functions:                                                        */
/*============================================================================*/


/*============================================================================*/
/* FUNCTION:    tmddTDA182I2Init                                              */
/*                                                                            */
/* DESCRIPTION: Create an instance of a TDA182I2 Tuner                        */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2Init
(
    tmUnitSelect_t              tUnit,      /* I: Unit number */
    tmbslFrontEndDependency_t*  psSrvFunc   /* I: setup parameters */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    if (psSrvFunc == Null)
    {
        err = ddTDA182I2_ERR_BAD_PARAMETER;
    }

    /* Get Instance Object */
    if(err == TM_OK)
    {
        err = ddTDA182I2GetInstance(tUnit, &pObj);
    }

    /* Check driver state */
    if (err == TM_OK || err == ddTDA182I2_ERR_NOT_INITIALIZED)
    {
        if (pObj != Null && pObj->init == True)
        {
            err = ddTDA182I2_ERR_NOT_INITIALIZED;
        }
        else 
        {
            /* Allocate the Instance Object */
            if (pObj == Null)
            {
                err = ddTDA182I2AllocInstance(tUnit, &pObj);
                if (err != TM_OK || pObj == Null)
                {
                    err = ddTDA182I2_ERR_NOT_INITIALIZED;        
                }
            }

            if(err == TM_OK)
            {
                /* initialize the Instance Object */
                pObj->sRWFunc = psSrvFunc->sIo;
                pObj->sTime = psSrvFunc->sTime;
                pObj->sDebug = psSrvFunc->sDebug;

                if(  psSrvFunc->sMutex.Init != Null
                    && psSrvFunc->sMutex.DeInit != Null
                    && psSrvFunc->sMutex.Acquire != Null
                    && psSrvFunc->sMutex.Release != Null)
                {
                    pObj->sMutex = psSrvFunc->sMutex;

                    err = pObj->sMutex.Init(&pObj->pMutex);
                }

                pObj->init = True;
                err = TM_OK;
            }
        }
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2DeInit                                            */
/*                                                                            */
/* DESCRIPTION: Destroy an instance of a TDA182I2 Tuner                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t 
tmddTDA182I2DeInit
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{
    tmErrorCode_t           err = TM_OK;
    ptmddTDA182I2Object_t   pObj = Null;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);

//    tmDBGPRINTEx(DEBUGLVL_VERBOSE, "tmddTDA182I2DeInit(0x%08X)", tUnit);

    if(err == TM_OK)
    {
        if(pObj->sMutex.DeInit != Null)
        {
            if(pObj->pMutex != Null)
            {
                err = pObj->sMutex.DeInit(pObj->pMutex);
            }

            pObj->sMutex.Init = Null;
            pObj->sMutex.DeInit = Null;
            pObj->sMutex.Acquire = Null;
            pObj->sMutex.Release = Null;

            pObj->pMutex = Null;
        }
    }

    err = ddTDA182I2DeAllocInstance(tUnit);

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetSWVersion                                      */
/*                                                                            */
/* DESCRIPTION: Return the version of this device                             */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:       Values defined in the tmddTDA182I2local.h file                */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetSWVersion
(
    ptmSWVersion_t  pSWVersion  /* I: Receives SW Version */
)
{
    pSWVersion->compatibilityNr = TDA182I2_DD_COMP_NUM;
    pSWVersion->majorVersionNr  = TDA182I2_DD_MAJOR_VER;
    pSWVersion->minorVersionNr  = TDA182I2_DD_MINOR_VER;

    return TM_OK;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2Reset                                             */
/*                                                                            */
/* DESCRIPTION: Initialize TDA182I2 Hardware                                  */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2Reset
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{   
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err = TM_OK;
    UInt8                   uSubAddress = 0x00;
    UInt8                   uData = 0x00;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bIRQWait)
        {
            err = ddTDA182I2WaitXtalCal_End(pObj, 100, 5);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2WaitXtalCal_End(0x%08X) failed.", tUnit));
        }

        /* ---------- ES2 PATCH BEGIN ---------- */
        if(pObj->bSoftReset)
        {
            /* Soft_Reset */
            uSubAddress = 0x60;
            uData = 0x2;
            err = POBJ_SRVFUNC_SIO.Write(pObj->tUnit, 1, &uSubAddress, 1, &uData);
            uData = 0x3;
            err = POBJ_SRVFUNC_SIO.Write(pObj->tUnit, 1, &uSubAddress, 1, &uData);

            err =ddTDA182I2Wait(pObj, 100);

            uData = 0x00;
            err = POBJ_SRVFUNC_SIO.Write(pObj->tUnit, 1, &uSubAddress, 1, &uData);

            pObj->bSoftReset = False;
        }
        if(err == TM_OK)
        {
            /* read all bytes */
            err = ddTDA182I2Read(pObj, 0x00, TDA182I2_I2C_MAP_NB_BYTES);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));
        }
        /****** ES2 I2C map initialization : begin *********/
        if(err == TM_OK)
        {
            /* RSSI_Ck_Speed    31,25 kHz   0 */
            err = tmddTDA182I2SetRSSI_Ck_Speed(tUnit, 0);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRSSI_Ck_Speed(0x%08X, 0) failed.", tUnit));
        }
        if(err == TM_OK)
        {
            /* AGC1_Do_step    8,176 ms   2 */
            err = tmddTDA182I2SetAGC1_Do_step(tUnit, 2);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC1_Do_step(0x%08X, 2) failed.", tUnit));
        }
        if(err == TM_OK)
        {
            /* AGC2_Do_step    8,176 ms   1 */
            err = tmddTDA182I2SetAGC2_Do_step(tUnit, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC2_Do_step(0x%08X, 1) failed.", tUnit));
        }
        if(err == TM_OK)
        {
            /* AGCs_Up_Step_assym       UP 12 Asym / 4 Asym  / 5 Asym   3 */
            err = tmddTDA182I2SetAGCs_Up_Step_assym(tUnit, 3);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGCs_Up_Step_assym(0x%08X, 3) failed.", tUnit));
        }
        if(err == TM_OK)
        {
            /* AGCs_Do_Step_assym       DO 12 Asym / 45 Sym   2 */
            err = tmddTDA182I2SetAGCs_Do_Step_assym(tUnit, 2);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGCs_Do_Step_assym(0x%08X, 2) failed.", tUnit));
        }

        if(err == TM_OK && pObj->I2CMap.uBx02.bF.Major_rev == 1 && pObj->I2CMap.uBx02.bF.Minor_rev == 0)
        {
            /* ---------- ES2 PATCH ---------- */
            if(err == TM_OK)
            {
                /* CP Div change  :   77  ( 0x4D)  */
                err = tmddTDA182I2SetN_CP_Current(tUnit, 0x4D);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetN_CP_Current(0x%08X, 0x4D) failed.", tUnit));
            }

            if(err == TM_OK)
            {
                /* AGC5 DN time constant    1,022 ms    3 */
                err = tmddTDA182I2SetAGC5_Do_step(tUnit, 3);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC5_Do_step(0x%08X, 3) failed.", tUnit));
            }

            if(err == TM_OK)
            {
                /* AGCs Up time constant    Fast : 262,131,65,65ms  1 */
                err = tmddTDA182I2SetAGCs_Up_Step(tUnit, 1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGCs_Up_Step(0x%08X, 1) failed.", tUnit));
            }

            if(err == TM_OK)
            {
                /* AGC Overload TOP AGC1 TOP DN + 6 / 110 / 85  7 */
                err = tmddTDA182I2SetAGC_Ovld_TOP(tUnit, 7);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC_Ovld_TOP(0x%08X, 7) failed.", tUnit));
            }

            if(err == TM_OK)
            {
                /* IR loop    7   7 */
                err = tmddTDA182I2SetIR_Loop(tUnit, 7);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIR_Loop(0x%08X, 7) failed.", tUnit));
            }

            if(err == TM_OK)
            {
                /* IR target    67 dB   7 */
                err = tmddTDA182I2SetIR_Target(tUnit, 7);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIR_Target(0x%08X, 7) failed.", tUnit));
            }

            if(err == TM_OK)
            {
                /* IR corr boost    0    0 */
                err = tmddTDA182I2SetIR_Corr_Boost(tUnit, 0);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIR_Corr_Boost(0x%08X, 0) failed.", tUnit));
            }

            if(err == TM_OK)
            {
                /* IR output calc    calc    0 */
                err = tmddTDA182I2SetIR_mode_ram_store(tUnit, 0);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIR_mode_ram_store(0x%08X, 0) failed.", tUnit));
            }

            if(err == TM_OK) /* PD Underload not active */
            {
                err = tmddTDA182I2SetPD_Udld(tUnit, 0x1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetPD_Udld(0x%08X) failed.", tUnit));
            }

            if(err == TM_OK) /* force PHI2 to -0.25 instead of +0.25 */
            {
                err =  tmddTDA182I2SetRFCAL_Phi2(tUnit, (UInt8) 1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRFCAL_Phi2(0x%08X) failed.", tUnit));
            }

            if(err == TM_OK) /*Disable RF Filter Bypass */
            {
                err = tmddTDA182I2SetRF_Filter_Bypass(tUnit, 0);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRF_Filter_Bypass(0x%08X) failed.", tUnit));
            }

            if(err == TM_OK)
            {
                /* Move IF_Reg from 2.8V to 3V */
                uSubAddress = 0x58;
                uData = 0x65;
                err = POBJ_SRVFUNC_SIO.Write (pObj->tUnit, 1, &uSubAddress, 1, &uData);
            }

            /* Enable RFCAL SW Algorithm */
            pObj->bRFCAL_SW_Algo_Enable = True;
            /* ---------- ES2 PATCH END ---------- */
        }
        /****** I2C map initialization : end *********/


        /*****************************************/
        /* Launch tuner calibration */
        /* State reached after 1.5 s max */
        if(err == TM_OK)
        {
            /* set IRQ_clear */
            err = tmddTDA182I2SetIRQ_Clear(tUnit, 0x1F);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIRQ_clear(0x%08X, 0x1F) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* set power state on */
            err = tmddTDA182I2SetPowerState(tUnit, tmddTDA182I2_PowerNormalMode);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetPowerState(0x%08X, PowerNormalMode) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* set & trigger MSM */
            pObj->I2CMap.uBx19.MSM_byte_1 = 0x3B;
            pObj->I2CMap.uBx1A.MSM_byte_2 = 0x01;

            /* write bytes 0x19 to 0x1A */
            err = ddTDA182I2Write(pObj, 0x19, 2);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

            pObj->I2CMap.uBx1A.MSM_byte_2 = 0x00;

        }

        if(pObj->bIRQWait)
        {
            if(err == TM_OK)
            {
                err = ddTDA182I2WaitIRQ(pObj, 1500, 50, 0x1F);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2WaitIRQ(0x%08X) failed.", tUnit));
            }
            /* PATCH ES2 in case of WAIT IRQ Timeout */
            if(err == ddTDA182I2_ERR_NOT_READY && pObj->I2CMap.uBx02.bF.Major_rev == 1 && pObj->I2CMap.uBx02.bF.Minor_rev == 0)
            {
                pObj->bSoftReset = True;
            }
            /* ---------- ES2 PATCH END ---------- */

            if(err == TM_OK && pObj->bRFCAL_SW_Algo_Enable)
            {
                err = tmddTDA182I2RFFilterCorrInit(tUnit);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2RFFilterCorrInit(0x%08X) failed.", tUnit));
            }
        }
        /*****************************************/

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2RFFilterCorrInit                                  */
/*                                                                            */
/* DESCRIPTION: RF Filter Correction to be done after hardware initialization */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2RFFilterCorrInit
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;
    UInt8                   uData = 0;
    UInt8*                  puData = Null;
    UInt32                  uCounter = 0;
    UInt8                   idx = 0;
    UInt32                  uRFCal0 = 0;
    UInt32                  uRFCal1 = 0;

    /* Int32 calIdx1 = 0; */
    /* Int32 calIdx0 = 0; */
    /* Int32 deltaRF = 0; */


    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* read byte 0x38-0x43 */
        err = ddTDA182I2Read(pObj, 0x38, 12);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            puData = &pObj->I2CMap.uBx38.rfcal_log_0;

            /* Store Cprog */
            for(uCounter = 0; uCounter<TDA182I2_RFCAL_PROG_ROW; uCounter++)
            {
                uData = ((*puData)&0x7F);
                pObj->RFCAL_Prog[uCounter].DeltaCprog = uData>63?uData-128:uData;
                puData++;
            }

            /* read byte 0x26-0x2B */
            err = ddTDA182I2Read(pObj, 0x26, 6);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));
        }

        for(uCounter = 0; uCounter<TDA182I2_RFCAL_COEFFS_ROW; uCounter++)
        {
            idx = pObj->RFCAL_Coeffs[uCounter].Cal_number;

            if(err == TM_OK)
            {
                err = ddTDA182I2GetRFCalFreq(pObj, idx, &uRFCal0, &uRFCal1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetRFCalFreq(0x%08X) failed.", tUnit));
            }
            else
            {
                break;
            }

            pObj->RFCAL_Coeffs[uCounter].RF_B1 = pObj->RFCAL_Prog[idx].DeltaCprog + TDA182I2_Cal_Map[idx].CprogOffset_Val[pObj->RFCAL_Prog[idx].CprogOffset];
            pObj->RFCAL_Coeffs[uCounter].RF_A1 = (((pObj->RFCAL_Prog[idx+1].DeltaCprog + TDA182I2_Cal_Map[idx+1].CprogOffset_Val[pObj->RFCAL_Prog[idx].CprogOffset]) - (pObj->RFCAL_Prog[idx].DeltaCprog + TDA182I2_Cal_Map[idx].CprogOffset_Val[pObj->RFCAL_Prog[idx].CprogOffset])) * 1000000)/((Int32)(uRFCal1 - uRFCal0)/1000);

            /* calIdx1 = (TDA182I2_Cal[idx+1].DeltaCprog + TDA182I2_Cal_Map[idx+1].CprogOffset_Val[TDA182I2_Cal_Map[idx].CprogOffset]); */
            /* calIdx0 = (TDA182I2_Cal[idx].DeltaCprog + TDA182I2_Cal_Map[idx].CprogOffset_Val[TDA182I2_Cal_Map[idx].CprogOffset]); */
            /* deltaRF = (uRFCal1 - uRFCal0)/1000; */

            /* TDA182I2_Coeff_Map[uCounter].RF_A1 = ((calIdx1 - calIdx0)*1000000)/deltaRF; */

        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2RFFilterCorrTune                                  */
/*                                                                            */
/* DESCRIPTION: RF Filter Correction to be done after tune                    */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2RFFilterCorrTune
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32          uRF     /* I: RF */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;
    UInt32                  uCounter = 0;
    UInt8                   idx = 0;
    UInt32                  uRFCal0 = 0;
    UInt32                  uRFCal1 = 0;
    UInt8                   uSubBand = 0;
    Int32                   cProg = 0;
    UInt8                   gainTapper = 0;
    UInt8                   RFBand = 0;
    Int32                   RF_A1 = 0;
    Int32                   RF_B1 = 0;


    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* read byte 0x26-0x2B */
        err = ddTDA182I2Read(pObj, 0x26, 6);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            err = ddTDA182I2GetRFCalFreq(pObj, 1, &uRFCal0, Null);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetRFCalFreq(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = ddTDA182I2GetRFCalFreq(pObj, 4, &uRFCal1, Null);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetRFCalFreq(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            if(uRF < uRFCal0)
                uSubBand = 0;
            else if(uRF < 145700000)
                uSubBand = 1;
            else if(uRF < uRFCal1)
                uSubBand = 2;
            else if(uRF < 367400000)
                uSubBand = 3;
            else
            {
                err = ddTDA182I2GetRFCalFreq(pObj, 7, &uRFCal0, Null);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetRFCalFreq(0x%08X) failed.", tUnit));

                if(err == TM_OK)
                {
                    err = ddTDA182I2GetRFCalFreq(pObj, 10, &uRFCal1, Null);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetRFCalFreq(0x%08X) failed.", tUnit));
                }

                if(err == TM_OK)
                {
                    if(uRF < uRFCal0)
                        uSubBand = 4;
                    else if(uRF < 625000000)
                        uSubBand = 5;
                    else if(uRF < uRFCal1)
                        uSubBand = 6;
                    else
                        uSubBand = 7;
                }
            }
        }

        if(err == TM_OK)
        {
            idx = pObj->RFCAL_Coeffs[uSubBand].Cal_number;
            err = ddTDA182I2GetRFCalFreq(pObj, idx, &uRFCal0, Null);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetRFCalFreq(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            RF_A1 = pObj->RFCAL_Coeffs[uSubBand].RF_A1;
            RF_B1 = pObj->RFCAL_Coeffs[uSubBand].RF_B1;

            uCounter = 0;
            do uCounter ++;
            while (uRF >= TDA182I2_Freq_ROM_Map[uCounter].uRF_Max && uCounter < TDA182I2_FREQ_ROM_MAP_ROW);

            cProg = TDA182I2_Freq_ROM_Map[uCounter - 1].uCprog;
            gainTapper = TDA182I2_Freq_ROM_Map[uCounter - 1].uGainTapper;
            RFBand = TDA182I2_Freq_ROM_Map[uCounter - 1].uRFBand;

            cProg = (Int32)(cProg + RF_B1 + (RF_A1*((Int32)(uRF - uRFCal0)/1000))/1000000);

            if(cProg>255)   cProg = 255;
            if(cProg<0)     cProg = 0;

            err = tmddTDA182I2SetRF_Filter_Bypass(tUnit, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRF_Filter_Bypass(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = tmddTDA182I2SetRF_Filter_Cap(tUnit, (UInt8)cProg);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRF_Filter_Cap(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = tmddTDA182I2SetGain_Taper(tUnit, gainTapper);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetGain_Taper(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = tmddTDA182I2SetRF_Filter_Band(tUnit, RFBand);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetRF_Filter_Band(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetLPF_Gain_Mode                                  */
/*                                                                            */
/* DESCRIPTION: Free/Freeze LPF Gain                                          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetLPF_Gain_Mode
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uMode   /* I: Unknown/Free/Frozen */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        switch(uMode)
        {
        case tmddTDA182I2_LPF_Gain_Unknown:
        default:
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetLPF_Gain_Free(0x%08X, tmddTDA182I2_LPF_Gain_Unknown).", tUnit));
            break;

        case tmddTDA182I2_LPF_Gain_Free:
            err = tmddTDA182I2SetAGC5_loop_off(tUnit, False); /* Disable AGC5 loop off */
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC5_loop_off(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                err = tmddTDA182I2SetForce_AGC5_gain(tUnit, False); /* Do not force AGC5 gain */
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetForce_AGC5_gain(0x%08X) failed.", tUnit));
            }
            break;

        case tmddTDA182I2_LPF_Gain_Frozen:
            err = tmddTDA182I2SetAGC5_loop_off(tUnit, True); /* Enable AGC5 loop off */
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC5_loop_off(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                err = tmddTDA182I2SetForce_AGC5_gain(tUnit, True); /* Force AGC5 gain */
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetForce_AGC5_gain(0x%08X) failed.", tUnit));
            }

            if(err == TM_OK)
            {
                err = tmddTDA182I2SetAGC5_Gain(tUnit, 0);  /* Force gain to 0dB */
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGC5_Gain(0x%08X) failed.", tUnit));
            }
            break;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetLPF_Gain_Mode                                  */
/*                                                                            */
/* DESCRIPTION: Free/Freeze LPF Gain                                          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetLPF_Gain_Mode
(
 tmUnitSelect_t  tUnit,  /* I: Unit number */
 UInt8           *puMode /* I/O: Unknown/Free/Frozen */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;
    UInt8                   AGC5_loop_off = 0;
    UInt8                   Force_AGC5_gain = 0;
    UInt8                   AGC5_Gain = 0;

    /* Test the parameter */
    if (puMode == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        *puMode = tmddTDA182I2_LPF_Gain_Unknown;

        err = tmddTDA182I2GetAGC5_loop_off(tUnit, &AGC5_loop_off);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetAGC5_loop_off(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            err = tmddTDA182I2GetForce_AGC5_gain(tUnit, &Force_AGC5_gain);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetForce_AGC5_gain(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            err = tmddTDA182I2GetAGC5_Gain(tUnit, &AGC5_Gain);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2GetAGC5_Gain(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            if(AGC5_loop_off==False && Force_AGC5_gain==False)
            {
                *puMode = tmddTDA182I2_LPF_Gain_Free;
            }
            else if(AGC5_loop_off==True && Force_AGC5_gain==True && AGC5_Gain==0)
            {
                *puMode = tmddTDA182I2_LPF_Gain_Frozen;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2Write                                             */
/*                                                                            */
/* DESCRIPTION: Write in TDA182I2 hardware                                    */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2Write
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32          uIndex,     /* I: Start index to write */
    UInt32          uNbBytes,   /* I: Number of bytes to write */
    UInt8*          puBytes     /* I: Pointer on an array of bytes */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err = TM_OK;
    UInt32                  uCounter;
    UInt8*                  pI2CMap = Null;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* pI2CMap initialization */
        pI2CMap = &(pObj->I2CMap.uBx00.ID_byte_1) + uIndex;

        /* Save the values written in the Tuner */
        for (uCounter = 0; uCounter < uNbBytes; uCounter++)
        {
            *pI2CMap = puBytes[uCounter];
            pI2CMap ++;
        }

        /* Write in the Tuner */
        err = ddTDA182I2Write(pObj,(UInt8)(uIndex),(UInt8)(uNbBytes));
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2Read                                              */
/*                                                                            */
/* DESCRIPTION: Read in TDA182I2 hardware                                     */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2Read
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32          uIndex,     /* I: Start index to read */
    UInt32          uNbBytes,   /* I: Number of bytes to read */
    UInt8*          puBytes     /* I: Pointer on an array of bytes */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err = TM_OK;
    UInt32                  uCounter = 0;
    UInt8*                  pI2CMap = Null;

    /* Test the parameters */
    if (uNbBytes > TDA182I2_I2C_MAP_NB_BYTES)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* pI2CMap initialization */
        pI2CMap = &(pObj->I2CMap.uBx00.ID_byte_1) + uIndex;

        /* Read from the Tuner */
        err = ddTDA182I2Read(pObj,(UInt8)(uIndex),(UInt8)(uNbBytes));
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Copy read values to puBytes */
            for (uCounter = 0; uCounter < uNbBytes; uCounter++)
            {
                *puBytes = (*pI2CMap);
                pI2CMap ++;
                puBytes ++;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetMS                                             */
/*                                                                            */
/* DESCRIPTION: Get the MS bit(s) status                                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetMS
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x00 */
        err = ddTDA182I2Read(pObj, 0x00, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        /* Get value */
        *puValue = pObj->I2CMap.uBx00.bF.MS ;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIdentity                                       */
/*                                                                            */
/* DESCRIPTION: Get the Identity bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIdentity
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt16*         puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x00-0x01 */
        err = ddTDA182I2Read(pObj, 0x00, 2);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx00.bF.Ident_1 << 8 |  pObj->I2CMap.uBx01.bF.Ident_2;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetMinorRevision                                  */
/*                                                                            */
/* DESCRIPTION: Get the Revision bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetMinorRevision
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x02 */
        err = ddTDA182I2Read(pObj, 0x02, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx02.bF.Minor_rev;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetMajorRevision                                  */
/*                                                                            */
/* DESCRIPTION: Get the Revision bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetMajorRevision
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x02 */
        err = ddTDA182I2Read(pObj, 0x02, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx02.bF.Major_rev;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetLO_Lock                                        */
/*                                                                            */
/* DESCRIPTION: Get the LO_Lock bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetLO_Lock
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x05 */
        err = ddTDA182I2Read(pObj, 0x05, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx05.bF.LO_Lock ;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetPowerState                                     */
/*                                                                            */
/* DESCRIPTION: Set the power state of the TDA182I2                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetPowerState
(
    tmUnitSelect_t              tUnit,      /* I: Unit number */
    tmddTDA182I2PowerState_t    powerState  /* I: Power state of this device */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read bytes 0x06-0x14 */
        err = ddTDA182I2Read(pObj, 0x06, 15);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        /* Set digital clock mode*/
        if(err == TM_OK)
        {
            switch (powerState)
            {
            case tmddTDA182I2_PowerStandbyWithLNAOnAndWithXtalOnAndWithSyntheOn:
            case tmddTDA182I2_PowerStandbyWithLNAOnAndWithXtalOn:
            case tmddTDA182I2_PowerStandbyWithXtalOn:
            case tmddTDA182I2_PowerStandby:
                /* Set 16 Mhz Xtal clock */
                err = tmddTDA182I2SetDigital_Clock_Mode(tUnit, 0);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetDigital_Clock_Mode(0x%08X, 16 Mhz xtal clock) failed.", tUnit));

                /* ---------- ES2 PATCH BEGIN ---------- */
                if(err == TM_OK && pObj->I2CMap.uBx02.bF.Major_rev == 1 && pObj->I2CMap.uBx02.bF.Minor_rev == 0)
                {
                    err = tmddTDA182I2SetAGCK_Mode(tUnit, 0x2); /* RSSI disable for analog PR/CR 165 */
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetAGCK_Mode(0x%08X, RSSI disable) failed.", tUnit));
                }
                /* ---------- ES2 PATCH END ---------- */
                break;

            default:
                break;
            }
        }

        /* Set power state */
        if(err == TM_OK)
        {
            switch (powerState)
            {
            case tmddTDA182I2_PowerNormalMode:          
                pObj->I2CMap.uBx06.bF.SM = 0x00;
                pObj->I2CMap.uBx06.bF.SM_Synthe = 0x00;
                pObj->I2CMap.uBx06.bF.SM_LT = 0x00;
                pObj->I2CMap.uBx06.bF.SM_XT = 0x00;
                break;

            case tmddTDA182I2_PowerStandbyWithLNAOnAndWithXtalOnAndWithSyntheOn:
                pObj->I2CMap.uBx06.bF.SM = 0x01;
                pObj->I2CMap.uBx06.bF.SM_Synthe = 0x00;
                pObj->I2CMap.uBx06.bF.SM_LT = 0x00;
                pObj->I2CMap.uBx06.bF.SM_XT = 0x00;
                break;

            case tmddTDA182I2_PowerStandbyWithLNAOnAndWithXtalOn:
                pObj->I2CMap.uBx06.bF.SM = 0x01;
                pObj->I2CMap.uBx06.bF.SM_Synthe = 0x01;
                pObj->I2CMap.uBx06.bF.SM_LT = 0x00;
                pObj->I2CMap.uBx06.bF.SM_XT = 0x00;
                break;

            case tmddTDA182I2_PowerStandbyWithXtalOn:
                pObj->I2CMap.uBx06.bF.SM = 0x01;
                pObj->I2CMap.uBx06.bF.SM_Synthe = 0x01;
                pObj->I2CMap.uBx06.bF.SM_LT = 0x01;
                pObj->I2CMap.uBx06.bF.SM_XT = 0x00;
                break;

            case tmddTDA182I2_PowerStandby:
                pObj->I2CMap.uBx06.bF.SM = 0x01;
                pObj->I2CMap.uBx06.bF.SM_Synthe = 0x01;
                pObj->I2CMap.uBx06.bF.SM_LT = 0x01;
                pObj->I2CMap.uBx06.bF.SM_XT = 0x01;
                break;

            default:
                /* Power state not supported*/
                return ddTDA182I2_ERR_NOT_SUPPORTED;
            }

            /* Write byte 0x06 */
            err = ddTDA182I2Write(pObj, 0x06, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        /* Set digital clock mode*/
        if(err == TM_OK)
        {
            switch (powerState)
            {
            case tmddTDA182I2_PowerNormalMode:            
                /* Set sigma delta clock*/
                err = tmddTDA182I2SetDigital_Clock_Mode(tUnit, 1);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetDigital_Clock_Mode(0x%08X, sigma delta clock) failed.", tUnit));
                break;

            default:
                break;
            }
        }

        if(err == TM_OK)
        {
            /* Store powerstate */
            pObj->curPowerState = powerState;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetPowerState                                     */
/*                                                                            */
/* DESCRIPTION: Get the power state of the TDA182I2                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetPowerState
(
    tmUnitSelect_t              tUnit,      /* I: Unit number */
    ptmddTDA182I2PowerState_t   pPowerState /* O: Power state of this device */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Get power state */
        if ((pObj->I2CMap.uBx06.bF.SM == 0x00) && (pObj->I2CMap.uBx06.bF.SM_Synthe == 0x00) && (pObj->I2CMap.uBx06.bF.SM_LT == 0x00) && (pObj->I2CMap.uBx06.bF.SM_XT == 0x00))
        {
            *pPowerState = tmddTDA182I2_PowerNormalMode;
        }
        else if ((pObj->I2CMap.uBx06.bF.SM == 0x01) && (pObj->I2CMap.uBx06.bF.SM_Synthe == 0x00) && (pObj->I2CMap.uBx06.bF.SM_LT == 0x00) && (pObj->I2CMap.uBx06.bF.SM_XT == 0x00))
        {
            *pPowerState = tmddTDA182I2_PowerStandbyWithLNAOnAndWithXtalOnAndWithSyntheOn;
        }
        else if ((pObj->I2CMap.uBx06.bF.SM == 0x01) && (pObj->I2CMap.uBx06.bF.SM_Synthe == 0x01) && (pObj->I2CMap.uBx06.bF.SM_LT == 0x00) && (pObj->I2CMap.uBx06.bF.SM_XT == 0x00))
        {
            *pPowerState = tmddTDA182I2_PowerStandbyWithLNAOnAndWithXtalOn;
        }
        else if ((pObj->I2CMap.uBx06.bF.SM == 0x01) && (pObj->I2CMap.uBx06.bF.SM_Synthe == 0x01) && (pObj->I2CMap.uBx06.bF.SM_LT == 0x01) && (pObj->I2CMap.uBx06.bF.SM_XT == 0x00))
        {
            *pPowerState = tmddTDA182I2_PowerStandbyWithXtalOn;
        }
        else if ((pObj->I2CMap.uBx06.bF.SM == 0x01) && (pObj->I2CMap.uBx06.bF.SM_Synthe == 0x01) && (pObj->I2CMap.uBx06.bF.SM_LT == 0x01) && (pObj->I2CMap.uBx06.bF.SM_XT == 0x01))
        {  
            *pPowerState = tmddTDA182I2_PowerStandby;
        }
        else
        {
            *pPowerState = tmddTDA182I2_PowerMax;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetPower_Level                                    */
/*                                                                            */
/* DESCRIPTION: Get the Power_Level bit(s) status                             */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetPower_Level
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;
    UInt8                   uValue = 0;
    /* ---------- ES2 PATCH BEGIN ---------- */
    UInt8                   uValue_AGC1 = 0;
    /* ---------- ES2 PATCH END ---------- */

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }
    if(err == TM_OK)
    {
        /* Set IRQ_clear*/
        err = tmddTDA182I2SetIRQ_Clear(tUnit, 0x10);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIRQ_clear(0x%08X, 0x10) failed.", tUnit));
    }
    if(err == TM_OK)
    {
        /* Trigger RSSI_Meas */
        pObj->I2CMap.uBx19.MSM_byte_1 = 0x80;
        err = ddTDA182I2Write(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2Write(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /*Trigger MSM_Launch */
            pObj->I2CMap.uBx1A.bF.MSM_Launch = 1;

            /* Write byte 0x1A */
            err = ddTDA182I2Write(pObj, 0x1A, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

            pObj->I2CMap.uBx1A.bF.MSM_Launch = 0;
            if(pObj->bIRQWait)
            {
                if(err == TM_OK)
                {
                    err = ddTDA182I2WaitIRQ(pObj, 700, 1, 0x10);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2WaitIRQ(0x%08X) failed.", tUnit));
                }
            }
        }

        if(err == TM_OK)
        {
            /* Read byte 0x07 */
            err = ddTDA182I2Read(pObj, 0x07, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Get value (limit range) */
            uValue = pObj->I2CMap.uBx07.bF.Power_Level;

            /* ---------- ES2 PATCH BEGIN ---------- */
            if(err == TM_OK && pObj->I2CMap.uBx02.bF.Major_rev == 1 && pObj->I2CMap.uBx02.bF.Minor_rev == 0)
            {
                /* Correct value according AGC1_Gain_Read */
                err = ddTDA182I2Read(pObj, 0x32, 1);
                if(err == TM_OK)
                {
                    uValue_AGC1 = pObj->I2CMap.uBx32.bF.AGC1_Gain_Read ;
                    if ( uValue_AGC1 <= 7)
                        uValue =  uValue + 6;
                    else  if ( uValue_AGC1 <= 8)
                        uValue = uValue - 6;
                    else 
                        uValue = uValue - 9;
                }
            }
            if (uValue < TDA182I2_POWER_LEVEL_MIN)
            {
                *puValue = 0x00;
            }
            else if (uValue > TDA182I2_POWER_LEVEL_MAX)
            {
                *puValue = 0xFF;
            }
            else
            {
                *puValue = uValue;
            }
            /* ---------- ES2 PATCH END ---------- */
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIRQ_status                                     */
/*                                                                            */
/* DESCRIPTION: Get the IRQ_status bit(s) status                              */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIRQ_status
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x08 */
        err = ddTDA182I2GetIRQ_status(pObj, puValue);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetIRQ_status(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetMSM_XtalCal_End                                */
/*                                                                            */
/* DESCRIPTION: Get the MSM_XtalCal_End bit(s) status                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetMSM_XtalCal_End
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x08 */
        err = ddTDA182I2GetMSM_XtalCal_End(pObj, puValue);

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIRQ_Clear                                      */
/*                                                                            */
/* DESCRIPTION: Set the IRQ_Clear bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIRQ_Clear
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt8           irqStatus   /* I: IRQs to clear */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set IRQ_Clear */
        /*pObj->I2CMap.uBx0A.bF.IRQ_Clear = 1; */
        pObj->I2CMap.uBx0A.IRQ_clear |= (0x80|(irqStatus&0x1F));

        /* Write byte 0x0A */
        err = ddTDA182I2Write(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        /* Reset IRQ_Clear (buffer only, no write) */
        /*pObj->I2CMap.uBx0A.bF.IRQ_Clear = 0;*/
        pObj->I2CMap.uBx0A.IRQ_clear &= (~(0x80|(irqStatus&0x1F)));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGC1_TOP                                       */
/*                                                                            */
/* DESCRIPTION: Set the AGC1_TOP bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGC1_TOP
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx0C.bF.AGC1_TOP = uValue;

        /* write byte 0x0C */
        err = ddTDA182I2Write(pObj, 0x0C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGC1_TOP                                       */
/*                                                                            */
/* DESCRIPTION: Get the AGC1_TOP bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGC1_TOP
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x0C */
        err = ddTDA182I2Read(pObj, 0x0C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx0C.bF.AGC1_TOP;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGC2_TOP                                       */
/*                                                                            */
/* DESCRIPTION: Set the AGC2_TOP bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGC2_TOP
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* set value */
        pObj->I2CMap.uBx0D.bF.AGC2_TOP = uValue;

        /* Write byte 0x0D */
        err = ddTDA182I2Write(pObj, 0x0D, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGC2_TOP                                       */
/*                                                                            */
/* DESCRIPTION: Get the AGC2_TOP bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGC2_TOP
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x0D */
        err = ddTDA182I2Read(pObj, 0x0D, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx0D.bF.AGC2_TOP;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGCs_Up_Step                                   */
/*                                                                            */
/* DESCRIPTION: Set the AGCs_Up_Step bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGCs_Up_Step
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx0E.bF.AGCs_Up_Step = uValue;

        /* Write byte 0x0E */
        err = ddTDA182I2Write(pObj, 0x0E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGCs_Up_Step                                   */
/*                                                                            */
/* DESCRIPTION: Get the AGCs_Up_Step bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGCs_Up_Step
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x0E */
        err = ddTDA182I2Read(pObj, 0x0E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx0E.bF.AGCs_Up_Step;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGCK_Step                                      */
/*                                                                            */
/* DESCRIPTION: Set the AGCK_Step bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGCK_Step
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx0E.bF.AGCK_Step = uValue;

        /* Write byte 0x0E */
        err = ddTDA182I2Write(pObj, 0x0E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGCK_Step                                      */
/*                                                                            */
/* DESCRIPTION: Get the AGCK_Step bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGCK_Step
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x0E */
        err = ddTDA182I2Read(pObj, 0x0E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx0E.bF.AGCK_Step;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGCK_Mode                                      */
/*                                                                            */
/* DESCRIPTION: Set the AGCK_Mode bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGCK_Mode
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx0E.bF.AGCK_Mode = uValue;

        /* Write byte 0x0E */
        err = ddTDA182I2Write(pObj, 0x0E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGCK_Mode                                      */
/*                                                                            */
/* DESCRIPTION: Get the AGCK_Mode bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGCK_Mode
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x0E */
        err = ddTDA182I2Read(pObj, 0x0E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx0E.bF.AGCK_Mode;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetPD_RFAGC_Adapt                                 */
/*                                                                            */
/* DESCRIPTION: Set the PD_RFAGC_Adapt bit(s) status                          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetPD_RFAGC_Adapt
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx0F.bF.PD_RFAGC_Adapt = uValue;

        /* Write byte 0x0F */
        err = ddTDA182I2Write(pObj, 0x0F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetPD_RFAGC_Adapt                                 */
/*                                                                            */
/* DESCRIPTION: Get the PD_RFAGC_Adapt bit(s) status                          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetPD_RFAGC_Adapt
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x0F */
        err = ddTDA182I2Read(pObj, 0x0F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx0F.bF.PD_RFAGC_Adapt;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetRFAGC_Adapt_TOP                                */
/*                                                                            */
/* DESCRIPTION: Set the RFAGC_Adapt_TOP bit(s) status                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetRFAGC_Adapt_TOP
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx0F.bF.RFAGC_Adapt_TOP = uValue;

        /* Write byte 0x0F */
        err = ddTDA182I2Write(pObj, 0x0F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetRFAGC_Adapt_TOP                                */
/*                                                                            */
/* DESCRIPTION: Get the RFAGC_Adapt_TOP bit(s) status                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetRFAGC_Adapt_TOP
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x0F */
        err = ddTDA182I2Read(pObj, 0x0F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx0F.bF.RFAGC_Adapt_TOP;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetRF_Atten_3dB                                   */
/*                                                                            */
/* DESCRIPTION: Set the RF_Atten_3dB bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetRF_Atten_3dB
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx0F.bF.RF_Atten_3dB = uValue;

        /* Write byte 0x0F */
        err = ddTDA182I2Write(pObj, 0x0F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetRF_Atten_3dB                                   */
/*                                                                            */
/* DESCRIPTION: Get the RF_Atten_3dB bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetRF_Atten_3dB
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x0F */
        err = ddTDA182I2Read(pObj, 0x0F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx0F.bF.RF_Atten_3dB;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetRFAGC_Top                                      */
/*                                                                            */
/* DESCRIPTION: Set the RFAGC_Top bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetRFAGC_Top
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx0F.bF.RFAGC_Top = uValue;

        /* Write byte 0x0F */
        err = ddTDA182I2Write(pObj, 0x0F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetRFAGC_Top                                      */
/*                                                                            */
/* DESCRIPTION: Get the RFAGC_Top bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetRFAGC_Top
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x0F */
        err = ddTDA182I2Read(pObj, 0x0F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx0F.bF.RFAGC_Top;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIR_Mixer_Top                                   */
/*                                                                            */
/* DESCRIPTION: Set the IR_Mixer_Top bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIR_Mixer_Top
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx10.bF.IR_Mixer_Top = uValue;

        /* Write byte 0x10 */
        err = ddTDA182I2Write(pObj, 0x10, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIR_Mixer_Top                                   */
/*                                                                            */
/* DESCRIPTION: Get the IR_Mixer_Top bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIR_Mixer_Top
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x10 */
        err = ddTDA182I2Read(pObj, 0x10, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx10.bF.IR_Mixer_Top;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGC5_Ana                                       */
/*                                                                            */
/* DESCRIPTION: Set the AGC5_Ana bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGC5_Ana
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx11.bF.AGC5_Ana = uValue;

        /* Write byte 0x11 */
        err = ddTDA182I2Write(pObj, 0x11, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGC5_Ana                                       */
/*                                                                            */
/* DESCRIPTION: Get the AGC5_Ana bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGC5_Ana
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x11 */
        err = ddTDA182I2Read(pObj, 0x11, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx11.bF.AGC5_Ana;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGC5_TOP                                       */
/*                                                                            */
/* DESCRIPTION: Set the AGC5_TOP bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGC5_TOP
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx11.bF.AGC5_TOP = uValue;

        /* Write byte 0x11 */
        err = ddTDA182I2Write(pObj, 0x11, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGC5_TOP                                       */
/*                                                                            */
/* DESCRIPTION: Get the AGC5_TOP bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGC5_TOP
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x11 */
        err = ddTDA182I2Read(pObj, 0x11, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx11.bF.AGC5_TOP;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIF_Level                                       */
/*                                                                            */
/* DESCRIPTION: Set the IF_level bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIF_Level
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx12.bF.IF_level = uValue;

        /* Write byte 0x12 */
        err = ddTDA182I2Write(pObj, 0x12, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIF_Level                                       */
/*                                                                            */
/* DESCRIPTION: Get the IF_level bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIF_Level
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x12 */
        err = ddTDA182I2Read(pObj, 0x12, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx12.bF.IF_level;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIF_HP_Fc                                       */
/*                                                                            */
/* DESCRIPTION: Set the IF_HP_Fc bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIF_HP_Fc
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx13.bF.IF_HP_Fc = uValue;

        /* Write byte 0x13 */
        err = ddTDA182I2Write(pObj, 0x13, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIF_HP_Fc                                       */
/*                                                                            */
/* DESCRIPTION: Get the IF_HP_Fc bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIF_HP_Fc
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x13 */
        err = ddTDA182I2Read(pObj, 0x13, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx13.bF.IF_HP_Fc;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIF_ATSC_Notch                                  */
/*                                                                            */
/* DESCRIPTION: Set the IF_ATSC_Notch bit(s) status                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIF_ATSC_Notch
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx13.bF.IF_ATSC_Notch = uValue;

        /* Write byte 0x13 */
        err = ddTDA182I2Write(pObj, 0x13, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIF_ATSC_Notch                                  */
/*                                                                            */
/* DESCRIPTION: Get the IF_ATSC_Notch bit(s) status                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIF_ATSC_Notch
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x13 */
        err = ddTDA182I2Read(pObj, 0x13, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx13.bF.IF_ATSC_Notch;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetLP_FC_Offset                                   */
/*                                                                            */
/* DESCRIPTION: Set the LP_FC_Offset bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetLP_FC_Offset
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx13.bF.LP_FC_Offset = uValue;

        /* Write byte 0x13 */
        err = ddTDA182I2Write(pObj, 0x13, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetLP_FC_Offset                                   */
/*                                                                            */
/* DESCRIPTION: Get the LP_FC_Offset bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetLP_FC_Offset
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x13 */
        err = ddTDA182I2Read(pObj, 0x13, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx13.bF.LP_FC_Offset;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetLP_FC                                          */
/*                                                                            */
/* DESCRIPTION: Set the LP_Fc bit(s) status                                   */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetLP_FC
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx13.bF.LP_Fc = uValue;

        /* Write byte 0x13 */
        err = ddTDA182I2Write(pObj, 0x13, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetLP_FC                                          */
/*                                                                            */
/* DESCRIPTION: Get the LP_Fc bit(s) status                                   */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetLP_FC
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x13 */
        err = ddTDA182I2Read(pObj, 0x13, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx13.bF.LP_Fc;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetDigital_Clock_Mode                             */
/*                                                                            */
/* DESCRIPTION: Set the Digital_Clock_Mode bit(s) status                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetDigital_Clock_Mode
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx14.bF.Digital_Clock_Mode = uValue;

        /* Write byte 0x14 */
        err = ddTDA182I2Write(pObj, 0x14, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetDigital_Clock_Mode                             */
/*                                                                            */
/* DESCRIPTION: Get the Digital_Clock_Mode bit(s) status                      */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetDigital_Clock_Mode
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x14 */
        err = ddTDA182I2Read(pObj, 0x14, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx14.bF.Digital_Clock_Mode;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIF_Freq                                        */
/*                                                                            */
/* DESCRIPTION: Set the IF_Freq bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIF_Freq
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32          uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx15.bF.IF_Freq = (UInt8)(uValue / 50000);

        /* Write byte 0x15 */
        err = ddTDA182I2Write(pObj, 0x15, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIF_Freq                                        */
/*                                                                            */
/* DESCRIPTION: Get the IF_Freq bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIF_Freq
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32*         puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x15 */
        err = ddTDA182I2Read(pObj, 0x15, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx15.bF.IF_Freq * 50000;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetRF_Freq                                        */
/*                                                                            */
/* DESCRIPTION: Set the RF_Freq bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetRF_Freq
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32          uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;
    UInt32                  uRF = 0;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        //***************************************
        // Configures the settings that depends on the chosen received TV standard 
        // (standard and demodulator dependent)

        //pObj->I2CMap.uBx0E.AGCK_byte_1 = 0x1F;
        //pObj->I2CMap.uBx0F.RF_AGC_byte = 0x15;
        //pObj->I2CMap.uBx10.IR_Mixer_byte_1 = 0x0A;
        //pObj->I2CMap.uBx11.AGC5_byte_1 = 0x0A;
        //pObj->I2CMap.uBx12.IF_AGC_byte = 0x00;
        //pObj->I2CMap.uBx13.IF_Byte_1 = 0xE0;
        //pObj->I2CMap.uBx14.Reference_Byte = 0x03;
        //pObj->I2CMap.uBx15.IF_Frequency_byte = 0x00;

        ///* Write bytes 0x0E to 0x15
        //err = ddTDA182I2Write(pObj, 0x0E, 8);
        //tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));  
        //***************************************

        /*****************************************/
        /* Tune the settings that depend on the RF input frequency, expressed in kHz.*/
        /* RF filters tuning, PLL locking*/
        /* State reached after 5ms*/

        if(err == TM_OK)
        {
            /* Set IRQ_clear */
            err = tmddTDA182I2SetIRQ_Clear(tUnit, 0x0C);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetIRQ_clear(0x%08X, 0x0C) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set power state ON */
            err = tmddTDA182I2SetPowerState(tUnit, tmddTDA182I2_PowerNormalMode);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2SetPowerState(0x%08X, PowerNormalMode) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set RF frequency expressed in kHz */
            uRF = uValue / 1000;
            pObj->I2CMap.uBx16.bF.RF_Freq_1 = (UInt8)((uRF & 0x00FF0000) >> 16);
            pObj->I2CMap.uBx17.bF.RF_Freq_2 = (UInt8)((uRF & 0x0000FF00) >> 8);
            pObj->I2CMap.uBx18.bF.RF_Freq_3 = (UInt8)(uRF & 0x000000FF);

            /* write bytes 0x16 to 0x18*/
            err = ddTDA182I2Write(pObj, 0x16, 3);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            /* Set & trigger MSM */
            pObj->I2CMap.uBx19.MSM_byte_1 = 0x41;
            pObj->I2CMap.uBx1A.MSM_byte_2 = 0x01;

            /* Write bytes 0x19 to 0x1A */
            err = ddTDA182I2Write(pObj, 0x19, 2);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

            pObj->I2CMap.uBx1A.MSM_byte_2 = 0x00;
        }
        if(pObj->bIRQWait)
        {
            if(err == TM_OK)
            {
                err = ddTDA182I2WaitIRQ(pObj, 50, 5, 0x0C);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2WaitIRQ(0x%08X) failed.", tUnit));
            }

            /* PATCH ES2 in case of WAIT IRQ Timeout */
            if(err == ddTDA182I2_ERR_NOT_READY && pObj->I2CMap.uBx02.bF.Major_rev == 1 && pObj->I2CMap.uBx02.bF.Minor_rev == 0)
            {
                pObj->bSoftReset = True;
            }
            /* ---------- ES2 PATCH END ---------- */

            if(err == TM_OK && pObj->bRFCAL_SW_Algo_Enable)
            {
                err = tmddTDA182I2RFFilterCorrTune(tUnit, uValue);
                tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "tmddTDA182I2RFFilterCorrTune(0x%08X) failed.", tUnit));
            }

        }        
        /*****************************************/


        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetRF_Freq                                        */
/*                                                                            */
/* DESCRIPTION: Get the RF_Freq bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetRF_Freq
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt32*         puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read bytes 0x16 to 0x18 */
        err = ddTDA182I2Read(pObj, 0x16, 3);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = (pObj->I2CMap.uBx16.bF.RF_Freq_1 << 16) | (pObj->I2CMap.uBx17.bF.RF_Freq_2 << 8) | pObj->I2CMap.uBx18.bF.RF_Freq_3;
            *puValue = *puValue * 1000;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetMSM_Launch                                     */
/*                                                                            */
/* DESCRIPTION: Set the MSM_Launch bit(s) status                              */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetMSM_Launch
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx1A.bF.MSM_Launch = 1;

        /* Write byte 0x1A */
        err = ddTDA182I2Write(pObj, 0x1A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        /* reset MSM_Launch (buffer only, no write) */
        pObj->I2CMap.uBx1A.bF.MSM_Launch = 0x00;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetMSM_Launch                                     */
/*                                                                            */
/* DESCRIPTION: Get the MSM_Launch bit(s) status                              */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetMSM_Launch
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x1A */
        err = ddTDA182I2Read(pObj, 0x1A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx1A.bF.MSM_Launch;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetPSM_StoB                                       */
/*                                                                            */
/* DESCRIPTION: Set the PSM_StoB bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetPSM_StoB
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx1B.bF.PSM_StoB = uValue;

        /* Read byte 0x1B */
        err = ddTDA182I2Write(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetPSM_StoB                                       */
/*                                                                            */
/* DESCRIPTION: Get the PSM_StoB bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetPSM_StoB
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x1B */
        err = ddTDA182I2Read(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx1B.bF.PSM_StoB;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIR_Loop                                        */
/*                                                                            */
/* DESCRIPTION: Set the IR_Loop bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIR_Loop
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx1E.bF.IR_Loop = uValue - 4;

        /* Read byte 0x1E */
        err = ddTDA182I2Write(pObj, 0x1E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIR_Loop                                        */
/*                                                                            */
/* DESCRIPTION: Get the IR_Loop bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIR_Loop
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x1E */
        err = ddTDA182I2Read(pObj, 0x1E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx1E.bF.IR_Loop + 4;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIR_Target                                      */
/*                                                                            */
/* DESCRIPTION: Set the IR_Target bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIR_Target
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx1E.bF.IR_Target = uValue;

        /* Read byte 0x1E */
        err = ddTDA182I2Write(pObj, 0x1E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIR_Target                                      */
/*                                                                            */
/* DESCRIPTION: Get the IR_Target bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIR_Target
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x1E */
        err = ddTDA182I2Read(pObj, 0x1E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx1E.bF.IR_Target;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIR_Corr_Boost                                  */
/*                                                                            */
/* DESCRIPTION: Set the IR_Corr_Boost bit(s) status                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIR_Corr_Boost
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx1F.bF.IR_Corr_Boost = uValue;

        /* Read byte 0x1F */
        err = ddTDA182I2Write(pObj, 0x1F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIR_Corr_Boost                                  */
/*                                                                            */
/* DESCRIPTION: Get the IR_Corr_Boost bit(s) status                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIR_Corr_Boost
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x1F */
        err = ddTDA182I2Read(pObj, 0x1F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx1F.bF.IR_Corr_Boost;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIR_mode_ram_store                              */
/*                                                                            */
/* DESCRIPTION: Set the IR_mode_ram_store bit(s) status                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIR_mode_ram_store
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx1F.bF.IR_mode_ram_store = uValue;

        /* Write byte 0x1F */
        err = ddTDA182I2Write(pObj, 0x1F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIR_mode_ram_store                              */
/*                                                                            */
/* DESCRIPTION: Get the IR_mode_ram_store bit(s) status                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIR_mode_ram_store
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x1F */
        err = ddTDA182I2Read(pObj, 0x1F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx1F.bF.IR_mode_ram_store;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetPD_Udld                                        */
/*                                                                            */
/* DESCRIPTION: Set the PD_Udld bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetPD_Udld
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx22.bF.PD_Udld = uValue;

        /* Write byte 0x22 */
        err = ddTDA182I2Write(pObj, 0x22, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetPD_Udld                                        */
/*                                                                            */
/* DESCRIPTION: Get the PD_Udld bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetPD_Udld
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x22 */
        err = ddTDA182I2Read(pObj, 0x22, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx22.bF.PD_Udld;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGC_Ovld_TOP                                   */
/*                                                                            */
/* DESCRIPTION: Set the AGC_Ovld_TOP bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGC_Ovld_TOP
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx22.bF.AGC_Ovld_TOP = uValue;

        /* Write byte 0x22 */
        err = ddTDA182I2Write(pObj, 0x22, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGC_Ovld_TOP                                   */
/*                                                                            */
/* DESCRIPTION: Get the AGC_Ovld_TOP bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGC_Ovld_TOP
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x22 */
        err = ddTDA182I2Read(pObj, 0x22, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx22.bF.AGC_Ovld_TOP;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetHi_Pass                                        */
/*                                                                            */
/* DESCRIPTION: Set the Hi_Pass bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetHi_Pass
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx23.bF.Hi_Pass = uValue;

        /* Read byte 0x23 */
        err = ddTDA182I2Write(pObj, 0x23, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetHi_Pass                                        */
/*                                                                            */
/* DESCRIPTION: Get the Hi_Pass bit(s) status                                 */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetHi_Pass
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x23 */
        err = ddTDA182I2Read(pObj, 0x23, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx23.bF.Hi_Pass;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIF_Notch                                       */
/*                                                                            */
/* DESCRIPTION: Set the IF_Notch bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIF_Notch
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx23.bF.IF_Notch = uValue;

        /* Read byte 0x23 */
        err = ddTDA182I2Write(pObj, 0x23, 1);

        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIF_Notch                                       */
/*                                                                            */
/* DESCRIPTION: Get the IF_Notch bit(s) status                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIF_Notch
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x23 */
        err = ddTDA182I2Read(pObj, 0x23, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx23.bF.IF_Notch;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGC5_loop_off                                  */
/*                                                                            */
/* DESCRIPTION: Set the AGC5_loop_off bit(s) status                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGC5_loop_off
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx25.bF.AGC5_loop_off = uValue;

        /* Read byte 0x25 */
        err = ddTDA182I2Write(pObj, 0x25, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGC5_loop_off                                  */
/*                                                                            */
/* DESCRIPTION: Get the AGC5_loop_off bit(s) status                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGC5_loop_off
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x25 */
        err = ddTDA182I2Read(pObj, 0x25, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx25.bF.AGC5_loop_off;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGC5_Do_step                                   */
/*                                                                            */
/* DESCRIPTION: Set the AGC5_Do_step bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGC5_Do_step
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx25.bF.AGC5_Do_step = uValue;

        /* Read byte 0x25 */
        err = ddTDA182I2Write(pObj, 0x25, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGC5_Do_step                                   */
/*                                                                            */
/* DESCRIPTION: Get the AGC5_Do_step bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGC5_Do_step
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x25 */
        err = ddTDA182I2Read(pObj, 0x25, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx25.bF.AGC5_Do_step;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetForce_AGC5_gain                                */
/*                                                                            */
/* DESCRIPTION: Set the Force_AGC5_gain bit(s) status                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetForce_AGC5_gain
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx25.bF.Force_AGC5_gain = uValue;

        /* Read byte 0x25 */
        err = ddTDA182I2Write(pObj, 0x25, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetForce_AGC5_gain                                */
/*                                                                            */
/* DESCRIPTION: Get the Force_AGC5_gain bit(s) status                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetForce_AGC5_gain
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x25 */
        err = ddTDA182I2Read(pObj, 0x25, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx25.bF.Force_AGC5_gain;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetAGC5_Gain                                      */
/*                                                                            */
/* DESCRIPTION: Set the AGC5_Gain bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetAGC5_Gain
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx25.bF.AGC5_Gain = uValue;

        /* Read byte 0x25 */
        err = ddTDA182I2Write(pObj, 0x25, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetAGC5_Gain                                      */
/*                                                                            */
/* DESCRIPTION: Get the AGC5_Gain bit(s) status                               */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetAGC5_Gain
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x25 */
        err = ddTDA182I2Read(pObj, 0x25, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx25.bF.AGC5_Gain;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetRF_Filter_Bypass                               */
/*                                                                            */
/* DESCRIPTION: Set the RF_Filter_Bypass bit(s) status                        */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetRF_Filter_Bypass
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx2C.bF.RF_Filter_Bypass = uValue;

        /* Read byte 0x2C */
        err = ddTDA182I2Write(pObj, 0x2C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetRF_Filter_Bypass                               */
/*                                                                            */
/* DESCRIPTION: Get the RF_Filter_Bypass bit(s) status                        */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetRF_Filter_Bypass
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x2C */
        err = ddTDA182I2Read(pObj, 0x2C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx2C.bF.RF_Filter_Bypass;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetRF_Filter_Band                                 */
/*                                                                            */
/* DESCRIPTION: Set the RF_Filter_Band bit(s) status                          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetRF_Filter_Band
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx2C.bF.RF_Filter_Band = uValue;

        /* Read byte 0x2C */
        err = ddTDA182I2Write(pObj, 0x2C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetRF_Filter_Band                                 */
/*                                                                            */
/* DESCRIPTION: Get the RF_Filter_Band bit(s) status                          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetRF_Filter_Band
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x2C */
        err = ddTDA182I2Read(pObj, 0x2C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx2C.bF.RF_Filter_Band;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetRF_Filter_Cap                                  */
/*                                                                            */
/* DESCRIPTION: Set the RF_Filter_Cap bit(s) status                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetRF_Filter_Cap
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx2D.bF.RF_Filter_Cap = uValue;

        /* Read byte 0x2D */
        err = ddTDA182I2Write(pObj, 0x2D, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetRF_Filter_Cap                                  */
/*                                                                            */
/* DESCRIPTION: Get the RF_Filter_Cap bit(s) status                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetRF_Filter_Cap
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x2D */
        err = ddTDA182I2Read(pObj, 0x2D, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx2D.bF.RF_Filter_Cap;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetGain_Taper                                     */
/*                                                                            */
/* DESCRIPTION: Set the Gain_Taper bit(s) status                              */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetGain_Taper
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx2E.bF.Gain_Taper = uValue;

        /* Read byte 0x2E */
        err = ddTDA182I2Write(pObj, 0x2E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetGain_Taper                                     */
/*                                                                            */
/* DESCRIPTION: Get the Gain_Taper bit(s) status                              */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetGain_Taper
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x2E */
        err = ddTDA182I2Read(pObj, 0x2E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx2E.bF.Gain_Taper;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetN_CP_Current                                   */
/*                                                                            */
/* DESCRIPTION: Set the N_CP_Current bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetN_CP_Current
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx30.bF.N_CP_Current = uValue;

        /* Read byte 0x30 */
        err = ddTDA182I2Write(pObj, 0x30, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetN_CP_Current                                   */
/*                                                                            */
/* DESCRIPTION: Get the N_CP_Current bit(s) status                            */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetN_CP_Current
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x30 */
        err = ddTDA182I2Read(pObj, 0x30, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx30.bF.N_CP_Current;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetRSSI_Ck_Speed                                  */
/*                                                                            */
/* DESCRIPTION: Set the RSSI_Ck_Speed bit(s) status                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetRSSI_Ck_Speed
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx36.bF.RSSI_Ck_Speed = uValue;

        /* Write byte 0x36 */
        err = ddTDA182I2Write(pObj, 0x36, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetRSSI_Ck_Speed                                  */
/*                                                                            */
/* DESCRIPTION: Get the RSSI_Ck_Speed bit(s) status                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetRSSI_Ck_Speed
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x36 */
        err = ddTDA182I2Read(pObj, 0x36, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx36.bF.RSSI_Ck_Speed;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetRFCAL_Phi2                                     */
/*                                                                            */
/* DESCRIPTION: Set the RFCAL_Phi2 bit(s) status                              */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetRFCAL_Phi2
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Set value */
        pObj->I2CMap.uBx37.bF.RFCAL_Phi2 = uValue;

        /* Write byte 0x37 */
        err = ddTDA182I2Write(pObj, 0x37, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetRFCAL_Phi2                                     */
/*                                                                            */
/* DESCRIPTION: Get the RFCAL_Phi2 bit(s) status                              */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetRFCAL_Phi2
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        /* Read byte 0x37 */
        err = ddTDA182I2Read(pObj, 0x37, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx37.bF.RFCAL_Phi2;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2WaitIRQ                                           */
/*                                                                            */
/* DESCRIPTION: Wait the IRQ to trigger                                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2WaitIRQ
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32          timeOut,    /* I: timeout */
    UInt32          waitStep,   /* I: wait step */
    UInt8           irqStatus   /* I: IRQs to wait */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2WaitIRQ(pObj, timeOut, waitStep, irqStatus);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2WaitIRQ(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2WaitXtalCal_End                                   */
/*                                                                            */
/* DESCRIPTION: Wait the MSM_XtalCal_End to trigger                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2WaitXtalCal_End
(
    tmUnitSelect_t  tUnit,      /* I: Unit number */
    UInt32          timeOut,    /* I: timeout */
    UInt32          waitStep    /* I: wait step */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2WaitXtalCal_End(pObj, timeOut, waitStep);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2WaitXtalCal_End(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetIRQWait                                        */
/*                                                                            */
/* DESCRIPTION: Set whether wait IRQ in driver or not                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetIRQWait
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool            bWait   /* I: Determine if we need to wait IRQ in driver functions */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        pObj->bIRQWait = bWait;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetIRQWait                                        */
/*                                                                            */
/* DESCRIPTION: Get whether wait IRQ in driver or not                         */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetIRQWait
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    Bool*           pbWait  /* O: Determine if we need to wait IRQ in driver functions */
)
{     
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (pbWait == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        *pbWait = pObj->bIRQWait;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SoftReset                                         */
/*                                                                            */
/* DESCRIPTION: Soft Reset                                                    */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SoftReset
(
    tmUnitSelect_t  tUnit   /* I: Unit number */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        pObj->bSoftReset = True;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2SetVersionForced                                  */
/*                                                                            */
/* DESCRIPTION: Get TDA182I2 IC version forced                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2SetVersionForced
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8           uValue  /* I: Item value */
)
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        pObj->versionForced = uValue;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    tmddTDA182I2GetVersionForced                                  */
/*                                                                            */
/* DESCRIPTION: Get TDA182I2 IC version forced                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
tmddTDA182I2GetVersionForced
(
    tmUnitSelect_t  tUnit,  /* I: Unit number */
    UInt8*          puValue /* I: Address of the variable to output item value */
)
{     
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        *puValue = pObj->versionForced;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}


/*============================================================================*/
/* FUNCTION:    ddTDA182I2GetIRQ_status                                       */
/*                                                                            */
/* DESCRIPTION: Get IRQ status                                                */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
ddTDA182I2GetIRQ_status
(
    ptmddTDA182I2Object_t   pObj,   /* I: Instance object */
    UInt8*                  puValue /* I: Address of the variable to output item value */
)
{     
    tmErrorCode_t   err  = TM_OK;

    /* Read byte 0x08 */
    err = ddTDA182I2Read(pObj, 0x08, 1);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", pObj->tUnit));

    if(err == TM_OK)
    {
        /* Get value */
        *puValue = pObj->I2CMap.uBx08.bF.IRQ_status;
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    ddTDA182I2GetMSM_XtalCal_End                                  */
/*                                                                            */
/* DESCRIPTION: Get MSM_XtalCal_End bit(s) status                             */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
ddTDA182I2GetMSM_XtalCal_End
(
    ptmddTDA182I2Object_t   pObj,   /* I: Instance object */
    UInt8*                  puValue /* I: Address of the variable to output item value */
)
{
    tmErrorCode_t           err  = TM_OK;

    /* Test the parameters */
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Read byte 0x08 */
        err = ddTDA182I2Read(pObj, 0x08, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", pObj->tUnit));

        if(err == TM_OK)
        {
            /* Get value */
            *puValue = pObj->I2CMap.uBx08.bF.MSM_XtalCal_End;
        }
    }
    return err;
}

/*============================================================================*/
/* FUNCTION:    ddTDA182I2WaitIRQ                                             */
/*                                                                            */
/* DESCRIPTION: Wait for IRQ to trigger                                       */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
ddTDA182I2WaitIRQ
(
    ptmddTDA182I2Object_t   pObj,       /* I: Instance object */
    UInt32                  timeOut,    /* I: timeout */
    UInt32                  waitStep,   /* I: wait step */
    UInt8                   irqStatus   /* I: IRQs to wait */
)
{     
    tmErrorCode_t   err  = TM_OK;
    UInt32          counter = timeOut/waitStep; /* Wait max timeOut/waitStep ms */
    UInt8           uIRQ = 0;
    UInt8           uIRQStatus = 0;
    Bool            bIRQTriggered = False;

    while(err == TM_OK && (--counter)>0)
    {
        err = ddTDA182I2GetIRQ_status(pObj, &uIRQ);

        if(err == TM_OK && uIRQ == 1)
        {
            bIRQTriggered = True;
        }

        if(bIRQTriggered)
        {
            /* IRQ triggered => Exit */
            break;
        }

        if(err == TM_OK && irqStatus != 0x00)
        {
            uIRQStatus = ((pObj->I2CMap.uBx08.IRQ_status)&0x1F);

            if(irqStatus == uIRQStatus)
            {
                bIRQTriggered = True;
            }
        }

        err = ddTDA182I2Wait(pObj, waitStep);
    }

    if(counter == 0)
    {
        err = ddTDA182I2_ERR_NOT_READY;
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    ddTDA182I2WaitXtalCal_End                                     */
/*                                                                            */
/* DESCRIPTION: Wait for MSM_XtalCal_End to trigger                           */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
ddTDA182I2WaitXtalCal_End
(
    ptmddTDA182I2Object_t   pObj,       /* I: Instance object */
    UInt32                  timeOut,    /* I: timeout */
    UInt32                  waitStep    /* I: wait step */
)
{     
    tmErrorCode_t   err  = TM_OK;
    UInt32          counter = timeOut/waitStep; /* Wait max timeOut/waitStepms */
    UInt8           uMSM_XtalCal_End = 0;

    while(err == TM_OK && (--counter)>0)
    {
        err = ddTDA182I2GetMSM_XtalCal_End(pObj, &uMSM_XtalCal_End);

        if(uMSM_XtalCal_End == 1)
        {
            /* MSM_XtalCal_End triggered => Exit */
            break;
        }

        ddTDA182I2Wait(pObj, waitStep);
    }

    if(counter == 0)
    {
        err = ddTDA182I2_ERR_NOT_READY;
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    ddTDA182I2Write                                               */
/*                                                                            */
/* DESCRIPTION: Write in TDA182I2 hardware                                    */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t 
ddTDA182I2Write
(
    ptmddTDA182I2Object_t   pObj,           /* I: Driver object */
    UInt8                   uSubAddress,    /* I: sub address */
    UInt8                   uNbData         /* I: nb of data */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt8*          pI2CMap = Null;

    /* pI2CMap initialization */
    pI2CMap = &(pObj->I2CMap.uBx00.ID_byte_1);

    err = POBJ_SRVFUNC_SIO.Write (pObj->tUnitW, 1, &uSubAddress, uNbData, &(pI2CMap[uSubAddress]));

    /* return value */
    return err;
}

/*============================================================================*/
/* FUNCTION:    ddTDA182I2Read                                                */
/*                                                                            */
/* DESCRIPTION: Read in TDA182I2 hardware                                     */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t 
ddTDA182I2Read
(
    ptmddTDA182I2Object_t   pObj,           /* I: Driver object */
    UInt8                   uSubAddress,    /* I: sub address */
    UInt8                   uNbData         /* I: nb of data */
)
{
    tmErrorCode_t   err = TM_OK;
    UInt8*          pI2CMap = Null;

    /* pRegister initialization */
    pI2CMap = &(pObj->I2CMap.uBx00.ID_byte_1) + uSubAddress;

    /* Read data from the Tuner */
    err = POBJ_SRVFUNC_SIO.Read(pObj->tUnitW, 1, &uSubAddress, uNbData, pI2CMap);

    if(err == TM_OK)
    {
        switch(pObj->versionForced)
        {
        default:
        case tmddTDA182I2_Version_Not_Forced:
            break;

        case tmddTDA182I2_Version_Forced_ES2:
            pObj->I2CMap.uBx02.bF.Major_rev = 1;
            pObj->I2CMap.uBx02.bF.Minor_rev = 0;
            break;

        case tmddTDA182I2_Version_Forced_ES3:
            pObj->I2CMap.uBx02.bF.Major_rev = 1;
            pObj->I2CMap.uBx02.bF.Minor_rev = 1;
            break;
        }
    }

    /* return value */
    return err;
}

/*============================================================================*/
/* FUNCTION:    ddTDA182I2Wait                                                */
/*                                                                            */
/* DESCRIPTION: Wait for the requested time                                   */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t 
ddTDA182I2Wait
(
    ptmddTDA182I2Object_t   pObj,   /* I: Driver object */
    UInt32                  Time    /*  I: time to wait for */
)
{
    tmErrorCode_t   err  = TM_OK;

    /* wait Time ms */
    err = POBJ_SRVFUNC_STIME.Wait (pObj->tUnit, Time);

    /* Return value */
    return err;
}

/*============================================================================*/
/* FUNCTION:    ddTDA182I2MutexAcquire                                        */
/*                                                                            */
/* DESCRIPTION: Acquire driver mutex                                          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
ddTDA182I2MutexAcquire
(
    ptmddTDA182I2Object_t   pObj,
    UInt32                  timeOut
)
{
    tmErrorCode_t   err = TM_OK;

    if(pObj->sMutex.Acquire != Null && pObj->pMutex != Null)
    {
        err = pObj->sMutex.Acquire(pObj->pMutex, timeOut);
    }

    return err;
}

/*============================================================================*/
/* FUNCTION:    ddTDA182I2MutexRelease                                        */
/*                                                                            */
/* DESCRIPTION: Release driver mutex                                          */
/*                                                                            */
/* RETURN:      TM_OK if no error                                             */
/*                                                                            */
/* NOTES:                                                                     */
/*                                                                            */
/*============================================================================*/
tmErrorCode_t
ddTDA182I2MutexRelease
(
    ptmddTDA182I2Object_t   pObj
)
{
    tmErrorCode_t   err = TM_OK;

    if(pObj->sMutex.Release != Null && pObj->pMutex != Null)
    {
        err = pObj->sMutex.Release(pObj->pMutex);
    }

    return err;
}























// NXP source code - .\tmddTDA182I2\src\tmddTDA182I2Instance.c


/*-----------------------------------------------------------------------------
// $Header: 
// (C) Copyright 2008 NXP Semiconductors, All rights reserved
//
// This source code and any compilation or derivative thereof is the sole
// property of NXP Corporation and is provided pursuant to a Software
// License Agreement.  This code is the proprietary information of NXP
// Corporation and is confidential in nature.  Its use and dissemination by
// any party other than NXP Corporation is strictly limited by the
// confidential information provisions of the Agreement referenced above.
//-----------------------------------------------------------------------------
// FILE NAME:    tmddTDA182I2Instance.c
//
// DESCRIPTION:  define the static Objects
//
// DOCUMENT REF: DVP Software Coding Guidelines v1.14
//               DVP Board Support Library Architecture Specification v0.5
//
// NOTES:        
//-----------------------------------------------------------------------------
*/

//#include "tmNxTypes.h"
//#include "tmCompId.h"
//#include "tmFrontEnd.h"
//#include "tmUnitParams.h"
//#include "tmbslFrontEndTypes.h"

//#include "tmddTDA182I2.h"
//#include "tmddTDA182I2local.h"

//#include "tmddTDA182I2Instance.h"

/*-----------------------------------------------------------------------------
// Global data:
//-----------------------------------------------------------------------------
//
*/


/* default instance */
tmddTDA182I2Object_t gddTDA182I2Instance[] = 
{
    {
        (tmUnitSelect_t)(-1),           /* Unit not set */
        (tmUnitSelect_t)(-1),           /* UnitW not set */
        Null,                           /* pMutex */
        False,                          /* init (instance initialization default) */
        {                               /* sRWFunc */
            Null,
            Null
        },
        {                               /* sTime */
            Null,
            Null
        },
        {                               /* sDebug */
            Null
        },
        {                               /* sMutex */
            Null,
            Null,
            Null,
            Null
        },
        tmddTDA182I2_PowerStandbyWithXtalOn,    /* curPowerState */
        tmddTDA182I2_Version_Not_Forced,        /* versionForced */
        True,                                   /* bIRQWait */
        False,                                  /* bSoftReset */
        False,                                  /* bRFCAL_SW_Algo_Enable */
        {                                       /* RFCAL_Prog */
            { 0, 0, 0 },
            { 1, 0, 0 },
            { 2, 0, 0 },
            { 3, 0, 0 },
            { 4, 0, 0 },
            { 5, 0, 0 },
            { 6, 0, 0 },
            { 7, 0, 0 },
            { 8, 0, 0 },
            { 9, 0, 0 },
            { 10, 0, 0 },
            { 11, 0, 0 }
        },
        {                                       /* RFCAL_Coeffs */
            { 0, 0, 0, 0 },
            { 1, 1, 0, 0 },
            { 2, 3, 0, 0 },
            { 3, 4, 0, 0 },
            { 4, 6, 0, 0 },
            { 5, 7, 0, 0 },
            { 6, 9, 0, 0 },
            { 7, 10, 0, 0 }
        },
        {
            0  // I2CMap;
        }
    },
    {
        (tmUnitSelect_t)(-1),           /* Unit not set */
        (tmUnitSelect_t)(-1),           /* UnitW not set */
        Null,                           /* pMutex */
        False,                          /* init (instance initialization default) */
        {                               /* sRWFunc */
            Null,
            Null
        },
        {                               /* sTime */
            Null,
            Null
        },
        {                               /* sDebug */
            Null
        },
        {                               /* sMutex */
            Null,
            Null,
            Null,
            Null
        },
        tmddTDA182I2_PowerStandbyWithXtalOn,    /* curPowerState */
        tmddTDA182I2_Version_Not_Forced,        /* versionForced */
        True,                                   /* bIRQWait */
        False,                                  /* bSoftReset */
        False,                                  /* bRFCAL_SW_Algo_Enable */
        {                                       /* RFCAL_Prog */
            { 0, 0, 0 },
            { 1, 0, 0 },
            { 2, 0, 0 },
            { 3, 0, 0 },
            { 4, 0, 0 },
            { 5, 0, 0 },
            { 6, 0, 0 },
            { 7, 0, 0 },
            { 8, 0, 0 },
            { 9, 0, 0 },
            { 10, 0, 0 },
            { 11, 0, 0 }
        },
        {                                       /* RFCAL_Coeffs */
            { 0, 0, 0, 0 },
            { 1, 1, 0, 0 },
            { 2, 3, 0, 0 },
            { 3, 4, 0, 0 },
            { 4, 6, 0, 0 },
            { 5, 7, 0, 0 },
            { 6, 9, 0, 0 },
            { 7, 10, 0, 0 }
        },
        {
            0  // I2CMap;
        }
    }
};

/*-----------------------------------------------------------------------------
// FUNCTION:    ddTDA182I2AllocInstance:
//
// DESCRIPTION: allocate new instance
//
// RETURN:      
//
// NOTES:       
//-----------------------------------------------------------------------------
*/
tmErrorCode_t
ddTDA182I2AllocInstance
(
 tmUnitSelect_t          tUnit,      /* I: Unit number */
 pptmddTDA182I2Object_t    ppDrvObject /* I: Device Object */
 )
{ 
    tmErrorCode_t       err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;
    ptmddTDA182I2Object_t pObj = Null;
    UInt32              uLoopCounter = 0;    

    /* Find a free instance */
    for(uLoopCounter = 0; uLoopCounter<TDA182I2_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gddTDA182I2Instance[uLoopCounter];
        if(pObj->init == False)
        {
            pObj->tUnit = tUnit;
            pObj->tUnitW = tUnit;

            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    /* return value */
    return err;
}

/*-----------------------------------------------------------------------------
// FUNCTION:    ddTDA182I2DeAllocInstance:
//
// DESCRIPTION: deallocate instance
//
// RETURN:      always TM_OK
//
// NOTES:       
//-----------------------------------------------------------------------------
*/
tmErrorCode_t
ddTDA182I2DeAllocInstance
(
 tmUnitSelect_t  tUnit   /* I: Unit number */
 )
{     
    tmErrorCode_t       err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;
    ptmddTDA182I2Object_t pObj = Null;

    /* check input parameters */
    err = ddTDA182I2GetInstance(tUnit, &pObj);

    /* check driver state */
    if (err == TM_OK)
    {
        if (pObj == Null || pObj->init == False)
        {
            err = ddTDA182I2_ERR_NOT_INITIALIZED;
        }
    }

    if ((err == TM_OK) && (pObj != Null)) 
    {
        pObj->init = False;
    }

    /* return value */
    return err;
}

/*-----------------------------------------------------------------------------
// FUNCTION:    ddTDA182I2GetInstance:
//
// DESCRIPTION: get the instance
//
// RETURN:      always True
//
// NOTES:       
//-----------------------------------------------------------------------------
*/
tmErrorCode_t
ddTDA182I2GetInstance
(
 tmUnitSelect_t          tUnit,      /* I: Unit number */
 pptmddTDA182I2Object_t    ppDrvObject /* I: Device Object */
 )
{     
    tmErrorCode_t           err = ddTDA182I2_ERR_NOT_INITIALIZED;
    ptmddTDA182I2Object_t     pObj = Null;
    UInt32                  uLoopCounter = 0;    

    /* get instance */
    for(uLoopCounter = 0; uLoopCounter<TDA182I2_MAX_UNITS; uLoopCounter++)
    {
        pObj = &gddTDA182I2Instance[uLoopCounter];
        if(pObj->init == True && pObj->tUnit == GET_INDEX_TYPE_TUNIT(tUnit))
        {
            pObj->tUnitW = tUnit;

            *ppDrvObject = pObj;
            err = TM_OK;
            break;
        }
    }

    /* return value */
    return err;
}























// NXP source code - .\tmddTDA182I2\src\tmddTDA182I2_Advanced.c


/*-----------------------------------------------------------------------------
// $Header: 
// (C) Copyright 2008 NXP Semiconductors, All rights reserved
//
// This source code and any compilation or derivative thereof is the sole
// property of Philips Corporation and is provided pursuant to a Software
// License Agreement.  This code is the proprietary information of Philips
// Corporation and is confidential in nature.  Its use and dissemination by
// any party other than Philips Corporation is strictly limited by the
// confidential information provisions of the Agreement referenced above.
//-----------------------------------------------------------------------------
// FILE NAME:    tmddTDA182I2_Alt.c
//
// DESCRIPTION:  TDA182I2 standard APIs
//
// NOTES:        
//-----------------------------------------------------------------------------
*/

//#include "tmNxTypes.h"
//#include "tmCompId.h"
//#include "tmFrontEnd.h"
//#include "tmbslFrontEndTypes.h"

//#include "tmddTDA182I2.h"
//#include "tmddTDA182I2local.h"

//#include "tmddTDA182I2Instance.h"

/*-----------------------------------------------------------------------------
// Project include files:
//-----------------------------------------------------------------------------
*/

/*-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
*/

/*-----------------------------------------------------------------------------
// Global data:
//-----------------------------------------------------------------------------
*/

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetTM_D:
//
// DESCRIPTION: Get the TM_D bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetTM_D
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // switch thermometer on
        pObj->I2CMap.uBx04.bF.TM_ON = 1;

        // write byte 0x04
        err = ddTDA182I2Write(pObj, 0x04, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // read byte 0x03
            err = ddTDA182I2Read(pObj, 0x03, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));
        }

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx03.bF.TM_D;

            // switch thermometer off
            pObj->I2CMap.uBx04.bF.TM_ON = 0;

            // write byte 0x04
            err = ddTDA182I2Write(pObj, 0x04, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetTM_ON:
//
// DESCRIPTION: Set the TM_ON bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetTM_ON
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx04.bF.TM_ON = uValue;

        // write byte 0x04
        err = ddTDA182I2Write(pObj, 0x04, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetTM_ON:
//
// DESCRIPTION: Get the TM_ON bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetTM_ON
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x04
        err = ddTDA182I2Read(pObj, 0x04, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx04.bF.TM_ON;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetPOR:
//
// DESCRIPTION: Get the POR bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetPOR
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*        puValue      //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x05
        err = ddTDA182I2Read(pObj, 0x05, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx05.bF.POR ;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RSSI_End:
//
// DESCRIPTION: Get the MSM_RSSI_End bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RSSI_End
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*        puValue   //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x08
        err = ddTDA182I2Read(pObj, 0x08, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx08.bF.MSM_RSSI_End;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_LOCalc_End:
//
// DESCRIPTION: Get the MSM_LOCalc_End bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_LOCalc_End
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x08
        err = ddTDA182I2Read(pObj, 0x08, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx08.bF.MSM_LOCalc_End;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RFCal_End:
//
// DESCRIPTION: Get the MSM_RFCal_End bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RFCal_End
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x08
        err = ddTDA182I2Read(pObj, 0x08, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx08.bF.MSM_RFCal_End;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_IRCAL_End:
//
// DESCRIPTION: Get the MSM_IRCAL_End bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_IRCAL_End
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*        puValue      //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x08
        err = ddTDA182I2Read(pObj, 0x08, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx08.bF.MSM_IRCAL_End;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RCCal_End:
//
// DESCRIPTION: Get the MSM_RCCal_End bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RCCal_End
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x08
        err = ddTDA182I2Read(pObj, 0x08, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx08.bF.MSM_RCCal_End;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIRQ_Enable:
//
// DESCRIPTION: Set the IRQ_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIRQ_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx09.bF.IRQ_Enable = uValue;

        // write byte 0x09
        err = ddTDA182I2Write(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIRQ_Enable:
//
// DESCRIPTION: Get the IRQ_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIRQ_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*        puValue      //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x09
        err = ddTDA182I2Read(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx09.bF.IRQ_Enable;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetXtalCal_Enable:
//
// DESCRIPTION: Set the XtalCal_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetXtalCal_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx09.bF.XtalCal_Enable = uValue;

        // write byte 0x09
        err = ddTDA182I2Write(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetXtalCal_Enable:
//
// DESCRIPTION: Get the XtalCal_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetXtalCal_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*        puValue      //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x09
        err = ddTDA182I2Read(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx09.bF.XtalCal_Enable;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_RSSI_Enable:
//
// DESCRIPTION: Set the MSM_RSSI_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_RSSI_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx09.bF.MSM_RSSI_Enable = uValue;

        // write byte 0x09
        err = ddTDA182I2Write(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RSSI_Enable:
//
// DESCRIPTION: Get the MSM_RSSI_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RSSI_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*        puValue      //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x09
        err = ddTDA182I2Read(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx09.bF.MSM_RSSI_Enable;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_LOCalc_Enable:
//
// DESCRIPTION: Set the MSM_LOCalc_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_LOCalc_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8          uValue      //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx09.bF.MSM_LOCalc_Enable = uValue;

        // write byte 0x09
        err = ddTDA182I2Write(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_LOCalc_Enable:
//
// DESCRIPTION: Get the MSM_LOCalc_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_LOCalc_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*        puValue      //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x09
        err = ddTDA182I2Read(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx09.bF.MSM_LOCalc_Enable;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_RFCAL_Enable:
//
// DESCRIPTION: Set the MSM_RFCAL_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_RFCAL_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx09.bF.MSM_RFCAL_Enable = uValue;

        // write byte 0x09
        err = ddTDA182I2Write(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RFCAL_Enable:
//
// DESCRIPTION: Get the MSM_RFCAL_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RFCAL_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x09
        err = ddTDA182I2Read(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx09.bF.MSM_RFCAL_Enable;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_IRCAL_Enable:
//
// DESCRIPTION: Set the MSM_IRCAL_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_IRCAL_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx09.bF.MSM_IRCAL_Enable = uValue;

        // write byte 0x09
        err = ddTDA182I2Write(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_IRCAL_Enable:
//
// DESCRIPTION: Get the MSM_IRCAL_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_IRCAL_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x09
        err = ddTDA182I2Read(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx09.bF.MSM_IRCAL_Enable;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_RCCal_Enable:
//
// DESCRIPTION: Set the MSM_RCCal_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_RCCal_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx09.bF.MSM_RCCal_Enable = uValue;

        // write byte 0x09
        err = ddTDA182I2Write(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RCCal_Enable:
//
// DESCRIPTION: Get the MSM_RCCal_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RCCal_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x09
        err = ddTDA182I2Read(pObj, 0x09, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx09.bF.MSM_RCCal_Enable;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetXtalCal_Clear:
//
// DESCRIPTION: Set the XtalCal_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetXtalCal_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8          uValue      //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0A.bF.XtalCal_Clear = uValue;

        // write byte 0x0A
        err = ddTDA182I2Write(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetXtalCal_Clear:
//
// DESCRIPTION: Get the XtalCal_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetXtalCal_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*        puValue      //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0A
        err = ddTDA182I2Read(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0A.bF.XtalCal_Clear;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_RSSI_Clear:
//
// DESCRIPTION: Set the MSM_RSSI_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_RSSI_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8          uValue      //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0A.bF.MSM_RSSI_Clear = uValue;

        // write byte 0x0A
        err = ddTDA182I2Write(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RSSI_Clear:
//
// DESCRIPTION: Get the MSM_RSSI_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RSSI_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0A
        err = ddTDA182I2Read(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0A.bF.MSM_RSSI_Clear;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_LOCalc_Clear:
//
// DESCRIPTION: Set the MSM_LOCalc_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_LOCalc_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0A.bF.MSM_LOCalc_Clear = uValue;

        // write byte 0x0A
        err = ddTDA182I2Write(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_LOCalc_Clear:
//
// DESCRIPTION: Get the MSM_LOCalc_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_LOCalc_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0A
        err = ddTDA182I2Read(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0A.bF.MSM_LOCalc_Clear;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_RFCal_Clear:
//
// DESCRIPTION: Set the MSM_RFCal_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_RFCal_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0A.bF.MSM_RFCal_Clear = uValue;

        // write byte 0x0A
        err = ddTDA182I2Write(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RFCal_Clear:
//
// DESCRIPTION: Get the MSM_RFCal_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RFCal_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0A
        err = ddTDA182I2Read(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0A.bF.MSM_RFCal_Clear;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_IRCAL_Clear:
//
// DESCRIPTION: Set the MSM_IRCAL_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_IRCAL_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8          uValue      //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0A.bF.MSM_IRCAL_Clear = uValue;

        // write byte 0x0A
        err = ddTDA182I2Write(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_IRCAL_Clear:
//
// DESCRIPTION: Get the MSM_IRCAL_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_IRCAL_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0A
        err = ddTDA182I2Read(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0A.bF.MSM_IRCAL_Clear;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_RCCal_Clear:
//
// DESCRIPTION: Set the MSM_RCCal_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_RCCal_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0A.bF.MSM_RCCal_Clear = uValue;

        // write byte 0x0A
        err = ddTDA182I2Write(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RCCal_Clear:
//
// DESCRIPTION: Get the MSM_RCCal_Clear bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RCCal_Clear
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0A
        err = ddTDA182I2Read(pObj, 0x0A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0A.bF.MSM_RCCal_Clear;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIRQ_Set:
//
// DESCRIPTION: Set the IRQ_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIRQ_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0B.bF.IRQ_Set = uValue;

        // write byte 0x0B
        err = ddTDA182I2Write(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIRQ_Set:
//
// DESCRIPTION: Get the IRQ_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIRQ_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0B
        err = ddTDA182I2Read(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0B.bF.IRQ_Set;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetXtalCal_Set:
//
// DESCRIPTION: Set the XtalCal_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetXtalCal_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0B.bF.XtalCal_Set = uValue;

        // write byte 0x0B
        err = ddTDA182I2Write(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetXtalCal_Set:
//
// DESCRIPTION: Get the XtalCal_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetXtalCal_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0B
        err = ddTDA182I2Read(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0B.bF.XtalCal_Set;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_RSSI_Set:
//
// DESCRIPTION: Set the MSM_RSSI_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_RSSI_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0B.bF.MSM_RSSI_Set = uValue;

        // write byte 0x0B
        err = ddTDA182I2Write(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RSSI_Set:
//
// DESCRIPTION: Get the MSM_RSSI_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RSSI_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0B
        err = ddTDA182I2Read(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0B.bF.MSM_RSSI_Set;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_LOCalc_Set:
//
// DESCRIPTION: Set the MSM_LOCalc_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_LOCalc_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0B.bF.MSM_LOCalc_Set = uValue;

        // write byte 0x0B
        err = ddTDA182I2Write(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_LOCalc_Set:
//
// DESCRIPTION: Get the MSM_LOCalc_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_LOCalc_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0B
        err = ddTDA182I2Read(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        // get value
        *puValue = pObj->I2CMap.uBx0B.bF.MSM_LOCalc_Set;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_RFCal_Set:
//
// DESCRIPTION: Set the MSM_RFCal_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_RFCal_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0B.bF.MSM_RFCal_Set = uValue;

        // write byte 0x0B
        err = ddTDA182I2Write(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RFCal_Set:
//
// DESCRIPTION: Get the MSM_RFCal_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RFCal_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0B
        err = ddTDA182I2Read(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0B.bF.MSM_RFCal_Set;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_IRCAL_Set:
//
// DESCRIPTION: Set the MSM_IRCAL_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_IRCAL_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0B.bF.MSM_IRCAL_Set = uValue;

        // write byte 0x0B
        err = ddTDA182I2Write(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_IRCAL_Set:
//
// DESCRIPTION: Get the MSM_IRCAL_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_IRCAL_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0B
        err = ddTDA182I2Read(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0B.bF.MSM_IRCAL_Set;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetMSM_RCCal_Set:
//
// DESCRIPTION: Set the MSM_RCCal_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetMSM_RCCal_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8            uValue //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0B.bF.MSM_RCCal_Set = uValue;

        // write byte 0x0B
        err = ddTDA182I2Write(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetMSM_RCCal_Set:
//
// DESCRIPTION: Get the MSM_RCCal_Set bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetMSM_RCCal_Set
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0B
        err = ddTDA182I2Read(pObj, 0x0B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0B.bF.MSM_RCCal_Set;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetLT_Enable:
//
// DESCRIPTION: Set the LT_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetLT_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0C.bF.LT_Enable = uValue;

        // write byte 0x0C
        err = ddTDA182I2Write(pObj, 0x0C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetLT_Enable:
//
// DESCRIPTION: Get the LT_Enable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetLT_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0C
        err = ddTDA182I2Read(pObj, 0x0C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0C.bF.LT_Enable;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetAGC1_6_15dB:
//
// DESCRIPTION: Set the AGC1_6_15dB bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetAGC1_6_15dB
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0C.bF.AGC1_6_15dB = uValue;

        // write byte 0x0C
        err = ddTDA182I2Write(pObj, 0x0C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC1_6_15dB:
//
// DESCRIPTION: Get the AGC1_6_15dB bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC1_6_15dB
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0C
        err = ddTDA182I2Read(pObj, 0x0C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0C.bF.AGC1_6_15dB;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetAGCs_Up_Step_assym:
//
// DESCRIPTION: Set the AGCs_Up_Step_assym bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetAGCs_Up_Step_assym
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0E.bF.AGCs_Up_Step_assym = uValue;

        // write byte 0x0E
        err = ddTDA182I2Write(pObj, 0x0E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGCs_Up_Step_assym:
//
// DESCRIPTION: Get the AGCs_Up_Step_assym bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGCs_Up_Step_assym
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*        puValue      //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0E
        err = ddTDA182I2Read(pObj, 0x0E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0E.bF.AGCs_Up_Step_assym;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetPulse_Shaper_Disable:
//
// DESCRIPTION: Set the Pulse_Shaper_Disable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetPulse_Shaper_Disable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0E.bF.Pulse_Shaper_Disable = uValue;

        // write byte 0x0E
        err = ddTDA182I2Write(pObj, 0x0E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetPulse_Shaper_Disable:
//
// DESCRIPTION: Get the Pulse_Shaper_Disable bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetPulse_Shaper_Disable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*        puValue      //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0E
        err = ddTDA182I2Read(pObj, 0x0E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0E.bF.Pulse_Shaper_Disable;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFAGC_Low_BW:
//
// DESCRIPTION: Set the RFAGC_Low_BW bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFAGC_Low_BW
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx0F.bF.RFAGC_Low_BW = uValue;

        // write byte 0x0F
        err = ddTDA182I2Write(pObj, 0x0F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFAGC_Low_BW:
//
// DESCRIPTION: Get the RFAGC_Low_BW bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFAGC_Low_BW
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x0F
        err = ddTDA182I2Read(pObj, 0x0F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx0F.bF.RFAGC_Low_BW;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetAGCs_Do_Step_assym:
//
// DESCRIPTION: Set the AGCs_Do_Step_assym bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetAGCs_Do_Step_assym
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx11.bF.AGCs_Do_Step_assym = uValue;

        // write byte 0x11
        err = ddTDA182I2Write(pObj, 0x11, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGCs_Do_Step_assym:
//
// DESCRIPTION: Get the AGCs_Do_Step_assym bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGCs_Do_Step_assym
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x11
        err = ddTDA182I2Read(pObj, 0x11, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx11.bF.AGCs_Do_Step_assym;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }
    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetI2C_Clock_Mode:
//
// DESCRIPTION: Set the I2C_Clock_Mode bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetI2C_Clock_Mode
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx14.bF.I2C_Clock_Mode = uValue;

        // write byte 0x14
        err = ddTDA182I2Write(pObj, 0x14, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetI2C_Clock_Mode:
//
// DESCRIPTION: Get the I2C_Clock_Mode bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetI2C_Clock_Mode
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x14
        err = ddTDA182I2Read(pObj, 0x14, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx14.bF.I2C_Clock_Mode;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetXtalOsc_AnaReg_En:
//
// DESCRIPTION: Set the XtalOsc_AnaReg_En bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetXtalOsc_AnaReg_En
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx14.bF.XtalOsc_AnaReg_En = uValue;

        // write byte 0x14
        err = ddTDA182I2Write(pObj, 0x14, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetXtalOsc_AnaReg_En:
//
// DESCRIPTION: Get the XtalOsc_AnaReg_En bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetXtalOsc_AnaReg_En
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x14
        err = ddTDA182I2Read(pObj, 0x14, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx14.bF.XtalOsc_AnaReg_En;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetXTout:
//
// DESCRIPTION: Set the XTout bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetXTout
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx14.bF.XTout = uValue;

        // write byte 0x14
        err = ddTDA182I2Write(pObj, 0x14, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetXTout:
//
// DESCRIPTION: Get the XTout bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetXTout
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x14
        err = ddTDA182I2Read(pObj, 0x14, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx14.bF.XTout;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRSSI_Meas:
//
// DESCRIPTION: Set the RSSI_Meas bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRSSI_Meas
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx19.bF.RSSI_Meas = uValue;

        // read byte 0x19
        err = ddTDA182I2Write(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRSSI_Meas:
//
// DESCRIPTION: Get the RSSI_Meas bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRSSI_Meas
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x19
        err = ddTDA182I2Read(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx19.bF.RSSI_Meas;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRF_CAL_AV:
//
// DESCRIPTION: Set the RF_CAL_AV bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRF_CAL_AV
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx19.bF.RF_CAL_AV = uValue;

        // read byte 0x19
        err = ddTDA182I2Write(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRF_CAL_AV:
//
// DESCRIPTION: Get the RF_CAL_AV bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRF_CAL_AV
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x19
        err = ddTDA182I2Read(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx19.bF.RF_CAL_AV;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRF_CAL:
//
// DESCRIPTION: Set the RF_CAL bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRF_CAL
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx19.bF.RF_CAL = uValue;

        // read byte 0x19
        err = ddTDA182I2Write(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRF_CAL:
//
// DESCRIPTION: Get the RF_CAL bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRF_CAL
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue      //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x19
        err = ddTDA182I2Read(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx19.bF.RF_CAL;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIR_CAL_Loop:
//
// DESCRIPTION: Set the IR_CAL_Loop bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIR_CAL_Loop
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx19.bF.IR_CAL_Loop = uValue;

        // read byte 0x19
        err = ddTDA182I2Write(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIR_CAL_Loop:
//
// DESCRIPTION: Get the IR_CAL_Loop bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIR_CAL_Loop
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;
    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x19
        err = ddTDA182I2Read(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx19.bF.IR_CAL_Loop;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIR_Cal_Image:
//
// DESCRIPTION: Set the IR_Cal_Image bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIR_Cal_Image
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx19.bF.IR_Cal_Image = uValue;

        // read byte 0x19
        err = ddTDA182I2Write(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIR_Cal_Image:
//
// DESCRIPTION: Get the IR_Cal_Image bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIR_Cal_Image
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x19
        err = ddTDA182I2Read(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx19.bF.IR_Cal_Image;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIR_CAL_Wanted:
//
// DESCRIPTION: Set the IR_CAL_Wanted bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIR_CAL_Wanted
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx19.bF.IR_CAL_Wanted = uValue;

        // read byte 0x19
        err = ddTDA182I2Write(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIR_CAL_Wanted:
//
// DESCRIPTION: Get the IR_CAL_Wanted bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIR_CAL_Wanted
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x19
        err = ddTDA182I2Read(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx19.bF.IR_CAL_Wanted;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRC_Cal:
//
// DESCRIPTION: Set the RC_Cal bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRC_Cal
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx19.bF.RC_Cal = uValue;

        // read byte 0x19
        err = ddTDA182I2Write(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRC_Cal:
//
// DESCRIPTION: Get the RC_Cal bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRC_Cal
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x19
        err = ddTDA182I2Read(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx19.bF.RC_Cal;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetCalc_PLL:
//
// DESCRIPTION: Set the Calc_PLL bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetCalc_PLL
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx19.bF.Calc_PLL = uValue;

        // read byte 0x19
        err = ddTDA182I2Write(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetCalc_PLL:
//
// DESCRIPTION: Get the Calc_PLL bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetCalc_PLL
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x19
        err = ddTDA182I2Read(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx19.bF.Calc_PLL;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetXtalCal_Launch:
//
// DESCRIPTION: Set the XtalCal_Launch bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetXtalCal_Launch
(
 tmUnitSelect_t      tUnit    //  I: Unit number
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1A.bF.XtalCal_Launch = 1;

        // write byte 0x1A
        err = ddTDA182I2Write(pObj, 0x1A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        // reset XtalCal_Launch (buffer only, no write)
        pObj->I2CMap.uBx1A.bF.XtalCal_Launch = 0;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetXtalCal_Launch:
//
// DESCRIPTION: Get the XtalCal_Launch bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetXtalCal_Launch
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1A
        err = ddTDA182I2Read(pObj, 0x1A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1A.bF.XtalCal_Launch;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetPSM_AGC1:
//
// DESCRIPTION: Set the PSM_AGC1 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetPSM_AGC1
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1B.bF.PSM_AGC1 = uValue;

        // read byte 0x1B
        err = ddTDA182I2Write(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetPSM_AGC1:
//
// DESCRIPTION: Get the PSM_AGC1 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetPSM_AGC1
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1B
        err = ddTDA182I2Read(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1B.bF.PSM_AGC1;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetPSMRFpoly:
//
// DESCRIPTION: Set the PSMRFpoly bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetPSMRFpoly
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1B.bF.PSMRFpoly = uValue;

        // read byte 0x1B
        err = ddTDA182I2Write(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetPSMRFpoly:
//
// DESCRIPTION: Get the PSMRFpoly bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetPSMRFpoly
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1B
        err = ddTDA182I2Read(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        // get value
        *puValue = pObj->I2CMap.uBx1B.bF.PSMRFpoly;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetPSM_Mixer:
//
// DESCRIPTION: Set the PSM_Mixer bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetPSM_Mixer
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1B.bF.PSM_Mixer = uValue;

        // read byte 0x1B
        err = ddTDA182I2Write(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetPSM_Mixer:
//
// DESCRIPTION: Get the PSM_Mixer bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetPSM_Mixer
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1B
        err = ddTDA182I2Read(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1B.bF.PSM_Mixer;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetPSM_Ifpoly:
//
// DESCRIPTION: Set the PSM_Ifpoly bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetPSM_Ifpoly
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1B.bF.PSM_Ifpoly = uValue;

        // read byte 0x1B
        err = ddTDA182I2Write(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetPSM_Ifpoly:
//
// DESCRIPTION: Get the PSM_Ifpoly bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetPSM_Ifpoly
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1B
        err = ddTDA182I2Read(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1B.bF.PSM_Ifpoly;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetPSM_Lodriver:
//
// DESCRIPTION: Set the PSM_Lodriver bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetPSM_Lodriver
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1B.bF.PSM_Lodriver = uValue;

        // read byte 0x1B
        err = ddTDA182I2Write(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetPSM_Lodriver:
//
// DESCRIPTION: Get the PSM_Lodriver bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetPSM_Lodriver
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1B
        err = ddTDA182I2Read(pObj, 0x1B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1B.bF.PSM_Lodriver;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetDCC_Bypass:
//
// DESCRIPTION: Set the DCC_Bypass bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetDCC_Bypass
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1C.bF.DCC_Bypass = uValue;

        // read byte 0x1C
        err = ddTDA182I2Write(pObj, 0x1C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetDCC_Bypass:
//
// DESCRIPTION: Get the DCC_Bypass bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetDCC_Bypass
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1C
        err = ddTDA182I2Read(pObj, 0x1C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1C.bF.DCC_Bypass;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetDCC_Slow:
//
// DESCRIPTION: Set the DCC_Slow bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetDCC_Slow
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1C.bF.DCC_Slow = uValue;

        // read byte 0x1C
        err = ddTDA182I2Write(pObj, 0x1C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetDCC_Slow:
//
// DESCRIPTION: Get the DCC_Slow bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetDCC_Slow
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1C
        err = ddTDA182I2Read(pObj, 0x1C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1C.bF.DCC_Slow;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetDCC_psm:
//
// DESCRIPTION: Set the DCC_psm bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetDCC_psm
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1C.bF.DCC_psm = uValue;

        // read byte 0x1C
        err = ddTDA182I2Write(pObj, 0x1C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetDCC_psm:
//
// DESCRIPTION: Get the DCC_psm bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetDCC_psm
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1C
        err = ddTDA182I2Read(pObj, 0x1C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1C.bF.DCC_psm;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetFmax_Lo:
//
// DESCRIPTION: Set the Fmax_Lo bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetFmax_Lo
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1D.bF.Fmax_Lo = uValue;

        // read byte 0x1D
        err = ddTDA182I2Write(pObj, 0x1D, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetFmax_Lo:
//
// DESCRIPTION: Get the Fmax_Lo bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetFmax_Lo
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1D
        err = ddTDA182I2Read(pObj, 0x1D, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1D.bF.Fmax_Lo;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIR_GStep:
//
// DESCRIPTION: Set the IR_GStep bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIR_GStep
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1E.bF.IR_GStep = uValue - 40;

        // read byte 0x1E
        err = ddTDA182I2Write(pObj, 0x1E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIR_GStep:
//
// DESCRIPTION: Get the IR_GStep bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIR_GStep
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1E
        err = ddTDA182I2Read(pObj, 0x1E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1E.bF.IR_GStep + 40;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIR_FreqLow_Sel:
//
// DESCRIPTION: Set the IR_FreqLow_Sel bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIR_FreqLow_Sel
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1F.bF.IR_FreqLow_Sel = uValue;

        // read byte 0x1F
        err = ddTDA182I2Write(pObj, 0x1F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIR_FreqLow_Sel:
//
// DESCRIPTION: Get the IR_FreqLow_Sel bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIR_FreqLow_Sel
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1F
        err = ddTDA182I2Read(pObj, 0x1F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1F.bF.IR_FreqLow_Sel;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIR_FreqLow:
//
// DESCRIPTION: Set the IR_FreqLow bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIR_FreqLow
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx1F.bF.IR_FreqLow = uValue;

        // read byte 0x1F
        err = ddTDA182I2Write(pObj, 0x1F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIR_FreqLow:
//
// DESCRIPTION: Get the IR_FreqLow bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIR_FreqLow
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x1F
        err = ddTDA182I2Read(pObj, 0x1F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx1F.bF.IR_FreqLow;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIR_FreqMid:
//
// DESCRIPTION: Set the IR_FreqMid bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIR_FreqMid
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx20.bF.IR_FreqMid = uValue;

        // read byte 0x20
        err = ddTDA182I2Write(pObj, 0x20, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIR_FreqMid:
//
// DESCRIPTION: Get the IR_FreqMid bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIR_FreqMid
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x20
        err = ddTDA182I2Read(pObj, 0x20, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx20.bF.IR_FreqMid;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetCoarse_IR_FreqHigh:
//
// DESCRIPTION: Set the Coarse_IR_FreqHigh bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetCoarse_IR_FreqHigh
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx21.bF.Coarse_IR_FreqHigh = uValue;

        // write byte 0x21
        err = ddTDA182I2Write(pObj, 0x21, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetCoarse_IR_FreqHigh:
//
// DESCRIPTION: Get the Coarse_IR_FreqHigh bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetCoarse_IR_FreqHigh
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x21
        err = ddTDA182I2Read(pObj, 0x21, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx21.bF.Coarse_IR_FreqHigh;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIR_FreqHigh:
//
// DESCRIPTION: Set the IR_FreqHigh bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIR_FreqHigh
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;


    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx21.bF.IR_FreqHigh = uValue;

        // read byte 0x21
        err = ddTDA182I2Write(pObj, 0x21, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIR_FreqHigh:
//
// DESCRIPTION: Get the IR_FreqHigh bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIR_FreqHigh
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x21
        err = ddTDA182I2Read(pObj, 0x21, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx21.bF.IR_FreqHigh;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetPD_Vsync_Mgt:
//
// DESCRIPTION: Set the PD_Vsync_Mgt bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetPD_Vsync_Mgt
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx22.bF.PD_Vsync_Mgt = uValue;

        // write byte 0x22
        err = ddTDA182I2Write(pObj, 0x22, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetPD_Vsync_Mgt:
//
// DESCRIPTION: Get the PD_Vsync_Mgt bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetPD_Vsync_Mgt
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x22
        err = ddTDA182I2Read(pObj, 0x22, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx22.bF.PD_Vsync_Mgt;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetPD_Ovld:
//
// DESCRIPTION: Set the PD_Ovld bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetPD_Ovld
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx22.bF.PD_Ovld = uValue;

        // write byte 0x22
        err = ddTDA182I2Write(pObj, 0x22, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetPD_Ovld:
//
// DESCRIPTION: Get the PD_Ovld bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetPD_Ovld
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x22
        err = ddTDA182I2Read(pObj, 0x22, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx22.bF.PD_Ovld;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetAGC_Ovld_Timer:
//
// DESCRIPTION: Set the AGC_Ovld_Timer bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetAGC_Ovld_Timer
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx22.bF.AGC_Ovld_Timer = uValue;

        // write byte 0x22
        err = ddTDA182I2Write(pObj, 0x22, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC_Ovld_Timer:
//
// DESCRIPTION: Get the AGC_Ovld_Timer bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC_Ovld_Timer
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x22
        err = ddTDA182I2Read(pObj, 0x22, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx22.bF.AGC_Ovld_Timer;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIR_Mixer_loop_off:
//
// DESCRIPTION: Set the IR_Mixer_loop_off bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIR_Mixer_loop_off
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx23.bF.IR_Mixer_loop_off = uValue;

        // read byte 0x23
        err = ddTDA182I2Write(pObj, 0x23, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIR_Mixer_loop_off:
//
// DESCRIPTION: Get the IR_Mixer_loop_off bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIR_Mixer_loop_off
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x23
        err = ddTDA182I2Read(pObj, 0x23, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx23.bF.IR_Mixer_loop_off;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIR_Mixer_Do_step:
//
// DESCRIPTION: Set the IR_Mixer_Do_step bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIR_Mixer_Do_step
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx23.bF.IR_Mixer_Do_step = uValue;

        // read byte 0x23
        err = ddTDA182I2Write(pObj, 0x23, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIR_Mixer_Do_step:
//
// DESCRIPTION: Get the IR_Mixer_Do_step bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetIR_Mixer_Do_step
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x23
        err = ddTDA182I2Read(pObj, 0x23, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx23.bF.IR_Mixer_Do_step;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetAGC1_loop_off:
//
// DESCRIPTION: Set the AGC1_loop_off bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetAGC1_loop_off
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx24.bF.AGC1_loop_off = uValue;

        // read byte 0x24
        err = ddTDA182I2Write(pObj, 0x24, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC1_loop_off:
//
// DESCRIPTION: Get the AGC1_loop_off bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC1_loop_off
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x24
        err = ddTDA182I2Read(pObj, 0x24, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx24.bF.AGC1_loop_off;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetAGC1_Do_step:
//
// DESCRIPTION: Set the AGC1_Do_step bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetAGC1_Do_step
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx24.bF.AGC1_Do_step = uValue;

        // read byte 0x24
        err = ddTDA182I2Write(pObj, 0x24, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC1_Do_step:
//
// DESCRIPTION: Get the AGC1_Do_step bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC1_Do_step
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x24
        err = ddTDA182I2Read(pObj, 0x24, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx24.bF.AGC1_Do_step;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetForce_AGC1_gain:
//
// DESCRIPTION: Set the Force_AGC1_gain bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetForce_AGC1_gain
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx24.bF.Force_AGC1_gain = uValue;

        // read byte 0x24
        err = ddTDA182I2Write(pObj, 0x24, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetForce_AGC1_gain:
//
// DESCRIPTION: Get the Force_AGC1_gain bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetForce_AGC1_gain
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x24
        err = ddTDA182I2Read(pObj, 0x24, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx24.bF.Force_AGC1_gain;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetAGC1_Gain:
//
// DESCRIPTION: Set the AGC1_Gain bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetAGC1_Gain
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx24.bF.AGC1_Gain = uValue;

        // read byte 0x24
        err = ddTDA182I2Write(pObj, 0x24, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC1_Gain:
//
// DESCRIPTION: Get the AGC1_Gain bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC1_Gain
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x24
        err = ddTDA182I2Read(pObj, 0x24, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx24.bF.AGC1_Gain;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq0:
//
// DESCRIPTION: Set the RFCAL_Freq0 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq0
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx26.bF.RFCAL_Freq0 = uValue;

        // read byte 0x26
        err = ddTDA182I2Write(pObj, 0x26, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq0:
//
// DESCRIPTION: Get the RFCAL_Freq0 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq0
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x26
        err = ddTDA182I2Read(pObj, 0x26, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx26.bF.RFCAL_Freq0;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq1:
//
// DESCRIPTION: Set the RFCAL_Freq1 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq1
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx26.bF.RFCAL_Freq1 = uValue;

        // read byte 0x26
        err = ddTDA182I2Write(pObj, 0x26, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq1:
//
// DESCRIPTION: Get the RFCAL_Freq1 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq1
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x26
        err = ddTDA182I2Read(pObj, 0x26, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx26.bF.RFCAL_Freq1;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq2:
//
// DESCRIPTION: Set the RFCAL_Freq2 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq2
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx27.bF.RFCAL_Freq2 = uValue;

        // read byte 0x27
        err = ddTDA182I2Write(pObj, 0x27, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq2:
//
// DESCRIPTION: Get the RFCAL_Freq2 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq2
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x27
        err = ddTDA182I2Read(pObj, 0x27, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx27.bF.RFCAL_Freq2;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq3:
//
// DESCRIPTION: Set the RFCAL_Freq3 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq3
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx27.bF.RFCAL_Freq3 = uValue;

        // read byte 0x27
        err = ddTDA182I2Write(pObj, 0x27, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq3:
//
// DESCRIPTION: Get the RFCAL_Freq3 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq3
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x27
        err = ddTDA182I2Read(pObj, 0x27, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx27.bF.RFCAL_Freq3;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq4:
//
// DESCRIPTION: Set the RFCAL_Freq4 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq4
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx28.bF.RFCAL_Freq4 = uValue;

        // read byte 0x28
        err = ddTDA182I2Write(pObj, 0x28, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq4:
//
// DESCRIPTION: Get the RFCAL_Freq4 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq4
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x28
        err = ddTDA182I2Read(pObj, 0x28, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx28.bF.RFCAL_Freq4;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq5:
//
// DESCRIPTION: Set the RFCAL_Freq5 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq5
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue   //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx28.bF.RFCAL_Freq5 = uValue;

        // read byte 0x28
        err = ddTDA182I2Write(pObj, 0x28, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq5:
//
// DESCRIPTION: Get the RFCAL_Freq5 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq5
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x28
        err = ddTDA182I2Read(pObj, 0x28, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx28.bF.RFCAL_Freq5;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq6:
//
// DESCRIPTION: Set the RFCAL_Freq6 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq6
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx29.bF.RFCAL_Freq6 = uValue;

        // read byte 0x29
        err = ddTDA182I2Write(pObj, 0x29, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq6:
//
// DESCRIPTION: Get the RFCAL_Freq6 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq6
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x29
        err = ddTDA182I2Read(pObj, 0x29, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx29.bF.RFCAL_Freq6;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq7:
//
// DESCRIPTION: Set the RFCAL_Freq7 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq7
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx29.bF.RFCAL_Freq7 = uValue;

        // read byte 0x29
        err = ddTDA182I2Write(pObj, 0x29, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq7:
//
// DESCRIPTION: Get the RFCAL_Freq7 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq7
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x29
        err = ddTDA182I2Read(pObj, 0x29, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx29.bF.RFCAL_Freq7;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq8:
//
// DESCRIPTION: Set the RFCAL_Freq8 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq8
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx2A.bF.RFCAL_Freq8 = uValue;

        // read byte 0x2A
        err = ddTDA182I2Write(pObj, 0x2A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq8:
//
// DESCRIPTION: Get the RFCAL_Freq8 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq8
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x2A
        err = ddTDA182I2Read(pObj, 0x2A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx2A.bF.RFCAL_Freq8;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq9:
//
// DESCRIPTION: Set the RFCAL_Freq9 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq9
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx2A.bF.RFCAL_Freq9 = uValue;

        // read byte 0x2A
        err = ddTDA182I2Write(pObj, 0x2A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq9:
//
// DESCRIPTION: Get the RFCAL_Freq9 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq9
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x2A
        err = ddTDA182I2Read(pObj, 0x2A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx2A.bF.RFCAL_Freq9;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq10:
//
// DESCRIPTION: Set the RFCAL_Freq10 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq10
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx2B.bF.RFCAL_Freq10 = uValue;

        // read byte 0x2B
        err = ddTDA182I2Write(pObj, 0x2B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq10:
//
// DESCRIPTION: Get the ? bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq10
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x2B
        err = ddTDA182I2Read(pObj, 0x2B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx2B.bF.RFCAL_Freq10;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Freq11:
//
// DESCRIPTION: Set the RFCAL_Freq11 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Freq11
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx2B.bF.RFCAL_Freq11 = uValue;

        // read byte 0x2B
        err = ddTDA182I2Write(pObj, 0x2B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Freq11:
//
// DESCRIPTION: Get the RFCAL_Freq11 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Freq11
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x2B
        err = ddTDA182I2Read(pObj, 0x2B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx2B.bF.RFCAL_Freq11;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset0:
//
// DESCRIPTION: Set the RFCAL_Offset0 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset0
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[0].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx26.bF.RFCAL_Offset_Cprog0 = uValue;

            // read byte 0x26
            err = ddTDA182I2Write(pObj, 0x26, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset0:
//
// DESCRIPTION: Get the RFCAL_Offset0 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset0
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[0].CprogOffset;
        }
        else
        {
            // read byte 0x26
            err = ddTDA182I2Read(pObj, 0x26, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx26.bF.RFCAL_Offset_Cprog0;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset1:
//
// DESCRIPTION: Set the RFCAL_Offset1 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset1
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[1].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx26.bF.RFCAL_Offset_Cprog1 = uValue;

            // read byte 0x26
            err = ddTDA182I2Write(pObj, 0x26, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset1:
//
// DESCRIPTION: Get the RFCAL_Offset1 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset1
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[1].CprogOffset;
        }
        else
        {
            // read byte 0x26
            err = ddTDA182I2Read(pObj, 0x26, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx26.bF.RFCAL_Offset_Cprog1;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset2:
//
// DESCRIPTION: Set the RFCAL_Offset2 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset2
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[2].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx27.bF.RFCAL_Offset_Cprog2 = uValue;

            // read byte 0x27
            err = ddTDA182I2Write(pObj, 0x27, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset2:
//
// DESCRIPTION: Get the RFCAL_Offset2 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset2
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[2].CprogOffset;
        }
        else
        {
            // read byte 0x27
            err = ddTDA182I2Read(pObj, 0x27, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx27.bF.RFCAL_Offset_Cprog2;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset3:
//
// DESCRIPTION: Set the RFCAL_Offset3 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset3

(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[3].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx27.bF.RFCAL_Offset_Cprog3 = uValue;

            // read byte 0x27
            err = ddTDA182I2Write(pObj, 0x27, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset3:
//
// DESCRIPTION: Get the RFCAL_Offset3 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset3
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[3].CprogOffset;
        }
        else
        {
            // read byte 0x27
            err = ddTDA182I2Read(pObj, 0x27, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx27.bF.RFCAL_Offset_Cprog3;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset4:
//
// DESCRIPTION: Set the RFCAL_Offset4 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset4
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[4].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx28.bF.RFCAL_Offset_Cprog4 = uValue;

            // read byte 0x28
            err = ddTDA182I2Write(pObj, 0x28, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset4:
//
// DESCRIPTION: Get the RFCAL_Offset4 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset4
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[4].CprogOffset;
        }
        else
        {
            // read byte 0x28
            err = ddTDA182I2Read(pObj, 0x28, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx28.bF.RFCAL_Offset_Cprog4;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset5:
//
// DESCRIPTION: Set the RFCAL_Offset5 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset5
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue   //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[5].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx28.bF.RFCAL_Offset_Cprog5 = uValue;

            // read byte 0x28
            err = ddTDA182I2Write(pObj, 0x28, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset5:
//
// DESCRIPTION: Get the RFCAL_Offset5 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset5
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[5].CprogOffset;
        }
        else
        {
            // read byte 0x28
            err = ddTDA182I2Read(pObj, 0x28, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx28.bF.RFCAL_Offset_Cprog5;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset6:
//
// DESCRIPTION: Set the RFCAL_Offset6 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset6
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[6].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx29.bF.RFCAL_Offset_Cprog6 = uValue;

            // read byte 0x29
            err = ddTDA182I2Write(pObj, 0x29, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset6:
//
// DESCRIPTION: Get the RFCAL_Offset6 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset6
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[6].CprogOffset;
        }
        else
        {
            // read byte 0x29
            err = ddTDA182I2Read(pObj, 0x29, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx29.bF.RFCAL_Offset_Cprog6;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset7:
//
// DESCRIPTION: Set the RFCAL_Offset7 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset7
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[7].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx29.bF.RFCAL_Offset_Cprog7 = uValue;

            // read byte 0x29
            err = ddTDA182I2Write(pObj, 0x29, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset7:
//
// DESCRIPTION: Get the RFCAL_Offset7 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset7
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[7].CprogOffset;
        }
        else
        {
            // read byte 0x29
            err = ddTDA182I2Read(pObj, 0x29, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx29.bF.RFCAL_Offset_Cprog7;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset8:
//
// DESCRIPTION: Set the RFCAL_Offset8 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset8
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[8].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx2A.bF.RFCAL_Offset_Cprog8 = uValue;

            // read byte 0x2A
            err = ddTDA182I2Write(pObj, 0x2A, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset8:
//
// DESCRIPTION: Get the RFCAL_Offset8 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset8
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[8].CprogOffset;
        }
        else
        {
            // read byte 0x2A
            err = ddTDA182I2Read(pObj, 0x2A, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx2A.bF.RFCAL_Offset_Cprog8;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset9:
//
// DESCRIPTION: Set the RFCAL_Offset9 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset9
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[9].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx2A.bF.RFCAL_Offset_Cprog9 = uValue;

            // read byte 0x2A
            err = ddTDA182I2Write(pObj, 0x2A, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset9:
//
// DESCRIPTION: Get the RFCAL_Offset9 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset9
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[9].CprogOffset;
        }
        else
        {
            // read byte 0x2A
            err = ddTDA182I2Read(pObj, 0x2A, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx2A.bF.RFCAL_Offset_Cprog9;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset10:
//
// DESCRIPTION: Set the RFCAL_Offset10 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset10
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[10].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx2B.bF.RFCAL_Offset_Cprog10 = uValue;

            // read byte 0x2B
            err = ddTDA182I2Write(pObj, 0x2B, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset10:
//
// DESCRIPTION: Get the ? bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset10
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[10].CprogOffset;
        }
        else
        {
            // read byte 0x2B
            err = ddTDA182I2Read(pObj, 0x2B, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx2B.bF.RFCAL_Offset_Cprog10;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_Offset11:
//
// DESCRIPTION: Set the RFCAL_Offset11 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_Offset11
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            pObj->RFCAL_Prog[11].CprogOffset = uValue;
        }
        else
        {
            // set value
            pObj->I2CMap.uBx2B.bF.RFCAL_Offset_Cprog11 = uValue;

            // read byte 0x2B
            err = ddTDA182I2Write(pObj, 0x2B, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_Offset11:
//
// DESCRIPTION: Get the RFCAL_Offset11 bit(s) status
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_Offset11
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        if(pObj->bRFCAL_SW_Algo_Enable)
        {
            *puValue = pObj->RFCAL_Prog[11].CprogOffset;
        }
        else
        {
            // read byte 0x2B
            err = ddTDA182I2Read(pObj, 0x2B, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

            if(err == TM_OK)
            {
                // get value
                *puValue = pObj->I2CMap.uBx2B.bF.RFCAL_Offset_Cprog11;
            }
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_SW_Algo_Enable:
//
// DESCRIPTION: Set the status of RFCAL Algo
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_SW_Algo_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->bRFCAL_SW_Algo_Enable = uValue;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_SW_Algo_Enable:
//
// DESCRIPTION: Get the status of RFCAL Algo
//
// RETURN:      TM_OK if no error
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_SW_Algo_Enable
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        *puValue = (UInt8)pObj->bRFCAL_SW_Algo_Enable;

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetAGC2_loop_off:
//
// DESCRIPTION: Set the AGC2_loop_off bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetAGC2_loop_off
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx2C.bF.AGC2_loop_off = uValue;

        // read byte 0x2C
        err = ddTDA182I2Write(pObj, 0x2C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC2_loop_off:
//
// DESCRIPTION: Get the AGC2_loop_off bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC2_loop_off
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x2C
        err = ddTDA182I2Read(pObj, 0x2C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx2C.bF.AGC2_loop_off;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetForce_AGC2_gain:
//
// DESCRIPTION: Set the Force_AGC2_gain bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetForce_AGC2_gain
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx2C.bF.Force_AGC2_gain = uValue;

        // write byte 0x2C
        err = ddTDA182I2Write(pObj, 0x2C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetForce_AGC2_gain:
//
// DESCRIPTION: Get the Force_AGC2_gain bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetForce_AGC2_gain
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x2C
        err = ddTDA182I2Read(pObj, 0x2C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx2C.bF.Force_AGC2_gain;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRF_Filter_Gv:
//
// DESCRIPTION: Set the RF_Filter_Gv bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRF_Filter_Gv
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx2C.bF.RF_Filter_Gv = uValue;

        // read byte 0x2C
        err = ddTDA182I2Write(pObj, 0x2C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRF_Filter_Gv:
//
// DESCRIPTION: Get the RF_Filter_Gv bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRF_Filter_Gv
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x2C
        err = ddTDA182I2Read(pObj, 0x2C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx2C.bF.RF_Filter_Gv;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetAGC2_Do_step:
//
// DESCRIPTION: Set the AGC2_Do_step bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetAGC2_Do_step
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx2E.bF.AGC2_Do_step = uValue;

        // read byte 0x2E
        err = ddTDA182I2Write(pObj, 0x2E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC2_Do_step:
//
// DESCRIPTION: Get the AGC2_Do_step bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC2_Do_step
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x2E
        err = ddTDA182I2Read(pObj, 0x2E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx2E.bF.AGC2_Do_step;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRF_BPF_Bypass:
//
// DESCRIPTION: Set the RF_BPF_Bypass bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRF_BPF_Bypass
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx2F.bF.RF_BPF_Bypass = uValue;

        // read byte 0x2F
        err = ddTDA182I2Write(pObj, 0x2F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRF_BPF_Bypass:
//
// DESCRIPTION: Get the RF_BPF_Bypass bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRF_BPF_Bypass
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x2F
        err = ddTDA182I2Read(pObj, 0x2F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx2F.bF.RF_BPF_Bypass;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRF_BPF:
//
// DESCRIPTION: Set the RF_BPF bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRF_BPF
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx2F.bF.RF_BPF = uValue;

        // read byte 0x2F
        err = ddTDA182I2Write(pObj, 0x2F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRF_BPF:
//
// DESCRIPTION: Get the RF_BPF bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRF_BPF
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x2F
        err = ddTDA182I2Read(pObj, 0x2F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx2F.bF.RF_BPF;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetUp_AGC5:
//
// DESCRIPTION: Get the Up_AGC5 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetUp_AGC5
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x31
        err = ddTDA182I2Read(pObj, 0x31, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx31.bF.Up_AGC5;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetDo_AGC5:
//
// DESCRIPTION: Get the Do_AGC5 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetDo_AGC5
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x31
        err = ddTDA182I2Read(pObj, 0x31, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx31.bF.Do_AGC5;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetUp_AGC4:
//
// DESCRIPTION: Get the Up_AGC4 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetUp_AGC4
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x31
        err = ddTDA182I2Read(pObj, 0x31, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx31.bF.Up_AGC4;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetDo_AGC4:
//
// DESCRIPTION: Get the Do_AGC4 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetDo_AGC4
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x31
        err = ddTDA182I2Read(pObj, 0x31, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx31.bF.Do_AGC4;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetUp_AGC2:
//
// DESCRIPTION: Get the Up_AGC2 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetUp_AGC2
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x31
        err = ddTDA182I2Read(pObj, 0x31, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx31.bF.Up_AGC2;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetDo_AGC2:
//
// DESCRIPTION: Get the Do_AGC2 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetDo_AGC2
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x31
        err = ddTDA182I2Read(pObj, 0x31, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx31.bF.Do_AGC2;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetUp_AGC1:
//
// DESCRIPTION: Get the Up_AGC1 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetUp_AGC1
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x31
        err = ddTDA182I2Read(pObj, 0x31, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx31.bF.Up_AGC1;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetDo_AGC1:
//
// DESCRIPTION: Get the Do_AGC1 bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetDo_AGC1
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x31
        err = ddTDA182I2Read(pObj, 0x31, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx31.bF.Do_AGC1;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC2_Gain_Read:
//
// DESCRIPTION: Get the AGC2_Gain_Read bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC2_Gain_Read
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x32
        err = ddTDA182I2Read(pObj, 0x32, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx32.bF.AGC2_Gain_Read;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC1_Gain_Read:
//
// DESCRIPTION: Get the AGC1_Gain_Read bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC1_Gain_Read
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x32
        err = ddTDA182I2Read(pObj, 0x32, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx32.bF.AGC1_Gain_Read;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetTOP_AGC3_Read:
//
// DESCRIPTION: Get the TOP_AGC3_Read bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetTOP_AGC3_Read
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x33
        err = ddTDA182I2Read(pObj, 0x33, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx33.bF.TOP_AGC3_Read;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}


//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC5_Gain_Read:
//
// DESCRIPTION: Get the AGC5_Gain_Read bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC5_Gain_Read
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x34
        err = ddTDA182I2Read(pObj, 0x34, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx34.bF.AGC5_Gain_Read;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetAGC4_Gain_Read:
//
// DESCRIPTION: Get the AGC4_Gain_Read bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetAGC4_Gain_Read
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x34
        err = ddTDA182I2Read(pObj, 0x34, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx34.bF.AGC4_Gain_Read;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRSSI:
//
// DESCRIPTION: Set the RSSI bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRSSI
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx35.RSSI = uValue;

        // write byte 0x35
        err = ddTDA182I2Write(pObj, 0x35, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRSSI:
//
// DESCRIPTION: Get the RSSI bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRSSI
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
       // tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x35
        err = ddTDA182I2Read(pObj, 0x35, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx35.RSSI;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRSSI_AV:
//
// DESCRIPTION: Set the RSSI_AV bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRSSI_AV
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx36.bF.RSSI_AV = uValue;

        // write byte 0x36
        err = ddTDA182I2Write(pObj, 0x36, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRSSI_AV:
//
// DESCRIPTION: Get the RSSI_AV bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRSSI_AV
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x36
        err = ddTDA182I2Read(pObj, 0x36, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx36.bF.RSSI_AV;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRSSI_Cap_Reset_En:
//
// DESCRIPTION: Set the RSSI_Cap_Reset_En bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRSSI_Cap_Reset_En
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx36.bF.RSSI_Cap_Reset_En = uValue;

        // write byte 0x36
        err = ddTDA182I2Write(pObj, 0x36, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRSSI_Cap_Reset_En:
//
// DESCRIPTION: Get the RSSI_Cap_Reset_En bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRSSI_Cap_Reset_En
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x36
        err = ddTDA182I2Read(pObj, 0x36, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx36.bF.RSSI_Cap_Reset_En;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRSSI_Cap_Val:
//
// DESCRIPTION: Set the RSSI_Cap_Val bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRSSI_Cap_Val
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx36.bF.RSSI_Cap_Val = uValue;

        // write byte 0x36
        err = ddTDA182I2Write(pObj, 0x36, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRSSI_Cap_Val:
//
// DESCRIPTION: Get the RSSI_Cap_Val bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRSSI_Cap_Val
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x36
        err = ddTDA182I2Read(pObj, 0x36, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx36.bF.RSSI_Cap_Val;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRSSI_Dicho_not:
//
// DESCRIPTION: Set the RSSI_Dicho_not bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRSSI_Dicho_not
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx36.bF.RSSI_Dicho_not = uValue;

        // write byte 0x36
        err = ddTDA182I2Write(pObj, 0x36, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRSSI_Dicho_not:
//
// DESCRIPTION: Get the RSSI_Dicho_not bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRSSI_Dicho_not
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x36
        err = ddTDA182I2Read(pObj, 0x36, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx36.bF.RSSI_Dicho_not;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetDDS_Polarity:
//
// DESCRIPTION: Set the DDS_Polarity bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetDDS_Polarity
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx37.bF.DDS_Polarity = uValue;

        // write byte 0x37
        err = ddTDA182I2Write(pObj, 0x37, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetDDS_Polarity:
//
// DESCRIPTION: Get the DDS_Polarity bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetDDS_Polarity
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x37
        err = ddTDA182I2Read(pObj, 0x37, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx37.bF.DDS_Polarity;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetRFCAL_DeltaGain:
//
// DESCRIPTION: Set the RFCAL_DeltaGain bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetRFCAL_DeltaGain
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx37.bF.RFCAL_DeltaGain = uValue;

        // read byte 0x37
        err = ddTDA182I2Write(pObj, 0x37, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetRFCAL_DeltaGain:
//
// DESCRIPTION: Get the RFCAL_DeltaGain bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2GetRFCAL_DeltaGain
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x37
        err = ddTDA182I2Read(pObj, 0x37, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx37.bF.RFCAL_DeltaGain;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2SetIRQ_Polarity:
//
// DESCRIPTION: Set the IRQ_Polarity bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2SetIRQ_Polarity
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8           uValue  //  I: Item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set value
        pObj->I2CMap.uBx37.bF.IRQ_Polarity = uValue;

        // read byte 0x37
        err = ddTDA182I2Write(pObj, 0x37, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2GetIRQ_Polarity:
//
// DESCRIPTION: Get the IRQ_Polarity bit(s) status
//
// RETURN:      tmdd_ERR_TUNER_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_PARAMETER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------

//
tmErrorCode_t
tmddTDA182I2GetIRQ_Polarity
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_PARAMETER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x37
        err = ddTDA182I2Read(pObj, 0x37, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx37.bF.IRQ_Polarity;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_0
//
// DESCRIPTION: Get the rfcal_log_0 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_0
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x38
        err = ddTDA182I2Read(pObj, 0x38, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx38.bF.rfcal_log_0;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_1
//
// DESCRIPTION: Get the rfcal_log_1 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_1
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x39
        err = ddTDA182I2Read(pObj, 0x39, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx39.bF.rfcal_log_1;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_2
//
// DESCRIPTION: Get the rfcal_log_2 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_2
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x3A
        err = ddTDA182I2Read(pObj, 0x3A, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx3A.bF.rfcal_log_2;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_3
//
// DESCRIPTION: Get the rfcal_log_3 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_3
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x3B
        err = ddTDA182I2Read(pObj, 0x3B, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx3B.bF.rfcal_log_3;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_4
//
// DESCRIPTION: Get the rfcal_log_4 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_4
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x3C
        err = ddTDA182I2Read(pObj, 0x3C, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx3C.bF.rfcal_log_4;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_5
//
// DESCRIPTION: Get the rfcal_log_5 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_5
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x3D
        err = ddTDA182I2Read(pObj, 0x3D, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx3D.bF.rfcal_log_5;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_6
//
// DESCRIPTION: Get the rfcal_log_6 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_6
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x3E
        err = ddTDA182I2Read(pObj, 0x3E, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx3E.bF.rfcal_log_6;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_7
//
// DESCRIPTION: Get the rfcal_log_7 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_7
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x3F
        err = ddTDA182I2Read(pObj, 0x3F, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx3F.bF.rfcal_log_7;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_8
//
// DESCRIPTION: Get the rfcal_log_8 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_8
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x40
        err = ddTDA182I2Read(pObj, 0x40, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx40.bF.rfcal_log_8;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_9
//
// DESCRIPTION: Get the rfcal_log_9 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_9
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x41
        err = ddTDA182I2Read(pObj, 0x41, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx41.bF.rfcal_log_9;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_10
//
// DESCRIPTION: Get the rfcal_log_10 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_10
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x42
        err = ddTDA182I2Read(pObj, 0x42, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx42.bF.rfcal_log_10;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2Getrfcal_log_11
//
// DESCRIPTION: Get the rfcal_log_11 bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2Getrfcal_log_11
(
 tmUnitSelect_t  tUnit,  //  I: Unit number
 UInt8*          puValue //  I: Address of the variable to output item value
 )
{
    ptmddTDA182I2Object_t    pObj = Null;
    tmErrorCode_t                   err  = TM_OK;

    // test the parameter
    if (puValue == Null)
        err = ddTDA182I2_ERR_BAD_UNIT_NUMBER;

    if(err == TM_OK)
    {
        /* Get Instance Object */
        err = ddTDA182I2GetInstance(tUnit, &pObj);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));
    }

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // read byte 0x43
        err = ddTDA182I2Read(pObj, 0x43, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Read(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // get value
            *puValue = pObj->I2CMap.uBx43.bF.rfcal_log_11;
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}

//-------------------------------------------------------------------------------------
// FUNCTION:    tmddTDA182I2LaunchRF_CAL
//
// DESCRIPTION: Launch the RF_CAL bit(s) status
//
// RETURN:      ddTDA182I2_ERR_BAD_UNIT_NUMBER
//        ddTDA182I2_ERR_BAD_UNIT_NUMBER
//         ddTDA182I2_ERR_NOT_INITIALIZED
//        tmdd_ERR_IIC_ERR
//         TM_OK 
//
// NOTES:       
//-------------------------------------------------------------------------------------
//
tmErrorCode_t
tmddTDA182I2LaunchRF_CAL
(
 tmUnitSelect_t  tUnit   //  I: Unit number
 )
{
    ptmddTDA182I2Object_t   pObj = Null;
    tmErrorCode_t           err  = TM_OK;

    /* Get Instance Object */
    err = ddTDA182I2GetInstance(tUnit, &pObj);
    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2GetInstance(0x%08X) failed.", tUnit));

    if(err == TM_OK)
    {
        err = ddTDA182I2MutexAcquire(pObj, ddTDA182I2_MUTEX_TIMEOUT);
    }

    if(err == TM_OK)
    {
        // set Calc_PLL & RF_CAL
        pObj->I2CMap.uBx19.MSM_byte_1 = 0x21;

        // write byte 0x19
        err = ddTDA182I2Write(pObj, 0x19, 1);
        tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

        if(err == TM_OK)
        {
            // trigger MSM_Launch
            pObj->I2CMap.uBx1A.bF.MSM_Launch = 1;

            // write byte 0x1A
            err = ddTDA182I2Write(pObj, 0x1A, 1);
            tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2Write(0x%08X) failed.", tUnit));

            // reset MSM_Launch (buffer only, no write)
            pObj->I2CMap.uBx1A.bF.MSM_Launch = 0;

            if(pObj->bIRQWait)
            {
                if(err == TM_OK)
                {
                    err = ddTDA182I2WaitIRQ(pObj, 1700, 50, 0x0C);
                    tmASSERTExT(err, TM_OK, (DEBUGLVL_ERROR, "ddTDA182I2WaitIRQ(0x%08X) failed.", tUnit));
                }
                /* PATCH ES2 in case of WAIT IRQ Timeout */
                if(err == ddTDA182I2_ERR_NOT_READY && pObj->I2CMap.uBx02.bF.Major_rev == 1 && pObj->I2CMap.uBx02.bF.Minor_rev == 0)
                {
                        pObj->bSoftReset = True;
                }
            }            
        }

        (void)ddTDA182I2MutexRelease(pObj);
    }

    return err;
}










