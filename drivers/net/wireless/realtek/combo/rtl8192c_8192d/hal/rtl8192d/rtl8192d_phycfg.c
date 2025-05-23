/******************************************************************************

     (c) Copyright 2008, RealTEK Technologies Inc. All Rights Reserved.

 Module:	rtl8192d_phycfg.c	

 Note:		Merge 92DE/SDU PHY config as below
			1. BB register R/W API
 			2. RF register R/W API
 			3. Initial BB/RF/MAC config by reading BB/MAC/RF txt.
 			3. Power setting API
 			4. Channel switch API
 			5. Initial gain switch API.
 			6. Other BB/MAC/RF API.
 			
 Function:	PHY: Extern function, phy: local function
 		 
 Export:	PHY_FunctionName

 Abbrev:	NONE

 History:
	Data		Who		Remark	
	08/08/2008  MHC    	1. Port from 9x series phycfg.c
						2. Reorganize code arch and ad description.
						3. Collect similar function.
						4. Seperate extern/local API.
	08/12/2008	MHC		We must merge or move USB PHY relative function later.
	10/07/2008	MHC		Add IQ calibration for PHY.(Only 1T2R mode now!!!)
	11/06/2008	MHC		Add TX Power index PG file to config in 0xExx register
						area to map with EEPROM/EFUSE tx pwr index.
	
******************************************************************************/
#define _HAL_8192D_PHYCFG_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_byteorder.h>

#include <hal_init.h>
#include <rtl8192d_hal.h>

/*---------------------------Define Local Constant---------------------------*/
/* Channel switch:The size of command tables for switch channel*/
#define MAX_PRECMD_CNT 16
#define MAX_RFDEPENDCMD_CNT 16
#define MAX_POSTCMD_CNT 16

#define MAX_DOZE_WAITING_TIMES_9x 64

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
extern ULONG    GlobalCounterForMutex;
#endif

#define MAX_RF_IMR_INDEX 12
#define MAX_RF_IMR_INDEX_NORMAL 13
#define RF_REG_NUM_for_C_CUT_5G 	6
#define RF_REG_NUM_for_C_CUT_2G 	5
#define RF_CHNL_NUM_5G			19	
#define RF_CHNL_NUM_5G_40M		17
//#define TARGET_CHNL_NUM_5G	221
//#define TARGET_CHNL_NUM_2G	14
//#define CV_CURVE_CNT			64

static u32     RF_REG_FOR_5G_SWCHNL[MAX_RF_IMR_INDEX]={0,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x38,0x39,0x0};
static u32     RF_REG_FOR_5G_SWCHNL_NORMAL[MAX_RF_IMR_INDEX_NORMAL]={0,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x0};

static u8	RF_REG_for_C_CUT_5G[RF_REG_NUM_for_C_CUT_5G] = 
			{RF_SYN_G1,	RF_SYN_G2,	RF_SYN_G3,	RF_SYN_G4,	RF_SYN_G5,	RF_SYN_G6};
static u8	RF_REG_for_C_CUT_2G[RF_REG_NUM_for_C_CUT_2G] = 
			{RF_SYN_G1, RF_SYN_G2,	RF_SYN_G3,	RF_SYN_G7,	RF_SYN_G8};

#if 0
static u32	RF_REG_MASK_for_C_CUT_5G[RF_REG_NUM_for_C_CUT_5G] = 
			{BIT18|BIT17|BIT14|BIT0,		BIT13|BIT12|BIT10|BIT9|BIT6|BIT0,	
			BIT19|BIT4|BIT1,				BIT10|BIT9|BIT8|BIT6|BIT5|BIT4,	
			BIT17|BIT16|BIT13|BIT10|BIT6,	BIT17};
static u32	RF_REG_MASK_for_C_CUT_2G[RF_REG_NUM_for_C_CUT_2G] = 
			{BIT19|BIT18|BIT17|BIT14|BIT1, 	BIT10|BIT9,	
			BIT18|BIT17|BIT16|BIT1,		BIT2|BIT1,	
			BIT15|BIT14|BIT13|BIT12|BIT11};
#endif
static u8	RF_CHNL_5G[RF_CHNL_NUM_5G] = 
			{36,40,44,48,52,56,60,64,100,104,108,112,116,120,124,128,132,136,140};
static u8	RF_CHNL_5G_40M[RF_CHNL_NUM_5G_40M] = 
			{38,42,46,50,54,58,62,102,106,110,114,118,122,126,130,134,138};

static u32	RF_REG_Param_for_C_CUT_5G[5][RF_REG_NUM_for_C_CUT_5G] = {
			{0xE43BE,	0xFC638,	0x77C0A,	0xDE471,	0xd7110,	0x8cb04},
			{0xE43BE,	0xFC078,	0xF7C1A,	0xE0C71,	0xD7550,	0xACB04},	
			{0xE43BF,	0xFF038,	0xF7C0A,	0xDE471,	0xE5550,	0xACB04},
			{0xE43BF,	0xFF079,	0xF7C1A,	0xDE471,	0xE5550,	0xACB04},
			{0xE43BF,	0xFF038,	0xF7C1A,	0xDE471,	0xd7550,	0xACB04}};

static u32	RF_REG_Param_for_C_CUT_2G[3][RF_REG_NUM_for_C_CUT_2G] = {
			{0x643BC,	0xFC038,	0x77C1A,	0x41289,	0x01840},
			{0x643BC,	0xFC038,	0x07C1A,	0x41289,	0x01840},
			{0x243BC,	0xFC438,	0x07C1A,	0x4128B,	0x0FC41}};
#if 0		//shift bit mask 
static u32	RF_REG_Param_for_C_CUT_5G[5][RF_REG_NUM_for_C_CUT_5G] = {
			{0xE43BE,	0xFC638,	0x3BE05,	0x0DE47,	0x035C4,		0x00},
			{0xE43BE,	0xFC078,	0x7BE0D,	0x0E0C7,	0x035D5,		0x01},
			{0xE43BF,	0xFF038,	0x7BE05,	0x0DE47,	0x03955,		0x01},
			{0xE43BF,	0xFF079,	0x7BE0D,	0x0DE47,	0x03955,		0x01},
			{0xE43BF,	0xFF038,	0x7BE0D,	0x0DE47,	0x035D5,		0x01}};

static u32	RF_REG_Param_for_C_CUT_2G[3][RF_REG_NUM_for_C_CUT_2G] = {
			{0x321DE,	0x007E0,	0x3BE0D,	0x20944,	0x03},
			{0x321DE,	0x007E0,	0x03E0D,	0x20944,	0x03},
			{0x121DE,	0x007E2,	0x03E0D,	0x20945,	0x1F}};			
#endif

//[mode][patha+b][reg]
static u32 RF_IMR_Param[2][8][MAX_RF_IMR_INDEX]={{
	{0x72c15,0x96578,0x8893f,0xdd6f8,0x88ff0,0x11481,0xffaa0,0x006f8,0xaa47a,0x7efaa,0xfff9c,0x32c15}, //Dual band patha 36-64
	{0x72c15,0x965f8,0xcc93f,0xbb6c7,0xccff0,0x77481,0xffaa0,0x666c7,0x55fee,0x7efaa,0xff000,0x32c15}, //100-114
	{0x72c15,0x965f8,0xcc93f,0xbb6c7,0xccff0,0x66488,0xddbd0,0x666c7,0x55fee,0x7efaa,0xe0000,0x32c15}, //116-140
	{0x72c15,0x965f8,0xff923,0xbb6c7,0xffff0,0xff480,0xffaa0,0xcc6c7,0x55fef,0x7efaa,0xe0000,0x32c15}, //149-165
	{0x72c15,0x967f8,0x8893f,0xdd6ff,0x88ff0,0x11481,0xffaa0,0x006c7,0xaa44e,0x7fffe,0xfffe0,0x32c15}, //dual band path a mac1 36-64
	{0x72c15,0x167f8,0xcc906,0xbb6c1,0xccff0,0x77481,0xffaa0,0x666c7,0x5511b,0x7effe,0xfc000,0x32c15}, //100-114
	{0x72c15,0x167f8,0xcc906,0xbb6c1,0xccff0,0x77481,0xffaa0,0x666c7,0x5511b,0x7effe,0xfc000,0x32c15}, // 116-140 the same with 100
	{0x72c15,0x96770,0xff907,0xbb6ff,0xffff0,0xff480,0xffac0,0xcc6c7,0x55ff0,0x7fffc,0xe387c,0x32c15}, // 149-165
},{
	{0x72c15,0x96578,0x8893f,0xdd6f8,0x88ff0,0x11481,0xffaa0,0x006f8,0xaa47a,0xfaa,0xfff9c,0x32c15}, //Single phy path A
	{0x72c15,0x965f8,0xcc93f,0xbb6c7,0xccff0,0x77481,0xffaa0,0x666c7,0x55fee,0xfaa,0xff000,0x32c15},
	{0x72c15,0x965f8,0xcc93f,0xbb6c7,0xccff0,0x66488,0xddbd0,0x666c7,0x55fee,0xfaa,0xe0000,0x32c15},
	{0x72c15,0x965f8,0xff923,0xbb6c7,0xffff0,0xff480,0xffaa0,0xcc6c7,0x55fef,0xfaa,0xe0000,0x32c15}, 
	{0x72c15,0x96560,0x8893f,0xdd6f8,0x88ff0,0x11481,0xfeaa0,0x006f8,0xaa47e,0xf02,0xfc020,0x32c15}, //SY,path b
	{0x72c15,0x965f8,0xcc938,0xbb6ff,0xccff0,0x77481,0xffaa0,0x666f9,0x55cff,0xf02,0xfc000,0x32c15},
	{0x72c15,0x965f8,0xcc938,0xbb6ff,0xccff0,0x77481,0xffaa0,0x666f9,0x55cff,0xf02,0xfc000,0x32c15}, //the same.
	{0x72c15,0x96678,0xff938,0xbb6c7,0xffff0,0xff480,0xffaa0,0xcc6e7,0x5589a,0xf02,0xe0000,0x32c15},
}
};

//[mode][patha+b][reg]
static u32 RF_IMR_Param_Normal[1][3][MAX_RF_IMR_INDEX_NORMAL]={{
	{0x70000,0x00ff0,0x4400f,0x00ff0,0x0,0x0,0x0,0x0,0x0,0x64888,0xe266c,0x00090,0x22fff},// channel 1-14.
	{0x70000,0x22880,0x4470f,0x55880,0x00070, 0x88000, 0x0,0x88080,0x70000,0x64a82,0xe466c,0x00090,0x32c9a}, //path 36-64
	{0x70000,0x44880,0x4477f,0x77880,0x00070, 0x88000, 0x0,0x880b0,0x0,0x64b82,0xe466c,0x00090,0x32c9a} //100 -165
}
};

#if 0
u32 CurveIndex_5G[TARGET_CHNL_NUM_5G]={0};
u32 CurveIndex_2G[TARGET_CHNL_NUM_2G]={0};

static u32 TargetChnl_5G[TARGET_CHNL_NUM_5G] = {
25141,	25116,	25091,	25066,	25041,
25016,	24991,	24966,	24941,	24917,
24892,	24867,	24843,	24818,	24794,
24770,	24765,	24721,	24697,	24672,
24648,	24624,	24600,	24576,	24552,
24528,	24504,	24480,	24457,	24433,
24409,	24385,	24362,	24338,	24315,
24291,	24268,	24245,	24221,	24198,
24175,	24151,	24128,	24105,	24082,
24059,	24036,	24013,	23990,	23967,
23945,	23922,	23899,	23876,	23854,
23831,	23809,	23786,	23764,	23741,
23719,	23697,	23674,	23652,	23630,
23608,	23586,	23564,	23541,	23519,
23498,	23476,	23454,	23432,	23410,
23388,	23367,	23345,	23323,	23302,
23280,	23259,	23237,	23216,	23194,
23173,	23152,	23130,	23109,	23088,
23067,	23046,	23025,	23003,	22982,
22962,	22941,	22920,	22899,	22878,
22857,	22837,	22816,	22795,	22775,
22754,	22733,	22713,	22692,	22672,
22652,	22631,	22611,	22591,	22570,
22550,	22530,	22510,	22490,	22469,
22449,	22429,	22409,	22390,	22370,
22350,	22336,	22310,	22290,	22271,
22251,	22231,	22212,	22192,	22173,
22153,	22134,	22114,	22095,	22075,
22056,	22037,	22017,	21998,	21979,
21960,	21941,	21921,	21902,	21883,
21864,	21845,	21826,	21807,	21789,
21770,	21751,	21732,	21713,	21695,
21676,	21657,	21639,	21620,	21602,
21583,	21565,	21546,	21528,	21509,
21491,	21473,	21454,	21436,	21418,
21400,	21381,	21363,	21345,	21327,
21309,	21291,	21273,	21255,	21237,
21219,	21201,	21183,	21166,	21148,
21130,	21112,	21095,	21077,	21059,
21042,	21024,	21007,	20989,	20972,
25679,	25653,	25627,	25601,	25575,
25549,	25523,	25497,	25471,	25446,
25420,	25394,	25369,	25343,	25318,
25292,	25267,	25242,	25216,	25191,
25166	};

static u32 TargetChnl_2G[TARGET_CHNL_NUM_2G] = {	// channel 1~14
26084, 26030, 25976, 25923, 25869, 25816, 25764,
25711, 25658, 25606, 25554, 25502, 25451, 25328
};
#endif

/*---------------------------Define Local Constant---------------------------*/


/*------------------------Define global variable-----------------------------*/

/*------------------------Define local variable------------------------------*/


/*--------------------Define export function prototype-----------------------*/
// Please refer to header file
/*--------------------Define export function prototype-----------------------*/
				
/*----------------------------Function Body----------------------------------*/

static u8 GetRightChnlPlace(u8 chnl)
{
	u8	channel_5G[59] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,136,138,140,149,151,153,154,157,159,161,163,165};
	u8	place = chnl;

	if(chnl > 14)
	{
		for(place = 14; place<sizeof(channel_5G); place++)
		{
			if(channel_5G[place] == chnl)
			{
				place++;
				break;
			}
		}
	}

	return place;
}


static u8 GetRightChnlPlaceforIQK(u8 chnl)
{
	u8	channel_all[59] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,136,138,140,149,151,153,155,157,159,161,163,165};
	u8	place = chnl;

	
	if(chnl > 14)
	{
		for(place = 14; place<sizeof(channel_all); place++)
		{
			if(channel_all[place] == chnl)
			{
				return place-13;
			}
		}
	}
	
	return 0;
}


//"chnl" begins from 0. It's not a real channel.
//"channel_info[chnl]" is a real channel.
u8 rtl8192d_getChnlGroupfromArray(u8 chnl)
{
	u8	group=0;
	u8	channel_info[59] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,100,102,104,106,108,110,112,114,116,118,120,122,124,126,128,130,132,134,136,138,140,149,151,153,154,157,159,161,163,165};

	if (channel_info[chnl] < 3)			// Chanel 1-3
		group = 0;
	else if (channel_info[chnl] < 9)		// Channel 4-9
		group = 1;
	else	if(channel_info[chnl] <14)				// Channel 10-14
		group = 2;
#if (TX_POWER_FOR_5G_BAND == 1)
	else if(channel_info[chnl] <= 42)
		group = 3;
	else if(channel_info[chnl] <= 54)
		group = 4;
	else if(channel_info[chnl] <= 64)
		group = 5;
	else if(channel_info[chnl] <= 112)
		group = 6;
	else if(channel_info[chnl] <= 126)
		group = 7;
	else if(channel_info[chnl] <= 140)
		group = 8;
	else if(channel_info[chnl] <= 154)
		group = 9;
	else if(channel_info[chnl] <= 160)
		group = 10;
	else 
		group = 11;
#endif
	
	return group;
}


//
// 1. BB register R/W API
//
/**
* Function:	phy_CalculateBitShift
*
* OverView:	Get shifted position of the BitMask
*
* Input:
*			u4Byte		BitMask,	
*
* Output:	none
* Return:		u4Byte		Return the shift bit bit position of the mask
*/
static	u32
phy_CalculateBitShift(
	u32 BitMask
	)
{
	u32 i;

	for(i=0; i<=31; i++)
	{
		if ( ((BitMask>>i) &  0x1 ) == 1)
			break;
	}

	return (i);
}

/**
* Function:	PHY_QueryBBReg
*
* OverView:	Read "sepcific bits" from BB register
*
* Input:
*			PADAPTER		Adapter,
*			u4Byte			RegAddr,		//The target address to be readback
*			u4Byte			BitMask		//The target bit position in the target address
*										//to be readback	
* Output:	None
* Return:		u4Byte			Data			//The readback register value
* Note:		This function is equal to "GetRegSetting" in PHY programming guide
*/
u32
rtl8192d_PHY_QueryBBReg(
	IN	PADAPTER	Adapter,
	IN	u32		RegAddr,
	IN	u32		BitMask
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);	
  	u32	ReturnValue = 0, OriginalValue, BitShift;
	u8	DBIdirect = 0;

#if (DISABLE_BB_RF == 1)
	return 0;
#endif

	//RT_TRACE(COMP_RF, DBG_TRACE, ("--->PHY_QueryBBReg(): RegAddr(%#lx), BitMask(%#lx)\n", RegAddr, BitMask));

#if (DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE)
	if (pHalData->bDuringMac1InitRadioA || pHalData->bDuringMac0InitRadioB)
	{
		if(pHalData->bDuringMac1InitRadioA)//MAC1 use PHY0 read radio_B.
			DBIdirect = BIT3;
		else if(pHalData->bDuringMac0InitRadioB)//MAC0 use PHY1 read radio_B.
			DBIdirect = BIT3|BIT2;
		OriginalValue = MpReadPCIDwordDBI8192C(Adapter, (u16)RegAddr, DBIdirect);  
	}
	else
#endif
	{
		OriginalValue = read32(Adapter, RegAddr);
	}
	BitShift = phy_CalculateBitShift(BitMask);
	ReturnValue = (OriginalValue & BitMask) >> BitShift;

	//RTPRINT(FPHY, PHY_BBR, ("BBR MASK=0x%lx Addr[0x%lx]=0x%lx\n", BitMask, RegAddr, OriginalValue));
	//RT_TRACE(COMP_RF, DBG_TRACE, ("<---PHY_QueryBBReg(): RegAddr(%#lx), BitMask(%#lx), OriginalValue(%#lx)\n", RegAddr, BitMask, OriginalValue));

	return (ReturnValue);

}


/**
* Function:	PHY_SetBBReg
*
* OverView:	Write "Specific bits" to BB register (page 8~) 
*
* Input:
*			PADAPTER		Adapter,
*			u4Byte			RegAddr,		//The target address to be modified
*			u4Byte			BitMask		//The target bit position in the target address
*										//to be modified	
*			u4Byte			Data			//The new register value in the target bit position
*										//of the target address			
*
* Output:	None
* Return:		None
* Note:		This function is equal to "PutRegSetting" in PHY programming guide
*/

VOID
rtl8192d_PHY_SetBBReg(
	IN	PADAPTER	Adapter,
	IN	u32		RegAddr,
	IN	u32		BitMask,
	IN	u32		Data
	)
{
	HAL_DATA_TYPE	*pHalData		= GET_HAL_DATA(Adapter);
	u8	DBIdirect=0;
	u32	OriginalValue, BitShift;

#if (DISABLE_BB_RF == 1)
	return;
#endif

	if(pHalData->bDuringMac1InitRadioA) //MAC1 use PHY0 wirte radio_A.
		DBIdirect = BIT3;
	else if(pHalData->bDuringMac0InitRadioB) //MAC0 use PHY1 wirte radio_B.
		DBIdirect = BIT3|BIT2;

	//RT_TRACE(COMP_RF, DBG_TRACE, ("--->PHY_SetBBReg(): RegAddr(%#lx), BitMask(%#lx), Data(%#lx)\n", RegAddr, BitMask, Data));

	if(BitMask!= bMaskDWord)
	{//if not "double word" write
#if (DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE)	
		if (pHalData->bDuringMac1InitRadioA || pHalData->bDuringMac0InitRadioB)
		{
			OriginalValue = MpReadPCIDwordDBI8192C(Adapter, (u16)RegAddr, DBIdirect);  
		}
		else
#endif
		{
			OriginalValue = read32(Adapter, RegAddr);
		}
		BitShift = phy_CalculateBitShift(BitMask);
		Data = (((OriginalValue) & (~BitMask)) | (Data << BitShift));
	}

#if (DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE)
	if (pHalData->bDuringMac1InitRadioA || pHalData->bDuringMac0InitRadioB)
	{
		MpWritePCIDwordDBI8192C(Adapter, 
        				(u16)RegAddr, 
        				Data,
        				DBIdirect);
	}
	else
#endif
	{
		write32(Adapter, RegAddr, Data);
	}

	//RTPRINT(FPHY, PHY_BBW, ("BBW MASK=0x%lx Addr[0x%lx]=0x%lx\n", BitMask, RegAddr, Data));
	//RT_TRACE(COMP_RF, DBG_TRACE, ("<---PHY_SetBBReg(): RegAddr(%#lx), BitMask(%#lx), Data(%#lx)\n", RegAddr, BitMask, Data));
	
}


//
// 2. RF register R/W API
//
/*-----------------------------------------------------------------------------
 * Function:	phy_FwRFSerialRead()
 *
 * Overview:	We support firmware to execute RF-R/W.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/21/2008	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
static	u32
phy_FwRFSerialRead(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				Offset	)
{
	u32		retValue = 0;		
	//RT_ASSERT(FALSE,("deprecate!\n"));
	return	(retValue);

}	/* phy_FwRFSerialRead */


/*-----------------------------------------------------------------------------
 * Function:	phy_FwRFSerialWrite()
 *
 * Overview:	We support firmware to execute RF-R/W.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	01/21/2008	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
static	VOID
phy_FwRFSerialWrite(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				Offset,
	IN	u32				Data	)
{
	//RT_ASSERT(FALSE,("deprecate!\n"));
}


/**
* Function:	phy_RFSerialRead
*
* OverView:	Read regster from RF chips 
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u4Byte			Offset,		//The target address to be read			
*
* Output:	None
* Return:		u4Byte			reback value
* Note:		Threre are three types of serial operations: 
*			1. Software serial write
*			2. Hardware LSSI-Low Speed Serial Interface 
*			3. Hardware HSSI-High speed
*			serial write. Driver need to implement (1) and (2).
*			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
*/
static	u32
phy_RFSerialRead(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				Offset
	)
{
	u32						retValue = 0;
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(Adapter);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &pHalData->PHYRegDef[eRFPath];
	u32						NewOffset;
	u32 						tmplong,tmplong2;
	u8					RfPiEnable=0;
	u8					i;
#if 0
	if(pHalData->RFChipID == RF_8225 && Offset > 0x24) //36 valid regs
		return	retValue;
	if(pHalData->RFChipID == RF_8256 && Offset > 0x2D) //45 valid regs
		return	retValue;
#endif
	//
	// Make sure RF register offset is correct 
	//
	//92D RF offset >0x3f

	//
	// Switch page for 8256 RF IC
	//
	NewOffset = Offset;

	// 2009/06/17 MH We can not execute IO for power save or other accident mode.
	//if(RT_CANNOT_IO(Adapter))
	//{
	//	RTPRINT(FPHY, PHY_RFR, ("phy_RFSerialRead return all one\n"));
	//	return	0xFFFFFFFF;
	//}

	// For 92S LSSI Read RFLSSIRead
	// For RF A/B write 0x824/82c(does not work in the future) 
	// We must use 0x824 for RF A and B to execute read trigger
	tmplong = PHY_QueryBBReg(Adapter, rFPGA0_XA_HSSIParameter2, bMaskDWord);
	if(eRFPath == RF90_PATH_A)
		tmplong2 = tmplong;
	else
		tmplong2 = PHY_QueryBBReg(Adapter, pPhyReg->rfHSSIPara2, bMaskDWord);
	tmplong2 = (tmplong2 & (~bLSSIReadAddress)) | (NewOffset<<23) | bLSSIReadEdge;	//T65 RF
	
	PHY_SetBBReg(Adapter, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong&(~bLSSIReadEdge));	
	//udelay_os(1000);
	udelay_os(10);
	
	PHY_SetBBReg(Adapter, pPhyReg->rfHSSIPara2, bMaskDWord, tmplong2);	
	//udelay_os(1000);
	for(i=0;i<2;i++)
		udelay_os(MAX_STALL_TIME);
	
	PHY_SetBBReg(Adapter, rFPGA0_XA_HSSIParameter2, bMaskDWord, tmplong|bLSSIReadEdge);
	//udelay_os(1000);
	udelay_os(10);

	if(eRFPath == RF90_PATH_A)
		RfPiEnable = (u8)PHY_QueryBBReg(Adapter, rFPGA0_XA_HSSIParameter1, BIT8);
	else if(eRFPath == RF90_PATH_B)
		RfPiEnable = (u8)PHY_QueryBBReg(Adapter, rFPGA0_XB_HSSIParameter1, BIT8);
	
	if(RfPiEnable)
	{	// Read from BBreg8b8, 12 bits for 8190, 20bits for T65 RF
		retValue = PHY_QueryBBReg(Adapter, pPhyReg->rfLSSIReadBackPi, bLSSIReadBackData);
		//RTPRINT(FINIT, INIT_RF, ("Readback from RF-PI : 0x%x\n", retValue));
	}
	else
	{	//Read from BBreg8a0, 12 bits for 8190, 20 bits for T65 RF
		retValue = PHY_QueryBBReg(Adapter, pPhyReg->rfLSSIReadBack, bLSSIReadBackData);
		//RTPRINT(FINIT, INIT_RF,("Readback from RF-SI : 0x%x\n", retValue));
	}
	//RTPRINT(FPHY, PHY_RFR, ("RFR-%d Addr[0x%lx]=0x%lx\n", eRFPath, pPhyReg->rfLSSIReadBack, retValue));
	
	return retValue;	
		
}



/**
* Function:	phy_RFSerialWrite
*
* OverView:	Write data to RF register (page 8~) 
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u4Byte			Offset,		//The target address to be read			
*			u4Byte			Data			//The new register Data in the target bit position
*										//of the target to be read			
*
* Output:	None
* Return:		None
* Note:		Threre are three types of serial operations: 
*			1. Software serial write
*			2. Hardware LSSI-Low Speed Serial Interface 
*			3. Hardware HSSI-High speed
*			serial write. Driver need to implement (1) and (2).
*			This function is equal to the combination of RF_ReadReg() and  RFLSSIRead()
 *
 * Note: 		  For RF8256 only
 *			 The total count of RTL8256(Zebra4) register is around 36 bit it only employs 
 *			 4-bit RF address. RTL8256 uses "register mode control bit" (Reg00[12], Reg00[10]) 
 *			 to access register address bigger than 0xf. See "Appendix-4 in PHY Configuration
 *			 programming guide" for more details. 
 *			 Thus, we define a sub-finction for RTL8526 register address conversion
 *		       ===========================================================
 *			 Register Mode		RegCTL[1]		RegCTL[0]		Note
 *								(Reg00[12])		(Reg00[10])
 *		       ===========================================================
 *			 Reg_Mode0				0				x			Reg 0 ~15(0x0 ~ 0xf)
 *		       ------------------------------------------------------------------
 *			 Reg_Mode1				1				0			Reg 16 ~30(0x1 ~ 0xf)
 *		       ------------------------------------------------------------------
 *			 Reg_Mode2				1				1			Reg 31 ~ 45(0x1 ~ 0xf)
 *		       ------------------------------------------------------------------
 *
 *	2008/09/02	MH	Add 92S RF definition
 *	
 *
 *
*/
static	VOID
phy_RFSerialWrite(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				Offset,
	IN	u32				Data
	)
{
	u32						DataAndAddr = 0;
	HAL_DATA_TYPE				*pHalData = GET_HAL_DATA(Adapter);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &pHalData->PHYRegDef[eRFPath];
	u32						NewOffset;
	
#if 0
	//<Roger_TODO> We should check valid regs for RF_6052 case.
	if(pHalData->RFChipID == RF_8225 && Offset > 0x24) //36 valid regs
		return;
	if(pHalData->RFChipID == RF_8256 && Offset > 0x2D) //45 valid regs
		return;
#endif

	// 2009/06/17 MH We can not execute IO for power save or other accident mode.
	//if(RT_CANNOT_IO(Adapter))
	//{
	//	RTPRINT(FPHY, PHY_RFW, ("phy_RFSerialWrite stop\n"));
	//	return;
	//}

	//
	//92D RF offset >0x3f

	//
	// Shadow Update
	//
	//PHY_RFShadowWrite(Adapter, eRFPath, Offset, Data);	

	//
	// Switch page for 8256 RF IC
	//
	NewOffset = Offset;

	//
	// Put write addr in [5:0]  and write data in [31:16]
	//
	//DataAndAddr = (Data<<16) | (NewOffset&0x3f);
	DataAndAddr = ((NewOffset<<20) | (Data&0x000fffff)) & 0x0fffffff;	// T65 RF

	//
	// Write Operation
	//
	PHY_SetBBReg(Adapter, pPhyReg->rf3wireOffset, bMaskDWord, DataAndAddr);
	//RTPRINT(FPHY, PHY_RFW, ("RFW-%d Addr[0x%lx]=0x%lx\n", eRFPath, pPhyReg->rf3wireOffset, DataAndAddr));

}


/**
* Function:	PHY_QueryRFReg
*
* OverView:	Query "Specific bits" to RF register (page 8~) 
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u4Byte			RegAddr,		//The target address to be read
*			u4Byte			BitMask		//The target bit position in the target address
*										//to be read	
*
* Output:	None
* Return:		u4Byte			Readback value
* Note:		This function is equal to "GetRFRegSetting" in PHY programming guide
*/
u32
rtl8192d_PHY_QueryRFReg(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				RegAddr,
	IN	u32				BitMask
	)
{
	u32 Original_Value, Readback_Value, BitShift;	
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	//u8	RFWaitCounter = 0;

#if (DISABLE_BB_RF == 1)
	return 0;
#endif

	if(!pHalData->bPhyValueInitReady)
		return 0;

	//RT_TRACE(COMP_RF, DBG_TRACE, ("--->PHY_QueryRFReg(): RegAddr(%#lx), eRFPath(%#x), BitMask(%#lx)\n", RegAddr, eRFPath,BitMask));
	
#if (DEV_BUS_TYPE==DEV_BUS_USB_INTERFACE)
	//PlatformAcquireMutex(&pHalData->mxRFOperate);
#else
	//PlatformAcquireSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
#endif

	
	Original_Value = phy_RFSerialRead(Adapter, eRFPath, RegAddr);
	
	BitShift =  phy_CalculateBitShift(BitMask);
	Readback_Value = (Original_Value & BitMask) >> BitShift;	

#if (DEV_BUS_TYPE==DEV_BUS_USB_INTERFACE)
	//PlatformReleaseMutex(&pHalData->mxRFOperate);
#else
	//PlatformReleaseSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
#endif


	//RTPRINT(FPHY, PHY_RFR, ("RFR-%d MASK=0x%lx Addr[0x%lx]=0x%lx\n", eRFPath, BitMask, RegAddr, Original_Value));//BitMask(%#lx),BitMask,
	//RT_TRACE(COMP_RF, DBG_TRACE, ("<---PHY_QueryRFReg(): RegAddr(%#lx), eRFPath(%#x),  Original_Value(%#lx)\n", 
	//				RegAddr, eRFPath, Original_Value));
	
	return (Readback_Value);
}

/**
* Function:	PHY_SetRFReg
*
* OverView:	Write "Specific bits" to RF register (page 8~) 
*
* Input:
*			PADAPTER		Adapter,
*			RF90_RADIO_PATH_E	eRFPath,	//Radio path of A/B/C/D
*			u4Byte			RegAddr,		//The target address to be modified
*			u4Byte			BitMask		//The target bit position in the target address
*										//to be modified	
*			u4Byte			Data			//The new register Data in the target bit position
*										//of the target address			
*
* Output:	None
* Return:		None
* Note:		This function is equal to "PutRFRegSetting" in PHY programming guide
*/
VOID
rtl8192d_PHY_SetRFReg(
	IN	PADAPTER			Adapter,
	IN	RF90_RADIO_PATH_E	eRFPath,
	IN	u32				RegAddr,
	IN	u32				BitMask,
	IN	u32				Data
	)
{

	HAL_DATA_TYPE	*pHalData		= GET_HAL_DATA(Adapter);
	//u1Byte			RFWaitCounter	= 0;
	u32 			Original_Value, BitShift;

#if (DISABLE_BB_RF == 1)
	return;
#endif

	if(!pHalData->bPhyValueInitReady)
		return;

	if(BitMask == 0)
		return;

	//RT_TRACE(COMP_RF, DBG_TRACE, ("--->PHY_SetRFReg(): RegAddr(%#lx), BitMask(%#lx), Data(%#lx), eRFPath(%#x)\n", 
	//	RegAddr, BitMask, Data, eRFPath));
	//RTPRINT(FINIT, INIT_RF, ("PHY_SetRFReg(): RegAddr(%#lx), BitMask(%#lx), Data(%#lx), eRFPath(%#x)\n", 
	//	RegAddr, BitMask, Data, eRFPath));


#if (DEV_BUS_TYPE==DEV_BUS_USB_INTERFACE)
	//PlatformAcquireMutex(&pHalData->mxRFOperate);
#else
	//PlatformAcquireSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
#endif

	
	// RF data is 12 bits only
	if (BitMask != bRFRegOffsetMask) 
	{
		Original_Value = phy_RFSerialRead(Adapter, eRFPath, RegAddr);
		BitShift =  phy_CalculateBitShift(BitMask);
		Data = (((Original_Value) & (~BitMask)) | (Data<< BitShift));
	}
		
	phy_RFSerialWrite(Adapter, eRFPath, RegAddr, Data);
	


#if (DEV_BUS_TYPE==DEV_BUS_USB_INTERFACE)
	//PlatformReleaseMutex(&pHalData->mxRFOperate);
#else
	//PlatformReleaseSpinLock(Adapter, RT_RF_OPERATE_SPINLOCK);
#endif
	
	//PHY_QueryRFReg(Adapter,eRFPath,RegAddr,BitMask);
	//RT_TRACE(COMP_RF, DBG_TRACE, ("<---PHY_SetRFReg(): RegAddr(%#lx), BitMask(%#lx), Data(%#lx), eRFPath(%#x)\n", 
	//		RegAddr, BitMask, Data, eRFPath));

}


//
// 3. Initial MAC/BB/RF config by reading MAC/BB/RF txt.
//

/*-----------------------------------------------------------------------------
 * Function:    phy_ConfigMACWithParaFile()
 *
 * Overview:    This function read BB parameters from general file format, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		Adapter
 *			ps1Byte 			pFileName			
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note: 		The format of MACPHY_REG.txt is different from PHY and RF. 
 *			[Register][Mask][Value]
 *---------------------------------------------------------------------------*/
static	int
phy_ConfigMACWithParaFile(
	IN	PADAPTER		Adapter,
	IN	u8* 			pFileName
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	int		rtStatus = _SUCCESS;

	return rtStatus;
}

