/*
 * r2300.c: R2000 and R3000 specific mmu/cache code.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 *
 * with a lot of changes to make this thing work for R3000s
 * Tx39XX R4k style caches added. HK
 * Copyright (C) 1998, 1999, 2000 Harald Koerfgen
 * Copyright (C) 1998 Gleb Raiko & Vladimir Roganov
 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/mm.h>

#include <asm/cacheops.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/mmu_context.h>
#include <asm/system.h>
#include <asm/isadep.h>
#include <asm/io.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>

/* For R3000 cores with R4000 style caches */
static unsigned long icache_size, dcache_size;		/* Size in bytes */
extern long scache_size;

#define icache_lsize	mips_cpu.icache.linesz
#define dcache_lsize	mips_cpu.dcache.linesz

#include <asm/r4kcache.h>

extern int r3k_have_wired_reg;	/* in r3k-tlb.c */

static void tx39h_flush_dcache_page(struct page * page)
{
}

/* TX39H-style cache flush routines. */
static void tx39h_flush_icache_all(void)
{
	unsigned long start = KSEG0;
	unsigned long end = (start + icache_size);
	unsigned long flags, config;

	/* disable icache (set ICE#) */
	local_irq_save(flags);
	config = read_c0_conf();
	write_c0_conf(config & ~TX39_CONF_ICE);

	/* invalidate icache */
	while (start < end) {
		cache16_unroll32(start, Index_Invalidate_I);
		start += 0x200;
	}

	write_c0_conf(config);
	local_irq_restore(flags);
}

static void tx39h_dma_cache_wback_inv(unsigned long addr, unsigned long size)
{
	unsigned long end, a;
	int lsize = dcache_lsize;

	iob();
	a = addr & ~(lsize - 1);
	end = (addr + size - 1) & ~(lsize - 1);
	while (1) {
		invalidate_dcache_line(a); /* Hit_Invalidate_D */
		if (a == end) break;
		a += lsize;
	}
}


/* TX39H2,TX39H3 */
static inline void tx39_blast_dcache_page(unsigned long addr)
{
	if (mips_cpu.cputype != CPU_TX3912)
		blast_dcache16_page(addr);
}

static inline void tx39_blast_dcache_page_indexed(unsigned long addr)
{
	blast_dcache16_page_indexed_wayLSB(addr);
}

static inline void tx39_blast_dcache(void)
{
	blast_dcache16_wayLSB();
}

static inline void tx39_blast_icache_page(unsigned long addr)
{
	unsigned long flags, config;
	/* disable icache (set ICE#) */
	local_irq_save(flags);
	config = read_c0_conf();
	write_c0_conf(config & ~TX39_CONF_ICE);
	blast_icache16_page(addr);
	write_c0_conf(config);
	local_irq_restore(flags);
}

static inline void tx39_blast_icache_page_indexed(unsigned long addr)
{
	unsigned long flags, config;
	/* disable icache (set ICE#) */
	local_irq_save(flags);
	config = read_c0_conf();
	write_c0_conf(config & ~TX39_CONF_ICE);
	blast_icache16_page_indexed_wayLSB(addr);
	write_c0_conf(config);
	local_irq_restore(flags);
}

static inline void tx39_blast_icache(void)
{
	unsigned long flags, config;
	/* disable icache (set ICE#) */
	local_irq_save(flags);
	config = read_c0_conf();
	write_c0_conf(config & ~TX39_CONF_ICE);
	blast_icache16_wayLSB();
	write_c0_conf(config);
	local_irq_restore(flags);
}

static inline void tx39_flush_cache_all(void)
{
	tx39_blast_dcache();
	tx39_blast_icache();
}

static void tx39_flush_cache_mm(struct mm_struct *mm)
{
	if (cpu_context(smp_processor_id(), mm) != 0) {
		tx39_flush_cache_all();
	}
}

static void tx39_flush_cache_range(struct vm_area_struct *vma,
	unsigned long start, unsigned long end)
{
	struct mm_struct *mm = vma->vm_mm;

	if (cpu_context(smp_processor_id(), mm) != 0) {
		tx39_blast_dcache();
		tx39_blast_icache();
	}
}

