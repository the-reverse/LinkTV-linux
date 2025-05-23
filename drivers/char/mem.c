/*
 *  linux/drivers/char/mem.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  Added devfs support. 
 *    Jan-11-1998, C. Scott Ananian <cananian@alumni.princeton.edu>
 *  Shared /dev/zero mmaping support, Feb 2000, Kanoj Sarcar <kanoj@sgi.com>
 */

#include <linux/config.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/smp_lock.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/backing-dev.h>
#include <linux/pagemap.h>

#include <asm/uaccess.h>
#include <asm/io.h>

#ifdef CONFIG_IA64
# include <linux/efi.h>
#endif

#if defined(CONFIG_S390_TAPE) && defined(CONFIG_S390_TAPE_CHAR)
extern void tapechar_init(void);
#endif

/*
 * Architectures vary in how they handle caching for addresses
 * outside of main memory.
 *
 */
static inline int uncached_access(struct file *file, unsigned long addr)
{
#if defined(__i386__)
	/*
	 * On the PPro and successors, the MTRRs are used to set
	 * memory types for physical addresses outside main memory,
	 * so blindly setting PCD or PWT on those pages is wrong.
	 * For Pentiums and earlier, the surround logic should disable
	 * caching for the high addresses through the KEN pin, but
	 * we maintain the tradition of paranoia in this code.
	 */
	if (file->f_flags & O_SYNC)
		return 1;
 	return !( test_bit(X86_FEATURE_MTRR, boot_cpu_data.x86_capability) ||
		  test_bit(X86_FEATURE_K6_MTRR, boot_cpu_data.x86_capability) ||
		  test_bit(X86_FEATURE_CYRIX_ARR, boot_cpu_data.x86_capability) ||
		  test_bit(X86_FEATURE_CENTAUR_MCR, boot_cpu_data.x86_capability) )
	  && addr >= __pa(high_memory);
#elif defined(__x86_64__)
	/* 
	 * This is broken because it can generate memory type aliases,
	 * which can cause cache corruptions
	 * But it is only available for root and we have to be bug-to-bug
	 * compatible with i386.
	 */
	if (file->f_flags & O_SYNC)
		return 1;
	/* same behaviour as i386. PAT always set to cached and MTRRs control the
	   caching behaviour. 
	   Hopefully a full PAT implementation will fix that soon. */	   
	return 0;
#elif defined(CONFIG_IA64)
	/*
	 * On ia64, we ignore O_SYNC because we cannot tolerate memory attribute aliases.
	 */
	return !(efi_mem_attributes(addr) & EFI_MEMORY_WB);
#else
	/*
	 * Accessing memory above the top the kernel knows about or through a file pointer
	 * that was marked O_SYNC will be done non-cached.
	 */
	if (file->f_flags & O_SYNC)
		return 1;
	return addr >= __pa(high_memory);
#endif
}

#ifndef ARCH_HAS_VALID_PHYS_ADDR_RANGE
static inline int valid_phys_addr_range(unsigned long addr, size_t *count)
{
	unsigned long end_mem;

	end_mem = __pa(high_memory);
	if (addr >= end_mem)
		return 0;

	if (*count > end_mem - addr)
		*count = end_mem - addr;

	return 1;
}
#endif

/*
 * This funcion reads the *physical* memory. The f_pos points directly to the 
 * memory location. 
 */
static ssize_t read_mem(struct file * file, char __user * buf,
			size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t read, sz;
	char *ptr;

	if (!valid_phys_addr_range(p, &count))
		return -EFAULT;
	read = 0;
#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
	/* we don't have page 0 mapped on sparc and m68k.. */
	if (p < PAGE_SIZE) {
		sz = PAGE_SIZE - p;
		if (sz > count) 
			sz = count; 
		if (sz > 0) {
			if (clear_user(buf, sz))
				return -EFAULT;
			buf += sz; 
			p += sz; 
			count -= sz; 
			read += sz; 
		}
	}
#endif

	while (count > 0) {
		/*
		 * Handle first page in case it's not aligned
		 */
		if (-p & (PAGE_SIZE - 1))
			sz = -p & (PAGE_SIZE - 1);
		else
			sz = PAGE_SIZE;

		sz = min_t(unsigned long, sz, count);

		/*
		 * On ia64 if a page has been mapped somewhere as
		 * uncached, then it must also be accessed uncached
		 * by the kernel or data corruption may occur
		 */
		ptr = xlate_dev_mem_ptr(p);

		if (copy_to_user(buf, ptr, sz))
			return -EFAULT;
		buf += sz;
		p += sz;
		count -= sz;
		read += sz;
	}

	*ppos += read;
	return read;
}

