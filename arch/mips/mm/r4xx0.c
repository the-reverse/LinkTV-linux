/*
 * r4xx0.c: R4000 processor variant specific MMU/Cache routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 *
 * $Id: r4xx0.c,v 1.9 1997/12/01 17:57:36 ralf Exp $
 *
 * To do:
 *
 *  - this code is a overbloated pig
 *  - many of the bug workarounds are not efficient at all, but at
 *    least they are functional ...
 */
#include <linux/config.h>

#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/autoconf.h>

#include <asm/sgi.h>
#include <asm/sgimc.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/bootinfo.h>
#include <asm/sgialib.h>
#include <asm/mmu_context.h>

/* CP0 hazard avoidance. */
#define BARRIER __asm__ __volatile__(".set noreorder\n\t" \
				     "nop; nop; nop; nop; nop; nop;\n\t" \
				     ".set reorder\n\t")

/* Primary cache parameters. */
static int icache_size, dcache_size; /* Size in bytes */
static int ic_lsize, dc_lsize;       /* LineSize in bytes */

/* Secondary cache (if present) parameters. */
static scache_size, sc_lsize;        /* Again, in bytes */

#include <asm/cacheops.h>
#include <asm/r4kcache.h>

#undef DEBUG_CACHE

/*
 * On processors with QED R4600 style two set assosicative cache
 * this is the bit which selects the way in the cache for the
 * indexed cachops.
 */
#define icache_waybit (icache_size >> 1)
#define dcache_waybit (dcache_size >> 1)

/*
 * Zero an entire page.  We have three flavours of the routine available.
 * One for CPU with 16byte, with 32byte cachelines plus a special version
 * with nops which handles the buggy R4600 v1.x.
 */

static void r4k_clear_page_d16(unsigned long page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tcache\t%3,(%0)\n\t"
		"sd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"cache\t%3,16(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"cache\t%3,-32(%0)\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"cache\t%3,-16(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D)
		:"$1","memory");
}

static void r4k_clear_page_d32(unsigned long page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tcache\t%3,(%0)\n\t"
		"sd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"cache\t%3,-32(%0)\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D)
		:"$1","memory");
}


/*
 * This flavour of r4k_clear_page is for the R4600 V1.x.  Cite from the
 * IDT R4600 V1.7 errata:
 *
 *  18. The CACHE instructions Hit_Writeback_Invalidate_D, Hit_Writeback_D,
 *      Hit_Invalidate_D and Create_Dirty_Excl_D should only be
 *      executed if there is no other dcache activity. If the dcache is
 *      accessed for another instruction immeidately preceding when these
 *      cache instructions are executing, it is possible that the dcache 
 *      tag match outputs used by these cache instructions will be 
 *      incorrect. These cache instructions should be preceded by at least
 *      four instructions that are not any kind of load or store 
 *      instruction.
 *
 *      This is not allowed:    lw
 *                              nop
 *                              nop
 *                              nop
 *                              cache       Hit_Writeback_Invalidate_D
 *
 *      This is allowed:        lw
 *                              nop
 *                              nop
 *                              nop
 *                              nop
 *                              cache       Hit_Writeback_Invalidate_D
 */
static void r4k_clear_page_r4600_v1(unsigned long page)
{
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tnop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"cache\t%3,(%0)\n\t"
		"sd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"cache\t%3,-32(%0)\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D)
		:"$1","memory");
}

/*
 * And this one is for the R4600 V2.0
 */
static void r4k_clear_page_r4600_v2(unsigned long page)
{
	unsigned int flags;

	save_and_cli(flags);
	*(volatile unsigned int *)KSEG1;
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%2\n"
		"1:\tcache\t%3,(%0)\n\t"
		"sd\t$0,(%0)\n\t"
		"sd\t$0,8(%0)\n\t"
		"sd\t$0,16(%0)\n\t"
		"sd\t$0,24(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"cache\t%3,-32(%0)\n\t"
		"sd\t$0,-32(%0)\n\t"
		"sd\t$0,-24(%0)\n\t"
		"sd\t$0,-16(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sd\t$0,-8(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (page)
		:"0" (page),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D)
		:"$1","memory");
	restore_flags(flags);
}


/*
 * This is still inefficient.  We only can do better if we know the
 * virtual address where the copy will be accessed.
 */

static void r4k_copy_page_d16(unsigned long to, unsigned long from)
{
	unsigned long dummy1, dummy2;
	unsigned long reg1, reg2, reg3, reg4;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%8\n"
		"1:\tcache\t%9,(%0)\n\t"
		"lw\t%2,(%1)\n\t"
		"lw\t%3,4(%1)\n\t"
		"lw\t%4,8(%1)\n\t"
		"lw\t%5,12(%1)\n\t"
		"sw\t%2,(%0)\n\t"
		"sw\t%3,4(%0)\n\t"
		"sw\t%4,8(%0)\n\t"
		"sw\t%5,12(%0)\n\t"
		"cache\t%9,16(%0)\n\t"
		"lw\t%2,16(%1)\n\t"
		"lw\t%3,20(%1)\n\t"
		"lw\t%4,24(%1)\n\t"
		"lw\t%5,28(%1)\n\t"
		"sw\t%2,16(%0)\n\t"
		"sw\t%3,20(%0)\n\t"
		"sw\t%4,24(%0)\n\t"
		"sw\t%5,28(%0)\n\t"
		"cache\t%9,32(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"daddiu\t%1,64\n\t"
		"lw\t%2,-32(%1)\n\t"
		"lw\t%3,-28(%1)\n\t"
		"lw\t%4,-24(%1)\n\t"
		"lw\t%5,-20(%1)\n\t"
		"sw\t%2,-32(%0)\n\t"
		"sw\t%3,-28(%0)\n\t"
		"sw\t%4,-24(%0)\n\t"
		"sw\t%5,-20(%0)\n\t"
		"cache\t%9,-16(%0)\n\t"
		"lw\t%2,-16(%1)\n\t"
		"lw\t%3,-12(%1)\n\t"
		"lw\t%4,-8(%1)\n\t"
		"lw\t%5,-4(%1)\n\t"
		"sw\t%2,-16(%0)\n\t"
		"sw\t%3,-12(%0)\n\t"
		"sw\t%4,-8(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sw\t%5,-4(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1), "=r" (dummy2),
		 "=&r" (reg1), "=&r" (reg2), "=&r" (reg3), "=&r" (reg4)
		:"0" (to), "1" (from),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D));
}

