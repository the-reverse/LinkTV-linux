/*
 * Architecture-specific trap handling.
 *
 * Copyright (C) 1998-2000 Hewlett-Packard Co
 * Copyright (C) 1998-2000 David Mosberger-Tang <davidm@hpl.hp.com>
 */

/*
 * The fpu_fault() handler needs to be able to access and update all
 * floating point registers.  Those saved in pt_regs can be accessed
 * through that structure, but those not saved, will be accessed
 * directly.  To make this work, we need to ensure that the compiler
 * does not end up using a preserved floating point register on its
 * own.  The following achieves this by declaring preserved registers
 * that are not marked as "fixed" as global register variables.
 */
register double f2 asm ("f2"); register double f3 asm ("f3");
register double f4 asm ("f4"); register double f5 asm ("f5");

register long f16 asm ("f16"); register long f17 asm ("f17");
register long f18 asm ("f18"); register long f19 asm ("f19");
register long f20 asm ("f20"); register long f21 asm ("f21");
register long f22 asm ("f22"); register long f23 asm ("f23");

register double f24 asm ("f24"); register double f25 asm ("f25");
register double f26 asm ("f26"); register double f27 asm ("f27");
register double f28 asm ("f28"); register double f29 asm ("f29");
register double f30 asm ("f30"); register double f31 asm ("f31");

#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>

#ifdef CONFIG_KDB
# include <linux/kdb.h>
#endif

#include <asm/processor.h>
#include <asm/uaccess.h>

#include <asm/fpswa.h>

static fpswa_interface_t *fpswa_interface;

void __init
trap_init (void)
{
	printk("fpswa interface at %lx\n", ia64_boot_param.fpswa);
	if (ia64_boot_param.fpswa) {
#define OLD_FIRMWARE
#ifdef OLD_FIRMWARE
		/*
		 * HACK to work around broken firmware.  This code
		 * applies the label fixup to the FPSWA interface and
		 * works both with old and new (fixed) firmware.
		 */
		unsigned long addr = (unsigned long) __va(ia64_boot_param.fpswa);
		unsigned long gp_val = *(unsigned long *)(addr + 8);

		/* go indirect and indexed to get table address */
		addr = gp_val;
		gp_val = *(unsigned long *)(addr + 8);

		while (gp_val == *(unsigned long *)(addr + 8)) {
			*(unsigned long *)addr |= PAGE_OFFSET;
			*(unsigned long *)(addr + 8) |= PAGE_OFFSET;
			addr += 16;
		}
#endif
		/* FPSWA fixup: make the interface pointer a kernel virtual address: */
		fpswa_interface = __va(ia64_boot_param.fpswa);
	}
}

void
die_if_kernel (char *str, struct pt_regs *regs, long err)
{
	if (user_mode(regs)) {
#if 1
		/* XXX for debugging only */
		printk ("!!die_if_kernel: %s(%d): %s %ld\n",
			current->comm, current->pid, str, err);
		show_regs(regs);
#endif
		return;
	}

	printk("%s[%d]: %s %ld\n", current->comm, current->pid, str, err);

#ifdef CONFIG_KDB
	while (1) {
                kdb(KDB_REASON_PANIC, 0, regs);
                printk("Cant go anywhere from Panic!\n");
	}
#endif

	show_regs(regs);

	if (current->thread.flags & IA64_KERNEL_DEATH) {
		printk("die_if_kernel recursion detected.\n");
		sti();
		while (1);
	}
	current->thread.flags |= IA64_KERNEL_DEATH;
	do_exit(SIGSEGV);
}

void
ia64_bad_break (unsigned long break_num, struct pt_regs *regs)
{
	siginfo_t siginfo;

	/* gdb uses a break number of 0xccccc for debug breakpoints: */
	if (break_num != 0xccccc)
		die_if_kernel("Bad break", regs, break_num);

	siginfo.si_signo = SIGTRAP;
	siginfo.si_errno = break_num;	/* XXX is it legal to abuse si_errno like this? */
	siginfo.si_code = TRAP_BRKPT;
	send_sig_info(SIGTRAP, &siginfo, current);
}

/*
 * Unimplemented system calls.  This is called only for stuff that
 * we're supposed to implement but haven't done so yet.  Everything
 * else goes to sys_ni_syscall.
 */
asmlinkage long
ia64_ni_syscall (unsigned long arg0, unsigned long arg1, unsigned long arg2, unsigned long arg3,
		 unsigned long arg4, unsigned long arg5, unsigned long arg6, unsigned long arg7,
		 unsigned long stack)
{
	struct pt_regs *regs = (struct pt_regs *) &stack;

	printk("<sc%ld(%lx,%lx,%lx,%lx)>\n", regs->r15, arg0, arg1, arg2, arg3);
	return -ENOSYS;
}

/*
 * disabled_fp_fault() is called when a user-level process attempts to
 * access one of the registers f32..f127 while it doesn't own the
 * fp-high register partition.  When this happens, we save the current
 * fph partition in the task_struct of the fpu-owner (if necessary)
 * and then load the fp-high partition of the current task (if
 * necessary).
 */
