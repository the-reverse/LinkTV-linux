/*
 * fs/direct-io.c
 *
 * Copyright (C) 2002, Linus Torvalds.
 *
 * O_DIRECT
 *
 * 04Jul2002	akpm@zip.com.au
 *		Initial version
 * 11Sep2002	janetinc@us.ibm.com
 * 		added readv/writev support.
 * 29Oct2002	akpm@zip.com.au
 *		rewrote bio_add_page() support.
 * 30Oct2002	pbadari@us.ibm.com
 *		added support for non-aligned IO.
 * 06Nov2002	pbadari@us.ibm.com
 *		added asynchronous IO support.
 * 21Jul2003	nathans@sgi.com
 *		added IO completion notifier.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/bio.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/rwsem.h>
#include <linux/uio.h>
#include <asm/atomic.h>

/*
 * How many user pages to map in one call to get_user_pages().  This determines
 * the size of a structure on the stack.
 */
#define DIO_PAGES	64

/*
 * This code generally works in units of "dio_blocks".  A dio_block is
 * somewhere between the hard sector size and the filesystem block size.  it
 * is determined on a per-invocation basis.   When talking to the filesystem
 * we need to convert dio_blocks to fs_blocks by scaling the dio_block quantity
 * down by dio->blkfactor.  Similarly, fs-blocksize quantities are converted
 * to bio_block quantities by shifting left by blkfactor.
 *
 * If blkfactor is zero then the user's request was aligned to the filesystem's
 * blocksize.
 *
 * lock_type is DIO_LOCKING for regular files on direct-IO-naive filesystems.
 * This determines whether we need to do the fancy locking which prevents
 * direct-IO from being able to read uninitialised disk blocks.  If its zero
 * (blockdev) this locking is not done, and if it is DIO_OWN_LOCKING i_sem is
 * not held for the entire direct write (taken briefly, initially, during a
 * direct read though, but its never held for the duration of a direct-IO).
 */

struct dio {
	/* BIO submission state */
	struct bio *bio;		/* bio under assembly */
	struct inode *inode;
	int rw;
	loff_t i_size;			/* i_size when submitted */
	int lock_type;			/* doesn't change */
	unsigned blkbits;		/* doesn't change */
	unsigned blkfactor;		/* When we're using an alignment which
					   is finer than the filesystem's soft
					   blocksize, this specifies how much
					   finer.  blkfactor=2 means 1/4-block
					   alignment.  Does not change */
	unsigned start_zero_done;	/* flag: sub-blocksize zeroing has
					   been performed at the start of a
					   write */
	int pages_in_io;		/* approximate total IO pages */
	size_t	size;			/* total request size (doesn't change)*/
	sector_t block_in_file;		/* Current offset into the underlying
					   file in dio_block units. */
	unsigned blocks_available;	/* At block_in_file.  changes */
	sector_t final_block_in_request;/* doesn't change */
	unsigned first_block_in_page;	/* doesn't change, Used only once */
	int boundary;			/* prev block is at a boundary */
	int reap_counter;		/* rate limit reaping */
	get_blocks_t *get_blocks;	/* block mapping function */
	dio_iodone_t *end_io;		/* IO completion function */
	sector_t final_block_in_bio;	/* current final block in bio + 1 */
	sector_t next_block_for_io;	/* next block to be put under IO,
					   in dio_blocks units */
	struct buffer_head map_bh;	/* last get_blocks() result */

	/*
	 * Deferred addition of a page to the dio.  These variables are
	 * private to dio_send_cur_page(), submit_page_section() and
	 * dio_bio_add_page().
	 */
	struct page *cur_page;		/* The page */
	unsigned cur_page_offset;	/* Offset into it, in bytes */
	unsigned cur_page_len;		/* Nr of bytes at cur_page_offset */
	sector_t cur_page_block;	/* Where it starts */

	/*
	 * Page fetching state. These variables belong to dio_refill_pages().
	 */
	int curr_page;			/* changes */
	int total_pages;		/* doesn't change */
	unsigned long curr_user_address;/* changes */

	/*
	 * Page queue.  These variables belong to dio_refill_pages() and
	 * dio_get_page().
	 */
	struct page *pages[DIO_PAGES];	/* page buffer */
	unsigned head;			/* next page to process */
	unsigned tail;			/* last valid page + 1 */
	int page_errors;		/* errno from get_user_pages() */

	/* BIO completion state */
	spinlock_t bio_lock;		/* protects BIO fields below */
	int bio_count;			/* nr bios to be completed */
	int bios_in_flight;		/* nr bios in flight */
	struct bio *bio_list;		/* singly linked via bi_private */
	struct task_struct *waiter;	/* waiting task (NULL if none) */

	/* AIO related stuff */
	struct kiocb *iocb;		/* kiocb */
	int is_async;			/* is IO async ? */
	ssize_t result;                 /* IO result */
};

/*
 * Release the dvr page.
 */
void page_cache_release_dvr(struct page *page)
{
	if (PageDVR(page)) {
	        put_page_testzero(page);
	} else {
	        page_cache_release(page);
	}
}

/*
 * How many pages are in the queue?
 */
static inline unsigned dio_pages_present(struct dio *dio)
{
	return dio->tail - dio->head;
}

/*
 * Go grab and pin some userspace pages.   Typically we'll get 64 at a time.
 */
