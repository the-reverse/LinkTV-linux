/*
 * main.c -- the bare se char module
 *
 * Copyright (C) 2001 Alessandro Rubini and Jonathan Corbet
 * Copyright (C) 2001 O'Reilly & Associates
 *
 * The source code in this file can be freely used, adapted,
 * and redistributed in source or binary form, so long as an
 * acknowledgment appears in derived source files.  The citation
 * should list that the code comes from the book "Linux Device
 * Drivers" by Alessandro Rubini and Jonathan Corbet, published
 * by O'Reilly & Associates.   No warranty is attached;
 * we cannot take responsibility for errors or fitness for use.
 *
 */

//#define SIMULATOR

//#define LINUX_2_6_34
#ifdef LINUX_2_6_34
#include <generated/autoconf.h>
#endif
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include <linux/kernel.h>	/* DBG_PRINT() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>

#include <asm/system.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <asm/io.h>
#include <linux/sched.h>

#include <asm/irq.h>
#include <linux/signal.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <platform.h>

#include "se_driver.h"
#include "SeReg.h"

#define SeClearWriteData 0

#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

#define SB2_CPU_INT_EN_reg 0xB801A108
#define SB2_CPU_INT_reg    0xB801A104


#define VSYNC_SHARE_MEM_ADDR 0xA00000DC
typedef enum
{
    SeWriteData = BIT0,
    SeGo = BIT1,
    SeEndianSwap = BIT2

} SE_CTRL_REG;

typedef enum
{
    SeIdle = BIT0

} SE_IDLE_REG;


//interrupt status and control bits
typedef enum
{
    SeIntCommandEmpty = BIT3,
    SeIntCommandError = BIT2,
    SeIntSync = BIT1

} SE_INT_STATUS_REG, SE_INT_CTRL_REG;


#define SEINFO_COMMAND_QUEUE0 0                         //The Definition of Command Queue Type: Command Queue 0
#define SEINFO_COMMAND_QUEUE1 1                         //The Definition of Command Queue Type: Command Queue 1
#define SEINFO_COMMAND_QUEUE2 2                         //The Definition of Command Queue Type: Command Queue 2

//#include <linux/devfs_fs_kernel.h>

//#define DBG_PRINT(s, args...) printk(s, ## args)
#define DBG_PRINT(s, args...)


#define endian_swap_32(a) (((a)>>24) | (((a)>>8) & 0xFF00) | (((a)<<8) & 0xFF0000) | (((a)<<24) & 0xFF000000))

#ifdef LINUX_2_6_34
static struct class *se_class;
#endif

struct se_dev *se_devices = NULL;	/* allocated in se_init_module */
/*
 * Our parameters which can be set at load time.
 */

int se_major =   SE_MAJOR;
int se_minor =   0;
int se_nr_devs = SE_NR_DEVS;	/* number of bare se devices */
int se_quantum = SE_QUANTUM;
int se_qset =    SE_QSET;

module_param(se_major, int, S_IRUGO);
module_param(se_minor, int, S_IRUGO);
module_param(se_nr_devs, int, S_IRUGO);
module_param(se_quantum, int, S_IRUGO);
module_param(se_qset, int, S_IRUGO);

MODULE_AUTHOR("Alessandro Rubini, Jonathan Corbet");
MODULE_LICENSE("Dual BSD/GPL");

#define DG_LOCK
#define DG_UNLOCK
#define SE_IRQ 5

inline void
InitSeReg(void)
{
    volatile SEREG_INFO  *SeRegInfo = (volatile SEREG_INFO  *)SEREG_BASE_ADDRESS;
    //Stop Streaming Engine
    SeRegInfo->SeCtrl[0].Value = (SeGo | SeEndianSwap | SeClearWriteData);
    SeRegInfo->SeCtrl[0].Value = (SeEndianSwap | SeWriteData);
    
	SeRegInfo->SeCmdBase[0].Value = (uint32_t) se_devices->CmdBase;
	SeRegInfo->SeCmdLimit[0].Value = (uint32_t) se_devices->CmdLimit;
	SeRegInfo->SeCmdReadPtr[0].Value = (uint32_t) se_devices->CmdBase;
	SeRegInfo->SeCmdWritePtr[0].Value = (uint32_t) se_devices->CmdBase;
	SeRegInfo->SeInstCnt[0].Value = 0;
	se_devices->CmdWritePtr = se_devices->CmdBase;;
	se_devices->u64InstCnt = 0;
	se_devices->u64IssuedInstCnt = 0;
    printk("instcnt=%d, u64cnt=%lld, u64IssuedInstCnt=%lld\n", SeRegInfo->SeInstCnt[0].Value & 0xFFFF, se_devices->u64InstCnt, se_devices->u64IssuedInstCnt);
}

