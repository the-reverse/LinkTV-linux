/******************************************************************************
* rtl8192c_recv.c                                                                                                                                 *
*                                                                                                                                          *
* Description :                                                                                                                       *
*                                                                                                                                           *
* Author :                                                                                                                       *
*                                                                                                                                         *
* History :
*
*
*                                                                                                                                       *
* Copyright 2008, Realtek Corp.                                                                                                  *
*                                                                                                                                        *
* The contents of this file is the sole property of Realtek Corp.  It can not be                                     *
* be used, copied or modified without written permission from Realtek Corp.                                         *
*                                                                                                                                          *
*******************************************************************************/
#define _RTL8192C_REDESC_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtl8192d_hal.h>

static u8 evm_db2percentage(s8 value)
{
	//
	// -33dB~0dB to 0%~99%
	//
	s8 ret_val;

	ret_val = value;
	//ret_val /= 2;

	RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("EVMdbToPercentage92S Value=%d / %x \n", ret_val, ret_val));

	if(ret_val >= 0)
		ret_val = 0;
	if(ret_val <= -33)
		ret_val = -33;

	ret_val = 0 - ret_val;
	ret_val*=3;

	if(ret_val == 99)
		ret_val = 100;

	return(ret_val);
}


static s32 signal_scale_mapping(_adapter *padapter, s32 cur_sig )
{
	s32 ret_sig;

#if DEV_BUS_TYPE==DEV_BUS_USB_INTERFACE
	if(cur_sig >= 51 && cur_sig <= 100)
	{
		ret_sig = 100;
	}
	else if(cur_sig >= 41 && cur_sig <= 50)
	{
		ret_sig = 80 + ((cur_sig - 40)*2);
	}
	else if(cur_sig >= 31 && cur_sig <= 40)
	{
		ret_sig = 66 + (cur_sig - 30);
	}
	else if(cur_sig >= 21 && cur_sig <= 30)
	{
		ret_sig = 54 + (cur_sig - 20);
	}
	else if(cur_sig >= 10 && cur_sig <= 20)
	{
		ret_sig = 42 + (((cur_sig - 10) * 2) / 3);
	}
	else if(cur_sig >= 5 && cur_sig <= 9)
	{
		ret_sig = 22 + (((cur_sig - 5) * 3) / 2);
	}
	else if(cur_sig >= 1 && cur_sig <= 4)
	{
		ret_sig = 6 + (((cur_sig - 1) * 3) / 2);
	}
	else
	{
		ret_sig = cur_sig;
	}
#else
	HAL_DATA_TYPE	*pHalData = GET_HAL_DATA(padapter);

	if(pHalData->CustomerID == RT_CID_819x_Lenovo)
	{
		return cur_sig;
	}

	// Step 1. Scale mapping.
	if(cur_sig >= 61 && cur_sig <= 100)
	{
		ret_sig = 90 + ((cur_sig - 60) / 4);
	}
	else if(cur_sig >= 41 && cur_sig <= 60)
	{
		ret_sig = 78 + ((cur_sig - 40) / 2);
	}
	else if(cur_sig >= 31 && cur_sig <= 40)
	{
		ret_sig = 66 + (cur_sig - 30);
	}
	else if(cur_sig >= 21 && cur_sig <= 30)
	{
		ret_sig = 54 + (cur_sig - 20);
	}
	else if(cur_sig >= 5 && cur_sig <= 20)
	{
		ret_sig = 42 + (((cur_sig - 5) * 2) / 3);
	}
	else if(cur_sig == 4)
	{
		ret_sig = 36;
	}
	else if(cur_sig == 3)
	{
		ret_sig = 27;
	}
	else if(cur_sig == 2)
	{
		ret_sig = 18;
	}
	else if(cur_sig == 1)
	{
		ret_sig = 9;
	}
	else
	{
		ret_sig = cur_sig;
	}
#endif

	return ret_sig;
}