static int dio_refill_pages(struct dio *dio)
{
	int ret;
	int nr_pages;

	nr_pages = min(dio->total_pages - dio->curr_page, DIO_PAGES);
#if CONFIG_USB_FILE_STORAGE_DIRECT_IO_MODE || CONFIG_BLK_DEV_LOOP_DIRECT_IO
	if (!(dio->curr_user_address & CAC_BASE)) {
#endif /* CONFIG_USB_FILE_STORAGE_DIRECT_IO_MODE */
	down_read(&current->mm->mmap_sem);
	ret = get_user_pages(
		current,			/* Task for fault acounting */
		current->mm,			/* whose pages? */
		dio->curr_user_address,		/* Where from? */
		nr_pages,			/* How many pages? */
		dio->rw == READ,		/* Write to memory? */
		0,				/* force (?) */
		&dio->pages[0],
		NULL);				/* vmas */
	up_read(&current->mm->mmap_sem);
#if CONFIG_USB_FILE_STORAGE_DIRECT_IO_MODE || CONFIG_BLK_DEV_LOOP_DIRECT_IO
	} else // kernel mode pages
	ret = get_kernel_pages(
		current,			/* Task for fault acounting */
		current->mm,			/* whose pages? */
		dio->curr_user_address,		/* Where from? */
		nr_pages,			/* How many pages? */
		dio->rw == READ,		/* Write to memory? */
		0,				/* force (?) */
		&dio->pages[0],
		NULL);				/* vmas */
#endif /* CONFIG_USB_FILE_STORAGE_DIRECT_IO_MODE */

	if (ret < 0 && dio->blocks_available && (dio->rw == WRITE)) {
		/*
		 * A memory fault, but the filesystem has some outstanding
		 * mapped blocks.  We need to use those blocks up to avoid
		 * leaking stale data in the file.
		 */
		if (dio->page_errors == 0)
			dio->page_errors = ret;
		dio->pages[0] = ZERO_PAGE(dio->curr_user_address);
		dio->head = 0;
		dio->tail = 1;
		ret = 0;
		goto out;
	}

	if (ret >= 0) {
		dio->curr_user_address += ret * PAGE_SIZE;
		dio->curr_page += ret;
		dio->head = 0;
		dio->tail = ret;
		ret = 0;
	}
out:
	return ret;	
}

/*
 * Get another userspace page.  Returns an ERR_PTR on error.  Pages are
 * buffered inside the dio so that we can call get_user_pages() against a
 * decent number of pages, less frequently.  To provide nicer use of the
 * L1 cache.
 */
static struct page *dio_get_page(struct dio *dio)
{
	if (dio_pages_present(dio) == 0) {
		int ret;

		ret = dio_refill_pages(dio);
		if (ret)
			return ERR_PTR(ret);
		BUG_ON(dio_pages_present(dio) == 0);
	}
	return dio->pages[dio->head++];
}

/*
 * Called when all DIO BIO I/O has been completed - let the filesystem
 * know, if it registered an interest earlier via get_blocks.  Pass the
 * private field of the map buffer_head so that filesystems can use it
 * to hold additional state between get_blocks calls and dio_complete.
 */
static void dio_complete(struct dio *dio, loff_t offset, ssize_t bytes)
{
	if (dio->end_io && dio->result)
		dio->end_io(dio->inode, offset, bytes, dio->map_bh.b_private);
	if (dio->lock_type == DIO_LOCKING)
		up_read(&dio->inode->i_alloc_sem);
}

/*
 * Called when a BIO has been processed.  If the count goes to zero then IO is
 * complete and we can signal this to the AIO layer.
 */
static void finished_one_bio(struct dio *dio)
{
	unsigned long flags;

	spin_lock_irqsave(&dio->bio_lock, flags);
	if (dio->bio_count == 1) {
		if (dio->is_async) {
			ssize_t transferred;
			loff_t offset;

			/*
			 * Last reference to the dio is going away.
			 * Drop spinlock and complete the DIO.
			 */
			spin_unlock_irqrestore(&dio->bio_lock, flags);

			/* Check for short read case */
			transferred = dio->result;
			offset = dio->iocb->ki_pos;

			if ((dio->rw == READ) &&
			    ((offset + transferred) > dio->i_size))
				transferred = dio->i_size - offset;

			dio_complete(dio, offset, transferred);

			/* Complete AIO later if falling back to buffered i/o */
			if (dio->result == dio->size ||
				((dio->rw == READ) && dio->result)) {
				aio_complete(dio->iocb, transferred, 0);
				kfree(dio);
				return;
			} else {
				/*
				 * Falling back to buffered
				 */
				spin_lock_irqsave(&dio->bio_lock, flags);
				dio->bio_count--;
				if (dio->waiter)
					wake_up_process(dio->waiter);
				spin_unlock_irqrestore(&dio->bio_lock, flags);
				return;
			}
		}
	}
	dio->bio_count--;
	spin_unlock_irqrestore(&dio->bio_lock, flags);
}

static int dio_bio_complete(struct dio *dio, struct bio *bio);
/*
 * Asynchronous IO callback. 
 */
static int dio_bio_end_aio(struct bio *bio, unsigned int bytes_done, int error)
{
	struct dio *dio = bio->bi_private;

	if (bio->bi_size)
		return 1;

	/* cleanup the bio */
	dio_bio_complete(dio, bio);
	return 0;
}

