EXTRA_CFLAGS += -I drivers/media/dvb/dvb-usb/
EXTRA_CFLAGS += -I drivers/media/dvb/dvb-core/
EXTRA_CFLAGS += -I drivers/media/dvb/frontends/
EXTRA_CFLAGS += -I drivers/media/dvb/dvb-usb/realtek/

dvb-usb-rtl2832u-drv-objs  = dvbt_demod_base.o \
                             dvbt_nim_base.o \
                             dtmb_demod_base.o \
                             dtmb_nim_base.o \
                             foundation.o \
                             math_mpi.o    
                                                      
dvb-usb-rtl2832u-drv-objs += demod_rtl2832.o \
                             demod_rtl2836.o
                             
dvb-usb-rtl2832u-drv-objs += tuner_mxl5007t.o \
                             tuner_fc2580.o \
                             tuner_mt2266.o \
                             tuner_e4000.o \
                             tuner_fc0012.o \
                             tuner_tua9001.o
                             
dvb-usb-rtl2832u-drv-objs += nim_rtl2832_e4000.o\
                             nim_rtl2832_fc0012.o\
                             nim_rtl2832_mxl5007t.o \
                             nim_rtl2832_fc2580.o \
                             nim_rtl2832_mt2266.o \
                             nim_rtl2832_tua9001.o\
                             nim_rtl2836_fc2580.o\
                             nim_rtl2836_mxl5007t.o

obj-$(CONFIG_DVB_USB_RTL2832U) += dvb-usb-rtl2832u-drv.o
