/*
 * Jupiter Assembler Sleep/WakeUp Management Routines
 *
 * Copyright (c) 2006 Colin <colin@realtek.com.tw>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License.
 *
 * History:
 *
 * 2008-12-30:	Colin	First version.
 */

//#include <linux/linkage.h>
//#include <asm/assembler.h>
//#include <asm/hardware.h>
#include <asm/asm.h>
#include <asm/mipsregs.h>
#include <asm/regdef.h>
#include <asm/stackframe.h>
#include <mars.h>
#include <jupiter.h>
#include <linux/serial_reg.h>

#define BISR_DONE_FLAG_ADDR	0xa0000ffc	/* flag indicate BISR done */

#define OPTION_SLEEP_PRINT			/* comment this line to print less message */

/* Following defines are for verification, DO NOT enable them. */
//#define DEBUG_OUTPUT_ISO_GPIO			/* output ISO GPIO[5:4] as debug message */
/* #define DEBUG_SKIP_CLOCK */
/* #define DEBUG_SKIP_PLL */
/* #define DEBUG_SKIP_ISO */
/* #define DEBUG_NOT_TURN_OFF_CPU */
/* #define DEBUG_SKIP_DCU_CALIBRATION */
/* #define DEBUG_SKIP_DDR_SELF_REFRESH */
/* #define DEBUG_SKIP_BISR */
/* #define DEBUG_USE_SEPERATE_STACK */

/* #define DEBUG_TOGGLE_CEC */
/* #define DEBUG_TOGGLE_GPIO */


#define __SLEEP		.section	".sleep.text","ax"
#define __SLEEPDATA	.section	".sleep.data","aw"

#ifdef DEBUG_USE_SEPERATE_STACK
	.bss
	.balign 32
	.globl	_test_stack_base
_test_stack_base:
	.rept 1024
	.byte 0
	.endr

	.globl	_test_stack 
_test_stack:
	.rept 1024
	.byte 0
	.endr
#endif

#define CACHE_OP( code, type )			( ((code) << 2) | (type) )
#define ICACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 0)
#define DCACHE_INDEX_STORE_TAG			CACHE_OP(0x2, 1)
#define ICACHE_ADDR_FILL			CACHE_OP(0x5, 0)
#define DCACHE_ADDR_FETCH			CACHE_OP(0x7, 1)
#define DCACHE_ADDR_WRITEBACK			CACHE_OP(0x6, 1)
#define DCACHE_ADDR_WRITEBACK_INVALIDATE	CACHE_OP(0x5, 1)
#define DCACHE_ADDR_INVALIDATE			CACHE_OP(0x4, 1)
#define DCACHE_INDEX_LOAD_TAG			CACHE_OP(0x1, 1)


#define FRAME_SIZE		200		/* stack size */
#define OFF_PARAM_5		(FRAME_SIZE+16)
#define OFF_PARAM_6		(FRAME_SIZE+20)
#define OFF_PARAM_7		(FRAME_SIZE+24)
#define OFF_PARAM_8		(FRAME_SIZE+28)
#define OFF_PARAM_9		(FRAME_SIZE+32)

/* define sp offset to store hardware register value */
#define OFF_ACPU_MN		36
#define OFF_VCPU_MN		40
#define OFF_SD_MN		44
#define OFF_HD_MN		48
#define OFF_BUS_MN		52
#define OFF_SCPU_MN		56
#define OFF_DDR_MN		60
#define OFF_DCU_1		64
#define OFF_DCU_2		68
#define OFF_DCU_3		72
#define OFF_DCU_4		76
#define OFF_DCU_5		80
#define OFF_DCU_6		84
#define OFF_DCU_7		88
#define OFF_DCU_8		92
#define OFF_DCU_9		96
#define OFF_DCU_10		100
#define OFF_DCU_11		104
#define OFF_DCU_12		108
#define OFF_DCU_13		112
#define OFF_DCU_14		116
#define OFF_DCU_15		120
#define OFF_DCU_16		124
#define OFF_DCU_17		128
#define OFF_DCU_18		132
#define OFF_DCU_19		136
#define OFF_DCU_20		140
/* leave some space here for future usage... */

#define OFF_DCU_CALI_PARAM	148	/* calibration parameter from bootcode */
#define OFF_CRT_SOFT_RESET1	152
#define OFF_CRT_SOFT_RESET2	156
#define OFF_CRT_CLK_ENABLE1	160
#define OFF_CRT_CLK_ENABLE2	164
#define OFF_DCU_CALI_RESULT	168	/* current calibration result */
#define OFF_BISR_FLAG_TMP	172
#define OFF_BISR_STATUS		176
#define OFF_BOOTCODE_OPTION	180 /* bootcode option */
#define OFF_DCU_CALI_SAVE1	184 /* original value in DDR (0xa0000000) */
#define OFF_DCU_CALI_SAVE2	188 /* original value in DDR (0xa0000008) */

	.text
	.set noreorder
	__SLEEP
/*
 * jupiter_cpu_suspend()
 *
 * Make jupiter enter sleep mode
 *
 * Parameters:
 *   a0 stores board_id
 *   a1 stores 12V/5V GPIO parameter. If it is -1, it means that there is no value for 12V/5V GPIO parameter. When the bit 28 of a1 is 1, it represents that 12V/5V will be on when 12V/5V GPIO is high. Bits 0-27 store the GPIO number of 12V/5V, and the value of it will locate between 0 and 35.
 *   a2 stores options:
 *       bit0: 0 represents normal resuming procedure. 1 means we want to use "reboot" to replace normal resuming procedure.
 *   a3 stores HW info:
 *       bit0-3: CPU type. 0 represents Venus, 1 represents Neptune, and 2 represents Mars.
 *       bit4: Package type. 0 represents QFP, and 1 represents BGA.
 *   Parameter5, at OFF_PARAM_5(sp), stores the IRRP value of IR power key:
 *   Parameter6, at OFF_PARAM_6(sp), stores the IRRP value of IR eject key:
 *   Parameter7, at OFF_PARAM_7(sp), stores the GPIO pin num of GPIO power key:
 *   Parameter8, at OFF_PARAM_8(sp), stores the GPIO pin num of GPIO eject key:
 *   Parameter9, at OFF_PARAM_9(sp), stores the IR type:
 *
 * These registers are used for these purposes in the whole function:
 *   s0 is used to save base address
 *   s3 is used to save HW info
 *   s4 is used to save options
 *   s5 is used to save 12V/5V GPIO parameter
 *   s6 is used to save return value
 *   s7 is used to save board_id
 * 
 * Return 2 to represent that it is woken up by RTC alarm, 1 to represent that cdrom should be ejected, and 0 to represent that power on key is pressed.
 */
NESTED(jupiter_cpu_suspend, 0, sp)
#ifdef DEBUG_USE_SEPERATE_STACK
	la	k0, _test_stack_base
	sw	sp, 0(k0)		# save kernel stack
	la	sp, _test_stack		# use pre-allocated buffer as stack
#endif
	addiu	sp, sp, -FRAME_SIZE
	sw	s0, 0(sp)
	sw	s1, 4(sp)
	sw	s2, 8(sp)
	sw	s3, 12(sp)
	sw	s4, 16(sp)
	sw	s5, 20(sp)
	sw	s6, 24(sp)
	sw	s7, 28(sp)
	sw	ra, 32(sp)

	move	s7, a0		/* board_id */
	move	s5, a1		/* 12V/5V GPIO pin number */
	move	s4, a2		/* options */
	move	s3, a3		/* HW info */



/*********************************************************************************/
/* For DRAM non-selfrefresh mode, putting code and data in cache is also necessary. If no doing this, system will halt on "turn on 27M OSC". */
	/* move code to I-cache */
	la	t1, _esleeptext
	addiu	t1, 32
	la	t2, _sleeptext
