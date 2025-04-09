#ifndef __PCI_OSINTF_H
#define __PCI_OSINTF_H

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>


void rtw_pci_disable_aspm(_adapter *padapter);
void rtw_pci_enable_aspm(_adapter *padapter);

void rtw_intf_start(_adapter *padapter);

#endif