/*-----------------------------------------------------------------------------
 * Function:    phy_ConfigMACWithHeaderFile()
 *
 * Overview:    This function read BB parameters from Header file we gen, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		Adapter
 *			ps1Byte 			pFileName			
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note: 		The format of MACPHY_REG.txt is different from PHY and RF. 
 *			[Register][Mask][Value]
 *---------------------------------------------------------------------------*/
static	int
phy_ConfigMACWithHeaderFile(
	IN	PADAPTER		Adapter
)
{
	u32					i = 0;
	u32					ArrayLength = 0;
	u32*				ptrArray;	
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);

	//2008.11.06 Modified by tynli.
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Read Rtl819XMACPHY_Array\n"));

	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		ArrayLength = MAC_2TArrayLength;
		ptrArray = Rtl819XMAC_Array;
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> phy_ConfigMACWithHeaderFile() Img:Rtl819XMAC_Array\n"));
	}
	else
	{
		ArrayLength = Rtl8192DTestMAC_2TArrayLength;
		ptrArray = Rtl8192DTestMAC_2TArray;
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> phy_ConfigMACWithHeaderFile() Img:Rtl8192DTestMAC_2T_Array\n"));
	}

	for(i = 0 ;i < ArrayLength;i=i+2){ // Add by tynli for 2 column
		write8(Adapter, ptrArray[i], (u8)ptrArray[i+1]);
	}
	
	return _SUCCESS;
	
}

/*-----------------------------------------------------------------------------
 * Function:    PHY_MACConfig8192C
 *
 * Overview:	Condig MAC by header file or parameter file.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 *  When		Who		Remark
 *  08/12/2008	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	int
PHY_MACConfig8192D(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	char		*pszMACRegFile;
	char		sz92DMACRegFile[] = RTL8192D_PHY_MACREG;
	int		rtStatus = _SUCCESS;

	if(Adapter->bSurpriseRemoved){
		rtStatus = _FAIL;
		return rtStatus;
	}

	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		//Now we just support SMSP.
		if(pHalData->MacPhyMode92D == SINGLEMAC_SINGLEPHY)
			pszMACRegFile = sz92DMACRegFile;
		else
			return rtStatus;
	}
	else{
		if(pHalData->MacPhyMode92D == SINGLEMAC_SINGLEPHY)
			pszMACRegFile = sz92DMACRegFile;
		else
			return rtStatus;
	}

	//
	// Config MAC
	//
#ifdef CONFIG_EMBEDDED_FWIMG
	rtStatus = phy_ConfigMACWithHeaderFile(Adapter);
#else
	
	// Not make sure EEPROM, add later
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Read MACREG.txt\n"));
	rtStatus = phy_ConfigMACWithParaFile(Adapter, pszMACRegFile);
#endif

	return rtStatus;

}

/**
* Function:	phy_InitBBRFRegisterDefinition
*
* OverView:	Initialize Register definition offset for Radio Path A/B/C/D
*
* Input:
*			PADAPTER		Adapter,
*
* Output:	None
* Return:		None
* Note:		The initialization value is constant and it should never be changes
*/
static	VOID
phy_InitBBRFRegisterDefinition(
	IN	PADAPTER		Adapter
)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);	

	// RF Interface Sowrtware Control
	pHalData->PHYRegDef[RF90_PATH_A].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 LSBs if read 32-bit from 0x870
	pHalData->PHYRegDef[RF90_PATH_B].rfintfs = rFPGA0_XAB_RFInterfaceSW; // 16 MSBs if read 32-bit from 0x870 (16-bit for 0x872)
	pHalData->PHYRegDef[RF90_PATH_C].rfintfs = rFPGA0_XCD_RFInterfaceSW;// 16 LSBs if read 32-bit from 0x874
	pHalData->PHYRegDef[RF90_PATH_D].rfintfs = rFPGA0_XCD_RFInterfaceSW;// 16 MSBs if read 32-bit from 0x874 (16-bit for 0x876)

	// RF Interface Readback Value
	pHalData->PHYRegDef[RF90_PATH_A].rfintfi = rFPGA0_XAB_RFInterfaceRB; // 16 LSBs if read 32-bit from 0x8E0	
	pHalData->PHYRegDef[RF90_PATH_B].rfintfi = rFPGA0_XAB_RFInterfaceRB;// 16 MSBs if read 32-bit from 0x8E0 (16-bit for 0x8E2)
	pHalData->PHYRegDef[RF90_PATH_C].rfintfi = rFPGA0_XCD_RFInterfaceRB;// 16 LSBs if read 32-bit from 0x8E4
	pHalData->PHYRegDef[RF90_PATH_D].rfintfi = rFPGA0_XCD_RFInterfaceRB;// 16 MSBs if read 32-bit from 0x8E4 (16-bit for 0x8E6)

	// RF Interface Output (and Enable)
	pHalData->PHYRegDef[RF90_PATH_A].rfintfo = rFPGA0_XA_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x860
	pHalData->PHYRegDef[RF90_PATH_B].rfintfo = rFPGA0_XB_RFInterfaceOE; // 16 LSBs if read 32-bit from 0x864

	// RF Interface (Output and)  Enable
	pHalData->PHYRegDef[RF90_PATH_A].rfintfe = rFPGA0_XA_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x860 (16-bit for 0x862)
	pHalData->PHYRegDef[RF90_PATH_B].rfintfe = rFPGA0_XB_RFInterfaceOE; // 16 MSBs if read 32-bit from 0x864 (16-bit for 0x866)

	//Addr of LSSI. Wirte RF register by driver
	pHalData->PHYRegDef[RF90_PATH_A].rf3wireOffset = rFPGA0_XA_LSSIParameter; //LSSI Parameter
	pHalData->PHYRegDef[RF90_PATH_B].rf3wireOffset = rFPGA0_XB_LSSIParameter;

	// RF parameter
	pHalData->PHYRegDef[RF90_PATH_A].rfLSSI_Select = rFPGA0_XAB_RFParameter;  //BB Band Select
	pHalData->PHYRegDef[RF90_PATH_B].rfLSSI_Select = rFPGA0_XAB_RFParameter;
	pHalData->PHYRegDef[RF90_PATH_C].rfLSSI_Select = rFPGA0_XCD_RFParameter;
	pHalData->PHYRegDef[RF90_PATH_D].rfLSSI_Select = rFPGA0_XCD_RFParameter;

	// Tx AGC Gain Stage (same for all path. Should we remove this?)
	pHalData->PHYRegDef[RF90_PATH_A].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	pHalData->PHYRegDef[RF90_PATH_B].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	pHalData->PHYRegDef[RF90_PATH_C].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage
	pHalData->PHYRegDef[RF90_PATH_D].rfTxGainStage = rFPGA0_TxGainStage; //Tx gain stage

	// Tranceiver A~D HSSI Parameter-1
	pHalData->PHYRegDef[RF90_PATH_A].rfHSSIPara1 = rFPGA0_XA_HSSIParameter1;  //wire control parameter1
	pHalData->PHYRegDef[RF90_PATH_B].rfHSSIPara1 = rFPGA0_XB_HSSIParameter1;  //wire control parameter1

	// Tranceiver A~D HSSI Parameter-2
	pHalData->PHYRegDef[RF90_PATH_A].rfHSSIPara2 = rFPGA0_XA_HSSIParameter2;  //wire control parameter2
	pHalData->PHYRegDef[RF90_PATH_B].rfHSSIPara2 = rFPGA0_XB_HSSIParameter2;  //wire control parameter2

	// RF switch Control
	pHalData->PHYRegDef[RF90_PATH_A].rfSwitchControl = rFPGA0_XAB_SwitchControl; //TR/Ant switch control
	pHalData->PHYRegDef[RF90_PATH_B].rfSwitchControl = rFPGA0_XAB_SwitchControl;
	pHalData->PHYRegDef[RF90_PATH_C].rfSwitchControl = rFPGA0_XCD_SwitchControl;
	pHalData->PHYRegDef[RF90_PATH_D].rfSwitchControl = rFPGA0_XCD_SwitchControl;

	// AGC control 1 
	pHalData->PHYRegDef[RF90_PATH_A].rfAGCControl1 = rOFDM0_XAAGCCore1;
	pHalData->PHYRegDef[RF90_PATH_B].rfAGCControl1 = rOFDM0_XBAGCCore1;
	pHalData->PHYRegDef[RF90_PATH_C].rfAGCControl1 = rOFDM0_XCAGCCore1;
	pHalData->PHYRegDef[RF90_PATH_D].rfAGCControl1 = rOFDM0_XDAGCCore1;

	// AGC control 2 
	pHalData->PHYRegDef[RF90_PATH_A].rfAGCControl2 = rOFDM0_XAAGCCore2;
	pHalData->PHYRegDef[RF90_PATH_B].rfAGCControl2 = rOFDM0_XBAGCCore2;
	pHalData->PHYRegDef[RF90_PATH_C].rfAGCControl2 = rOFDM0_XCAGCCore2;
	pHalData->PHYRegDef[RF90_PATH_D].rfAGCControl2 = rOFDM0_XDAGCCore2;

	// RX AFE control 1 
	pHalData->PHYRegDef[RF90_PATH_A].rfRxIQImbalance = rOFDM0_XARxIQImbalance;
	pHalData->PHYRegDef[RF90_PATH_B].rfRxIQImbalance = rOFDM0_XBRxIQImbalance;
	pHalData->PHYRegDef[RF90_PATH_C].rfRxIQImbalance = rOFDM0_XCRxIQImbalance;
	pHalData->PHYRegDef[RF90_PATH_D].rfRxIQImbalance = rOFDM0_XDRxIQImbalance;	

	// RX AFE control 1  
	pHalData->PHYRegDef[RF90_PATH_A].rfRxAFE = rOFDM0_XARxAFE;
	pHalData->PHYRegDef[RF90_PATH_B].rfRxAFE = rOFDM0_XBRxAFE;
	pHalData->PHYRegDef[RF90_PATH_C].rfRxAFE = rOFDM0_XCRxAFE;
	pHalData->PHYRegDef[RF90_PATH_D].rfRxAFE = rOFDM0_XDRxAFE;	

	// Tx AFE control 1 
	pHalData->PHYRegDef[RF90_PATH_A].rfTxIQImbalance = rOFDM0_XATxIQImbalance;
	pHalData->PHYRegDef[RF90_PATH_B].rfTxIQImbalance = rOFDM0_XBTxIQImbalance;
	pHalData->PHYRegDef[RF90_PATH_C].rfTxIQImbalance = rOFDM0_XCTxIQImbalance;
	pHalData->PHYRegDef[RF90_PATH_D].rfTxIQImbalance = rOFDM0_XDTxIQImbalance;	

	// Tx AFE control 2 
	pHalData->PHYRegDef[RF90_PATH_A].rfTxAFE = rOFDM0_XATxAFE;
	pHalData->PHYRegDef[RF90_PATH_B].rfTxAFE = rOFDM0_XBTxAFE;
	pHalData->PHYRegDef[RF90_PATH_C].rfTxAFE = rOFDM0_XCTxAFE;
	pHalData->PHYRegDef[RF90_PATH_D].rfTxAFE = rOFDM0_XDTxAFE;	

	// Tranceiver LSSI Readback SI mode 
	pHalData->PHYRegDef[RF90_PATH_A].rfLSSIReadBack = rFPGA0_XA_LSSIReadBack;
	pHalData->PHYRegDef[RF90_PATH_B].rfLSSIReadBack = rFPGA0_XB_LSSIReadBack;
	pHalData->PHYRegDef[RF90_PATH_C].rfLSSIReadBack = rFPGA0_XC_LSSIReadBack;
	pHalData->PHYRegDef[RF90_PATH_D].rfLSSIReadBack = rFPGA0_XD_LSSIReadBack;	

	// Tranceiver LSSI Readback PI mode 
	pHalData->PHYRegDef[RF90_PATH_A].rfLSSIReadBackPi = TransceiverA_HSPI_Readback;
	pHalData->PHYRegDef[RF90_PATH_B].rfLSSIReadBackPi = TransceiverB_HSPI_Readback;
	//pHalData->PHYRegDef[RF90_PATH_C].rfLSSIReadBackPi = rFPGA0_XC_LSSIReadBack;
	//pHalData->PHYRegDef[RF90_PATH_D].rfLSSIReadBackPi = rFPGA0_XD_LSSIReadBack;	
	pHalData->bPhyValueInitReady = _TRUE;
}

// Joseph test: new initialize order!!
// Test only!! This part need to be re-organized.
// Now it is just for 8256.
static	int
phy_BB8190_Config_HardCode(
	IN	PADAPTER	Adapter
	)
{
	//RT_ASSERT(FALSE, ("This function is not implement yet!! \n"));
	return _SUCCESS;
}

//****************************************
// The following is for High Power PA
//****************************************
VOID
phy_ConfigBBExternalPA(
	IN	PADAPTER			Adapter
)
{
#if (DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u16	i=0;
	u32	temp=0;

	if(!pHalData->ExternalPA)
	{
		return;
	}

	PHY_SetBBReg(Adapter, 0xee8, BIT28, 1);
	temp = PHY_QueryBBReg(Adapter, rFPGA0_XA_RFInterfaceOE, bMaskDWord);
	temp |= (BIT26|BIT21|BIT10|BIT5);
	PHY_SetBBReg(Adapter, rFPGA0_XA_RFInterfaceOE, bMaskDWord, temp);
	PHY_SetBBReg(Adapter, rFPGA0_XAB_RFInterfaceSW, BIT10, 0);
	PHY_SetBBReg(Adapter, rOFDM0_XATxIQImbalance, bMaskDWord, 0x20000080);
	PHY_SetBBReg(Adapter, rOFDM0_XBTxIQImbalance, bMaskDWord, 0x40000100);
#endif
}

/*-----------------------------------------------------------------------------
 * Function:    phy_ConfigBBWithHeaderFile()
 *
 * Overview:    This function read BB parameters from general file format, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		Adapter
 *			u1Byte 			ConfigType     0 => PHY_CONFIG
 *										 1 =>AGC_TAB
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 *---------------------------------------------------------------------------*/
static	int
phy_ConfigBBWithHeaderFile(
	IN	PADAPTER		Adapter,
	IN	u8 			ConfigType
)
{
	int i;
	u32*	Rtl819XPHY_REGArray_Table;
	u32*	Rtl819XAGCTAB_Array_Table;
	u32*	Rtl819XAGCTAB_5GArray_Table;
	u16		PHY_REGArrayLen, AGCTAB_ArrayLen, AGCTAB_5GArrayLen;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	//vivi modify for BB header array, to ID 92d and 92c, 20100528
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		//Normal chip,Mac0 use AGC_TAB.txt for 2G and 5G band.
		if(pHalData->interfaceIndex == 0)
		{
			AGCTAB_ArrayLen = AGCTAB_ArrayLength;
			Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_Array;
			//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> phy_ConfigBBWithHeaderFile() phy:MAC0, Rtl819XAGCTAB_Array\n"));
		}
		else
		{
			if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
			{
				AGCTAB_ArrayLen = AGCTAB_2GArrayLength;
				Rtl819XAGCTAB_Array_Table = Rtl819XAGCTAB_2GArray;
				//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> phy_ConfigBBWithHeaderFile() phy:MAC1, Rtl819XAGCTAB_2GArray\n"));
			}
			else
			{
				AGCTAB_5GArrayLen = AGCTAB_5GArrayLength;
				Rtl819XAGCTAB_5GArray_Table = Rtl819XAGCTAB_5GArray;
				//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> phy_ConfigBBWithHeaderFile() phy:MAC1, Rtl819XAGCTAB_5GArray\n"));
			}
		}
	
		PHY_REGArrayLen = PHY_REG_2TArrayLength;
		Rtl819XPHY_REGArray_Table = Rtl819XPHY_REG_2TArray;
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> phy_ConfigBBWithHeaderFile() phy:Rtl819XPHY_REG_Array_PG\n"));
	}
	else
	{
		if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
		{
			AGCTAB_ArrayLen = Rtl8192DTestAGCTAB_2GArrayLength;
			Rtl819XAGCTAB_Array_Table = Rtl8192DTestAGCTAB_2GArray;
			//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> phy_ConfigBBWithHeaderFile() agc:Rtl8192CTestAGCTAB_2G\n"))

		}
		else
		{
			AGCTAB_5GArrayLen = Rtl8192DTestAGCTAB_5GArrayLength;
			Rtl819XAGCTAB_5GArray_Table = Rtl8192DTestAGCTAB_5GArray;
			//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> phy_ConfigBBWithHeaderFile() agc:Rtl8192CTestAGCTAB_5G\n"));
		}

		PHY_REGArrayLen = Rtl8192DTestPHY_REG_2TArrayLength;
		Rtl819XPHY_REGArray_Table = Rtl8192DTestPHY_REG_2TArray;
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> phy_ConfigBBWithHeaderFile() phy:Rtl8192DTestPHY_REG_Array_PG\n"));
	}

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArrayLen;i=i+2)
		{
			if (Rtl819XPHY_REGArray_Table[i] == 0xfe)
				mdelay_os(50);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfd)
				mdelay_os(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfc)
				mdelay_os(1);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfb)
				udelay_os(50);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xfa)
				udelay_os(5);
			else if (Rtl819XPHY_REGArray_Table[i] == 0xf9)
				udelay_os(1);
			PHY_SetBBReg(Adapter, Rtl819XPHY_REGArray_Table[i], bMaskDWord, Rtl819XPHY_REGArray_Table[i+1]);		

			// Add 1us delay between BB/RF register setting.
			udelay_os(1);

			//RT_TRACE(COMP_INIT, DBG_TRACE, ("The Rtl819XPHY_REGArray_Table[0] is %lx Rtl819XPHY_REGArray[1] is %lx \n",Rtl819XPHY_REGArray_Table[i], Rtl819XPHY_REGArray_Table[i+1]));
		}
		// for External PA
		phy_ConfigBBExternalPA(Adapter);
	}
	else if(ConfigType == BaseBand_Config_AGC_TAB)
	{
		//especial for 5G, vivi, 20100528
		if(IS_NORMAL_CHIP(pHalData->VersionID)&&(pHalData->interfaceIndex == 0))
		{
			for(i=0;i<AGCTAB_ArrayLen;i=i+2)
			{
				PHY_SetBBReg(Adapter, Rtl819XAGCTAB_Array_Table[i], bMaskDWord, Rtl819XAGCTAB_Array_Table[i+1]);		

				// Add 1us delay between BB/RF register setting.
				udelay_os(1);
					
				//RT_TRACE(COMP_INIT, DBG_TRACE, ("The Rtl819XAGCTAB_Array_Table[0] is %lx Rtl819XPHY_REGArray[1] is %lx \n",Rtl819XAGCTAB_Array_Table[i], Rtl819XAGCTAB_Array_Table[i+1]));
			}
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("Normal Chip, MAC0, load Rtl819XAGCTAB_Array\n"));
		}
		else
		{
			if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
			{
				for(i=0;i<AGCTAB_ArrayLen;i=i+2)
				{
					PHY_SetBBReg(Adapter, Rtl819XAGCTAB_Array_Table[i], bMaskDWord, Rtl819XAGCTAB_Array_Table[i+1]);		

					// Add 1us delay between BB/RF register setting.
					udelay_os(1);
					
					//RT_TRACE(COMP_INIT, DBG_TRACE, ("The Rtl819XAGCTAB_Array_Table[0] is %lx Rtl819XPHY_REGArray[1] is %lx \n",Rtl819XAGCTAB_Array_Table[i], Rtl819XAGCTAB_Array_Table[i+1]));
				}
				//RT_TRACE(COMP_INIT, DBG_LOUD, ("Load Rtl819XAGCTAB_2GArray\n"));
			}
			else
			{
				for(i=0;i<AGCTAB_5GArrayLen;i=i+2)
				{
					PHY_SetBBReg(Adapter, Rtl819XAGCTAB_5GArray_Table[i], bMaskDWord, Rtl819XAGCTAB_5GArray_Table[i+1]);		

					// Add 1us delay between BB/RF register setting.
					udelay_os(1);
					
					//RT_TRACE(COMP_INIT, DBG_TRACE, ("The Rtl819XAGCTAB_5GArray_Table[0] is %lx Rtl819XPHY_REGArray[1] is %lx \n",Rtl819XAGCTAB_5GArray_Table[i], Rtl819XAGCTAB_5GArray_Table[i+1]));
				}
				//RT_TRACE(COMP_INIT, DBG_LOUD, ("Load Rtl819XAGCTAB_5GArray\n"));
			}
		}
	}
	
	return _SUCCESS;
	
}

/*-----------------------------------------------------------------------------
 * Function:    phy_ConfigBBWithParaFile()
 *
 * Overview:    This function read BB parameters from general file format, and do register
 *			  Read/Write 
 *
 * Input:      	PADAPTER		Adapter
 *			ps1Byte 			pFileName			
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *	2008/11/06	MH	For 92S we do not support silent reset now. Disable 
 *					parameter file compare!!!!!!??
 *			
 *---------------------------------------------------------------------------*/
static	int
phy_ConfigBBWithParaFile(
	IN	PADAPTER		Adapter,
	IN	u8* 			pFileName
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	int		rtStatus = _SUCCESS;

	return rtStatus;	
}

static VOID
storePwrIndexDiffRateOffset(
	IN	PADAPTER	Adapter,
	IN	u32		RegAddr,
	IN	u32		BitMask,
	IN	u32		Data
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	if(RegAddr == rTxAGC_A_Rate18_06)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][0] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][0] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][0]));
	}
	if(RegAddr == rTxAGC_A_Rate54_24)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][1] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][1] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][1]));
	}
	if(RegAddr == rTxAGC_A_CCK1_Mcs32)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][6] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][6] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][6]));
	}
	if(RegAddr == rTxAGC_B_CCK11_A_CCK2_11 && BitMask == 0xffffff00)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][7] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][7] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][7]));
	}	
	if(RegAddr == rTxAGC_A_Mcs03_Mcs00)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][2] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][2] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][2]));
	}
	if(RegAddr == rTxAGC_A_Mcs07_Mcs04)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][3] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][3] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][3]));
	}
	if(RegAddr == rTxAGC_A_Mcs11_Mcs08)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][4] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][4] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][4]));
	}
	if(RegAddr == rTxAGC_A_Mcs15_Mcs12)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][5] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][5] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][5]));
	}
	if(RegAddr == rTxAGC_B_Rate18_06)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][8] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][8] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][8]));
	}
	if(RegAddr == rTxAGC_B_Rate54_24)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][9] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][9] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][9]));
	}
	if(RegAddr == rTxAGC_B_CCK1_55_Mcs32)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][14] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][14] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][14]));
	}
	if(RegAddr == rTxAGC_B_CCK11_A_CCK2_11 && BitMask == 0x000000ff)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][15] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][15] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][15]));
	}	
	if(RegAddr == rTxAGC_B_Mcs03_Mcs00)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][10] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][10] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][10]));
	}
	if(RegAddr == rTxAGC_B_Mcs07_Mcs04)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][11] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][11] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][11]));
	}
	if(RegAddr == rTxAGC_B_Mcs11_Mcs08)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][12] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][12] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][12]));
	}
	if(RegAddr == rTxAGC_B_Mcs15_Mcs12)
	{
		pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][13] = Data;
		//RT_TRACE(COMP_INIT, DBG_TRACE, ("MCSTxPowerLevelOriginalOffset[%d][13] = 0x%lx\n", pHalData->pwrGroupCnt,
		//	pHalData->MCSTxPowerLevelOriginalOffset[pHalData->pwrGroupCnt][13]));
		pHalData->pwrGroupCnt++;
	}
}

/*-----------------------------------------------------------------------------
 * Function:	phy_ConfigBBWithPgHeaderFile
 *
 * Overview:	Config PHY_REG_PG array 
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/06/2008 	MHC		Add later!!!!!!.. Please modify for new files!!!!
 * 11/10/2008	tynli		Modify to mew files.
 *---------------------------------------------------------------------------*/
static	int
phy_ConfigBBWithPgHeaderFile(
	IN	PADAPTER		Adapter,
	IN	u8 			ConfigType)
{
	int i;
	u32*	Rtl819XPHY_REGArray_Table_PG;
	u16	PHY_REGArrayPGLen;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	// Default: pHalData->RF_Type = RF_2T2R.
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		PHY_REGArrayPGLen = PHY_REG_Array_PGLength;
		Rtl819XPHY_REGArray_Table_PG = Rtl819XPHY_REG_Array_PG;
	}
	else
	{
		return _SUCCESS;
	}

	if(ConfigType == BaseBand_Config_PHY_REG)
	{
		for(i=0;i<PHY_REGArrayPGLen;i=i+3)
		{
			if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfe)
				mdelay_os(50);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfd)
				mdelay_os(5);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfc)
				mdelay_os(1);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfb)
				udelay_os(50);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xfa)
				udelay_os(5);
			else if (Rtl819XPHY_REGArray_Table_PG[i] == 0xf9)
				udelay_os(1);
			storePwrIndexDiffRateOffset(Adapter, Rtl819XPHY_REGArray_Table_PG[i], 
				Rtl819XPHY_REGArray_Table_PG[i+1], 
				Rtl819XPHY_REGArray_Table_PG[i+2]);
			//PHY_SetBBReg(Adapter, Rtl819XPHY_REGArray_Table_PG[i], Rtl819XPHY_REGArray_Table_PG[i+1], Rtl819XPHY_REGArray_Table_PG[i+2]);		
			//RT_TRACE(COMP_SEND, DBG_TRACE, ("The Rtl819XPHY_REGArray_Table_PG[0] is %lx Rtl819XPHY_REGArray_Table_PG[1] is %lx \n",Rtl819XPHY_REGArray_Table_PG[i], Rtl819XPHY_REGArray_Table_PG[i+1]));
		}
	}
	else
	{

		//RT_TRACE(COMP_SEND, DBG_LOUD, ("phy_ConfigBBWithPgHeaderFile(): ConfigType != BaseBand_Config_PHY_REG\n"));
	}
	
	return _SUCCESS;
	
}	/* phy_ConfigBBWithPgHeaderFile */

/*-----------------------------------------------------------------------------
 * Function:	phy_ConfigBBWithPgParaFile
 *
 * Overview:	
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/06/2008 	MHC		Create Version 0.
 * 2009/07/29	tynli		(porting from 92SE branch)2009/03/11 Add copy parameter file to buffer for silent reset
 *---------------------------------------------------------------------------*/
static	int
phy_ConfigBBWithPgParaFile(
	IN	PADAPTER		Adapter,
	IN	u8* 			pFileName)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	int		rtStatus = _SUCCESS;


	return rtStatus;
	
}	/* phy_ConfigBBWithPgParaFile */

static	int
phy_BB8192D_Config_ParaFile(
	IN	PADAPTER	Adapter
	)
{
	EEPROM_EFUSE_PRIV	*pEEPROM = GET_EEPROM_EFUSE_PRIV(Adapter);
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	int		rtStatus = _SUCCESS;

	char		szBBRegPgFile[] = RTL819X_PHY_REG_PG;
	char		sz92DBBRegFile[] = RTL8192D_PHY_REG;
	char		sz92DBBRegPgFile[] = RTL8192D_PHY_REG;
	char		sz92DAGCTableFile[] = RTL8192D_AGC_TAB;
	char		sz92D2GAGCTableFile[] = RTL8192D_AGC_TAB_2G;
	char		sz92D5GAGCTableFile[] = RTL8192D_AGC_TAB_5G;

	char		sz92DTestBBRegFile[] = RTL8192D_PHY_REG;
	char		sz92DTestBBRegPgFile[] = RTL8192D_PHY_REG;
	char		sz92DTest2GAGCTableFile[] = RTL8192D_AGC_TAB_2G;
	char		sz92DTest5GAGCTableFile[] = RTL8192D_AGC_TAB_5G;

	char		*pszBBRegFile, *pszAGCTableFile, *pszBBRegPgFile;
	
	//RT_TRACE(COMP_INIT, DBG_TRACE, ("==>phy_BB8192S_Config_ParaFile\n"));

	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		pszBBRegFile = sz92DBBRegFile;
		pszBBRegPgFile = sz92DBBRegPgFile;

		//Normal chip,Mac0 use AGC_TAB.txt for 2G and 5G band.
		if(pHalData->interfaceIndex == 0)
		{
			pszAGCTableFile = sz92DAGCTableFile;
		}
		else
		{
			if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
				pszAGCTableFile = sz92D2GAGCTableFile;
			else
				pszAGCTableFile = sz92D5GAGCTableFile;
		}
	}
	else// testchip.
	{
		pszBBRegFile = sz92DTestBBRegFile;
		pszBBRegPgFile = sz92DTestBBRegPgFile;

		//pszAGCTableFile = sz92DTestAGCTableFile;
		if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
			pszAGCTableFile = sz92DTest2GAGCTableFile;	
		else
			pszAGCTableFile = sz92DTest5GAGCTableFile;
	}

	//
	// 1. Read PHY_REG.TXT BB INIT!!
	// We will seperate as 88C / 92C according to chip version
	//
#ifdef CONFIG_EMBEDDED_FWIMG
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(pHalData->bSlaveOfDMSP)
	{
		RT_TRACE(COMP_INIT,DBG_LOUD,("BB config slave skip \n"));
	}	
	else		
#endif
	{
		rtStatus = phy_ConfigBBWithHeaderFile(Adapter, BaseBand_Config_PHY_REG);
	}
#else	
	// No matter what kind of CHIP we always read PHY_REG.txt. We must copy different 
	// type of parameter files to phy_reg.txt at first.	
	rtStatus = phy_ConfigBBWithParaFile(Adapter,pszBBRegFile);
#endif

	if(rtStatus != _SUCCESS){
		//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("phy_BB8192S_Config_ParaFile():Write BB Reg Fail!!"));
		goto phy_BB8190_Config_ParaFile_Fail;
	}

#if MP_DRIVER == 1 
	if(IS_HARDWARE_TYPE_8192C(Adapter) || IS_HARDWARE_TYPE_8192D(Adapter))
	{
		//
		// 1.1 Read PHY_REG_MP.TXT BB INIT!!
		// We will seperate as 88C / 92C according to chip version
		//
#ifdef CONFIG_EMBEDDED_FWIMG
		rtStatus = phy_ConfigBBWithMpHeaderFile(Adapter, BaseBand_Config_PHY_REG);
#else	
		// No matter what kind of CHIP we always read PHY_REG.txt. We must copy different 
		// type of parameter files to phy_reg.txt at first.	
		rtStatus = phy_ConfigBBWithMpParaFile(Adapter,pszBBRegMpFile);
#endif

		if(rtStatus != RT_STATUS_SUCCESS){
			RT_TRACE(COMP_INIT, DBG_SERIOUS, ("phy_BB8192S_Config_ParaFile():Write BB Reg MP Fail!!"));
			goto phy_BB8190_Config_ParaFile_Fail;
		}
	}
#endif

	//
	// 2. If EEPROM or EFUSE autoload OK, We must config by PHY_REG_PG.txt
	//
	if (pEEPROM->bautoload_fail_flag == _FALSE)
	{
		pHalData->pwrGroupCnt = 0;

#ifdef CONFIG_EMBEDDED_FWIMG
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
		if(pHalData->bSlaveOfDMSP)
		{
			RT_TRACE(COMP_INIT,DBG_LOUD,("BB config slave skip  1111\n"));
		}
		else
#endif
		{
			rtStatus = phy_ConfigBBWithPgHeaderFile(Adapter, BaseBand_Config_PHY_REG);
		}
#else
		rtStatus = phy_ConfigBBWithPgParaFile(Adapter, (u8*)&szBBRegPgFile);
#endif
	}
	
	if(rtStatus != _SUCCESS){
		//RT_TRACE(COMP_INIT, DBG_SERIOUS, ("phy_BB8192S_Config_ParaFile():BB_PG Reg Fail!!"));
		goto phy_BB8190_Config_ParaFile_Fail;
	}

	//
	// 3. BB AGC table Initialization
	//
#ifdef CONFIG_EMBEDDED_FWIMG
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(pHalData->bSlaveOfDMSP)
	{
		RT_TRACE(COMP_INIT,DBG_LOUD,("BB config slave skip  2222 \n"));
	}
	else
#endif
		rtStatus = phy_ConfigBBWithHeaderFile(Adapter, BaseBand_Config_AGC_TAB);
#else
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("phy_BB8192S_Config_ParaFile AGC_TAB.txt\n"));
	rtStatus = phy_ConfigBBWithParaFile(Adapter, pszAGCTableFile);
#endif

	if(rtStatus != _SUCCESS){
		//RT_TRACE(COMP_FPGA, DBG_SERIOUS, ("phy_BB8192S_Config_ParaFile():AGC Table Fail\n"));
		goto phy_BB8190_Config_ParaFile_Fail;
	}

	// Check if the CCK HighPower is turned ON.
	// This is used to calculate PWDB.
	pHalData->bCckHighPower = (BOOLEAN)(PHY_QueryBBReg(Adapter, rFPGA0_XA_HSSIParameter2, 0x200));
	
phy_BB8190_Config_ParaFile_Fail:

	return rtStatus;
}

int
PHY_BBConfig8192D(
	IN	PADAPTER	Adapter
	)
{
	int	rtStatus = _SUCCESS;
	//u8		PathMap = 0, index = 0, rf_num = 0;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u32	RegVal;
	u8	value;

	if(Adapter->bSurpriseRemoved){
		rtStatus = _FAIL;
		return rtStatus;
	}

	phy_InitBBRFRegisterDefinition(Adapter);

	// Enable BB and RF
	RegVal = read16(Adapter, REG_SYS_FUNC_EN);
	write16(Adapter, REG_SYS_FUNC_EN, RegVal|BIT13|BIT0|BIT1);

	// 20090923 Joseph: Advised by Steven and Jenyu. Power sequence before init RF.
	write8(Adapter, REG_AFE_PLL_CTRL, 0x83);
	write8(Adapter, REG_AFE_PLL_CTRL+1, 0xdb);
	value=read8(Adapter, REG_RF_CTRL);     //  0x1f bit7 bit6 represent for mac0/mac1 driver ready
	write8(Adapter, REG_RF_CTRL, value|RF_EN|RF_RSTB|RF_SDMRSTB);

#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
	write8(Adapter, REG_SYS_FUNC_EN, FEN_USBA | FEN_USBD | FEN_BB_GLB_RSTn | FEN_BBRSTB);
#else
	write8(Adapter, REG_SYS_FUNC_EN, FEN_PPLL|FEN_PCIEA|FEN_DIO_PCIE|FEN_BB_GLB_RSTn|FEN_BBRSTB);
#endif

	// 2009/10/21 by SD1 Jong. Modified by tynli. Not in Documented in V8.1. 
	if(!IS_NORMAL_CHIP(pHalData->VersionID))
	{
#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
		write8(Adapter, REG_LDOHCI12_CTRL, 0x1f);
#else
		write8(Adapter, REG_LDOHCI12_CTRL, 0x1f);
#endif
	}

	write8(Adapter, REG_AFE_XTAL_CTRL+1, 0x80);

#if DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE
	// Force use left antenna by default for 88C.
	if(!(IS_92D_SINGLEPHY(pHalData->VersionID)))
	{
		RegVal = read32(Adapter, REG_LEDCFG0);
		write32(Adapter, REG_LEDCFG0, RegVal|BIT23);
	}
#endif

	//
	// Config BB and AGC
	//
	rtStatus = phy_BB8192D_Config_ParaFile(Adapter);
#if 0	
	switch(Adapter->MgntInfo.bRegHwParaFile)
	{
		case 0:
			phy_BB8190_Config_HardCode(Adapter);
			break;

		case 1:
			rtStatus = phy_BB8192C_Config_ParaFile(Adapter);
			break;

		case 2:
			// Partial Modify. 
			phy_BB8190_Config_HardCode(Adapter);
			phy_BB8192C_Config_ParaFile(Adapter);
			break;

		default:
			phy_BB8190_Config_HardCode(Adapter);
			break;
	}
#endif

#if MP_DRIVER == 1
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		PHY_SetBBReg(Adapter, 0x24, 0xF0, pHalData->CrystalCap & 0x0F);
		PHY_SetBBReg(Adapter, 0x28, 0xF0000000, ((pHalData->CrystalCap & 0xF0) >> 4));
	}
