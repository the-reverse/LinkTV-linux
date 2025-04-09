#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include "pci-jupiter.h"


#define ADDR_TO_DEVICE_NO(addr)         ((addr >> 19) & 0x1F)



/*======================================================================
 * Func  : _pci_byte_mask
 * 
 * Desc  : generate byte mask 
 *
 * Parm  : addr  : register address
 *         size : specified pci device  
 *
 * Retn  : bit mask
 *======================================================================*/
unsigned long _pci_byte_mask(
    unsigned long           addr, 
    unsigned char           size
    )
{
    unsigned char offset = (addr & 0x03);
    switch (size)    		
    {
    case 0x01:  return 0x1 << offset;        
    case 0x02:  
        if (offset <=2)
            return 0x3 << offset;                      
            
        printk("compute config mask - data cross dword boundrary (addr=%p, length=2)\n", (void*) addr);        
        break;

    case 0x03:        
        if (offset <=1)
            return 0x7 << offset;                      
            
        printk("compute config mask - data cross dword boundrary (addr=%p, length=3)\n", (void*) addr);        
        break;                                   
        
    case 0x04:  
        if (offset==0)
            return 0xF;    
        
        printk("compute config mask - data cross dword boundrary (addr=%p, length=4)\n", (void*) addr);        
        break;
        
    default:        
        printk("compute config mask failed - size %d should between 1~4\n", size);                
    }
    return 0;
}



/*======================================================================
 * Func  : _pci_bit_mask
 * 
 * Desc  : convert bit mask to byte mask
 *
 * Parm  : byte_mask : byte mask
 *
 * Retn  : bit mask
 *======================================================================*/
unsigned long _pci_bit_mask(
    unsigned char           byte_mask
    )
{    
    int i;
    unsigned long mask = 0;
    
    for (i=0; i<4;i++)    
    {
        if ((byte_mask>>i) & 0x1)
            mask |= (0xFF << (i<<3));                
    }
    
    return mask;     
}



/*======================================================================
 * Func  : _pci_bit_shift
 * 
 * Desc  : convert bit mask to byte mask
 *
 * Parm  : byte_mask : byte mask
 *
 * Retn  : bit mask
 *======================================================================*/
unsigned long _pci_bit_shift(
    unsigned long           addr
    )
{            
    return ((addr & 0x3) << 3);     
}



/*======================================================================
 * Func  : _indirect_cfg_read
 * 
 * Desc  : This function used for device driver to read data from registers in
 *         configuration
 *
 * Parm  : addr  : address to read (the last 2 bits should be 0)
 *         data  : readed data
 *         size  : size of read data (should be 1 2 4)
 *
 * Retn  : PCIBIOS_SUCCESSFUL  for success
 *         PCIBIOS_DEVICE_NOT_FOUND  for no device 
 *======================================================================*/
int _indirect_cfg_read(
    unsigned long           addr,
    unsigned long*          pdata,
    unsigned char           size
    )
{	        
    unsigned long   status;     
    unsigned char   mask;
    int             try_count = 20000;
    
    if (ADDR_TO_DEVICE_NO(addr)!=0)         // Jupiter only supports one device per slot,
        return PCIBIOS_DEVICE_NOT_FOUND;    // it's uses bit0 to select enable device's configuration space
                        
    mask = _pci_byte_mask(addr, size);
    
    if (!mask)
        return PCIBIOS_SET_FAILED;
    
    SET_PCIE_INDIR_CTR(0x10);               //read
    
    SET_PCIE_CFG_ST(0x3);                   // clear status flag
			
	SET_PCIE_CFG_ADDR((addr & ~0x3));		        // the lastest 2 bits will be ingnore automatically
	        
    SET_PCIE_CFG_EN(BYTE_CNT(mask) | BYTE_EN | WRRD_EN(0));
	
	SET_PCIE_CFG_CT(GO_CT);						
			
	do
	{	    
	    udelay(50);
	    status = GET_PCIE_CFG_ST();             	    
	}while(!(status & CFG_ST_DONE) && try_count--);
	
	if (try_count<0)
    {
        printk("Read config data (%p) failed - timeout\n", (void*)addr);        
        goto error_occur;
    }
	
	if (GET_PCIE_CFG_ST()& CFG_ST_ERROR)
	{
	    status = GET_CFG(0x04);
	    
	    if (status & CFG_ST_DETEC_PAR_ERROR)    	                        
            printk("Read config data failed - PAR error detected\n");        
          
        if (status & CFG_ST_SIGNAL_SYS_ERROR)    	            
            printk("Read config data failed - system error\n");        
            
        if (status & CFG_ST_REC_MASTER_ABORT)    	            
            printk("Read config data failed - master abort\n");
            
        if (status & CFG_ST_REC_TARGET_ABORT)    	            
            printk("Read config data failed - target abort\n");                                    
        
        if (status & CFG_ST_SIG_TAR_ABORT)    	            
            printk("Read config data failed - tar abort\n");   
                    
        SET_CFG(0x04, status);		                    //clear status bits
        
        goto error_occur;
	}
	
    SET_PCIE_CFG_ST(0x3);	            	
    
    *pdata = (GET_PCIE_CFG_RDATA() & _pci_bit_mask(mask)) >> _pci_bit_shift(addr) ;   
    
    //printk("Read : addr = %p(%p), size=%d, shift=%d, byte_mask=%p, mask=%p, val=%08x (%08x)\n",
      //      addr, (addr & ~0x3), size, _pci_bit_shift(addr), mask, _pci_bit_mask(mask), *pdata, GET_PCIE_CFG_RDATA());    
    		                   		    
	return PCIBIOS_SUCCESSFUL;
	
error_occur:
    SET_PCIE_CFG_ST(0x3);	
    return PCIBIOS_SET_FAILED;    	
}




