/*
 *  Mars soundcard
 *  Copyright (c) by Jaroslav Kysela <perex@suse.cz>
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

#include <sound/driver.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/moduleparam.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/pcm.h>
#include <sound/rawmidi.h>
#include <sound/initval.h>

MODULE_AUTHOR("EJ Hsu <ejhsu@realtek.com.tw>");
MODULE_DESCRIPTION("Mars soundcard");
MODULE_LICENSE("GPL");
MODULE_SUPPORTED_DEVICE("{{ALSA,Mars soundcard}}");

#define MAX_PCM_DEVICES		4
#define MAX_PCM_SUBSTREAMS	16
#define MAX_MIDI_DEVICES	2

#if 0 /* emu10k1 emulation */
#define MAX_BUFFER_SIZE		(128 * 1024)
static int emu10k1_playback_constraints(snd_pcm_runtime_t *runtime)
{
	int err;
	if ((err = snd_pcm_hw_constraint_integer(runtime, SNDRV_PCM_HW_PARAM_PERIODS)) < 0)
		return err;
	if ((err = snd_pcm_hw_constraint_minmax(runtime, SNDRV_PCM_HW_PARAM_BUFFER_BYTES, 256, UINT_MAX)) < 0)
		return err;
	return 0;
}
#define add_playback_constraints emu10k1_playback_constraints
#endif

#if 0 /* RME9652 emulation */
#define MAX_BUFFER_SIZE		(26 * 64 * 1024)
#define USE_FORMATS		SNDRV_PCM_FMTBIT_S32_LE
#define USE_CHANNELS_MIN	26
#define USE_CHANNELS_MAX	26
#define USE_PERIODS_MIN		2
#define USE_PERIODS_MAX		2
#endif

#if 0 /* ICE1712 emulation */
#define MAX_BUFFER_SIZE		(256 * 1024)
#define USE_FORMATS		SNDRV_PCM_FMTBIT_S32_LE
#define USE_CHANNELS_MIN	10
#define USE_CHANNELS_MAX	10
#define USE_PERIODS_MIN		1
#define USE_PERIODS_MAX		1024
#endif

#if 0 /* UDA1341 emulation */
#define MAX_BUFFER_SIZE		(16380)
#define USE_FORMATS		SNDRV_PCM_FMTBIT_S16_LE
#define USE_CHANNELS_MIN	2
#define USE_CHANNELS_MAX	2
#define USE_PERIODS_MIN		2
#define USE_PERIODS_MAX		255
#endif

#if 0 /* simple AC97 bridge (intel8x0) with 48kHz AC97 only codec */
#define USE_FORMATS		SNDRV_PCM_FMTBIT_S16_LE
#define USE_CHANNELS_MIN	2
#define USE_CHANNELS_MAX	2
#define USE_RATE		SNDRV_PCM_RATE_48000
#define USE_RATE_MIN		48000
#define USE_RATE_MAX		48000
#endif


/* defaults */
#ifndef MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE		(64*1024)
#endif
#ifndef USE_FORMATS
#define USE_FORMATS 		(SNDRV_PCM_FMTBIT_U8 | SNDRV_PCM_FMTBIT_S16_LE)
#endif
#ifndef USE_RATE
#define USE_RATE		SNDRV_PCM_RATE_CONTINUOUS | SNDRV_PCM_RATE_8000_48000
#define USE_RATE_MIN		5500
#define USE_RATE_MAX		48000
#endif
#ifndef USE_CHANNELS_MIN
#define USE_CHANNELS_MIN 	1
#endif
#ifndef USE_CHANNELS_MAX
#define USE_CHANNELS_MAX 	2
#endif
#ifndef USE_PERIODS_MIN
#define USE_PERIODS_MIN 	1
#endif
#ifndef USE_PERIODS_MAX
#define USE_PERIODS_MAX 	1024
#endif
#ifndef add_playback_constraints
#define add_playback_constraints(x) 0
#endif
#ifndef add_capture_constraints
#define add_capture_constraints(x) 0
#endif

