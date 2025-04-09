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

#include <linux/mmc/card.h>
#include <linux/mmc/protocol_mmc.h>
#include <linux/mmc/protocol_ms.h>
#include <linux/mmc/host.h>

#include <asm/system.h>
#include <asm/uaccess.h>

#include "mmc_queue.h"

#include <linux/rtk_cr.h>
#include "mmc.h"
#include "rtk_sd.h"
#include "mmc_debug.h"

/*
 * max 8 partitions per card
 */
#define MMC_SHIFT   3
static int major;
#define MMC_NUM_MINORS  (256 >> MMC_SHIFT)
static unsigned long dev_use[MMC_NUM_MINORS/(8*sizeof(unsigned long))];

/*
 * There is one mmc_blk_data per slot.
 */
struct mmc_blk_data {
    spinlock_t  lock;
    struct mmc_card *card;
    struct gendisk  *disk;
    struct mmc_queue queue;

    unsigned int    usage;
    unsigned int    block_bits;
    unsigned short page_size;
    unsigned short cylinders;
    unsigned short heads;
    unsigned short sectors_per_track;
};

static DECLARE_MUTEX(open_lock);

static struct mmc_blk_data *mmc_blk_get(struct gendisk *disk)
{
    struct mmc_blk_data *md;
    mmcinfo("\n");
    down(&open_lock);
    md = disk->private_data;
    if (md && md->usage == 0)
        md = NULL;
    if (md)
        md->usage++;
    up(&open_lock);

    return md;
}

static void mmc_blk_put(struct mmc_blk_data *md)
{
    down(&open_lock);
    mmcinfo("md->usage:%u\n",md->usage);
    md->usage--;
    if (md->usage == 0) {
        int devidx;
        devidx = md->disk->first_minor >> MMC_SHIFT;

        blk_cleanup_queue(md->queue.queue);   /////

        __clear_bit(devidx, dev_use);

        put_disk(md->disk);
        //mmc_cleanup_queue(&md->queue);
        kfree(md);
    }
    up(&open_lock);
}

static int mmc_blk_open(struct inode *inode, struct file *filp)
{
    struct mmc_blk_data *md;
    mmcinfo("\n");
    int ret = -ENXIO;

    md = mmc_blk_get(inode->i_bdev->bd_disk);
    if (md) {
        if (md->usage == 2)
            check_disk_change(inode->i_bdev);
        ret = 0;
    }

    return ret;
}

static int mmc_blk_release(struct inode *inode, struct file *filp)
{
    struct mmc_blk_data *md = inode->i_bdev->bd_disk->private_data;
    mmcinfo("md:0x%p\n",md);
    if(md->usage)
        mmc_blk_put(md);
    return 0;
}

#define SDIO_GETCID 0x302
static int
mmc_blk_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct block_device *bdev = inode->i_bdev;
    mmcinfo("\n");
    if (cmd == HDIO_GETGEO) {
        struct hd_geometry geo;

        memset(&geo, 0, sizeof(struct hd_geometry));

        geo.cylinders   = get_capacity(bdev->bd_disk) / (4 * 16);
        geo.heads       = 4;
        geo.sectors     = 16;
        geo.start       = get_start_sect(bdev);

        return copy_to_user((void __user *)arg, &geo, sizeof(geo))
            ? -EFAULT : 0;
    }
    else if(cmd == SDIO_GETCID) {
        struct mmc_blk_data *md;
        struct mmc_card *card;

        md = mmc_blk_get(inode->i_bdev->bd_disk);
        card=md->card;

        return copy_to_user((void __user *)arg, card->raw_cid, 16)
            ? -EFAULT : 0;
    }

    return -ENOTTY;
}

static struct block_device_operations mmc_bdops = {
    .open           = mmc_blk_open,
    .release        = mmc_blk_release,
    .ioctl          = mmc_blk_ioctl,
    .owner          = THIS_MODULE,
};

