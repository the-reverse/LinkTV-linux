/**

@file

@brief   RTL2832 TUA9001 NIM module definition

One can manipulate RTL2832 TUA9001 NIM through RTL2832 TUA9001 NIM module.
RTL2832 TUA9001 NIM module is derived from DVB-T NIM module.

*/


#include "nim_rtl2832_tua9001.h"





/**

@brief   RTL2832 TUA9001 NIM module builder

Use BuildRtl2832Tua9001Module() to build RTL2832 TUA9001 NIM module, set all module function pointers with the
corresponding functions, and initialize module private variables.


@param [in]   ppNim                        Pointer to RTL2832 TUA9001 NIM module pointer
@param [in]   pDvbtNimModuleMemory         Pointer to an allocated DVB-T NIM module memory
@param [in]   I2cReadingByteNumMax         Maximum I2C reading byte number for basic I2C reading function
@param [in]   I2cWritingByteNumMax         Maximum I2C writing byte number for basic I2C writing function
@param [in]   I2cRead                      Basic I2C reading function pointer
@param [in]   I2cWrite                     Basic I2C writing function pointer
@param [in]   WaitMs                       Basic waiting function pointer
@param [in]   pRtl2832ExtraModuleMemory    Pointer to an allocated RTL2832 extra module memory
@param [in]   DemodDeviceAddr              RTL2832 I2C device address
@param [in]   DemodCrystalFreqHz           RTL2832 crystal frequency in Hz
@param [in]   DemodAppMode                 RTL2832 application mode for setting
@param [in]   DemodTsInterfaceMode         RTL2832 TS interface mode for setting
@param [in]   DemodUpdateFuncRefPeriodMs   RTL2832 update function reference period in millisecond for setting
@param [in]   DemodIsFunc1Enabled          RTL2832 Function 1 enabling status for setting
@param [in]   pTua9001ExtraModuleMemory    Pointer to an allocated TUA9001 extra module memory
@param [in]   TunerDeviceAddr              TUA9001 I2C device address
@param [in]   TunerAgcMode                 TUA9001 AGC mode


@note
	-# One should call BuildRtl2832Tua9001Module() to build RTL2832 TUA9001 NIM module before using it.

*/
void
BuildRtl2832Tua9001Module(
	DVBT_NIM_MODULE **ppNim,								// DVB-T NIM dependence
	DVBT_NIM_MODULE *pDvbtNimModuleMemory,

	unsigned char I2cReadingByteNumMax,						// Base interface dependence
	unsigned char I2cWritingByteNumMax,
	BASE_FP_I2C_READ I2cRead,
	BASE_FP_I2C_WRITE I2cWrite,
	BASE_FP_WAIT_MS WaitMs,

	RTL2832_EXTRA_MODULE *pRtl2832ExtraModuleMemory,		// Demod dependence
	unsigned char DemodDeviceAddr,
	unsigned long DemodCrystalFreqHz,
	int DemodAppMode,
	int DemodTsInterfaceMode,
	unsigned long DemodUpdateFuncRefPeriodMs,
	int DemodIsFunc1Enabled,

	TUA9001_EXTRA_MODULE *pTua9001ExtraModuleMemory,		// Tuner dependence
	unsigned char TunerDeviceAddr
	)
{
	DVBT_NIM_MODULE *pNim;



	// Set NIM module pointer with NIM module memory.
	*ppNim = pDvbtNimModuleMemory;
	
	// Get NIM module.
	pNim = *ppNim;

	// Set I2C bridge module pointer with I2C bridge module memory.
	pNim->pI2cBridge = &pNim->I2cBridgeModuleMemory;

	// Set NIM extra module pointer.
	pNim->pExtra = INVALID_POINTER_VALUE;


	// Set NIM type.
	pNim->NimType = DVBT_NIM_RTL2832_TUA9001;


	// Build base interface module.
	BuildBaseInterface(
		&pNim->pBaseInterface,
		&pNim->BaseInterfaceModuleMemory,
		I2cReadingByteNumMax,
		I2cWritingByteNumMax,
		I2cRead,
		I2cWrite,
		WaitMs
		);

	// Build RTL2832 demod module.
	BuildRtl2832Module(
		&pNim->pDemod,
		&pNim->DvbtDemodModuleMemory,
		pRtl2832ExtraModuleMemory,
		&pNim->BaseInterfaceModuleMemory,
		&pNim->I2cBridgeModuleMemory,
		DemodDeviceAddr,
		DemodCrystalFreqHz,
		DemodAppMode,
		DemodUpdateFuncRefPeriodMs,
		DemodIsFunc1Enabled
		);

	// Build TUA9001 tuner module.
	BuildTua9001Module(
		&pNim->pTuner,
		&pNim->TunerModuleMemory,
		pTua9001ExtraModuleMemory,
		&pNim->BaseInterfaceModuleMemory,
		&pNim->I2cBridgeModuleMemory,
		TunerDeviceAddr
		);


	// Set NIM module variables.
	pNim->DemodTsInterfaceMode = DemodTsInterfaceMode;


	// Set NIM module function pointers with default functions.
	pNim->GetNimType        = dvbt_nim_default_GetNimType;
	pNim->GetParameters     = dvbt_nim_default_GetParameters;
	pNim->IsSignalPresent   = dvbt_nim_default_IsSignalPresent;
	pNim->IsSignalLocked    = dvbt_nim_default_IsSignalLocked;
	pNim->GetSignalStrength = dvbt_nim_default_GetSignalStrength;
	pNim->GetSignalQuality  = dvbt_nim_default_GetSignalQuality;
	pNim->GetBer            = dvbt_nim_default_GetBer;
	pNim->GetSnrDb          = dvbt_nim_default_GetSnrDb;
	pNim->GetTrOffsetPpm    = dvbt_nim_default_GetTrOffsetPpm;
	pNim->GetCrOffsetHz     = dvbt_nim_default_GetCrOffsetHz;
	pNim->GetTpsInfo        = dvbt_nim_default_GetTpsInfo;
	pNim->UpdateFunction    = dvbt_nim_default_UpdateFunction;

	// Set NIM module function pointers with particular functions.
	pNim->Initialize     = rtl2832_tua9001_Initialize;
	pNim->SetParameters  = rtl2832_tua9001_SetParameters;


	return;
}





