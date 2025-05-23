/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1994 - 2000, 2001, 2003 Ralf Baechle
 * Copyright (C) 1999, 2000 Silicon Graphics, Inc.
 * Copyright (C) 2001 MIPS Technologies, Inc.
 */
#include <linux/config.h>

#include <asm/asm.h>
#include <asm/asmmacro.h>
#include <asm/regdef.h>
#include <asm/mipsregs.h>
#include <asm/stackframe.h>
#include <asm/isadep.h>
#include <asm/thread_info.h>
#include <asm/war.h>

#ifdef CONFIG_PREEMPT
	.macro	preempt_stop
	.endm
#else
	.macro	preempt_stop
	local_irq_disable
	.endm
#define resume_kernel	restore_all
#endif

	.text
	.align	5
#if defined(CONFIG_REALTEK_USE_FAST_INTERRUPT) || defined(CONFIG_REALTEK_USE_FAST_INTERRUPT_AND_DMEM)
FEXPORT(fast_ret_from_irq)
/*
 *       LONG_L  t0, PT_STATUS(sp)               # returning to kernel mode?
 *       andi    t0, t0, KU_USER
 *       beqz    t0, resume_kernel
 */
	local_irq_disable
        .set    noat
        FAST_RESTORE_ALL
        .set    at
#endif

#if defined(CONFIG_REALTEK_USE_SHADOW_REGISTERS) || defined(CONFIG_REALTEK_USE_DMEM)
FEXPORT(ret_from_irq)
        LONG_L  t0, PT_STATUS(sp)               # returning to kernel mode?
        andi    t0, t0, KU_USER
#ifdef CONFIG_REALTEK_USE_DMEM
        beqz    t0, my_resume_kernel
#else
        beqz    t0, resume_kernel
#endif

my_resume_userspace:
        local_irq_disable               # make sure we dont miss an
                                        # interrupt setting need_resched
                                        # between sampling and return
        LONG_L  a2, TI_FLAGS($28)       # current->work
        andi    t0, a2, _TIF_WORK_MASK  # (ignoring syscall_trace)
        bnez    t0, my_work_pending
#ifdef CONFIG_REALTEK_USE_DMEM
        j       restore_all
#else
        j       my_restore_all
#endif

my_work_pending:
#ifdef CONFIG_REALTEK_USE_DMEM
        DMEM_SAVE_ON_DEMAND
#else
        MY_SAVE_ON_DEMAND
#endif
        andi    t0, a2, _TIF_NEED_RESCHED # a2 is preloaded with TI_FLAGS
        beqz    t0, my_work_notifysig
my_work_resched:
        jal     schedule

        local_irq_disable               # make sure need_resched and
                                        # signals dont change between
                                        # sampling and return
        LONG_L  a2, TI_FLAGS($28)
        andi    t0, a2, _TIF_WORK_MASK  # is there any work to be done
                                        # other than syscall tracing?
        beqz    t0, restore_all
        andi    t0, a2, _TIF_NEED_RESCHED
        bnez    t0, my_work_resched

my_work_notifysig:                      # deal with pending signals and
                                        # notify-resume requests
        move    a0, sp
        li      a1, 0
        jal     do_notify_resume        # a2 already loaded
        j       restore_all
#endif

#ifdef CONFIG_REALTEK_USE_DMEM
my_resume_kernel:
	local_irq_disable
	lw	t0, TI_PRE_COUNT($28)
	bnez	t0, restore_all
        DMEM_SAVE_ON_DEMAND
my_need_resched:
	LONG_L	t0, TI_FLAGS($28)
	andi	t1, t0, _TIF_NEED_RESCHED
	beqz	t1, restore_all
	LONG_L	t0, PT_STATUS(sp)		# Interrupts off?
	andi	t0, 1
	beqz	t0, restore_all
	jal	preempt_schedule_irq
	b	my_need_resched
#endif

FEXPORT(ret_from_exception)
	preempt_stop
#ifdef CONFIG_REALTEK_SBSS_IN_DMEM_ADVANCED
	b	1f
