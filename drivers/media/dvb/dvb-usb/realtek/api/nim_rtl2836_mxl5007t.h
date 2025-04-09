#ifndef __NIM_RTL2836_MXL5007T
#define __NIM_RTL2836_MXL5007T

/**

@file

@brief   RTL2836 MxL5007T NIM module declaration

One can manipulate RTL2836 MxL5007T NIM through RTL2836 MxL5007T NIM module.
RTL2836 MxL5007T NIM module is derived from DTMB NIM module.



@par Example:
@code

// The example is the same as the NIM example in dtmb_nim_base.h except the listed lines.



#include "nim_rtl2836_mxl5007t.h"


...



int main(void)
{
	DTMB_NIM_MODULE *pNim;
	DTMB_NIM_MODULE DtmbNimModuleMemory;
	RTL2836_EXTRA_MODULE Rtl2836ExtraModuleMemory;
	MXL5007T_EXTRA_MODULE Mxl5007tExtraModuleMemory;

	...



	// Build RTL2836 MxL5007T NIM module.
	BuildRtl2836Mxl5007tModule(
		&pNim,
		&DtmbNimModuleMemory,

		9,								// Maximum I2C reading byte number is 9.
		8,								// Maximum I2C writing byte number is 8.
		CustomI2cRead,					// Employ CustomI2cRead() as basic I2C reading function.
		CustomI2cWrite,					// Employ CustomI2cWrite() as basic I2C writing function.
		CustomWaitMs,					// Employ CustomWaitMs() as basic waiting function.

		&Rtl2836ExtraModuleMemory,		// Employ RTL2836 extra module for RTL2836 module.
		0x3e,							// The RTL2836 I2C device address is 0x3e in 8-bit format.
		CRYSTAL_FREQ_27000000HZ,		// The RTL2836 crystal frequency is 27.0 MHz.
		TS_INTERFACE_SERIAL,			// The RTL2836 TS interface mode is serial.
		50,								// The RTL2836 update function reference period is 50 millisecond
		ON,								// The RTL2836 Function 1 enabling status is on.

		&Mxl5007tExtraModuleMemory,		// Employ MxL5007T extra module for MxL5007T module.
		0xc0							// The MxL5007T I2C device address is 0xc0 in 8-bit format.
		CRYSTAL_FREQ_16000000HZ,		// The MxL5007T Crystal frequency is 16.0 MHz.
		MXL5007T_LOOP_THROUGH_DISABLE,	// The MxL5007T loop-through mode is disabled.
		MXL5007T_CLK_OUT_DISABLE,		// The MxL5007T clock output mode is disabled.
		MXL5007T_CLK_OUT_AMP_0			// The MxL5007T clock output amplitude is 0.
		);



	// See the example for other NIM functions in dtmb_nim_base.h

	...


	return 0;
}


@endcode

*/


#include "demod_rtl2836.h"
#include "tuner_mxl5007t.h"
#include "dtmb_nim_base.h"





// Definitions
#define RTL2836_MXL5007T_ADDITIONAL_INIT_REG_TABLE_LEN		1

// Default
#define RTL2836_MXL5007T_STANDARD_MODE_DEFAULT				MXL5007T_STANDARD_DVBT
#define RTL2836_MXL5007T_IF_FREQ_HZ_DEFAULT					IF_FREQ_4570000HZ
#define RTL2836_MXL5007T_SPECTRUM_MODE_DEFAULT				SPECTRUM_NORMAL
#define RTL2836_MXL5007T_QAM_IF_DIFF_OUT_LEVEL_DEFAULT		0





// Builder
void
BuildRtl2836Mxl5007tModule(
	DTMB_NIM_MODULE **ppNim,								// DTMB NIM dependence
	DTMB_NIM_MODULE *pDtmbNimModuleMemory,

	unsigned char I2cReadingByteNumMax,						// Base interface dependence
	unsigned char I2cWritingByteNumMax,
	BASE_FP_I2C_READ I2cRead,
	BASE_FP_I2C_WRITE I2cWrite,
	BASE_FP_WAIT_MS WaitMs,

	RTL2836_EXTRA_MODULE *pRtl2836ExtraModuleMemory,		// Demod dependence
	unsigned char DemodDeviceAddr,
	unsigned long DemodCrystalFreqHz,
	int DemodTsInterfaceMode,
	unsigned long DemodUpdateFuncRefPeriodMs,
	int DemodIsFunc1Enabled,

	MXL5007T_EXTRA_MODULE *pMxl5007tExtraModuleMemory,		// Tuner dependence
	unsigned char TunerDeviceAddr,
	unsigned long TunerCrystalFreqHz,
	int TunerLoopThroughMode,
	int TunerClkOutMode,
	int TunerClkOutAmpMode
	);





// RTL2836 MxL5007T NIM manipulaing functions
int
rtl2836_mxl5007t_Initialize(
	DTMB_NIM_MODULE *pNim
	);

int
rtl2836_mxl5007t_SetParameters(
	DTMB_NIM_MODULE *pNim,
	unsigned long RfFreqHz
	);







#endif