#endif

#if 0
	// Check BB/RF confiuration setting.
	// We only need to configure RF which is turned on.
	PathMap = (u1Byte)(PHY_QueryBBReg(Adapter, rFPGA0_TxInfo, 0xf) |
				PHY_QueryBBReg(Adapter, rOFDM0_TRxPathEnable, 0xf));
	pHalData->RF_PathMap = PathMap;
	for(index = 0; index<4; index++)
	{
		if((PathMap>>index)&0x1)
			rf_num++;
	}

	if((GET_RF_TYPE(Adapter) ==RF_1T1R && rf_num!=1) ||
		(GET_RF_TYPE(Adapter)==RF_1T2R && rf_num!=2) ||
		(GET_RF_TYPE(Adapter)==RF_2T2R && rf_num!=2) ||
		(GET_RF_TYPE(Adapter)==RF_2T2R_GREEN && rf_num!=2) ||
		(GET_RF_TYPE(Adapter)==RF_2T4R && rf_num!=4))
	{
		RT_TRACE(
			COMP_INIT, 
			DBG_LOUD, 
			("PHY_BBConfig8192C: RF_Type(%x) does not match RF_Num(%x)!!\n", pHalData->RF_Type, rf_num));
	}
#endif

	return rtStatus;
}


extern	int
PHY_RFConfig8192D(
	IN	PADAPTER	Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	int		rtStatus = _SUCCESS;

	if(Adapter->bSurpriseRemoved){
		rtStatus = _FAIL;
		return rtStatus;
	}

	//
	// RF config
	//
	rtStatus = PHY_RF6052_Config8192D(Adapter);
#if 0	
	switch(pHalData->rf_chip)
	{
		case RF_6052:
			rtStatus = PHY_RF6052_Config(Adapter);
			break;
		case RF_8225:
			rtStatus = PHY_RF8225_Config(Adapter);
			break;
		case RF_8256:			
			rtStatus = PHY_RF8256_Config(Adapter);
			break;
		case RF_8258:
			break;
		case RF_PSEUDO_11N:
			rtStatus = PHY_RF8225_Config(Adapter);
			break;
		default: //for MacOs Warning: "RF_TYPE_MIN" not handled in switch
			break;
	}
#endif
	return rtStatus;
}


/*-----------------------------------------------------------------------------
 * Function:    PHY_ConfigRFWithParaFile()
 *
 * Overview:    This function read RF parameters from general file format, and do RF 3-wire
 *
 * Input:      	PADAPTER			Adapter
 *			ps1Byte 				pFileName			
 *			RF90_RADIO_PATH_E	eRFPath
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note:		Delay may be required for RF configuration
 *---------------------------------------------------------------------------*/
int
rtl8192d_PHY_ConfigRFWithParaFile(
	IN	PADAPTER			Adapter,
	IN	u8* 				pFileName,
	RF90_RADIO_PATH_E		eRFPath
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	int	rtStatus = _SUCCESS;


	return rtStatus;
	
}

//****************************************
// The following is for High Power PA
//****************************************
#define HighPowerRadioAArrayLen 22
//This is for High power PA
u32 Rtl8192S_HighPower_RadioA_Array[HighPowerRadioAArrayLen] = {
0x013,0x00029ea4,
0x013,0x00025e74,
0x013,0x00020ea4,
0x013,0x0001ced0,
0x013,0x00019f40,
0x013,0x00014e70,
0x013,0x000106a0,
0x013,0x0000c670,
0x013,0x000082a0,
0x013,0x00004270,
0x013,0x00000240,
};

static int
PHY_ConfigRFExternalPA(
	IN	PADAPTER			Adapter,
	RF90_RADIO_PATH_E		eRFPath
)
{
	int	rtStatus = _SUCCESS;
#if (DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u16 i=0;

	if(!pHalData->ExternalPA)
	{
		return rtStatus;
	}
	
	DBG_8192C("external PA, RF Setting\n");

	//add for SU High Power PA
	for(i = 0;i<HighPowerRadioAArrayLen; i=i+2)
	{
		//RT_TRACE(COMP_INIT, DBG_LOUD, ("External PA, write RF 0x%x=0x%x\n", Rtl8192S_HighPower_RadioA_Array[i], Rtl8192S_HighPower_RadioA_Array[i+1]));
		PHY_SetRFReg(Adapter, eRFPath, Rtl8192S_HighPower_RadioA_Array[i], bRFRegOffsetMask, Rtl8192S_HighPower_RadioA_Array[i+1]);
	}
#endif
	return rtStatus;
}
//****************************************
/*-----------------------------------------------------------------------------
 * Function:    PHY_ConfigRFWithHeaderFile()
 *
 * Overview:    This function read RF parameters from general file format, and do RF 3-wire
 *
 * Input:      	PADAPTER			Adapter
 *			ps1Byte 				pFileName			
 *			RF90_RADIO_PATH_E	eRFPath
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: configuration file exist
 *			
 * Note:		Delay may be required for RF configuration
 *---------------------------------------------------------------------------*/
int
rtl8192d_PHY_ConfigRFWithHeaderFile(
	IN	PADAPTER			Adapter,
	RF_CONTENT				Content,
	RF90_RADIO_PATH_E		eRFPath
)
{
	int	i;
	int	rtStatus = _SUCCESS;
	u32*	Rtl819XRadioA_Array_Table;
	u32*	Rtl819XRadioB_Array_Table;
	u16		RadioA_ArrayLen,RadioB_ArrayLen;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u32	j;

	DBG_8192C(" ===> PHY_ConfigRFWithHeaderFile() intferace = %d, Radio_txt = %d, eRFPath = %d\n", pHalData->interfaceIndex, Content,eRFPath);

	//vivi modify for RF header array, to ID 92d and 92c, 20100528
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		RadioA_ArrayLen = RadioA_2TArrayLength;
		Rtl819XRadioA_Array_Table=Rtl819XRadioA_2TArray;
		RadioB_ArrayLen = RadioB_2TArrayLength;	
		Rtl819XRadioB_Array_Table = Rtl819XRadioB_2TArray;
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> PHY_ConfigRFWithHeaderFile() Radio_A:Rtl819XRadioA_1TArray\n"));
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> PHY_ConfigRFWithHeaderFile() Radio_B:Rtl819XRadioB_1TArray\n"));
	}
	else
	{
		RadioA_ArrayLen = Rtl8192DTestRadioA_2TArrayLength;
		Rtl819XRadioA_Array_Table=Rtl8192DTestRadioA_2TArray;			
		RadioB_ArrayLen = Rtl8192DTestRadioB_2TArrayLength;	
		Rtl819XRadioB_Array_Table = Rtl8192DTestRadioB_2TArray;
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> PHY_ConfigRFWithHeaderFile() Radio_A:Rtl8192CTestRadioA_1T\n"));
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> PHY_ConfigRFWithHeaderFile() Radio_B:Rtl8192CTestRadioB_1T\n"));
	}


	//RT_TRACE(COMP_INIT, DBG_TRACE, ("PHY_ConfigRFWithHeaderFile: Radio No %x\n", eRFPath));
	rtStatus = _SUCCESS;

	//vivi added this for read parameter from header, 20100908
	//1this only happens when DMDP, mac0 start on 2.4G, mac1 start on 5G, 
	//1mac 0 has to set phy0&phy1 pathA or mac1 has to set phy0&phy1 pathA
	if((Content == radiob_txt)&&(eRFPath == RF90_PATH_A))
	{
		//RT_TRACE(COMP_INIT, DBG_LOUD, (" ===> PHY_ConfigRFWithHeaderFile(), althougth Path A, we load radiob.txt\n"));
		if(IS_NORMAL_CHIP(pHalData->VersionID))
		{
			RadioA_ArrayLen = RadioB_2TArrayLength;
			Rtl819XRadioA_Array_Table=Rtl819XRadioB_2TArray;
		}
		else
		{
			RadioA_ArrayLen = Rtl8192DTestRadioB_1TArrayLength;
			Rtl819XRadioA_Array_Table=Rtl8192DTestRadioB_1TArray;
		}
	}

	switch(eRFPath){
		case RF90_PATH_A:
			for(i = 0;i<RadioA_ArrayLen; i=i+2)
			{
				if(Rtl819XRadioA_Array_Table[i] == 0xfe)
				{
					//mdelay_os(50);
					for(j=0;j<1000;j++)
						udelay_os(MAX_STALL_TIME);
				}
				else if (Rtl819XRadioA_Array_Table[i] == 0xfd)
				{
					//mdelay_os(5);
					for(j=0;j<100;j++)
						udelay_os(MAX_STALL_TIME);
				}
				else if (Rtl819XRadioA_Array_Table[i] == 0xfc)
				{
					//mdelay_os(1);
					for(j=0;j<20;j++)
						udelay_os(MAX_STALL_TIME);
				}
				else if (Rtl819XRadioA_Array_Table[i] == 0xfb)
				{
					udelay_os(50);
				}
				else if (Rtl819XRadioA_Array_Table[i] == 0xfa)
				{
					udelay_os(5);
				}
				else if (Rtl819XRadioA_Array_Table[i] == 0xf9)
				{
					udelay_os(1);
				}
				else
				{
					PHY_SetRFReg(Adapter, eRFPath, Rtl819XRadioA_Array_Table[i], bRFRegOffsetMask, Rtl819XRadioA_Array_Table[i+1]);
					// Add 1us delay between BB/RF register setting.
					udelay_os(1);
					//for(j=0;j<20;j++)
					//	udelay_os(MAX_STALL_TIME);
				}
			}			
			//Add for High Power PA
			PHY_ConfigRFExternalPA(Adapter, eRFPath);
			break;
		case RF90_PATH_B:
			for(i = 0;i<RadioB_ArrayLen; i=i+2)
			{
				if(Rtl819XRadioB_Array_Table[i] == 0xfe)
				{ // Deay specific ms. Only RF configuration require delay.												
					//mdelay_os(50);
					for(j=0;j<1000;j++)
						udelay_os(MAX_STALL_TIME);
				}
				else if (Rtl819XRadioB_Array_Table[i] == 0xfd)
				{
					//mdelay_os(5);
					for(j=0;j<100;j++)
						udelay_os(MAX_STALL_TIME);
				}
				else if (Rtl819XRadioB_Array_Table[i] == 0xfc)
				{
					//mdelay_os(1);
					for(j=0;j<20;j++)
						udelay_os(MAX_STALL_TIME);
				}
				else if (Rtl819XRadioB_Array_Table[i] == 0xfb)
				{
					udelay_os(50);
				}
				else if (Rtl819XRadioB_Array_Table[i] == 0xfa)
				{
					udelay_os(5);
				}
				else if (Rtl819XRadioB_Array_Table[i] == 0xf9)
				{
					udelay_os(1);
				}
				else
				{
					PHY_SetRFReg(Adapter, eRFPath, Rtl819XRadioB_Array_Table[i], bRFRegOffsetMask, Rtl819XRadioB_Array_Table[i+1]);
					// Add 1us delay between BB/RF register setting.
					udelay_os(1);
					//for(j=0;j<20;j++)
					//	udelay_os(MAX_STALL_TIME);
				}
			}			
			break;
		case RF90_PATH_C:
			break;
		case RF90_PATH_D:
			break;
	}
	
	return _SUCCESS;

}


/*-----------------------------------------------------------------------------
 * Function:    PHY_CheckBBAndRFOK()
 *
 * Overview:    This function is write register and then readback to make sure whether
 *			  BB[PHY0, PHY1], RF[Patha, path b, path c, path d] is Ok
 *
 * Input:      	PADAPTER			Adapter
 *			HW90_BLOCK_E		CheckBlock
 *			RF90_RADIO_PATH_E	eRFPath		// it is used only when CheckBlock is HW90_BLOCK_RF
 *
 * Output:      NONE
 *
 * Return:      RT_STATUS_SUCCESS: PHY is OK
 *			
 * Note:		This function may be removed in the ASIC
 *---------------------------------------------------------------------------*/
int
rtl8192d_PHY_CheckBBAndRFOK(
	IN	PADAPTER			Adapter,
	IN	HW90_BLOCK_E		CheckBlock,
	IN	RF90_RADIO_PATH_E	eRFPath
	)
{
	int			rtStatus = _SUCCESS;

	u32				i, CheckTimes = 4,ulRegRead;

	u32				WriteAddr[4];
	u32				WriteData[] = {0xfffff027, 0xaa55a02f, 0x00000027, 0x55aa502f};

	// Initialize register address offset to be checked
	WriteAddr[HW90_BLOCK_MAC] = 0x100;
	WriteAddr[HW90_BLOCK_PHY0] = 0x900;
	WriteAddr[HW90_BLOCK_PHY1] = 0x800;
	WriteAddr[HW90_BLOCK_RF] = 0x3;
	
	for(i=0 ; i < CheckTimes ; i++)
	{

		//
		// Write Data to register and readback
		//
		switch(CheckBlock)
		{
		case HW90_BLOCK_MAC:
			//RT_ASSERT(FALSE, ("PHY_CheckBBRFOK(): Never Write 0x100 here!"));
			//RT_TRACE(COMP_INIT, DBG_LOUD, ("PHY_CheckBBRFOK(): Never Write 0x100 here!\n"));
			break;
			
		case HW90_BLOCK_PHY0:
		case HW90_BLOCK_PHY1:
			write32(Adapter, WriteAddr[CheckBlock], WriteData[i]);
			ulRegRead = read32(Adapter, WriteAddr[CheckBlock]);
			break;

		case HW90_BLOCK_RF:
			// When initialization, we want the delay function(delay_ms(), delay_us() 
			// ==> actually we call PlatformStallExecution()) to do NdisStallExecution()
			// [busy wait] instead of NdisMSleep(). So we acquire RT_INITIAL_SPINLOCK 
			// to run at Dispatch level to achive it.	
			//cosa PlatformAcquireSpinLock(Adapter, RT_INITIAL_SPINLOCK);
			WriteData[i] &= 0xfff;
			PHY_SetRFReg(Adapter, eRFPath, WriteAddr[HW90_BLOCK_RF], bRFRegOffsetMask, WriteData[i]);
			// TODO: we should not delay for such a long time. Ask SD3
			mdelay_os(10);
			ulRegRead = PHY_QueryRFReg(Adapter, eRFPath, WriteAddr[HW90_BLOCK_RF], bRFRegOffsetMask);				
			mdelay_os(10);
			//cosa PlatformReleaseSpinLock(Adapter, RT_INITIAL_SPINLOCK);
			break;
			
		default:
			rtStatus = _FAIL;
			break;
		}


		//
		// Check whether readback data is correct
		//
		if(ulRegRead != WriteData[i])
		{
			//RT_TRACE(COMP_FPGA, DBG_LOUD, ("ulRegRead: %lx, WriteData: %lx \n", ulRegRead, WriteData[i]));
			rtStatus = _FAIL;			
			break;
		}
	}

	return rtStatus;
}


VOID
rtl8192d_PHY_GetHWRegOriginalValue(
	IN	PADAPTER		Adapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	
	// read rx initial gain
	pHalData->DefaultInitialGain[0] = (u8)PHY_QueryBBReg(Adapter, rOFDM0_XAAGCCore1, bMaskByte0);
	pHalData->DefaultInitialGain[1] = (u8)PHY_QueryBBReg(Adapter, rOFDM0_XBAGCCore1, bMaskByte0);
	pHalData->DefaultInitialGain[2] = (u8)PHY_QueryBBReg(Adapter, rOFDM0_XCAGCCore1, bMaskByte0);
	pHalData->DefaultInitialGain[3] = (u8)PHY_QueryBBReg(Adapter, rOFDM0_XDAGCCore1, bMaskByte0);
	//RT_TRACE(COMP_INIT, DBG_LOUD,
	//("Default initial gain (c50=0x%x, c58=0x%x, c60=0x%x, c68=0x%x) \n", 
	//pHalData->DefaultInitialGain[0], pHalData->DefaultInitialGain[1], 
	//pHalData->DefaultInitialGain[2], pHalData->DefaultInitialGain[3]));

	// read framesync
	pHalData->framesync = (u8)PHY_QueryBBReg(Adapter, rOFDM0_RxDetector3, bMaskByte0);	 
	pHalData->framesyncC34 = PHY_QueryBBReg(Adapter, rOFDM0_RxDetector2, bMaskDWord);
	//RT_TRACE(COMP_INIT, DBG_LOUD, ("Default framesync (0x%x) = 0x%x \n", 
	//	rOFDM0_RxDetector3, pHalData->framesync));
}


//
//	Description:
//		Map dBm into Tx power index according to 
//		current HW model, for example, RF and PA, and
//		current wireless mode.
//	By Bruce, 2008-01-29.
//
static	u8
phy_DbmToTxPwrIdx(
	IN	PADAPTER		Adapter,
	IN	WIRELESS_MODE	WirelessMode,
	IN	int			PowerInDbm
	)
{
	u8				TxPwrIdx = 0;
	int				Offset = 0;
	

	//
	// Tested by MP, we found that CCK Index 0 equals to 8dbm, OFDM legacy equals to 
	// 3dbm, and OFDM HT equals to 0dbm repectively.
	// Note:
	//	The mapping may be different by different NICs. Do not use this formula for what needs accurate result.  
	// By Bruce, 2008-01-29.
	// 
	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = -7;
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		Offset = -8;
		break;

	default:
		break;
	}

	if((PowerInDbm - Offset) > 0)
	{
		TxPwrIdx = (u8)((PowerInDbm - Offset) * 2);
	}
	else
	{
		TxPwrIdx = 0;
	}

	// Tx Power Index is too large.
	if(TxPwrIdx > MAX_TXPWR_IDX_NMODE_92S)
		TxPwrIdx = MAX_TXPWR_IDX_NMODE_92S;

	return TxPwrIdx;
}


//
//	Description:
//		Map Tx power index into dBm according to 
//		current HW model, for example, RF and PA, and
//		current wireless mode.
//	By Bruce, 2008-01-29.
//
static int
phy_TxPwrIdxToDbm(
	IN	PADAPTER		Adapter,
	IN	WIRELESS_MODE	WirelessMode,
	IN	u8			TxPwrIdx
	)
{
	int				Offset = 0;
	int				PwrOutDbm = 0;
	
	//
	// Tested by MP, we found that CCK Index 0 equals to -7dbm, OFDM legacy equals to -8dbm.
	// Note:
	//	The mapping may be different by different NICs. Do not use this formula for what needs accurate result.  
	// By Bruce, 2008-01-29.
	// 
	switch(WirelessMode)
	{
	case WIRELESS_MODE_B:
		Offset = -7;
		break;

	case WIRELESS_MODE_G:
	case WIRELESS_MODE_N_24G:
		Offset = -8;
		break;

	default:
		break;
	}

	PwrOutDbm = TxPwrIdx / 2 + Offset; // Discard the decimal part.

	return PwrOutDbm;
}


/*-----------------------------------------------------------------------------
 * Function:    GetTxPowerLevel8190()
 *
 * Overview:    This function is export to "common" moudule
 *
 * Input:       PADAPTER		Adapter
 *			psByte			Power Level
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 *---------------------------------------------------------------------------*/
VOID
PHY_GetTxPowerLevel8192D(
	IN	PADAPTER		Adapter,
	OUT u32*    		powerlevel
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			TxPwrLevel = 0;
	int			TxPwrDbm;
	
	//
	// Because the Tx power indexes are different, we report the maximum of them to 
	// meet the CCX TPC request. By Bruce, 2008-01-31.
	//

	// CCK
	TxPwrLevel = pHalData->CurrentCckTxPwrIdx;
	TxPwrDbm = phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_B, TxPwrLevel);

	// Legacy OFDM
	TxPwrLevel = pHalData->CurrentOfdm24GTxPwrIdx + pHalData->LegacyHTTxPowerDiff;

	// Compare with Legacy OFDM Tx power.
	if(phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_G, TxPwrLevel);

	// HT OFDM
	TxPwrLevel = pHalData->CurrentOfdm24GTxPwrIdx;
	
	// Compare with HT OFDM Tx power.
	if(phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_N_24G, TxPwrLevel) > TxPwrDbm)
		TxPwrDbm = phy_TxPwrIdxToDbm(Adapter, WIRELESS_MODE_N_24G, TxPwrLevel);

	*powerlevel = TxPwrDbm;
}


static void getTxPowerIndex(
	IN	PADAPTER		Adapter,
	IN	u8			channel,
	IN OUT u8*		cckPowerLevel,
	IN OUT u8*		ofdmPowerLevel
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	index = (channel -1);

	// 1. CCK
	if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
	{
		cckPowerLevel[RF90_PATH_A] = pHalData->TxPwrLevelCck[RF90_PATH_A][index];	//RF-A
		cckPowerLevel[RF90_PATH_B] = pHalData->TxPwrLevelCck[RF90_PATH_B][index];	//RF-B
	}
	else
		cckPowerLevel[RF90_PATH_A] = cckPowerLevel[RF90_PATH_B] = 0;

	// 2. OFDM for 1S or 2S
	if (GET_RF_TYPE(Adapter) == RF_1T2R || GET_RF_TYPE(Adapter) == RF_1T1R)
	{
		// Read HT 40 OFDM TX power
		ofdmPowerLevel[RF90_PATH_A] = pHalData->TxPwrLevelHT40_1S[RF90_PATH_A][index];
		ofdmPowerLevel[RF90_PATH_B] = pHalData->TxPwrLevelHT40_1S[RF90_PATH_B][index];
	}
	else if (GET_RF_TYPE(Adapter) == RF_2T2R)
	{
		// Read HT 40 OFDM TX power
		ofdmPowerLevel[RF90_PATH_A] = pHalData->TxPwrLevelHT40_2S[RF90_PATH_A][index];
		ofdmPowerLevel[RF90_PATH_B] = pHalData->TxPwrLevelHT40_2S[RF90_PATH_B][index];
	}
	//RTPRINT(FPHY, PHY_TXPWR, ("Channel-%d, set tx power index !!\n", channel));
}

static void ccxPowerIndexCheck(
	IN	PADAPTER		Adapter,
	IN	u8			channel,
	IN OUT u8*		cckPowerLevel,
	IN OUT u8*		ofdmPowerLevel
	)
{
#if 0
	PMGNT_INFO			pMgntInfo = &(Adapter->MgntInfo);
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	PRT_CCX_INFO		pCcxInfo = GET_CCX_INFO(pMgntInfo);

	//
	// CCX 2 S31, AP control of client transmit power:
	// 1. We shall not exceed Cell Power Limit as possible as we can.
	// 2. Tolerance is +/- 5dB.
	// 3. 802.11h Power Contraint takes higher precedence over CCX Cell Power Limit.
	// 
	// TODO: 
	// 1. 802.11h power contraint 
	//
	// 071011, by rcnjko.
	//
	if(	pMgntInfo->OpMode == RT_OP_MODE_INFRASTRUCTURE && 
		pMgntInfo->mAssoc &&
		pCcxInfo->bUpdateCcxPwr &&
		pCcxInfo->bWithCcxCellPwr &&
		channel == pMgntInfo->dot11CurrentChannelNumber)
	{
		u1Byte	CckCellPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_B, pCcxInfo->CcxCellPwr);
		u1Byte	LegacyOfdmCellPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_G, pCcxInfo->CcxCellPwr);
		u1Byte	OfdmCellPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_N_24G, pCcxInfo->CcxCellPwr);

		RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("CCX Cell Limit: %d dbm => CCK Tx power index : %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n", 
		pCcxInfo->CcxCellPwr, CckCellPwrIdx, LegacyOfdmCellPwrIdx, OfdmCellPwrIdx));
		RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("EEPROM channel(%d) => CCK Tx power index: %d, Legacy OFDM Tx power index : %d, OFDM Tx power index: %d\n",
		channel, cckPowerLevel[0], ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff, ofdmPowerLevel[0])); 

		// CCK
		if(cckPowerLevel[0] > CckCellPwrIdx)
			cckPowerLevel[0] = CckCellPwrIdx;
		// Legacy OFDM, HT OFDM
		if(ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff > LegacyOfdmCellPwrIdx)
		{
			if((OfdmCellPwrIdx - pHalData->LegacyHTTxPowerDiff) > 0)
			{
				ofdmPowerLevel[0] = OfdmCellPwrIdx - pHalData->LegacyHTTxPowerDiff;
			}
			else
			{
				ofdmPowerLevel[0] = 0;
			}
		}

		RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("Altered CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		cckPowerLevel[0], ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff, ofdmPowerLevel[0]));
	}

	pHalData->CurrentCckTxPwrIdx = cckPowerLevel[0];
	pHalData->CurrentOfdm24GTxPwrIdx = ofdmPowerLevel[0];

	RT_TRACE(COMP_TXAGC, DBG_LOUD, 
		("PHY_SetTxPowerLevel8192S(): CCK Tx power index : %d, Legacy OFDM Tx power index: %d, OFDM Tx power index: %d\n", 
		cckPowerLevel[0], ofdmPowerLevel[0] + pHalData->LegacyHTTxPowerDiff, ofdmPowerLevel[0]));
#endif	
}
/*-----------------------------------------------------------------------------
 * Function:    SetTxPowerLevel8190()
 *
 * Overview:    This function is export to "HalCommon" moudule
 *			We must consider RF path later!!!!!!!
 *
 * Input:       PADAPTER		Adapter
 *			u1Byte		channel
 *
 * Output:      NONE
 *
 * Return:      NONE
 *	2008/11/04	MHC		We remove EEPROM_93C56.
 *						We need to move CCX relative code to independet file.
 *	2009/01/21	MHC		Support new EEPROM format from SD3 requirement.
 *
 *---------------------------------------------------------------------------*/
VOID
PHY_SetTxPowerLevel8192D(
	IN	PADAPTER		Adapter,
	IN	u8			channel
	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u8	cckPowerLevel[2], ofdmPowerLevel[2];	// [0]:RF-A, [1]:RF-B

#if(MP_DRIVER == 1)
	return;
#endif

#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
	if((check_fwstate(&(Adapter->mlmepriv), _FW_UNDER_SURVEY))&&(Adapter->dvobjpriv.ishighspeed == _FALSE))
		return;
#endif

	if(pHalData->bTXPowerDataReadFromEEPORM == _FALSE)
		return;

#if TX_POWER_FOR_5G_BAND == 1
	channel = GetRightChnlPlace(channel);
#endif

	getTxPowerIndex(Adapter, channel, &cckPowerLevel[0], &ofdmPowerLevel[0]);
	//printk("Channel-%d, cckPowerLevel (A / B) = 0x%x / 0x%x,   ofdmPowerLevel (A / B) = 0x%x / 0x%x\n", 
	//	channel, cckPowerLevel[0], cckPowerLevel[1], ofdmPowerLevel[0], ofdmPowerLevel[1]);

	if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
		ccxPowerIndexCheck(Adapter, channel, &cckPowerLevel[0], &ofdmPowerLevel[0]);

	if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
		rtl8192d_PHY_RF6052SetCckTxPower(Adapter, &cckPowerLevel[0]);
	rtl8192d_PHY_RF6052SetOFDMTxPower(Adapter, &ofdmPowerLevel[0], channel);

#if 0
	switch(pHalData->rf_chip)
	{
		case RF_8225:
			PHY_SetRF8225CckTxPower(Adapter, cckPowerLevel[0]);
			PHY_SetRF8225OfdmTxPower(Adapter, ofdmPowerLevel[0]);
		break;

		case RF_8256:
			PHY_SetRF8256CCKTxPower(Adapter, cckPowerLevel[0]);
			PHY_SetRF8256OFDMTxPower(Adapter, ofdmPowerLevel[0]);
			break;

		case RF_6052:
			PHY_RF6052SetCckTxPower(Adapter, &cckPowerLevel[0]);
			PHY_RF6052SetOFDMTxPower(Adapter, &ofdmPowerLevel[0], channel);
			break;

		case RF_8258:
			break;
	}
#endif

}


//
//	Description:
//		Update transmit power level of all channel supported.
//
//	TODO: 
//		A mode.
//	By Bruce, 2008-02-04.
//
BOOLEAN
PHY_UpdateTxPowerDbm8192D(
	IN	PADAPTER	Adapter,
	IN	int		powerInDbm
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	idx;
	u8	rf_path;

	// TODO: A mode Tx power.
	u8	CckTxPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_B, powerInDbm);
	u8	OfdmTxPwrIdx = phy_DbmToTxPwrIdx(Adapter, WIRELESS_MODE_N_24G, powerInDbm);

	if(OfdmTxPwrIdx - pHalData->LegacyHTTxPowerDiff > 0)
		OfdmTxPwrIdx -= pHalData->LegacyHTTxPowerDiff;
	else
		OfdmTxPwrIdx = 0;

	//RT_TRACE(COMP_TXAGC, DBG_LOUD, ("PHY_UpdateTxPowerDbm8192S(): %ld dBm , CckTxPwrIdx = %d, OfdmTxPwrIdx = %d\n", powerInDbm, CckTxPwrIdx, OfdmTxPwrIdx));

	for(idx = 0; idx < CHANNEL_MAX_NUMBER; idx++)
	{
		for (rf_path = 0; rf_path < 2; rf_path++)
		{
			if(idx < CHANNEL_MAX_NUMBER_2G)
				pHalData->TxPwrLevelCck[rf_path][idx] = CckTxPwrIdx;
			pHalData->TxPwrLevelHT40_1S[rf_path][idx] = 
			pHalData->TxPwrLevelHT40_2S[rf_path][idx] = OfdmTxPwrIdx;
		}
	}

	//Adapter->HalFunc.SetTxPowerLevelHandler(Adapter, pHalData->CurrentChannel);//gtest:todo

	return _TRUE;	
}


/*
	Description:
		When beacon interval is changed, the values of the 
		hw registers should be modified.
	By tynli, 2008.10.24.

*/


void	
rtl8192d_PHY_SetBeaconHwReg(	
	IN	PADAPTER		Adapter,
	IN	u16			BeaconInterval	
	)
{

}


VOID 
PHY_ScanOperationBackup8192D(
	IN	PADAPTER	Adapter,
	IN	u8		Operation
	)
{
#if 0
	IO_TYPE	IoType;
	
	if(!Adapter->bDriverStopped)
	{
		switch(Operation)
		{
			case SCAN_OPT_BACKUP:
				IoType = IO_CMD_PAUSE_DM_BY_SCAN;
				Adapter->HalFunc.SetHwRegHandler(Adapter,HW_VAR_IO_CMD,  (pu1Byte)&IoType);

				break;

			case SCAN_OPT_RESTORE:
				IoType = IO_CMD_RESUME_DM_BY_SCAN;
				Adapter->HalFunc.SetHwRegHandler(Adapter,HW_VAR_IO_CMD,  (pu1Byte)&IoType);
				break;

			default:
				RT_TRACE(COMP_SCAN, DBG_LOUD, ("Unknown Scan Backup Operation. \n"));
				break;
		}
	}
#endif	
}

/*-----------------------------------------------------------------------------
 * Function:    PHY_SetBWModeCallback8192C()
 *
 * Overview:    Timer callback function for SetSetBWMode
 *
 * Input:       	PRT_TIMER		pTimer
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Note:		(1) We do not take j mode into consideration now
 *			(2) Will two workitem of "switch channel" and "switch channel bandwidth" run
 *			     concurrently?
 *---------------------------------------------------------------------------*/
