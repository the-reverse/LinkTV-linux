#define _HCI_INTF_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>
#include <recv_osdep.h>
#include <xmit_osdep.h>
#include <hal_init.h>
#include <rtw_version.h>

#ifndef CONFIG_PCI_HCI

#error "CONFIG_PCI_HCI shall be on!\n"

#endif

#include <pci_ops.h>
#include <pci_osintf.h>
#include <pci_hal.h>

#if defined (PLATFORM_LINUX) && defined (PLATFORM_WINDOWS)

#error "Shall be Linux or Windows, but not both!\n"

#endif

#ifdef CONFIG_RTL8192C
#define DRV_NAME "rtl8192ce"
#elif defined CONFIG_RTL8192D
#define DRV_NAME "rtl8192de"
#endif

#ifdef CONFIG_80211N_HT
extern int ht_enable;
extern int cbw40_enable;
extern int ampdu_enable;//for enable tx_ampdu
#endif

extern char* initmac;

extern u32 start_drv_threads(_adapter *padapter);
extern void stop_drv_threads (_adapter *padapter);
extern u8 init_drv_sw(_adapter *padapter);
extern u8 free_drv_sw(_adapter *padapter);
extern struct net_device *init_netdev(void);
extern void cancel_all_timer(_adapter *padapter);
#ifdef CONFIG_IPS
extern int  ips_netdrv_open(_adapter *padapter);
extern void ips_dev_unload(_adapter *padapter);
#endif
#ifdef CONFIG_PM
extern int pm_netdev_open(struct net_device *pnetdev);
static int rtw_suspend(struct pci_dev *pdev, pm_message_t state);
static int rtw_resume(struct pci_dev *pdev);
#endif
extern u8 reset_drv_sw(_adapter *padapter);
static void rtw_dev_unload(_adapter *padapter);

static int rtw_drv_init(struct pci_dev *pdev, const struct pci_device_id *pdid);
static void rtw_dev_remove(struct pci_dev *pdev);

static struct specific_device_id specific_device_id_tbl[] = {
	{.idVendor=0x0b05, .idProduct=0x1791, .flags=SPEC_DEV_ID_DISABLE_HT},
	{.idVendor=0x13D3, .idProduct=0x3311, .flags=SPEC_DEV_ID_DISABLE_HT},
	{}
};

struct pci_device_id rtw_pci_id_tbl[] = {
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0x8191)},
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0x8178)},
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0x8177)},
	{PCI_DEVICE(PCI_VENDER_ID_REALTEK, 0x8176)},
	{},
};

typedef struct _driver_priv{

	struct pci_driver rtw_pci_drv;
	int drv_registered;

}drv_priv, *pdrv_priv;


static drv_priv drvpriv = {
	.rtw_pci_drv.name = (char*)"rtw_pci_drv",
	.rtw_pci_drv.probe = rtw_drv_init,
	.rtw_pci_drv.remove = rtw_dev_remove,
	.rtw_pci_drv.id_table = rtw_pci_id_tbl,
#ifdef CONFIG_PM	
//	.rtw_pci_drv.suspend = rtw_suspend,
//	.rtw_pci_drv.resume = rtw_resume,
#else	
	.rtw_pci_drv.suspend = NULL,
	.rtw_pci_drv.resume = NULL,
#endif
};


MODULE_DEVICE_TABLE(pci, rtw_pci_id_tbl);


static u16 pcibridge_vendors[PCI_BRIDGE_VENDOR_MAX] = {
	INTEL_VENDOR_ID,
	ATI_VENDOR_ID,
	AMD_VENDOR_ID,
	SIS_VENDOR_ID
};

static u8 rtw_pci_platform_switch_device_pci_aspm(_adapter *padapter, u8 value)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	u8	bresult = _FALSE;

	value |= 0x40;

	pci_write_config_byte(pdvobjpriv->ppcidev, 0x80, value);

	return bresult;
}

// 
// When we set 0x01 to enable clk request. Set 0x0 to disable clk req.  
// 
static u8 rtw_pci_switch_clk_req(_adapter *padapter, u8 value)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	u8 buffer, bresult = _FALSE;

	buffer = value;

	pci_write_config_byte(pdvobjpriv->ppcidev, 0x81, value);
	bresult = _TRUE;

	return bresult;
}

#if 0
//Description: 
//Disable RTL8192SE ASPM & Disable Pci Bridge ASPM
void rtw_pci_disable_aspm(_adapter *padapter)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;
	u32	pcicfg_addrport = 0;
	u8	num4bytes;
	u8	linkctrl_reg;
	u16	pcibridge_linkctrlreg, aspmlevel = 0;

	// When there exists anyone's busnum, devnum, and funcnum that are set to 0xff,
	// we do not execute any action and return. 
	// if it is not intel bus then don't enable ASPM. 
	if ((pdvobjpriv->ndis_adapter.busnumber == 0xff
		&& pdvobjpriv->ndis_adapter.devnumber == 0xff
		&& pdvobjpriv->ndis_adapter.funcnumber == 0xff)
		|| (pdvobjpriv->ndis_adapter.pcibridge_busnum == 0xff
		&& pdvobjpriv->ndis_adapter.pcibridge_devnum == 0xff
		&& pdvobjpriv->ndis_adapter.pcibridge_funcnum == 0xff))
	{
		DBG_8192C("PlatformEnableASPM(): Fail to enable ASPM. Cannot find the Bus of PCI(Bridge).\n");
		return;
	}

	if (pdvobjpriv->ndis_adapter.pcibridge_vendor == PCI_BRIDGE_VENDOR_UNKNOWN) {
		DBG_8192C("%s(): Disable ASPM. Recognize the Bus of PCI(Bridge) as UNKNOWN.\n", __func__);
	}

	if (pwrpriv->reg_rfps_level & RT_RF_OFF_LEVL_CLK_REQ) {
		RT_CLEAR_PS_LEVEL(pwrpriv, RT_RF_OFF_LEVL_CLK_REQ);
		rtw_pci_switch_clk_req(padapter, 0x0);
	}

	{
		// Suggested by SD1 for promising device will in L0 state after an I/O.
		u8 tmp_u1b;

		pci_read_config_byte(pdvobjpriv->ppcidev, 0x80, &tmp_u1b);
	}

	// Retrieve original configuration settings.
	linkctrl_reg = pdvobjpriv->ndis_adapter.linkctrl_reg;
	pcibridge_linkctrlreg = pdvobjpriv->ndis_adapter.pcibridge_linkctrlreg;

	// Set corresponding value.
	aspmlevel |= BIT(0) | BIT(1);
	linkctrl_reg &= ~aspmlevel;
	pcibridge_linkctrlreg &= ~(BIT(0) | BIT(1));

	rtw_pci_platform_switch_device_pci_aspm(padapter, linkctrl_reg);
	udelay_os(50);

	//When there exists anyone's busnum, devnum, and funcnum that are set to 0xff,
	// we do not execute any action and return.
	if ((pdvobjpriv->ndis_adapter.busnumber == 0xff &&
		pdvobjpriv->ndis_adapter.devnumber == 0xff &&
		pdvobjpriv->ndis_adapter.funcnumber == 0xff) ||
		(pdvobjpriv->ndis_adapter.pcibridge_busnum == 0xff &&
		pdvobjpriv->ndis_adapter.pcibridge_devnum == 0xff
		&& pdvobjpriv->ndis_adapter.pcibridge_funcnum == 0xff))
	{
		//Do Nothing!!
	}
	else
	{
		//4 //Disable Pci Bridge ASPM 
		pcicfg_addrport = (pdvobjpriv->ndis_adapter.pcibridge_busnum << 16) |
						(pdvobjpriv->ndis_adapter.pcibridge_devnum << 11) |
						(pdvobjpriv->ndis_adapter.pcibridge_funcnum << 8) | (1 << 31);
		num4bytes = (pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset + 0x10) / 4;

		// set up address port at 0xCF8 offset field= 0 (dev|vend)
		NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (num4bytes << 2));

		// now grab data port with device|vendor 4 byte dword
		NdisRawWritePortUchar(PCI_CONF_DATA, pcibridge_linkctrlreg);

		DBG_8192C("rtw_pci_disable_aspm():PciBridge busnumber[%x], DevNumbe[%x], funcnumber[%x], Write reg[%x] = %x\n",
			pdvobjpriv->ndis_adapter.pcibridge_busnum, pdvobjpriv->ndis_adapter.pcibridge_devnum, 
			pdvobjpriv->ndis_adapter.pcibridge_funcnum, 
			(pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset+0x10), pcibridge_linkctrlreg);

		udelay_os(50);
	}
}

