/*
 * linux/drivers/char/keyboard.c
 *
 * Written for linux by Johan Myreen as a translation from
 * the assembly version by Linus (with diacriticals added)
 *
 * Some additional features added by Christoph Niemann (ChN), March 1993
 *
 * Loadable keymaps by Risto Kankkunen, May 1993
 *
 * Diacriticals redone & other small changes, aeb@cwi.nl, June 1993
 * Added decr/incr_console, dynamic keymaps, Unicode support,
 * dynamic function/string keys, led setting,  Sept 1994
 * `Sticky' modifier keys, 951006.
 *
 * 11-11-96: SAK should now work in the raw mode (Martin Mares)
 * 
 * Modified to provide 'generic' keyboard support by Hamish Macdonald
 * Merge with the m68k keyboard driver and split-off of the PC low-level
 * parts by Geert Uytterhoeven, May 1997
 *
 * 27-05-97: Added support for the Magic SysRq Key (Martin Mares)
 * 30-07-98: Dead keys redone, aeb@cwi.nl.
 * 21-08-02: Converted to input API, major cleanup. (Vojtech Pavlik)
 */

#include <linux/config.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/slab.h>

#include <linux/kbd_kern.h>
#include <linux/kbd_diacr.h>
#include <linux/vt_kern.h>
#include <linux/sysrq.h>
#include <linux/input.h>

static void kbd_disconnect(struct input_handle *handle);
extern void ctrl_alt_del(void);

/*
 * Exported functions/variables
 */

#define KBD_DEFMODE ((1 << VC_REPEAT) | (1 << VC_META))

/*
 * Some laptops take the 789uiojklm,. keys as number pad when NumLock is on.
 * This seems a good reason to start with NumLock off. On HIL keyboards
 * of PARISC machines however there is no NumLock key and everyone expects the keypad 
 * to be used for numbers.
 */

#if defined(CONFIG_PARISC) && (defined(CONFIG_KEYBOARD_HIL) || defined(CONFIG_KEYBOARD_HIL_OLD))
#define KBD_DEFLEDS (1 << VC_NUMLOCK)
#else
#define KBD_DEFLEDS 0
#endif

#define KBD_DEFLOCK 0

void compute_shiftstate(void);

/*
 * Handler Tables.
 */

#define K_HANDLERS\
	k_self,		k_fn,		k_spec,		k_pad,\
	k_dead,		k_cons,		k_cur,		k_shift,\
	k_meta,		k_ascii,	k_lock,		k_lowercase,\
	k_slock,	k_dead2,	k_ignore,	k_ignore

typedef void (k_handler_fn)(struct vc_data *vc, unsigned char value, 
			    char up_flag, struct pt_regs *regs);
static k_handler_fn K_HANDLERS;
static k_handler_fn *k_handler[16] = { K_HANDLERS };

#define FN_HANDLERS\
	fn_null, 	fn_enter,	fn_show_ptregs,	fn_show_mem,\
	fn_show_state,	fn_send_intr, 	fn_lastcons, 	fn_caps_toggle,\
	fn_num,		fn_hold, 	fn_scroll_forw,	fn_scroll_back,\
	fn_boot_it, 	fn_caps_on, 	fn_compose,	fn_SAK,\
	fn_dec_console, fn_inc_console, fn_spawn_con, 	fn_bare_num

typedef void (fn_handler_fn)(struct vc_data *vc, struct pt_regs *regs);
static fn_handler_fn FN_HANDLERS;
static fn_handler_fn *fn_handler[] = { FN_HANDLERS };

/*
 * Variables exported for vt_ioctl.c
 */

/* maximum values each key_handler can handle */
const int max_vals[] = {
	255, ARRAY_SIZE(func_table) - 1, ARRAY_SIZE(fn_handler) - 1, NR_PAD - 1,
	NR_DEAD - 1, 255, 3, NR_SHIFT - 1, 255, NR_ASCII - 1, NR_LOCK - 1,
	255, NR_LOCK - 1, 255
};

const int NR_TYPES = ARRAY_SIZE(max_vals);

struct kbd_struct kbd_table[MAX_NR_CONSOLES];
static struct kbd_struct *kbd = kbd_table;
static struct kbd_struct kbd0;

int spawnpid, spawnsig;

/*
 * Variables exported for vt.c
 */

int shift_state = 0;

/*
 * Internal Data.
 */

static struct input_handler kbd_handler;
static unsigned long key_down[NBITS(KEY_MAX)];		/* keyboard key bitmap */
static unsigned char shift_down[NR_SHIFT];		/* shift state counters.. */
static int dead_key_next;
static int npadch = -1;					/* -1 or number assembled on pad */
static unsigned char diacr;
static char rep;					/* flag telling character repeat */

static unsigned char ledstate = 0xff;			/* undefined */
static unsigned char ledioctl;

static struct ledptr {
	unsigned int *addr;
	unsigned int mask;
	unsigned char valid:1;
} ledptrs[3];

/* Simple translation table for the SysRq keys */