static s32  translate2dbm(_adapter *padapter,u8 signal_strength_idx	)
{
	s32	signal_power; // in dBm.


	// Translate to dBm (x=0.5y-95).
	signal_power = (s32)((signal_strength_idx + 1) >> 1);
	signal_power -= 95;

	return signal_power;
}


void rtl8192d_query_rx_phy_status(union recv_frame *prframe, struct phy_stat *pphy_stat)
{
	struct phy_cck_rx_status *pcck_buf;
	u8	i, max_spatial_stream, evm;
	s8	rx_pwr[4], rx_pwr_all;
	u8	pwdb_all;
	u32	rssi,total_rssi=0;
	u8 	bcck_rate=0, rf_rx_num = 0, cck_highpwr = 0;
	_adapter				*padapter = prframe->u.hdr.adapter;
	struct rx_pkt_attrib	*pattrib = &prframe->u.hdr.attrib;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(padapter);
	u8	*pphy_head=(u8 *)pphy_stat;

	// Record it for next packet processing
	bcck_rate=(pattrib->mcs_rate<=3? 1:0);

	if(bcck_rate) //CCK
	{
		u8 report;

		// CCK Driver info Structure is not the same as OFDM packet.
		pcck_buf = (struct phy_cck_rx_status *)pphy_stat;
		//Adapter->RxStats.NumQryPhyStatusCCK++;

		//
		// (1)Hardware does not provide RSSI for CCK
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//

		if(padapter->pwrctrlpriv.rf_pwrstate == rf_on)
			cck_highpwr = (u8)pHalData->bCckHighPower;
		else
			cck_highpwr = _FALSE;

		if(!cck_highpwr)
		{
			report = pcck_buf->cck_agc_rpt&0xc0;
			report = report>>6;
			switch(report)
			{
				// 03312009 modified by cosa
				// Modify the RF RNA gain value to -40, -20, -2, 14 by Jenyu's suggestion
				// Note: different RF with the different RNA gain.
				case 0x3:
					rx_pwr_all = (-46) - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x2:
					rx_pwr_all = (-26) - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x1:
					rx_pwr_all = (-12) - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
				case 0x0:
					rx_pwr_all = (16) - (pcck_buf->cck_agc_rpt & 0x3e);
					break;
			}
		}
		else
		{
			report =((u8)(le32_to_cpu( pphy_stat->phydw1) >>8)) & 0x60;
			report = report>>5;
			switch(report)
			{
				case 0x3:
					rx_pwr_all = (-46) - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x2:
					rx_pwr_all = (-26)- ((pcck_buf->cck_agc_rpt & 0x1f)<<1);
					break;
				case 0x1:
					rx_pwr_all = (-12) - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
				case 0x0:
					rx_pwr_all = (16) - ((pcck_buf->cck_agc_rpt & 0x1f)<<1) ;
					break;
			}
		}

		pwdb_all= query_rx_pwr_percentage(rx_pwr_all);
		//if(pHalData->CustomerID == RT_CID_819x_Lenovo)
		{
			// CCK gain is smaller than OFDM/MCS gain,
			// so we add gain diff by experiences, the val is 6
			pwdb_all+=6;
			if(pwdb_all > 100)
				pwdb_all = 100;
			// modify the offset to make the same gain index with OFDM.
			if(pwdb_all > 34 && pwdb_all <= 42)
				pwdb_all -= 2;
			else if(pwdb_all > 26 && pwdb_all <= 34)
				pwdb_all -= 6;
			else if(pwdb_all > 14 && pwdb_all <= 26)
				pwdb_all -= 8;
			else if(pwdb_all > 4 && pwdb_all <= 14)
				pwdb_all -= 4;
		}

		pattrib->RxPWDBAll = pwdb_all;
		pattrib->RecvSignalPower = rx_pwr_all;

		//
		// (3) Get Signal Quality (EVM)
		//
		//if(bPacketMatchBSSID)
		{
			u8	sq;

			if(pHalData->CustomerID == RT_CID_819x_Lenovo)
			{
				// mapping to 5 bars for vista signal strength
				// signal quality in driver will be displayed to signal strength
				// in vista.
				if(pwdb_all >= 50)
					sq = 100;
				else if(pwdb_all >= 35 && pwdb_all < 50)
					sq = 80;
				else if(pwdb_all >= 22 && pwdb_all < 35)
					sq = 60;
				else if(pwdb_all >= 18 && pwdb_all < 22)
					sq = 40;
				else
					sq = 20;
			}
			else
			{
				if(pwdb_all> 40)
				{
					sq = 100;
				}
				else
				{
					sq = pcck_buf->sq_rpt;

					if(pcck_buf->sq_rpt > 64)
						sq = 0;
					else if (pcck_buf->sq_rpt < 20)
						sq= 100;
					else
						sq = ((64-sq) * 100) / 44;

				}
			}

			pattrib->signal_qual=sq;
			pattrib->rx_mimo_signal_qual[0]=sq;
			pattrib->rx_mimo_signal_qual[1]=(-1);
		}
	}
	else //OFDM/HT
	{

		//
		// (1)Get RSSI for HT rate
		//
		for(i=0; i<pHalData->NumTotalRFPath; i++)
		{
			// 2008/01/30 MH we will judge RF RX path now.
			if (pHalData->bRFPathRxEnable[i])
				rf_rx_num++;
			//else
				//continue;

			rx_pwr[i] = ((pphy_head[PHY_STAT_GAIN_TRSW_SHT+i]&0x3F)*2) - 110;

			/* Translate DBM to percentage. */
			rssi=query_rx_pwr_percentage(rx_pwr[i]);
			total_rssi += rssi;

			RT_TRACE(_module_rtl871x_recv_c_, _drv_err_, ("RF-%d RXPWR=%x RSSI=%d\n", i, rx_pwr[i], rssi));

			//Get Rx snr value in DB
			//Adapter->RxStats.RxSNRdB[i] = (s4Byte)(pDrvInfo->rxsnr[i]/2);

			/* Record Signal Strength for next packet */
			//if(bPacketMatchBSSID)
			{
				//pRfd->Status.RxMIMOSignalStrength[i] =(u8) rssi;

				//The following is for lenovo signal strength in vista
				if(pHalData->CustomerID == RT_CID_819x_Lenovo)
				{
					u8	SQ;

					if(i == 0)
					{
						// mapping to 5 bars for vista signal strength
						// signal quality in driver will be displayed to signal strength
						// in vista.
						if(rssi >= 50)
							SQ = 100;
						else if(rssi >= 35 && rssi < 50)
							SQ = 80;
						else if(rssi >= 22 && rssi < 35)
							SQ = 60;
						else if(rssi >= 18 && rssi < 22)
							SQ = 40;
						else
							SQ = 20;
						//DbgPrint("ofdm/mcs RSSI=%d\n", RSSI);
						pattrib->signal_qual = SQ;
						//DbgPrint("ofdm/mcs SQ = %d\n", pRfd->Status.SignalQuality);
					}
				}
			}
		}


		//
		// (2)PWDB, Average PWDB cacluated by hardware (for rate adaptive)
		//
		rx_pwr_all = (((pphy_head[PHY_STAT_PWDB_ALL_SHT]) >> 1 )& 0x7f) -106;
		pwdb_all = query_rx_pwr_percentage(rx_pwr_all);

		RT_TRACE(_module_rtl871x_recv_c_, _drv_err_, ("PWDB_ALL=%d\n",	pwdb_all));

		pattrib->RxPWDBAll = pwdb_all;	//for DIG/rate adaptive
		pattrib->RecvSignalPower = rx_pwr_all;

		//
		// (3)EVM of HT rate
		//
		if(pHalData->CustomerID != RT_CID_819x_Lenovo)
		{
			if(pattrib->rxht &&  pattrib->mcs_rate >=20 && pattrib->mcs_rate<=27)
				max_spatial_stream = 2; //both spatial stream make sense
			else
				max_spatial_stream = 1; //only spatial stream 1 makes sense

			for(i=0; i<max_spatial_stream; i++)
			{
				// Do not use shift operation like "rx_evmX >>= 1" because the compilor of free build environment
				// fill most significant bit to "zero" when doing shifting operation which may change a negative
				// value to positive one, then the dbm value (which is supposed to be negative)  is not correct anymore.
				evm = evm_db2percentage( (pphy_head[PHY_STAT_RXEVM_SHT+i] /*/ 2*/));//dbm

				RT_TRACE(_module_rtl871x_recv_c_, _drv_err_, ("RXRATE=%x RXEVM=%x EVM=%s%d\n",
					pattrib->mcs_rate, pphy_head[PHY_STAT_RXEVM_SHT+i], "%",evm));

				//if(bPacketMatchBSSID)
				{
					if(i==0) // Fill value in RFD, Get the first spatial stream only
					{
						pattrib->signal_qual = (u8)(evm & 0xff);
					}
					pattrib->rx_mimo_signal_qual[i] = (u8)(evm & 0xff);
				}
			}

		}

		//
		// 4. Record rx statistics for debug
		//

	}


	//UI BSS List signal strength(in percentage), make it good looking, from 0~100.
	//It is assigned to the BSS List in GetValueFromBeaconOrProbeRsp().
	if(bcck_rate)
	{
		pattrib->signal_strength=(u8)signal_scale_mapping(padapter, pwdb_all);
	}
	else
	{
		if (rf_rx_num != 0)
		{
			pattrib->signal_strength= (u8)(signal_scale_mapping(padapter, total_rssi/=rf_rx_num));
		}
	}

}


