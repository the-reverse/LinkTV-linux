NIC_SELECT = RTL8192CE


ifeq ($(NIC_SELECT),RTL8192CE)
    sub_folder = rtl8192c
    EXTRA_CFLAGS = -DRTL8192CE
    EXTRA_CFLAGS += -DEEPROM_OLD_FORMAT_SUPPORT=0 		\
		-DRTL8192SU_DISABLE_CCK_RATE=0 		\
		-DDISABLE_BB_RF=0 			\
		-DRTL92SE_FPGA_VERIFY=0	 		\
		-DSIMULATE_ETHERNET=0 			\
		-DSUPERMAC_92D_ENABLE=0			\
		-DEARLYMODE_ENABLE_FOR_92D=0 		\
		-DRTL8192S_DISABLE_FW_DM=0 		\
		-DMP_CCK_WORKAROUND=0			\
		-DRTL8192SE_DISABLE_MGNT_QUEUE=0	\
		-DRTL8190_Download_Firmware_From_Header=1 \
		-DMUTUAL_AUTHENTICATION=1		\
		-DFOR_QMI=0				\
		-DCONFIG_PM_RTL 			\
		-DCONFIG_PM 				\
		-DENABLE_IPS 				\
		-DENABLE_LPS 				\
		-DCONFIG_ASPM_OR_D3 			\
		-DENABLE_GPIO_RADIO_CTL 		\
		-DMP_DRIVER=0				\
		-DENABLE_DOT11D				\
		-DENABLE_DYNAMIC_TXPOWER		\
		-DCONFIG_CRDA	 			\
		-DCONFIG_CFG_80211 			\
		#					\
		-DENABLE_TKIP11N 			\
		-D_ENABLE_SW_BEACON 			\
		-DUSE_FW_SOURCE_IMG_FILE 		\
		-DEFUSE_REPG_WORKAROUND			\
		-DENABLE_AMSDU 				\
		-DADHOC_11N 				\
		-DFOR_ANDROID_X86			\
		-DCONFIG_64BIT_DMA			\
		-DCONFIG_BT_30				\
		#
endif
    
ifeq ($(NIC_SELECT),RTL8192SE)
    sub_folder = rtl8192s
    EXTRA_CFLAGS = -DRTL8192SE
    EXTRA_CFLAGS += -DEEPROM_OLD_FORMAT_SUPPORT=0 		\
    		-DRTL92SE_FPGA_VERIFY=0 		\
    		-DDEMO_BOARD_SUPPORT=1 			\
    		-DRTL8192SU_DISABLE_CCK_RATE=0 		\
    		-DRTL8192S_DISABLE_FW_DM=0 		\
    		-DHAL_MAC_ENABLE=1 			\
    		-DHAL_BB_ENABLE=1 			\
    		-DHAL_RF_ENABLE=1 			\
    		-DRTL8192S_PREPARE_FOR_NORMAL_RELEASE=1 \
    		-DDISABLE_BB_RF=0 			\
    		-DCONFIG_PM_RTL 			\
    		-DCONFIG_PM 				\
    		-DENABLE_GPIO_RADIO_CTL 		\
    		-DRTL8190_Download_Firmware_From_Header=1 \
    		-DENABLE_LPS 				\
    		-DCONFIG_RX_CMD				\
    		-DENABLE_IPS 				\
    		-DUSE_FW_SOURCE_IMG_FILE 		\
    		-DENABLE_DOT11D 			\
    		-DCONFIG_ASPM_OR_D3 			\
    		-DRTLLIB_RADIOTAP			\
    		#					\
    		-DCONFIG_CRDA	 			\
    		-DCONFIG_CFG_80211 			\
    		-DCONFIG_RTL_RFKILL	 		\
    		-DCONFIG_FW_SETCHAN			\
    		-DRTL8192S_WAPI_SUPPORT			\
    		-D_ENABLE_SW_BEACON 			\
    		-DCONFIG_MP 				\
    		-DENABLE_AMSDU 				\
    		-DADHOC_11N				\
    		-DEFUSE_REPG_WORKAROUN			\
    		-DENABLE_TKIP11N			\
    		-DFOR_ANDROID_X86			\
    		#
    		#NOTICE:
    		#[1] CONFIG_MP is for MP test, 
    		#    you should close IPS/LPS/GPIO/ASPM 
    		#[2] In order to support CRDA, RFKILL 
          		#    CONFIG_CFG_80211 need to be enabled at first.
    		#[3] In order to support CRDA, 
          		#    DOT11D need to be enabled at first.
    		#[4] CRDA and RFKILL can be enabled individually.                         		
