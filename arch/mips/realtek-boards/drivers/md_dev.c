#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/io.h>                  
#include <asm/page.h>                  
#include <asm/mach-venus/md.h>
#include <platform.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <linux/devfs_fs_kernel.h>

#define MD_DEV_FILE_NAME       "md"
static struct cdev md_cdev;
static dev_t devno;


typedef struct {
    MD_COPY_HANDLE      handle;     // handle
    unsigned char*      p_desc;     // descriptor [MAX: 16 Words]
    unsigned long       len;        // length of desc in word
}MD_COMMAND;

#define MD_IOCTL_WRITE_COMMAND     0x12831000
#define MD_IOCTL_CHECK_FINISH      0x12831001
#define MD_IOCTL_WAIT_COMPLETE     0x12831002


/*------------------------------------------------------------------
 * Func : md_dev_open
 *
 * Desc : open function of md dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int md_dev_open(struct inode *inode, struct file *file)
{       
    return 0;    
}



/*------------------------------------------------------------------
 * Func : mcp_dev_release
 *
 * Desc : release function of mcp dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int md_dev_release(
    struct inode*           inode, 
    struct file*            file
    )
{               
	return 0;
}




/*------------------------------------------------------------------
 * Func : md_dev_ioctl
 *
 * Desc : ioctl function of md dev
 *
 * Parm : inode : inode of dev
 *        file  : context of file
 *        cmd   : control command
 *        arg   : arguments
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static 
int md_dev_ioctl(
    struct inode*           inode, 
    struct file*            file,
    unsigned int            cmd, 
    unsigned long           arg
    )
{                      
    MD_COPY_HANDLE  handle;
    MD_COMMAND      md_cmd;    
    unsigned char*  p_desc;
    unsigned char   desc[64];

    //printk("cmd=%08x, arg=%08x\n", cmd, arg);
    
    switch (cmd)
    {
    case MD_IOCTL_WRITE_COMMAND:
        
        if (copy_from_user((void *) &md_cmd, (void __user *)arg, sizeof(MD_COMMAND)))
        {            
            printk("[MD] WARNING, write md command failed, copy md command from user space failed\n");
			return -EFAULT;
        }
        
        if (md_cmd.len > sizeof(desc))
        {
            if ((p_desc = kmalloc(md_cmd.len,GFP_KERNEL))==NULL)
            {
                printk("[MD] WARNING, write md command failed, alloc desc buffer failed (%lu)\n", md_cmd.len);
			    return -EFAULT;
            }

            if (copy_from_user((void *) p_desc, (void __user *)md_cmd.p_desc, md_cmd.len))
            {               
                printk("[MD] WARNING, write md command failed, copy md desc from user space failed\n");
                kfree(p_desc);
			    return -EFAULT;
            }
            
            md_cmd.handle = (MD_CMD_HANDLE) md_write(p_desc, md_cmd.len);
            
            kfree(p_desc);
        }
        else
        {
            if (copy_from_user((void *) desc, (void __user *)md_cmd.p_desc, md_cmd.len))
            {               
                printk("[MD] WARNING, write md command failed, copy md desc from user space failed\n");
			    return -EFAULT;
            }
            
            md_cmd.handle = (MD_CMD_HANDLE) md_write(desc, md_cmd.len);
        }

        if (copy_to_user((void __user *)arg, (void *) &md_cmd, sizeof(MD_COMMAND)))
        {            
            printk("[MD] WARNING, write md command failed, copy result to user space failed\n");
			return -EFAULT;
        }        
                         
        return 0;        
                
    case MD_IOCTL_CHECK_FINISH:
        
        if (copy_from_user((void *) &handle, (void __user *)arg, sizeof(MD_CMD_HANDLE)))
        {            
            printk("[MD] WARNING, check finish failed, copy md handle from user space failed\n");
			return -EFAULT;
        }
			
        return md_checkfinish(handle);        
        
    case MD_IOCTL_WAIT_COMPLETE:
        
        if (copy_from_user((void *) &handle, (void __user *)arg, sizeof(MD_CMD_HANDLE)))
        {            
            printk("[MD] WARNING, check finish failed, copy md handle from user space failed\n");
			return -EFAULT;
        }
			
        return md_copy_sync(handle);        
        
                                                
    default:
	    
	printk("[MD] WARNING, unknown command %08x\n", cmd);                
        return -EFAULT;          
    }	
	
    return 0;          
}


static struct file_operations md_ops = 
{
	.owner		= THIS_MODULE,	
	.ioctl		= md_dev_ioctl,
	.open		= md_dev_open,
	.release	= md_dev_release,
};




//////////////////////////////////////////////////////////////////////////////////////////////
// Platform Device Interface
//////////////////////////////////////////////////////////////////////////////////////////////




/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/

static int __init md_dev_init(void)
{
    cdev_init(&md_cdev, &md_ops);            

    if (alloc_chrdev_region(&devno, 0, 1, MD_DEV_FILE_NAME)!=0)    
    {
        cdev_del(&md_cdev);
        return -EFAULT;
    }                                 

    if (cdev_add(&md_cdev, devno, 1)<0)
        return -EFAULT;                          

    devfs_mk_cdev(devno, S_IFCHR|S_IRUSR|S_IWUSR, MD_DEV_FILE_NAME);         
    
    return 0;	
}


static void __exit md_dev_exit(void)
{
    cdev_del(&md_cdev);
    devfs_remove(MD_DEV_FILE_NAME);
    unregister_chrdev_region(devno, 1);
}


module_init(md_dev_init);
module_exit(md_dev_exit);