1:
.set mips3
	cache	ICACHE_ADDR_FILL, 0(t2)
.set mips0
	sltu	t0, t2, t1
	bnez	t0, 1b
	addiu	t2, 32

	/* move data to D-cache */
	la	t1, _esleepdata
	addiu	t1, 32
	la	t2, _sleepdata
2:
.set mips3
	cache	DCACHE_ADDR_FETCH, 0(t2)
.set mips0
	sltu	t0, t2, t1
	bnez	t0, 2b
	addiu	t2, 32

	/* move stack to D-cache */
	li	t1, 256
	move	t2, sp
	addiu	t2, t2, FRAME_SIZE
3:
.set mips3
	cache	DCACHE_ADDR_FETCH, 0(t2)
.set mips0
	addiu	t1, -1
	bnez	t1, 3b
	addiu	t2, -32


#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa
	jal	print_char
	li	a0, 'b'

#ifdef DEBUG_OUTPUT_ISO_GPIO
	/* mux to ISO GPIO[5:3] */
	li	t1, 0xb801bdd8
	lw	t2, 0(t1)
	li	t3, ~0x3f
	and	t2, t3, t2
	sw	t2, 0(t1)

	/* set ISO GPIO[5:4] output direction */
	li	t1, 0xb801bd70
	lw	t2, 0(t1)
	ori	t2, t2, 0x30
	sw	t2, 0(t1)
#endif
#endif
	/* save CRT reset & clock setting */
	lw	t1, 0xb8000000
	sw	t1, OFF_CRT_SOFT_RESET1(sp)
	lw	t1, 0xb8000004
	sw	t1, OFF_CRT_SOFT_RESET2(sp)
	lw	t1, 0xb800000c
	sw	t1, OFF_CRT_CLK_ENABLE1(sp)
	lw	t1, 0xb8000010
	sw	t1, OFF_CRT_CLK_ENABLE2(sp)

	/* save DCU setting */
	lw	t1, 0xb8000030
	sw	t1, OFF_DCU_1(sp)
	lw	t1, 0xb8000154
	sw	t1, OFF_DCU_2(sp)
	lw	t1, 0xb8008004
	sw	t1, OFF_DCU_3(sp)
	lw	t1, 0xb8008800
	sw	t1, OFF_DCU_4(sp)
	lw	t1, 0xb8008808
	sw	t1, OFF_DCU_5(sp)
	lw	t1, 0xb8008818
	sw	t1, OFF_DCU_6(sp)
	lw	t1, 0xb8008828
	sw	t1, OFF_DCU_7(sp)
	lw	t1, 0xb8008880
	sw	t1, OFF_DCU_8(sp)
	lw	t1, 0xb800882c
	sw	t1, OFF_DCU_9(sp)
	lw	t1, 0xb8008844
	sw	t1, OFF_DCU_20(sp)
	lw	t1, 0xb8008848
	sw	t1, OFF_DCU_10(sp)
	lw	t1, 0xb800884c
	sw	t1, OFF_DCU_11(sp)
	lw	t1, 0xb8008850
	sw	t1, OFF_DCU_12(sp)
	lw	t1, 0xb8008904
	sw	t1, OFF_DCU_13(sp)
	lw	t1, 0xb8008908
	sw	t1, OFF_DCU_14(sp)
	lw	t1, 0xb800890c
	sw	t1, OFF_DCU_15(sp)
	lw	t1, 0xb8008910
	sw	t1, OFF_DCU_16(sp)
	lw	t1, 0xb8008930
	sw	t1, OFF_DCU_17(sp)
	lw	t1, 0xb8008914
	sw	t1, OFF_DCU_18(sp)
	lw	t1, 0xb8008920
	sw	t1, OFF_DCU_19(sp)
	lw	t1, 0xb8008880
	andi	t1, t1, 0x3ff
	sw	t1, OFF_DCU_CALI_RESULT(sp)
	lw	t1, 0xb801a618
	sw	t1, OFF_DCU_CALI_PARAM(sp)
	lw	t1, 0xb801a61c
	sw	t1, OFF_BOOTCODE_OPTION(sp)

	/* save original value in DDR */
	lw	t1, 0xa0000000
	sw	t1, OFF_DCU_CALI_SAVE1(sp)
	lw	t1, 0xa0000008
	sw	t1, OFF_DCU_CALI_SAVE2(sp)

#ifndef DEBUG_SKIP_DDR_SELF_REFRESH
	/* DDR goes into self-refresh (DO NOT polling done!)*/
	li	t1, 0x08
	sw	t1, 0xb800880c

	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
#endif

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, 'y'
#endif


	/* Turn off Video DAC A~F */
	li	t1, 0x8208
	li	t2, 0xb8018200
	sw	t1, 0(t2)
	sw	t1, 4(t2)

	/* Turn off Audio DAC */
	li	t1, 0x8a000000
	sw	t1, 0xb8006604

	/* Turn off ETH Phy */
	li	t1, 0x84000800
	sw	t1, 0xb801605c

	/* Turn off PCIE PHY */
	lw	t1, 0xb8000004
	li	t2, ~0x32000
	and	t1, t2, t1		# bit[17,16,13] = 0
	sw	t1, 0xb8000004

	/* Turn off SATA PHY & USB PHY */
	lw	t1, 0xb8000000
	li	t2, ~0x190
	and	t1, t2, t1		# bit[7,6,4] = 0
	sw	t1, 0xb8000000

	/* Turn off HDMI_TMDS */
	lw	t1, 0xb800d040
	li	t2, ~0x1f000		# bit[16:12] = 0
	and	t1, t2, t1
	sw	t1, 0xb800d040

	jal	long_sleep_delay
	nop

#ifdef DEBUG_SKIP_CLOCK
	b	re_run_hw_setting
	nop
#endif

	/* Turn off CRT clock */
	li	t1, 0x1
	sw	t1, 0xb800000c
	li	t1, 0x1e00
	sw	t1, 0xb8000010
	
#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, 'e'
#endif

#ifdef DEBUG_SKIP_PLL
	b	re_run_hw_setting
	nop
#endif


	lui	s0, 0xb800
	li	t1, 0x4

	/* disable ACPU PLL */
	sw	t1, 0x10c(s0)
	lw	t2, 0x108(s0)
	sw	t2, OFF_ACPU_MN(sp)	# save ACPU PLL MN value

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '1'
#endif

	/* disable VCPU PLL */
	sw	t1, 0x114(s0)
	lw	t2, 0x110(s0)
	sw	t2, OFF_VCPU_MN(sp)	# save VCPU PLL MN value

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '2'
#endif

	/* disable SD PLL */
	sw	t1, 0x124(s0)
	lw	t2, 0x118(s0)
	sw	t2, OFF_SD_MN(sp)	# save SD PLL MN value

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '3'
#endif

	/* disable HD PLL */
	sw	t1, 0x134(s0)
	lw	t2, 0x130(s0)
	sw	t2, OFF_HD_MN(sp)	# save HD PLL MN value

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '4'
#endif

	/* disable DDR PLL */
	sw	t1, 0x15c(s0)
	lw	t2, 0x150(s0)
	sw	t2, OFF_DDR_MN(sp)	# save DDR PLL MN value

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '5'
#endif

	/* disable SD2 PLL */
	sw	t1, 0x180(s0)

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '6'
#endif

	/* disable HDMI PLL */
	sw	t1, 0x194(s0)

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '7'
#endif

	/* disable ETN PLL */
	sw	t1, 0x1a4(s0)

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '8'
#endif


	/* Switch SYS clock source to 27MHz OSC */
	li	t1, 0x1
	sw	t1, 0xb8000018
	jal	long_sleep_delay
	nop

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '9'
#endif


	/* BUS PLL */
	li	t1, 0x4
	sw	t1, 0x16c(s0)
	lw	t2, 0x164(s0)
	sw	t2, OFF_BUS_MN(sp)	# save BUS PLL MN value

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '0'
#endif

	/* SCPU PLL */
	li	t1, 0x4
	sw	t1, 0x104(s0)
	lw	t2, 0x100(s0)
	sw	t2, OFF_SCPU_MN(sp)	# save SCPU PLL MN value


