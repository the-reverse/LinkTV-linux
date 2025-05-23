#ifndef _LINUX_KERNEL_H
#define _LINUX_KERNEL_H

/*
 * 'kernel.h' contains some often-used function prototypes etc
 */

#ifdef __KERNEL__

#include <stdarg.h>
#include <linux/linkage.h>
#include <linux/stddef.h>
#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/bitops.h>
#include <asm/byteorder.h>
#include <asm/bug.h>

extern const char linux_banner[];

#ifdef CONFIG_REALTEK_RESERVE_DVR
#define SYNC_STRUCT_ADDR 0xa00000cc
#define UNLZMA_SYNC_ADDR 0xa00000e0
#endif

#define INT_MAX		((int)(~0U>>1))
#define INT_MIN		(-INT_MAX - 1)
#define UINT_MAX	(~0U)
#define LONG_MAX	((long)(~0UL>>1))
#define LONG_MIN	(-LONG_MAX - 1)
#define ULONG_MAX	(~0UL)

#define STACK_MAGIC	0xdeadbeef

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#define ALIGN(x,a) (((x)+(a)-1)&~((a)-1))

#define	KERN_EMERG	"<0>"	/* system is unusable			*/
#define	KERN_ALERT	"<1>"	/* action must be taken immediately	*/
#define	KERN_CRIT	"<2>"	/* critical conditions			*/
#define	KERN_ERR	"<3>"	/* error conditions			*/
#define	KERN_WARNING	"<4>"	/* warning conditions			*/
#define	KERN_NOTICE	"<5>"	/* normal but significant condition	*/
#define	KERN_INFO	"<6>"	/* informational			*/
#define	KERN_DEBUG	"<7>"	/* debug-level messages			*/

extern int console_printk[];

#define console_loglevel (console_printk[0])
#define default_message_loglevel (console_printk[1])
#define minimum_console_loglevel (console_printk[2])
#define default_console_loglevel (console_printk[3])

struct completion;

/**
 * might_sleep - annotation for functions that can sleep
 *
 * this macro will print a stack trace if it is executed in an atomic
 * context (spinlock, irq-handler, ...).
 *
 * This is a useful debugging help to be able to catch problems early and not
 * be biten later when the calling function happens to sleep when it is not
 * supposed to.
 */
#ifdef CONFIG_DEBUG_SPINLOCK_SLEEP
#define might_sleep() __might_sleep(__FILE__, __LINE__)
#define might_sleep_if(cond) do { if (unlikely(cond)) might_sleep(); } while (0)
void __might_sleep(char *file, int line);
#else
#define might_sleep() do {} while(0)
#define might_sleep_if(cond) do {} while (0)
#endif

#define abs(x) ({				\
		int __x = (x);			\
		(__x < 0) ? -__x : __x;		\
	})

#define labs(x) ({				\
		long __x = (x);			\
		(__x < 0) ? -__x : __x;		\
	})

extern struct notifier_block *panic_notifier_list;
extern long (*panic_blink)(long time);
NORET_TYPE void panic(const char * fmt, ...)
	__attribute__ ((NORET_AND format (printf, 1, 2)));
fastcall NORET_TYPE void do_exit(long error_code)
	ATTRIB_NORET;
NORET_TYPE void complete_and_exit(struct completion *, long)
	ATTRIB_NORET;
extern unsigned long simple_strtoul(const char *,char **,unsigned int);
extern long simple_strtol(const char *,char **,unsigned int);
extern unsigned long long simple_strtoull(const char *,char **,unsigned int);
extern long long simple_strtoll(const char *,char **,unsigned int);
extern int sprintf(char * buf, const char * fmt, ...)
	__attribute__ ((format (printf, 2, 3)));
extern int vsprintf(char *buf, const char *, va_list)
	__attribute__ ((format (printf, 2, 0)));
extern int snprintf(char * buf, size_t size, const char * fmt, ...)
	__attribute__ ((format (printf, 3, 4)));
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
	__attribute__ ((format (printf, 3, 0)));
extern int scnprintf(char * buf, size_t size, const char * fmt, ...)
	__attribute__ ((format (printf, 3, 4)));
extern int vscnprintf(char *buf, size_t size, const char *fmt, va_list args)
	__attribute__ ((format (printf, 3, 0)));

extern int sscanf(const char *, const char *, ...)
	__attribute__ ((format (scanf, 2, 3)));
extern int vsscanf(const char *, const char *, va_list)
	__attribute__ ((format (scanf, 2, 0)));