static VOID
_PHY_SetBWMode92D(
	IN	PADAPTER	Adapter
)
{
//	PADAPTER			Adapter = (PADAPTER)pTimer->Adapter;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u8				regBwOpMode;
	u8				regRRSR_RSC;

	//return;

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//u4Byte				NowL, NowH;
	//u8Byte				BeginTime, EndTime; 

	/*RT_TRACE(COMP_SCAN, DBG_LOUD, ("==>PHY_SetBWModeCallback8192C()  Switch to %s bandwidth\n", \
					pHalData->CurrentChannelBW == HT_CHANNEL_WIDTH_20?"20MHz":"40MHz"))*/

	if(pHalData->rf_chip == RF_PSEUDO_11N)
	{
		//pHalData->SetBWModeInProgress= _FALSE;
		return;
	}

	// There is no 40MHz mode in RF_8225.
	if(pHalData->rf_chip==RF_8225)
		return;

	if(Adapter->bDriverStopped)
		return;

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = PlatformEFIORead4Byte(Adapter, TSFR);
	//NowH = PlatformEFIORead4Byte(Adapter, TSFR+4);
	//BeginTime = ((u8Byte)NowH << 32) + NowL;
		
	//3//
	//3//<1>Set MAC register
	//3//
	//Adapter->HalFunc.SetBWModeHandler();
	
	regBwOpMode = read8(Adapter, REG_BWOPMODE);
	regRRSR_RSC = read8(Adapter, REG_RRSR+2);
	//regBwOpMode = Adapter->HalFunc.GetHwRegHandler(Adapter,HW_VAR_BWMODE,(pu1Byte)&regBwOpMode);
	
	switch(pHalData->CurrentChannelBW)
	{
		case HT_CHANNEL_WIDTH_20:
			regBwOpMode |= BW_OPMODE_20MHZ;
			   // 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			write8(Adapter, REG_BWOPMODE, regBwOpMode);
			break;
			   
		case HT_CHANNEL_WIDTH_40:
			regBwOpMode &= ~BW_OPMODE_20MHZ;
				// 2007/02/07 Mark by Emily becasue we have not verify whether this register works
			write8(Adapter, REG_BWOPMODE, regBwOpMode);

			regRRSR_RSC = (regRRSR_RSC&0x90) |(pHalData->nCur40MhzPrimeSC<<5);
			write8(Adapter, REG_RRSR+2, regRRSR_RSC);
			break;

		default:
			/*RT_TRACE(COMP_DBG, DBG_LOUD, ("PHY_SetBWModeCallback8192C():
						unknown Bandwidth: %#X\n",pHalData->CurrentChannelBW));*/
			break;
	}
	
	//3//
	//3//<2>Set PHY related register
	//3//
	switch(pHalData->CurrentChannelBW)
	{
		/* 20 MHz channel*/
		case HT_CHANNEL_WIDTH_20:
			PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bRFMOD, 0x0);
			PHY_SetBBReg(Adapter, rFPGA1_RFMOD, bRFMOD, 0x0);

			if(IS_NORMAL_CHIP(pHalData->VersionID)){
				PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10|BIT11, 3);// SET BIT10 BIT11  for receive cck
			}
			else
			{
				if(pHalData->MacPhyMode92D==DUALMAC_DUALPHY)
			  	{//In DMDP ,bit10/bit11 control phy0 phy1 seperately
				  	if(pHalData->interfaceIndex==0)
				  	{
				  		PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10, 1);
					}
					else
					{
						if(IS_NORMAL_CHIP(pHalData->VersionID))
						{
							PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT11, 1);
						}
						else
						{
#if(DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE)
							RT_TRACE(COMP_INIT,DBG_LOUD,("Test chip, MAC1 Use DBI update 0x884!"));
							MpWritePCIDwordDBI8192C(Adapter, 
													rFPGA0_AnalogParameter2, 
													MpReadPCIDwordDBI8192C(Adapter, rFPGA0_AnalogParameter2, BIT3)|BIT11,
													BIT3);
#elif (DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
							PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT11, 1);
#endif
						}
					}
			  	}
			  	else
			  	{
					PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10|BIT11, 3);// SET BIT10 BIT11  for receive cck
				}
			}

			break;

		/* 40 MHz channel*/
		case HT_CHANNEL_WIDTH_40:
			PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bRFMOD, 0x1);
			PHY_SetBBReg(Adapter, rFPGA1_RFMOD, bRFMOD, 0x1);
			
			// Set Control channel to upper or lower. These settings are required only for 40MHz
			PHY_SetBBReg(Adapter, rCCK0_System, bCCKSideBand, (pHalData->nCur40MhzPrimeSC>>1));
			PHY_SetBBReg(Adapter, rOFDM1_LSTF, 0xC00, pHalData->nCur40MhzPrimeSC);

			if(IS_NORMAL_CHIP(pHalData->VersionID)){
				PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10|BIT11, 0);// SET BIT10 BIT11  for receive cck
			}
			else
			{
				  if(pHalData->MacPhyMode92D==DUALMAC_DUALPHY)
				  {//In DMDP ,bit10/bit11 control phy0 phy1 seperately
				  	if(pHalData->interfaceIndex==0)
				  	{
				  			PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10, 0);
					}
					else
					{
						if(IS_NORMAL_CHIP(pHalData->VersionID))
						{
								PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT11, 0);
						}
						else
						{
#if(DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE)
							//MAC1 Read or Write  MAC0 reg. 0x352 BIT3: 1 acess reg. 0 pci config spcace
												//BIT2: 0: MAC0,1 MAC1 
							MpWritePCIDwordDBI8192C(Adapter, 
													rFPGA0_AnalogParameter2, 
													MpReadPCIDwordDBI8192C(Adapter, rFPGA0_AnalogParameter2, BIT3)&0xfffff7ff,
													BIT3);
#elif (DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE)
							PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT11, 0);
#endif
						}
					}
				}
				else
				{
					PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10|BIT11, 0);// SET BIT10 BIT11  for receive cck
				}
			}

			PHY_SetBBReg(Adapter, 0x818, (BIT26|BIT27), (pHalData->nCur40MhzPrimeSC==HAL_PRIME_CHNL_OFFSET_LOWER)?2:1);

			break;

		default:
			/*RT_TRACE(COMP_DBG, DBG_LOUD, ("PHY_SetBWModeCallback8192C(): unknown Bandwidth: %#X\n"\
						,pHalData->CurrentChannelBW));*/
			break;
			
	}
	//Skip over setting of J-mode in BB register here. Default value is "None J mode". Emily 20070315

	// Added it for 20/40 mhz switch time evaluation by guangan 070531
	//NowL = PlatformEFIORead4Byte(Adapter, TSFR);
	//NowH = PlatformEFIORead4Byte(Adapter, TSFR+4);
	//EndTime = ((u8Byte)NowH << 32) + NowL;
	//RT_TRACE(COMP_SCAN, DBG_LOUD, ("SetBWModeCallback8190Pci: time of SetBWMode = %I64d us!\n", (EndTime - BeginTime)));

	//3<3>Set RF related register
	switch(pHalData->rf_chip)
	{
		case RF_8225:		
			//PHY_SetRF8225Bandwidth(Adapter, pHalData->CurrentChannelBW);
			break;	
			
		case RF_8256:
			// Please implement this function in Hal8190PciPhy8256.c
			//PHY_SetRF8256Bandwidth(Adapter, pHalData->CurrentChannelBW);
			break;
			
		case RF_8258:
			// Please implement this function in Hal8190PciPhy8258.c
			// PHY_SetRF8258Bandwidth();
			break;

		case RF_PSEUDO_11N:
			// Do Nothing
			break;
			
		case RF_6052:
			rtl8192d_PHY_RF6052SetBandwidth(Adapter, pHalData->CurrentChannelBW);
			break;	
			
		default:
			//RT_ASSERT(FALSE, ("Unknown RFChipID: %d\n", pHalData->RFChipID));
			break;
	}

	//pHalData->SetBWModeInProgress= FALSE;

	//RT_TRACE(COMP_SCAN, DBG_LOUD, ("<==PHY_SetBWModeCallback8192C() \n" ));
}

 /*-----------------------------------------------------------------------------
 * Function:   SetBWMode8190Pci()
 *
 * Overview:  This function is export to "HalCommon" moudule
 *
 * Input:       	PADAPTER			Adapter
 *			HT_CHANNEL_WIDTH	Bandwidth	//20M or 40M
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Note:		We do not take j mode into consideration now
 *---------------------------------------------------------------------------*/
VOID
PHY_SetBWMode8192D(
	IN	PADAPTER					Adapter,
	IN	HT_CHANNEL_WIDTH	Bandwidth,	// 20M or 40M
	IN	unsigned char	Offset		// Upper, Lower, or Don't care
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	HT_CHANNEL_WIDTH 	tmpBW= pHalData->CurrentChannelBW;
#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	PADAPTER	BuddyAdapter = Adapter->BuddyAdapter;
#endif

	//if(pHalData->SetBWModeInProgress)
	//	return;

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
		if(Adapter->bInModeSwitchProcess)
		{
			RT_TRACE(COMP_DUALMACSWITCH,DBG_LOUD,("PHY_SwChnl8192C(): During mode switch \n"));
			pHalData->SetBWModeInProgress=FALSE;
			return;
		}
#endif

	//pHalData->SetBWModeInProgress= _TRUE;

	pHalData->CurrentChannelBW = Bandwidth;

#if 0
	if(Offset==HT_EXTCHNL_OFFSET_LOWER)
		pHalData->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_UPPER;
	else if(Offset==HT_EXTCHNL_OFFSET_UPPER)
		pHalData->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_LOWER;
	else
		pHalData->nCur40MhzPrimeSC = HAL_PRIME_CHNL_OFFSET_DONT_CARE;
#else
	pHalData->nCur40MhzPrimeSC = Offset;
#endif

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if((BuddyAdapter !=NULL) && (Adapter->bSlaveOfDMSP))
	{
		//if((BuddyAdapter->MgntInfo.bJoinInProgress) ||(BuddyAdapter->MgntInfo.bScanInProgress))
		{
			RT_TRACE(COMP_DUALMACSWITCH,DBG_LOUD,("PHY_SwChnl8192C():slave return when slave \n"));
			pHalData->SetBWModeInProgress=_FALSE;
			return;
		}
	}
#endif

	if((!Adapter->bDriverStopped) && (!Adapter->bSurpriseRemoved))
	{
#ifdef USE_WORKITEM	
		//PlatformScheduleWorkItem(&(pHalData->SetBWModeWorkItem));
#else
	#if 0
		//PlatformSetTimer(Adapter, &(pHalData->SetBWModeTimer), 0);
	#else
		_PHY_SetBWMode92D(Adapter);
	#endif
#endif		
	}
	else
	{
		//RT_TRACE(COMP_SCAN, DBG_LOUD, ("PHY_SetBWMode8192C() SetBWModeInProgress FALSE driver sleep or unload\n"));	
		//pHalData->SetBWModeInProgress= FALSE;	
		pHalData->CurrentChannelBW = tmpBW;
	}
	
}


/*******************************************************************
Descriptor:
			stop TRX Before change bandType dynamically	

********************************************************************/
static VOID 
PHY_StopTRXBeforeChangeBand8192D(
	  PADAPTER		Adapter
)
{
	//u16	retry = 0;	
	//u8	bytetmp;
	//u32	RegValueForRQPN, RegValueForFifoPage;
	//int	i;
#if 0
#if MP_DRIVER == 1
	HAL_DATA_TYPE *pHalData = GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;

	pdmpriv->RegC04_TP = (u8)PHY_QueryBBReg(Adapter, rOFDM0_TRxPathEnable, bMaskByte0);
	pdmpriv->RegD04_TP = PHY_QueryBBReg(Adapter, rOFDM1_TRxPathEnable, bDWord);
#endif
#endif
	PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bCCKEn, 0);
	PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bOFDMEn, 0);
	PHY_SetBBReg(Adapter, rOFDM0_TRxPathEnable, bMaskByte0, 0x00);
	PHY_SetBBReg(Adapter, rOFDM1_TRxPathEnable, bDWord, 0x0);
}

/*

*/
int
PHY_SwitchWirelessBand(
	IN PADAPTER		 Adapter,
	IN u8		Band)
{		
	HAL_DATA_TYPE	*pHalData	= GET_HAL_DATA(Adapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	int 	rtStatus = _FAIL;
	s8	sz92DTest2GAGCTableFile[] = RTL8192D_TEST_AGC_TAB_2G;
	s8	sz92DTest5GAGCTableFile[] = RTL8192D_TEST_AGC_TAB_5G;
	s8	*pszAGCTableFile;
	u8	regBwOpMode,i;
	u32	BBRegValue = 0;
	u32	BitMask = 0;

	DBG_8192C("PHY_SwitchWirelessBand():Before Switch Band \n");

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(Adapter->bInModeSwitchProcess || Adapter->bSlaveOfDMSP)
	{
		RT_TRACE(COMP_DUALMACSWITCH,DBG_LOUD,("PHY_SwitchWirelessBand(): skip for mode switch or slave \n"));
		return rtStatus;
	}
#endif

	pHalData->BandSet92D = pHalData->CurrentBandType92D = Band;	
	if(IS_92D_SINGLEPHY(pHalData->VersionID))
         	pHalData->BandSet92D = BAND_ON_BOTH;
	
#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
	//RT_ASSERT((KeGetCurrentIrql() == PASSIVE_LEVEL),
	//	("MPT_ActSetWirelessMode819x(): not in PASSIVE_LEVEL!\n"));
#endif

	//stop RX/Tx
	PHY_StopTRXBeforeChangeBand8192D(Adapter);

	//reconfig BB/RF according to wireless mode
	if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
	{
		//BB & RF Config
		if(!IS_NORMAL_CHIP(pHalData->VersionID) || (IS_NORMAL_CHIP(pHalData->VersionID) && pHalData->interfaceIndex == 1))
		{
#ifdef CONFIG_EMBEDDED_FWIMG
			rtStatus = phy_ConfigBBWithHeaderFile(Adapter, BaseBand_Config_AGC_TAB);
#else
			rtStatus = PHY_SetAGCTab8192D(Adapter);
#endif
		}
	}
	else	//5G band
	{
		if(!IS_NORMAL_CHIP(pHalData->VersionID) || (IS_NORMAL_CHIP(pHalData->VersionID) && pHalData->interfaceIndex == 1))
		{
#ifdef CONFIG_EMBEDDED_FWIMG
			rtStatus = phy_ConfigBBWithHeaderFile(Adapter, BaseBand_Config_AGC_TAB);
#else
			rtStatus = PHY_SetAGCTab8192D(Adapter);
#endif
		}
	}
		
	PHY_UpdateBBRFConfiguration8192D(Adapter);

	if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
		PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bCCKEn, 0x1);
	PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bOFDMEn, 0x1);

	//20M BW.
	PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter2, BIT10, 1);

	pdmpriv->bReloadtxpowerindex = _TRUE;

	//if(!IS_NORMAL_CHIP(pHalData->VersionID))
	{
		for(i=0;i<20;i++)
			udelay_os(MAX_STALL_TIME);
	}

	DBG_8192C("PHY_SwitchWirelessBand():Switch Band OK.\n");

	return rtStatus;	
}


static VOID
PHY_EnableRFENV(
	IN	PADAPTER		Adapter,	
	IN	u8				eRFPath	,
	OUT	u32*			pu4RegValue
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &pHalData->PHYRegDef[eRFPath];

	//RT_TRACE(COMP_RF, DBG_LOUD, ("====>PHY_EnableRFENV\n"));

	/*----Store original RFENV control type----*/		
	switch(eRFPath)
	{
		case RF90_PATH_A:
		case RF90_PATH_C:
			*pu4RegValue = PHY_QueryBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			*pu4RegValue = PHY_QueryBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV<<16);
			break;
	}	

	/*----Set RF_ENV enable----*/		
	PHY_SetBBReg(Adapter, pPhyReg->rfintfe, bRFSI_RFENV<<16, 0x1);
	udelay_os(1);
	
	/*----Set RF_ENV output high----*/
	PHY_SetBBReg(Adapter, pPhyReg->rfintfo, bRFSI_RFENV, 0x1);
	udelay_os(1);
	
	/* Set bit number of Address and Data for RF register */
	PHY_SetBBReg(Adapter, pPhyReg->rfHSSIPara2, b3WireAddressLength, 0x0);	// Set 1 to 4 bits for 8255
	udelay_os(1);
	
	PHY_SetBBReg(Adapter, pPhyReg->rfHSSIPara2, b3WireDataLength, 0x0); // Set 0 to 12	bits for 8255
	udelay_os(1);

	//RT_TRACE(COMP_RF, DBG_LOUD, ("<====PHY_EnableRFENV\n"));	

}

static VOID
PHY_RestoreRFENV(
	IN	PADAPTER		Adapter,	
	IN	u8				eRFPath,
	IN	u32*			pu4RegValue
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	BB_REGISTER_DEFINITION_T	*pPhyReg = &pHalData->PHYRegDef[eRFPath];

	//RT_TRACE(COMP_RF, DBG_LOUD, ("=====>PHY_RestoreRFENV\n"));

	/*----Restore RFENV control type----*/;
	switch(eRFPath)
	{
		case RF90_PATH_A:
		case RF90_PATH_C:
			PHY_SetBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV, *pu4RegValue);
			break;
		case RF90_PATH_B :
		case RF90_PATH_D:
			PHY_SetBBReg(Adapter, pPhyReg->rfintfs, bRFSI_RFENV<<16, *pu4RegValue);
			break;
	}
	//RT_TRACE(COMP_RF, DBG_LOUD, ("<=====PHY_RestoreRFENV\n"));	

}


/*-----------------------------------------------------------------------------
 * Function:	phy_SwitchRfSetting
 *
 * Overview:	Change RF Setting when we siwthc channel for 92D C-cut.
 *
 * Input:       IN	PADAPTER				pAdapter
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 01/08/2009	MHC		Suggestion from SD3 Willis for 92S series.
 * 01/09/2009	MHC		Add CCK modification for 40MHZ. Suggestion from SD3.
 *
 *---------------------------------------------------------------------------*/
 static	VOID	
 phy_SwitchRfSetting(
 	IN	PADAPTER				Adapter,
	IN	u8					channel 	
 	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u8			path = pHalData->CurrentBandType92D==BAND_ON_5G?RF90_PATH_A:RF90_PATH_B;
	u8			index = 0,	i = 0, eRFPath = RF90_PATH_A;
	BOOLEAN		bNeedPowerDownRadio = _FALSE;
	u32			u4RegValue, mask = 0x1C000, value = 0;

	//RT_TRACE(COMP_CMD, DBG_LOUD, ("====>phy_SwitchRfSetting interface %d\n", pHalData->interfaceIndex));

	//only for 92D C-cut SMSP
#if 1	
	if(!IS_NORMAL_CHIP(pHalData->VersionID))
		return;
#endif

	//config path A for 5G
	if(pHalData->CurrentBandType92D==BAND_ON_5G)
	{
		//RT_TRACE(COMP_CMD, DBG_LOUD, ("====>phy_SwitchRfSetting interface %d 5G\n", pHalData->interfaceIndex));

	
		for(i = 0; i < RF_CHNL_NUM_5G; i++)
		{
			if(channel == RF_CHNL_5G[i] && channel <= 140)
				index = 0;
		}

		for(i = 0; i < RF_CHNL_NUM_5G_40M; i++)
		{
			if(channel == RF_CHNL_5G_40M[i] && channel <= 140)
				index = 1;
		}

		if(channel ==149 || channel == 155 || channel ==161)
			index = 2;
		else if(channel == 151 || channel == 153 || channel == 163 || channel == 165)
			index = 3;
		else if(channel == 157 || channel == 159 )
			index = 4;

		if(pHalData->MacPhyMode92D == DUALMAC_DUALPHY && pHalData->interfaceIndex == 1)
		{
			bNeedPowerDownRadio = rtl8192d_PHY_EnableAnotherPHY(Adapter, _FALSE);
			pHalData->bDuringMac1InitRadioA = _TRUE;
			//asume no this case
			if(bNeedPowerDownRadio)
			 	PHY_EnableRFENV(Adapter, path,  &u4RegValue);
		}

		for(i = 0; i < RF_REG_NUM_for_C_CUT_5G; i++)
		{
			if(i == 0 && (pHalData->MacPhyMode92D == DUALMAC_DUALPHY))
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)path, RF_REG_for_C_CUT_5G[i], bRFRegOffsetMask, 0xE439D);
			else if (RF_REG_for_C_CUT_5G[i] == RF_SYN_G4)
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)path, RF_REG_for_C_CUT_5G[i], 0xFF8FF, RF_REG_Param_for_C_CUT_5G[index][i]);
			else
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)path, RF_REG_for_C_CUT_5G[i], bRFRegOffsetMask, RF_REG_Param_for_C_CUT_5G[index][i]);
//			RT_TRACE(COMP_RF, DBG_LOUD, ("phy_SwitchRfSetting offset 0x%x value 0x%x path %d index %d readback 0x%x\n", 
//				RF_REG_for_C_CUT_5G[i], RF_REG_Param_for_C_CUT_5G[index][i], path,  index,
//				PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E) path, RF_REG_for_C_CUT_5G[i], bRFRegOffsetMask)));
			
		}
		
		if(bNeedPowerDownRadio)
			PHY_RestoreRFENV(Adapter, path, &u4RegValue);

		if(pHalData->bDuringMac1InitRadioA)
			rtl8192d_PHY_PowerDownAnotherPHY(Adapter, _FALSE);

		if(channel < 149)
			value = 0x07;
		else if(channel >= 149)
			value = 0x02;

		for(eRFPath = RF90_PATH_A; eRFPath < pHalData->NumTotalRFPath; eRFPath++)
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x0B, mask, value);
		
	}
	else if(pHalData->CurrentBandType92D==BAND_ON_2_4G)
	{
		//DBG_8192C("====>phy_SwitchRfSetting interface %d 2.4G\n", pHalData->interfaceIndex);
	
		if(channel == 2 || channel ==4 || channel == 9 || channel == 10 || 
			channel == 11 || channel ==12)
			index = 0;
		else if(channel ==3 || channel == 13 || channel == 14)
			index = 1;
		else if(channel >= 5 && channel <= 8)
			index = 2;

		if(pHalData->MacPhyMode92D == DUALMAC_DUALPHY)
		{
			path = RF90_PATH_A;		
			if(pHalData->interfaceIndex == 0)
			{
				bNeedPowerDownRadio = rtl8192d_PHY_EnableAnotherPHY(Adapter, _TRUE);
				pHalData->bDuringMac0InitRadioB = _TRUE;
				if(bNeedPowerDownRadio)
					PHY_EnableRFENV(Adapter, path, &u4RegValue);

			}
		}


		for(i = 0; i < RF_REG_NUM_for_C_CUT_2G; i++)
		{
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)path, RF_REG_for_C_CUT_2G[i], bRFRegOffsetMask, RF_REG_Param_for_C_CUT_2G[index][i]);
//			RT_TRACE(COMP_RF, DBG_LOUD, ("phy_SwitchRfSetting offset 0x%x value 0x%x path %d index %d readback 0x%x\n", 
//				RF_REG_for_C_CUT_2G[i], RF_REG_Param_for_C_CUT_2G[index][i], path,  index,
//				PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E) path, RF_REG_for_C_CUT_2G[i], bRFRegOffsetMask)));
		}
		
		if(bNeedPowerDownRadio)
			PHY_RestoreRFENV(Adapter, path, &u4RegValue);	
		if(pHalData->bDuringMac0InitRadioB)
			rtl8192d_PHY_PowerDownAnotherPHY(Adapter, _TRUE);
	}

	//RT_TRACE(COMP_CMD, DBG_LOUD, ("<====phy_SwitchRfSetting\n"));

}


#if 0
/*-----------------------------------------------------------------------------
 * Function:	phy_ReloadLCKSetting
 *
 * Overview:	Change RF Setting when we siwthc channel for 92D C-cut.
 *
 * Input:       IN	PADAPTER				pAdapter
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 01/08/2009	MHC		Suggestion from SD3 Willis for 92S series.
 * 01/09/2009	MHC		Add CCK modification for 40MHZ. Suggestion from SD3.
 *
 *---------------------------------------------------------------------------*/
static  VOID	
 phy_ReloadLCKSetting(
 	IN	PADAPTER				Adapter,
	IN	u8					channel 	
 	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u8		eRFPath = pHalData->CurrentBandType92D == BAND_ON_5G?RF90_PATH_A:IS_92D_SINGLEPHY(pHalData->VersionID)?RF90_PATH_B:RF90_PATH_A;
	u32 		u4tmp = 0, u4RegValue = 0;
	BOOLEAN		bNeedPowerDownRadio = _FALSE;

	//RT_TRACE(COMP_CMD, DBG_LOUD, ("====>phy_ReloadLCKSetting interface %d path %d\n", Adapter->interfaceIndex, eRFPath));

	//only for 92D C-cut SMSP
#if 1	
	if(!IS_NORMAL_CHIP(pHalData->VersionID))
		return;
#endif

	//RTPRINT(FINIT, INIT_IQK, ("cosa pHalData->CurrentBandType92D = %d\n", pHalData->CurrentBandType92D));
	//RTPRINT(FINIT, INIT_IQK, ("cosa channel = %d\n", channel));
	if(pHalData->CurrentBandType92D == BAND_ON_5G)
	{
		//Path-A for 5G
		{
			u4tmp = CurveIndex_5G[channel-1];
			//RTPRINT(FINIT, INIT_IQK, ("cosa ver 1 set RF-A, 5G,	0x28 = 0x%x !!\n", u4tmp));

			if(pHalData->MacPhyMode92D == DUALMAC_DUALPHY && pHalData->interfaceIndex == 1)
			{
				bNeedPowerDownRadio = rtl8192d_PHY_EnableAnotherPHY(Adapter, _FALSE);
				pHalData->bDuringMac1InitRadioA = _TRUE;
				//asume no this case
				if(bNeedPowerDownRadio)
				 	PHY_EnableRFENV(Adapter, eRFPath,  &u4RegValue);
			}		
			
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, RF_SYN_G4, 0x3f800, u4tmp);			

			if(bNeedPowerDownRadio)
				PHY_RestoreRFENV(Adapter, eRFPath, &u4RegValue);
			if(pHalData->bDuringMac1InitRadioA)
				rtl8192d_PHY_PowerDownAnotherPHY(Adapter, _FALSE);				
		}
	}
	else if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
	{
		{
			u32 u4tmp=0;
			u4tmp = CurveIndex_2G[channel-1];
			//RTPRINT(FINIT, INIT_IQK, ("cosa ver 3 set RF-B, 2G, 0x28 = 0x%x !!\n", u4tmp));

			if(pHalData->MacPhyMode92D == DUALMAC_DUALPHY && pHalData->interfaceIndex == 0)
			{
				bNeedPowerDownRadio = rtl8192d_PHY_EnableAnotherPHY(Adapter, _TRUE);
				pHalData->bDuringMac0InitRadioB = _TRUE;
				if(bNeedPowerDownRadio)
					PHY_EnableRFENV(Adapter, eRFPath, &u4RegValue);							
			}			
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, RF_SYN_G4, 0x3f800, u4tmp);
			//RTPRINT(FINIT, INIT_IQK, ("cosa ver 3 set RF-B, 2G, 0x28 = 0x%x !!\n", PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E) eRFPath, RF_SYN_G4, 0x3f800)));

			if(bNeedPowerDownRadio)
				PHY_RestoreRFENV(Adapter, eRFPath, &u4RegValue);	
			if(pHalData->bDuringMac0InitRadioB)
				rtl8192d_PHY_PowerDownAnotherPHY(Adapter, _TRUE);			
		}
	}



	//RT_TRACE(COMP_CMD, DBG_LOUD, ("<====phy_ReloadLCKSetting\n"));

}

/*-----------------------------------------------------------------------------
 * Function:	phy_ReloadLCKSetting
 *
 * Overview:	Change RF Setting when we siwthc channel for 92D C-cut.
 *
 * Input:       IN	PADAPTER				pAdapter
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 01/08/2009	MHC		Suggestion from SD3 Willis for 92S series.
 * 01/09/2009	MHC		Add CCK modification for 40MHZ. Suggestion from SD3.
 *
 *---------------------------------------------------------------------------*/
 static VOID	
 phy_ReloadIMRSetting(
 	IN	PADAPTER				Adapter,
	IN	u8					channel,
	IN  	u8					eRFPath
 	)
{
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(Adapter);
	u32		IMR_NUM = MAX_RF_IMR_INDEX;
	BOOLEAN		isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	u32	   	RFMask=bRFRegOffsetMask;
	u8	   	group=0,pathindex,mode=0, i;

#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
	if(Adapter->dvobjpriv.ishighspeed == _FALSE)
		return;
#endif	

	//only for 92D C-cut SMSP
#if 1	
	if(!IS_NORMAL_CHIP(pHalData->VersionID))
		return;
#endif

	//RT_TRACE(COMP_CMD, DBG_LOUD, ("====>phy_ReloadIMRSetting interface %d path %d\n", pHalData->interfaceIndex, eRFPath));

	if(pHalData->CurrentBandType92D == BAND_ON_5G)
	{
		
		PHY_SetBBReg(Adapter, rFPGA0_RFMOD, BIT25|BIT24, 0);			
		PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, 0x00f00000,	0xf);
	
		// fc area 0xd2c
		if(channel>99)
			PHY_SetBBReg(Adapter, rOFDM1_CFOTracking, BIT13|BIT14,2);	
		else
			PHY_SetBBReg(Adapter, rOFDM1_CFOTracking, BIT13|BIT14,1);
		
		if(isNormal)
		{
			group = channel<=64?1:2; //leave 0 for channel1-14.
			IMR_NUM = MAX_RF_IMR_INDEX_NORMAL;
		}
		else
		{
			group = channel<=64?0:(channel<=114)?1:(channel<=140)?2:3;
		}
		
		for(i=0; i<IMR_NUM; i++){
			if(!isNormal)
			{
				if(RF_REG_FOR_5G_SWCHNL[i] == 0x38){
					//Just add to fix rx temporarily.
					PHY_SetRFReg(Adapter, eRFPath, 0x38, BIT16, 1);
					RFMask = 0xfff;
				}
				if(pHalData->MacPhyMode92D==DUALMAC_DUALPHY){
					pathindex = (u8)pHalData->interfaceIndex*4;
				}
				else{
					mode = 1;
					pathindex = eRFPath*4;
				}
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, RF_REG_FOR_5G_SWCHNL[i], RFMask,RF_IMR_Param[mode][pathindex+group][i]);
			}
			else
			{
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, RF_REG_FOR_5G_SWCHNL_NORMAL[i], RFMask,RF_IMR_Param_Normal[0][group][i]); 						
			}
		}				
		PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, 0x00f00000,0);
		PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bOFDMEn, 1);
	}
	else{ //G band.
		if(isNormal)
		{ // just for normal chip.
			//RT_TRACE(COMP_SCAN,DBG_LOUD,("Load RF IMR parameters for G band. IMR already setting %d \n",pMgntInfo->bLoadIMRandIQKSettingFor2G));

			if(!pHalData->bLoadIMRandIQKSettingFor2G){
				//RT_TRACE(COMP_SCAN,DBG_LOUD,("Load RF IMR parameters for G band. %d \n",eRFPath));
				PHY_SetBBReg(Adapter, rFPGA0_RFMOD, BIT25|BIT24, 0);			
				PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, 0x00f00000,	0xf);
	
				IMR_NUM = MAX_RF_IMR_INDEX_NORMAL;
				for(i=0; i<IMR_NUM; i++){
					PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, RF_REG_FOR_5G_SWCHNL_NORMAL[i], bRFRegOffsetMask,RF_IMR_Param_Normal[0][0][i]);							
				}
				PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, 0x00f00000,0);
				PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bOFDMEn|bCCKEn, 3);
			}
		  }
	}
	
	//RT_TRACE(COMP_CMD, DBG_LOUD, ("<====phy_ReloadIMRSetting\n"));

}	

/*-----------------------------------------------------------------------------
 * Function:	phy_ReloadLCKSetting
 *
 * Overview:	Change RF Setting when we siwthc channel for 92D C-cut.
 *
 * Input:       IN	PADAPTER				pAdapter
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 01/08/2009	MHC		Suggestion from SD3 Willis for 92S series.
 * 01/09/2009	MHC		Add CCK modification for 40MHZ. Suggestion from SD3.
 *
 *---------------------------------------------------------------------------*/
 static VOID	
 phy_ReloadIQKSetting(
 	IN	PADAPTER				Adapter,
	IN	u8					channel
 	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv	*pmlmepriv = &Adapter->mlmepriv;
	BOOLEAN		isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	u8	   	index, Indexforchannel;


	//only for 92D C-cut SMSP
#if 1	
	if(!IS_NORMAL_CHIP(pHalData->VersionID))
		return;
#endif

#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
	if(Adapter->dvobjpriv.ishighspeed == _FALSE)
		return;
#endif

	//RT_TRACE(COMP_CMD, DBG_LOUD, ("====>phy_ReloadIQKSetting interface %d channel %d \n", Adapter->interfaceIndex, channel));

	//---------Do IQK for normal chip and test chip 5G band----------------
	if(isNormal||((!isNormal)&&(pHalData->CurrentBandType92D==BAND_ON_5G||pHalData->BandSet92D == BAND_ON_BOTH)))
	{
		Indexforchannel = rtl8192d_GetRightChnlPlaceforIQK(channel);

		//RT_TRACE(COMP_CMD, DBG_LOUD, ("====>Indexforchannel %d done %d\n", Indexforchannel, pHalData->IQKMatrixRegSetting[Indexforchannel].bIQKDone));

		
#if MP_DRIVER == 1
		pHalData->bNeedIQK = _TRUE;
		pHalData->bLoadIMRandIQKSettingFor2G = _FALSE;
#endif
		
		if(pHalData->bNeedIQK && !pHalData->IQKMatrixRegSetting[Indexforchannel].bIQKDone)
		{ //Re Do IQK.
			DBG_8192C("Do IQK Matrix reg for channel:%d....\n", channel);
			rtl8192d_PHY_IQCalibrate(Adapter);
		}
		else //Just load the value.
		{
			// 2G band just load once.
			if(((!pHalData->bLoadIMRandIQKSettingFor2G) && Indexforchannel==0) ||Indexforchannel>0)
			{
				//DBG_8192C("Just Read IQK Matrix reg for channel:%d....\n", channel);

				for(index=0;index< 9;index++)
				{
					PHY_SetBBReg(Adapter, pHalData->IQKMatrixReg[index],
						pHalData->IQKMatrixRegSetting[Indexforchannel].Mark[index], 
						pHalData->IQKMatrixRegSetting[Indexforchannel].Value[index]);
					//RT_TRACE(COMP_SCAN,DBG_LOUD,
					//	(" 0x%03X =%08X ",pHalData->IQKMatrixReg[index],PHY_QueryBBReg(Adapter, pHalData->IQKMatrixReg[index],bMaskDWord)));
					//RT_TRACE(COMP_SCAN|COMP_MLME,DBG_LOUD,
					//	(" 0x%03X(mark:%08X)=%08X ",pHalData->IQKMatrixReg[index],pHalData->IQKMatrixRegSetting[Indexforchannel].Mark[index],pHalData->IQKMatrixRegSetting[Indexforchannel].Value[index])); 					}
				}
				if((check_fwstate(pmlmepriv, _FW_UNDER_SURVEY)== _TRUE)&&(Indexforchannel==0))
					pHalData->bLoadIMRandIQKSettingFor2G=_TRUE;
			}				
		}					
		pHalData->bNeedIQK = _FALSE;
	}
	
	//RT_TRACE(COMP_CMD, DBG_LOUD, ("<====phy_ReloadIQKSetting\n"));

}
#endif