static void tx39_flush_cache_page(struct vm_area_struct *vma,
				   unsigned long page)
{
	int exec = vma->vm_flags & VM_EXEC;
	struct mm_struct *mm = vma->vm_mm;
	pgd_t *pgdp;
	pmd_t *pmdp;
	pte_t *ptep;

	/*
	 * If ownes no valid ASID yet, cannot possibly have gotten
	 * this page into the cache.
	 */
	if (cpu_context(smp_processor_id(), mm) == 0)
		return;

	page &= PAGE_MASK;
	pgdp = pgd_offset(mm, page);
	pmdp = pmd_offset(pgdp, page);
	ptep = pte_offset(pmdp, page);

	/*
	 * If the page isn't marked valid, the page cannot possibly be
	 * in the cache.
	 */
	if (!(pte_val(*ptep) & _PAGE_VALID))
		return;

	/*
	 * Doing flushes for another ASID than the current one is
	 * too difficult since stupid R4k caches do a TLB translation
	 * for every cache flush operation.  So we do indexed flushes
	 * in that case, which doesn't overly flush the cache too much.
	 */
	if (mm == current->active_mm) {
		tx39_blast_dcache_page(page);
		if (exec)
			tx39_blast_icache_page(page);

		return;
	}

	/*
	 * Do indexed flush, too much work to get the (possible) TLB refills
	 * to work correctly.
	 */
	page = (KSEG0 + (page & (dcache_size - 1)));
	tx39_blast_dcache_page_indexed(page);
	if (exec)
		tx39_blast_icache_page_indexed(page);
}

static void tx39_flush_dcache_page_impl(struct page *page)
{
	unsigned long addr = (unsigned long) page_address(page);

	tx39_blast_dcache_page(addr);
}

static void tx39_flush_dcache_page(struct page *page)
{
	if (page->mapping &&
	    list_empty(&page->mapping->i_mmap) &&
	    list_empty(&page->mapping->i_mmap_shared)) {
		SetPageDcacheDirty(page);

		return;
	}

	/*
	 * We could delay the flush for the !page->mapping case too.  But that
	 * case is for exec env/arg pages and those are %99 certainly going to
	 * get faulted into the tlb (and thus flushed) anyways.
	 */
	tx39_flush_dcache_page_impl(page);
}

static void tx39_flush_icache_range(unsigned long start, unsigned long end)
{
	flush_cache_all();
}

/*
 * Ok, this seriously sucks.  We use them to flush a user page but don't
 * know the virtual address, so we have to blast away the whole icache
 * which is significantly more expensive than the real thing.  Otoh we at
 * least know the kernel address of the page so we can flush it
 * selectivly.
 */
static void tx39_flush_icache_page(struct vm_area_struct *vma, struct page *page)
{
	if (vma->vm_flags & VM_EXEC) {
		unsigned long addr = (unsigned long) page_address(page);

		tx39_blast_dcache_page(addr);
		tx39_blast_icache();
	}
}

static void tx39_dma_cache_wback_inv(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (((size | addr) & (PAGE_SIZE - 1)) == 0) {
		end = addr + size;
		do {
			tx39_blast_dcache_page(addr);
			addr += PAGE_SIZE;
		} while(addr != end);
	} else if (size > dcache_size) {
		tx39_blast_dcache();
	} else {
		int lsize = dcache_lsize;
		a = addr & ~(lsize - 1);
		end = (addr + size - 1) & ~(lsize - 1);
		while (1) {
			flush_dcache_line(a); /* Hit_Writeback_Inv_D */
			if (a == end) break;
			a += lsize;
		}
	}
}

static void tx39_dma_cache_inv(unsigned long addr, unsigned long size)
{
	unsigned long end, a;

	if (((size | addr) & (PAGE_SIZE - 1)) == 0) {
		end = addr + size;
		do {
			tx39_blast_dcache_page(addr);
			addr += PAGE_SIZE;
		} while(addr != end);
	} else if (size > dcache_size) {
		tx39_blast_dcache();
	} else {
		int lsize = dcache_lsize;
		a = addr & ~(lsize - 1);
		end = (addr + size - 1) & ~(lsize - 1);
		while (1) {
			invalidate_dcache_line(a); /* Hit_Invalidate_D */
			if (a == end) break;
			a += lsize;
		}
	}
}

static void tx39_dma_cache_wback(unsigned long addr, unsigned long size)
{
	panic("tx39_dma_cache called - should not happen.");
}

