/*
 * This file is subject to the terms and conditions of the GNU General
 * Public License.  See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * Copyright (C) 2000 - 2001 by Kanoj Sarcar (kanoj@sgi.com)
 * Copyright (C) 2000 - 2001 by Silicon Graphics, Inc.
 */
#include <linux/init.h>
#include <linux/sched.h>
#include <asm/page.h>
#include <asm/processor.h>
#include <asm/sn/arch.h>
#include <asm/sn/gda.h>
#include <asm/sn/intr.h>
#include <asm/sn/klconfig.h>
#include <asm/sn/launch.h>
#include <asm/sn/mapped_kernel.h>
#include <asm/sn/sn_private.h>
#include <asm/sn/types.h>
#include <asm/sn/sn0/hubpi.h>
#include <asm/sn/sn0/hubio.h>
#include <asm/sn/sn0/ip27.h>

static atomic_t numstarted = ATOMIC_INIT(1);
static int maxcpus;

/*
 * Takes as first input the PROM assigned cpu id, and the kernel
 * assigned cpu id as the second.
 */
static void alloc_cpupda(cpuid_t cpu, int cpunum)
{
	cnodeid_t node = get_cpu_cnode(cpu);
	nasid_t nasid = COMPACT_TO_NASID_NODEID(node);

	cputonasid(cpunum) = nasid;
	cputocnode(cpunum) = node;
	cputoslice(cpunum) = get_cpu_slice(cpu);
}

static nasid_t get_actual_nasid(lboard_t *brd)
{
	klhub_t *hub;

	if (!brd)
		return INVALID_NASID;

	/* find out if we are a completely disabled brd. */
	hub  = (klhub_t *)find_first_component(brd, KLSTRUCT_HUB);
	if (!hub)
		return INVALID_NASID;
	if (!(hub->hub_info.flags & KLINFO_ENABLE))	/* disabled node brd */
		return hub->hub_info.physid;
	else
		return brd->brd_nasid;
}

static int do_cpumask(cnodeid_t cnode, nasid_t nasid, int highest)
{
	static int tot_cpus_found = 0;
	lboard_t *brd;
	klcpu_t *acpu;
	int cpus_found = 0;
	cpuid_t cpuid;

	brd = find_lboard((lboard_t *)KL_CONFIG_INFO(nasid), KLTYPE_IP27);

	do {
		acpu = (klcpu_t *)find_first_component(brd, KLSTRUCT_CPU);
		while (acpu) {
			cpuid = acpu->cpu_info.virtid;
			/* cnode is not valid for completely disabled brds */
			if (get_actual_nasid(brd) == brd->brd_nasid)
				cpuid_to_compact_node[cpuid] = cnode;
			if (cpuid > highest)
				highest = cpuid;
			/* Only let it join in if it's marked enabled */
			if ((acpu->cpu_info.flags & KLINFO_ENABLE) &&
			    (tot_cpus_found != NR_CPUS)) {
				cpu_set(cpuid, phys_cpu_present_map);
				alloc_cpupda(cpuid, cpus_found);
				cpus_found++;
				tot_cpus_found++;
			}
			acpu = (klcpu_t *)find_component(brd, (klinfo_t *)acpu,
								KLSTRUCT_CPU);
		}
		brd = KLCF_NEXT(brd);
		if (!brd)
			break;

		brd = find_lboard(brd, KLTYPE_IP27);
	} while (brd);

	return highest;
}

static cpuid_t cpu_node_probe(void)
{
	int i, highest = 0;
	gda_t *gdap = GDA;

	/*
	 * Initialize the arrays to invalid nodeid (-1)
	 */
	for (i = 0; i < MAX_COMPACT_NODES; i++)
		compact_to_nasid_node[i] = INVALID_NASID;
	for (i = 0; i < MAX_NASIDS; i++)
		nasid_to_compact_node[i] = INVALID_CNODEID;
	for (i = 0; i < MAXCPUS; i++)
		cpuid_to_compact_node[i] = INVALID_CNODEID;

	numnodes = 0;
	for (i = 0; i < MAX_COMPACT_NODES; i++) {
		nasid_t nasid = gdap->g_nasidtable[i];
		if (nasid == INVALID_NASID)
			break;
		compact_to_nasid_node[i] = nasid;
		nasid_to_compact_node[nasid] = i;
		numnodes++;
		highest = do_cpumask(i, nasid, highest);
	}

	/*
	 * Cpus are numbered in order of cnodes. Currently, disabled
	 * cpus are not numbered.
	 */

	return highest + 1;
}