#ifdef CONFIG_MAGIC_SYSRQ
unsigned char kbd_sysrq_xlate[KEY_MAX + 1] =
        "\000\0331234567890-=\177\t"                    /* 0x00 - 0x0f */
        "qwertyuiop[]\r\000as"                          /* 0x10 - 0x1f */
        "dfghjkl;'`\000\\zxcv"                          /* 0x20 - 0x2f */
        "bnm,./\000*\000 \000\201\202\203\204\205"      /* 0x30 - 0x3f */
        "\206\207\210\211\212\000\000789-456+1"         /* 0x40 - 0x4f */
        "230\177\000\000\213\214\000\000\000\000\000\000\000\000\000\000" /* 0x50 - 0x5f */
        "\r\000/";                                      /* 0x60 - 0x6f */
static int sysrq_down;
#endif
static int sysrq_alt;

// -------------------------------------------------------------------------------------------------
/* Realtek keyboard mapping helper function */
#if defined(CONFIG_REALTEK_USE_RTD_KEYMAP)
#include <linux/rtd_hid_keymap.h>
#include <linux/kernel.h>

static void puts_queue(struct vc_data *vc, char *cp);

static inline void rtd_kbd_lock_key(struct vc_data *vc, unsigned char lock_key, unsigned char key_state)
{
    // static char buf[] = { 0x1b, '[', 'R', 'L',0x0,0x0,0x0};
    char buf[] = { 0x1b, '[', 'R', 'L',0x0,0x0,0x0};
    	    		    	    	    		   	    	    
	switch (lock_key)
    {
        case VC_SCROLLOCK:
            buf[4] = 'S';
        break;
        
        case VC_NUMLOCK:
            buf[4] = 'N';
        break;
        
        case VC_CAPSLOCK:
            buf[4] = 'C';
        break;
        
        case VC_KANALOCK:
            buf[4] = 'K';
        break;        
    }   
    
    buf[5] = (key_state ? '1' : '0');	  
		
	puts_queue(vc, buf);
}   

// cyhuang (2011/05/16) : Add for realtek keymap (MCE or keyboard media key support)
static inline int rtd_kbd_map_key(struct vc_data *vc,unsigned int keycode,char up_flag) 
{
    char buf[] = { 0x1b, '[', 'R', 0x0,0x0,0x0,0x0,0x0,0x0};   
            
    // printk("%s : keycode = 0x%x\n",__func__,keycode);
    
    if (keycode < RTD_GENDESK_KEY_BASE)
        return 0;
        
    if (up_flag)
		return 1;    
    
    if ((keycode > RTD_GENDESK_KEY_BASE) && (keycode < RTD_KEY_BASE))
        sprintf(buf+1,"[RG%04x",keycode);
    else if ((keycode >= RTD_KEY_BASE) && (keycode < RTD_CONSUMER_KEY_BASE))
        sprintf(buf+1,"[RM%04x",keycode); 
    else if (keycode >= RTD_CONSUMER_KEY_BASE)
        sprintf(buf+1,"[RC%04x",keycode);           
    else
        return 0;   
        
    puts_queue(vc, buf);     
    return 1;
}

#endif	// CONFIG_REALTEK_USE_RTD_KEYMAP
// -------------------------------------------------------------------------------------------------

/*
 * Translation of scancodes to keycodes. We set them on only the first attached
 * keyboard - for per-keyboard setting, /dev/input/event is more useful.
 */
int getkeycode(unsigned int scancode)
{
	struct list_head * node;
	struct input_dev *dev = NULL;

	list_for_each(node,&kbd_handler.h_list) {
		struct input_handle * handle = to_handle_h(node);
		if (handle->dev->keycodesize) { 
			dev = handle->dev; 
			break;
		}
	}

	if (!dev)
		return -ENODEV;

	if (scancode >= dev->keycodemax)
		return -EINVAL;

	return INPUT_KEYCODE(dev, scancode);
}

int setkeycode(unsigned int scancode, unsigned int keycode)
{
	struct list_head * node;
	struct input_dev *dev = NULL;
	unsigned int i, oldkey;

	list_for_each(node,&kbd_handler.h_list) {
		struct input_handle *handle = to_handle_h(node);
		if (handle->dev->keycodesize) { 
			dev = handle->dev; 
			break; 
		}
	}

	if (!dev)
		return -ENODEV;

	if (scancode >= dev->keycodemax)
		return -EINVAL;
	if (keycode > KEY_MAX)
		return -EINVAL;
	if (keycode < 0 || keycode > KEY_MAX)
		return -EINVAL;

	oldkey = SET_INPUT_KEYCODE(dev, scancode, keycode);

	clear_bit(oldkey, dev->keybit);
	set_bit(keycode, dev->keybit);

	for (i = 0; i < dev->keycodemax; i++)
		if (INPUT_KEYCODE(dev,i) == oldkey)
			set_bit(oldkey, dev->keybit);

	return 0;
}

/*
 * Making beeps and bells. 
 */
static void kd_nosound(unsigned long ignored)
{
	struct list_head * node;

	list_for_each(node,&kbd_handler.h_list) {
		struct input_handle *handle = to_handle_h(node);
		if (test_bit(EV_SND, handle->dev->evbit)) {
			if (test_bit(SND_TONE, handle->dev->sndbit))
				input_event(handle->dev, EV_SND, SND_TONE, 0);
			if (test_bit(SND_BELL, handle->dev->sndbit))
				input_event(handle->dev, EV_SND, SND_BELL, 0);
		}
	}
}

static struct timer_list kd_mksound_timer =
		TIMER_INITIALIZER(kd_nosound, 0, 0);

