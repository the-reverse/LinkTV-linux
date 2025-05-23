/*
 *  linux/kernel/printk.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 * Modified to make sys_syslog() more flexible: added commands to
 * return the last 4k of kernel messages, regardless of whether
 * they've been read or not.  Added option to suppress kernel printk's
 * to the console.  Added hook for sending the console messages
 * elsewhere, in preparation for a serial line console (someday).
 * Ted Ts'o, 2/11/93.
 * Modified for sysctl support, 1/8/97, Chris Horn.
 * Fixed SMP synchronization, 08/08/99, Manfred Spraul 
 *     manfreds@colorfullife.com
 * Rewrote bits to get rid of console_lock
 *	01Mar01 Andrew Morton <andrewm@uow.edu.au>
 */

#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/smp_lock.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/interrupt.h>			/* For in_interrupt() */
#include <linux/config.h>
#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/security.h>
#include <linux/bootmem.h>
#include <linux/syscalls.h>

#include <asm/uaccess.h>

#define __LOG_BUF_LEN	(1 << CONFIG_LOG_BUF_SHIFT)

/* printk's without a loglevel use this.. */
#define DEFAULT_MESSAGE_LOGLEVEL 4 /* KERN_WARNING */

/* We show everything that is MORE important than this.. */
#define MINIMUM_CONSOLE_LOGLEVEL 1 /* Minimum loglevel we let people use */
#define DEFAULT_CONSOLE_LOGLEVEL 7 /* anything MORE serious than KERN_DEBUG */

DECLARE_WAIT_QUEUE_HEAD(log_wait);

int console_printk[4] = {
	DEFAULT_CONSOLE_LOGLEVEL,	/* console_loglevel */
	DEFAULT_MESSAGE_LOGLEVEL,	/* default_message_loglevel */
	MINIMUM_CONSOLE_LOGLEVEL,	/* minimum_console_loglevel */
	DEFAULT_CONSOLE_LOGLEVEL,	/* default_console_loglevel */
};

EXPORT_SYMBOL(console_printk);

/*
 * Low lever drivers may need that to know if they can schedule in
 * their unblank() callback or not. So let's export it.
 */
int oops_in_progress;
EXPORT_SYMBOL(oops_in_progress);

/*
 * console_sem protects the console_drivers list, and also
 * provides serialisation for access to the entire console
 * driver system.
 */
static DECLARE_MUTEX(console_sem);
struct console *console_drivers;
/*
 * This is used for debugging the mess that is the VT code by
 * keeping track if we have the console semaphore held. It's
 * definitely not the perfect debug tool (we don't know if _WE_
 * hold it are racing, but it helps tracking those weird code
 * path in the console code where we end up in places I want
 * locked without the console sempahore held
 */
static int console_locked;

/*
 * logbuf_lock protects log_buf, log_start, log_end, con_start and logged_chars
 * It is also used in interesting ways to provide interlocking in
 * release_console_sem().
 */
static DEFINE_SPINLOCK(logbuf_lock);

#define LOG_BUF_MASK	(log_buf_len-1)
#define LOG_BUF(idx) (log_buf[(idx) & LOG_BUF_MASK])

/*
 * The indices into log_buf are not constrained to log_buf_len - they
 * must be masked before subscripting
 */
#ifdef CONFIG_SHARED_PRINTK
unsigned int av_lock = 0;

static unsigned long log_start __attribute__ ((section(".bss.log.start")));	/* Index into log_buf: next char to be read by syslog() */
static unsigned long con_start __attribute__ ((section(".bss.con.start")));	/* Index into log_buf: next char to be sent to consoles */
static unsigned long log_end __attribute__ ((section(".bss.log.end")));	/* Index into log_buf: most-recently-written-char + 1 */
#else
static unsigned long log_start;	/* Index into log_buf: next char to be read by syslog() */
static unsigned long con_start;	/* Index into log_buf: next char to be sent to consoles */
static unsigned long log_end;	/* Index into log_buf: most-recently-written-char + 1 */
#endif

/*
 *	Array of consoles built from command line options (console=)
 */
struct console_cmdline
{
	char	name[8];			/* Name of the driver	    */
	int	index;				/* Minor dev. to use	    */
	char	*options;			/* Options for the driver   */
};