static void process_rssi(_adapter *padapter,union recv_frame *prframe)
{
	u32	last_rssi, tmp_val;
	struct rx_pkt_attrib *pattrib = &prframe->u.hdr.attrib;

	//if(pRfd->Status.bPacketToSelf || pRfd->Status.bPacketBeacon)
	{
		//Adapter->RxStats.RssiCalculateCnt++;	//For antenna Test
		if(padapter->recvpriv.signal_strength_data.total_num++ >= PHY_RSSI_SLID_WIN_MAX)
		{
			padapter->recvpriv.signal_strength_data.total_num = PHY_RSSI_SLID_WIN_MAX;
			last_rssi = padapter->recvpriv.signal_strength_data.elements[padapter->recvpriv.signal_strength_data.index];
			padapter->recvpriv.signal_strength_data.total_val -= last_rssi;
		}
		padapter->recvpriv.signal_strength_data.total_val  +=pattrib->signal_strength;

		padapter->recvpriv.signal_strength_data.elements[padapter->recvpriv.signal_strength_data.index++] = pattrib->signal_strength;
		if(padapter->recvpriv.signal_strength_data.index >= PHY_RSSI_SLID_WIN_MAX)
			padapter->recvpriv.signal_strength_data.index = 0;


		tmp_val = padapter->recvpriv.signal_strength_data.total_val/padapter->recvpriv.signal_strength_data.total_num;
		padapter->recvpriv.rssi=(s8)translate2dbm( padapter,(u8)tmp_val);

		RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("UI RSSI = %d, ui_rssi.TotalVal = %d, ui_rssi.TotalNum = %d\n", tmp_val, padapter->recvpriv.signal_strength_data.total_val,padapter->recvpriv.signal_strength_data.total_num));
	}

}// Process_UI_RSSI_8192S