static inline void
disabled_fph_fault (struct pt_regs *regs)
{
	struct task_struct *fpu_owner = ia64_get_fpu_owner();

	regs->cr_ipsr &= ~(IA64_PSR_DFH | IA64_PSR_MFH);
	if (fpu_owner != current) {
		ia64_set_fpu_owner(current);

		if (fpu_owner && ia64_psr(ia64_task_regs(fpu_owner))->mfh) {
			fpu_owner->thread.flags |= IA64_THREAD_FPH_VALID;
			__ia64_save_fpu(fpu_owner->thread.fph);
		}
		if ((current->thread.flags & IA64_THREAD_FPH_VALID) != 0) {
			__ia64_load_fpu(current->thread.fph);
		} else {
			__ia64_init_fpu();
		}
	}
}

static inline int
fp_emulate (int fp_fault, void *bundle, long *ipsr, long *fpsr, long *isr, long *pr, long *ifs,
	    struct pt_regs *regs)
{
	fp_state_t fp_state;
	fpswa_ret_t ret;
#ifdef FPSWA_BUG
	struct ia64_fpreg f6_15[10];
#endif

	if (!fpswa_interface)
		return -1;

	memset(&fp_state, 0, sizeof(fp_state_t));

	/*
	 * compute fp_state.  only FP registers f6 - f11 are used by the 
	 * kernel, so set those bits in the mask and set the low volatile
	 * pointer to point to these registers.
	 */
	fp_state.bitmask_low64 = 0xffc0;  /* bit6..bit15 */
#ifndef FPSWA_BUG
	fp_state.fp_state_low_volatile = &regs->f6;
#else
	f6_15[0] = regs->f6;
	f6_15[1] = regs->f7;
	f6_15[2] = regs->f8;
	f6_15[3] = regs->f9;
 	__asm__ ("stf.spill %0=f10" : "=m"(f6_15[4]));
 	__asm__ ("stf.spill %0=f11" : "=m"(f6_15[5]));
 	__asm__ ("stf.spill %0=f12" : "=m"(f6_15[6]));
 	__asm__ ("stf.spill %0=f13" : "=m"(f6_15[7]));
 	__asm__ ("stf.spill %0=f14" : "=m"(f6_15[8]));
 	__asm__ ("stf.spill %0=f15" : "=m"(f6_15[9]));
	fp_state.fp_state_low_volatile = (fp_state_low_volatile_t *) f6_15;
#endif
        /*
	 * unsigned long (*EFI_FPSWA) (
	 *      unsigned long    trap_type,
	 *	void             *Bundle,
	 *	unsigned long    *pipsr,
	 *	unsigned long    *pfsr,
	 *	unsigned long    *pisr,
	 *	unsigned long    *ppreds,
	 *	unsigned long    *pifs,
	 *	void             *fp_state);
	 */
	ret = (*fpswa_interface->fpswa)((unsigned long) fp_fault, bundle,
					(unsigned long *) ipsr, (unsigned long *) fpsr,
					(unsigned long *) isr, (unsigned long *) pr,
					(unsigned long *) ifs, &fp_state);
#ifdef FPSWA_BUG
 	__asm__ ("ldf.fill f10=%0" :: "m"(f6_15[4]));
 	__asm__ ("ldf.fill f11=%0" :: "m"(f6_15[5]));
 	__asm__ ("ldf.fill f12=%0" :: "m"(f6_15[6]));
 	__asm__ ("ldf.fill f13=%0" :: "m"(f6_15[7]));
 	__asm__ ("ldf.fill f14=%0" :: "m"(f6_15[8]));
 	__asm__ ("ldf.fill f15=%0" :: "m"(f6_15[9]));
	regs->f6 = f6_15[0];
	regs->f7 = f6_15[1];
	regs->f8 = f6_15[2];
	regs->f9 = f6_15[3];
#endif
	return ret.status;
}

/*
 * Handle floating-point assist faults and traps.
 */
static int
handle_fpu_swa (int fp_fault, struct pt_regs *regs, unsigned long isr)
{
	long exception, bundle[2];
	unsigned long fault_ip;
	static int fpu_swa_count = 0;
	static unsigned long last_time;

	fault_ip = regs->cr_iip;
	if (!fp_fault && (ia64_psr(regs)->ri == 0))
		fault_ip -= 16;
	if (copy_from_user(bundle, (void *) fault_ip, sizeof(bundle)))
		return -1;

	if (fpu_swa_count > 5 && jiffies - last_time > 5*HZ)
		fpu_swa_count = 0;
	if (++fpu_swa_count < 5) {
		last_time = jiffies;
		printk("%s(%d): floating-point assist fault at ip %016lx\n",
		       current->comm, current->pid, regs->cr_iip + ia64_psr(regs)->ri);
	}

	exception = fp_emulate(fp_fault, bundle, &regs->cr_ipsr, &regs->ar_fpsr, &isr, &regs->pr,
 			       &regs->cr_ifs, regs);
	if (fp_fault) {
		if (exception == 0) {
			/* emulation was successful */
 			ia64_increment_ip(regs);
		} else if (exception == -1) {
			printk("handle_fpu_swa: fp_emulate() returned -1\n");
			return -2;
		} else {
			/* is next instruction a trap? */
			if (exception & 2) {
				ia64_increment_ip(regs);
			}
			return -1;
		}
	} else {
		if (exception == -1) {
			printk("handle_fpu_swa: fp_emulate() returned -1\n");
			return -2;
		} else if (exception != 0) {
			/* raise exception */
			return -1;
		}
	}
	return 0;
}