static void r4k_copy_page_d32(unsigned long to, unsigned long from)
{
	unsigned long dummy1, dummy2;
	unsigned long reg1, reg2, reg3, reg4;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%8\n"
		"1:\tcache\t%9,(%0)\n\t"
		"lw\t%2,(%1)\n\t"
		"lw\t%3,4(%1)\n\t"
		"lw\t%4,8(%1)\n\t"
		"lw\t%5,12(%1)\n\t"
		"sw\t%2,(%0)\n\t"
		"sw\t%3,4(%0)\n\t"
		"sw\t%4,8(%0)\n\t"
		"sw\t%5,12(%0)\n\t"
		"lw\t%2,16(%1)\n\t"
		"lw\t%3,20(%1)\n\t"
		"lw\t%4,24(%1)\n\t"
		"lw\t%5,28(%1)\n\t"
		"sw\t%2,16(%0)\n\t"
		"sw\t%3,20(%0)\n\t"
		"sw\t%4,24(%0)\n\t"
		"sw\t%5,28(%0)\n\t"
		"cache\t%9,32(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"daddiu\t%1,64\n\t"
		"lw\t%2,-32(%1)\n\t"
		"lw\t%3,-28(%1)\n\t"
		"lw\t%4,-24(%1)\n\t"
		"lw\t%5,-20(%1)\n\t"
		"sw\t%2,-32(%0)\n\t"
		"sw\t%3,-28(%0)\n\t"
		"sw\t%4,-24(%0)\n\t"
		"sw\t%5,-20(%0)\n\t"
		"lw\t%2,-16(%1)\n\t"
		"lw\t%3,-12(%1)\n\t"
		"lw\t%4,-8(%1)\n\t"
		"lw\t%5,-4(%1)\n\t"
		"sw\t%2,-16(%0)\n\t"
		"sw\t%3,-12(%0)\n\t"
		"sw\t%4,-8(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sw\t%5,-4(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1), "=r" (dummy2),
		 "=&r" (reg1), "=&r" (reg2), "=&r" (reg3), "=&r" (reg4)
		:"0" (to), "1" (from),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D));
}

/*
 * Again a special version for the R4600 V1.x
 */
static void r4k_copy_page_r4600_v1(unsigned long to, unsigned long from)
{
	unsigned long dummy1, dummy2;
	unsigned long reg1, reg2, reg3, reg4;

	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%8\n"
		"1:\tnop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"\tcache\t%9,(%0)\n\t"
		"lw\t%2,(%1)\n\t"
		"lw\t%3,4(%1)\n\t"
		"lw\t%4,8(%1)\n\t"
		"lw\t%5,12(%1)\n\t"
		"sw\t%2,(%0)\n\t"
		"sw\t%3,4(%0)\n\t"
		"sw\t%4,8(%0)\n\t"
		"sw\t%5,12(%0)\n\t"
		"lw\t%2,16(%1)\n\t"
		"lw\t%3,20(%1)\n\t"
		"lw\t%4,24(%1)\n\t"
		"lw\t%5,28(%1)\n\t"
		"sw\t%2,16(%0)\n\t"
		"sw\t%3,20(%0)\n\t"
		"sw\t%4,24(%0)\n\t"
		"sw\t%5,28(%0)\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"cache\t%9,32(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"daddiu\t%1,64\n\t"
		"lw\t%2,-32(%1)\n\t"
		"lw\t%3,-28(%1)\n\t"
		"lw\t%4,-24(%1)\n\t"
		"lw\t%5,-20(%1)\n\t"
		"sw\t%2,-32(%0)\n\t"
		"sw\t%3,-28(%0)\n\t"
		"sw\t%4,-24(%0)\n\t"
		"sw\t%5,-20(%0)\n\t"
		"lw\t%2,-16(%1)\n\t"
		"lw\t%3,-12(%1)\n\t"
		"lw\t%4,-8(%1)\n\t"
		"lw\t%5,-4(%1)\n\t"
		"sw\t%2,-16(%0)\n\t"
		"sw\t%3,-12(%0)\n\t"
		"sw\t%4,-8(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sw\t%5,-4(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1), "=r" (dummy2),
		 "=&r" (reg1), "=&r" (reg2), "=&r" (reg3), "=&r" (reg4)
		:"0" (to), "1" (from),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D));
}

static void r4k_copy_page_r4600_v2(unsigned long to, unsigned long from)
{
	unsigned long dummy1, dummy2;
	unsigned long reg1, reg2, reg3, reg4;
	unsigned int flags;

	__save_and_cli(flags);
	__asm__ __volatile__(
		".set\tnoreorder\n\t"
		".set\tnoat\n\t"
		".set\tmips3\n\t"
		"daddiu\t$1,%0,%8\n"
		"1:\tnop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"\tcache\t%9,(%0)\n\t"
		"lw\t%2,(%1)\n\t"
		"lw\t%3,4(%1)\n\t"
		"lw\t%4,8(%1)\n\t"
		"lw\t%5,12(%1)\n\t"
		"sw\t%2,(%0)\n\t"
		"sw\t%3,4(%0)\n\t"
		"sw\t%4,8(%0)\n\t"
		"sw\t%5,12(%0)\n\t"
		"lw\t%2,16(%1)\n\t"
		"lw\t%3,20(%1)\n\t"
		"lw\t%4,24(%1)\n\t"
		"lw\t%5,28(%1)\n\t"
		"sw\t%2,16(%0)\n\t"
		"sw\t%3,20(%0)\n\t"
		"sw\t%4,24(%0)\n\t"
		"sw\t%5,28(%0)\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"cache\t%9,32(%0)\n\t"
		"daddiu\t%0,64\n\t"
		"daddiu\t%1,64\n\t"
		"lw\t%2,-32(%1)\n\t"
		"lw\t%3,-28(%1)\n\t"
		"lw\t%4,-24(%1)\n\t"
		"lw\t%5,-20(%1)\n\t"
		"sw\t%2,-32(%0)\n\t"
		"sw\t%3,-28(%0)\n\t"
		"sw\t%4,-24(%0)\n\t"
		"sw\t%5,-20(%0)\n\t"
		"lw\t%2,-16(%1)\n\t"
		"lw\t%3,-12(%1)\n\t"
		"lw\t%4,-8(%1)\n\t"
		"lw\t%5,-4(%1)\n\t"
		"sw\t%2,-16(%0)\n\t"
		"sw\t%3,-12(%0)\n\t"
		"sw\t%4,-8(%0)\n\t"
		"bne\t$1,%0,1b\n\t"
		"sw\t%5,-4(%0)\n\t"
		".set\tmips0\n\t"
		".set\tat\n\t"
		".set\treorder"
		:"=r" (dummy1), "=r" (dummy2),
		 "=&r" (reg1), "=&r" (reg2), "=&r" (reg3), "=&r" (reg4)
		:"0" (to), "1" (from),
		 "I" (PAGE_SIZE),
		 "i" (Create_Dirty_Excl_D));
	restore_flags(flags);
}

/*
 * If you think for one second that this stuff coming up is a lot
 * of bulky code eating too many kernel cache lines.  Think _again_.
 *
 * Consider:
 * 1) Taken branches have a 3 cycle penalty on R4k
 * 2) The branch itself is a real dead cycle on even R4600/R5000.
 * 3) Only one of the following variants of each type is even used by
 *    the kernel based upon the cache parameters we detect at boot time.
 *
 * QED.
 */

