#ifndef	__RTW_RF_H_ 
#define __RTW_RF_H_

#include <drv_conf.h>
#include <rtw_cmd.h>

#define OFDM_PHY		1
#define MIXED_PHY		2
#define CCK_PHY		3

#define NumRates	(13)

// slot time for 11g
#define SHORT_SLOT_TIME					9
#define NON_SHORT_SLOT_TIME				20

#define RTL8711_RF_MAX_SENS 6
#define RTL8711_RF_DEF_SENS 4


#define NUM_CHANNELS	32
//#define NUM_REGULATORYS	21
#define NUM_REGULATORYS	1

//Country codes
#define USA							0x555320
#define EUROPE						0x1 //temp, should be provided later	
#define JAPAN						0x2 //temp, should be provided later	

struct	regulatory_class {
	u32	starting_freq;					//MHz, 
	u8	channel_set[NUM_CHANNELS];
	u8	channel_cck_power[NUM_CHANNELS];//dbm
	u8	channel_ofdm_power[NUM_CHANNELS];//dbm
	u8	txpower_limit;  				//dbm
	u8	channel_spacing;				//MHz
	u8	modem;
};

typedef enum _CAPABILITY{
	cESS			= 0x0001,
	cIBSS			= 0x0002,
	cPollable		= 0x0004,
	cPollReq			= 0x0008,
	cPrivacy		= 0x0010,
	cShortPreamble	= 0x0020,
	cPBCC			= 0x0040,
	cChannelAgility	= 0x0080,
	cSpectrumMgnt	= 0x0100,
	cQos			= 0x0200,	// For HCCA, use with CF-Pollable and CF-PollReq
	cShortSlotTime	= 0x0400,
	cAPSD			= 0x0800,
	cRM				= 0x1000,	// RRM (Radio Request Measurement)
	cDSSS_OFDM	= 0x2000,
	cDelayedBA		= 0x4000,
	cImmediateBA	= 0x8000,
}CAPABILITY, *PCAPABILITY;

enum	_REG_PREAMBLE_MODE{
	PREAMBLE_LONG	= 1,
	PREAMBLE_AUTO	= 2,
	PREAMBLE_SHORT	= 3,
};


enum _RTL8712_RF_MIMO_CONFIG_{
 RTL8712_RFCONFIG_1T=0x10,
 RTL8712_RFCONFIG_2T=0x20,
 RTL8712_RFCONFIG_1R=0x01,
 RTL8712_RFCONFIG_2R=0x02,
 RTL8712_RFCONFIG_1T1R=0x11,
 RTL8712_RFCONFIG_1T2R=0x12,
 RTL8712_RFCONFIG_TURBO=0x92,
 RTL8712_RFCONFIG_2T2R=0x22
};


// Bandwidth Offset
#define HAL_PRIME_CHNL_OFFSET_DONT_CARE	0
#define HAL_PRIME_CHNL_OFFSET_LOWER	1
#define HAL_PRIME_CHNL_OFFSET_UPPER	2

// Represent Channel Width in HT Capabilities
//
typedef enum _HT_CHANNEL_WIDTH {
	HT_CHANNEL_WIDTH_20 = 0,
	HT_CHANNEL_WIDTH_40 = 1,
}HT_CHANNEL_WIDTH, *PHT_CHANNEL_WIDTH;

//
// Represent Extention Channel Offset in HT Capabilities
// This is available only in 40Mhz mode.
//
typedef enum _HT_EXTCHNL_OFFSET{
	HT_EXTCHNL_OFFSET_NO_EXT = 0,
	HT_EXTCHNL_OFFSET_UPPER = 1,
	HT_EXTCHNL_OFFSET_NO_DEF = 2,
	HT_EXTCHNL_OFFSET_LOWER = 3,
}HT_EXTCHNL_OFFSET, *PHT_EXTCHNL_OFFSET;

/* 2007/11/15 MH Define different RF type. */
typedef	enum _RT_RF_TYPE_DEFINITION
{
	RF_1T2R = 0,
	RF_2T4R = 1,
	RF_2T2R = 2,
	RF_1T1R = 3,
	RF_2T2R_GREEN = 4,
	RF_819X_MAX_TYPE = 5,
}RT_RF_TYPE_DEF_E;


u32 ch2freq(u32 ch);
u32 freq2ch(u32 freq);


#endif //_RTL8711_RF_H_

