#ifndef __RTL8192D_LED_H_
#define __RTL8192D_LED_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


//================================================================================
// Interface to manipulate LED objects.
//================================================================================
#ifdef CONFIG_USB_HCI
void rtl8192du_InitSwLeds(_adapter *padapter);
void rtl8192du_DeInitSwLeds(_adapter *padapter);
#endif

#endif

