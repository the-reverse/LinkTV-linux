/*
 * TLB support routines.
 *
 * Copyright (C) 1998, 1999 Hewlett-Packard Co
 * Copyright (C) 1998, 1999 David Mosberger-Tang <davidm@hpl.hp.com>
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/smp.h>
#include <linux/mm.h>

#include <asm/mmu_context.h>
#include <asm/pgalloc.h>
#include <asm/pal.h>

#define SUPPORTED_PGBITS (			\
		1 << _PAGE_SIZE_256M |		\
		1 << _PAGE_SIZE_64M  |		\
		1 << _PAGE_SIZE_16M  |		\
		1 << _PAGE_SIZE_4M   |		\
		1 << _PAGE_SIZE_1M   |		\
		1 << _PAGE_SIZE_256K |		\
		1 << _PAGE_SIZE_64K  |		\
		1 << _PAGE_SIZE_16K  |		\
		1 << _PAGE_SIZE_8K   |		\
		1 << _PAGE_SIZE_4K )

static void wrap_context (struct mm_struct *mm);

unsigned long ia64_next_context = (1UL << IA64_HW_CONTEXT_BITS) + 1;

 /*
  * Put everything in a struct so we avoid the global offset table whenever
  * possible.
  */
ia64_ptce_info_t ia64_ptce_info;

/*
 * Seralize usage of ptc.g 
 */
spinlock_t ptcg_lock = SPIN_LOCK_UNLOCKED; /* see <asm/pgtable.h> */

#if defined(CONFIG_SMP) && !defined(CONFIG_ITANIUM_PTCG)

#include <linux/irq.h>

unsigned long 	flush_end, flush_start, flush_nbits, flush_rid;
atomic_t flush_cpu_count;

/*
 * flush_tlb_no_ptcg is called with ptcg_lock locked
 */
static inline void
flush_tlb_no_ptcg (unsigned long start, unsigned long end, unsigned long nbits)
{
	extern void smp_send_flush_tlb (void);
	unsigned long saved_tpr = 0;
	unsigned long flags;

	/*
	 * Some times this is called with interrupts disabled and causes
	 * dead-lock; to avoid this we enable interrupt and raise the TPR
	 * to enable ONLY IPI.
	 */
	__save_flags(flags);
	if (!(flags & IA64_PSR_I)) {
		saved_tpr = ia64_get_tpr();
		ia64_srlz_d();
		ia64_set_tpr(IPI_IRQ - 16);
		ia64_srlz_d();
		local_irq_enable();
	}

	spin_lock(&ptcg_lock);
	flush_rid = ia64_get_rr(start);
	ia64_srlz_d();
	flush_start = start;
	flush_end = end;
	flush_nbits = nbits;
	atomic_set(&flush_cpu_count, smp_num_cpus - 1);
	smp_send_flush_tlb();
	/*
	 * Purge local TLB entries. ALAT invalidation is done in ia64_leave_kernel.
	 */
	do {
		asm volatile ("ptc.l %0,%1" :: "r"(start), "r"(nbits<<2) : "memory");
		start += (1UL << nbits);
	} while (start < end);

	ia64_srlz_i();			/* srlz.i implies srlz.d */

	/*
	 * Wait for other CPUs to finish purging entries.
	 */
	while (atomic_read(&flush_cpu_count)) {
		/* Nothing */
	}
	if (!(flags & IA64_PSR_I)) {
		local_irq_disable();
		ia64_set_tpr(saved_tpr);
		ia64_srlz_d();
	}
}

#endif /* CONFIG_SMP && !CONFIG_ITANIUM_PTCG */

void
get_new_mmu_context (struct mm_struct *mm)
{
	if ((ia64_next_context & IA64_HW_CONTEXT_MASK) == 0) {
		wrap_context(mm);
	}
	mm->context = ia64_next_context++;
}