// EJ: device configuration
static int index[SNDRV_CARDS] = SNDRV_DEFAULT_IDX;	/* Index 0-MAX */
static char *id[SNDRV_CARDS] = SNDRV_DEFAULT_STR;	/* ID for this card */
static int enable[SNDRV_CARDS] = {1, [1 ... (SNDRV_CARDS - 1)] = 0};
static int pcm_devs[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 1};
static int pcm_substreams[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 8};
//static int midi_devs[SNDRV_CARDS] = {[0 ... (SNDRV_CARDS - 1)] = 2};

module_param_array(index, int, NULL, 0444);
MODULE_PARM_DESC(index, "Index value for mars soundcard.");
module_param_array(id, charp, NULL, 0444);
MODULE_PARM_DESC(id, "ID string for mars soundcard.");
module_param_array(enable, bool, NULL, 0444);
MODULE_PARM_DESC(enable, "Enable this mars soundcard.");
module_param_array(pcm_devs, int, NULL, 0444);
MODULE_PARM_DESC(pcm_devs, "PCM devices # (0-4) for mars driver.");
module_param_array(pcm_substreams, int, NULL, 0444);
MODULE_PARM_DESC(pcm_substreams, "PCM substreams # (1-16) for mars driver.");
//module_param_array(midi_devs, int, NULL, 0444);
//MODULE_PARM_DESC(midi_devs, "MIDI devices # (0-2) for mars driver.");

#define MIXER_ADDR_MASTER	0
#define MIXER_ADDR_LINE		1
#define MIXER_ADDR_MIC		2
#define MIXER_ADDR_SYNTH	3
#define MIXER_ADDR_CD		4
#define MIXER_ADDR_LAST		4

typedef struct snd_card_mars {
	snd_card_t *card;
	spinlock_t mixer_lock;
	int mixer_volume[MIXER_ADDR_LAST+1][2];
	int capture_source[MIXER_ADDR_LAST+1][2];
} snd_card_mars_t;

typedef struct snd_card_mars_pcm {
	snd_card_mars_t *mars;
	spinlock_t lock;
	struct timer_list timer;
	unsigned int pcm_size;		/* buffer sizze */
	unsigned int pcm_count;		/* period length */
	unsigned int pcm_bps;		/* bytes per second */
	unsigned int pcm_jiffie;	/* bytes per one jiffie */
	unsigned int pcm_irq_pos;	/* IRQ position */
	unsigned int pcm_buf_pos;	/* position in buffer */
	snd_pcm_substream_t *substream;
} snd_card_mars_pcm_t;

static snd_card_t *snd_mars_cards[SNDRV_CARDS] = SNDRV_DEFAULT_PTR;


static void snd_card_mars_pcm_timer_start(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_mars_pcm_t *dpcm = runtime->private_data;

	dpcm->timer.expires = 1 + jiffies;
	add_timer(&dpcm->timer);
}

static void snd_card_mars_pcm_timer_stop(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_mars_pcm_t *dpcm = runtime->private_data;

	del_timer(&dpcm->timer);
}

static int snd_card_mars_playback_trigger(snd_pcm_substream_t * substream,
					   int cmd)
{
	if (cmd == SNDRV_PCM_TRIGGER_START) {
		snd_card_mars_pcm_timer_start(substream);
	} else if (cmd == SNDRV_PCM_TRIGGER_STOP) {
		snd_card_mars_pcm_timer_stop(substream);
	} else {
		return -EINVAL;
	}
	return 0;
}

static int snd_card_mars_capture_trigger(snd_pcm_substream_t * substream,
					  int cmd)
{
	if (cmd == SNDRV_PCM_TRIGGER_START) {
		snd_card_mars_pcm_timer_start(substream);
	} else if (cmd == SNDRV_PCM_TRIGGER_STOP) {
		snd_card_mars_pcm_timer_stop(substream);
	} else {
		return -EINVAL;
	}
	return 0;
}

