/*
 *  pci-jupiter.h, Include file for PCI-Express Control Unit of the Realtek Jupiter series.
 *
 *  Copyright (C) 2009  Realtek Semiconductor Corp.
 *    Author: Kevin Wang
 *  Copyright (C)  Kevin Wang <kevin_wang@realtek.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __PCI_JUPITER_H__
#define __PCI_JUPITER_H__

#include "pci-jupiter-reg.h"

/*---------------------------------------------------------------------------------
  Debug message
 ---------------------------------------------------------------------------------*/
//#define PCI_DBG_EN
//#define PCI_REG_DBG_EN 
//#define PCI_CFG_DBG_EN 

#define PCI_PRINT                   printk
 
#ifdef  PCI_DBG_EN
#define PCI_DBG(fmt, args...)       printk("[PCI] DBG, " fmt, ## args)
#else
#define PCI_DBG(fmt, args...)       do {} while(0)
#endif

#define PCI_INFO(fmt, args...)      printk("[PCI] INFO," fmt, ## args)
#define PCI_WARNING(fmt, args...)   printk("[PCI] WARNING," fmt, ## args)

#ifdef  PCI_REG_DBG_EN
#define PCI_REG_DBG(fmt, args...)   printk("[PCI] REG DBG," fmt, ## args)
#else
#define PCI_REG_DBG(fmt, args...)   do {} while(0)
#endif


#ifdef  PCI_CFG_DBG_EN
#define PCI_CFG_DBG(fmt, args...)   printk("[PCI] CFG DBG," fmt, ## args)
#else
#define PCI_CFG_DBG(fmt, args...)   do {} while(0)
#endif



/*---------------------------------------------------------------------------------
  Read / Write Command
 ---------------------------------------------------------------------------------*/
static unsigned int wr_reg(unsigned long addr, unsigned int val)
{
    PCI_REG_DBG("wr %08x %08x\n", addr, val);        
    writel(val, (volatile unsigned int*) addr);
    //udelay(1000);
    return 0;
}

static unsigned int rd_reg(unsigned long addr)
{
    unsigned int val = readl((volatile unsigned int*) addr);
    PCI_REG_DBG("rd %08x = %08x\n", addr, val);        
    //udelay(1000);
    return val;
}    


/*---------------------------------------------------------------------------------
  Misc
 ---------------------------------------------------------------------------------*/  

#define GET_SB2_PCI_BASE0()             rd_reg(SB2_PCI_BASE0)
#define GET_SB2_PCI_BASE1()             rd_reg(SB2_PCI_BASE1)
#define GET_SB2_PCI_BASE2()             rd_reg(SB2_PCI_BASE2)
#define GET_SB2_PCI_BASE3()             rd_reg(SB2_PCI_BASE3)
#define GET_SB2_PCI_TRAN0()             rd_reg(SB2_PCI_TRAN0)
#define GET_SB2_PCI_TRAN1()             rd_reg(SB2_PCI_TRAN1)   
#define GET_SB2_PCI_TRAN2()             rd_reg(SB2_PCI_TRAN2) 
#define GET_SB2_PCI_TRAN3()             rd_reg(SB2_PCI_TRAN3)   
#define GET_SB2_PCI_MASK0()             rd_reg(SB2_PCI_MASK0)   
#define GET_SB2_PCI_MASK1()             rd_reg(SB2_PCI_MASK1)   
#define GET_SB2_PCI_MASK2()             rd_reg(SB2_PCI_MASK2)   
#define GET_SB2_PCI_MASK3()             rd_reg(SB2_PCI_MASK3)   
#define GET_SB2_PCI_CTRL()              rd_reg(SB2_PCI_CTRL)   
#define GET_CFG(addr)                   rd_reg(CFG(addr))   
#define GET_PCIE_MAC_ST()               rd_reg(PCIE_MAC_ST)   
#define GET_PCIE_SYS_CTR()              rd_reg(PCIE_SYS_CTR)
#define GET_PCIE_GNR_INT()              rd_reg(DVR_GNR_INT)
#define GET_PCIE_INT()                  rd_reg(DVR_PCIE_INT)
#define GET_PCIE_MDIO_CTR(x)            rd_reg(PCIE_MDIO_CTR)   

#define SET_SB2_PCI_BASE0(x)            wr_reg(SB2_PCI_BASE0, x)
#define SET_SB2_PCI_BASE1(x)            wr_reg(SB2_PCI_BASE1, x)
#define SET_SB2_PCI_BASE2(x)            wr_reg(SB2_PCI_BASE2, x)
#define SET_SB2_PCI_BASE3(x)            wr_reg(SB2_PCI_BASE3, x)
#define SET_SB2_PCI_TRAN0(x)            wr_reg(SB2_PCI_TRAN0, x)
#define SET_SB2_PCI_TRAN1(x)            wr_reg(SB2_PCI_TRAN1, x)   
#define SET_SB2_PCI_TRAN2(x)            wr_reg(SB2_PCI_TRAN2, x) 
#define SET_SB2_PCI_TRAN3(x)            wr_reg(SB2_PCI_TRAN3, x)   
#define SET_SB2_PCI_MASK0(x)            wr_reg(SB2_PCI_MASK0, x)   
#define SET_SB2_PCI_MASK1(x)            wr_reg(SB2_PCI_MASK1, x)   
#define SET_SB2_PCI_MASK2(x)            wr_reg(SB2_PCI_MASK2, x)   
#define SET_SB2_PCI_MASK3(x)            wr_reg(SB2_PCI_MASK3, x)   
#define SET_SB2_PCI_CTRL(x)             wr_reg(SB2_PCI_CTRL,  x)   
#define SET_CFG(addr, x)                wr_reg(CFG(addr),  x)   
#define SET_PCIE_DBG(x)                 wr_reg(PCIE_DBG, x)
#define SET_PCIE_SYS_CTR(x)             wr_reg(PCIE_SYS_CTR, x)
#define SET_PCIE_MAC_ST(x)              wr_reg(PCIE_MAC_ST, x)   
#define SET_PCIE_MDIO_CTR(x)            wr_reg(PCIE_MDIO_CTR, x)   


#define GET_PCIE_INDIR_CTR()            rd_reg(PCIE_INDIR_CTR)
#define GET_PCIE_CFG_CT()               rd_reg(PCIE_CFG_CT)   
#define GET_PCIE_CFG_EN()               rd_reg(PCIE_CFG_EN) 
#define GET_PCIE_CFG_ST()               rd_reg(PCIE_CFG_ST)   
#define GET_PCIE_CFG_ADDR()             rd_reg(PCIE_CFG_ADDR)
#define GET_PCIE_CFG_WDATA()            rd_reg(PCIE_CFG_WDATA)
#define GET_PCIE_CFG_RDATA()            rd_reg(PCIE_CFG_RDATA)

#define SET_PCIE_INDIR_CTR(x)           wr_reg(PCIE_INDIR_CTR, x)
#define SET_PCIE_CFG_CT(x)              wr_reg(PCIE_CFG_CT, x)   
#define SET_PCIE_CFG_EN(x)              wr_reg(PCIE_CFG_EN, x) 
#define SET_PCIE_CFG_ST(x)              wr_reg(PCIE_CFG_ST, x)   
#define SET_PCIE_CFG_ADDR(x)            wr_reg(PCIE_CFG_ADDR, x)
#define SET_PCIE_CFG_WDATA(x)           wr_reg(PCIE_CFG_WDATA, x)
#define SET_PCIE_CFG_RDATA(x)           wr_reg(PCIE_CFG_RDATA, x)

 
/*---------------------------------------------------------------------------------
  Misc
 ---------------------------------------------------------------------------------*/ 

/*
 * Handle errors from the bridge.  This includes master and target aborts,
 * various command and address errors, and the interrupt test.  This gets
 * registered on the bridge error irq.  It's conceivable that some of these
 * conditions warrant a panic.  Anybody care to say which ones?
 */
typedef struct {
	unsigned int err_reg;
	unsigned int err_cnt;
	char         err_msg[64];
} master_abort_msg;




#endif  // __PCI_JUPITER_H__
