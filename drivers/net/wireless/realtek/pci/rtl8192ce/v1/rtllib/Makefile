NIC_SELECT = RTL8192CE

EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -O2

#it will fail to compile in suse linux enterprise 10 sp2. This flag is to solve this problem.
ifeq ($(shell uname -r | cut -d. -f1,2,3,4), 2.6.16.60-0)
EXTRA_CFLAGS += -DOPENSUSE_SLED
endif

ifeq ($(NIC_SELECT), RTL8192SE)
	EXTRA_CFLAGS += -DRTL8192S_DISABLE_FW_DM=0
	EXTRA_CFLAGS += -DRTL8192SE
	EXTRA_CFLAGS += -DENABLE_IPS
	EXTRA_CFLAGS += -DENABLE_LPS
endif

ifeq ($(NIC_SELECT), RTL8192SU)
	EXTRA_CFLAGS += -DRTL8192S_DISABLE_FW_DM=0
	EXTRA_CFLAGS += -DRTL8192SU
	EXTRA_CFLAGS += -DENABLE_IPS
	EXTRA_CFLAGS += -DENABLE_LPS
	EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
	EXTRA_CFLAGS += -DTCP_CSUM_OFFLOAD_RX
endif

ifeq ($(NIC_SELECT), RTL8190P)
	EXTRA_CFLAGS += -DRTL8190P
endif

ifeq ($(NIC_SELECT), RTL8192E)
    EXTRA_CFLAGS += -DRTL8192E
    EXTRA_CFLAGS += -DENABLE_IPS
    EXTRA_CFLAGS += -DENABLE_LPS
endif

ifeq ($(NIC_SELECT),RTL8192U)
    EXTRA_CFLAGS += -DRTL8192U
    #EXTRA_CFLAGS += -DENABLE_AMSDU
    EXTRA_CFLAGS += -DUSB_TX_DRIVER_AGGREGATION_ENABLE
    EXTRA_CFLAGS += -DUSB_RX_AGGREGATION_SUPPORT
    
    # For RTK DMP platform
    #EXTRA_CFLAGS += -DRTK_DMP_PLATFORM
    #EXTRA_CFLAGS += -DWIFI_TEST        
endif

ifeq ($(NIC_SELECT), RTL8191SE_MESH)
    AUTHTYPE=SAE
    #AUTHTYPE=OPEN
    EXTRA_CFLAGS += -DRTL8192SE \
    		-DRTL8192S_DISABLE_FW_DM=0 \
    		-D_RTL8192_EXT_PATCH_\
    		-DENABLE_IPS \
    		-DENABLE_LPS \
    		-DCUSTOMER_ID_INTEL_CMPC
    ifeq ($(AUTHTYPE),SAE)
        EXTRA_CFLAGS += -DMESH_ABBR_HANDSHK_SUPPORT -DMESH_ATH_2
    endif
endif

ifeq ($(NIC_SELECT), RTL8192CE)
    EXTRA_CFLAGS += -DRTL8192CE        
endif
        
#EXTRA_CFLAGS += -DJOHN_NOCPY
EXTRA_CFLAGS += -DTHOMAS_TURBO
#flags to enable or disble 80211D feature
EXTRA_CFLAGS += -DENABLE_DOT11D
EXTRA_CFLAGS += -DRTK_DMP_PLATFORM

    
rtllib-rsl-objs := rtllib_rx.o \
    		      rtllib_softmac.o \
    		      rtllib_tx.o \
    		      rtllib_wx.o \
    		      rtllib_module.o \
    		      rtllib_softmac_wx.o\
    		      rtl819x_HTProc.o\
    		      rtl819x_TSProc.o\
    		      rtl819x_BAProc.o\
    		      dot11d.o

rtllib_crypt-rsl-objs := rtllib_crypt.o
rtllib_crypt_tkip-rsl-objs := rtllib_crypt_tkip.o
rtllib_crypt_ccmp-rsl-objs := rtllib_crypt_ccmp.o
rtllib_crypt_wep-rsl-objs := rtllib_crypt_wep.o
    
obj-$(CONFIG_REALTEK_PCI_RTL8192CE) += rtllib-rsl.o
obj-$(CONFIG_REALTEK_PCI_RTL8192CE) += rtllib_crypt-rsl.o
obj-$(CONFIG_REALTEK_PCI_RTL8192CE) += rtllib_crypt_wep-rsl.o
obj-$(CONFIG_REALTEK_PCI_RTL8192CE) += rtllib_crypt_tkip-rsl.o
obj-$(CONFIG_REALTEK_PCI_RTL8192CE) += rtllib_crypt_ccmp-rsl.o