extern int get_option(char **str, int *pint);
extern char *get_options(const char *str, int nints, int *ints);
extern unsigned long long memparse(char *ptr, char **retptr);

extern int __kernel_text_address(unsigned long addr);
extern int kernel_text_address(unsigned long addr);
extern int session_of_pgrp(int pgrp);

#ifdef CONFIG_PRINTK
asmlinkage int vprintk(const char *fmt, va_list args)
	__attribute__ ((format (printf, 1, 0)));
asmlinkage int printk(const char * fmt, ...)
	__attribute__ ((format (printf, 1, 2)));
#else
/*static inline int vprintk(const char *s, va_list args)
	__attribute__ ((format (printf, 1, 0)));
static inline int vprintk(const char *s, va_list args) { return 0; }
static inline int printk(const char *s, ...)
	__attribute__ ((format (printf, 1, 2)));
static inline int printk(const char *s, ...) { return 0; }
*/
// This definition can save space and the passed arguments like "i++" and "func()" can be run.
#define vprintk(fmt, args)      ((void)(fmt, args), 0)
#define printk(...)             (__VA_ARGS__, 0)
#endif
#ifdef CONFIG_REALTEK_WATCHPOINT
#define WATCH_W         0x1
#define WATCH_R         0x2
#define WATCH_I         0x4
int set_watch_point(unsigned long addr, unsigned long prot);
int clr_watch_point(void);
#endif
unsigned long int_sqrt(unsigned long);

static inline int __attribute_pure__ long_log2(unsigned long x)
{
	int r = 0;
	for (x >>= 1; x > 0; x >>= 1)
		r++;
	return r;
}

static inline unsigned long __attribute_const__ roundup_pow_of_two(unsigned long x)
{
	return (1UL << fls(x - 1));
}

extern int printk_ratelimit(void);
extern int __printk_ratelimit(int ratelimit_jiffies, int ratelimit_burst);

static inline void console_silent(void)
{
	console_loglevel = 0;
}

static inline void console_verbose(void)
{
	if (console_loglevel)
		console_loglevel = 15;
}

extern void bust_spinlocks(int yes);
extern int oops_in_progress;		/* If set, an oops, panic(), BUG() or die() is in progress */
extern int panic_timeout;
extern int panic_on_oops;
extern int tainted;
extern const char *print_tainted(void);
extern void add_taint(unsigned);

/* Values used for system_state */
extern enum system_states {
	SYSTEM_BOOTING,
	SYSTEM_RUNNING,
	SYSTEM_HALT,
	SYSTEM_POWER_OFF,
	SYSTEM_RESTART,
} system_state;

#define TAINT_PROPRIETARY_MODULE	(1<<0)
#define TAINT_FORCED_MODULE		(1<<1)
#define TAINT_UNSAFE_SMP		(1<<2)
#define TAINT_FORCED_RMMOD		(1<<3)
#define TAINT_MACHINE_CHECK		(1<<4)
#define TAINT_BAD_PAGE			(1<<5)

extern void dump_stack(void);

