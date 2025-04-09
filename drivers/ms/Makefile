#
# Makefile for the kernel ms device drivers.
#

#
# Core
#
obj-$(CONFIG_MS)		+= ms_core.o

#
# Media drivers
#
obj-$(CONFIG_MS_BLOCK)		+= ms_block.o

#
# Host drivers
#
obj-$(CONFIG_MS_RTKMS)         += rtk_ms.o

ms_core-y := ms.o ms_queue.o ms_sysfs.o 