void kd_mksound(unsigned int hz, unsigned int ticks)
{
	struct list_head * node;

	del_timer(&kd_mksound_timer);

	if (hz) {
		list_for_each_prev(node,&kbd_handler.h_list) {
			struct input_handle *handle = to_handle_h(node);
			if (test_bit(EV_SND, handle->dev->evbit)) {
				if (test_bit(SND_TONE, handle->dev->sndbit)) {
					input_event(handle->dev, EV_SND, SND_TONE, hz);
					break;
				}
				if (test_bit(SND_BELL, handle->dev->sndbit)) {
					input_event(handle->dev, EV_SND, SND_BELL, 1);
					break;
				}
			}
		}
		if (ticks)
			mod_timer(&kd_mksound_timer, jiffies + ticks);
	} else
		kd_nosound(0);
}

/*
 * Setting the keyboard rate.
 */

int kbd_rate(struct kbd_repeat *rep)
{
	struct list_head *node;
	unsigned int d = 0;
	unsigned int p = 0;

	list_for_each(node,&kbd_handler.h_list) {
		struct input_handle *handle = to_handle_h(node);
		struct input_dev *dev = handle->dev;

		if (test_bit(EV_REP, dev->evbit)) {
			if (rep->delay > 0)
				input_event(dev, EV_REP, REP_DELAY, rep->delay);
			if (rep->period > 0)
				input_event(dev, EV_REP, REP_PERIOD, rep->period);
			d = dev->rep[REP_DELAY];
			p = dev->rep[REP_PERIOD];
		}
	}
	rep->delay  = d;
	rep->period = p;
	return 0;
}

/*
 * Helper Functions.
 */
static void put_queue(struct vc_data *vc, int ch)
{
	struct tty_struct *tty = vc->vc_tty;

	if (tty) {
		tty_insert_flip_char(tty, ch, 0);
		con_schedule_flip(tty);
	}
}

static void puts_queue(struct vc_data *vc, char *cp)
{
	struct tty_struct *tty = vc->vc_tty;

	if (!tty)
		return;

	while (*cp) {
		tty_insert_flip_char(tty, *cp, 0);
		cp++;
	}
	con_schedule_flip(tty);
}

static void applkey(struct vc_data *vc, int key, char mode)
{
	static char buf[] = { 0x1b, 'O', 0x00, 0x00 };

	buf[1] = (mode ? 'O' : '[');
	buf[2] = key;
	puts_queue(vc, buf);
}

/*
 * Many other routines do put_queue, but I think either
 * they produce ASCII, or they produce some user-assigned
 * string, and in both cases we might assume that it is
 * in utf-8 already. UTF-8 is defined for words of up to 31 bits,
 * but we need only 16 bits here
 */
static void to_utf8(struct vc_data *vc, ushort c)
{
	if (c < 0x80)
		/*  0******* */
		put_queue(vc, c);
    	else if (c < 0x800) {
		/* 110***** 10****** */
		put_queue(vc, 0xc0 | (c >> 6)); 
		put_queue(vc, 0x80 | (c & 0x3f));
    	} else {
		/* 1110**** 10****** 10****** */
		put_queue(vc, 0xe0 | (c >> 12));
		put_queue(vc, 0x80 | ((c >> 6) & 0x3f));
		put_queue(vc, 0x80 | (c & 0x3f));
    	}
}

/* 
 * Called after returning from RAW mode or when changing consoles - recompute
 * shift_down[] and shift_state from key_down[] maybe called when keymap is
 * undefined, so that shiftkey release is seen
 */
void compute_shiftstate(void)
{
	unsigned int i, j, k, sym, val;

	shift_state = 0;
	memset(shift_down, 0, sizeof(shift_down));
	
	for (i = 0; i < ARRAY_SIZE(key_down); i++) {

		if (!key_down[i])
			continue;

		k = i * BITS_PER_LONG;

		for (j = 0; j < BITS_PER_LONG; j++, k++) {

			if (!test_bit(k, key_down))
				continue;

			sym = U(key_maps[0][k]);
			if (KTYP(sym) != KT_SHIFT && KTYP(sym) != KT_SLOCK)
				continue;

			val = KVAL(sym);
			if (val == KVAL(K_CAPSSHIFT))
				val = KVAL(K_SHIFT);

			shift_down[val]++;
			shift_state |= (1 << val);
		}
	}
}

/*
 * We have a combining character DIACR here, followed by the character CH.
 * If the combination occurs in the table, return the corresponding value.
 * Otherwise, if CH is a space or equals DIACR, return DIACR.
 * Otherwise, conclude that DIACR was not combining after all,
 * queue it and return CH.
 */
static unsigned char handle_diacr(struct vc_data *vc, unsigned char ch)
{
	int d = diacr;
	unsigned int i;

	diacr = 0;

	for (i = 0; i < accent_table_size; i++) {
		if (accent_table[i].diacr == d && accent_table[i].base == ch)
			return accent_table[i].result;
	}

	if (ch == ' ' || ch == d)
		return d;

	put_queue(vc, d);
	return ch;
}

/*
 * Special function handlers
 */