void
ia64_fault (unsigned long vector, unsigned long isr, unsigned long ifa,
	    unsigned long iim, unsigned long itir, unsigned long arg5,
	    unsigned long arg6, unsigned long arg7, unsigned long stack)
{
	struct pt_regs *regs = (struct pt_regs *) &stack;
	unsigned long code, error = isr;
	struct siginfo siginfo;
	char buf[128];
	int result;
	static const char *reason[] = {
		"IA-64 Illegal Operation fault",
		"IA-64 Privileged Operation fault",
		"IA-64 Privileged Register fault",
		"IA-64 Reserved Register/Field fault",
		"Disabled Instruction Set Transition fault",
		"Unknown fault 5", "Unknown fault 6", "Unknown fault 7", "Illegal Hazard fault",
		"Unknown fault 9", "Unknown fault 10", "Unknown fault 11", "Unknown fault 12", 
		"Unknown fault 13", "Unknown fault 14", "Unknown fault 15"
	};

#if 0
	/* this is for minimal trust debugging; yeah this kind of stuff is useful at times... */

	if (vector != 25) {
		static unsigned long last_time;
		static char count;
		unsigned long n = vector;
		char buf[32], *cp;

		if (count > 5 && jiffies - last_time > 5*HZ)
			count = 0;

		if (count++ < 5) {
			last_time = jiffies;
			cp = buf + sizeof(buf);
			*--cp = '\0';
			while (n) {
				*--cp = "0123456789abcdef"[n & 0xf];
				n >>= 4;
			}
			printk("<0x%s>", cp);
		}
	}
#endif

	switch (vector) {
	      case 24: /* General Exception */
		code = (isr >> 4) & 0xf;
		sprintf(buf, "General Exception: %s%s", reason[code],
			(code == 3) ? ((isr & (1UL << 37))
				       ? " (RSE access)" : " (data access)") : "");
#ifndef CONFIG_ITANIUM_ASTEP_SPECIFIC
		if (code == 8) {
# ifdef CONFIG_IA64_PRINT_HAZARDS
			printk("%016lx:possible hazard, pr = %016lx\n", regs->cr_iip, regs->pr);
# endif
			return;
		}
#endif
		break;

	      case 25: /* Disabled FP-Register */
		if (isr & 2) {
			disabled_fph_fault(regs);
			return;
		}
		sprintf(buf, "Disabled FPL fault---not supposed to happen!");
		break;

	      case 29: /* Debug */
	      case 35: /* Taken Branch Trap */
	      case 36: /* Single Step Trap */
		switch (vector) {
		      case 29: siginfo.si_code = TRAP_BRKPT; break;
		      case 35: siginfo.si_code = TRAP_BRANCH; break;
		      case 36: siginfo.si_code = TRAP_TRACE; break;
		}
		siginfo.si_signo = SIGTRAP;
		siginfo.si_errno = 0;
		force_sig_info(SIGTRAP, &siginfo, current);
		return;

	      case 30: /* Unaligned fault */
		sprintf(buf, "Unaligned access in kernel mode---don't do this!");
		break;

	      case 32: /* fp fault */
	      case 33: /* fp trap */
		result = handle_fpu_swa((vector == 32) ? 1 : 0, regs, isr);
		if (result < 0) {
			siginfo.si_signo = SIGFPE;
			siginfo.si_errno = 0;
			siginfo.si_code = 0;	/* XXX fix me */
			siginfo.si_addr = (void *) (regs->cr_iip + ia64_psr(regs)->ri);
			send_sig_info(SIGFPE, &siginfo, current);
			if (result == -1)
				send_sig_info(SIGFPE, &siginfo, current);
			else
				force_sig(SIGFPE, current);
		}
		return;

	      case 34:		/* Unimplemented Instruction Address Trap */
		if (user_mode(regs)) {
			printk("Woah! Unimplemented Instruction Address Trap!\n");
			siginfo.si_code = ILL_BADIADDR;
			siginfo.si_signo = SIGILL;
			siginfo.si_errno = 0;
			force_sig_info(SIGILL, &siginfo, current);
			return;
		}
		sprintf(buf, "Unimplemented Instruction Address fault");
		break;

	      case 45:
		printk("Unexpected IA-32 exception\n");
		force_sig(SIGSEGV, current);
		return;

	      case 46:
		printk("Unexpected IA-32 intercept trap\n");
		force_sig(SIGSEGV, current);
		return;

	      case 47:
		sprintf(buf, "IA-32 Interruption Fault (int 0x%lx)", isr >> 16);
		break;

	      default:
		sprintf(buf, "Fault %lu", vector);
		break;
	}
	die_if_kernel(buf, regs, error);
	force_sig(SIGILL, current);
}
