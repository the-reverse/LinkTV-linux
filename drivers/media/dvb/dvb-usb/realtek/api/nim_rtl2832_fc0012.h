#ifndef __NIM_RTL2832_FC0012
#define __NIM_RTL2832_FC0012

/**

@file

@brief   RTL2832 FC0012 NIM module declaration

One can manipulate RTL2832 FC0012 NIM through RTL2832 FC0012 NIM module.
RTL2832 FC0012 NIM module is derived from DVB-T NIM module.



@par Example:
@code

// The example is the same as the NIM example in dvbt_nim_base.h except the listed lines.



#include "nim_rtl2832_fc0012.h"


...



int main(void)
{
	DVBT_NIM_MODULE *pNim;
	DVBT_NIM_MODULE DvbtNimModuleMemory;
	RTL2832_EXTRA_MODULE Rtl2832ExtraModuleMemory;
	FC0012_EXTRA_MODULE Fc0012ExtraModuleMemory;

	...



	// Build RTL2832 FC0012 NIM module.
	BuildRtl2832Fc0012Module(
		&pNim,
		&DvbtNimModuleMemory,

		9,								// Maximum I2C reading byte number is 9.
		8,								// Maximum I2C writing byte number is 8.
		CustomI2cRead,					// Employ CustomI2cRead() as basic I2C reading function.
		CustomI2cWrite,					// Employ CustomI2cWrite() as basic I2C writing function.
		CustomWaitMs,					// Employ CustomWaitMs() as basic waiting function.

		&Rtl2832ExtraModuleMemory,		// Employ RTL2832 extra module for RTL2832 module.
		0x20,							// The RTL2832 I2C device address is 0x20 in 8-bit format.
		CRYSTAL_FREQ_28800000HZ,		// The RTL2832 crystal frequency is 28.8 MHz.
		RTL2832_APPLICATION_STB,		// The RTL2832 application mode is STB mode.
		TS_INTERFACE_SERIAL,			// The RTL2832 TS interface mode is serial.
		50,								// The RTL2832 update function reference period is 50 millisecond
		ON,								// The RTL2832 Function 1 enabling status is on.

		&Fc0012ExtraModuleMemory,		// Employ FC0012 extra module for FC0012 module.
		0xc6,							// The FC0012 I2C device address is 0xc6 in 8-bit format.
		CRYSTAL_FREQ_36000000HZ			// The FC0012 crystal frequency is 36.0 MHz.
		);



	// See the example for other NIM functions in dvbt_nim_base.h

	...


	return 0;
}


@endcode

*/


#include "demod_rtl2832.h"
#include "tuner_fc0012.h"
#include "dvbt_nim_base.h"





// Definitions
#define RTL2832_FC0012_ADDITIONAL_INIT_REG_TABLE_LEN		29
#define RTL2832_FC0012_LNA_UPDATE_WAIT_TIME_MS				1000





// RTL2832 MT2266 extra module
typedef struct RTL2832_FC0012_EXTRA_MODULE_TAG RTL2832_FC0012_EXTRA_MODULE;
struct RTL2832_FC0012_EXTRA_MODULE_TAG
{
	// Extra variables
	unsigned long LnaUpdateWaitTimeMax;
	unsigned long LnaUpdateWaitTime;
	unsigned long RssiRCalOn;
};





// Builder
void
BuildRtl2832Fc0012Module(
	DVBT_NIM_MODULE **ppNim,									// DVB-T NIM dependence
	DVBT_NIM_MODULE *pDvbtNimModuleMemory,
	RTL2832_FC0012_EXTRA_MODULE *pRtl2832Fc0012ExtraModuleMemory,

	unsigned char I2cReadingByteNumMax,							// Base interface dependence
	unsigned char I2cWritingByteNumMax,
	BASE_FP_I2C_READ I2cRead,
	BASE_FP_I2C_WRITE I2cWrite,
	BASE_FP_WAIT_MS WaitMs,

	RTL2832_EXTRA_MODULE *pRtl2832ExtraModuleMemory,			// Demod dependence
	unsigned char DemodDeviceAddr,
	unsigned long DemodCrystalFreqHz,
	int DemodAppMode,
	int DemodTsInterfaceMode,
	unsigned long DemodUpdateFuncRefPeriodMs,
	int DemodIsFunc1Enabled,

	FC0012_EXTRA_MODULE *pFc0012ExtraModuleMemory,				// Tuner dependence
	unsigned char TunerDeviceAddr,
	unsigned long TunerCrystalFreqHz
	);





// RTL2832 FC0012 NIM manipulaing functions
int
rtl2832_fc0012_Initialize(
	DVBT_NIM_MODULE *pNim
	);

int
rtl2832_fc0012_SetParameters(
	DVBT_NIM_MODULE *pNim,
	unsigned long RfFreqHz,
	int BandwidthMode
	);

int
rtl2832_fc0012_UpdateFunction(
	DVBT_NIM_MODULE *pNim
	);

int
rtl2832_fc0012_GetTunerRssiCalOn(
	DVBT_NIM_MODULE *pNim
	);

int
rtl2832_fc0012_UpdateTunerLnaGainWithRssi(
	DVBT_NIM_MODULE *pNim
	);







#endif