#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '_'
#endif

	jal	long_sleep_delay
	nop

#ifdef DEBUG_SKIP_ISO
	b	re_run_hw_setting
	nop
#endif

jupiter_switch_into_iso:
	/* Read iso_rbus_sel before write */
	lw	t1, JUPITER_ISO_CHIP_CTRL

	/* Switch Rbus from SB2 to ISO internal */
	li	a0, ~0xc00
	jal	jupiter_set_ISO_CTRL
	li	a1, 0xc00		# bit[11:10] = 3
	sync

	jal	long_sleep_delay
	nop

#ifdef DEBUG_OUTPUT_ISO_GPIO
	/* output ISO GPIO[5:4] = 3 */
	li	t1, 0xb801bd74
	lw	t2, 0(t1)
	li	t3, ~0x30
	and	t2, t3, t2
	ori	t2, t2, 0x30
	sw	t2, 0(t1)
#endif

	/* Switch ISO clock from CRT to exclusive 27MHz */
	li	a0, ~0xc
	jal	jupiter_set_ISO_CTRL
	li	a1, 0xc			# bit[3:2] = 3
	sync

	jal	long_sleep_delay
	nop

	/* Turn on ISO_CELL for ISO I/O blocking */
	li	t1, 0x2
	sw	t1, JUPITER_ISO_CELL

	/* Power down (IGPIO0 output 0) */
	li	t1, 0xb801bd70
	lw	t2, 0(t1)
	ori	t2, t2, 0x1		# bit[0] = 1
	sw	t2, 0(t1)

	lw	t2, 0x4(t1)
	li	t3, ~0x1
	and	t2, t3, t2		# bit[0] = 0
	sw	t2, 0x4(t1)

	/* Reset CRT to initial value */
	li	a0, ~0x30
	jal	jupiter_set_ISO_CTRL
	li	a1, 0x20		# bit[5:4] = 2

#ifdef DEBUG_OUTPUT_ISO_GPIO
	/* output ISO GPIO[5:4] = 1 */
	li	t1, 0xb801bd74
	lw	t2, 0(t1)
	li	t3, ~0x30
	and	t2, t3, t2
	ori	t2, t2, 0x10
	sw	t2, 0(t1)
#endif

	/* config IR, VFD... */

jupiter_resleep:
	/* Clear all interrupts that could wake up system */
	li	s0, JUPITER_ISO_BASE
	li	t1, 0xDFE020
	sw	t1, OFFSET_JUPITER_ISO_ISR(s0)

#ifdef DEBUG_OUTPUT_ISO_GPIO
	/* output ISO GPIO[5:4] = 0 */
	li	t1, 0xb801bd74
	lw	t2, 0(t1)
	li	t3, ~0x30
	and	t2, t3, t2
	ori	t2, t2, 0
	sw	t2, 0(t1)
#endif

#ifndef DEBUG_NOT_TURN_OFF_CPU 
	li	a0, ~0xc0
	/* Turn off CPU */
	jal	jupiter_set_ISO_CTRL
	li	a1, 0x80		# bit[7:6] = 2
#endif

#ifdef DEBUG_TOGGLE_CEC
	li	t1, 0x90
	sw	t1, 0xb801be80		# config CEC to DAC output mode

	li	t0, 0			# set counter

loop_toggle_cec:
	andi	t1, t0, 0x1
	beqz	t1, output_cec
	nop

set_cec_hi:
	li	t1, 0x1f

output_cec:
	sw	t1, 0xb801be8c
	b	loop_toggle_cec
	addiu	t0, t0, 0x1
#endif

#ifdef DEBUG_TOGGLE_GPIO
	li	t0, 0			# set counter
	li	t2, 0xb801bd74

loop_toggle_gpio:
	andi	t1, t0, 0x1
	lw	t3, 0(t2)
	li	t4, ~0x20
	and	t3, t4, t3
	sll	t1, t1, 5		# bit 5
	or	t3, t1, t3
	sw	t3, 0(t2)
	b	loop_toggle_gpio
	addiu	t0, t0, 0x1
#endif

#ifdef DEBUG_OUTPUT_ISO_GPIO
	/* output ISO GPIO[5:4] = 1 */
	li	t1, 0xb801bd74
	lw	t2, 0(t1)
	li	t3, ~0x30
	and	t2, t3, t2
	ori	t2, t2, 0x10
	sw	t2, 0(t1)
#endif


mars_resleep:

	li	s0, JUPITER_ISO_BASE

/* When alarm shall happen, half-second and alarm interrupts may not appear
 * at the same time. Therefore, alarm interrupt may be lost. Add some delay
 * to prevent from this. */
	jal	sleep_delay
	nop

#ifndef DEBUG_NOT_TURN_OFF_CPU
jupiter_check_cec:
        /* check CEC Interrupt Here */
	jal     cec_wakeup_check          /* run cec wake up check */
	nop
	move    s6, v0	
	bne     s6, zero, jupiter_power_on
	nop

jupiter_check_ir:
	/* Check IR interrupt here */
	lw	t1, OFFSET_JUPITER_ISO_ISR(s0)
	andi	t1, t1, 0x20				# check IRDA_INT
	beqz	t1, jupiter_check_front_panel
	nop

	lw	t2, OFFSET_JUPITER_ISO_IR_SR(s0)
	andi	t1, t2, 0x1
	beq	t1, zero, jupiter_check_front_panel
	nop

	andi	t3, t2, 0x2				# check IRRF
	beqz	t3, jupiter_ir_not_repeat
	nop

	li	t1, 0x3
	sw	t1, OFFSET_JUPITER_ISO_IR_SR(s0)
	lw	t1, OFFSET_JUPITER_ISO_IR_RP(s0)
	b	jupiter_prepare_to_resleep
	nop

jupiter_ir_not_repeat:
	li	t1, 0x1
	sw 	t1, OFFSET_JUPITER_ISO_IR_SR(s0)
	lw	t1, OFFSET_JUPITER_ISO_IR_RP(s0)
	andi	t2, 0x10
	bne	t2, zero, jupiter_check_front_panel	// For IR_SR, 0x01 is what we want. Discard 0x11 after clear the buffer.
	nop
	/* if protocol id is RC6, shift-right for 10 bits */
	lw	t3, OFF_PARAM_9(sp)
	lui	t2, 0x0000
	ori	t2, 0x0006 /* RC6 = 6 */
	bne	t3, t2, jupiter_ir_examine
	lui	t4, 0x0000
	ori	t4, 0x000a
	srl	t1, t1, t4
	lui	t3, 0x003f
	ori	t3, 0xffdf
	and	t1, t1, t3
jupiter_ir_examine:
	lw	t2, OFF_PARAM_5(sp)
	beq	t1, t2, jupiter_power_on	/* power key - IR */
	li	s6, 0
	lw	t2, OFF_PARAM_6(sp)
	beq	t1, t2, jupiter_power_on	/* eject key - IR */
	li	s6, 1
jupiter_reread_irda:
	li	t1, 0x1
	sw	t1, OFFSET_JUPITER_ISO_IR_SR(s0)
	lw	t1, OFFSET_JUPITER_ISO_IR_RP(s0)
	lw	t1, OFFSET_JUPITER_ISO_IR_SR(s0)
	andi	t1, 0x1
	bnez	t1, jupiter_reread_irda
	nop
#endif

jupiter_check_front_panel:
#if 0		/* Turn this on when VFD isn't plugged or it will send wrong data */
	j	jupiter_check_gpio_powerkey
	nop