//[ASPM]
//Description: 
//              Enable RTL8192SE ASPM & Enable Pci Bridge ASPM for power saving
//              We should follow the sequence to enable RTL8192SE first then enable Pci Bridge ASPM
//              or the system will show bluescreen.
void rtw_pci_enable_aspm(_adapter *padapter)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;
	u16	aspmlevel = 0;
	u32	pcicfg_addrport = 0;
	u8	num4bytes;
	u8	u_pcibridge_aspmsetting = 0;
	u8	u_device_aspmsetting = 0;

	// When there exists anyone's busnum, devnum, and funcnum that are set to 0xff,
	// we do not execute any action and return. 
	// if it is not intel bus then don't enable ASPM. 

	if ((pdvobjpriv->ndis_adapter.busnumber == 0xff &&
		pdvobjpriv->ndis_adapter.devnumber == 0xff &&
		pdvobjpriv->ndis_adapter.funcnumber == 0xff) ||
		(pdvobjpriv->ndis_adapter.pcibridge_busnum == 0xff &&
		pdvobjpriv->ndis_adapter.pcibridge_devnum == 0xff
		&& pdvobjpriv->ndis_adapter.pcibridge_funcnum == 0xff)) 
	{
		DBG_8192C("PlatformEnableASPM(): Fail to enable ASPM. Cannot find the Bus of PCI(Bridge).\n");
		return;
	}

	//4 Enable Pci Bridge ASPM 
	pcicfg_addrport = (pdvobjpriv->ndis_adapter.pcibridge_busnum << 16) 
					| (pdvobjpriv->ndis_adapter.pcibridge_devnum << 11)
					| (pdvobjpriv->ndis_adapter.pcibridge_funcnum << 8) | (1 << 31);
	num4bytes = (pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset + 0x10) / 4;
	// set up address port at 0xCF8 offset field= 0 (dev|vend)
	NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (num4bytes << 2));
	// now grab data port with device|vendor 4 byte dword

	u_pcibridge_aspmsetting = pdvobjpriv->ndis_adapter.pcibridge_linkctrlreg | pdvobjpriv->const_hostpci_aspm_setting;

	if (pdvobjpriv->ndis_adapter.pcibridge_vendor == PCI_BRIDGE_VENDOR_INTEL)
		u_pcibridge_aspmsetting &= ~BIT(0);

	NdisRawWritePortUchar(PCI_CONF_DATA, u_pcibridge_aspmsetting);

	DBG_8192C("PlatformEnableASPM():PciBridge busnumber[%x], DevNumbe[%x], funcnumber[%x], Write reg[%x] = %x\n",
		pdvobjpriv->ndis_adapter.pcibridge_busnum, 
		pdvobjpriv->ndis_adapter.pcibridge_devnum, 
		pdvobjpriv->ndis_adapter.pcibridge_funcnum, 
		(pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset+0x10), 
		u_pcibridge_aspmsetting);

	udelay_os(50);

	// Get ASPM level (with/without Clock Req)
	aspmlevel |= pdvobjpriv->const_devicepci_aspm_setting;
	u_device_aspmsetting = pdvobjpriv->ndis_adapter.linkctrl_reg;

	//rtw_pci_platform_switch_device_pci_aspm(dev, (priv->ndis_adapter.linkctrl_reg | ASPMLevel));

	u_device_aspmsetting |= aspmlevel;

	rtw_pci_platform_switch_device_pci_aspm(padapter, u_device_aspmsetting);	//(priv->linkctrl_reg | ASPMLevel));

	if (pwrpriv->reg_rfps_level & RT_RF_OFF_LEVL_CLK_REQ) {
		rtw_pci_switch_clk_req(padapter, (pwrpriv->reg_rfps_level & RT_RF_OFF_LEVL_CLK_REQ) ? 1 : 0);
		RT_SET_PS_LEVEL(pwrpriv, RT_RF_OFF_LEVL_CLK_REQ);
	}
	udelay_os(100);

	udelay_os(100);
}

//
//Description: 
//To get link control field by searching from PCIe capability lists.
//
static u8
rtw_get_link_control_field(_adapter *padapter, u8 busnum, u8 devnum,
				u8 funcnum)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct rt_pci_capabilities_header capability_hdr;
	u8	capability_offset, num4bytes;
	u32	pcicfg_addrport = 0;
	u8	linkctrl_reg;
	u8	status = _FALSE;

	//If busnum, devnum, funcnum are set to 0xff.
	if (busnum == 0xff && devnum == 0xff && funcnum == 0xff) {
		printk("GetLinkControlField(): Fail to find PCIe Capability\n");
		return _FALSE;
	}

	pcicfg_addrport = (busnum << 16) | (devnum << 11) | (funcnum << 8) | (1 << 31);

	//2PCIeCap

	// The device supports capability lists. Find the capabilities.
	num4bytes = 0x34 / 4;
	//get capability_offset
	// set up address port at 0xCF8 offset field= 0 (dev|vend)
	NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (num4bytes << 2));
	// now grab data port with device|vendor 4 byte dword
	NdisRawReadPortUchar(PCI_CONF_DATA, &capability_offset);

	// Loop through the capabilities in search of the power management capability. 
	// The list is NULL-terminated, so the last offset will always be zero.

	while (capability_offset != 0) {
		// First find the number of 4 Byte. 
		num4bytes = capability_offset / 4;

		// Read the header of the capability at  this offset. If the retrieved capability is not
		// the power management capability that we are looking for, follow the link to the 
		// next capability and continue looping.

		//4 get capability_hdr
		// set up address port at 0xCF8 offset field= 0 (dev|vend)
		NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (num4bytes << 2));
		// now grab data port with device|vendor 4 byte dword
		NdisRawReadPortUshort(PCI_CONF_DATA, (u16 *) & capability_hdr);

		// Found the PCI express capability
		if (capability_hdr.capability_id == PCI_CAPABILITY_ID_PCI_EXPRESS)
		{
			break;
		}
		else
		{
			// This is some other capability. Keep looking for the PCI express capability.
			capability_offset = capability_hdr.next;
		}
	}

	if (capability_hdr.capability_id == PCI_CAPABILITY_ID_PCI_EXPRESS)	//
	{
		num4bytes = (capability_offset + 0x10) / 4;

		//4 Read  Link Control Register
		// set up address port at 0xCF8 offset field= 0 (dev|vend)
		NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (num4bytes << 2));
		// now grab data port with device|vendor 4 byte dword
		NdisRawReadPortUchar(PCI_CONF_DATA, &linkctrl_reg);

		pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset = capability_offset;
		pdvobjpriv->ndis_adapter.pcibridge_linkctrlreg = linkctrl_reg;

		status = _TRUE;
	}
	else
	{
		// We didn't find a PCIe capability. 
		printk("GetLinkControlField(): Cannot Find PCIe Capability\n");
	}

	return status;
}

