#
# Makefile for some libs needed in the kernel.
#

lib-y := errno.o ctype.o string.o vsprintf.o cmdline.o \
	 bust_spinlocks.o rbtree.o radix-tree.o dump_stack.o \
	 kobject.o kref.o idr.o div64.o int_sqrt.o \
	 bitmap.o extable.o kobject_uevent.o prio_tree.o sha1.o \
	 halfmd4.o

obj-y += sort.o parser.o

ifeq ($(CONFIG_DEBUG_KOBJECT),y)
CFLAGS_kobject.o += -DDEBUG
CFLAGS_kobject_uevent.o += -DDEBUG
endif

lib-$(CONFIG_RWSEM_GENERIC_SPINLOCK) += rwsem-spinlock.o
lib-$(CONFIG_RWSEM_XCHGADD_ALGORITHM) += rwsem.o
lib-$(CONFIG_GENERIC_FIND_NEXT_BIT) += find_next_bit.o
obj-$(CONFIG_LOCK_KERNEL) += kernel_lock.o

ifneq ($(CONFIG_HAVE_DEC_LOCK),y) 
  lib-y += dec_and_lock.o
endif

obj-$(CONFIG_CRC_CCITT)	+= crc-ccitt.o
obj-$(CONFIG_CRC32)	+= crc32.o
obj-$(CONFIG_CRC7)	+= crc7.o
obj-$(CONFIG_LIBCRC32C)	+= libcrc32c.o
obj-$(CONFIG_GENERIC_IOMAP) += iomap.o

obj-$(CONFIG_ZLIB_INFLATE) += zlib_inflate/
obj-$(CONFIG_ZLIB_DEFLATE) += zlib_deflate/
obj-$(CONFIG_REED_SOLOMON) += reed_solomon/

hostprogs-y	:= gen_crc32table
clean-files	:= crc32table.h

$(obj)/crc32.o: $(obj)/crc32table.h

quiet_cmd_crc32 = GEN     $@
      cmd_crc32 = $< > $@

$(obj)/crc32table.h: $(obj)/gen_crc32table
	$(call cmd,crc32)