/*
 * The BIO completion handler simply queues the BIO up for the process-context
 * handler.
 *
 * During I/O bi_private points at the dio.  After I/O, bi_private is used to
 * implement a singly-linked list of completed BIOs, at dio->bio_list.
 */
static int dio_bio_end_io(struct bio *bio, unsigned int bytes_done, int error)
{
	struct dio *dio = bio->bi_private;
	unsigned long flags;

	if (bio->bi_size)
		return 1;

	spin_lock_irqsave(&dio->bio_lock, flags);
	bio->bi_private = dio->bio_list;
	dio->bio_list = bio;
	dio->bios_in_flight--;
	if (dio->waiter && dio->bios_in_flight == 0)
		wake_up_process(dio->waiter);
	spin_unlock_irqrestore(&dio->bio_lock, flags);
	return 0;
}

static int
dio_bio_alloc(struct dio *dio, struct block_device *bdev,
		sector_t first_sector, int nr_vecs)
{
	struct bio *bio;

	bio = bio_alloc(GFP_KERNEL, nr_vecs);
	if (bio == NULL)
		return -ENOMEM;

	bio->bi_bdev = bdev;
	bio->bi_sector = first_sector;
	if (dio->is_async)
		bio->bi_end_io = dio_bio_end_aio;
	else
		bio->bi_end_io = dio_bio_end_io;

	dio->bio = bio;
	return 0;
}

/*
 * In the AIO read case we speculatively dirty the pages before starting IO.
 * During IO completion, any of these pages which happen to have been written
 * back will be redirtied by bio_check_pages_dirty().
 */
static void dio_bio_submit(struct dio *dio)
{
	struct bio *bio = dio->bio;
	unsigned long flags;

	bio->bi_private = dio;
	spin_lock_irqsave(&dio->bio_lock, flags);
	dio->bio_count++;
	dio->bios_in_flight++;
	spin_unlock_irqrestore(&dio->bio_lock, flags);
	if (dio->is_async && dio->rw == READ)
		bio_set_pages_dirty(bio);
	submit_bio(dio->rw, bio);

	dio->bio = NULL;
	dio->boundary = 0;
}

/*
 * Release any resources in case of a failure
 */
static void dio_cleanup(struct dio *dio)
{
	while (dio_pages_present(dio))
		page_cache_release_dvr(dio_get_page(dio));
}

/*
 * Wait for the next BIO to complete.  Remove it and return it.
 */
static struct bio *dio_await_one(struct dio *dio)
{
	unsigned long flags;
	struct bio *bio;

	spin_lock_irqsave(&dio->bio_lock, flags);
	while (dio->bio_list == NULL) {
		set_current_state(TASK_UNINTERRUPTIBLE);
		if (dio->bio_list == NULL) {
			dio->waiter = current;
			spin_unlock_irqrestore(&dio->bio_lock, flags);
			blk_run_address_space(dio->inode->i_mapping);
			io_schedule();
			spin_lock_irqsave(&dio->bio_lock, flags);
			dio->waiter = NULL;
		}
		set_current_state(TASK_RUNNING);
	}
	bio = dio->bio_list;
	dio->bio_list = bio->bi_private;
	spin_unlock_irqrestore(&dio->bio_lock, flags);
	return bio;
}

/*
 * Process one completed BIO.  No locks are held.
 */
static int dio_bio_complete(struct dio *dio, struct bio *bio)
{
	const int uptodate = test_bit(BIO_UPTODATE, &bio->bi_flags);
	struct bio_vec *bvec = bio->bi_io_vec;
	int page_no;

	if (!uptodate)
		dio->result = -EIO;

	if (dio->is_async && dio->rw == READ) {
		bio_check_pages_dirty(bio);	/* transfers ownership */
	} else {
		for (page_no = 0; page_no < bio->bi_vcnt; page_no++) {
			struct page *page = bvec[page_no].bv_page;

			if (dio->rw == READ && !PageCompound(page) && !PageDVR(page)) {
				set_page_dirty_lock(page);
			}
			page_cache_release_dvr(page);
		}
		bio_put(bio);
	}
	finished_one_bio(dio);
#if 0
	return uptodate ? 0 : -EIO;
#else
	// added by cfyeh 2007/04/24
	// for return different errno for remove scsi device
	if(test_bit(BIO_SCSI_NOT_READY, &bio->bi_flags))
	{
		return uptodate ? 0 : -ENODEV;
	}
	else
	{
		return uptodate ? 0 : -EIO;
	}
#endif
}

/*
 * Wait on and process all in-flight BIOs.
 */
static int dio_await_completion(struct dio *dio)
{
	int ret = 0;

	if (dio->bio)
		dio_bio_submit(dio);

	/*
	 * The bio_lock is not held for the read of bio_count.
	 * This is ok since it is the dio_bio_complete() that changes
	 * bio_count.
	 */
	while (dio->bio_count) {
		struct bio *bio = dio_await_one(dio);
		int ret2;

		ret2 = dio_bio_complete(dio, bio);
		if (ret == 0)
			ret = ret2;
	}
	return ret;
}

/*
 * A really large O_DIRECT read or write can generate a lot of BIOs.  So
 * to keep the memory consumption sane we periodically reap any completed BIOs
 * during the BIO generation phase.
 *
 * This also helps to limit the peak amount of pinned userspace memory.
 */