#endif


#ifndef DEBUG_NOT_TURN_OFF_CPU
jupiter_check_gpio_powerkey:
	lw	t6, OFF_PARAM_7(sp)
	bltz	t6, jupiter_no_gpio_powerkey
	nop

	li	t2, 0x1
	sll	t2, t2, t6
	lw	t3, OFFSET_JUPITER_ISO_GPDATI(s0)
	and	t3, t3, t2
	beq	t3, zero, jupiter_power_on	/* power key - GPIO */
	li	s6, 0
jupiter_no_gpio_powerkey:

jupiter_check_gpio_ejectkey:
	lw	t6, OFF_PARAM_8(sp)
	bltz	t6, jupiter_no_gpio_ejectkey
	nop

	li	t2, 0x1
	sll	t2, t2, t6
	lw	t3, OFFSET_JUPITER_ISO_GPDATI(s0)
	and	t3, t3, t2
	beq	t3, zero, jupiter_power_on	/* eject key - GPIO */
	li	s6, 1
jupiter_no_gpio_ejectkey:
#endif



#ifndef DEBUG_NOT_TURN_OFF_CPU
jupiter_check_rtc_alarm:
	lw	t1, OFFSET_JUPITER_ISO_ISR(s0)
	andi	t1, t1, 0x2000				# check RTC_ALARM_INT
	bnez	t1, jupiter_rtc_wake_up
	nop

#ifdef DEBUG_OUTPUT_ISO_GPIO
	/* output ISO GPIO[5:4] = 3 */
	li	t1, 0xb801bd74
	lw	t2, 0(t1)
	li	t3, ~0x30
	and	t2, t3, t2
	ori	t2, t2, 0x30
	sw	t2, 0(t1)
#endif

jupiter_prepare_to_resleep:	
	/* Turn on SCPU before clear interrupt status */
	li	a0, ~0xc0
	jal	jupiter_set_ISO_CTRL
	li	a1, 0xc0		# bit[7:6] = 3

	b	jupiter_resleep
	nop
#else
	b	jupiter_power_on
	li	s6, 0xff		# wake up by ???
#endif /* #ifndef DEBUG_NOT_TURN_OFF_CPU */ 


jupiter_rtc_wake_up:
	li	s6, 2


jupiter_power_on:
#ifdef DEBUG_OUTPUT_ISO_GPIO
	/* output ISO GPIO[5:4] = 0 */
	li	t1, 0xb801bd74
	lw	t2, 0(t1)
	li	t3, ~0x30
	and	t2, t3, t2
	ori	t2, t2, 0
	sw	t2, 0(t1)
#endif

	/* Turn on SCPU */
	li	a0, ~0xc0
	jal	jupiter_set_ISO_CTRL
	li	a1, 0xc0		# bit[7:6] = 3

	/* Switch IGPIO0 to input mode */
	li	t0, 0xb801bd70
	lw	t1, 0(t0)
	li	t2, ~0x1
	and	t1, t2, t1		# bit[0] = 0
	sw	t1, 0(t0)

	/* Polling 1.0V power-good */
	li	t0, JUPITER_ISO_POWER
	li	t1, 0x1
1:
	lw	t2, 0(t0)
	bne	t1, t2, 1b
	nop

#ifdef DEBUG_OUTPUT_ISO_GPIO
	/* output ISO GPIO[5:4] = 1 */
	li	t1, 0xb801bd74
	lw	t2, 0(t1)
	li	t3, ~0x30
	and	t2, t3, t2
	ori	t2, t2, 0x10
	sw	t2, 0(t1)
#endif


	/* Event reset signals to CRT */
	li	a0, ~0x30
	jal	jupiter_set_ISO_CTRL
	li	a1, 0x30		# bit[5:4] = 3

	/* sw delay 100 cycles */
	jal	sleep_delay
	nop

	/* Mask reset signals from CRT to ISO */
	li	a0, ~0x300
	jal	jupiter_set_ISO_CTRL
	li	a1, 0x300		# bit[9:8] = 3

	/* Turn off ISO_CELL */
	li	t1, 0x1
	sw	t1, JUPITER_ISO_CELL
	sync

	jal	long_sleep_delay
	nop

#ifdef DEBUG_OUTPUT_ISO_GPIO
	/* output ISO GPIO[5:4] = 2 */
	li	t1, 0xb801bd74
	lw	t2, 0(t1)
	li	t3, ~0x30
	and	t2, t3, t2
	ori	t2, t2, 0x20
	sw	t2, 0(t1)
#endif

	/* Check ISO_CELL */
	li	t0, JUPITER_ISO_CELL
	li	t1, 0x1
1:
	lw	t2, 0(t0)
	bne	t1, t2, 1b
	nop

	/* Temp release ISO_MIS clock from CRT */
	li	a0, ~0x3000
	jal	jupiter_set_ISO_CTRL
	li	a1, 0x3000		# bit[13:12] = 3

	/* Switch ISO clock source back to CRT */
	li	a0, ~0xc
	jal	jupiter_set_ISO_CTRL
	li	a1, 0x8			# bit[3:2] = 2

	/* Switch Rbus back to SB2 */	
	li	a0, ~0xc00
	jal	jupiter_set_ISO_CTRL
	li	a1, 0x800		# bit[11:10] = 2
	sync

	jal	long_sleep_delay
	nop

#ifdef DEBUG_OUTPUT_ISO_GPIO
	/* output ISO GPIO[5:4] = 3 */
	li	t1, 0xb801bd74
	lw	t2, 0(t1)
	li	t3, ~0x30
	and	t2, t3, t2
	ori	t2, t2, 0x30
	sw	t2, 0(t1)
#endif

	/* Turn on CRT MISC clock */
	li	t1, 0x1
	sw	t1, 0xb800000c

	/* Turn off temp clock for ISO_MIS */
	li	a0, ~0xc000
	jal	jupiter_set_ISO_CTRL
	li	a1, 0x8000		# bit[13:12] = 2
	sync

	jal	long_sleep_delay
	nop

	/* Turn off CRT MISC clock as default setting */
	sw	zero, 0xb800000c

re_run_hw_setting:
/* ======= re-run hwsetting START ======= */
	lui	s0, 0xb800

#ifndef DEBUG_SKIP_PLL
	/* Turn on SCPU PLL */
	lw	t1, OFF_SCPU_MN(sp)	# restore SCPU PLL MN value
	sw	t1, 0x100(s0)
	jal	jupiter_enable_PLL
	addiu	a0, s0, 0x104

	/* Turn on BUS PLL */
	lw	t1, OFF_BUS_MN(sp)	# restore BUS PLL MN value
	sw	t1, 0x164(s0)
	jal	jupiter_enable_PLL
	addiu	a0, s0, 0x16c

	/* set serial flash controller */
	li	t1, 0x5			# fdiv = 5
	sw	t1, 0xb801a808
	sw	zero, 0xb801a814

	/* Turn on ACPU PLL */
	lw	t1, OFF_ACPU_MN(sp)	# restore ACPU PLL MN value
	sw	t1, 0x108(s0)
	jal	jupiter_enable_PLL
	addiu	a0, s0, 0x10c

	/* Turn on VCPU PLL */
	lw	t1, OFF_VCPU_MN(sp)	# restore VCPU PLL MN value
	sw	t1, 0x110(s0)
	jal	jupiter_enable_PLL
	addiu	a0, s0, 0x114

	/* Turn on SD PLL */
	lw	t1, OFF_SD_MN(sp)	# restore SD PLL MN value
	sw	t1, 0x118(s0)
	li	t1, 0xff
	sw	t1, 0x11c(s0)		# Phase Shifter
	li	t1, 0x5
	sw	t1, 0x120(s0)		# Enable PSAUD1A, PSAUD2A
	jal	jupiter_enable_PLL
	addiu	a0, s0, 0x124

	/* Turn on HD PLL */
	lw	t1, OFF_HD_MN(sp)	# restore HD PLL MN value
	sw	t1, 0x130(s0)
	jal	jupiter_enable_PLL
	addiu	a0, s0, 0x134

	/* Turn on SD2 PLL */
	li	t1, 0x5
	sw	t1, 0x17c(s0)		# Enable PSAUD1B, PSAUD2B
	li	t1, 0xff
	sw	t1, 0x178(s0)		# Phase Shifter
	jal	jupiter_enable_PLL
	addiu	a0, s0, 0x180

	/* Turn on ETN PLL */
	jal	jupiter_enable_PLL
	addiu	a0, s0, 0x1a4

	/* Turn on DDR PLL */
	lw	t1, OFF_DDR_MN(sp)	# restore DDR PLL MN value
	sw	t1, 0x150(s0)
	jal	jupiter_enable_PLL
	addiu	a0, s0, 0x15c

	/* Switch clock source */
	sw	zero, 0x18(s0)
	#li	t1, 0x1
	#sw	t1, 0x24(s0)
	li	t1, 0x200
	sw	t1, 0x2c(s0)

	/* Enable SSC */
	li	t1, 0xbf0a0000
	sw	t1, 0x64(s0)		# Set DDR SSC
	li	t1, 0x70
	sw	t1, 0x158(s0)		# enable DDR SSC control