void __init prom_build_cpu_map(void)
{
	/*
	 * Probe for all CPUs - this creates the cpumask and
	 * sets up the mapping tables.
	 */
	maxcpus = cpu_node_probe();
	printk("Discovered %d cpus on %d nodes\n", maxcpus, numnodes);
}

static void intr_clear_bits(nasid_t nasid, volatile hubreg_t *pend,
	int base_level, char *name)
{
	volatile hubreg_t bits;
	int i;

	/* Check pending interrupts */
	if ((bits = HUB_L(pend)) != 0)
		for (i = 0; i < N_INTPEND_BITS; i++)
			if (bits & (1 << i))
				LOCAL_HUB_CLR_INTR(base_level + i);
}

static void intr_clear_all(nasid_t nasid)
{
	REMOTE_HUB_S(nasid, PI_INT_MASK0_A, 0);
	REMOTE_HUB_S(nasid, PI_INT_MASK0_B, 0);
	REMOTE_HUB_S(nasid, PI_INT_MASK1_A, 0);
	REMOTE_HUB_S(nasid, PI_INT_MASK1_B, 0);
	intr_clear_bits(nasid, REMOTE_HUB_ADDR(nasid, PI_INT_PEND0),
		INT_PEND0_BASELVL, "INT_PEND0");
	intr_clear_bits(nasid, REMOTE_HUB_ADDR(nasid, PI_INT_PEND1),
		INT_PEND1_BASELVL, "INT_PEND1");
}

static void sn_mp_setup(void)
{
	cnodeid_t	cnode;
//	cpuid_t		cpu;

	for (cnode = 0; cnode < numnodes; cnode++) {
//		init_platform_nodepda();
		intr_clear_all(COMPACT_TO_NASID_NODEID(cnode));
	}
//	for (cpu = 0; cpu < maxcpus; cpu++)
//		init_platform_pda();
}

void __init prom_prepare_cpus(unsigned int max_cpus)
{
	sn_mp_setup();
	/* Master has already done per_cpu_init() */
	install_cpuintr(smp_processor_id());
#if 0
	bte_lateinit();
	ecc_init();
#endif

	replicate_kernel_text(numnodes);

	/*
	 * Assumption to be fixed: we're always booted on logical / physical
	 * processor 0.  While we're always running on logical processor 0
	 * this still means this is physical processor zero; it might for
	 * example be disabled in the firwware.
	 */
	alloc_cpupda(0, 0);
}

/*
 * Launch a slave into smp_bootstrap().  It doesn't take an argument, and we
 * set sp to the kernel stack of the newly created idle process, gp to the proc
 * struct so that current_thread_info() will work.
 */
void __init prom_boot_secondary(int cpu, struct task_struct *idle)
{
	unsigned long gp = (unsigned long) idle->thread_info;
	unsigned long sp = gp + THREAD_SIZE - 32;

	LAUNCH_SLAVE(cputonasid(cpu),cputoslice(cpu),
		(launch_proc_t)MAPPED_KERN_RW_TO_K0(smp_bootstrap),
		0, (void *) sp, (void *) gp);
}

void prom_init_secondary(void)
{
	int cpu = smp_processor_id();
	cnodeid_t cnode = get_compact_nodeid();

#if 0
	intr_init();
#endif
	clear_c0_status(ST0_IM);
	per_hub_init(cnode);
	cpu_time_init();
	if (smp_processor_id())	/* master can't do this early, no kmalloc */
		install_cpuintr(cpu);
	/* Install our NMI handler if symmon hasn't installed one. */
	install_cpu_nmi_handler(cputoslice(cpu));
#if 0
	install_tlbintr(cpu);
#endif
	set_c0_status(SRB_DEV0 | SRB_DEV1);
	local_irq_enable();
	atomic_inc(&numstarted);
}

/* XXXKW implement prom_init_secondary() and prom_smp_finish to share
 * start_secondary with kernel/smp.c; otherwise, roll your own. */

void __init prom_cpus_done(void)
{
	cnodeid_t cnode;

#ifdef LATER
	Wait logic goes here.
#endif
	for (cnode = 0; cnode < numnodes; cnode++) {
#if 0
		if (cnodetocpu(cnode) == -1) {
			printk("Initializing headless hub,cnode %d", cnode);
			per_hub_init(cnode);
		}
#endif
	}
#if 0
	cpu_io_setup();
	init_mfhi_war();
#endif
}

void prom_smp_finish(void)
{
}