static int dio_bio_reap(struct dio *dio)
{
	int ret = 0;

	if (dio->reap_counter++ >= 64) {
		while (dio->bio_list) {
			unsigned long flags;
			struct bio *bio;
			int ret2;

			spin_lock_irqsave(&dio->bio_lock, flags);
			bio = dio->bio_list;
			dio->bio_list = bio->bi_private;
			spin_unlock_irqrestore(&dio->bio_lock, flags);
			ret2 = dio_bio_complete(dio, bio);
			if (ret == 0)
				ret = ret2;
		}
		dio->reap_counter = 0;
	}
	return ret;
}

/*
 * Call into the fs to map some more disk blocks.  We record the current number
 * of available blocks at dio->blocks_available.  These are in units of the
 * fs blocksize, (1 << inode->i_blkbits).
 *
 * The fs is allowed to map lots of blocks at once.  If it wants to do that,
 * it uses the passed inode-relative block number as the file offset, as usual.
 *
 * get_blocks() is passed the number of i_blkbits-sized blocks which direct_io
 * has remaining to do.  The fs should not map more than this number of blocks.
 *
 * If the fs has mapped a lot of blocks, it should populate bh->b_size to
 * indicate how much contiguous disk space has been made available at
 * bh->b_blocknr.
 *
 * If *any* of the mapped blocks are new, then the fs must set buffer_new().
 * This isn't very efficient...
 *
 * In the case of filesystem holes: the fs may return an arbitrarily-large
 * hole by returning an appropriate value in b_size and by clearing
 * buffer_mapped().  However the direct-io code will only process holes one
 * block at a time - it will repeatedly call get_blocks() as it walks the hole.
 */
static int get_more_blocks(struct dio *dio)
{
	int ret;
	struct buffer_head *map_bh = &dio->map_bh;
	sector_t fs_startblk;	/* Into file, in filesystem-sized blocks */
	unsigned long fs_count;	/* Number of filesystem-sized blocks */
	unsigned long dio_count;/* Number of dio_block-sized blocks */
	unsigned long blkmask;
	int create;

	/*
	 * If there was a memory error and we've overwritten all the
	 * mapped blocks then we can now return that memory error
	 */
	ret = dio->page_errors;
	if (ret == 0) {
		map_bh->b_state = 0;
		map_bh->b_size = 0;
		BUG_ON(dio->block_in_file >= dio->final_block_in_request);
		fs_startblk = dio->block_in_file >> dio->blkfactor;
		dio_count = dio->final_block_in_request - dio->block_in_file;
		fs_count = dio_count >> dio->blkfactor;
		blkmask = (1 << dio->blkfactor) - 1;
		if (dio_count & blkmask)	
			fs_count++;

		create = dio->rw == WRITE;
		if (dio->lock_type == DIO_LOCKING) {
			if (dio->block_in_file < (i_size_read(dio->inode) >>
							dio->blkbits))
				create = 0;
		} else if (dio->lock_type == DIO_NO_LOCKING) {
			create = 0;
		}
		/*
		 * For writes inside i_size we forbid block creations: only
		 * overwrites are permitted.  We fall back to buffered writes
		 * at a higher level for inside-i_size block-instantiating
		 * writes.
		 */
		ret = (*dio->get_blocks)(dio->inode, fs_startblk, fs_count,
						map_bh, create);
	}
	return ret;
}

/*
 * There is no bio.  Make one now.
 */
static int dio_new_bio(struct dio *dio, sector_t start_sector)
{
	sector_t sector;
	int ret, nr_pages;

	ret = dio_bio_reap(dio);
	if (ret)
		goto out;
	sector = start_sector << (dio->blkbits - 9);
	nr_pages = min(dio->pages_in_io, bio_get_nr_vecs(dio->map_bh.b_bdev));
	BUG_ON(nr_pages <= 0);
	ret = dio_bio_alloc(dio, dio->map_bh.b_bdev, sector, nr_pages);
	dio->boundary = 0;
out:
	return ret;
}

/*
 * Attempt to put the current chunk of 'cur_page' into the current BIO.  If
 * that was successful then update final_block_in_bio and take a ref against
 * the just-added page.
 *
 * Return zero on success.  Non-zero means the caller needs to start a new BIO.
 */
static int dio_bio_add_page(struct dio *dio)
{
	int ret;

	ret = bio_add_page(dio->bio, dio->cur_page,
			dio->cur_page_len, dio->cur_page_offset);
	if (ret == dio->cur_page_len) {
		/*
		 * Decrement count only, if we are done with this page
		 */
		if ((dio->cur_page_len + dio->cur_page_offset) == PAGE_SIZE)
			dio->pages_in_io--;
		page_cache_get(dio->cur_page);
		dio->final_block_in_bio = dio->cur_page_block +
			(dio->cur_page_len >> dio->blkbits);
		ret = 0;
	} else {
		ret = 1;
	}
	return ret;
}
		
/*
 * Put cur_page under IO.  The section of cur_page which is described by
 * cur_page_offset,cur_page_len is put into a BIO.  The section of cur_page
 * starts on-disk at cur_page_block.
 *
 * We take a ref against the page here (on behalf of its presence in the bio).
 *
 * The caller of this function is responsible for removing cur_page from the
 * dio, and for dropping the refcount which came from that presence.
 */