#define MAX_CMDLINECONSOLES 8

static struct console_cmdline console_cmdline[MAX_CMDLINECONSOLES];
static int selected_console = -1;
static int preferred_console = -1;

/* Flag: console code may call schedule() */
static int console_may_schedule;

/*
 *	Setup a list of consoles. Called from init/main.c
 */
static int __init console_setup(char *str)
{
	char name[sizeof(console_cmdline[0].name)];
	char *s, *options;
	int idx;

	/*
	 *	Decode str into name, index, options.
	 */
	if (str[0] >= '0' && str[0] <= '9') {
		strcpy(name, "ttyS");
		strncpy(name + 4, str, sizeof(name) - 5);
	} else
		strncpy(name, str, sizeof(name) - 1);
	name[sizeof(name) - 1] = 0;
	if ((options = strchr(str, ',')) != NULL)
		*(options++) = 0;
#ifdef __sparc__
	if (!strcmp(str, "ttya"))
		strcpy(name, "ttyS0");
	if (!strcmp(str, "ttyb"))
		strcpy(name, "ttyS1");
#endif
	for(s = name; *s; s++)
		if ((*s >= '0' && *s <= '9') || *s == ',')
			break;
	idx = simple_strtoul(s, NULL, 10);
	*s = 0;

	add_preferred_console(name, idx, options);
	return 1;
}

__setup("console=", console_setup);

#ifdef CONFIG_PRINTK

static char __log_buf[__LOG_BUF_LEN];
#ifdef CONFIG_SHARED_PRINTK
static char *log_buf __attribute__ ((section(".bss.buff")));
static int log_buf_len __attribute__ ((section(".bss.buff.len")));
static unsigned long logged_chars __attribute__ ((section(".bss.logged"))); /* Number of chars produced since last read+clear operation */

static struct work_struct printk_work;
static unsigned int printk_data;

static void printk_wq(void *ptr)
{
	if (!down_trylock(&console_sem)) {
		console_locked = 1;
		console_may_schedule = 0;
		release_console_sem();
	}
	schedule_delayed_work(&printk_work, 1);
}

static void lock_hw_sem(void)
{
	volatile int *ptr = (int *)0xb801a000;
	if (av_lock)
		return;
	while (*ptr != 1) ;
//		if (system_state == SYSTEM_RUNNING)
//			msleep(1);
}

static void unlock_hw_sem(void)
{
	volatile int *ptr = (int *)0xb801a000;
	if (av_lock)
		return;
	*ptr = 0;
}
/*
static void flush_log_buffer()
{
//	unsigned long dc_lsize = current_cpu_data.dcache.linesz;
	unsigned long dc_lsize = 16;
	unsigned long addr = (unsigned long)log_buf;
	unsigned long size = log_buf_len;

	
	while (size > 0) {
		flush_dcache_line(addr);
		addr += dc_lsize;
		size -= dc_lsize;
	}
}
*/
#else
static char *log_buf = __log_buf;
static int log_buf_len = __LOG_BUF_LEN;
static unsigned long logged_chars; /* Number of chars produced since last read+clear operation */

#define lock_hw_sem()
#define unlock_hw_sem()
#endif
void __init printk_setup(void)
{
	int *ptr = (int *)0xa00000e8;
	int cnt;

	/* clear the bss */
	for (cnt = 0; cnt < 6; cnt++)
		ptr[cnt] = 0;

#ifdef CONFIG_SHARED_PRINTK
	log_buf = __log_buf;
	log_buf = (char *)((unsigned)log_buf | 0xa0000000);
	log_buf_len = __LOG_BUF_LEN;

	INIT_WORK(&printk_work, printk_wq, &printk_data);
#endif
}

void __init printk_setup_tail(void)
{
#ifdef CONFIG_SHARED_PRINTK
	schedule_work(&printk_work);
#endif
}