static int snd_card_mars_pcm_prepare(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_mars_pcm_t *dpcm = runtime->private_data;
	unsigned int bps;

	bps = runtime->rate * runtime->channels;
	bps *= snd_pcm_format_width(runtime->format);
	bps /= 8;
	if (bps <= 0)
		return -EINVAL;
	dpcm->pcm_bps = bps;
	dpcm->pcm_jiffie = runtime->rate / HZ;
	dpcm->pcm_size = snd_pcm_lib_buffer_bytes(substream);
	dpcm->pcm_count = snd_pcm_lib_period_bytes(substream);
//	printk("%s jiffie: %d period: %d buffer: %d POS: %d \n", __FUNCTION__, dpcm->pcm_jiffie, 
//			dpcm->pcm_count, dpcm->pcm_size, dpcm->pcm_buf_pos);
	dpcm->pcm_irq_pos = 0;
	dpcm->pcm_buf_pos = 0;
	return 0;
}

static int snd_card_mars_playback_prepare(snd_pcm_substream_t * substream)
{
	return snd_card_mars_pcm_prepare(substream);
}

static int snd_card_mars_capture_prepare(snd_pcm_substream_t * substream)
{
	return snd_card_mars_pcm_prepare(substream);
}

static void snd_card_mars_pcm_timer_function(unsigned long data)
{
	snd_card_mars_pcm_t *dpcm = (snd_card_mars_pcm_t *)data;
	snd_pcm_substream_t *substream = dpcm->substream;
	snd_pcm_runtime_t *runtime = substream->runtime;
	int left;
	
	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		left = runtime->control->appl_ptr - dpcm->pcm_buf_pos;
		if (left < 0)
			left += runtime->boundary;
		if (left > dpcm->pcm_jiffie)
			left = dpcm->pcm_jiffie;
	} else
		left = dpcm->pcm_jiffie;

	dpcm->timer.expires = 1 + jiffies;
	add_timer(&dpcm->timer);
	spin_lock_irq(&dpcm->lock);
	dpcm->pcm_irq_pos += left;
	dpcm->pcm_buf_pos += left;
	dpcm->pcm_buf_pos %= runtime->boundary;
	if (dpcm->pcm_irq_pos >= runtime->period_size) {
		dpcm->pcm_irq_pos %= runtime->period_size;
		snd_pcm_period_elapsed(dpcm->substream);
	}
	spin_unlock_irq(&dpcm->lock);
}

static snd_pcm_uframes_t snd_card_mars_playback_pointer(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_mars_pcm_t *dpcm = runtime->private_data;

	return dpcm->pcm_buf_pos % runtime->buffer_size;
}

static snd_pcm_uframes_t snd_card_mars_capture_pointer(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_mars_pcm_t *dpcm = runtime->private_data;

	return dpcm->pcm_buf_pos % runtime->buffer_size;
}

