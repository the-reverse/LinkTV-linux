/*
 * Setup pointers to hardware dependand routines.
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1996 by Ralf Baechle
 */
#include <asm/ptrace.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/timex.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/processor.h>
#include <asm/vector.h>

/*
 * Initial irq handlers.
 */
static void no_action(int cpl, void *dev_id, struct pt_regs *regs) { }

/*
 * IRQ2 is cascade interrupt to second interrupt controller
 */
static struct irqaction irq2  = { no_action, 0, 0, "cascade", NULL, NULL};

extern asmlinkage void sni_rm200_pci_handle_int(void);
extern asmlinkage void sni_fd_cacheflush(const void *addr, size_t size);
extern struct feature sni_rm200_pci_feature;
extern void sni_hard_reset_now(void);

static void
sni_irq_setup(void)
{
	set_except_vector(0, sni_rm200_pci_handle_int);
	request_region(0x20,0x20, "pic1");
	request_region(0xa0,0x20, "pic2");	
	setup_x86_irq(2, &irq2);
	/*
	 * IRQ0 seems to be the irq for PC style stuff.
	 * I don't know how to handle the debug button interrupt, so
	 * don't use this button yet or bad things happen ...
	 */
	set_cp0_status(ST0_IM, IE_IRQ0);
}

void (*board_time_init)(struct irqaction *irq);

static void sni_rm200_pci_time_init(struct irqaction *irq)
{
	/* set the clock to 100 Hz */
	outb_p(0x34,0x43);		/* binary, mode 2, LSB/MSB, ch 0 */
	outb_p(LATCH & 0xff , 0x40);	/* LSB */
	outb(LATCH >> 8 , 0x40);	/* MSB */
	setup_x86_irq(0, irq);
}

unsigned char aux_device_present;

void
sni_rm200_pci_setup(void)
{
	tag *atag;

	/*
	 * We just check if a tag_screen_info can be gathered
	 * in setup_arch(), if yes we don't proceed futher...
	 */
	atag = bi_TagFind(tag_screen_info);
	if (!atag) {
		/*
		 * If no, we try to find the tag_arc_displayinfo which is
		 * always created by Milo for an ARC box (for now Milo only
		 * works on ARC boxes :) -Stoned.
		 */
		atag = bi_TagFind(tag_arcdisplayinfo);
		if (atag) {
			screen_info.orig_x = 
				((mips_arc_DisplayInfo*)TAGVALPTR(atag))->cursor_x;
			screen_info.orig_y = 
				((mips_arc_DisplayInfo*)TAGVALPTR(atag))->cursor_y;
			screen_info.orig_video_cols  = 
				((mips_arc_DisplayInfo*)TAGVALPTR(atag))->columns;
			screen_info.orig_video_lines  = 
				((mips_arc_DisplayInfo*)TAGVALPTR(atag))->lines;
		}
	}

	irq_setup = sni_irq_setup;
	fd_cacheflush = sni_fd_cacheflush;	// Will go away
	feature = &sni_rm200_pci_feature;
	port_base = PORT_BASE_SNI;
	isa_slot_offset = 0xb0000000;
	request_region(0x00,0x20,"dma1");
	request_region(0x40,0x20,"timer");
	request_region(0x70,0x10,"rtc");
	board_time_init = sni_rm200_pci_time_init;

	hard_reset_now = sni_hard_reset_now;

	if (mips_machtype == MACH_SNI_RM200_PCI)
		EISA_bus = 1;
	aux_device_present = 0xaa;
}