//
//Description: 
//To get PCI bus infomation and return busnum, devnum, and funcnum about 
//the bus(bridge) which the device binds.
//
static u8
rtw_get_pci_bus_info(_adapter *padapter,
			  u16 vendorid,
			  u16 deviceid,
			  u8 irql, u8 basecode, u8 subclass, u8 filed19val,
			  u8 * busnum, u8 * devnum, u8 * funcnum)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct pci_dev	*pdev = pdvobjpriv->ppcidev;
	u8	busnum_idx, devicenum_idx, functionnum_idx;
	u32	pcicfg_addrport = 0;
	u32	dev_venid = 0, classcode, field19, headertype;
	u16	venId, devId;
	u8	basec, subc, irqline;
	u16	regoffset;
	u8	b_singlefunc = _FALSE;
	u8	b_bridgechk = _FALSE;

	*busnum = 0xFF;
	*devnum = 0xFF;
	*funcnum = 0xFF;

	//printk("==============>vendorid:%x,deviceid:%x,irql:%x\n", vendorid,deviceid,irql);
	if ((basecode == PCI_CLASS_BRIDGE_DEV) &&
		(subclass == PCI_SUBCLASS_BR_PCI_TO_PCI)
		&& (filed19val == U1DONTCARE))
		b_bridgechk = _TRUE;

	// perform a complete pci bus scan operation
	for (busnum_idx = 0; busnum_idx < PCI_MAX_BRIDGE_NUMBER; busnum_idx++)	//255
	{
		for (devicenum_idx = 0; devicenum_idx < PCI_MAX_DEVICES; devicenum_idx++)	//32
		{
			b_singlefunc = _FALSE;
			for (functionnum_idx = 0; functionnum_idx < PCI_MAX_FUNCTION; functionnum_idx++)	//8
			{
				//
				// <Roger_Notes> We have to skip redundant Bus scan to prevent unexpected system hang
				// if single function is present in this device.
				// 2009.02.26.
				//                              
				if (functionnum_idx == 0) {
					//4 get header type (DWORD #3)
					pcicfg_addrport = (busnum_idx << 16) | (devicenum_idx << 11) | (functionnum_idx << 8) | (1 << 31);
					NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (3 << 2));
					NdisRawReadPortUlong(PCI_CONF_DATA, &headertype);
					headertype = ((headertype >> 16) & 0x0080) >> 7;	// address 0x0e[7].
					if (headertype == 0)	//Single function                                                                                  
						b_singlefunc = _TRUE;
				}
				else
				{//By pass the following scan process.
					if (b_singlefunc == _TRUE)
						break;
				}

				// Set access enable control.
				pcicfg_addrport = (busnum_idx << 16) | (devicenum_idx << 11) | (functionnum_idx << 8) | (1 << 31);

				//4 // Get vendorid/ deviceid
				// set up address port at 0xCF8 offset field= 0 (dev|vend)
				NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport);
				// now grab data port with device|vendor 4 byte dword
				NdisRawReadPortUlong(PCI_CONF_DATA, &dev_venid);

				// if data port is full of 1s, no device is present
				// some broken boards return 0 if a slot is empty:
				if (dev_venid == 0xFFFFFFFF || dev_venid == 0)
					continue;	//PCI_INVALID_VENDORID

				// 4 // Get irql
				regoffset = 0x3C;
				pcicfg_addrport = (busnum_idx << 16) | (devicenum_idx << 11) | (functionnum_idx << 8) | (1 << 31) | (regoffset & 0xFFFFFFFC);
				NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport);
				NdisRawReadPortUchar((PCI_CONF_DATA +(regoffset & 0x3)), &irqline);

				venId = (u16) (dev_venid >> 0) & 0xFFFF;
				devId = (u16) (dev_venid >> 16) & 0xFFFF;

				// Check Vendor ID
				if (!b_bridgechk && (venId != vendorid) && (vendorid != U2DONTCARE))
					continue;

				// Check Device ID
				if (!b_bridgechk && (devId != deviceid) && (deviceid != U2DONTCARE))
					continue;

				// Check irql
				if (!b_bridgechk && (irqline != irql) && (irql != U1DONTCARE))
					continue;

				//4 get Class Code
				pcicfg_addrport = (busnum_idx << 16) | (devicenum_idx << 11) | (functionnum_idx << 8) | (1 << 31);
				NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (2 << 2));
				NdisRawReadPortUlong(PCI_CONF_DATA, &classcode);
				classcode = classcode >> 8;

				basec = (u8) (classcode >> 16) & 0xFF;
				subc = (u8) (classcode >> 8) & 0xFF;
				if (b_bridgechk && (venId != vendorid) && (basec == basecode) && (subc == subclass))
					return _TRUE;

				// Check Vendor ID
				if (b_bridgechk && (venId != vendorid) && (vendorid != U2DONTCARE))
					continue;

				// Check Device ID
				if (b_bridgechk && (devId != deviceid) && (deviceid != U2DONTCARE))
					continue;

				// Check irql
				if (b_bridgechk && (irqline != irql) && (irql != U1DONTCARE))
					continue;

				//4 get field 0x19 value  (DWORD #6)
				NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (6 << 2));
				NdisRawReadPortUlong(PCI_CONF_DATA, &field19);
				field19 = (field19 >> 8) & 0xFF;

				//4 Matching Class Code and filed19.
				if ((basec == basecode) && (subc == subclass) && ((field19 == filed19val) || (filed19val == U1DONTCARE))) {
					*busnum = busnum_idx;
					*devnum = devicenum_idx;
					*funcnum = functionnum_idx;

					printk("GetPciBusInfo(): Find Device(%X:%X)  bus=%d dev=%d, func=%d\n",
						vendorid, deviceid, busnum_idx, devicenum_idx, functionnum_idx);
					return _TRUE;
				}
			}
		}
	}

	printk("GetPciBusInfo(): Cannot Find Device(%X:%X:%X)\n", vendorid, deviceid, dev_venid);

	return _FALSE;
}

static u8
rtw_get_pci_brideg_info(_adapter *padapter,
			     u8 basecode,
			     u8 subclass,
			     u8 filed19val, u8 * busnum, u8 * devnum,
			     u8 * funcnum, u16 * vendorid, u16 * deviceid)
{
	u8	busnum_idx, devicenum_idx, functionnum_idx;
	u32	pcicfg_addrport = 0;
	u32	dev_venid, classcode, field19, headertype;
	u16	venId, devId;
	u8	basec, subc, irqline;
	u16	regoffset;
	u8	b_singlefunc = _FALSE;

	*busnum = 0xFF;
	*devnum = 0xFF;
	*funcnum = 0xFF;

	// perform a complete pci bus scan operation
	for (busnum_idx = 0; busnum_idx < PCI_MAX_BRIDGE_NUMBER; busnum_idx++)	//255
	{
		for (devicenum_idx = 0; devicenum_idx < PCI_MAX_DEVICES; devicenum_idx++)	//32
		{
			b_singlefunc = _FALSE;
			for (functionnum_idx = 0; functionnum_idx < PCI_MAX_FUNCTION; functionnum_idx++)	//8
			{
				//
				// <Roger_Notes> We have to skip redundant Bus scan to prevent unexpected system hang
				// if single function is present in this device.
				// 2009.02.26.
				//
				if (functionnum_idx == 0)
				{
					//4 get header type (DWORD #3)
					pcicfg_addrport = (busnum_idx << 16) | (devicenum_idx << 11) | (functionnum_idx << 8) | (1 << 31);
					//NdisRawWritePortUlong((ULONG_PTR)PCI_CONF_ADDRESS ,  pcicfg_addrport + (3 << 2));
					//NdisRawReadPortUlong((ULONG_PTR)PCI_CONF_DATA, &headertype);
					NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (3 << 2));
					NdisRawReadPortUlong(PCI_CONF_DATA, &headertype);
					headertype = ((headertype >> 16) & 0x0080) >> 7;	// address 0x0e[7].
					if (headertype == 0)	//Single function                                                                                  
						b_singlefunc = _TRUE;
				}
				else
				{//By pass the following scan process.
					if (b_singlefunc == _TRUE)
						break;
				}

				pcicfg_addrport = (busnum_idx << 16) | (devicenum_idx << 11) | (functionnum_idx << 8) | (1 << 31);

				//4 // Get vendorid/ deviceid
				// set up address port at 0xCF8 offset field= 0 (dev|vend)
				NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport);
				// now grab data port with device|vendor 4 byte dword
				NdisRawReadPortUlong(PCI_CONF_DATA, &dev_venid);

				//4 Get irql
				regoffset = 0x3C;
				pcicfg_addrport = (busnum_idx << 16) | (devicenum_idx << 11) | (functionnum_idx << 8) | (1 << 31) | (regoffset & 0xFFFFFFFC);
				NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport);
				NdisRawReadPortUchar((PCI_CONF_DATA + (regoffset & 0x3)), &irqline);

				venId = (u16) (dev_venid >> 0) & 0xFFFF;
				devId = (u16) (dev_venid >> 16) & 0xFFFF;

				//4 get Class Code
				pcicfg_addrport = (busnum_idx << 16) | (devicenum_idx << 11) | (functionnum_idx << 8) | (1 << 31);
				NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (2 << 2));
				NdisRawReadPortUlong(PCI_CONF_DATA, &classcode);
				classcode = classcode >> 8;

				basec = (u8) (classcode >> 16) & 0xFF;
				subc = (u8) (classcode >> 8) & 0xFF;

				//4 get field 0x19 value  (DWORD #6)
				NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (6 << 2));
				NdisRawReadPortUlong(PCI_CONF_DATA, &field19);
				field19 = (field19 >> 8) & 0xFF;

				//4 Matching Class Code and filed19.
				if ((basec == basecode) && (subc == subclass) && ((field19 == filed19val) || (filed19val == U1DONTCARE))) {
					*busnum = busnum_idx;
					*devnum = devicenum_idx;
					*funcnum = functionnum_idx;
					*vendorid = venId;
					*deviceid = devId;

					printk("GetPciBridegInfo : Find Device(%X:%X)  bus=%d dev=%d, func=%d\n",
						venId, devId, busnum_idx, devicenum_idx, functionnum_idx);

					return _TRUE;
				}
			}
		}
	}

	printk("GetPciBridegInfo(): Cannot Find PciBridge for Device\n");

	return _FALSE;
}				// end of GetPciBridegInfo

//
//Description: 
//To find specific bridge information.
//
static void rtw_find_bridge_info(_adapter *padapter)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	u8	pcibridge_busnum = 0xff;
	u8	pcibridge_devnum = 0xff;
	u8	pcibridge_funcnum = 0xff;
	u16	pcibridge_vendorid = 0xff;
	u16	pcibridge_deviceid = 0xff;
	u8	tmp = 0;

	rtw_get_pci_brideg_info(padapter,
				     PCI_CLASS_BRIDGE_DEV,
				     PCI_SUBCLASS_BR_PCI_TO_PCI,
				     pdvobjpriv->ndis_adapter.busnumber,
				     &pcibridge_busnum,
				     &pcibridge_devnum, &pcibridge_funcnum,
				     &pcibridge_vendorid, &pcibridge_deviceid);

	// match the array of vendor id and regonize which chipset is used.
	pdvobjpriv->ndis_adapter.pcibridge_vendor = PCI_BRIDGE_VENDOR_UNKNOWN;

	for (tmp = 0; tmp < PCI_BRIDGE_VENDOR_MAX; tmp++) {
		if (pcibridge_vendorid == pcibridge_vendors[tmp]) {
			pdvobjpriv->ndis_adapter.pcibridge_vendor = tmp;
			printk("Pci Bridge Vendor is found index: %d\n", tmp);
			break;
		}
	}
	printk("Pci Bridge Vendor is %x\n", pcibridge_vendors[tmp]);

	// Update corresponding PCI bus info.
	pdvobjpriv->ndis_adapter.pcibridge_busnum = pcibridge_busnum;
	pdvobjpriv->ndis_adapter.pcibridge_devnum = pcibridge_devnum;
	pdvobjpriv->ndis_adapter.pcibridge_funcnum = pcibridge_funcnum;
	pdvobjpriv->ndis_adapter.pcibridge_vendorid = pcibridge_vendorid;
	pdvobjpriv->ndis_adapter.pcibridge_deviceid = pcibridge_deviceid;

}