static snd_pcm_hardware_t snd_card_mars_playback =
{
	.info =			(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		USE_FORMATS,
	.rates =		USE_RATE,
	.rate_min =		USE_RATE_MIN,
	.rate_max =		USE_RATE_MAX,
	.channels_min =		USE_CHANNELS_MIN,
	.channels_max =		USE_CHANNELS_MAX,
	.buffer_bytes_max =	MAX_BUFFER_SIZE,
	.period_bytes_min =	64,
	.period_bytes_max =	MAX_BUFFER_SIZE,
	.periods_min =		USE_PERIODS_MIN,
	.periods_max =		USE_PERIODS_MAX,
	.fifo_size =		0,
};

static snd_pcm_hardware_t snd_card_mars_capture =
{
	.info =			(SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_INTERLEAVED |
				 SNDRV_PCM_INFO_MMAP_VALID),
	.formats =		USE_FORMATS,
	.rates =		USE_RATE,
	.rate_min =		USE_RATE_MIN,
	.rate_max =		USE_RATE_MAX,
	.channels_min =		USE_CHANNELS_MIN,
	.channels_max =		USE_CHANNELS_MAX,
	.buffer_bytes_max =	MAX_BUFFER_SIZE,
	.period_bytes_min =	64,
	.period_bytes_max =	MAX_BUFFER_SIZE,
	.periods_min =		USE_PERIODS_MIN,
	.periods_max =		USE_PERIODS_MAX,
	.fifo_size =		0,
};

static void snd_card_mars_runtime_free(snd_pcm_runtime_t *runtime)
{
	snd_card_mars_pcm_t *dpcm = runtime->private_data;
	kfree(dpcm);
}

static int snd_card_mars_hw_params(snd_pcm_substream_t * substream,
				    snd_pcm_hw_params_t * hw_params)
{
	return snd_pcm_lib_malloc_pages(substream, params_buffer_bytes(hw_params));
}

static int snd_card_mars_hw_free(snd_pcm_substream_t * substream)
{
	return snd_pcm_lib_free_pages(substream);
}

static int snd_card_mars_playback_open(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_mars_pcm_t *dpcm;
	int err;

	dpcm = kcalloc(1, sizeof(*dpcm), GFP_KERNEL);
	if (dpcm == NULL)
		return -ENOMEM;
	init_timer(&dpcm->timer);
	dpcm->timer.data = (unsigned long) dpcm;
	dpcm->timer.function = snd_card_mars_pcm_timer_function;
	spin_lock_init(&dpcm->lock);
	dpcm->substream = substream;
	runtime->private_data = dpcm;
	runtime->private_free = snd_card_mars_runtime_free;
	runtime->hw = snd_card_mars_playback;
//	if (substream->pcm->device & 1) {
//		runtime->hw.info &= ~SNDRV_PCM_INFO_INTERLEAVED;
//		runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;
//	}
//	if (substream->pcm->device & 2)
//		runtime->hw.info &= ~(SNDRV_PCM_INFO_MMAP|SNDRV_PCM_INFO_MMAP_VALID);
	if ((err = add_playback_constraints(runtime)) < 0) {
		kfree(dpcm);
		return err;
	}

	return 0;
}

static int snd_card_mars_capture_open(snd_pcm_substream_t * substream)
{
	snd_pcm_runtime_t *runtime = substream->runtime;
	snd_card_mars_pcm_t *dpcm;
	int err;

	dpcm = kcalloc(1, sizeof(*dpcm), GFP_KERNEL);
	if (dpcm == NULL)
		return -ENOMEM;
	init_timer(&dpcm->timer);
	dpcm->timer.data = (unsigned long) dpcm;
	dpcm->timer.function = snd_card_mars_pcm_timer_function;
	spin_lock_init(&dpcm->lock);
	dpcm->substream = substream;
	runtime->private_data = dpcm;
	runtime->private_free = snd_card_mars_runtime_free;
	runtime->hw = snd_card_mars_capture;
//	if (substream->pcm->device == 1) {
//		runtime->hw.info &= ~SNDRV_PCM_INFO_INTERLEAVED;
//		runtime->hw.info |= SNDRV_PCM_INFO_NONINTERLEAVED;
//	}
//	if (substream->pcm->device & 2)
//		runtime->hw.info &= ~(SNDRV_PCM_INFO_MMAP|SNDRV_PCM_INFO_MMAP_VALID);
	if ((err = add_capture_constraints(runtime)) < 0) {
		kfree(dpcm);
		return err;
	}

	return 0;
}

static int snd_card_mars_playback_close(snd_pcm_substream_t * substream)
{
	return 0;
}

static int snd_card_mars_capture_close(snd_pcm_substream_t * substream)
{
	return 0;
}

static snd_pcm_ops_t snd_card_mars_playback_ops = {
	.open =			snd_card_mars_playback_open,
	.close =		snd_card_mars_playback_close,
	.ioctl =		snd_pcm_lib_ioctl,
	.hw_params =		snd_card_mars_hw_params,
	.hw_free =		snd_card_mars_hw_free,
	.prepare =		snd_card_mars_playback_prepare,
	.trigger =		snd_card_mars_playback_trigger,
	.pointer =		snd_card_mars_playback_pointer,
};

static snd_pcm_ops_t snd_card_mars_capture_ops = {
	.open =			snd_card_mars_capture_open,
	.close =		snd_card_mars_capture_close,
	.ioctl =		snd_pcm_lib_ioctl,
	.hw_params =		snd_card_mars_hw_params,
	.hw_free =		snd_card_mars_hw_free,
	.prepare =		snd_card_mars_capture_prepare,
	.trigger =		snd_card_mars_capture_trigger,
	.pointer =		snd_card_mars_capture_pointer,
};

static int __init snd_card_mars_pcm(snd_card_mars_t *mars, int device, int substreams)
{
	snd_pcm_t *pcm;
	int err;

	if ((err = snd_pcm_new(mars->card, "Mars PCM", device, substreams, substreams, &pcm)) < 0)
		return err;
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK, &snd_card_mars_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &snd_card_mars_capture_ops);
	pcm->private_data = mars;
	pcm->info_flags = 0;
	strcpy(pcm->name, "Mars PCM");
	snd_pcm_lib_preallocate_pages_for_all(pcm, SNDRV_DMA_TYPE_CONTINUOUS,
					      snd_dma_continuous_data(GFP_KERNEL),
					      0, 64*1024);
	return 0;
}