static void fn_enter(struct vc_data *vc, struct pt_regs *regs)
{
	if (diacr) {
		put_queue(vc, diacr);
		diacr = 0;
	}
	put_queue(vc, 13);
	if (vc_kbd_mode(kbd, VC_CRLF))
		put_queue(vc, 10);
}

static void fn_caps_toggle(struct vc_data *vc, struct pt_regs *regs)
{
	if (rep)
		return;
		
	chg_vc_kbd_led(kbd, VC_CAPSLOCK);

#if defined(CONFIG_REALTEK_USE_RTD_KEYMAP)
    rtd_kbd_lock_key(vc,VC_CAPSLOCK,vc_kbd_led(kbd, VC_CAPSLOCK));
#endif	
}

static void fn_caps_on(struct vc_data *vc, struct pt_regs *regs)
{
	if (rep)
		return;
		
	set_vc_kbd_led(kbd, VC_CAPSLOCK);
	
#if defined(CONFIG_REALTEK_USE_RTD_KEYMAP)
    rtd_kbd_lock_key(vc,VC_CAPSLOCK,vc_kbd_led(kbd, VC_CAPSLOCK));
#endif			
}

static void fn_show_ptregs(struct vc_data *vc, struct pt_regs *regs)
{
	if (regs)
		show_regs(regs);
}

static void fn_hold(struct vc_data *vc, struct pt_regs *regs)
{
	struct tty_struct *tty = vc->vc_tty;

	if (rep || !tty)
		return;

	/*
	 * Note: SCROLLOCK will be set (cleared) by stop_tty (start_tty);
	 * these routines are also activated by ^S/^Q.
	 * (And SCROLLOCK can also be set by the ioctl KDSKBLED.)
	 */
	if (tty->stopped)
		start_tty(tty);
	else
		stop_tty(tty);
}

static void fn_num(struct vc_data *vc, struct pt_regs *regs)
{
	if (vc_kbd_mode(kbd,VC_APPLIC))
		applkey(vc, 'P', 1);
	else
		fn_bare_num(vc, regs);
		
#if defined(CONFIG_REALTEK_USE_RTD_KEYMAP)
    rtd_kbd_lock_key(vc,VC_NUMLOCK,vc_kbd_led(kbd, VC_NUMLOCK));
#endif		
}

/*
 * Bind this to Shift-NumLock if you work in application keypad mode
 * but want to be able to change the NumLock flag.
 * Bind this to NumLock if you prefer that the NumLock key always
 * changes the NumLock flag.
 */
static void fn_bare_num(struct vc_data *vc, struct pt_regs *regs)
{
	if (!rep)
		chg_vc_kbd_led(kbd, VC_NUMLOCK);
}

static void fn_lastcons(struct vc_data *vc, struct pt_regs *regs)
{
	/* switch to the last used console, ChN */
	set_console(last_console);
}

static void fn_dec_console(struct vc_data *vc, struct pt_regs *regs)
{
	int i, cur = fg_console;

	/* Currently switching?  Queue this next switch relative to that. */
	if (want_console != -1)
		cur = want_console;

	for (i = cur-1; i != cur; i--) {
		if (i == -1)
			i = MAX_NR_CONSOLES-1;
		if (vc_cons_allocated(i))
			break;
	}
	set_console(i);
}

static void fn_inc_console(struct vc_data *vc, struct pt_regs *regs)
{
	int i, cur = fg_console;

	/* Currently switching?  Queue this next switch relative to that. */
	if (want_console != -1)
		cur = want_console;

	for (i = cur+1; i != cur; i++) {
		if (i == MAX_NR_CONSOLES)
			i = 0;
		if (vc_cons_allocated(i))
			break;
	}
	set_console(i);
}

static void fn_send_intr(struct vc_data *vc, struct pt_regs *regs)
{
	struct tty_struct *tty = vc->vc_tty;

	if (!tty)
		return;
	tty_insert_flip_char(tty, 0, TTY_BREAK);
	con_schedule_flip(tty);
}

static void fn_scroll_forw(struct vc_data *vc, struct pt_regs *regs)
{
	scrollfront(vc, 0);
}

static void fn_scroll_back(struct vc_data *vc, struct pt_regs *regs)
{
	scrollback(vc, 0);
}

static void fn_show_mem(struct vc_data *vc, struct pt_regs *regs)
{
	show_mem();
}

static void fn_show_state(struct vc_data *vc, struct pt_regs *regs)
{
	show_state();
}

static void fn_boot_it(struct vc_data *vc, struct pt_regs *regs)
{
	ctrl_alt_del();
}

static void fn_compose(struct vc_data *vc, struct pt_regs *regs)
{
	dead_key_next = 1;
}

static void fn_spawn_con(struct vc_data *vc, struct pt_regs *regs)
{
        if (spawnpid)
	   if(kill_proc(spawnpid, spawnsig, 1))
	     spawnpid = 0;
}

static void fn_SAK(struct vc_data *vc, struct pt_regs *regs)
{
	struct tty_struct *tty = vc->vc_tty;

	/*
	 * SAK should also work in all raw modes and reset
	 * them properly.
	 */
	if (tty)
		do_SAK(tty);
	reset_vc(vc);
}

static void fn_null(struct vc_data *vc, struct pt_regs *regs)
{
	compute_shiftstate();
}

/*
 * Special key handlers
 */
static void k_ignore(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
}

