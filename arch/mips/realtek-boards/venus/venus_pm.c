/*
 * Venus Power Management Routines
 *
 * Copyright (c) 2006 Colin <colin@realtek.com.tw>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2006-05-22:	Colin	Preliminary power saving code.
 */
#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <venus.h>
#include <mars.h>
#include <jupiter.h>
#include <asm/io.h>
#include <platform.h>
#include <linux/parser.h>
#include <linux/kernel.h>
#include <prom.h>
#include <linux/delay.h>

#define LOGO_INFO_ADDR1	0xbfc6ff00
#define LOGO_INFO_ADDR2	0xbfc3ff00
#define BOOT_PARAM_ADDR_NEPTUNE	0xbfc01010
#define BOOT_PARAM_ADDR_MARS	0xa0002800
#define LOGO_INFO_SIZE	128

typedef struct {
	unsigned int flash_type;
	unsigned int region;
	unsigned int mac_hi;
	unsigned int mac_lo;
	unsigned char *logo_img_saddr;
	unsigned int logo_img_len;
	unsigned int logo_type;
	unsigned int logo_offset;
	unsigned int logo_reg_5370;
	unsigned int logo_reg_5374;
	unsigned int logo_reg_5378;
	unsigned int logo_reg_537c;
	unsigned int rescue_img_size;
	unsigned char *rescue_img_part0_saddr;
	unsigned int rescue_img_part0_len;
	unsigned char *rescue_img_part1_saddr;
	unsigned int rescue_img_part1_len;
	unsigned char *env_param_saddr;
} boot_extern_param;

logo_info_struct logo_info;

extern unsigned long dvr_task;	/* dvr application's PID */
extern unsigned long (*rtc_get_time)(void);
extern int venus_cpu_suspend(int board_id, int voltage_gpio, unsigned int options, unsigned int hwinfo, unsigned int powerkey_irrp, unsigned int ejectkey_irrp, int powerkey_gpio, int ejectkey_gpio, int vfd_type);
extern int mars_cpu_suspend(int board_id, int voltage_gpio, unsigned int options, unsigned int hwinfo, unsigned int powerkey_irrp, unsigned int ejectkey_irrp, int powerkey_gpio, int ejectkey_gpio, int vfd_type);
extern int jupiter_cpu_suspend(int board_id, int voltage_gpio, unsigned int options, unsigned int hwinfo, unsigned int powerkey_irrp, unsigned int ejectkey_irrp, int powerkey_gpio, int ejectkey_gpio, int vfd_type);

int Suspend_ret;	/* The return value of venus_cpu_suspend. If Linux is woken up by RTC alarm, it is 2; if woken up by eject event, it is 1; for the other situation, it is 0. */
int suspend_options = 0;/* 0: normal suspend. 1: cut off DRAM. */

extern platform_info_t platform_info;

#ifndef CONFIG_REALTEK_HDMI_NONE
#define ARRAY_COUNT(x)  (sizeof(x) / sizeof((x)[0]))

static void BusyWaiting(unsigned int count)
{
	unsigned int i;

	for(i = 0; i < count; i++) ;
}

void I2C_Write1(unsigned int addr, unsigned int *array)
{
	int n;
	unsigned int *interrupt;
	*(volatile unsigned int *)0xb801b36c = 0x0;//disable i2c
	*(volatile unsigned int *)0xb801b304 = addr&0x0ff;
	*(volatile unsigned int *)0xb801b36c = 0x1;//enable i2c
	interrupt = (void *)0xb801b334;
	for (n = 1; n <= array[0]; n++)
	{
		while((*interrupt&0x00000010) != 0x10)		//wait for Tx empty
			BusyWaiting(100000);
		BusyWaiting(7000);
		*(volatile unsigned int *)0xb801b310=array[n] & 0x0ff;

	}
}

#ifdef CONFIG_REALTEK_HDMI_NXP
void SET_NXP_HDMI_480P()
{
	unsigned int	I2CBUF[5];
	unsigned int	i = 0;
	unsigned int	i2c_addr = 0xe0;	
	unsigned int P480[]={
		0xff, 0x0,
		0x1, 0x0,
		0xa, 0x0,
		0xb, 0x0,
		0xc, 0x1,
		0xf, 0x0,
		0x10, 0x80,
		0x11, 0x0,
		0x12, 0x1,
		0x15, 0x0,
		0x16, 0x1,
		0x17, 0x0,
		0x18, 0xff,
		0x19, 0xff,
		0x1a, 0xff,
		0x1b, 0x0,
		0x1c, 0x0,
		0x1d, 0x0,
		0x1e, 0xff,
		0x1f, 0x0,
		0x20, 0x23,
		0x21, 0x50,
		0x22, 0x14,
		0x23, 0x8f,
		0x24, 0xc,
		0x25, 0x0,
		0x80, 0x5,
		0x81, 0x0,
		0x82, 0x0,
		0x83, 0x6,
		0x84, 0x0,
		0x85, 0x6,
		0x86, 0x0,
		0x87, 0x2,
		0x88, 0x0,
		0x89, 0x6,
		0x8a, 0x92,
		0x8b, 0x7,
		0x8c, 0x50,
		0x8d, 0x2,
		0x8e, 0x0,
		0x8f, 0x2,
		0x90, 0xce,
		0x91, 0x0,
		0x92, 0x0,
		0x93, 0x2,
		0x94, 0x0,
		0x95, 0x0,
		0x96, 0x0,
		0x97, 0x3,
		0x98, 0x8c,
		0x99, 0x0,
		0x9a, 0x0,
		0x9b, 0x0,
		0x9c, 0x0,
		0x9d, 0x0,
		0x9e, 0x0,
		0xa0, 0x1,
		0xa1, 0x0,
		0xa2, 0x8d,
		0xa3, 0x0,
		0xa4, 0x2b,
		0xa5, 0x0,
		0xa6, 0x0,
		0xa7, 0x0,
		0xa8, 0x0,
		0xa9, 0x0,
		0xaa, 0x0,
		0xab, 0x0,
		0xac, 0x0,
		0xad, 0x0,
		0xae, 0x0,
		0xaf, 0x0,
		0xb0, 0x0,
		0xb1, 0x0,
		0xb2, 0x0,
		0xb3, 0x0,
		0xb4, 0x0,
		0xb5, 0x0,
		0xb6, 0x0,
		0xb7, 0x0,
		0xb8, 0x0,
		0xb9, 0x0,
		0xba, 0x0,
		0xbb, 0x0,
		0xbc, 0x0,
		0xbd, 0x0,
		0xbe, 0x0,
		0xbf, 0x0,
		0xc0, 0x0,
		0xc1, 0x0,
		0xc2, 0x0,
		0xc3, 0x0,
		0xc4, 0x0,
		0xc5, 0x0,
		0xc6, 0x0,
		0xc7, 0x0,
		0xc8, 0x0,
		0xca, 0x40,
		0xcb, 0x0,
		0xcc, 0x0,
		0xcd, 0x0,
		0xce, 0x0,
		0xcf, 0x0,
		0xd0, 0x11,
		0xd1, 0x7a,
		0xe4, 0x0,
		0xe5, 0x48,
		0xe8, 0x80,
		0xe9, 0x75,
		0xea, 0x30,
		0xeb, 0x0,
		0xec, 0x0,
		0xee, 0x3,
		0xef, 0x1b,
		0xf0, 0x0,
		0xf1, 0x2,
		0xf2, 0x2,
		0xf5, 0x0,
		0xf8, 0x0,
		0xf9, 0x60,
		0xfc, 0x0,
		0xfd, 0x2,
		0xfe, 0xa1,
		0xff, 0x1,
		0x0, 0x0,
		0x1, 0x0,
		0x2, 0x0,
		0x3, 0x0,
		0x4, 0x0,
		0x5, 0x0,
		0x6, 0x0,
		0x7, 0x0,
		0x8, 0x0,
		0x9, 0x0,
		0xa, 0x0,
		0xb, 0x0,
		0xc, 0x0,
		0xd, 0x0,
		0x1b, 0x0,
		0x1c, 0x0,
		0x1d, 0x0,
		0x1e, 0x0,
		0x1f, 0x0,
		0x20, 0x0,
		0x21, 0x0,
		0x22, 0x0,
		0x23, 0x0,
		0x24, 0x0,
		0x25, 0x0,
		0x26, 0x0,
		0x27, 0x0,
		0x28, 0x0,
		0x29, 0x0,
		0x2a, 0x0,
		0x2b, 0x0,
		0x2c, 0x0,
		0x2d, 0x0,
		0x2e, 0x0,
		0x2f, 0x0,
		0x30, 0x0,
		0x31, 0x0,
		0x32, 0x0,
		0x33, 0x0,
		0x34, 0x0,
		0x35, 0x0,
		0x36, 0x0,
		0x37, 0x0,
		0x38, 0x0,
		0x39, 0x0,
		0x3a, 0x0,
		0x3b, 0x0,
		0x3c, 0x0,
		0x3d, 0x0,
		0x3e, 0x0,
		0x3f, 0x0,
		0x40, 0x0,
		0x41, 0x0,
		0x42, 0x0,
		0x43, 0x0,
		0x44, 0x0,
		0x45, 0x0,
		0x46, 0x0,
		0x47, 0x0,
		0x48, 0x0,
		0xa0, 0x0,
		0xa1, 0x0,
		0xa2, 0x0,
		0xa3, 0x0,
		0xa4, 0x0,
		0xa5, 0x0,
		0xa6, 0x0,
		0xa7, 0x0,
		0xa8, 0x0,
		0xbd, 0x0,
		0xbe, 0x0,
		0xbf, 0x0,
		0xc0, 0x0,
		0xc1, 0x0,
		0xc2, 0x0,
		0xc3, 0x0,
		0xc4, 0x0,
		0xc5, 0x0,
		0xc6, 0x0,
		0xc7, 0x0,
		0xc8, 0x0,
		0xca, 0x0,
		0xff, 0x2,
		0x0, 0x1e,
		0x1, 0x2,
		0x2, 0x0,
		0x3, 0x84,
		0x4, 0x0,
		0x5, 0x1,
		0x6, 0x92,
		0x7, 0x14,
		0x8, 0x0,
		0x9, 0xa,
		0xa, 0x0,
		0xb, 0xa1,
		0xc, 0x0,
		0xd, 0x1,
		0xe, 0x2,
		0x10, 0x0,
		0x11, 0x0,
		0xff, 0x9,
		0xfa, 0x0,
		0xfb, 0xa0,
		0xfc, 0x80,
		0xfd, 0x60,
		0xfe, 0x0,
		0xff, 0x10,
		0x20, 0x81,
		0x21, 0x0,
		0x22, 0x0,
		0x23, 0x0,
		0x24, 0x0,
		0x25, 0x0,
		0x26, 0x0,
		0x27, 0x0,
		0x28, 0x0,
		0x29, 0x0,
		0x2a, 0x0,
		0x2b, 0x0,
		0x2c, 0x0,
		0x2d, 0x0,
		0x2e, 0x0,
		0x2f, 0x0,
		0x30, 0x0,
		0x31, 0x0,
		0x32, 0x0,
		0x33, 0x0,
		0x34, 0x0,
		0x35, 0x0,
		0x36, 0x0,
		0x37, 0x0,
		0x38, 0x0,
		0x39, 0x0,
		0x3a, 0x0,
		0x3b, 0x0,
		0x3c, 0x0,
		0x3d, 0x0,
		0x3e, 0x0,
		0x40, 0x82,
		0x41, 0x2,
		0x42, 0xd,
		0x43, 0xe4,
		0x44, 0x20,
		0x45, 0x68,
		0x46, 0x0,
		0x47, 0x3,
		0x48, 0x0,
		0x49, 0x0,
		0x4a, 0x0,
		0x4b, 0x0,
		0x4c, 0x0,
		0x4d, 0x0,
		0x4e, 0x0,
		0x4f, 0x0,
		0x50, 0x0,
		0x51, 0x0,
		0x52, 0x0,
		0x53, 0x0,
		0x54, 0x0,
		0x55, 0x0,
		0x56, 0x0,
		0x57, 0x0,
		0x58, 0x0,
		0x59, 0x0,
		0x5a, 0x0,
		0x5b, 0x0,
		0x5c, 0x0,
		0x5d, 0x0,
		0x5e, 0x0,
		0x60, 0x83,
		0x61, 0x0,
		0x62, 0x0,
		0x63, 0x0,
		0x64, 0x0,
		0x65, 0x0,
		0x66, 0x0,
		0x67, 0x0,
		0x68, 0x0,
		0x69, 0x0,
		0x6a, 0x0,
		0x6b, 0x0,
		0x6c, 0x0,
		0x6d, 0x0,
		0x6e, 0x0,
		0x6f, 0x0,
		0x70, 0x0,
		0x71, 0x0,
		0x72, 0x0,
		0x73, 0x0,
		0x74, 0x0,
		0x75, 0x0,
		0x76, 0x0,
		0x77, 0x0,
		0x78, 0x0,
		0x79, 0x0,
		0x7a, 0x0,
		0x7b, 0x0,
		0x7c, 0x0,
		0x7d, 0x0,
		0x7e, 0x0,
		0x80, 0x84,
		0x81, 0x1,
		0x82, 0xa,
		0x83, 0x70,
		0x84, 0x1,
		0x85, 0x0,
		0x86, 0x0,
		0x87, 0x0,
		0x88, 0x0,
		0x89, 0x0,
		0x8a, 0x0,
		0x8b, 0x0,
		0x8c, 0x0,
		0x8d, 0x0,
		0x8e, 0x0,
		0x8f, 0x0,
		0x90, 0x0,
		0x91, 0x0,
		0x92, 0x0,
		0x93, 0x0,
		0x94, 0x0,
		0x95, 0x0,
		0x96, 0x0,
		0x97, 0x0,
		0x98, 0x0,
		0x99, 0x0,
		0x9a, 0x0,
		0x9b, 0x0,
		0x9c, 0x0,
		0x9d, 0x0,
		0x9e, 0x0,
		0xa0, 0x85,
		0xa1, 0x0,
		0xa2, 0x0,
		0xa3, 0x0,
		0xa4, 0x0,
		0xa5, 0x0,
		0xa6, 0x0,
		0xa7, 0x0,
		0xa8, 0x0,
		0xa9, 0x0,
		0xaa, 0x0,
		0xab, 0x0,
		0xac, 0x0,
		0xad, 0x0,
		0xae, 0x0,
		0xaf, 0x0,
		0xb0, 0x0,
		0xb1, 0x0,
		0xb2, 0x0,
		0xb3, 0x0,
		0xb4, 0x0,
		0xb5, 0x0,
		0xb6, 0x0,
		0xb7, 0x0,
		0xb8, 0x0,
		0xb9, 0x0,
		0xba, 0x0,
		0xbb, 0x0,
		0xbc, 0x0,
		0xbd, 0x0,
		0xbe, 0x0,
		0xff, 0x11,
		0x0, 0x1, // audio mute
		0x1, 0x0,
		0x2, 0x0,
		0x3, 0x0,
		0x4, 0x80,
		0x5, 0x78,
		0x6, 0x69,
		0x7, 0x0,
		0x8, 0x0,
		0x9, 0x18,
		0xa, 0x0,
		0xb, 0x0,
		0xc, 0x33,
		0xd, 0x4,
		0xe, 0x1,
		0xf, 0x14,
		0x14, 0x0,
		0x15, 0x0,
		0x16, 0x2,
		0x17, 0x0,
		0x18, 0x0,
		0x19, 0x0,
		0x1a, 0x0,
		0x1b, 0x0,
		0x1c, 0x0,
		0x1d, 0x0,
		0x1e, 0x0,
		0x1f, 0x0,
		0x20, 0x5,
		0x21, 0x0,
		0x22, 0x0,
		0x23, 0x0,
		0x24, 0x0,
		0x25, 0x0,
		0x26, 0x0,
		0x27, 0x0,
		0x28, 0x0,
		0x29, 0x0,
		0x2a, 0x0,
		0x2b, 0x0,
		0x2c, 0x0,
		0x2d, 0x0,
		0x2e, 0x0,
		0x2f, 0x0,
		0x30, 0x0,
		0x31, 0x0,
		0x32, 0x0,
		0x33, 0x0,
		0x34, 0x0,
		0x35, 0x0,
		0x36, 0x0,
		0x37, 0x0,
		0x38, 0x0,
		0x39, 0x0,
		0x3a, 0x0,
		0x3b, 0x0,
		0x3c, 0x0,
		0x3d, 0x0,
		0x3e, 0x0,
		0x40, 0x6,
		0x41, 0x0,
		0x42, 0x0,
		0x43, 0x0,
		0x44, 0x0,
		0x45, 0x0,
		0x46, 0x0,
		0x47, 0x0,
		0x48, 0x0,
		0x49, 0x0,
		0x4a, 0x0,
		0x4b, 0x0,
		0x4c, 0x0,
		0x4d, 0x0,
		0x4e, 0x0,
		0x4f, 0x0,
		0x50, 0x0,
		0x51, 0x0,
		0x52, 0x0,
		0x53, 0x0,
		0x54, 0x0,
		0x55, 0x0,
		0x56, 0x0,
		0x57, 0x0,
		0x58, 0x0,
		0x59, 0x0,
		0x5a, 0x0,
		0x5b, 0x0,
		0x5c, 0x0,
		0x5d, 0x0,
		0x5e, 0x0,
		0x60, 0x4,
		0x61, 0x0,
		0x62, 0x0,
		0x63, 0x0,
		0x64, 0x0,
		0x65, 0x0,
		0x66, 0x0,
		0x67, 0x0,
		0x68, 0x0,
		0x69, 0x0,
		0x6a, 0x0,
		0x6b, 0x0,
		0x6c, 0x0,
		0x6d, 0x0,
		0x6e, 0x0,
		0x6f, 0x0,
		0x70, 0x0,
		0x71, 0x0,
		0x72, 0x0,
		0x73, 0x0,
		0x74, 0x0,
		0x75, 0x0,
		0x76, 0x0,
		0x77, 0x0,
		0x78, 0x0,
		0x79, 0x0,
		0x7a, 0x0,
		0x7b, 0x0,
		0x7c, 0x0,
		0x7d, 0x0,
		0x7e, 0x0,
		0xff, 0x12,
		0x31, 0x50,
		0x32, 0x9,
		0x33, 0xae,
		0x34, 0x0,
		0x36, 0x0,
		0x37, 0x0,
		0x38, 0x0,
		0x39, 0x0,
		0x40, 0x0,
		0x42, 0x12,
		0x43, 0x34,
		0x49, 0x0,
		0x4b, 0x5e,
		0x4c, 0x77,
		0x4d, 0x50,
		0x4e, 0x9,
		0x4f, 0xae,
		0x50, 0x0,
		0x60, 0x0,
		0x61, 0x0,
		0x63, 0x0,
		0x64, 0x0,
		0x65, 0x0,
		0x66, 0x0,
		0x67, 0x0,
		0x68, 0x0,
		0x69, 0x0,
		0x6a, 0x0,
		0x6b, 0x0,
		0x6c, 0x0,
		0x6d, 0x0,
		0x6e, 0x0,
		0x6f, 0x0,
		0x70, 0x0,
		0x71, 0x0,
		0x90, 0x0,
		0x91, 0x0,
		0x97, 0x9,
		0x98, 0x0,
		0x99, 0x0,
		0x9a, 0x64,
		0xb8, 0x16
	};

	i2c_addr = i2c_addr >> 1;
	I2CBUF[0] = 2;
	for(i = 0; i < (ARRAY_COUNT(P480)/2); i++) {
		I2CBUF[1] = P480[i*2]&0x0ff;
		I2CBUF[2] = P480[(i*2)+1]&0x0ff;
		I2C_Write1(i2c_addr,I2CBUF);
		BusyWaiting(70000);
	}
}

