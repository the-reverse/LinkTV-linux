#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <platform.h>
#include "pci-jupiter.h"


/*-------------------------------------------------------
 FIX@ME : Please write your fixup over here

 fixup function declaration 

    DECLARE_PCI_FIXUP_EARLY (PID, VID, early_fixup)              
    DECLARE_PCI_FIXUP_HEADER(PID, VID, header_fixup)        
    DECLARE_PCI_FIXUP_FINAL (PID, VID, final_fixup)                
    DECLARE_PCI_FIXUP_ENABLE(PID, VID, enable_fixup)
   
 fixup function prototype
 
    void __init fixup_func(struct pci_dev *pdev)     
 -------------------------------------------------------*/
	

static 
irqreturn_t jupiter_pcie_check_isr(int irq, void* dev, struct pt_regs* regs)
{
    u16 command;     		

	if (GET_PCIE_GNR_INT() || GET_PCIE_INT())
    {                        
        pci_read_config_word((struct pci_dev*)dev, PCI_COMMAND, &command);        
        pci_write_config_word((struct pci_dev*)dev, PCI_COMMAND, command | PCI_COMMAND_INTX_DISABLE);                             	                    
        //PCI_WARNING("test PCI-E interrupt failed - unexcepted PCI-E interrupts detected, Please check your PCI-E Card first....\n");                                       
        return IRQ_HANDLED;
    }

	return IRQ_NONE;
}


void jupiter_fixup_early(struct pci_dev *dev)
{       
    u16 command;     	
	
	request_irq(2, jupiter_pcie_check_isr, SA_SHIRQ, "jupiter pci-e interrupt handler", dev);  // test driver...        
	
    pci_read_config_word(dev, PCI_COMMAND, &command);
    pci_write_config_word(dev, PCI_COMMAND, (command & ~PCI_COMMAND_INTX_DISABLE));	      	    
    
    msleep(100);        
    free_irq(2, dev);            
          
    pci_read_config_word(dev, PCI_COMMAND, &command);
    
    if (command & PCI_COMMAND_INTX_DISABLE)
    {
        PCI_WARNING("This card's interrupt has beed disabled for some reason, you may need to check your card first\n");                                              
    }   	
}

DECLARE_PCI_FIXUP_EARLY (PCI_ANY_ID, PCI_ANY_ID, jupiter_fixup_early);