static void k_spec(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	if (up_flag)
		return;
	if (value >= ARRAY_SIZE(fn_handler))
		return;
	if ((kbd->kbdmode == VC_RAW || 
	     kbd->kbdmode == VC_MEDIUMRAW) && 
	     value != KVAL(K_SAK))
		return;		/* SAK is allowed even in raw mode */
	fn_handler[value](vc, regs);
}

static void k_lowercase(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	printk(KERN_ERR "keyboard.c: k_lowercase was called - impossible\n");
}

static void k_self(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	if (up_flag)
		return;		/* no action, if this is a key release */

	if (diacr)
		value = handle_diacr(vc, value);

	if (dead_key_next) {
		dead_key_next = 0;
		diacr = value;
		return;
	}
	put_queue(vc, value);
}

/*
 * Handle dead key. Note that we now may have several
 * dead keys modifying the same character. Very useful
 * for Vietnamese.
 */
static void k_dead2(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	if (up_flag)
		return;
	diacr = (diacr ? handle_diacr(vc, value) : value);
}

/*
 * Obsolete - for backwards compatibility only
 */
static void k_dead(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	static unsigned char ret_diacr[NR_DEAD] = {'`', '\'', '^', '~', '"', ',' };
	value = ret_diacr[value];
	k_dead2(vc, value, up_flag, regs);
}

static void k_cons(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	if (up_flag)
		return;
	set_console(value);
}

static void k_fn(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	unsigned v;

	if (up_flag)
		return;
	v = value;
	if (v < ARRAY_SIZE(func_table)) {
		if (func_table[value])
			puts_queue(vc, func_table[value]);
	} else
		printk(KERN_ERR "k_fn called with value=%d\n", value);
}

static void k_cur(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	static const char *cur_chars = "BDCA";

	if (up_flag)
		return;
	applkey(vc, cur_chars[value], vc_kbd_mode(kbd, VC_CKMODE));
}

static void k_pad(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	static const char *pad_chars = "0123456789+-*/\015,.?()#";
	static const char *app_map = "pqrstuvwxylSRQMnnmPQS";

	if (up_flag)
		return;		/* no action, if this is a key release */

	/* kludge... shift forces cursor/number keys */
	if (vc_kbd_mode(kbd, VC_APPLIC) && !shift_down[KG_SHIFT]) {
		applkey(vc, app_map[value], 1);
		return;
	}

	if (!vc_kbd_led(kbd, VC_NUMLOCK))
		switch (value) {
			case KVAL(K_PCOMMA):
			case KVAL(K_PDOT):
				k_fn(vc, KVAL(K_REMOVE), 0, regs);
				return;
			case KVAL(K_P0):
				k_fn(vc, KVAL(K_INSERT), 0, regs);
				return;
			case KVAL(K_P1):
				k_fn(vc, KVAL(K_SELECT), 0, regs);
				return;
			case KVAL(K_P2):
				k_cur(vc, KVAL(K_DOWN), 0, regs);
				return;
			case KVAL(K_P3):
				k_fn(vc, KVAL(K_PGDN), 0, regs);
				return;
			case KVAL(K_P4):
				k_cur(vc, KVAL(K_LEFT), 0, regs);
				return;
			case KVAL(K_P6):
				k_cur(vc, KVAL(K_RIGHT), 0, regs);
				return;
			case KVAL(K_P7):
				k_fn(vc, KVAL(K_FIND), 0, regs);
				return;
			case KVAL(K_P8):
				k_cur(vc, KVAL(K_UP), 0, regs);
				return;
			case KVAL(K_P9):
				k_fn(vc, KVAL(K_PGUP), 0, regs);
				return;
			case KVAL(K_P5):
				applkey(vc, 'G', vc_kbd_mode(kbd, VC_APPLIC));
				return;
		}

	put_queue(vc, pad_chars[value]);
	if (value == KVAL(K_PENTER) && vc_kbd_mode(kbd, VC_CRLF))
		put_queue(vc, 10);
}

static void k_shift(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	int old_state = shift_state;

    // printk("%s : value = 0x%x , up_flag = %d\n",__func__,value,up_flag);

	if (rep)
		return;
	/*
	 * Mimic typewriter:
	 * a CapsShift key acts like Shift but undoes CapsLock
	 */
	if (value == KVAL(K_CAPSSHIFT)) {
		value = KVAL(K_SHIFT);
		if (!up_flag)
			clr_vc_kbd_led(kbd, VC_CAPSLOCK);
	}

	if (up_flag) {
		/*
		 * handle the case that two shift or control
		 * keys are depressed simultaneously
		 */
		if (shift_down[value])
			shift_down[value]--;
	} else
		shift_down[value]++;

	if (shift_down[value])
		shift_state |= (1 << value);
	else
		shift_state &= ~(1 << value);

	/* kludge */
	if (up_flag && shift_state != old_state && npadch != -1) {
		if (kbd->kbdmode == VC_UNICODE)
			to_utf8(vc, npadch & 0xffff);
		else
			put_queue(vc, npadch & 0xff);
		npadch = -1;
	}
}

static void k_meta(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	if (up_flag)
		return;

	if (vc_kbd_mode(kbd, VC_META)) {
		put_queue(vc, '\033');
		put_queue(vc, value);
	} else
		put_queue(vc, value | 0x80);
}