static void _PHY_SwChnl8192D(PADAPTER Adapter, u8 channel)
{
	u8	eRFPath, i = 0;
	u32	param1, param2;
	u32	ret_value;
	BAND_TYPE	bandtype, target_bandtype;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	struct mlme_priv	*pmlmepriv = &Adapter->mlmepriv;
	u8			Indexforchannel=0,index;
	BOOLEAN		is2T = IS_92D_SINGLEPHY(pHalData->VersionID);
	BOOLEAN		isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	u32			IMR_NUM = MAX_RF_IMR_INDEX;


	if(pHalData->BandSet92D == BAND_ON_BOTH){
		// Need change band?
		// BB {Reg878[0],[16]} bit0= 1 is 5G, bit0=0 is 2G.
		ret_value = PHY_QueryBBReg(Adapter, rFPGA0_XAB_RFParameter, bMaskDWord);

		if(ret_value & BIT0)
			bandtype = BAND_ON_5G;
		else
			bandtype = BAND_ON_2_4G;

		// Use current channel to judge Band Type and switch Band if need.
		if(channel > 14)
		{
			target_bandtype = BAND_ON_5G;
		}
		else
		{
			target_bandtype = BAND_ON_2_4G;
		}

		if(target_bandtype != bandtype)
			PHY_SwitchWirelessBand(Adapter,target_bandtype);
	}

	do{
		//s1. pre common command - CmdID_SetTxPowerLevel
		PHY_SetTxPowerLevel8192D(Adapter, channel);

		//s2. RF dependent command - CmdID_RF_WriteReg, param1=RF_CHNLBW, param2=channel
		param1 = RF_CHNLBW;
		param2 = channel;
		for(eRFPath = 0; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
		{
#if 1
			pHalData->RfRegChnlVal[eRFPath] = PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, RF_CHNLBW, bRFRegOffsetMask);
			// & 0xFFFFFC00 just for 2.4G. So for 5G band,bit[9:8]= 1. we change this setting for 5G. by wl
			pHalData->RfRegChnlVal[eRFPath] = ((pHalData->RfRegChnlVal[eRFPath] & 0xffffff00) | param2);
			if(pHalData->CurrentBandType92D == BAND_ON_5G)
			{
				if(param2>99)
				{
					pHalData->RfRegChnlVal[eRFPath]=pHalData->RfRegChnlVal[eRFPath]|(BIT18);
				}
				else
				{
					pHalData->RfRegChnlVal[eRFPath]=pHalData->RfRegChnlVal[eRFPath]&(~BIT18);
				}
				//PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, param1, bRFRegOffsetMask, pHalData->RfRegChnlVal[eRFPath]);
			}
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, param1, bRFRegOffsetMask, pHalData->RfRegChnlVal[eRFPath]);
#else
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, param1, bRFRegOffsetMask, param2);
#endif
#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
			if(Adapter->dvobjpriv.ishighspeed == _FALSE)
				continue;
#endif
			if(pHalData->CurrentBandType92D == BAND_ON_5G)
			{
				u32     RFMask=bRFRegOffsetMask;
				u8	   group=0,pathindex,mode=0;
				
				PHY_SetBBReg(Adapter, rFPGA0_RFMOD, BIT25|BIT24, 0);			
				PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, 0x00f00000,  0xf);

				// fc area 0xd2c
				if(param2>99)
					PHY_SetBBReg(Adapter, rOFDM1_CFOTracking, BIT13|BIT14,2);	
				else
					PHY_SetBBReg(Adapter, rOFDM1_CFOTracking, BIT13|BIT14,1);
				
				if(isNormal)
				{
					group = param2<=64?1:2; //leave 0 for channel1-14.
					IMR_NUM = MAX_RF_IMR_INDEX_NORMAL;
				}
				else
				{
					group = param2<=64?0:(param2<=114)?1:(param2<=140)?2:3;
				}
				
				for(i=0; i<IMR_NUM; i++){
					if(!isNormal)
					{
						if(RF_REG_FOR_5G_SWCHNL[i] == 0x38){
							//Just add to fix rx temporarily.
							PHY_SetRFReg(Adapter, eRFPath, 0x38, BIT16, 1);
							RFMask = 0xfff;
						}
						if(pHalData->MacPhyMode92D==DUALMAC_DUALPHY){
							pathindex = (u8)pHalData->interfaceIndex*4;
						}
						else{
							mode = 1;
							pathindex = eRFPath*4;
						}
						PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, RF_REG_FOR_5G_SWCHNL[i], RFMask,RF_IMR_Param[mode][pathindex+group][i]);
					}
					else
					{
						PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, RF_REG_FOR_5G_SWCHNL_NORMAL[i], RFMask,RF_IMR_Param_Normal[0][group][i]);							
					}
				}				
				PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, 0x00f00000,0);
				PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bOFDMEn, 1);
			}
			else{ //G band.
				if(isNormal)
				{ // just for normal chip.
					//RT_TRACE(COMP_SCAN,DBG_LOUD,("Load RF IMR parameters for G band. IMR already setting %d \n",pMgntInfo->bLoadIMRandIQKSettingFor2G));
				
					if(!pHalData->bLoadIMRandIQKSettingFor2G){
						//RT_TRACE(COMP_SCAN,DBG_LOUD,("Load RF IMR parameters for G band. %d \n",eRFPath));
						PHY_SetBBReg(Adapter, rFPGA0_RFMOD, BIT25|BIT24, 0);			
						PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, 0x00f00000,  0xf);

						IMR_NUM = MAX_RF_IMR_INDEX_NORMAL;
						for(i=0; i<IMR_NUM; i++){
							PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, RF_REG_FOR_5G_SWCHNL_NORMAL[i], bRFRegOffsetMask,RF_IMR_Param_Normal[0][0][i]);							
						}
						PHY_SetBBReg(Adapter, rFPGA0_AnalogParameter4, 0x00f00000,0);
						PHY_SetBBReg(Adapter, rFPGA0_RFMOD, bOFDMEn|bCCKEn, 3);
					}
				  }
			}

		}

#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
		if(Adapter->dvobjpriv.ishighspeed == _FALSE)
				break;
#endif
		phy_SwitchRfSetting(Adapter, channel);

#if MP_DRIVER == 1
		RT_TRACE(COMP_SCAN, DBG_LOUD, ("phy_SwChnlStepByStep adapterindex %d currentbandtype %d \n", Adapter->interfaceIndex, pHalData->CurrentBandType92D));
		rtl8192d_PHY_IQCalibrate(Adapter);
#else
#if DEV_BUS_TYPE == DEV_BUS_USB_INTERFACE
		if(Adapter->dvobjpriv.ishighspeed == _FALSE)
			break;
#endif

		//---------Do IQK for normal chip and test chip 5G band----------------
		if(isNormal||((!isNormal)&&(pHalData->CurrentBandType92D==BAND_ON_5G||pHalData->BandSet92D == BAND_ON_BOTH)))
		{
			Indexforchannel = GetRightChnlPlaceforIQK((u8)param2);
			
			if(pHalData->bNeedIQK && !pHalData->IQKMatrixRegSetting[Indexforchannel].bIQKDone)
			{ //Re Do IQK.
				DBG_8192C("Do IQK Matrix reg for channel:%d....\n",param2);
				rtl8192d_PHY_IQCalibrate(Adapter);
			}
			else //Just load the value.
			{
				// 2G band just load once.
				if(((!pHalData->bLoadIMRandIQKSettingFor2G) && Indexforchannel==0) ||Indexforchannel>0)
				{
					//RT_TRACE(COMP_SCAN,DBG_LOUD,("Just Read IQK Matrix reg for channel:%d....\n",CurrentCmd->Para2));

					for(index=0;index< 9;index++)
					{
						PHY_SetBBReg(Adapter, pHalData->IQKMatrixReg[index],
							pHalData->IQKMatrixRegSetting[Indexforchannel].Mark[index], 
							pHalData->IQKMatrixRegSetting[Indexforchannel].Value[index]);
						//RT_TRACE(COMP_SCAN,DBG_LOUD,
						//	(" 0x%03X =%08X ",pHalData->IQKMatrixReg[index],PHY_QueryBBReg(Adapter, pHalData->IQKMatrixReg[index],bMaskDWord)));
						//RT_TRACE(COMP_SCAN|COMP_MLME,DBG_LOUD,
						//	(" 0x%03X(mark:%08X)=%08X ",pHalData->IQKMatrixReg[index],pHalData->IQKMatrixRegSetting[Indexforchannel].Mark[index],pHalData->IQKMatrixRegSetting[Indexforchannel].Value[index]));						}
					}
					if((check_fwstate(pmlmepriv, _FW_UNDER_SURVEY)== _TRUE)&&(Indexforchannel==0))
						pHalData->bLoadIMRandIQKSettingFor2G=_TRUE;
				}				
			}					
			pHalData->bNeedIQK = _FALSE;
		}
#endif
		break;
	}while(_TRUE);

	mdelay_os(10);

	//s3. post common command - CmdID_End, None

}


VOID
PHY_SwChnl8192D(	// Call after initialization
	IN	PADAPTER	Adapter,
	IN	u8		channel
	)
{
	//PADAPTER Adapter =  ADJUST_TO_ADAPTIVE_ADAPTER(pAdapter, _TRUE);
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	tmpchannel = pHalData->CurrentChannel;
	BOOLEAN  bResult = _TRUE;

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	PADAPTER	BuddyAdapter = Adapter->BuddyAdapter;
#endif

	if(check_fwstate(&Adapter->mlmepriv, _FW_UNDER_SURVEY) ==_TRUE)
	{
		pHalData->bLoadIMRandIQKSettingFor2G = _FALSE;
	}

	if(pHalData->rf_chip == RF_PSEUDO_11N)
	{
		//pHalData->SwChnlInProgress=FALSE;
		return; 								//return immediately if it is peudo-phy	
	}
	
	//if(pHalData->SwChnlInProgress)
	//	return;

	//if(pHalData->SetBWModeInProgress)
	//	return;

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(Adapter->bInModeSwitchProcess)
	{
		RT_TRACE(COMP_DUALMACSWITCH,DBG_LOUD,("PHY_SwChnl8192C(): During mode switch \n"));
		pHalData->SwChnlInProgress=FALSE;
		return;
	}
#endif

	//--------------------------------------------
	switch(pHalData->CurrentWirelessMode)
	{
		case WIRELESS_MODE_A:
		case WIRELESS_MODE_N_5G:
			//Get first channel error when change between 5G and 2.4G band.
			//FIX ME!!!
			//if(channel <=14)
			//	return;
			//RT_ASSERT((channel>14), ("WIRELESS_MODE_A but channel<=14"));
			break;
		
		case WIRELESS_MODE_B:
			//if(channel>14)
			//	return;
			//RT_ASSERT((channel<=14), ("WIRELESS_MODE_B but channel>14"));
			break;
		
		case WIRELESS_MODE_G:
		case WIRELESS_MODE_N_24G:
			//Get first channel error when change between 5G and 2.4G band.
			//FIX ME!!!
			//if(channel > 14)
			//	return;
			//RT_ASSERT((channel<=14), ("WIRELESS_MODE_G but channel>14"));
			break;

		default:
			//RT_ASSERT(FALSE, ("Invalid WirelessMode(%#x)!!\n", pHalData->CurrentWirelessMode));
			break;
	}
	//--------------------------------------------

	//pHalData->SwChnlInProgress = TRUE;
	if( channel == 0){//FIXME!!!A band?
		channel = 1;
	}

	pHalData->CurrentChannel=channel;

	//pHalData->SwChnlStage=0;
	//pHalData->SwChnlStep=0;

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if((BuddyAdapter !=NULL) && (Adapter->bSlaveOfDMSP))
	{
		RT_TRACE(COMP_DUALMACSWITCH,DBG_LOUD,("PHY_SwChnl8192C():slave return when slave  \n"));
		pHalData->SwChnlInProgress=_FALSE;
		return;
	}
#endif

	if((!Adapter->bDriverStopped) && (!Adapter->bSurpriseRemoved))
	{
#ifdef USE_WORKITEM	
		//bResult = PlatformScheduleWorkItem(&(pHalData->SwChnlWorkItem));
#else
		#if 0		
		//PlatformSetTimer(Adapter, &(pHalData->SwChnlTimer), 0);
		#else
		_PHY_SwChnl8192D(Adapter, channel);
		#endif
#endif
		if(bResult)
		{
			//RT_TRACE(COMP_SCAN, DBG_LOUD, ("PHY_SwChnl8192C SwChnlInProgress TRUE schdule workitem done\n"));
		}
		else
		{
			//RT_TRACE(COMP_SCAN, DBG_LOUD, ("PHY_SwChnl8192C SwChnlInProgress FALSE schdule workitem error\n"));		
			//if(IS_HARDWARE_TYPE_8192SU(Adapter))
			//{
			//	pHalData->SwChnlInProgress = FALSE; 	
				pHalData->CurrentChannel = tmpchannel;			
			//}
		}

	}
	else
	{
		//RT_TRACE(COMP_SCAN, DBG_LOUD, ("PHY_SwChnl8192C SwChnlInProgress FALSE driver sleep or unload\n"));	
		//if(IS_HARDWARE_TYPE_8192SU(Adapter))
		//{
		//	pHalData->SwChnlInProgress = FALSE;		
			pHalData->CurrentChannel = tmpchannel;
		//}
	}
}


static	BOOLEAN
phy_SwChnlStepByStep(
	IN	PADAPTER	Adapter,
	IN	u8		channel,
	IN	u8		*stage,
	IN	u8		*step,
	OUT u32		*delay
	)
{
#if 0
	HAL_DATA_TYPE			*pHalData = GET_HAL_DATA(Adapter);
	PCHANNEL_ACCESS_SETTING	pChnlAccessSetting;
	SwChnlCmd				PreCommonCmd[MAX_PRECMD_CNT];
	u4Byte					PreCommonCmdCnt;
	SwChnlCmd				PostCommonCmd[MAX_POSTCMD_CNT];
	u4Byte					PostCommonCmdCnt;
	SwChnlCmd				RfDependCmd[MAX_RFDEPENDCMD_CNT];
	u4Byte					RfDependCmdCnt;
	SwChnlCmd				*CurrentCmd;	
	u1Byte					eRFPath;	
	u4Byte					RfTXPowerCtrl;
	BOOLEAN					bAdjRfTXPowerCtrl = _FALSE;
	
	
	RT_ASSERT((Adapter != NULL), ("Adapter should not be NULL\n"));
#if(MP_DRIVER != 1)
	RT_ASSERT(IsLegalChannel(Adapter, channel), ("illegal channel: %d\n", channel));
#endif
	RT_ASSERT((pHalData != NULL), ("pHalData should not be NULL\n"));
	
	pChnlAccessSetting = &Adapter->MgntInfo.Info8185.ChannelAccessSetting;
	RT_ASSERT((pChnlAccessSetting != NULL), ("pChnlAccessSetting should not be NULL\n"));
	
	//for(eRFPath = RF90_PATH_A; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
	//for(eRFPath = 0; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
	//{
		// <1> Fill up pre common command.
	PreCommonCmdCnt = 0;
	phy_SetSwChnlCmdArray(PreCommonCmd, PreCommonCmdCnt++, MAX_PRECMD_CNT, 
				CmdID_SetTxPowerLevel, 0, 0, 0);
	phy_SetSwChnlCmdArray(PreCommonCmd, PreCommonCmdCnt++, MAX_PRECMD_CNT, 
				CmdID_End, 0, 0, 0);
	
		// <2> Fill up post common command.
	PostCommonCmdCnt = 0;

	phy_SetSwChnlCmdArray(PostCommonCmd, PostCommonCmdCnt++, MAX_POSTCMD_CNT, 
				CmdID_End, 0, 0, 0);
	
		// <3> Fill up RF dependent command.
	RfDependCmdCnt = 0;
	switch( pHalData->RFChipID )
	{
		case RF_8225:		
		RT_ASSERT((channel >= 1 && channel <= 14), ("illegal channel for Zebra: %d\n", channel));
		// 2008/09/04 MH Change channel. 
		if(channel==14) channel++;
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rZebra1_Channel, (0x10+channel-1), 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;	
		
	case RF_8256:
		// TEST!! This is not the table for 8256!!
		RT_ASSERT((channel >= 1 && channel <= 14), ("illegal channel for Zebra: %d\n", channel));
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, rRfChannel, channel, 10);
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);
		break;
		
	case RF_6052:
		RT_ASSERT((channel >= 1 && channel <= 14), ("illegal channel for Zebra: %d\n", channel));
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
			CmdID_RF_WriteReg, RF_CHNLBW, channel, 10);		
		phy_SetSwChnlCmdArray(RfDependCmd, RfDependCmdCnt++, MAX_RFDEPENDCMD_CNT, 
		CmdID_End, 0, 0, 0);		
		
		break;

	case RF_8258:
		break;

	// For FPGA two MAC verification
	case RF_PSEUDO_11N:
		return TRUE;
	default:
		RT_ASSERT(FALSE, ("Unknown RFChipID: %d\n", pHalData->RFChipID));
		return FALSE;
		break;
	}

	
	do{
		switch(*stage)
		{
		case 0:
			CurrentCmd=&PreCommonCmd[*step];
			break;
		case 1:
			CurrentCmd=&RfDependCmd[*step];
			break;
		case 2:
			CurrentCmd=&PostCommonCmd[*step];
			break;
		}
		
		if(CurrentCmd->CmdID==CmdID_End)
		{
			if((*stage)==2)
			{
				return TRUE;
			}
			else
			{
				(*stage)++;
				(*step)=0;
				continue;
			}
		}
		
		switch(CurrentCmd->CmdID)
		{
		case CmdID_SetTxPowerLevel:
			PHY_SetTxPowerLevel8192C(Adapter,channel);
			break;
		case CmdID_WritePortUlong:
			PlatformEFIOWrite4Byte(Adapter, CurrentCmd->Para1, CurrentCmd->Para2);
			break;
		case CmdID_WritePortUshort:
			PlatformEFIOWrite2Byte(Adapter, CurrentCmd->Para1, (u2Byte)CurrentCmd->Para2);
			break;
		case CmdID_WritePortUchar:
			PlatformEFIOWrite1Byte(Adapter, CurrentCmd->Para1, (u1Byte)CurrentCmd->Para2);
			break;
		case CmdID_RF_WriteReg:	// Only modify channel for the register now !!!!!
			for(eRFPath = 0; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
			{
#if 1
				pHalData->RfRegChnlVal[eRFPath] = ((pHalData->RfRegChnlVal[eRFPath] & 0xfffffc00) | CurrentCmd->Para2);
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, bRFRegOffsetMask, pHalData->RfRegChnlVal[eRFPath]);
#else
				PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, CurrentCmd->Para1, bRFRegOffsetMask, (CurrentCmd->Para2));
#endif
			}
			break;
		}
		
		break;
	}while(TRUE);
	//cosa }/*for(Number of RF paths)*/

	(*delay)=CurrentCmd->msDelay;
	(*step)++;
	return FALSE;
#endif	
	return _TRUE;
}


static	BOOLEAN
phy_SetSwChnlCmdArray(
	SwChnlCmd*		CmdTable,
	u32			CmdTableIdx,
	u32			CmdTableSz,
	SwChnlCmdID		CmdID,
	u32			Para1,
	u32			Para2,
	u32			msDelay
	)
{
	SwChnlCmd* pCmd;

	if(CmdTable == NULL)
	{
		//RT_ASSERT(FALSE, ("phy_SetSwChnlCmdArray(): CmdTable cannot be NULL.\n"));
		return _FALSE;
	}
	if(CmdTableIdx >= CmdTableSz)
	{
		//RT_ASSERT(FALSE, 
		//		("phy_SetSwChnlCmdArray(): Access invalid index, please check size of the table, CmdTableIdx:%ld, CmdTableSz:%ld\n",
		//		CmdTableIdx, CmdTableSz));
		return _FALSE;
	}

	pCmd = CmdTable + CmdTableIdx;
	pCmd->CmdID = CmdID;
	pCmd->Para1 = Para1;
	pCmd->Para2 = Para2;
	pCmd->msDelay = msDelay;

	return _TRUE;
}


static	void
phy_FinishSwChnlNow(	// We should not call this function directly
		IN	PADAPTER	Adapter,
		IN	u8		channel
		)
{
#if 0
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u32			delay;
  
	while(!phy_SwChnlStepByStep(Adapter,channel,&pHalData->SwChnlStage,&pHalData->SwChnlStep,&delay))
	{
		if(delay>0)
			mdelay_os(delay);
	}
#endif	
}


//
// Description:
//	Switch channel synchronously. Called by SwChnlByDelayHandler.
//
// Implemented by Bruce, 2008-02-14.
// The following procedure is operted according to SwChanlCallback8190Pci().
// However, this procedure is performed synchronously  which should be running under
// passive level.
// 
VOID
PHY_SwChnlPhy8192D(	// Only called during initialize
	IN	PADAPTER	Adapter,
	IN	u8		channel
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);

	//RT_TRACE(COMP_SCAN | COMP_RM, DBG_LOUD, ("==>PHY_SwChnlPhy8192S(), switch from channel %d to channel %d.\n", pHalData->CurrentChannel, channel));

	// Cannot IO.
	//if(RT_CANNOT_IO(Adapter))
	//	return;

	// Channel Switching is in progress.
	//if(pHalData->SwChnlInProgress)
	//	return;
	
	//return immediately if it is peudo-phy
	if(pHalData->rf_chip == RF_PSEUDO_11N)
	{
		//pHalData->SwChnlInProgress=FALSE;
		return;
	}
	
	//pHalData->SwChnlInProgress = TRUE;
	if( channel == 0)
		channel = 1;
	
	pHalData->CurrentChannel=channel;
	
	//pHalData->SwChnlStage = 0;
	//pHalData->SwChnlStep = 0;
	
	phy_FinishSwChnlNow(Adapter,channel);
	
	//pHalData->SwChnlInProgress = FALSE;
}


//
//	Description:
//		Configure H/W functionality to enable/disable Monitor mode.
//		Note, because we possibly need to configure BB and RF in this function, 
//		so caller should in PASSIVE_LEVEL. 080118, by rcnjko.
//
VOID
PHY_SetMonitorMode8192D(
	IN	PADAPTER			pAdapter,
	IN	BOOLEAN				bEnableMonitorMode
	)
{
#if 0
	HAL_DATA_TYPE		*pHalData	= GET_HAL_DATA(pAdapter);
	BOOLEAN				bFilterOutNonAssociatedBSSID = FALSE;

	//2 Note: we may need to stop antenna diversity.
	if(bEnableMonitorMode)
	{
		bFilterOutNonAssociatedBSSID = FALSE;
		RT_TRACE(COMP_RM, DBG_LOUD, ("PHY_SetMonitorMode8192S(): enable monitor mode\n"));

		pHalData->bInMonitorMode = TRUE;
		pAdapter->HalFunc.AllowAllDestAddrHandler(pAdapter, TRUE, TRUE);
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_CHECK_BSSID, (pu1Byte)&bFilterOutNonAssociatedBSSID);
	}
	else
	{
		bFilterOutNonAssociatedBSSID = TRUE;
		RT_TRACE(COMP_RM, DBG_LOUD, ("PHY_SetMonitorMode8192S(): disable monitor mode\n"));

		pAdapter->HalFunc.AllowAllDestAddrHandler(pAdapter, FALSE, TRUE);
		pHalData->bInMonitorMode = FALSE;
		pAdapter->HalFunc.SetHwRegHandler(pAdapter, HW_VAR_CHECK_BSSID, (pu1Byte)&bFilterOutNonAssociatedBSSID);
	}
#endif	
}


/*-----------------------------------------------------------------------------
 * Function:	PHYCheckIsLegalRfPath8190Pci()
 *
 * Overview:	Check different RF type to execute legal judgement. If RF Path is illegal
 *			We will return false.
 *
 * Input:		NONE
 *
 * Output:		NONE
 *
 * Return:		NONE
 *
 * Revised History:
 *	When		Who		Remark
 *	11/15/2007	MHC		Create Version 0.  
 *
 *---------------------------------------------------------------------------*/
BOOLEAN	
PHY_CheckIsLegalRfPath8192D(	
	IN	PADAPTER	pAdapter,
	IN	u32	eRFPath)
{
//	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	BOOLEAN				rtValue = _TRUE;

	// NOt check RF Path now.!
#if 0	
	if (pHalData->RF_Type == RF_1T2R && eRFPath != RF90_PATH_A)
	{		
		rtValue = FALSE;
	}
	if (pHalData->RF_Type == RF_1T2R && eRFPath != RF90_PATH_A)
	{

	}
#endif
	return	rtValue;

}	/* PHY_CheckIsLegalRfPath8192D */

//-------------------------------------------------------------------------
//
//	IQK
//
//-------------------------------------------------------------------------
#define MAX_TOLERANCE		5
#define IQK_DELAY_TIME		1 	//ms

static u8			//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
_PHY_PathA_IQK(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		configPathB
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	u32	regEAC, regE94, regE9C, regEA4;
	u8	result = 0x00;
	BOOLEAN		isNormal = IS_NORMAL_CHIP(pHalData->VersionID);

	//RTPRINT(FINIT, INIT_IQK, ("Path A IQK!\n"));

	//path-A IQK setting
	//RTPRINT(FINIT, INIT_IQK, ("Path-A IQK setting!\n"));
	if(pHalData->interfaceIndex == 0)
	{
		PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x10008c1f);
		PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x10008c1f);
	}
	else
	{
		PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x10008c22);
		PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x10008c22);	
	}

	PHY_SetBBReg(pAdapter, 0xe38, bMaskDWord, 0x82140102);

	if(isNormal)
		PHY_SetBBReg(pAdapter, 0xe3c, bMaskDWord, 0x28160206);		
	else
		PHY_SetBBReg(pAdapter, 0xe3c, bMaskDWord, configPathB ? 0x28160202 : 0x28160502);

	//path-B IQK setting
	if(configPathB)
	{
		PHY_SetBBReg(pAdapter, 0xe50, bMaskDWord, 0x10008c22);
		PHY_SetBBReg(pAdapter, 0xe54, bMaskDWord, 0x10008c22);
		PHY_SetBBReg(pAdapter, 0xe58, bMaskDWord, 0x82140102);			

		if(isNormal)
			PHY_SetBBReg(pAdapter, 0xe5c, bMaskDWord, 0x28160206);
		else	
			PHY_SetBBReg(pAdapter, 0xe5c, bMaskDWord, 0x28160202);
	}

	//LO calibration setting
	//RTPRINT(FINIT, INIT_IQK, ("LO calibration setting!\n"));
	if(isNormal)
		PHY_SetBBReg(pAdapter, 0xe4c, bMaskDWord, 0x00462911);	
	else	
		PHY_SetBBReg(pAdapter, 0xe4c, bMaskDWord, 0x001028d1);

	//One shot, path A LOK & IQK
	//RTPRINT(FINIT, INIT_IQK, ("One shot, path A LOK & IQK!\n"));
	PHY_SetBBReg(pAdapter, 0xe48, bMaskDWord, 0xf9000000);
	PHY_SetBBReg(pAdapter, 0xe48, bMaskDWord, 0xf8000000);
	
	// delay x ms
	//RTPRINT(FINIT, INIT_IQK, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME));
	udelay_os(IQK_DELAY_TIME*1000);//PlatformStallExecution(IQK_DELAY_TIME*1000);

	// Check failed
	regEAC = PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord);
	//RTPRINT(FINIT, INIT_IQK, ("0xeac = 0x%x\n", regEAC));
	regE94 = PHY_QueryBBReg(pAdapter, 0xe94, bMaskDWord);
	//RTPRINT(FINIT, INIT_IQK, ("0xe94 = 0x%x\n", regE94));
	regE9C= PHY_QueryBBReg(pAdapter, 0xe9c, bMaskDWord);
	//RTPRINT(FINIT, INIT_IQK, ("0xe9c = 0x%x\n", regE9C));
	regEA4= PHY_QueryBBReg(pAdapter, 0xea4, bMaskDWord);
	//RTPRINT(FINIT, INIT_IQK, ("0xea4 = 0x%x\n", regEA4));

        if(!(regEAC & BIT28) &&		
		(((regE94 & 0x03FF0000)>>16) != 0x142) &&
		(((regE9C & 0x03FF0000)>>16) != 0x42) )
		result |= 0x01;
	else							//if Tx not OK, ignore Rx
		return result;

	if(!(regEAC & BIT27) &&		//if Tx is OK, check whether Rx is OK
		(((regEA4 & 0x03FF0000)>>16) != 0x132) &&
		(((regEAC & 0x03FF0000)>>16) != 0x36))
		result |= 0x02;
	else
		DBG_8192C("Path A Rx IQK fail!!\n");
	
	return result;


}

static u8			//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
_PHY_PathA_IQK_5G_Normal(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		configPathB
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u32	regEAC, regE94, regE9C, regEA4;
	u8	result = 0x00;
	u8	i = 0;
#if MP_DRIVER == 1
	u8	retryCount = 9;
#else
	u8	retryCount = 2;
#endif

	u32	TxOKBit = BIT28, RxOKBit = BIT27;

	if(pHalData->interfaceIndex == 1)	//PHY1
	{
		TxOKBit = BIT31;
		RxOKBit = BIT30;
	}

	//RTPRINT(FINIT, INIT_IQK, ("Path A IQK!\n"));

	//path-A IQK setting
	//RTPRINT(FINIT, INIT_IQK, ("Path-A IQK setting!\n"));

	PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x18008c1f);
	PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x18008c1f);
	PHY_SetBBReg(pAdapter, 0xe38, bMaskDWord, 0x821402e6);
	PHY_SetBBReg(pAdapter, 0xe3c, bMaskDWord, 0x68160966);		

	//path-B IQK setting
	if(configPathB)
	{
		PHY_SetBBReg(pAdapter, 0xe50, bMaskDWord, 0x18008c2f );
		PHY_SetBBReg(pAdapter, 0xe54, bMaskDWord, 0x18008c2f );			
		PHY_SetBBReg(pAdapter, 0xe58, bMaskDWord, 0x82110000);						
		PHY_SetBBReg(pAdapter, 0xe5c, bMaskDWord, 0x68110000);
	}
	
	//LO calibration setting
	//RTPRINT(FINIT, INIT_IQK, ("LO calibration setting!\n"));
	PHY_SetBBReg(pAdapter, 0xe4c, bMaskDWord, 0x00462911);	

	//path-A PA on
	PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFInterfaceSW, BIT10|BIT11, 0x03);
	PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFInterfaceSW, BIT6|BIT5, 0x03);	
	PHY_SetBBReg(pAdapter, rFPGA0_XA_RFInterfaceOE, BIT10|BIT11, 0x03);

	for(i = 0 ; i < retryCount ; i++)
	{

		//One shot, path A LOK & IQK
		//RTPRINT(FINIT, INIT_IQK, ("One shot, path A LOK & IQK!\n"));
		PHY_SetBBReg(pAdapter, 0xe48, bMaskDWord, 0xf9000000);
		PHY_SetBBReg(pAdapter, 0xe48, bMaskDWord, 0xf8000000);
		
		// delay x ms
		//RTPRINT(FINIT, INIT_IQK, ("Delay %d ms for One shot, path A LOK & IQK.\n", IQK_DELAY_TIME));
		udelay_os(IQK_DELAY_TIME*1000*10);

		// Check failed
		regEAC = PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("0xeac = 0x%lx\n", regEAC));
		regE94 = PHY_QueryBBReg(pAdapter, 0xe94, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("0xe94 = 0x%lx\n", regE94));
		regE9C= PHY_QueryBBReg(pAdapter, 0xe9c, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("0xe9c = 0x%lx\n", regE9C));
		regEA4= PHY_QueryBBReg(pAdapter, 0xea4, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("0xea4 = 0x%lx\n", regEA4));

		if(!(regEAC & TxOKBit) &&		
			(((regE94 & 0x03FF0000)>>16) != 0x142)  )
		{
			result |= 0x01;
		}
		else			//if Tx not OK, ignore Rx
		{
			//RTPRINT(FINIT, INIT_IQK, ("Path A Tx IQK fail!!\n"));		
			continue;
		}		

		if(!(regEAC & RxOKBit) &&			//if Tx is OK, check whether Rx is OK
			(((regEA4 & 0x03FF0000)>>16) != 0x132))
		{
			result |= 0x02;
			break;
		}	
		else
		{
			//RTPRINT(FINIT, INIT_IQK, ("Path A Rx IQK fail!!\n"));
		}
	}

	//path A PA off
	PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFInterfaceSW, bMaskDWord, pdmpriv->IQK_BB_backup[0]);
	PHY_SetBBReg(pAdapter, rFPGA0_XA_RFInterfaceOE, bMaskDWord, pdmpriv->IQK_BB_backup[1]);
	
	return result;
}

static u8				//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
_PHY_PathB_IQK(
	IN	PADAPTER	pAdapter
	)
{
	u32 regEAC, regEB4, regEBC, regEC4, regECC;
	u8	result = 0x00;
	//RTPRINT(FINIT, INIT_IQK, ("Path B IQK!\n"));

	//One shot, path B LOK & IQK
	//RTPRINT(FINIT, INIT_IQK, ("One shot, path A LOK & IQK!\n"));
	PHY_SetBBReg(pAdapter, 0xe60, bMaskDWord, 0x00000002);
	PHY_SetBBReg(pAdapter, 0xe60, bMaskDWord, 0x00000000);

	// delay x ms
	//RTPRINT(FINIT, INIT_IQK, ("Delay %d ms for One shot, path B LOK & IQK.\n", IQK_DELAY_TIME));
	udelay_os(IQK_DELAY_TIME*1000);//PlatformStallExecution(IQK_DELAY_TIME*1000);

	// Check failed
	regEAC = PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord);
	//RTPRINT(FINIT, INIT_IQK, ("0xeac = 0x%x\n", regEAC));
	regEB4 = PHY_QueryBBReg(pAdapter, 0xeb4, bMaskDWord);
	//RTPRINT(FINIT, INIT_IQK, ("0xeb4 = 0x%x\n", regEB4));
	regEBC= PHY_QueryBBReg(pAdapter, 0xebc, bMaskDWord);
	//RTPRINT(FINIT, INIT_IQK, ("0xebc = 0x%x\n", regEBC));
	regEC4= PHY_QueryBBReg(pAdapter, 0xec4, bMaskDWord);
	//RTPRINT(FINIT, INIT_IQK, ("0xec4 = 0x%x\n", regEC4));
	regECC= PHY_QueryBBReg(pAdapter, 0xecc, bMaskDWord);
	//RTPRINT(FINIT, INIT_IQK, ("0xecc = 0x%x\n", regECC));

	if(!(regEAC & BIT31) &&
		(((regEB4 & 0x03FF0000)>>16) != 0x142) &&
		(((regEBC & 0x03FF0000)>>16) != 0x42))
		result |= 0x01;
	else
		return result;

	if(!(regEAC & BIT30) &&
		(((regEC4 & 0x03FF0000)>>16) != 0x132) &&
		(((regECC & 0x03FF0000)>>16) != 0x36))
		result |= 0x02;
	else
		DBG_8192C("Path B Rx IQK fail!!\n");
	

	return result;

}