static void process_PWDB(_adapter *padapter, union recv_frame *prframe)
{
	int	UndecoratedSmoothedPWDB;
	HAL_DATA_TYPE		*pHalData = GET_HAL_DATA(padapter);
	struct dm_priv		*pdmpriv = &pHalData->dmpriv;
	struct rx_pkt_attrib	*pattrib= &prframe->u.hdr.attrib;
	struct sta_info		*psta = prframe->u.hdr.psta;

	if(psta)
	{
		//UndecoratedSmoothedPWDB = pEntry->rssi_stat.UndecoratedSmoothedPWDB;//todo:
		UndecoratedSmoothedPWDB = pdmpriv->UndecoratedSmoothedPWDB;
	}
	else
	{
		UndecoratedSmoothedPWDB = pdmpriv->UndecoratedSmoothedPWDB;
	}

	//if(pRfd->Status.bPacketToSelf || pRfd->Status.bPacketBeacon)
	{
		if(UndecoratedSmoothedPWDB < 0) // initialize
		{
			UndecoratedSmoothedPWDB = pattrib->RxPWDBAll;
		}

		if(pattrib->RxPWDBAll > (u32)UndecoratedSmoothedPWDB)
		{
			UndecoratedSmoothedPWDB =
					( ((UndecoratedSmoothedPWDB)*(Rx_Smooth_Factor-1)) +
					(pattrib->RxPWDBAll)) /(Rx_Smooth_Factor);

			UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB + 1;
		}
		else
		{
			UndecoratedSmoothedPWDB =
					( ((UndecoratedSmoothedPWDB)*(Rx_Smooth_Factor-1)) +
					(pattrib->RxPWDBAll)) /(Rx_Smooth_Factor);
		}

		if(psta)
		{
			//pEntry->rssi_stat.UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB;//todo:
			pdmpriv->UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB;
		}
		else
		{
			pdmpriv->UndecoratedSmoothedPWDB = UndecoratedSmoothedPWDB;
		}

		//UpdateRxSignalStatistics8192C(Adapter, pRfd);
	}
}


