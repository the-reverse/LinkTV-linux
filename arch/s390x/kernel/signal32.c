/*
 *  arch/s390/kernel/signal32.c
 *
 *  S390 version
 *    Copyright (C) 2000 IBM Deutschland Entwicklung GmbH, IBM Corporation
 *    Author(s): Denis Joseph Barrow (djbarrow@de.ibm.com,barrow_dj@yahoo.com)
 *               Gerhard Tonn (ton@de.ibm.com)                  
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  1997-11-28  Modified for POSIX.1b signals by Richard Henderson
 */

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/smp.h>
#include <linux/smp_lock.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/ptrace.h>
#include <linux/unistd.h>
#include <linux/stddef.h>
#include <linux/personality.h>
#include <asm/ucontext.h>
#include <asm/uaccess.h>
#include "linux32.h"

#define _BLOCKABLE (~(sigmask(SIGKILL) | sigmask(SIGSTOP)))

#define _USER_PSW_MASK32 0x0701C00080000000

typedef struct 
{
	__u8 callee_used_stack[__SIGNAL_FRAMESIZE32];
	struct sigcontext32 sc;
	_sigregs32 sregs;
	__u8 retcode[S390_SYSCALL_SIZE];
} sigframe32;

typedef struct 
{
	__u8 callee_used_stack[__SIGNAL_FRAMESIZE32];
	__u8 retcode[S390_SYSCALL_SIZE];
	struct siginfo32 info;
	struct ucontext32 uc;
} rt_sigframe32;

asmlinkage int FASTCALL(do_signal(struct pt_regs *regs, sigset_t *oldset));

int do_signal32(struct pt_regs *regs, sigset_t *oldset);

int copy_siginfo_to_user32(siginfo_t32 *to, siginfo_t *from)
{
	int err;

	if (!access_ok (VERIFY_WRITE, to, sizeof(siginfo_t32)))
		return -EFAULT;

	/* If you change siginfo_t structure, please be sure
	   this code is fixed accordingly.
	   It should never copy any pad contained in the structure
	   to avoid security leaks, but must copy the generic
	   3 ints plus the relevant union member.  
	   This routine must convert siginfo from 64bit to 32bit as well
	   at the same time.  */
	err = __put_user(from->si_signo, &to->si_signo);
	err |= __put_user(from->si_errno, &to->si_errno);
	err |= __put_user((short)from->si_code, &to->si_code);
	if (from->si_code < 0)
		err |= __copy_to_user(&to->_sifields._pad, &from->_sifields._pad, SI_PAD_SIZE);
	else {
		switch (from->si_code >> 16) {
		case __SI_KILL >> 16:
			err |= __put_user(from->si_pid, &to->si_pid);
			err |= __put_user(from->si_uid, &to->si_uid);
			break;
		case __SI_CHLD >> 16:
			err |= __put_user(from->si_pid, &to->si_pid);
			err |= __put_user(from->si_uid, &to->si_uid);
			err |= __put_user(from->si_utime, &to->si_utime);
			err |= __put_user(from->si_stime, &to->si_stime);
			err |= __put_user(from->si_status, &to->si_status);
			break;
		case __SI_FAULT >> 16:
			err |= __put_user(from->si_addr, &to->si_addr);
			break;
		case __SI_POLL >> 16:
		case __SI_TIMER >> 16:
			err |= __put_user(from->si_band, &to->si_band);
			err |= __put_user(from->si_fd, &to->si_fd);
			break;
		default:
			break;
		/* case __SI_RT: This is not generated by the kernel as of now.  */
		}
	}
	return err;
}

/*
 * Atomically swap in the new signal mask, and wait for a signal.
 */
asmlinkage int
sys32_sigsuspend(struct pt_regs * regs,int history0, int history1, old_sigset_t mask)
{
	sigset_t saveset;

	mask &= _BLOCKABLE;
	spin_lock_irq(&current->sigmask_lock);
	saveset = current->blocked;
	siginitset(&current->blocked, mask);
	recalc_sigpending(current);
	spin_unlock_irq(&current->sigmask_lock);
	regs->gprs[2] = -EINTR;

	while (1) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();
		if (do_signal32(regs, &saveset))
			return -EINTR;
	}
}

