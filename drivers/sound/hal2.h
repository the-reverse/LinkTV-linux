/*
 * drivers/sgi/audio/hal2.h
 *
 * Copyright (C) 1998 Ulf Carlsson (ulfc@bun.falkenberg.se)
 * 
 */

#define H2_HAL2_BASE		(HPC3_CHIP0_PBASE + 0x58000)
#define H2_CTRL_PIO		(H2_HAL2_BASE + 0 * 0x200)
#define H2_AES_PIO		(H2_HAL2_BASE + 1 * 0x200)
#define H2_VOLUME_PIO		(H2_HAL2_BASE + 2 * 0x200)
#define H2_SYNTH_PIO		(H2_HAL2_BASE + 3 * 0x200)

typedef volatile unsigned int hal_reg;

struct hal2_ctrl_regs {
	hal_reg _unused0[4];
	hal_reg isr;		/* 0x10 Status Register */
	hal_reg _unused1[3];
	hal_reg rev;		/* 0x20 Revision Register */
	hal_reg _unused2[3];
	hal_reg iar;		/* 0x30 Indirect Address Register */
	hal_reg _unused3[3];
	hal_reg idr0;		/* 0x40 Indirect Data Register 0 */
	hal_reg _unused4[3];
	hal_reg idr1;		/* 0x50 Indirect Data Register 1 */
	hal_reg _unused5[3];
	hal_reg idr2;		/* 0x60 Indirect Data Register 2 */
	hal_reg _unused6[3];
	hal_reg idr3;		/* 0x70 Indirect Data Register 3 */
} volatile *h2_ctrl = (struct hal2_ctrl_regs *) KSEG1ADDR(H2_CTRL_PIO);

struct hal2_vol_regs {
	hal_reg right;		/* 0x00 Right volume */
	hal_reg left;		/* 0x04 Left volume */
} volatile *h2_vol = (struct hal2_vol_regs *) KSEG1ADDR(H2_VOLUME_PIO);

/* AES and synth regs should end up here if we ever support them */

/* Indirect status register */

#define H2_ISR_TSTATUS		0x01	/* RO: transaction status 1=busy */
#define H2_ISR_USTATUS		0x02	/* RO: utime status bit 1=armed */
#define H2_ISR_QUAD_MODE	0x04	/* codec mode 0=indigo 1=quad */
#define H2_ISR_GLOBAL_RESET_N	0x08	/* chip global reset 0=reset */
#define H2_ISR_CODEC_RESET_N	0x10	/* codec/synth reset 0=reset  */

	/* Revision register */

#define H2_REV_AUDIO_PRESENT	0x8000	/* RO: audio present 0=present */
#define H2_REV_BOARD_M		0x7000	/* RO: bits 14:12, board revision */
#define H2_REV_MAJOR_CHIP_M	0x00F0	/* RO: bits 7:4, major chip revision */
#define H2_REV_MINOR_CHIP_M	0x000F	/* RO: bits 3:0, minor chip revision */

/* Indirect address register */

/*
 * Address of indirect internal register to be accessed. A write to this
 * register initiates read or write access to the indirect registers in the
 * HAL2. Note that there af four indirect data registers for write access to
 * registers larger than 16 byte.
 */

#define H2_IAR_TYPE_M		0xF000	/* bits 15:12, type of functional */
					/* block the register resides in */
					/* 1=DMA Port */
					/* 9=Global DMA Control */
					/* 2=Bresenham */
					/* 3=Unix Timer */
#define H2_IAR_NUM_M		0x0F00	/* bits 11:8 instance of the */
					/* blockin which the indirect */
					/* register resides */
					/* If IAR_TYPE_M=DMA Port: */
					/* 1=Synth In */
					/* 2=AES In */
					/* 3=AES Out */
					/* 4=DAC Out */
					/* 5=ADC Out */
					/* 6=Synth Control */
					/* If IAR_TYPE_M=Global DMA Control: */
					/* 1=Control */
					/* If IAR_TYPE_M=Bresenham: */
					/* 1=Bresenham Clock Gen 1 */
					/* 2=Bresenham Clock Gen 2 */
					/* 3=Bresenham Clock Gen 3 */
					/* If IAR_TYPE_M=Unix Timer: */
					/* 1=Unix Timer */
#define H2_IAR_ACCESS_SELECT	0x0080	/* 1=read 0=write */
#define H2_IAR_PARAM		0x000C	/* Parameter Select */
#define H2_IAR_RB_INDEX_M	0x0003	/* Read Back Index */
					/* 00:word0 */
					/* 01:word1 */
					/* 10:word2 */
					/* 11:word3 */
/*
 * HAL2 internal addressing
 *
 * The HAL2 has "indirect registers" (idr) which are accessed by writing to the
 * Indirect Data registers. Write the address to the Indirect Address register
 * to transfer the data.
 *
 * We define the H2IR_* to the read address and H2IW_* to the write address and
 * H2I_* to be fields in whatever register is referred to.
 *
 * When we write to indirect registers which are larger than one word (16 bit)
 * we have to fill more than one indirect register before writing. When we read
 * back however we have to read several times, each time with different Read
 * Back Indexes (there are defs for doing this easily).
 */

/*
 * Relay Control
 */
#define H2I_RELAY_C		0x9100
#define H2I_RELAY_C_STATE	0x01		/* state of RELAY pin signal */

/* DMA port enable */

#define H2I_DMA_PORT_EN		0x9104
#define H2I_DMA_PORT_EN_SY_IN	0x01		/* Synth_in DMA port */
#define H2I_DMA_PORT_EN_AESRX	0x02		/* AES receiver DMA port */
#define H2I_DMA_PORT_EN_AESTX	0x04		/* AES transmitter DMA port */
#define H2I_DMA_PORT_EN_CODECTX	0x08		/* CODEC transmit DMA port */
#define H2I_DMA_PORT_EN_CODECR	0x10		/* CODEC receive DMA port */

#define H2I_DMA_END		0x9108 		/* global dma endian select */
#define H2I_DMA_END_SY_IN	0x01		/* Synth_in DMA port */
#define H2I_DMA_END_AESRX	0x02		/* AES receiver DMA port */
#define H2I_DMA_END_AESTX	0x04		/* AES transmitter DMA port */
#define H2I_DMA_END_CODECTX	0x08		/* CODEC transmit DMA port */
#define H2I_DMA_END_CODECR	0x10		/* CODEC receive DMA port */
						/* 0=b_end 1=l_end */

#define H2I_DMA_DRV		0x910C  	/* global PBUS DMA enable */

#define H2I_SYNTH_C		0x1104		/* Synth DMA control */

#define H2I_AESRX_C		0x1204	 	/* AES RX dma control */
#define H2I_AESRX_C_TS_EN	0x20		/* timestamp enable */
#define H2I_AESRX_C_TS_FMT	0x40		/* timestamp format */
#define H2I_AESRX_C_NAUDIO	0x80		/* PBUS DMA data format */

/* AESRX CTRL, 16 bit */

#define H2I_AESTX_C		0x1304		/* AES TX DMA control */
#define H2I_AESTX_C_CLKID_SHIFT	3		/* Bresenham Clock Gen 1-3 */
#define H2I_AESTX_C_CLKID_M	0x18
#define H2I_AESTX_C_DATAT_SHIFT	8		/* 1=mono 2=stereo (3=quad) */
#define H2I_AESTX_C_DATAT_M	0x300

/* DAC CTRL1, 16 bit */

#define H2I_DAC_C1		0x1404 		/* DAC dma control */
#define H2I_DAC_C1_DMA_SHIFT	0		/* DMA channel */
#define H2I_DAC_C1_DMA_M	0x7
#define H2I_DAC_C1_CLKID_SHIFT	3		/* Bresenham Clock Gen 1-3 */
#define H2I_DAC_C1_CLKID_M	0x18
#define H2I_DAC_C1_DATAT_SHIFT	8		/* 1=mono 2=stereo (3=quad) */
#define H2I_DAC_C1_DATAT_M	0x300

/* DAC CTRL2, 32 bit */

#define H2I_DAC_C2		0x1408
#define H2I_DAC_C2_RGAIN_SHIFT	0		/* right a/d input gain */	
#define H2I_DAC_C2_R_GAIN_M	0xf	
#define H2I_DAC_C2_L_GAIN_SHIFT	4		/* left a/d input gain */
#define H2I_DAC_C2_L_GAIN_M	0xf0
#define H2I_DAC_C2_R_SEL	0x100		/* right input select */
#define H2I_DAC_C2_L_SEL	0x200		/* left input select */
#define H2I_DAC_C2_MUTE		0x400		/* mute */
#define H2I_DAC_C2_DO1		0x10000		/* digital output port bit 0 */
#define H2I_DAC_C2_DO2		0x20000		/* digital output port bit 1 */
#define H2I_DAC_C2_R_ATT_SHIFT	18		/* right a/d output - */
#define H2I_DAC_C2_R_ATT_M	0x7c0000	/* attenuation */
#define H2I_DAC_C2_L_ATT_SHIFT	23		/* left a/d output - */
#define H2I_DAC_C2_L_ATT_M	0x0000f80	/* attenuation */

/* ADC CTRL1, 16 bit */

#define H2I_ADC_C1		0x1504 		/* DAC dma control */
#define H2I_ADC_C1_DMA_SHIFT	0		/* DMA channel */
#define H2I_ADC_C1_DMA_M	0x7
#define H2I_ADC_C1_CLKID_SHIFT	3		/* Bresenham Clock Gen 1-3 */
#define H2I_ADC_C1_CLKID_M	0x18
#define H2I_ADC_C1_DATAT_SHIFT	8		/* 1=mono 2=stereo (3=quad) */
#define H2I_ADC_C1_DATAT_M	0x300

/* ADC CTRL2, 32 bit */

#define H2I_ADC_C2		0x1508
#define H2I_ADC_C2_RGAIN_SHIFT	0		/* right a/d input gain */	
#define H2I_ADC_C2_R_GAIN_M	0xf	
#define H2I_ADC_C2_L_GAIN_SHIFT	4		/* left a/d input gain */
#define H2I_ADC_C2_L_GAIN_M	0xf0
#define H2I_ADC_C2_R_SEL	0x100		/* right input select */
#define H2I_ADC_C2_L_SEL	0x200		/* left input select */
#define H2I_ADC_C2_MUTE		0x400		/* mute */
#define H2I_ADC_C2_DO1		0x10000		/* digital output port bit 0 */
#define H2I_ADC_C2_DO2		0x20000		/* digital output port bit 1 */
#define H2I_ADC_C2_R_ATT_SHIFT	18		/* right a/d output - */
#define H2I_ADC_C2_R_ATT_M	0x7c0000	/* attenuation */
#define H2I_ADC_C2_L_ATT_SHIFT	23		/* left a/d output - */
#define H2I_ADC_C2_L_ATT_M	0x0000f80	/* attenuation */


#define H2I_SYNTH_MAP_C		0x1104		/* synth dma handshake ctrl */

/* Clock generator 1 CTRL 1, 16 bit */

#define H2I_BRES1_C1		0x2104 
#define H2I_BRES1_C1_SHIFT	0		/* 0=48.0 1=44.1 2=aes_rx */
#define H2I_BRES1_C1_M		0x03
				
/* Clock generator 1 CTRL 2, 32 bit */

#define H2I_BRES1_C2		0x2108
#define H2I_BRES1_C2_INC_SHIFT	0		/* increment value */
#define H2I_BRES1_C2_INC_M	0xffff
#define H2I_BRES1_C2_MOD_SHIFT	16		/* modcontrol value */
#define H2I_BRES1_C2_MOD_M	0xffff0000	/* modctrl=0xffff&(modinc-1) */

/* Clock generator 2 CTRL 1, 16 bit */

#define H2I_BRES2_C1		0x2204 
#define H2I_BRES2_C1_SHIFT	0		/* 0=48.0 1=44.1 2=aes_rx */
#define H2I_BRES2_C1_M		0x03
				
/* Clock generator 2 CTRL 2, 32 bit */

#define H2I_BRES2_C2		0x2208
#define H2I_BRES2_C2_INC_SHIFT	0		/* increment value */
#define H2I_BRES2_C2_INC_M	0xffff
#define H2I_BRES2_C2_MOD_SHIFT	16		/* modcontrol value */
#define H2I_BRES2_C2_MOD_M	0xffff0000	/* modctrl=0xffff&(modinc-1) */

/* Clock generator 3 CTRL 1, 16 bit */

#define H2I_BRES3_C1		0x2304 
#define H2I_BRES3_C1_SHIFT	0		/* 0=48.0 1=44.1 2=aes_rx */
#define H2I_BRES3_C1_M		0x03
				
/* Clock generator 3 CTRL 2, 32 bit */

#define H2I_BRES3_C2		0x2308
#define H2I_BRES3_C2_INC_SHIFT	0		/* increment value */
#define H2I_BRES3_C2_INC_M	0xffff
#define H2I_BRES3_C2_MOD_SHIFT	16		/* modcontrol value */
#define H2I_BRES3_C2_MOD_M	0xffff0000	/* modctrl=0xffff&(modinc-1) */

/* Unix timer, 64 bit */

#if 0
#define H2I_UTIME		0x3104
#define H2I_UTIME_0_LD		0xffff		/* microseconds, LSB's */
#define H2I_UTIME_1_LD0		0x0f		/* microseconds, MSB's */
#define H2I_UTIME_1_LD1		0xf0		/* tenths of microseconds */
#define H2I_UTIME_2_LD		0xffff		/* seconds, LSB's */
#define H2I_UTIME_3_LD		0xffff		/* seconds, MSB's */
#endif