endif

ifeq ($(NIC_SELECT),RTL8190P)
    sub_folder = rtl8192e
    EXTRA_CFLAGS += -DRTL8190P				\
    		-DUSE_FW_SOURCE_IMG_FILE		\
    		-DEEPROM_OLD_FORMAT_SUPPORT=1		\
    		-DENABLE_DOT11D 			\
    		-DCONFIG_PM				\
    		-DCONFIG_PM_RTL 			\
    		#					\
    		-DENABLE_TKIP11N			\
    		-DFOR_ANDROID_X86			\
    		#
endif

ifeq ($(NIC_SELECT),RTL8192E)
    sub_folder = rtl8192e
    EXTRA_CFLAGS+=-DRTL8192E				\
		-DEEPROM_OLD_FORMAT_SUPPORT=1		\
		-DUSE_FW_SOURCE_IMG_FILE		\
		-DENABLE_IPS				\
		-DENABLE_LPS				\
		-DENABLE_GPIO_RADIO_CTL			\
		-DCONFIG_PM_RTL 			\
		-DCONFIG_PM 				\
		-DENABLE_DOT11D 			
		#					\
		-DCONFIG_RTL8192_IO_MAP			\
		-DCONFIG_ASPM_OR_D3 			\
		-DENABLE_TKIP11N			\
		-DFOR_ANDROID_X86			\
		#
endif

ifeq ($(NIC_SELECT),RTL8191SE_MESH)
sub_folder = rtl8192s
EXTRA_CFLAGS += -DRTL8192SE 				\
		-DEEPROM_OLD_FORMAT_SUPPORT=0 		\
		-DRTL92SE_FPGA_VERIFY=0 		\
		-DDEMO_BOARD_SUPPORT=1 			\
		-DRTL8192SU_DISABLE_CCK_RATE=0 		\
		-DRTL8192S_DISABLE_FW_DM=0 		\
		-DHAL_MAC_ENABLE=1 			\
		-DHAL_BB_ENABLE=1 			\
		-DHAL_RF_ENABLE=1 			\
		-DRTL8192S_PREPARE_FOR_NORMAL_RELEASE=1 \
		-DDISABLE_BB_RF=0 			\
		-DRTL8190_Download_Firmware_From_Header=1 \
		-DEFUSE_REPG_WORKAROUND 		\
		-DCONFIG_PM 				\
		-DCONFIG_PM_RTL 			\
		-DENABLE_GPIO_RADIO_CTL			\
		-DENABLE_IPS 				\
		-DENABLE_LPS				\
		-DCONFIG_ASPM_OR_D3 			\
		-DCUSTOMER_ID_INTEL_CMPC 		\
		-DMESH_8192_SPECIFIED 			\
		-D_RTL8192_EXT_PATCH_ 			\
		-D_MESH_DEBUG_ 				\
		-DTREE_BASED_ROUTING_ENABLE 		\
		-DMESH_PATHSEL_DBG_LV=0 		\
		-DMESH_PATHSEL_DBG_SIMPLE=0 		\
		-DMESH_PATHSEL_DBG_TEST=0		\
		-DBUILT_IN_MSHCLASS 			\
		-DENABLE_AMSDU				\
		-DUSE_FW_SOURCE_IMG_FILE 		\
		-DENABLE_DOT11D 			\
		-DCONFIG_RX_CMD				\
		#					\
		-DADHOC_11N				\
		-DCOMPATIBLE_WITH_RALINK_MESH 		\
		-DCONFIG_CRDA	 			\
		-DCONFIG_FW_SETCHAN			\
		-DRTL8192S_WAPI_SUPPORT			\
		-DMESH_AUTO_TEST			\
		-D_ENABLE_SW_BEACON			\
		-DEFUSE_REPG_WORKAROUND 		\
		-DENABLE_TKIP11N			\
		-DFOR_ANDROID_X86			\
		-DCONFIG_MP 				\
		#
endif