static ssize_t write_mem(struct file * file, const char __user * buf, 
			 size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t written, sz;
	unsigned long copied;
	void *ptr;

	if (!valid_phys_addr_range(p, &count))
		return -EFAULT;

	written = 0;

#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
	/* we don't have page 0 mapped on sparc and m68k.. */
	if (p < PAGE_SIZE) {
		unsigned long sz = PAGE_SIZE - p;
		if (sz > count)
			sz = count;
		/* Hmm. Do something? */
		buf += sz;
		p += sz;
		count -= sz;
		written += sz;
	}
#endif

	while (count > 0) {
		/*
		 * Handle first page in case it's not aligned
		 */
		if (-p & (PAGE_SIZE - 1))
			sz = -p & (PAGE_SIZE - 1);
		else
			sz = PAGE_SIZE;

		sz = min_t(unsigned long, sz, count);

		/*
		 * On ia64 if a page has been mapped somewhere as
		 * uncached, then it must also be accessed uncached
		 * by the kernel or data corruption may occur
		 */
		ptr = xlate_dev_mem_ptr(p);

		copied = copy_from_user(ptr, buf, sz);
		if (copied) {
			ssize_t ret;

			ret = written + (sz - copied);
			if (ret)
				return ret;
			return -EFAULT;
		}
		buf += sz;
		p += sz;
		count -= sz;
		written += sz;
	}

	*ppos += written;
	return written;
}

static int mem_populate(struct vm_area_struct *vma, unsigned long addr,
		unsigned long len, pgprot_t prot, unsigned long pgoff,
		int nonblock)
{
	struct mm_struct *mm = vma->vm_mm;
	int err;

repeat:
	err = install_phys_pte(mm, vma, addr, pgoff, prot);
	if (err)
		return err;

	len -= PAGE_SIZE;
	addr += PAGE_SIZE;
	pgoff++;
	if (len)
		goto repeat;

	return 0;
}

static struct vm_operations_struct mem_vm_ops = {
	.populate	= mem_populate,
};

static int mmap_mem(struct file * file, struct vm_area_struct * vma)
{
	unsigned long prot;
#if defined(__HAVE_PHYS_MEM_ACCESS_PROT)
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;

	vma->vm_page_prot = phys_mem_access_prot(file, offset,
						 vma->vm_end - vma->vm_start,
						 vma->vm_page_prot);
#elif defined(pgprot_noncached)
	unsigned long offset = vma->vm_pgoff << PAGE_SHIFT;
	int uncached;

	uncached = uncached_access(file, offset);
	if (uncached)
		vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
#endif

	prot = pgprot_val(vma->vm_page_prot);
//	prot = (prot & ~_CACHE_MASK) | _CACHE_UNCACHED;
	if (prot & _PAGE_WRITE)
		prot = prot | _PAGE_FILE | _PAGE_VALID | _PAGE_DIRTY;
	else
		prot = prot | _PAGE_FILE | _PAGE_VALID;
	prot &= ~_PAGE_PRESENT;
	vma->vm_page_prot = __pgprot(prot);

	vma->vm_flags |= VM_NONLINEAR;
	vma->vm_ops = &mem_vm_ops;

	/* Remap-pfn-range will mark the range VM_IO and VM_RESERVED */
	if (remap_pfn_range(vma,
			    vma->vm_start,
			    vma->vm_pgoff,
			    vma->vm_end-vma->vm_start,
			    vma->vm_page_prot))
		return -EAGAIN;
	return 0;
}

