/*
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 2004, 2005 Ralf Baechle
 * Copyright (C) 2005 MIPS Technologies, Inc.
 */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/oprofile.h>
#include <linux/smp.h>
#include <asm/cpu-info.h>

#include "op_impl.h"

extern struct op_mips_model op_model_mipsxx __attribute__((weak));
extern struct op_mips_model op_model_rm9000 __attribute__((weak));
extern struct op_mips_model op_model_venus  __attribute__((weak));

static struct op_mips_model *model;

static struct op_counter_config ctr[20];

static int op_mips_setup(void)
{
	/* Pre-compute the values to stuff in the hardware registers.  */
	model->reg_setup(ctr);

	/* Configure the registers on all cpus.  */
	on_each_cpu(model->cpu_setup, 0, 0, 1);

        return 0;
}

static int op_mips_create_files(struct super_block * sb, struct dentry * root)
{
	int i;

	for (i = 0; i < model->num_counters; ++i) {
		struct dentry *dir;
		char buf[3];

		snprintf(buf, sizeof buf, "%d", i);
		dir = oprofilefs_mkdir(sb, root, buf);

		oprofilefs_create_ulong(sb, dir, "enabled", &ctr[i].enabled);
		oprofilefs_create_ulong(sb, dir, "event", &ctr[i].event);
		oprofilefs_create_ulong(sb, dir, "count", &ctr[i].count);
		oprofilefs_create_ulong(sb, dir, "kernel", &ctr[i].kernel);
		oprofilefs_create_ulong(sb, dir, "user", &ctr[i].user);
		oprofilefs_create_ulong(sb, dir, "exl", &ctr[i].exl);
		/* Dummy.  */
		oprofilefs_create_ulong(sb, dir, "unit_mask", &ctr[i].unit_mask);
	}

	return 0;
}

static int op_mips_start(void)
{
	on_each_cpu(model->cpu_start, NULL, 0, 1);

	return 0;
}

static void op_mips_stop(void)
{
	/* Disable performance monitoring for all counters.  */
	on_each_cpu(model->cpu_stop, NULL, 0, 1);
}

int __init oprofile_arch_init(struct oprofile_operations *ops)
{
	struct op_mips_model *lmodel = NULL;
	int res;

	switch (current_cpu_data.cputype) {
	case CPU_5KC:
	case CPU_20KC:
	case CPU_24K:
	case CPU_25KF:
#ifdef CONFIG_VENUS_OPROFILE_PERFORMANCE_COUNTER
		lmodel = &op_model_mipsxx;
#endif
		break;

	case CPU_RM9000:
		lmodel = &op_model_rm9000;
		break;
	};

#ifdef CONFIG_VENUS_OPROFILE_CLOCK
/* This can used on all MIPS CPUs */
	lmodel = &op_model_venus;
#endif

	if (!lmodel)
		return -ENODEV;

	res = lmodel->init();
	if (res)
		return res;

	model = lmodel;

	ops->create_files	= op_mips_create_files;
	ops->setup		= op_mips_setup;
	//ops->shutdown         = op_mips_shutdown;
	ops->start		= op_mips_start;
	ops->stop		= op_mips_stop;
	ops->cpu_type		= lmodel->cpu_type;

	printk(KERN_INFO "oprofile: using %s performance monitoring.\n",
	       lmodel->cpu_type);

	return 0;
}

void oprofile_arch_exit(void)
{
	model->exit();
}