/**

@see   DVBT_NIM_FP_INITIALIZE

*/
int
rtl2832_tua9001_Initialize(
	DVBT_NIM_MODULE *pNim
	)
{
	typedef struct
	{
		int RegBitName;
		unsigned long Value;
	}
	REG_VALUE_ENTRY;


	static const REG_VALUE_ENTRY AdditionalInitRegValueTable[RTL2832_TUA9001_ADDITIONAL_INIT_REG_TABLE_LEN] =
	{
		// RegBitName,				Value
		{DVBT_DAGC_TRG_VAL,			0x39	},
		{DVBT_AGC_TARG_VAL_0,		0x0		},
		{DVBT_AGC_TARG_VAL_8_1,		0x5a	},
		{DVBT_AAGC_LOOP_GAIN,		0x16    },
		{DVBT_LOOP_GAIN2_3_0,		0x6		},
		{DVBT_LOOP_GAIN2_4,			0x1		},
		{DVBT_LOOP_GAIN3,			0x16	},
		{DVBT_VTOP1,				0x35	},
		{DVBT_VTOP2,				0x21	},
		{DVBT_VTOP3,				0x21	},
		{DVBT_KRF1,					0x0		},
		{DVBT_KRF2,					0x40	},
		{DVBT_KRF3,					0x10	},
		{DVBT_KRF4,					0x10	},
		{DVBT_IF_AGC_MIN,			0x80	},
		{DVBT_IF_AGC_MAX,			0x7f	},
		{DVBT_RF_AGC_MIN,			0x9c	},
		{DVBT_RF_AGC_MAX,			0x7f	},
		{DVBT_POLAR_RF_AGC,			0x0		},
		{DVBT_POLAR_IF_AGC,			0x0		},
		{DVBT_AD7_SETTING,			0xe9f4	},
		{DVBT_OPT_ADC_IQ,			0x1		},
		{DVBT_AD_AVI,				0x0		},
		{DVBT_AD_AVQ,				0x0		},
	};


	TUNER_MODULE *pTuner;
	DVBT_DEMOD_MODULE *pDemod;

	int i;

	int RegBitName;
	unsigned long Value;



	// Get tuner module and demod module.
	pTuner = pNim->pTuner;
	pDemod = pNim->pDemod;


	// Enable demod DVBT_IIC_REPEAT.
	if(pDemod->SetRegBitsWithPage(pDemod, DVBT_IIC_REPEAT, 0x1) != FUNCTION_SUCCESS)
		goto error_status_set_registers;

	// Initialize tuner.
	if(pTuner->Initialize(pTuner) != FUNCTION_SUCCESS)
		goto error_status_execute_function;

	// Disable demod DVBT_IIC_REPEAT.
	if(pDemod->SetRegBitsWithPage(pDemod, DVBT_IIC_REPEAT, 0x0) != FUNCTION_SUCCESS)
		goto error_status_set_registers;


	// Initialize demod.
	if(pDemod->Initialize(pDemod) != FUNCTION_SUCCESS)
		goto error_status_execute_function;

	// Set demod IF frequency with 0 Hz.
	if(pDemod->SetIfFreqHz(pDemod, IF_FREQ_0HZ) != FUNCTION_SUCCESS)
		goto error_status_execute_function;

	// Set demod spectrum mode with SPECTRUM_NORMAL.
	if(pDemod->SetSpectrumMode(pDemod, SPECTRUM_NORMAL) != FUNCTION_SUCCESS)
		goto error_status_execute_function;


	// Set demod registers.
	for(i = 0; i < RTL2832_TUA9001_ADDITIONAL_INIT_REG_TABLE_LEN; i++)
	{
		// Get register bit name and its value.
		RegBitName = AdditionalInitRegValueTable[i].RegBitName;
		Value      = AdditionalInitRegValueTable[i].Value;

		// Set demod registers
		if(pDemod->SetRegBitsWithPage(pDemod, RegBitName, Value) != FUNCTION_SUCCESS)
			goto error_status_set_registers;
	}


	// Set TS interface according to TS interface mode.
	switch(pNim->DemodTsInterfaceMode)
	{
		case TS_INTERFACE_PARALLEL:

			// Set demod TS interface with parallel mode.
			if(pDemod->SetRegBitsWithPage(pDemod, DVBT_SERIAL,   0) != FUNCTION_SUCCESS)
				goto error_status_set_registers;

			if(pDemod->SetRegBitsWithPage(pDemod, DVBT_CDIV_PH0, 9) != FUNCTION_SUCCESS)
				goto error_status_set_registers;

			if(pDemod->SetRegBitsWithPage(pDemod, DVBT_CDIV_PH1, 9) != FUNCTION_SUCCESS)
				goto error_status_set_registers;

			break;


		default:
		case TS_INTERFACE_SERIAL:

			// Set demod TS interface with serial mode.
			if(pDemod->SetRegBitsWithPage(pDemod, DVBT_SERIAL,   1) != FUNCTION_SUCCESS)
				goto error_status_set_registers;

			if(pDemod->SetRegBitsWithPage(pDemod, DVBT_CDIV_PH0, 2) != FUNCTION_SUCCESS)
				goto error_status_set_registers;

			if(pDemod->SetRegBitsWithPage(pDemod, DVBT_CDIV_PH1, 2) != FUNCTION_SUCCESS)
				goto error_status_set_registers;

			break;
	}


	return FUNCTION_SUCCESS;


error_status_execute_function:
error_status_set_registers:
	return FUNCTION_ERROR;
}