static u8
rtw_get_amd_l1_patch(_adapter *padapter, u8 busnum, u8 devnum,
			  u8 funcnum)
{
	u8	status = _FALSE;
	u8	offset_e0;
	unsigned	offset_e4;
	u32	pcicfg_addrport = 0;

	pcicfg_addrport = (busnum << 16) | (devnum << 11) | (funcnum << 8) | (1 << 31);

	NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + 0xE0);
	NdisRawWritePortUchar(PCI_CONF_DATA, 0xA0);

	NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + 0xE0);
	NdisRawReadPortUchar(PCI_CONF_DATA, &offset_e0);

	if (offset_e0 == 0xA0)
	{
		NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + 0xE4);
		NdisRawReadPortUlong(PCI_CONF_DATA, &offset_e4);
		//DbgPrint("Offset E4 %x\n", offset_e4);
		if (offset_e4 & BIT(23))
			status = _TRUE;
	}

	return status;
}
#else
/*Disable RTL8192SE ASPM & Disable Pci Bridge ASPM*/
void rtw_pci_disable_aspm(_adapter *padapter)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;
	struct pci_dev	*pdev = pdvobjpriv->ppcidev;
	struct pci_dev	*bridge_pdev = pdev->bus->self;
	u8	pcibridge_vendor = pdvobjpriv->ndis_adapter.pcibridge_vendor;
	//u32	pcicfg_addrport = pdvobjpriv->ndis_adapter.pcicfg_addrport;
	//u8	num4bytes = pdvobjpriv->ndis_adapter.num4bytes;
	/*Retrieve original configuration settings.*/ 
	u8	linkctrl_reg = pdvobjpriv->ndis_adapter.linkctrl_reg;
	u16	pcibridge_linkctrlreg = pdvobjpriv->ndis_adapter.pcibridge_linkctrlreg;
	u16	aspmlevel = 0;

	if (pcibridge_vendor == PCI_BRIDGE_VENDOR_UNKNOWN) {
		RT_TRACE(_module_hci_intfs_c_, _drv_err_, ("%s(): PCI(Bridge) UNKNOWN.\n", __FUNCTION__));
		return;
	}

	if (pwrpriv->reg_rfps_level & RT_RF_OFF_LEVL_CLK_REQ) {
		RT_CLEAR_PS_LEVEL(pwrpriv, RT_RF_OFF_LEVL_CLK_REQ);
		rtw_pci_switch_clk_req(padapter, 0x0);
	}

	if(1) {
		/*for promising device will in L0 state after an I/O.*/ 
		u8 tmp_u1b;
		pci_read_config_byte(pdev, 0x80, &tmp_u1b);
	}

	/*Set corresponding value.*/ 
	aspmlevel |= BIT(0) | BIT(1);
	linkctrl_reg &= ~aspmlevel;
	pcibridge_linkctrlreg &= ~(BIT(0) | BIT(1));

	rtw_pci_platform_switch_device_pci_aspm(padapter, linkctrl_reg);
	udelay_os(50);

	/*Disable Pci Bridge ASPM*/ 
	//NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (num4bytes << 2));
	//NdisRawWritePortUchar(PCI_CONF_DATA, pcibridge_linkctrlreg);
	pci_write_config_byte(bridge_pdev, pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset + PCI_EXP_LNKCTL, pcibridge_linkctrlreg);

	DBG_8192C("rtw_pci_disable_aspm():PciBridge busnumber[%x], DevNumbe[%x], funcnumber[%x], Write reg[%x] = %x\n",
		pdvobjpriv->ndis_adapter.pcibridge_busnum, pdvobjpriv->ndis_adapter.pcibridge_devnum, 
		pdvobjpriv->ndis_adapter.pcibridge_funcnum, 
		(pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset+0x10), pcibridge_linkctrlreg);

	udelay_os(50);

}

/*Enable RTL8192SE ASPM & Enable Pci Bridge ASPM for 
power saving We should follow the sequence to enable 
RTL8192SE first then enable Pci Bridge ASPM
or the system will show bluescreen.*/
void rtw_pci_enable_aspm(_adapter *padapter)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;
	struct pci_dev	*pdev = pdvobjpriv->ppcidev;
	struct pci_dev	*bridge_pdev = pdev->bus->self;
	u8	pcibridge_busnum = pdvobjpriv->ndis_adapter.pcibridge_busnum;
	u8	pcibridge_devnum = pdvobjpriv->ndis_adapter.pcibridge_devnum;
	u8	pcibridge_funcnum = pdvobjpriv->ndis_adapter.pcibridge_funcnum;
	u8	pcibridge_vendor = pdvobjpriv->ndis_adapter.pcibridge_vendor;
	//u32	pcicfg_addrport = pdvobjpriv->ndis_adapter.pcicfg_addrport;
	//u8	num4bytes = pdvobjpriv->ndis_adapter.num4bytes;
	u16	aspmlevel = 0;		
	u8	u_pcibridge_aspmsetting = 0;
	u8	u_device_aspmsetting = 0;


	if (pcibridge_vendor == PCI_BRIDGE_VENDOR_UNKNOWN) {
		RT_TRACE(_module_hci_intfs_c_, _drv_err_, ("%s(): PCI(Bridge) UNKNOWN.\n", __FUNCTION__));
		return;
	}

	/*Enable Pci Bridge ASPM*/  
	u_pcibridge_aspmsetting = pdvobjpriv->ndis_adapter.pcibridge_linkctrlreg | pdvobjpriv->const_hostpci_aspm_setting;

	if (pcibridge_vendor == PCI_BRIDGE_VENDOR_INTEL)
		u_pcibridge_aspmsetting &= ~BIT(0);

	//NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + (num4bytes << 2));
	//NdisRawWritePortUchar(PCI_CONF_DATA, u_pcibridge_aspmsetting);

	pci_write_config_byte(bridge_pdev, pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset + PCI_EXP_LNKCTL, u_pcibridge_aspmsetting);

	DBG_8192C("PlatformEnableASPM():PciBridge busnumber[%x], DevNumbe[%x], funcnumber[%x], Write reg[%x] = %x\n",
		pcibridge_busnum, pcibridge_devnum, pcibridge_funcnum, 
		(pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset+0x10), 
		u_pcibridge_aspmsetting);

	udelay_os(50);

	/*Get ASPM level (with/without Clock Req)*/ 
	aspmlevel |= pdvobjpriv->const_devicepci_aspm_setting;
	u_device_aspmsetting = pdvobjpriv->ndis_adapter.linkctrl_reg;

	//rtw_pci_platform_switch_device_pci_aspm(padapter, (pdvobjpriv->ndis_adapter.linkctrl_reg | aspmlevel));

	u_device_aspmsetting |= aspmlevel;

	rtw_pci_platform_switch_device_pci_aspm(padapter, u_device_aspmsetting);

	if (pwrpriv->reg_rfps_level & RT_RF_OFF_LEVL_CLK_REQ) {
		rtw_pci_switch_clk_req(padapter, (pwrpriv->reg_rfps_level & RT_RF_OFF_LEVL_CLK_REQ) ? 1 : 0);
		RT_SET_PS_LEVEL(pwrpriv, RT_RF_OFF_LEVL_CLK_REQ);
	}
	udelay_os(100);

	udelay_os(100);
}

static u8 rtw_pci_get_amd_l1_patch(_adapter *padapter)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct pci_dev	*pdev = pdvobjpriv->ppcidev;
	struct pci_dev	*bridge_pdev = pdev->bus->self;
	//u32	pcicfg_addrport = pdvobjpriv->ndis_adapter.pcicfg_addrport;
	u8	status = _FALSE;
	u8	offset_e0;
	u32	offset_e4;

	//NdisRawWritePortUlong(PCI_CONF_ADDRESS,pcicfg_addrport + 0xE0);
	//NdisRawWritePortUchar(PCI_CONF_DATA, 0xA0);
	pci_write_config_byte(bridge_pdev, 0xE0, 0xA0);

	//NdisRawWritePortUlong(PCI_CONF_ADDRESS,pcicfg_addrport + 0xE0);
	//NdisRawReadPortUchar(PCI_CONF_DATA, &offset_e0);
	pci_read_config_byte(bridge_pdev, 0xE0, &offset_e0);

	if (offset_e0 == 0xA0) {
		//NdisRawWritePortUlong(PCI_CONF_ADDRESS, pcicfg_addrport + 0xE4);
		//NdisRawReadPortUlong(PCI_CONF_DATA, &offset_e4);
		pci_read_config_dword(bridge_pdev, 0xE4, &offset_e4);
		if (offset_e4 & BIT(23))
			status = _TRUE;
	}

	return status;
}

