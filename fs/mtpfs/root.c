/*
 * ptpfs filesystem for Linux.
 *
 * This file is released under the GPL.
 */


#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/errno.h>
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/fcntl.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/smp_lock.h>
#include <linux/string.h>
//#include <linux/locks.h>
#include <asm/uaccess.h>
#include <asm-mips/types.h>

#include <linux/mtp/ptp.h>
#include "mtpfs.h"

/*
 * Root of the PTPFS is a read/only directory which 
 * holds directories for each of the storage units.
 *
 * Eventually, we may add a "Albums" or similar sub
 * directory for the album support.
 * 
 * We could also add readonly files like "Camera.txt"
 * for querying camera information and settings
 */
#ifndef HAVE_ARCH_PAGE_BUG
#define PAGE_BUG(page) do { \
        printk("page BUG for page at %p\n", page); \
        BUG(); \
} while (0)
#endif

#define PASSPORT_FREE 0xff
//extern DECLARE_MUTEX (ptp_passport_mutex);
// extern ptp_passport_mutex;
// extern unsigned char passport;

static void get_root_dir_name(LIBMTP_devicestorage_t *storage, char *fsname, int* typeCount) 
{
    int count = 0;
    const char *base;
    switch (storage->StorageType)
    {
    case PTP_ST_FixedROM:
        count = typeCount[0];
        typeCount[0]++;
        base = "Internal_ROM";
        break;
    case PTP_ST_RemovableROM:
        count = typeCount[1];
        typeCount[1]++;
        base = "Removable_ROM";
        break;
    case PTP_ST_FixedRAM:
        count = typeCount[2];
        typeCount[2]++;
        base = "InternalMemory";
        break;
    case PTP_ST_RemovableRAM:
        count = typeCount[3];
        typeCount[3]++;
        base = "MemoryCard";
        break;
    default:
        count = typeCount[4];
        typeCount[4]++;
        base = "Unknown";
        break;
    }
    if (count)
    {
        sprintf(fsname,"%s_%d",base,count);
    }
    else
    {
        sprintf(fsname,base);
    }
}
static int mtpfs_root_readdir(struct file *filp, void *dirent, filldir_t filldir)
{
	// printk(KERN_INFO "%s\n",  __FUNCTION__);

    struct inode *inode = filp->f_dentry->d_inode;
    struct dentry *dentry = filp->f_dentry;    
    int ino;   
    char fsname[256];
    int typeCount[5];
    memset(typeCount,0,sizeof(typeCount));	

    int offset = filp->f_pos;
    
	switch (offset)
	{
		case 0:
			if (filldir(dirent, ".", 1, offset, inode->i_ino, DT_DIR) < 0)
			{
				return 0;
			}
			offset++;
			filp->f_pos++;
			// fall through 
		break;	
		case 1:
			spin_lock(&dcache_lock);
			ino = dentry->d_parent->d_inode->i_ino;
			spin_unlock(&dcache_lock);
			if (filldir(dirent, "..", 2, offset, ino, DT_DIR) < 0)
				break;
			filp->f_pos++;
			offset++;
		break;		
		default:
		{    
		    LIBMTP_mtpdevice_t *mtp_device = PTPFSSB(inode->i_sb)->device_info->mtp_device;
		    LIBMTP_devicestorage_t *storage = NULL;
		    
		    storage = mtp_device->storage;
		    
		    // printk("mtp_device->num_storage = %d\n",mtp_device->num_storage);
		    
		    if (offset > (mtp_device->num_storage + 1))
		    {
		        return 1;    
		    }    
		    
		    while(storage)
	        {                
	            get_root_dir_name(storage, fsname, typeCount) ;
	            
	            // printk("fsname = %s , storage = 0x%x\n",fsname,storage);
	            
	            ino = storage->id;
	            if (filldir(dirent, fsname, strlen(fsname), filp->f_pos, ino ,DT_DIR) < 0)
				{						
				    return 0;
				}								
				
				filp->f_pos++;
	            storage = storage->next;      
	        }		    
			return 1;
		
	    }
	    break;
	} // End of switch
	return 0;
}
/*
 * Lookup the data. This is trivial - if the dentry didn't already
 * exist, we know it is negative.
 */
static struct dentry * mtpfs_root_lookup(struct inode *dir, struct dentry *dentry,struct nameidata *nidata)
{
    // printk(KERN_INFO "%s\n",  __FUNCTION__);

    char fsname[256];
    int typeCount[5];
        
    int offset = 0;
    LIBMTP_mtpdevice_t *mtp_device = PTPFSSB(dir->i_sb)->device_info->mtp_device;
    LIBMTP_devicestorage_t *storage = NULL;
    
	memset(typeCount,0,sizeof(typeCount));	    
    storage = mtp_device->storage;
		    
	while(storage)
	{                
        get_root_dir_name(storage, fsname, typeCount) ;
	    offset++;
	    if (!memcmp(fsname,dentry->d_name.name,dentry->d_name.len))
        {
            struct inode *newi = mtpfs_get_inode(dir->i_sb, S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH , 0,storage->id);
            PTPFSINO(newi)->parent = dir; //dir should be root directory
            PTPFSINO(newi)->type = MTP_INO_TYPE_STGDIR;
            PTPFSINO(newi)->storage = storage->id;
            
            if (newi) 
                d_add(dentry, newi);
            return NULL;
        }
	    storage = storage->next;      
	}

    return NULL;

}

static int mtpfs_root_setattr(struct dentry *d, struct iattr * a)
{
    return -EPERM;
}


struct inode_operations mtpfs_root_dir_inode_operations = {
    lookup:     mtpfs_root_lookup,
    setattr:    mtpfs_root_setattr,
};
struct file_operations mtpfs_root_file_operations = {
    read:       generic_read_dir,  
    readdir:    mtpfs_root_readdir,
};