static int dio_send_cur_page(struct dio *dio)
{
	int ret = 0;

	if (dio->bio) {
		/*
		 * See whether this new request is contiguous with the old
		 */
		if (dio->final_block_in_bio != dio->cur_page_block)
			dio_bio_submit(dio);
		/*
		 * Submit now if the underlying fs is about to perform a
		 * metadata read
		 */
		if (dio->boundary)
			dio_bio_submit(dio);
	}

	if (dio->bio == NULL) {
		ret = dio_new_bio(dio, dio->cur_page_block);
		if (ret)
			goto out;
	}

	if (dio_bio_add_page(dio) != 0) {
		dio_bio_submit(dio);
		ret = dio_new_bio(dio, dio->cur_page_block);
		if (ret == 0) {
			ret = dio_bio_add_page(dio);
			BUG_ON(ret != 0);
		}
	}
out:
	return ret;
}

/*
 * An autonomous function to put a chunk of a page under deferred IO.
 *
 * The caller doesn't actually know (or care) whether this piece of page is in
 * a BIO, or is under IO or whatever.  We just take care of all possible 
 * situations here.  The separation between the logic of do_direct_IO() and
 * that of submit_page_section() is important for clarity.  Please don't break.
 *
 * The chunk of page starts on-disk at blocknr.
 *
 * We perform deferred IO, by recording the last-submitted page inside our
 * private part of the dio structure.  If possible, we just expand the IO
 * across that page here.
 *
 * If that doesn't work out then we put the old page into the bio and add this
 * page to the dio instead.
 */
static int
submit_page_section(struct dio *dio, struct page *page,
		unsigned offset, unsigned len, sector_t blocknr)
{
	int ret = 0;

	/*
	 * Can we just grow the current page's presence in the dio?
	 */
	if (	(dio->cur_page == page) &&
		(dio->cur_page_offset + dio->cur_page_len == offset) &&
		(dio->cur_page_block +
			(dio->cur_page_len >> dio->blkbits) == blocknr)) {
		dio->cur_page_len += len;

		/*
		 * If dio->boundary then we want to schedule the IO now to
		 * avoid metadata seeks.
		 */
		if (dio->boundary) {
			ret = dio_send_cur_page(dio);
			page_cache_release_dvr(dio->cur_page);
			dio->cur_page = NULL;
		}
		goto out;
	}

	/*
	 * If there's a deferred page already there then send it.
	 */
	if (dio->cur_page) {
		ret = dio_send_cur_page(dio);
		page_cache_release_dvr(dio->cur_page);
		dio->cur_page = NULL;
		if (ret)
			goto out;
	}

	page_cache_get(page);		/* It is in dio */
	dio->cur_page = page;
	dio->cur_page_offset = offset;
	dio->cur_page_len = len;
	dio->cur_page_block = blocknr;
out:
	return ret;
}

/*
 * Clean any dirty buffers in the blockdev mapping which alias newly-created
 * file blocks.  Only called for S_ISREG files - blockdevs do not set
 * buffer_new
 */
static void clean_blockdev_aliases(struct dio *dio)
{
	unsigned i;
	unsigned nblocks;

	nblocks = dio->map_bh.b_size >> dio->inode->i_blkbits;

	for (i = 0; i < nblocks; i++) {
		unmap_underlying_metadata(dio->map_bh.b_bdev,
					dio->map_bh.b_blocknr + i);
	}
}

/*
 * If we are not writing the entire block and get_block() allocated
 * the block for us, we need to fill-in the unused portion of the
 * block with zeros. This happens only if user-buffer, fileoffset or
 * io length is not filesystem block-size multiple.
 *
 * `end' is zero if we're doing the start of the IO, 1 at the end of the
 * IO.
 */
static void dio_zero_block(struct dio *dio, int end)
{
	unsigned dio_blocks_per_fs_block;
	unsigned this_chunk_blocks;	/* In dio_blocks */
	unsigned this_chunk_bytes;
	struct page *page;

	dio->start_zero_done = 1;
	if (!dio->blkfactor || !buffer_new(&dio->map_bh))
		return;

	dio_blocks_per_fs_block = 1 << dio->blkfactor;
	this_chunk_blocks = dio->block_in_file & (dio_blocks_per_fs_block - 1);

	if (!this_chunk_blocks)
		return;

	/*
	 * We need to zero out part of an fs block.  It is either at the
	 * beginning or the end of the fs block.
	 */
	if (end) 
		this_chunk_blocks = dio_blocks_per_fs_block - this_chunk_blocks;

	this_chunk_bytes = this_chunk_blocks << dio->blkbits;

	page = ZERO_PAGE(dio->curr_user_address);
	if (submit_page_section(dio, page, 0, this_chunk_bytes, 
				dio->next_block_for_io))
		return;

	dio->next_block_for_io += this_chunk_blocks;
}

/*
 * Walk the user pages, and the file, mapping blocks to disk and generating
 * a sequence of (page,offset,len,block) mappings.  These mappings are injected
 * into submit_page_section(), which takes care of the next stage of submission
 *
 * Direct IO against a blockdev is different from a file.  Because we can
 * happily perform page-sized but 512-byte aligned IOs.  It is important that
 * blockdev IO be able to have fine alignment and large sizes.
 *
 * So what we do is to permit the ->get_blocks function to populate bh.b_size
 * with the size of IO which is permitted at this offset and this i_blkbits.
 *
 * For best results, the blockdev should be set up with 512-byte i_blkbits and
 * it should set b_size to PAGE_SIZE or more inside get_blocks().  This gives
 * fine alignment but still allows this function to work in PAGE_SIZE units.
 */
