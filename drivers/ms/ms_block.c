/*
 * Block driver for media (i.e., flash cards)
 *
 * Copyright 2002 Hewlett-Packard Company
 *
 * Use consistent with the GNU GPL is permitted,
 * provided that this copyright notice is
 * preserved in its entirety in all copies and derived works.
 *
 * HEWLETT-PACKARD COMPANY MAKES NO WARRANTIES, EXPRESSED OR IMPLIED,
 * AS TO THE USEFULNESS OR CORRECTNESS OF THIS CODE OR ITS
 * FITNESS FOR ANY PARTICULAR PURPOSE.
 *
 * Many thanks to Alessandro Rubini and Jonathan Corbet!
 *
 * Author:  Andrew Christian
 *          28 May 2002
 *
 * Modification History:
 *    #000 2009-07-09 CMYu   modify from mmc_block.c for ms usage
 */
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>

#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/devfs_fs_kernel.h>

#include <linux/ms/card.h>
#include <linux/ms/protocol.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include "ms_queue.h"
#include "ms_debug.h"
#include <linux/rtk_cr.h>

/*
 * max 8 partitions per card
 */
#define MSPRO_BLOCK_PART_SHIFT	3

static int major;

/*
 * There is one ms_blk_data per slot.
 */
struct ms_blk_data {
	spinlock_t	lock;
	struct gendisk	*disk;
	struct ms_queue queue;

	unsigned int	usage;
	unsigned int	block_bits;
	unsigned short page_size;
	unsigned short cylinders;
	unsigned short heads;
	unsigned short sectors_per_track;
};

static DECLARE_MUTEX(open_lock);

static struct ms_blk_data *ms_blk_get(struct gendisk *disk)
{

	struct ms_blk_data *md;

	down(&open_lock);
	md = disk->private_data;
	if (md && md->usage == 0)
		md = NULL;
	if (md)
		md->usage++;
	up(&open_lock);

	return md;
}

static void ms_blk_put(struct ms_blk_data *md)
{

	down(&open_lock);
	md->usage--;
	if (md->usage == 0) {
		put_disk(md->disk);
		ms_cleanup_queue(&md->queue);
		kfree(md);
	}
	up(&open_lock);
}

static int ms_blk_open(struct inode *inode, struct file *filp)
{

	struct ms_blk_data *md;
	int ret = -ENXIO;

	md = ms_blk_get(inode->i_bdev->bd_disk);
	if (md) {
		if (md->usage == 2)
			check_disk_change(inode->i_bdev);
		ret = 0;
	}

	return ret;
}

static int ms_blk_release(struct inode *inode, struct file *filp)
{

	struct ms_blk_data *md = inode->i_bdev->bd_disk->private_data;

	ms_blk_put(md);
	return 0;
}

static int
ms_blk_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{

	struct block_device *bdev = inode->i_bdev;

	//CMYu, 20090805
	struct ms_blk_data *md = bdev->bd_disk->private_data;

	if (cmd == HDIO_GETGEO) {
		struct hd_geometry geo;

		memset(&geo, 0, sizeof(struct hd_geometry));

		geo.cylinders	= md->cylinders;
		geo.heads	= md->heads;
		geo.sectors = md->sectors_per_track;

		geo.start	= get_start_sect(bdev);

		return copy_to_user((void __user *)arg, &geo, sizeof(geo))
			? -EFAULT : 0;
	}

	return -ENOTTY;
}

static struct block_device_operations ms_bdops = {
	.open			= ms_blk_open,
	.release		= ms_blk_release,
	.ioctl			= ms_blk_ioctl,
	.owner			= THIS_MODULE,
};

struct ms_blk_request {
	struct ms_request	mrq;
	struct ms_command	cmd;
	struct ms_data		data;
};


static int ms_blk_prep_rq(struct ms_queue *mq, struct request *req)
{

	struct ms_blk_data *md = mq->data;
	int stat = BLKPREP_OK;

	/*
	 * If we have no device, we haven't finished initialising.
	 */
	if (!md || !mq->card) {
		printk(KERN_ERR "%s: killing request - no device/host\n",
		       req->rq_disk->disk_name);
		stat = BLKPREP_KILL;
	}

	return stat;
}