ifeq ($(NIC_SELECT),RTL8192CE_MESH)
sub_folder = rtl8192c
EXTRA_CFLAGS = -DRTL8192CE
EXTRA_CFLAGS += -DEEPROM_OLD_FORMAT_SUPPORT=0 		\
		-DRTL8192SU_DISABLE_CCK_RATE=0 		\
		-DDISABLE_BB_RF=0 			\
		-DRTL92SE_FPGA_VERIFY=0	 		\
		-DSIMULATE_ETHERNET=0 			\
		-DSUPERMAC_92D_ENABLE=0			\
		-DEARLYMODE_ENABLE_FOR_92D=0 		\
		-DRTL8192S_DISABLE_FW_DM=0 		\
		-DMP_CCK_WORKAROUND=0			\
		-DRTL8192SE_DISABLE_MGNT_QUEUE=0	\
		-DRTL8190_Download_Firmware_From_Header=1 \
		-DMUTUAL_AUTHENTICATION=1		\
		-DFOR_QMI=0				\
		-DENABLE_TKIP11N 			\
		-DCONFIG_PM_RTL 			\
		-DCONFIG_PM 				\
		-DENABLE_IPS 				\
		-DENABLE_LPS 				\
		-DCONFIG_ASPM_OR_D3 			\
		-DENABLE_GPIO_RADIO_CTL 		\
		-DMP_DRIVER=0				\
		-DENABLE_DOT11D				\
		-DEFUSE_REPG_WORKAROUND 		\
		-DCUSTOMER_ID_INTEL_CMPC 		\
		-DMESH_8192_SPECIFIED 			\
		-D_RTL8192_EXT_PATCH_ 			\
		-D_MESH_DEBUG_ 				\
		-DTREE_BASED_ROUTING_ENABLE 		\
		-DMESH_PATHSEL_DBG_LV=0 		\
		-DMESH_PATHSEL_DBG_SIMPLE=0 		\
		-DMESH_PATHSEL_DBG_TEST=0		\
		-DBUILT_IN_MSHCLASS 			\
		-DENABLE_AMSDU				\
		-DENABLE_DYNAMIC_TXPOWER		\
		#					\
		-D_ENABLE_SW_BEACON 			\
		-DUSE_FW_SOURCE_IMG_FILE 		\
		-DCOMPATIBLE_WITH_RALINK_MESH 		\
		-DEFUSE_REPG_WORKAROUND			\
		-DENABLE_AMSDU 				\
		-DADHOC_11N 				\
		-DFOR_ANDROID_X86			\
		-DCONFIG_64BIT_DMA			\
		#
endif

#it will fail to compile in suse linux enterprise 10 sp2. This flag is to solve this problem.
ifeq ($(shell uname -r | cut -d. -f1,2,3,4), 2.6.16.60-0)
	EXTRA_CFLAGS += -DOPENSUSE_SLED=1
else
	EXTRA_CFLAGS += -DOPENSUSE_SLED=0
endif

# Switch AP will fail in moblin2. This flag is to solve this issue.
ifeq ($(shell uname -r | cut -d. -f6 | cut -d- -f1), moblin2)
	EXTRA_CFLAGS += -DFOR_MOBLIN
endif

EXTRA_CFLAGS += -I$(TOPDIR)/drivers/net/wireless
EXTRA_CFLAGS += -std=gnu89
EXTRA_CFLAGS += -O2
EXTRA_CFLAGS += -mhard-float -DCONFIG_FORCE_HARD_FLOAT=y
EXTRA_CFLAGS += -DTHOMAS_TURBO
#flag for single module compile
EXTRA_CFLAGS += -DBUILT_IN_RTLLIB
#EXTRA_CFLAGS += -DCONFIG_RTLWIFI_DEBUGFS

ifeq ($(NIC_SELECT),RTL8190P)
r8190_pci-objs := rtl_core.o 		\
		  rtl_eeprom.o 		\
		  rtl_ps.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_dm.o 		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_debug.o 		\
		  rtl_ethtool.o		\
		  rtl8192e/r8190P_hwimg.o	\
		  rtl8192e/r8190P_rtl8256.o 	\
		  rtl8192e/r8192E_dev.o		\
		  rtl8192e/r8192E_phy.o 		\
		  rtl8192e/r8192E_firmware.o 	\
		  rtl8192e/r8192E_cmdpkt.o 	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o

obj-$(CONFIG_REALTEK_PCI_RTL8192CE) := r8190_pci.o
endif

ifeq ($(NIC_SELECT),RTL8192E)
r8192e_pci-objs := rtl_core.o 		\
		  rtl_eeprom.o 	\
		  rtl_ps.o 		\
		  rtl_wx.o   	\
		  rtl_cam.o		\
		  rtl_dm.o 		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o		\
		  rtl8192e/r8192E_dev.o		\
		  rtl8192e/r8192E_phy.o 		\
		  rtl8192e/r8192E_firmware.o 	\
		  rtl8192e/r8192E_cmdpkt.o 	\
		  rtl8192e/r8192E_hwimg.o 	\
		  rtl8192e/r8190P_rtl8256.o 	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o