void SET_NXP_HDMI_576P()
{
	unsigned int	I2CBUF[5];
	unsigned int	i = 0;
	unsigned int	i2c_addr = 0xe0;	
	unsigned int P576[]={
		0xff, 0x0,
		0x1, 0x0,
		0xa, 0x0,
		0xb, 0x0,
		0xc, 0x1,
		0xf, 0x0,
		0x10, 0x80,
		0x11, 0x0,
		0x12, 0x1,
		0x15, 0x0,
		0x16, 0x1,
		0x17, 0x0,
		0x18, 0xff,
		0x19, 0xff,
		0x1a, 0xff,
		0x1b, 0xff,
		0x1c, 0x0,
		0x1d, 0x0,
		0x1e, 0x0,//0x3
		0x1f, 0xfc,
		0x20, 0x23,
		0x21, 0x50,
		0x22, 0x14,
		0x23, 0x8f,
		0x24, 0xc,
		0x25, 0x0,
		0x80, 0x6,
		0x81, 0x7,
		0x82, 0xc0,
		0x83, 0x7,
		0x84, 0xc0,
		0x85, 0x7,
		0x86, 0xc0,
		0x87, 0x2,
		0x88, 0x59,
		0x89, 0x1,
		0x8a, 0x32,
		0x8b, 0x0,
		0x8c, 0x75,
		0x8d, 0x6,
		0x8e, 0x4a,
		0x8f, 0x2,
		0x90, 0xc,
		0x91, 0x7,
		0x92, 0xab,
		0x93, 0x6,
		0x94, 0xa5,
		0x95, 0x7,
		0x96, 0x4f,
		0x97, 0x2,
		0x98, 0xc,
		0x99, 0x0,
		0x9a, 0x40,
		0x9b, 0x2,
		0x9c, 0x0,
		0x9d, 0x2,
		0x9e, 0x0,
		0xa0, 0x7,
		0xa1, 0x0,
		0xa2, 0x93,
		0xa3, 0x0,
		0xa4, 0x2d,
		0xa5, 0x0,
		0xa6, 0x0,
		0xa7, 0x0,
		0xa8, 0x0,
		0xa9, 0x0,
		0xaa, 0x0,
		0xab, 0x0,
		0xac, 0x0,
		0xad, 0x0,
		0xae, 0x0,
		0xaf, 0x0,
		0xb0, 0x0,
		0xb1, 0x0,
		0xb2, 0x0,
		0xb3, 0x0,
		0xb4, 0x0,
		0xb5, 0x0,
		0xb6, 0x0,
		0xb7, 0x0,
		0xb8, 0x0,
		0xb9, 0x0,
		0xba, 0x0,
		0xbb, 0x0,
		0xbc, 0x0,
		0xbd, 0x0,
		0xbe, 0x0,
		0xbf, 0x0,
		0xc0, 0x0,
		0xc1, 0x0,
		0xc2, 0x0,
		0xc3, 0x0,
		0xc4, 0x0,
		0xc5, 0x0,
		0xc6, 0x0,
		0xc7, 0x0,
		0xc8, 0x0,
		0xca, 0x40,
		0xcb, 0x0,
		0xcc, 0x0,
		0xcd, 0x0,
		0xce, 0x0,
		0xcf, 0x0,
		0xd0, 0x11,
		0xd1, 0x7a,
		0xe4, 0x1,
		0xe5, 0x48,
		0xe8, 0x80,
		0xe9, 0xc2,
		0xea, 0x40,
		0xeb, 0x0,
		0xec, 0x0,
		0xee, 0x3,
		0xef, 0x1b,
		0xf0, 0x0,
		0xf1, 0x2,
		0xf2, 0x2,
		0xf5, 0x0,
		0xf8, 0x0,
		0xf9, 0x60,
		0xfc, 0x0,
		0xfd, 0x8,
		0xfe, 0xa1,
		0xff, 0x1,
		0x0, 0x0,
		0x1, 0x0,
		0x2, 0x0,
		0x3, 0x0,
		0x4, 0x0,
		0x5, 0x0,
		0x6, 0x0,
		0x7, 0x0,
		0x8, 0x0,
		0x9, 0x0,
		0xa, 0x0,
		0xb, 0x0,
		0xc, 0x0,
		0xd, 0x0,
		0x1b, 0x0,
		0x1c, 0x0,
		0x1d, 0x0,
		0x1e, 0x0,
		0x1f, 0x0,
		0x20, 0x0,
		0x21, 0x0,
		0x22, 0x0,
		0x23, 0x0,
		0x24, 0x0,
		0x25, 0x0,
		0x26, 0x0,
		0x27, 0x0,
		0x28, 0x0,
		0x29, 0x0,
		0x2a, 0x0,
		0x2b, 0x0,
		0x2c, 0x0,
		0x2d, 0x0,
		0x2e, 0x0,
		0x2f, 0x0,
		0x30, 0x0,
		0x31, 0x0,
		0x32, 0x0,
		0x33, 0x0,
		0x34, 0x0,
		0x35, 0x0,
		0x36, 0x0,
		0x37, 0x0,
		0x38, 0x0,
		0x39, 0x0,
		0x3a, 0x0,
		0x3b, 0x0,
		0x3c, 0x0,
		0x3d, 0x0,
		0x3e, 0x0,
		0x3f, 0x0,
		0x40, 0x0,
		0x41, 0x0,
		0x42, 0x0,
		0x43, 0x0,
		0x44, 0x0,
		0x45, 0x0,
		0x46, 0x0,
		0x47, 0x0,
		0x48, 0x0,
		0xa0, 0x0,
		0xa1, 0x0,
		0xa2, 0x0,
		0xa3, 0x0,
		0xa4, 0x0,
		0xa5, 0x0,
		0xa6, 0x0,
		0xa7, 0x0,
		0xa8, 0x0,
		0xbd, 0x0,
		0xbe, 0x0,
		0xbf, 0x0,
		0xc0, 0x0,
		0xc1, 0x0,
		0xc2, 0x0,
		0xc3, 0x0,
		0xc4, 0x0,
		0xc5, 0x0,
		0xc6, 0x0,
		0xc7, 0x0,
		0xc8, 0x0,
		0xca, 0x0,
		0xff, 0x2,
		0x0, 0x1e,
		0x1, 0x2,
		0x2, 0x0,
		0x3, 0x84,
		0x4, 0x0,
		0x5, 0x1,
		0x6, 0x92,
		0x7, 0xfa,
		0x8, 0x0,
		0x9, 0x5b,
		0xa, 0x0,
		0xb, 0xa1,
		0xc, 0x0,
		0xd, 0x1,
		0xe, 0x2,
		0x10, 0x0,
		0x11, 0x0,
		0xff, 0x9,
		0xfa, 0x0,
		0xfb, 0xa0,
		0xfc, 0x80,
		0xfd, 0x60,
		0xfe, 0x0,
		0xff, 0x10,
		0x20, 0x81,
		0x21, 0x0,
		0x22, 0x0,
		0x23, 0x0,
		0x24, 0x0,
		0x25, 0x0,
		0x26, 0x0,
		0x27, 0x0,
		0x28, 0x0,
		0x29, 0x0,
		0x2a, 0x0,
		0x2b, 0x0,
		0x2c, 0x0,
		0x2d, 0x0,
		0x2e, 0x0,
		0x2f, 0x0,
		0x30, 0x0,
		0x31, 0x0,
		0x32, 0x0,
		0x33, 0x0,
		0x34, 0x0,
		0x35, 0x0,
		0x36, 0x0,
		0x37, 0x0,
		0x38, 0x0,
		0x39, 0x0,
		0x3a, 0x0,
		0x3b, 0x0,
		0x3c, 0x0,
		0x3d, 0x0,
		0x3e, 0x0,
		0x40, 0x82,
		0x41, 0x2,
		0x42, 0xd,
		0x43, 0xc6,
		0x44, 0x40,
		0x45, 0x58,
		0x46, 0x0,
		0x47, 0x11,
		0x48, 0x0,
		0x49, 0x0,
		0x4a, 0x0,
		0x4b, 0x0,
		0x4c, 0x0,
		0x4d, 0x0,
		0x4e, 0x0,
		0x4f, 0x0,
		0x50, 0x0,
		0x51, 0x0,
		0x52, 0x0,
		0x53, 0x0,
		0x54, 0x0,
		0x55, 0x0,
		0x56, 0x0,
		0x57, 0x0,
		0x58, 0x0,
		0x59, 0x0,
		0x5a, 0x0,
		0x5b, 0x0,
		0x5c, 0x0,
		0x5d, 0x0,
		0x5e, 0x0,
		0x60, 0x83,
		0x61, 0x0,
		0x62, 0x0,
		0x63, 0x0,
		0x64, 0x0,
		0x65, 0x0,
		0x66, 0x0,
		0x67, 0x0,
		0x68, 0x0,
		0x69, 0x0,
		0x6a, 0x0,
		0x6b, 0x0,
		0x6c, 0x0,
		0x6d, 0x0,
		0x6e, 0x0,
		0x6f, 0x0,
		0x70, 0x0,
		0x71, 0x0,
		0x72, 0x0,
		0x73, 0x0,
		0x74, 0x0,
		0x75, 0x0,
		0x76, 0x0,
		0x77, 0x0,
		0x78, 0x0,
		0x79, 0x0,
		0x7a, 0x0,
		0x7b, 0x0,
		0x7c, 0x0,
		0x7d, 0x0,
		0x7e, 0x0,
		0x80, 0x84,
		0x81, 0x1,
		0x82, 0xa,
		0x83, 0x70,
		0x84, 0x1,
		0x85, 0x0,
		0x86, 0x0,
		0x87, 0x0,
		0x88, 0x0,
		0x89, 0x0,
		0x8a, 0x0,
		0x8b, 0x0,
		0x8c, 0x0,
		0x8d, 0x0,
		0x8e, 0x0,
		0x8f, 0x0,
		0x90, 0x0,
		0x91, 0x0,
		0x92, 0x0,
		0x93, 0x0,
		0x94, 0x0,
		0x95, 0x0,
		0x96, 0x0,
		0x97, 0x0,
		0x98, 0x0,
		0x99, 0x0,
		0x9a, 0x0,
		0x9b, 0x0,
		0x9c, 0x0,
		0x9d, 0x0,
		0x9e, 0x0,
		0xa0, 0x85,
		0xa1, 0x0,
		0xa2, 0x0,
		0xa3, 0x0,
		0xa4, 0x0,
		0xa5, 0x0,
		0xa6, 0x0,
		0xa7, 0x0,
		0xa8, 0x0,
		0xa9, 0x0,
		0xaa, 0x0,
		0xab, 0x0,
		0xac, 0x0,
		0xad, 0x0,
		0xae, 0x0,
		0xaf, 0x0,
		0xb0, 0x0,
		0xb1, 0x0,
		0xb2, 0x0,
		0xb3, 0x0,
		0xb4, 0x0,
		0xb5, 0x0,
		0xb6, 0x0,
		0xb7, 0x0,
		0xb8, 0x0,
		0xb9, 0x0,
		0xba, 0x0,
		0xbb, 0x0,
		0xbc, 0x0,
		0xbd, 0x0,
		0xbe, 0x0,
		0xff, 0x11,
		0x0, 0x1, // audio mute
		0x1, 0x0,
		0x2, 0x0,
		0x3, 0x0,
		0x4, 0x80,
		0x5, 0x78,
		0x6, 0x69,
		0x7, 0x0,
		0x8, 0x0,
		0x9, 0x18,
		0xa, 0x0,
		0xb, 0x0,
		0xc, 0x33,
		0xd, 0x4,
		0xe, 0x1,
		0xf, 0x14,
		0x14, 0x0,
		0x15, 0x0,
		0x16, 0x2,
		0x17, 0x0,
		0x18, 0x0,
		0x19, 0x0,
		0x1a, 0x0,
		0x1b, 0x0,
		0x1c, 0x0,
		0x1d, 0x0,
		0x1e, 0x0,
		0x1f, 0x0,
		0x20, 0x5,
		0x21, 0x0,
		0x22, 0x0,
		0x23, 0x0,
		0x24, 0x0,
		0x25, 0x0,
		0x26, 0x0,
		0x27, 0x0,
		0x28, 0x0,
		0x29, 0x0,
		0x2a, 0x0,
		0x2b, 0x0,
		0x2c, 0x0,
		0x2d, 0x0,
		0x2e, 0x0,
		0x2f, 0x0,
		0x30, 0x0,
		0x31, 0x0,
		0x32, 0x0,
		0x33, 0x0,
		0x34, 0x0,
		0x35, 0x0,
		0x36, 0x0,
		0x37, 0x0,
		0x38, 0x0,
		0x39, 0x0,
		0x3a, 0x0,
		0x3b, 0x0,
		0x3c, 0x0,
		0x3d, 0x0,
		0x3e, 0x0,
		0x40, 0x6,
		0x41, 0x0,
		0x42, 0x0,
		0x43, 0x0,
		0x44, 0x0,
		0x45, 0x0,
		0x46, 0x0,
		0x47, 0x0,
		0x48, 0x0,
		0x49, 0x0,
		0x4a, 0x0,
		0x4b, 0x0,
		0x4c, 0x0,
		0x4d, 0x0,
		0x4e, 0x0,
		0x4f, 0x0,
		0x50, 0x0,
		0x51, 0x0,
		0x52, 0x0,
		0x53, 0x0,
		0x54, 0x0,
		0x55, 0x0,
		0x56, 0x0,
		0x57, 0x0,
		0x58, 0x0,
		0x59, 0x0,
		0x5a, 0x0,
		0x5b, 0x0,
		0x5c, 0x0,
		0x5d, 0x0,
		0x5e, 0x0,
		0x60, 0x4,
		0x61, 0x0,
		0x62, 0x0,
		0x63, 0x0,
		0x64, 0x0,
		0x65, 0x0,
		0x66, 0x0,
		0x67, 0x0,
		0x68, 0x0,
		0x69, 0x0,
		0x6a, 0x0,
		0x6b, 0x0,
		0x6c, 0x0,
		0x6d, 0x0,
		0x6e, 0x0,
		0x6f, 0x0,
		0x70, 0x0,
		0x71, 0x0,
		0x72, 0x0,
		0x73, 0x0,
		0x74, 0x0,
		0x75, 0x0,
		0x76, 0x0,
		0x77, 0x0,
		0x78, 0x0,
		0x79, 0x0,
		0x7a, 0x0,
		0x7b, 0x0,
		0x7c, 0x0,
		0x7d, 0x0,
		0x7e, 0x0,
		0xff, 0x12,
		0x31, 0xcd,
		0x32, 0x5e,
		0x33, 0x50,
		0x34, 0x0,
		0x36, 0x0,
		0x37, 0x0,
		0x38, 0x0,
		0x39, 0x0,
		0x40, 0x0,
		0x42, 0x0,
		0x43, 0x0,
		0x49, 0x0,
		0x4b, 0x8e,
		0x4c, 0x1b,
		0x4d, 0xcd,
		0x4e, 0x5e,
		0x4f, 0x50,
		0x50, 0x0,
		0x60, 0x0,
		0x61, 0x0,
		0x63, 0x0,
		0x64, 0x0,
		0x65, 0x0,
		0x66, 0x0,
		0x67, 0x0,
		0x68, 0x0,
		0x69, 0x0,
		0x6a, 0x0,
		0x6b, 0x0,
		0x6c, 0x0,
		0x6d, 0x0,
		0x6e, 0x0,
		0x6f, 0x0,
		0x70, 0x0,
		0x71, 0x0,
		0x90, 0x0,
		0x91, 0x0,
		0x97, 0x9,
		0x98, 0x0,
		0x99, 0x0,
		0x9a, 0x64,
		0xb8, 0x16
	};

	i2c_addr = i2c_addr >> 1;
	I2CBUF[0] = 2;
	for(i = 0; i < (ARRAY_COUNT(P576)/2); i++) {
		I2CBUF[1] = P576[i*2]&0x0ff;
		I2CBUF[2] = P576[(i*2)+1]&0x0ff;
		I2C_Write1(i2c_addr,I2CBUF);
		BusyWaiting(70000);
	}
}
#else
void SET_CAT_HDMI_480P()
{
	unsigned int	I2CBUF[5];
	unsigned int	i = 0;
	unsigned int	i2c_addr = 0x98;	
	//Change to Bank 0
	unsigned int P480[]={
		0x0F, 0x00,
		// HDMITX Reset Enable
		0x04, 0xFF,
		//$$$ Idle 1FF,
		//$$$ Idle 1FF,
		// HDMITX Reset Disable
		0x04, 0x1F,
		// Switch to PC program DDC mode
		0x10, 0x01,
		// HDCP Registers, reg20[0] CPDesired, reg20h[1] 1.1Feature
		0x20, 0x00,
		0x22, 0x02, 
		// Clock Control Registers
		0x58, 0x11,
		0x59, 0x08,//edge should be 1
		// input format 
		0x70, 0x48,
		// input color mode reg70[7:6], sync_emb reg70[3]
		0x71, 0x00,
		// EnUdFilt reg72[6], CSCSel reg72h[1:0] 
		0x72, 0x00,  
		0x73, 0x10,
		0x74, 0x80,
		0x75, 0x10,
		// CSC Matrix 
		0x76, 0x00,
		0x77, 0x08,
		0x78, 0x6a,
		0x79, 0x1a,
		0x7A, 0x4f,
		0x7B, 0x1d,
		0x7C, 0x00,
		0x7D, 0x08,
		0x7E, 0xf7,
		0x7F, 0x0a,
		0x80, 0x00,
		0x81, 0x00,
		0x82, 0x00,
		0x83, 0x08,
		0x84, 0x00,
		0x85, 0x00,
		0x86, 0xdb,
		0x87, 0x0d,
		0x88, 0x9e,
		0x89, 0x12,
		0x8A, 0x9e,
		0x8B, 0x12,
		0x8C, 0x9e,
		0x8D, 0x12, 
		// SYNC/DE Generation 
		0x90, 0xF0,
		0x91, 0x31,
		0x92, 0x1F,
		0x93, 0x7F,
		0x94, 0x23,
		0x95, 0x0E,
		0x96, 0x4C,
		0x97, 0x00,
		0x98, 0x0C,
		0x99, 0x02,
		0x9A, 0x0C,
		0x9B, 0xDF,
		0x9C, 0x12,
		0x9D, 0x23,
		0x9E, 0x36,
		0x9F, 0x00,
		0xA0, 0x09,
		0xA1, 0xF0,
		0xA2, 0xFF,
		0xA3, 0xFF, 
		// Pattern Generation
		0xA8, 0x80,
		// PGEn = regA8[0]
		0xA9, 0x00,
		0xAA, 0x90,
		0xAB, 0x70,
		0xAC, 0x50,
		0xAD, 0x00,
		0xAE, 0x00,
		0xAF, 0x10,
		0xB0, 0x08,
		// HDMI General Control , HDMIMODE REGC0H[0]
		0xC0, 0x01,
		0xC1, 0x00,
		0xC2, 0x0A,
		0xC3, 0x08,
		0xC4, 0x00,
		0xC5, 0x00,
		0xC6, 0x03,
		0xC7, 0x00,
		0xC8, 0x00,
		0xC9, 0x03,
		0xCA, 0x00,
		0xCB, 0x00,
		0xCC, 0x00,
		0xCD, 0x03,
		0xCE, 0x03,
		0xCF, 0x00,
		0xD0, 0x00,
		// Audio Channel Registers
		0xE0, 0xD1,	// [7:6]=REGAudSWL, [5]=REGSPDIFTC, [4]=REGAudSel, [3:0]=REGAudioEn
		0xE1, 0x01,
		0xE2, 0x00,	// SRC0/1/2/3 maps to SRC0
		0xE3, 0x00,
		0xE4, 0x08,  
		// Change to Bank 1
		0x0F, 0x01,
		// N/CTS Packet 
		0x30, 0x00,
		0x31, 0x00,
		0x32, 0x00,
		0x33, 0x00,
		0x34, 0x60,
		0x35, 0x00,
		// Audio Sample Packet
		0x36, 0x00,
		0x37, 0x30,
		// Null Packet
		0x38, 0x00, 
		0x39, 0x00, 
		0x3A, 0x00, 
		0x3B, 0x00, 
		0x3C, 0x00, 
		0x3D, 0x00, 
		0x3E, 0x00,
		0x3F, 0x00,
		0x40, 0x00, 
		0x41, 0x00, 
		0x42, 0x00, 
		0x43, 0x00, 
		0x44, 0x00, 
		0x45, 0x00, 
		0x46, 0x00,
		0x47, 0x00,
		0x48, 0x00, 
		0x49, 0x00, 
		0x4A, 0x00, 
		0x4B, 0x00, 
		0x4C, 0x00, 
		0x4D, 0x00, 
		0x4E, 0x00,
		0x4F, 0x00,
		0x50, 0x00, 
		0x51, 0x00, 
		0x52, 0x00, 
		0x53, 0x00, 
		0x54, 0x00, 
		0x55, 0x00, 
		0x56, 0x00,
		// AVI Packet
		0x58, 0x20, 
		0x59, 0x08, 
		0x5A, 0x00, 
		0x5B, 0x02,	// Video Format Code
		0x5C, 0x00,
		0x5D, 0x45,	// CheckSum 
		0x5E, 0x00, 
		0x5F, 0x00, 
		0x60, 0x00, 
		0x61, 0x00,
		0x62, 0x00,
		0x63, 0x00,
		0x64, 0x00,
		0x65, 0x00,
		// AUDIO Info Frame
		0x68, 0x01,	// should be 00, temp assign for solving MulCh problem
		0x69, 0x00,
		0x6A, 0x00,
		0x6B, 0x00,	// InfoCA
		0x6C, 0x00,
		0x6D, 0x70,	// CA=00
		// SPD Packet
		0x70, 0x00, 	// Checksum
		0x71, 0x00, 
		0x72, 0x00, 
		0x73, 0x00, 
		0x74, 0x00, 
		0x75, 0x00, 
		0x76, 0x00,
		0x78, 0x00, 
		0x79, 0x00, 
		0x7A, 0x00, 
		0x7B, 0x00,
		0x7C, 0x00,
		0x7D, 0x00,
		0x7E, 0x00, 
		0x7F, 0x00, 
		0x80, 0x00, 
		0x81, 0x00,
		0x82, 0x00,
		0x83, 0x00,
		0x84, 0x00,
		0x85, 0x00,
		0x86, 0x00,
		0x87, 0x00,
		0x88, 0x00,
		0x89, 0x00,
		// MPEG Info Frame
		0x8A, 0x00,
		0x8B, 0x00,
		0x8C, 0x00,
		0x8D, 0x00,
		0x8E, 0x00,
		0x8F, 0x00, 	// Checksum
		// Audio Channel Status
		0x91, 0x00,
		0x92, 0x80,
		0x93, 0x04,
		0x94, 0x21,
		0x95, 0x43, 
		0x96, 0x65, 
		0x97, 0x87,
		0x98, 0x0E,	// Sampling Frequency
		0x99, 0x1B, 
		// Change to Bank 0
		0x0F, 0x00,
		// HDMITX VCLK Reset Disable
		0x04, 0x14,
		0xF3, 0x00,
		0xF4, 0x00,
		// All HDMITX Reset Disable
		0x04, 0x00,
		// Enable HDMITX AFE
		0x61, 0x00,
		0x62, 0x89,
		// set audio mute
		0xE0, 0xC0,
		0x04, 0xC4
	};
	
	i2c_addr = i2c_addr >> 1;
	I2CBUF[0] = 2;
	for(i = 0; i < (ARRAY_COUNT(P480)/2); i++) {
		I2CBUF[1] = P480[i*2]&0x0ff;
		I2CBUF[2] = P480[(i*2)+1]&0x0ff;
		I2C_Write1(i2c_addr,I2CBUF);
		BusyWaiting(70000);
	}
}

void SET_CAT_HDMI_576P()
{
	unsigned int	I2CBUF[5];
	unsigned int	i = 0;
	unsigned int	i2c_addr = 0x98;	
	//Change to Bank 0
	unsigned int P576[]={
		0x0F, 0x00,
		// HDMITX Reset Enable
		0x04, 0xFF,
		//$$$ Idle 1FF,
		//$$$ Idle 1FF,
		// HDMITX Reset Disable
		0x04, 0x1F,
		// Switch to PC program DDC mode
		0x10, 0x01,
		// HDCP Registers, reg20[0] CPDesired, reg20h[1] 1.1Feature
		0x20, 0x00,
		0x22, 0x02, 
		// Clock Control Registers
		0x58, 0x11,
		0x59, 0x08,//edge should be 1
		// input format 
		0x70, 0x48,
		// input color mode reg70[7:6], sync_emb reg70[3]
		0x71, 0x00,
		// EnUdFilt reg72[6], CSCSel reg72h[1:0] 
		0x72, 0x00,  
		0x73, 0x10,
		0x74, 0x80,
		0x75, 0x10,
		// CSC Matrix 
		0x76, 0x00,
		0x77, 0x08,
		0x78, 0x6a,
		0x79, 0x1a,
		0x7A, 0x4f,
		0x7B, 0x1d,
		0x7C, 0x00,
		0x7D, 0x08,
		0x7E, 0xf7,
		0x7F, 0x0a,
		0x80, 0x00,
		0x81, 0x00,
		0x82, 0x00,
		0x83, 0x08,
		0x84, 0x00,
		0x85, 0x00,
		0x86, 0xdb,
		0x87, 0x0d,
		0x88, 0x9e,
		0x89, 0x12,
		0x8A, 0x9e,
		0x8B, 0x12,
		0x8C, 0x9e,
		0x8D, 0x12, 
		// SYNC/DE Generation 
		0x90, 0x00,
		0x91, 0xFF,
		0x92, 0x1F,
		0x93, 0x7F,
		0x94, 0x23,
		0x95, 0x0a,
		0x96, 0x4a,
		0x97, 0x00,
		0x98, 0x0C,
		0x99, 0x02,
		0x9A, 0x0C,
		0x9B, 0xDF,
		0x9C, 0x12,
		0x9D, 0x23,
		0x9E, 0x36,
		0x9F, 0x00,
		0xA0, 0x04,//0x5
		0xA1, 0x90,//0xa
		0xA2, 0xFF,
		0xA3, 0xFF, 
		// Pattern Generation
		0xA8, 0x80,
		// PGEn = regA8[0]
		0xA9, 0x00,
		0xAA, 0x90,
		0xAB, 0x70,
		0xAC, 0x50,
		0xAD, 0x00,
		0xAE, 0x00,
		0xAF, 0x10,
		0xB0, 0x08,
		// HDMI General Control , HDMIMODE REGC0H[0]
		0xC0, 0x01,
		0xC1, 0x00,
		0xC2, 0x0A,
		0xC3, 0x08,
		0xC4, 0x00,
		0xC5, 0x00,
		0xC6, 0x03,
		0xC7, 0x00,
		0xC8, 0x00,
		0xC9, 0x03,
		0xCA, 0x00,
		0xCB, 0x00,
		0xCC, 0x00,
		0xCD, 0x03,
		0xCE, 0x03,
		0xCF, 0x00,
		0xD0, 0x00,
		// Audio Channel Registers
		0xE0, 0xD1,	// [7:6]=REGAudSWL, [5]=REGSPDIFTC, [4]=REGAudSel, [3:0]=REGAudioEn
		0xE1, 0x01,
		0xE2, 0x00,	// SRC0/1/2/3 maps to SRC0
		0xE3, 0x00,
		0xE4, 0x08,  
		// Change to Bank 1
		0x0F, 0x01,
		// N/CTS Packet 
		0x30, 0x00,
		0x31, 0x00,
		0x32, 0x00,
		0x33, 0x00,
		0x34, 0x60,
		0x35, 0x00,
		// Audio Sample Packet
		0x36, 0x00,
		0x37, 0x30,
		// Null Packet
		0x38, 0x00, 
		0x39, 0x00, 
		0x3A, 0x00, 
		0x3B, 0x00, 
		0x3C, 0x00, 
		0x3D, 0x00, 
		0x3E, 0x00,
		0x3F, 0x00,
		0x40, 0x00, 
		0x41, 0x00, 
		0x42, 0x00, 
		0x43, 0x00, 
		0x44, 0x00, 
		0x45, 0x00, 
		0x46, 0x00,
		0x47, 0x00,
		0x48, 0x00, 
		0x49, 0x00, 
		0x4A, 0x00, 
		0x4B, 0x00, 
		0x4C, 0x00, 
		0x4D, 0x00, 
		0x4E, 0x00,
		0x4F, 0x00,
		0x50, 0x00, 
		0x51, 0x00, 
		0x52, 0x00, 
		0x53, 0x00, 
		0x54, 0x00, 
		0x55, 0x00, 
		0x56, 0x00,
		// AVI Packet
		0x58, 0x20, 
		0x59, 0x08, 
		0x5A, 0x00, 
		0x5B, 0x11,	// Video Format Code
		0x5C, 0x00,
		0x5D, 0x36,	// CheckSum 
		0x5E, 0x00, 
		0x5F, 0x00, 
		0x60, 0x00, 
		0x61, 0x00,
		0x62, 0x00,
		0x63, 0x00,
		0x64, 0x00,
		0x65, 0x00,
		// AUDIO Info Frame
		0x68, 0x01,	// should be 00, temp assign for solving MulCh problem
		0x69, 0x00,
		0x6A, 0x00,
		0x6B, 0x00,	// InfoCA
		0x6C, 0x00,
		0x6D, 0x70,	// CA=00
		// SPD Packet
		0x70, 0x00, 	// Checksum
		0x71, 0x00, 
		0x72, 0x00, 
		0x73, 0x00, 
		0x74, 0x00, 
		0x75, 0x00, 
		0x76, 0x00,
		0x78, 0x00, 
		0x79, 0x00, 
		0x7A, 0x00, 
		0x7B, 0x00,
		0x7C, 0x00,
		0x7D, 0x00,
		0x7E, 0x00, 
		0x7F, 0x00, 
		0x80, 0x00, 
		0x81, 0x00,
		0x82, 0x00,
		0x83, 0x00,
		0x84, 0x00,
		0x85, 0x00,
		0x86, 0x00,
		0x87, 0x00,
		0x88, 0x00,
		0x89, 0x00,
		// MPEG Info Frame
		0x8A, 0x00,
		0x8B, 0x00,
		0x8C, 0x00,
		0x8D, 0x00,
		0x8E, 0x00,
		0x8F, 0x00, 	// Checksum
		// Audio Channel Status
		0x91, 0x00,
		0x92, 0x80,
		0x93, 0x04,
		0x94, 0x21,
		0x95, 0x43, 
		0x96, 0x65, 
		0x97, 0x87,
		0x98, 0x0E,	// Sampling Frequency
		0x99, 0x1B, 
		// Change to Bank 0
		0x0F, 0x00,
		// HDMITX VCLK Reset Disable
		0x04, 0x14,
		0xF3, 0x00,
		0xF4, 0x00,
		// All HDMITX Reset Disable
		0x04, 0x00,
		// Enable HDMITX AFE
		0x61, 0x00,
		0x62, 0x89,
		// set audio mute
		0xE0, 0xC0,
		0x04, 0xC4
	};

	i2c_addr = i2c_addr >> 1;
	I2CBUF[0] = 2;
	for(i = 0; i < (ARRAY_COUNT(P576)/2); i++) {
		I2CBUF[1] = P576[i*2]&0x0ff;
		I2CBUF[2] = P576[(i*2)+1]&0x0ff;
		I2C_Write1(i2c_addr,I2CBUF);
		BusyWaiting(70000);
	}
}
#endif
#endif