static int __init log_buf_len_setup(char *str)
{
	unsigned long size = memparse(str, &str);
	unsigned long flags;

	if (size)
		size = roundup_pow_of_two(size);
	if (size > log_buf_len) {
		unsigned long start, dest_idx, offset;
		char * new_log_buf;

		new_log_buf = alloc_bootmem(size);
		if (!new_log_buf) {
			printk("log_buf_len: allocation failed\n");
			goto out;
		}

		spin_lock_irqsave(&logbuf_lock, flags);
		lock_hw_sem();
		log_buf_len = size;
		log_buf = new_log_buf;

		offset = start = min(con_start, log_start);
		dest_idx = 0;
		while (start != log_end) {
			log_buf[dest_idx] = __log_buf[start & (__LOG_BUF_LEN - 1)];
			start++;
			dest_idx++;
		}
		log_start -= offset;
		con_start -= offset;
		log_end -= offset;
		unlock_hw_sem();
		spin_unlock_irqrestore(&logbuf_lock, flags);

		printk("log_buf_len: %d\n", log_buf_len);
	}
out:

	return 1;
}

__setup("log_buf_len=", log_buf_len_setup);

/*
 * Commands to do_syslog:
 *
 * 	0 -- Close the log.  Currently a NOP.
 * 	1 -- Open the log. Currently a NOP.
 * 	2 -- Read from the log.
 * 	3 -- Read all messages remaining in the ring buffer.
 * 	4 -- Read and clear all messages remaining in the ring buffer
 * 	5 -- Clear ring buffer.
 * 	6 -- Disable printk's to console
 * 	7 -- Enable printk's to console
 *	8 -- Set level of messages printed to console
 *	9 -- Return number of unread characters in the log buffer
 *     10 -- Return size of the log buffer
 */
int do_syslog(int type, char __user * buf, int len)
{
	unsigned long i, j, limit, count;
	int do_clear = 0;
	char c;
	int error = 0;

	error = security_syslog(type);
	if (error)
		return error;

	switch (type) {
	case 0:		/* Close log */
		break;
	case 1:		/* Open log */
		break;
	case 2:		/* Read from log */
		error = -EINVAL;
		if (!buf || len < 0)
			goto out;
		error = 0;
		if (!len)
			goto out;
		if (!access_ok(VERIFY_WRITE, buf, len)) {
			error = -EFAULT;
			goto out;
		}
		error = wait_event_interruptible(log_wait, (log_start - log_end));
		if (error)
			goto out;
		i = 0;
//		flush_log_buffer();
		spin_lock_irq(&logbuf_lock);
		lock_hw_sem();
		while (!error && (log_start != log_end) && i < len) {
			c = LOG_BUF(log_start);
			log_start++;
			unlock_hw_sem();
			spin_unlock_irq(&logbuf_lock);
			error = __put_user(c,buf);
			buf++;
			i++;
			cond_resched();
			spin_lock_irq(&logbuf_lock);
			lock_hw_sem();
		}
		unlock_hw_sem();
		spin_unlock_irq(&logbuf_lock);
		if (!error)
			error = i;
		break;
	case 4:		/* Read/clear last kernel messages */
		do_clear = 1; 
		/* FALL THRU */
	case 3:		/* Read last kernel messages */
		error = -EINVAL;
		if (!buf || len < 0)
			goto out;
		error = 0;
		if (!len)
			goto out;
		if (!access_ok(VERIFY_WRITE, buf, len)) {
			error = -EFAULT;
			goto out;
		}
		count = len;
		if (count > log_buf_len)
			count = log_buf_len;
//		flush_log_buffer();
		spin_lock_irq(&logbuf_lock);
		lock_hw_sem();
		if (count > logged_chars)
			count = logged_chars;
		if (do_clear)
			logged_chars = 0;
		limit = log_end;
		/*
		 * __put_user() could sleep, and while we sleep
		 * printk() could overwrite the messages 
		 * we try to copy to user space. Therefore
		 * the messages are copied in reverse. <manfreds>
		 */
		for(i = 0; i < count && !error; i++) {
			j = limit-1-i;
			if (j + log_buf_len < log_end)
				break;
			c = LOG_BUF(j);
			unlock_hw_sem();
			spin_unlock_irq(&logbuf_lock);
			error = __put_user(c,&buf[count-1-i]);
			cond_resched();
			spin_lock_irq(&logbuf_lock);
			lock_hw_sem();
		}
		unlock_hw_sem();
		spin_unlock_irq(&logbuf_lock);
		if (error)
			break;
		error = i;
		if(i != count) {
			int offset = count-error;
			/* buffer overflow during copy, correct user buffer. */
			for(i=0;i<error;i++) {
				if (__get_user(c,&buf[i+offset]) ||
				    __put_user(c,&buf[i])) {
					error = -EFAULT;
					break;
				}
				cond_resched();
			}
		}
		break;
	case 5:		/* Clear ring buffer */
		logged_chars = 0;
		break;
	case 6:		/* Disable logging to console */
		console_loglevel = minimum_console_loglevel;
		break;
	case 7:		/* Enable logging to console */
		console_loglevel = default_console_loglevel;
		break;
	case 8:		/* Set level of messages printed to console */
		error = -EINVAL;
		if (len < 1 || len > 8)
			goto out;
		if (len < minimum_console_loglevel)
			len = minimum_console_loglevel;
		console_loglevel = len;
		error = 0;
		break;
	case 9:		/* Number of chars in the log buffer */
		error = log_end - log_start;
		break;
	case 10:	/* Size of the log buffer */
		error = log_buf_len;
		break;
	default:
		error = -EINVAL;
		break;
	}
out:
	return error;
}

