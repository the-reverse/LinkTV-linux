#ifndef _ASM_IA64_IO_H
#define _ASM_IA64_IO_H

/*
 * This file contains the definitions for the emulated IO instructions
 * inb/inw/inl/outb/outw/outl and the "string versions" of the same
 * (insb/insw/insl/outsb/outsw/outsl). You can also use "pausing"
 * versions of the single-IO instructions (inb_p/inw_p/..).
 *
 * This file is not meant to be obfuscating: it's just complicated to
 * (a) handle it all in a way that makes gcc able to optimize it as
 * well as possible and (b) trying to avoid writing the same thing
 * over and over again with slight variations and possibly making a
 * mistake somewhere.
 *
 * Copyright (C) 1998, 1999 Hewlett-Packard Co
 * Copyright (C) 1998, 1999 David Mosberger-Tang <davidm@hpl.hp.com>
 * Copyright (C) 1999 Asit Mallick <asit.k.mallick@intel.com>
 * Copyright (C) 1999 Don Dugger <don.dugger@intel.com>
 */

/* We don't use IO slowdowns on the ia64, but.. */
#define __SLOW_DOWN_IO	do { } while (0)
#define SLOW_DOWN_IO	do { } while (0)

#define __IA64_UNCACHED_OFFSET	0xc000000000000000	/* region 6 */

#define IO_SPACE_LIMIT 0xffff

# ifdef __KERNEL__

#include <asm/page.h>
#include <asm/system.h>

/*
 * Change virtual addresses to physical addresses and vv.
 */
static inline unsigned long
virt_to_phys (volatile void *address)
{
	return (unsigned long) address - PAGE_OFFSET;
}

static inline void*
phys_to_virt(unsigned long address)
{
	return (void *) (address + PAGE_OFFSET);
}

#define bus_to_virt	phys_to_virt
#define virt_to_bus	virt_to_phys

# else /* !KERNEL */
# endif /* !KERNEL */

/*
 * Memory fence w/accept.  This should never be used in code that is
 * not IA-64 specific.
 */
#define __ia64_mf_a()	__asm__ __volatile__ ("mf.a" ::: "memory")

extern inline const unsigned long
__ia64_get_io_port_base (void)
{
	unsigned long addr;

	__asm__ ("mov %0=ar.k0;;" : "=r"(addr));
	return __IA64_UNCACHED_OFFSET | addr;
}

extern inline void*
__ia64_mk_io_addr (unsigned long port)
{
	const unsigned long io_base = __ia64_get_io_port_base();
	unsigned long addr;

	addr = io_base | ((port >> 2) << 12) | (port & 0xfff);
	return (void *) addr;
}

/*
 * For the in/out instructions, we need to do:
 *
 *	o "mf" _before_ doing the I/O access to ensure that all prior
 *	  accesses to memory occur before the I/O access
 *	o "mf.a" _after_ doing the I/O access to ensure that the access
 *	  has completed before we're doing any other I/O accesses
 *
 * The former is necessary because we might be doing normal (cached) memory
 * accesses, e.g., to set up a DMA descriptor table and then do an "outX()"
 * to tell the DMA controller to start the DMA operation.  The "mf" ahead
 * of the I/O operation ensures that the DMA table is correct when the I/O
 * access occurs.
 *
 * The mf.a is necessary to ensure that all I/O access occur in program
 * order. --davidm 99/12/07 
 */

extern inline unsigned int
__inb (unsigned long port)
{
	volatile unsigned char *addr = __ia64_mk_io_addr(port);
	unsigned char ret;

	ret = *addr;
	__ia64_mf_a();
	return ret;
}

extern inline unsigned int
__inw (unsigned long port)
{
	volatile unsigned short *addr = __ia64_mk_io_addr(port);
	unsigned short ret;

	ret = *addr;
	__ia64_mf_a();
	return ret;
}

extern inline unsigned int
__inl (unsigned long port)
{
	volatile unsigned int *addr = __ia64_mk_io_addr(port);
	unsigned int ret;

	ret = *addr;
	__ia64_mf_a();
	return ret;
}