static int mmap_kmem(struct file * file, struct vm_area_struct * vma)
{
        unsigned long long val;
	/*
	 * RED-PEN: on some architectures there is more mapped memory
	 * than available in mem_map which pfn_valid checks
	 * for. Perhaps should add a new macro here.
	 *
	 * RED-PEN: vmalloc is not supported right now.
	 */
	if (!pfn_valid(vma->vm_pgoff))
		return -EIO;
	val = (u64)vma->vm_pgoff << PAGE_SHIFT;
	vma->vm_pgoff = __pa(val) >> PAGE_SHIFT;
	return mmap_mem(file, vma);
}

extern long vread(char *buf, char *addr, unsigned long count);
extern long vwrite(char *buf, char *addr, unsigned long count);

/*
 * This function reads the *virtual* memory as seen by the kernel.
 */
static ssize_t read_kmem(struct file *file, char __user *buf, 
			 size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t low_count, read, sz;
	char * kbuf; /* k-addr because vread() takes vmlist_lock rwlock */

	read = 0;
	if (p < (unsigned long) high_memory) {
		low_count = count;
		if (count > (unsigned long) high_memory - p)
			low_count = (unsigned long) high_memory - p;

#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
		/* we don't have page 0 mapped on sparc and m68k.. */
		if (p < PAGE_SIZE && low_count > 0) {
			size_t tmp = PAGE_SIZE - p;
			if (tmp > low_count) tmp = low_count;
			if (clear_user(buf, tmp))
				return -EFAULT;
			buf += tmp;
			p += tmp;
			read += tmp;
			low_count -= tmp;
			count -= tmp;
		}
#endif
		while (low_count > 0) {
			/*
			 * Handle first page in case it's not aligned
			 */
			if (-p & (PAGE_SIZE - 1))
				sz = -p & (PAGE_SIZE - 1);
			else
				sz = PAGE_SIZE;

			sz = min_t(unsigned long, sz, low_count);

			/*
			 * On ia64 if a page has been mapped somewhere as
			 * uncached, then it must also be accessed uncached
			 * by the kernel or data corruption may occur
			 */
			kbuf = xlate_dev_kmem_ptr((char *)p);

			if (copy_to_user(buf, kbuf, sz))
				return -EFAULT;
			buf += sz;
			p += sz;
			read += sz;
			low_count -= sz;
			count -= sz;
		}
	}

	if (count > 0) {
		kbuf = (char *)__get_free_page(GFP_KERNEL);
		if (!kbuf)
			return -ENOMEM;
		while (count > 0) {
			int len = count;

			if (len > PAGE_SIZE)
				len = PAGE_SIZE;
			len = vread(kbuf, (char *)p, len);
			if (!len)
				break;
			if (copy_to_user(buf, kbuf, len)) {
				free_page((unsigned long)kbuf);
				return -EFAULT;
			}
			count -= len;
			buf += len;
			read += len;
			p += len;
		}
		free_page((unsigned long)kbuf);
	}
 	*ppos = p;
 	return read;
}


static inline ssize_t
do_write_kmem(void *p, unsigned long realp, const char __user * buf,
	      size_t count, loff_t *ppos)
{
	ssize_t written, sz;
	unsigned long copied;

	written = 0;
#ifdef __ARCH_HAS_NO_PAGE_ZERO_MAPPED
	/* we don't have page 0 mapped on sparc and m68k.. */
	if (realp < PAGE_SIZE) {
		unsigned long sz = PAGE_SIZE - realp;
		if (sz > count)
			sz = count;
		/* Hmm. Do something? */
		buf += sz;
		p += sz;
		realp += sz;
		count -= sz;
		written += sz;
	}
#endif

	while (count > 0) {
		char *ptr;
		/*
		 * Handle first page in case it's not aligned
		 */
		if (-realp & (PAGE_SIZE - 1))
			sz = -realp & (PAGE_SIZE - 1);
		else
			sz = PAGE_SIZE;

		sz = min_t(unsigned long, sz, count);

		/*
		 * On ia64 if a page has been mapped somewhere as
		 * uncached, then it must also be accessed uncached
		 * by the kernel or data corruption may occur
		 */
		ptr = xlate_dev_kmem_ptr(p);

		copied = copy_from_user(ptr, buf, sz);
		if (copied) {
			ssize_t ret;

			ret = written + (sz - copied);
			if (ret)
				return ret;
			return -EFAULT;
		}
		buf += sz;
		p += sz;
		realp += sz;
		count -= sz;
		written += sz;
	}

	*ppos += written;
	return written;
}


