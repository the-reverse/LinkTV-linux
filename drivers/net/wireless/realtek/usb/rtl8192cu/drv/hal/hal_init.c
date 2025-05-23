/******************************************************************************
* hal_init.c                                                                                                                                 *
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

#define _HAL_INIT_C_
#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <rtw_byteorder.h>

#include <hal_init.h>

#ifdef CONFIG_SDIO_HCI
	#include <sdio_hal.h>
#elif defined(CONFIG_USB_HCI)
	#include <usb_hal.h>
#endif


uint	 rtw_hal_init(_adapter *padapter) 
{
	uint	status = _SUCCESS;
	
	padapter->hw_init_completed=_FALSE;

	padapter->bfirst_init = _TRUE;
	status = padapter->HalFunc.hal_init(padapter);

	if(status == _SUCCESS){
		padapter->hw_init_completed = _TRUE;
	}
	else{
	 	padapter->hw_init_completed = _FALSE;
		RT_TRACE(_module_hal_init_c_,_drv_err_,("rtw_hal_init: hal__init fail\n"));
	}
	padapter->bfirst_init = _FALSE;

	RT_TRACE(_module_hal_init_c_,_drv_err_,("-rtl871x_hal_init:status=0x%x\n",status));

	return status;

}	

uint	 rtw_hal_deinit(_adapter *padapter)
{
	uint	status = _SUCCESS;
	
_func_enter_;

	status = padapter->HalFunc.hal_deinit(padapter);

	if(status == _SUCCESS){
		padapter->hw_init_completed = _FALSE;
	}
	else
	{
		RT_TRACE(_module_hal_init_c_,_drv_err_,("\n rtw_hal_deinit: hal_init fail\n"));
	}
	
_func_exit_;
	
	return status;
	
}