void UpdateInstCount(struct se_dev *dev)
{
    uint16_t        u16CmdExecuted;
    uint16_t        u16InsCntLow;
    volatile SEREG_INFO  *SeRegInfo = (volatile SEREG_INFO  *)SEREG_BASE_ADDRESS;

    u16CmdExecuted = SeRegInfo->SeInstCnt[0].Value & 0xFFFF;
    u16InsCntLow = dev->u64InstCnt & 0xFFFFLL;
    if(u16InsCntLow > u16CmdExecuted)
    {
        dev->u64InstCnt = (dev->u64InstCnt & ~0xFFFFLL) + 0x10000LL + u16CmdExecuted;
    }
    else
    {
        dev->u64InstCnt = (dev->u64InstCnt & ~0xFFFFLL) + u16CmdExecuted;
    }
}

void WriteCmd(struct se_dev *dev, uint8_t *pbyCommandBuffer, int32_t lCommandLength)
{
    uint32_t dwDataCounter = 0;
    uint32_t wrptr;
    volatile SEREG_INFO  *SeRegInfo = (volatile SEREG_INFO  *)SEREG_BASE_ADDRESS;

    wrptr = dev->CmdWritePtr - dev->CmdBase;

    while(1)
    {
        uint32_t rdptr = (uint32_t) SeRegInfo->SeCmdReadPtr[0].Value;
        if(rdptr <= wrptr)
        {
            rdptr += dev->size;
        }

        if((wrptr + lCommandLength) < rdptr)
        {
            break;
        }
    }


	//printk("[%x-%d]\n", &dev->CmdBuf[wrptr], lCommandLength);
    //DBG_PRINT("[\n");
    //Start writing command words to the ring buffer.
    for(dwDataCounter = 0; dwDataCounter < (uint32_t) lCommandLength; dwDataCounter += sizeof(uint32_t))
    {
        //DBG_PRINT("(%8x-%8x)\n", (uint32_t)pWptr, *(uint32_t *)(pbyCommandBuffer + dwDataCounter));

        *(uint32_t *)((dev->CmdBase + wrptr) | 0xA0000000) = *(uint32_t *)(pbyCommandBuffer + dwDataCounter);
        wrptr += sizeof(uint32_t);
        if(wrptr >= dev->size)
        {
            wrptr = 0;
        }
    }

    // sync the write buffer
    __asm__ __volatile__ (".set push");
    __asm__ __volatile__ (".set mips32");
    __asm__ __volatile__ ("sync;");
    __asm__ __volatile__ (".set pop");

    //convert to physical address
    dev->CmdWritePtr = (uint32_t)wrptr + dev->CmdBase;
    SeRegInfo->SeCmdWritePtr[0].Value = (uint32_t) dev->CmdWritePtr;
    SeRegInfo->SeCtrl[0].Value = (SeGo | SeEndianSwap | SeWriteData);
}

/* This function services keyboard interrupts. It reads the relevant
 *  * information from the keyboard and then scheduales the bottom half
 *   * to run when the kernel considers it safe.
 *    */