asmlinkage int
sys32_rt_sigsuspend(struct pt_regs * regs,sigset_t32 *unewset, size_t sigsetsize)
{
	sigset_t saveset, newset;
	sigset_t32 set32;

	/* XXX: Don't preclude handling different sized sigset_t's.  */
	if (sigsetsize != sizeof(sigset_t))
		return -EINVAL;

	if (copy_from_user(&set32, unewset, sizeof(set32)))
		return -EFAULT;
	switch (_NSIG_WORDS) {
	case 4: newset.sig[3] = set32.sig[6] + (((long)set32.sig[7]) << 32);
	case 3: newset.sig[2] = set32.sig[4] + (((long)set32.sig[5]) << 32);
	case 2: newset.sig[1] = set32.sig[2] + (((long)set32.sig[3]) << 32);
	case 1: newset.sig[0] = set32.sig[0] + (((long)set32.sig[1]) << 32);
	}
        sigdelsetmask(&newset, ~_BLOCKABLE);

        spin_lock_irq(&current->sigmask_lock);
        saveset = current->blocked;
        current->blocked = newset;
        recalc_sigpending(current);
        spin_unlock_irq(&current->sigmask_lock);
        regs->gprs[2] = -EINTR;

        while (1) {
                set_current_state(TASK_INTERRUPTIBLE);
                schedule();
                if (do_signal32(regs, &saveset))
                        return -EINTR;
        }
}                                                         

asmlinkage int
sys32_sigaction(int sig, const struct old_sigaction32 *act,
		 struct old_sigaction32 *oact)
{
        struct k_sigaction new_ka, old_ka;
        int ret;

        if (act) {
		old_sigset_t32 mask;
		if (verify_area(VERIFY_READ, act, sizeof(*act)) ||
		    __get_user((unsigned long)new_ka.sa.sa_handler, &act->sa_handler) ||
		    __get_user((unsigned long)new_ka.sa.sa_restorer, &act->sa_restorer))
			return -EFAULT;
		__get_user(new_ka.sa.sa_flags, &act->sa_flags);
		__get_user(mask, &act->sa_mask);
		siginitset(&new_ka.sa.sa_mask, mask);
        }

        ret = do_sigaction(sig, act ? &new_ka : NULL, oact ? &old_ka : NULL);

	if (!ret && oact) {
		if (verify_area(VERIFY_WRITE, oact, sizeof(*oact)) ||
		    __put_user((unsigned long)old_ka.sa.sa_handler, &oact->sa_handler) ||
		    __put_user((unsigned long)old_ka.sa.sa_restorer, &oact->sa_restorer))
			return -EFAULT;
		__put_user(old_ka.sa.sa_flags, &oact->sa_flags);
		__put_user(old_ka.sa.sa_mask.sig[0], &oact->sa_mask);
        }

	return ret;
}

int
do_sigaction(int sig, const struct k_sigaction *act, struct k_sigaction *oact);

asmlinkage long 
sys32_rt_sigaction(int sig, const struct sigaction32 *act,
	   struct sigaction32 *oact,  size_t sigsetsize)
{
	struct k_sigaction new_ka, old_ka;
	int ret;
	sigset_t32 set32;

	/* XXX: Don't preclude handling different sized sigset_t's.  */
	if (sigsetsize != sizeof(sigset_t32))
		return -EINVAL;

	if (act) {
		ret = get_user((unsigned long)new_ka.sa.sa_handler, &act->sa_handler);
		ret |= __copy_from_user(&set32, &act->sa_mask,
					sizeof(sigset_t32));
		switch (_NSIG_WORDS) {
		case 4: new_ka.sa.sa_mask.sig[3] = set32.sig[6]
				| (((long)set32.sig[7]) << 32);
		case 3: new_ka.sa.sa_mask.sig[2] = set32.sig[4]
				| (((long)set32.sig[5]) << 32);
		case 2: new_ka.sa.sa_mask.sig[1] = set32.sig[2]
				| (((long)set32.sig[3]) << 32);
		case 1: new_ka.sa.sa_mask.sig[0] = set32.sig[0]
				| (((long)set32.sig[1]) << 32);
		}
		ret |= __get_user(new_ka.sa.sa_flags, &act->sa_flags);
		
		if (ret)
			return -EFAULT;
	}

	ret = do_sigaction(sig, act ? &new_ka : NULL, oact ? &old_ka : NULL);

	if (!ret && oact) {
		switch (_NSIG_WORDS) {
		case 4:
			set32.sig[7] = (old_ka.sa.sa_mask.sig[3] >> 32);
			set32.sig[6] = old_ka.sa.sa_mask.sig[3];
		case 3:
			set32.sig[5] = (old_ka.sa.sa_mask.sig[2] >> 32);
			set32.sig[4] = old_ka.sa.sa_mask.sig[2];
		case 2:
			set32.sig[3] = (old_ka.sa.sa_mask.sig[1] >> 32);
			set32.sig[2] = old_ka.sa.sa_mask.sig[1];
		case 1:
			set32.sig[1] = (old_ka.sa.sa_mask.sig[0] >> 32);
			set32.sig[0] = old_ka.sa.sa_mask.sig[0];
		}
		ret = put_user((unsigned long)old_ka.sa.sa_handler, &oact->sa_handler);
		ret |= __copy_to_user(&oact->sa_mask, &set32,
				      sizeof(sigset_t32));
		ret |= __put_user(old_ka.sa.sa_flags, &oact->sa_flags);
	}

	return ret;
}

asmlinkage int
sys32_sigaltstack(const stack_t32 *uss, stack_t32 *uoss, struct pt_regs *regs)
{
	stack_t kss, koss;
	int ret, err = 0;
	mm_segment_t old_fs = get_fs();

	if (uss) {
		if (!access_ok(VERIFY_READ, uss, sizeof(*uss)))
			return -EFAULT;
		err |= __get_user(kss.ss_sp, &uss->ss_sp);
		err |= __get_user(kss.ss_size, &uss->ss_size);
		err |= __get_user(kss.ss_flags, &uss->ss_flags);
		if (err)
			return -EFAULT;
	}

	set_fs (KERNEL_DS);
	ret = do_sigaltstack(uss ? &kss : NULL , uoss ? &koss : NULL, regs->gprs[15]);
	set_fs (old_fs);

	if (!ret && uoss) {
		if (!access_ok(VERIFY_WRITE, uoss, sizeof(*uoss)))
			return -EFAULT;
		err |= __put_user(koss.ss_sp, &uoss->ss_sp);
		err |= __put_user(koss.ss_size, &uoss->ss_size);
		err |= __put_user(koss.ss_flags, &uoss->ss_flags);
		if (err)
			return -EFAULT;
	}
	return ret;
}

static int save_sigregs32(struct pt_regs *regs,_sigregs32 *sregs)
{
	int err = 0;
	s390_fp_regs fpregs;
	int i;

	for(i=0; i<NUM_GPRS; i++) 
		err |= __put_user(regs->gprs[i], &sregs->regs.gprs[i]);  
	for(i=0; i<NUM_ACRS; i++)
		err |= __put_user(regs->acrs[i], &sregs->regs.acrs[i]);  
	err |= __copy_to_user(&sregs->regs.psw.mask, &regs->psw.mask, 4);
	err |= __copy_to_user(&sregs->regs.psw.addr, ((char*)&regs->psw.addr)+4, 4);
	if(!err)
	{
		save_fp_regs(&fpregs);
		__put_user(fpregs.fpc, &sregs->fpregs.fpc);
		for(i=0; i<NUM_FPRS; i++)
			err |= __put_user(fpregs.fprs[i].d, &sregs->fpregs.fprs[i].d);  
	}
	return(err);
	
}

static int restore_sigregs32(struct pt_regs *regs,_sigregs32 *sregs)
{
	int err = 0;
	s390_fp_regs fpregs;
	psw_t saved_psw=regs->psw;
	int i;

	for(i=0; i<NUM_GPRS; i++)
		err |= __get_user(regs->gprs[i], &sregs->regs.gprs[i]);  
	for(i=0; i<NUM_ACRS; i++)
		err |= __get_user(regs->acrs[i], &sregs->regs.acrs[i]);  
	err |= __copy_from_user(&regs->psw.mask, &sregs->regs.psw.mask, 4);
	err |= __copy_from_user(((char*)&regs->psw.addr)+4, &sregs->regs.psw.addr, 4);

	if(!err)
	{
		regs->trap = -1;		/* disable syscall checks */
		regs->psw.mask=(saved_psw.mask&~PSW_MASK_DEBUGCHANGE)|
		(regs->psw.mask&PSW_MASK_DEBUGCHANGE);
		regs->psw.addr=(saved_psw.addr&~PSW_ADDR_DEBUGCHANGE)|
		(regs->psw.addr&PSW_ADDR_DEBUGCHANGE);
		__get_user(fpregs.fpc, &sregs->fpregs.fpc);
                for(i=0; i<NUM_FPRS; i++)
                        err |= __get_user(fpregs.fprs[i].d, &sregs->fpregs.fprs[i].d);              
		if(!err)
			restore_fp_regs(&fpregs);
	}
	return(err);
}

asmlinkage long sys32_sigreturn(struct pt_regs *regs)
{
	sigframe32 *frame = (sigframe32 *)regs->gprs[15];
	sigset_t set;

	if (verify_area(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__copy_from_user(&set.sig, &frame->sc.oldmask, _SIGMASK_COPY_SIZE32))
		goto badframe;

	sigdelsetmask(&set, ~_BLOCKABLE);
	spin_lock_irq(&current->sigmask_lock);
	current->blocked = set;
	recalc_sigpending(current);
	spin_unlock_irq(&current->sigmask_lock);

	if (restore_sigregs32(regs, &frame->sregs))
		goto badframe;

	return regs->gprs[2];

badframe:
	force_sig(SIGSEGV, current);
	return 0;
}	

asmlinkage long sys32_rt_sigreturn(struct pt_regs *regs)
{
	rt_sigframe32 *frame = (rt_sigframe32 *)regs->gprs[15];
	sigset_t set;
	stack_t st;
	int err;
	mm_segment_t old_fs = get_fs();

	if (verify_area(VERIFY_READ, frame, sizeof(*frame)))
		goto badframe;
	if (__copy_from_user(&set, &frame->uc.uc_sigmask, sizeof(set)))
		goto badframe;

	sigdelsetmask(&set, ~_BLOCKABLE);
	spin_lock_irq(&current->sigmask_lock);
	current->blocked = set;
	recalc_sigpending(current);
	spin_unlock_irq(&current->sigmask_lock);

	if (restore_sigregs32(regs, &frame->uc.uc_mcontext))
		goto badframe;

	err = __get_user(st.ss_sp, &frame->uc.uc_stack.ss_sp);
	st.ss_sp = (void *) A((unsigned long)st.ss_sp);
	err |= __get_user(st.ss_size, &frame->uc.uc_stack.ss_size);
	err |= __get_user(st.ss_flags, &frame->uc.uc_stack.ss_flags);
	if (err)
		goto badframe; 

	/* It is more difficult to avoid calling this function than to
	   call it and ignore errors.  */
	set_fs (KERNEL_DS);   
	do_sigaltstack(&st, NULL, regs->gprs[15]);
	set_fs (old_fs);

	return regs->gprs[2];

badframe:
        force_sig(SIGSEGV, current);
        return 0;
}	

/*
 * Set up a signal frame.
 */


/*
 * Determine which stack to use..
 */
static inline void *
get_sigframe(struct k_sigaction *ka, struct pt_regs * regs, size_t frame_size)
{
	unsigned long sp;

	/* Default to using normal stack */
	sp = (unsigned long) A(regs->gprs[15]);

	/* This is the X/Open sanctioned signal stack switching.  */
	if (ka->sa.sa_flags & SA_ONSTACK) {
		if (! on_sig_stack(sp))
			sp = current->sas_ss_sp + current->sas_ss_size;
	}

	/* This is the legacy signal stack switching. */
	else if (!user_mode(regs) &&
		 !(ka->sa.sa_flags & SA_RESTORER) &&
		 ka->sa.sa_restorer) {
		sp = (unsigned long) ka->sa.sa_restorer;
	}

	return (void *)((sp - frame_size) & -8ul);
}

static inline int map_signal(int sig)
{
	if (current->exec_domain
	    && current->exec_domain->signal_invmap
	    && sig < 32)
		return current->exec_domain->signal_invmap[sig];
        else
		return sig;
}

static void setup_frame32(int sig, struct k_sigaction *ka,
			sigset_t *set, struct pt_regs * regs)
{
	sigframe32 *frame = get_sigframe(ka, regs, sizeof(sigframe32));
	if (!access_ok(VERIFY_WRITE, frame, sizeof(sigframe32)))
		goto give_sigsegv;

	if (__copy_to_user(&frame->sc.oldmask, &set->sig, _SIGMASK_COPY_SIZE32))
		goto give_sigsegv;

	if (save_sigregs32(regs, &frame->sregs))
		goto give_sigsegv;
	if (__put_user(&frame->sregs, &frame->sc.sregs))
		goto give_sigsegv;

	/* Set up to return from userspace.  If provided, use a stub
	   already in userspace.  */
	if (ka->sa.sa_flags & SA_RESTORER) {
		regs->gprs[14] = FIX_PSW(ka->sa.sa_restorer);
	} else {
		regs->gprs[14] = FIX_PSW(frame->retcode);
		if (__put_user(S390_SYSCALL_OPCODE | __NR_sigreturn,
		               (u16 *)(frame->retcode)))
			goto give_sigsegv;
        }

	/* Set up registers for signal handler */
	regs->gprs[15] = (addr_t)frame;
	regs->psw.addr = FIX_PSW(ka->sa.sa_handler);
	regs->psw.mask = _USER_PSW_MASK32;

	regs->gprs[2] = map_signal(sig);
	regs->gprs[3] = (addr_t)&frame->sc;

	/* We forgot to include these in the sigcontext.
	   To avoid breaking binary compatibility, they are passed as args. */
	regs->gprs[4] = current->thread.trap_no;
	regs->gprs[5] = current->thread.prot_addr;
	return;

give_sigsegv:
	if (sig == SIGSEGV)
		ka->sa.sa_handler = SIG_DFL;
	force_sig(SIGSEGV, current);
}

static void setup_rt_frame32(int sig, struct k_sigaction *ka, siginfo_t *info,
			   sigset_t *set, struct pt_regs * regs)
{
	int err = 0;
	rt_sigframe32 *frame = get_sigframe(ka, regs, sizeof(rt_sigframe32));
	if (!access_ok(VERIFY_WRITE, frame, sizeof(rt_sigframe32)))
		goto give_sigsegv;

	if (copy_siginfo_to_user32(&frame->info, info))
		goto give_sigsegv;

	/* Create the ucontext.  */
	err |= __put_user(0, &frame->uc.uc_flags);
	err |= __put_user(0, &frame->uc.uc_link);
	err |= __put_user(current->sas_ss_sp, &frame->uc.uc_stack.ss_sp);
	err |= __put_user(sas_ss_flags(regs->gprs[15]),
	                  &frame->uc.uc_stack.ss_flags);
	err |= __put_user(current->sas_ss_size, &frame->uc.uc_stack.ss_size);
	err |= save_sigregs32(regs, &frame->uc.uc_mcontext);
	err |= __copy_to_user(&frame->uc.uc_sigmask, set, sizeof(*set));
	if (err)
		goto give_sigsegv;

	/* Set up to return from userspace.  If provided, use a stub
	   already in userspace.  */
	if (ka->sa.sa_flags & SA_RESTORER) {
		regs->gprs[14] = FIX_PSW(ka->sa.sa_restorer);
	} else {
		regs->gprs[14] = FIX_PSW(frame->retcode);
		err |= __put_user(S390_SYSCALL_OPCODE | __NR_rt_sigreturn,
		                  (u16 *)(frame->retcode));
	}

	/* Set up registers for signal handler */
	regs->gprs[15] = (addr_t)frame;
	regs->psw.addr = FIX_PSW(ka->sa.sa_handler);
	regs->psw.mask = _USER_PSW_MASK32;

	regs->gprs[2] = map_signal(sig);
	regs->gprs[3] = (addr_t)&frame->info;
	regs->gprs[4] = (addr_t)&frame->uc;
	return;

give_sigsegv:
	if (sig == SIGSEGV)
		ka->sa.sa_handler = SIG_DFL;
	force_sig(SIGSEGV, current);
}

/*
 * OK, we're invoking a handler
 */	

static void
handle_signal32(unsigned long sig, struct k_sigaction *ka,
	      siginfo_t *info, sigset_t *oldset, struct pt_regs * regs)
{
	/* Are we from a system call? */
	if (regs->trap == __LC_SVC_OLD_PSW) {
		/* If so, check system call restarting.. */
		switch (regs->gprs[2]) {
			case -ERESTARTNOHAND:
				regs->gprs[2] = -EINTR;
				break;

			case -ERESTARTSYS:
				if (!(ka->sa.sa_flags & SA_RESTART)) {
					regs->gprs[2] = -EINTR;
					break;
				}
			/* fallthrough */
			case -ERESTARTNOINTR:
				regs->gprs[2] = regs->orig_gpr2;
				regs->psw.addr -= 2;
		}
	}

	/* Set up the stack frame */
	if (ka->sa.sa_flags & SA_SIGINFO)
		setup_rt_frame32(sig, ka, info, oldset, regs);
	else
		setup_frame32(sig, ka, oldset, regs);

	if (ka->sa.sa_flags & SA_ONESHOT)
		ka->sa.sa_handler = SIG_DFL;

	if (!(ka->sa.sa_flags & SA_NODEFER)) {
		spin_lock_irq(&current->sigmask_lock);
		sigorsets(&current->blocked,&current->blocked,&ka->sa.sa_mask);
		sigaddset(&current->blocked,sig);
		recalc_sigpending(current);
		spin_unlock_irq(&current->sigmask_lock);
	}
}

/*
 * Note that 'init' is a special process: it doesn't get signals it doesn't
 * want to handle. Thus you cannot kill init even with a SIGKILL even by
 * mistake.
 *
 * Note that we go through the signals twice: once to check the signals that
 * the kernel can handle, and then we build all the user-level signal handling
 * stack-frames in one go after that.
 */
int do_signal32(struct pt_regs *regs, sigset_t *oldset)
{
	siginfo_t info;
	struct k_sigaction *ka;

	/*
	 * We want the common case to go fast, which
	 * is why we may in certain cases get here from
	 * kernel mode. Just return without doing anything
	 * if so.
	 */
	if (!user_mode(regs))
		return 1;

	if (!oldset)
		oldset = &current->blocked;

	for (;;) {
		unsigned long signr;

		spin_lock_irq(&current->sigmask_lock);
		signr = dequeue_signal(&current->blocked, &info);
		spin_unlock_irq(&current->sigmask_lock);

		if (!signr)
			break;

		if ((current->ptrace & PT_PTRACED) && signr != SIGKILL) {
			/* Let the debugger run.  */
			current->exit_code = signr;
			set_current_state(TASK_STOPPED);
			notify_parent(current, SIGCHLD);
			schedule();

			/* We're back.  Did the debugger cancel the sig?  */
			if (!(signr = current->exit_code))
				continue;
			current->exit_code = 0;

			/* The debugger continued.  Ignore SIGSTOP.  */
			if (signr == SIGSTOP)
				continue;

			/* Update the siginfo structure.  Is this good?  */
			if (signr != info.si_signo) {
				info.si_signo = signr;
				info.si_errno = 0;
				info.si_code = SI_USER;
				info.si_pid = current->p_pptr->pid;
				info.si_uid = current->p_pptr->uid;
			}

			/* If the (new) signal is now blocked, requeue it.  */
			if (sigismember(&current->blocked, signr)) {
				send_sig_info(signr, &info, current);
				continue;
			}
		}

		ka = &current->sig->action[signr-1];
		if (ka->sa.sa_handler == SIG_IGN) {
			if (signr != SIGCHLD)
				continue;
			/* Check for SIGCHLD: it's special.  */
			while (sys_wait4(-1, NULL, WNOHANG, NULL) > 0)
				/* nothing */;
			continue;
		}

		if (ka->sa.sa_handler == SIG_DFL) {
			int exit_code = signr;

			/* Init gets no signals it doesn't want.  */
			if (current->pid == 1)
				continue;

			switch (signr) {
			case SIGCONT: case SIGCHLD: case SIGWINCH:
				continue;

			case SIGTSTP: case SIGTTIN: case SIGTTOU:
				if (is_orphaned_pgrp(current->pgrp))
					continue;
				/* FALLTHRU */

			case SIGSTOP:
				set_current_state(TASK_STOPPED);
				current->exit_code = signr;
				if (!(current->p_pptr->sig->action[SIGCHLD-1].sa.sa_flags & SA_NOCLDSTOP))
					notify_parent(current, SIGCHLD);
				schedule();
				continue;

			case SIGQUIT: case SIGILL: case SIGTRAP:
			case SIGABRT: case SIGFPE: case SIGSEGV:
			case SIGBUS: case SIGSYS: case SIGXCPU: case SIGXFSZ:
                                if (do_coredump(signr, regs))
                                        exit_code |= 0x80;
                                /* FALLTHRU */

			default:
				sig_exit(signr, exit_code, &info);
				/* NOTREACHED */
			}
		}

		/* Whee!  Actually deliver the signal.  */
		handle_signal32(signr, ka, &info, oldset, regs);
		return 1;
	}

	/* Did we come from a system call? */
	if ( regs->trap == __LC_SVC_OLD_PSW /* System Call! */ ) {
		/* Restart the system call - no handlers present */
		if (regs->gprs[2] == -ERESTARTNOHAND ||
		    regs->gprs[2] == -ERESTARTSYS ||
		    regs->gprs[2] == -ERESTARTNOINTR) {
			regs->gprs[2] = regs->orig_gpr2;
			regs->psw.addr -= 2;
		}
	}
	return 0;
}
