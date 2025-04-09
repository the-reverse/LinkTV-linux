/* -------------------------------------------------------------------- 
 * pci-jupiter.c  jupiter pci-dvr bridge driver
 * -------------------------------------------------------------------- 
 * Copyright (C) 2008 Kevin Wang <kevin_wang@realtek.com.tw>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.    
 * -------------------------------------------------------------------- 
 * Update List :
 * -------------------------------------------------------------------- 
 *     1.1     |   20080908    | register irq at jupiter-pci initialization stage
 * --------------------------------------------------------------------*/ 
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <platform.h>
#include "pci-jupiter.h"


#define FORCE_MAX_RX_LENGTH

extern struct pci_ops jupiter_pci_ops;

static struct resource py_mem_resource = { "Realtek Jupiter PCI-Express MEM", 0x12000000UL, 0x17ffffffUL, IORESOURCE_MEM };
static struct resource  py_io_resource = { "Realtek Jupiter PCI-Express IO",  0x00030000UL, 0x0003ffffUL, IORESOURCE_IO  };
static struct pci_controller py_controller = 
{
    .pci_ops        = &jupiter_pci_ops,
    .mem_resource   = &py_mem_resource,
    .mem_offset     = 0,
    .io_resource    = &py_io_resource,
    .io_offset      = 0x00030000UL
};



#define payload_size_conv(x)        (((x)<=5) ? (128 << (x)) : 0)

extern int _indirect_cfg_read(unsigned long addr, unsigned long* pdata, unsigned char size);
extern int _indirect_cfg_write(unsigned long addr, unsigned long data, unsigned char size);


/*--------------------------------------------------------
 * Func  : pcibios_plat_dev_init 
 *
 * Desc  : platform specific device initialization.
 *
 *         this function will be invoked when PCI driver calling
 *         pc_enable_device() 
 *
 * Parm  : dev  : device to be init 
 *
 * Retn  : 0 for success
 *--------------------------------------------------------*/
int pcibios_plat_dev_init(struct pci_dev* dev)
{        
    u16 command;
    
#ifdef FORCE_MAX_RX_LENGTH

    int cap = pci_find_capability(dev, PCI_CAP_ID_EXP);
    u32 devcap;
    u16 devctrl;
    pci_read_config_dword(dev, cap + PCI_EXP_DEVCAP, &devcap);
    pci_read_config_word (dev, cap + PCI_EXP_DEVCTL, &devctrl);
    
    printk("dev cap (%xh) = %08x, max payload size supported = %d (%x)\n", 
            cap + PCI_EXP_DEVCAP, devcap, payload_size_conv(devcap & 0x07), devcap & 0x07);
            
    printk("dev ctrl(%xh) = %04x, max read size= %d (%x), max payload size=%d(%x)\n", 
            cap + PCI_EXP_DEVCTL, devctrl, payload_size_conv((devctrl >>12) & 0x7),
            (devctrl >>12) & 0x7, payload_size_conv((devctrl >>5) & 0x7), (devctrl >> 5) & 0x7);            
            
    devctrl &= ~((0x07 << 12) | (0x07 << 5));
    pci_write_config_word (dev, cap + PCI_EXP_DEVCTL, devctrl);
    
    printk("dev ctrl(%xh) = %04x, max read size= %d (%x), max payload size=%d(%x)\n", 
            cap + PCI_EXP_DEVCTL, devctrl, payload_size_conv((devctrl >>12) & 0x7),
            (devctrl >>12) & 0x7, payload_size_conv((devctrl >>5) & 0x7), (devctrl >> 5) & 0x7);

#endif

    if (dev->irq != 2)
        dev->irq  = 2;    
    
    pci_read_config_word(dev, PCI_COMMAND, &command);
    
    if (command & PCI_COMMAND_INTX_DISABLE)
    {
        PCI_WARNING("This card's interrupt has beed disabled for some reason, you may need to check your card first\n");                                      
    }

	return 0;
}