static int ms_blk_issue_rq(struct ms_queue *mq, struct request *req)
{

	struct ms_blk_data *md = mq->data;
	struct ms_card *card = md->queue.card;
	int ret;
	sector_t sector_start;
	int page_size = 512;

	//CMYu, 20090721
	unsigned int count;
	int try_cnt = 0;
    msinfo("\n");
	if (ms_card_claim_host(card))
		goto req_err;

	do {
		struct ms_blk_request brq;

		memset(&brq, 0, sizeof(struct ms_blk_request));
		brq.mrq.data = &brq.data;

		sector_start = req->sector << 9;
		sector_div(sector_start, page_size);

		brq.data.blksz_bits = md->block_bits;
		brq.data.blocks = req->nr_sectors >> (md->block_bits - 9);


		count = req->nr_sectors << 9;
		count /= page_size;

		brq.mrq.tpc = MSPRO_TPC_WRITE_REG;

		brq.mrq.byte_count = 7;
		brq.mrq.timeout = PRO_WRITE_TIMEOUT;
		brq.mrq.reg_data[0] = PARELLEL_IF;
		brq.mrq.reg_data[1] = SHORT2CHAR(count, 1);
		brq.mrq.reg_data[2] = SHORT2CHAR(count, 0);
		brq.mrq.reg_data[3] = LONG2CHAR(sector_start, 3);
		brq.mrq.reg_data[4] = LONG2CHAR(sector_start, 2);
		brq.mrq.reg_data[5] = LONG2CHAR(sector_start, 1);
		brq.mrq.reg_data[6] = LONG2CHAR(sector_start, 0);

		ms_wait_for_req(card->host, &brq.mrq);

		if (brq.mrq.error) {
			printk(KERN_ERR "%s: error %d sending read/write command\n",
			       req->rq_disk->disk_name, brq.mrq.error);
			goto req_err;
		}

		brq.mrq.tpc = MSPRO_TPC_SET_CMD;
		brq.mrq.option |= WAIT_INT;
		brq.mrq.byte_count = 1;
		if (rq_data_dir(req) == READ) {
			brq.mrq.reg_data[0] = MSPRO_CMD_READ_DATA;
		} else {
			brq.mrq.reg_data[0] = MSPRO_CMD_WRITE_DATA;
		}
		ms_wait_for_req(card->host, &brq.mrq);

		if (brq.mrq.error) {
			printk(KERN_ERR "%s: error %d sending read/write command\n",
			       req->rq_disk->disk_name, brq.mrq.error);
			goto req_err;
		}

		if ( !(brq.mrq.status & INT_BREQ) ){
			printk("[%d] INT_BREQ errors, mrq->status=%x\n", __LINE__, brq.mrq.status);
			goto req_err;
		}

		if (rq_data_dir(req) == READ) {
			brq.mrq.tpc = MSPRO_TPC_READ_LONG_DATA;
			brq.data.flags |= MS_DATA_READ;
		}else{
			brq.mrq.tpc = MSPRO_TPC_WRITE_LONG_DATA;
			brq.data.flags |= MS_DATA_WRITE;
		}
		brq.mrq.sector_cnt = count;
		brq.mrq.sector_start = sector_start;
		brq.mrq.timeout = 10000;
		brq.data.sg = mq->sg;
		brq.data.sg_len = blk_rq_map_sg(req->q, req, brq.data.sg);
		ms_wait_for_req(card->host, &brq.mrq);

		if (brq.mrq.error) {
			printk(KERN_ERR "%s: error %d transferring data\n",
			       req->rq_disk->disk_name, brq.data.error);
			goto req_err;
		}
/*

		brq.mrq.tpc = MSPRO_TPC_SET_CMD;
		brq.mrq.reg_data[0] = MSPRO_CMD_STOP;
		brq.mrq.option|= WAIT_INT;
		brq.mrq.byte_count = 1;

		ms_wait_for_req(card->host, &brq.mrq);

		if (brq.mrq.error) {
			printk(KERN_ERR "%s: error %d sending read/write command\n",
			       req->rq_disk->disk_name, brq.mrq.error);
			goto req_err;
		}
*/
		/*
		 * A block was successfully transferred.
		 */
		spin_lock_irq(&md->lock);
		ret = end_that_request_chunk(req, 1, brq.data.bytes_xfered);
		if (!ret) {
			/*
			 * The whole request completed successfully.
			 */
			add_disk_randomness(req->rq_disk);
			blkdev_dequeue_request(req);
			end_that_request_last(req);
		}
		spin_unlock_irq(&md->lock);
	} while (ret);

	ms_card_release_host(card);

	return 1;

 req_err:
	ms_card_release_host(card);
	/*
	 * This is a little draconian, but until we get proper
	 * error handling sorted out here, its the best we can
	 * do - especially as some hosts have no idea how much
	 * data was transferred before the error occurred.
	 */
	spin_lock_irq(&md->lock);
	do {
		ret = end_that_request_chunk(req, 0, req->current_nr_sectors << 9);
	} while (ret);
	add_disk_randomness(req->rq_disk);
	blkdev_dequeue_request(req);
	end_that_request_last(req);
	spin_unlock_irq(&md->lock);
	return 0;
}


#define MS_NUM_MINORS	(256 >> MSPRO_BLOCK_PART_SHIFT)

static unsigned long dev_use[MS_NUM_MINORS/(8*sizeof(unsigned long))];

static struct ms_blk_data *ms_blk_alloc(struct ms_card *card)
{

