

#ifndef __OSDEP_INTF_H_
#define __OSDEP_INTF_H_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#define RND4(x)	(((x >> 2) + (((x & 3) == 0) ?  0: 1)) << 2)


struct intf_priv {
	
	u8 *intf_dev;
	u32	max_iosz; 	//USB2.0: 128, USB1.1: 64, SDIO:64
	u32	max_xmitsz; //USB2.0: unlimited, SDIO:512
	u32	max_recvsz; //USB2.0: unlimited, SDIO:512

	volatile u8 *io_rwmem;
	volatile u8 *allocated_io_rwmem;
	u32	io_wsz; //unit: 4bytes
	u32	io_rsz;//unit: 4bytes
	u8 intf_status;	
	
	void (*_bus_io)(u8 *priv);	

/*
Under Sync. IRP (SDIO/USB)
A protection mechanism is necessary for the io_rwmem(read/write protocol)

Under Async. IRP (SDIO/USB)
The protection mechanism is through the pending queue.
*/

	_rwlock rwlock;	

	
#ifdef PLATFORM_LINUX	
	#ifdef CONFIG_USB_HCI	
	// when in USB, IO is through interrupt in/out endpoints
	struct usb_device 	*udev;
	PURB	piorw_urb;
	u8 io_irp_cnt;
	u8 bio_irp_pending;
	_sema io_retevt;
	_timer	io_timer;
	u8 bio_irp_timeout;
	u8 bio_timer_cancel;
	#endif
#endif

#ifdef PLATFORM_OS_XP
	#ifdef CONFIG_SDIO_HCI
		// below is for io_rwmem...	
		PMDL pmdl;
		PSDBUS_REQUEST_PACKET  sdrp;
		PSDBUS_REQUEST_PACKET  recv_sdrp;
		PSDBUS_REQUEST_PACKET  xmit_sdrp;

			PIRP		piorw_irp;

	#endif
	#ifdef CONFIG_USB_HCI
		PURB	piorw_urb;
		PIRP		piorw_irp;
		u8 io_irp_cnt;
		u8 bio_irp_pending;
		_sema io_retevt;	
	#endif	
#endif

};	


struct intf_hdl;

extern uint _init_intf_hdl(_adapter *padapter, struct intf_hdl *pintf_hdl);
extern void _unload_intf_hdl(struct intf_priv *pintfpriv);

u32 rtw_open_fw(_adapter * padapter, void **pphfwfile_hdl, u8 **ppmappedfw);
void rtw_close_fw(_adapter *padapter, void *phfwfile_hdl);


#ifdef PLATFORM_LINUX
int rtw_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);

#ifdef RTK_DMP_PLATFORM
void rtw_proc_init_one(struct net_device *dev);
void rtw_proc_remove_one(struct net_device *dev);
#endif
#endif


#endif	//_OSDEP_INTF_H_