/*======================================================================
 * Func  : _indirect_cfg_write
 * 
 * Desc  : This function used for device driver to write data to registers in
 *         configuration
 *
 * Parm  : addr  : address to write 
 *         data  : data to write 
 *         size  : number of bytes to write (it should be 1/2/4) 
 *
 * Retn  : PCIBIOS_SUCCESSFUL  for success
 *         PCIBIOS_DEVICE_NOT_FOUND  for no device 
 *======================================================================*/
int _indirect_cfg_write(
    unsigned long           addr,
    unsigned long           data,
    unsigned char           size
    )
{	unsigned long   status;    
    unsigned char   mask;
    int             try_count = 2000;
    
    if (ADDR_TO_DEVICE_NO(addr)!=0)         // Jupiter only supports one device per slot,
        return PCIBIOS_DEVICE_NOT_FOUND;    // it's uses bit0 to select enable device's configuration space
            
    mask = _pci_byte_mask(addr, size);
                
    if (!mask)
        return PCIBIOS_SET_FAILED;
        
    //printk("Write : addr = %p(%p), size=%d, shift=%d, byte_mask=%p, mask=%p, val=%08x (%08x)\n",
      //    addr,(addr & ~0x3), size, _pci_bit_shift(addr), mask, _pci_bit_mask(mask), data, (data << _pci_bit_shift(addr)) & _pci_bit_mask(mask));
                
    data = (data << _pci_bit_shift(addr)) & _pci_bit_mask(mask);        
                
    SET_PCIE_INDIR_CTR(0x12);        
    
    SET_PCIE_CFG_ST(CFG_ST_ERROR|CFG_ST_DONE);                   // clear status flag
			
	SET_PCIE_CFG_ADDR(addr);
	
	SET_PCIE_CFG_WDATA(data);	
	
	if (size == 4)
	    SET_PCIE_CFG_EN(WRRD_EN(1));
    else		
        SET_PCIE_CFG_EN(BYTE_CNT(mask) | BYTE_EN | WRRD_EN(1));
	
	SET_PCIE_CFG_CT(GO_CT);						
			
	do
	{	    
	    udelay(50);
	    status = GET_PCIE_CFG_ST();        
	}while(!(status & CFG_ST_DONE) && try_count--);
	
	if (try_count<0)
    {
        printk("Write config data (%p) failed - timeout\n", (void*) addr);        
        goto error_occur;
    }
	
	if (GET_PCIE_CFG_ST()& CFG_ST_ERROR)
	{
	    status = GET_CFG(0x04);
	    
	    if (status & CFG_ST_DETEC_PAR_ERROR)    	                        
            printk("Write config data failed - PAR error detected\n");        
          
        if (status & CFG_ST_SIGNAL_SYS_ERROR)    	            
            printk("Write config data failed - system error\n");        
            
        if (status & CFG_ST_REC_MASTER_ABORT)    	            
            printk("Write config data failed - master abort\n");
            
        if (status & CFG_ST_REC_TARGET_ABORT)    	            
            printk("Write config data failed - target abort\n");                                    
        
        if (status & CFG_ST_SIG_TAR_ABORT)    	            
            printk("Write config data failed - tar abort\n");   
                    
        SET_CFG(0x04, status);		                    //clear status bits
        goto error_occur;
	}
	      		
	SET_PCIE_CFG_ST(CFG_ST_ERROR|CFG_ST_DONE);	
	
	return PCIBIOS_SUCCESSFUL;
	
error_occur:
    
    SET_PCIE_CFG_ST(CFG_ST_ERROR|CFG_ST_DONE);
    
    return PCIBIOS_SET_FAILED;    	
}