static void rtw_pci_get_linkcontrol_field(_adapter *padapter)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct pci_dev	*pdev = pdvobjpriv->ppcidev;
	struct pci_dev	*bridge_pdev = pdev->bus->self;
	u8	capabilityoffset = pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset;
	//u32	pcicfg_addrport = pdvobjpriv->ndis_adapter.pcicfg_addrport;
	u8	linkctrl_reg;	
	//u8	num4bBytes;

	//num4bBytes = (capabilityoffset+0x10)/4;
			
	/*Read  Link Control Register*/
	//NdisRawWritePortUlong(PCI_CONF_ADDRESS , pcicfg_addrport+(num4bBytes << 2));
	//NdisRawReadPortUchar(PCI_CONF_DATA, &linkctrl_reg);

	pci_read_config_byte(bridge_pdev, capabilityoffset + PCI_EXP_LNKCTL, &linkctrl_reg);

	pdvobjpriv->ndis_adapter.pcibridge_linkctrlreg = linkctrl_reg;
}
#endif

static void rtw_pci_parse_configuration(struct pci_dev *pdev, _adapter *padapter)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;

	u8 tmp;
	int pos;
	u8 linkctrl_reg;

	//Link Control Register
	pos = pci_find_capability(pdev, PCI_CAP_ID_EXP);
	pci_read_config_byte(pdev, pos + PCI_EXP_LNKCTL, &linkctrl_reg);
	pdvobjpriv->ndis_adapter.linkctrl_reg = linkctrl_reg;

	//DBG_8192C("Link Control Register = %x\n", pdvobjpriv->ndis_adapter.linkctrl_reg);

	pci_read_config_byte(pdev, 0x98, &tmp);
	tmp |= BIT(4);
	pci_write_config_byte(pdev, 0x98, tmp);

	tmp = 0x17;
	pci_write_config_byte(pdev, 0x70f, tmp);
}

//
// Update PCI dependent default settings.
//
static void rtw_pci_update_default_setting(_adapter *padapter)
{
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;

	//reset pPSC->reg_rfps_level & priv->b_support_aspm
	pwrpriv->reg_rfps_level = 0;
	pwrpriv->b_support_aspm = 0;

	// Dynamic Mechanism, 
	//pAdapter->HalFunc.SetHalDefVarHandler(pAdapter, HAL_DEF_INIT_GAIN, &(pDevice->InitGainState));

	// Update PCI ASPM setting
	pwrpriv->const_amdpci_aspm = pdvobjpriv->const_amdpci_aspm;
	switch (pdvobjpriv->const_pci_aspm) {
		case 0:		// No ASPM
			break;

		case 1:		// ASPM dynamically enabled/disable.
			pwrpriv->reg_rfps_level |= RT_RF_LPS_LEVEL_ASPM;
			break;

		case 2:		// ASPM with Clock Req dynamically enabled/disable.
			pwrpriv->reg_rfps_level |= (RT_RF_LPS_LEVEL_ASPM | RT_RF_OFF_LEVL_CLK_REQ);
			break;

		case 3:		// Always enable ASPM and Clock Req from initialization to halt.
			pwrpriv->reg_rfps_level &= ~(RT_RF_LPS_LEVEL_ASPM);
			pwrpriv->reg_rfps_level |= (RT_RF_PS_LEVEL_ALWAYS_ASPM | RT_RF_OFF_LEVL_CLK_REQ);
			break;

		case 4:		// Always enable ASPM without Clock Req from initialization to halt.
			pwrpriv->reg_rfps_level &= ~(RT_RF_LPS_LEVEL_ASPM | RT_RF_OFF_LEVL_CLK_REQ);
			pwrpriv->reg_rfps_level |= RT_RF_PS_LEVEL_ALWAYS_ASPM;
			break;
	}

	pwrpriv->reg_rfps_level |= RT_RF_OFF_LEVL_HALT_NIC;

	// Update Radio OFF setting
	switch (pdvobjpriv->const_hwsw_rfoff_d3) {
		case 1:
			if (pwrpriv->reg_rfps_level & RT_RF_LPS_LEVEL_ASPM)
				pwrpriv->reg_rfps_level |= RT_RF_OFF_LEVL_ASPM;
			break;

		case 2:
			if (pwrpriv->reg_rfps_level & RT_RF_LPS_LEVEL_ASPM)
				pwrpriv->reg_rfps_level |= RT_RF_OFF_LEVL_ASPM;
			pwrpriv->reg_rfps_level |= RT_RF_OFF_LEVL_HALT_NIC;
			break;

		case 3:
			pwrpriv->reg_rfps_level |= RT_RF_OFF_LEVL_PCI_D3;
			break;
	}

	// Update Rx 2R setting
	//pPSC->reg_rfps_level |= ((pDevice->RegLPS2RDisable) ? RT_RF_LPS_DISALBE_2R : 0);

	//
	// Set HW definition to determine if it supports ASPM.
	//
	switch (pdvobjpriv->const_support_pciaspm) {
		case 0:	// Not support ASPM.
			{
				u8	b_support_aspm = _FALSE;
				pwrpriv->b_support_aspm = b_support_aspm;
			}
			break;

		case 1:	// Support ASPM.
			{
				u8	b_support_aspm = _TRUE;
				u8	b_support_backdoor = _TRUE;

				pwrpriv->b_support_aspm = b_support_aspm;

				/*if(pAdapter->MgntInfo.CustomerID == RT_CID_TOSHIBA &&
					pdvobjpriv->ndis_adapter.pcibridge_vendor == PCI_BRIDGE_VENDOR_AMD && 
					!pdvobjpriv->ndis_adapter.amd_l1_patch)
					b_support_backdoor = _FALSE;*/

				pwrpriv->b_support_backdoor = b_support_backdoor;
			}
			break;

		case 2:	// Set by Chipset.
			// ASPM value set by chipset. 
			if (pdvobjpriv->ndis_adapter.pcibridge_vendor == PCI_BRIDGE_VENDOR_INTEL) {
				u8	b_support_aspm = _TRUE;
				pwrpriv->b_support_aspm = b_support_aspm;
			}
			break;

		default:
			// Do nothing. Set when finding the chipset.
			break;
	}
}

static void rtw_pci_initialize_adapter_common(_adapter *padapter)
{
	struct pwrctrl_priv	*pwrpriv = &padapter->pwrctrlpriv;

	rtw_pci_update_default_setting(padapter);

	if (pwrpriv->reg_rfps_level & RT_RF_PS_LEVEL_ALWAYS_ASPM) {
		// Always enable ASPM & Clock Req.
		rtw_pci_enable_aspm(padapter);
		RT_SET_PS_LEVEL(pwrpriv, RT_RF_PS_LEVEL_ALWAYS_ASPM);
	}

}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)) || (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18))
#define rtw_pci_interrupt(x,y,z) rtw_pci_interrupt(x,y)
#endif

static irqreturn_t rtw_pci_interrupt(int irq, void *priv, struct pt_regs *regs)
{
	_adapter			*padapter = (_adapter *)priv;
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;


	if (pdvobjpriv->irq_enabled == 0) {
		return IRQ_HANDLED;
	}

	if(padapter->HalFunc.interrupt_handler(padapter) == _FAIL)
		return IRQ_HANDLED;
		//turn IRQ_NONE;

	return IRQ_HANDLED;
}

static void intf_chip_configure(_adapter *padapter)
{
	if(padapter->HalFunc.intf_chip_configure)
		padapter->HalFunc.intf_chip_configure(padapter);
}

static void intf_read_chip_info(_adapter *padapter)
{
	padapter->HalFunc.read_adapter_info(padapter);
}

