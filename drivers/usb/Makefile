#
# Makefile for the kernel USB device drivers.
#

# Object files in subdirectories

obj-$(CONFIG_USB)		+= core/

obj-$(CONFIG_USB_MON)		+= mon/

obj-$(CONFIG_USB_EHCI_HCD)	+= host/
obj-$(CONFIG_USB_OHCI_HCD)	+= host/
obj-$(CONFIG_USB_UHCI_HCD)	+= host/
obj-$(CONFIG_USB_SL811_HCD)	+= host/
obj-$(CONFIG_ETRAX_USB_HOST)	+= host/

obj-$(CONFIG_USB_ACM)		+= class/
obj-$(CONFIG_USB_AUDIO)		+= class/
obj-$(CONFIG_USB_BLUETOOTH_TTY)	+= class/
obj-$(CONFIG_USB_MIDI)		+= class/
obj-$(CONFIG_USB_PRINTER)	+= class/

obj-$(CONFIG_USB_STORAGE)	+= storage/

obj-$(CONFIG_USB_AIPTEK)	+= input/
obj-$(CONFIG_USB_ATI_REMOTE)	+= input/
obj-$(CONFIG_USB_HID)		+= input/
obj-$(CONFIG_USB_KBD)		+= input/
obj-$(CONFIG_USB_KBTAB)		+= input/
obj-$(CONFIG_USB_MOUSE)		+= input/
obj-$(CONFIG_USB_MTOUCH)	+= input/
obj-$(CONFIG_USB_POWERMATE)	+= input/
obj-$(CONFIG_USB_WACOM)		+= input/
obj-$(CONFIG_USB_XPAD)		+= input/

obj-$(CONFIG_USB_UVC)       	+= media/
obj-$(CONFIG_USB_DABUSB)	+= media/
obj-$(CONFIG_USB_DSBR)		+= media/
obj-$(CONFIG_USB_IBMCAM)	+= media/
obj-$(CONFIG_USB_KONICAWC)	+= media/
obj-$(CONFIG_USB_OV511)		+= media/
obj-$(CONFIG_USB_PWC)		+= media/
obj-$(CONFIG_USB_SE401)		+= media/
obj-$(CONFIG_USB_SN9C102)	+= media/
obj-$(CONFIG_USB_STV680)	+= media/
obj-$(CONFIG_USB_VICAM)		+= media/
obj-$(CONFIG_USB_W9968CF)	+= media/

obj-$(CONFIG_USB_CATC)		+= net/
obj-$(CONFIG_USB_KAWETH)	+= net/
obj-$(CONFIG_USB_PEGASUS)	+= net/
obj-$(CONFIG_USB_RTL8150)	+= net/
obj-$(CONFIG_USB_USBNET)	+= net/
obj-$(CONFIG_USB_ZD1201)	+= net/

obj-$(CONFIG_USB_MDC800)	+= image/
obj-$(CONFIG_USB_MICROTEK)	+= image/

obj-$(CONFIG_USB_SERIAL)	+= serial/

obj-$(CONFIG_USB_AUERSWALD)	+= misc/
obj-$(CONFIG_USB_CYTHERM)	+= misc/
obj-$(CONFIG_USB_EMI26)		+= misc/
obj-$(CONFIG_USB_EMI62)		+= misc/
obj-$(CONFIG_USB_IDMOUSE)	+= misc/
obj-$(CONFIG_USB_LCD)		+= misc/
obj-$(CONFIG_USB_LED)		+= misc/
obj-$(CONFIG_USB_LEGOTOWER)	+= misc/
obj-$(CONFIG_USB_RIO500)	+= misc/
obj-$(CONFIG_USB_TEST)		+= misc/
obj-$(CONFIG_USB_USS720)	+= misc/
obj-$(CONFIG_USB_PHIDGETSERVO)	+= misc/

obj-$(CONFIG_USB_ATM)		+= atm/
obj-$(CONFIG_USB_SPEEDTOUCH)	+= atm/

# cyhuang (2011/02/25) : Add MTP/PTP support.
obj-$(CONFIG_USB_MTP)       += mtp/