static u8				//bit0 = 1 => Tx OK, bit1 = 1 => Rx OK
_PHY_PathB_IQK_5G_Normal(
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u32	regEAC, regEB4, regEBC, regEC4, regECC;
	u8	result = 0x00;
	u8	i = 0, j = 0;
#if MP_DRIVER == 1
	u8	retryCount = 9;
#else
	u8	retryCount = 2;
#endif

	//RTPRINT(FINIT, INIT_IQK, ("Path B IQK!\n"));

	//path-A IQK setting
	//RTPRINT(FINIT, INIT_IQK, ("Path-A IQK setting!\n"));
	PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x18008c1f);
	PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x18008c1f);

	PHY_SetBBReg(pAdapter, 0xe38, bMaskDWord, 0x82110000);
	PHY_SetBBReg(pAdapter, 0xe3c, bMaskDWord, 0x68110000);		

	//path-B IQK setting
	PHY_SetBBReg(pAdapter, 0xe50, bMaskDWord, 0x18008c2f );
	PHY_SetBBReg(pAdapter, 0xe54, bMaskDWord, 0x18008c2f );			
	PHY_SetBBReg(pAdapter, 0xe58, bMaskDWord, 0x821402e6);						
	PHY_SetBBReg(pAdapter, 0xe5c, bMaskDWord, 0x68160966);

	//LO calibration setting
	//RTPRINT(FINIT, INIT_IQK, ("LO calibration setting!\n"));
	PHY_SetBBReg(pAdapter, 0xe4c, bMaskDWord, 0x00462911);	

	//path-B PA on
	PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFInterfaceSW, BIT27|BIT26, 0x03);
	PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFInterfaceSW, BIT22|BIT21, 0x03);	
	PHY_SetBBReg(pAdapter, rFPGA0_XB_RFInterfaceOE, BIT10|BIT11, 0x03);

	for(i = 0 ; i < retryCount ; i++)
	{
		//One shot, path B LOK & IQK
		//RTPRINT(FINIT, INIT_IQK, ("One shot, path A LOK & IQK!\n"));
		//PHY_SetBBReg(pAdapter, 0xe60, bMaskDWord, 0x00000002);
		//PHY_SetBBReg(pAdapter, 0xe60, bMaskDWord, 0x00000000);
		PHY_SetBBReg(pAdapter, 0xe48, bMaskDWord, 0xfa000000);
		PHY_SetBBReg(pAdapter, 0xe48, bMaskDWord, 0xf8000000);


		// delay x ms
		//RTPRINT(FINIT, INIT_IQK, ("Delay %d ms for One shot, path B LOK & IQK.\n", 10));
		udelay_os(IQK_DELAY_TIME*1000*10);

		// Check failed
		regEAC = PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("0xeac = 0x%lx\n", regEAC));
		regEB4 = PHY_QueryBBReg(pAdapter, 0xeb4, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("0xeb4 = 0x%lx\n", regEB4));
		regEBC= PHY_QueryBBReg(pAdapter, 0xebc, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("0xebc = 0x%lx\n", regEBC));
		regEC4= PHY_QueryBBReg(pAdapter, 0xec4, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("0xec4 = 0x%lx\n", regEC4));
		regECC= PHY_QueryBBReg(pAdapter, 0xecc, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("0xecc = 0x%lx\n", regECC));

		if(!(regEAC & BIT31) &&
			(((regEB4 & 0x03FF0000)>>16) != 0x142))
			result |= 0x01;
		else
			continue;

		if(!(regEAC & BIT30) &&
			(((regEC4 & 0x03FF0000)>>16) != 0x132))
		{
			result |= 0x02;
			break;
		}
		else
		{
			//RTPRINT(FINIT, INIT_IQK, ("Path B Rx IQK fail!!\n"));		
		}
	}

	//path B PA off
	PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFInterfaceSW, bMaskDWord, pdmpriv->IQK_BB_backup[0]);
	PHY_SetBBReg(pAdapter, rFPGA0_XB_RFInterfaceOE, bMaskDWord, pdmpriv->IQK_BB_backup[2]);
	
	return result;
}

static VOID
_PHY_PathAFillIQKMatrix(
	IN	PADAPTER	pAdapter,
	IN  BOOLEAN    	bIQKOK,
	IN	int		result[][8],
	IN	u8		final_candidate,
	IN  BOOLEAN		bTxOnly
	)
{
	u32	Oldval_0, X, TX0_A, reg;
	int	Y, TX0_C;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);	
	BOOLEAN       is2T =  IS_92D_SINGLEPHY(pHalData->VersionID) || 
					pHalData->MacPhyMode92D == DUALMAC_DUALPHY;
	
	DBG_8192C("Path A IQ Calibration %s !\n",(bIQKOK)?"Success":"Failed");

        if(final_candidate == 0xFF)
		return;

	else if(bIQKOK)
	{
		Oldval_0 = (PHY_QueryBBReg(pAdapter, rOFDM0_XATxIQImbalance, bMaskDWord) >> 22) & 0x3FF;//OFDM0_D

		X = result[final_candidate][0];
		if ((X & 0x00000200) != 0)
			X = X | 0xFFFFFC00;				
		TX0_A = (X * Oldval_0) >> 8;
		//RTPRINT(FINIT, INIT_IQK, ("X = 0x%lx, TX0_A = 0x%lx, Oldval_0 0x%lx\n", X, TX0_A, Oldval_0));
		PHY_SetBBReg(pAdapter, rOFDM0_XATxIQImbalance, 0x3FF, TX0_A);
		PHY_SetBBReg(pAdapter, rOFDM0_ECCAThreshold, BIT(24), ((X* Oldval_0>>7) & 0x1));

		Y = result[final_candidate][1];
		if ((Y & 0x00000200) != 0)
			Y = Y | 0xFFFFFC00;		
		TX0_C = (Y * Oldval_0) >> 8;
		//RTPRINT(FINIT, INIT_IQK, ("Y = 0x%lx, TX = 0x%lx\n", Y, TX0_C));
		PHY_SetBBReg(pAdapter, rOFDM0_XCTxAFE, 0xF0000000, ((TX0_C&0x3C0)>>6));
		PHY_SetBBReg(pAdapter, rOFDM0_XATxIQImbalance, 0x003F0000, (TX0_C&0x3F));
		if(is2T)
			PHY_SetBBReg(pAdapter, rOFDM0_ECCAThreshold, BIT26, ((Y* Oldval_0>>7) & 0x1));

		//RTPRINT(FINIT, INIT_IQK, ("0xC80 = 0x%lx\n", PHY_QueryBBReg(pAdapter, rOFDM0_XATxIQImbalance, bMaskDWord)));	

	        if(bTxOnly)
		{
			DBG_8192C("_PHY_PathAFillIQKMatrix only Tx OK\n");
			return;
		}

		reg = result[final_candidate][2];
		PHY_SetBBReg(pAdapter, rOFDM0_XARxIQImbalance, 0x3FF, reg);
	
		reg = result[final_candidate][3] & 0x3F;
		PHY_SetBBReg(pAdapter, rOFDM0_XARxIQImbalance, 0xFC00, reg);

		reg = (result[final_candidate][3] >> 6) & 0xF;
		PHY_SetBBReg(pAdapter, 0xca0, 0xF0000000, reg);
	}
}

static VOID
_PHY_PathBFillIQKMatrix(
	IN	PADAPTER	pAdapter,
	IN  BOOLEAN   	bIQKOK,
	IN	int		result[][8],
	IN	u8		final_candidate,
	IN	BOOLEAN		bTxOnly			//do Tx only
	)
{
	u32	Oldval_1, X, TX1_A, reg;
	int	Y, TX1_C;
	
	DBG_8192C("Path B IQ Calibration %s !\n",(bIQKOK)?"Success":"Failed");

        if(final_candidate == 0xFF)
		return;

	else if(bIQKOK)
	{
		Oldval_1 = (PHY_QueryBBReg(pAdapter, rOFDM0_XBTxIQImbalance, bMaskDWord) >> 22) & 0x3FF;

		X = result[final_candidate][4];
		if ((X & 0x00000200) != 0)
			X = X | 0xFFFFFC00;		
		TX1_A = (X * Oldval_1) >> 8;
		//RTPRINT(FINIT, INIT_IQK, ("X = 0x%lx, TX1_A = 0x%lx\n", X, TX1_A));
		PHY_SetBBReg(pAdapter, rOFDM0_XBTxIQImbalance, 0x3FF, TX1_A);
		PHY_SetBBReg(pAdapter, rOFDM0_ECCAThreshold, BIT28, ((X* Oldval_1>>7) & 0x1));

		Y = result[final_candidate][5];
		if ((Y & 0x00000200) != 0)
			Y = Y | 0xFFFFFC00;
		//Y += 3;		//temp modify for preformance
		TX1_C = (Y * Oldval_1) >> 8;
		//RTPRINT(FINIT, INIT_IQK, ("Y = 0x%lx, TX1_C = 0x%lx\n", Y, TX1_C));
		PHY_SetBBReg(pAdapter, rOFDM0_XDTxAFE, 0xF0000000, ((TX1_C&0x3C0)>>6));
		PHY_SetBBReg(pAdapter, rOFDM0_XBTxIQImbalance, 0x003F0000, (TX1_C&0x3F));
		PHY_SetBBReg(pAdapter, rOFDM0_ECCAThreshold, BIT30, ((Y* Oldval_1>>7) & 0x1));

		if(bTxOnly)
			return;

		reg = result[final_candidate][6];
		PHY_SetBBReg(pAdapter, rOFDM0_XBRxIQImbalance, 0x3FF, reg);
	
		reg = result[final_candidate][7] & 0x3F;
		PHY_SetBBReg(pAdapter, rOFDM0_XBRxIQImbalance, 0xFC00, reg);

		reg = (result[final_candidate][7] >> 6) & 0xF;
		PHY_SetBBReg(pAdapter, rOFDM0_AGCRSSITable, 0x0000F000, reg);
	}
}

static VOID
_PHY_SaveADDARegisters(
	IN	PADAPTER	pAdapter,
	IN	u32*		ADDAReg,
	IN	u32*		ADDABackup,
	IN	u32			RegisterNum
	)
{
	u32	i;
	
	//RTPRINT(FINIT, INIT_IQK, ("Save ADDA parameters.\n"));
	for( i = 0 ; i < RegisterNum ; i++){
		ADDABackup[i] = PHY_QueryBBReg(pAdapter, ADDAReg[i], bMaskDWord);
	}
}

static VOID
_PHY_SaveMACRegisters(
	IN	PADAPTER	pAdapter,
	IN	u32*		MACReg,
	IN	u32*		MACBackup
	)
{
	u32	i;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);	
	
	//RTPRINT(FINIT, INIT_IQK, ("Save MAC parameters.\n"));
	for( i = 0 ; i < (IQK_MAC_REG_NUM - 1); i++){
		MACBackup[i] = read8(pAdapter, MACReg[i]);		
	}
	if(pHalData->CurrentBandType92D != BAND_ON_5G)		
		MACBackup[i] = read32(pAdapter, MACReg[i]);		

}

static VOID
_PHY_ReloadADDARegisters(
	IN	PADAPTER	pAdapter,
	IN	u32*		ADDAReg,
	IN	u32*		ADDABackup,
	IN	u32			RegiesterNum
	)
{
	u32	i;

	//RTPRINT(FINIT, INIT_IQK, ("Reload ADDA power saving parameters !\n"));
	for(i = 0 ; i < RegiesterNum ; i++){
		PHY_SetBBReg(pAdapter, ADDAReg[i], bMaskDWord, ADDABackup[i]);
	}
}

static VOID
_PHY_ReloadMACRegisters(
	IN	PADAPTER	pAdapter,
	IN	u32*		MACReg,
	IN	u32*		MACBackup
	)
{
	u32	i;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);		

	//RTPRINT(FINIT, INIT_IQK, ("Reload MAC parameters !\n"));
	for(i = 0 ; i < (IQK_MAC_REG_NUM - 1); i++){
		write8(pAdapter, MACReg[i], (u8)MACBackup[i]);
	}
	if(pHalData->CurrentBandType92D != BAND_ON_5G)			
		write32(pAdapter, MACReg[i], MACBackup[i]);	
}

static VOID
_PHY_PathADDAOn(
	IN	PADAPTER	pAdapter,
	IN	u32*		ADDAReg,
	IN	BOOLEAN		isPathAOn,
	IN	BOOLEAN		is2T
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	u32	pathOn;
	u32	i;

	//RTPRINT(FINIT, INIT_IQK, ("ADDA ON.\n"));


	//from luke.
	pathOn = isPathAOn ? 0x04db25a4 : 0x0b1b25a4;
	if(isPathAOn)
		pathOn = pHalData->interfaceIndex == 0? 0x04db25a4 : 0x0b1b25a4;
	for( i = 0 ; i < IQK_ADDA_REG_NUM ; i++){
		PHY_SetBBReg(pAdapter, ADDAReg[i], bMaskDWord, pathOn);
	}

}

static VOID
_PHY_MACSettingCalibration(
	IN	PADAPTER	pAdapter,
	IN	u32*		MACReg,
	IN	u32*		MACBackup	
	)
{
	u32	i = 0;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);	

	//RTPRINT(FINIT, INIT_IQK, ("MAC settings for Calibration.\n"));

	write8(pAdapter, MACReg[i], 0x3F);

	for(i = 1 ; i < (IQK_MAC_REG_NUM - 1); i++){
		write8(pAdapter, MACReg[i], (u8)(MACBackup[i]&(~BIT3)));
	}
	if(pHalData->CurrentBandType92D != BAND_ON_5G)	
		write8(pAdapter, MACReg[i], (u8)(MACBackup[i]&(~BIT5)));	

}

static VOID
_PHY_PathAStandBy(
	IN	PADAPTER	pAdapter
	)
{
	//RTPRINT(FINIT, INIT_IQK, ("Path-A standby mode!\n"));

	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x0);
	PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x00010000);
	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x80800000);
}

static VOID
_PHY_PIModeSwitch(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		PIMode
	)
{
	u32	mode;

	//RTPRINT(FINIT, INIT_IQK, ("BB Switch to %s mode!\n", (PIMode ? "PI" : "SI")));

	mode = PIMode ? 0x01000100 : 0x01000000;
	PHY_SetBBReg(pAdapter, 0x820, bMaskDWord, mode);
	PHY_SetBBReg(pAdapter, 0x828, bMaskDWord, mode);
}

/*
return _FALSE => do IQK again
*/
static BOOLEAN							
_PHY_SimularityCompare(
	IN	PADAPTER	pAdapter,
	IN	int 		result[][8],
	IN	u8		 c1,
	IN	u8		 c2
	)
{
	u32		i, j, diff, SimularityBitMap, bound = 0;
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);	
	u8		final_candidate[2] = {0xFF, 0xFF};	//for path A and path B
	BOOLEAN		bResult = _TRUE;
	BOOLEAN       is2T = (IS_92C_SERIAL(pHalData->VersionID) || IS_92D_SINGLEPHY(pHalData->VersionID));
	
	if(is2T)
		bound = 8;
	else
		bound = 4;

	SimularityBitMap = 0;
	
	for( i = 0; i < bound; i++ )
	{
		diff = (result[c1][i] > result[c2][i]) ? (result[c1][i] - result[c2][i]) : (result[c2][i] - result[c1][i]);
		if (diff > MAX_TOLERANCE)
		{
			if((i == 2 || i == 6) && !SimularityBitMap)
			{
				if(result[c1][i]+result[c1][i+1] == 0)
					final_candidate[(i/4)] = c2;
				else if (result[c2][i]+result[c2][i+1] == 0)
					final_candidate[(i/4)] = c1;
				else
					SimularityBitMap = SimularityBitMap|(1<<i);					
			}
			else
				SimularityBitMap = SimularityBitMap|(1<<i);
		}
	}
	
	if ( SimularityBitMap == 0)
	{
		for( i = 0; i < (bound/4); i++ )
		{
			if(final_candidate[i] != 0xFF)
			{
				for( j = i*4; j < (i+1)*4-2; j++)
					result[3][j] = result[final_candidate[i]][j];
				bResult = _FALSE;
			}
		}
		return bResult;
	}
	else if (!(SimularityBitMap & 0x0F))			//path A OK
	{
		for(i = 0; i < 4; i++)
			result[3][i] = result[c1][i];
		return _FALSE;
	}
	else if (!(SimularityBitMap & 0xF0) && is2T)	//path B OK
	{
		for(i = 4; i < 8; i++)
			result[3][i] = result[c1][i];
		return _FALSE;
	}	
	else		
		return _FALSE;
	
}

static VOID	
_PHY_IQCalibrate(
	IN	PADAPTER	pAdapter,
	IN	int 		result[][8],
	IN	u8		t,
	IN	BOOLEAN		is2T
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u32			i;
	u8			PathAOK, PathBOK;
	u32			ADDA_REG[IQK_ADDA_REG_NUM] = {	
						rFPGA0_XCD_SwitchControl, 	0xe6c, 	0xe70, 	0xe74,
						0xe78, 	0xe7c, 	0xe80, 	0xe84,
						0xe88, 	0xe8c, 	0xed0, 	0xed4,
						0xed8, 	0xedc, 	0xee0, 	0xeec };
	u32			IQK_MAC_REG[IQK_MAC_REG_NUM] = {
						0x522, 0x550,	0x551,	0x040};

	u32			IQK_BB_REG[IQK_BB_REG_NUM] = {	//for normal
						rFPGA0_XAB_RFInterfaceSW,	rFPGA0_XA_RFInterfaceOE,	
						rFPGA0_XB_RFInterfaceOE,	rOFDM0_TRMuxPar,
						rFPGA0_XCD_RFInterfaceSW,	rOFDM0_TRxPathEnable,	
						rFPGA0_RFMOD,			rFPGA0_AnalogParameter4					
					};
#if MP_DRIVER
	const u32	retryCount = 9;
#else
	const u32	retryCount = 2;
#endif

	// Note: IQ calibration must be performed after loading 
	// 		PHY_REG.txt , and radio_a, radio_b.txt	
	
	u32 bbvalue;
	BOOLEAN		isNormal = IS_NORMAL_CHIP(pHalData->VersionID);

	//RTPRINT(FINIT, INIT_IQK, ("IQK for 2.4G :Start!!! interface %d\n", pAdapter->interfaceIndex));

	//if(isNormal)		
	//	udelay_os(IQK_DELAY_TIME*1000*100);	//delay after set IMR

	if(t==0)
	{
	 	bbvalue = PHY_QueryBBReg(pAdapter, rFPGA0_RFMOD, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("PHY_IQCalibrate()==>0x%08lx\n",bbvalue));
		//RTPRINT(FINIT, INIT_IQK, ("IQ Calibration for %s\n", (is2T ? "2T2R" : "1T1R")));

	 	// Save ADDA parameters, turn Path A ADDA on
	 	_PHY_SaveADDARegisters(pAdapter, ADDA_REG, pdmpriv->ADDA_backup, IQK_ADDA_REG_NUM);
		_PHY_SaveMACRegisters(pAdapter, IQK_MAC_REG, pdmpriv->IQK_MAC_backup);
		if(isNormal)
		 	_PHY_SaveADDARegisters(pAdapter, IQK_BB_REG, pdmpriv->IQK_BB_backup, IQK_BB_REG_NUM);
		else
			_PHY_SaveADDARegisters(pAdapter, IQK_BB_REG, pdmpriv->IQK_BB_backup, IQK_BB_REG_NUM-1);
	}

 	_PHY_PathADDAOn(pAdapter, ADDA_REG, _TRUE, is2T);

	if(t==0)
	{
		pdmpriv->bRfPiEnable = (u8)PHY_QueryBBReg(pAdapter, rFPGA0_XA_HSSIParameter1, BIT(8));
	}

	if(!pdmpriv->bRfPiEnable){
		// Switch BB to PI mode to do IQ Calibration.
		_PHY_PIModeSwitch(pAdapter, _TRUE);
	}

	PHY_SetBBReg(pAdapter, rFPGA0_RFMOD, BIT24, 0x00);			
	PHY_SetBBReg(pAdapter, rOFDM0_TRxPathEnable, bMaskDWord, 0x03a05600);
	PHY_SetBBReg(pAdapter, rOFDM0_TRMuxPar, bMaskDWord, 0x000800e4);
	PHY_SetBBReg(pAdapter, rFPGA0_XCD_RFInterfaceSW, bMaskDWord, 0x22204000);
	if(!isNormal)
	{
		PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFInterfaceSW, BIT10, 0x01);
		PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFInterfaceSW, BIT26, 0x01);	
		PHY_SetBBReg(pAdapter, rFPGA0_XA_RFInterfaceOE, BIT10, 0x00);
		PHY_SetBBReg(pAdapter, rFPGA0_XB_RFInterfaceOE, BIT10, 0x00);	
	}
	if(isNormal)
		PHY_SetBBReg(pAdapter, rFPGA0_AnalogParameter4, 0xf00000, 0x0f); 

	if(is2T)
	{
		PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x00010000);
		PHY_SetBBReg(pAdapter, rFPGA0_XB_LSSIParameter, bMaskDWord, 0x00010000);
	}

	//MAC settings
	_PHY_MACSettingCalibration(pAdapter, IQK_MAC_REG, pdmpriv->IQK_MAC_backup);
	
	//Page B init
	PHY_SetBBReg(pAdapter, 0xb68, bMaskDWord, 0x0f600000);
	
	if(is2T)
	{
		PHY_SetBBReg(pAdapter, 0xb6c, bMaskDWord, 0x0f600000);
	}

	// IQ calibration setting
	//RTPRINT(FINIT, INIT_IQK, ("IQK setting!\n"));		
	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x80800000);
	PHY_SetBBReg(pAdapter, 0xe40, bMaskDWord, 0x01007c00);
	PHY_SetBBReg(pAdapter, 0xe44, bMaskDWord, 0x01004800);

	for(i = 0 ; i < retryCount ; i++){
		PathAOK = _PHY_PathA_IQK(pAdapter, is2T);
		if(PathAOK == 0x03){
			//RTPRINT(FINIT, INIT_IQK, ("Path A IQK Success!!\n"));
			result[t][0] = (PHY_QueryBBReg(pAdapter, 0xe94, bMaskDWord)&0x3FF0000)>>16;
			result[t][1] = (PHY_QueryBBReg(pAdapter, 0xe9c, bMaskDWord)&0x3FF0000)>>16;
			result[t][2] = (PHY_QueryBBReg(pAdapter, 0xea4, bMaskDWord)&0x3FF0000)>>16;
			result[t][3] = (PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord)&0x3FF0000)>>16;
			break;
		}
		else if (i == (retryCount-1) && PathAOK == 0x01)	//Tx IQK OK
		{
			//RTPRINT(FINIT, INIT_IQK, ("Path A IQK Only  Tx Success!!\n"));
			
			result[t][0] = (PHY_QueryBBReg(pAdapter, 0xe94, bMaskDWord)&0x3FF0000)>>16;
			result[t][1] = (PHY_QueryBBReg(pAdapter, 0xe9c, bMaskDWord)&0x3FF0000)>>16;			
		}
	}

	if(0x00 == PathAOK){		
		//RTPRINT(FINIT, INIT_IQK, ("Path A IQK failed!!\n"));		
	}

	if(is2T){
		_PHY_PathAStandBy(pAdapter);

		// Turn Path B ADDA on
		_PHY_PathADDAOn(pAdapter, ADDA_REG, _FALSE, is2T);

		for(i = 0 ; i < retryCount ; i++){
			PathBOK = _PHY_PathB_IQK(pAdapter);
			if(PathBOK == 0x03){
				//RTPRINT(FINIT, INIT_IQK, ("Path B IQK Success!!\n"));
				result[t][4] = (PHY_QueryBBReg(pAdapter, 0xeb4, bMaskDWord)&0x3FF0000)>>16;
				result[t][5] = (PHY_QueryBBReg(pAdapter, 0xebc, bMaskDWord)&0x3FF0000)>>16;
				result[t][6] = (PHY_QueryBBReg(pAdapter, 0xec4, bMaskDWord)&0x3FF0000)>>16;
				result[t][7] = (PHY_QueryBBReg(pAdapter, 0xecc, bMaskDWord)&0x3FF0000)>>16;
				break;
			}
			else if (i == (retryCount - 1) && PathBOK == 0x01)	//Tx IQK OK
			{
				//RTPRINT(FINIT, INIT_IQK, ("Path B Only Tx IQK Success!!\n"));
				result[t][4] = (PHY_QueryBBReg(pAdapter, 0xeb4, bMaskDWord)&0x3FF0000)>>16;
				result[t][5] = (PHY_QueryBBReg(pAdapter, 0xebc, bMaskDWord)&0x3FF0000)>>16;				
			}
		}

		if(0x00 == PathBOK){		
			//RTPRINT(FINIT, INIT_IQK, ("Path B IQK failed!!\n"));		
		}
	}

	//Back to BB mode, load original value
	//RTPRINT(FINIT, INIT_IQK, ("IQK:Back to BB mode, load original value!\n"));

	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0);

	if(t!=0)
	{
		if(!pdmpriv->bRfPiEnable){
			// Switch back BB to SI mode after finish IQ Calibration.
			_PHY_PIModeSwitch(pAdapter, _FALSE);
		}

	 	// Reload ADDA power saving parameters
	 	_PHY_ReloadADDARegisters(pAdapter, ADDA_REG, pdmpriv->ADDA_backup, IQK_ADDA_REG_NUM);

		// Reload MAC parameters
		_PHY_ReloadMACRegisters(pAdapter, IQK_MAC_REG, pdmpriv->IQK_MAC_backup);		

		if(isNormal)
	 		_PHY_ReloadADDARegisters(pAdapter, IQK_BB_REG, pdmpriv->IQK_BB_backup, IQK_BB_REG_NUM);
		else
	 		_PHY_ReloadADDARegisters(pAdapter, IQK_BB_REG, pdmpriv->IQK_BB_backup, IQK_BB_REG_NUM-1);			

		// Restore RX initial gain
		if(isNormal)
			PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x00032fff);
		else	
			PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x00032ed3);
		
		if(is2T){
			if(isNormal)
				PHY_SetBBReg(pAdapter, rFPGA0_XB_LSSIParameter, bMaskDWord, 0x00032fff);	
			else	
				PHY_SetBBReg(pAdapter, rFPGA0_XB_LSSIParameter, bMaskDWord, 0x00032ed3);
		}

		//load 0xe30 IQC default value
		PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x01008c00);		
		PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x01008c00);				

	}
	//RTPRINT(FINIT, INIT_IQK, ("_PHY_IQCalibrate() <==\n"));

}


static VOID	
_PHY_IQCalibrate_5G(
	IN	PADAPTER	pAdapter,
	IN	int 		result[][8]
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u32			i, extPAon, REG0xe5c, RX0REG0xe40, REG0xe40, REG0xe94, REG0xe9c;
	u32			REG0xeac, RX1REG0xe40, REG0xeb4, REG0xebc, REG0xea4,REG0xec4;
	u8			TX0IQKOK = _FALSE, TX1IQKOK = _FALSE ;
	u32			TX_X0, TX_Y0, TX_X1, TX_Y1, RX_X0, RX_Y0, RX_X1, RX_Y1;
	u32			ADDA_REG[IQK_ADDA_REG_NUM] = 
					{	rFPGA0_XCD_SwitchControl, 0xe6c, 0xe70, 0xe74,
													0xe78, 0xe7c, 0xe80, 0xe84,
													0xe88, 0xe8c, 0xed0, 0xed4,
													0xed8, 0xedc, 0xee0, 0xeec };

	u32			IQK_MAC_REG[IQK_MAC_REG_NUM] = {
						0x522, 	0x550,	0x551,	0x040};			

	u32			IQK_BB_REG[IQK_BB_REG_NUM-2] = {	//for normal
						rFPGA0_XAB_RFInterfaceSW,	rOFDM0_TRMuxPar,	
						rFPGA0_XCD_RFInterfaceSW,	rOFDM0_TRxPathEnable,
						rFPGA0_RFMOD,			rFPGA0_AnalogParameter4					
					};

	BOOLEAN       		is2T =  IS_92D_SINGLEPHY(pHalData->VersionID);

	DBG_8192C("IQK for 5G:Start!!!interface %d\n", pHalData->interfaceIndex);

	DBG_8192C("IQ Calibration for %s\n", (is2T ? "2T2R" : "1T1R"));

	//Save MAC default value
	_PHY_SaveMACRegisters(pAdapter, IQK_MAC_REG, pdmpriv->IQK_MAC_backup);

 	//Save BB Parameter
	_PHY_SaveADDARegisters(pAdapter, IQK_BB_REG, pdmpriv->IQK_BB_backup, IQK_BB_REG_NUM-2);		

	//Save AFE Parameters
	_PHY_SaveADDARegisters(pAdapter, ADDA_REG, pdmpriv->ADDA_backup, IQK_ADDA_REG_NUM);

	//1 Path-A TX IQK
	//Path-A AFE all on
 	_PHY_PathADDAOn(pAdapter, ADDA_REG, _TRUE, _TRUE);

	//MAC register setting
	_PHY_MACSettingCalibration(pAdapter, IQK_MAC_REG, pdmpriv->IQK_MAC_backup);

	//IQK must be done in PI mode	
	pdmpriv->bRfPiEnable = (u8)PHY_QueryBBReg(pAdapter, rFPGA0_XA_HSSIParameter1, BIT(8));
	if(!pdmpriv->bRfPiEnable)
		_PHY_PIModeSwitch(pAdapter, _TRUE);

	//TXIQK RF setting
	PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x01940000);
	PHY_SetBBReg(pAdapter, rFPGA0_XB_LSSIParameter, bMaskDWord, 0x01940000);

	//BB setting
	PHY_SetBBReg(pAdapter, rOFDM0_TRxPathEnable, bMaskDWord, 0x03a05600);
	PHY_SetBBReg(pAdapter, rOFDM0_TRMuxPar, bMaskDWord, 0x000800e4);
	PHY_SetBBReg(pAdapter, rFPGA0_XCD_RFInterfaceSW, bMaskDWord, 0x22208000);
	PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFInterfaceSW, BIT6|BIT5,  0x03);
	PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFInterfaceSW, BIT22|BIT21,  0x03);
	PHY_SetBBReg(pAdapter, rFPGA0_AnalogParameter4, 0xf00000,  0x0f);

	//AP or IQK
	PHY_SetBBReg(pAdapter, 0xb68, bMaskDWord, 0x0f600000);
	PHY_SetBBReg(pAdapter, 0xb6c, bMaskDWord, 0x0f600000);

	//IQK global setting
	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x80800000);
	PHY_SetBBReg(pAdapter, 0xe40, bMaskDWord, 0x10007c00);
	PHY_SetBBReg(pAdapter, 0xe44, bMaskDWord, 0x01004800);

	//path-A IQK setting
	if(pHalData->interfaceIndex == 0)
	{
		PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x14008c1f);
		PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x30008c1f);
	}
	else
	{
		PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x14008c22);
		PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x30008c22);	
	}

	if(is2T)
		PHY_SetBBReg(pAdapter, 0xe38, bMaskDWord, 0x821402e2);
	else
		PHY_SetBBReg(pAdapter, 0xe38, bMaskDWord, 0x821402e6);	
	PHY_SetBBReg(pAdapter, 0xe3c, bMaskDWord, 0x68110000);

	//path-B IQK setting
	if(is2T)
	{
		PHY_SetBBReg(pAdapter, 0xe50, bMaskDWord, 0x14008c22);
		PHY_SetBBReg(pAdapter, 0xe54, bMaskDWord, 0x30008c22);
		PHY_SetBBReg(pAdapter, 0xe58, bMaskDWord, 0x82110000);
		PHY_SetBBReg(pAdapter, 0xe5c, bMaskDWord, 0x68110000);
	}

	//LO calibration setting
	PHY_SetBBReg(pAdapter, 0xe4c, bMaskDWord, 0x00462911);

	//RTPRINT(FINIT, INIT_IQK, ("0x522 %x\n", read8(pAdapter, 0x522)));

	//One shot, path A LOK & IQK
	PHY_SetBBReg(pAdapter, 0xe48, bMaskDWord, 0xf9000000);
	PHY_SetBBReg(pAdapter, 0xe48, bMaskDWord, 0xf8000000);

	//Delay 1 ms
	udelay_os(IQK_DELAY_TIME*1000);

	//Exit IQK mode
	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x00000000);

	//Check_TX_IQK_A_result()
	REG0xe40 = PHY_QueryBBReg(pAdapter, 0xe40, bMaskDWord);
	REG0xeac = PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord);
	REG0xe94 = PHY_QueryBBReg(pAdapter, 0xe94, bMaskDWord);

	if(((REG0xeac&BIT(28)) == 0) && (((REG0xe94&0x3FF0000)>>16)!=0x142))
	{		
		REG0xe9c = PHY_QueryBBReg(pAdapter, 0xe9c, bMaskDWord);
		TX_X0 = (PHY_QueryBBReg(pAdapter, 0xe94, bMaskDWord)&0x3FF0000)>>16;
		TX_Y0 = (PHY_QueryBBReg(pAdapter, 0xe9c, bMaskDWord)&0x3FF0000)>>16;
		RX0REG0xe40 =  0x80000000 | (REG0xe40 & 0xfc00fc00) | (TX_X0<<16) | TX_Y0;
		result[0][0] = TX_X0;
		result[0][1] = TX_Y0;
		TX0IQKOK = _TRUE;
		DBG_8192C("IQK for 5G: Path A TxOK interface %d\n", pHalData->interfaceIndex);
		
	}
	else
		DBG_8192C("IQK for 5G: Path A Tx Fail interface %d\n", pHalData->interfaceIndex);

	//1 path A RX IQK
	if(TX0IQKOK == _TRUE)
	{

		DBG_8192C("IQK for 5G: Path A Rx  START interface %d\n", pHalData->interfaceIndex);
	
		//TXIQK RF setting
		PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x01900000);
		PHY_SetBBReg(pAdapter, rFPGA0_XB_LSSIParameter, bMaskDWord, 0x01900000);

		//turn on external PA
		if(pHalData->interfaceIndex == 1)
			PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFParameter, BIT(30), 0x01);

		//IQK global setting
		PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x80800000);

		//path-A IQK setting
		if(pHalData->interfaceIndex == 0)
		{
			PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x14008c1f);
			PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x14008c1f);
		}
		else
		{
			PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x14008c22);
			PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x14008c22);	
		}
		PHY_SetBBReg(pAdapter, 0xe38, bMaskDWord, 0x82110000);
		if(pHalData->interfaceIndex == 0)
			PHY_SetBBReg(pAdapter, 0xe3c, bMaskDWord, (pHalData->CurrentChannel<=140)?0x68160c62:0x68160c66);
		else
			PHY_SetBBReg(pAdapter, 0xe3c, bMaskDWord, 0x68160962);

		//path-B IQK setting
		if(is2T)
		{
			PHY_SetBBReg(pAdapter, 0xe50, bMaskDWord, 0x14008c22);
			PHY_SetBBReg(pAdapter, 0xe54, bMaskDWord, 0x14008c22);
			PHY_SetBBReg(pAdapter, 0xe58, bMaskDWord, 0x82110000);
			PHY_SetBBReg(pAdapter, 0xe5c, bMaskDWord, 0x68110000);
		}

		//load TX0 IMR setting
		PHY_SetBBReg(pAdapter, 0xe40, bMaskDWord, RX0REG0xe40);
		//Sleep(5) -> delay 1ms
		udelay_os(IQK_DELAY_TIME*1000);

		//LO calibration setting
		PHY_SetBBReg(pAdapter, 0xe4c, bMaskDWord, 0x0046a911);

		//One shot, path A LOK & IQK
		PHY_SetBBReg(pAdapter, 0xe48, bMaskDWord, 0xf9000000);
		PHY_SetBBReg(pAdapter, 0xe48, bMaskDWord, 0xf8000000);

		//Delay 3 ms
		udelay_os(3*IQK_DELAY_TIME*1000);

		//Exit IQK mode
		PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x00000000);

		//Check_RX_IQK_A_result()
		REG0xeac = PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord);
		REG0xea4 = PHY_QueryBBReg(pAdapter, 0xea4, bMaskDWord);
		if(pHalData->interfaceIndex == 0)
		{
			if(((REG0xeac&BIT(27)) == 0) && (((REG0xea4&0x3FF0000)>>16)!=0x132))
			{
				RX_X0 =  (PHY_QueryBBReg(pAdapter, 0xea4, bMaskDWord)&0x3FF0000)>>16;
				RX_Y0 =  (PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord)&0x3FF0000)>>16;
				result[0][2] = RX_X0;
				result[0][3] = RX_Y0;
			}
		}
		else
		{
			if(((REG0xeac&BIT(30)) == 0) && (((REG0xea4&0x3FF0000)>>16)!=0x132))
			{
				RX_X0 =  (PHY_QueryBBReg(pAdapter, 0xea4, bMaskDWord)&0x3FF0000)>>16;
				RX_Y0 =  (PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord)&0x3FF0000)>>16;
				result[0][2] = RX_X0;
				result[0][3] = RX_Y0;
			}		
		}
	}

	if(!is2T)
		goto Exit_IQK;

	//1 path B TX IQK
	//Path-B AFE all on

	DBG_8192C("IQK for 5G: Path B Tx  START interface %d\n", pHalData->interfaceIndex);
	
 	_PHY_PathADDAOn(pAdapter, ADDA_REG, _FALSE, _TRUE);

	//TXIQK RF setting
	PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x01940000);
	PHY_SetBBReg(pAdapter, rFPGA0_XB_LSSIParameter, bMaskDWord, 0x01940000);

	//IQK global setting
	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x80800000);
	PHY_SetBBReg(pAdapter, 0xe40, bMaskDWord, 0x10007c00);

	//path-A IQK setting
	PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x14008c1f);
	PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x30008c1f);
	PHY_SetBBReg(pAdapter, 0xe38, bMaskDWord, 0x82110000);
	PHY_SetBBReg(pAdapter, 0xe3c, bMaskDWord, 0x68110000);

	//path-B IQK setting
	PHY_SetBBReg(pAdapter, 0xe50, bMaskDWord, 0x14008c22);
	PHY_SetBBReg(pAdapter, 0xe54, bMaskDWord, 0x30008c22);
	PHY_SetBBReg(pAdapter, 0xe58, bMaskDWord, 0x82140386);
	PHY_SetBBReg(pAdapter, 0xe5c, bMaskDWord, 0x68110000);

	//LO calibration setting
	PHY_SetBBReg(pAdapter, 0xe4c, bMaskDWord, 0x00462911);

	//One shot, path A LOK & IQK
	PHY_SetBBReg(pAdapter, 0xe60, bMaskDWord, 0x00000002);
	PHY_SetBBReg(pAdapter, 0xe60, bMaskDWord, 0x00000000);

	//Delay 1 ms
	udelay_os(IQK_DELAY_TIME*1000);

	//Exit IQK mode
	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x00000000);

	// Check_TX_IQK_B_result()
	REG0xe40 = PHY_QueryBBReg(pAdapter, 0xe40, bMaskDWord);
	REG0xeac = PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord);
	REG0xeb4 = PHY_QueryBBReg(pAdapter, 0xeb4, bMaskDWord);
	if(((REG0xeac&BIT(31)) == 0) && ((REG0xeb4&0x3FF0000)!=0x142))
	{
		TX_X1 = (PHY_QueryBBReg(pAdapter, 0xeb4, bMaskDWord)&0x3FF0000)>>16;
		TX_Y1 = (PHY_QueryBBReg(pAdapter, 0xebc, bMaskDWord)&0x3FF0000)>>16;
		RX1REG0xe40 = 0x80000000 | (REG0xe40 & 0xfc00fc00) | (TX_X1<<16) | TX_Y1;
		result[0][4] = TX_X1;
		result[0][5] = TX_Y1;
		TX1IQKOK = _TRUE;
	}

	//1 path B RX IQK
	if(TX1IQKOK == _TRUE)
	{

		DBG_8192C("IQK for 5G: Path B Rx  START interface %d\n", pHalData->interfaceIndex);
	
		if(pHalData->CurrentChannel<=140)
		{
			REG0xe5c = 0x68160960;
			extPAon = 0x1;
		}	
		else
		{
			REG0xe5c = 0x68150c66;
	   		extPAon = 0x0;
		}

		//TXIQK RF setting
		PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x01900000);
		PHY_SetBBReg(pAdapter, rFPGA0_XB_LSSIParameter, bMaskDWord, 0x01900000);

		//turn on external PA
		PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFParameter, BIT(30), extPAon);

		//BB setting
		PHY_SetBBReg(pAdapter, rFPGA0_AnalogParameter4, bMaskDWord, 0xcc300080);

		//IQK global setting
		PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x80800000);

		//path-A IQK setting
		PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x14008c1f);
		PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x34008c1f);
		PHY_SetBBReg(pAdapter, 0xe38, bMaskDWord, 0x82110000);
		PHY_SetBBReg(pAdapter, 0xe3c, bMaskDWord, 0x68110000);

		//path-B IQK setting
		PHY_SetBBReg(pAdapter, 0xe50, bMaskDWord, 0x14008c22);
		PHY_SetBBReg(pAdapter, 0xe54, bMaskDWord, 0x14008c22);
		PHY_SetBBReg(pAdapter, 0xe58, bMaskDWord, 0x82110000);
		PHY_SetBBReg(pAdapter, 0xe5c, bMaskDWord, REG0xe5c);

		//load TX0 IMR setting
		PHY_SetBBReg(pAdapter, 0xe40, bMaskDWord, RX1REG0xe40);

		//Sleep(5) -> delay 1ms
		udelay_os(IQK_DELAY_TIME*1000);

		//LO calibration setting
		PHY_SetBBReg(pAdapter, 0xe4c, bMaskDWord, 0x0046a911);

		//One shot, path A LOK & IQK
		PHY_SetBBReg(pAdapter, 0xe60, bMaskDWord, 0x00000002);
		PHY_SetBBReg(pAdapter, 0xe60, bMaskDWord, 0x00000000);

		//Delay 1 ms
		udelay_os(3*IQK_DELAY_TIME*1000);

		//Check_RX_IQK_B_result()
		REG0xeac = PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord);
		REG0xec4 = PHY_QueryBBReg(pAdapter, 0xec4, bMaskDWord);
		if(((REG0xeac&BIT(30)) == 0) && (((REG0xec4&0x3FF0000)>>16)!=0x132))
		{
			RX_X1 =  (PHY_QueryBBReg(pAdapter, 0xec4, bMaskDWord)&0x3FF0000)>>16;
			RX_Y1 =  (PHY_QueryBBReg(pAdapter, 0xecc, bMaskDWord)&0x3FF0000)>>16;
			result[0][6] = RX_X1;
			result[0][7] = RX_Y1;
		}
	}

