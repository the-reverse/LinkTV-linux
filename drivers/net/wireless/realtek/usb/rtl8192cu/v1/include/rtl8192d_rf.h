/******************************************************************************
 * 
 *     (c) Copyright  2008, RealTEK Technologies Inc. All Rights Reserved.
 * 
 * Module:	rtl8192d_rf.h	( Header File)
 * 
 * Note:	Collect every HAL RF type exter API or constant.	 
 *
 * Function:	
 * 		 
 * Export:	
 * 
 * Abbrev:	
 * 
 * History:
 * Data			Who		Remark
 * 
 * 09/25/2008	MHC		Create initial version.
 * 
 * 
******************************************************************************/
#ifndef _RTL8192D_RF_H_
#define _RTL8192D_RF_H_
/* Check to see if the file has been included already.  */


/*--------------------------Define Parameters-------------------------------*/

//
// For RF 6052 Series
//
#define		RF6052_MAX_TX_PWR			0x3F
#define		RF6052_MAX_REG				0x3F
#define		RF6052_MAX_PATH				2
/*--------------------------Define Parameters-------------------------------*/


/*------------------------------Define structure----------------------------*/ 

/*------------------------------Define structure----------------------------*/ 


/*------------------------Export global variable----------------------------*/
/*------------------------Export global variable----------------------------*/

/*------------------------Export Marco Definition---------------------------*/

/*------------------------Export Marco Definition---------------------------*/


/*--------------------------Exported Function prototype---------------------*/

//
// RF RL6052 Series API
//
void		rtl8192d_RF_ChangeTxPath(	IN	PADAPTER	Adapter, 
										IN	u16		DataRate);
void		rtl8192d_PHY_RF6052SetBandwidth(	
										IN	PADAPTER				Adapter,
										IN	HT_CHANNEL_WIDTH		Bandwidth);	
VOID	rtl8192d_PHY_RF6052SetCckTxPower(
										IN	PADAPTER	Adapter,
										IN	u8*		pPowerlevel);
VOID	rtl8192d_PHY_RF6052SetOFDMTxPower(
										IN	PADAPTER	Adapter,
										IN	u8*		pPowerLevel,
										IN	u8		Channel);
int	PHY_RF6052_Config8192D(	IN	PADAPTER		Adapter	);

/*--------------------------Exported Function prototype---------------------*/


#endif/* End of HalRf.h */