static int do_direct_IO(struct dio *dio)
{
	const unsigned blkbits = dio->blkbits;
	const unsigned blocks_per_page = PAGE_SIZE >> blkbits;
	struct page *page;
	unsigned block_in_page;
	struct buffer_head *map_bh = &dio->map_bh;
	int ret = 0;

	/* The I/O can start at any block offset within the first page */
	block_in_page = dio->first_block_in_page;

	while (dio->block_in_file < dio->final_block_in_request) {
		page = dio_get_page(dio);
		if (IS_ERR(page)) {
			ret = PTR_ERR(page);
			goto out;
		}

		while (block_in_page < blocks_per_page) {
			unsigned offset_in_page = block_in_page << blkbits;
			unsigned this_chunk_bytes;	/* # of bytes mapped */
			unsigned this_chunk_blocks;	/* # of blocks */
			unsigned u;

			if (dio->blocks_available == 0) {
				/*
				 * Need to go and map some more disk
				 */
				unsigned long blkmask;
				unsigned long dio_remainder;

				ret = get_more_blocks(dio);
				if (ret) {
					page_cache_release_dvr(page);
					goto out;
				}
				if (!buffer_mapped(map_bh))
					goto do_holes;

				dio->blocks_available =
						map_bh->b_size >> dio->blkbits;
				dio->next_block_for_io =
					map_bh->b_blocknr << dio->blkfactor;
				if (buffer_new(map_bh))
					clean_blockdev_aliases(dio);

				if (!dio->blkfactor)
					goto do_holes;

				blkmask = (1 << dio->blkfactor) - 1;
				dio_remainder = (dio->block_in_file & blkmask);

				/*
				 * If we are at the start of IO and that IO
				 * starts partway into a fs-block,
				 * dio_remainder will be non-zero.  If the IO
				 * is a read then we can simply advance the IO
				 * cursor to the first block which is to be
				 * read.  But if the IO is a write and the
				 * block was newly allocated we cannot do that;
				 * the start of the fs block must be zeroed out
				 * on-disk
				 */
				if (!buffer_new(map_bh))
					dio->next_block_for_io += dio_remainder;
				dio->blocks_available -= dio_remainder;
			}
do_holes:
			/* Handle holes */
			if (!buffer_mapped(map_bh)) {
				char *kaddr;

				/* AKPM: eargh, -ENOTBLK is a hack */
				if (dio->rw == WRITE) {
					page_cache_release_dvr(page);
					return -ENOTBLK;
				}

				if (dio->block_in_file >=
					i_size_read(dio->inode)>>blkbits) {
					/* We hit eof */
					page_cache_release_dvr(page);
					goto out;
				}
				kaddr = kmap_atomic(page, KM_USER0);
				memset(kaddr + (block_in_page << blkbits),
						0, 1 << blkbits);
				flush_dcache_page(page);
				kunmap_atomic(kaddr, KM_USER0);
				dio->block_in_file++;
				block_in_page++;
				goto next_block;
			}

			/*
			 * If we're performing IO which has an alignment which
			 * is finer than the underlying fs, go check to see if
			 * we must zero out the start of this block.
			 */
			if (unlikely(dio->blkfactor && !dio->start_zero_done))
				dio_zero_block(dio, 0);

			/*
			 * Work out, in this_chunk_blocks, how much disk we
			 * can add to this page
			 */
			this_chunk_blocks = dio->blocks_available;
			u = (PAGE_SIZE - offset_in_page) >> blkbits;
			if (this_chunk_blocks > u)
				this_chunk_blocks = u;
			u = dio->final_block_in_request - dio->block_in_file;
			if (this_chunk_blocks > u)
				this_chunk_blocks = u;
			this_chunk_bytes = this_chunk_blocks << blkbits;
			BUG_ON(this_chunk_bytes == 0);

			dio->boundary = buffer_boundary(map_bh);
			ret = submit_page_section(dio, page, offset_in_page,
				this_chunk_bytes, dio->next_block_for_io);
			if (ret) {
				page_cache_release_dvr(page);
				goto out;
			}
			dio->next_block_for_io += this_chunk_blocks;

			dio->block_in_file += this_chunk_blocks;
			block_in_page += this_chunk_blocks;
			dio->blocks_available -= this_chunk_blocks;
next_block:
			if (dio->block_in_file > dio->final_block_in_request)
				BUG();
			if (dio->block_in_file == dio->final_block_in_request)
				break;
		}

		/* Drop the ref which was taken in get_user_pages() */
		page_cache_release_dvr(page);
		block_in_page = 0;
	}
out:
	return ret;
}

/*
 * Releases both i_sem and i_alloc_sem
 */
static ssize_t
direct_io_worker(int rw, struct kiocb *iocb, struct inode *inode, 
	const struct iovec *iov, loff_t offset, unsigned long nr_segs, 
	unsigned blkbits, get_blocks_t get_blocks, dio_iodone_t end_io,
	struct dio *dio)
{
	unsigned long user_addr; 
	int seg;
	ssize_t ret = 0;
	ssize_t ret2;
	size_t bytes;

	dio->bio = NULL;
	dio->inode = inode;
	dio->rw = rw;
	dio->blkbits = blkbits;
	dio->blkfactor = inode->i_blkbits - blkbits;
	dio->start_zero_done = 0;
	dio->size = 0;
	dio->block_in_file = offset >> blkbits;
	dio->blocks_available = 0;
	dio->cur_page = NULL;