asmlinkage long sys_syslog(int type, char __user * buf, int len)
{
	return do_syslog(type, buf, len);
}

/*
 * Call the console drivers on a range of log_buf
 */
static void __call_console_drivers(unsigned long start, unsigned long end)
{
	struct console *con;

	for (con = console_drivers; con; con = con->next) {
		if ((con->flags & CON_ENABLED) && con->write)
			con->write(con, &LOG_BUF(start), end - start);
	}
}

/*
 * Write out chars from start to end - 1 inclusive
 */
static void _call_console_drivers(unsigned long start,
				unsigned long end, int msg_log_level)
{
	if (msg_log_level < console_loglevel &&
			console_drivers && start != end) {
		if ((start & LOG_BUF_MASK) > (end & LOG_BUF_MASK)) {
			/* wrapped write */
			__call_console_drivers(start & LOG_BUF_MASK,
						log_buf_len);
			__call_console_drivers(0, end & LOG_BUF_MASK);
		} else {
			__call_console_drivers(start, end);
		}
	}
}

/*
 * Call the console drivers, asking them to write out
 * log_buf[start] to log_buf[end - 1].
 * The console_sem must be held.
 */
static void call_console_drivers(unsigned long start, unsigned long end)
{
	unsigned long cur_index, start_print;
	static int msg_level = -1;

	if (((long)(start - end)) > 0)
		BUG();

	cur_index = start;
	start_print = start;
	while (cur_index != end) {
		if (	msg_level < 0 &&
			((end - cur_index) > 2) &&
			LOG_BUF(cur_index + 0) == '<' &&
			LOG_BUF(cur_index + 1) >= '0' &&
			LOG_BUF(cur_index + 1) <= '7' &&
			LOG_BUF(cur_index + 2) == '>')
		{
			msg_level = LOG_BUF(cur_index + 1) - '0';
			cur_index += 3;
			start_print = cur_index;
		}
		while (cur_index != end) {
			char c = LOG_BUF(cur_index);
			cur_index++;

			if (c == '\n') {
				if (msg_level < 0) {
					/*
					 * printk() has already given us loglevel tags in
					 * the buffer.  This code is here in case the
					 * log buffer has wrapped right round and scribbled
					 * on those tags
					 */
					msg_level = default_message_loglevel;
				}
				_call_console_drivers(start_print, cur_index, msg_level);
				msg_level = -1;
				start_print = cur_index;
				break;
			}
		}
	}
/* When shared_printk is turned on, chars from AP will be directed to printk buffer and "msg_level" may be -1.
    For this situation, some chars will still slip to console when we set console_loglevel to 1. */
	if(msg_level == -1)
		_call_console_drivers(start_print, end, default_message_loglevel);
	else
		_call_console_drivers(start_print, end, msg_level);
}

static void emit_log_char(char c)
{
	LOG_BUF(log_end) = c;
	log_end++;
	if (log_end - log_start > log_buf_len)
		log_start = log_end - log_buf_len;
	if (log_end - con_start > log_buf_len)
		con_start = log_end - log_buf_len;
	if (logged_chars < log_buf_len)
		logged_chars++;
}