static u32 pci_dvobj_init(_adapter *padapter)
{
	u32	status = _SUCCESS;
	struct dvobj_priv	*pdvobjpriv = &padapter->dvobjpriv;
	struct pci_dev *pdev = pdvobjpriv->ppcidev;
	struct pci_dev *bridge_pdev = pdev->bus->self;
	u8	tmp;

_func_enter_;

#if 1
	/*find bus info*/
	pdvobjpriv->ndis_adapter.busnumber = pdev->bus->number;
	pdvobjpriv->ndis_adapter.devnumber = PCI_SLOT(pdev->devfn);
	pdvobjpriv->ndis_adapter.funcnumber = PCI_FUNC(pdev->devfn);

	/*find bridge info*/
	pdvobjpriv->ndis_adapter.pcibridge_vendor = PCI_BRIDGE_VENDOR_UNKNOWN;
	if(bridge_pdev){
		pdvobjpriv->ndis_adapter.pcibridge_vendorid = bridge_pdev->vendor;
		for (tmp = 0; tmp < PCI_BRIDGE_VENDOR_MAX; tmp++) {
			if (bridge_pdev->vendor == pcibridge_vendors[tmp]) {
				pdvobjpriv->ndis_adapter.pcibridge_vendor = tmp;
				printk("Pci Bridge Vendor is found index: %d, %x\n", tmp, pcibridge_vendors[tmp]);
				break;
			}
		}
	}

	if (pdvobjpriv->ndis_adapter.pcibridge_vendor != PCI_BRIDGE_VENDOR_UNKNOWN) {
		pdvobjpriv->ndis_adapter.pcibridge_busnum = bridge_pdev->bus->number;
		pdvobjpriv->ndis_adapter.pcibridge_devnum = PCI_SLOT(bridge_pdev->devfn);
		pdvobjpriv->ndis_adapter.pcibridge_funcnum = PCI_FUNC(bridge_pdev->devfn);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,34))
		pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset = pci_find_capability(bridge_pdev, PCI_CAP_ID_EXP);
#else
		pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset = bridge_pdev->pcie_cap;
#endif

		pdvobjpriv->ndis_adapter.pcicfg_addrport= 
			(pdvobjpriv->ndis_adapter.pcibridge_busnum<< 16) |
			(pdvobjpriv->ndis_adapter.pcibridge_devnum << 11) |
			(pdvobjpriv->ndis_adapter.pcibridge_funcnum <<  8) |
			(1 << 31);
		pdvobjpriv->ndis_adapter.num4bytes = (pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset + 0x10) / 4;

		rtw_pci_get_linkcontrol_field(padapter);
		
		if (pdvobjpriv->ndis_adapter.pcibridge_vendor == PCI_BRIDGE_VENDOR_AMD) {
			pdvobjpriv->ndis_adapter.amd_l1_patch = rtw_pci_get_amd_l1_patch(padapter);
		}
	}
#else
	//
	// Find bridge related info. 
	//
	rtw_get_pci_bus_info(padapter,
				  pdev->vendor,
				  pdev->device,
				  (u8) pdvobjpriv->irqline,
				  0x02, 0x80, U1DONTCARE,
				  &pdvobjpriv->ndis_adapter.busnumber,
				  &pdvobjpriv->ndis_adapter.devnumber,
				  &pdvobjpriv->ndis_adapter.funcnumber);

	rtw_find_bridge_info(padapter);

	if (pdvobjpriv->ndis_adapter.pcibridge_vendor != PCI_BRIDGE_VENDOR_UNKNOWN) {
		rtw_get_link_control_field(padapter,
						pdvobjpriv->ndis_adapter.pcibridge_busnum,
						pdvobjpriv->ndis_adapter.pcibridge_devnum,
						pdvobjpriv->ndis_adapter.pcibridge_funcnum);

		if (pdvobjpriv->ndis_adapter.pcibridge_vendor == PCI_BRIDGE_VENDOR_AMD) {
			pdvobjpriv->ndis_adapter.amd_l1_patch =
				rtw_get_amd_l1_patch(padapter,
							pdvobjpriv->ndis_adapter.pcibridge_busnum,
							pdvobjpriv->ndis_adapter.pcibridge_devnum,
							pdvobjpriv->ndis_adapter.pcibridge_funcnum);
		}
	}
#endif

	//
	// Allow the hardware to look at PCI config information.
	//
	rtw_pci_parse_configuration(pdev, padapter);

	printk("pcidev busnumber:devnumber:funcnumber:"
		"vendor:link_ctl %d:%d:%d:%x:%x\n",
		pdvobjpriv->ndis_adapter.busnumber,
		pdvobjpriv->ndis_adapter.devnumber,
		pdvobjpriv->ndis_adapter.funcnumber,
		pdev->vendor,
		pdvobjpriv->ndis_adapter.linkctrl_reg);

	printk("pci_bridge busnumber:devnumber:funcnumber:vendor:"
		"pcie_cap:link_ctl_reg:amd %d:%d:%d:%x:%x:%x:%x\n", 
		pdvobjpriv->ndis_adapter.pcibridge_busnum,
		pdvobjpriv->ndis_adapter.pcibridge_devnum,
		pdvobjpriv->ndis_adapter.pcibridge_funcnum,
		pcibridge_vendors[pdvobjpriv->ndis_adapter.pcibridge_vendor],
		pdvobjpriv->ndis_adapter.pcibridge_pciehdr_offset,
		pdvobjpriv->ndis_adapter.pcibridge_linkctrlreg,
		pdvobjpriv->ndis_adapter.amd_l1_patch);

	//.2
	if ((init_io_priv(padapter)) == _FAIL)
	{
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,(" \n Can't init io_reqs\n"));
		status = _FAIL;
	}

	//.3

	//.4
	intf_chip_configure(padapter);

_func_exit_;

	return status;
}

static void pci_dvobj_deinit(_adapter * padapter)
{
	//struct dvobj_priv *pdvobjpriv=&padapter->dvobjpriv;

_func_enter_;

_func_exit_;
}


static void decide_chip_type_by_pci_device_id(_adapter *padapter, struct pci_dev *pdev)
{
	u16	venderid, deviceid, irqline;
	u8	revisionid;
	struct dvobj_priv	*pdvobjpriv=&padapter->dvobjpriv;


	venderid = pdev->vendor;
	deviceid = pdev->device;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,23))
	pci_read_config_byte(pdev, 0x8, &revisionid);
#else
	revisionid = pdev->revision;
#endif
	pci_read_config_word(pdev, 0x3C, &irqline);
	pdvobjpriv->irqline = irqline;


	if (deviceid == HAL_HW_PCI_8190_DEVICE_ID ||
		deviceid == HAL_HW_PCI_0045_DEVICE_ID ||
		deviceid == HAL_HW_PCI_0046_DEVICE_ID ||
		deviceid == HAL_HW_PCI_DLINK_DEVICE_ID)
	{
		printk("Adapter(8190 PCI) is found - vendorid/deviceid=%x/%x\n", venderid, deviceid);
		//rtlhal->hw_type = HARDWARE_TYPE_RTL8190P;
	}
	else if (deviceid == HAL_HW_PCI_8192_DEVICE_ID ||
		deviceid == HAL_HW_PCI_0044_DEVICE_ID ||
		deviceid == HAL_HW_PCI_0047_DEVICE_ID ||
		deviceid == HAL_HW_PCI_8192SE_DEVICE_ID ||
		deviceid == HAL_HW_PCI_8174_DEVICE_ID ||
		deviceid == HAL_HW_PCI_8173_DEVICE_ID ||
		deviceid == HAL_HW_PCI_8172_DEVICE_ID ||
		deviceid == HAL_HW_PCI_8171_DEVICE_ID)
	{
		// 8192e and and 8192se may have the same device ID 8192. However, their Revision
		// ID is different
		switch (revisionid) {
			case HAL_HW_PCI_REVISION_ID_8192PCIE:
				printk("Adapter(8192 PCI-E) is found - vendorid/deviceid=%x/%x\n", venderid, deviceid);
				//rtlhal->hw_type = HARDWARE_TYPE_RTL8192E;
				break;
			case HAL_HW_PCI_REVISION_ID_8192SE:
				printk("Adapter(8192SE) is found - vendorid/deviceid=%x/%x\n", venderid, deviceid);
				//rtlhal->hw_type = HARDWARE_TYPE_RTL8192SE;
				break;
			default:
				printk("Err: Unknown device - vendorid/deviceid=%x/%x\n", venderid, deviceid);
				//rtlhal->hw_type = HARDWARE_TYPE_RTL8192SE;
				break;
		}
	}
	else if (deviceid == HAL_HW_PCI_8192CET_DEVICE_ID ||
		deviceid == HAL_HW_PCI_8192CE_DEVICE_ID ||
		deviceid == HAL_HW_PCI_8191CE_DEVICE_ID ||
		deviceid == HAL_HW_PCI_8188CE_DEVICE_ID) 
	{
		printk("Adapter(8192C PCI-E) is found - vendorid/deviceid=%x/%x\n", venderid, deviceid);
		//rtlhal->hw_type = HARDWARE_TYPE_RTL8192CE;
	}
	else
	{
		printk("Err: Unknown device - vendorid/deviceid=%x/%x\n", venderid, deviceid);
		//rtlhal->hw_type = HAL_DEFAULT_HARDWARE_TYPE;
	}


	padapter->chip_type = NULL_CHIP_TYPE;

	//TODO:
	padapter->chip_type = RTL8188C_8192C;

}

void rtw_intf_start(_adapter *padapter)
{

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+rtw_intf_start\n"));
	printk("+rtw_intf_start\n");

	//Enable hw interrupt
	padapter->HalFunc.enable_interrupt(padapter);

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-rtw_intf_start\n"));
	printk("-rtw_intf_start\n");
}