	dio->boundary = 0;
	dio->reap_counter = 0;
	dio->get_blocks = get_blocks;
	dio->end_io = end_io;
	dio->map_bh.b_private = NULL;
	dio->final_block_in_bio = -1;
	dio->next_block_for_io = -1;

	dio->page_errors = 0;
	dio->result = 0;
	dio->iocb = iocb;
	dio->i_size = i_size_read(inode);

	/*
	 * BIO completion state.
	 *
	 * ->bio_count starts out at one, and we decrement it to zero after all
	 * BIOs are submitted.  This to avoid the situation where a really fast
	 * (or synchronous) device could take the count to zero while we're
	 * still submitting BIOs.
	 */
	dio->bio_count = 1;
	dio->bios_in_flight = 0;
	spin_lock_init(&dio->bio_lock);
	dio->bio_list = NULL;
	dio->waiter = NULL;

	/*
	 * In case of non-aligned buffers, we may need 2 more
	 * pages since we need to zero out first and last block.
	 */
	if (unlikely(dio->blkfactor))
		dio->pages_in_io = 2;
	else
		dio->pages_in_io = 0;

	for (seg = 0; seg < nr_segs; seg++) {
		user_addr = (unsigned long)iov[seg].iov_base;
		dio->pages_in_io +=
			((user_addr+iov[seg].iov_len +PAGE_SIZE-1)/PAGE_SIZE
				- user_addr/PAGE_SIZE);
	}