#ifdef CONFIG_REALTEK_CPU_MARS
void setup_boot_image_mars(void)
{
	// reset video exception entry point
	*(volatile unsigned int *)0xa00000a4 = 0x00801a3c;
	*(volatile unsigned int *)0xa00000a8 = 0x00185a37;
	*(volatile unsigned int *)0xa00000ac = 0x08004003;
	*(volatile unsigned int *)0xa00000b0 = 0x00000000;

	if (logo_info.mode == 0) {
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : disable interrupt
		*(volatile unsigned int *)0xb801801c = 0xfffffffe ;
		// VO_FC_reg : disable  go bit
		*(volatile unsigned int *)0xb8005014 = 0x01 ;    
		
		// VO_MIX1_reg : enable SUB1
		*(volatile unsigned int *)0xb8005008 = 0x41 ;
		// VO_M1_SIZE_reg : set mixer1's width 720 and height 480 
		*(volatile unsigned int *)0xb8005994 = 0x2d01e0 ;
		
		// VO_SUB1_PXDT_reg  VO_SUB1_PXDB_reg : top and bottom address 
		*(volatile unsigned int *)0xb80054ac = 0x5170;
		*(volatile unsigned int *)0xb80054b0 = 0x5170 + logo_info.size;
		// VO_SUB1_CLUT_reg : color lookup table address  ** 8 bytes aligned **
		*(volatile unsigned int *)0xb80054a4 = 0x17e0;
		// VO_SUB1_reg : used for DVD subpicture 
		*(volatile unsigned int *)0xb8005490 = 0x800001 ;
		// VO_SUB1_SUBP_reg : sub1 width 720
		*(volatile unsigned int *)0xb8005494 = 0x2d0 ;
		// VO_SUB1_SIZE_reg : canvas width 720 and height 480
		*(volatile unsigned int *)0xb8005498 = 0x2d01e0 ;
		
		// VO_CH1_reg : enable reinterlace 
		*(volatile unsigned int *)0xb80059f0 = 0x11 ;
		// VO_MODE_reg : enable ch1 interlace and progressive 
		*(volatile unsigned int *)0xb8005000 = 0x7 ;
		// VO_OUT_reg  : enable i and p port 
		*(volatile unsigned int *)0xb8005004 = 0x465 ;
		
		// TVE_TV_525p_700
		*(volatile unsigned int *)0xb8000020 = 0x11820 ;
		*(volatile unsigned int *)0xb801820c = 0x124 ;
		*(volatile unsigned int *)0xb8018208 = 0x1fc0 ;
		*(volatile unsigned int *)0xb8018210 = 0x200000 ;
		*(volatile unsigned int *)0xb8018200 = 0x7bbefb ;
		*(volatile unsigned int *)0xb8018204 = 0x7bbefb ;
		*(volatile unsigned int *)0xb8018218 = 0x597e ;
		*(volatile unsigned int *)0xb801821c = 0x597e ;
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb8018038 = 0x1 ;
		*(volatile unsigned int *)0xb8018a00 = 0x200 ;
		*(volatile unsigned int *)0xb8018880 = 0x1f ;
		*(volatile unsigned int *)0xb8018884 = 0x7c ;
		*(volatile unsigned int *)0xb8018888 = 0xf0 ;
		*(volatile unsigned int *)0xb801888c = 0x21 ;
		*(volatile unsigned int *)0xb8018890 = 0x0 ;
		*(volatile unsigned int *)0xb8018894 = 0x2 ;
		*(volatile unsigned int *)0xb8018898 = 0x2 ;
		*(volatile unsigned int *)0xb801889c = 0x3f ;
		*(volatile unsigned int *)0xb80188a0 = 0x2 ;
		*(volatile unsigned int *)0xb80188a8 = 0x8d ;
		*(volatile unsigned int *)0xb80188ac = 0x78 ;
		*(volatile unsigned int *)0xb80188b0 = 0x10 ;
		*(volatile unsigned int *)0xb80188b4 = 0x7 ;
		*(volatile unsigned int *)0xb80188b8 = 0x1c ;
		*(volatile unsigned int *)0xb80188b8 = 0x1c ;
		*(volatile unsigned int *)0xb8018984 = 0x20 ;
		*(volatile unsigned int *)0xb801898c = 0x2 ;
		*(volatile unsigned int *)0xb80188bc = 0x0 ;
		*(volatile unsigned int *)0xb80188c8 = 0xc8 ;
		*(volatile unsigned int *)0xb80188cc = 0x0 ;
		*(volatile unsigned int *)0xb80188d0 = 0x0 ;
		*(volatile unsigned int *)0xb80188e0 = 0x8 ;
		*(volatile unsigned int *)0xb80188e4 = 0x31 ;
		*(volatile unsigned int *)0xb80188e8 = 0x6 ;
		*(volatile unsigned int *)0xb80188ec = 0x6 ;
		*(volatile unsigned int *)0xb80188f0 = 0xb3 ;
		*(volatile unsigned int *)0xb80188f4 = 0x3 ;
		*(volatile unsigned int *)0xb80188f8 = 0x59 ;
		*(volatile unsigned int *)0xb80189c0 = 0x64 ;
		*(volatile unsigned int *)0xb80189c4 = 0x2d ;
		*(volatile unsigned int *)0xb80189c8 = 0x7 ;
		*(volatile unsigned int *)0xb80189cc = 0x14 ;
		*(volatile unsigned int *)0xb8018920 = 0x0 ;
		*(volatile unsigned int *)0xb8018924 = 0x3a ;
		*(volatile unsigned int *)0xb8018928 = 0x11 ;
		*(volatile unsigned int *)0xb801892c = 0x4b ;
		*(volatile unsigned int *)0xb8018930 = 0x11 ;
		*(volatile unsigned int *)0xb8018934 = 0x3c ;
		*(volatile unsigned int *)0xb8018938 = 0x1b ;
		*(volatile unsigned int *)0xb801893c = 0x1b ;
		*(volatile unsigned int *)0xb8018940 = 0x24 ;
		*(volatile unsigned int *)0xb8018944 = 0x7 ;
		*(volatile unsigned int *)0xb8018948 = 0xf8 ;
		*(volatile unsigned int *)0xb801894c = 0x0 ;
		*(volatile unsigned int *)0xb8018950 = 0x0 ;
		*(volatile unsigned int *)0xb8018954 = 0xf ;
		*(volatile unsigned int *)0xb8018958 = 0xf ;
		*(volatile unsigned int *)0xb801895c = 0x60 ;
		*(volatile unsigned int *)0xb8018960 = 0xa0 ;
		*(volatile unsigned int *)0xb8018964 = 0x54 ;
		*(volatile unsigned int *)0xb8018968 = 0xff ;
		*(volatile unsigned int *)0xb801896c = 0x3 ;
		*(volatile unsigned int *)0xb8018130 = 0x8 ;
		*(volatile unsigned int *)0xb8018990 = 0x0 ;
		*(volatile unsigned int *)0xb80189d0 = 0xc ;
		*(volatile unsigned int *)0xb80189d4 = 0x4b ;
		*(volatile unsigned int *)0xb80189d8 = 0x7a ;
		*(volatile unsigned int *)0xb80189dc = 0x2b ;
		*(volatile unsigned int *)0xb80189e0 = 0x85 ;
		*(volatile unsigned int *)0xb80189e4 = 0xaa ;
		*(volatile unsigned int *)0xb80189e8 = 0x5a ;
		*(volatile unsigned int *)0xb80189ec = 0x62 ;
		*(volatile unsigned int *)0xb80189f0 = 0x84 ;
		*(volatile unsigned int *)0xb8018004 = 0x2832359 ;
		*(volatile unsigned int *)0xb8018008 = 0xa832359 ;
		*(volatile unsigned int *)0xb801800c = 0xa832359 ;
		*(volatile unsigned int *)0xb80180b0 = 0x306505 ;
		*(volatile unsigned int *)0xb80180b4 = 0x316 ;
		*(volatile unsigned int *)0xb8018050 = 0x8106310 ;
		*(volatile unsigned int *)0xb801805c = 0x815904 ;
		*(volatile unsigned int *)0xb8018060 = 0x91ca0b ;
		*(volatile unsigned int *)0xb8018054 = 0x3820a352 ;
		*(volatile unsigned int *)0xb8018064 = 0x82aa09 ;
		*(volatile unsigned int *)0xb8018058 = 0x3820a352 ;
		*(volatile unsigned int *)0xb801806c = 0x82aa09 ;
		*(volatile unsigned int *)0xb8018070 = 0xbe8be8 ;
		*(volatile unsigned int *)0xb8018048 = 0x3f6 ;
		*(volatile unsigned int *)0xb8018040 = 0x7 ;
		*(volatile unsigned int *)0xb8018040 = 0x18 ;
		*(volatile unsigned int *)0xb8018074 = 0x22d43f ;
		*(volatile unsigned int *)0xb8018078 = 0x22cc88 ;
		*(volatile unsigned int *)0xb801807c = 0x22cc89 ;
		*(volatile unsigned int *)0xb8018108 = 0x80009404 ;
		*(volatile unsigned int *)0xb801810c = 0x80008000 ;
		*(volatile unsigned int *)0xb8018110 = 0x93929392 ;
		*(volatile unsigned int *)0xb8018114 = 0x8d408c70 ;
		*(volatile unsigned int *)0xb8018118 = 0x28c70 ;
		*(volatile unsigned int *)0xb8018104 = 0x80008000 ;
		*(volatile unsigned int *)0xb80180f0 = 0x27e1800 ;
		*(volatile unsigned int *)0xb8018100 = 0x7f400e0 ;
		*(volatile unsigned int *)0xb80180e0 = 0x1a0817a0 ;
		*(volatile unsigned int *)0xb80180ec = 0x28a1bb0 ;
		*(volatile unsigned int *)0xb80180e4 = 0x256d0d0 ;
		*(volatile unsigned int *)0xb80180e8 = 0x2080000 ;
		*(volatile unsigned int *)0xb8018044 = 0x1 ;
		*(volatile unsigned int *)0xb8018080 = 0x20e0024e ;
		*(volatile unsigned int *)0xb8018088 = 0x804232a ;
		*(volatile unsigned int *)0xb801808c = 0x9ab ;
		*(volatile unsigned int *)0xb8018010 = 0x3ffe ;
		*(volatile unsigned int *)0xb8018130 = 0xc ;
		*(volatile unsigned int *)0xb8018010 = 0x30cb ;
		*(volatile unsigned int *)0xb8018130 = 0x5 ;
		*(volatile unsigned int *)0xb80181b0 = 0x81ee34f ;
		*(volatile unsigned int *)0xb80181cc = 0xbe8800 ;
		*(volatile unsigned int *)0xb80181d0 = 0xa0c82c ;
		*(volatile unsigned int *)0xb80181d4 = 0xbe8be8 ;
		*(volatile unsigned int *)0xb80181d8 = 0x8002000 ;
		*(volatile unsigned int *)0xb80181b4 = 0xfe ;
		*(volatile unsigned int *)0xb8018000 = 0x3c0 ;
		
		// HDMI reg
		*(volatile unsigned int *)0xb8000020 = 0x520000 ;
		*(volatile unsigned int *)0xb80180b8 = 0x3a002000 ;
		*(volatile unsigned int *)0xb80180bc = 0x8022046 ;
		*(volatile unsigned int *)0xb80180c0 = 0x201a008 ;
		*(volatile unsigned int *)0xb80180c4 = 0x2032008 ;
		*(volatile unsigned int *)0xb80180c8 = 0x2002000 ;
		*(volatile unsigned int *)0xb80180cc = 0x2002000 ;
		*(volatile unsigned int *)0xb80180d0 = 0x8d4a074 ;
		*(volatile unsigned int *)0xb800d020 = 0x0 ;
		*(volatile unsigned int *)0xb800d024 = 0xf9c92a ;
		*(volatile unsigned int *)0xb800d028 = 0xa05f31 ;
		*(volatile unsigned int *)0xb800d02c = 0x121b99a ;
		*(volatile unsigned int *)0xb800d200 = 0x374fc84 ;
		*(volatile unsigned int *)0xb800d034 = 0x3f ;
		*(volatile unsigned int *)0xb800d030 = 0x924 ;
		*(volatile unsigned int *)0xb800d040 = 0x8ca0f0 ;
		*(volatile unsigned int *)0xb800d044 = 0xa70 ;
		*(volatile unsigned int *)0xb800d0bc = 0x14000000 ;
		*(volatile unsigned int *)0xb800d0b8 = 0x7d04a ;
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb8018000 = 0x3c0 ;
		*(volatile unsigned int *)0xb800d07c = 0xd0282 ;
		*(volatile unsigned int *)0xb800d080 = 0xd83065 ;
		*(volatile unsigned int *)0xb800d084 = 0x2 ;
		*(volatile unsigned int *)0xb800d088 = 0x0 ;
		*(volatile unsigned int *)0xb800d08c = 0x0 ;
		*(volatile unsigned int *)0xb800d090 = 0x0 ;
		*(volatile unsigned int *)0xb800d094 = 0x0 ;
		*(volatile unsigned int *)0xb800d098 = 0x0 ;
		*(volatile unsigned int *)0xb800d09c = 0x0 ;
		*(volatile unsigned int *)0xb800d0b0 = 0x7762 ;
		*(volatile unsigned int *)0xb800d0a0 = 0x11 ;
		*(volatile unsigned int *)0xb800d0a4 = 0x7 ;
		
		// TVE_AV_CTRL_reg : disable i_black
		*(volatile unsigned int *)0xb8018040 = 0x4 ;
		// TVE_AV_CTRL_reg : disable p_black
		*(volatile unsigned int *)0xb8018040 = 0x2 ;
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : enable i  channel interrupt
		*(volatile unsigned int *)0xb801801c = 0x5 ;
		// VO_FC_reg : enable mix1's go bit
		*(volatile unsigned int *)0xb8005014 = 0x3 ;
	} else if (logo_info.mode == 1) {
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : disable interrupt
		*(volatile unsigned int *)0xb801801c = 0xfffffffe ;
		// VO_FC_reg : disable  go bit
		*(volatile unsigned int *)0xb8005014 = 0x01 ;    
		
		// VO_MIX1_reg : enable SUB1
		*(volatile unsigned int *)0xb8005008 = 0x41 ;
		// VO_M1_SIZE_reg : set mixer1's width 720 and height 576 
		*(volatile unsigned int *)0xb8005994 = 0x2d0240 ;
		
		// VO_SUB1_PXDT_reg  VO_SUB1_PXDB_reg : top and bottom address 
		*(volatile unsigned int *)0xb80054ac = 0x5170;
		*(volatile unsigned int *)0xb80054b0 = 0x5170 + logo_info.size;
		// VO_SUB1_CLUT_reg : color lookup table address  ** 8 bytes aligned **
		*(volatile unsigned int *)0xb80054a4 = 0x17e0;
		// VO_SUB1_reg : used for DVD subpicture 
		*(volatile unsigned int *)0xb8005490 = 0x800001 ;
		// VO_SUB1_SUBP_reg : sub1 width 720
		*(volatile unsigned int *)0xb8005494 = 0x2d0 ;
		// VO_SUB1_SIZE_reg : canvas width 720 and height 576
		*(volatile unsigned int *)0xb8005498 = 0x2d0240 ;
		
		// VO_CH1_reg : enable reinterlace 
		*(volatile unsigned int *)0xb80059f0 = 0x11 ;
		// VO_MODE_reg : enable ch1 interlace and progressive
		*(volatile unsigned int *)0xb8005000 = 0x7 ;
		// VO_OUT_reg  : enable i and p port 
		*(volatile unsigned int *)0xb8005004 = 0x465 ;
		
		// TVE_TV_625p
		*(volatile unsigned int *)0xb8000020 = 0x11820 ;
		*(volatile unsigned int *)0xb801820c = 0x124 ;
		*(volatile unsigned int *)0xb8018208 = 0x1fc0 ;
		*(volatile unsigned int *)0xb8018210 = 0x200000 ;
		*(volatile unsigned int *)0xb8018200 = 0x7bbefb ;
		*(volatile unsigned int *)0xb8018204 = 0x7bbefb ;
		*(volatile unsigned int *)0xb8018218 = 0x597e ;
		*(volatile unsigned int *)0xb801821c = 0x597e ;
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb8018038 = 0x1 ;
		*(volatile unsigned int *)0xb8018a00 = 0x205 ;
		*(volatile unsigned int *)0xb8018880 = 0xcb ;
		*(volatile unsigned int *)0xb8018884 = 0x8a ;
		*(volatile unsigned int *)0xb8018888 = 0x9 ;
		*(volatile unsigned int *)0xb801888c = 0x2a ;
		*(volatile unsigned int *)0xb8018890 = 0x0 ;
		*(volatile unsigned int *)0xb8018894 = 0x0 ;
		*(volatile unsigned int *)0xb8018898 = 0x0 ;
		*(volatile unsigned int *)0xb801889c = 0x9b ;
		*(volatile unsigned int *)0xb80188a0 = 0x2 ;
		*(volatile unsigned int *)0xb80188a8 = 0x78 ;
		*(volatile unsigned int *)0xb80188ac = 0x78 ;
		*(volatile unsigned int *)0xb80188b0 = 0x10 ;
		*(volatile unsigned int *)0xb80188b4 = 0x3 ;
		*(volatile unsigned int *)0xb80188b8 = 0x1d ;
		*(volatile unsigned int *)0xb80188b8 = 0x1d ;
		*(volatile unsigned int *)0xb8018984 = 0x20 ;
		*(volatile unsigned int *)0xb801898c = 0x2 ;
		*(volatile unsigned int *)0xb80188bc = 0x0 ;
		*(volatile unsigned int *)0xb80188c8 = 0xd7 ;
		*(volatile unsigned int *)0xb80188cc = 0x29 ;
		*(volatile unsigned int *)0xb80188d0 = 0x3 ;
		*(volatile unsigned int *)0xb80188e0 = 0x9 ;
		*(volatile unsigned int *)0xb80188e4 = 0x31 ;
		*(volatile unsigned int *)0xb80188e8 = 0x38 ;
		*(volatile unsigned int *)0xb80188ec = 0x6 ;
		*(volatile unsigned int *)0xb80188f0 = 0xbf ;
		*(volatile unsigned int *)0xb80188f4 = 0x3 ;
		*(volatile unsigned int *)0xb80188f8 = 0x5f ;
		*(volatile unsigned int *)0xb80189c0 = 0x5c ;
		*(volatile unsigned int *)0xb80189c4 = 0x40 ;
		*(volatile unsigned int *)0xb80189c8 = 0x24 ;
		*(volatile unsigned int *)0xb80189cc = 0x1c ;
		*(volatile unsigned int *)0xb8018920 = 0x0 ;
		*(volatile unsigned int *)0xb8018924 = 0x39 ;
		*(volatile unsigned int *)0xb8018928 = 0x22 ;
		*(volatile unsigned int *)0xb801892c = 0x5a ;
		*(volatile unsigned int *)0xb8018930 = 0x22 ;
		*(volatile unsigned int *)0xb8018934 = 0xa8 ;
		*(volatile unsigned int *)0xb8018938 = 0x1c ;
		*(volatile unsigned int *)0xb801893c = 0x34 ;
		*(volatile unsigned int *)0xb8018940 = 0x14 ;
		*(volatile unsigned int *)0xb8018944 = 0x3 ;
		*(volatile unsigned int *)0xb8018948 = 0xfe ;
		*(volatile unsigned int *)0xb801894c = 0x1 ;
		*(volatile unsigned int *)0xb8018950 = 0x54 ;
		*(volatile unsigned int *)0xb8018954 = 0xfe ;
		*(volatile unsigned int *)0xb8018958 = 0x7e ;
		*(volatile unsigned int *)0xb801895c = 0x60 ;
		*(volatile unsigned int *)0xb8018960 = 0x80 ;
		*(volatile unsigned int *)0xb8018964 = 0x47 ;
		*(volatile unsigned int *)0xb8018968 = 0x55 ;
		*(volatile unsigned int *)0xb801896c = 0x1 ;
		*(volatile unsigned int *)0xb8018130 = 0x8 ;
		*(volatile unsigned int *)0xb8018990 = 0x0 ;
		*(volatile unsigned int *)0xb80189d0 = 0x0 ;
		*(volatile unsigned int *)0xb80189d4 = 0x3f ;
		*(volatile unsigned int *)0xb80189d8 = 0x71 ;
		*(volatile unsigned int *)0xb80189dc = 0x20 ;
		*(volatile unsigned int *)0xb80189e0 = 0x80 ;
		*(volatile unsigned int *)0xb80189e4 = 0xa4 ;
		*(volatile unsigned int *)0xb80189e8 = 0x50 ;
		*(volatile unsigned int *)0xb80189ec = 0x57 ;
		*(volatile unsigned int *)0xb80189f0 = 0x75 ;
		*(volatile unsigned int *)0xb8018004 = 0x29c235f ;
		*(volatile unsigned int *)0xb8018008 = 0xa9c235f ;
		*(volatile unsigned int *)0xb801800c = 0xa9c235f ;
		*(volatile unsigned int *)0xb80180b0 = 0x29ae6d ;
		*(volatile unsigned int *)0xb80180b4 = 0x31b ;
		*(volatile unsigned int *)0xb8018050 = 0x80fa30d ;
		*(volatile unsigned int *)0xb801805c = 0x816935 ;
		*(volatile unsigned int *)0xb8018060 = 0x94fa6e ;
		*(volatile unsigned int *)0xb8018054 = 0x381f234c ;
		*(volatile unsigned int *)0xb8018064 = 0x82ca6b ;
		*(volatile unsigned int *)0xb8018058 = 0x381f234c ;
		*(volatile unsigned int *)0xb801806c = 0x82ca6b ;
		*(volatile unsigned int *)0xb8018048 = 0x3f6 ;
		*(volatile unsigned int *)0xb8018040 = 0x7 ;
		*(volatile unsigned int *)0xb8018040 = 0x18 ;
		*(volatile unsigned int *)0xb8018074 = 0x22d43c ;
		*(volatile unsigned int *)0xb8018078 = 0x22cc88 ;
		*(volatile unsigned int *)0xb801807c = 0x22cc89 ;
		*(volatile unsigned int *)0xb8018108 = 0x80009404 ;
		*(volatile unsigned int *)0xb801810c = 0x80008000 ;
		*(volatile unsigned int *)0xb8018110 = 0x93929392 ;
		*(volatile unsigned int *)0xb8018114 = 0x8d408c70 ;
		*(volatile unsigned int *)0xb8018118 = 0x28c70 ;
		*(volatile unsigned int *)0xb8018104 = 0x80008000 ;
		*(volatile unsigned int *)0xb80180f0 = 0x27e1800 ;
		*(volatile unsigned int *)0xb8018100 = 0x7f400e0 ;
		*(volatile unsigned int *)0xb80180e0 = 0x1a0817a0 ;
		*(volatile unsigned int *)0xb80180ec = 0x28a1bb0 ;
		*(volatile unsigned int *)0xb80180e4 = 0x256d0d0 ;
		*(volatile unsigned int *)0xb8018044 = 0x1 ;
		*(volatile unsigned int *)0xb8018080 = 0x20290242 ;
		*(volatile unsigned int *)0xb8018088 = 0x800e32a ;
		*(volatile unsigned int *)0xb801808c = 0x824 ;
		*(volatile unsigned int *)0xb8018010 = 0x3ffe ;
		*(volatile unsigned int *)0xb8018130 = 0xc ;
		*(volatile unsigned int *)0xb8018010 = 0x30c9 ;
		*(volatile unsigned int *)0xb8018130 = 0x5 ;
		*(volatile unsigned int *)0xb80181b0 = 0x81e634d ;
		*(volatile unsigned int *)0xb80181cc = 0xbe8800 ;
		*(volatile unsigned int *)0xb80181d0 = 0xa6c82c ;
		*(volatile unsigned int *)0xb80181d4 = 0xbe8be8 ;
		*(volatile unsigned int *)0xb80181d8 = 0x8002000 ;
		*(volatile unsigned int *)0xb80181b4 = 0xfe ;
		*(volatile unsigned int *)0xb8018000 = 0x3c0 ;
		
		// HDMI reg
		*(volatile unsigned int *)0xb8000020 = 0x520000 ;
		*(volatile unsigned int *)0xb80180b8 = 0x3a002000 ;
		*(volatile unsigned int *)0xb80180bc = 0x8d62038 ;
		*(volatile unsigned int *)0xb80180c0 = 0x29c2358 ;
		*(volatile unsigned int *)0xb80180c4 = 0x2012358 ;
		*(volatile unsigned int *)0xb80180c8 = 0x2002000 ;
		*(volatile unsigned int *)0xb80180cc = 0x2002000 ;
		*(volatile unsigned int *)0xb80180d0 = 0x8d3206e ;
		*(volatile unsigned int *)0xb800d020 = 0x0 ;
		*(volatile unsigned int *)0xb800d024 = 0xf9c92a ;
		*(volatile unsigned int *)0xb800d028 = 0xa05f31 ;
		*(volatile unsigned int *)0xb800d02c = 0x121b99a ;
		*(volatile unsigned int *)0xb800d200 = 0x374fc84 ;
		*(volatile unsigned int *)0xb800d034 = 0x3f ;
		*(volatile unsigned int *)0xb800d030 = 0x924 ;
		*(volatile unsigned int *)0xb800d040 = 0x8ca0f0 ;
		*(volatile unsigned int *)0xb800d044 = 0xa70 ;
		*(volatile unsigned int *)0xb800d0bc = 0x14000000 ;
		*(volatile unsigned int *)0xb800d0b8 = 0x7d04a ;
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb8018000 = 0x3c0 ;
		*(volatile unsigned int *)0xb800d07c = 0xd0282 ;
		*(volatile unsigned int *)0xb800d080 = 0xd83056 ;
		*(volatile unsigned int *)0xb800d084 = 0x11 ;
		*(volatile unsigned int *)0xb800d088 = 0x0 ;
		*(volatile unsigned int *)0xb800d08c = 0x0 ;
		*(volatile unsigned int *)0xb800d090 = 0x0 ;
		*(volatile unsigned int *)0xb800d094 = 0x0 ;
		*(volatile unsigned int *)0xb800d098 = 0x0 ;
		*(volatile unsigned int *)0xb800d09c = 0x0 ;
		*(volatile unsigned int *)0xb800d0b0 = 0x7762 ;
		*(volatile unsigned int *)0xb800d0a0 = 0x11 ;
		*(volatile unsigned int *)0xb800d0a4 = 0x7 ;
		
		// TVE_AV_CTRL_reg : disable i_black
		*(volatile unsigned int *)0xb8018040 = 0x4 ;
		// TVE_AV_CTRL_reg : disable p_black
		*(volatile unsigned int *)0xb8018040 = 0x2 ;
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : enable i  channel interrupt
		*(volatile unsigned int *)0xb801801c = 0x5 ;
		// VO_FC_reg : enable mix1's go bit
		*(volatile unsigned int *)0xb8005014 = 0x3 ;
	} else {
		// not support yet...
	}
}
#endif

#ifdef CONFIG_REALTEK_CPU_JUPITER
#define	ISO_REG_BASE		0xb801b000
#define	ISO_CHIP_INFO1		(ISO_REG_BASE + 0xd28)  //ISO CHIP Information 1

#define	S_BONDING		8
#define	M_BONDING		(0x3 << S_BONDING)
#define	BONDING_QFP_128		(0x0 << S_BONDING)
#define	BONDING_QFP_256		(0x1 << S_BONDING)
#define	BONDING_BGA		(0x2 << S_BONDING)