#endif /*  DEBUG_SKIP_PLL */

#ifndef DEBUG_SKIP_CLOCK
	/* Turn on clocks for all module */
	lw	t1, OFF_CRT_CLK_ENABLE1(sp)
	li	t2, ~0x10
	and	t1, t1, t2		# turn on all clock (except USB)
	sw	t1, 0xc(s0)
	lw	t1, OFF_CRT_CLK_ENABLE2(sp)
	li	t2, ~0x30
	and	t1, t1, t2		# turn on all clock (except ACPU, VCPU)
	sw	t1, 0x10(s0)

	jal	sleep_delay
	nop

	sw	zero, 0xc(s0)		# turn off all clock
	sw	zero, 0x10(s0)

	jal	sleep_delay
	nop

	lw	t1, OFF_CRT_SOFT_RESET1(sp)
	li	t2, ~0x40
	and	t1, t1, t2		# release all reset (except USB)
	sw	t1, 0(s0)
	lw	t1, OFF_CRT_SOFT_RESET2(sp)
	sw	t1, 0x4(s0)
	lw	t1, 0x450(s0)
	ori	t1, 0x800		# release USB-PHY port 2 reset
	sw	t1, 0x450(s0)

	jal	sleep_delay
	nop

	lw	t1, OFF_CRT_CLK_ENABLE1(sp)
	li	t2, ~0x10
	and	t1, t1, t2		# turn on all clock (except USB)
	sw	t1, 0xc(s0)
	lw	t1, OFF_CRT_CLK_ENABLE2(sp)
	li	t2, ~0x30
	and	t1, t1, t2		# turn on all clock (except ACPU, VCPU)
	sw	t1, 0x10(s0)

	jal	sleep_delay
	nop
#endif

	/* MISC Setting and Initial */
	li	t1, 0x15546455		# UART,I2C,GPIO
	sw	t1, 0xb8000378

	li	s0, 0xb801b200		# UART base

	li	t1, 0x90
	sw	t1, 0xc(s0)		# U0LCR
	sw	t1, 0x2c(s0)
	li	t1, 0xe			# 115200 baud (for ASIC 27MHz)
	sw	t1, 0(s0)		# U0RBR_THR_DLL
	sw	t1, 0x20(s0)
	sw	zero, 0x4(s0)		# U0IER_DLH
	sw	zero, 0x24(s0)
	li	t1, 0x3
	sw	t1, 0xc(s0)		# U0LCR (8-N-1)
	sw	t1, 0x2c(s0)
	li	t1, 0xc7
	sw	t1, 0x8(s0)		# U0IIR_FCR
	sw	t1, 0x28(s0)
	li	t1, 0x1
	sw	t1, 0x4(s0)		# U0IER_DLH
	sw	t1, 0x24(s0)

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa
	jal	print_char
	li	a0, '0'
#endif

	lui	s0, 0xb800

	/* EMA */
	sw	zero, 0xb801bf50
	li	t1, 0x00301111
	sw	t1, 0x300(s0)

#ifndef DEBUG_SKIP_DDR_SELF_REFRESH
	/* restore DCU setting */
	lw	t1, OFF_DCU_1(sp)
	sw	t1, 0xb8000030
	lw	t1, OFF_DCU_2(sp)
	sw	t1, 0xb8000154
	lw	t1, OFF_DCU_3(sp)
	sw	t1, 0xb8008004
	lw	t1, OFF_DCU_4(sp)
	sw	t1, 0xb8008800
	lw	t1, OFF_DCU_5(sp)
	sw	t1, 0xb8008808
	lw	t1, OFF_DCU_6(sp)
	sw	t1, 0xb8008818
	lw	t1, OFF_DCU_7(sp)
	sw	t1, 0xb8008828
	lw	t1, OFF_DCU_8(sp)
	sw	t1, 0xb8008880
	lw	t1, OFF_DCU_9(sp)
	sw	t1, 0xb800882c
	lw	t1, OFF_DCU_20(sp)
	sw	t1, 0xb8008844
	lw	t1, OFF_DCU_10(sp)
	sw	t1, 0xb8008848
	lw	t1, OFF_DCU_11(sp)
	sw	t1, 0xb800884c
	lw	t1, OFF_DCU_12(sp)
	sw	t1, 0xb8008850
	lw	t1, OFF_DCU_13(sp)
	sw	t1, 0xb8008904
	lw	t1, OFF_DCU_14(sp)
	sw	t1, 0xb8008908
	lw	t1, OFF_DCU_15(sp)
	sw	t1, 0xb800890c
	lw	t1, OFF_DCU_16(sp)
	sw	t1, 0xb8008910
	lw	t1, OFF_DCU_17(sp)
	sw	t1, 0xb8008930
	lw	t1, OFF_DCU_18(sp)
	sw	t1, 0xb8008914
	lw	t1, OFF_DCU_19(sp)
	sw	t1, 0xb8008920

	/* check if memory type is DDR3 */
	lw	t1, 0xb8008004
	srl	t1, t1, 22
	andi	t1, t1, 0x1
	beqz	t1, 1f			# branch if not DDR3
	nop

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '>'
#endif

	/* additional setting for DDR3 */
	li	t2, 0xb8008840
	li	t1, 0x30000000
	sw	t1, 0(t2)
	li	t1, 0x0000c000
	sw	t1, 0(t2)

1:
	li	t1, 0x00200000
	sw	t1, 0xb8008840
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop

exit_self_refresh_mode:
	/* Exit self-refresh mode */
	li	t1, 0x4
	sw	t1, 0xb800880c
	sw	zero, 0xb800880c
	li	t1, 0x3
	sw	t1, 0xb800894c	

	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
#endif

	/* SYSBRG2 Setting */
	li	t1, 0x00000031
	sw	t1, 0xb801a004		# Rbus time out enable
	li	t1, 0x0000133b
	sw	t1, 0xb801a018		# enable SCPU merge and bypass

#if 1
	/* check if memory type is DDR3 */
	lw	t1, 0xb8008004
	srl	t1, t1, 22
	andi	t1, t1, 0x1
	beqz	t1, ddr3_workaround_end	# branch if not DDR3
	nop

ddr3_workaround_start:
	lw	t1, 0xa0000000		# read memory

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, 'a'
#endif

	/* enter self-refresh mode again */
	li	t1, 0x08
	sw	t1, 0xb800880c

	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, 'b'