#ifdef LINUX_2_6_34
irqreturn_t se_irq_handler(int irq, void *dev_id)
#else
irqreturn_t se_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
  struct se_dev *dev = se_devices;
    
  if(dev->isMars || dev->isJupiter)
  {
    if((*(volatile uint32_t *)VSYNC_SHARE_MEM_ADDR) & endian_swap_32(0x2))
    {
        int dirty = 0;
        vsync_queue_node_t *p_node = dev->p_vsync_queue_head;

        DBG_PRINT("se_irq_handler\n");

        *(volatile uint32_t *)VSYNC_SHARE_MEM_ADDR = endian_swap_32(1);

        if(dev->ignore_interrupt)
        {
            return IRQ_HANDLED;
        }

        while(p_node)
        {
            vsync_queue_t *p_vsync_queue = (vsync_queue_t *)p_node->p_vsync_queue;

            if(p_vsync_queue)
            {
                uint8_t *u8Buf = (uint8_t *)&p_vsync_queue->u8Buf;
                uint32_t u32RdPtr = p_vsync_queue->u32RdPtr;
                uint32_t u32WrPtr = p_vsync_queue->u32WrPtr;

                while(u32RdPtr != u32WrPtr)
                {
                    uint32_t len = *(uint32_t *)&u8Buf[u32RdPtr];
                    dirty = 1;

                    DBG_PRINT("2: r=%d, w=%d, size=%d\n",
                                   u32RdPtr, u32WrPtr, *(uint32_t *)&u8Buf[u32RdPtr]);

                    if(len == 0)
                    {
                        u32RdPtr += 4;
                        break;
                    }
                    else
                    {
                        len -= 4;
                        u32RdPtr += 4;
                        WriteCmd(dev, &u8Buf[u32RdPtr], len);
                        u32RdPtr += len;
                    }
                }
                p_vsync_queue->u32RdPtr = u32RdPtr;
            }
            p_node = p_node->p_next;
        }
        if(!dirty)
        {
            vsync_queue_node_t *p_node = dev->p_vsync_queue_head;
            while(p_node)
            {
                vsync_queue_node_t *p_node_tmp = p_node;
                if(p_node->p_vsync_queue)
                {
                    kfree((void *)p_node->p_vsync_queue);
                }
                p_node = p_node->p_next;
                kfree((void *)p_node_tmp);
            }
            dev->p_vsync_queue_head = dev->p_vsync_queue_tail = NULL;
            *(volatile uint32_t *)VSYNC_SHARE_MEM_ADDR = endian_swap_32(0);
        }
        return IRQ_HANDLED;
    }
  }
  return IRQ_NONE;
}
/*
 * Open and close
 */

int se_open(struct inode *inode, struct file *filp)
{
	struct se_dev *dev; /* device information */
    int queue = iminor(inode);    //we use minor number to indicate queue
    printk(" *****************    se open: queue = %d   *********************\n", queue);


	DBG_PRINT(KERN_INFO "se open\n");

	dev = container_of(inode->i_cdev, struct se_dev, cdev);
	filp->private_data = dev; /* for other methods */

	/* now trim to 0 the length of the device if open was write-only */
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

    printk(" *****************    se open: initialized = %d  *********************\n", dev->initialized);

    DBG_PRINT("se se_dev = %x\n", (uint32_t)dev);

	if(dev->initialized)
	{
	    dev->initialized ++;
	    up(&dev->sem);
	    return 0;          /* success */
	}

	dev->initialized ++;

    dev->queue = queue;
        
    *(volatile uint32_t *)VSYNC_SHARE_MEM_ADDR = 0;

	up(&dev->sem);
    DBG_PRINT("%d, %d, %d, %d\n", SE_IOC_READ_INST_COUNT, SE_IOC_READ_SW_CMD_COUNTER, SE_IOC_WRITE_ISSUE_CMD, SE_IOC_WRITE_QUEUE_CMD);
	return 0;          /* success */
}

int se_release(struct inode *inode, struct file *filp)
{
	//struct se_dev *dev = filp->private_data; /* for other methods */
	struct se_dev *dev = se_devices;
    int queue = dev->queue;
    volatile SEREG_INFO  *SeRegInfo = (volatile SEREG_INFO  *)SEREG_BASE_ADDRESS;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

    dev->initialized --;
	printk("se release, count = %d\n", dev->initialized);
	if(dev->initialized > 0)
	{
	    up(&dev->sem);
		return  0;
	}

	//stop SE
    SeRegInfo->SeCtrl[queue].Value = (SeGo | SeEndianSwap | SeClearWriteData);

    //free_irq(SE_IRQ, dev);

	up(&dev->sem);

	DBG_PRINT(KERN_INFO "se release\n");

	return 0;
}

/*
 * Data management: read and write
 */

ssize_t se_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	return -EFAULT;
}

ssize_t _se_write(struct file *filp, const char __user *buf, size_t count)
{
	//struct se_dev *dev = filp->private_data;
	struct se_dev *dev = se_devices;
	ssize_t retval = -ENOMEM; /* value used in "goto out" statements */

    if(count <= 256)
    {
        uint8_t data[256];
	    if (copy_from_user(data, buf, count))
        {
		    retval = -EFAULT;
		    goto out;
        } 

        WriteCmd(dev, (uint8_t *)data, count);
    }
    else
    {
        uint8_t *data = kmalloc(count, GFP_KERNEL);
        if (copy_from_user(data, buf, count))
        {
            retval = -EFAULT;
            goto out;
        }

        WriteCmd(dev, (uint8_t *)data, count);
		kfree(data);
    }

	retval = count;

out:
	return retval;
}