static inline void r4k_flush_cache_all_s16d16i16(void)
{
	unsigned long flags;

	save_and_cli(flags);
	blast_dcache16(); blast_icache16(); blast_scache16();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s32d16i16(void)
{
	unsigned long flags;

	save_and_cli(flags);
	blast_dcache16(); blast_icache16(); blast_scache32();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s64d16i16(void)
{
	unsigned long flags;

	save_and_cli(flags);
	blast_dcache16(); blast_icache16(); blast_scache64();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s128d16i16(void)
{
	unsigned long flags;

	save_and_cli(flags);
	blast_dcache16(); blast_icache16(); blast_scache128();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s16d32i32(void)
{
	unsigned long flags;

	save_and_cli(flags);
	blast_dcache32(); blast_icache32(); blast_scache16();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s32d32i32(void)
{
	unsigned long flags;

	save_and_cli(flags);
	blast_dcache32(); blast_icache32(); blast_scache32();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s64d32i32(void)
{
	unsigned long flags;

	save_and_cli(flags);
	blast_dcache32(); blast_icache32(); blast_scache64();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_s128d32i32(void)
{
	unsigned long flags;

	save_and_cli(flags);
	blast_dcache32(); blast_icache32(); blast_scache128();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_d16i16(void)
{
	unsigned long flags;

	save_and_cli(flags);
	blast_dcache16(); blast_icache16();
	restore_flags(flags);
}

static inline void r4k_flush_cache_all_d32i32(void)
{
	unsigned long flags;

	save_and_cli(flags);
	blast_dcache32(); blast_icache32();
	restore_flags(flags);
}

static void
r4k_flush_cache_range_s16d16i16(struct mm_struct *mm,
                                unsigned long start,
                                unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s16d16i16();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_and_cli(flags);
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache16_page(start);
					if(text)
						blast_icache16_page(start);
					blast_scache16_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void
r4k_flush_cache_range_s32d16i16(struct mm_struct *mm,
                                unsigned long start,
                                unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s32d16i16();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_and_cli(flags);
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache16_page(start);
					if(text)
						blast_icache16_page(start);
					blast_scache32_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s64d16i16(struct mm_struct *mm,
					    unsigned long start,
					    unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s64d16i16();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_and_cli(flags);
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache16_page(start);
					if(text)
						blast_icache16_page(start);
					blast_scache64_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s128d16i16(struct mm_struct *mm,
					     unsigned long start,
					     unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s128d16i16();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_and_cli(flags);
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache16_page(start);
					if(text)
						blast_icache16_page(start);
					blast_scache128_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s16d32i32(struct mm_struct *mm,
					    unsigned long start,
					    unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s16d32i32();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_and_cli(flags);
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache32_page(start);
					if(text)
						blast_icache32_page(start);
					blast_scache16_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s32d32i32(struct mm_struct *mm,
					    unsigned long start,
					    unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s32d32i32();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_and_cli(flags);
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache32_page(start);
					if(text)
						blast_icache32_page(start);
					blast_scache32_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s64d32i32(struct mm_struct *mm,
					    unsigned long start,
					    unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s64d32i32();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_and_cli(flags);
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache32_page(start);
					if(text)
						blast_icache32_page(start);
					blast_scache64_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_s128d32i32(struct mm_struct *mm,
					     unsigned long start,
					     unsigned long end)
{
	struct vm_area_struct *vma;
	unsigned long flags;

	if(mm->context == 0)
		return;

	start &= PAGE_MASK;
#ifdef DEBUG_CACHE
	printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
	vma = find_vma(mm, start);
	if(vma) {
		if(mm->context != current->mm->context) {
			r4k_flush_cache_all_s128d32i32();
		} else {
			pgd_t *pgd;
			pmd_t *pmd;
			pte_t *pte;
			int text;

			save_and_cli(flags);
			text = vma->vm_flags & VM_EXEC;
			while(start < end) {
				pgd = pgd_offset(mm, start);
				pmd = pmd_offset(pgd, start);
				pte = pte_offset(pmd, start);

				if(pte_val(*pte) & _PAGE_VALID) {
					blast_dcache32_page(start);
					if(text)
						blast_icache32_page(start);
					blast_scache128_page(start);
				}
				start += PAGE_SIZE;
			}
			restore_flags(flags);
		}
	}
}

static void r4k_flush_cache_range_d16i16(struct mm_struct *mm,
					 unsigned long start,
					 unsigned long end)
{
	if(mm->context != 0) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
		save_and_cli(flags);
		blast_dcache16(); blast_icache16();
		restore_flags(flags);
	}
}

static void r4k_flush_cache_range_d32i32(struct mm_struct *mm,
					 unsigned long start,
					 unsigned long end)
{
	if(mm->context != 0) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("crange[%d,%08lx,%08lx]", (int)mm->context, start, end);
#endif
		save_and_cli(flags);
		blast_dcache32(); blast_icache32();
		restore_flags(flags);
	}
}

/*
 * On architectures like the Sparc, we could get rid of lines in
 * the cache created only by a certain context, but on the MIPS
 * (and actually certain Sparc's) we cannot.
 */
static void r4k_flush_cache_mm_s16d16i16(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s16d16i16();
	}
}

static void r4k_flush_cache_mm_s32d16i16(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s32d16i16();
	}
}

static void r4k_flush_cache_mm_s64d16i16(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s64d16i16();
	}
}

static void r4k_flush_cache_mm_s128d16i16(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s128d16i16();
	}
}

static void r4k_flush_cache_mm_s16d32i32(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s16d32i32();
	}
}

static void r4k_flush_cache_mm_s32d32i32(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s32d32i32();
	}
}

static void r4k_flush_cache_mm_s64d32i32(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s64d32i32();
	}
}

static void r4k_flush_cache_mm_s128d32i32(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_s128d32i32();
	}
}

static void r4k_flush_cache_mm_d16i16(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_d16i16();
	}
}

static void r4k_flush_cache_mm_d32i32(struct mm_struct *mm)
{
	if(mm->context != 0) {
#ifdef DEBUG_CACHE
		printk("cmm[%d]", (int)mm->context);
#endif
		r4k_flush_cache_all_d32i32();
	}
}

static void r4k_flush_cache_page_s16d16i16(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/* Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache16_page_indexed(page);
		if(text)
			blast_icache16_page_indexed(page);
		blast_scache16_page_indexed(page);
	} else {
		blast_dcache16_page(page);
		if(text)
			blast_icache16_page(page);
		blast_scache16_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s32d16i16(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/* If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/* Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache16_page_indexed(page);
		if(text)
			blast_icache16_page_indexed(page);
		blast_scache32_page_indexed(page);
	} else {
		blast_dcache16_page(page);
		if(text)
			blast_icache16_page(page);
		blast_scache32_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s64d16i16(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/* If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache16_page_indexed(page);
		if(text)
			blast_icache16_page_indexed(page);
		blast_scache64_page_indexed(page);
	} else {
		blast_dcache16_page(page);
		if(text)
			blast_icache16_page(page);
		blast_scache64_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s128d16i16(struct vm_area_struct *vma,
					    unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/* Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/*
		 * Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache16_page_indexed(page);
		if(text)
			blast_icache16_page_indexed(page);
		blast_scache128_page_indexed(page);
	} else {
		blast_dcache16_page(page);
		if(text)
			blast_icache16_page(page);
		blast_scache128_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s16d32i32(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/* If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache32_page_indexed(page);
		if(text)
			blast_icache32_page_indexed(page);
		blast_scache16_page_indexed(page);
	} else {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
		blast_scache16_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s32d32i32(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/*
		 * Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache32_page_indexed(page);
		if(text)
			blast_icache32_page_indexed(page);
		blast_scache32_page_indexed(page);
	} else {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
		blast_scache32_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s64d32i32(struct vm_area_struct *vma,
					   unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/*
		 * Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache32_page_indexed(page);
		if(text)
			blast_icache32_page_indexed(page);
		blast_scache64_page_indexed(page);
	} else {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
		blast_scache64_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_s128d32i32(struct vm_area_struct *vma,
					    unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/* If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm->context != current->mm->context) {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (scache_size - 1)));
		blast_dcache32_page_indexed(page);
		if(text)
			blast_icache32_page_indexed(page);
		blast_scache128_page_indexed(page);
	} else {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
		blast_scache128_page(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_d16i16(struct vm_area_struct *vma,
					unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/* If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_VALID))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if(mm == current->mm) {
		blast_dcache16_page(page);
		if(text)
			blast_icache16_page(page);
	} else {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (dcache_size - 1)));
		blast_dcache16_page_indexed(page);
		if(text)
			blast_icache16_page_indexed(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_d32i32(struct vm_area_struct *vma,
					unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_PRESENT))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if((mm == current->mm) && (pte_val(*ptep) & _PAGE_VALID)) {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
	} else {
		/*
		 * Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (dcache_size - 1)));
		blast_dcache32_page_indexed(page);
		if(text)
			blast_icache32_page_indexed(page);
	}
out:
	restore_flags(flags);
}

static void r4k_flush_cache_page_d32i32_r4600(struct vm_area_struct *vma,
					      unsigned long page)
{
	struct mm_struct *mm = vma->vm_mm;
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int text;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if(mm->context == 0)
		return;

#ifdef DEBUG_CACHE
	printk("cpage[%d,%08lx]", (int)mm->context, page);
#endif
	save_and_cli(flags);
	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if(!(pte_val(*ptep) & _PAGE_PRESENT))
		goto out;

	text = (vma->vm_flags & VM_EXEC);
	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if((mm == current->mm) && (pte_val(*ptep) & _PAGE_VALID)) {
		blast_dcache32_page(page);
		if(text)
			blast_icache32_page(page);
	} else {
		/* Do indexed flush, too much work to get the (possible)
		 * tlb refills to work correctly.
		 */
		page = (KSEG0 + (page & (dcache_size - 1)));
		blast_dcache32_page_indexed(page);
		blast_dcache32_page_indexed(page ^ dcache_waybit);
		if(text) {
			blast_icache32_page_indexed(page);
			blast_icache32_page_indexed(page ^ icache_waybit);
		}
	}
out:
	restore_flags(flags);
}

/* If the addresses passed to these routines are valid, they are
 * either:
 *
 * 1) In KSEG0, so we can do a direct flush of the page.
 * 2) In KSEG2, and since every process can translate those
 *    addresses all the time in kernel mode we can do a direct
 *    flush.
 * 3) In KSEG1, no flush necessary.
 */
static void r4k_flush_page_to_ram_s16d16i16(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache16_page(page);
		blast_scache16_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s32d16i16(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache16_page(page);
		blast_scache32_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s64d16i16(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache16_page(page);
		blast_scache64_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s128d16i16(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache16_page(page);
		blast_scache128_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s16d32i32(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache32_page(page);
		blast_scache16_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s32d32i32(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache32_page(page);
		blast_scache32_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s64d32i32(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache32_page(page);
		blast_scache64_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_s128d32i32(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache32_page(page);
		blast_scache128_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_d16i16(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache16_page(page);
		restore_flags(flags);
	}
}

static void r4k_flush_page_to_ram_d32i32(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache32_page(page);
		restore_flags(flags);
	}
}

/*
 * Writeback and invalidate the primary cache dcache before DMA.
 *
 * R4600 v2.0 bug: "The CACHE instructions Hit_Writeback_Inv_D,
 * Hit_Writeback_D, Hit_Invalidate_D and Create_Dirty_Exclusive_D will only
 * operate correctly if the internal data cache refill buffer is empty.  These
 * CACHE instructions should be separated from any potential data cache miss
 * by a load instruction to an uncached address to empty the response buffer."
 * (Revision 2.0 device errata from IDT available on http://www.idt.com/
 * in .pdf format.)
 */
static void
r4k_flush_cache_pre_dma_out_pc(unsigned long addr, unsigned long size)
{
	unsigned long end, a;
	unsigned int cmode, flags;

	cmode = read_32bit_cp0_register(CP0_CONFIG) & CONFIG_CM_CMASK;
	if (cmode == CONFIG_CM_CACHABLE_WA ||
	    cmode == CONFIG_CM_CACHABLE_NO_WA) {
		/* primary dcache is writethrough, therefore memory
		   is already consistent with the caches.  */
		return;
	}

	if (size >= dcache_size) {
		flush_cache_all();
		return;
	}

	/* Workaround for R4600 bug.  See comment above. */
	save_and_cli(flags);
	*(volatile unsigned long *)KSEG1;

	a = addr & ~(dc_lsize - 1);
	end = (addr + size) & ~(dc_lsize - 1);
	while (1) {
		flush_dcache_line(a); /* Hit_Writeback_Inv_D */
		if (a == end) break;
		a += dc_lsize;
	}
	restore_flags(flags);
}

static void
r4k_flush_cache_pre_dma_out_sc(unsigned long addr, unsigned long size)
{
	unsigned long end;
	unsigned long a;

	if (size >= scache_size) {
		flush_cache_all();
		return;
	}

	a = addr & ~(sc_lsize - 1);
	end = (addr + size) & ~(sc_lsize - 1);
	while (1) {
		flush_scache_line(addr); /* Hit_Writeback_Inv_SD */
		if (addr == end) break;
		addr += sc_lsize;
	}
	r4k_flush_cache_pre_dma_out_pc(addr, size);
}

static void
r4k_flush_cache_post_dma_in_pc(unsigned long addr, unsigned long size)
{
	unsigned long end;
	unsigned long a;

	if (size >= dcache_size) {
		flush_cache_all();
		return;
	}

	/* Workaround for R4600 bug.  See comment above. */
	*(volatile unsigned long *)KSEG1;

	a = addr & ~(dc_lsize - 1);
	end = (addr + size) & ~(dc_lsize - 1);
	while (1) {
		invalidate_dcache_line(a); /* Hit_Invalidate_D */
		if (a == end) break;
		a += dc_lsize;
	}
}

static void
r4k_flush_cache_post_dma_in_sc(unsigned long addr, unsigned long size)
{
	unsigned long end;
	unsigned long a;

	if (size >= scache_size) {
		flush_cache_all();
		return;
	}

	a = addr & ~(sc_lsize - 1);
	end = (addr + size) & ~(sc_lsize - 1);
	while (1) {
		invalidate_scache_line(addr); /* Hit_Invalidate_SD */
		if (addr == end) break;
		addr += sc_lsize;
	}
	r4k_flush_cache_pre_dma_out_pc(addr, size);
}

static void r4k_flush_page_to_ram_d32i32_r4600(unsigned long page)
{
	page &= PAGE_MASK;
	if((page >= KSEG0 && page < KSEG1) || (page >= KSEG2)) {
		unsigned long flags;

#ifdef DEBUG_CACHE
		printk("r4600_cram[%08lx]", page);
#endif
		save_and_cli(flags);
		blast_dcache32_page(page);
#ifdef CONFIG_SGI
		{
			unsigned long tmp1, tmp2;

			__asm__ __volatile__("
			.set noreorder
			.set mips3
			li	%0, 0x1
			dsll	%0, 31
			or	%0, %0, %2
			lui	%1, 0x9000
			dsll32	%1, 0
			or	%0, %0, %1
			daddu	%1, %0, 0x0fe0
			li	%2, 0x80
			mtc0	%2, $12
			nop; nop; nop; nop;
1:			sw	$0, 0(%0)
			bltu	%0, %1, 1b
			daddu	%0, 32
			mtc0	$0, $12
			nop; nop; nop; nop;
			.set mips0
			.set reorder"
			: "=&r" (tmp1), "=&r" (tmp2),
			  "=&r" (page)
			: "2" (page & 0x0007f000));
		}
#endif /* CONFIG_SGI */
		restore_flags(flags);
	}
}

/*
 * While we're protected against bad userland addresses we don't care
 * very much about what happens in that case.  Usually a segmentation
 * fault will dump the process later on anyway ...
 */
static void r4k_flush_cache_sigtramp(unsigned long addr)
{
	unsigned long daddr, iaddr;

	daddr = addr & ~(dc_lsize - 1);
	__asm__ __volatile__("nop;nop;nop;nop");	/* R4600 V1.7 */
	protected_writeback_dcache_line(daddr);
	protected_writeback_dcache_line(daddr + dc_lsize);
	iaddr = addr & ~(ic_lsize - 1);
	protected_flush_icache_line(iaddr);
	protected_flush_icache_line(iaddr + ic_lsize);
}

static void r4600v20k_flush_cache_sigtramp(unsigned long addr)
{
	unsigned long daddr, iaddr;
	unsigned int flags;

	daddr = addr & ~(dc_lsize - 1);
	save_and_cli(flags);

	/* Clear internal cache refill buffer */
	*(volatile unsigned int *)KSEG1;

	protected_writeback_dcache_line(daddr);
	protected_writeback_dcache_line(daddr + dc_lsize);
	iaddr = addr & ~(ic_lsize - 1);
	protected_flush_icache_line(iaddr);
	protected_flush_icache_line(iaddr + ic_lsize);
	restore_flags(flags);
}

#undef DEBUG_TLB
#undef DEBUG_TLBUPDATE

#define NTLB_ENTRIES       48  /* Fixed on all R4XX0 variants... */

#define NTLB_ENTRIES_HALF  24  /* Fixed on all R4XX0 variants... */

static inline void r4k_flush_tlb_all(void)
{
	unsigned long flags;
	unsigned long old_ctx;
	int entry;

#ifdef DEBUG_TLB
	printk("[tlball]");
#endif

	save_and_cli(flags);
	/* Save old context and create impossible VPN2 value */
	old_ctx = (get_entryhi() & 0xff);
	set_entryhi(KSEG0);
	set_entrylo0(0);
	set_entrylo1(0);
	BARRIER;

	entry = get_wired();

	/* Blast 'em all away. */
	while(entry < NTLB_ENTRIES) {
		set_index(entry);
		BARRIER;
		tlb_write_indexed();
		BARRIER;
		entry++;
	}
	BARRIER;
	set_entryhi(old_ctx);
	restore_flags(flags);
}

static void r4k_flush_tlb_mm(struct mm_struct *mm)
{
	if(mm->context != 0) {
		unsigned long flags;

#ifdef DEBUG_TLB
		printk("[tlbmm<%d>]", mm->context);
#endif
		save_and_cli(flags);
		get_new_mmu_context(mm, asid_cache);
		if(mm == current->mm)
			set_entryhi(mm->context & 0xff);
		restore_flags(flags);
	}
}

static void r4k_flush_tlb_range(struct mm_struct *mm, unsigned long start,
				unsigned long end)
{
	if(mm->context != 0) {
		unsigned long flags;
		int size;

#ifdef DEBUG_TLB
		printk("[tlbrange<%02x,%08lx,%08lx>]", (mm->context & 0xff),
		       start, end);
#endif
		save_and_cli(flags);
		size = (end - start + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
		size = (size + 1) >> 1;
		if(size <= NTLB_ENTRIES_HALF) {
			int oldpid = (get_entryhi() & 0xff);
			int newpid = (mm->context & 0xff);

			start &= (PAGE_MASK << 1);
			end += ((PAGE_SIZE << 1) - 1);
			end &= (PAGE_MASK << 1);
			while(start < end) {
				int idx;

				set_entryhi(start | newpid);
				start += (PAGE_SIZE << 1);
				BARRIER;
				tlb_probe();
				BARRIER;
				idx = get_index();
				set_entrylo0(0);
				set_entrylo1(0);
				set_entryhi(KSEG0);
				BARRIER;
				if(idx < 0)
					continue;
				tlb_write_indexed();
				BARRIER;
			}
			set_entryhi(oldpid);
		} else {
			get_new_mmu_context(mm, asid_cache);
			if(mm == current->mm)
				set_entryhi(mm->context & 0xff);
		}
		restore_flags(flags);
	}
}

static void r4k_flush_tlb_page(struct vm_area_struct *vma, unsigned long page)
{
	if(vma->vm_mm->context != 0) {
		unsigned long flags;
		int oldpid, newpid, idx;

#ifdef DEBUG_TLB
		printk("[tlbpage<%d,%08lx>]", vma->vm_mm->context, page);
#endif
		newpid = (vma->vm_mm->context & 0xff);
		page &= (PAGE_MASK << 1);
		save_and_cli(flags);
		oldpid = (get_entryhi() & 0xff);
		set_entryhi(page | newpid);
		BARRIER;
		tlb_probe();
		BARRIER;
		idx = get_index();
		set_entrylo0(0);
		set_entrylo1(0);
		set_entryhi(KSEG0);
		if(idx < 0)
			goto finish;
		BARRIER;
		tlb_write_indexed();

	finish:
		BARRIER;
		set_entryhi(oldpid);
		restore_flags(flags);
	}
}

/* Load a new root pointer into the TLB. */
static void r4k_load_pgd(unsigned long pg_dir)
{
}

static void r4k_pgd_init(unsigned long page)
{
	unsigned long *p = (unsigned long *) page;
	int i;

	for(i = 0; i < 1024; i+=8) {
		p[i + 0] = (unsigned long) invalid_pte_table;
		p[i + 1] = (unsigned long) invalid_pte_table;
		p[i + 2] = (unsigned long) invalid_pte_table;
		p[i + 3] = (unsigned long) invalid_pte_table;
		p[i + 4] = (unsigned long) invalid_pte_table;
		p[i + 5] = (unsigned long) invalid_pte_table;
		p[i + 6] = (unsigned long) invalid_pte_table;
		p[i + 7] = (unsigned long) invalid_pte_table;
	}
}

#ifdef DEBUG_TLBUPDATE
static unsigned long ehi_debug[NTLB_ENTRIES];
static unsigned long el0_debug[NTLB_ENTRIES];
static unsigned long el1_debug[NTLB_ENTRIES];
#endif

/* We will need multiple versions of update_mmu_cache(), one that just
 * updates the TLB with the new pte(s), and another which also checks
 * for the R4k "end of page" hardware bug and does the needy.
 */
static void r4k_update_mmu_cache(struct vm_area_struct * vma,
				 unsigned long address, pte_t pte)
{
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int idx, pid;

	pid = (get_entryhi() & 0xff);

#ifdef DEBUG_TLB
	if((pid != (vma->vm_mm->context & 0xff)) || (vma->vm_mm->context == 0)) {
		printk("update_mmu_cache: Wheee, bogus tlbpid mmpid=%d tlbpid=%d\n",
		       (int) (vma->vm_mm->context & 0xff), pid);
	}
#endif

	save_and_cli(flags);
	address &= (PAGE_MASK << 1);
	set_entryhi(address | (pid));
	pgdp = pgd_offset(vma->vm_mm, address);
	BARRIER;
	tlb_probe();
	BARRIER;
	pmdp = pmd_offset(pgdp, address);
	idx = get_index();
	ptep = pte_offset(pmdp, address);
	BARRIER;
	set_entrylo0(pte_val(*ptep++) >> 6);
	set_entrylo1(pte_val(*ptep) >> 6);
	set_entryhi(address | (pid));
	BARRIER;
	if(idx < 0) {
		tlb_write_random();
#if 0
		BARRIER;
		printk("[MISS]");
#endif
	} else {
		tlb_write_indexed();
#if 0
		BARRIER;
		printk("[HIT]");
#endif
	}
#if 0
	if(!strcmp(current->comm, "args")) {
		printk("<");
		for(idx = 0; idx < NTLB_ENTRIES; idx++) {
			BARRIER;
			set_index(idx);	BARRIER;
			tlb_read(); BARRIER;
			address = get_entryhi(); BARRIER;
			if((address & 0xff) != 0)
				printk("[%08lx]", address);
		}
		printk(">");
	}
	BARRIER;
#endif
	BARRIER;
	set_entryhi(pid);
	BARRIER;
	restore_flags(flags);
}

#if 0
static void r4k_update_mmu_cache_hwbug(struct vm_area_struct * vma,
				       unsigned long address, pte_t pte)
{
	unsigned long flags;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;
	int idx;

	save_and_cli(flags);
	address &= (PAGE_MASK << 1);
	set_entryhi(address | (get_entryhi() & 0xff));
	pgdp = pgd_offset(vma->vm_mm, address);
	tlb_probe();
	pmdp = pmd_offset(pgdp, address);
	idx = get_index();
	ptep = pte_offset(pmdp, address);
	set_entrylo0(pte_val(*ptep++) >> 6);
	set_entrylo1(pte_val(*ptep) >> 6);
	BARRIER;
	if(idx < 0)
		tlb_write_random();
	else
		tlb_write_indexed();
	BARRIER;
	restore_flags(flags);
}
#endif

static void r4k_show_regs(struct pt_regs * regs)
{
	/* Saved main processor registers. */
	printk("$0 : %08lx %08lx %08lx %08lx\n",
	       0UL, regs->regs[1], regs->regs[2], regs->regs[3]);
	printk("$4 : %08lx %08lx %08lx %08lx\n",
               regs->regs[4], regs->regs[5], regs->regs[6], regs->regs[7]);
	printk("$8 : %08lx %08lx %08lx %08lx\n",
	       regs->regs[8], regs->regs[9], regs->regs[10], regs->regs[11]);
	printk("$12: %08lx %08lx %08lx %08lx\n",
               regs->regs[12], regs->regs[13], regs->regs[14], regs->regs[15]);
	printk("$16: %08lx %08lx %08lx %08lx\n",
	       regs->regs[16], regs->regs[17], regs->regs[18], regs->regs[19]);
	printk("$20: %08lx %08lx %08lx %08lx\n",
               regs->regs[20], regs->regs[21], regs->regs[22], regs->regs[23]);
	printk("$24: %08lx %08lx\n",
	       regs->regs[24], regs->regs[25]);
	printk("$28: %08lx %08lx %08lx %08lx\n",
	       regs->regs[28], regs->regs[29], regs->regs[30], regs->regs[31]);

	/* Saved cp0 registers. */
	printk("epc   : %08lx\nStatus: %08lx\nCause : %08lx\n",
	       regs->cp0_epc, regs->cp0_status, regs->cp0_cause);
}
			
static void r4k_add_wired_entry(unsigned long entrylo0, unsigned long entrylo1,
				      unsigned long entryhi, unsigned long pagemask)
{
        unsigned long flags;
        unsigned long wired;
        unsigned long old_pagemask;
        unsigned long old_ctx;

        save_and_cli(flags);
        /* Save old context and create impossible VPN2 value */
        old_ctx = (get_entryhi() & 0xff);
        old_pagemask = get_pagemask();
        wired = get_wired();
        set_wired (wired + 1);
        set_index (wired);
        BARRIER;    
        set_pagemask (pagemask);
        set_entryhi(entryhi);
        set_entrylo0(entrylo0);
        set_entrylo1(entrylo1);
        BARRIER;    
        tlb_write_indexed();
        BARRIER;    
    
        set_entryhi(old_ctx);
        BARRIER;    
        set_pagemask (old_pagemask);
        flush_tlb_all();    
        restore_flags(flags);
}

/* Detect and size the various r4k caches. */
static void probe_icache(unsigned long config)
{
	unsigned long tmp;

	tmp = (config >> 9) & 7;
	icache_size = (1 << (12 + tmp));
	if((config >> 5) & 1)
		ic_lsize = 32;
	else
		ic_lsize = 16;

	printk("Primary ICACHE %dK (linesize %d bytes)\n",
	       (int)(icache_size >> 10), (int)ic_lsize);
}

static void probe_dcache(unsigned long config)
{
	unsigned long tmp;

	tmp = (config >> 6) & 7;
	dcache_size = (1 << (12 + tmp));
	if((config >> 4) & 1)
		dc_lsize = 32;
	else
		dc_lsize = 16;

	printk("Primary DCACHE %dK (linesize %d bytes)\n",
	       (int)(dcache_size >> 10), (int)dc_lsize);
}

static int probe_scache_eeprom(unsigned long config)
{
#ifdef CONFIG_SGI
	volatile unsigned int *cpu_control;
	unsigned short cmd = 0xc220;
	unsigned long data = 0;
	int i, n;

#ifdef __MIPSEB__
	cpu_control = (volatile unsigned int *) KSEG1ADDR(0x1fa00034);
#else
	cpu_control = (volatile unsigned int *) KSEG1ADDR(0x1fa00030);
#endif
#define DEASSERT(bit) (*(cpu_control) &= (~(bit)))
#define ASSERT(bit) (*(cpu_control) |= (bit))
#define DELAY  for(n = 0; n < 100000; n++) __asm__ __volatile__("")
	DEASSERT(SGIMC_EEPROM_PRE);
	DEASSERT(SGIMC_EEPROM_SDATAO);
	DEASSERT(SGIMC_EEPROM_SECLOCK);
	DEASSERT(SGIMC_EEPROM_PRE);
	DELAY;
	ASSERT(SGIMC_EEPROM_CSEL); ASSERT(SGIMC_EEPROM_SECLOCK);
	for(i = 0; i < 11; i++) {
		if(cmd & (1<<15))
			ASSERT(SGIMC_EEPROM_SDATAO);
		else
			DEASSERT(SGIMC_EEPROM_SDATAO);
		DEASSERT(SGIMC_EEPROM_SECLOCK);
		ASSERT(SGIMC_EEPROM_SECLOCK);
		cmd <<= 1;
	}
	DEASSERT(SGIMC_EEPROM_SDATAO);
	for(i = 0; i < (sizeof(unsigned short) * 8); i++) {
		unsigned int tmp;

		DEASSERT(SGIMC_EEPROM_SECLOCK);
		DELAY;
		ASSERT(SGIMC_EEPROM_SECLOCK);
		DELAY;
		data <<= 1;
		tmp = *cpu_control;
		if(tmp & SGIMC_EEPROM_SDATAI)
			data |= 1;
	}
	DEASSERT(SGIMC_EEPROM_SECLOCK);
	DEASSERT(SGIMC_EEPROM_CSEL);
	ASSERT(SGIMC_EEPROM_PRE);
	ASSERT(SGIMC_EEPROM_SECLOCK);
	data <<= PAGE_SHIFT;
	printk("R4600/R5000 SCACHE size %dK ", (int) (data >> 10));
	switch(mips_cputype) {
	case CPU_R4600:
	case CPU_R4640:
		sc_lsize = 32;
		break;

	default:
		sc_lsize = 128;
		break;
	}
	printk("linesize %d bytes\n", sc_lsize);
	scache_size = data;
	if(data) {
		unsigned long addr, tmp1, tmp2;

		/* Enable r4600/r5000 cache.  But flush it first. */
		for(addr = KSEG0; addr < (KSEG0 + dcache_size);
		    addr += dc_lsize)
			flush_dcache_line_indexed(addr);
		for(addr = KSEG0; addr < (KSEG0 + icache_size);
		    addr += ic_lsize)
			flush_icache_line_indexed(addr);
		for(addr = KSEG0; addr < (KSEG0 + scache_size);
		    addr += sc_lsize)
			flush_scache_line_indexed(addr);

		/* R5000 scache enable is in CP0 config, on R4600 variants
		 * the scache is enable by the memory mapped cache controller.
		 */
		if(mips_cputype == CPU_R5000) {
			unsigned long config;

			config = read_32bit_cp0_register(CP0_CONFIG);
			config |= 0x1000;
			write_32bit_cp0_register(CP0_CONFIG, config);
		} else {
			/* This is really cool... */
			printk("Enabling R4600 SCACHE\n");
			__asm__ __volatile__("
			.set noreorder
			.set mips3
			mfc0	%2, $12
			nop; nop; nop; nop;
			li	%1, 0x80
			mtc0	%1, $12
			nop; nop; nop; nop;
			li	%0, 0x1
			dsll	%0, 31
			lui	%1, 0x9000
			dsll32	%1, 0
			or	%0, %1, %0
			sb	$0, 0(%0)
			mtc0	$0, $12
			nop; nop; nop; nop;
			mtc0	%2, $12
			nop; nop; nop; nop;
			.set mips0
			.set reorder
		        " : "=r" (tmp1), "=r" (tmp2), "=r" (addr));
		}

		return 1;
	} else {
		if(mips_cputype == CPU_R5000)
			return -1;
		else
			return 0;
	}
#else
	/*
	 * XXX For now we don't panic and assume that existing chipset
	 * controlled caches are setup correnctly and are completly
	 * transparent.  Works fine for those MIPS machines I know.
	 * Morituri the salutant ...
	 */
	return 0;

	panic("Cannot probe SCACHE on this machine.");
#endif
}

/* If you even _breathe_ on this function, look at the gcc output
 * and make sure it does not pop things on and off the stack for
 * the cache sizing loop that executes in KSEG1 space or else
 * you will crash and burn badly.  You have been warned.
 */
static int probe_scache(unsigned long config)
{
	extern unsigned long stext;
	unsigned long flags, addr, begin, end, pow2;
	int tmp;

	tmp = ((config >> 17) & 1);
	if(tmp)
		return 0;
	tmp = ((config >> 22) & 3);
	switch(tmp) {
	case 0:
		sc_lsize = 16;
		break;
	case 1:
		sc_lsize = 32;
		break;
	case 2:
		sc_lsize = 64;
		break;
	case 3:
		sc_lsize = 128;
		break;
	}

	begin = (unsigned long) &stext;
	begin &= ~((4 * 1024 * 1024) - 1);
	end = begin + (4 * 1024 * 1024);

	/* This is such a bitch, you'd think they would make it
	 * easy to do this.  Away you daemons of stupidity!
	 */
	save_and_cli(flags);

	/* Fill each size-multiple cache line with a valid tag. */
	pow2 = (64 * 1024);
	for(addr = begin; addr < end; addr = (begin + pow2)) {
		unsigned long *p = (unsigned long *) addr;
		__asm__ __volatile__("nop" : : "r" (*p)); /* whee... */
		pow2 <<= 1;
	}

	/* Load first line with zero (therefore invalid) tag. */
	set_taglo(0);
	set_taghi(0);
	__asm__ __volatile__("nop; nop; nop; nop;"); /* avoid the hazard */
	__asm__ __volatile__("\n\t.set noreorder\n\t"
			     ".set mips3\n\t"
			     "cache 8, (%0)\n\t"
			     ".set mips0\n\t"
			     ".set reorder\n\t" : : "r" (begin));
	__asm__ __volatile__("\n\t.set noreorder\n\t"
			     ".set mips3\n\t"
			     "cache 9, (%0)\n\t"
			     ".set mips0\n\t"
			     ".set reorder\n\t" : : "r" (begin));
	__asm__ __volatile__("\n\t.set noreorder\n\t"
			     ".set mips3\n\t"
			     "cache 11, (%0)\n\t"
			     ".set mips0\n\t"
			     ".set reorder\n\t" : : "r" (begin));

	/* Now search for the wrap around point. */
	pow2 = (128 * 1024);
	tmp = 0;
	for(addr = (begin + (128 * 1024)); addr < (end); addr = (begin + pow2)) {
		__asm__ __volatile__("\n\t.set noreorder\n\t"
				     ".set mips3\n\t"
				     "cache 7, (%0)\n\t"
				     ".set mips0\n\t"
				     ".set reorder\n\t" : : "r" (addr));
		__asm__ __volatile__("nop; nop; nop; nop;"); /* hazard... */
		if(!get_taglo())
			break;
		pow2 <<= 1;
	}
	restore_flags(flags);
	addr -= begin;
	printk("Secondary cache sized at %dK linesize %d\n", (int) (addr >> 10),
	       sc_lsize);
	scache_size = addr;
	return 1;
}

static void setup_noscache_funcs(void)
{
	unsigned int prid;

	switch(dc_lsize) {
	case 16:
		clear_page = r4k_clear_page_d16;
		copy_page = r4k_copy_page_d16;
		flush_cache_all = r4k_flush_cache_all_d16i16;
		flush_cache_mm = r4k_flush_cache_mm_d16i16;
		flush_cache_range = r4k_flush_cache_range_d16i16;
		flush_cache_page = r4k_flush_cache_page_d16i16;
		flush_page_to_ram = r4k_flush_page_to_ram_d16i16;
		break;
	case 32:
		prid = read_32bit_cp0_register(CP0_PRID) & 0xfff0;
		if (prid == 0x2010) {			/* R4600 V1.7 */
			clear_page = r4k_clear_page_r4600_v1;
			copy_page = r4k_copy_page_r4600_v1;
		} else if (prid == 0x2020) {		/* R4600 V2.0 */
			clear_page = r4k_clear_page_r4600_v2;
			copy_page = r4k_copy_page_r4600_v2;
		} else {
			clear_page = r4k_clear_page_d32;
			copy_page = r4k_copy_page_d32;
		}
		flush_cache_all = r4k_flush_cache_all_d32i32;
		flush_cache_mm = r4k_flush_cache_mm_d32i32;
		flush_cache_range = r4k_flush_cache_range_d32i32;
		flush_cache_page = r4k_flush_cache_page_d32i32;
		flush_page_to_ram = r4k_flush_page_to_ram_d32i32;
		break;
	}
	flush_cache_pre_dma_out = r4k_flush_cache_pre_dma_out_pc;
	flush_cache_post_dma_in = r4k_flush_cache_post_dma_in_pc;
}

static void setup_scache_funcs(void)
{
	switch(sc_lsize) {
	case 16:
		switch(dc_lsize) {
		case 16:
			clear_page = r4k_clear_page_d16;
			copy_page = r4k_copy_page_d16;
			flush_cache_all = r4k_flush_cache_all_s16d16i16;
			flush_cache_mm = r4k_flush_cache_mm_s16d16i16;
			flush_cache_range = r4k_flush_cache_range_s16d16i16;
			flush_cache_page = r4k_flush_cache_page_s16d16i16;
			flush_page_to_ram = r4k_flush_page_to_ram_s16d16i16;
			break;
		case 32:
			clear_page = r4k_clear_page_d32;
			copy_page = r4k_copy_page_d32;
			flush_cache_all = r4k_flush_cache_all_s16d32i32;
			flush_cache_mm = r4k_flush_cache_mm_s16d32i32;
			flush_cache_range = r4k_flush_cache_range_s16d32i32;
			flush_cache_page = r4k_flush_cache_page_s16d32i32;
			flush_page_to_ram = r4k_flush_page_to_ram_s16d32i32;
			break;
		};
		break;
	case 32:
		switch(dc_lsize) {
		case 16:
			clear_page = r4k_clear_page_d16;
			copy_page = r4k_copy_page_d16;
			flush_cache_all = r4k_flush_cache_all_s32d16i16;
			flush_cache_mm = r4k_flush_cache_mm_s32d16i16;
			flush_cache_range = r4k_flush_cache_range_s32d16i16;
			flush_cache_page = r4k_flush_cache_page_s32d16i16;
			flush_page_to_ram = r4k_flush_page_to_ram_s32d16i16;
			break;
		case 32:
			clear_page = r4k_clear_page_d32;
			copy_page = r4k_copy_page_d32;
			flush_cache_all = r4k_flush_cache_all_s32d32i32;
			flush_cache_mm = r4k_flush_cache_mm_s32d32i32;
			flush_cache_range = r4k_flush_cache_range_s32d32i32;
			flush_cache_page = r4k_flush_cache_page_s32d32i32;
			flush_page_to_ram = r4k_flush_page_to_ram_s32d32i32;
			break;
		};
	case 64:
		switch(dc_lsize) {
		case 16:
			clear_page = r4k_clear_page_d16;
			copy_page = r4k_copy_page_d16;
			flush_cache_all = r4k_flush_cache_all_s64d16i16;
			flush_cache_mm = r4k_flush_cache_mm_s64d16i16;
			flush_cache_range = r4k_flush_cache_range_s64d16i16;
			flush_cache_page = r4k_flush_cache_page_s64d16i16;
			flush_page_to_ram = r4k_flush_page_to_ram_s64d16i16;
			break;
		case 32:
			clear_page = r4k_clear_page_d32;
			copy_page = r4k_copy_page_d32;
			flush_cache_all = r4k_flush_cache_all_s64d32i32;
			flush_cache_mm = r4k_flush_cache_mm_s64d32i32;
			flush_cache_range = r4k_flush_cache_range_s64d32i32;
			flush_cache_page = r4k_flush_cache_page_s64d32i32;
			flush_page_to_ram = r4k_flush_page_to_ram_s64d32i32;
			break;
		};
	case 128:
		switch(dc_lsize) {
		case 16:
			clear_page = r4k_clear_page_d16;
			copy_page = r4k_copy_page_d16;
			flush_cache_all = r4k_flush_cache_all_s128d16i16;
			flush_cache_mm = r4k_flush_cache_mm_s128d16i16;
			flush_cache_range = r4k_flush_cache_range_s128d16i16;
			flush_cache_page = r4k_flush_cache_page_s128d16i16;
			flush_page_to_ram = r4k_flush_page_to_ram_s128d16i16;
			break;
		case 32:
			clear_page = r4k_clear_page_d32;
			copy_page = r4k_copy_page_d32;
			flush_cache_all = r4k_flush_cache_all_s128d32i32;
			flush_cache_mm = r4k_flush_cache_mm_s128d32i32;
			flush_cache_range = r4k_flush_cache_range_s128d32i32;
			flush_cache_page = r4k_flush_cache_page_s128d32i32;
			flush_page_to_ram = r4k_flush_page_to_ram_s128d32i32;
			break;
		};
		break;
	}

	/* XXX Do these for Indy style caches also.  No need for now ... */
	flush_cache_pre_dma_out = r4k_flush_cache_pre_dma_out_sc;
	flush_cache_post_dma_in = r4k_flush_cache_post_dma_in_sc;
}

typedef int (*probe_func_t)(unsigned long);
static probe_func_t probe_scache_kseg1;

void ld_mmu_r4xx0(void)
{
	unsigned long cfg = read_32bit_cp0_register(CP0_CONFIG);
	int sc_present = 0;

	printk("CPU revision is: %08x\n", read_32bit_cp0_register(CP0_PRID));

	set_cp0_config(CONFIG_CM_CMASK, CONFIG_CM_CACHABLE_NONCOHERENT);

	probe_icache(cfg);
	probe_dcache(cfg);

	switch(mips_cputype) {
	case CPU_R4000PC:
	case CPU_R4000SC:
	case CPU_R4000MC:
	case CPU_R4400PC:
	case CPU_R4400SC:
	case CPU_R4400MC:
		probe_scache_kseg1 = (probe_func_t) (KSEG1ADDR(&probe_scache));
		sc_present = probe_scache_kseg1(cfg);
		break;

	case CPU_R4600:
	case CPU_R4640:
	case CPU_R4700:
	case CPU_R5000:	/* XXX: We don't handle the true R5000 SCACHE */
	case CPU_NEVADA:
		probe_scache_kseg1 = (probe_func_t)
			(KSEG1ADDR(&probe_scache_eeprom));
		sc_present = probe_scache_eeprom(cfg);

		/* Try using tags if eeprom gives us bogus data. */
		if(sc_present == -1) {
			probe_scache_kseg1 =
				(probe_func_t) (KSEG1ADDR(&probe_scache));
			sc_present = probe_scache_kseg1(cfg);
		}
		break;
	};

	if(sc_present == 1
	   && (mips_cputype == CPU_R4000SC
               || mips_cputype == CPU_R4000MC
               || mips_cputype == CPU_R4400SC
               || mips_cputype == CPU_R4400MC)) {
		/* Has a true secondary cache. */
		setup_scache_funcs();
	} else {
		/* Lacks true secondary cache. */
		setup_noscache_funcs();
		if((mips_cputype != CPU_R5000)) { /* XXX */
			flush_cache_page =
				r4k_flush_cache_page_d32i32_r4600;
			flush_page_to_ram =
				r4k_flush_page_to_ram_d32i32_r4600;
		}
	}

	/* XXX Handle true second level cache w/ split I/D */
	flush_cache_sigtramp = r4k_flush_cache_sigtramp;
	if ((read_32bit_cp0_register(CP0_PRID) & 0xfff0) == 0x2020) {
		flush_cache_sigtramp = r4600v20k_flush_cache_sigtramp;
	}

	flush_tlb_all = r4k_flush_tlb_all;
	flush_tlb_mm = r4k_flush_tlb_mm;
	flush_tlb_range = r4k_flush_tlb_range;
	flush_tlb_page = r4k_flush_tlb_page;

	load_pgd = r4k_load_pgd;
	pgd_init = r4k_pgd_init;
	update_mmu_cache = r4k_update_mmu_cache;

	show_regs = r4k_show_regs;
    
        add_wired_entry = r4k_add_wired_entry;

	flush_cache_all();
	write_32bit_cp0_register(CP0_WIRED, 0);

	/*
	 * You should never change this register:
	 *   - On R4600 1.7 the tlbp never hits for pages smaller than
	 *     the value in the c0_pagemask register.
	 *   - The entire mm handling assumes the c0_pagemask register to
	 *     be set for 4kb pages.
	 */
	write_32bit_cp0_register(CP0_PAGEMASK, PM_4K);
	flush_tlb_all();
}