static void rtw_intf_stop(_adapter *padapter)
{

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+rtw_intf_stop\n"));

	//Disable hw interrupt
	if(padapter->bSurpriseRemoved == _FALSE)
	{
		//device still exists, so driver can do i/o operation
		padapter->HalFunc.disable_interrupt(padapter);
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("rtw_intf_stop: SurpriseRemoved==_FALSE\n"));
	}
	else
	{
		// Clear irq_enabled to prevent handle interrupt function.
		padapter->dvobjpriv.irq_enabled = 0;
	}

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-rtw_intf_stop\n"));

}

#ifdef CONFIG_IPS
void ips_dev_unload(_adapter *padapter)
{
	struct net_device *pnetdev= (struct net_device*)padapter->pnetdev;

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+ips_dev_unload\n"));
	printk("%s...\n",__FUNCTION__);
//	if(padapter->bup == _TRUE)
	{
		printk("+ips_dev_unload\n");
		//padapter->bup = _FALSE;
		//padapter->bDriverStopped = _TRUE;

		//s3.
		write8(padapter,0x522,0xff);//pause tx/rx
		rtw_intf_stop(padapter);//cancel read /write port

		//s5.
		if(padapter->bSurpriseRemoved == _FALSE)
		{
			printk("r871x_dev_unload()->rtl871x_hal_deinit()\n");
			rtw_hal_deinit(padapter);

			//padapter->bSurpriseRemoved = _TRUE;
		}

		//s6.
		if(padapter->dvobj_deinit)
		{
			padapter->dvobj_deinit(padapter);

		}
		else
		{
			RT_TRACE(_module_hci_intfs_c_,_drv_err_,("Initialize hcipriv.hci_priv_init error!!!\n"));
		}

	}
/*	else
	{
		printk("ips_dev_unload():padapter->bup == _FALSE\n" );
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("r871x_dev_unload():padapter->bup == _FALSE\n" ));
	}*/
	printk("-ips_dev_unload\n");

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-r871x_dev_unload\n"));
}
#endif

static void rtw_dev_unload(_adapter *padapter)
{
	struct net_device *pnetdev= (struct net_device*)padapter->pnetdev;

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+rtw_dev_unload\n"));

	if(padapter->bup == _TRUE)
	{
		printk("+rtw_dev_unload\n");
		//s1.
/*		if(pnetdev)
		{
			netif_carrier_off(pnetdev);
			netif_stop_queue(pnetdev);
		}

		//s2.
		//s2-1.  issue disassoc_cmd to fw
		disassoc_cmd(padapter);
		//s2-2.  indicate disconnect to os
		indicate_disconnect(padapter);
		//s2-3.
		free_assoc_resources(padapter);
		//s2-4.
		free_network_queue(padapter);*/

		padapter->bDriverStopped = _TRUE;

		//s3.
		rtw_intf_stop(padapter);

		//s4.
		stop_drv_threads(padapter);


		//s5.
		if(padapter->bSurpriseRemoved == _FALSE)
		{
			printk("r871x_dev_unload()->rtl871x_hal_deinit()\n");
			rtw_hal_deinit(padapter);

			padapter->bSurpriseRemoved = _TRUE;
		}

		//s6.
		pci_dvobj_deinit(padapter);
		

		padapter->bup = _FALSE;

	}
	else
	{
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("r871x_dev_unload():padapter->bup == _FALSE\n" ));
	}

	printk("-rtw_dev_unload\n");

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-rtw_dev_unload\n"));

}

static void disable_ht_for_spec_devid(const struct pci_device_id *pdid)
{
#ifdef CONFIG_80211N_HT
	u16 vid, pid;
	u32 flags;
	int i;
	int num = sizeof(specific_device_id_tbl)/sizeof(struct specific_device_id);

	for(i=0; i<num; i++)
	{
		vid = specific_device_id_tbl[i].idVendor;
		pid = specific_device_id_tbl[i].idProduct;
		flags = specific_device_id_tbl[i].flags;

		if((pdid->vendor==vid) && (pdid->device==pid) && (flags&SPEC_DEV_ID_DISABLE_HT))
		{
			 ht_enable = 0;
			 cbw40_enable = 0;
			 ampdu_enable = 0;
		}

	}
#endif
}

#ifdef CONFIG_PM
static int rtw_suspend(struct pci_dev *pdev, pm_message_t state)
{	
	_func_enter_;


	_func_exit_;
	return 0;
}

static int rtw_resume(struct pci_dev *pdev)
{
	_func_enter_;


	_func_exit_;
	
	return 0;
}
#endif


u8 key_char2num(u8 ch)
{
    if((ch>='0')&&(ch<='9'))
        return ch - '0';
    else if ((ch>='a')&&(ch<='f'))
        return ch - 'a' + 10;
    else if ((ch>='A')&&(ch<='F'))
        return ch - 'A' + 10;
    else
	 return 0xff;
}

u8 key_2char2num(u8 hch, u8 lch)
{
    return ((key_char2num(hch) << 4) | key_char2num(lch));
}

#ifdef RTK_DMP_PLATFORM
#define pci_iounmap(x,y) iounmap(y)
#endif

/*
 * drv_init() - a device potentially for us
 *
 * notes: drv_init() is called when the bus driver has located a card for us to support.
 *        We accept the new device by returning 0.
*/
static int rtw_drv_init(struct pci_dev *pdev, const struct pci_device_id *pdid)
{
	int i, err = -ENODEV;
	u8 mac[ETH_ALEN];
	uint status;
	_adapter *padapter = NULL;
	struct dvobj_priv *pdvobjpriv;
	struct net_device *pnetdev;
	unsigned long pmem_start, pmem_len, pmem_flags;
	u8	bdma64 = _FALSE;

	RT_TRACE(_module_hci_intfs_c_, _drv_err_, ("+rtw_drv_init\n"));
	//printk("+rtw_drv_init\n");

	err = pci_enable_device(pdev);
	if (err) {
		printk(KERN_ERR "%s : Cannot enable new PCI device\n", pci_name(pdev));
		return err;
	}

	pci_set_master(pdev);

#ifdef CONFIG_64BIT_DMA
	if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(64))) {
		printk("RTL819xCE: Using 64bit DMA\n");
		if (pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(64))) {
			printk(KERN_ERR "Unable to obtain 64bit DMA for consistent allocations\n");
			err = -ENOMEM;
			pci_disable_device(pdev);
			return err;
		}
		bdma64 = _TRUE;
	} else 
#endif
	{
		if (!pci_set_dma_mask(pdev, DMA_BIT_MASK(32))) {
			if (pci_set_consistent_dma_mask(pdev, DMA_BIT_MASK(32))) {
				printk(KERN_ERR "Unable to obtain 32bit DMA for consistent allocations\n");
				err = -ENOMEM;
				pci_disable_device(pdev);
				return err;
			}
		}
	}

#ifdef CONFIG_80211N_HT
	//step 0.
	disable_ht_for_spec_devid(pdid);
#endif

	//step 1. set USB interface data
	// init data
	pnetdev = init_netdev();
	if (!pnetdev){
		err = -ENOMEM;
		goto fail1;
	}

	if(bdma64){
		pnetdev->features |= NETIF_F_HIGHDMA;
	}

#if LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0)
	SET_NETDEV_DEV(pnetdev, &pdev->dev);
#endif

	padapter = netdev_priv(pnetdev);
	pdvobjpriv = &padapter->dvobjpriv;
	pdvobjpriv->padapter = padapter;
	pdvobjpriv->ppcidev = pdev;

	// set data
	pci_set_drvdata(pdev, pnetdev);

	err = pci_request_regions(pdev, DRV_NAME);
	if (err) {
		printk(KERN_ERR "Can't obtain PCI resources\n");
		goto fail1;
	}
	//MEM map
	pmem_start = pci_resource_start(pdev, 2);
	pmem_len = pci_resource_len(pdev, 2);
	pmem_flags = pci_resource_flags(pdev, 2);

#ifdef RTK_DMP_PLATFORM
	pdvobjpriv->pci_mem_start = (unsigned long)ioremap_nocache( pmem_start, pmem_len);
#else
	pdvobjpriv->pci_mem_start = (unsigned long)pci_iomap(pdev, 2, pmem_len);	// shared mem start
