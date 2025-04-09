#ifndef __PCI_OPS_H_
#define __PCI_OPS_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <osdep_intf.h>

//s32	rtl8192ce_init_rx_ring(_adapter * padapter);
//void	rtl8192ce_free_rx_ring(_adapter * padapter);
//s32	rtl8192ce_init_tx_ring(_adapter * padapter, unsigned int prio, unsigned int entries);
//void	rtl8192ce_free_tx_ring(_adapter * padapter, unsigned int prio);

u32	rtl8192ce_init_desc_ring(_adapter * padapter);
u32	rtl8192ce_free_desc_ring(_adapter * padapter);
void	rtl8192ce_reset_desc_ring(_adapter * padapter);

u8	PlatformEnable92CEDMA64(PADAPTER Adapter);

int	rtl8192ce_interrupt(PADAPTER Adapter);

void rtl8192ce_xmit_tasklet(void *priv);

void rtl8192ce_recv_tasklet(void *priv);

void rtl8192ce_set_intf_ops(struct _io_ops	*pops);

#endif
