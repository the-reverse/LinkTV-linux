/******************************************************************************
* rtw_rf.c                                                                                                                                 *
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
#define _RTW_RF_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <xmit_osdep.h>


struct ch_freq {
	u32 channel;
	u32 frequency;
};

struct ch_freq ch_freq_map[] = {
	{1, 2412},{2, 2417},{3, 2422},{4, 2427},{5, 2432},
	{6, 2437},{7, 2442},{8, 2447},{9, 2452},{10, 2457},
	{11, 2462},{12, 2467},{13, 2472},{14, 2484},
	/*  UNII */
	{36, 5180},{40, 5200},{44, 5220},{48, 5240},{52, 5260},
	{56, 5280},{60, 5300},{64, 5320},{149, 5745},{153, 5765},
	{157, 5785},{161, 5805},{165, 5825},{167, 5835},{169, 5845},
	{171, 5855},{173, 5865},
	/* HiperLAN2 */
	{100, 5500},{104, 5520},{108, 5540},{112, 5560},{116, 5580},
	{120, 5600},{124, 5620},{128, 5640},{132, 5660},{136, 5680},
	{140, 5700},
	/* Japan MMAC */
	{34, 5170},{38, 5190},{42, 5210},{46, 5230},
	/*  Japan */
	{184, 4920},{188, 4940},{192, 4960},{196, 4980},
	{208, 5040},/* Japan, means J08 */
	{212, 5060},/* Japan, means J12 */
	{216, 5080},/* Japan, means J16 */
};

int ch_freq_map_num = (sizeof(ch_freq_map) / sizeof(struct ch_freq));

u32 ch2freq(u32 channel)
{
	u8	i;
	u32	freq;

	for (i = 0; i < ch_freq_map_num; i++)
	{
		if (channel == ch_freq_map[i].channel)
		{
			freq = ch_freq_map[i].frequency;
				break;
		}
	}
	if (i == ch_freq_map_num)
		freq = 2412;

	return freq;
}

u32 freq2ch(u32 freq)
{
	u8	i;
	u32	ch;

	for (i = 0; i < ch_freq_map_num; i++)
	{
		if (freq == ch_freq_map[i].frequency)
		{
			ch = ch_freq_map[i].channel;
				break;
		}
	}
	if (i == ch_freq_map_num)
		ch = 1;

	return ch;
}