	struct ms_blk_data *md;
	int devidx, ret;

	devidx = find_first_zero_bit(dev_use, MS_NUM_MINORS);
	if (devidx >= MS_NUM_MINORS)
		return ERR_PTR(-ENOSPC);
	__set_bit(devidx, dev_use);

	md = kmalloc(sizeof(struct ms_blk_data), GFP_KERNEL);
	if (md) {
		memset(md, 0, sizeof(struct ms_blk_data));

		md->disk = alloc_disk(1 << MSPRO_BLOCK_PART_SHIFT);
		if (md->disk == NULL) {
			kfree(md);
			md = ERR_PTR(-ENOMEM);
			goto out;
		}

		spin_lock_init(&md->lock);
		md->usage = 1;

		ret = ms_init_queue(&md->queue, card, &md->lock);
		if (ret) {
			put_disk(md->disk);
			kfree(md);
			md = ERR_PTR(ret);
			goto out;
		}
		md->queue.prep_fn = ms_blk_prep_rq;
		md->queue.issue_fn = ms_blk_issue_rq;
		md->queue.data = md;

		md->disk->major	= major;
		md->disk->first_minor = devidx << MSPRO_BLOCK_PART_SHIFT;
		md->disk->fops = &ms_bdops;
		md->disk->private_data = md;
		md->disk->queue = md->queue.queue;
		md->disk->driverfs_dev = &card->dev;

        strcpy(md->disk->port_structure, "1");  /*  2010/05/31 port structure */

        strcpy(md->disk->bus_type, "msblk");    /*  2010/05/31 bus type */


		sprintf(md->disk->disk_name, "msblk%d", devidx);
		sprintf(md->disk->devfs_name, "ms/blk%d", devidx);

		md->block_bits = 9;

		blk_queue_hardsect_size(md->queue.queue, 1 << md->block_bits);
		set_capacity(md->disk, card->capacity);
		//CMYu, 20090804
		md->cylinders = card->cylinders;
		md->heads = card->heads;
		md->sectors_per_track = card->sectors_per_track;
	}else{
		printk("Allocating ms_blk_data fails.");
		md = ERR_PTR(-ENOMEM);
		goto out;
	}

 out:
	return md;
}


static int ms_blk_probe(struct ms_card *card)
{

	struct ms_blk_data *md;
	int rc;

	md = ms_blk_alloc(card);
	if (IS_ERR(md)){
		printk("ms_blk_alloc() fails\n");
		return PTR_ERR(md);
	}

	md->page_size = 512;

	ms_set_drvdata(card, md);
	add_disk(md->disk);
	return 0;

 out:
 	ms_set_drvdata(card, NULL);
	ms_blk_put(md);

	return rc;
}

static void ms_blk_remove(struct ms_card *card)
{

	struct ms_blk_data *md = ms_get_drvdata(card);

	if (md) {
		int devidx;

		del_gendisk(md->disk);

		/*
		 * I think this is needed.
		 */
		md->disk->queue = NULL;

		devidx = md->disk->first_minor >> MSPRO_BLOCK_PART_SHIFT;
		__clear_bit(devidx, dev_use);

		ms_blk_put(md);
	}
	ms_set_drvdata(card, NULL);
}

#ifdef CONFIG_PM
static int ms_blk_suspend(struct ms_card *card, pm_message_t state)
{
	struct ms_blk_data *md = ms_get_drvdata(card);

	if (md) {
		ms_queue_suspend(&md->queue);
	}
	return 0;
}

static int ms_blk_resume(struct ms_card *card)
{
	struct ms_blk_data *md = ms_get_drvdata(card);

	if (md) {
		ms_queue_resume(&md->queue);
	}
	return 0;
}
#else
#define	ms_blk_suspend	NULL
#define ms_blk_resume	NULL
#endif

static struct ms_driver ms_driver = {
	.drv		= {
		.name	= "msblk",
	},
	.probe		= ms_blk_probe,
	.remove		= ms_blk_remove,
	.suspend	= ms_blk_suspend,
	.resume		= ms_blk_resume,
};

static int __init ms_blk_init(void)
{

	int res = -ENOMEM;

	res = register_blkdev(major, "ms");
	if (res < 0) {
		printk(KERN_WARNING "Unable to get major %d for MS media: %d\n",
		       major, res);
		goto out;
	}
	if (major == 0)
		major = res;

	devfs_mk_dir("ms");
	return ms_register_driver(&ms_driver);

 out:
	return res;
}

static void __exit ms_blk_exit(void)
{

	ms_unregister_driver(&ms_driver);
	devfs_remove("ms");
	unregister_blkdev(major, "ms");
}

module_init(ms_blk_init);
module_exit(ms_blk_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MSPRO block device driver");

module_param(major, int, 0444);
MODULE_PARM_DESC(major, "specify the major device number for MMC block driver");
