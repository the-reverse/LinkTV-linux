/*
 *  linux/drivers/mmc/rtk_ms.h - Realtek MS driver
 *
 *  Copyright (C) 2009 CMYu, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */


struct ms_info {
	char detect_counter;
	char reset_flag;
	char error_card;
	char write_protect_flag;
	unsigned int capacity;

	char eject;

	char bIsMSPro;
	char MS_4bit;

	unsigned char multi_flag;     // bit flag, each bit represents a certain flag

	//MSPro
	unsigned char MSProSequentialFlag;
	unsigned int MSProPreAddr;
	unsigned char MSProPreOP;
	unsigned char MSProINT;

	//MS
	unsigned char MSBootBlock;
	unsigned char MSBlockShift;
	unsigned char MSPageOff;
	unsigned short MSTotalBlock;

	//L2P
	struct zone_entry *segment;
	int segmentcount;

#ifdef MS_DELAY_WRITE
	struct ms_delay_write_tag delay_write;
#endif

#ifdef ERROR_TRACE
	unsigned char time_para[2];
#endif
};


//struct rtkms_host {
struct rtkms_port {
	struct ms_host 	*ms;		/* MS structure */
	spinlock_t		* rtlock;		/* Mutex */

	int			rtflags;		/* Driver states */
//#define RTKSD_FCARD_PRESENT	    (1<<0)		/* Card is present */
//#define RTKSD_FIGNORE_DETECT	(1<<1)		/* Ignore card detection */
#define RTKMS_FCARD_DETECTED     (1<<0)      /* Card is detected */
#define RTKMS_FIGNORE_DETECT    (1<<1)      /* Ignore card detection */
#define RTKMS_FCARD_POWER       (1<<2)      /* Card is power on */
#define RTKMS_FHOST_POWER       (1<<3)      /* Host is power on */
#define RTKMS_BUS_BIT4          (1<<4)      /* 4-bit bus width */
#define RTKMS_BUS_BIT8          (1<<5)      /* 8-bit bus width */


    int hp_event;
#define HP_NON_ENENT    0
#define HP_INS_ENENT    1
#define HP_RMV_ENENT    2

    int act_host_clk;

#define CARD_SWITCHCLOCK_20MHZ  0x00
#define CARD_SWITCHCLOCK_24MHZ  0x01
#define CARD_SWITCHCLOCK_30MHZ  0x02
#define CARD_SWITCHCLOCK_40MHZ  0x03
#define CARD_SWITCHCLOCK_48MHZ  0x04
#define CARD_SWITCHCLOCK_60MHZ  0x05
#define CARD_SWITCHCLOCK_80MHZ  0x06
#define CARD_SWITCHCLOCK_96MHZ  0x07

	struct ms_request 	*mrq;		/* Current request */
//	u8			isr;		/* Accumulated ISR */

//	struct scatterlist	*cur_sg;	/* Current SG entry */
//	unsigned int		num_sg;		/* Number of entries left */
//	void			*mapped_sg;	/* vaddr of mapped sg */

//	unsigned int		offset;		/* Offset into current entry */
//	unsigned int		remain;		/* Data left in current entry */

	int			size;		/* Total size of transfer */

	char			*dma_buffer;	/* ISA DMA buffer */
//	dma_addr_t		dma_addr;	/* Physical address for same */

//	int			firsterr;	/* See fifo functions */

//	u8			clk;		/* Current clock speed */

//	int			config;		/* Config port */
//	u8			unlock_code;	/* Code to unlock config */

//	int			chip_id;	/* ID of controller */

//	int			base;		/* I/O port base */
//	int			irq;		/* Interrupt */
//	int			dma;		/* DMA channel */

//	struct tasklet_struct	card_tasklet;	/* Tasklet structures */
//	struct tasklet_struct	fifo_tasklet;
//	struct tasklet_struct	crc_tasklet;
//	struct tasklet_struct	timeout_tasklet;
//	struct tasklet_struct	finish_tasklet;
//	struct tasklet_struct	block_tasklet;

	//CMYu adds it.
	struct ms_info ms_card;
};
