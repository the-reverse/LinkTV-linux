/* $Id: irq_ipr.c,v 1.6 2000/05/14 08:41:25 gniibe Exp $
 *
 * linux/arch/sh/kernel/irq_ipr.c
 *
 * Copyright (C) 1999  Niibe Yutaka & Takeshi Yaegashi
 * Copyright (C) 2000  Kazumoto Kojima
 *
 * Interrupt handling for IPR-based IRQ.
 *
 * Supported system:
 *	On-chip supporting modules (TMU, RTC, etc.).
 *	On-chip supporting modules for SH7709/SH7709A/SH7729.
 *	Hitachi SolutionEngine external I/O:
 *		MS7709SE01, MS7709ASE01, and MS7750SE01
 *
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/irq.h>

#include <asm/system.h>
#include <asm/io.h>

struct ipr_data {
	unsigned int addr;	/* Address of Interrupt Priority Register */
	int shift;		/* Shifts of the 16-bit data */
	int priority;		/* The priority */
};
static struct ipr_data ipr_data[NR_IRQS];

void set_ipr_data(unsigned int irq, unsigned int addr, int pos, int priority)
{
	ipr_data[irq].addr = addr;
	ipr_data[irq].shift = pos*4; /* POSition (0-3) x 4 means shift */
	ipr_data[irq].priority = priority;
}

static void enable_ipr_irq(unsigned int irq);
void disable_ipr_irq(unsigned int irq);

/* shutdown is same as "disable" */
#define shutdown_ipr_irq disable_ipr_irq

static void mask_and_ack_ipr(unsigned int);
static void end_ipr_irq(unsigned int irq);

static unsigned int startup_ipr_irq(unsigned int irq)
{ 
	enable_ipr_irq(irq);
	return 0; /* never anything pending */
}

static struct hw_interrupt_type ipr_irq_type = {
	"IPR-based-IRQ",
	startup_ipr_irq,
	shutdown_ipr_irq,
	enable_ipr_irq,
	disable_ipr_irq,
	mask_and_ack_ipr,
	end_ipr_irq
};

void disable_ipr_irq(unsigned int irq)
{
	unsigned long val, flags;
	unsigned int addr = ipr_data[irq].addr;
	unsigned short mask = 0xffff ^ (0x0f << ipr_data[irq].shift);

	/* Set the priority in IPR to 0 */
	save_and_cli(flags);
	val = ctrl_inw(addr);
	val &= mask;
	ctrl_outw(val, addr);
	restore_flags(flags);
}

static void enable_ipr_irq(unsigned int irq)
{
	unsigned long val, flags;
	unsigned int addr = ipr_data[irq].addr;
	int priority = ipr_data[irq].priority;
	unsigned short value = (priority << ipr_data[irq].shift);

	/* Set priority in IPR back to original value */
	save_and_cli(flags);
	val = ctrl_inw(addr);
	val |= value;
	ctrl_outw(val, addr);
	restore_flags(flags);
}

void make_ipr_irq(unsigned int irq)
{
	disable_irq_nosync(irq);
	irq_desc[irq].handler = &ipr_irq_type;
	disable_ipr_irq(irq);
}

static void mask_and_ack_ipr(unsigned int irq)
{
	disable_ipr_irq(irq);

#ifdef CONFIG_CPU_SUBTYPE_SH7709
	/* This is needed when we use edge triggered setting */
	/* XXX: Is it really needed? */
	if (IRQ0_IRQ <= irq && irq <= IRQ5_IRQ) {
		/* Clear external interrupt request */
		int a = ctrl_inb(INTC_IRR0);
		a &= ~(1 << (irq - IRQ0_IRQ));
		ctrl_outb(a, INTC_IRR0);
	}
#endif
}

static void end_ipr_irq(unsigned int irq)
{
	enable_ipr_irq(irq);
}

void __init init_IRQ(void)
{
	int i;

	for (i = TIMER_IRQ; i < NR_IRQS; i++) {
		irq_desc[i].handler = &ipr_irq_type;
	}

	set_ipr_data(TIMER_IRQ, TIMER_IPR_ADDR, TIMER_IPR_POS, TIMER_PRIORITY);
	set_ipr_data(RTC_IRQ, RTC_IPR_ADDR, RTC_IPR_POS, RTC_PRIORITY);

#ifdef CONFIG_CPU_SUBTYPE_SH7709
	/*
	 * Initialize the Interrupt Controller (INTC)
	 * registers to their power on values
	 */ 
#if 0
	/*
	 * XXX: I think that this is the job of boot loader. -- gniibe
	 *
	 * When Takeshi released new boot loader following setting
	 * will be removed shortly.
	 */
	ctrl_outb(0, INTC_IRR0);
	ctrl_outb(0, INTC_IRR1);
	ctrl_outb(0, INTC_IRR2);

	ctrl_outw(0, INTC_ICR0);
	ctrl_outw(0, INTC_ICR1);/* Really? 0x4000?*/
	ctrl_outw(0, INTC_ICR2);
	ctrl_outw(0, INTC_INTER);
	ctrl_outw(0, INTC_IPRA);
	ctrl_outw(0, INTC_IPRB);
	ctrl_outw(0, INTC_IPRC);
	ctrl_outw(0, INTC_IPRD);
	ctrl_outw(0, INTC_IPRE);
#endif

	/*
	 * Enable external irq (INTC IRQ mode).
	 * You should set corresponding bits of PFC to "00"
	 * to enable these interrupts.
	 */
	set_ipr_data(IRQ0_IRQ, IRQ0_IRP_ADDR, IRQ0_IRP_POS, IRQ0_PRIORITY);
	set_ipr_data(IRQ1_IRQ, IRQ1_IRP_ADDR, IRQ1_IRP_POS, IRQ1_PRIORITY);
	set_ipr_data(IRQ2_IRQ, IRQ2_IRP_ADDR, IRQ2_IRP_POS, IRQ2_PRIORITY);
	set_ipr_data(IRQ3_IRQ, IRQ3_IRP_ADDR, IRQ3_IRP_POS, IRQ3_PRIORITY);
	set_ipr_data(IRQ4_IRQ, IRQ4_IRP_ADDR, IRQ4_IRP_POS, IRQ4_PRIORITY);
	set_ipr_data(IRQ5_IRQ, IRQ5_IRP_ADDR, IRQ5_IRP_POS, IRQ5_PRIORITY);
#endif /* CONFIG_CPU_SUBTYPE_SH7709 */
}