static void k_ascii(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	int base;

	if (up_flag)
		return;

	if (value < 10) {
		/* decimal input of code, while Alt depressed */
		base = 10;
	} else {
		/* hexadecimal input of code, while AltGr depressed */
		value -= 10;
		base = 16;
	}

	if (npadch == -1)
		npadch = value;
	else
		npadch = npadch * base + value;
}

static void k_lock(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	if (up_flag || rep)
		return;
	chg_vc_kbd_lock(kbd, value);
}

static void k_slock(struct vc_data *vc, unsigned char value, char up_flag, struct pt_regs *regs)
{
	k_shift(vc, value, up_flag, regs);
	if (up_flag || rep)
		return;
	chg_vc_kbd_slock(kbd, value);
	/* try to make Alt, oops, AltGr and such work */
	if (!key_maps[kbd->lockstate ^ kbd->slockstate]) {
		kbd->slockstate = 0;
		chg_vc_kbd_slock(kbd, value);
	}
}

/*
 * The leds display either (i) the status of NumLock, CapsLock, ScrollLock,
 * or (ii) whatever pattern of lights people want to show using KDSETLED,
 * or (iii) specified bits of specified words in kernel memory.
 */
unsigned char getledstate(void)
{
	return ledstate;
}

void setledstate(struct kbd_struct *kbd, unsigned int led)
{
	if (!(led & ~7)) {
		ledioctl = led;
		kbd->ledmode = LED_SHOW_IOCTL;
	} else
		kbd->ledmode = LED_SHOW_FLAGS;
	set_leds();
}

static inline unsigned char getleds(void)
{
	struct kbd_struct *kbd = kbd_table + fg_console;
	unsigned char leds;
	int i;

	if (kbd->ledmode == LED_SHOW_IOCTL)
		return ledioctl;

	leds = kbd->ledflagstate;

	if (kbd->ledmode == LED_SHOW_MEM) {
		for (i = 0; i < 3; i++)
			if (ledptrs[i].valid) {
				if (*ledptrs[i].addr & ledptrs[i].mask)
					leds |= (1 << i);
				else
					leds &= ~(1 << i);
			}
	}
	return leds;
}

/*
 * This routine is the bottom half of the keyboard interrupt
 * routine, and runs with all interrupts enabled. It does
 * console changing, led setting and copy_to_cooked, which can
 * take a reasonably long time.
 *
 * Aside from timing (which isn't really that important for
 * keyboard interrupts as they happen often), using the software
 * interrupt routines for this thing allows us to easily mask
 * this when we don't want any of the above to happen.
 * This allows for easy and efficient race-condition prevention
 * for kbd_refresh_leds => input_event(dev, EV_LED, ...) => ...
 */

static void kbd_bh(unsigned long dummy)
{
	struct list_head * node;
	unsigned char leds = getleds();

	if (leds != ledstate) {
		list_for_each(node,&kbd_handler.h_list) {
			struct input_handle * handle = to_handle_h(node);
			input_event(handle->dev, EV_LED, LED_SCROLLL, !!(leds & 0x01));
			input_event(handle->dev, EV_LED, LED_NUML,    !!(leds & 0x02));
			input_event(handle->dev, EV_LED, LED_CAPSL,   !!(leds & 0x04));
			input_sync(handle->dev);
		}
	}

	ledstate = leds;
}

DECLARE_TASKLET_DISABLED(keyboard_tasklet, kbd_bh, 0);

/*
 * This allows a newly plugged keyboard to pick the LED state.
 */
static void kbd_refresh_leds(struct input_handle *handle)
{
	unsigned char leds = ledstate;

	tasklet_disable(&keyboard_tasklet);
	if (leds != 0xff) {
		input_event(handle->dev, EV_LED, LED_SCROLLL, !!(leds & 0x01));
		input_event(handle->dev, EV_LED, LED_NUML,    !!(leds & 0x02));
		input_event(handle->dev, EV_LED, LED_CAPSL,   !!(leds & 0x04));
		input_sync(handle->dev);
	}
	tasklet_enable(&keyboard_tasklet);
}

#if defined(CONFIG_X86) || defined(CONFIG_IA64) || defined(CONFIG_ALPHA) ||\
    defined(CONFIG_MIPS) || defined(CONFIG_PPC) || defined(CONFIG_SPARC32) ||\
    defined(CONFIG_SPARC64) || defined(CONFIG_PARISC) || defined(CONFIG_SUPERH) ||\
    (defined(CONFIG_ARM) && defined(CONFIG_KEYBOARD_ATKBD) && !defined(CONFIG_ARCH_RPC))

#define HW_RAW(dev) (test_bit(EV_MSC, dev->evbit) && test_bit(MSC_RAW, dev->mscbit) &&\
			((dev)->id.bustype == BUS_I8042) && ((dev)->id.vendor == 0x0001) && ((dev)->id.product == 0x0001))

