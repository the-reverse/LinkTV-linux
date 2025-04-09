EXTRA_CFLAGS += -I ../core/

ifdef CONFIG_MULTI_PORT_MARS_CEC
    obj-$(CONFIG_MARS_CEC) += cec_mars_multi.o gcec_rx.o
else
    obj-$(CONFIG_MARS_CEC) += cec_mars.o 
    obj-$(CONFIG_GCEC_FIXUP_EN) += gcec_fixup.o 
endif

