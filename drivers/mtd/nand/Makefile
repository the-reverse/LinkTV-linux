#
# linux/drivers/nand/Makefile
#
# $Id: Makefile.common,v 1.15 2004/11/26 12:28:22 dedekind Exp $

#obj-$(CONFIG_MTD_NAND)			+= nand.o nand_ecc.o
obj-$(CONFIG_MTD_NAND)			+= nand.o
#obj-$(CONFIG_MTD_NAND_IDS)		+= nand_ids.o

obj-$(CONFIG_MTD_NAND_SPIA)		+= spia.o
obj-$(CONFIG_MTD_NAND_TOTO)		+= toto.o
obj-$(CONFIG_MTD_NAND_AUTCPU12)		+= autcpu12.o
obj-$(CONFIG_MTD_NAND_EDB7312)		+= edb7312.o
obj-$(CONFIG_MTD_NAND_TX4925NDFMC)	+= tx4925ndfmc.o
obj-$(CONFIG_MTD_NAND_TX4938NDFMC)	+= tx4938ndfmc.o
obj-$(CONFIG_MTD_NAND_AU1550)		+= au1550nd.o
obj-$(CONFIG_MTD_NAND_PPCHAMELEONEVB)	+= ppchameleonevb.o
obj-$(CONFIG_MTD_NAND_S3C2410)		+= s3c2410.o
obj-$(CONFIG_MTD_NAND_DISKONCHIP)	+= diskonchip.o
obj-$(CONFIG_MTD_NAND_H1900)		+= h1910.o
obj-$(CONFIG_MTD_NAND_RTC_FROM4)	+= rtc_from4.o
obj-$(CONFIG_MTD_NAND_SHARPSL)		+= sharpsl.o
obj-$(CONFIG_MTD_NAND_NANDSIM)		+= nandsim.o
obj-$(CONFIG_MTD_NAND_RTK)		+= rtk_nand.o rtk_nand_test.o
#obj-$(CONFIG_MTD_NAND_MARS)             += rtk_nand.o nand_base_mars.o

nand-objs = nand_base_rtk.o

#nand-objs = nand_base_mars.o
#nand-objs = nand_base.o nand_bbt.o