obj-$(CONFIG_REALTEK_PCI_RTL8192CE) := r8192e_pci.o
endif

ifeq ($(NIC_SELECT),RTL8192SE)
r8192se_pci-objs := rtl_core.o 		\
		  rtl_regd.o		\
		  rtl_rfkill.o		\
		  rtl_eeprom.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_ps.o 		\
		  rtl_dm.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o         \
		  rtl8192s/r8192S_dev.o 	\
		  rtl8192s/r8192S_Efuse.o 	\
		  rtl8192s/r8192S_phy.o 	\
		  rtl8192s/r8192S_firmware.o 	\
		  rtl8192s/r8192S_rtl6052.o 	\
		  rtl8192s/r8192S_hwimg.o 	\
		  rtl8192s/r8192S_led.o 	\
		  rtl8192s/r8192S_mp.o		\
		  rtl8192s/r8192S_scan.o	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o		\
		  ../../rtllib/wapi.o        			\
		  ../../rtllib/wapi_interface.o
#		  rtl8192e/r8190P_rtl8256.o 	
obj-$(CONFIG_REALTEK_PCI_RTL8192CE) := r8192se_pci.o
endif

ifeq ($(NIC_SELECT),RTL8192CE)
r8192ce_pci-objs := rtl_core.o 		\
		  rtl_eeprom.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_ps.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o         \
		  rtl_regd.o		\
		  rtl8192c/r8192C_dev.o 	\
		  rtl8192c/r8192C_tx.o 		\
		  rtl8192c/r8192C_rx.o 		\
		  rtl8192c/r8192C_Efuse.o 	\
		  rtl8192c/r8192C_phy.o 	\
		  rtl8192c/r8192C_firmware.o 	\
		  rtl8192c/r8192C_dm.o 		\
		  rtl8192c/r8192C_rtl6052.o 	\
		  rtl8192c/r8192C_hwimg.o 	\
		  rtl8192c/r8192C_led.o 	\
		  rtl8192c/r8192C_com.o 	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o      \
		  # BT3.0				\
		  ../../btlib/bt_aes.o			\
		  ../../btlib/bt_rc4.o			\
		  ../../btlib/bt_md5c.o			\
		  ../../btlib/bt_kmsm_hmac.o		\
		  ../../btlib/bt_kmsm_prf.o		\
		  ../../btlib/bt_kmsm_eapolkey.o	\
		  ../../btlib/bt_hci.o			\
		  ../../btlib/bt_handlePacket.o		\
		  ../../btlib/bt_wifi.o
obj-$(CONFIG_REALTEK_PCI_RTL8192CE) := r8192ce_pci.o
endif

ifeq ($(NIC_SELECT),RTL8191SE_MESH)
r8192se_pci_mesh-objs := rtl_core.o 	\
		  rtl_regd.o		\
		  rtl_rfkill.o		\
		  rtl_eeprom.o 		\
		  rtl_ps.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_dm.o 		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o         \
		  rtl_mesh.o		\
		  rtl8192s/r8192S_dev.o \
		  rtl8192s/r8192S_Efuse.o 	\
		  rtl8192s/r8192S_phy.o 	\
		  rtl8192s/r8192S_firmware.o 	\
		  rtl8192s/r8192S_rtl6052.o 	\
		  rtl8192s/r8192S_hwimg.o 	\
		  rtl8192s/r8192S_led.o 	\
		  rtl8192s/r8192S_mp.o		\
		  rtl8192s/r8192S_scan.o		\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o           \
		  ../../rtllib/rtl819x_TSProc.o           \
		  ../../rtllib/rtl819x_BAProc.o           \
		  ../../rtllib/dot11d.o                   \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o	  \
		  ../../mshclass/msh_class.o 		  \
		  ../../mshclass/mesh_8192_util.o  	  \
		  ../../mshclass/rtl8192_mesh.o 	  \
		  ../../mshclass/rtl8192s_sme.o 	  \
	  	  ../../mshclass/rtl8192_proc.o 	  \
		  ../../mshclass/8192s_pathsel.o 	  \
		  ../../mshclass/hash_table.o 		  \
		  ../../mshclass/rtl8192_sha1.o           \
		  ../../mshclass/rtl8192_sha256.o         \
		  ../../mshclass/rtl8192_aes.o            \
		  ../../mshclass/mesh_abbr_handshk.o      \
		  ../../mshclass/rtl8192_bn_ctx.o         \
		  ../../mshclass/rtl8192_bn_asm.o         \
		  ../../mshclass/rtl8192_bn_gcd.o         \
		  ../../mshclass/rtl8192_bn_mont.o        \
		  ../../mshclass/rtl8192_bn_recp.o        \
		  ../../mshclass/rtl8192_bn_lib.o         \
		  ../../mshclass/mesh_ath.o		  \
		  ../../mshclass/pathselection.o 	  \
		  ../../mshclass/tree_proactive.o
