
#ifndef _RTL2832U_H_
#define _RTL2832U_H_



#include "dvb-usb.h"

#define	USB_VID_REALTEK					0x0BDA
#define	USB_PID_RTD2832U_WARM			0x2832 
#define	USB_PID_RTD2832U_2ND_WARM		0x2838 
#define	USB_PID_RTD2832U_3RD_WARM		0x2834 
#define USB_PID_RTD2832U_4TH_WARM       0x2837
#define USB_PID_RTL2836_WARM            0x2836
#define USB_PID_RTL2836_2ND_WARM        0x2839

#define	USB_VID_GOLDENBRIDGE				0x1680
#define	USB_PID_GOLDENBRIDGE_WARM		0xA332

#define	USB_VID_YUAN						0x1164
#define	USB_PID_YUAN_WARM				0x6601

#define	USB_VID_AZUREWAVE				0x13D3
#define	USB_PID_AZUREWAVE_MINI_WARM	0x3234
#define	USB_PID_AZUREWAVE_USB_WARM		0x3274
#define	USB_PID_AZUREWAVE_GPS_WARM		0x3282

#define	USB_VID_KWORLD_1ST					0x1B80
#define	USB_PID_KWORLD_WARM_4				0xD394
#define	USB_PID_KWORLD_WARM_6				0xD396
#define	USB_PID_KWORLD_WARM_3				0xD393
#define	USB_PID_KWORLD_WARM_7				0xD397
#define	USB_PID_KWORLD_WARM_8				0xD398

#define	USB_VID_DEXATEK					0x1D19
#define	USB_PID_DEXATEK_USB_WARM		0x1101
#define	USB_PID_DEXATEK_MINIUSB_WARM	0x1102
#define	USB_PID_DEXATEK_5217_WARM		0x1103
#define	USB_PID_DEXATEK_WARM_4		       0x1104

#define	USB_VID_GTEK                    0x1F4D
#define	USB_PID_GTEK_WARM			       0x0837
#define	USB_PID_GTEK_WARM_A			       0xA803
#define	USB_PID_GTEK_WARM_B			       0xB803
#define	USB_PID_GTEK_WARM_C			       0xC803
#define	USB_PID_GTEK_WARM_D			       0xD803

#define	USB_VID_THP					       0x1554
#define USB_PID_THP                        0x5013
#define	USB_PID_THP2                       0x5020

//#define RTD2831_URB_SIZE						   39480
//#define RTD2831_URB_NUMBER						 4
//#define RTD2831_URB_SIZE							 24064
#define RTD2831_URB_SIZE							 24064
#define RTD2831_URB_NUMBER							 16
//#define RTD2831_URB_NUMBER							 4
#define RTL2832U_STRAMING_ON             0x01
#define RTL2832U_CHANNEL_SCAN_MODE_EN    0x02
#define RTL2832U_URB_RDY                 0x04
#define RTL2832U_CHANNEL_SCAN_URB_SIZE	 512
#define RTL2832U_CHANNEL_SCAN_URB_NUMBER 64

#define RTL2832U_PID_COUNT               32



extern struct dvb_frontend * rtl2832u_fe_attach(struct dvb_usb_device *d);

#define rtl2832u_dbg(fmt, args...)      printk("[RTL2832U] Dbg, " fmt, ## args)	
#define rtl2832u_warning(fmt, args...)  printk("[RTL2832U] Warnig, " fmt, ## args)

#endif