extern inline void
__insb (unsigned long port, void *dst, unsigned long count)
{
	volatile unsigned char *addr = __ia64_mk_io_addr(port);
	unsigned char *dp = dst;

	__ia64_mf_a();
	while (count--) {
		*dp++ = *addr;
	}
	__ia64_mf_a();
	return;
}

extern inline void
__insw (unsigned long port, void *dst, unsigned long count)
{
	volatile unsigned short *addr = __ia64_mk_io_addr(port);
	unsigned short *dp = dst;

	__ia64_mf_a();
	while (count--) {
		*dp++ = *addr;
	}
	__ia64_mf_a();
	return;
}

extern inline void
__insl (unsigned long port, void *dst, unsigned long count)
{
	volatile unsigned int *addr = __ia64_mk_io_addr(port);
	unsigned int *dp = dst;

	__ia64_mf_a();
	while (count--) {
		*dp++ = *addr;
	}
	__ia64_mf_a();
	return;
}

extern inline void
__outb (unsigned char val, unsigned long port)
{
	volatile unsigned char *addr = __ia64_mk_io_addr(port);

	*addr = val;
	__ia64_mf_a();
}

extern inline void
__outw (unsigned short val, unsigned long port)
{
	volatile unsigned short *addr = __ia64_mk_io_addr(port);

	*addr = val;
	__ia64_mf_a();
}

extern inline void
__outl (unsigned int val, unsigned long port)
{
	volatile unsigned int *addr = __ia64_mk_io_addr(port);

	*addr = val;
	__ia64_mf_a();
}

extern inline void
__outsb (unsigned long port, const void *src, unsigned long count)
{
	volatile unsigned char *addr = __ia64_mk_io_addr(port);
	const unsigned char *sp = src;

	while (count--) {
		*addr = *sp++;
	}
	__ia64_mf_a();
	return;
}

extern inline void
__outsw (unsigned long port, const void *src, unsigned long count)
{
	volatile unsigned short *addr = __ia64_mk_io_addr(port);
	const unsigned short *sp = src;

	while (count--) {
		*addr = *sp++;
	}
	__ia64_mf_a();
	return;
}

extern inline void
__outsl (unsigned long port, void *src, unsigned long count)
{
	volatile unsigned int *addr = __ia64_mk_io_addr(port);
	const unsigned int *sp = src;

	while (count--) {
		*addr = *sp++;
	}
	__ia64_mf_a();
	return;
}

#define inb		__inb
#define inw		__inw
#define inl		__inl
#define insb		__insb
#define insw		__insw
#define insl		__insl
#define outb		__outb
#define outw		__outw
#define outl		__outl
#define outsb		__outsb
#define outsw		__outsw
#define outsl		__outsl

/*
 * The address passed to these functions are ioremap()ped already.
 */
extern inline unsigned long
__readb (unsigned long addr)
{
	return *(volatile unsigned char *)addr;
}

extern inline unsigned long
__readw (unsigned long addr)
{
	return *(volatile unsigned short *)addr;
}

extern inline unsigned long
__readl (unsigned long addr)
{
	return *(volatile unsigned int *) addr;
}

extern inline unsigned long
__readq (unsigned long addr)
{
	return *(volatile unsigned long *) addr;
}

extern inline void
__writeb (unsigned char val, unsigned long addr)
{
	*(volatile unsigned char *) addr = val;
}

extern inline void
__writew (unsigned short val, unsigned long addr)
{
	*(volatile unsigned short *) addr = val;
}

extern inline void
__writel (unsigned int val, unsigned long addr)
{
	*(volatile unsigned int *) addr = val;
}

extern inline void
__writeq (unsigned long val, unsigned long addr)
{
	*(volatile unsigned long *) addr = val;
}

#define readb		__readb
#define readw		__readw
#define readl		__readl
#define readq		__readqq
#define __raw_readb	readb
#define __raw_readw	readw
#define __raw_readl	readl
#define __raw_readq	readq
#define writeb		__writeb
#define writew		__writew
#define writel		__writel
#define writeq		__writeq
#define __raw_writeb	writeb
#define __raw_writew	writew
#define __raw_writeq	writeq

