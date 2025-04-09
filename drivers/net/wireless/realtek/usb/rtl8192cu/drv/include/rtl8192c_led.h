#ifndef __RTL8192C_LED_H_
#define __RTL8192C_LED_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


//================================================================================
// Interface to manipulate LED objects.
//================================================================================
#ifdef CONFIG_USB_HCI
void rtl8192cu_InitSwLeds(_adapter *padapter);
void rtl8192cu_DeInitSwLeds(_adapter *padapter);
#endif
#ifdef CONFIG_PCI_HCI
void rtl8192ce_gen_RefreshLedState(PADAPTER Adapter);
void rtl8192ce_InitSwLeds(_adapter *padapter);
void rtl8192ce_DeInitSwLeds(_adapter *padapter);
#endif

#endif