#		  rtl8192e/r8190P_rtl8256.o 	
obj-$(CONFIG_REALTEK_PCI_RTL8192CE) := r8192se_pci_mesh.o
endif

ifeq ($(NIC_SELECT),RTL8192CE_MESH)
r8192ce_pci_mesh-objs := rtl_core.o 		\
		  rtl_eeprom.o 		\
		  rtl_wx.o 		\
		  rtl_cam.o		\
		  rtl_pm.o 		\
		  rtl_pci.o 		\
		  rtl_ps.o 		\
		  rtl_debug.o		\
		  rtl_ethtool.o         \
		  rtl_mesh.o		\
		  rtl8192c/r8192C_dev.o 	\
		  rtl8192c/r8192C_tx.o 		\
		  rtl8192c/r8192C_rx.o 		\
		  rtl8192c/r8192C_Efuse.o 	\
		  rtl8192c/r8192C_phy.o 	\
		  rtl8192c/r8192C_firmware.o 	\
		  rtl8192c/r8192C_dm.o 		\
		  rtl8192c/r8192C_rtl6052.o 	\
		  rtl8192c/r8192C_hwimg.o 	\
		  rtl8192c/r8192C_led.o 	\
		  rtl8192c/r8192C_com.o 	\
		  rtl8192c/r8192C_PhyParam.o 	\
		  ../../rtllib/rtllib_rx.o                \
		  ../../rtllib/rtllib_softmac.o           \
		  ../../rtllib/rtllib_tx.o                \
		  ../../rtllib/rtllib_wx.o                \
		  ../../rtllib/rtllib_module.o            \
		  ../../rtllib/rtllib_softmac_wx.o        \
		  ../../rtllib/rtl819x_HTProc.o              \
		  ../../rtllib/rtl819x_TSProc.o              \
		  ../../rtllib/rtl819x_BAProc.o              \
		  ../../rtllib/dot11d.o                      \
		  ../../rtllib/rtllib_crypt.o             \
		  ../../rtllib/rtllib_crypt_tkip.o        \
		  ../../rtllib/rtllib_crypt_ccmp.o        \
		  ../../rtllib/rtllib_crypt_wep.o      \
		  ../../mshclass/msh_class.o 		  \
		  ../../mshclass/mesh_8192_util.o  	  \
		  ../../mshclass/rtl8192_mesh.o 	  \
		  ../../mshclass/rtl8192s_sme.o 	  \
	  	  ../../mshclass/rtl8192_proc.o 	  \
		  ../../mshclass/8192s_pathsel.o 	  \
		  ../../mshclass/hash_table.o 		  \
		  ../../mshclass/rtl8192_sha1.o           \
		  ../../mshclass/rtl8192_sha256.o         \
		  ../../mshclass/rtl8192_aes.o            \
		  ../../mshclass/mesh_abbr_handshk.o      \
		  ../../mshclass/rtl8192_bn_ctx.o         \
		  ../../mshclass/rtl8192_bn_asm.o         \
		  ../../mshclass/rtl8192_bn_gcd.o         \
		  ../../mshclass/rtl8192_bn_mont.o        \
		  ../../mshclass/rtl8192_bn_recp.o        \
		  ../../mshclass/rtl8192_bn_lib.o         \
		  ../../mshclass/mesh_ath.o		  \
		  ../../mshclass/pathselection.o 	  \
		  ../../mshclass/tree_proactive.o
		  
obj-$(CONFIG_REALTEK_PCI_RTL8192CE) := r8192ce_pci_mesh.o
endif

EXTRA_CFLAGS += -DRTK_DMP_PLATFORM
EXTRA_CFLAGS += -DUSE_FW_SOURCE_IMG_FILE