/*======================================================================
 * Func  : _pci_address_conversion
 * 
 * Desc  : convert device's register address to pci address 
 *
 *         Format of PCI Address: 
 *           D[31:24] : Bus number
 *           D[23:19] : Device number
 *           D[18: 16]: Function number (0~7)
 *           D[11: 0] : Regiater address (0~0x3FF)  
 *
 * Parm  : bus   : pci bus
 *         devfn : specified pci device  
 *         reg   : register address
 *
 * Retn  : pci address of the register  
 *======================================================================*/
static 
unsigned long _pci_address_conversion(
    struct pci_bus*         bus, 
    unsigned int            devfn, 
    int                     reg
    )
{   
	int busno = bus->number;
	int dev   = PCI_SLOT(devfn);
	int func  = PCI_FUNC(devfn);	
    return (busno << 24) | (dev << 19) | (func << 16) | reg;
}



/*======================================================================
 * Func  : read_config
 * 
 * Desc  : read device's configuration space
 *
 * Parm  : bus   : which bus does the device belongs to
 *         devfn : specified pci device 
 *         reg   : register address
 *         size  : how much bytes to read (1 / 2 / 4)
 *         val   : output data  buffer
 *
 * Retn  : PCIBIOS_SUCCESSFUL / PCIBIOS_DEVICE_NOT_FOUND
 *======================================================================*/
static 
int read_config(
    struct pci_bus*         bus, 
    unsigned int            devfn, 
    int                     reg,
	int                     size, 
	u32*                    pval
	)
{        
    unsigned long address;
    
    int ret = PCIBIOS_DEVICE_NOT_FOUND;
    
    PCI_CFG_DBG("bus->number = %d\n", bus->number);
    
    if (bus->number==0)        
    {           
        address = _pci_address_conversion(bus, devfn, reg);
        ret = _indirect_cfg_read(address, (unsigned long*) pval, size);	                    
        PCI_CFG_DBG("RD Config (%p:%d) = %08x (ret=%d)\n",  (void*)address, size, *pval, ret);
    }    
    return ret;    
}

    


/*====================================================================== 
 * Func : write_config 
 *
 * Desc : write device's configuration space
 *
 * Parm : bus    : pci bus (IN) 
 *        devfn  : target device & function (IN) 
 *        reg    : register to write (IN) 
 *        size   : bytes to write (IN) (1 / 2 / 4)
 *        val    : data to write (IN) 
 *
 * Retn : PCIBIOS_SUCCESSFUL / PCIBIOS_DEVICE_NOT_FOUND
 *======================================================================*/  
static 
int write_config(
    struct pci_bus*         bus, 
    unsigned int            devfn, 
    int                     reg,
	int                     size, 
	u32                     val
	)
{	            
    unsigned long address;
    
    int ret = PCIBIOS_DEVICE_NOT_FOUND;
    
    PCI_CFG_DBG("bus->number = %d\n", bus->number);
        
    if (bus->number==0)        
    {        
        address = _pci_address_conversion(bus, devfn, reg);                     
        ret = _indirect_cfg_write(address, val, size);                
        PCI_CFG_DBG("WR Config (%p:%d) = %08x (ret=%d)\n",  (void*) address, size, val, ret);
    }    
    return ret;
}



/*======================================================================
 * Data  : jupiter_pci_ops
 * 
 * Desc  : This data structure specifies the operations provided by Jupiter PCI-E    
 *         bridge. so far, only configuration space read/write are provided.
 *======================================================================*/
struct pci_ops jupiter_pci_ops = 
{
	read_config,
	write_config,
};