#endif
#if !defined(CONFIG_REALTEK_USE_SHADOW_REGISTERS) && !defined(CONFIG_REALTEK_USE_DMEM)
FEXPORT(ret_from_irq)
#endif
#ifdef CONFIG_REALTEK_SBSS_IN_DMEM_ADVANCED
	.set    push
	.set    noreorder
	lui     k0, %hi(interrupt_cnt)
	LONG_L  k1, %lo(interrupt_cnt)(k0)
	subu    k1, k1, 1
	bnez    k1, 1f
	LONG_S  k1, %lo(interrupt_cnt)(k0)
	LONG_L  sp, %lo(interrupt_tmp)(k0)
1:
	.set    pop
#endif
	LONG_L	t0, PT_STATUS(sp)		# returning to kernel mode?
	andi	t0, t0, KU_USER
	beqz	t0, resume_kernel

resume_userspace:
	local_irq_disable		# make sure we dont miss an
					# interrupt setting need_resched
					# between sampling and return
	LONG_L	a2, TI_FLAGS($28)	# current->work
	andi	t0, a2, _TIF_WORK_MASK	# (ignoring syscall_trace)
	bnez	t0, work_pending
	j	restore_all

#ifdef CONFIG_PREEMPT
resume_kernel:
	local_irq_disable
	lw	t0, TI_PRE_COUNT($28)
	bnez	t0, restore_all
need_resched:
	LONG_L	t0, TI_FLAGS($28)
	andi	t1, t0, _TIF_NEED_RESCHED
	beqz	t1, restore_all
	LONG_L	t0, PT_STATUS(sp)		# Interrupts off?
	andi	t0, 1
	beqz	t0, restore_all
	jal	preempt_schedule_irq
	b	need_resched
#endif

FEXPORT(ret_from_fork)
	jal	schedule_tail		# a0 = task_t *prev

FEXPORT(syscall_exit)
	local_irq_disable		# make sure need_resched and
					# signals dont change between
					# sampling and return
	LONG_L	a2, TI_FLAGS($28)	# current->work
	li	t0, _TIF_ALLWORK_MASK
	and	t0, a2, t0
	bnez	t0, syscall_exit_work

FEXPORT(restore_all)			# restore full frame
	.set	noat
	RESTORE_TEMP
	RESTORE_AT
	RESTORE_STATIC
FEXPORT(restore_partial)		# restore partial frame
	RESTORE_SOME
	RESTORE_SP_AND_RET
	.set	at

#ifdef CONFIG_REALTEK_USE_SHADOW_REGISTERS
FEXPORT(my_restore_all)                 # restore full frame
        .set    noat
        MY_RESTORE_ALL
        .set    at
#endif

work_pending:
	andi	t0, a2, _TIF_NEED_RESCHED # a2 is preloaded with TI_FLAGS
	beqz	t0, work_notifysig
work_resched:
	jal	schedule

	local_irq_disable		# make sure need_resched and
					# signals dont change between
					# sampling and return
	LONG_L	a2, TI_FLAGS($28)
	andi	t0, a2, _TIF_WORK_MASK	# is there any work to be done
					# other than syscall tracing?
	beqz	t0, restore_all
	andi	t0, a2, _TIF_NEED_RESCHED
	bnez	t0, work_resched

work_notifysig:				# deal with pending signals and
					# notify-resume requests
	move	a0, sp
	li	a1, 0
	jal	do_notify_resume	# a2 already loaded
	j	restore_all

FEXPORT(syscall_exit_work_partial)
	SAVE_STATIC
syscall_exit_work:
	li	t0, _TIF_SYSCALL_TRACE | _TIF_SYSCALL_AUDIT
	and	t0, a2			# a2 is preloaded with TI_FLAGS
	beqz	t0, work_pending	# trace bit set?
	local_irq_enable		# could let do_syscall_trace()
					# call schedule() instead
	move	a0, sp
	li	a1, 1
	jal	do_syscall_trace
	b	resume_userspace

/*
 * Common spurious interrupt handler.
 */
LEAF(spurious_interrupt)
	/*
	 * Someone tried to fool us by sending an interrupt but we
	 * couldn't find a cause for it.
	 */
	PTR_LA	t1, irq_err_count
#ifdef CONFIG_SMP
1:	ll      t0, (t1)
	addiu   t0, 1
	sc      t0, (t1)
#if R10000_LLSC_WAR
	beqzl	t0, 1b
#else
	beqz	t0, 1b
#endif
#else
	lw      t0, (t1)
	addiu   t0, 1
	sw      t0, (t1)
#endif
	j	ret_from_irq
	END(spurious_interrupt)