static void process_link_qual(_adapter *padapter,union recv_frame *prframe)
{
	u32	last_evm=0, nSpatialStream, tmpVal;
 	struct rx_pkt_attrib *pattrib;

	if(prframe == NULL || padapter==NULL){
		return;
	}

	pattrib = &prframe->u.hdr.attrib;
	if(pattrib->signal_qual != 0)
	{
			//
			// 1. Record the general EVM to the sliding window.
			//
			if(padapter->recvpriv.signal_qual_data.total_num++ >= PHY_LINKQUALITY_SLID_WIN_MAX)
			{
				padapter->recvpriv.signal_qual_data.total_num = PHY_LINKQUALITY_SLID_WIN_MAX;
				last_evm = padapter->recvpriv.signal_qual_data.elements[padapter->recvpriv.signal_qual_data.index];
				padapter->recvpriv.signal_qual_data.total_val -= last_evm;
			}
			padapter->recvpriv.signal_qual_data.total_val += pattrib->signal_qual;

			padapter->recvpriv.signal_qual_data.elements[padapter->recvpriv.signal_qual_data.index++] = pattrib->signal_qual;
			if(padapter->recvpriv.signal_qual_data.index >= PHY_LINKQUALITY_SLID_WIN_MAX)
				padapter->recvpriv.signal_qual_data.index = 0;

			RT_TRACE(_module_rtl871x_recv_c_,_drv_info_,("Total SQ=%d  pattrib->signal_qual= %d\n", padapter->recvpriv.signal_qual_data.total_val, pattrib->signal_qual));

			// <1> Showed on UI for user, in percentage.
			tmpVal = padapter->recvpriv.signal_qual_data.total_val/padapter->recvpriv.signal_qual_data.total_num;
			padapter->recvpriv.signal_qual=(u8)tmpVal;
	}
	else
	{
		RT_TRACE(_module_rtl871x_recv_c_,_drv_err_,(" pattrib->signal_qual =%d\n", pattrib->signal_qual));
	}

}// Process_UiLinkQuality8192S


//void rtl8192c_process_phy_info(_adapter *padapter, union recv_frame *prframe)
void rtl8192d_process_phy_info(_adapter *padapter, void *prframe)
{
	union recv_frame *precvframe = (union recv_frame *)prframe;

	//
	// Check RSSI
	//
	process_rssi(padapter, precvframe);
	//
	// Check PWDB.
	//
	process_PWDB(padapter, precvframe); 
	//
	// Check EVM
	//
	process_link_qual(padapter,  precvframe);
}