/*
 * This function writes to the *virtual* memory as seen by the kernel.
 */
static ssize_t write_kmem(struct file * file, const char __user * buf, 
			  size_t count, loff_t *ppos)
{
	unsigned long p = *ppos;
	ssize_t wrote = 0;
	ssize_t virtr = 0;
	ssize_t written;
	char * kbuf; /* k-addr because vwrite() takes vmlist_lock rwlock */

	if (p < (unsigned long) high_memory) {

		wrote = count;
		if (count > (unsigned long) high_memory - p)
			wrote = (unsigned long) high_memory - p;

		written = do_write_kmem((void*)p, p, buf, wrote, ppos);
		if (written != wrote)
			return written;
		wrote = written;
		p += wrote;
		buf += wrote;
		count -= wrote;
	}

	if (count > 0) {
		kbuf = (char *)__get_free_page(GFP_KERNEL);
		if (!kbuf)
			return wrote ? wrote : -ENOMEM;
		while (count > 0) {
			int len = count;

			if (len > PAGE_SIZE)
				len = PAGE_SIZE;
			if (len) {
				written = copy_from_user(kbuf, buf, len);
				if (written) {
					ssize_t ret;

					free_page((unsigned long)kbuf);
					ret = wrote + virtr + (len - written);
					return ret ? ret : -EFAULT;
				}
			}
			len = vwrite(kbuf, (char *)p, len);
			count -= len;
			buf += len;
			virtr += len;
			p += len;
		}
		free_page((unsigned long)kbuf);
	}

 	*ppos = p;
 	return virtr + wrote;
}

#if defined(CONFIG_ISA) || !defined(__mc68000__)
static ssize_t read_port(struct file * file, char __user * buf,
			 size_t count, loff_t *ppos)
{
	unsigned long i = *ppos;
	char __user *tmp = buf;

	if (!access_ok(VERIFY_WRITE, buf, count))
		return -EFAULT; 
	while (count-- > 0 && i < 65536) {
		if (__put_user(inb(i),tmp) < 0) 
			return -EFAULT;  
		i++;
		tmp++;
	}
	*ppos = i;
	return tmp-buf;
}

static ssize_t write_port(struct file * file, const char __user * buf,
			  size_t count, loff_t *ppos)
{
	unsigned long i = *ppos;
	const char __user * tmp = buf;

	if (!access_ok(VERIFY_READ,buf,count))
		return -EFAULT;
	while (count-- > 0 && i < 65536) {
		char c;
		if (__get_user(c, tmp)) 
			return -EFAULT; 
		outb(c,i);
		i++;
		tmp++;
	}
	*ppos = i;
	return tmp-buf;
}
#endif

