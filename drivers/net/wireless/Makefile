#
# Makefile for the Linux Wireless network device drivers.
#

obj-$(CONFIG_STRIP) += strip.o
obj-$(CONFIG_ARLAN) += arlan.o 

arlan-objs := arlan-main.o arlan-proc.o

# Obsolete cards
obj-$(CONFIG_WAVELAN)		+= wavelan.o
obj-$(CONFIG_PCMCIA_NETWAVE)	+= netwave_cs.o
obj-$(CONFIG_PCMCIA_WAVELAN)	+= wavelan_cs.o

obj-$(CONFIG_HERMES)		+= orinoco.o hermes.o
obj-$(CONFIG_PCMCIA_HERMES)	+= orinoco_cs.o
obj-$(CONFIG_APPLE_AIRPORT)	+= airport.o
obj-$(CONFIG_PLX_HERMES)	+= orinoco_plx.o
obj-$(CONFIG_PCI_HERMES)	+= orinoco_pci.o
obj-$(CONFIG_TMD_HERMES)	+= orinoco_tmd.o

obj-$(CONFIG_AIRO)		+= airo.o
obj-$(CONFIG_AIRO_CS)		+= airo_cs.o airo.o

obj-$(CONFIG_ATMEL)             += atmel.o
obj-$(CONFIG_PCI_ATMEL)         += atmel_pci.o 
obj-$(CONFIG_PCMCIA_ATMEL)      += atmel_cs.o

obj-$(CONFIG_PRISM54)		+= prism54/

# 16-bit wireless PCMCIA client drivers
obj-$(CONFIG_PCMCIA_RAYCS)	+= ray_cs.o
obj-$(CONFIG_PCMCIA_WL3501)	+= wl3501_cs.o

obj-$(CONFIG_REALTEK_PCI_RTL8185)   += realtek/pci/rtl8185/

obj-$(CONFIG_REALTEK_PCI_RTL8190)   += realtek/pci/rtl8190/

obj-$(CONFIG_REALTEK_PCI_RTL8192SE)	+= realtek/pci/rtl8192se/

obj-$(CONFIG_REALTEK_PCI_RTL8192CE)	+= realtek/pci/rtl8192ce/

obj-$(CONFIG_REALTEK_USB_RTL8187)   += realtek/usb/rtl8187/

obj-$(CONFIG_REALTEK_USB_RTL8709)   += realtek/usb/rtl8709/

obj-$(CONFIG_REALTEK_USB_RTL8192)   += realtek/usb/11n8709/

obj-$(CONFIG_REALTEK_USB_RTL8191SU) += realtek/usb/rtl8712/

obj-$(CONFIG_REALTEK_USB_RTL8192CU) += realtek/usb/rtl8192cu/

obj-$(CONFIG_REALTEK_USB_RTL8192DU) += realtek/usb/rtl8192du/