#define MARS_VOLUME(xname, xindex, addr) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, .index = xindex, \
  .info = snd_mars_volume_info, \
  .get = snd_mars_volume_get, .put = snd_mars_volume_put, \
  .private_value = addr }

static int snd_mars_volume_info(snd_kcontrol_t * kcontrol, snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 2;
	uinfo->value.integer.min = -50;
	uinfo->value.integer.max = 100;
	return 0;
}
 
static int snd_mars_volume_get(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	snd_card_mars_t *mars = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int addr = kcontrol->private_value;

	spin_lock_irqsave(&mars->mixer_lock, flags);
	ucontrol->value.integer.value[0] = mars->mixer_volume[addr][0];
	ucontrol->value.integer.value[1] = mars->mixer_volume[addr][1];
	spin_unlock_irqrestore(&mars->mixer_lock, flags);
	return 0;
}

static int snd_mars_volume_put(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	snd_card_mars_t *mars = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int change, addr = kcontrol->private_value;
	int left, right;

	left = ucontrol->value.integer.value[0];
	if (left < -50)
		left = -50;
	if (left > 100)
		left = 100;
	right = ucontrol->value.integer.value[1];
	if (right < -50)
		right = -50;
	if (right > 100)
		right = 100;
	spin_lock_irqsave(&mars->mixer_lock, flags);
	change = mars->mixer_volume[addr][0] != left ||
	         mars->mixer_volume[addr][1] != right;
	mars->mixer_volume[addr][0] = left;
	mars->mixer_volume[addr][1] = right;
	spin_unlock_irqrestore(&mars->mixer_lock, flags);
	return change;
}

#define MARS_CAPSRC(xname, xindex, addr) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, .name = xname, .index = xindex, \
  .info = snd_mars_capsrc_info, \
  .get = snd_mars_capsrc_get, .put = snd_mars_capsrc_put, \
  .private_value = addr }

static int snd_mars_capsrc_info(snd_kcontrol_t * kcontrol, snd_ctl_elem_info_t * uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_BOOLEAN;
	uinfo->count = 2;
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}
 
static int snd_mars_capsrc_get(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	snd_card_mars_t *mars = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int addr = kcontrol->private_value;

	spin_lock_irqsave(&mars->mixer_lock, flags);
	ucontrol->value.integer.value[0] = mars->capture_source[addr][0];
	ucontrol->value.integer.value[1] = mars->capture_source[addr][1];
	spin_unlock_irqrestore(&mars->mixer_lock, flags);
	return 0;
}

static int snd_mars_capsrc_put(snd_kcontrol_t * kcontrol, snd_ctl_elem_value_t * ucontrol)
{
	snd_card_mars_t *mars = snd_kcontrol_chip(kcontrol);
	unsigned long flags;
	int change, addr = kcontrol->private_value;
	int left, right;

	left = ucontrol->value.integer.value[0] & 1;
	right = ucontrol->value.integer.value[1] & 1;
	spin_lock_irqsave(&mars->mixer_lock, flags);
	change = mars->capture_source[addr][0] != left &&
	         mars->capture_source[addr][1] != right;
	mars->capture_source[addr][0] = left;
	mars->capture_source[addr][1] = right;
	spin_unlock_irqrestore(&mars->mixer_lock, flags);
	return change;
}

