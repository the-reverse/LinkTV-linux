/*
 * The Linux soundcard driver for 1200 baud and 9600 baud packet radio
 * (C) 1996 by Thomas Sailer, HB9JNX/AE4WA
 */

#ifndef _SOUNDMODEM_H
#define _SOUNDMODEM_H

#include <linux/sockios.h>
#include <linux/if_ether.h>

/* -------------------------------------------------------------------- */
/*
 * structs for the IOCTL commands
 */

struct sm_debug_data {
	unsigned long debug1;
	unsigned long debug2;
	long debug3;
};

struct sm_diag_data {
	unsigned int mode;
	unsigned int flags;
	unsigned int samplesperbit;
	unsigned int datalen;
	short *data;
};

struct sm_mixer_data {
	unsigned int mixer_type;
	unsigned int sample_rate;
	unsigned int bit_rate;
	unsigned int reg;
	unsigned int data;
};

struct sm_config {
	int hardware;
	int mode;
};

struct sm_ioctl {
	int cmd;
	union {
		struct sm_config cfg;
		struct sm_diag_data diag;	
		struct sm_mixer_data mix;
		struct sm_debug_data dbg;
	} data;
};

/* -------------------------------------------------------------------- */

/*
 * config: hardware
 */
#define SM_HARDWARE_INVALID   -1
#define SM_HARDWARE_SBC       0
#define SM_HARDWARE_WSS       1
#define SM_HARDWARE_WSSFDX    2  /* currently does not work! */

/*
 * config: mode
 */
#define SM_MODE_INVALID       -1
#define SM_MODE_AFSK1200      0
#define SM_MODE_FSK9600       1

/*
 * diagnose modes
 */
#define SM_DIAGMODE_OFF        0
#define SM_DIAGMODE_INPUT      1
#define SM_DIAGMODE_DEMOD      2

/*
 * diagnose flags
 */
#define SM_DIAGFLAG_DCDGATE    (1<<0)
#define SM_DIAGFLAG_VALID      (1<<1)

/*
 * mixer types
 */
#define SM_MIXER_INVALID       0
#define SM_MIXER_AD1848        0x10
#define SM_MIXER_CT1335        0x20
#define SM_MIXER_CT1345        0x21
#define SM_MIXER_CT1745        0x22

/*
 * ioctl values
 */
#define SMCTL_GETMODEMTYPE     0x80
#define SMCTL_SETMODEMTYPE     0x81
#define SMCTL_DIAGNOSE         0x82
#define SMCTL_GETMIXER         0x83
#define SMCTL_SETMIXER         0x84
#define SMCTL_GETDEBUG         0x85

/* -------------------------------------------------------------------- */

#endif /* _SOUNDMODEM_H */

/* --------------------------------------------------------------------- */