void lock_emit_log_char(char c) {
	unsigned long flags;

	spin_lock_irqsave(&logbuf_lock, flags);
	lock_hw_sem();
	emit_log_char(c);
	unlock_hw_sem();
	spin_unlock_irqrestore(&logbuf_lock, flags);
}

/*
 * Zap console related locks when oopsing. Only zap at most once
 * every 10 seconds, to leave time for slow consoles to print a
 * full oops.
 */
static void zap_locks(void)
{
	static unsigned long oops_timestamp;

	if (time_after_eq(jiffies, oops_timestamp) &&
			!time_after(jiffies, oops_timestamp + 30*HZ))
		return;

	oops_timestamp = jiffies;

	/* If a crash is occurring, make sure we can't deadlock */
	spin_lock_init(&logbuf_lock);
	/* And make sure that we print immediately */
	init_MUTEX(&console_sem);
}

#if defined(CONFIG_PRINTK_TIME)
static int printk_time = 1;
#else
static int printk_time = 0;
#endif

static int __init printk_time_setup(char *str)
{
	if (*str)
		return 0;
	printk_time = 1;
	return 1;
}

__setup("time", printk_time_setup);

/*
 * This is printk.  It can be called from any context.  We want it to work.
 * 
 * We try to grab the console_sem.  If we succeed, it's easy - we log the output and
 * call the console drivers.  If we fail to get the semaphore we place the output
 * into the log buffer and return.  The current holder of the console_sem will
 * notice the new output in release_console_sem() and will send it to the
 * consoles before releasing the semaphore.
 *
 * One effect of this deferred printing is that code which calls printk() and
 * then changes console_loglevel may break. This is because console_loglevel
 * is inspected when the actual printing occurs.
 */

asmlinkage int printk(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = vprintk(fmt, args);
	va_end(args);

	return r;
}

asmlinkage int vprintk(const char *fmt, va_list args)
{
	unsigned long flags;
	int printed_len;
	char *p;
	static char printk_buf[1024];
	static int log_level_unknown = 1;

	if (unlikely(oops_in_progress))
		zap_locks();

	/* This stops the holder of console_sem just where we want him */
	spin_lock_irqsave(&logbuf_lock, flags);
	lock_hw_sem();

	/* Emit the output into the temporary buffer */
	printed_len = vscnprintf(printk_buf, sizeof(printk_buf), fmt, args);

	/*
	 * Copy the output into log_buf.  If the caller didn't provide
	 * appropriate log level tags, we insert them here
	 */
	for (p = printk_buf; *p; p++) {
		if (log_level_unknown) {
                        /* log_level_unknown signals the start of a new line */
			if (printk_time) {
				int loglev_char;
				char tbuf[50], *tp;
				unsigned tlen;
				unsigned long long t;
				unsigned long nanosec_rem;

				/*
				 * force the log level token to be
				 * before the time output.
				 */
				if (p[0] == '<' && p[1] >='0' &&
				   p[1] <= '7' && p[2] == '>') {
					loglev_char = p[1];
					p += 3;
					printed_len += 3;
				} else {
					loglev_char = default_message_loglevel
						+ '0';
				}
				t = sched_clock();
				nanosec_rem = do_div(t, 1000000000);
#if 1
				tlen = sprintf(tbuf,
						"<%c>[%5lu.%06lu] ",
						loglev_char,
						(unsigned long)t,
						nanosec_rem/1000);
#else
/* This can be used to measure the time by "Count" register. */
				tlen = sprintf(tbuf,
						"<%c>[%lu] ",
						loglev_char,
						(unsigned long)read_c0_count());
#endif

				for (tp = tbuf; tp < tbuf + tlen; tp++)
					emit_log_char(*tp);
				printed_len += tlen - 3;
			} else {
				if (p[0] != '<' || p[1] < '0' ||
				   p[1] > '7' || p[2] != '>') {
					emit_log_char('<');
					emit_log_char(default_message_loglevel
						+ '0');
					emit_log_char('>');
				}
				printed_len += 3;
			}
			log_level_unknown = 0;
			if (!*p)
				break;
		}
		emit_log_char(*p);
		if (*p == '\n')
			log_level_unknown = 1;
	}

	if (!cpu_online(smp_processor_id()) &&
	    system_state != SYSTEM_RUNNING) {
		/*
		 * Some console drivers may assume that per-cpu resources have
		 * been allocated.  So don't allow them to be called by this
		 * CPU until it is officially up.  We shouldn't be calling into
		 * random console drivers on a CPU which doesn't exist yet..
		 */
		unlock_hw_sem();
		spin_unlock_irqrestore(&logbuf_lock, flags);
		goto out;
	}
	if (!down_trylock(&console_sem)) {
		console_locked = 1;
		/*
		 * We own the drivers.  We can drop the spinlock and let
		 * release_console_sem() print the text
		 */
		unlock_hw_sem();
		spin_unlock_irqrestore(&logbuf_lock, flags);
		console_may_schedule = 0;
		release_console_sem();
	} else {
		/*
		 * Someone else owns the drivers.  We drop the spinlock, which
		 * allows the semaphore holder to proceed and to call the
		 * console drivers with the output which we just produced.
		 */
		unlock_hw_sem();
		spin_unlock_irqrestore(&logbuf_lock, flags);
	}
out:
	return printed_len;
}
EXPORT_SYMBOL(printk);
EXPORT_SYMBOL(vprintk);