Exit_IQK:
	//turn off external PA
	if(pHalData->interfaceIndex == 1 || is2T)
		PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFParameter, BIT(30), 0);

	//Exit IQK mode
	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x00000000);	
	_PHY_ReloadADDARegisters(pAdapter, IQK_BB_REG, pdmpriv->IQK_BB_backup, IQK_BB_REG_NUM-2);
	
	PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x01900000);
	PHY_SetBBReg(pAdapter, rFPGA0_XB_LSSIParameter, bMaskDWord, 0x01900000);	
	PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x00032fff);
	PHY_SetBBReg(pAdapter, rFPGA0_XB_LSSIParameter, bMaskDWord, 0x00032fff);


	//reload MAC default value	
	_PHY_ReloadMACRegisters(pAdapter, IQK_MAC_REG, pdmpriv->IQK_MAC_backup);
	
	if(!pdmpriv->bRfPiEnable)
		_PHY_PIModeSwitch(pAdapter, _FALSE);
	//Reload ADDA power saving parameters
	_PHY_ReloadADDARegisters(pAdapter, ADDA_REG, pdmpriv->ADDA_backup, IQK_ADDA_REG_NUM);
	
}


VOID	
_PHY_IQCalibrate_5G_Normal(
	IN	PADAPTER	pAdapter,
	IN	int 		result[][8],
	IN	u8		t
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u32			i = 0;
	u32			PathAOK, PathBOK;
	u32			ADDA_REG[IQK_ADDA_REG_NUM] = {	
						rFPGA0_XCD_SwitchControl, 	0xe6c, 	0xe70, 	0xe74,
						0xe78, 	0xe7c, 	0xe80, 	0xe84,
						0xe88, 	0xe8c, 	0xed0, 	0xed4,
						0xed8, 	0xedc, 	0xee0, 	0xeec };
	u32			IQK_MAC_REG[IQK_MAC_REG_NUM] = {
						0x522, 0x550,	0x551,	0x040};

	u32			IQK_BB_REG[IQK_BB_REG_NUM] = {	//for normal
						rFPGA0_XAB_RFInterfaceSW,	rFPGA0_XA_RFInterfaceOE,	
						rFPGA0_XB_RFInterfaceOE,	rOFDM0_TRMuxPar,
						rFPGA0_XCD_RFInterfaceSW,	rOFDM0_TRxPathEnable,	
						rFPGA0_RFMOD,			rFPGA0_AnalogParameter4					
					};							

	// Note: IQ calibration must be performed after loading 
	// 		PHY_REG.txt , and radio_a, radio_b.txt	
	
	u32	bbvalue;
	BOOLEAN		is2T =  IS_92D_SINGLEPHY(pHalData->VersionID);

	//RTPRINT(FINIT, INIT_IQK, ("IQK for 5G NORMAL:Start!!! interface %d\n", pAdapter->interfaceIndex));

	//udelay_os(IQK_DELAY_TIME*1000*100);	//delay after set IMR
	
	udelay_os(IQK_DELAY_TIME*1000*20);

	if(t==0)
	{
		bbvalue = PHY_QueryBBReg(pAdapter, rFPGA0_RFMOD, bMaskDWord);
		//RTPRINT(FINIT, INIT_IQK, ("PHY_IQCalibrate()==>0x%08lx\n",bbvalue));
		//RTPRINT(FINIT, INIT_IQK, ("IQ Calibration for %s\n", (is2T ? "2T2R" : "1T1R")));

	 	// Save ADDA parameters, turn Path A ADDA on
		_PHY_SaveADDARegisters(pAdapter, ADDA_REG, pdmpriv->ADDA_backup, IQK_ADDA_REG_NUM);
		_PHY_SaveMACRegisters(pAdapter, IQK_MAC_REG, pdmpriv->IQK_MAC_backup);
		_PHY_SaveADDARegisters(pAdapter, IQK_BB_REG, pdmpriv->IQK_BB_backup, IQK_BB_REG_NUM);
	}
	
 	_PHY_PathADDAOn(pAdapter, ADDA_REG, _TRUE, is2T);

	//MAC settings
	_PHY_MACSettingCalibration(pAdapter, IQK_MAC_REG, pdmpriv->IQK_MAC_backup);
		
	if(t==0)
	{
		pdmpriv->bRfPiEnable = (u8)PHY_QueryBBReg(pAdapter, rFPGA0_XA_HSSIParameter1, BIT(8));
	}
	
	if(!pdmpriv->bRfPiEnable){
		// Switch BB to PI mode to do IQ Calibration.
		_PHY_PIModeSwitch(pAdapter, _TRUE);
	}

	PHY_SetBBReg(pAdapter, rFPGA0_RFMOD, BIT24, 0x00);			
	PHY_SetBBReg(pAdapter, rOFDM0_TRxPathEnable, bMaskDWord, 0x03a05600);
	PHY_SetBBReg(pAdapter, rOFDM0_TRMuxPar, bMaskDWord, 0x000800e4);
	PHY_SetBBReg(pAdapter, rFPGA0_XCD_RFInterfaceSW, bMaskDWord, 0x22208000);	
	PHY_SetBBReg(pAdapter, rFPGA0_AnalogParameter4, 0xf00000, 0x0f); 
			
	//Page B init
	PHY_SetBBReg(pAdapter, 0xb68, bMaskDWord, 0x0f600000);
	
	if(is2T)
	{
		PHY_SetBBReg(pAdapter, 0xb6c, bMaskDWord, 0x0f600000);
	}
	
	// IQ calibration setting
	//RTPRINT(FINIT, INIT_IQK, ("IQK setting!\n"));		
	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x80800000);
	PHY_SetBBReg(pAdapter, 0xe40, bMaskDWord, 0x01007c00);
	PHY_SetBBReg(pAdapter, 0xe44, bMaskDWord, 0x01004800);

	{
		PathAOK = _PHY_PathA_IQK_5G_Normal(pAdapter, is2T);
		if(PathAOK == 0x03){
			DBG_8192C("Path A IQK Success!!\n");
			result[t][0] = (PHY_QueryBBReg(pAdapter, 0xe94, bMaskDWord)&0x3FF0000)>>16;
			result[t][1] = (PHY_QueryBBReg(pAdapter, 0xe9c, bMaskDWord)&0x3FF0000)>>16;
			result[t][2] = (PHY_QueryBBReg(pAdapter, 0xea4, bMaskDWord)&0x3FF0000)>>16;
			result[t][3] = (PHY_QueryBBReg(pAdapter, 0xeac, bMaskDWord)&0x3FF0000)>>16;
		}
		else if (PathAOK == 0x01)	//Tx IQK OK
		{
			DBG_8192C("Path A IQK Only  Tx Success!!\n");
			
			result[t][0] = (PHY_QueryBBReg(pAdapter, 0xe94, bMaskDWord)&0x3FF0000)>>16;
			result[t][1] = (PHY_QueryBBReg(pAdapter, 0xe9c, bMaskDWord)&0x3FF0000)>>16;			
		}
		else
		{
			DBG_8192C("Path A IQK Fail!!\n");
		}
	}

	if(is2T){
		//_PHY_PathAStandBy(pAdapter);

		// Turn Path B ADDA on
		_PHY_PathADDAOn(pAdapter, ADDA_REG, _FALSE, is2T);

		{
			PathBOK = _PHY_PathB_IQK_5G_Normal(pAdapter);
			if(PathBOK == 0x03){
				DBG_8192C("Path B IQK Success!!\n");
				result[t][4] = (PHY_QueryBBReg(pAdapter, 0xeb4, bMaskDWord)&0x3FF0000)>>16;
				result[t][5] = (PHY_QueryBBReg(pAdapter, 0xebc, bMaskDWord)&0x3FF0000)>>16;
				result[t][6] = (PHY_QueryBBReg(pAdapter, 0xec4, bMaskDWord)&0x3FF0000)>>16;
				result[t][7] = (PHY_QueryBBReg(pAdapter, 0xecc, bMaskDWord)&0x3FF0000)>>16;
			}
			else if (PathBOK == 0x01)	//Tx IQK OK
			{
				DBG_8192C("Path B Only Tx IQK Success!!\n");
				result[t][4] = (PHY_QueryBBReg(pAdapter, 0xeb4, bMaskDWord)&0x3FF0000)>>16;
				result[t][5] = (PHY_QueryBBReg(pAdapter, 0xebc, bMaskDWord)&0x3FF0000)>>16;				
			}
			else{		
				DBG_8192C("Path B IQK failed!!\n");
			}			
		}
	}
	
	//Back to BB mode, load original value
	//RTPRINT(FINIT, INIT_IQK, ("IQK:Back to BB mode, load original value!\n"));
	PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0);
	
	if(t!=0)
	{
	 	_PHY_ReloadADDARegisters(pAdapter, IQK_BB_REG, pdmpriv->IQK_BB_backup, IQK_BB_REG_NUM);

		// Restore RX initial gain
		PHY_SetBBReg(pAdapter, rFPGA0_XA_LSSIParameter, bMaskDWord, 0x00032fff);
		
		if(is2T)
			PHY_SetBBReg(pAdapter, rFPGA0_XB_LSSIParameter, bMaskDWord, 0x00032fff);	

		// Reload MAC parameters
		_PHY_ReloadMACRegisters(pAdapter, IQK_MAC_REG, pdmpriv->IQK_MAC_backup);		
		
		if(!pdmpriv->bRfPiEnable){
			// Switch back BB to SI mode after finish IQ Calibration.
			_PHY_PIModeSwitch(pAdapter, _FALSE);
		}

	 	// Reload ADDA power saving parameters
	 	_PHY_ReloadADDARegisters(pAdapter, ADDA_REG, pdmpriv->ADDA_backup, IQK_ADDA_REG_NUM);

		//load 0xe30 IQC default value
		//PHY_SetBBReg(pAdapter, 0xe30, bMaskDWord, 0x01008c00);
		//PHY_SetBBReg(pAdapter, 0xe34, bMaskDWord, 0x01008c00);

	}
	//RTPRINT(FINIT, INIT_IQK, ("_PHY_IQCalibrate_5G_Normal() <==\n"));
	
}


static VOID	
_PHY_LCCalibrate92D(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		is2T
	)
{
	u8	tmpReg, index = 0;
	u32 	RF_mode[2],LC_Cal, tmpu4Byte[2];
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	BOOLEAN	isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	u8	path = is2T?2:1;
	u16	timeout = 800, timecount = 0;	

	//Check continuous TX and Packet TX
	tmpReg = read8(pAdapter, 0xd03);

	if((tmpReg&0x70) != 0)			//Deal with contisuous TX case
		write8(pAdapter, 0xd03, tmpReg&0x8F);	//disable all continuous TX
	else 							// Deal with Packet TX case
		write8(pAdapter, REG_TXPAUSE, 0xFF);			// block all queues

	PHY_SetBBReg(pAdapter, rFPGA0_AnalogParameter4, 0xF00000, 0x0F);

	for(index = 0; index <path; index ++)
	{
		//1. Read original RF mode
		RF_mode[index] = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_AC, bRFRegOffsetMask);
		
		//2. Set RF mode = standby mode
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_AC, 0x70000, 0x01);

		tmpu4Byte[index] = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G4, bRFRegOffsetMask);
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G4, 0x700, 0x07);			

		//4. Set LC calibration begin
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_CHNLBW, 0x08000, 0x01);
		
	}
		
	if(isNormal)
		mdelay_os(100);
	else
		mdelay_os(3);

	//Restore original situation	
	for(index = 0; index <path; index ++)
	{
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G4, bRFRegOffsetMask, tmpu4Byte[index]);					
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_AC, bRFRegOffsetMask, RF_mode[index]);	
	}

	if((tmpReg&0x70) != 0)	
	{  
		//Path-A
		write8(pAdapter, 0xd03, tmpReg);
	}
	else // Deal with Packet TX case
	{
		write8(pAdapter, REG_TXPAUSE, 0x00);	
	}

	PHY_SetBBReg(pAdapter, rFPGA0_AnalogParameter4, 0xF00000, 0x00);
	
}

#if 0
static u32
get_abs(
	IN	u32	val1,
	IN	u32	val2
	)
{
	u32 ret=0;
	
	if(val1 >= val2)
	{
		ret = val1 - val2;
	}
	else
	{
		ret = val2 - val1;
	}
	return ret;
}

#define	TESTFLAG		1
#define	TESTDelay	20

static VOID
_PHY_CalcCurvIndex(
	IN	PADAPTER	pAdapter,
	IN	u32*		TargetChnl,
	IN	u32*		CurveCountVal,	
	IN	BOOLEAN		is5G,				
	OUT	u32*		CurveIndex
	)
{
	u32	smallestABSVal = 0xffffffff, u4tmp;
	u8	i, j;
	u8	chnl_num = is5G?TARGET_CHNL_NUM_5G:TARGET_CHNL_NUM_2G;
	
	
	for(i=0; i<chnl_num; i++)
	{
		if(is5G && !IsLegal5GChannel(pAdapter, i+1))
			continue;

		CurveIndex[i] = 0;
		for(j=0; j<(CV_CURVE_CNT*2); j++)
		{
			u4tmp = get_abs(TargetChnl[i], CurveCountVal[j]);

#if 0	
			if(i == 0)
			{
				RTPRINT(FINIT, INIT_IQK,("i=%d, j=%d, TargetChnl[%d]=%d, curveCountVal[%d]=%d, u4tmp=%d\n", 
					i,j,i, TargetChnl[i],j, CurveCountVal[j], u4tmp));
			}
#endif			
			
			if(u4tmp < smallestABSVal)
			{
				CurveIndex[i] = j;						
				smallestABSVal = u4tmp;
			}
		}
		smallestABSVal = 0xffffffff;
		if(i == 0)
			DBG_8192C("cosa, CurveIndex[%d] = %d\n", i, CurveIndex[i]);
	}

}

static VOID	
_PHY_LCCalibrate92DSW(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		is2T
	)
{
	u8	tmpReg, index = 0;
	u32 	RF_mode[2],LC_Cal, tmpu4Byte[2];
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	BOOLEAN	isNormal = IS_NORMAL_CHIP(pHalData->VersionID);
	u8	path = is2T?2:1;
	u32	i, j, smallestABSVal, u4tmp;
	u32	curveCountVal[CV_CURVE_CNT*2]={0};
	u16	timeout = 800, timecount = 0;

	//Check continuous TX and Packet TX
	tmpReg = read8(pAdapter, 0xd03);

	if((tmpReg&0x70) != 0)			//Deal with contisuous TX case
		write8(pAdapter, 0xd03, tmpReg&0x8F);	//disable all continuous TX
	else 							// Deal with Packet TX case
		write8(pAdapter, REG_TXPAUSE, 0xFF);			// block all queues

	PHY_SetBBReg(pAdapter, rFPGA0_AnalogParameter4, 0xF00000, 0x0F);
//	RTPRINT(FINIT, INIT_IQK,("cosa test for curve-cnt\n"));
	for(index = 0; index <path; index ++)
	{
		//1. Read original RF mode
		RF_mode[index] = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_AC, bRFRegOffsetMask);
		
		//2. Set RF mode = standby mode
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_AC, 0x70000, 0x01);
#if (TESTFLAG == 0)
		tmpu4Byte[index] = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G4, bRFRegOffsetMask);
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G4, 0x700, 0x07);			
#endif
		// switch CV-curve control by LC-calibration
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G7, BIT17, 0x0);
		
		//4. Set LC calibration begin
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_CHNLBW, 0x08000, 0x01);
		
		if(isNormal)
		{
			u4tmp = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G6, bRFRegOffsetMask);
//			RTPRINT(FINIT, INIT_IQK,("PHY_LCK RF0x2a for 0x%x \n", u4tmp));
			
//			while((!(PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G6, BIT11))) &&
			while((!(u4tmp & BIT11)) &&
				timecount <= timeout)
			{
				
				mdelay_os(50);		
				timecount += 50;
//				RTPRINT(FINIT, INIT_IQK,("PHY_LCK delay for %d ms=2\n", timecount));
				
				u4tmp = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G6, bRFRegOffsetMask);
//				RTPRINT(FINIT, INIT_IQK,("PHY_LCK RF0x2a for 0x%x ~bit11 %d time bound %d\n", u4tmp, (!(u4tmp & BIT11)), timecount <= timeout));
				
			}	
			//RTPRINT(FINIT, INIT_IQK,("PHY_LCK finish delay for %d ms=2\n", timecount));
			
		}
		else
			mdelay_os(3);

		u4tmp = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G4, bRFRegOffsetMask);
//			RTPRINT(FINIT, INIT_IQK,("cosa RF 0x28 = 0x%x\n", u4tmp));
		// Only path-A 5G need to do this
//		if(index == 0)
		{
			u32 test=0;

			if(index == 0 && pHalData->interfaceIndex == 0)
			{
				//RTPRINT(FINIT, INIT_IQK,("cosa, path-A / 5G LCK\n"));
			}
			else
			{
				//RTPRINT(FINIT, INIT_IQK,("cosa, path-B / 2.4G LCK\n"));				
			}
			_memset(&curveCountVal[0], 0, CV_CURVE_CNT*2);
			
//			test = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, 0x18, bRFRegOffsetMask);
//			RTPRINT(FINIT, INIT_IQK,("cosa, RF 0x18 = 0x%x\n", test));
			
			//Set LC calibration off
			PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_CHNLBW, 0x08000, 0x0);
			//RTPRINT(FINIT, INIT_IQK,("cosa, set RF 0x18[15] = 0\n"));

			//save Curve-counting number
			for(i=0; i<CV_CURVE_CNT; i++)
			{
				u32	readVal=0, readVal2=0;
				PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, 0x3F, 0x7f, i);
//				RTPRINT(FINIT, INIT_IQK,("cosa, set RF 0x3F[7:0] = 0\n"));
				
//				test = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, 0x3F, bRFRegOffsetMask);
//				RTPRINT(FINIT, INIT_IQK,("cosa, RF 0x3f = 0x%x\n", test));
				PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, 0x4D, bRFRegOffsetMask, 0x0);
//				delay_ms(TESTDelay);
				readVal = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, 0x4F, bRFRegOffsetMask);
//				RTPRINT(FINIT, INIT_IQK,("cosa 1, RF 0x4F[19:0] = 0x%x\n", readVal));				

				curveCountVal[2*i+1] = (readVal & 0xfffe0) >> 5;
				// reg 0x4f [4:0]
				// reg 0x50 [19:10]
				readVal2 = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)index, 0x50, 0xffc00);
//				RTPRINT(FINIT, INIT_IQK,("cosa 5, RF 0x50[19:10] = 0x%x\n", readVal2));									
				curveCountVal[2*i] = (((readVal & 0x1F) << 10) | readVal2);
#if 0				
//				if(i == 0)
				{

					RTPRINT(FINIT, INIT_IQK,("cosa 2, curveCountVal[%d] = 0x%x\n", 2*i+1, curveCountVal[2*i+1]));				
					RTPRINT(FINIT, INIT_IQK,("cosa 6, curveCountVal[%d] = 0x%x\n", 2*i, curveCountVal[2*i]));
				}
#endif
			}

			if(index == 0 && pHalData->interfaceIndex == 0)
				_PHY_CalcCurvIndex(pAdapter, TargetChnl_5G, curveCountVal, _TRUE, CurveIndex_5G);
			else 
				_PHY_CalcCurvIndex(pAdapter, TargetChnl_2G, curveCountVal, _FALSE, CurveIndex_2G);

			// switch CV-curve control mode
			PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G7, BIT17, 0x1);
		}
		
	}
	
	//Restore original situation	
	for(index = 0; index <path; index ++)
	{
#if (TESTFLAG == 0)
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_SYN_G4, bRFRegOffsetMask, tmpu4Byte[index]);					
#endif		
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)index, RF_AC, bRFRegOffsetMask, RF_mode[index]);	
	}

	if((tmpReg&0x70) != 0)	
	{  
		//Path-A
		write8(pAdapter, 0xd03, tmpReg);
	}
	else // Deal with Packet TX case
	{
		write8(pAdapter, REG_TXPAUSE, 0x00);	
	}

	PHY_SetBBReg(pAdapter, rFPGA0_AnalogParameter4, 0xF00000, 0x00);

	phy_ReloadLCKSetting(pAdapter, pHalData->CurrentChannel);	
	
}
#endif

VOID	
_PHY_LCCalibrate(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		is2T
	)
{
	_PHY_LCCalibrate92D(pAdapter, is2T);
}


//Analog Pre-distortion calibration
#define		APK_BB_REG_NUM	5
#define		APK_AFE_REG_NUM	16
#define		APK_CURVE_REG_NUM 4
#define		PATH_NUM		2

static VOID
_PHY_APCalibrate(
	IN	PADAPTER	pAdapter,
	IN	char 		delta,
	IN	BOOLEAN		is2T
	)
{
#if 1//(PLATFORM == PLATFORM_WINDOWS)//???
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	u32 			regD[PATH_NUM];
	u32			tmpReg, index, offset, path, i, pathbound = PATH_NUM;
			
	u32			BB_backup[APK_BB_REG_NUM];
	u32			BB_REG[APK_BB_REG_NUM] = {	
						0x904, rOFDM0_TRxPathEnable, rFPGA0_RFMOD, rOFDM0_TRMuxPar, rFPGA0_XCD_RFInterfaceSW };
	u32			BB_AP_MODE[APK_BB_REG_NUM] = {	
						0x00000020, 0x00a05430, 0x02040000, 
						0x000800e4, 0x00204000 };
	u32			BB_normal_AP_MODE[APK_BB_REG_NUM] = {	
						0x00000020, 0x00a05430, 0x02040000, 
						0x000800e4, 0x22204000 };						

	u32			AFE_backup[APK_AFE_REG_NUM];
	u32			AFE_REG[APK_AFE_REG_NUM] = {	
						0x85c, 0xe6c, 0xe70, 0xe74, 0xe78, 
						0xe7c, 0xe80, 0xe84, 0xe88, 0xe8c, 
						0xed0, 0xed4, 0xed8, 0xedc, 0xee0,
						0xeec};

	u32			APK_RF_init_value[PATH_NUM][APK_BB_REG_NUM] = {
					{0x0852c, 0x1852c, 0x5852c, 0x1852c, 0x5852c},
					{0x2852e, 0x0852e, 0x3852e, 0x0852e, 0x0852e}
					};	

	u32			APK_normal_RF_init_value[PATH_NUM][APK_BB_REG_NUM] = {
					{0x0852c, 0x3852c, 0x0852c, 0x0852c, 0x4852c},
					{0x2852e, 0x0852e, 0x3852e, 0x0852e, 0x0852e}
					};
	
	u32			APK_RF_value_0[PATH_NUM][APK_BB_REG_NUM] = {
					{0x52019, 0x52014, 0x52013, 0x5200f, 0x5208d},
					{0x5201a, 0x52019, 0x52016, 0x52033, 0x52050}
					};

	u32			APK_normal_RF_value_0[PATH_NUM][APK_BB_REG_NUM] = {
					{0x52019, 0x52017, 0x52013, 0x52010, 0x5200d},
					{0x5201a, 0x52019, 0x52016, 0x52033, 0x52050}
					};
	
	u32			APK_RF_value_A[PATH_NUM][APK_BB_REG_NUM] = {
					{0x1adb0, 0x1adb0, 0x1ada0, 0x1ad90, 0x1ad80},		
					{0x00fb0, 0x00fb0, 0x00fa0, 0x00f90, 0x00f80}						
					};

	u32			AFE_on_off[PATH_NUM] = {
					0x04db25a4, 0x0b1b25a4};	//path A on path B off / path A off path B on

	u32			APK_offset[PATH_NUM] = {
					0xb68, 0xb6c};

	u32			APK_normal_offset[PATH_NUM] = {
					0xb28, 0xb98};
					
	u32			APK_value[PATH_NUM] = {
					0x92fc0000, 0x12fc0000};					

	u32			APK_normal_value[PATH_NUM] = {
					0x92680000, 0x12680000};					

	char			APK_delta_mapping[APK_BB_REG_NUM][13] = {
					{-4, -3, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},
					{-4, -3, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},											
					{-6, -4, -2, -2, -1, -1, 0, 1, 2, 3, 4, 5, 6},
					{-1, -1, -1, -1, -1, -1, 0, 1, 2, 3, 4, 5, 6},
					{-11, -9, -7, -5, -3, -1, 0, 0, 0, 0, 0, 0, 0}
					};
	
	u32			APK_normal_setting_value_1[13] = {
					0x01017018, 0xf7ed8f84, 0x40372d20, 0x5b554e48, 0x6f6a6560,
					0x807c7873, 0x8f8b8884, 0x9d999693, 0xa9a6a3a0, 0xb5b2afac,
					0x12680000, 0x00880000, 0x00880000
					};

	u32			APK_normal_setting_value_2[16] = {
					0x00810100, 0x00400056, 0x002b0032, 0x001f0024, 0x0019001c,
					0x00150017, 0x00120013, 0x00100011, 0x000e000f, 0x000c000d,
					0x000b000c, 0x000a000b, 0x0009000a, 0x00090009, 0x00080008,
					0x00080008
					};
	
	u32			APK_result[PATH_NUM][APK_BB_REG_NUM];	//val_1_1a, val_1_2a, val_2a, val_3a, val_4a
	u32			AP_curve[PATH_NUM][APK_CURVE_REG_NUM];

	int			BB_offset, delta_V, delta_offset;

	BOOLEAN			isNormal = IS_NORMAL_CHIP(pHalData->VersionID);

#if (MP_DRIVER == 1)
	PMPT_CONTEXT	pMptCtx = &(pAdapter->MptCtx);	

	pMptCtx->APK_bound[0] = 45;
	pMptCtx->APK_bound[1] = 52;		
#endif

	//RTPRINT(FINIT, INIT_IQK, ("==>PHY_APCalibrate() delta %d\n", delta));
	
	//RTPRINT(FINIT, INIT_IQK, ("AP Calibration for %s %s\n", (is2T ? "2T2R" : "1T1R"), (isNormal ? "Normal chip" : "Test chip")));

	if(!is2T)
		pathbound = 1;

	if(isNormal)
	{
#if (MP_DRIVER != 1)
		return;
#endif
	
		//settings adjust for normal chip
		for(index = 0; index < PATH_NUM; index ++)
		{
 			APK_offset[index] = APK_normal_offset[index];
			APK_value[index] = APK_normal_value[index];
			AFE_on_off[index] = 0x6fdb25a4;
		}

		for(index = 0; index < APK_BB_REG_NUM; index ++)
		{
			APK_RF_init_value[0][index] = APK_normal_RF_init_value[0][index];
			APK_RF_value_0[0][index] = APK_normal_RF_value_0[0][index];
			BB_AP_MODE[index] = BB_normal_AP_MODE[index];
		}

		//path A APK
		//load APK setting
		//path-A		
		offset = 0xb00;
		for(index = 0; index < 11; index ++)			
		{
			PHY_SetBBReg(pAdapter, offset, bMaskDWord, APK_normal_setting_value_1[index]);
			//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", offset, PHY_QueryBBReg(pAdapter, offset, bMaskDWord))); 	
			
			offset += 0x04;
		}

		PHY_SetBBReg(pAdapter, 0xb98, bMaskDWord, 0x12680000);

		offset = 0xb68;
		for(; index < 13; index ++)			
		{
			PHY_SetBBReg(pAdapter, offset, bMaskDWord, APK_normal_setting_value_1[index]);
			//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", offset, PHY_QueryBBReg(pAdapter, offset, bMaskDWord))); 	
			
			offset += 0x04;
		}	

		//page-B1
		PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x40000000);

		offset = 0xb00;
		for(index = 0; index < 16; index++)
		{
			PHY_SetBBReg(pAdapter, offset, bMaskDWord, APK_normal_setting_value_2[index]);		
			//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", offset, PHY_QueryBBReg(pAdapter, offset, bMaskDWord))); 	
			
			offset += 0x04;
		}

		PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x00000000);
		
			
	}
	else
	{
		PHY_SetBBReg(pAdapter, 0xb68, bMaskDWord, 0x0fe00000);
		if(is2T)
			PHY_SetBBReg(pAdapter, 0xb68, bMaskDWord, 0x0fe00000);
	}
	
	//save BB default value													
	for(index = 0; index < APK_BB_REG_NUM ; index++)
		BB_backup[index] = PHY_QueryBBReg(pAdapter, BB_REG[index], bMaskDWord);

	//save AFE default value
	for(index = 0; index < APK_AFE_REG_NUM ; index++)
		AFE_backup[index] = PHY_QueryBBReg(pAdapter, AFE_REG[index], bMaskDWord);

	for(path = 0; path < pathbound; path++)
	{
		//save old AP curve													
		if(isNormal)
		{
			tmpReg = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0x3, bRFRegOffsetMask);
			AP_curve[path][0] = tmpReg & 0x1F;				//[4:0]

			tmpReg = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0x4, bRFRegOffsetMask);			
			AP_curve[path][1] = (tmpReg & 0xF8000) >> 15; 	//[19:15]						
			AP_curve[path][2] = (tmpReg & 0x7C00) >> 10;	//[14:10]
			AP_curve[path][3] = (tmpReg & 0x3E0) >> 5;		//[9:5]			
		}
		else
		{
			tmpReg = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xe, bRFRegOffsetMask);
		
			AP_curve[path][0] = (tmpReg & 0xF8000) >> 15; 	//[19:15]			
			AP_curve[path][1] = (tmpReg & 0x7C00) >> 10;	//[14:10]
			AP_curve[path][2] = (tmpReg & 0x3E0) >> 5;		//[9:5]
			AP_curve[path][3] = tmpReg & 0x1F;				//[4:0]
		}
		
		//save RF default value
		regD[path] = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xd, bRFRegOffsetMask);
		
		//Path A AFE all on, path B AFE All off or vise versa
		for(index = 0; index < APK_AFE_REG_NUM ; index++)
			PHY_SetBBReg(pAdapter, AFE_REG[index], bMaskDWord, AFE_on_off[path]);
		//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0xe70 %x\n", PHY_QueryBBReg(pAdapter, 0xe70, bMaskDWord)));		

		//BB to AP mode
		if(path == 0)
		{
			for(index = 0; index < APK_BB_REG_NUM ; index++)
				PHY_SetBBReg(pAdapter, BB_REG[index], bMaskDWord, BB_AP_MODE[index]);
		}

		//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x800 %x\n", PHY_QueryBBReg(pAdapter, 0x800, bMaskDWord)));				
		
		if(path == 0)	//Path B to standby mode
			PHY_SetRFReg(pAdapter, RF90_PATH_B, 0x0, bRFRegOffsetMask, 0x10000);			
		else			//Path A to standby mode
			PHY_SetRFReg(pAdapter, RF90_PATH_A, 0x0, bRFRegOffsetMask, 0x10000);			

		delta_offset = ((delta+14)/2);
		if(delta_offset < 0)
			delta_offset = 0;
		else if (delta_offset > 12)
			delta_offset = 12;
			
		//AP calibration
		for(index = 0; index < APK_BB_REG_NUM; index++)
		{
	
			if(index == 0 && isNormal)		//skip 
				continue;
					
			tmpReg = APK_RF_init_value[path][index];
#if 1
			if(!pAdapter->eeprompriv.bautoload_fail_flag)
			{
				BB_offset = (tmpReg & 0xF0000) >> 16;

				if(!(tmpReg & BIT15)) //sign bit 0
				{
					BB_offset = -BB_offset;
				}

				delta_V = APK_delta_mapping[index][delta_offset];
				
				BB_offset += delta_V;

				//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() APK num %d delta_V %d delta_offset %d\n", index, delta_V, delta_offset));		
				
				if(BB_offset < 0)
				{
					tmpReg = tmpReg & (~BIT15);
					BB_offset = -BB_offset;
				}
				else
				{
					tmpReg = tmpReg | BIT15;
				}
				tmpReg = (tmpReg & 0xFFF0FFFF) | (BB_offset << 16);
			}
#endif
			PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0x0, bRFRegOffsetMask, APK_RF_value_0[path][index]);
			//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x0 %x\n", PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0x0, bRFRegOffsetMask)));		
			PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xd, bRFRegOffsetMask, tmpReg);
			//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0xd %x\n", PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xd, bRFRegOffsetMask)));					
			if(!isNormal)
			{
				PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xa, bRFRegOffsetMask, APK_RF_value_A[path][index]);
				//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0xa %x\n", PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xa, bRFRegOffsetMask)));					
			}
			
			// PA11+PAD01111, one shot	
			i = 0;
			do
			{
				PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x80000000);