#endif

	/* reset DCU & DC-PHY clk */
	lui	t0, 0xb800
	addiu	a0, t0, 0xc
	li	a1, ~0x4000
	jal	mask_operation		# set clk_en_dcu = 0
	move	a2, zero

	move	a0, t0
	li	a1, ~0x1800000
	jal	mask_operation		# set rstn_dc_phy = rstn_dcu = 0
	move	a2, zero

	move	a0, t0
	not	a1, zero
	li	a2, 0x1800000
	jal	mask_operation		# set rstn_dc_phy = rstn_dcu = 1
	nop

	addiu	a0, t0, 0xc
	not	a1, zero
	li	a2, 0x40000
	jal	mask_operation		# set clk_en_dcu = 1
	nop

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, 'c'
#endif

	/* restore DCU again */
	lw	t1, OFF_DCU_1(sp)
	sw	t1, 0xb8000030
	lw	t1, OFF_DCU_2(sp)
	sw	t1, 0xb8000154
	lw	t1, OFF_DCU_3(sp)
	sw	t1, 0xb8008004
	lw	t1, OFF_DCU_4(sp)
	sw	t1, 0xb8008800
	lw	t1, OFF_DCU_5(sp)
	sw	t1, 0xb8008808
	lw	t1, OFF_DCU_6(sp)
	sw	t1, 0xb8008818
	lw	t1, OFF_DCU_7(sp)
	sw	t1, 0xb8008828
	lw	t1, OFF_DCU_8(sp)
	sw	t1, 0xb8008880
	lw	t1, OFF_DCU_9(sp)
	sw	t1, 0xb800882c
	lw	t1, OFF_DCU_20(sp)
	sw	t1, 0xb8008844
	lw	t1, OFF_DCU_10(sp)
	sw	t1, 0xb8008848
	lw	t1, OFF_DCU_11(sp)
	sw	t1, 0xb800884c
	lw	t1, OFF_DCU_12(sp)
	sw	t1, 0xb8008850
	lw	t1, OFF_DCU_13(sp)
	sw	t1, 0xb8008904
	lw	t1, OFF_DCU_14(sp)
	sw	t1, 0xb8008908
	lw	t1, OFF_DCU_15(sp)
	sw	t1, 0xb800890c
	lw	t1, OFF_DCU_16(sp)
	sw	t1, 0xb8008910
	lw	t1, OFF_DCU_17(sp)
	sw	t1, 0xb8008930
	lw	t1, OFF_DCU_18(sp)
	sw	t1, 0xb8008914
	lw	t1, OFF_DCU_19(sp)
	sw	t1, 0xb8008920

	li	t2, 0xb8008840
	li	t1, 0x30000000
	sw	t1, 0(t2)
	li	t1, 0x0000c000
	sw	t1, 0(t2)
	li	t1, 0x00200000
	sw	t1, 0xb8008840
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, 'd'
#endif

	/* exit self-refresh mode again */
	li	t1, 0x4
	sw	t1, 0xb800880c
	sw	zero, 0xb800880c
	li	t1, 0x3
	sw	t1, 0xb800894c	

	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop
	jal	long_sleep_delay
	nop

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, 'e'
#endif

ddr3_workaround_end:
#endif /* #if 1 */

	/* enable audio DAC */
	li	t1, 0xcc1e0000
	sw	t1, 0xb8006604

	/* ETN */
	li	t1, 0x47
	sw	t1, 0xb800034c		# N.P MOS
	li	t1, 0x44444000
	sw	t1, 0xb80003b8		# driving 8mA

	/* restore calibration parameter */
	lw	t1, OFF_DCU_CALI_PARAM(sp)
	sw	t1, 0xb801a618
	lw	t1, OFF_BOOTCODE_OPTION(sp)
	sw	t1, 0xb801a61c

/* ======= re-run hwsetting END ======= */


#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '1'
#endif

#ifndef DEBUG_SKIP_ISO
	/* Turn off reset mask */
	li	a0, ~0x300
	jal	jupiter_set_ISO_CTRL
	li	a1, 0x200		# bit[9:8] = 2
	sync
#endif

#ifdef CONFIG_PM_SLEEP_REBOOT
	li	t0, 0x2
	beq	t0, s6, jupiter_save_reboot_info	# branch if wake up by RTC alarm 
	li	t1, 0x4

	li	t1, 0x2

jupiter_save_reboot_info:
	/* 0xb801bf0c bit2  bit1 bit0
	 *              0     0    x   cold boot
	 *              0     1    x   power key warm boot
	 *              1     0    x   rtc warm boot
	 */
	li	s0, JUPITER_ISO_BASE
	lw	t0, OFFSET_JUPITER_ISO_COLD_RST(s0)
	ori	t0, t0, 0x7
	xori	t0, t0, 0x7		# clear bit[2:0]
	or	t0, t0, t1
	sw	t0, OFFSET_JUPITER_ISO_COLD_RST(s0)
	
	/* reboot after CPU can access outside Rbus */
jupiter_check_watchdog_reset:
	sw	zero, VENUS_MIS_TCWCR+VENUS_IO_PORT_BASE+0xA0000000

1:
	jal	long_sleep_delay
	nop
#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '~'
#endif
	b	1b
	nop
#endif /* #ifdef CONFIG_PM_SLEEP_REBOOT */


#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '2'
#endif

	/* DCU calibration */
#ifndef DEBUG_SKIP_DCU_CALIBRATION
	jal	jupiter_dcu_calibration
	nop
#else
	lw	t1, OFF_DCU_CALI_RESULT(sp)
	sw	t1, 0xb8008880
	li	t0, 0xb800882c
	lw	t1, 0(t0)
	lui	t2, 0x0200
	or	t1, t2, t1		# fw_set = 1
	sw	t1, 0(t0)
	jal	sleep_delay
	nop

	li	t0, 0xb800882c
	lui	t2, 0x0200
1:
	lw	t1, 0(t0)
	and	t1, t2, t1
	bnez	t1, 1b
	nop
#endif /* DEBUG_SKIP_DCU_CALIBRATION */

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '3'
#endif
	
	# restore original value in DDR
	lw	t1, OFF_DCU_CALI_SAVE1(sp)
	sw	t1, 0xa0000000
	lw	t1, OFF_DCU_CALI_SAVE2(sp)
	sw	t1, 0xa0000008

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, ','
#endif

	/* Get HW semaphore */
#ifdef CONFIG_REALTEK_USE_HWSEM_AS_SENTINEL
	lui	s0, 0xb801
1:
	lw	t1, VENUS_SB2_HD_SEM(s0)
	beqz	t1, 1b
	nop	

	/* init SW mutex */
	sw	zero, JUPITER_SW_MUTEX_RESET	# SCPU hold reset mutex
	li	t0, JUPITER_SW_MUTEX_DC
	li	t1, 0x1
	sw	t1, 0(t0)			# set DC mutex as "available"

	/* release HW semaphore */
	sw	zero, VENUS_SB2_HD_SEM(s0)
#else
	lw	t0, 0xb801a000
#endif

#ifndef DEBUG_SKIP_BISR
	li	t0, BISR_DONE_FLAG_ADDR
	lw	t1, 0(t0)
	sw	t1, OFF_BISR_FLAG_TMP(sp)	# save DDR original value

	/* reset BISR done flag for 5280 */
	sw	zero, 0(t0)
	sw	zero, OFF_BISR_STATUS(sp)

	/* Turn on ACPU, VCPU clock */
	lui	s0, 0xb800
	lw	t1, 0x10(s0)
	ori	t1, t1, 0x30
	sw	t1, 0x10(s0)

/* ======= run BISR START ======= */
	/* ACPU BISR */
	li	a0, 0x40
	li	a1, 0x8
	jal	jupiter_turn_on_BISR
	nop

	li	s0, 0xb8000600
  