#else

#define lock_hw_sem()  
#define unlock_hw_sem()

asmlinkage long sys_syslog(int type, char __user * buf, int len)
{
	return 0;
}

int do_syslog(int type, char __user * buf, int len) { return 0; }
static void call_console_drivers(unsigned long start, unsigned long end) {}

#endif

/**
 * add_preferred_console - add a device to the list of preferred consoles.
 *
 * The last preferred console added will be used for kernel messages
 * and stdin/out/err for init.  Normally this is used by console_setup
 * above to handle user-supplied console arguments; however it can also
 * be used by arch-specific code either to override the user or more
 * commonly to provide a default console (ie from PROM variables) when
 * the user has not supplied one.
 */
int __init add_preferred_console(char *name, int idx, char *options)
{
	struct console_cmdline *c;
	int i;

	/*
	 *	See if this tty is not yet registered, and
	 *	if we have a slot free.
	 */
	for(i = 0; i < MAX_CMDLINECONSOLES && console_cmdline[i].name[0]; i++)
		if (strcmp(console_cmdline[i].name, name) == 0 &&
			  console_cmdline[i].index == idx) {
				selected_console = i;
				return 0;
		}
	if (i == MAX_CMDLINECONSOLES)
		return -E2BIG;
	if (selected_console == -1)
		selected_console = i;
	c = &console_cmdline[i];
	memcpy(c->name, name, sizeof(c->name));
	c->name[sizeof(c->name) - 1] = 0;
	c->options = options;
	c->index = idx;
	return 0;
}

/**
 * acquire_console_sem - lock the console system for exclusive use.
 *
 * Acquires a semaphore which guarantees that the caller has
 * exclusive access to the console system and the console_drivers list.
 *
 * Can sleep, returns nothing.
 */
void acquire_console_sem(void)
{
	if (in_interrupt())
		BUG();
	down(&console_sem);
	console_locked = 1;
	console_may_schedule = 1;
}
EXPORT_SYMBOL(acquire_console_sem);

int try_acquire_console_sem(void)
{
	if (down_trylock(&console_sem))
		return -1;
	console_locked = 1;
	console_may_schedule = 0;
	return 0;
}
EXPORT_SYMBOL(try_acquire_console_sem);

int is_console_locked(void)
{
	return console_locked;
}
EXPORT_SYMBOL(is_console_locked);

/**
 * release_console_sem - unlock the console system
 *
 * Releases the semaphore which the caller holds on the console system
 * and the console driver list.
 *
 * While the semaphore was held, console output may have been buffered
 * by printk().  If this is the case, release_console_sem() emits
 * the output prior to releasing the semaphore.
 *
 * If there is output waiting for klogd, we wake it up.
 *
 * release_console_sem() may be called from any context.
 */