void setup_boot_image_jupiter(void)
{
	int bonding = (*(volatile unsigned int *)ISO_CHIP_INFO1) & M_BONDING;

	// reset video exception entry point
	*(volatile unsigned int *)0xa00000a4 = 0x00801a3c;
	*(volatile unsigned int *)0xa00000a8 = 0x00155a37;
	*(volatile unsigned int *)0xa00000ac = 0x08004003;
	*(volatile unsigned int *)0xa00000b0 = 0x00000000;

	if (logo_info.mode == 0) {
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : disable interrupt
		*(volatile unsigned int *)0xb801801c = 0xfffffffe ;
		// VO_FC_reg : disable  go bit
		*(volatile unsigned int *)0xb8005014 = 0x01 ;    

		// VO_MIX1_reg : enable SUB1
		*(volatile unsigned int *)0xb8005008 = 0x41 ;
		// VO_M1_SIZE_reg : set mixer1's width 720 and height 480 
		*(volatile unsigned int *)0xb80054ac = 0x2d01e0 ;

		// VO_SUB1_PXDT_reg  VO_SUB1_PXDB_reg : top and bottom address 
		*(volatile unsigned int *)0xb8005208 = 0x3000;
		*(volatile unsigned int *)0xb800520c = 0x3000 + logo_info.size;
		// VO_SUB1_CLUT_reg : color lookup table address  ** 8 bytes aligned **
		*(volatile unsigned int *)0xb8005204 = 0x1880;
		// VO_SUB1_reg : used for DVD subpicture 
		*(volatile unsigned int *)0xb80051ec = 0x800001 ;
		// VO_SUB1_SUBP_reg : sub1 width 720
		*(volatile unsigned int *)0xb80051f4 = 0x2d0 ;
		// VO_SUB1_SIZE_reg : canvas width 720 and height 480
		*(volatile unsigned int *)0xb80051f8 = 0x2d01e0 ;

		// VO_CH1_reg : enable reinterlace 
		*(volatile unsigned int *)0xb80054e0 = 0x11 ;
		// VO_MODE_reg : enable ch1 interlace and progressive 
		*(volatile unsigned int *)0xb8005000 = 0x7 ;
		// VO_OUT_reg  : enable i and p port 
		*(volatile unsigned int *)0xb8005004 = 0x465 ;

		// TVE_TV_525p_700
		if (bonding == BONDING_QFP_128)
		{
			*(volatile unsigned int *)0xb8000020 = 0x800 ;
			*(volatile unsigned int *)0xb8018204 = 0x8208 ;
			if (logo_info.cvbs)
				*(volatile unsigned int *)0xb8018208 = 0x3d0 ;
			else
				*(volatile unsigned int *)0xb8018208 = 0x3f0 ;
			*(volatile unsigned int *)0xb801821c = 0x591e ;
		}
		else
		{
			*(volatile unsigned int *)0xb8000020 = 0x20 ;
			*(volatile unsigned int *)0xb8018200 = 0x0078f3cf; //set video DAC_ABC output current to MAX
			*(volatile unsigned int *)0xb8018208 = 0x1c40 ;
			*(volatile unsigned int *)0xb8018218 = 0x591e ;
		}

		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb8018038 = 0x1 ;
		*(volatile unsigned int *)0xb8018a00 = 0x200 ;
		*(volatile unsigned int *)0xb8018880 = 0x1f ;
		*(volatile unsigned int *)0xb8018884 = 0x7c ;
		*(volatile unsigned int *)0xb8018888 = 0xf0 ;
		*(volatile unsigned int *)0xb801888c = 0x21 ;
		*(volatile unsigned int *)0xb8018890 = 0x0 ;
		*(volatile unsigned int *)0xb8018894 = 0x2 ;
		*(volatile unsigned int *)0xb8018898 = 0x2 ;
		*(volatile unsigned int *)0xb801889c = 0x44 ;
		*(volatile unsigned int *)0xb80188a0 = 0x2 ;
		*(volatile unsigned int *)0xb80188a8 = 0x8d ;
		*(volatile unsigned int *)0xb80188ac = 0x78 ;
		*(volatile unsigned int *)0xb80188b0 = 0x10 ;
		*(volatile unsigned int *)0xb80188b4 = 0x7 ;
		*(volatile unsigned int *)0xb80188b8 = 0x1c ;
		*(volatile unsigned int *)0xb8018984 = 0x20 ;
		*(volatile unsigned int *)0xb801898c = 0x2 ;
		*(volatile unsigned int *)0xb80188bc = 0x0 ;
		*(volatile unsigned int *)0xb80188c8 = 0xc8 ;
		*(volatile unsigned int *)0xb80188cc = 0x0 ;
		*(volatile unsigned int *)0xb80188d0 = 0x0 ;
		*(volatile unsigned int *)0xb80188e0 = 0x8 ;
		*(volatile unsigned int *)0xb80188e4 = 0x31 ;
		*(volatile unsigned int *)0xb80188e8 = 0x6 ;
		*(volatile unsigned int *)0xb80188ec = 0x6 ;
		*(volatile unsigned int *)0xb80188f0 = 0xb3 ;
		*(volatile unsigned int *)0xb80188f4 = 0x3 ;
		*(volatile unsigned int *)0xb80188f8 = 0x59 ;
		*(volatile unsigned int *)0xb80189c0 = 0x64 ;
		*(volatile unsigned int *)0xb80189c4 = 0x2d ;
		*(volatile unsigned int *)0xb80189c8 = 0x7 ;
		*(volatile unsigned int *)0xb80189cc = 0x14 ;
		*(volatile unsigned int *)0xb80189d0 = 0xc ;
		*(volatile unsigned int *)0xb80189d4 = 0x4b ;
		*(volatile unsigned int *)0xb80189d8 = 0x7a ;
		*(volatile unsigned int *)0xb80189dc = 0x2b ;
		*(volatile unsigned int *)0xb80189e0 = 0x85 ;
		*(volatile unsigned int *)0xb80189e4 = 0xaa ;
		*(volatile unsigned int *)0xb80189e8 = 0x5a ;
		*(volatile unsigned int *)0xb80189ec = 0x62 ;
		*(volatile unsigned int *)0xb80189f0 = 0x84 ;
		*(volatile unsigned int *)0xb8018004 = 0x2832359 ;
		*(volatile unsigned int *)0xb80180b0 = 0x306505 ;
		*(volatile unsigned int *)0xb80180b4 = 0x359 ;
		*(volatile unsigned int *)0xb8018050 = 0x8212353 ;
		*(volatile unsigned int *)0xb801805c = 0x815904 ;
		*(volatile unsigned int *)0xb8018060 = 0x91ca0b ;
		*(volatile unsigned int *)0xb8018048 = 0x330 ;
		*(volatile unsigned int *)0xb8018040 = 0x7e ;
		*(volatile unsigned int *)0xb8018040 = 0x27 ;
		*(volatile unsigned int *)0xb8018074 = 0x22cc82 ;

		if (bonding == BONDING_QFP_128)
		{
			*(volatile unsigned int *)0xb8000020 = 0x11020 ;
			*(volatile unsigned int *)0xb8018200 = 0x0078f3cf; //set video DAC_ABC output current to MAX
			if (logo_info.cvbs)
				*(volatile unsigned int *)0xb8018208 = 0x1c50 ;
			else
				*(volatile unsigned int *)0xb8018208 = 0x1c70 ;
			*(volatile unsigned int *)0xb8018218 = 0x591e ;
		}
		else
		{
			*(volatile unsigned int *)0xb8000020 = 0x11800 ;
			*(volatile unsigned int *)0xb8018204 = 0x0078f3cf; //set video DAC_DEF output current to MAX
			*(volatile unsigned int *)0xb8018208 = 0x3c0 ;
			*(volatile unsigned int *)0xb801821c = 0x591e ;
		}

		*(volatile unsigned int *)0xb801820c = 0x1a4 ;
		*(volatile unsigned int *)0xb8018210 = 0x200000 ;
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb8018008 = 0xa832359 ;
		*(volatile unsigned int *)0xb801800c = 0xa832359 ;
		*(volatile unsigned int *)0xb8018054 = 0x3820a352 ;
		*(volatile unsigned int *)0xb8018064 = 0x82aa09 ;
		*(volatile unsigned int *)0xb8018058 = 0x3820a352 ;
		*(volatile unsigned int *)0xb801806c = 0x82aa09 ;
		*(volatile unsigned int *)0xb8018048 = 0xc6 ;
		*(volatile unsigned int *)0xb8018040 = 0x7e ;
		*(volatile unsigned int *)0xb8018040 = 0x27 ;
		*(volatile unsigned int *)0xb8018078 = 0x22cc82 ;
		*(volatile unsigned int *)0xb801807c = 0x22cc82 ;
		*(volatile unsigned int *)0xb8018044 = 0x1 ;
		*(volatile unsigned int *)0xb8018080 = 0x20f8024e ;
		*(volatile unsigned int *)0xb8018088 = 0x804232a ;
		*(volatile unsigned int *)0xb801808c = 0x9ab ;
		*(volatile unsigned int *)0xb80180f0 = 0x27e1800 ;
		*(volatile unsigned int *)0xb8018104 = 0x80008000 ;
		*(volatile unsigned int *)0xb8018108 = 0x80009404 ;
		*(volatile unsigned int *)0xb801810c = 0x80008000 ;
		*(volatile unsigned int *)0xb8018110 = 0x93a09470 ;
		*(volatile unsigned int *)0xb8018114 = 0x8d508c70 ;
		*(volatile unsigned int *)0xb8018118 = 0x28ba0 ;
		*(volatile unsigned int *)0xb80180e0 = 0x16021000; //set Hsync level over 300mV
		*(volatile unsigned int *)0xb80180e4 = 0x256d0d0 ;
		*(volatile unsigned int *)0xb80180e8 = 0x256c000 ;
		*(volatile unsigned int *)0xb80180ec = 0x28a1000 ;
		*(volatile unsigned int *)0xb8018010 = 0x3ffe ;
		*(volatile unsigned int *)0xb8018130 = 0xc ;
		*(volatile unsigned int *)0xb8018010 = 0x304b ;
		*(volatile unsigned int *)0xb8018130 = 0x5 ;
		*(volatile unsigned int *)0xb8018000 = 0x3ea ;

		// DVI reg
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb800d034 = 0x22 ;
		*(volatile unsigned int *)0xb800d0bc = 0x1c000000 ;
		*(volatile unsigned int *)0xb8000020 = 0x5520000 ;
		*(volatile unsigned int *)0xb800d040 = 0x15a3f0aa ;
		*(volatile unsigned int *)0xb800d044 = 0x1 ;
		*(volatile unsigned int *)0xb80180b8 = 0x3a002000 ;
		*(volatile unsigned int *)0xb80180bc = 0x803e04d ;
		*(volatile unsigned int *)0xb80180c0 = 0x201a00f ;
		*(volatile unsigned int *)0xb80180c4 = 0x203200f ;
		*(volatile unsigned int *)0xb80180c8 = 0x2002000 ;
		*(volatile unsigned int *)0xb80180cc = 0x2002000 ;
		*(volatile unsigned int *)0xb80180d0 = 0x8d4a074 ;
		*(volatile unsigned int *)0xb800d030 = 0x924 ;
		*(volatile unsigned int *)0xb800d154 = 0x5280a102 ;
		*(volatile unsigned int *)0xb800d020 = 0x0 ;
		*(volatile unsigned int *)0xb800d024 = 0xf9c92a ;
		*(volatile unsigned int *)0xb800d028 = 0xa05f31 ;
		*(volatile unsigned int *)0xb800d02c = 0x121b99a ;
		*(volatile unsigned int *)0xb800d200 = 0x374fc84 ;
		*(volatile unsigned int *)0xb800d144 =  ((*(volatile unsigned int *)0xb800d144)&0xfffffffe)|0 ;
		*(volatile unsigned int *)0xb800d078 = 0x8 ;
		*(volatile unsigned int *)0xb800d078 = 0x9 ;
		*(volatile unsigned int *)0xb800d0a0 = 0x21084010 ;
		*(volatile unsigned int *)0xb800d0a4 = 0x7a ;
		*(volatile unsigned int *)0xb800d0b4 = 0xca1 ;
		*(volatile unsigned int *)0xb800d0b8 = 0x7d04a ;
		*(volatile unsigned int *)0xb800d0f0 = 0x2a ;
		*(volatile unsigned int *)0xb800d000 = 0x6 ;
		*(volatile unsigned int *)0xb800d034 = 0x3f ;
		*(volatile unsigned int *)0xb8018000 = 0x3ea ;

		// TVE_AV_CTRL_reg : disable i_black
		*(volatile unsigned int *)0xb8018040 = 0x4 ;
		// TVE_AV_CTRL_reg : disable p_black
		*(volatile unsigned int *)0xb8018040 = 0x2 ;
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : enable i  channel interrupt
		*(volatile unsigned int *)0xb801801c = 0x5 ;
		// VO_FC_reg : enable mix1's go bit
		*(volatile unsigned int *)0xb8005014 = 0x3 ;
		// TVE_SYNCGEN_RST_reg : enable go bits
		*(volatile unsigned int *)0xb8018000 = 0x3c0 ;
	} else if (logo_info.mode == 1) {
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : disable interrupt
		*(volatile unsigned int *)0xb801801c = 0xfffffffe ;
		// VO_FC_reg : disable  go bit
		*(volatile unsigned int *)0xb8005014 = 0x01 ;    

		// VO_MIX1_reg : enable SUB1
		*(volatile unsigned int *)0xb8005008 = 0x41 ;
		// VO_M1_SIZE_reg : set mixer1's width 720 and height 576
		*(volatile unsigned int *)0xb80054ac = 0x2d0240 ;

		// VO_SUB1_PXDT_reg  VO_SUB1_PXDB_reg : top and bottom address 
		*(volatile unsigned int *)0xb8005208 = 0x3000;
		*(volatile unsigned int *)0xb800520c = 0x3000 + logo_info.size;
		// VO_SUB1_CLUT_reg : color lookup table address  ** 8 bytes aligned **
		*(volatile unsigned int *)0xb8005204 = 0x1880;
		// VO_SUB1_reg : used for DVD subpicture
		*(volatile unsigned int *)0xb80051ec = 0x800001 ;
		// VO_SUB1_SUBP_reg : sub1 width 720
		*(volatile unsigned int *)0xb80051f4 = 0x2d0 ;
		// VO_SUB1_SIZE_reg : canvas width 720 and height 576
		*(volatile unsigned int *)0xb80051f8 = 0x2d0240 ;

		// VO_CH1_reg : enable reinterlace
		*(volatile unsigned int *)0xb80054e0 = 0x11 ;
		// VO_MODE_reg : enable ch1 interlace and progressive
		*(volatile unsigned int *)0xb8005000 = 0x7 ;
		// VO_OUT_reg  : enable i and p port
		*(volatile unsigned int *)0xb8005004 = 0x465 ;

		// TVE_TV_625p
		if (bonding == BONDING_QFP_128)
		{
			*(volatile unsigned int *)0xb8000020 = 0x800 ;
			*(volatile unsigned int *)0xb8018204 = 0x8208 ;
			if (logo_info.cvbs)
				*(volatile unsigned int *)0xb8018208 = 0x3d0 ;
			else
				*(volatile unsigned int *)0xb8018208 = 0x3f0 ;
			*(volatile unsigned int *)0xb801821c = 0x5b1e ;
		}
		else
		{
			*(volatile unsigned int *)0xb8000020 = 0x20 ;
			*(volatile unsigned int *)0xb8018200 = 0x0078f3cf; //set video DAC_ABC output current to MAX
			*(volatile unsigned int *)0xb8018208 = 0x1c40 ;
			*(volatile unsigned int *)0xb8018218 = 0x5b1e ;
		}

		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb8018038 = 0x1 ;
		*(volatile unsigned int *)0xb8018a00 = 0x205 ;
		*(volatile unsigned int *)0xb8018880 = 0xcb ;
		*(volatile unsigned int *)0xb8018884 = 0x8a ;
		*(volatile unsigned int *)0xb8018888 = 0x9 ;
		*(volatile unsigned int *)0xb801888c = 0x2a ;
		*(volatile unsigned int *)0xb8018890 = 0x0 ;
		*(volatile unsigned int *)0xb8018894 = 0x0 ;
		*(volatile unsigned int *)0xb8018898 = 0x0 ;
		*(volatile unsigned int *)0xb801889c = 0x99 ;
		*(volatile unsigned int *)0xb80188a0 = 0x2 ;
		*(volatile unsigned int *)0xb80188a8 = 0x78 ;
		*(volatile unsigned int *)0xb80188ac = 0x78 ;
		*(volatile unsigned int *)0xb80188b0 = 0x10 ;
		*(volatile unsigned int *)0xb80188b4 = 0x3 ;
		*(volatile unsigned int *)0xb80188b8 = 0x1d ;
		*(volatile unsigned int *)0xb8018984 = 0x20 ;
		*(volatile unsigned int *)0xb801898c = 0x2 ;
		*(volatile unsigned int *)0xb80188bc = 0x0 ;
		*(volatile unsigned int *)0xb80188c8 = 0xd7 ;
		*(volatile unsigned int *)0xb80188cc = 0x29 ;
		*(volatile unsigned int *)0xb80188d0 = 0x3 ;
		*(volatile unsigned int *)0xb80188e0 = 0x9 ;
		*(volatile unsigned int *)0xb80188e4 = 0x31 ;
		*(volatile unsigned int *)0xb80188e8 = 0x38 ;
		*(volatile unsigned int *)0xb80188ec = 0x6 ;
		*(volatile unsigned int *)0xb80188f0 = 0xbf ;
		*(volatile unsigned int *)0xb80188f4 = 0x3 ;
		*(volatile unsigned int *)0xb80188f8 = 0x5f ;
		*(volatile unsigned int *)0xb80189c0 = 0x58 ;
		*(volatile unsigned int *)0xb80189c4 = 0x3e ;
		*(volatile unsigned int *)0xb80189c8 = 0x28 ;
		*(volatile unsigned int *)0xb80189cc = 0x1c ;
		*(volatile unsigned int *)0xb80189d0 = 0x0 ;
		*(volatile unsigned int *)0xb80189d4 = 0x40 ;
		*(volatile unsigned int *)0xb80189d8 = 0x71 ;
		*(volatile unsigned int *)0xb80189dc = 0x20 ;
		*(volatile unsigned int *)0xb80189e0 = 0x80 ;
		*(volatile unsigned int *)0xb80189e4 = 0xa4 ;
		*(volatile unsigned int *)0xb80189e8 = 0x50 ;
		*(volatile unsigned int *)0xb80189ec = 0x56 ;
		*(volatile unsigned int *)0xb80189f0 = 0x74 ;
		*(volatile unsigned int *)0xb8018004 = 0x29c235f ;
		*(volatile unsigned int *)0xb80180b0 = 0x29ae6d ;
		*(volatile unsigned int *)0xb80180b4 = 0x31b ;
		*(volatile unsigned int *)0xb8018050 = 0x80fa30d ;
		*(volatile unsigned int *)0xb801805c = 0x816935 ;
		*(volatile unsigned int *)0xb8018060 = 0x94fa6e ;
		*(volatile unsigned int *)0xb8018048 = 0x330 ;
		*(volatile unsigned int *)0xb8018040 = 0x7e ;
		*(volatile unsigned int *)0xb8018040 = 0x27 ;
		*(volatile unsigned int *)0xb8018074 = 0x22cc3e ;

		if (bonding == BONDING_QFP_128)
		{
			*(volatile unsigned int *)0xb8000020 = 0x11020 ;
			*(volatile unsigned int *)0xb8018200 = 0x0078f3cf; //set video DAC_ABC output current to MAX
			if (logo_info.cvbs)
				*(volatile unsigned int *)0xb8018208 = 0x1c50 ;
			else
				*(volatile unsigned int *)0xb8018208 = 0x1c70 ;
			*(volatile unsigned int *)0xb8018218 = 0x5b1e ;
		}
		else
		{
			*(volatile unsigned int *)0xb8000020 = 0x11800 ;
			*(volatile unsigned int *)0xb8018204 = 0x0078f3cf; //set video DAC_DEF output current to MAX
			*(volatile unsigned int *)0xb8018208 = 0x3c0 ;
			*(volatile unsigned int *)0xb801821c = 0x5b1e ;
		}

		*(volatile unsigned int *)0xb801820c = 0x124 ;
		*(volatile unsigned int *)0xb8018210 = 0x200000 ;
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb8018008 = 0xa9c235f ;
		*(volatile unsigned int *)0xb801800c = 0xa9c235f ;
		*(volatile unsigned int *)0xb8018054 = 0x381f234c ;
		*(volatile unsigned int *)0xb8018064 = 0x82ca6b ;
		*(volatile unsigned int *)0xb8018058 = 0x381f234c ;
		*(volatile unsigned int *)0xb801806c = 0x82ca6b ;
		*(volatile unsigned int *)0xb8018048 = 0xc6 ;
		*(volatile unsigned int *)0xb8018040 = 0x7e ;
		*(volatile unsigned int *)0xb8018040 = 0x27 ;
		*(volatile unsigned int *)0xb8018078 = 0x22cc7c ;
		*(volatile unsigned int *)0xb801807c = 0x22cc7c ;
		*(volatile unsigned int *)0xb8018044 = 0x1 ;
		*(volatile unsigned int *)0xb8018080 = 0x20280242 ;
		*(volatile unsigned int *)0xb8018088 = 0x800e325 ;
		*(volatile unsigned int *)0xb801808c = 0x824 ;
		*(volatile unsigned int *)0xb80180f0 = 0x27e1800 ;
		*(volatile unsigned int *)0xb8018104 = 0x80008000 ;
		*(volatile unsigned int *)0xb8018108 = 0x80009404 ;
		*(volatile unsigned int *)0xb801810c = 0x80008000 ;
		*(volatile unsigned int *)0xb8018110 = 0x93a09470 ;
		*(volatile unsigned int *)0xb8018114 = 0x8d508c70 ;
		*(volatile unsigned int *)0xb8018118 = 0x28ba0 ;
		*(volatile unsigned int *)0xb80180e0 = 0x16021000; //set Hsync level over 300mV
		*(volatile unsigned int *)0xb80180e4 = 0x256d0d0 ;
		*(volatile unsigned int *)0xb80180ec = 0x28a1000 ;
		*(volatile unsigned int *)0xb8018010 = 0x3ffe ;
		*(volatile unsigned int *)0xb8018130 = 0xc ;
		*(volatile unsigned int *)0xb8018010 = 0x3049 ;
		*(volatile unsigned int *)0xb8018130 = 0x5 ;
		*(volatile unsigned int *)0xb8018000 = 0x3ea ;

		// DVI reg
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb800d034 = 0x22 ;
		*(volatile unsigned int *)0xb800d0bc = 0x1c000000 ;
		*(volatile unsigned int *)0xb8000020 = 0x5520000 ;
		*(volatile unsigned int *)0xb800d040 = 0x15a3f0aa ;
		*(volatile unsigned int *)0xb800d044 = 0x1 ;
		*(volatile unsigned int *)0xb80180b8 = 0x3a002000 ;
		*(volatile unsigned int *)0xb80180bc = 0x8d7e03f ;
		*(volatile unsigned int *)0xb80180c0 = 0x29c235f ;
		*(volatile unsigned int *)0xb80180c4 = 0x201235f ;
		*(volatile unsigned int *)0xb80180c8 = 0x2002000 ;
		*(volatile unsigned int *)0xb80180cc = 0x2002000 ;
		*(volatile unsigned int *)0xb80180d0 = 0x8d3206e ;
		*(volatile unsigned int *)0xb800d030 = 0x924 ;
		*(volatile unsigned int *)0xb800d154 = 0x5280a102 ;
		*(volatile unsigned int *)0xb800d020 = 0x0 ;
		*(volatile unsigned int *)0xb800d024 = 0xf9c92a ;
		*(volatile unsigned int *)0xb800d028 = 0xa05f31 ;
		*(volatile unsigned int *)0xb800d02c = 0x121b99a ;
		*(volatile unsigned int *)0xb800d200 = 0x374fc84 ;
		*(volatile unsigned int *)0xb800d144 =  ((*(volatile unsigned int *)0xb800d144)&0xfffffffe)|0 ;
		*(volatile unsigned int *)0xb800d078 = 0x8 ;
		*(volatile unsigned int *)0xb800d078 = 0x9 ;
		*(volatile unsigned int *)0xb800d0a0 = 0x21084010 ;
		*(volatile unsigned int *)0xb800d0a4 = 0x7a ;
		*(volatile unsigned int *)0xb800d0b4 = 0xca1 ;
		*(volatile unsigned int *)0xb800d0b8 = 0x7d04a ;
		*(volatile unsigned int *)0xb800d0f0 = 0x2a ;
		*(volatile unsigned int *)0xb800d000 = 0x6 ;
		*(volatile unsigned int *)0xb800d034 = 0x3f ;
		*(volatile unsigned int *)0xb8018000 = 0x3ea ;

		// TVE_AV_CTRL_reg : disable i_black
		*(volatile unsigned int *)0xb8018040 = 0x4 ;
		// TVE_AV_CTRL_reg : disable p_black
		*(volatile unsigned int *)0xb8018040 = 0x2 ;
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : enable i  channel interrupt
		*(volatile unsigned int *)0xb801801c = 0x5 ;
		// VO_FC_reg : enable mix1's go bit
		*(volatile unsigned int *)0xb8005014 = 0x3 ;
		// TVE_SYNCGEN_RST_reg : enable go bits
		*(volatile unsigned int *)0xb8018000 = 0x3c0 ;
	} else if (logo_info.mode == 2) {
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : disable interrupt
		*(volatile unsigned int *)0xb801801c = 0xfffffffe ;
		// VO_FC_reg : disable  go bit
		*(volatile unsigned int *)0xb8005014 = 0x01 ;    

		// VO_MIX1_reg : enable SUB1
		*(volatile unsigned int *)0xb8005008 = 0x41 ;
		// VO_M1_SIZE_reg : set mixer1's width 1920 and height 1080
		*(volatile unsigned int *)0xb80054ac = 0x780438 ;

		// VO_SUB1_PXDT_reg  VO_SUB1_PXDB_reg : top and bottom address 
		*(volatile unsigned int *)0xb8005208 = 0x3000;
		*(volatile unsigned int *)0xb800520c = 0x3000 + logo_info.size;
		// VO_SUB1_CLUT_reg : color lookup table address  ** 8 bytes aligned **
		*(volatile unsigned int *)0xb8005204 = 0x1880;
		// VO_SUB1_reg : used for HDDVD subpicture
		*(volatile unsigned int *)0xb80051ec = 0x400001 ;
		// VO_SUB1_SUBP_reg : sub1 width 1920
		*(volatile unsigned int *)0xb80051f4 = 0x780 ;
		// VO_SUB1_SIZE_reg : canvas width 1920 and height 1080
		*(volatile unsigned int *)0xb80051f8 = 0x780438 ;

		// VO_MODE_reg : enable ch1 progressive
		*(volatile unsigned int *)0xb8005000 = 0xe ;
		*(volatile unsigned int *)0xb8005000 = 0x3 ;
		// VO_OUT_reg  : enable p port
		*(volatile unsigned int *)0xb8005004 = 0x445 ;

		// TVE_TV_1080p50
		if (bonding == BONDING_QFP_128)
		{
			*(volatile unsigned int *)0xb8000020 = 0x14024 ;
			*(volatile unsigned int *)0xb8018200 = 0x788208 ;
			*(volatile unsigned int *)0xb8018208 = 0x1070 ;
			*(volatile unsigned int *)0xb8018218 = 0x591e ;
		}
		else
		{
			*(volatile unsigned int *)0xb8000020 = 0x14900 ;
			*(volatile unsigned int *)0xb8018204 = 0x788208 ;
			*(volatile unsigned int *)0xb8018208 = 0x240 ;
			*(volatile unsigned int *)0xb801821c = 0x591e ;
		}

		*(volatile unsigned int *)0xb801820c = 0x124 ;
		*(volatile unsigned int *)0xb8018210 = 0x200000 ;
		*(volatile unsigned int *)0xb8018920 = 0x0 ;
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb8018008 = 0xb192a4f ;
		*(volatile unsigned int *)0xb801800c = 0xb192a4f ;
		*(volatile unsigned int *)0xb8018054 = 0x384d68b5 ;
		*(volatile unsigned int *)0xb8018064 = 0x829c60 ;
		*(volatile unsigned int *)0xb8018058 = 0x384d68b5 ;
		*(volatile unsigned int *)0xb801806c = 0x829c60 ;
		*(volatile unsigned int *)0xb8018048 = 0xc6 ;
		*(volatile unsigned int *)0xb8018040 = 0x7e ;
		*(volatile unsigned int *)0xb8018040 = 0x27 ;
		*(volatile unsigned int *)0xb8018078 = 0x277d33 ;
		*(volatile unsigned int *)0xb801807c = 0x277d33 ;
		*(volatile unsigned int *)0xb8018044 = 0x1 ;
		*(volatile unsigned int *)0xb8018080 = 0x24d9e6a5 ;
		*(volatile unsigned int *)0xb8018088 = 0x83f68b9 ;
		*(volatile unsigned int *)0xb801808c = 0x824 ;
		*(volatile unsigned int *)0xb80180f0 = 0x27e1800 ;
		*(volatile unsigned int *)0xb8018104 = 0x80008000 ;
		*(volatile unsigned int *)0xb8018108 = 0x80009404 ;
		*(volatile unsigned int *)0xb801810c = 0x80008000 ;
		*(volatile unsigned int *)0xb8018110 = 0x944094c0 ;
		*(volatile unsigned int *)0xb8018114 = 0x8d508bd0 ;
		*(volatile unsigned int *)0xb8018118 = 0x28b48 ;
		*(volatile unsigned int *)0xb80180e0 = 0x1a0617b0 ;
		*(volatile unsigned int *)0xb80180e4 = 0x256d0d0 ;
		*(volatile unsigned int *)0xb80180ec = 0x28a1bb0 ;
		*(volatile unsigned int *)0xb8018010 = 0x3ffe ;
		*(volatile unsigned int *)0xb8018130 = 0xc ;
		*(volatile unsigned int *)0xb8018010 = 0x304d ;
		*(volatile unsigned int *)0xb8018000 = 0x3ea ;

		// DVI reg
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb800d034 = 0x22 ;
		*(volatile unsigned int *)0xb800d0bc = 0x1c000000 ;
		*(volatile unsigned int *)0xb8000020 = 0x5500000 ;
		*(volatile unsigned int *)0xb800d040 = 0x1c3f0ff ;
		*(volatile unsigned int *)0xb800d044 = 0x44447 ;
		*(volatile unsigned int *)0xb80180b8 = 0x3a002000 ;
		*(volatile unsigned int *)0xb80180bc = 0x81f20a8 ;
		*(volatile unsigned int *)0xb80180c0 = 0x200207c ;
		*(volatile unsigned int *)0xb80180c4 = 0x201607c ;
		*(volatile unsigned int *)0xb80180c8 = 0x2002000 ;
		*(volatile unsigned int *)0xb80180cc = 0x2002000 ;
		*(volatile unsigned int *)0xb80180d0 = 0xa2ee12d ;
		*(volatile unsigned int *)0xb800d030 = 0x924 ;
		*(volatile unsigned int *)0xb800d154 = 0x5280a102 ;
		*(volatile unsigned int *)0xb800d020 = 0x0 ;
		*(volatile unsigned int *)0xb800d024 = 0xf9c92a ;
		*(volatile unsigned int *)0xb800d028 = 0xa05f31 ;
		*(volatile unsigned int *)0xb800d02c = 0x121b99a ;
		*(volatile unsigned int *)0xb800d200 = 0x374fc84 ;
		*(volatile unsigned int *)0xb800d144 =  ((*(volatile unsigned int *)0xb800d144)&0xfffffffe)|0 ;
		*(volatile unsigned int *)0xb800d078 = 0x8 ;
		*(volatile unsigned int *)0xb800d078 = 0x9 ;
		*(volatile unsigned int *)0xb800d0a0 = 0x21084010 ;
		*(volatile unsigned int *)0xb800d0a4 = 0x7a ;
		*(volatile unsigned int *)0xb800d0b4 = 0xcb1 ;
		*(volatile unsigned int *)0xb800d0b8 = 0x5504a ;
		*(volatile unsigned int *)0xb800d0f0 = 0x2a ;
		*(volatile unsigned int *)0xb800d000 = 0x6 ;
		*(volatile unsigned int *)0xb800d034 = 0x3f ;
		*(volatile unsigned int *)0xb8018000 = 0x3ea ;

		// TVE_AV_CTRL_reg : disable p_black
		*(volatile unsigned int *)0xb8018040 = 0x2 ;
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : enable p  channel interrupt
		*(volatile unsigned int *)0xb801801c = 0x3 ;
		// VO_FC_reg : enable mix1's go bit
		*(volatile unsigned int *)0xb8005014 = 0x3 ;
		// TVE_SYNCGEN_RST_reg : enable go bits
		*(volatile unsigned int *)0xb8018000 = 0x3c0 ;
	} else if (logo_info.mode == 3) {
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : disable interrupt
		*(volatile unsigned int *)0xb801801c = 0xfffffffe ;
		// VO_FC_reg : disable  go bit
		*(volatile unsigned int *)0xb8005014 = 0x01 ;    

		// VO_MIX1_reg : enable SUB1
		*(volatile unsigned int *)0xb8005008 = 0x41 ;
		// VO_M1_SIZE_reg : set mixer1's width 1920 and height 1080
		*(volatile unsigned int *)0xb80054ac = 0x780438 ;

		// VO_SUB1_PXDT_reg  VO_SUB1_PXDB_reg : top and bottom address 
		*(volatile unsigned int *)0xb8005208 = 0x3000;
		*(volatile unsigned int *)0xb800520c = 0x3000 + logo_info.size;
		// VO_SUB1_CLUT_reg : color lookup table address  ** 8 bytes aligned **
		*(volatile unsigned int *)0xb8005204 = 0x1880;
		// VO_SUB1_reg : used for HDDVD subpicture
		*(volatile unsigned int *)0xb80051ec = 0x400001 ;
		// VO_SUB1_SUBP_reg : sub1 width 1920
		*(volatile unsigned int *)0xb80051f4 = 0x780 ;
		// VO_SUB1_SIZE_reg : canvas width 1920 and height 1080
		*(volatile unsigned int *)0xb80051f8 = 0x780438 ;

		// VO_MODE_reg : enable ch1 progressive
		*(volatile unsigned int *)0xb8005000 = 0xe ;
		*(volatile unsigned int *)0xb8005000 = 0x3 ;
		// VO_OUT_reg  : enable p port
		*(volatile unsigned int *)0xb8005004 = 0x445 ;

		// TVE_TV_1080p60
		if (bonding == BONDING_QFP_128)
		{
			*(volatile unsigned int *)0xb8000020 = 0x14024 ;
			*(volatile unsigned int *)0xb8018200 = 0x788208 ;
			*(volatile unsigned int *)0xb8018208 = 0x1070 ;
			*(volatile unsigned int *)0xb8018218 = 0x591e ;
		}
		else
		{
			*(volatile unsigned int *)0xb8000020 = 0x14900 ;
			*(volatile unsigned int *)0xb8018204 = 0x788208 ;
			*(volatile unsigned int *)0xb8018208 = 0x240 ;
			*(volatile unsigned int *)0xb801821c = 0x591e ;
		}

		*(volatile unsigned int *)0xb801820c = 0x124 ;
		*(volatile unsigned int *)0xb8018210 = 0x200000 ;
		*(volatile unsigned int *)0xb8018920 = 0x0 ;
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb8018008 = 0xb192897 ;
		*(volatile unsigned int *)0xb801800c = 0xb192897 ;
		*(volatile unsigned int *)0xb8018054 = 0x38446891 ;
		*(volatile unsigned int *)0xb8018064 = 0x829c60 ;
		*(volatile unsigned int *)0xb8018058 = 0x38446891 ;
		*(volatile unsigned int *)0xb801806c = 0x829c60 ;
		*(volatile unsigned int *)0xb8018048 = 0xc6 ;
		*(volatile unsigned int *)0xb8018040 = 0x7e ;
		*(volatile unsigned int *)0xb8018040 = 0x27 ;
		*(volatile unsigned int *)0xb8018078 = 0x277d0f ;
		*(volatile unsigned int *)0xb801807c = 0x277d0f ;
		*(volatile unsigned int *)0xb8018044 = 0x1 ;
		*(volatile unsigned int *)0xb8018080 = 0x22c95a82 ;
		*(volatile unsigned int *)0xb8018088 = 0x8366895 ;
		*(volatile unsigned int *)0xb801808c = 0x824 ;
		*(volatile unsigned int *)0xb80180f0 = 0x27e1800 ;
		*(volatile unsigned int *)0xb8018104 = 0x80008000 ;
		*(volatile unsigned int *)0xb8018108 = 0x80009404 ;
		*(volatile unsigned int *)0xb801810c = 0x80008000 ;
		*(volatile unsigned int *)0xb8018110 = 0x944094c0 ;
		*(volatile unsigned int *)0xb8018114 = 0x8d508bd0 ;
		*(volatile unsigned int *)0xb8018118 = 0x28b48 ;
		*(volatile unsigned int *)0xb80180e0 = 0x1a0617b0 ;
		*(volatile unsigned int *)0xb80180e4 = 0x256d0d0 ;
		*(volatile unsigned int *)0xb80180ec = 0x28a1bb0 ;
		*(volatile unsigned int *)0xb8018010 = 0x3ffe ;
		*(volatile unsigned int *)0xb8018130 = 0xc ;
		*(volatile unsigned int *)0xb8018010 = 0x304d ;
		*(volatile unsigned int *)0xb8018000 = 0x3ea ;

		// DVI reg
		*(volatile unsigned int *)0xb8018000 = 0x2aa ;
		*(volatile unsigned int *)0xb800d034 = 0x22 ;
		*(volatile unsigned int *)0xb800d0bc = 0x1c000000 ;
		*(volatile unsigned int *)0xb8000020 = 0x5500000 ;
		*(volatile unsigned int *)0xb800d040 = 0x1c3ffff ;
		*(volatile unsigned int *)0xb800d044 = 0x44447 ;
		*(volatile unsigned int *)0xb80180b8 = 0x3a002000 ;
		*(volatile unsigned int *)0xb80180bc = 0x8162084 ;
		*(volatile unsigned int *)0xb80180c0 = 0x2002058 ;
		*(volatile unsigned int *)0xb80180c4 = 0x2016058 ;
		*(volatile unsigned int *)0xb80180c8 = 0x2002000 ;
		*(volatile unsigned int *)0xb80180cc = 0x2002000 ;
		*(volatile unsigned int *)0xb80180d0 = 0xa25e109 ;
		*(volatile unsigned int *)0xb800d030 = 0x924 ;
		*(volatile unsigned int *)0xb800d154 = 0x5280a102 ;
		*(volatile unsigned int *)0xb800d020 = 0x0 ;
		*(volatile unsigned int *)0xb800d024 = 0xf9c92a ;
		*(volatile unsigned int *)0xb800d028 = 0xa05f31 ;
		*(volatile unsigned int *)0xb800d02c = 0x121b99a ;
		*(volatile unsigned int *)0xb800d200 = 0x374fc84 ;
		*(volatile unsigned int *)0xb800d144 =  ((*(volatile unsigned int *)0xb800d144)&0xfffffffe)|0 ;
		*(volatile unsigned int *)0xb800d078 = 0x8 ;
		*(volatile unsigned int *)0xb800d078 = 0x9 ;
		*(volatile unsigned int *)0xb800d0a0 = 0x21084010 ;
		*(volatile unsigned int *)0xb800d0a4 = 0x7a ;
		*(volatile unsigned int *)0xb800d0b4 = 0xca5 ;
		*(volatile unsigned int *)0xb800d0b8 = 0x5504a ;
		*(volatile unsigned int *)0xb800d0f0 = 0x2a ;
		*(volatile unsigned int *)0xb800d000 = 0x6 ;
		*(volatile unsigned int *)0xb800d034 = 0x3f ;
		*(volatile unsigned int *)0xb8018000 = 0x3ea ;

		// TVE_AV_CTRL_reg : disable p_black
		*(volatile unsigned int *)0xb8018040 = 0x2 ;
		// TVE_INTST_reg : reset interrupt status
		*(volatile unsigned int *)0xb8018020 = 0xfffffffe ;
		// TVE_INTEN_reg : enable p  channel interrupt
		*(volatile unsigned int *)0xb801801c = 0x3 ;
		// VO_FC_reg : enable mix1's go bit
		*(volatile unsigned int *)0xb8005014 = 0x3 ;
		// TVE_SYNCGEN_RST_reg : enable go bits
		*(volatile unsigned int *)0xb8018000 = 0x3c0 ;
	} else {
		// not support yet...
	}
}
#endif