static ssize_t read_null(struct file * file, char __user * buf,
			 size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t write_null(struct file * file, const char __user * buf,
			  size_t count, loff_t *ppos)
{
	return count;
}

#ifdef CONFIG_MMU
/*
 * For fun, we are using the MMU for this.
 */
static inline size_t read_zero_pagealigned(char __user * buf, size_t size)
{
	struct mm_struct *mm;
	struct vm_area_struct * vma;
	unsigned long addr=(unsigned long)buf;

	mm = current->mm;
	/* Oops, this was forgotten before. -ben */
	down_read(&mm->mmap_sem);

	/* For private mappings, just map in zero pages. */
	for (vma = find_vma(mm, addr); vma; vma = vma->vm_next) {
		unsigned long count;

		if (vma->vm_start > addr || (vma->vm_flags & VM_WRITE) == 0)
			goto out_up;
		if (vma->vm_flags & (VM_SHARED | VM_HUGETLB))
			break;
		count = vma->vm_end - addr;
		if (count > size)
			count = size;

		zap_page_range(vma, addr, count, NULL);
        	zeromap_page_range(vma, addr, count, PAGE_COPY);

		size -= count;
		buf += count;
		addr += count;
		if (size == 0)
			goto out_up;
	}

	up_read(&mm->mmap_sem);
	
	/* The shared case is hard. Let's do the conventional zeroing. */ 
	do {
		unsigned long unwritten = clear_user(buf, PAGE_SIZE);
		if (unwritten)
			return size + unwritten - PAGE_SIZE;
		cond_resched();
		buf += PAGE_SIZE;
		size -= PAGE_SIZE;
	} while (size);

	return size;
out_up:
	up_read(&mm->mmap_sem);
	return size;
}

static ssize_t read_zero(struct file * file, char __user * buf, 
			 size_t count, loff_t *ppos)
{
	unsigned long left, unwritten, written = 0;

	if (!count)
		return 0;

	if (!access_ok(VERIFY_WRITE, buf, count))
		return -EFAULT;

	left = count;

	/* do we want to be clever? Arbitrary cut-off */
	if (count >= PAGE_SIZE*4) {
		unsigned long partial;

		/* How much left of the page? */
		partial = (PAGE_SIZE-1) & -(unsigned long) buf;
		unwritten = clear_user(buf, partial);
		written = partial - unwritten;
		if (unwritten)
			goto out;
		left -= partial;
		buf += partial;
		unwritten = read_zero_pagealigned(buf, left & PAGE_MASK);
		written += (left & PAGE_MASK) - unwritten;
		if (unwritten)
			goto out;
		buf += left & PAGE_MASK;
		left &= ~PAGE_MASK;
	}
	unwritten = clear_user(buf, left);
	written += left - unwritten;
out:
	return written ? written : -EFAULT;
}

static int mmap_zero(struct file * file, struct vm_area_struct * vma)
{
	if (vma->vm_flags & VM_SHARED)
		return shmem_zero_setup(vma);
	if (zeromap_page_range(vma, vma->vm_start, vma->vm_end - vma->vm_start, vma->vm_page_prot))
		return -EAGAIN;
	return 0;
}
#else /* CONFIG_MMU */
static ssize_t read_zero(struct file * file, char * buf, 
			 size_t count, loff_t *ppos)
{
	size_t todo = count;

	while (todo) {
		size_t chunk = todo;

		if (chunk > 4096)
			chunk = 4096;	/* Just for latency reasons */
		if (clear_user(buf, chunk))
			return -EFAULT;
		buf += chunk;
		todo -= chunk;
		cond_resched();
	}
	return count;
}

static int mmap_zero(struct file * file, struct vm_area_struct * vma)
{
	return -ENOSYS;
}
#endif /* CONFIG_MMU */

static ssize_t write_full(struct file * file, const char __user * buf,
			  size_t count, loff_t *ppos)
{
	return -ENOSPC;
}

/*
 * Special lseek() function for /dev/null and /dev/zero.  Most notably, you
 * can fopen() both devices with "a" now.  This was previously impossible.
 * -- SRB.
 */

static loff_t null_lseek(struct file * file, loff_t offset, int orig)
{
	return file->f_pos = 0;
}

/*
 * The memory devices use the full 32/64 bits of the offset, and so we cannot
 * check against negative addresses: they are ok. The return value is weird,
 * though, in that case (0).
 *
 * also note that seeking relative to the "end of file" isn't supported:
 * it has no meaning, so it returns -EINVAL.
 */
static loff_t memory_lseek(struct file * file, loff_t offset, int orig)
{
	loff_t ret;

	down(&file->f_dentry->d_inode->i_sem);
	switch (orig) {
		case 0:
			file->f_pos = offset;
			ret = file->f_pos;
			force_successful_syscall_return();
			break;
		case 1:
			file->f_pos += offset;
			ret = file->f_pos;
			force_successful_syscall_return();
			break;
		default:
			ret = -EINVAL;
	}
	up(&file->f_dentry->d_inode->i_sem);
	return ret;
}

static int open_port(struct inode * inode, struct file * filp)
{
	return capable(CAP_SYS_RAWIO) ? 0 : -EPERM;
}

#define zero_lseek	null_lseek
#define full_lseek      null_lseek
#define write_zero	write_null
#define read_full       read_zero
#define open_mem	open_port
#define open_kmem	open_mem

static struct file_operations mem_fops = {
	.llseek		= memory_lseek,
	.read		= read_mem,
	.write		= write_mem,
	.mmap		= mmap_mem,
	.open		= open_mem,
};

static struct file_operations kmem_fops = {
	.llseek		= memory_lseek,
	.read		= read_kmem,
	.write		= write_kmem,
	.mmap		= mmap_kmem,
	.open		= open_kmem,
};

static struct file_operations null_fops = {
	.llseek		= null_lseek,
	.read		= read_null,
	.write		= write_null,
};

#if defined(CONFIG_ISA) || !defined(__mc68000__)
static struct file_operations port_fops = {
	.llseek		= memory_lseek,
	.read		= read_port,
	.write		= write_port,
	.open		= open_port,
};
#endif

static struct file_operations zero_fops = {
	.llseek		= zero_lseek,
	.read		= read_zero,
	.write		= write_zero,
	.mmap		= mmap_zero,
};

static struct backing_dev_info zero_bdi = {
	.capabilities	= BDI_CAP_MAP_COPY,
};

static struct file_operations full_fops = {
	.llseek		= full_lseek,
	.read		= read_full,
	.write		= write_full,
};

static ssize_t kmsg_write(struct file * file, const char __user * buf,
			  size_t count, loff_t *ppos)
{
	char *tmp;
	int ret;

	tmp = kmalloc(count + 1, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;
	ret = -EFAULT;
	if (!copy_from_user(tmp, buf, count)) {
		tmp[count] = 0;
		ret = printk("%s", tmp);
	}
	kfree(tmp);
	return ret;
}

static struct file_operations kmsg_fops = {
	.write =	kmsg_write,
};

static int memory_open(struct inode * inode, struct file * filp)
{
	switch (iminor(inode)) {
		case 1:
			filp->f_op = &mem_fops;
			break;
		case 2:
			filp->f_op = &kmem_fops;
			break;
		case 3:
			filp->f_op = &null_fops;
			break;
#if defined(CONFIG_ISA) || !defined(__mc68000__)
		case 4:
			filp->f_op = &port_fops;
			break;
#endif
		case 5:
			filp->f_mapping->backing_dev_info = &zero_bdi;
			filp->f_op = &zero_fops;
			break;
		case 7:
			filp->f_op = &full_fops;
			break;
		case 8:
			filp->f_op = &random_fops;
			break;
		case 9:
			filp->f_op = &urandom_fops;
			break;
		case 11:
			filp->f_op = &kmsg_fops;
			break;
		default:
			return -ENXIO;
	}
	if (filp->f_op && filp->f_op->open)
		return filp->f_op->open(inode,filp);
	return 0;
}

static struct file_operations memory_fops = {
	.open		= memory_open,	/* just a selector for the real open */
};

static const struct {
	unsigned int		minor;
	char			*name;
	umode_t			mode;
	struct file_operations	*fops;
} devlist[] = { /* list of minor devices */
	{1, "mem",     S_IRUSR | S_IWUSR | S_IRGRP, &mem_fops},
	{2, "kmem",    S_IRUSR | S_IWUSR | S_IRGRP, &kmem_fops},
	{3, "null",    S_IRUGO | S_IWUGO,           &null_fops},
#if defined(CONFIG_ISA) || !defined(__mc68000__)
	{4, "port",    S_IRUSR | S_IWUSR | S_IRGRP, &port_fops},
#endif
	{5, "zero",    S_IRUGO | S_IWUGO,           &zero_fops},
	{7, "full",    S_IRUGO | S_IWUGO,           &full_fops},
	{8, "random",  S_IRUGO | S_IWUSR,           &random_fops},
	{9, "urandom", S_IRUGO | S_IWUSR,           &urandom_fops},
	{11,"kmsg",    S_IRUGO | S_IWUSR,           &kmsg_fops},
};

static struct class_simple *mem_class;

static int __init chr_dev_init(void)
{
	int i;

	if (register_chrdev(MEM_MAJOR,"mem",&memory_fops))
		printk("unable to get major %d for memory devs\n", MEM_MAJOR);

	mem_class = class_simple_create(THIS_MODULE, "mem");
	for (i = 0; i < ARRAY_SIZE(devlist); i++) {
		class_simple_device_add(mem_class,
					MKDEV(MEM_MAJOR, devlist[i].minor),
					NULL, devlist[i].name);
		devfs_mk_cdev(MKDEV(MEM_MAJOR, devlist[i].minor),
				S_IFCHR | devlist[i].mode, devlist[i].name);
	}
	
	return 0;
}

fs_initcall(chr_dev_init);