jupiter_polling_acpu_bisr:
	lw	t1, 0x90(s0)
	andi	t1, t1, 0x1
	beqz	t1, jupiter_polling_acpu_bisr
	nop
	
	lw	t1, 0x90(s0)
	andi	t1, t1, 0x7e		# only acpu_bisr_fail0~5
	beqz	t1, 9f
	nop

	lw	t1, OFF_BISR_STATUS(sp)
	ori	t1, t1, 0x8
	sw	t1, OFF_BISR_STATUS(sp)	

9:
	li	a0, 0x40
	li	a1, 0x8
	jal	jupiter_turn_off_BISR
	nop

	/* VCPU BISR */
	li	a0, 0x20
	li	a1, 0x4
	jal	jupiter_turn_on_BISR
	nop

jupiter_polling_vcpu_bisr:
	lw	t1, 0x9c(s0)
	andi	t1, t1, 0x1
	beqz	t1, jupiter_polling_vcpu_bisr
	nop
	
	lw	t1, 0x9c(s0)
	andi	t1, t1, 0x3e		# only vcpu_bisr_fail0~4
	beqz	t1, 9f
	nop
	
	lw	t1, OFF_BISR_STATUS(sp)
	ori	t1, t1, 0x4
	sw	t1, OFF_BISR_STATUS(sp)	

9:
	li	a0, 0x20
	li	a1, 0x4
	jal	jupiter_turn_off_BISR
	nop

	/* VO BISR */
	li	a0, 0x10
	li	a1, 0x2
	jal	jupiter_turn_on_BISR
	nop

jupiter_polling_vo_bisr:
	lw	t1, 0x4c(s0)
	andi	t1, t1, 0x1
	beqz	t1, jupiter_polling_vo_bisr
	nop
	
	lw	t1, 0x4c(s0)
	andi	t1, t1, 0x1e		# only vo_bisr_fail0~3
	beqz	t1, 9f
	nop
	
	lw	t1, OFF_BISR_STATUS(sp)
	ori	t1, t1, 0x2
	sw	t1, OFF_BISR_STATUS(sp)	

9:
	li	a0, 0x10
	li	a1, 0x2
	jal	jupiter_turn_off_BISR
	nop

	li	t1, 0x1			# set flag as BISR done
	sw	t1, BISR_DONE_FLAG_ADDR

	/* delay some time before restore BISR done flag on memroy */
	li	s1, 16
1:	
	jal	long_sleep_delay
	nop
	
	bnez	s1, 1b
	subu	s1, s1, 0x1

	lw	t1, OFF_BISR_STATUS(sp)
	beqz	t1, jupiter_bisr_done
	nop

	jal	print_registerr		# print BISR fail status if exist
	move	a0, t1

	jal	print_char
	li	a0, '!'
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa

jupiter_bisr_done:
	lw	t1, OFF_BISR_FLAG_TMP(sp)
	sw	t1, BISR_DONE_FLAG_ADDR	# restore DDR to original value
/* ======= run BISR END ======= */
#endif /* DEBUG_SKIP_BISR */


#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa
	jal	print_char
	li	a0, '4'
#endif

jupiter_flush_cache:
#if 1
	/* flush data segment in D-cache */
	la	t1, _esleepdata
	addiu	t1, 32
	la	t2, _sleepdata
2:
.set mips3
	cache	DCACHE_ADDR_WRITEBACK_INVALIDATE, 0(t2)
.set mips0
	sltu	t0, t2, t1
	bnez	t0, 2b
	addiu	t2, 32

#endif


#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '5'
#endif

#if 1
	/* flush stack in D-cache */
	li	t1, 256
	move	t2, sp
	addiu	t2, t2, FRAME_SIZE
3:
.set mips3
	cache	DCACHE_ADDR_WRITEBACK_INVALIDATE, 0(t2)
.set mips0
	addiu	t1, -1
	bnez	t1, 3b
	addiu	t2, -32
#endif

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '6'
#endif


#if 0
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa

	/* load data segment from D-cache */
	la	a0, _sleepdata
	jal	print_registerr
	nop
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa

	la	a3, _sleepdata
	andi	a3, a3, 0x1fff
	lui	t0, 0x8000
	or	a3, a3, t0
	jal	print_registerr
	move	a0, a3
	jal	print_char
	li	a0, ':'

.set mips3
	cache	DCACHE_INDEX_LOAD_TAG, 0(a3)
.set mips0
	nop
	nop
	mfc0	a0, CP0_TAGLO, 2
	jal	print_registerr
	nop
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa

	addiu	a3, a3, 0x2000
	jal	print_registerr
	move	a0, a3
	jal	print_char
	li	a0, ':'

.set mips3
	cache	DCACHE_INDEX_LOAD_TAG, 0(a3)
.set mips0
	nop
	nop
	mfc0	a0, CP0_TAGLO, 2
	jal	print_registerr
	nop
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa

	addiu	a3, a3, 0x2000
	jal	print_registerr
	move	a0, a3
	jal	print_char
	li	a0, ':'

.set mips3
	cache	DCACHE_INDEX_LOAD_TAG, 0(a3)
.set mips0
	nop
	nop
	mfc0	a0, CP0_TAGLO, 2
	jal	print_registerr
	nop
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa

	addiu	a3, a3, 0x2000
	jal	print_registerr
	move	a0, a3
	jal	print_char
	li	a0, ':'

.set mips3
	cache	DCACHE_INDEX_LOAD_TAG, 0(a3)
.set mips0
	nop
	nop
	mfc0	a0, CP0_TAGLO, 2
	jal	print_registerr
	nop
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa

#endif


jupiter_turn_on_usb_clk:
	/* delay some time before turn on USB clock */
	li	s1, 30
1:	
	jal	long_sleep_delay
	nop
	
	bnez	s1, 1b
	subu	s1, s1, 0x1

#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '7'
#endif

	/* Turn on USB clock */
	lui	s0, 0xb800
	lw	t1, 0xc(s0)
	ori	t1, t1, 0x10
	sw	t1, 0xc(s0)
	jal	sleep_delay
	nop

	li	t2, ~0x10
	or	t1, t1, t2
	sw	t1, 0xc(s0)
	jal	sleep_delay
	nop

	lw	t1, 0(s0)
	ori	t1, t1, 0x40
	sw	t1, 0(s0)
	jal	sleep_delay
	nop

	lw	t1, 0xc(s0)
	ori	t1, t1, 0x10
	sw	t1, 0xc(s0)
	
epilog_jupiter_cpu_suspend:
#ifdef OPTION_SLEEP_PRINT
	jal	print_char
	li	a0, '8'
	jal	print_char
	li	a0, 0xd
	jal	print_char
	li	a0, 0xa
#endif

	move	v0, s6
	lw	s0, 0(sp)
	lw	s1, 4(sp)
	lw	s2, 8(sp)
	lw	s3, 12(sp)
	lw	s4, 16(sp)
	lw	s5, 20(sp)
	lw	s6, 24(sp)
	lw	s7, 28(sp)
	lw	ra, 32(sp)
	addiu	sp, sp, FRAME_SIZE
#ifdef DEBUG_USE_SEPERATE_STACK
	la	k0, _test_stack_base
	lw	sp, 0(k0)		# restore kernel stack
#endif
	jr	ra
	nop

#undef FRAME_SIZE
#undef OFF_PARAM_5
#undef OFF_PARAM_6
#undef OFF_PARAM_7
#undef OFF_PARAM_8
#undef OFF_PARAM_9
END(jupiter_cpu_suspend)

/*************************************************
 * Set ISO CTRL register
 *
 *************************************************
 * input argument:
 *	a0: "AND" mask value
 *	a1: "OR" mask value
 * use other registers:
 *	v0-v1: storage for register value
 * return:
 *	none (ISO CTRL register would be updated with AND then OR mask value)
 ************************************************/