ssize_t se_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	//struct se_dev *dev = filp->private_data;
	struct se_dev *dev = se_devices;
    ssize_t size;
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
    size = _se_write(filp, buf, count);
    if(f_pos) *f_pos += size;
	up(&dev->sem);
    return size;
}

/*
 * The ioctl() implementation
 */

int se_ioctl(struct inode *inode, struct file *filp,
                 unsigned int cmd, unsigned long arg)
{
	//struct se_dev *dev = filp->private_data;
	struct se_dev *dev = se_devices;
	int retval = 0;

    DBG_PRINT("se ioctl code=%d !!!!!!!!!!!!!!!1\n", cmd);
	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
    
    //DBG_PRINT("se ioctl code = %d\n", cmd);
	switch(cmd) {
    case SE_IOC_READ_INST_COUNT:
        {
            DBG_PRINT("%s:%d, se ioctl code=%d!!!!!!!!!!!!!!!1\n", __FILE__, __LINE__, cmd);
            UpdateInstCount(dev);
            if (copy_to_user((void __user *)arg, (void *)&dev->u64InstCnt, sizeof(dev->u64InstCnt))) {
                retval = -EFAULT;
                goto out;
            }
            //DBG_PRINT("se ioctl code=SE_IOC_READ_INST_COUNT:\n");
            break;
        }
    case SE_IOC_READ_SW_CMD_COUNTER:
        {
            DBG_PRINT("%s:%d, se ioctl code=%d, counter = %lld!!!!!!!!!!!!!!!1\n", __FILE__, __LINE__, cmd, dev->u64IssuedInstCnt);
//            printk("<%lld\>\n", dev->u64IssuedInstCnt);
            if (copy_to_user((void __user *)arg, (void *)&dev->u64IssuedInstCnt, sizeof(dev->u64IssuedInstCnt))) {
                retval = -EFAULT;
                goto out;
            }
            //DBG_PRINT("se ioctl code=SE_IOC_READ_INST_COUNT:\n");
            break;
        }
    case SE_IOC_WRITE_ISSUE_CMD:
        {
            struct {
                uint32_t instCnt;
                uint32_t length;
                void *   cmdBuf;
            } header;
            DBG_PRINT("%s:%d, se ioctl code=%d!!!!!!!!!!!!!!!1\n", __FILE__, __LINE__, cmd);
            dev->ignore_interrupt = 1;
            if (copy_from_user((void *)&header, (const void __user *)arg, sizeof(header))) {
                retval = -EFAULT;
                DBG_PRINT("se ioctl code=SE_IOC_WRITE_ISSUE_CMD failed!!!!!!!!!!!!!!!1\n");
                goto out;
            }
            
            DBG_PRINT("se ioctl code=SE_IOC_WRITE_ISSUE_CMD!!!!! count = %d, size = %d, 0x%08x\n", header.instCnt, header.length, (uint32_t)header.cmdBuf);
            _se_write(filp, (const void __user *)header.cmdBuf, header.length);
            dev->u64IssuedInstCnt += header.instCnt;
			//printk("<%lld>\n", dev->u64IssuedInstCnt);
            dev->ignore_interrupt = 0;
            DBG_PRINT("se ioctl code=SE_IOC_WRITE_ISSUE_CMD\n");
            break;
        }
    case SE_IOC_WRITE_QUEUE_CMD:
        {
            int ii;
            struct {
                uint32_t instCnt;;
                uint32_t count;
            } header;
            uint32_t count;
            vsync_queue_t *p_vsync_queue[MAX_QUEUE_ENTRIES] = {NULL,};

	        DBG_PRINT("%s:%d:%s\n", __FILE__, __LINE__, __func__);
            DBG_PRINT("se ioctl code=SE_IOC_WRITE_QUEUE_CMD\n");

            if (copy_from_user((void *)&header, (const void __user *)arg, sizeof(header)))
            {
                retval = -EFAULT;
                DBG_PRINT("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
                goto out;
            }
            count = header.count;
            if(count > MAX_QUEUE_ENTRIES)
            {
                retval = -EFAULT;
                DBG_PRINT("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
                goto out;
            }
            if (copy_from_user((void *)p_vsync_queue, (const void __user *)((uint32_t)arg + sizeof(header)), count*sizeof(vsync_queue_t *)))
            {
                retval = -EFAULT;
                DBG_PRINT("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
                goto out;
            }
            DBG_PRINT("%d, count = %d, instCount = %d\n", __LINE__, count, header.instCnt);
            dev->ignore_interrupt = 1;
            for(ii=0; ii<count; ii++)
            {
                uint32_t size;
                uint8_t *buf;
                vsync_queue_node_t *p_node;
                if (copy_from_user((void *)&size, (const void __user *)p_vsync_queue[ii], sizeof(uint32_t)))
                {
                    retval = -EFAULT;
                    DBG_PRINT("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
                    dev->ignore_interrupt = 0;
                    goto out;
                }

                size += offsetof(vsync_queue_t, u8Buf);
                buf = kmalloc(size, GFP_KERNEL);
                if(buf == NULL)
                {
                    retval = -EFAULT;
                    DBG_PRINT("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
                    dev->ignore_interrupt = 0;
                    goto out;
                }

                if(copy_from_user((void *)buf, (const void __user *)p_vsync_queue[ii], size))
                {
                    retval = -EFAULT;
                    DBG_PRINT("%d, se ioctl code=SE_IOC_WRITE_QUEUE_CMD failed!!!!!!!!!!!!!!!1\n", __LINE__);
                    dev->ignore_interrupt = 0;
                    goto out;
                }
                p_node = (vsync_queue_node_t *)kmalloc(sizeof(vsync_queue_node_t), GFP_KERNEL);
                p_node->p_next = NULL;
                p_node->p_vsync_queue = (vsync_queue_t *)buf;;
                if(dev->p_vsync_queue_head == NULL)
                {
                    dev->p_vsync_queue_head = dev->p_vsync_queue_tail = p_node;
                }
                else
                {
                    dev->p_vsync_queue_tail->p_next = p_node;
                    dev->p_vsync_queue_tail = p_node;
                }
            }
			DBG_PRINT("@@@@@@@@@@@ se.c,(%p, %lld)\n", &dev->u64IssuedInstCnt, dev->u64IssuedInstCnt);
            //dev->u64QueueInstCnt += header.instCnt;
            dev->u64IssuedInstCnt += header.instCnt;
			DBG_PRINT("@@@@@@@@@@@ se.c,(%lld)\n", dev->u64IssuedInstCnt);
            dev->ignore_interrupt = 0;
            *(volatile uint32_t *)VSYNC_SHARE_MEM_ADDR |= endian_swap_32(1);
            break;
        }
    default:  /* redundant, as cmd was checked against MAXNR */
        DBG_PRINT("se ioctl code not supported\n");
		retval = -ENOTTY;
        goto out;
	}
  out:
    DBG_PRINT("%s:%d, se ioctl code=%d!!!!!!!!!!!!!!!1\n", __FILE__, __LINE__, cmd);
	up(&dev->sem);
	return retval;
}


/*
 * The "extended" operations -- only seek
 */

loff_t se_llseek(struct file *filp, loff_t off, int whence)
{
	return -EINVAL;
}

#include <linux/mm.h>
int se_mmap(struct file *filp, struct vm_area_struct *vma)
{
	//struct se_dev *dev = filp->private_data;
	struct se_dev *dev = se_devices;
    unsigned long offset = vma->vm_pgoff;
    unsigned long prot;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

	printk("%s, %d: offset = %ld, se_devices = %x\n", __FILE__, __LINE__, offset, (unsigned int)se_devices);
    if (offset > (~0UL >> PAGE_SHIFT))
	{
	    printk("%s, %d: offset = %ld\n", __FILE__, __LINE__, offset);
        up(&dev->sem);
        return -EINVAL;
	}

    offset = offset << PAGE_SHIFT;
    
	printk("%s, %d: offset = %ld\n", __FILE__, __LINE__, offset);
#ifdef LINUX_2_6_34
    vma->vm_flags |= VM_IO | VM_RESERVED;
    if(offset == 0)
    {
        offset = (unsigned long)__pa(dev);
    }
    else if(offset == 0x1800C000)
    {
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    }
    else
    {
        printk("%s, %d: offset = %ld\n", __FILE__, __LINE__, offset);
        up(&dev->sem);
        return -EINVAL;
    }
#else
    prot = pgprot_val(vma->vm_page_prot);
    if(offset == 0)
    {
	    offset = (unsigned long)__pa(dev);
    }
    else if(offset == 0x1800C000)
    {
        prot = (prot & ~_CACHE_MASK) | _CACHE_UNCACHED;
    }
	else
	{
	    printk("%s, %d: offset = %ld\n", __FILE__, __LINE__, offset);
        up(&dev->sem);
        return -EINVAL;
	}
	/* This is an IO map - tell maydump to skip this VMA */
    vma->vm_flags |= VM_RESERVED | VM_IO;
	vma->vm_pgoff = offset >> PAGE_SHIFT;

    if (prot & _PAGE_WRITE)
        prot = prot | _PAGE_FILE | _PAGE_VALID | _PAGE_DIRTY;
    else
        prot = prot | _PAGE_FILE | _PAGE_VALID;
    prot &= ~_PAGE_PRESENT;
    vma->vm_page_prot = __pgprot(prot);
#endif
	printk("%s, %d: length = %ld\n", __FILE__, __LINE__, vma->vm_end-vma->vm_start);
	printk("%s, %d: offset = %ld\n", __FILE__, __LINE__, offset);
    if (io_remap_pfn_range(vma, vma->vm_start, offset >> PAGE_SHIFT, 
           vma->vm_end-vma->vm_start, vma->vm_page_prot))
    {
        up(&dev->sem);
        return -EAGAIN;
    }
	printk("%s, %d: offset = %ld\n", __FILE__, __LINE__, offset);
    up(&dev->sem);
    return 0;
}

static int se_suspend(struct device *dev, pm_message_t state, u32 level){
    printk("se_suspend\n");
    se_devices->ignore_interrupt = 1;
    return 0;
}

static int se_resume(struct device *dev, u32 level)  {
    printk("se_resume\n");
    InitSeReg();
    se_devices->ignore_interrupt = 0;
    return 0;
}

static struct platform_device *se_platform_devs;

static struct device_driver se_device_driver = {
    .name       = "SE",
    .bus        = &platform_bus_type,
    .suspend    = se_suspend,
    .resume     = se_resume,
};

struct file_operations se_fops = {
	.owner =    THIS_MODULE,
	.llseek =   se_llseek,
	.read =     se_read,
	.write =    se_write,
	.ioctl =    se_ioctl,
	.open =     se_open,
	.release =  se_release,
	.mmap =  se_mmap,
};

/*
 * Finally, the module stuff
 */

/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */
void se_cleanup_module(void)
{
	dev_t devno = MKDEV(se_major, se_minor);

	DBG_PRINT(KERN_INFO "se clean module se_major = %d\n", se_major);

    *(volatile uint32_t *)VSYNC_SHARE_MEM_ADDR = endian_swap_32(0);
	free_irq(SE_IRQ, (void *)se_devices);
	/* Get rid of our char dev entries */
	if (se_devices) {
		cdev_del(&se_devices->cdev);
#ifdef LINUX_2_6_34
        device_destroy(se_class, MKDEV(se_major, 0));
#endif
		kfree(se_devices);
	}
#ifdef LINUX_2_6_34
    class_destroy(se_class);
#endif

    /* device driver removal */
    platform_device_unregister(se_platform_devs);
    driver_unregister(&se_device_driver);

	/* cleanup_module is never called if registering failed */
	unregister_chrdev_region(devno, se_nr_devs);
}


/*
 * Set up the char_dev structure for this device.
 */
static void se_setup_cdev(struct se_dev *dev, int index)
{
	int err, devno = MKDEV(se_major, se_minor + index);
    
	cdev_init(&dev->cdev, &se_fops);
	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &se_fops;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		DBG_PRINT(KERN_NOTICE "Error %d adding se%d", err, index);
#ifdef LINUX_2_6_34
    device_create(se_class, NULL, MKDEV(se_major, index), NULL, "se%d", index);
#endif
}

#ifdef LINUX_2_6_34
static char *se_devnode(struct device *dev, mode_t *mode)
{
    return NULL;
}
#endif


int se_init_module(void)
{
	int result;
	dev_t dev = 0;

/*
 * Get a range of minor numbers to work with, asking for a dynamic
 * major unless directed otherwise at load time.
 */

    printk("\n\n\n\n *****************    se init module  *********************\n\n\n\n");
	if (se_major) {
		dev = MKDEV(se_major, se_minor);
		result = register_chrdev_region(dev, se_nr_devs, "se");
	} else {
		result = alloc_chrdev_region(&dev, se_minor, se_nr_devs,
				"se");
		se_major = MAJOR(dev);
	}
	if (result < 0) {
		DBG_PRINT(KERN_WARNING "se: can't get major %d\n", se_major);
		return result;
	}

	printk("se init module major number = %d\n", se_major);
#ifdef LINUX_2_6_34
	// devfs_mk_cdev(MKDEV(se_major, se_minor), S_IFCHR|S_IRUSR|S_IWUSR, "se0");
	se_class = class_create(THIS_MODULE, "se");
	if (IS_ERR(se_class))
	    return PTR_ERR(se_class);

	se_class->devnode = se_devnode;
#else
    devfs_mk_cdev(MKDEV(se_major, se_minor), S_IFCHR|S_IRUSR|S_IWUSR, "se0");
    //devfs_mk_cdev(MKDEV(se_major, se_minor+1), S_IFCHR|S_IRUSR|S_IWUSR, "se1");
    //devfs_mk_cdev(MKDEV(se_major, se_minor+2), S_IFCHR|S_IRUSR|S_IWUSR, "se2");
    //devfs_mk_cdev(MKDEV(se_major, se_minor+3), S_IFCHR|S_IRUSR|S_IWUSR, "se3");
#endif

    se_platform_devs = platform_device_register_simple("SE", -1, NULL, 0);
    if(driver_register(&se_device_driver) != 0)
    {
        platform_device_unregister(se_platform_devs);
        goto fail;
    }
    
        /* 
	 * allocate the devices -- we can't have them static, as the number
	 * can be specified at load time
	 */
	se_devices = (struct se_dev *)kmalloc(se_nr_devs * sizeof(struct se_dev), GFP_KERNEL);
	//se_devices = (struct se_dev *)__get_free_pages(GFP_KERNEL, 3);
	//printk("EJ page address: %p \n", virt_to_page(se_devices));
	if (!se_devices) {
		result = -ENOMEM;
		goto fail;  /* Make this more graceful */
	}
	memset(se_devices, 0, sizeof(struct se_dev));

    /* Initialize each device. */
	init_MUTEX(&se_devices->sem);
	se_setup_cdev(se_devices, 0);

        /* At this point call the init function for any friend device */
	dev = MKDEV(se_major, se_minor + se_nr_devs);

    se_devices->isMars = is_mars_cpu() ? 1 : 0;
    se_devices->isJupiter = is_jupiter_cpu() ? 1 : 0;

    {
		uint32_t dwPhysicalAddress = 0;
        
		se_devices->size = (SE_COMMAND_ENTRIES * sizeof(uint32_t));
        
		//se_devices[0].CmdBuf = kmalloc(se_devices[0].size, GFP_KERNEL);

        dwPhysicalAddress = (uint32_t)__pa(se_devices->CmdBuf);
        
		DBG_PRINT("Command Buffer Address = %x, Physical Address = %x\n", (uint32_t) se_devices->CmdBuf, (uint32_t) dwPhysicalAddress);
        
        //dev->v_to_p_offset = (int32_t) ((uint32_t)dev->CmdBuf - (uint32_t)dwPhysicalAddress);
        //dev->wrptr = 0;
		se_devices->CmdBase = (uint32_t) dwPhysicalAddress;
		se_devices->CmdLimit = (uint32_t) (dwPhysicalAddress + se_devices->size);

        InitSeReg();
        
    }
#ifdef LINUX_2_6_34
	result = request_irq(SE_IRQ, se_irq_handler, /*IRQF_DISABLED|*/IRQF_SAMPLE_RANDOM|IRQF_SHARED, "se", (void *)se_devices);
#else
	result = request_irq(SE_IRQ, se_irq_handler, SA_INTERRUPT|SA_SHIRQ, "se", (void *)se_devices);
#endif
    if(result)
	{
        DBG_PRINT(KERN_INFO "se: can't get assigned irq%i\n", SE_IRQ);
	}
    else
    {
        DBG_PRINT(KERN_INFO "se: request irq ok\n");
    }


	return 0; /* succeed */

  fail:
	se_cleanup_module();
	return result;
}

module_init(se_init_module);
module_exit(se_cleanup_module);