/*
 * This is where we handle the case where (ia64_next_context &
 * IA64_HW_CONTEXT_MASK) == 0.  Whenever this happens, we need to
 * flush the entire TLB and skip over region id number 0, which is
 * used by the kernel.
 */
static void
wrap_context (struct mm_struct *mm)
{
	struct task_struct *task;

	/*
	 * We wrapped back to the first region id so we nuke the TLB
	 * so we can switch to the next generation of region ids.
	 */
	__flush_tlb_all();
	if (ia64_next_context++ == 0) {
		/*
		 * Oops, we've used up all 64 bits of the context
		 * space---walk through task table to ensure we don't
		 * get tricked into using an old context.  If this
		 * happens, the machine has been running for a long,
		 * long time!
		 */
		ia64_next_context = (1UL << IA64_HW_CONTEXT_BITS) + 1;

		read_lock(&tasklist_lock);
		for_each_task (task) {
			if (task->mm == mm)
				continue;
			flush_tlb_mm(mm);
		}
		read_unlock(&tasklist_lock);
	}
}

void
__flush_tlb_all (void)
{
	unsigned long i, j, flags, count0, count1, stride0, stride1, addr = ia64_ptce_info.base;

	count0  = ia64_ptce_info.count[0];
	count1  = ia64_ptce_info.count[1];
	stride0 = ia64_ptce_info.stride[0];
	stride1 = ia64_ptce_info.stride[1];

	local_irq_save(flags);
	for (i = 0; i < count0; ++i) {
		for (j = 0; j < count1; ++j) {
			asm volatile ("ptc.e %0" :: "r"(addr));
			addr += stride1;
		}
		addr += stride0;
	}
	local_irq_restore(flags);
	ia64_insn_group_barrier();
	ia64_srlz_i();			/* srlz.i implies srlz.d */
	ia64_insn_group_barrier();
}

void
flush_tlb_range (struct mm_struct *mm, unsigned long start, unsigned long end)
{
	unsigned long size = end - start;
	unsigned long nbits;

	if (mm != current->active_mm) {
		/* this doesn't happen often, if at all, so it's not worth optimizing for... */
		mm->context = 0;
		return;
	}

	nbits = ia64_fls(size + 0xfff);
	if (((1UL << nbits) & SUPPORTED_PGBITS) == 0) {
		if (nbits > _PAGE_SIZE_256M)
			nbits = _PAGE_SIZE_256M;
		else
			/*
			 * Some page sizes are not implemented in the
			 * IA-64 arch, so if we get asked to clear an
			 * unsupported page size, round up to the
			 * nearest page size.  Note that we depend on
			 * the fact that if page size N is not
			 * implemented, 2*N _is_ implemented.
			 */
			++nbits;
		if (((1UL << nbits) & SUPPORTED_PGBITS) == 0)
			panic("flush_tlb_range: BUG: nbits=%lu\n", nbits);
	}
	start &= ~((1UL << nbits) - 1);

#if defined(CONFIG_SMP) && !defined(CONFIG_ITANIUM_PTCG)
	flush_tlb_no_ptcg(start, end, nbits);
#else
	spin_lock(&ptcg_lock);
	do {
# ifdef CONFIG_SMP
		/*
		 * Flush ALAT entries also.
		 */
		asm volatile ("ptc.ga %0,%1;;srlz.i;;" :: "r"(start), "r"(nbits<<2) : "memory");
# else
		asm volatile ("ptc.l %0,%1" :: "r"(start), "r"(nbits<<2) : "memory");
# endif
		start += (1UL << nbits);
	} while (start < end);
#endif /* CONFIG_SMP && !defined(CONFIG_ITANIUM_PTCG) */
	spin_unlock(&ptcg_lock);
	ia64_insn_group_barrier();
	ia64_srlz_i();			/* srlz.i implies srlz.d */
	ia64_insn_group_barrier();
}

void __init
ia64_tlb_init (void)
{
	ia64_get_ptce(&ia64_ptce_info);
	__flush_tlb_all();		/* nuke left overs from bootstrapping... */
}
