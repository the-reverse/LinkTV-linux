#
# Makefile for the USB input drivers
#

# Multipart objects.
usbhid-objs	:= hid-core.o

# Optional parts of multipart objects.

ifeq ($(CONFIG_USB_HIDDEV),y)
	usbhid-objs	+= hiddev.o
endif
ifeq ($(CONFIG_USB_HIDINPUT),y)
	usbhid-objs	+= hid-input.o
endif
ifeq ($(CONFIG_HID_PID),y)
	usbhid-objs	+= pid.o
endif
ifeq ($(CONFIG_LOGITECH_FF),y)
	usbhid-objs	+= hid-lgff.o
endif
ifeq ($(CONFIG_THRUSTMASTER_FF),y)
	usbhid-objs	+= hid-tmff.o
endif
ifeq ($(CONFIG_HID_FF),y)
	usbhid-objs	+= hid-ff.o
endif

# cyhuang (2011/05/16) : Add for realtek keymap (MCE or keyboard media key support)
ifeq ($(CONFIG_REALTEK_USE_RTD_KEYMAP),y)
    usbhid-objs += hid-rtd.o
endif

obj-$(CONFIG_USB_AIPTEK)	+= aiptek.o
obj-$(CONFIG_USB_ATI_REMOTE)	+= ati_remote.o
obj-$(CONFIG_USB_HID)		+= usbhid.o
obj-$(CONFIG_USB_KBD)		+= usbkbd.o
obj-$(CONFIG_USB_KBTAB)		+= kbtab.o
obj-$(CONFIG_USB_MOUSE)		+= usbmouse.o
obj-$(CONFIG_USB_MTOUCH)	+= mtouchusb.o
obj-$(CONFIG_USB_EGALAX)	+= touchkitusb.o
obj-$(CONFIG_USB_POWERMATE)	+= powermate.o
obj-$(CONFIG_USB_WACOM)		+= wacom.o
obj-$(CONFIG_USB_XPAD)		+= xpad.o

ifeq ($(CONFIG_USB_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

