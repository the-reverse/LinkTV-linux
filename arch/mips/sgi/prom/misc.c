/*
 * misc.c: Miscellaneous ARCS PROM routines.
 *
 * Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
 *
 * $Id: misc.c,v 1.5 1998/03/27 08:53:47 ralf Exp $
 */
#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel.h>

#include <asm/bcache.h>
#include <asm/sgialib.h>
#include <asm/bootinfo.h>
#include <asm/system.h>

extern unsigned long mips_cputype;
extern int initialize_kbd(void);
extern void *sgiwd93_host;
extern void reset_wd33c93(void *instance);

void prom_halt(void)
{
	bcops->bc_disable();
	initialize_kbd();
	cli();
#if CONFIG_SCSI_SGIWD93
	reset_wd33c93(sgiwd93_host);
#endif
	romvec->halt();
}

void prom_powerdown(void)
{
	bcops->bc_disable();
	initialize_kbd();
	cli();
#if CONFIG_SCSI_SGIWD93
	reset_wd33c93(sgiwd93_host);
#endif
	romvec->pdown();
}

/* XXX is this a soft reset basically? XXX */
void prom_restart(void)
{
	bcops->bc_disable();
	initialize_kbd();
	cli();
#if CONFIG_SCSI_SGIWD93
	reset_wd33c93(sgiwd93_host);
#endif
	romvec->restart();
}

void prom_reboot(void)
{
	bcops->bc_disable();
	initialize_kbd();
	cli();
#if CONFIG_SCSI_SGIWD93
	reset_wd33c93(sgiwd93_host);
#endif
	romvec->reboot();
}

void prom_imode(void)
{
	bcops->bc_disable();
	initialize_kbd();
	cli();
#if CONFIG_SCSI_SGIWD93
	reset_wd33c93(sgiwd93_host);
#endif
	romvec->imode();
}

long prom_cfgsave(void)
{
	return romvec->cfg_save();
}

struct linux_sysid *prom_getsysid(void)
{
	return romvec->get_sysid();
}

__initfunc(void prom_cacheflush(void))
{
	romvec->cache_flush();
}
