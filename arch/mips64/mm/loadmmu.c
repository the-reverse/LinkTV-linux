/* $Id: loadmmu.c,v 1.3 1999/12/04 03:59:00 ralf Exp $
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 * Copyright (C) 1997, 1999 Ralf Baechle (ralf@gnu.org)
 * Copyright (C) 1999 Silicon Graphics, Inc.
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/bootinfo.h>
#include <asm/sgialib.h>

/* memory functions */
void (*clear_page)(unsigned long page);
void (*copy_page)(unsigned long to, unsigned long from);

/* Cache operations. */
void (*flush_cache_all)(void);
void (*flush_cache_mm)(struct mm_struct *mm);
void (*flush_cache_range)(struct mm_struct *mm, unsigned long start,
			  unsigned long end);
void (*flush_cache_page)(struct vm_area_struct *vma, unsigned long page);
void (*flush_cache_sigtramp)(unsigned long addr);
void (*flush_page_to_ram)(unsigned long page);

/* DMA cache operations. */
void (*dma_cache_wback_inv)(unsigned long start, unsigned long size);
void (*dma_cache_wback)(unsigned long start, unsigned long size);
void (*dma_cache_inv)(unsigned long start, unsigned long size);

/* TLB operations. */
void (*flush_tlb_all)(void);
void (*flush_tlb_mm)(struct mm_struct *mm);
void (*flush_tlb_range)(struct mm_struct *mm, unsigned long start,
			unsigned long end);
void (*flush_tlb_page)(struct vm_area_struct *vma, unsigned long page);

/* Miscellaneous. */
void (*update_mmu_cache)(struct vm_area_struct * vma,
			 unsigned long address, pte_t pte);

void (*show_regs)(struct pt_regs *);

int (*user_mode)(struct pt_regs *);

extern void ld_mmu_r4xx0(void);
extern void ld_mmu_tfp(void);
extern void ld_mmu_andes(void);

void __init load_mmu(void)
{
	switch(mips_cputype) {
#if defined (CONFIG_CPU_R4300)						\
    || defined (CONFIG_CPU_R4X00)					\
    || defined (CONFIG_CPU_R5000)					\
    || defined (CONFIG_CPU_NEVADA)
	case CPU_R4000PC:
	case CPU_R4000SC:
	case CPU_R4000MC:
	case CPU_R4200:
	case CPU_R4300:
	case CPU_R4400PC:
	case CPU_R4400SC:
	case CPU_R4400MC:
	case CPU_R4600:
	case CPU_R4640:
	case CPU_R4650:
	case CPU_R4700:
	case CPU_R5000:
	case CPU_R5000A:
	case CPU_NEVADA:
		printk("Loading R4000 MMU routines.\n");
		ld_mmu_r4xx0();
		break;
#endif

#if defined (CONFIG_CPU_R8000)
	case CPU_R8000:
		printk("Loading TFP MMU routines.\n");
		ld_mmu_tfp();
		break;
#endif

#if defined (CONFIG_CPU_R10000)
	case CPU_R10000:
		printk("Loading R10000 MMU routines.\n");
		ld_mmu_andes();
		break;
#endif

	default:
		/* XXX We need an generic routine in the MIPS port
		 * XXX to jabber stuff onto the screen on all machines
		 * XXX before the console is setup.  The ARCS prom
		 * XXX routines look good for this, but only the SGI
		 * XXX code has a full library for that at this time.
		 */
		panic("Yeee, unsupported mmu/cache architecture or "
		      "wrong compiletime kernel configuration.");
	}
}