LEAF(jupiter_set_ISO_CTRL)
	li	v0, JUPITER_ISO_CHIP_CTRL
	lw	v1, 0(v0)
	and	v1, a0, v1
	or	v1, a1, v1
	sw	v1, 0(v0)
	jr	ra
	nop
END(jupiter_set_ISO_CTRL)


/*************************************************
 * enabel PLL
 *
 *************************************************
 * input argument:
 *	a0: PLL register address
 * use other registers:
 *	t1: storage for register value
 * return:
 *	none
 ************************************************/
LEAF(jupiter_enable_PLL)
	addiu   sp, sp, -8
	sw	a0, 0(sp)
	sw      ra, 4(sp)

	# turn off PLL (disable OEB, set RSTB to reset mode, set POW to power down)
	li	t1, 0x4
	sw	t1, 0(a0)
	
	# disable OEB, set RSTB to reset mode, set POW to active
	li	t1, 0x5
	sw	t1, 0(a0)

	# wait at least 0.5us between POW=1 and RSTB=1
	jal	sleep_delay
	nop

	lw	a0, 0(sp)
	# disable OEB, set RSTB to normal mode, set PWDN to active
	li	t1, 0x7
	sw	t1, 0(a0)
	
	# wait at least 152us for PLL becoming stable
	jal	sleep_delay
	nop

	lw	a0, 0(sp)
	# enable OEB, set RSTB to normal mode, set PWDN to active
	li	t1, 0x3
	sw	t1, 0(a0)
	
	lw	ra, 4(sp)
	jr	ra
	addiu   sp, sp, 8
END(jupiter_enable_PLL)



/*************************************************
 * Perform AND, OR operation to register/memory
 *
 *************************************************
 * input argument:
 *	a0: register address
 *	a1: "AND" mask value
 *	a2: "OR" mask value
 * use other registers:
 *	v0: storage for register value
 * return:
 *	none
 ************************************************/
LEAF(mask_operation)
	lw	v0, 0(a0)
	and	v0, v0, a1
	or	v0, v0, a2
	jr	ra
	sw	v0, 0(a0)
END(mask_operation)


/*************************************************
 * Execute DCU calibration process
 *
 *************************************************
 * input argument:
 *	none
 * use other registers:
 *	a0-a3:
 *	t0-t3: storage for register value
 *	v0-v1:
 * return:
 *	none
 ************************************************/
LEAF(jupiter_dcu_calibration)
	addiu   sp, sp, -12
	sw	ra, 0(sp)

	# see if there's DCU calibration parameter in SB2 register
	li	t0, 0xb801a618
	lw	v0, 0(t0)
	srl	v1, v0, 16		# DCU balibration magic number in bit[31:16]
	li	t0, 0xdcca
	beq	v1, t0, jupiter_dcu_calibration_get_pos
	andi	v0, v0, 0xffff

	# use default value: 0-31
	li	v0, 0x001f

jupiter_dcu_calibration_get_pos:
	srl	a0, v0, 8		# start position in bit[15:8]
	andi	a1, v0, 0xff		# end position in bit[7:0]

jupiter_dcu_calibration_init:
	li	t0, 0xb8008000

	# put test data in DDR
	li	a2, 0x5a5aa5a5		# golden data
	li	v0, 0xa0000000
	sw	a2, 0(v0)		# set golden data
	sw	a2, 0x8(v0)

	move	t1, zero		# whether first correct pos is found (default is not found)
	move	v0, zero		# calibration result: start_pos
	move	v1, zero		# calibration result: end_pos

	# run calibration
loop_jupiter_dcu_calibration:
	blt	a1, a0, epilog_jupiter_dcu_calibration
	nop

	lw	t2, 0x880(t0)
	and	t2, ~0x3ff
	sll	t3, a1, 0x5
	or	t3, a1, t3		# set ph_sel_dq_neg_0_0 & ph_sel_dq_pos_0_0
	or	t2, t3, t2
	sw	t2, 0x880(t0)
	
	lw	t2, 0x82c(t0)
	lui	t3, 0x0200
	or	t2, t3, t2		# set fw_set = 1
	sw	t2, 0x82c(t0)

	lw	t2, 0xa0000000		# get golden data from DDR
	bne	t2, a2, 9f		# branch if value in DDR is not correct
	nop
	
	lw	t3, 0xa0000008		# get golden data from DDR
	bne	t3, a2, 9f		# branch if value in DDR is not correct
	nop

	# value in DDR is correct
	bnez	t1, 9f			# branch if end_pos already set
	move	v0, a1			# always update start_pos with latest pos
	
	move	v1, a1			# set end_pos only once
	addiu	t1, 0x1

9:
	# next loop
	b	loop_jupiter_dcu_calibration
	subu	a1, 0x1

epilog_jupiter_dcu_calibration:
	# save pos in dummy register
	sll	a3, v0, 8		# start_pos
	or	a3, v1, a3		# end_pos
	sw	a3, 0x7f0(t0)
	
	# set final pos
	addu	a3, v0, v1
	srl	a3, a3, 1			# (start_pos + end_post) / 2
	andi	a3, 0x3f

	lw	t2, 0x880(t0)
	and	t2, ~0x3ff
	sll	t3, a3, 0x5
	or	t3, a3			# set fw_dlc_neg_dqs_dq0 & fw_dlc_pos_dqs_dq0
	or	t2, t3
	sw	t2, 0x880(t0)

	lw	t2, 0x82c(t0)
	lui	t3, 0x0200
	or	t2, t3			# set fw_set = 1
	sw	t2, 0x82c(t0)

1:
	lw	v0, 0x82c(t0)
	and	v0, t3, v0
	bnez	v0, 1b
	nop

	lw	ra, 0(sp)
	jr	ra
	addiu   sp, sp, 12
END(jupiter_dcu_calibration)


/*************************************************
 * Turn on BISR
 *
 *************************************************
 * input argument:
 *	a0: bisr_pwn_rstn mask (in 0xb8000600)
 *	a1: bisr_hold_remap mask (in 0xb8000604)
 * use other registers:
 *	v0: storage for register value
 * return:
 *	none
 ************************************************/
LEAF(jupiter_turn_on_BISR)
	lui	v0, 0xb800
	andi	a0, a0, 0xfe
	sw	a0, 0x600(v0)
	and	a1, a1, 0xfe
	sw	a1, 0x604(v0)
	ori	a0, a0, 0x1	
	sw	a0, 0x600(v0)
	jr	ra
	nop
END(jupiter_turn_on_BISR)


/*************************************************
 * Turn off BISR
 *
 *************************************************
 * input argument:
 *	a0: bisr_pwn_rstn mask
 *	a1: bisr_hold_remap mask
 * use other registers:
 *	v0: storage for register value
 * return:
 *	none
 ************************************************/
LEAF(jupiter_turn_off_BISR)
	lui	v0, 0xb800
	ori	a1, a1, 0x1	
	sw	a1, 0x604(v0)
	andi	a0, a0, 0xfe
	sw	a0, 0x600(v0)
	ori	a0, a0, 0x1	
	sw	a0, 0x600(v0)
	jr	ra
	nop
END(jupiter_turn_off_BISR)


/*************************************************
 * Clear D-cache tags
 *
 *************************************************
 * input argument:
 *	a0: size of whole cache
 *	a1: size of each cache line
 * use other registers:
 *	v0-v1: storage for register value
 * return:
 *	none
 ************************************************/
LEAF(clear_dcache_tag)
	mtc0	zero, CP0_TAGLO, 2
	lui	v0, 0x8000
	addu	v1, v0, a0
	subu	v1, v1, a1
1:
.set mips3
	cache	DCACHE_INDEX_STORE_TAG, 0(v0)
.set mips0
	bne	v0, v1, 1b
	addu	v0, v0, a1
	jr	ra
	nop
END(clear_dcache_tag)


end:


