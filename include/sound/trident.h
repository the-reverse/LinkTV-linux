#ifndef __SOUND_TRIDENT_H
#define __SOUND_TRIDENT_H

/*
 *  audio@tridentmicro.com
 *  Fri Feb 19 15:55:28 MST 1999
 *  Definitions for Trident 4DWave DX/NX chips
 *
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include "pcm.h"
#include "mpu401.h"
#include "ac97_codec.h"
#include "seq_midi_emul.h"
#include "seq_device.h"
#include "util_mem.h"
//#include "ainstr_iw.h"
//#include "ainstr_gf1.h"
#include "ainstr_simple.h"

#ifndef PCI_VENDOR_ID_TRIDENT
#define PCI_VENDOR_ID_TRIDENT		0x1023
#endif
#ifndef PCI_DEVICE_ID_TRIDENT_4DWAVE_DX 
#define PCI_DEVICE_ID_TRIDENT_4DWAVE_DX	0x2000
#endif
#ifndef PCI_DEVICE_ID_TRIDENT_4DWAVE_NX 
#define PCI_DEVICE_ID_TRIDENT_4DWAVE_NX	0x2001
#endif

#ifndef PCI_VENDOR_ID_SI
#define PCI_VENDOR_ID_SI		0x1039
#endif
#ifndef PCI_DEVICE_ID_SI_7018
#define PCI_DEVICE_ID_SI_7018		0x7018
#endif

#define TRIDENT_DEVICE_ID_DX		((PCI_VENDOR_ID_TRIDENT<<16)|PCI_DEVICE_ID_TRIDENT_4DWAVE_DX)
#define TRIDENT_DEVICE_ID_NX		((PCI_VENDOR_ID_TRIDENT<<16)|PCI_DEVICE_ID_TRIDENT_4DWAVE_NX)
#define TRIDENT_DEVICE_ID_SI7018	((PCI_VENDOR_ID_SI<<16)|PCI_DEVICE_ID_SI_7018)

#define SNDRV_SEQ_DEV_ID_TRIDENT			"trident-synth"

#define SNDRV_TRIDENT_VOICE_TYPE_PCM		0
#define SNDRV_TRIDENT_VOICE_TYPE_SYNTH		1
#define SNDRV_TRIDENT_VOICE_TYPE_MIDI		2

#define SNDRV_TRIDENT_VFLG_RUNNING		(1<<0)

/* TLB code constants */
#define SNDRV_TRIDENT_PAGE_SIZE			4096
#define SNDRV_TRIDENT_PAGE_SHIFT			12
#define SNDRV_TRIDENT_PAGE_MASK			((1<<SNDRV_TRIDENT_PAGE_SHIFT)-1)
#define SNDRV_TRIDENT_MAX_PAGES			4096

/*
 * Direct registers
 */

#define TRID_REG(trident, x) ((trident)->port + (x))

#define ID_4DWAVE_DX        0x2000
#define ID_4DWAVE_NX        0x2001

/* Bank definitions */

#define T4D_BANK_A	0
#define T4D_BANK_B	1
#define T4D_NUM_BANKS	2

/* Register definitions */

/* Global registers */

enum global_control_bits {
	CHANNEL_IDX	= 0x0000003f,
	OVERRUN_IE	= 0x00000400,	/* interrupt enable: capture overrun */
	UNDERRUN_IE	= 0x00000800,	/* interrupt enable: playback underrun */
	ENDLP_IE	= 0x00001000,	/* interrupt enable: end of buffer */
	MIDLP_IE	= 0x00002000,	/* interrupt enable: middle buffer */
	ETOG_IE		= 0x00004000,	/* interrupt enable: envelope toggling */
	EDROP_IE	= 0x00008000,	/* interrupt enable: envelope drop */
	BANK_B_EN	= 0x00010000,	/* SiS: enable bank B (64 channels) */
	PCMIN_B_MIX	= 0x00020000,	/* SiS: PCM IN B mixing enable */
	I2S_OUT_ASSIGN	= 0x00040000,	/* SiS: I2S Out contains surround PCM */
	SPDIF_OUT_ASSIGN= 0x00080000,	/* SiS: 0=S/PDIF L/R | 1=PCM Out FIFO */
	MAIN_OUT_ASSIGN = 0x00100000,	/* SiS: 0=PCM Out FIFO | 1=MMC Out buffer */
};

enum miscint_bits {
	PB_UNDERRUN_IRQ = 0x00000001, REC_OVERRUN_IRQ = 0x00000002,
	SB_IRQ		= 0x00000004, MPU401_IRQ      = 0x00000008,
	OPL3_IRQ        = 0x00000010, ADDRESS_IRQ     = 0x00000020,
	ENVELOPE_IRQ    = 0x00000040, PB_UNDERRUN     = 0x00000100,
	REC_OVERRUN	= 0x00000200, MIXER_UNDERFLOW = 0x00000400,
	MIXER_OVERFLOW  = 0x00000800, NX_SB_IRQ_DISABLE = 0x00001000,
        ST_TARGET_REACHED = 0x00008000,
	PB_24K_MODE     = 0x00010000, ST_IRQ_EN       = 0x00800000,
	ACGPIO_IRQ	= 0x01000000
};

/* T2 legacy dma control registers. */
#define LEGACY_DMAR0                0x00  // ADR0
#define LEGACY_DMAR4                0x04  // CNT0
#define LEGACY_DMAR6		    0x06  // CNT0 - High bits
#define LEGACY_DMAR11               0x0b  // MOD 
#define LEGACY_DMAR15               0x0f  // MMR 

#define T4D_START_A		     0x80
#define T4D_STOP_A		     0x84
#define T4D_DLY_A		     0x88
#define T4D_SIGN_CSO_A		     0x8c
#define T4D_CSPF_A		     0x90
#define T4D_CSPF_B		     0xbc
#define T4D_CEBC_A		     0x94
#define T4D_AINT_A		     0x98
#define T4D_AINTEN_A		     0x9c
#define T4D_LFO_GC_CIR               0xa0
#define T4D_MUSICVOL_WAVEVOL         0xa8
#define T4D_SBDELTA_DELTA_R          0xac
#define T4D_MISCINT                  0xb0
#define T4D_START_B                  0xb4
#define T4D_STOP_B                   0xb8
#define T4D_SBBL_SBCL                0xc0
#define T4D_SBCTRL_SBE2R_SBDD        0xc4
#define T4D_STIMER		     0xc8
#define T4D_AINT_B                   0xd8
#define T4D_AINTEN_B                 0xdc
#define T4D_RCI                      0x70

/* MPU-401 UART */
#define T4D_MPU401_BASE             0x20
#define T4D_MPUR0                   0x20
#define T4D_MPUR1                   0x21
#define T4D_MPUR2                   0x22
#define T4D_MPUR3                   0x23

/* S/PDIF Registers */
#define NX_SPCTRL_SPCSO             0x24
#define NX_SPLBA                    0x28
#define NX_SPESO                    0x2c
#define NX_SPCSTATUS                0x64

/* Joystick */
#define GAMEPORT_GCR                0x30
#define GAMEPORT_MODE_ADC           0x80
#define GAMEPORT_LEGACY             0x31
#define GAMEPORT_AXES               0x34

/* NX Specific Registers */
#define NX_TLBC                     0x6c

/* Channel Registers */

#define CH_START		    0xe0

#define CH_DX_CSO_ALPHA_FMS         0xe0
#define CH_DX_ESO_DELTA             0xe8
#define CH_DX_FMC_RVOL_CVOL         0xec

#define CH_NX_DELTA_CSO             0xe0
#define CH_NX_DELTA_ESO             0xe8
#define CH_NX_ALPHA_FMS_FMC_RVOL_CVOL 0xec

#define CH_LBA                      0xe4
#define CH_GVSEL_PAN_VOL_CTRL_EC    0xf0
#define CH_EBUF1                    0xf4
#define CH_EBUF2                    0xf8

/* AC-97 Registers */

#define DX_ACR0_AC97_W              0x40
#define DX_ACR1_AC97_R              0x44
#define DX_ACR2_AC97_COM_STAT       0x48

#define NX_ACR0_AC97_COM_STAT       0x40
#define NX_ACR1_AC97_W              0x44
#define NX_ACR2_AC97_R_PRIMARY      0x48
#define NX_ACR3_AC97_R_SECONDARY    0x4c

#define SI_AC97_WRITE		    0x40
#define SI_AC97_READ		    0x44
#define SI_SERIAL_INTF_CTRL	    0x48
#define SI_AC97_GPIO		    0x4c
#define SI_ASR0			    0x50
#define SI_SPDIF_CS		    0x70
#define SI_GPIO			    0x7c

enum trident_nx_ac97_bits {
	/* ACR1-3 */
	NX_AC97_BUSY_WRITE 	= 0x0800,
	NX_AC97_BUSY_READ	= 0x0800,
	NX_AC97_BUSY_DATA 	= 0x0400,
	NX_AC97_WRITE_SECONDARY = 0x0100,
	/* ACR0 */
	NX_AC97_SECONDARY_READY = 0x0040,
	NX_AC97_SECONDARY_RECORD = 0x0020,
	NX_AC97_SURROUND_OUTPUT = 0x0010,
	NX_AC97_PRIMARY_READY	= 0x0008,
	NX_AC97_PRIMARY_RECORD	= 0x0004,
	NX_AC97_PCM_OUTPUT	= 0x0002,
	NX_AC97_WARM_RESET	= 0x0001
};

enum trident_dx_ac97_bits {
	DX_AC97_BUSY_WRITE	= 0x8000,
	DX_AC97_BUSY_READ	= 0x8000,
	DX_AC97_READY		= 0x0010,
	DX_AC97_RECORD		= 0x0008,
	DX_AC97_PLAYBACK	= 0x0002
};

enum sis7018_ac97_bits {
	SI_AC97_BUSY_WRITE =	0x00008000,
	SI_AC97_AUDIO_BUSY =	0x00004000,
	SI_AC97_MODEM_BUSY =	0x00002000,
	SI_AC97_BUSY_READ =	0x00008000,
	SI_AC97_SECONDARY =	0x00000080,
};

enum serial_intf_ctrl_bits {
	WARM_RESET	= 0x00000001,
	COLD_RESET	= 0x00000002,
	I2S_CLOCK	= 0x00000004,
	PCM_SEC_AC97	= 0x00000008,
	AC97_DBL_RATE	= 0x00000010,
	SPDIF_EN	= 0x00000020,
	I2S_OUTPUT_EN	= 0x00000040,
	I2S_INPUT_EN	= 0x00000080,
	PCMIN		= 0x00000100,
	LINE1IN		= 0x00000200,
	MICIN		= 0x00000400,
	LINE2IN		= 0x00000800,
	HEAD_SET_IN	= 0x00001000,
	GPIOIN		= 0x00002000,
	/* 7018 spec says id = 01 but the demo board routed to 10
	   SECONDARY_ID= 0x00004000, */
	SECONDARY_ID	= 0x00004000,
	PCMOUT		= 0x00010000,
	SURROUT		= 0x00020000,
	CENTEROUT	= 0x00040000,
	LFEOUT		= 0x00080000,
	LINE1OUT	= 0x00100000,
	LINE2OUT	= 0x00200000,
	GPIOOUT		= 0x00400000,
	SI_AC97_PRIMARY_READY = 0x01000000,
	SI_AC97_SECONDARY_READY = 0x02000000,
	SI_AC97_POWERDOWN = 0x04000000,
};
                                                                                                                                   
/* PCM defaults */

#define T4D_DEFAULT_PCM_VOL	10	/* 0 - 255 */
#define T4D_DEFAULT_PCM_PAN	0	/* 0 - 127 */
#define T4D_DEFAULT_PCM_RVOL	127	/* 0 - 127 */
#define T4D_DEFAULT_PCM_CVOL	127	/* 0 - 127 */

typedef struct _snd_trident trident_t;
typedef struct _snd_trident_voice snd_trident_voice_t;
typedef struct _snd_trident_pcm_mixer snd_trident_pcm_mixer_t;

