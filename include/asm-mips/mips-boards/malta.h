/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 2000 MIPS Technologies, Inc.  All rights reserved.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * Defines of the Malta board specific address-MAP, registers, etc.
 */
#ifndef __ASM_MIPS_BOARDS_MALTA_H
#define __ASM_MIPS_BOARDS_MALTA_H

#include <asm/addrspace.h>
#include <asm/io.h>
#include <asm/mips-boards/msc01_pci.h>
#include <asm/gt64120/gt64120.h>

/*
 * Malta I/O ports base address for the Galileo GT64120 and Algorithmics
 * Bonito system controllers.
 */
#define MALTA_GT_PORT_BASE      get_gt_port_base(GT_PCI0IOLD_OFS)
#define MALTA_BONITO_PORT_BASE  ((unsigned long)ioremap (0x1fd00000, 0x10000))
#define MALTA_MSC_PORT_BASE     get_msc_port_base(MSC01_PCI_SC2PIOBASL)

static inline unsigned long get_gt_port_base(unsigned long reg)
{
	unsigned long addr;
	addr = GT_READ(reg);
	return (unsigned long) ioremap (((addr & 0xffff) << 21), 0x10000);
}

static inline unsigned long get_msc_port_base(unsigned long reg)
{
	unsigned long addr;
	MSC_READ(reg, addr);
	return (unsigned long) ioremap(addr, 0x10000);
}

/*
 * Malta RTC-device indirect register access.
 */
#define MALTA_RTC_ADR_REG       0x70
#define MALTA_RTC_DAT_REG       0x71

/*
 * Malta SMSC FDC37M817 Super I/O Controller register.
 */
#define SMSC_CONFIG_REG		0x3f0
#define SMSC_DATA_REG		0x3f1

#define SMSC_CONFIG_DEVNUM	0x7
#define SMSC_CONFIG_ACTIVATE	0x30
#define SMSC_CONFIG_ENTER	0x55
#define SMSC_CONFIG_EXIT	0xaa

#define SMSC_CONFIG_DEVNUM_FLOPPY     0

#define SMSC_CONFIG_ACTIVATE_ENABLE   1

#define SMSC_WRITE(x,a)     outb(x,a)

#define MALTA_JMPRS_REG		0x1f000210

#endif /* __ASM_MIPS_BOARDS_MALTA_H */
