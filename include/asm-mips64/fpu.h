/*
 * Copyright (C) 2002 MontaVista Software Inc.
 * Author: Jun Sun, jsun@mvista.com or jsun@junsun.net
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 */
#ifndef _ASM_FPU_H
#define _ASM_FPU_H

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/thread_info.h>

#include <asm/mipsregs.h>
#include <asm/cpu.h>
#include <asm/processor.h>
#include <asm/current.h>

struct sigcontext;

extern asmlinkage int (*save_fp_context)(struct sigcontext *sc);
extern asmlinkage int (*restore_fp_context)(struct sigcontext *sc);

extern void fpu_emulator_init_fpu(void);
extern void _init_fpu(void);
extern void _save_fp(struct task_struct *);
extern void _restore_fp(struct task_struct *);

#if defined(CONFIG_CPU_SB1)
#define __enable_fpu_hazard()						\
do {									\
	asm(".set	push		\n\t"				\
	    ".set	mips2		# .set mips64	\n\t"		\
	    ".set	noreorder	\n\t"				\
	    "sll	$0, $0, 1	# ssnop		\n\t"		\
	    "bnezl	$0, .+4		\n\t"				\
	    "sll	$0, $0, 1	# ssnop		\n\t"		\
	    ".set pop");						\
} while (0)
#else
#define __enable_fpu_hazard()						\
do {									\
	asm("nop;nop;nop;nop");		/* max. hazard */		\
} while (0)
#endif

#define __enable_fpu()							\
do {									\
	set_cp0_status(ST0_CU1);					\
	__enable_fpu_hazard();						\
} while (0)

#define __disable_fpu()							\
do {									\
	clear_cp0_status(ST0_CU1);					\
	/* We don't care about the cp0 hazard here  */			\
} while (0)

#define enable_fpu()							\
do {									\
	if (mips_cpu.options & MIPS_CPU_FPU)				\
		__enable_fpu();						\
} while (0)

#define disable_fpu()							\
do {									\
	if (mips_cpu.options & MIPS_CPU_FPU)				\
		__disable_fpu();					\
} while (0)


#define clear_fpu_owner()	clear_thread_flag(TIF_USEDFPU)

static inline int is_fpu_owner(void)
{
	return (mips_cpu.options & MIPS_CPU_FPU) && 
		test_and_set_thread_flag(TIF_USEDFPU); 
}

static inline void own_fpu(void)
{
	if (mips_cpu.options & MIPS_CPU_FPU) {
		__enable_fpu();
		KSTK_STATUS(current) |= ST0_CU1;
		set_thread_flag(TIF_USEDFPU); 
	}
}

static inline void loose_fpu(void)
{
	if (mips_cpu.options & MIPS_CPU_FPU) {
		KSTK_STATUS(current) &= ~ST0_CU1;
		clear_thread_flag(TIF_USEDFPU); 
		__disable_fpu();
	}
}

static inline void init_fpu(void)
{
	if (mips_cpu.options & MIPS_CPU_FPU) {
		_init_fpu();
	} else {
		fpu_emulator_init_fpu();
	}
}

static inline void save_fp(struct task_struct *tsk)
{
	if (mips_cpu.options & MIPS_CPU_FPU) 
		_save_fp(tsk);
}

static inline void restore_fp(struct task_struct *tsk)
{
	if (mips_cpu.options & MIPS_CPU_FPU) 
		_restore_fp(tsk);
}

static inline unsigned long *get_fpu_regs(struct task_struct *tsk)
{
	if (mips_cpu.options & MIPS_CPU_FPU) {
		if ((tsk == current) && is_fpu_owner()) 
			_save_fp(current);
		return (unsigned long *)&tsk->thread.fpu.hard.fp_regs[0];
	} else {
		return (unsigned long *)tsk->thread.fpu.soft.regs;
	}
}

#endif /* _ASM_FPU_H */