#if CONFIG_REALTEK_CHIP_VENUS || CONFIG_REALTEK_CPU_NEPTUNE
void setup_boot_image(void)
{
	unsigned int value;

	if (platform_info.board_id == realtek_pvr_demo_board)
		return;

	// change to "composite" mode for SCART
	value = *(volatile unsigned int *)0xb801b108;
	*(volatile unsigned int *)0xb801b108 = (value & 0xffbfffff) ;
	
	// change to AV mode for SCART
	if (platform_info.board_id == realtek_1261_demo_board) {
		value = *(volatile unsigned int *)0xb801b10c;
		*(volatile unsigned int *)0xb801b10c = (value & 0xfffffff3) ;
	}

	// reset video exception entry point
	*(volatile unsigned int *)0xa00000a4 = 0x00801a3c;
	*(volatile unsigned int *)0xa00000a8 = 0x00185a37;
	*(volatile unsigned int *)0xa00000ac = 0x08004003;
	*(volatile unsigned int *)0xa00000b0 = 0x00000000;

	if (logo_info.mode == 0) {
		// NTSC mode
		if (is_venus_cpu()) {
//			*(volatile unsigned int *)0xb801b200 = 0x61 ;
			// set VO registers
			*(volatile unsigned int *)0xb8005350 = 0xfffffffe ;
			*(volatile unsigned int *)0xb8005350 = 0x3 ;
			*(volatile unsigned int *)0xb8005354 = 0x2cf ;
			*(volatile unsigned int *)0xb8005358 = 0x2ef5df ;
			*(volatile unsigned int *)0xb800535c = 0xffff3210 ;
			*(volatile unsigned int *)0xb8005360 = 0x0 ;
	
			// color lookup table of Sub-Picture , index0~index3
			*(volatile unsigned int *)0xb8005370 = logo_info.color[0];
			*(volatile unsigned int *)0xb8005374 = logo_info.color[1];
			*(volatile unsigned int *)0xb8005378 = logo_info.color[2];
			*(volatile unsigned int *)0xb800537c = logo_info.color[3];
			*(volatile unsigned int *)0xb80053b0 = 0xacf800 ;
			*(volatile unsigned int *)0xb80053b4 = 0x9df800 ;
			*(volatile unsigned int *)0xb80053bc = 0x4000 ;
			*(volatile unsigned int *)0xb80053c0 = 0x0 ;
			*(volatile unsigned int *)0xb80053c4 = 0x0 ;
			*(volatile unsigned int *)0xb80053c8 = 0x0 ;
			*(volatile unsigned int *)0xb80053cc = 0x0 ;
			*(volatile unsigned int *)0xb80053d0 = 0x200 ;
			*(volatile unsigned int *)0xb80053d8 = 0x4000 ;
			*(volatile unsigned int *)0xb80053dc = 0x0 ;
			*(volatile unsigned int *)0xb80053e0 = 0x0 ;
			*(volatile unsigned int *)0xb80053e4 = 0x0 ;
			*(volatile unsigned int *)0xb80053e8 = 0x0 ;
			*(volatile unsigned int *)0xb80053ec = 0x0 ;
			*(volatile unsigned int *)0xb80053f0 = 0x0 ;
			*(volatile unsigned int *)0xb80053f4 = 0x0 ;
			*(volatile unsigned int *)0xb80053f8 = 0x0 ;
			*(volatile unsigned int *)0xb80053fc = 0x400 ;
			*(volatile unsigned int *)0xb8005400 = 0xffd ;
			*(volatile unsigned int *)0xb8005404 = 0xf8f ;
			*(volatile unsigned int *)0xb8005408 = 0xf60 ;
			*(volatile unsigned int *)0xb800540c = 0xf50 ;
			*(volatile unsigned int *)0xb8005410 = 0xfa8 ;
			*(volatile unsigned int *)0xb8005414 = 0x207 ;
			*(volatile unsigned int *)0xb8005418 = 0x30a ;
			*(volatile unsigned int *)0xb800541c = 0x50b ;
			*(volatile unsigned int *)0xb80054b0 = 0x9dfacf ;
			*(volatile unsigned int *)0xb80054e8 = 0x2001400 ;
			*(volatile unsigned int *)0xb80054ec = 0x2001000 ;
			*(volatile unsigned int *)0xb80054f0 = 0x2001400 ;
			*(volatile unsigned int *)0xb80054f4 = 0x2001000 ;
			*(volatile unsigned int *)0xb80054f8 = 0x400 ;
			*(volatile unsigned int *)0xb80054fc = 0x20080200 ;
			*(volatile unsigned int *)0xb8005500 = 0xf010eb10 ;
			*(volatile unsigned int *)0xb8005564 = 0x0 ;
			*(volatile unsigned int *)0xb8005568 = 0x1 ;
			*(volatile unsigned int *)0xb800556c = 0x2 ;
			*(volatile unsigned int *)0xb8005570 = 0x3 ;
			*(volatile unsigned int *)0xb8005574 = 0x4 ;
			*(volatile unsigned int *)0xb8005578 = 0x5 ;
			*(volatile unsigned int *)0xb800557c = 0x6 ;
			*(volatile unsigned int *)0xb8005580 = 0x7 ;
			*(volatile unsigned int *)0xb8005584 = 0x8 ;
	
			// Top and Bot address of Sub-Picture 
			*(volatile unsigned int *)0xb8005530 = 0xf0000 ;
			*(volatile unsigned int *)0xb8005534 = 0xf0000+logo_info.size ;
	
			// set TVE registers
			*(volatile unsigned int *)0xb8000084 = 0x101cd800 ;
			*(volatile unsigned int *)0xb800000c = 0x1 ;
			*(volatile unsigned int *)0xB80180C8 = 0x12D;     
			*(volatile unsigned int *)0xB80180CC = 0x188;     
			*(volatile unsigned int *)0xB8018134 = 0x278000;  
			*(volatile unsigned int *)0xB80180D0 = 0x1F9AAA;  
			*(volatile unsigned int *)0xB80180D4 = 0x1F9AAA;  
			*(volatile unsigned int *)0xB80180E8 = 0x1;       
			*(volatile unsigned int *)0xB8018154 = 0x200;     
			*(volatile unsigned int *)0xB8018880 = 0x1F;      
			*(volatile unsigned int *)0xB8018884 = 0x7C;      
			*(volatile unsigned int *)0xB8018888 = 0xF0;      
			*(volatile unsigned int *)0xB801888C = 0x21;      
			*(volatile unsigned int *)0xB8018890 = 0x0;       
			*(volatile unsigned int *)0xB8018894 = 0x2;       
			*(volatile unsigned int *)0xB8018898 = 0x2;       
			*(volatile unsigned int *)0xB801889C = 0x3F;      
			*(volatile unsigned int *)0xB80188A0 = 0x2;       
			*(volatile unsigned int *)0xB80188A8 = 0x8D;      
			*(volatile unsigned int *)0xB80188AC = 0x78;      
			*(volatile unsigned int *)0xB80188B0 = 0x10;      
			*(volatile unsigned int *)0xB80188B4 = 0x7;       
			*(volatile unsigned int *)0xB80188B8 = 0x1C;      
			*(volatile unsigned int *)0xB80188B8 = 0x1C;      
			*(volatile unsigned int *)0xB8018984 = 0x20;      
			*(volatile unsigned int *)0xB801898C = 0x2;       
			*(volatile unsigned int *)0xB80188BC = 0x0;       
			*(volatile unsigned int *)0xB80188C8 = 0xC8;      
			*(volatile unsigned int *)0xB80188CC = 0x0;       
			*(volatile unsigned int *)0xB80188D0 = 0x0;       
			*(volatile unsigned int *)0xB80188E0 = 0x8;       
			*(volatile unsigned int *)0xB80188E4 = 0x31;      
			*(volatile unsigned int *)0xB80188E8 = 0x6;       
			*(volatile unsigned int *)0xB80188EC = 0x6;       
			*(volatile unsigned int *)0xB80188F0 = 0xB3;      
			*(volatile unsigned int *)0xB80188F4 = 0x3;       
			*(volatile unsigned int *)0xB80188F8 = 0x59;      
			*(volatile unsigned int *)0xB80189C0 = 0x64;      
			*(volatile unsigned int *)0xB80189C4 = 0x2D;      
			*(volatile unsigned int *)0xB80189C8 = 0x7;       
			*(volatile unsigned int *)0xB80189CC = 0x14;      
			*(volatile unsigned int *)0xB8018920 = 0x0;       
			*(volatile unsigned int *)0xB8018924 = 0x3A;      
			*(volatile unsigned int *)0xB8018928 = 0x11;      
			*(volatile unsigned int *)0xB801892C = 0x4B;      
			*(volatile unsigned int *)0xB8018930 = 0x11;      
			*(volatile unsigned int *)0xB8018934 = 0x3C;      
			*(volatile unsigned int *)0xB8018938 = 0x1B;      
			*(volatile unsigned int *)0xB801893C = 0x1B;      
			*(volatile unsigned int *)0xB8018940 = 0x24;      
			*(volatile unsigned int *)0xB8018944 = 0x7;       
			*(volatile unsigned int *)0xB8018948 = 0xF8;      
			*(volatile unsigned int *)0xB801894C = 0x0;       
			*(volatile unsigned int *)0xB8018950 = 0x0;       
			*(volatile unsigned int *)0xB8018954 = 0xF;       
			*(volatile unsigned int *)0xB8018958 = 0xF;       
			*(volatile unsigned int *)0xB801895C = 0x60;      
			*(volatile unsigned int *)0xB8018960 = 0xA0;      
			*(volatile unsigned int *)0xB8018964 = 0x54;      
			*(volatile unsigned int *)0xB8018968 = 0xFF;      
			*(volatile unsigned int *)0xB801896C = 0x3;       
			*(volatile unsigned int *)0xB80180D8 = 0x40;      
			*(volatile unsigned int *)0xB8018990 = 0x0;       
			*(volatile unsigned int *)0xB80189D0 = 0xC;       
			*(volatile unsigned int *)0xB80189D4 = 0x4B;      
			*(volatile unsigned int *)0xB80189D8 = 0x7A;      
			*(volatile unsigned int *)0xB80189DC = 0x2B;      
			*(volatile unsigned int *)0xB80189E0 = 0x85;      
			*(volatile unsigned int *)0xB80189E4 = 0xAA;      
			*(volatile unsigned int *)0xB80189E8 = 0x5A;      
			*(volatile unsigned int *)0xB80189EC = 0x62;      
			*(volatile unsigned int *)0xB80189F0 = 0x84;      
			*(volatile unsigned int *)0xB8018000 = 0x2A832359;
			*(volatile unsigned int *)0xB8018004 = 0x306505;  
			*(volatile unsigned int *)0xB80180AC = 0xE0B16;   
			*(volatile unsigned int *)0xB8018048 = 0x8106310; 
			*(volatile unsigned int *)0xB8018050 = 0x815904;  
			*(volatile unsigned int *)0xB8018054 = 0x91CA0B;  
			*(volatile unsigned int *)0xB801804C = 0x820A352; 
			*(volatile unsigned int *)0xB8018058 = 0x82AA09;  
			*(volatile unsigned int *)0xB80180C4 = 0x7EC00;   
			*(volatile unsigned int *)0xB80180EC = 0x19;      
			*(volatile unsigned int *)0xB80180EC = 0x6;       
			*(volatile unsigned int *)0xB80180EC = 0x1;       
			*(volatile unsigned int *)0xB80180F0 = 0x22D43F;  
			*(volatile unsigned int *)0xB80180F4 = 0x22CC88;  
			*(volatile unsigned int *)0xB8018084 = 0x80009404;
			*(volatile unsigned int *)0xB8018088 = 0x80008000;
			*(volatile unsigned int *)0xB801808C = 0x93929392;
			*(volatile unsigned int *)0xB8018090 = 0x8C708D40;
			*(volatile unsigned int *)0xB8018110 = 0xC70;     
			*(volatile unsigned int *)0xB8018094 = 0x80008000;
			*(volatile unsigned int *)0xB80180FC = 0x27E1800; 
			*(volatile unsigned int *)0xB80180A8 = 0x7F4800;  
			*(volatile unsigned int *)0xB8018098 = 0x6A0817A0;
			*(volatile unsigned int *)0xB801809C = 0x28A1BB0; 
			*(volatile unsigned int *)0xB8018148 = 0xE56D0D0; 
			*(volatile unsigned int *)0xB801814C = 0x40;      
			*(volatile unsigned int *)0xB8018040 = 0x1;       
			*(volatile unsigned int *)0xB8018044 = 0x20E0024E;
			*(volatile unsigned int *)0xB801805C = 0x804232A; 
			*(volatile unsigned int *)0xB8018060 = 0x9AB;     
			*(volatile unsigned int *)0xB80180D8 = 0x7FE;     
			*(volatile unsigned int *)0xB80180D8 = 0x32B;     
			*(volatile unsigned int *)0xB80180A4 = 0x8216359; 
			*(volatile unsigned int *)0xB8018100 = 0xBE8800;  
			*(volatile unsigned int *)0xB8018104 = 0xA0C82C;  
			*(volatile unsigned int *)0xB8018108 = 0xBE8BE8;  
			*(volatile unsigned int *)0xB80180A0 = 0x7E;      
			*(volatile unsigned int *)0xB80180A0 = 0x2F;      
			*(volatile unsigned int *)0xB8018000 = 0x30000000;
	
			// disable TVE colorbar, enable interrupt
			*(volatile unsigned int *)0xb80180ec = 0x18 ;
			*(volatile unsigned int *)0xb8005504 = 0xfffffffe ;
			*(volatile unsigned int *)0xb8005504 = 0xf ;
			*(volatile unsigned int *)0xb801810c = 0xa00a000 ;
			*(volatile unsigned int *)0xb80180e0 = 0x0 ;
			*(volatile unsigned int *)0xb80055e4 = 0x9 ;
			*(volatile unsigned int *)0xb80180dc = 0x1 ;

#ifndef CONFIG_REALTEK_HDMI_NONE
			// HDMI
			*(volatile unsigned int *)0xb801b36c = 0x0;
			*(volatile unsigned int *)0xb801b300 = 0x63;
			*(volatile unsigned int *)0xb801b330 = 0x0;
			*(volatile unsigned int *)0xb801b338 = 0x0;
			*(volatile unsigned int *)0xb801b33c = 0x8;
			*(volatile unsigned int *)0xb801b36c = 0x1;
			
			BusyWaiting(70000);
#ifdef CONFIG_REALTEK_HDMI_NXP
			SET_NXP_HDMI_480P();
#else
			SET_CAT_HDMI_480P();
#endif
			BusyWaiting(70000);
#endif
		} else {
//			*(volatile unsigned int *)0xb801b200 = 0x62 ;
			// set VO registers
			*(volatile unsigned int *)0xb8005350 = 0xfffffffe ;
			*(volatile unsigned int *)0xb8005350 = 0x3 ;
			*(volatile unsigned int *)0xb8005354 = 0x2cf ;
			*(volatile unsigned int *)0xb8005358 = 0x2ef5df ;
			*(volatile unsigned int *)0xb800535c = 0xffff3210 ;
			*(volatile unsigned int *)0xb8005360 = 0x0 ;
	
			// color lookup table of Sub-Picture , index0~index3
			*(volatile unsigned int *)0xb8005370 = logo_info.color[0];
			*(volatile unsigned int *)0xb8005374 = logo_info.color[1];
			*(volatile unsigned int *)0xb8005378 = logo_info.color[2];
			*(volatile unsigned int *)0xb800537c = logo_info.color[3];
			*(volatile unsigned int *)0xb80053b0 = 0xacf800 ;
			*(volatile unsigned int *)0xb80053b4 = 0x9df800 ;
			*(volatile unsigned int *)0xb80056cc = 0x4000 ;
			*(volatile unsigned int *)0xb80056d0 = 0x0 ;
			*(volatile unsigned int *)0xb80056c8 = 0x2 ;
			*(volatile unsigned int *)0xb80056fc = 0x4000 ;
			*(volatile unsigned int *)0xb8005700 = 0x0 ;
			*(volatile unsigned int *)0xb80056f4 = 0x2 ;
			*(volatile unsigned int *)0xb80054b0 = 0x9dfacf ;
			*(volatile unsigned int *)0xb80054e8 = 0x2001400 ;
			*(volatile unsigned int *)0xb80054ec = 0x2001000 ;
			*(volatile unsigned int *)0xb80054f0 = 0x2001400 ;
			*(volatile unsigned int *)0xb80054f4 = 0x2001000 ;
			*(volatile unsigned int *)0xb80054f8 = 0x400 ;
			*(volatile unsigned int *)0xb80054fc = 0x20080200 ;
			*(volatile unsigned int *)0xb8005500 = 0xf010eb10 ;
			*(volatile unsigned int *)0xb8005564 = 0x0 ;
			*(volatile unsigned int *)0xb8005568 = 0x1 ;
			*(volatile unsigned int *)0xb800556c = 0x2 ;
			*(volatile unsigned int *)0xb8005570 = 0x3 ;
			*(volatile unsigned int *)0xb8005574 = 0x4 ;
			*(volatile unsigned int *)0xb8005578 = 0x5 ;
			*(volatile unsigned int *)0xb800557c = 0x6 ;
			*(volatile unsigned int *)0xb8005580 = 0x7 ;
			*(volatile unsigned int *)0xb8005584 = 0x8 ;
	
			// Top and Bot address of Sub-Picture 
			*(volatile unsigned int *)0xb8005530 = 0xf0000 ;
			*(volatile unsigned int *)0xb8005534 = 0xf0000+logo_info.size ;
	
			// set TVE registers
			*(volatile unsigned int *)0xb8000084 = 0x101cd800 ;
			*(volatile unsigned int *)0xb800000c = 0x1 ;
			*(volatile unsigned int *)0xB80180C8 = 0x124 ;    
			*(volatile unsigned int *)0xB80180CC = 0x188 ;    
			*(volatile unsigned int *)0xB8018134 = 0x278000 ; 
			*(volatile unsigned int *)0xB80180D0 = 0x3FBBBB ; 
			*(volatile unsigned int *)0xB80180D4 = 0x3FCBBB ; 
			*(volatile unsigned int *)0xB80180E8 = 0x1 ;      
			*(volatile unsigned int *)0xB8018154 = 0x200 ;     
			*(volatile unsigned int *)0xB8018880 = 0x1F ;     
			*(volatile unsigned int *)0xB8018884 = 0x7C ;     
			*(volatile unsigned int *)0xB8018888 = 0xF0 ;     
			*(volatile unsigned int *)0xB801888C = 0x21 ;     
			*(volatile unsigned int *)0xB8018890 = 0x0 ;      
			*(volatile unsigned int *)0xB8018894 = 0x2 ;      
			*(volatile unsigned int *)0xB8018898 = 0x2 ;      
			*(volatile unsigned int *)0xB801889C = 0x3F ;     
			*(volatile unsigned int *)0xB80188A0 = 0x2 ;      
			*(volatile unsigned int *)0xB80188A8 = 0x8D ;     
			*(volatile unsigned int *)0xB80188AC = 0x78 ;     
			*(volatile unsigned int *)0xB80188B0 = 0x10 ;     
			*(volatile unsigned int *)0xB80188B4 = 0x7 ;      
			*(volatile unsigned int *)0xB80188B8 = 0x1C ;     
			*(volatile unsigned int *)0xB80188B8 = 0x1C ;     
			*(volatile unsigned int *)0xB8018984 = 0x20 ;     
			*(volatile unsigned int *)0xB801898C = 0x2 ;      
			*(volatile unsigned int *)0xB80188BC = 0x0 ;      
			*(volatile unsigned int *)0xB80188C8 = 0xC8 ;      
			*(volatile unsigned int *)0xB80188CC = 0x0 ;      
			*(volatile unsigned int *)0xB80188D0 = 0x0 ;      
			*(volatile unsigned int *)0xB80188E0 = 0x8 ;      
			*(volatile unsigned int *)0xB80188E4 = 0x31 ;     
			*(volatile unsigned int *)0xB80188E8 = 0x6 ;      
			*(volatile unsigned int *)0xB80188EC = 0x6 ;      
			*(volatile unsigned int *)0xB80188F0 = 0xB3 ;     
			*(volatile unsigned int *)0xB80188F4 = 0x3 ;      
			*(volatile unsigned int *)0xB80188F8 = 0x59 ;     
			*(volatile unsigned int *)0xB80189C0 = 0x64 ;     
			*(volatile unsigned int *)0xB80189C4 = 0x2D ;     
			*(volatile unsigned int *)0xB80189C8 = 0x7 ;      
			*(volatile unsigned int *)0xB80189CC = 0x18 ;     
			*(volatile unsigned int *)0xB8018920 = 0x0 ;      
			*(volatile unsigned int *)0xB8018924 = 0x3A ;     
			*(volatile unsigned int *)0xB8018928 = 0x11 ;     
			*(volatile unsigned int *)0xB801892C = 0x4B ;     
			*(volatile unsigned int *)0xB8018930 = 0x11 ;     
			*(volatile unsigned int *)0xB8018934 = 0x3C ;     
			*(volatile unsigned int *)0xB8018938 = 0x1B ;     
			*(volatile unsigned int *)0xB801893C = 0x1B ;     
			*(volatile unsigned int *)0xB8018940 = 0x24 ;     
			*(volatile unsigned int *)0xB8018944 = 0x7 ;      
			*(volatile unsigned int *)0xB8018948 = 0xF8 ;     
			*(volatile unsigned int *)0xB801894C = 0x0 ;      
			*(volatile unsigned int *)0xB8018950 = 0x0 ;      
			*(volatile unsigned int *)0xB8018954 = 0xF ;      
			*(volatile unsigned int *)0xB8018958 = 0xF ;      
			*(volatile unsigned int *)0xB801895C = 0x60 ;     
			*(volatile unsigned int *)0xB8018960 = 0xA0 ;     
			*(volatile unsigned int *)0xB8018964 = 0x54 ;     
			*(volatile unsigned int *)0xB8018968 = 0xFF ;     
			*(volatile unsigned int *)0xB801896C = 0x3 ;      
			*(volatile unsigned int *)0xB80180D8 = 0x40 ;     
			*(volatile unsigned int *)0xB8018990 = 0x0 ;      
			*(volatile unsigned int *)0xB80189D0 = 0xC ;      
			*(volatile unsigned int *)0xB80189D4 = 0x4B ;     
			*(volatile unsigned int *)0xB80189D8 = 0x7A ;     
			*(volatile unsigned int *)0xB80189DC = 0x2B ;     
			*(volatile unsigned int *)0xB80189E0 = 0x85 ;     
			*(volatile unsigned int *)0xB80189E4 = 0xAA ;     
			*(volatile unsigned int *)0xB80189E8 = 0x5A ;     
			*(volatile unsigned int *)0xB80189EC = 0x62 ;     
			*(volatile unsigned int *)0xB80189F0 = 0x84 ;     
			*(volatile unsigned int *)0xB8018000 = 0x2A832359 ;
			*(volatile unsigned int *)0xB8018004 = 0x306505 ; 
			*(volatile unsigned int *)0xB80180AC = 0xB16 ;    
			*(volatile unsigned int *)0xB8018048 = 0x8106310 ;
			*(volatile unsigned int *)0xB8018050 = 0x815904 ; 
			*(volatile unsigned int *)0xB8018054 = 0x91CA0B ; 
			*(volatile unsigned int *)0xB801804C = 0x820A352 ;
			*(volatile unsigned int *)0xB8018058 = 0x82AA09 ; 
			*(volatile unsigned int *)0xB80180C4 = 0x7EC00 ;  
			*(volatile unsigned int *)0xB80180EC = 0x19 ;     
			*(volatile unsigned int *)0xB80180EC = 0x6 ;      
			*(volatile unsigned int *)0xB80180EC = 0x1 ;     
			*(volatile unsigned int *)0xB80180F0 = 0x22D43F ; 
			*(volatile unsigned int *)0xB80180F4 = 0x22CC88 ; 
			*(volatile unsigned int *)0xB8018084 = 0x80009404 ;
			*(volatile unsigned int *)0xB8018088 = 0x80008000 ;
			*(volatile unsigned int *)0xB801808C = 0x93929392 ;
			*(volatile unsigned int *)0xB8018090 = 0x8C708D40 ;
			*(volatile unsigned int *)0xB8018110 = 0xC70 ;    
			*(volatile unsigned int *)0xB8018094 = 0x80008000 ;
			*(volatile unsigned int *)0xB80180FC = 0x27E1800 ;
			*(volatile unsigned int *)0xB80180A8 = 0x7F400E0 ;
			*(volatile unsigned int *)0xB80180AC = 0x2359 ;   
			*(volatile unsigned int *)0xB8018098 = 0x6A0817A0 ;
			*(volatile unsigned int *)0xB801809C = 0x28A1BB0 ; 
			*(volatile unsigned int *)0xB8018148 = 0xE56D0D0 ;
			*(volatile unsigned int *)0xB801814C = 0x40 ;     
			*(volatile unsigned int *)0xB8018040 = 0x1 ;      
			*(volatile unsigned int *)0xB8018044 = 0x20E0024E ;
			*(volatile unsigned int *)0xB801805C = 0x804232A  ;
			*(volatile unsigned int *)0xB8018060 = 0x9AB ;    
			*(volatile unsigned int *)0xB80180D8 = 0x7FE ;    
			*(volatile unsigned int *)0xB80180D8 = 0x32B ;    
			*(volatile unsigned int *)0xB80180A4 = 0x81EE34F ;
			*(volatile unsigned int *)0xB8018100 = 0xBE8800 ; 
			*(volatile unsigned int *)0xB8018104 = 0xA0C82C ; 
			*(volatile unsigned int *)0xB8018108 = 0xBE8BE8 ; 
			*(volatile unsigned int *)0xB80180A0 = 0x7E ;     
			*(volatile unsigned int *)0xB8018000 = 0x30000000 ;
	
			// set HDMI registers           
			*(volatile unsigned int *)0xB80000D0 = 0x1 ;       
			*(volatile unsigned int *)0xB8018158 = 0x3A002000 ;
			*(volatile unsigned int *)0xB8018164 = 0x8022046 ; 
			*(volatile unsigned int *)0xB8018168 = 0x201A008 ; 
			*(volatile unsigned int *)0xB801816C = 0x2032008 ; 
			*(volatile unsigned int *)0xB8018170 = 0x2002000 ; 
			*(volatile unsigned int *)0xB8018174 = 0x2002000 ; 
			*(volatile unsigned int *)0xB8018160 = 0x8D4A074 ; 
			*(volatile unsigned int *)0xB800D010 = 0x0 ;       
			*(volatile unsigned int *)0xB800D014 = 0xF9C938 ;  
			*(volatile unsigned int *)0xB800D018 = 0xA05F30 ;  
			*(volatile unsigned int *)0xB800D01C = 0x125499A ; 
			*(volatile unsigned int *)0xB800D150 = 0x37AFCB4 ; 
			*(volatile unsigned int *)0xB800D020 = 0x3B ;      
			*(volatile unsigned int *)0xB800D154 = 0x924 ;     
			*(volatile unsigned int *)0xB800D02C = 0x4630F0 ;  
			*(volatile unsigned int *)0xB800D030 = 0x73 ;      
			*(volatile unsigned int *)0xB800D034 = 0x2AA2AA ;   
			*(volatile unsigned int *)0xB800D038 = 0x3E02AA ;  
			*(volatile unsigned int *)0xB800D03C = 0xFFE11 ;   
			*(volatile unsigned int *)0xB800D040 = 0x2A3503 ;  
			*(volatile unsigned int *)0xB800D044 = 0xFEDCBA98 ;
			*(volatile unsigned int *)0xB800D048 = 0x1501800 ; 
			*(volatile unsigned int *)0xB800D04C = 0x6978 ;    
			*(volatile unsigned int *)0xB800D040 = 0x202003 ;  
			*(volatile unsigned int *)0xB800D048 = 0x501800 ;  
			*(volatile unsigned int *)0xB800D054 = 0xD ;       
			*(volatile unsigned int *)0xB800D058 = 0x0 ;       
			*(volatile unsigned int *)0xB800D05C = 0x0 ;       
			*(volatile unsigned int *)0xB800D060 = 0x0 ;       
			*(volatile unsigned int *)0xB800D064 = 0x0 ;       
			*(volatile unsigned int *)0xB800D068 = 0x0 ;       
			*(volatile unsigned int *)0xB800D06C = 0x0 ;       
			*(volatile unsigned int *)0xB800D070 = 0x0 ;       
			*(volatile unsigned int *)0xB800D074 = 0x0 ;        
			*(volatile unsigned int *)0xB800D078 = 0x0 ;        
			*(volatile unsigned int *)0xB800D07C = 0x21084210 ; 
			*(volatile unsigned int *)0xB800D080 = 0x7E ;       
			*(volatile unsigned int *)0xB800D088 = 0x0 ;        
			*(volatile unsigned int *)0xB800D08C = 0xCA1 ;      
			*(volatile unsigned int *)0xB800D094 = 0x1C000000 ; 
			*(volatile unsigned int *)0xB800D090 = 0x7D04B ;    
			*(volatile unsigned int *)0xB800D094 = 0x1C000000 ; 
			*(volatile unsigned int *)0xB800D098 = 0x0 ;        
			*(volatile unsigned int *)0xB800D100 = 0x2A ;       
			*(volatile unsigned int *)0xB800D104 = 0x25131FB ;  
			*(volatile unsigned int *)0xB800D108 = 0x241F1FF ;  
			*(volatile unsigned int *)0xB800D10C = 0xE8 ;       
			*(volatile unsigned int *)0xB800D110 = 0x0 ;        
			*(volatile unsigned int *)0xB800D118 = 0x0 ;        
			*(volatile unsigned int *)0xB800D11C = 0x0 ;        
			*(volatile unsigned int *)0xB800D120 = 0x0 ;        
			*(volatile unsigned int *)0xB800D124 = 0x0 ;        
			*(volatile unsigned int *)0xB800D128 = 0x0 ;        
			*(volatile unsigned int *)0xB800D12C = 0x0 ;        
			*(volatile unsigned int *)0xB800D134 = 0x0 ;        
			*(volatile unsigned int *)0xB800D13C = 0x0 ;        
			*(volatile unsigned int *)0xB800D140 = 0x0 ;        
			*(volatile unsigned int *)0xB800D144 = 0x0 ;        
			*(volatile unsigned int *)0xB800D148 = 0x0 ;        
			*(volatile unsigned int *)0xB800D14C = 0x0 ;        
			*(volatile unsigned int *)0xB800D000 = 0x6 ;        
			*(volatile unsigned int *)0xB800D058 = 0xD0282 ;   
			*(volatile unsigned int *)0xB800D05C = 0x580015 ;  
			*(volatile unsigned int *)0xB800D060 = 0x2 ;       
			*(volatile unsigned int *)0xB800D064 = 0x0 ;       
			*(volatile unsigned int *)0xB800D068 = 0x0 ;       
			*(volatile unsigned int *)0xB800D06C = 0x0 ;       
			*(volatile unsigned int *)0xB800D070 = 0x0 ;      
			*(volatile unsigned int *)0xB800D074 = 0x0 ;      
			*(volatile unsigned int *)0xB800D078 = 0x0 ;      
			*(volatile unsigned int *)0xB800D088 = 0x7762 ;   
			*(volatile unsigned int *)0xB800D07C = 0x21084210 ;
			*(volatile unsigned int *)0xB800D080 = 0x7E ;     
			*(volatile unsigned int *)0xB800D07C = 0x11 ;     
			*(volatile unsigned int *)0xB800D080 = 0x3 ;     
	
			// disable TVE colorbar, enable interrupt
			*(volatile unsigned int *)0xb80180ec = 0x18 ;
			*(volatile unsigned int *)0xb8005504 = 0xfffffffe ;
			*(volatile unsigned int *)0xb8005504 = 0xf ;
			*(volatile unsigned int *)0xb801810c = 0xa00a000 ;
			*(volatile unsigned int *)0xb80180e0 = 0x0 ;
			*(volatile unsigned int *)0xb80055e4 = 0x9 ;
			*(volatile unsigned int *)0xb80180dc = 0x1 ;
		}
	} else if (logo_info.mode == 1) {
		// PAL_I mode
		if (is_venus_cpu()) {
//			*(volatile unsigned int *)0xb801b200 = 0x63 ;
			// set VO registers
			*(volatile unsigned int *)0xb8005350 = 0xfffffffe ;
			*(volatile unsigned int *)0xb8005350 = 0x3 ;
			*(volatile unsigned int *)0xb8005354 = 0x2cf ;
			*(volatile unsigned int *)0xb8005358 = 0x31f63f ;
			*(volatile unsigned int *)0xb800535c = 0xffff3210 ;
			*(volatile unsigned int *)0xb8005360 = 0x0 ;
			
			// color lookup table of Sub-Picture , index0~index3
			*(volatile unsigned int *)0xb8005370 = logo_info.color[0] ;
			*(volatile unsigned int *)0xb8005374 = logo_info.color[1] ;
			*(volatile unsigned int *)0xb8005378 = logo_info.color[2] ;
			*(volatile unsigned int *)0xb800537c = logo_info.color[3] ;
			*(volatile unsigned int *)0xb80053b0 = 0xacf800 ;
			*(volatile unsigned int *)0xb80053b4 = 0xa3f800 ;
			*(volatile unsigned int *)0xb80053bc = 0x4000 ;
			*(volatile unsigned int *)0xb80053c0 = 0x0 ;
			*(volatile unsigned int *)0xb80053c4 = 0x0 ;
			*(volatile unsigned int *)0xb80053c8 = 0x0 ;
			*(volatile unsigned int *)0xb80053cc = 0x0 ;
			*(volatile unsigned int *)0xb80053d0 = 0x200 ;
			*(volatile unsigned int *)0xb80053d8 = 0x4000 ;
			*(volatile unsigned int *)0xb80053dc = 0x0 ;
			*(volatile unsigned int *)0xb80053e0 = 0x0 ;
			*(volatile unsigned int *)0xb80053e4 = 0x0 ;
			*(volatile unsigned int *)0xb80053e8 = 0x0 ;
			*(volatile unsigned int *)0xb80053ec = 0x0 ;
			*(volatile unsigned int *)0xb80053f0 = 0x0 ;
			*(volatile unsigned int *)0xb80053f4 = 0x0 ;
			*(volatile unsigned int *)0xb80053f8 = 0x0 ;
			*(volatile unsigned int *)0xb80053fc = 0x400 ;
			*(volatile unsigned int *)0xb8005400 = 0xffd ;
			*(volatile unsigned int *)0xb8005404 = 0xf8f ;
			*(volatile unsigned int *)0xb8005408 = 0xf60 ;
			*(volatile unsigned int *)0xb800540c = 0xf50 ;
			*(volatile unsigned int *)0xb8005410 = 0xfa8 ;
			*(volatile unsigned int *)0xb8005414 = 0x207 ;
			*(volatile unsigned int *)0xb8005418 = 0x30a ;
			*(volatile unsigned int *)0xb800541c = 0x50b ;
			*(volatile unsigned int *)0xb80054b0 = 0xa3facf ;
			*(volatile unsigned int *)0xb80054b4 = 0x16030100 ;
			*(volatile unsigned int *)0xb80054e8 = 0x2001400 ;
			*(volatile unsigned int *)0xb80054ec = 0x2001000 ;
			*(volatile unsigned int *)0xb80054f0 = 0x2001400 ;
			*(volatile unsigned int *)0xb80054f4 = 0x2001000 ;
			*(volatile unsigned int *)0xb80054f8 = 0x400 ;
			*(volatile unsigned int *)0xb80054fc = 0x20080200 ;
			*(volatile unsigned int *)0xb8005500 = 0xf010eb10 ;
			*(volatile unsigned int *)0xb8005564 = 0x0 ;
			*(volatile unsigned int *)0xb8005568 = 0x1 ;
			*(volatile unsigned int *)0xb800556c = 0x2 ;
			*(volatile unsigned int *)0xb8005570 = 0x3 ;
			*(volatile unsigned int *)0xb8005574 = 0x4 ;
			*(volatile unsigned int *)0xb8005578 = 0x5 ;
			*(volatile unsigned int *)0xb800557c = 0x6 ;
			*(volatile unsigned int *)0xb8005580 = 0x7 ;
			*(volatile unsigned int *)0xb8005584 = 0x8 ;
			
			// Top and Bot address of Sub-Picture 
			*(volatile unsigned int *)0xb8005530 = 0xf0000 ;
			*(volatile unsigned int *)0xb8005534 = 0xf0000+logo_info.size ;
			
			// set TVE registers
			*(volatile unsigned int *)0xb8000084 = 0x101cd800 ;
			*(volatile unsigned int *)0xb800000c = 0x1 ;
			*(volatile unsigned int *)0xB80180C8 = 0x136;
			*(volatile unsigned int *)0xB80180CC = 0x188;
			*(volatile unsigned int *)0xB8018134 = 0x278000;
			*(volatile unsigned int *)0xB80180D0 = 0x1F9AAA;
			*(volatile unsigned int *)0xB80180D4 = 0x1F9AAA;
			*(volatile unsigned int *)0xB80180E8 = 0x1;
			*(volatile unsigned int *)0xB8018154 = 0x205;
			*(volatile unsigned int *)0xB8018880 = 0xCB;
			*(volatile unsigned int *)0xB8018884 = 0x8A;
			*(volatile unsigned int *)0xB8018888 = 0x9;
			*(volatile unsigned int *)0xB801888C = 0x2A;
			*(volatile unsigned int *)0xB8018890 = 0x0;
			*(volatile unsigned int *)0xB8018894 = 0x0;
			*(volatile unsigned int *)0xB8018898 = 0x0;
			*(volatile unsigned int *)0xB801889C = 0x9B;
			*(volatile unsigned int *)0xB80188A0 = 0x2;
			*(volatile unsigned int *)0xB80188A8 = 0x78;
			*(volatile unsigned int *)0xB80188AC = 0x78;
			*(volatile unsigned int *)0xB80188B0 = 0x10;
			*(volatile unsigned int *)0xB80188B4 = 0x3;
			*(volatile unsigned int *)0xB80188B8 = 0x1F;
			*(volatile unsigned int *)0xB80188B8 = 0x1D;
			*(volatile unsigned int *)0xB8018984 = 0x20;
			*(volatile unsigned int *)0xB801898C = 0x2;
			*(volatile unsigned int *)0xB80188BC = 0x0;
			*(volatile unsigned int *)0xB80188C8 = 0xD7;
			*(volatile unsigned int *)0xB80188CC = 0x29;
			*(volatile unsigned int *)0xB80188D0 = 0x3;
			*(volatile unsigned int *)0xB80188E0 = 0x9;
			*(volatile unsigned int *)0xB80188E4 = 0x31;
			*(volatile unsigned int *)0xB80188E8 = 0x38;
			*(volatile unsigned int *)0xB80188EC = 0x6;
			*(volatile unsigned int *)0xB80188F0 = 0xBF;
			*(volatile unsigned int *)0xB80188F4 = 0x3;
			*(volatile unsigned int *)0xB80188F8 = 0x5F;
			*(volatile unsigned int *)0xB80189C0 = 0x5C;
			*(volatile unsigned int *)0xB80189C4 = 0x40;
			*(volatile unsigned int *)0xB80189C8 = 0x24;
			*(volatile unsigned int *)0xB80189CC = 0x1C;
			*(volatile unsigned int *)0xB8018920 = 0x0;
			*(volatile unsigned int *)0xB8018924 = 0x39;
			*(volatile unsigned int *)0xB8018928 = 0x22;
			*(volatile unsigned int *)0xB801892C = 0x5A;
			*(volatile unsigned int *)0xB8018930 = 0x22;
			*(volatile unsigned int *)0xB8018934 = 0xA8;
			*(volatile unsigned int *)0xB8018938 = 0x1C;
			*(volatile unsigned int *)0xB801893C = 0x34;
			*(volatile unsigned int *)0xB8018940 = 0x14;
			*(volatile unsigned int *)0xB8018944 = 0x3;
			*(volatile unsigned int *)0xB8018948 = 0xFE;
			*(volatile unsigned int *)0xB801894C = 0x1;
			*(volatile unsigned int *)0xB8018950 = 0x54;
			*(volatile unsigned int *)0xB8018954 = 0xFE;
			*(volatile unsigned int *)0xB8018958 = 0x7E;
			*(volatile unsigned int *)0xB801895C = 0x60;
			*(volatile unsigned int *)0xB8018960 = 0x80;
			*(volatile unsigned int *)0xB8018964 = 0x47;
			*(volatile unsigned int *)0xB8018968 = 0x55;
			*(volatile unsigned int *)0xB801896C = 0x1;
			*(volatile unsigned int *)0xB80180D8 = 0x40;
			*(volatile unsigned int *)0xB8018990 = 0x0;
			*(volatile unsigned int *)0xB80189D0 = 0x0;
			*(volatile unsigned int *)0xB80189D4 = 0x3F;
			*(volatile unsigned int *)0xB80189D8 = 0x71;
			*(volatile unsigned int *)0xB80189DC = 0x20;
			*(volatile unsigned int *)0xB80189E0 = 0x80;
			*(volatile unsigned int *)0xB80189E4 = 0xA4;
			*(volatile unsigned int *)0xB80189E8 = 0x50;
			*(volatile unsigned int *)0xB80189EC = 0x57;
			*(volatile unsigned int *)0xB80189F0 = 0x75;
			*(volatile unsigned int *)0xB8018000 = 0x2A9C235F;
			*(volatile unsigned int *)0xB8018004 = 0x29AE6D;
			*(volatile unsigned int *)0xB80180AC = 0xE0B1B;
			*(volatile unsigned int *)0xB8018048 = 0x80FA30D;
			*(volatile unsigned int *)0xB8018050 = 0x816935;
			*(volatile unsigned int *)0xB8018054 = 0x94FA6E;
			*(volatile unsigned int *)0xB801804C = 0x81F234C;
			*(volatile unsigned int *)0xB8018058 = 0x82CA6B;
			*(volatile unsigned int *)0xB80180C4 = 0x7EC00;
			*(volatile unsigned int *)0xB80180EC = 0x19;
			*(volatile unsigned int *)0xB80180EC = 0x6;
			*(volatile unsigned int *)0xB80180EC = 0x1;
			*(volatile unsigned int *)0xB80180F0 = 0x22D43C;
			*(volatile unsigned int *)0xB80180F4 = 0x22CC88;
			*(volatile unsigned int *)0xB8018084 = 0x80009404;
			*(volatile unsigned int *)0xB8018088 = 0x80008000;
			*(volatile unsigned int *)0xB801808C = 0x93929392;
			*(volatile unsigned int *)0xB8018090 = 0x8C708D40;
			*(volatile unsigned int *)0xB8018110 = 0xC70;
			*(volatile unsigned int *)0xB8018094 = 0x80008000;
			*(volatile unsigned int *)0xB80180FC = 0x27E1800;
			*(volatile unsigned int *)0xB80180A8 = 0x7F4800;
			*(volatile unsigned int *)0xB8018098 = 0x6A0817A0;
			*(volatile unsigned int *)0xB801809C = 0x28A1BB0;
			*(volatile unsigned int *)0xB8018148 = 0xA56D0D0;
			*(volatile unsigned int *)0xB8018040 = 0x1;
			*(volatile unsigned int *)0xB8018044 = 0x20290242;
			*(volatile unsigned int *)0xB801805C = 0x800E32A;
			*(volatile unsigned int *)0xB8018060 = 0x824;
			*(volatile unsigned int *)0xB80180D8 = 0x3FE;
			*(volatile unsigned int *)0xB80180D8 = 0x32B;
			*(volatile unsigned int *)0xB80180A4 = 0x81E634D;
			*(volatile unsigned int *)0xB8018100 = 0xBE8800;
			*(volatile unsigned int *)0xB8018104 = 0xA6C82C;
			*(volatile unsigned int *)0xB8018108 = 0xBE8BE8;
			*(volatile unsigned int *)0xB80180A0 = 0x7E;
			*(volatile unsigned int *)0xB80180A0 = 0x2F ; 
			*(volatile unsigned int *)0xB8018000 = 0x30000000;

			
			// disable TVE colorbar, enable interrupt
			*(volatile unsigned int *)0xb80180ec = 0x18 ;
			*(volatile unsigned int *)0xb8005504 = 0xfffffffe ;
			*(volatile unsigned int *)0xb8005504 = 0xf ;
			*(volatile unsigned int *)0xb801810c = 0xa00a000 ;
			*(volatile unsigned int *)0xb80180e0 = 0x0 ;
			*(volatile unsigned int *)0xb80055f0 = 0x9 ;
			*(volatile unsigned int *)0xb80180dc = 0x1 ;

#ifndef CONFIG_REALTEK_HDMI_NONE
			// HDMI
			*(volatile unsigned int *)0xb801b36c = 0x0;
			*(volatile unsigned int *)0xb801b300 = 0x63;
			*(volatile unsigned int *)0xb801b330 = 0x0;
			*(volatile unsigned int *)0xb801b338 = 0x0;
			*(volatile unsigned int *)0xb801b33c = 0x8;
			*(volatile unsigned int *)0xb801b36c = 0x1;
			
			BusyWaiting(70000);
#ifdef CONFIG_REALTEK_HDMI_NXP
			SET_NXP_HDMI_576P();
#else
			SET_CAT_HDMI_576P();
#endif
			BusyWaiting(70000);
#endif
		} else {
//			*(volatile unsigned int *)0xb801b200 = 0x64 ;
			// set VO registers
			*(volatile unsigned int *)0xb8005350 = 0xfffffffe ;
			*(volatile unsigned int *)0xb8005350 = 0x3 ;
			*(volatile unsigned int *)0xb8005354 = 0x2cf ;
			*(volatile unsigned int *)0xb8005358 = 0x31f63f ;
			*(volatile unsigned int *)0xb800535c = 0xffff3210 ;
			*(volatile unsigned int *)0xb8005360 = 0x0 ;

			//color lookup table of Sub-Picture , index0~index3
			*(volatile unsigned int *)0xb8005370 = logo_info.color[0] ;
			*(volatile unsigned int *)0xb8005374 = logo_info.color[1] ;
			*(volatile unsigned int *)0xb8005378 = logo_info.color[2] ;
			*(volatile unsigned int *)0xb800537c = logo_info.color[3] ;
			*(volatile unsigned int *)0xb80053b0 = 0xacf800 ;
			*(volatile unsigned int *)0xb80053b4 = 0xa3f800 ;
			*(volatile unsigned int *)0xb80056cc = 0x4000 ;
			*(volatile unsigned int *)0xb80056d0 = 0x0 ;
			*(volatile unsigned int *)0xb80056c8 = 0x2 ;
			*(volatile unsigned int *)0xb80056fc = 0x4000 ;
			*(volatile unsigned int *)0xb8005700 = 0x0 ;
			*(volatile unsigned int *)0xb80056f4 = 0x2 ;
			*(volatile unsigned int *)0xb80054b0 = 0xa3facf ;
			*(volatile unsigned int *)0xb80054b4 = 0x16030100 ;
			*(volatile unsigned int *)0xb80054e8 = 0x2001400 ;
			*(volatile unsigned int *)0xb80054ec = 0x2001000 ;
			*(volatile unsigned int *)0xb80054f0 = 0x2001400 ;
			*(volatile unsigned int *)0xb80054f4 = 0x2001000 ;
			*(volatile unsigned int *)0xb80054f8 = 0x400 ;
			*(volatile unsigned int *)0xb80054fc = 0x20080200 ;
			*(volatile unsigned int *)0xb8005500 = 0xf010eb10 ;
			*(volatile unsigned int *)0xb8005564 = 0x0 ;
			*(volatile unsigned int *)0xb8005568 = 0x1 ;
			*(volatile unsigned int *)0xb800556c = 0x2 ;
			*(volatile unsigned int *)0xb8005570 = 0x3 ;
			*(volatile unsigned int *)0xb8005574 = 0x4 ;
			*(volatile unsigned int *)0xb8005578 = 0x5 ;
			*(volatile unsigned int *)0xb800557c = 0x6 ;
			*(volatile unsigned int *)0xb8005580 = 0x7 ;
			*(volatile unsigned int *)0xb8005584 = 0x8 ;

			// Top and Bot address of Sub-Picture 
			*(volatile unsigned int *)0xb8005530 = 0xf0000 ;
			*(volatile unsigned int *)0xb8005534 = 0xf0000+logo_info.size ;

			// set TVE registers
			*(volatile unsigned int *)0xb8000084 = 0x101cd800 ;
			*(volatile unsigned int *)0xb800000c = 0x1 ;
			*(volatile unsigned int *)0xB80180C8 = 0x124 ;    
			*(volatile unsigned int *)0xB80180CC = 0x188 ;    
			*(volatile unsigned int *)0xB8018134 = 0x278000 ; 
			*(volatile unsigned int *)0xB80180D0 = 0x3FBBBB ; 
			*(volatile unsigned int *)0xB80180D4 = 0x3FCBBB ; 
			*(volatile unsigned int *)0xB80180E8 = 0x1 ;      
			*(volatile unsigned int *)0xB8018154 = 0x205 ;    
			*(volatile unsigned int *)0xB8018880 = 0xCB ;     
			*(volatile unsigned int *)0xB8018884 = 0x8A ;     
			*(volatile unsigned int *)0xB8018888 = 0x9 ;      
			*(volatile unsigned int *)0xB801888C = 0x2A ;     
			*(volatile unsigned int *)0xB8018890 = 0x0 ;      
			*(volatile unsigned int *)0xB8018894 = 0x0 ;      
			*(volatile unsigned int *)0xB8018898 = 0x0 ;      
			*(volatile unsigned int *)0xB801889C = 0x9B ;     
			*(volatile unsigned int *)0xB80188A0 = 0x2 ;      
			*(volatile unsigned int *)0xB80188A8 = 0x78 ;     
			*(volatile unsigned int *)0xB80188AC = 0x78 ;     
			*(volatile unsigned int *)0xB80188B0 = 0x10 ;     
			*(volatile unsigned int *)0xB80188B4 = 0x3 ;      
			*(volatile unsigned int *)0xB80188B8 = 0x1D ;     
			*(volatile unsigned int *)0xB80188B8 = 0x1D ;     
			*(volatile unsigned int *)0xB8018984 = 0x20 ;     
			*(volatile unsigned int *)0xB801898C = 0x2 ;      
			*(volatile unsigned int *)0xB80188BC = 0x0 ;      
			*(volatile unsigned int *)0xB80188C8 = 0xD7 ;     
			*(volatile unsigned int *)0xB80188CC = 0x29 ;     
			*(volatile unsigned int *)0xB80188D0 = 0x3 ;      
			*(volatile unsigned int *)0xB80188E0 = 0x9 ;      
			*(volatile unsigned int *)0xB80188E4 = 0x31 ;     
			*(volatile unsigned int *)0xB80188E8 = 0x38 ;     
			*(volatile unsigned int *)0xB80188EC = 0x6 ;      
			*(volatile unsigned int *)0xB80188F0 = 0xBF ;     
			*(volatile unsigned int *)0xB80188F4 = 0x3 ;      
			*(volatile unsigned int *)0xB80188F8 = 0x5F ;     
			*(volatile unsigned int *)0xB80189C0 = 0x5C ;     
			*(volatile unsigned int *)0xB80189C4 = 0x40 ;     
			*(volatile unsigned int *)0xB80189C8 = 0x24 ;     
			*(volatile unsigned int *)0xB80189CC = 0x2B ;     
			*(volatile unsigned int *)0xB8018920 = 0x0 ;      
			*(volatile unsigned int *)0xB8018924 = 0x39 ;     
			*(volatile unsigned int *)0xB8018928 = 0x22 ;     
			*(volatile unsigned int *)0xB801892C = 0x5A ;     
			*(volatile unsigned int *)0xB8018930 = 0x22 ;     
			*(volatile unsigned int *)0xB8018934 = 0xA8 ;     
			*(volatile unsigned int *)0xB8018938 = 0x1C ;     
			*(volatile unsigned int *)0xB801893C = 0x34 ;     
			*(volatile unsigned int *)0xB8018940 = 0x14 ;     
			*(volatile unsigned int *)0xB8018944 = 0x3 ;      
			*(volatile unsigned int *)0xB8018948 = 0xFE ;     
			*(volatile unsigned int *)0xB801894C = 0x1 ;      
			*(volatile unsigned int *)0xB8018950 = 0x54 ;     
			*(volatile unsigned int *)0xB8018954 = 0xFE ;     
			*(volatile unsigned int *)0xB8018958 = 0x7E ;     
			*(volatile unsigned int *)0xB801895C = 0x60 ;     
			*(volatile unsigned int *)0xB8018960 = 0x80 ;     
			*(volatile unsigned int *)0xB8018964 = 0x47 ;     
			*(volatile unsigned int *)0xB8018968 = 0x55 ;     
			*(volatile unsigned int *)0xB801896C = 0x1 ;      
			*(volatile unsigned int *)0xB80180D8 = 0x40 ;     
			*(volatile unsigned int *)0xB8018990 = 0x0 ;      
			*(volatile unsigned int *)0xB80189D0 = 0x0 ;      
			*(volatile unsigned int *)0xB80189D4 = 0x3F ;     
			*(volatile unsigned int *)0xB80189D8 = 0x71 ;     
			*(volatile unsigned int *)0xB80189DC = 0x20 ;     
			*(volatile unsigned int *)0xB80189E0 = 0x80 ;     
			*(volatile unsigned int *)0xB80189E4 = 0xA4 ;     
			*(volatile unsigned int *)0xB80189E8 = 0x50 ;     
			*(volatile unsigned int *)0xB80189EC = 0x57 ;     
			*(volatile unsigned int *)0xB80189F0 = 0x75 ;     
			*(volatile unsigned int *)0xB8018000 = 0x2A9C235F ;
			*(volatile unsigned int *)0xB8018004 = 0x29AE6D ; 
			*(volatile unsigned int *)0xB80180AC = 0xB1B ;    
			*(volatile unsigned int *)0xB8018048 = 0x80FA30D ;
			*(volatile unsigned int *)0xB8018050 = 0x816935 ; 
			*(volatile unsigned int *)0xB8018054 = 0x94FA6E ; 
			*(volatile unsigned int *)0xB801804C = 0x81F234C ;
			*(volatile unsigned int *)0xB8018058 = 0x82CA6B ; 
			*(volatile unsigned int *)0xB80180C4 = 0x7EC00 ;  
			*(volatile unsigned int *)0xB80180EC = 0x19 ;     
			*(volatile unsigned int *)0xB80180EC = 0x6 ;     
			*(volatile unsigned int *)0xB80180EC = 0x1 ;      
			*(volatile unsigned int *)0xB80180F0 = 0x22D43C ; 
			*(volatile unsigned int *)0xB80180F4 = 0x22CC88 ; 
			*(volatile unsigned int *)0xB8018084 = 0x80009404 ;
			*(volatile unsigned int *)0xB8018088 = 0x80008000 ;
			*(volatile unsigned int *)0xB801808C = 0x93929392 ;
			*(volatile unsigned int *)0xB8018090 = 0x8C708D40 ;
			*(volatile unsigned int *)0xB8018110 = 0xC70 ;    
			*(volatile unsigned int *)0xB8018094 = 0x80008000 ;
			*(volatile unsigned int *)0xB80180FC = 0x27E1800 ;
			*(volatile unsigned int *)0xB80180A8 = 0x7F400E0 ;
			*(volatile unsigned int *)0xB80180AC = 0x2359 ;   
			*(volatile unsigned int *)0xB8018098 = 0x6A0817A0 ;
			*(volatile unsigned int *)0xB801809C = 0x28A1BB0 ;
			*(volatile unsigned int *)0xB8018148 = 0xA56D0D0 ;
			*(volatile unsigned int *)0xB8018040 = 0x1 ;      
			*(volatile unsigned int *)0xB8018044 = 0x20290242 ;
			*(volatile unsigned int *)0xB801805C = 0x800E32A ;
			*(volatile unsigned int *)0xB8018060 = 0x824 ;    
			*(volatile unsigned int *)0xB80180D8 = 0x3FE ;    
			*(volatile unsigned int *)0xB80180D8 = 0x32B ;    
			*(volatile unsigned int *)0xB80180A4 = 0x81E634D ; 
			*(volatile unsigned int *)0xB8018100 = 0xBE8800 ; 
			*(volatile unsigned int *)0xB8018104 = 0xA6C82C ; 
			*(volatile unsigned int *)0xB8018108 = 0xBE8BE8 ; 
			*(volatile unsigned int *)0xB80180A0 = 0x7E ;     
			*(volatile unsigned int *)0xB8018000 = 0x30000000 ;

			// set HDMI registers            
			*(volatile unsigned int *)0xB80000D0 = 0x1 ;      
			*(volatile unsigned int *)0xB8018158 = 0x3A002000 ;
			*(volatile unsigned int *)0xB8018164 = 0x8D62038 ;
			*(volatile unsigned int *)0xB8018168 = 0x2002008 ; 
			*(volatile unsigned int *)0xB801816C = 0x2016008 ; 
			*(volatile unsigned int *)0xB8018170 = 0x2002000 ;
			*(volatile unsigned int *)0xB8018174 = 0x2002000 ;
			*(volatile unsigned int *)0xB8018160 = 0x8D3206E ; 
			*(volatile unsigned int *)0xB800D010 = 0x0 ;      
			*(volatile unsigned int *)0xB800D014 = 0xF9C938 ; 
			*(volatile unsigned int *)0xB800D018 = 0xA05F30 ; 
			*(volatile unsigned int *)0xB800D01C = 0x125499A ; 
			*(volatile unsigned int *)0xB800D150 = 0x37AFCB4 ;
			*(volatile unsigned int *)0xB800D020 = 0x3B ;     
			*(volatile unsigned int *)0xB800D154 = 0x924 ;    
			*(volatile unsigned int *)0xB800D02C = 0x4630F0 ; 
			*(volatile unsigned int *)0xB800D030 = 0x73 ;     
			*(volatile unsigned int *)0xB800D034 = 0x2AA2AA ; 
			*(volatile unsigned int *)0xB800D038 = 0x3E02AA ; 
			*(volatile unsigned int *)0xB800D03C = 0xFFE11 ;  
			*(volatile unsigned int *)0xB800D040 = 0x2A3503 ; 
			*(volatile unsigned int *)0xB800D044 = 0xFEDCBA98 ;
			*(volatile unsigned int *)0xB800D048 = 0x1501800 ;
			*(volatile unsigned int *)0xB800D04C = 0x6978 ;   
			*(volatile unsigned int *)0xB800D040 = 0x202003 ; 
			*(volatile unsigned int *)0xB800D048 = 0x501800 ; 
			*(volatile unsigned int *)0xB800D054 = 0xD ;      
			*(volatile unsigned int *)0xB800D058 = 0x0 ;      
			*(volatile unsigned int *)0xB800D05C = 0x0 ;      
			*(volatile unsigned int *)0xB800D060 = 0x0 ;      
			*(volatile unsigned int *)0xB800D064 = 0x0 ;      
			*(volatile unsigned int *)0xB800D068 = 0x0 ;      
			*(volatile unsigned int *)0xB800D06C = 0x0 ;      
			*(volatile unsigned int *)0xB800D070 = 0x0 ;      
			*(volatile unsigned int *)0xB800D074 = 0x0 ;      
			*(volatile unsigned int *)0xB800D078 = 0x0 ;      
			*(volatile unsigned int *)0xB800D07C = 0x21084210 ;
			*(volatile unsigned int *)0xB800D080 = 0x7E ;     
			*(volatile unsigned int *)0xB800D088 = 0x0 ;      
			*(volatile unsigned int *)0xB800D08C = 0xCA1 ;    
			*(volatile unsigned int *)0xB800D094 = 0x1C000000 ;
			*(volatile unsigned int *)0xB800D090 = 0x7D04B ;  
			*(volatile unsigned int *)0xB800D094 = 0x1C000000 ;
			*(volatile unsigned int *)0xB800D098 = 0x0 ;      
			*(volatile unsigned int *)0xB800D100 = 0x2A ;     
			*(volatile unsigned int *)0xB800D104 = 0x25131FB ;
			*(volatile unsigned int *)0xB800D108 = 0x241F1FF ;
			*(volatile unsigned int *)0xB800D10C = 0xE8 ;     
			*(volatile unsigned int *)0xB800D110 = 0x0 ;      
			*(volatile unsigned int *)0xB800D118 = 0x0 ;      
			*(volatile unsigned int *)0xB800D11C = 0x0 ;      
			*(volatile unsigned int *)0xB800D120 = 0x0 ;      
			*(volatile unsigned int *)0xB800D124 = 0x0 ;      
			*(volatile unsigned int *)0xB800D128 = 0x0 ;      
			*(volatile unsigned int *)0xB800D12C = 0x0 ;      
			*(volatile unsigned int *)0xB800D134 = 0x0 ;      
			*(volatile unsigned int *)0xB800D13C = 0x0 ;      
			*(volatile unsigned int *)0xB800D140 = 0x0 ;      
			*(volatile unsigned int *)0xB800D144 = 0x0 ;      
			*(volatile unsigned int *)0xB800D148 = 0x0 ;      
			*(volatile unsigned int *)0xB800D14C = 0x0 ;      
			*(volatile unsigned int *)0xB800D000 = 0x6 ;      
			*(volatile unsigned int *)0xB800D058 = 0xD0282 ;  
			*(volatile unsigned int *)0xB800D05C = 0x580006 ; 
			*(volatile unsigned int *)0xB800D060 = 0x11 ;     
			*(volatile unsigned int *)0xB800D064 = 0x0 ;      
			*(volatile unsigned int *)0xB800D068 = 0x0 ;      
			*(volatile unsigned int *)0xB800D06C = 0x0 ;      
			*(volatile unsigned int *)0xB800D070 = 0x0 ;      
			*(volatile unsigned int *)0xB800D074 = 0x0 ;      
			*(volatile unsigned int *)0xB800D078 = 0x0 ;      
			*(volatile unsigned int *)0xB800D088 = 0x7762 ;   
			*(volatile unsigned int *)0xB800D07C = 0x21084210 ;
			*(volatile unsigned int *)0xB800D080 = 0x7E ;     
			*(volatile unsigned int *)0xB800D07C = 0x11 ;     
			*(volatile unsigned int *)0xB800D080 = 0x3 ;      

			// disable TVE colorbar, enable interrupt
			*(volatile unsigned int *)0xb80180ec = 0x18 ;
			*(volatile unsigned int *)0xb8005504 = 0xfffffffe ;
			*(volatile unsigned int *)0xb8005504 = 0xf ;
			*(volatile unsigned int *)0xb801810c = 0xa00a000 ;
			*(volatile unsigned int *)0xb80180e0 = 0x0 ;
			*(volatile unsigned int *)0xb80055f0 = 0x9 ;
			*(volatile unsigned int *)0xb80180dc = 0x1 ;
		}
	} else {
		// PAL_M mode
		// set VO registers
		*(volatile unsigned int *)0xb8005350 = 0xfffffffe ;
		*(volatile unsigned int *)0xb8005350 = 0x3 ;
		*(volatile unsigned int *)0xb8005354 = 0x2cf ;
		*(volatile unsigned int *)0xb8005358 = 0x2ef5df ;
		*(volatile unsigned int *)0xb800535c = 0xffff3210 ;
		*(volatile unsigned int *)0xb8005360 = 0x0 ;

		// color lookup table of Sub-Picture , index0~index3
		*(volatile unsigned int *)0xb8005370 = logo_info.color[0] ;
		*(volatile unsigned int *)0xb8005374 = logo_info.color[1] ;
		*(volatile unsigned int *)0xb8005378 = logo_info.color[2] ;
		*(volatile unsigned int *)0xb800537c = logo_info.color[3] ;
		*(volatile unsigned int *)0xb80053b0 = 0xacf800 ;
		*(volatile unsigned int *)0xb80053b4 = 0x9df800 ;
		*(volatile unsigned int *)0xb80053bc = 0x4000 ;
		*(volatile unsigned int *)0xb80053c0 = 0x0 ;
		*(volatile unsigned int *)0xb80053c4 = 0x0 ;
		*(volatile unsigned int *)0xb80053c8 = 0x0 ;
		*(volatile unsigned int *)0xb80053cc = 0x0 ;
		*(volatile unsigned int *)0xb80053d0 = 0x200 ;
		*(volatile unsigned int *)0xb80053d8 = 0x4000 ;
		*(volatile unsigned int *)0xb80053dc = 0x0 ;
		*(volatile unsigned int *)0xb80053e0 = 0x0 ;
		*(volatile unsigned int *)0xb80053e4 = 0x0 ;
		*(volatile unsigned int *)0xb80053e8 = 0x0 ;
		*(volatile unsigned int *)0xb80053ec = 0x0 ;
		*(volatile unsigned int *)0xb80053f0 = 0x0 ;
		*(volatile unsigned int *)0xb80053f4 = 0x0 ;
		*(volatile unsigned int *)0xb80053f8 = 0x0 ;
		*(volatile unsigned int *)0xb80053fc = 0x400 ;
		*(volatile unsigned int *)0xb8005400 = 0xffd ;
		*(volatile unsigned int *)0xb8005404 = 0xf8f ;
		*(volatile unsigned int *)0xb8005408 = 0xf60 ;
		*(volatile unsigned int *)0xb800540c = 0xf50 ;
		*(volatile unsigned int *)0xb8005410 = 0xfa8 ;
		*(volatile unsigned int *)0xb8005414 = 0x207 ;
		*(volatile unsigned int *)0xb8005418 = 0x30a ;
		*(volatile unsigned int *)0xb800541c = 0x50b ;
		*(volatile unsigned int *)0xb80054b0 = 0x9dfacf ;
		*(volatile unsigned int *)0xb80054e8 = 0x2001400 ;
		*(volatile unsigned int *)0xb80054ec = 0x2001000 ;
		*(volatile unsigned int *)0xb80054f0 = 0x2001400 ;
		*(volatile unsigned int *)0xb80054f4 = 0x2001000 ;
		*(volatile unsigned int *)0xb80054f8 = 0x400 ;
		*(volatile unsigned int *)0xb80054fc = 0x20080200 ;
		*(volatile unsigned int *)0xb8005500 = 0xf010eb10 ;
		*(volatile unsigned int *)0xb8005564 = 0x0 ;
		*(volatile unsigned int *)0xb8005568 = 0x1 ;
		*(volatile unsigned int *)0xb800556c = 0x2 ;
		*(volatile unsigned int *)0xb8005570 = 0x3 ;
		*(volatile unsigned int *)0xb8005574 = 0x4 ;
		*(volatile unsigned int *)0xb8005578 = 0x5 ;
		*(volatile unsigned int *)0xb800557c = 0x6 ;
		*(volatile unsigned int *)0xb8005580 = 0x7 ;
		*(volatile unsigned int *)0xb8005584 = 0x8 ;

		// Top and Bot address of Sub-Picture 
		*(volatile unsigned int *)0xb8005530 = 0xf0000 ;
		*(volatile unsigned int *)0xb8005534 = 0xf0000+logo_info.size ;

		// set TVE registers
		*(volatile unsigned int *)0xb8000084 = 0x101cd800 ;
		*(volatile unsigned int *)0xb800000c = 0x1 ;
		*(volatile unsigned int *)0xb80180c8 = 0x124 ;
		*(volatile unsigned int *)0xb80180cc = 0x188 ;
		*(volatile unsigned int *)0xb8018134 = 0x3f8000 ;
		if (is_venus_cpu()) {
			*(volatile unsigned int *)0xb80180d0 = 0x1f9aaa ;
			*(volatile unsigned int *)0xb80180d4 = 0x1f9aaa ;
		} else {
			*(volatile unsigned int *)0xb80180d0 = 0x3fbbbb ;
			*(volatile unsigned int *)0xb80180d4 = 0x3fbbbb ;
		}
		*(volatile unsigned int *)0xb80180e8 = 0x1 ;
		*(volatile unsigned int *)0xb8018154 = 0x200 ;
		*(volatile unsigned int *)0xb8018880 = 0xa3 ;
		*(volatile unsigned int *)0xb8018884 = 0xef ;
		*(volatile unsigned int *)0xb8018888 = 0xe6 ;
		*(volatile unsigned int *)0xb801888c = 0x21 ;
		*(volatile unsigned int *)0xb8018890 = 0x0 ;
		*(volatile unsigned int *)0xb8018894 = 0x0 ;
		*(volatile unsigned int *)0xb8018898 = 0x0 ;
		*(volatile unsigned int *)0xb801889c = 0x0 ;
		*(volatile unsigned int *)0xb80188a0 = 0x0 ;
		*(volatile unsigned int *)0xb80188a8 = 0x8d ;
		*(volatile unsigned int *)0xb80188ac = 0x78 ;
		*(volatile unsigned int *)0xb80188b0 = 0x10 ;
		*(volatile unsigned int *)0xb80188b4 = 0x7 ;
		*(volatile unsigned int *)0xb80188b8 = 0x1d ;
		*(volatile unsigned int *)0xb80188b8 = 0x1d ;
		*(volatile unsigned int *)0xb8018984 = 0x20 ;
		*(volatile unsigned int *)0xb801898c = 0x2 ;
		*(volatile unsigned int *)0xb80188bc = 0x0 ;
		*(volatile unsigned int *)0xb80188c8 = 0xd7 ;
		*(volatile unsigned int *)0xb80188cc = 0x29 ;
		*(volatile unsigned int *)0xb80188d0 = 0x3 ;
		*(volatile unsigned int *)0xb80188e0 = 0xa ;
		*(volatile unsigned int *)0xb80188e4 = 0x31 ;
		*(volatile unsigned int *)0xb80188e8 = 0x6 ;
		*(volatile unsigned int *)0xb80188ec = 0x6 ;
		*(volatile unsigned int *)0xb80188f0 = 0xb3 ;
		*(volatile unsigned int *)0xb80188f4 = 0x3 ;
		*(volatile unsigned int *)0xb80188f8 = 0x58 ;
		*(volatile unsigned int *)0xb80189c0 = 0x64 ;
		*(volatile unsigned int *)0xb80189c4 = 0x2d ;
		*(volatile unsigned int *)0xb80189c8 = 0x7 ;
		*(volatile unsigned int *)0xb80189cc = 0x18 ;
		*(volatile unsigned int *)0xb8018920 = 0x0 ;
		*(volatile unsigned int *)0xb8018924 = 0x3a ;
		*(volatile unsigned int *)0xb8018928 = 0x11 ;
		*(volatile unsigned int *)0xb801892c = 0x4b ;
		*(volatile unsigned int *)0xb8018930 = 0x11 ;
		*(volatile unsigned int *)0xb8018934 = 0x3c ;
		*(volatile unsigned int *)0xb8018938 = 0x1b ;
		*(volatile unsigned int *)0xb801893c = 0x1b ;
		*(volatile unsigned int *)0xb8018940 = 0x24 ;
		*(volatile unsigned int *)0xb8018944 = 0x7 ;
		*(volatile unsigned int *)0xb8018948 = 0xf8 ;
		*(volatile unsigned int *)0xb801894c = 0x0 ;
		*(volatile unsigned int *)0xb8018950 = 0x0 ;
		*(volatile unsigned int *)0xb8018954 = 0xf ;
		*(volatile unsigned int *)0xb8018958 = 0xf ;
		*(volatile unsigned int *)0xb801895c = 0x60 ;
		*(volatile unsigned int *)0xb8018960 = 0xa0 ;
		*(volatile unsigned int *)0xb8018964 = 0x54 ;
		*(volatile unsigned int *)0xb8018968 = 0xff ;
		*(volatile unsigned int *)0xb801896c = 0x3 ;
		*(volatile unsigned int *)0xb80180d8 = 0x40 ;
		*(volatile unsigned int *)0xb8018990 = 0x0 ;
		*(volatile unsigned int *)0xb80189d0 = 0xc ;
		*(volatile unsigned int *)0xb80189d4 = 0x4b ;
		*(volatile unsigned int *)0xb80189d8 = 0x7a ;
		*(volatile unsigned int *)0xb80189dc = 0x2b ;
		*(volatile unsigned int *)0xb80189e0 = 0x85 ;
		*(volatile unsigned int *)0xb80189e4 = 0xaa ;
		*(volatile unsigned int *)0xb80189e8 = 0x5a ;
		*(volatile unsigned int *)0xb80189ec = 0x62 ;
		*(volatile unsigned int *)0xb80189f0 = 0x84 ;
		*(volatile unsigned int *)0xb8018000 = 0x2e832359 ;
		*(volatile unsigned int *)0xb8018004 = 0x306505 ;
		if (is_venus_cpu()) {
			*(volatile unsigned int *)0xb80180ac = 0xe0b59 ;
		} else {
			*(volatile unsigned int *)0xb80180ac = 0x3b59 ;
		}
		*(volatile unsigned int *)0xb8018048 = 0x8212353 ;
		*(volatile unsigned int *)0xb8018050 = 0x815904 ;
		*(volatile unsigned int *)0xb8018054 = 0x91ca0b ;
		*(volatile unsigned int *)0xb801804c = 0x8222358 ;
		*(volatile unsigned int *)0xb8018058 = 0x82aa09 ;
		*(volatile unsigned int *)0xb80180c4 = 0x76800 ;
		*(volatile unsigned int *)0xb80180ec = 0x19 ;
		*(volatile unsigned int *)0xb80180ec = 0x6 ;
		*(volatile unsigned int *)0xb80180ec = 0x11 ;
		*(volatile unsigned int *)0xb80180f0 = 0x22d484 ;
		*(volatile unsigned int *)0xb80180d8 = 0x3fe ;
		*(volatile unsigned int *)0xb80180d8 = 0x3ab ;
		*(volatile unsigned int *)0xb8018084 = 0x80008800 ;
		*(volatile unsigned int *)0xb8018088 = 0x80008000 ;
		*(volatile unsigned int *)0xb801808c = 0x937c937c ;
		*(volatile unsigned int *)0xb8018090 = 0xa0038380 ;
		*(volatile unsigned int *)0xb8018110 = 0x2003 ;
		*(volatile unsigned int *)0xb8018094 = 0x88008380 ;
		*(volatile unsigned int *)0xb8018098 = 0x6a081000 ;
		*(volatile unsigned int *)0xb801809c = 0x2081bb0 ;
		*(volatile unsigned int *)0xb80180fc = 0x2781800 ;
		*(volatile unsigned int *)0xb80180a0 = 0x3e ;
		*(volatile unsigned int *)0xb80180a0 = 0x35 ;
		*(volatile unsigned int *)0xb80180a4 = 0x822e001 ;
		*(volatile unsigned int *)0xb8018100 = 0x909803 ;
		*(volatile unsigned int *)0xb8018104 = 0x800816 ;
		*(volatile unsigned int *)0xb8018108 = 0x90691d ;
		*(volatile unsigned int *)0xb8018000 = 0x30000000 ;

		// disable TVE colorbar, enable interrupt
		*(volatile unsigned int *)0xb80180ec = 0x10 ;
		*(volatile unsigned int *)0xb8005504 = 0xfffffffe ;
		*(volatile unsigned int *)0xb8005504 = 0xb ;
		*(volatile unsigned int *)0xb801810c = 0xa00a000 ;
		*(volatile unsigned int *)0xb80180e0 = 0x0 ;
		*(volatile unsigned int *)0xb80055e4 = 0x9 ;
		*(volatile unsigned int *)0xb80180dc = 0x1 ;
	}
}
#endif