static unsigned short x86_keycodes[256] =
	{ 0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
	 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
	 80, 81, 82, 83, 84,118, 86, 87, 88,115,120,119,121,112,123, 92,
	284,285,309,298,312, 91,327,328,329,331,333,335,336,337,338,339,
	367,288,302,304,350, 89,334,326,267,126,268,269,125,347,348,349,
	360,261,262,263,268,376,100,101,321,316,373,286,289,102,351,355,
	103,104,105,275,287,279,306,106,274,107,294,364,358,363,362,361,
	291,108,381,281,290,272,292,305,280, 99,112,257,258,359,113,114,
	264,117,271,374,379,265,266, 93, 94, 95, 85,259,375,260, 90,116,
	377,109,111,277,278,282,283,295,296,297,299,300,301,293,303,307,
	308,310,313,314,315,317,318,319,320,357,322,323,324,325,276,330,
	332,340,365,342,343,344,345,346,356,270,341,368,369,370,371,372 };

#ifdef CONFIG_MAC_EMUMOUSEBTN
extern int mac_hid_mouse_emulate_buttons(int, int, int);
#endif /* CONFIG_MAC_EMUMOUSEBTN */

#if defined(CONFIG_SPARC32) || defined(CONFIG_SPARC64)
static int sparc_l1_a_state = 0;
extern void sun_do_break(void);
#endif

static int emulate_raw(struct vc_data *vc, unsigned int keycode, 
		       unsigned char up_flag)
{
	if (keycode > 255 || !x86_keycodes[keycode])
		return -1; 

	switch (keycode) {
		case KEY_PAUSE:
			put_queue(vc, 0xe1);
			put_queue(vc, 0x1d | up_flag);
			put_queue(vc, 0x45 | up_flag);
			return 0;
		case KEY_HANGUEL:
			if (!up_flag) put_queue(vc, 0xf1);
			return 0;
		case KEY_HANJA:
			if (!up_flag) put_queue(vc, 0xf2);
			return 0;
	} 

	if (keycode == KEY_SYSRQ && sysrq_alt) {
		put_queue(vc, 0x54 | up_flag);
		return 0;
	}

	if (x86_keycodes[keycode] & 0x100)
		put_queue(vc, 0xe0);

	put_queue(vc, (x86_keycodes[keycode] & 0x7f) | up_flag);

	if (keycode == KEY_SYSRQ) {
		put_queue(vc, 0xe0);
		put_queue(vc, 0x37 | up_flag);
	}

	return 0;
}

#else

#define HW_RAW(dev)	0

#warning "Cannot generate rawmode keyboard for your architecture yet."

static int emulate_raw(struct vc_data *vc, unsigned int keycode, unsigned char up_flag)
{
	if (keycode > 127)
		return -1;

	put_queue(vc, keycode | up_flag);
	return 0;
}
#endif

static void kbd_rawcode(unsigned char data)
{
	struct vc_data *vc = vc_cons[fg_console].d;
	kbd = kbd_table + fg_console;
	if (kbd->kbdmode == VC_RAW)
		put_queue(vc, data);
}

static void kbd_keycode(unsigned int keycode, int down,
			int hw_raw, struct pt_regs *regs)
{
	struct vc_data *vc = vc_cons[fg_console].d;
	unsigned short keysym, *key_map;
	unsigned char type, raw_mode;
	struct tty_struct *tty;
	int shift_final;

#if 0
#ifdef CONFIG_REALTEK_VENUS_USB	//cfyeh+ 2005/11/07
	printk("%s : keycode = 0x%.2x\n", __FUNCTION__, keycode);
#endif /* CONFIG_REALTEK_VENUS_USB */	//cfyeh- 2005/11/07
#endif
	
	tty = vc->vc_tty;

	if (tty && (!tty->driver_data)) {
		/* No driver data? Strange. Okay we fix it then. */
		tty->driver_data = vc;
	}

	kbd = kbd_table + fg_console;

	if (keycode == KEY_LEFTALT || keycode == KEY_RIGHTALT)
		sysrq_alt = down;
#if defined(CONFIG_SPARC32) || defined(CONFIG_SPARC64)
	if (keycode == KEY_STOP)
		sparc_l1_a_state = down;
#endif

	rep = (down == 2);

#ifdef CONFIG_MAC_EMUMOUSEBTN
	if (mac_hid_mouse_emulate_buttons(1, keycode, down))
		return;
#endif /* CONFIG_MAC_EMUMOUSEBTN */

	if ((raw_mode = (kbd->kbdmode == VC_RAW)) && !hw_raw)
		if (emulate_raw(vc, keycode, !down << 7))
			if (keycode < BTN_MISC)
				printk(KERN_WARNING "keyboard.c: can't emulate rawmode for keycode %d\n", keycode);

#ifdef CONFIG_MAGIC_SYSRQ	       /* Handle the SysRq Hack */
	if (keycode == KEY_SYSRQ && (sysrq_down || (down == 1 && sysrq_alt))) {
		sysrq_down = down;
		return;
	}
	if (sysrq_down && down && !rep) {
		handle_sysrq(kbd_sysrq_xlate[keycode], regs, tty);
		return;
	}
#endif
#if defined(CONFIG_SPARC32) || defined(CONFIG_SPARC64)
	if (keycode == KEY_A && sparc_l1_a_state) {
		sparc_l1_a_state = 0;
		sun_do_break();
	}
#endif

	if (kbd->kbdmode == VC_MEDIUMRAW) {
		/*
		 * This is extended medium raw mode, with keys above 127
		 * encoded as 0, high 7 bits, low 7 bits, with the 0 bearing
		 * the 'up' flag if needed. 0 is reserved, so this shouldn't
		 * interfere with anything else. The two bytes after 0 will
		 * always have the up flag set not to interfere with older
		 * applications. This allows for 16384 different keycodes,
		 * which should be enough.
		 */
		if (keycode < 128) {
			put_queue(vc, keycode | (!down << 7));
		} else {
			put_queue(vc, !down << 7);
			put_queue(vc, (keycode >> 7) | 0x80);
			put_queue(vc, keycode | 0x80);
		}
		raw_mode = 1;
	}

	if (down)
		set_bit(keycode, key_down);
	else
		clear_bit(keycode, key_down);

	if (rep && (!vc_kbd_mode(kbd, VC_REPEAT) || (tty && 
		(!L_ECHO(tty) && tty->driver->chars_in_buffer(tty))))) {
		/*
		 * Don't repeat a key if the input buffers are not empty and the
		 * characters get aren't echoed locally. This makes key repeat 
		 * usable with slow applications and under heavy loads.
		 */
		return;
	}

	shift_final = (shift_state | kbd->slockstate) ^ kbd->lockstate;
	key_map = key_maps[shift_final];

	if (!key_map) {
		compute_shiftstate();
		kbd->slockstate = 0;
		return;
	}

// cyhuang (2011/05/16) : Add for realtek keymap (MCE or keyboard media key support)
#if defined(CONFIG_REALTEK_USE_RTD_KEYMAP)
    if (rtd_kbd_map_key(vc,keycode,!down) == 1)
        return;
#endif
	
	if (keycode > NR_KEYS)
		return;
	

	keysym = key_map[keycode];
	type = KTYP(keysym);

	if (type < 0xf0) {
		if (down && !raw_mode) to_utf8(vc, keysym);
		return;
	}

	type -= 0xf0;

	if (raw_mode && type != KT_SPEC && type != KT_SHIFT)
		return;

	if (type == KT_LETTER) {
		type = KT_LATIN;
		if (vc_kbd_led(kbd, VC_CAPSLOCK)) {
			key_map = key_maps[shift_final ^ (1 << KG_SHIFT)];
			if (key_map)
				keysym = key_map[keycode];
		}
	}
    
	(*k_handler[type])(vc, keysym & 0xff, !down, regs);

	if (type != KT_SLOCK)
		kbd->slockstate = 0;
}