/**

@see   DVBT_NIM_FP_SET_PARAMETERS

*/
int
rtl2832_tua9001_SetParameters(
	DVBT_NIM_MODULE *pNim,
	unsigned long RfFreqHz,
	int BandwidthMode
	)
{
	TUNER_MODULE *pTuner;
	DVBT_DEMOD_MODULE *pDemod;

	TUA9001_EXTRA_MODULE *pTunerExtra;
	int TunerBandwidthMode;



	// Get tuner module and demod module.
	pTuner = pNim->pTuner;
	pDemod = pNim->pDemod;

	// Get tuner extra module.
	pTunerExtra = (TUA9001_EXTRA_MODULE *)pTuner->pExtra;


	// Enable demod DVBT_IIC_REPEAT.
	if(pDemod->SetRegBitsWithPage(pDemod, DVBT_IIC_REPEAT, 0x1) != FUNCTION_SUCCESS)
		goto error_status_set_registers;

	// Set tuner RF frequency in Hz.
	if(pTuner->SetRfFreqHz(pTuner, RfFreqHz) != FUNCTION_SUCCESS)
		goto error_status_execute_function;

	// Determine TunerBandwidthMode according to bandwidth mode.
	switch(BandwidthMode)
	{
		default:
		case DVBT_BANDWIDTH_6MHZ:		TunerBandwidthMode = TUA9001_BANDWIDTH_6MHZ;		break;
		case DVBT_BANDWIDTH_7MHZ:		TunerBandwidthMode = TUA9001_BANDWIDTH_7MHZ;		break;
		case DVBT_BANDWIDTH_8MHZ:		TunerBandwidthMode = TUA9001_BANDWIDTH_8MHZ;		break;
	}

	// Set tuner bandwidth mode with TunerBandwidthMode.
	if(pTunerExtra->SetBandwidthMode(pTuner, TunerBandwidthMode) != FUNCTION_SUCCESS)
		goto error_status_execute_function;

	// Disable demod DVBT_IIC_REPEAT.
	if(pDemod->SetRegBitsWithPage(pDemod, DVBT_IIC_REPEAT, 0x0) != FUNCTION_SUCCESS)
		goto error_status_set_registers;


	// Set demod bandwidth mode.
	if(pDemod->SetBandwidthMode(pDemod, BandwidthMode) != FUNCTION_SUCCESS)
		goto error_status_execute_function;

	// Reset demod particular registers.
	if(pDemod->ResetFunction(pDemod) != FUNCTION_SUCCESS)
		goto error_status_execute_function;


	// Reset demod by software reset.
	if(pDemod->SoftwareReset(pDemod) != FUNCTION_SUCCESS)
		goto error_status_execute_function;


	return FUNCTION_SUCCESS;


error_status_execute_function:
error_status_set_registers:
	return FUNCTION_ERROR;
}