static void jupiter_save_state(
		unsigned int *mis_tc0,
		unsigned int *mis_tc1,
		unsigned int *mis_tc2,
		unsigned int *crt_padmux,
		unsigned int *mis_gpio_dir,
		unsigned int *mis_gpio_out,
		unsigned int *mis_gpio_ie,
		unsigned int *mis_gpio_dp)
{
	// save timer registers before enter suspend
	mis_tc0[0] = inl(VENUS_MIS_TC0CR);
	mis_tc0[1] = inl(VENUS_MIS_TC0ICR);
	mis_tc0[2] = inl(VENUS_MIS_TC0TVR);
	mis_tc1[0] = inl(VENUS_MIS_TC1CR);
	mis_tc1[1] = inl(VENUS_MIS_TC1ICR);
	mis_tc1[2] = inl(VENUS_MIS_TC1TVR);
	mis_tc2[0] = inl(VENUS_MIS_TC2CR);
	mis_tc2[1] = inl(VENUS_MIS_TC2ICR);
	mis_tc2[2] = inl(VENUS_MIS_TC2TVR);

	// save pad mux
	crt_padmux[0] = readl(JUPITER_MUXPAD_0);
	crt_padmux[1] = readl(JUPITER_MUXPAD_1);
	crt_padmux[2] = readl(JUPITER_MUXPAD_2);
	crt_padmux[3] = readl(JUPITER_MUXPAD_3);
	crt_padmux[4] = readl(JUPITER_MUXPAD_4);
	crt_padmux[5] = readl(JUPITER_MUXPAD_5);
	crt_padmux[6] = readl(JUPITER_MUXPAD_6);
	crt_padmux[7] = readl(JUPITER_MUXPAD_7);
	crt_padmux[8] = readl(JUPITER_MUXPAD_8);
	crt_padmux[9] = readl(JUPITER_MUXPAD_9);
	crt_padmux[10] = readl(JUPITER_MUXPAD_10);
	crt_padmux[11] = readl(JUPITER_MUXPAD_11);
	crt_padmux[12] = readl(JUPITER_MUXPAD_12);

	// save GPIO
	mis_gpio_dir[0] = inl(MARS_MIS_GP0DIR);
	mis_gpio_dir[1] = inl(MARS_MIS_GP1DIR);
	mis_gpio_dir[2] = inl(MARS_MIS_GP2DIR);
	mis_gpio_dir[3] = inl(MARS_MIS_GP3DIR);
	mis_gpio_out[0] = inl(MARS_MIS_GP0DATO);
	mis_gpio_out[1] = inl(MARS_MIS_GP1DATO);
	mis_gpio_out[2] = inl(MARS_MIS_GP2DATO);
	mis_gpio_out[3] = inl(MARS_MIS_GP3DATO);
	mis_gpio_ie[0]  = inl(MARS_MIS_GP0IE);
	mis_gpio_ie[1]  = inl(MARS_MIS_GP1IE);
	mis_gpio_ie[2]  = inl(MARS_MIS_GP2IE);
	mis_gpio_ie[3]  = inl(MARS_MIS_GP3IE);
	mis_gpio_dp[0]  = inl(MARS_MIS_GP0DP);
	mis_gpio_dp[1]  = inl(MARS_MIS_GP1DP);
	mis_gpio_dp[2]  = inl(MARS_MIS_GP2DP);
	mis_gpio_dp[3]  = inl(MARS_MIS_GP3DP);

/*
	// print msg for debug use...
	prom_printf("===> MIS_TC0TVR:%X, MIS_TC0CVR:%X, MIS_TC0CR:%X, MIS_TC0ICR:%X\n", inl(VENUS_MIS_TC0TVR), inl(VENUS_MIS_TC0CVR), inl(VENUS_MIS_TC0CR), inl(VENUS_MIS_TC0ICR));
	prom_printf("===> MIS_TC1TVR:%X, MIS_TC1CVR:%X, MIS_TC1CR:%X, MIS_TC1ICR:%X\n", inl(VENUS_MIS_TC1TVR), inl(VENUS_MIS_TC1CVR), inl(VENUS_MIS_TC1CR), inl(VENUS_MIS_TC1ICR));
	prom_printf("===> MIS_TC2TVR:%X, MIS_TC2CVR:%X, MIS_TC2CR:%X, MIS_TC2ICR:%X\n", inl(VENUS_MIS_TC2TVR), inl(VENUS_MIS_TC2CVR), inl(VENUS_MIS_TC2CR), inl(VENUS_MIS_TC2ICR));
	prom_printf("===> MIS_TCWCR:%X, MIS_TCWTR:%X, MIS_CLK90K_CTRL:%X\n", inl(VENUS_MIS_TCWCR), inl(VENUS_MIS_TCWTR), inl(VENUS_MIS_CLK27M_CLK90K_CNT));
*/
}