//				if(index == 4)
				{
					PHY_SetBBReg(pAdapter, APK_offset[path], bMaskDWord, APK_value[0]);		
					//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", APK_offset[path], PHY_QueryBBReg(pAdapter, APK_offset[path], bMaskDWord)));
					mdelay_os(3);				
					PHY_SetBBReg(pAdapter, APK_offset[path], bMaskDWord, APK_value[1]);
					//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0x%x value 0x%x\n", APK_offset[path], PHY_QueryBBReg(pAdapter, APK_offset[path], bMaskDWord)));
					if(isNormal)
					    mdelay_os(20);
					else
					    mdelay_os(3);
				}
				PHY_SetBBReg(pAdapter, 0xe28, bMaskDWord, 0x00000000);
				tmpReg = PHY_QueryRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xb, bRFRegOffsetMask);
				//RTPRINT(FINIT, INIT_IQK, ("PHY_APCalibrate() offset 0xb %x\n", tmpReg));		
				
				tmpReg = (tmpReg & 0x3E00) >> 9;
				i++;
			}
			while(tmpReg > 12 && i < 4);

			APK_result[path][index] = tmpReg;
		}
	}
	
	//reload BB default value	
	for(index = 0; index < APK_BB_REG_NUM ; index++)
		PHY_SetBBReg(pAdapter, BB_REG[index], bMaskDWord, BB_backup[index]);

	//reload AFE default value
	for(index = 0; index < APK_AFE_REG_NUM ; index++)
		PHY_SetBBReg(pAdapter, AFE_REG[index], bMaskDWord, AFE_backup[index]);

	//reload RF path default value
	for(path = 0; path < pathbound; path++)
	{
		PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xd, bRFRegOffsetMask, regD[path]);

#if 0
		for(index = 0; index < APK_BB_REG_NUM ; index++)
		{
			if(APK_result[path][index] > 12)
				APK_result[path][index] = AP_curve[path][index];
				
			RTPRINT(FINIT, INIT_IQK, ("apk result %d 0x%x \t", index, APK_result[path][index]));
		}
#endif
		if(APK_result[path][1] < 1 || APK_result[path][1] > 5)
			APK_result[path][1] = AP_curve[path][0];
		//RTPRINT(FINIT, INIT_IQK, ("apk result %d 0x%x \t", 1, APK_result[path][1]));			

		if(APK_result[path][2] < 2 || APK_result[path][2] > 6)
			APK_result[path][2] = AP_curve[path][1];
		//RTPRINT(FINIT, INIT_IQK, ("apk result %d 0x%x \t", 2, APK_result[path][2]));			

		if(APK_result[path][3] < 2 || APK_result[path][3] > 6)
			APK_result[path][3] = AP_curve[path][2];
		//RTPRINT(FINIT, INIT_IQK, ("apk result %d 0x%x \t", 3, APK_result[path][3]));			

		if(APK_result[path][4] < 5 || APK_result[path][4] > 9)
			APK_result[path][4] = AP_curve[path][3];
		//RTPRINT(FINIT, INIT_IQK, ("apk result %d 0x%x \t", 4, APK_result[path][4]));			
		
		
		
	}

	//RTPRINT(FINIT, INIT_IQK, ("\n"));
	

	for(path = 0; path < pathbound; path++)
	{
		if(isNormal)
		{
			PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0x3, bRFRegOffsetMask, 
			((APK_result[path][1] << 15) | (APK_result[path][1] << 10) | (APK_result[path][1] << 5) | APK_result[path][1]));
			PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0x4, bRFRegOffsetMask, 
			((APK_result[path][2] << 15) | (APK_result[path][3] << 10) | (APK_result[path][4] << 5) | APK_result[path][4]));		
			PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xe, bRFRegOffsetMask, 
			((APK_result[path][4] << 15) | (APK_result[path][4] << 10) | (APK_result[path][4] << 5) | APK_result[path][4]));			
		}
		else
		{
			for(index = 0; index < 2; index++)
				pdmpriv->APKoutput[path][index] = ((APK_result[path][index] << 15) | (APK_result[path][2] << 10) | (APK_result[path][3] << 5) | APK_result[path][4]);

#if (MP_DRIVER == 1)
			if(pMptCtx->TxPwrLevel[path] > pMptCtx->APK_bound[path])	
			{
				PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xe, bRFRegOffsetMask, 
				pdmpriv->APKoutput[path][0]);
			}
			else
			{
				PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xe, bRFRegOffsetMask, 
				pdmpriv->APKoutput[path][1]);		
			}
#else
			PHY_SetRFReg(pAdapter, (RF90_RADIO_PATH_E)path, 0xe, bRFRegOffsetMask, 
			pdmpriv->APKoutput[path][0]);
#endif
		}
	}

	pdmpriv->bAPKdone = _TRUE;

	//RTPRINT(FINIT, INIT_IQK, ("<==PHY_APCalibrate()\n"));
#endif		
}

static VOID _PHY_SetRFPathSwitch(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		bMain,
	IN	BOOLEAN		is2T
	)
{
//	if(is2T)
//		return;
	
	if(!pAdapter->hw_init_completed)
	{
		PHY_SetBBReg(pAdapter, 0x4C, BIT23, 0x01);
		PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFParameter, BIT13, 0x01);
	}
	
	if(bMain)
		PHY_SetBBReg(pAdapter, rFPGA0_XA_RFInterfaceOE, 0x300, 0x2);	
	else
		PHY_SetBBReg(pAdapter, rFPGA0_XA_RFInterfaceOE, 0x300, 0x1);		

	//RT_TRACE(COMP_OID_SET, DBG_LOUD, ("_PHY_SetRFPathSwitch 0x4C %lx, 0x878 %lx, 0x860 %lx \n", PHY_QueryBBReg(pAdapter, 0x4C, BIT23), PHY_QueryBBReg(pAdapter, 0x878, BIT13), PHY_QueryBBReg(pAdapter, 0x860, 0x300)));

}

//return value TRUE => Main; FALSE => Aux

static BOOLEAN _PHY_QueryRFPathSwitch(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		is2T
	)
{
//	if(is2T)
//		return _TRUE;
	
	if(!pAdapter->hw_init_completed)
	{
		PHY_SetBBReg(pAdapter, 0x4C, BIT23, 0x01);
		PHY_SetBBReg(pAdapter, rFPGA0_XAB_RFParameter, BIT13, 0x01);
	}

	//RT_TRACE(COMP_OID_SET, DBG_LOUD, ("_PHY_QueryRFPathSwitch 0x4C %lx, 0x878 %lx, 0x860 %lx \n", PHY_QueryBBReg(pAdapter, 0x4C, BIT23), PHY_QueryBBReg(pAdapter, 0x878, BIT13), PHY_QueryBBReg(pAdapter, 0x860, 0x300)));

	if(PHY_QueryBBReg(pAdapter, rFPGA0_XA_RFInterfaceOE, 0x300) == 0x01)
		return _TRUE;
	else 
		return _FALSE;


}


static VOID
_PHY_DumpRFReg(IN	PADAPTER	pAdapter)
{
	u32 rfRegValue,rfRegOffset;

	//RTPRINT(FINIT, INIT_RF, ("PHY_DumpRFReg()====>\n"));
	
	for(rfRegOffset = 0x00;rfRegOffset<=0x30;rfRegOffset++){		
		rfRegValue = PHY_QueryRFReg(pAdapter,RF90_PATH_A, rfRegOffset, bRFRegOffsetMask);
		//RTPRINT(FINIT, INIT_RF, (" 0x%02x = 0x%08x\n",rfRegOffset,rfRegValue));
	}
	//RTPRINT(FINIT, INIT_RF, ("<===== PHY_DumpRFReg()\n"));
}


VOID
rtl8192d_PHY_IQCalibrate(
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;
	int			result[4][8];	//last is final result
	u8			i, final_candidate, Indexforchannel;
	BOOLEAN		bPathAOK, bPathBOK;
	int			RegE94, RegE9C, RegEA4, RegEAC, RegEB4, RegEBC, RegEC4, RegECC, RegTmp = 0;
	BOOLEAN		is12simular, is13simular, is23simular;	
	BOOLEAN 	bStartContTx = _FALSE;
#if (MP_DRIVER == 1)
	bStartContTx = pAdapter->MptCtx.bStartContTx;
#endif

	//ignore IQK when continuous Tx
	if(bStartContTx)
		return;

#if DISABLE_BB_RF
	return;
#endif

#if (RTL8192D_DUAL_MAC_MODE_SWITCH == 1)
	if(pAdapter->BuddyAdapter != NULL && pHalData->bSlaveOfDMSP)
		return;
#endif

	//RTPRINT(FINIT, INIT_IQK, ("IQK:Start!!!interface %d channel %d\n", pHalData->interfaceIndex, pHalData->CurrentChannel));

	for(i = 0; i < 8; i++)
	{
		result[0][i] = 0;
		result[1][i] = 0;
		result[2][i] = 0;
		result[3][i] = 0;
	}
	final_candidate = 0xff;
	bPathAOK = _FALSE;
	bPathBOK = _FALSE;
	is12simular = _FALSE;
	is23simular = _FALSE;
	is13simular = _FALSE;

	//RTPRINT(FINIT, INIT_IQK, ("IQK !!!interface %d currentband %d ishardwareD %d \n", pAdapter->interfaceIndex, pHalData->CurrentBandType92D, IS_HARDWARE_TYPE_8192D(pAdapter)));

	for (i=0; i<3; i++)
	{
		if(pHalData->CurrentBandType92D == BAND_ON_5G)
		{
			if(IS_NORMAL_CHIP(pHalData->VersionID)) 					
				_PHY_IQCalibrate_5G_Normal(pAdapter, result, i);
			else
			{
				//Luke: 5G IQK only do once for temp use
				if(i == 0)
				{
					_PHY_IQCalibrate_5G(pAdapter, result);
					final_candidate = 0;
					break;
				}
			}
		}
		else if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
		{
			if(IS_92D_SINGLEPHY(pHalData->VersionID))
				_PHY_IQCalibrate(pAdapter, result, i, _TRUE);
			else
				_PHY_IQCalibrate(pAdapter, result, i, _FALSE);
		}
		
		if(i == 1)
		{
			is12simular = _PHY_SimularityCompare(pAdapter, result, 0, 1);
			if(is12simular)
			{
				final_candidate = 0;
				break;
			}
		}
		
		if(i == 2)
		{
			is13simular = _PHY_SimularityCompare(pAdapter, result, 0, 2);
			if(is13simular)
			{
				final_candidate = 0;			
				break;
			}
			
			is23simular = _PHY_SimularityCompare(pAdapter, result, 1, 2);
			if(is23simular)
				final_candidate = 1;
			else
			{
				for(i = 0; i < 8; i++)
					RegTmp += result[3][i];

				if(RegTmp != 0)
					final_candidate = 3;			
				else
					final_candidate = 0xFF;
			}
		}
	}

        for (i=0; i<4; i++)
	{
		RegE94 = result[i][0];
		RegE9C = result[i][1];
		RegEA4 = result[i][2];
		RegEAC = result[i][3];
		RegEB4 = result[i][4];
		RegEBC = result[i][5];
		RegEC4 = result[i][6];
		RegECC = result[i][7];
		//RTPRINT(FINIT, INIT_IQK, ("IQK: RegE94=%lx RegE9C=%lx RegEA4=%lx RegEAC=%lx RegEB4=%lx RegEBC=%lx RegEC4=%lx RegECC=%lx\n ", RegE94, RegE9C, RegEA4, RegEAC, RegEB4, RegEBC, RegEC4, RegECC));
	}

	if(final_candidate != 0xff)
	{
		pdmpriv->RegE94 = RegE94 = result[final_candidate][0];
		pdmpriv->RegE9C = RegE9C = result[final_candidate][1];
		RegEA4 = result[final_candidate][2];
		RegEAC = result[final_candidate][3];
		pdmpriv->RegEB4 = RegEB4 = result[final_candidate][4];
		pdmpriv->RegEBC = RegEBC = result[final_candidate][5];
		RegEC4 = result[final_candidate][6];
		RegECC = result[final_candidate][7];
		DBG_8192C("IQK: final_candidate is %x\n", final_candidate);
		DBG_8192C("IQK: RegE94=%x RegE9C=%x RegEA4=%x RegEAC=%x RegEB4=%x RegEBC=%x RegEC4=%x RegECC=%x\n ", RegE94, RegE9C, RegEA4, RegEAC, RegEB4, RegEBC, RegEC4, RegECC);
		bPathAOK = bPathBOK = _TRUE;
	}
	else
	{
		pdmpriv->RegE94 = pdmpriv->RegEB4 = 0x100;	//X default value
		pdmpriv->RegE9C = pdmpriv->RegEBC = 0x0;		//Y default value
	}
	
	if((RegE94 != 0)/*&&(RegEA4 != 0)*/)
		_PHY_PathAFillIQKMatrix(pAdapter, bPathAOK, result, final_candidate, (RegEA4 == 0));
	if (IS_92C_SERIAL(pHalData->VersionID) || IS_92D_SINGLEPHY(pHalData->VersionID))
	{
		if((RegEB4 != 0)/*&&(RegEC4 != 0)*/)
			_PHY_PathBFillIQKMatrix(pAdapter, bPathBOK, result, final_candidate, (RegEC4 == 0));
	}


	Indexforchannel = GetRightChnlPlaceforIQK(pHalData->CurrentChannel);

	//save the value.
	for(i=0; i< 9; i++)
	{
		pHalData->IQKMatrixRegSetting[Indexforchannel].Value[i] 
			= PHY_QueryBBReg(pAdapter, pHalData->IQKMatrixReg[i],pHalData->IQKMatrixRegSetting[Indexforchannel].Mark[i]);
		//RT_TRACE(COMP_SCAN|COMP_MLME,DBG_LOUD,
		//		(" 0x%03X =%08X \n",pHalData->IQKMatrixReg[i],PHY_QueryBBReg(pAdapter, pHalData->IQKMatrixReg[i], pHalData->IQKMatrixRegSetting[Indexforchannel].Mark[i])));
		//RT_TRACE(COMP_SCAN|COMP_MLME,DBG_LOUD,
		//	(" 0x%03X(mark:%08X)=%08X ",pHalData->IQKMatrixReg[i],pHalData->IQKMatrixRegSetting[Indexforchannel].Mark[i],pHalData->IQKMatrixRegSetting[Indexforchannel].Value[i]));	
	}

	//pHalData->IQKMatrixRegSetting[Indexforchannel].Value[9] =  pHalData->RegE94;		
	//pHalData->IQKMatrixRegSetting[Indexforchannel].Value[10] =  pHalData->RegE9C;		
	//pHalData->IQKMatrixRegSetting[Indexforchannel].Value[11] =  pHalData->RegEB4;		
	//pHalData->IQKMatrixRegSetting[Indexforchannel].Value[12] =  pHalData->RegEBC;		
	pHalData->IQKMatrixRegSetting[Indexforchannel].bIQKDone = _TRUE;		

	//RT_TRACE(COMP_SCAN|COMP_MLME,DBG_LOUD,("\nIQK OK Indexforchannel %d.\n", Indexforchannel));	


}


VOID
rtl8192d_PHY_LCCalibrate(
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	BOOLEAN 		bStartContTx = _FALSE;
#if MP_DRIVER == 1
	bStartContTx = pAdapter->MptCtx.bStartContTx;
#endif

#if DISABLE_BB_RF
	return;
#endif

	//ignore IQK when continuous Tx
	if(bStartContTx)
		return;

	//RTPRINT(FINIT, INIT_IQK, ("LCK:Start!!!interface %d\n", pHalData->interfaceIndex));

	if(IS_92C_SERIAL(pHalData->VersionID) || IS_92D_SINGLEPHY(pHalData->VersionID))
	{
		_PHY_LCCalibrate(pAdapter, _TRUE);
	}
	else{
		// For 88C 1T1R
		_PHY_LCCalibrate(pAdapter, _FALSE);
	}

	//RTPRINT(FINIT, INIT_IQK, ("LCK:Finish!!!interface %d\n", pHalData->interfaceIndex));
}

VOID
rtl8192d_PHY_APCalibrate(
	IN	PADAPTER	pAdapter,
	IN	char 		delta	
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);
	struct dm_priv	*pdmpriv = &pHalData->dmpriv;

#if DISABLE_BB_RF
	return;
#endif

	//if(IS_HARDWARE_TYPE_8192D(pAdapter))
		return;

	if(pdmpriv->bAPKdone)
		return;

//	if(IS_NORMAL_CHIP(pHalData->VersionID))
//		return;

	if(IS_92D_SINGLEPHY(pHalData->VersionID)){
		_PHY_APCalibrate(pAdapter, delta, _TRUE);
	}
	else{
		// For 88C 1T1R
		_PHY_APCalibrate(pAdapter, delta, _FALSE);
	}
}

/*
VOID PHY_SetRFPathSwitch(
	IN	PADAPTER	pAdapter,
	IN	BOOLEAN		bMain
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

#if DISABLE_BB_RF
	return;
#endif

	if (IS_92D_SINGLEPHY(pHalData->VersionID))
	{
		_PHY_SetRFPathSwitch(pAdapter, bMain, _TRUE);
	}
	else{
		// For 88C 1T1R
		_PHY_SetRFPathSwitch(pAdapter, bMain, _FALSE);
	}
}


//return value TRUE => Main; FALSE => Aux
BOOLEAN PHY_QueryRFPathSwitch(	
	IN	PADAPTER	pAdapter
	)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(pAdapter);

#if DISABLE_BB_RF
	return _TRUE;
#endif

#if (DEV_BUS_TYPE==DEV_BUS_USB_INTERFACE)
	return _TRUE;
#endif

	if(IS_92D_SINGLEPHY(pHalData->VersionID))
	{
		return _PHY_QueryRFPathSwitch(pAdapter, _TRUE);
	}
	else{
		// For 88C 1T1R
		return _PHY_QueryRFPathSwitch(pAdapter, _FALSE);
	}
}
*/

VOID
PHY_UpdateBBRFConfiguration8192D(
	IN PADAPTER Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8	eRFPath = 0;
	u32	u4RegValue = 0;

	//DBG_8192C("PHY_UpdateBBRFConfiguration8192D()=====>\n");

	//Update BB
	/*u4RegValue = read32(Adapter, REG_SPS0_CTRL+3)&0xFF0FFFFF;
	if(IS_NORMAL_CHIP(pHalData->VersionID))
	{
		// 1.5V_LDO 0x14[23:20]=0x8
		u4RegValue |= BIT23;
		write32(Adapter, REG_SPS0_CTRL+3,u4RegValue);

	}
	else
	{
		// 1.5V_LDO 0x14[23:20]=0xd
		u4RegValue |= 0x00d00000;
		write32(Adapter, REG_SPS0_CTRL+3,u4RegValue);
	}*/

	//r_select_5G for path_A/B.0 for 2.4G,1 for 5G
	if(pHalData->CurrentBandType92D == BAND_ON_2_4G)
	{// 2.4G band
		//r_select_5G for path_A/B,0x878
		PHY_SetBBReg(Adapter, rFPGA0_XAB_RFParameter, BIT0, 0x0);
		PHY_SetBBReg(Adapter, rFPGA0_XAB_RFParameter, BIT15, 0x0);
		if(pHalData->MacPhyMode92D != DUALMAC_DUALPHY)
		{
			PHY_SetBBReg(Adapter, rFPGA0_XAB_RFParameter, BIT16, 0x0);
			PHY_SetBBReg(Adapter, rFPGA0_XAB_RFParameter, BIT31, 0x0);
		}
		
		//rssi_table_select:index 0 for 2.4G.1~3 for 5G,0xc78
		PHY_SetBBReg(Adapter, rOFDM0_AGCRSSITable, BIT6|BIT7, 0x0);
		
		//fc_area//0xd2c
		PHY_SetBBReg(Adapter, rOFDM1_CFOTracking, BIT14|BIT13, 0x0);
		// 5G LAN ON
		PHY_SetBBReg(Adapter, 0xB30, 0x00F00000, 0xa);
		//AGC trsw threshold,0xc70
//		PHY_SetBBReg(Adapter, rOFDM0_AGCParameter1, 0x7F0000, 0x7f);
		
		//TX BB gain shift*1,Just for testchip,0xc80,0xc88
		//if(!IS_NORMAL_CHIP(pHalData->VersionID))
		{
			PHY_SetBBReg(Adapter, rOFDM0_XATxIQImbalance, bMaskDWord, 0x40000100);
			PHY_SetBBReg(Adapter, rOFDM0_XBTxIQImbalance, bMaskDWord, 0x40000100);
		}
		
		// 1.5V_LDO
		if(!IS_NORMAL_CHIP(pHalData->VersionID)&&(pHalData->MacPhyMode92D != DUALMAC_DUALPHY))
			write32(Adapter, REG_SPS0_CTRL+3, (read32(Adapter, REG_SPS0_CTRL+3)&0xFF0FFFFF)|0x00700000);
	}
	else	//5G band
	{
		//r_select_5G for path_A/B
		PHY_SetBBReg(Adapter, rFPGA0_XAB_RFParameter, BIT0, 0x1);
		PHY_SetBBReg(Adapter, rFPGA0_XAB_RFParameter, BIT15, 0x1);
		if(pHalData->MacPhyMode92D != DUALMAC_DUALPHY){
			PHY_SetBBReg(Adapter, rFPGA0_XAB_RFParameter, BIT16, 0x1);
			PHY_SetBBReg(Adapter, rFPGA0_XAB_RFParameter, BIT31, 0x1);
		}
		
		//rssi_table_select:index 0 for 2.4G.1~3 for 5G
		PHY_SetBBReg(Adapter, rOFDM0_AGCRSSITable, BIT6|BIT7, 0x1);
		
		//fc_area
		PHY_SetBBReg(Adapter, rOFDM1_CFOTracking, BIT14|BIT13, 0x1);
		// 5G LAN ON
		PHY_SetBBReg(Adapter, 0xB30, 0x00F00000, 0x0);
		//AGC trsw threshold,0xc70
//		PHY_SetBBReg(Adapter, rOFDM0_AGCParameter1, 0x7F0000, 0x4e);
		
		//TX BB gain shift,Just for testchip,0xc80,0xc88
		//if(!IS_NORMAL_CHIP(pHalData->VersionID))
		{
			PHY_SetBBReg(Adapter, rOFDM0_XATxIQImbalance, bMaskDWord, 0x20000080);
			PHY_SetBBReg(Adapter, rOFDM0_XBTxIQImbalance, bMaskDWord, 0x20000080);
		}
	}

	//update IQK related settings
	{
		PHY_SetBBReg(Adapter, rOFDM0_XARxIQImbalance, bMaskDWord, 0x40000100);
		PHY_SetBBReg(Adapter, rOFDM0_XBRxIQImbalance, bMaskDWord, 0x40000100);	
		PHY_SetBBReg(Adapter, rOFDM0_XCTxAFE, 0xF0000000, 0x00);
		PHY_SetBBReg(Adapter, rOFDM0_ECCAThreshold,  BIT30|BIT28|BIT26|BIT24,  0x00);
		PHY_SetBBReg(Adapter, rOFDM0_XDTxAFE, 0xF0000000, 0x00);		
		PHY_SetBBReg(Adapter, 0xca0, 0xF0000000, 0x00);
		PHY_SetBBReg(Adapter, rOFDM0_AGCRSSITable, 0x0000F000, 0x00);

		
	}
		
	//Update RF	
	for(eRFPath = RF90_PATH_A; eRFPath <pHalData->NumTotalRFPath; eRFPath++)
	{
		if(pHalData->CurrentBandType92D == BAND_ON_2_4G){
			//MOD_AG for RF paht_A 0x18 BIT8,BIT16
			PHY_SetRFReg(Adapter, eRFPath, RF_CHNLBW, BIT8|BIT16|BIT18, 0);

			//RF0x0b[16:14] =3b'111  
			PHY_SetRFReg(Adapter, (RF90_RADIO_PATH_E)eRFPath, 0x0B, 0x1c000, 0x07);

			//LDO_DIV,0x28{0_RF_A{Reg28[7:6], 0_RF_B{Reg28[7:6]}
			//PHY_SetRFReg(Adapter, eRFPath, RF_SYN_G4, BIT6|BIT7, 0);							
			if(!IS_NORMAL_CHIP(pHalData->VersionID)){
				//A/G mode LO buffer,{0_RF_A{Reg38[16:14], 0_RF_B{Reg38[16:14]}
				PHY_SetRFReg(Adapter, eRFPath, 0x38, BIT14|BIT15|BIT16, 0x4);
				//Peak Detector 0x38
				PHY_SetRFReg(Adapter, eRFPath, 0x38, BIT19, 0);
				//Gain setting 0x1c
				PHY_SetRFReg(Adapter, eRFPath, RF_RX_BB2, BIT0, 1);
			}
		}
		else{ //5G band
			//MOD_AG for RF paht_A 0x18 BIT8,BIT16
			PHY_SetRFReg(Adapter, eRFPath, RF_CHNLBW, BIT8|BIT16|BIT18, (BIT16|BIT8)>>8);

			//LDO_DIV,0x28{0_RF_A{Reg28[7:6], 0_RF_B{Reg28[7:6]}
			//PHY_SetRFReg(Adapter, eRFPath, RF_SYN_G4, BIT6|BIT7, 1);				
			if(!IS_NORMAL_CHIP(pHalData->VersionID)){
				//A/G mode LO buffer,0x38{0_RF_A{Reg38[16:14], 0_RF_B{Reg38[16:14]}
				PHY_SetRFReg(Adapter, eRFPath, 0x38, BIT14|BIT15|BIT16, 0x3);
				//Turn off peak detection
				PHY_SetRFReg(Adapter, eRFPath, 0x38, BIT19, 0);
				//Gain setting 0x1c
				PHY_SetRFReg(Adapter, eRFPath, RF_RX_BB2, BIT0, eRFPath==RF90_PATH_A?0:1);
			}
		}
#if 0		
		if(IS_NORMAL_CHIP(pHalData->VersionID)){
			//0x28[10:8] BS_VCO	
			if(pHalData->MacPhyMode92D!=DUALMAC_DUALPHY)		
				PHY_SetRFReg(Adapter, eRFPath, RF_SYN_G4, BIT9|BIT10|BIT8, eRFPath==RF90_PATH_A?7:5);
			else
				PHY_SetRFReg(Adapter, eRFPath, RF_SYN_G4, BIT9|BIT10|BIT8, Adapter->interfaceIndex==0?7:5);
		}
#endif		
	}

	//Update for all band.
	if(pHalData->rf_type == RF_1T1R)
	{ //DMDP
		//Use antenna 0,0xc04,0xd04
		PHY_SetBBReg(Adapter, rOFDM0_TRxPathEnable, bMaskByte0, 0x11);
		PHY_SetBBReg(Adapter, rOFDM1_TRxPathEnable, bDWord, 0x1);

		//enable ad/da clock1 for dual-phy reg0x888
		if(pHalData->interfaceIndex == 0)
			PHY_SetBBReg(Adapter, rFPGA0_AdDaClockEn, BIT12|BIT13, 0x3);
		else
		{
			rtl8192d_PHY_EnableAnotherPHY(Adapter, _FALSE);
#if DEV_BUS_TYPE == DEV_BUS_PCI_INTERFACE
			RT_TRACE(COMP_INIT,DBG_LOUD,("MAC1 use DBI to update 0x888"));
			//0x888
			MpWritePCIDwordDBI8192C(Adapter, 
									rFPGA0_AdDaClockEn, 
									MpReadPCIDwordDBI8192C(Adapter, rFPGA0_AdDaClockEn, BIT3)|BIT12|BIT13,
									BIT3);
#else	//USB interface
			PHY_SetBBReg(Adapter, rFPGA0_AdDaClockEn, BIT12|BIT13, 0x3);
#endif
			rtl8192d_PHY_PowerDownAnotherPHY(Adapter, _FALSE);
		}
		
		//disable RF 2T2R mode
		if(!IS_NORMAL_CHIP(pHalData->VersionID))
			PHY_SetRFReg(Adapter, RF90_PATH_A, 0x38, BIT12, 0);		
	}
	else // 2T2R //Single PHY
	{
		//Use antenna 0 & 1,0xc04,0xd04
		PHY_SetBBReg(Adapter, rOFDM0_TRxPathEnable, bMaskByte0, 0x33);
		PHY_SetBBReg(Adapter, rOFDM1_TRxPathEnable, bDWord, 0x3);

		//disable ad/da clock1,0x888
		PHY_SetBBReg(Adapter, rFPGA0_AdDaClockEn, BIT12|BIT13, 0);
		
		//enable RF 2T2R mode
		if(!IS_NORMAL_CHIP(pHalData->VersionID))
			PHY_SetRFReg(Adapter, RF90_PATH_A, 0x38, BIT12, 1);
	}

	pHalData->RfRegChnlVal[0] = PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E)0, RF_CHNLBW, bRFRegOffsetMask);
	pHalData->RfRegChnlVal[1] = PHY_QueryRFReg(Adapter, (RF90_RADIO_PATH_E)1, RF_CHNLBW, bRFRegOffsetMask);

	//RT_TRACE(COMP_INIT,DBG_LOUD,("<==PHY_UpdateBBRFConfiguration8192D()\n"));

}

#if 0
VOID
rtl8192d_PHY_ResetIQKResult(
	IN	PADAPTER Adapter
)
{
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(Adapter);
	u8			i, j;


	//RT_TRACE(COMP_INIT, DBG_LOUD, ("PHY_ResetIQKResult interface %d settings regs %d default regs %d\n", Adapter->interfaceIndex, sizeof(pHalData->IQKMatrixRegSetting)/sizeof(IQK_MATRIX_REGS_SETTING), IQK_Matrix_Settings_NUM));

//	RTPRINT(FINIT, INIT_IQK, ("PHY_ResetIQKResult interface %d settings regs %d default regs %d\n", Adapter->interfaceIndex, sizeof(pHalData->IQKMatrixRegSetting)/sizeof(IQK_MATRIX_REGS_SETTING), IQK_Matrix_Settings_NUM));
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
	pHalData->IQKMatrixReg[9] = 0xE94;
	pHalData->IQKMatrixReg[10] = 0xE9C;
	pHalData->IQKMatrixReg[11] = 0xEB4;
	pHalData->IQKMatrixReg[12] = 0xEBC;	

//	for(i=0;i<sizeof(pHalData->IQKMatrixRegSetting)/sizeof(IQK_MATRIX_REGS_SETTING);i++)
	for(i=0; i<IQK_Matrix_Settings_NUM; i++)
	{
	
  		pHalData->IQKMatrixRegSetting[i].bIQKDone = _FALSE;
		
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
		//0xc94, 0xc9c, 0xca0, 0xE94, 0xE9C, 0xEB4, 0xEBC
		for(j = 6; j < IQK_Matrix_REG_NUM; j++)
		{
			if(j < 9)
				pHalData->IQKMatrixRegSetting[i].Mark[j] = bMaskByte3;
			else
				pHalData->IQKMatrixRegSetting[i].Mark[j] = 0x3FF0000;
			if(j == 9 || j ==11)
				pHalData->IQKMatrixRegSetting[i].Value[j] = 0x100;	//0xE94, 0xEB4
			else
				pHalData->IQKMatrixRegSetting[i].Value[j] = 0x0;
		}
		
	} 
}
#endif