#ifndef inb_p
# define inb_p		inb
#endif
#ifndef inw_p
# define inw_p		inw
#endif
#ifndef inl_p
# define inl_p		inl
#endif

#ifndef outb_p
# define outb_p		outb
#endif
#ifndef outw_p
# define outw_p		outw
#endif
#ifndef outl_p
# define outl_p		outl
#endif

/*
 * An "address" in IO memory space is not clearly either an integer
 * or a pointer. We will accept both, thus the casts.
 *
 * On ia-64, we access the physical I/O memory space through the
 * uncached kernel region.
 */
static inline void *
ioremap (unsigned long offset, unsigned long size)
{
	return (void *) (__IA64_UNCACHED_OFFSET | (offset));
} 

static inline void
iounmap (void *addr)
{
}

#define ioremap_nocache(o,s)	ioremap(o,s)

# ifdef __KERNEL__

/*
 * String version of IO memory access ops:
 */
extern void __ia64_memcpy_fromio (void *, unsigned long, long);
extern void __ia64_memcpy_toio (unsigned long, void *, long);
extern void __ia64_memset_c_io (unsigned long, unsigned long, long);

#define memcpy_fromio(to,from,len) \
  __ia64_memcpy_fromio((to),(unsigned long)(from),(len))
#define memcpy_toio(to,from,len) \
  __ia64_memcpy_toio((unsigned long)(to),(from),(len))
#define memset_io(addr,c,len) \
  __ia64_memset_c_io((unsigned long)(addr),0x0101010101010101UL*(u8)(c),(len))

#define __HAVE_ARCH_MEMSETW_IO
#define memsetw_io(addr,c,len) \
  _memset_c_io((unsigned long)(addr),0x0001000100010001UL*(u16)(c),(len))

/*
 * XXX - We don't have csum_partial_copy_fromio() yet, so we cheat here and 
 * just copy it. The net code will then do the checksum later. Presently 
 * only used by some shared memory 8390 Ethernet cards anyway.
 */

#define eth_io_copy_and_sum(skb,src,len,unused)		memcpy_fromio((skb)->data,(src),(len))

#if 0

/*
 * XXX this is the kind of legacy stuff we want to get rid of with IA-64... --davidm 99/12/02
 */

/*
 * This is used for checking BIOS signatures.  It's not clear at all
 * why this is here.  This implementation seems to be the same on
 * all architectures.  Strange.
 */
static inline int
check_signature (unsigned long io_addr, const unsigned char *signature, int length)
{
	int retval = 0;
	do {
		if (readb(io_addr) != *signature)
			goto out;
		io_addr++;
		signature++;
		length--;
	} while (length);
	retval = 1;
out:
	return retval;
}

#define RTC_PORT(x)		(0x70 + (x))
#define RTC_ALWAYS_BCD		0

#endif

/*
 * The caches on some architectures aren't DMA-coherent and have need
 * to handle this in software.  There are two types of operations that
 * can be applied to dma buffers.
 *
 * - dma_cache_inv(start, size) invalidates the affected parts of the
 *   caches.  Dirty lines of the caches may be written back or simply
 *   be discarded.  This operation is necessary before dma operations
 *   to the memory.
 *
 * - dma_cache_wback(start, size) makes caches and memory coherent
 *   by writing the content of the caches back to memory, if necessary
 *   (cache flush).
 *
 * - dma_cache_wback_inv(start, size) Like dma_cache_wback() but the
 *   function also invalidates the affected part of the caches as
 *   necessary before DMA transfers from outside to memory.
 *
 * Fortunately, the IA-64 architecture mandates cache-coherent DMA, so
 * these functions can be implemented as no-ops.
 */
#define dma_cache_inv(_start,_size)		do { } while (0)
#define dma_cache_wback(_start,_size)		do { } while (0)
#define dma_cache_wback_inv(_start,_size)	do { } while (0)

# endif /* __KERNEL__ */
#endif /* _ASM_IA64_IO_H */