void release_console_sem(void)
{
	unsigned long flags;
	unsigned long _con_start, _log_end;
	unsigned long wake_klogd = 0;

//	flush_log_buffer();
	for ( ; ; ) {
		spin_lock_irqsave(&logbuf_lock, flags);
		lock_hw_sem();
		wake_klogd |= log_start - log_end;
		if (con_start == log_end)
			break;			/* Nothing to print */
		_con_start = con_start;
#ifdef CONFIG_SHARED_PRINTK
		if (log_end - con_start > 100) {
			_log_end = _con_start+100;
			con_start = _log_end;
		} else {
			_log_end = log_end;
			con_start = log_end;	/* Flush */
		}
#else
		_log_end = log_end;
		con_start = log_end;		/* Flush */
#endif
		unlock_hw_sem();
		spin_unlock(&logbuf_lock);
		call_console_drivers(_con_start, _log_end);
		local_irq_restore(flags);
	}
	console_locked = 0;
	console_may_schedule = 0;
	up(&console_sem);
	unlock_hw_sem();
	spin_unlock_irqrestore(&logbuf_lock, flags);
	if (wake_klogd && !oops_in_progress && waitqueue_active(&log_wait))
		wake_up_interruptible(&log_wait);
}
EXPORT_SYMBOL(release_console_sem);

/** console_conditional_schedule - yield the CPU if required
 *
 * If the console code is currently allowed to sleep, and
 * if this CPU should yield the CPU to another task, do
 * so here.
 *
 * Must be called within acquire_console_sem().
 */
void __sched console_conditional_schedule(void)
{
	if (console_may_schedule)
		cond_resched();
}
EXPORT_SYMBOL(console_conditional_schedule);

void console_print(const char *s)
{
	printk(KERN_EMERG "%s", s);
}
EXPORT_SYMBOL(console_print);

void console_unblank(void)
{
	struct console *c;

	/*
	 * console_unblank can no longer be called in interrupt context unless
	 * oops_in_progress is set to 1..
	 */
	if (oops_in_progress) {
		if (down_trylock(&console_sem) != 0)
			return;
	} else
		acquire_console_sem();

	console_locked = 1;
	console_may_schedule = 0;
	for (c = console_drivers; c != NULL; c = c->next)
		if ((c->flags & CON_ENABLED) && c->unblank)
			c->unblank();
	release_console_sem();
}
EXPORT_SYMBOL(console_unblank);

/*
 * Return the console tty driver structure and its associated index
 */
struct tty_driver *console_device(int *index)
{
	struct console *c;
	struct tty_driver *driver = NULL;

	acquire_console_sem();
	for (c = console_drivers; c != NULL; c = c->next) {
		if (!c->device)
			continue;
		driver = c->device(c, index);
		if (driver)
			break;
	}
	release_console_sem();
	return driver;
}

/*
 * Prevent further output on the passed console device so that (for example)
 * serial drivers can disable console output before suspending a port, and can
 * re-enable output afterwards.
 */
void console_stop(struct console *console)
{
	acquire_console_sem();
	console->flags &= ~CON_ENABLED;
	release_console_sem();
}
EXPORT_SYMBOL(console_stop);

void console_start(struct console *console)
{
	acquire_console_sem();
	console->flags |= CON_ENABLED;
	release_console_sem();
}
EXPORT_SYMBOL(console_start);

/*
 * The console driver calls this routine during kernel initialization
 * to register the console printing procedure with printk() and to
 * print any messages that were printed by the kernel before the
 * console driver was initialized.
 */