#endif
	if (pdvobjpriv->pci_mem_start == 0) {
		printk(KERN_ERR "Can't map PCI mem\n");
		goto fail2;
	}

	printk("Memory mapped space start: 0x%08lx len:%08lx flags:%08lx, after map:0x%08lx\n",
		pmem_start, pmem_len, pmem_flags, pdvobjpriv->pci_mem_start);

	// Disable Clk Request */
	pci_write_config_byte(pdev, 0x81, 0);
	// leave D3 mode */
	pci_write_config_byte(pdev, 0x44, 0);
	pci_write_config_byte(pdev, 0x04, 0x06);
	pci_write_config_byte(pdev, 0x04, 0x07);

	//step 1-1., decide the chip_type via vid/pid
	decide_chip_type_by_pci_device_id(padapter, pdev);

	//step 2.	
	if(padapter->chip_type & RTL8188C_8192C)
	{
#ifdef CONFIG_RTL8192C
		rtl8192ce_set_hal_ops(padapter);
#endif
	}
	else if(padapter->chip_type & RTL8192D)
	{
#ifdef CONFIG_RTL8192D
		rtl8192de_set_hal_ops(padapter);
#endif
	}
	else
	{
		status = _FAIL;
		goto error;
	}
		

	//step 3.
	//initialize the dvobj_priv
	status = pci_dvobj_init(padapter);	
	if (status != _SUCCESS) {
		RT_TRACE(_module_hci_intfs_c_, _drv_err_, ("initialize device object priv Failed!\n"));
		goto error;
	}

	pnetdev->irq = pdev->irq;

	//step 4.
	status = init_drv_sw(padapter);
	if(status ==_FAIL){
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("Initialize driver software resource Failed!\n"));
		goto error;
	}

	status = padapter->HalFunc.inirp_init(padapter);
	if(status ==_FAIL){
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("Initialize PCI desc ring Failed!\n"));
		goto error;
	}
	


	//step 5. read efuse/eeprom data and get mac_addr
	intf_read_chip_info(padapter);	

	if ( initmac )
	{	//	Users specify the mac address
		int jj,kk;

		for( jj = 0, kk = 0; jj < ETH_ALEN; jj++, kk += 3 )
		{
			mac[jj] = key_2char2num(initmac[kk], initmac[kk+ 1]);
		}
	}
	else
	{	//	Use the mac address stored in the Efuse
		_memcpy(mac, padapter->eeprompriv.mac_addr, ETH_ALEN);
	}

	if (((mac[0]==0xff) &&(mac[1]==0xff) && (mac[2]==0xff) &&
	     (mac[3]==0xff) && (mac[4]==0xff) &&(mac[5]==0xff)) ||
	    ((mac[0]==0x0) && (mac[1]==0x0) && (mac[2]==0x0) &&
	     (mac[3]==0x0) && (mac[4]==0x0) &&(mac[5]==0x0)))
	{
		mac[0] = 0x00;
		mac[1] = 0xe0;
		mac[2] = 0x4c;
		mac[3] = 0x87;
		mac[4] = 0x00;
		mac[5] = 0x00;
	}
	_memcpy(pnetdev->dev_addr, mac, ETH_ALEN);
	printk("MAC Address from efuse= %02x:%02x:%02x:%02x:%02x:%02x\n",
				mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

	padapter->HalFunc.disable_interrupt(padapter);

#if defined(IRQF_SHARED)
	err = request_irq(pdev->irq, &rtw_pci_interrupt, IRQF_SHARED, DRV_NAME, padapter);
#else
	err = request_irq(pdev->irq, &rtw_pci_interrupt, SA_SHIRQ, DRV_NAME, padapter);
#endif
	if (err) {
		printk("Error allocating IRQ %d",pdev->irq);
		goto error;
	} else {
		pdvobjpriv->irq_alloc = 1;
		printk("Request_irq OK, IRQ %d\n",pdev->irq);
	}

	//step 6. Init pci related configuration
	rtw_pci_initialize_adapter_common(padapter);

	//step 7.
	/* Tell the network stack we exist */
	if (register_netdev(pnetdev) != 0) {
		RT_TRACE(_module_hci_intfs_c_,_drv_err_,("register_netdev() failed\n"));
		goto error;
	}

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-drv_init - Adapter->bDriverStopped=%d, Adapter->bSurpriseRemoved=%d\n",padapter->bDriverStopped, padapter->bSurpriseRemoved));
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("-871x_drv - drv_init, success!\n"));
	//printk("-871x_drv - drv_init, success!\n");

#ifdef RTK_DMP_PLATFORM
	rtw_proc_init_one(pnetdev);
#endif

#ifdef CONFIG_HOSTAPD_MLME
	hostapd_mode_init(padapter);
#endif

#ifdef CONFIG_PLATFORM_RTD2880B
	printk("wlan link up\n");
	//rtd2885_wlan_netlink_sendMsg("linkup", "8712");
#endif

	return 0;

error:

	pci_set_drvdata(pdev, NULL);

	if (pdvobjpriv->irq_alloc) {
		free_irq(pdev->irq, padapter);
		pdvobjpriv->irq_alloc = 0;
	}

	if (pdvobjpriv->pci_mem_start != 0) {
		pci_iounmap(pdev, (void *)pdvobjpriv->pci_mem_start);
	}

	pci_dvobj_deinit(padapter);

	if (pnetdev)
	{
		//unregister_netdev(pnetdev);
		free_netdev(pnetdev);
	}

fail2:
	pci_release_regions(pdev);

fail1:
	pci_disable_device(pdev);

	printk("-871x_pci - drv_init, fail!\n");

	return err;
}

#ifdef CONFIG_IPS
int r871xu_ips_pwr_up(_adapter *padapter)
{	
	int result;
	printk("===>  r871xu_ips_pwr_up..............\n");
	reset_drv_sw(padapter);
	result = ips_netdrv_open(padapter);
 	printk("<===  r871xu_ips_pwr_up..............\n");
	return result;

}

void r871xu_ips_pwr_down(_adapter *padapter)
{
	printk("===> r871xu_ips_pwr_down...................\n");

	padapter->bCardDisableWOHSM = _TRUE;
	padapter->net_closed = _TRUE;
		
	free_network_queue(padapter);

	padapter->ledpriv.LedControlHandler(padapter, LED_CTL_NO_LINK);
	
	ips_dev_unload(padapter);
	padapter->bCardDisableWOHSM = _FALSE;
	printk("<=== r871xu_ips_pwr_down.....................\n");
}
#endif

/*
 * dev_remove() - our device is being removed
*/
//rmmod module & unplug(SurpriseRemoved) will call r871xu_dev_remove() => how to recognize both
static void rtw_dev_remove(struct pci_dev *pdev)
{
	struct net_device *pnetdev=pci_get_drvdata(pdev);
	_adapter *padapter = (_adapter*)netdev_priv(pnetdev);
	struct dvobj_priv *pdvobjpriv = &padapter->dvobjpriv;

_func_exit_;

	if (unlikely(!padapter)) {
		return;
	}

	printk("+rtw_dev_remove\n");

#ifdef CONFIG_HOSTAPD_MLME
	hostapd_mode_unload(padapter);
#endif
	LeaveAllPowerSaveMode(padapter);

#ifdef RTK_DMP_PLATFORM    
    padapter->bSurpriseRemoved = _FALSE;        // always trate as device exists
                                                // this will let the driver to disable it's interrupt
#else	
	if(drvpriv.drv_registered == _TRUE)
	{
		//printk("r871xu_dev_remove():padapter->bSurpriseRemoved == _TRUE\n");
		padapter->bSurpriseRemoved = _TRUE;
	}
	/*else
	{
		//printk("r871xu_dev_remove():module removed\n");
		padapter->hw_init_completed = _FALSE;
	}*/
#endif

	if(pnetdev){
		unregister_netdev(pnetdev); //will call netdev_close()
#ifdef RTK_DMP_PLATFORM
		rtw_proc_remove_one(pnetdev);
#endif
	}

	cancel_all_timer(padapter);

	rtw_dev_unload(padapter);

	printk("+r871xu_dev_remove, hw_unload_completed=%d\n", padapter->hw_init_completed);	

	if (pdvobjpriv->irq_alloc) {
		free_irq(pdev->irq, padapter);
		pdvobjpriv->irq_alloc = 0;
	}

	if (pdvobjpriv->pci_mem_start != 0) {
		pci_iounmap(pdev, (void *)pdvobjpriv->pci_mem_start);
		pci_release_regions(pdev);
	}

	pci_disable_device(pdev);
	pci_set_drvdata(pdev, NULL);

	padapter->HalFunc.inirp_deinit(padapter);

	free_drv_sw(padapter);

	//after free_drv_sw(), padapter has beed freed, don't refer to it.

	printk("-r871xu_dev_remove, done\n");

#ifdef CONFIG_PLATFORM_RTD2880B
	printk("wlan link down\n");
	//rtd2885_wlan_netlink_sendMsg("linkdown", "8712");
#endif

_func_exit_;

	return;

}


static int __init rtw_drv_entry(void)
{
	int ret = 0;

	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+rtw_drv_entry\n"));
	printk("rtw driver version=%s\n", DRIVERVERSION);
	drvpriv.drv_registered = _TRUE;
	ret = pci_register_driver(&drvpriv.rtw_pci_drv);
	if (ret) {
		RT_TRACE(_module_hci_intfs_c_, _drv_err_, (": No device found\n"));
	}

	return ret;
}

static void __exit rtw_drv_halt(void)
{
	RT_TRACE(_module_hci_intfs_c_,_drv_err_,("+rtw_drv_halt\n"));
	printk("+rtw_drv_halt\n");
	drvpriv.drv_registered = _FALSE;
	pci_unregister_driver(&drvpriv.rtw_pci_drv);
	printk("-rtw_drv_halt\n");
}


module_init(rtw_drv_entry);
module_exit(rtw_drv_halt);