typedef struct {
	void (*sample_start)(trident_t *gus, snd_trident_voice_t *voice, snd_seq_position_t position);
	void (*sample_stop)(trident_t *gus, snd_trident_voice_t *voice, snd_seq_stop_mode_t mode);
	void (*sample_freq)(trident_t *gus, snd_trident_voice_t *voice, snd_seq_frequency_t freq);
	void (*sample_volume)(trident_t *gus, snd_trident_voice_t *voice, snd_seq_ev_volume_t *volume);
	void (*sample_loop)(trident_t *card, snd_trident_voice_t *voice, snd_seq_ev_loop_t *loop);
	void (*sample_pos)(trident_t *card, snd_trident_voice_t *voice, snd_seq_position_t position);
	void (*sample_private1)(trident_t *card, snd_trident_voice_t *voice, unsigned char *data);
} snd_trident_sample_ops_t;

typedef struct {
	snd_midi_channel_set_t * chset;
	trident_t * trident;
	int mode;		/* operation mode */
	int client;		/* sequencer client number */
	int port;		/* sequencer port number */
	int midi_has_voices: 1;
} snd_trident_port_t;

typedef struct snd_trident_memblk_arg {
	short first_page, last_page;
} snd_trident_memblk_arg_t;

typedef struct {
	unsigned int * entries;		/* 16k-aligned TLB table */
	dma_addr_t entries_dmaaddr;	/* 16k-aligned PCI address to TLB table */
	unsigned long * shadow_entries;	/* shadow entries with virtual addresses */
	struct snd_dma_buffer buffer;
	snd_util_memhdr_t * memhdr;	/* page allocation list */
	struct snd_dma_buffer silent_page;
} snd_trident_tlb_t;

struct _snd_trident_voice {
	unsigned int number;
	int use: 1,
	    pcm: 1,
	    synth:1,
	    midi: 1;
	unsigned int flags;
	unsigned char client;
	unsigned char port;
	unsigned char index;

	snd_seq_instr_t instr;
	snd_trident_sample_ops_t *sample_ops;

	/* channel parameters */
	unsigned int CSO;		/* 24 bits (16 on DX) */
	unsigned int ESO;		/* 24 bits (16 on DX) */
	unsigned int LBA;		/* 30 bits */
	unsigned short EC;		/* 12 bits */
	unsigned short Alpha;		/* 12 bits */
	unsigned short Delta;		/* 16 bits */
	unsigned short Attribute;	/* 16 bits - SiS 7018 */
	unsigned short Vol;		/* 12 bits (6.6) */
	unsigned char Pan;		/* 7 bits (1.4.2) */
	unsigned char GVSel;		/* 1 bit */
	unsigned char RVol;		/* 7 bits (5.2) */
	unsigned char CVol;		/* 7 bits (5.2) */
	unsigned char FMC;		/* 2 bits */
	unsigned char CTRL;		/* 4 bits */
	unsigned char FMS;		/* 4 bits */
	unsigned char LFO;		/* 8 bits */

	unsigned int negCSO;	/* nonzero - use negative CSO */

	snd_util_memblk_t *memblk;	/* memory block if TLB enabled */

	/* PCM data */

	trident_t *trident;
	snd_pcm_substream_t *substream;
	snd_trident_voice_t *extra;	/* extra PCM voice (acts as interrupt generator) */
	int running: 1,
            capture: 1,
            spdif: 1,
            foldback: 1,
            isync: 1,
            isync2: 1,
            isync3: 1;
	int foldback_chan;		/* foldback subdevice number */
	unsigned int stimer;		/* global sample timer (to detect spurious interrupts) */
	unsigned int spurious_threshold; /* spurious threshold */
	unsigned int isync_mark;
	unsigned int isync_max;
	unsigned int isync_ESO;

	/* --- */

	void *private_data;
	void (*private_free)(snd_trident_voice_t *voice);
};

struct _snd_4dwave {
	int seq_client;

	snd_trident_port_t seq_ports[4];
	snd_simple_ops_t simple_ops;
	snd_seq_kinstr_list_t *ilist;

	snd_trident_voice_t voices[64];	

	int ChanSynthCount;		/* number of allocated synth channels */
	int max_size;			/* maximum synth memory size in bytes */
	int current_size;		/* current allocated synth mem in bytes */
};

struct _snd_trident_pcm_mixer {
	snd_trident_voice_t *voice;	/* active voice */
	unsigned short vol;		/* front volume */
	unsigned char pan;		/* pan control */
	unsigned char rvol;		/* rear volume */
	unsigned char cvol;		/* center volume */
	unsigned char pad;
};

