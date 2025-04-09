#ifndef __RTL8192C_LED_H_
#define __RTL8192C_LED_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


//================================================================================
// Interface to manipulate LED objects.
//================================================================================
void rtl8192c_InitSwLeds(_adapter *padapter);
void rtl8192c_DeInitSwLeds(_adapter *padapter);

#endif