struct mmc_blk_request {
    struct mmc_request  mrq;
    struct mmc_command  cmd;
    struct mmc_command  stop;
    struct mmc_data     data;
};

static int mmc_blk_prep_rq(struct mmc_queue *mq, struct request *req)
{
    struct mmc_blk_data *md = mq->data;
    int stat = BLKPREP_OK;
    mmcinfo("\n");
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

static int mmc_blk_issue_rq(struct mmc_queue *mq, struct request *req)
{
    struct mmc_blk_data *md = mq->data;
    struct mmc_card *card = md->queue.card;
    int ret;
    mmcinfo("\n");

    if (mmc_card_claim_host(card)){
        goto cmd_err;
    }

    do {
        struct mmc_blk_request brq;

        memset(&brq, 0, sizeof(struct mmc_blk_request));

        brq.cmd.retries=3;
        brq.mrq.cmd = &brq.cmd;
        brq.mrq.data = &brq.data;

        if ( mmc_card_blockaddr(card)){
            brq.cmd.arg = req->sector;
        }else{
            brq.cmd.arg = req->sector << 9;
        }

        brq.cmd.flags = MMC_RSP_R1;
        brq.data.timeout_ns = card->csd.tacc_ns * 10;
        brq.data.timeout_clks = card->csd.tacc_clks * 10;
        brq.data.blksz = (1<<md->block_bits);
        brq.data.blocks = req->nr_sectors >> (md->block_bits - 9);
        brq.stop.opcode = CMD12_STOP_TRANSMISSION;
        brq.stop.arg = 0;
        brq.stop.flags = MMC_RSP_R1B;

        if (rq_data_dir(req) == READ) {
            brq.cmd.opcode = brq.data.blocks > 1 ?
                        CMD18_READ_MULTIPLE_BLOCK : CMD17_READ_SINGLE_BLOCK;
            brq.data.flags |= MMC_DATA_READ;
        } else {
            brq.cmd.opcode = brq.data.blocks > 1 ?
                        CMD25_WRITE_MULTIPLE_BLOCK : CMD24_WRITE_BLOCK;
            brq.cmd.flags = MMC_RSP_R1B;
            brq.data.flags |= MMC_DATA_WRITE;
            //brq.data.blocks = 1;  //org
        }

        //brq.mrq.stop = brq.data.blocks > 1 ? &brq.stop : NULL;
        brq.mrq.stop = NULL;  /* RTK mmc hard ware support auto cmd12 */
        brq.data.sg = mq->sg;
        brq.data.sg_len = blk_rq_map_sg(req->q, req, brq.data.sg);

        mmc_wait_for_req(card->host, &brq.mrq);
        if (brq.cmd.error) {
            printk(KERN_ERR "%s: error %d sending read/write command\n",
                   req->rq_disk->disk_name, brq.cmd.error);
            goto cmd_err;
        }

        if (brq.data.error) {
            printk(KERN_ERR "%s: error %d transferring data\n",
                   req->rq_disk->disk_name, brq.data.error);
            goto cmd_err;
        }

        if (brq.stop.error) {
            printk(KERN_ERR "%s: error %d sending stop command\n",
                   req->rq_disk->disk_name, brq.stop.error);
            goto cmd_err;
        }
#ifndef TEST_DSC_MOD
        if ( rq_data_dir(req) != READ ){    //CMYu, 20090622
            unsigned long start;
            struct mmc_command cmd;
            start=jiffies+HZ;
            do {
                if(time_after(jiffies, start)){
                    printk(KERN_ERR "%s: requesting timeout\n",
                           req->rq_disk->disk_name);
                    goto cmd_err;
                }

                memset(&cmd, 0, sizeof(struct mmc_command));
                cmd.opcode = CMD13_SEND_STATUS;
                cmd.arg = card->rca << RCA_SHIFTER;
                cmd.flags = MMC_RSP_R1;
                mmc_wait_for_cmd(card->host, &cmd, 3);
                if (cmd.error) {
                    printk(KERN_ERR "%s: error %d requesting status\n",
                           req->rq_disk->disk_name, cmd.error);
                    goto cmd_err;
                }
            } while ( !(cmd.resp[0] & R1_READY_FOR_DATA) ); //Card Status: resp[0]=0x00000900

        }
#endif
        if (brq.cmd.error || brq.data.error || brq.stop.error){
            printk(KERN_ERR "brq error occurs\n");
            goto cmd_err;
        }

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

    mmc_card_release_host(card);

    return 1;

 cmd_err:
    mmc_card_release_host(card);
// rmv_err:
    //brq.cmd.error=MMC_ERR_FAILED;
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

static int ms_blk_issue_rq(struct mmc_queue *mq, struct request *req)
{

    struct mmc_blk_data *md = mq->data;
    struct mmc_card *card = md->queue.card;
    int ret;
    sector_t sector_start;
    int page_size = 512;

    unsigned int count;
    //int try_cnt = 0;

    mmcinfo("\n");

    if(card->host->ins_event){
        goto plug_err;
    }
    if (mmc_card_claim_host(card))
        goto req_err;

    do {
        struct mmc_blk_request brq;

        memset(&brq, 0, sizeof(struct mmc_blk_request));
        brq.mrq.data = &brq.data;

        sector_start = req->sector << 9;
        sector_div(sector_start, page_size);

        brq.data.blksz = (1<<md->block_bits);
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

        mmc_wait_for_req(card->host, &brq.mrq);

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

        mmc_wait_for_req(card->host, &brq.mrq);

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
            brq.data.flags |= MMC_DATA_READ;
        }else{
            brq.mrq.tpc = MSPRO_TPC_WRITE_LONG_DATA;
            brq.data.flags |= MMC_DATA_WRITE;
        }
        brq.mrq.sector_cnt = count;
        //brq.mrq.sector_start = sector_start;
        brq.mrq.timeout = 10000;
        brq.data.sg = mq->sg;
        brq.data.sg_len = blk_rq_map_sg(req->q, req, brq.data.sg);
        mmc_wait_for_req(card->host, &brq.mrq);

        if (brq.mrq.error) {
            printk(KERN_ERR "%s: error %d transferring data\n",
                   req->rq_disk->disk_name, brq.data.error);
            goto req_err;
        }

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

    mmc_card_release_host(card);

    return 1;

 req_err:
    mmc_card_release_host(card);
 plug_err:
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
//#define MMC_NUM_MINORS    (256 >> MMC_SHIFT)
//static unsigned long dev_use[MMC_NUM_MINORS/(8*sizeof(unsigned long))];

static struct mmc_blk_data *mmc_blk_alloc(struct mmc_card *card)
{
    struct mmc_blk_data *md;
    int devidx, ret;
    mmcinfo("\n");
    devidx = find_first_zero_bit(dev_use, MMC_NUM_MINORS);
    if (devidx >= MMC_NUM_MINORS)
        return ERR_PTR(-ENOSPC);
    __set_bit(devidx, dev_use);

    md = kmalloc(sizeof(struct mmc_blk_data), GFP_KERNEL);
    if (md) {
        memset(md, 0, sizeof(struct mmc_blk_data));

        md->disk = alloc_disk(1 << MMC_SHIFT);
        if (md->disk == NULL) {
            kfree(md);
            md = ERR_PTR(-ENOMEM);
            goto out;
        }

        spin_lock_init(&md->lock);
        md->usage = 1;

        ret = mmc_init_queue(&md->queue, card, &md->lock);
        if (ret) {
            put_disk(md->disk);
            kfree(md);
            md = ERR_PTR(ret);
            goto out;
        }
        md->queue.prep_fn = mmc_blk_prep_rq;

        if(card->card_type&(CR_SD|CR_SDHC|CR_MMC))
            md->queue.issue_fn = mmc_blk_issue_rq;
        else
            md->queue.issue_fn = ms_blk_issue_rq;

        md->queue.data = md;

        md->disk->major = major;
        md->disk->first_minor = devidx << MMC_SHIFT;
        md->disk->fops = &mmc_bdops;
        md->disk->private_data = md;
        md->disk->queue = md->queue.queue;
        md->disk->driverfs_dev = &card->dev;

        strcpy(md->disk->port_structure, "1");    /*  2010/05/31 port structure */
        strcpy(md->disk->bus_type, "card");     /*  2010/05/31 bus type */

        /*
         * As discussed on lkml, GENHD_FL_REMOVABLE should:
         *
         * - be set for removable media with permanent block devices
         * - be unset for removable block devices with permanent media
         *
         * Since MMC block devices clearly fall under the second
         * case, we do not set GENHD_FL_REMOVABLE.  Userspace
         * should use the block device creation/destruction hotplug
         * messages to tell when the card is present.
         */
        sprintf(md->disk->disk_name, "mmcblk%d", devidx);
        sprintf(md->disk->devfs_name, "mmc/blk%d", devidx);

        if(card->card_type&(CR_SD|CR_SDHC|CR_MMC)){
            if(card->csd.read_blkbits>MMC_BLK_BITS)
                md->block_bits = MMC_BLK_BITS;
            else
                md->block_bits = card->csd.read_blkbits;

            blk_queue_hardsect_size(md->queue.queue, 1 << md->block_bits);

            if(card->csd.read_blkbits>MMC_BLK_BITS){
                set_capacity(md->disk,card->csd.capacity << (card->csd.read_blkbits - MMC_BLK_BITS));
                //printk(KERN_INFO"1. capacity=%u block\n",
                //            card->csd.capacity << (card->csd.read_blkbits - MMC_BLK_BITS));
                if((card->csd.capacity << (card->csd.read_blkbits - MMC_BLK_BITS)) < 0x100000)
                    mmc_card_set_one_blk(card);

            }else{
                set_capacity(md->disk, card->csd.capacity);

                //printk(KERN_INFO"2. capacity=%u block\n",
                //            card->csd.capacity);
                if(card->csd.capacity < 0x100000)
                    mmc_card_set_one_blk(card);

            }
            if(mmc_card_one_blk(card))
                printk(KERN_INFO"one block mode at write\n");

        }else{
            md->block_bits = 9;

            blk_queue_hardsect_size(md->queue.queue, 1 << md->block_bits);
            set_capacity(md->disk, card->capacity);

            md->cylinders = card->cylinders;
            md->heads = card->heads;
            md->sectors_per_track = card->sectors_per_track;
        }
    }else{
        printk("Allocating mmc_blk_data fails.");
        md = ERR_PTR(-ENOMEM);
        goto out;
    }

 out:
    return md;
}

static int
mmc_blk_set_blksize(struct mmc_blk_data *md, struct mmc_card *card)
{
    struct mmc_command cmd;

    mmcinfo("\n");
    mmc_card_claim_host(card);
    memset(&cmd, 0, sizeof(struct mmc_command));
    cmd.opcode = CMD16_SET_BLOCKLEN;
    cmd.arg = 1 << MMC_BLK_BITS;

    cmd.flags = MMC_RSP_R1;
    mmc_wait_for_cmd(card->host, &cmd, 5);
    mmc_card_release_host(card);

    if (cmd.error) {
        printk(KERN_ERR "%s: unable to set block size to %d: %d\n",
            md->disk->disk_name, cmd.arg, cmd.error);
        return -EINVAL;
    }

    return 0;
}

static int mmc_blk_probe(struct mmc_card *card)
{
    struct mmc_blk_data *md;
    int err;
     mmcinfo("\n");
    /*
     * Check that the card supports the command class(es) we need.
     */
    if(card->card_type & (CR_SD|CR_SDHC|CR_MMC)){
        if (!(card->csd.cmdclass & CCC_BLOCK_READ)){
            printk(KERN_WARNING "do not support CCC_BLOCK_READ\n");
            return -ENODEV;
        }

        if (card->csd.read_blkbits < MMC_BLK_BITS) {
            printk(KERN_WARNING "%s: read blocksize too small (%u)\n",
                mmc_card_id(card), 1 << card->csd.read_blkbits);
            return -ENODEV;
        }

        md = mmc_blk_alloc(card);
        if (IS_ERR(md)){
            printk("mmc_blk_alloc() fails\n");
            return PTR_ERR(md);
        }

        err = mmc_blk_set_blksize(md, card);
        if (err){
            printk("mmc_blk_set_blksize() fails\n");
            goto out;
        }

        printk(KERN_INFO "CARD-READER:%s: ID:%s NAME:%s %u KiB\n",
            md->disk->disk_name,
            mmc_card_id(card),
            mmc_card_name(card),
            (card->csd.capacity / 1024) << card->csd.read_blkbits );
    }else{
        md = mmc_blk_alloc(card);
        if (IS_ERR(md)){
            printk("ms_blk_alloc() fails\n");
            return PTR_ERR(md);
        }

        md->page_size = 512;
    }
    mmc_set_drvdata(card, md);
    add_disk(md->disk);
    return 0;

 out:
    mmc_blk_put(md);

    return err;
}

static void mmc_blk_remove(struct mmc_card *card)
{
    struct mmc_blk_data *md = mmc_get_drvdata(card);
    mmcinfo("\n");
    if (md) {
        //int devidx;
        del_gendisk(md->disk);
        /*
         * I think this is needed.
         */
        //md->disk->queue = NULL;

        //devidx = md->disk->first_minor >> MMC_SHIFT;
        //__clear_bit(devidx, dev_use);
        mmc_cleanup_queue(&md->queue);
        mmc_blk_put(md);
    }
    mmc_set_drvdata(card, NULL);
}

#ifdef CONFIG_PM
static int mmc_blk_suspend(struct mmc_card *card, pm_message_t state)
{
    struct mmc_blk_data *md = mmc_get_drvdata(card);
    mmcinfo("\n");
    if (md) {
        mmc_queue_suspend(&md->queue);
    }
    return 0;
}

static int mmc_blk_resume(struct mmc_card *card)
{
    struct mmc_blk_data *md = mmc_get_drvdata(card);
    if (md) {
        //mmc_blk_set_blksize(md, card);
        mmc_queue_resume(&md->queue);
    }
    return 0;
}
#else
#define mmc_blk_suspend NULL
#define mmc_blk_resume  NULL
#endif

static struct mmc_driver mmc_driver = {
    .drv        = {
        .name   = "mmcblk",
    },
    .probe      = mmc_blk_probe,
    .remove     = mmc_blk_remove,
    .suspend    = mmc_blk_suspend,
    .resume     = mmc_blk_resume,
};

static int __init mmc_blk_init(void)
{
    int res = -ENOMEM;
//    printk("mmc_blk_init\n");
    res = register_blkdev(major, "mmc");
    if (res < 0) {
        printk(KERN_WARNING "Unable to get major %d for MMC media: %d\n",
               major, res);
        goto out;
    }
    if (major == 0)
        major = res;

    devfs_mk_dir("mmc");
    return mmc_register_driver(&mmc_driver);

 out:
    return res;
}

static void __exit mmc_blk_exit(void)
{
//    printk("mmc_blk_exit\n");
    mmc_unregister_driver(&mmc_driver);
    devfs_remove("mmc");
    unregister_blkdev(major, "mmc");
}

module_init(mmc_blk_init);
module_exit(mmc_blk_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Multimedia Card (MMC) block device driver");

module_param(major, int, 0444);
MODULE_PARM_DESC(major, "specify the major device number for MMC block driver");