static void jupiter_restore_state(
		unsigned int *mis_tc0,
		unsigned int *mis_tc1,
		unsigned int *mis_tc2,
		unsigned int *crt_padmux,
		unsigned int *mis_gpio_dir,
		unsigned int *mis_gpio_out,
		unsigned int *mis_gpio_ie,
		unsigned int *mis_gpio_dp)
{
	// restore timer registers after return from suspend
	outl(mis_tc0[0], VENUS_MIS_TC0CR);
	outl(mis_tc0[1], VENUS_MIS_TC0ICR);
	outl(mis_tc0[2], VENUS_MIS_TC0TVR);
	outl(mis_tc1[0], VENUS_MIS_TC1CR);
	outl(mis_tc1[1], VENUS_MIS_TC1ICR);
	outl(mis_tc1[2], VENUS_MIS_TC1TVR);
	outl(mis_tc2[0], VENUS_MIS_TC2CR);
	outl(mis_tc2[1], VENUS_MIS_TC2ICR);
	outl(mis_tc2[2], VENUS_MIS_TC2TVR);

	// restore pad mux
	writel(crt_padmux[0],  JUPITER_MUXPAD_0);
	writel(crt_padmux[1],  JUPITER_MUXPAD_1);
	writel(crt_padmux[2],  JUPITER_MUXPAD_2);
	writel(crt_padmux[3],  JUPITER_MUXPAD_3);
	writel(crt_padmux[4],  JUPITER_MUXPAD_4);
	writel(crt_padmux[5],  JUPITER_MUXPAD_5);
	writel(crt_padmux[6],  JUPITER_MUXPAD_6);
	writel(crt_padmux[7],  JUPITER_MUXPAD_7);
	writel(crt_padmux[8],  JUPITER_MUXPAD_8);
	writel(crt_padmux[9],  JUPITER_MUXPAD_9);
	writel(crt_padmux[10], JUPITER_MUXPAD_10);
	writel(crt_padmux[11], JUPITER_MUXPAD_11);
	writel(crt_padmux[12], JUPITER_MUXPAD_12);

	// restore GPIO
	outl(mis_gpio_dir[0], MARS_MIS_GP0DIR);
	outl(mis_gpio_dir[1], MARS_MIS_GP1DIR);
	outl(mis_gpio_dir[2], MARS_MIS_GP2DIR);
	outl(mis_gpio_dir[3], MARS_MIS_GP3DIR);
	outl(mis_gpio_out[0], MARS_MIS_GP0DATO);
	outl(mis_gpio_out[1], MARS_MIS_GP1DATO);
	outl(mis_gpio_out[2], MARS_MIS_GP2DATO);
	outl(mis_gpio_out[3], MARS_MIS_GP3DATO);
	outl(mis_gpio_ie[0] , MARS_MIS_GP0IE);
	outl(mis_gpio_ie[1] , MARS_MIS_GP1IE);
	outl(mis_gpio_ie[2] , MARS_MIS_GP2IE);
	outl(mis_gpio_ie[3] , MARS_MIS_GP3IE);
	outl(mis_gpio_dp[0] , MARS_MIS_GP0DP);
	outl(mis_gpio_dp[1] , MARS_MIS_GP1DP);
	outl(mis_gpio_dp[2] , MARS_MIS_GP2DP);
	outl(mis_gpio_dp[3] , MARS_MIS_GP3DP);

/*
	// print msg for debug use...
	prom_printf("===> MIS_TC0TVR:%X, MIS_TC0CVR:%X, MIS_TC0CR:%X, MIS_TC0ICR:%X\n", inl(VENUS_MIS_TC0TVR), inl(VENUS_MIS_TC0CVR), inl(VENUS_MIS_TC0CR), inl(VENUS_MIS_TC0ICR));
	prom_printf("===> MIS_TC1TVR:%X, MIS_TC1CVR:%X, MIS_TC1CR:%X, MIS_TC1ICR:%X\n", inl(VENUS_MIS_TC1TVR), inl(VENUS_MIS_TC1CVR), inl(VENUS_MIS_TC1CR), inl(VENUS_MIS_TC1ICR));
	prom_printf("===> MIS_TC2TVR:%X, MIS_TC2CVR:%X, MIS_TC2CR:%X, MIS_TC2ICR:%X\n", inl(VENUS_MIS_TC2TVR), inl(VENUS_MIS_TC2CVR), inl(VENUS_MIS_TC2CR), inl(VENUS_MIS_TC2ICR));
	prom_printf("===> MIS_TCWCR:%X, MIS_TCWTR:%X, MIS_CLK90K_CTRL:%X\n", inl(VENUS_MIS_TCWCR), inl(VENUS_MIS_TCWTR), inl(VENUS_MIS_CLK27M_CLK90K_CNT));
*/
}