#ifdef DEBUG
#define pr_debug(fmt,arg...) \
	printk(KERN_DEBUG fmt,##arg)
#else
#define pr_debug(fmt,arg...) \
	do { } while (0)
#endif

#define pr_info(fmt,arg...) \
	printk(KERN_INFO fmt,##arg)

/*
 *      Display an IP address in readable format.
 */

#define NIPQUAD(addr) \
	((unsigned char *)&addr)[0], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[3]

#define NIP6(addr) \
	ntohs((addr).s6_addr16[0]), \
	ntohs((addr).s6_addr16[1]), \
	ntohs((addr).s6_addr16[2]), \
	ntohs((addr).s6_addr16[3]), \
	ntohs((addr).s6_addr16[4]), \
	ntohs((addr).s6_addr16[5]), \
	ntohs((addr).s6_addr16[6]), \
	ntohs((addr).s6_addr16[7])

#if defined(__LITTLE_ENDIAN)
#define HIPQUAD(addr) \
	((unsigned char *)&addr)[3], \
	((unsigned char *)&addr)[2], \
	((unsigned char *)&addr)[1], \
	((unsigned char *)&addr)[0]
#elif defined(__BIG_ENDIAN)
#define HIPQUAD	NIPQUAD
#else
#error "Please fix asm/byteorder.h"
#endif /* __LITTLE_ENDIAN */

/*
 * min()/max() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define min(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);		\
	_x < _y ? _x : _y; })

#define max(x,y) ({ \
	typeof(x) _x = (x);	\
	typeof(y) _y = (y);	\
	(void) (&_x == &_y);		\
	_x > _y ? _x : _y; })

/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max at all, of course.
 */
#define min_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x < __y ? __x: __y; })
#define max_t(type,x,y) \
	({ type __x = (x); type __y = (y); __x > __y ? __x: __y; })


/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({			\
        const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
        (type *)( (char *)__mptr - offsetof(type,member) );})

/*
 * Check at compile time that something is of a particular type.
 * Always evaluates to 1 so you may use it easily in comparisons.
 */
#define typecheck(type,x) \
({	type __dummy; \
	typeof(x) __dummy2; \
	(void)(&__dummy == &__dummy2); \
	1; \
})

#endif /* __KERNEL__ */

#define SI_LOAD_SHIFT	16
struct sysinfo {
	long uptime;			/* Seconds since boot */
	unsigned long loads[3];		/* 1, 5, and 15 minute load averages */
	unsigned long totalram;		/* Total usable main memory size */
	unsigned long freeram;		/* Available memory size */
	unsigned long sharedram;	/* Amount of shared memory */
	unsigned long bufferram;	/* Memory used by buffers */
	unsigned long totalswap;	/* Total swap space size */
	unsigned long freeswap;		/* swap space still available */
	unsigned short procs;		/* Number of current processes */
	unsigned short pad;		/* explicit padding for m68k */
	unsigned long totalhigh;	/* Total high memory size */
	unsigned long freehigh;		/* Available high memory size */
	unsigned int mem_unit;		/* Memory unit size in bytes */
	char _f[20-2*sizeof(long)-sizeof(int)];	/* Padding: libc5 uses this.. */
};

extern void BUILD_BUG(void);
#define BUILD_BUG_ON(condition) do { if (condition) BUILD_BUG(); } while(0)

#ifdef  CONFIG_REALTEK_SCHED_LOG
extern unsigned int            *buf_head;
extern unsigned int            *buf_tail;
extern unsigned int            *buf_ptr;
extern unsigned int            sched_log_flag;
extern unsigned int            time_scale;
static inline void log_sched(int pid)
{
	unsigned int count;

	asm ("mfc0 %0, $9;": "=r"(count));
	count /= time_scale;
	*buf_ptr = count;
	if (++buf_ptr == buf_tail) {
		buf_ptr = buf_head;
		sched_log_flag |= 2;
	}

	*buf_ptr = 0xc0000000 | pid;
	if (++buf_ptr == buf_tail) {
		buf_ptr = buf_head;
		sched_log_flag |= 2;
	}
}

static inline void log_event(int event)
{
	unsigned int count;
	unsigned int status;

	asm ("mfc0 %0, $12;": "=r"(status));
	asm ("mtc0 %0, $12;": : "r"(status & 0xfffffffe));
	asm ("mfc0 %0, $9;": "=r"(count));
	count /= time_scale;
	*buf_ptr = count;
	if (++buf_ptr == buf_tail) {
		buf_ptr = buf_head;
		sched_log_flag |= 2;
	}

	*buf_ptr = 0x20000000 | event;
	if (++buf_ptr == buf_tail) {
		buf_ptr = buf_head;
		sched_log_flag |= 2;
	}
	asm ("mtc0 %0, $12;": : "r"(status));
}

static inline void log_intr_enter(int irq)
{
	unsigned int count;

	asm ("mfc0 %0, $9;": "=r"(count));
	count /= time_scale;
	*buf_ptr = count;
	if (++buf_ptr == buf_tail) {
		buf_ptr = buf_head;
		sched_log_flag |= 2;
	}

	*buf_ptr = 0x10000000 | irq;
	if (++buf_ptr == buf_tail) {
		buf_ptr = buf_head;
		sched_log_flag |= 2;
	}
}

static inline void log_intr_exit(int irq)
{
	unsigned int count;

	asm ("mfc0 %0, $9;": "=r"(count));
	count /= time_scale;
	*buf_ptr = count;
	if (++buf_ptr == buf_tail) {
		buf_ptr = buf_head;
		sched_log_flag |= 2;
	}

	*buf_ptr = irq;
	if (++buf_ptr == buf_tail) {
		buf_ptr = buf_head;
		sched_log_flag |= 2;
	}
}
#endif

#ifdef CONFIG_SYSCTL
extern int randomize_va_space;
#else
#define randomize_va_space 1
#endif

/* Trap pasters of __FUNCTION__ at compile-time */
#if __GNUC__ > 2 || __GNUC_MINOR__ >= 95
#define __FUNCTION__ (__func__)
#endif

#endif
