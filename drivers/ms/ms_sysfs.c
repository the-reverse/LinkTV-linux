/*
 *  linux/drivers/ms/ms_sysfs.c
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  MS sysfs/driver model support.
 *
 * Modification History:
 *    #000 2009-07-09 CMYu   modify from mmc_sysfs.c for ms usage
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>

#include <linux/ms/card.h>
#include <linux/ms/host.h>

#include "ms.h"

#define dev_to_ms_card(d)	container_of(d, struct ms_card, dev)
#define to_ms_driver(d)		container_of(d, struct ms_driver, drv)


//CMYu, 20090727
#define MEMSTICK_ATTR(name, fmt, args...)					\
static ssize_t ms_##name##_show (struct device *dev, char *buf)	\
{									\
	struct ms_card *card = dev_to_ms_card(dev);			\
	return sprintf(buf, fmt, args);					\
}

MEMSTICK_ATTR(type, "%02X", card->id.type);
MEMSTICK_ATTR(category, "%02X", card->id.category);
MEMSTICK_ATTR(class, "%02X", card->id.class);

#define MEMSTICK_ATTR_RO(name) __ATTR(name, S_IRUGO, ms_##name##_show, NULL)

static struct device_attribute ms_dev_attrs[] = {
	MEMSTICK_ATTR_RO(type),
	MEMSTICK_ATTR_RO(category),
	MEMSTICK_ATTR_RO(class),
	__ATTR_NULL
};


static void ms_release_card(struct device *dev)
{

	struct ms_card *card = dev_to_ms_card(dev);

	kfree(card);
}


static int ms_bus_match(struct device *dev, struct device_driver *drv)
{

	struct ms_card *card = dev_to_ms_card(dev);
	return !ms_card_bad(card);
}

static int
ms_bus_hotplug(struct device *dev, char **envp, int num_envp, char *buf,
		int buf_size)
{
		struct ms_card *card = dev_to_ms_card(dev);
	int i = 0;

#define add_env(fmt,val)						\
	({								\
		int len, ret = -ENOMEM;					\
		if (i < num_envp) {					\
			envp[i++] = buf;				\
			len = snprintf(buf, buf_size, fmt, val) + 1;	\
			buf_size -= len;				\
			buf += len;					\
			if (buf_size >= 0)				\
				ret = 0;				\
		}							\
		ret;							\
	})

	add_env("MS_TYPE=%02X", card->id.type);
	add_env("MS_CATEGORY=%02X", card->id.category);
	add_env("MS_CLASS=%02X", card->id.class);
	//add_env("MS_SN=%s", ms_card_sn(card));

	return 0;
}


static int ms_bus_suspend(struct device *dev, pm_message_t state)
{
	struct ms_driver *drv = to_ms_driver(dev->driver);
	struct ms_card *card = dev_to_ms_card(dev);
	int ret = 0;

	if (dev->driver && drv->suspend)
		ret = drv->suspend(card, state);
	return ret;
}

static int ms_bus_resume(struct device *dev)
{
	struct ms_driver *drv = to_ms_driver(dev->driver);
	struct ms_card *card = dev_to_ms_card(dev);
	int ret = 0;

	if (dev->driver && drv->resume)
		ret = drv->resume(card);
	return ret;
}

static struct bus_type ms_bus_type = {
	.name		= "ms_bus",
	.dev_attrs	= ms_dev_attrs,
	.match		= ms_bus_match,
	.hotplug	= ms_bus_hotplug,
	.suspend	= ms_bus_suspend,
	.resume		= ms_bus_resume,
};


static int ms_drv_probe(struct device *dev)
{
	struct ms_driver *drv = to_ms_driver(dev->driver);
	struct ms_card *card = dev_to_ms_card(dev);

	return drv->probe(card);
}


static int ms_drv_remove(struct device *dev)
{
	struct ms_driver *drv = to_ms_driver(dev->driver);
	struct ms_card *card = dev_to_ms_card(dev);

	drv->remove(card);

	return 0;
}


int ms_register_driver(struct ms_driver *driver)
{
	driver->drv.bus = &ms_bus_type;
	driver->drv.probe = ms_drv_probe;
	driver->drv.remove = ms_drv_remove;
	return driver_register(&driver->drv);
}
EXPORT_SYMBOL(ms_register_driver);


void ms_unregister_driver(struct ms_driver *drv)
{

	drv->drv.bus = &ms_bus_type;
	driver_unregister(&drv->drv);
}
EXPORT_SYMBOL(ms_unregister_driver);


void ms_init_card_sysfs(struct ms_card *card, struct ms_host *host)
{
	memset(card, 0, sizeof(struct ms_card));
	card->host = host;
	device_initialize(&card->dev);
	card->dev.parent = card->host->dev;
	card->dev.bus = &ms_bus_type;
	card->dev.release = ms_release_card;
}


int ms_register_card(struct ms_card *card)
{
	snprintf(card->dev.bus_id, sizeof(card->dev.bus_id),
		 "%s:%04x", card->host->host_name, card->id.type);

	return device_add(&card->dev);
}


void ms_remove_card(struct ms_card *card)
{

	if (ms_card_present(card))
		device_del(&card->dev);

	put_device(&card->dev);
}


static int __init ms_init(void)
{
	return bus_register(&ms_bus_type);
}

static void __exit ms_exit(void)
{
	bus_unregister(&ms_bus_type);
}

module_init(ms_init);
module_exit(ms_exit);