static void tx39_flush_cache_sigtramp(unsigned long addr)
{
	unsigned long config;
	unsigned int flags;

	protected_writeback_dcache_line(addr & ~(dcache_lsize - 1));

	/* disable icache (set ICE#) */
	local_irq_save(flags);
	config = read_c0_conf();
	write_c0_conf(config & ~TX39_CONF_ICE);
	protected_flush_icache_line(addr & ~(icache_lsize - 1));
	write_c0_conf(config);
	local_irq_restore(flags);
}

void __update_cache(struct vm_area_struct *vma, unsigned long address,
	pte_t pte)
{
	struct page *page;
	unsigned long pfn;

	pfn = pte_pfn(pte);
	if (pfn_valid(pfn) && (page = pfn_to_page(pfn), page->mapping) &&
	    Page_dcache_dirty(page)) {
		tx39_flush_dcache_page_impl(page);

		ClearPageDcacheDirty(page);
	}
}

static __init void tx39_probe_cache(void)
{
	unsigned long config;

	config = read_c0_conf();

	icache_size = 1 << (10 + ((config >> 19) & 3));
	dcache_size = 1 << (10 + ((config >> 16) & 3));

	icache_lsize = 16;
	switch (mips_cpu.cputype) {
	case CPU_TX3912:
		dcache_lsize = 4;
		break;
	case CPU_TX3922:
	case CPU_TX3927:
	case CPU_TX39XX:
	default:
		dcache_lsize = 16;
		break;
	}
}

void __init ld_mmu_tx39(void)
{
	unsigned long config;

	_clear_page = r3k_clear_page;
	_copy_page = r3k_copy_page;

	config = read_c0_conf();
	config &= ~TX39_CONF_WBON;
	write_c0_conf(config);

	tx39_probe_cache();

	switch (mips_cpu.cputype) {
	case CPU_TX3912:
		/* TX39/H core (writethru direct-map cache) */
		_flush_cache_all	= tx39h_flush_icache_all;
		___flush_cache_all	= tx39h_flush_icache_all;
		_flush_cache_mm		= (void *) tx39h_flush_icache_all;
		_flush_cache_range	= (void *) tx39h_flush_icache_all;
		_flush_cache_page	= (void *) tx39h_flush_icache_all;
		_flush_cache_sigtramp	= (void *) tx39h_flush_icache_all;
		_flush_dcache_page	= tx39h_flush_dcache_page;
		_flush_icache_page	= (void *) tx39h_flush_icache_all;
		_flush_icache_range	= (void *) tx39h_flush_icache_all;

		_dma_cache_wback_inv = tx39h_dma_cache_wback_inv;
		break;

	case CPU_TX3922:
	case CPU_TX3927:
	default:
		/* TX39/H2,H3 core (writeback 2way-set-associative cache) */
		r3k_have_wired_reg = 1;
		write_c0_wired(0);	/* set 8 on reset... */
		/* board-dependent init code may set WBON */

		_flush_cache_all = tx39_flush_cache_all;
		___flush_cache_all = tx39_flush_cache_all;
		_flush_cache_mm = tx39_flush_cache_mm;
		_flush_cache_range = tx39_flush_cache_range;
		_flush_cache_page = tx39_flush_cache_page;
		_flush_cache_sigtramp = tx39_flush_cache_sigtramp;
		_flush_dcache_page = tx39_flush_dcache_page;
		_flush_icache_page = tx39_flush_icache_page;
		_flush_icache_range = tx39_flush_icache_range;

		_dma_cache_wback_inv = tx39_dma_cache_wback_inv;
		_dma_cache_wback = tx39_dma_cache_wback;
		_dma_cache_inv = tx39_dma_cache_inv;

		break;
	}

	if (mips_cpu.icache.ways == 0)
		mips_cpu.icache.ways = 1;
	if (mips_cpu.dcache.ways == 0)
		mips_cpu.dcache.ways = 1;
	mips_cpu.icache.sets =
		icache_size / mips_cpu.icache.ways / mips_cpu.icache.linesz;
	mips_cpu.dcache.sets =
		dcache_size / mips_cpu.dcache.ways / mips_cpu.dcache.linesz;

	printk("Primary instruction cache %dkb, linesize %d bytes\n",
		(int) (icache_size >> 10), (int) icache_lsize);
	printk("Primary data cache %dkb, linesize %d bytes\n",
		(int) (dcache_size >> 10), (int) dcache_lsize);
}