/*
 *	venus_pm_enter - Actually enter a sleep state.
 *	@state:		State we're entering.
 *
 */
static int venus_pm_enter(suspend_state_t state)
{
	int value = 0;
	char *value_ptr, *value_end_ptr;
	unsigned int options=0, hwinfo=0, powerkey_irrp, ejectkey_irrp;
	int voltage_gpio, powerkey_gpio, ejectkey_gpio, vfd_type;

	switch (state)
	{
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
#if 1
		// This is to check RTC time and Alarm time when schedule record is not waken up at right time
		prom_printf("\nRTC time: %x %x %x %x %x\nAlarm time: %x %x %x %x\n", 
			inl(VENUS_MIS_RTCDATE2), inl(VENUS_MIS_RTCDATE1), 
			inl(VENUS_MIS_RTCHR), inl(VENUS_MIS_RTCMIN), 
			inl(VENUS_MIS_RTCSEC), 
			inl(VENUS_MIS_ALMDATE2), inl(VENUS_MIS_ALMDATE1), 
			inl(VENUS_MIS_ALMHR), inl(VENUS_MIS_ALMMIN));
#endif
#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
		value = inl(VENUS_MIS_TC2CR);
		value |= 0x20000000;
		outl(value, VENUS_MIS_TC2CR);		/* Disable external timer interrupt */
#endif
		value = inl(VENUS_MIS_RTCCR);
		value |= 0x1;
		outl(value, VENUS_MIS_RTCCR);		/* Enable RTC's half-second interrupt */
/* Here are 2 methods for power saving. One is polling that is to avoid Venus's bug of edge trigger interrupt in power saving mode. The other is interrupt mode. */
/* In fact, IrDA is still running in polling mode when using interrupt mode to wake up system. */
#ifdef CONFIG_PM_SLEEP_POLLING
/* We will here close RTC's alarm interrupt that is useless in polling mode */
		value = inl(VENUS_MIS_RTCCR);
		if(value&0x10) {
			value &= ~0x10;
			outl(value, VENUS_MIS_RTCCR);		/* RTC alarm should be checked by polling */
		} else {
			outl(0, VENUS_MIS_ALMMIN);
			outl(0, VENUS_MIS_ALMHR);
			outl(0, VENUS_MIS_ALMDATE1);
			outl(0, VENUS_MIS_ALMDATE2);
		}
#endif
		/* Clear VFD, IrDA, and all RTC's interrupts */
		outl(0x7FE20, VENUS_MIS_ISR);
		if (is_jupiter_cpu())
			outl(0x1FC020, OFFSET_JUPITER_ISO_ISR);

		if (is_jupiter_cpu()) {
			while(inl(OFFSET_JUPITER_ISO_IR_SR) & 0x01) {
				inl(OFFSET_JUPITER_ISO_IR_RP);
				outl(0x1, OFFSET_JUPITER_ISO_IR_SR);
			}
		}
		else {
			while(inl(VENUS_MIS_IR_SR) & 0x01) {
				inl(VENUS_MIS_IR_RP);
				outl(0x1, VENUS_MIS_IR_SR);
			}
		}

		value_ptr = parse_token(platform_info.system_parameters, "12V5V_GPIO");
		if(value_ptr && strlen(value_ptr)) {
			voltage_gpio = (int)simple_strtol(value_ptr, &value_end_ptr, 10);
			if(value_ptr == value_end_ptr) {
				prom_printf("The value of 12V5V_GPIO seems to have problem.\n");
				voltage_gpio = -1;
			} else if(voltage_gpio < 0 || (is_venus_cpu() && voltage_gpio > 35) || (is_neptune_cpu() && voltage_gpio > 47) || (is_mars_cpu() && voltage_gpio > 105)) {
				prom_printf("GPIO number setting exceeds the limits.\n");
				voltage_gpio = -1;
			} else {
				if(!strcmp(value_end_ptr, ",hion"))
					voltage_gpio |= 0x10000000;
				else if(strcmp(value_end_ptr, ",hioff")) {
					prom_printf("The value of 12V5V_GPIO doesn't have a sub-parameter of \"hion\" or \"hioff\"\n");
					voltage_gpio = -1;
				}
			}
		} else {
			prom_printf("No 12V5V_GPIO setting in system_parameters. If possible, use board_id instead.\n");
			if(platform_info.board_id == 0x1 || platform_info.board_id == 0x5)
				voltage_gpio = 33 | 0x10000000;
			else if(platform_info.board_id == 0xa || platform_info.board_id == 0xb || platform_info.board_id == 0x5000a)
				voltage_gpio = 34 | 0x10000000;
			else if(platform_info.board_id == 0x4)
				voltage_gpio = 12 | 0x10000000;
			else
				voltage_gpio = -1;
		}
		if(value_ptr)
			kfree(value_ptr);
		prom_printf("12V5V_GPIO setting=%x\n", voltage_gpio);

/* For Venus and Neptune CPU, if bit0 of 0xb8008800 is equal to 1, it represents that the package of the chip is QFP, otherwise it is BGA. */
/* For Mars, the register for this is 0xb8000300 */
/* Bit 4 of hwinfo represents package type */
		if(((is_venus_cpu() || is_neptune_cpu()) && ((*(volatile unsigned int *)0xb8008800) & 0x1)) || 
			(is_mars_cpu() && ~((*(volatile unsigned int *)0xb8000300) & 0x8000)))
			;
		else
			hwinfo |= 0x10;
		if(suspend_options == 1)
			options |= 0x1;
		prom_printf("options=%x\n",options);

/* Bit 0-3 of hwinfo represent CPU type */
		if(is_venus_cpu())
			;
		else if(is_neptune_cpu())
			hwinfo |= 0x1;
		else if(is_mars_cpu())
			hwinfo |= 0x2;
		else if (is_jupiter_cpu())
			hwinfo |= 0x3;
		else
			hwinfo = -1;
		if(hwinfo>=0)
			prom_printf("hwinfo=%x\n",hwinfo);
		else
			prom_printf("Unknown CPU type.\n");
		
		//POWERKEY_IRRP
		value_ptr = parse_token(platform_info.system_parameters, "POWERKEY_IRRP");
		if(value_ptr && strlen(value_ptr)) {
			powerkey_irrp = (int)simple_strtol(value_ptr, &value_end_ptr, 16);
			if(value_ptr == value_end_ptr) {
				prom_printf("The value of POWERKEY_IRRP seems to have problem.\n");
				powerkey_irrp = 0;
			}
		} else {
			powerkey_irrp = 0xFF00FC03;
		}
		prom_printf("powerkey_irrp=%x\n",powerkey_irrp);
		if(value_ptr)
			kfree(value_ptr);
		
		//EJECTKEY_IRRP
		value_ptr = parse_token(platform_info.system_parameters, "EJECTKEY_IRRP");
		if(value_ptr && strlen(value_ptr)) {
			ejectkey_irrp = (int)simple_strtol(value_ptr, &value_end_ptr, 16);
			if(value_ptr == value_end_ptr) {
				prom_printf("The value of EJECTKEY_IRRP seems to have problem.\n");
				ejectkey_irrp = 0;
			}
		} else {
			ejectkey_irrp = 0xE817FC03;
		}
		prom_printf("ejectkey_irrp=%x\n",ejectkey_irrp);
		if(value_ptr)
			kfree(value_ptr);

		//POWERKEY_GPIO
		value_ptr = parse_token(platform_info.system_parameters, "POWERKEY_GPIO");
		if(value_ptr && strlen(value_ptr)) {
			powerkey_gpio = (int)simple_strtol(value_ptr, &value_end_ptr, 10);
			if(value_ptr == value_end_ptr) {
				prom_printf("The value of POWERKEY_GPIO seems to have problem.\n");
				powerkey_gpio = -1;
			} else if(powerkey_gpio < 0 || (is_venus_cpu() && powerkey_gpio > 35) || (is_neptune_cpu() && powerkey_gpio > 47) || (is_mars_cpu() && powerkey_gpio > 105) || (is_jupiter_cpu() && powerkey_gpio > 9)) {
				prom_printf("GPIO number of POWERKEY_GPIO setting exceeds the limits.\n");
				powerkey_gpio = -1;
			}
		} else {
			powerkey_gpio = -1;
		}
		prom_printf("powerkey_gpio=%d\n",powerkey_gpio);
		if(value_ptr)
			kfree(value_ptr);
		
		//EJECTKEY_GPIO
		value_ptr = parse_token(platform_info.system_parameters, "EJECTKEY_GPIO");
		if(value_ptr && strlen(value_ptr)) {
			ejectkey_gpio = (int)simple_strtol(value_ptr, &value_end_ptr, 10);
			if(value_ptr == value_end_ptr) {
				prom_printf("The value of EJECTKEY_GPIO seems to have problem.\n");
				ejectkey_gpio = -1;
			} else if(ejectkey_gpio < 0 || (is_venus_cpu() && ejectkey_gpio > 35) || (is_neptune_cpu() && ejectkey_gpio > 47) || (is_mars_cpu() && ejectkey_gpio > 105) || (is_jupiter_cpu() && powerkey_gpio > 9)) {
				prom_printf("GPIO number of EJECTKEY_GPIO setting exceeds the limits.\n");
				ejectkey_gpio = -1;
			}
		} else {
			ejectkey_gpio = -1;
		}
		prom_printf("ejectkey_gpio=%d\n",ejectkey_gpio);
		if(value_ptr)
			kfree(value_ptr);

		//VFD_TYPE
		value_ptr = parse_token(platform_info.system_parameters, "VFD_TYPE");
		if(value_ptr && strlen(value_ptr)) {
			if(!strcmp(value_ptr, "NEC"))
				vfd_type = 1;
			else if(!strcmp(value_ptr, "RC5"))
				vfd_type = 2;
			else if(!strcmp(value_ptr, "SHARP"))
				vfd_type = 3;
			else if(!strcmp(value_ptr, "SONY"))
				vfd_type = 4;
			else if(!strcmp(value_ptr, "C03"))
				vfd_type = 5;
			else if(!strcmp(value_ptr, "RC6"))
				vfd_type = 6;
			else
				vfd_type = 0;
		} else {
			vfd_type = 0;
		}
		prom_printf("vfd_type=%d\n",vfd_type);
		if(value_ptr)
			kfree(value_ptr);

#ifndef CONFIG_PM_SLEEP_POLLING
// To simulate the boards that have no RTC
//		outl(inl(VENUS_MIS_RTCCR)&~0x1, VENUS_MIS_RTCCR);
		if (is_jupiter_cpu())
			outl(inl(OFFSET_JUPITER_ISO_IR_CR) | 0x400, OFFSET_JUPITER_ISO_IR_CR);
		else
			outl(inl(VENUS_MIS_IR_CR) | 0x400, VENUS_MIS_IR_CR);

		int gpio_num, reg_adr, gpie_base;

		if(is_mars_cpu())
			gpie_base = MARS_MIS_GP0IE;
		else if (is_jupiter_cpu())
			gpie_base = OFFSET_JUPITER_ISO_GPIE;
		else
			gpie_base = VENUS_MIS_GP0IE;
		
		gpio_num = powerkey_gpio;
		reg_adr = gpie_base;
		while(gpio_num >= 0) {
			if(gpio_num < 32)
				outl(inl(reg_adr)|(1<<gpio_num), reg_adr);
			gpio_num -= 32;
			reg_adr += 4;
		}

		gpio_num = ejectkey_gpio;
		reg_adr = gpie_base;
		while(gpio_num >= 0) {
			if(gpio_num < 32)
				outl(inl(reg_adr)|(1<<gpio_num), reg_adr);
			gpio_num -= 32;
			reg_adr += 4;
		}
#endif

		if(is_mars_cpu()) {
/* For Mars CPU, UART1 is assumed to be useless. Here the mux will be set to be GPIO, and the direct of GPIO pins would be input. This is to save power while in power saving mode. */
			unsigned int uart1mux, gpiodir, regtmp;

			regtmp = *(volatile unsigned int*)0xb800035c;
			uart1mux = regtmp&0xc;
			*(volatile unsigned int*)0xb800035c = (regtmp&~0xc) | 0x4;
			regtmp = inl(VENUS_MIS_GP0DIR);
			gpiodir = regtmp&0xc;
			outl(regtmp&~0xc, VENUS_MIS_GP0DIR);
			
			Suspend_ret = mars_cpu_suspend(platform_info.board_id, voltage_gpio, options, hwinfo, 
					powerkey_irrp, ejectkey_irrp, powerkey_gpio, ejectkey_gpio, vfd_type);
#ifdef CONFIG_REALTEK_CPU_MARS
			setup_boot_image_mars();
#endif

			outl((inl(VENUS_MIS_GP0DIR)&~0xc)|gpiodir, VENUS_MIS_GP0DIR);
			*(volatile unsigned int*)0xb800035c = (*(volatile unsigned int*)0xb800035c&~0xc) | uart1mux;
		} else if (is_jupiter_cpu()) {
			unsigned int tc0_reg[3], tc1_reg[3], tc2_reg[3];
			unsigned int padmux[13], gpio_dir[4], gpio_out[4], gpio_ie[4], gpio_dp[4];

			prom_printf("===> before enter jupiter_cpu_suspend\n");
			// save timer registers before enter suspend
			jupiter_save_state(tc0_reg, tc1_reg, tc2_reg, padmux, gpio_dir, gpio_out, gpio_ie, gpio_dp);

			Suspend_ret = jupiter_cpu_suspend(platform_info.board_id, voltage_gpio, options, hwinfo, 
					powerkey_irrp, ejectkey_irrp, powerkey_gpio, ejectkey_gpio, vfd_type);

			prom_printf("===> after leave jupiter_cpu_suspend: %d\n", Suspend_ret);
			// restore timer registers after return from suspend
			jupiter_restore_state(tc0_reg, tc1_reg, tc2_reg, padmux, gpio_dir, gpio_out, gpio_ie, gpio_dp);
			//System CPU invalid access interrupt enable.
			outl(0x3,VENUS_SB2_INV_INTEN);

#ifdef CONFIG_REALTEK_CPU_JUPITER
			setup_boot_image_jupiter();
#endif
		} else {
			Suspend_ret = venus_cpu_suspend(platform_info.board_id, voltage_gpio, options, hwinfo, 
					powerkey_irrp, ejectkey_irrp, powerkey_gpio, ejectkey_gpio, vfd_type);
#if CONFIG_REALTEK_CHIP_VENUS || CONFIG_REALTEK_CPU_NEPTUNE
			setup_boot_image();
#endif
		}

#ifndef CONFIG_PM_SLEEP_POLLING
		if (is_jupiter_cpu())
			outl(inl(OFFSET_JUPITER_ISO_IR_CR) & ~0x400, OFFSET_JUPITER_ISO_IR_CR);
		else
			outl(inl(VENUS_MIS_IR_CR) & ~0x400, VENUS_MIS_IR_CR);

		gpio_num = powerkey_gpio;
		reg_adr = gpie_base;
		while(gpio_num >= 0) {
			if(gpio_num < 32)
				outl(inl(reg_adr)&~(1<<gpio_num), reg_adr);
			gpio_num -= 32;
			reg_adr += 4;
		}

		gpio_num = ejectkey_gpio;
		reg_adr = gpie_base;
		while(gpio_num >= 0) {
			if(gpio_num < 32)
				outl(inl(reg_adr)&~(1<<gpio_num), reg_adr);
			gpio_num -= 32;
			reg_adr += 4;
		}
#endif
		value = inl(VENUS_MIS_RTCCR);
		value &= ~0x1;
		outl(value, VENUS_MIS_RTCCR);

		outl(0x7FE20, VENUS_MIS_ISR);
		if (is_jupiter_cpu()) {
			outl(0x1FC020, OFFSET_JUPITER_ISO_ISR);
#ifdef CONFIG_REALTEK_PCI_2883
			jupiter_pcie_rc_init();     // reinit PCI-E RC
#endif
		}
#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT
		value = inl(VENUS_MIS_TC2CR);
		value &= ~0x20000000;
		outl(value, VENUS_MIS_TC2CR);		/* Enable external timer interrupt */
#endif
#if 1
		// This is to check RTC time and Alarm time when schedule record is not waken up at right time
		prom_printf("\nRTC time: %x %x %x %x %x\nAlarm time: %x %x %x %x\n", 
			inl(VENUS_MIS_RTCDATE2), inl(VENUS_MIS_RTCDATE1), 
			inl(VENUS_MIS_RTCHR), inl(VENUS_MIS_RTCMIN), 
			inl(VENUS_MIS_RTCSEC), 
			inl(VENUS_MIS_ALMDATE2), inl(VENUS_MIS_ALMDATE1), 
			inl(VENUS_MIS_ALMHR), inl(VENUS_MIS_ALMMIN));
#endif
		break;
	case PM_SUSPEND_DISK:
		return -ENOTSUPP;
	default:
		return -EINVAL;
	}

	return 0;
}

/*
 *	venus_pm_prepare - Do preliminary suspend work.
 *	@state:		suspend state we're entering.
 *
 */
static int venus_pm_prepare(suspend_state_t state)
{
	switch (state)
	{
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		break;

	case PM_SUSPEND_DISK:
		return -ENOTSUPP;

	default:
		return -EINVAL;
	}
	return 0;
}

extern atomic_t rtk_rtc_found;

/**
 *	venus_pm_finish - Finish up suspend sequence.
 *	@state:		State we're coming out of.
 *
 *	This is called after we wake back up (or if entering the sleep state
 *	failed).
 */
static int venus_pm_finish(suspend_state_t state)
{
	struct timespec tv;
	switch (state)
	{
	case PM_SUSPEND_STANDBY:
	case PM_SUSPEND_MEM:
		if(atomic_read(&rtk_rtc_found)>0) {
			tv.tv_sec = rtc_get_time();
			tv.tv_nsec = 0;
			do_settimeofday(&tv);
		}
		if (dvr_task != 0) {
			if(Suspend_ret==2)
				send_sig_info(SIGUSR_RTC_ALARM, (void *)2, (struct task_struct *)dvr_task);
			else if(Suspend_ret==1)
				send_sig_info(SIGUSR2, (void *)2, (struct task_struct *)dvr_task);
			else
				send_sig_info(SIGUSR1, (void *)2, (struct task_struct *)dvr_task);
		} else {
			printk("error condition, wrong dvr_task value...\n");
		} 

		break;

	case PM_SUSPEND_DISK:
		return -ENOTSUPP;

	default:
		return -EINVAL;
	}
	return 0;
}

/*
 * Set to PM_DISK_FIRMWARE so we can quickly veto suspend-to-disk.
 */
static struct pm_ops venus_pm_ops = {
	.pm_disk_mode	= PM_DISK_FIRMWARE,
	.prepare	= venus_pm_prepare,
	.enter		= venus_pm_enter,
	.finish		= venus_pm_finish,
};

static int __init venus_logo_init(void)
{
	printk("========== board id: %x ==========\n", platform_info.board_id);

/*
	if ((platform_info.board_id == realtek_avhdd_demo_board) || (platform_info.board_id == C02_avhdd_board)
	 || (platform_info.board_id == realtek_pvrbox_demo_board) || (platform_info.board_id == C05_pvrbox_board))
		strptr = (char *)LOGO_INFO_ADDR2;
	else
		strptr = (char *)LOGO_INFO_ADDR1;
*/
	if (is_venus_cpu()) {
		char *strptr;
		int count = LOGO_INFO_SIZE;

		switch(platform_info.board_id) {
		case realtek_1071_avhdd_mk_demo_board:
		case C10_1071_avhdd_board:
			goto neptune_mode;
			break;
		case realtek_avhdd_demo_board:
		case realtek_avhdd2_demo_board:
		case C02_avhdd_board:
		case realtek_pvrbox_demo_board:
		case C05_pvrbox_board:
		case C05_pvrbox2_board:
		case C07_avhdd_board:
		case C07_pvrbox_board:
		case C07_pvrbox2_board:
		case C09_pvrbox_board:
		case C09_pvrbox2_board:
		case C03_pvr2_board:
			strptr = (char *)LOGO_INFO_ADDR2;
			break;
		default:
			strptr = (char *)LOGO_INFO_ADDR1;
			break;
		}
	
	        while (--count > 0) {
	                if (!memcmp(strptr, "-l", 2))
				break;
			strptr++;
	        }
//		printk("strptr value: %s \n", strptr);
		if (count != 0) {
			strptr += 3;
			sscanf(strptr, "%d %d %x %x %x %x", &logo_info.mode, &logo_info.size,
			&logo_info.color[0], &logo_info.color[1], &logo_info.color[2], &logo_info.color[3]);
		} else {
			printk("[INFO] logo info not found, use PAL as default...\n");
			logo_info.mode = 1;	// PAL
			logo_info.size = 2724;
			logo_info.color[0] = 0x6ba53f;
			logo_info.color[1] = 0x6da555;
			logo_info.color[2] = 0x749889;
			logo_info.color[3] = 0x8080eb;
		}
	} else {
		boot_extern_param *boot_param;

neptune_mode:
		printk("[INFO] neptune mode...\n");
		if (is_venus_cpu() || is_neptune_cpu()) 
			boot_param = (boot_extern_param *)BOOT_PARAM_ADDR_NEPTUNE;
		else
			boot_param = (boot_extern_param *)BOOT_PARAM_ADDR_MARS;
		printk("boot_param value: %x \n", boot_param);
		logo_info.mode = boot_param->logo_type & 0xffff; 
		logo_info.size = boot_param->logo_offset; 
		logo_info.cvbs = boot_param->logo_type >> 31; 
		logo_info.color[0] = boot_param->logo_reg_5370;
		logo_info.color[1] = boot_param->logo_reg_5374;
		logo_info.color[2] = boot_param->logo_reg_5378;
		logo_info.color[3] = boot_param->logo_reg_537c;
	}
	printk("mode: %d \n", logo_info.mode);
	printk("size: %d \n", logo_info.size);
	printk("color1: 0x%x \n", logo_info.color[0]);
	printk("color2: 0x%x \n", logo_info.color[1]);
	printk("color3: 0x%x \n", logo_info.color[2]);
	printk("color4: 0x%x \n", logo_info.color[3]);
}

static int __init venus_pm_init(void)
{
	printk("Realtek Venus Power Management, (c) 2006 Realtek Semiconductor Corp.\n");

	pm_set_ops(&venus_pm_ops);
	return 0;
}

core_initcall(venus_logo_init);
late_initcall(venus_pm_init);