static snd_kcontrol_new_t snd_mars_controls[] = {
MARS_VOLUME("Master Volume", 0, MIXER_ADDR_MASTER),
MARS_CAPSRC("Master Capture Switch", 0, MIXER_ADDR_MASTER),
MARS_VOLUME("Synth Volume", 0, MIXER_ADDR_SYNTH),
MARS_CAPSRC("Synth Capture Switch", 0, MIXER_ADDR_MASTER),
MARS_VOLUME("Line Volume", 0, MIXER_ADDR_LINE),
MARS_CAPSRC("Line Capture Switch", 0, MIXER_ADDR_MASTER),
MARS_VOLUME("Mic Volume", 0, MIXER_ADDR_MIC),
MARS_CAPSRC("Mic Capture Switch", 0, MIXER_ADDR_MASTER),
MARS_VOLUME("CD Volume", 0, MIXER_ADDR_CD),
MARS_CAPSRC("CD Capture Switch", 0, MIXER_ADDR_MASTER)
};

static int __init snd_card_mars_new_mixer(snd_card_mars_t * mars)
{
	snd_card_t *card = mars->card;
	unsigned int idx;
	int err;

	snd_assert(mars != NULL, return -EINVAL);
	spin_lock_init(&mars->mixer_lock);
	strcpy(card->mixername, "Mars Mixer");

	for (idx = 0; idx < ARRAY_SIZE(snd_mars_controls); idx++) {
		if ((err = snd_ctl_add(card, snd_ctl_new1(&snd_mars_controls[idx], mars))) < 0)
			return err;
	}
	return 0;
}

static int __init snd_card_mars_probe(int dev)
{
	snd_card_t *card;
	struct snd_card_mars *mars;
	int idx, err;

	if (!enable[dev])
		return -ENODEV;
	card = snd_card_new(index[dev], id[dev], THIS_MODULE,
			    sizeof(struct snd_card_mars));
	if (card == NULL)
		return -ENOMEM;
	mars = (struct snd_card_mars *)card->private_data;
	mars->card = card;
	for (idx = 0; idx < MAX_PCM_DEVICES && idx < pcm_devs[dev]; idx++) {
		if (pcm_substreams[dev] < 1)
			pcm_substreams[dev] = 1;
		if (pcm_substreams[dev] > MAX_PCM_SUBSTREAMS)
			pcm_substreams[dev] = MAX_PCM_SUBSTREAMS;
		if ((err = snd_card_mars_pcm(mars, idx, pcm_substreams[dev])) < 0)
			goto __nodev;
	}
	if ((err = snd_card_mars_new_mixer(mars)) < 0)
		goto __nodev;
	strcpy(card->driver, "Mars Driver");
	strcpy(card->shortname, "Mars");
	sprintf(card->longname, "Mars %i", dev + 1);
	if ((err = snd_card_register(card)) == 0) {
		snd_mars_cards[dev] = card;
		return 0;
	}
      __nodev:
	snd_card_free(card);
	return err;
}

static int __init alsa_card_mars_init(void)
{
	int dev, cards;

	for (dev = cards = 0; dev < SNDRV_CARDS && enable[dev]; dev++) {
		if (snd_card_mars_probe(dev) < 0) {
#ifdef MODULE
			printk(KERN_ERR "Mars soundcard #%i not found or device busy\n", dev + 1);
#endif
			break;
		}
		cards++;
	}
	if (!cards) {
#ifdef MODULE
		printk(KERN_ERR "Mars soundcard not found or device busy\n");
#endif
		return -ENODEV;
	}
	return 0;
}

static void __exit alsa_card_mars_exit(void)
{
	int idx;

	for (idx = 0; idx < SNDRV_CARDS; idx++)
		snd_card_free(snd_mars_cards[idx]);
}

module_init(alsa_card_mars_init)
module_exit(alsa_card_mars_exit)