void register_console(struct console * console)
{
	int     i;
	unsigned long flags;

	if (preferred_console < 0)
		preferred_console = selected_console;

	/*
	 *	See if we want to use this console driver. If we
	 *	didn't select a console we take the first one
	 *	that registers here.
	 */
	if (preferred_console < 0) {
		if (console->index < 0)
			console->index = 0;
		if (console->setup == NULL ||
		    console->setup(console, NULL) == 0) {
			console->flags |= CON_ENABLED | CON_CONSDEV;
			preferred_console = 0;
		}
	}

	/*
	 *	See if this console matches one we selected on
	 *	the command line.
	 */
	for(i = 0; i < MAX_CMDLINECONSOLES && console_cmdline[i].name[0]; i++) {
		if (strcmp(console_cmdline[i].name, console->name) != 0)
			continue;
		if (console->index >= 0 &&
		    console->index != console_cmdline[i].index)
			continue;
		if (console->index < 0)
			console->index = console_cmdline[i].index;
		if (console->setup &&
		    console->setup(console, console_cmdline[i].options) != 0)
			break;
		console->flags |= CON_ENABLED;
		console->index = console_cmdline[i].index;
		if (i == preferred_console)
			console->flags |= CON_CONSDEV;
		break;
	}

	if (!(console->flags & CON_ENABLED))
		return;

	if (console_drivers && (console_drivers->flags & CON_BOOT)) {
		unregister_console(console_drivers);
		console->flags &= ~CON_PRINTBUFFER;
	}

	/*
	 *	Put this console in the list - keep the
	 *	preferred driver at the head of the list.
	 */
	acquire_console_sem();
	if ((console->flags & CON_CONSDEV) || console_drivers == NULL) {
		console->next = console_drivers;
		console_drivers = console;
	} else {
		console->next = console_drivers->next;
		console_drivers->next = console;
	}
	if (console->flags & CON_PRINTBUFFER) {
		/*
		 * release_console_sem() will print out the buffered messages
		 * for us.
		 */
		spin_lock_irqsave(&logbuf_lock, flags);
		lock_hw_sem();
		con_start = log_start;
		unlock_hw_sem();
		spin_unlock_irqrestore(&logbuf_lock, flags);
	}
	release_console_sem();
}
EXPORT_SYMBOL(register_console);

int unregister_console(struct console * console)
{
        struct console *a,*b;
	int res = 1;

	acquire_console_sem();
	if (console_drivers == console) {
		console_drivers=console->next;
		res = 0;
	} else {
		for (a=console_drivers->next, b=console_drivers ;
		     a; b=a, a=b->next) {
			if (a == console) {
				b->next = a->next;
				res = 0;
				break;
			}  
		}
	}
	
	/* If last console is removed, we re-enable picking the first
	 * one that gets registered. Without that, pmac early boot console
	 * would prevent fbcon from taking over.
	 */
	if (console_drivers == NULL)
		preferred_console = selected_console;
		

	release_console_sem();
	return res;
}
EXPORT_SYMBOL(unregister_console);

/**
 * tty_write_message - write a message to a certain tty, not just the console.
 *
 * This is used for messages that need to be redirected to a specific tty.
 * We don't put it into the syslog queue right now maybe in the future if
 * really needed.
 */
void tty_write_message(struct tty_struct *tty, char *msg)
{
	if (tty && tty->driver->write)
		tty->driver->write(tty, msg, strlen(msg));
	return;
}

/*
 * printk rate limiting, lifted from the networking subsystem.
 *
 * This enforces a rate limit: not more than one kernel message
 * every printk_ratelimit_jiffies to make a denial-of-service
 * attack impossible.
 */
int __printk_ratelimit(int ratelimit_jiffies, int ratelimit_burst)
{
	static DEFINE_SPINLOCK(ratelimit_lock);
	static unsigned long toks = 10*5*HZ;
	static unsigned long last_msg;
	static int missed;
	unsigned long flags;
	unsigned long now = jiffies;

	spin_lock_irqsave(&ratelimit_lock, flags);
	toks += now - last_msg;
	last_msg = now;
	if (toks > (ratelimit_burst * ratelimit_jiffies))
		toks = ratelimit_burst * ratelimit_jiffies;
	if (toks >= ratelimit_jiffies) {
		int lost = missed;
		missed = 0;
		toks -= ratelimit_jiffies;
		spin_unlock_irqrestore(&ratelimit_lock, flags);
		if (lost)
			printk(KERN_WARNING "printk: %d messages suppressed.\n", lost);
		return 1;
	}
	missed++;
	spin_unlock_irqrestore(&ratelimit_lock, flags);
	return 0;
}
EXPORT_SYMBOL(__printk_ratelimit);

/* minimum time in jiffies between messages */
int printk_ratelimit_jiffies = 5*HZ;

/* number of messages we send before ratelimiting */
int printk_ratelimit_burst = 10;

int printk_ratelimit(void)
{
	return __printk_ratelimit(printk_ratelimit_jiffies,
				printk_ratelimit_burst);
}
EXPORT_SYMBOL(printk_ratelimit);
