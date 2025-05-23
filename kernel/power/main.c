/*
 * kernel/power/main.c - PM subsystem core functionality.
 *
 * Copyright (c) 2003 Patrick Mochel
 * Copyright (c) 2003 Open Source Development Lab
 * 
 * This file is released under the GPLv2
 *
 */

#include <linux/suspend.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/pm.h>
#include <venus.h>
#include <platform.h>
#include <asm/io.h>

#include "power.h"

void free_all_memory(void);
#ifdef CONFIG_REALTEK_SUPPORT_BOOT_AV
int DVR_zone_disable(void);
int DVR_zone_enable(void);

extern int reserveDVR;
#endif

DECLARE_MUTEX(pm_sem);

struct pm_ops * pm_ops = NULL;
suspend_disk_method_t pm_disk_mode = PM_DISK_SHUTDOWN;

/**
 *	pm_set_ops - Set the global power method table. 
 *	@ops:	Pointer to ops structure.
 */

void pm_set_ops(struct pm_ops * ops)
{
	down(&pm_sem);
	pm_ops = ops;
	up(&pm_sem);
}

/**
 *	suspend_prepare - Do prep work before entering low-power state.
 *	@state:		State we're entering.
 *
 *	This is common code that is called for each state that we're 
 *	entering. Allocate a console, stop all processes, then make sure
 *	the platform can enter the requested state.
 */

static int suspend_prepare(suspend_state_t state)
{
	int error = 0;

	if (!pm_ops || !pm_ops->enter)
		return -EPERM;

	pm_prepare_console();

	if (freeze_processes()) {
		error = -EAGAIN;
		goto Thaw;
	}

	if (pm_ops->prepare) {
		if ((error = pm_ops->prepare(state)))
			goto Thaw;
	}
	free_all_memory();
#ifdef CONFIG_REALTEK_SUPPORT_BOOT_AV
	if (DVR_zone_disable()) {
		printk("reserve DVR zone...\n");
		reserveDVR = 1;
	}
#endif

	if ((error = device_suspend(PMSG_SUSPEND))) {
		printk(KERN_ERR "Some devices failed to suspend\n");
		goto Finish;
	}
	return 0;
 Finish:
	if (pm_ops->finish)
		pm_ops->finish(state);
 Thaw:
	thaw_processes();
	pm_restore_console();
	return error;
}


static int suspend_enter(suspend_state_t state)
{
	int error = 0;
	unsigned long flags;

	local_irq_save(flags);

	if ((error = device_power_down(PMSG_SUSPEND))) {
		printk(KERN_ERR "Some devices failed to power down\n");
		goto Done;
	}
	error = pm_ops->enter(state);
	device_power_up();
 Done:
	local_irq_restore(flags);
	return error;
}


/**
 *	suspend_finish - Do final work before exiting suspend sequence.
 *	@state:		State we're coming out of.
 *
 *	Call platform code to clean up, restart processes, and free the 
 *	console that we've allocated. This is not called for suspend-to-disk.
 */

static void suspend_finish(suspend_state_t state)
{
	device_resume();
	if (pm_ops && pm_ops->finish)
		pm_ops->finish(state);
/* For Venus and Neptune, resuming may fail. Therefore, use watchdog timeout
 * to prevent from this. */
	if(is_venus_cpu()||is_neptune_cpu()) {
		outl(0x1, VENUS_MIS_TCWTR);
		udelay(10);
		outl(0x1E0000A5, VENUS_MIS_TCWCR);
	}
	thaw_processes();
	pm_restore_console();
#ifdef CONFIG_REALTEK_SUPPORT_BOOT_AV
	if (DVR_zone_enable()) {
		printk("release DVR zone...\n");
		reserveDVR = 0;
	}
#endif
}




static char * pm_states[] = {
	[PM_SUSPEND_STANDBY]	= "standby",
	[PM_SUSPEND_MEM]	= "mem",
	[PM_SUSPEND_DISK]	= "disk",
	NULL,
};