/*--------------------------------------------------------
 * Func  : pcibios_map_irq
 * 
 * Desc  : This function be invoked when the pci core about 
 *         to associates an irq to a pci device
 *
 * Parm  : dev  : specified pci device
 *         slot : which slot the pci device at
 *         pin  : which INT# pin does the pci device used  
 *
 * Retn  : associated irq
 *--------------------------------------------------------*/
int __init pcibios_map_irq(struct pci_dev* dev, u8 slot, u8 pin)
{	
	return (pin==0) ? -1 : 2;
}




/*--------------------------------------------------------
 * Func  : jupiter_pcie_rc_init  
 *
 * Desc  : Init PCI-e bridge.
 *
 * Parm  : N/A
 *
 * Retn  : 0 : success, others failed  
 *--------------------------------------------------------*/  
int jupiter_pcie_rc_init(void)
{
	/*-------------------------------------------
	 * SB2 configurations
	 *-------------------------------------------*/
	/* SB2 RBUS1 Timeout timeout */
    #define RBUS1_TIMEOUT(x)  ((x & 0x7) << 2)                        
    #define RBUS_TIMEOUT(x)   (x & 0x3)

	wr_reg(0xb801A010, RBUS1_TIMEOUT(0x3) | RBUS_TIMEOUT(0x3));        // OK Intel Pass, RT iperf Pa
	wr_reg(0xb8017A1C, 0x18D30801);

	/* SB2 address translation for PCI-E */
    SET_SB2_PCI_BASE0(0x12000000);    
    SET_SB2_PCI_MASK0(0xfe000000);    
    SET_SB2_PCI_TRAN0(0x12000000);

    SET_SB2_PCI_BASE1(0x14000000);
    SET_SB2_PCI_MASK1(0xfc000000);
    SET_SB2_PCI_TRAN1(0x14000000);

    SET_SB2_PCI_BASE2(0x18040000);    
    SET_SB2_PCI_MASK2(0xffff0000);    
    SET_SB2_PCI_TRAN2(0x00000000);

    SET_SB2_PCI_CTRL(0x0000017f); 	

    /*-------------------------------------------
	 * PCI-E RC configuration space initialization
	 *-------------------------------------------*/
    SET_CFG(0x04, 0x0010007);           //Enable bus master and receive IO/MM cmd 	
	SET_CFG(0x78, 0x100010);            //payload size = 128

    /*-------------------------------------------
	 * PCI-E RC Initialization
	 *-------------------------------------------*/	
	SET_PCIE_SYS_CTR(INDOR_CFG_EN | APP_LTSSM_EN);	    //release MDIO reset, and enable CFG	

	SET_PCIE_SYS_CTR(GET_PCIE_SYS_CTR() | APP_INIT_RST);       
    udelay(100);
    SET_PCIE_SYS_CTR(GET_PCIE_SYS_CTR() & ~APP_INIT_RST);
    udelay(100);
               
    return 0;
}



/*--------------------------------------------------------
 * Func  : jupiter_pcie_init  
 *
 * Desc  : Init PCI-e bridge.
 *
 * Parm  : N/A
 *
 * Retn  : 0 : success, others failed  
 *--------------------------------------------------------*/  
static int __init jupiter_pcie_init(void)
{ 
    unsigned long cmd;
    
    if (!is_jupiter_cpu())
    {
        PCI_WARNING("FETAL error - not Jupiter CPU\n");
        return -ENODEV;
    }
    
    jupiter_pcie_rc_init();
        
    if (GET_PCIE_MAC_ST() & (XMLH_LINK_UP | RDLH_LINK_UP))
    {
        // disable PCI card's interrupt first
        _indirect_cfg_read(PCI_COMMAND, &cmd, 2);       
        _indirect_cfg_write(PCI_COMMAND, cmd | PCI_COMMAND_INTX_DISABLE, 2);        
    }

    register_pci_controller(&py_controller);
    
    return 0;
}
    
arch_initcall(jupiter_pcie_init);