struct _snd_trident {
	int irq;

	unsigned int device;	/* device ID */

        unsigned char  bDMAStart;

	unsigned long port;
	unsigned long midi_port;

	unsigned int spurious_irq_count;
	unsigned int spurious_irq_max_delta;

        snd_trident_tlb_t tlb;	/* TLB entries for NX cards */

	unsigned char spdif_ctrl;
	unsigned char spdif_pcm_ctrl;
	unsigned int spdif_bits;
	unsigned int spdif_pcm_bits;
	snd_kcontrol_t *spdif_pcm_ctl;	/* S/PDIF settings */
	unsigned int ac97_ctrl;
        
        unsigned int ChanMap[2];	/* allocation map for hardware channels */
        
        int ChanPCM;			/* max number of PCM channels */
	int ChanPCMcnt;			/* actual number of PCM channels */

	unsigned int ac97_detect: 1;	/* 1 = AC97 in detection phase */
	unsigned int in_suspend: 1;	/* 1 during suspend/resume */

	struct _snd_4dwave synth;	/* synth specific variables */

	spinlock_t event_lock;
	spinlock_t voice_alloc;

	struct snd_dma_device dma_dev;

	struct pci_dev *pci;
	snd_card_t *card;
	snd_pcm_t *pcm;		/* ADC/DAC PCM */
	snd_pcm_t *foldback;	/* Foldback PCM */
	snd_pcm_t *spdif;	/* SPDIF PCM */
	snd_rawmidi_t *rmidi;
	snd_seq_device_t *seq_dev;

	ac97_bus_t *ac97_bus;
	ac97_t *ac97;
	ac97_t *ac97_sec;

	unsigned int musicvol_wavevol;
	snd_trident_pcm_mixer_t pcm_mixer[32];
	snd_kcontrol_t *ctl_vol;	/* front volume */
	snd_kcontrol_t *ctl_pan;	/* pan */
	snd_kcontrol_t *ctl_rvol;	/* rear volume */
	snd_kcontrol_t *ctl_cvol;	/* center volume */

	spinlock_t reg_lock;

	struct snd_trident_gameport *gameport;
};

int snd_trident_create(snd_card_t * card,
		       struct pci_dev *pci,
		       int pcm_streams,
		       int pcm_spdif_device,
		       int max_wavetable_size,
		       trident_t ** rtrident);
int snd_trident_free(trident_t *trident);
void snd_trident_gameport(trident_t *trident);

int snd_trident_pcm(trident_t * trident, int device, snd_pcm_t **rpcm);
int snd_trident_foldback_pcm(trident_t * trident, int device, snd_pcm_t **rpcm);
int snd_trident_spdif_pcm(trident_t * trident, int device, snd_pcm_t **rpcm);
int snd_trident_attach_synthesizer(trident_t * trident);
int snd_trident_detach_synthesizer(trident_t * trident);
snd_trident_voice_t *snd_trident_alloc_voice(trident_t * trident, int type, int client, int port);
void snd_trident_free_voice(trident_t * trident, snd_trident_voice_t *voice);
void snd_trident_start_voice(trident_t * trident, unsigned int voice);
void snd_trident_stop_voice(trident_t * trident, unsigned int voice);
void snd_trident_write_voice_regs(trident_t * trident, snd_trident_voice_t *voice);
void snd_trident_clear_voices(trident_t * trident, unsigned short v_min, unsigned short v_max);

/* TLB memory allocation */
snd_util_memblk_t *snd_trident_alloc_pages(trident_t *trident, snd_pcm_substream_t *substream);
int snd_trident_free_pages(trident_t *trident, snd_util_memblk_t *blk);
snd_util_memblk_t *snd_trident_synth_alloc(trident_t *trident, unsigned int size);
int snd_trident_synth_free(trident_t *trident, snd_util_memblk_t *blk);
int snd_trident_synth_bzero(trident_t *trident, snd_util_memblk_t *blk, int offset, int size);
int snd_trident_synth_copy_from_user(trident_t *trident, snd_util_memblk_t *blk, int offset, const char __user *data, int size);

#endif /* __SOUND_TRIDENT_H */