	for (seg = 0; seg < nr_segs; seg++) {
		user_addr = (unsigned long)iov[seg].iov_base;
		dio->size += bytes = iov[seg].iov_len;

		/* Index into the first page of the first block */
		dio->first_block_in_page = (user_addr & ~PAGE_MASK) >> blkbits;
		dio->final_block_in_request = dio->block_in_file +
						(bytes >> blkbits);
		/* Page fetching state */
		dio->head = 0;
		dio->tail = 0;
		dio->curr_page = 0;

		dio->total_pages = 0;
		if (user_addr & (PAGE_SIZE-1)) {
			dio->total_pages++;
			bytes -= PAGE_SIZE - (user_addr & (PAGE_SIZE - 1));
		}
		dio->total_pages += (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
		dio->curr_user_address = user_addr;
	
		ret = do_direct_IO(dio);

		dio->result += iov[seg].iov_len -
			((dio->final_block_in_request - dio->block_in_file) <<
					blkbits);

		if (ret) {
			dio_cleanup(dio);
			break;
		}
	} /* end iovec loop */

	if (ret == -ENOTBLK && rw == WRITE) {
		/*
		 * The remaining part of the request will be
		 * be handled by buffered I/O when we return
		 */
		ret = 0;
	}
	/*
	 * There may be some unwritten disk at the end of a part-written
	 * fs-block-sized block.  Go zero that now.
	 */
	dio_zero_block(dio, 1);

	if (dio->cur_page) {
		ret2 = dio_send_cur_page(dio);
		if (ret == 0)
			ret = ret2;
		page_cache_release_dvr(dio->cur_page);
		dio->cur_page = NULL;
	}
	if (dio->bio)
		dio_bio_submit(dio);

	/*
	 * It is possible that, we return short IO due to end of file.
	 * In that case, we need to release all the pages we got hold on.
	 */
	dio_cleanup(dio);

	/*
	 * All block lookups have been performed. For READ requests
	 * we can let i_sem go now that its achieved its purpose
	 * of protecting us from looking up uninitialized blocks.
	 */
	if ((rw == READ) && (dio->lock_type == DIO_LOCKING))
		up(&dio->inode->i_sem);

	/*
	 * OK, all BIOs are submitted, so we can decrement bio_count to truly
	 * reflect the number of to-be-processed BIOs.
	 */
	if (dio->is_async) {
		int should_wait = 0;

		if (dio->result < dio->size && rw == WRITE) {
			dio->waiter = current;
			should_wait = 1;
		}
		if (ret == 0)
			ret = dio->result;
		finished_one_bio(dio);		/* This can free the dio */
		blk_run_address_space(inode->i_mapping);
		if (should_wait) {
			unsigned long flags;
			/*
			 * Wait for already issued I/O to drain out and
			 * release its references to user-space pages
			 * before returning to fallback on buffered I/O
			 */

			spin_lock_irqsave(&dio->bio_lock, flags);
			set_current_state(TASK_UNINTERRUPTIBLE);
			while (dio->bio_count) {
				spin_unlock_irqrestore(&dio->bio_lock, flags);
				io_schedule();
				spin_lock_irqsave(&dio->bio_lock, flags);
				set_current_state(TASK_UNINTERRUPTIBLE);
			}
			spin_unlock_irqrestore(&dio->bio_lock, flags);
			set_current_state(TASK_RUNNING);
			kfree(dio);
		}
	} else {
		ssize_t transferred = 0;

		finished_one_bio(dio);
		ret2 = dio_await_completion(dio);
		if (ret == 0)
			ret = ret2;
		if (ret == 0)
			ret = dio->page_errors;
		if (dio->result) {
			loff_t i_size = i_size_read(inode);

			transferred = dio->result;
			/*
			 * Adjust the return value if the read crossed a
			 * non-block-aligned EOF.
			 */
			if (rw == READ && (offset + transferred > i_size))
				transferred = i_size - offset;
		}
		dio_complete(dio, offset, transferred);
		if (ret == 0)
			ret = transferred;

		/* We could have also come here on an AIO file extend */
		if (!is_sync_kiocb(iocb) && rw == WRITE &&
		    ret >= 0 && dio->result == dio->size)
			/*
			 * For AIO writes where we have completed the
			 * i/o, we have to mark the the aio complete.
			 */
			aio_complete(iocb, ret, 0);
		kfree(dio);
	}
	return ret;
}

/*
 * This is a library function for use by filesystem drivers.
 * The locking rules are governed by the dio_lock_type parameter.
 *
 * DIO_NO_LOCKING (no locking, for raw block device access)
 * For writes, i_sem is not held on entry; it is never taken.
 *
 * DIO_LOCKING (simple locking for regular files)
 * For writes we are called under i_sem and return with i_sem held, even though
 * it is internally dropped.
 * For reads, i_sem is not held on entry, but it is taken and dropped before
 * returning.
 *
 * DIO_OWN_LOCKING (filesystem provides synchronisation and handling of
 *	uninitialised data, allowing parallel direct readers and writers)
 * For writes we are called without i_sem, return without it, never touch it.
 * For reads, i_sem is held on entry and will be released before returning.
 *
 * Additional i_alloc_sem locking requirements described inline below.
 */
ssize_t
__blockdev_direct_IO(int rw, struct kiocb *iocb, struct inode *inode,
	struct block_device *bdev, const struct iovec *iov, loff_t offset, 
	unsigned long nr_segs, get_blocks_t get_blocks, dio_iodone_t end_io,
	int dio_lock_type)
{
	int seg;
	size_t size;
	unsigned long addr;
	unsigned blkbits = inode->i_blkbits;
	unsigned bdev_blkbits = 0;
	unsigned blocksize_mask = (1 << blkbits) - 1;
	ssize_t retval = -EINVAL;
	loff_t end = offset;
	struct dio *dio;
	int reader_with_isem = (rw == READ && dio_lock_type == DIO_OWN_LOCKING);

	if (rw & WRITE)
		current->flags |= PF_SYNCWRITE;

	if (bdev)
		bdev_blkbits = blksize_bits(bdev_hardsect_size(bdev));

	if (offset & blocksize_mask) {
		if (bdev)
			 blkbits = bdev_blkbits;
		blocksize_mask = (1 << blkbits) - 1;
		if (offset & blocksize_mask)
			goto out;
	}

	/* Check the memory alignment.  Blocks cannot straddle pages */
	for (seg = 0; seg < nr_segs; seg++) {
		addr = (unsigned long)iov[seg].iov_base;
		size = iov[seg].iov_len;
		end += size;
		if ((addr & blocksize_mask) || (size & blocksize_mask))  {
			if (bdev)
				 blkbits = bdev_blkbits;
			blocksize_mask = (1 << blkbits) - 1;
			if ((addr & blocksize_mask) || (size & blocksize_mask))  
				goto out;
		}
	}

	dio = kmalloc(sizeof(*dio), GFP_KERNEL);
	retval = -ENOMEM;
	if (!dio)
		goto out;

	/*
	 * For block device access DIO_NO_LOCKING is used,
	 *	neither readers nor writers do any locking at all
	 * For regular files using DIO_LOCKING,
	 *	readers need to grab i_sem and i_alloc_sem
	 *	writers need to grab i_alloc_sem only (i_sem is already held)
	 * For regular files using DIO_OWN_LOCKING,
	 *	neither readers nor writers take any locks here
	 *	(i_sem is already held and release for writers here)
	 */
	dio->lock_type = dio_lock_type;
	if (dio_lock_type != DIO_NO_LOCKING) {
		/* watch out for a 0 len io from a tricksy fs */
		if (rw == READ && end > offset) {
			struct address_space *mapping;

			mapping = iocb->ki_filp->f_mapping;
			if (dio_lock_type != DIO_OWN_LOCKING) {
				down(&inode->i_sem);
				reader_with_isem = 1;
			}

			retval = filemap_write_and_wait_range(mapping, offset,
							      end - 1);
			if (retval) {
				kfree(dio);
				goto out;
			}

			if (dio_lock_type == DIO_OWN_LOCKING) {
				up(&inode->i_sem);
				reader_with_isem = 0;
			}
		}

		if (dio_lock_type == DIO_LOCKING)
			down_read(&inode->i_alloc_sem);
	}

	/*
	 * For file extending writes updating i_size before data
	 * writeouts complete can expose uninitialized blocks. So
	 * even for AIO, we need to wait for i/o to complete before
	 * returning in this case.
	 */
	dio->is_async = !is_sync_kiocb(iocb) && !((rw == WRITE) &&
		(end > i_size_read(inode)));

	retval = direct_io_worker(rw, iocb, inode, iov, offset,
				nr_segs, blkbits, get_blocks, end_io, dio);

	if (rw == READ && dio_lock_type == DIO_LOCKING)
		reader_with_isem = 0;

out:
	if (reader_with_isem)
		up(&inode->i_sem);
	if (rw & WRITE)
		current->flags &= ~PF_SYNCWRITE;
	return retval;
}
EXPORT_SYMBOL(__blockdev_direct_IO);