/**
 *	enter_state - Do common work of entering low-power state.
 *	@state:		pm_state structure for state we're entering.
 *
 *	Make sure we're the only ones trying to enter a sleep state. Fail
 *	if someone has beat us to it, since we don't want anything weird to
 *	happen when we wake up.
 *	Then, do the setup for suspend, enter the state, and cleaup (after
 *	we've woken up).
 */

static int enter_state(suspend_state_t state)
{
	int error;

	if (down_trylock(&pm_sem))
		return -EBUSY;

	if (state == PM_SUSPEND_DISK) {
		error = pm_suspend_disk();
		goto Unlock;
	}

	/* Suspend is hard to get right on SMP. */
	if (num_online_cpus() != 1) {
		error = -EPERM;
		goto Unlock;
	}

	pr_debug("PM: Preparing system for %s sleep\n", pm_states[state]);
	if ((error = suspend_prepare(state)))
		goto Unlock;

	pr_debug("PM: Entering %s sleep\n", pm_states[state]);
	error = suspend_enter(state);

	pr_debug("PM: Finishing wakeup.\n");
	suspend_finish(state);
 Unlock:
	up(&pm_sem);
	return error;
}

/*
 * This is main interface to the outside world. It needs to be
 * called from process context.
 */
int software_suspend(void)
{
	return enter_state(PM_SUSPEND_DISK);
}


/**
 *	pm_suspend - Externally visible function for suspending system.
 *	@state:		Enumarted value of state to enter.
 *
 *	Determine whether or not value is within range, get state 
 *	structure, and enter (above).
 */

int pm_suspend(suspend_state_t state)
{
	if (state > PM_SUSPEND_ON && state < PM_SUSPEND_MAX)
		return enter_state(state);
	return -EINVAL;
}



decl_subsys(power,NULL,NULL);


/**
 *	state - control system power state.
 *
 *	show() returns what states are supported, which is hard-coded to
 *	'standby' (Power-On Suspend), 'mem' (Suspend-to-RAM), and
 *	'disk' (Suspend-to-Disk).
 *
 *	store() accepts one of those strings, translates it into the 
 *	proper enumerated value, and initiates a suspend transition.
 */

static ssize_t state_show(struct subsystem * subsys, char * buf)
{
	int i;
	char * s = buf;

	for (i = 0; i < PM_SUSPEND_MAX; i++) {
		if (pm_states[i])
			s += sprintf(s,"%s ",pm_states[i]);
	}
#ifdef CONFIG_PM_SLEEP_REBOOT
	s += sprintf(s,"reboot ");
#endif
	s += sprintf(s,"\n");
	return (s - buf);
}

static ssize_t state_store(struct subsystem * subsys, const char * buf, size_t n)
{
	suspend_state_t state = PM_SUSPEND_STANDBY;
	char ** s;
	char *p;
	int error;
	int len;

	p = memchr(buf, '\n', n);
	len = p ? p - buf : n;

	for (s = &pm_states[state]; state < PM_SUSPEND_MAX; s++, state++) {
		if (*s && !strncmp(buf, *s, len))
			break;
	}
	if (*s)
		error = enter_state(state);
	else {
#ifdef CONFIG_PM_SLEEP_REBOOT
/*
User can choose the resuming method.
	echo mem > /sys/power/state: normal resuming
	echo reboot > /sys/power/state: use reboot instead because the power of DRAM may be cut off.
*/
		if(!strncmp(buf, "reboot", len)) {
			extern int suspend_options;
			
			suspend_options = 1;
			error = enter_state(PM_SUSPEND_MEM);
		} else
#endif
			error = -EINVAL;
	}
	return error ? error : n;
}

power_attr(state);

static struct attribute * g[] = {
	&state_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};


static int __init pm_init(void)
{
	int error = subsystem_register(&power_subsys);
	if (!error)
		error = sysfs_create_group(&power_subsys.kset.kobj,&attr_group);
	return error;
}

core_initcall(pm_init);