static void kbd_event(struct input_handle *handle, unsigned int event_type, 
		      unsigned int event_code, int value)
{
	if (event_type == EV_MSC && event_code == MSC_RAW && HW_RAW(handle->dev))
		kbd_rawcode(value);
	if (event_type == EV_KEY)
		kbd_keycode(event_code, value, HW_RAW(handle->dev), handle->dev->regs);
	tasklet_schedule(&keyboard_tasklet);
	do_poke_blanked_console = 1;
	schedule_console_callback();
}

static char kbd_name[] = "kbd";

/*
 * When a keyboard (or other input device) is found, the kbd_connect
 * function is called. The function then looks at the device, and if it
 * likes it, it can open it and get events from it. In this (kbd_connect)
 * function, we should decide which VT to bind that keyboard to initially.
 */
static struct input_handle *kbd_connect(struct input_handler *handler, 
					struct input_dev *dev,
					struct input_device_id *id)
{
	struct input_handle *handle;
	int i;

	for (i = KEY_RESERVED; i < BTN_MISC; i++)
		if (test_bit(i, dev->keybit)) break;

	if ((i == BTN_MISC) && !test_bit(EV_SND, dev->evbit)) 
		return NULL;

	if (!(handle = kmalloc(sizeof(struct input_handle), GFP_KERNEL))) 
		return NULL;
	memset(handle, 0, sizeof(struct input_handle));

	handle->dev = dev;
	handle->handler = handler;
	handle->name = kbd_name;

	input_open_device(handle);
	kbd_refresh_leds(handle);

	return handle;
}

static void kbd_disconnect(struct input_handle *handle)
{
	input_close_device(handle);
	kfree(handle);
}

static struct input_device_id kbd_ids[] = {
	{
                .flags = INPUT_DEVICE_ID_MATCH_EVBIT,
                .evbit = { BIT(EV_KEY) },
        },
	
	{
                .flags = INPUT_DEVICE_ID_MATCH_EVBIT,
                .evbit = { BIT(EV_SND) },
        },	

	{ },    /* Terminating entry */
};

MODULE_DEVICE_TABLE(input, kbd_ids);

static struct input_handler kbd_handler = {
	.event		= kbd_event,
	.connect	= kbd_connect,
	.disconnect	= kbd_disconnect,
	.name		= "kbd",
	.id_table	= kbd_ids,
};

int __init kbd_init(void)
{
	int i;

        kbd0.ledflagstate = kbd0.default_ledflagstate = KBD_DEFLEDS;
        kbd0.ledmode = LED_SHOW_FLAGS;
        kbd0.lockstate = KBD_DEFLOCK;
        kbd0.slockstate = 0;
        kbd0.modeflags = KBD_DEFMODE;
        kbd0.kbdmode = VC_XLATE;

        for (i = 0 ; i < MAX_NR_CONSOLES ; i++)
                kbd_table[i] = kbd0;

	input_register_handler(&kbd_handler);

	tasklet_enable(&keyboard_tasklet);
	tasklet_schedule(&keyboard_tasklet);

	return 0;
}
