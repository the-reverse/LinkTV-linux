/*
 * super.c - MTP File System kernel super block handling.
 *
 * Copyright (c) 2011 Ching-Yuh Huang
 *
 */

#include <linux/config.h>
#include <linux/kernel.h> 
#include <linux/module.h>

#include <linux/stddef.h>
#include <linux/init.h>
#include <linux/buffer_head.h>
#include <linux/vfs.h>
#include <linux/ctype.h>
#include <linux/parser.h>
#include <linux/nls.h> 

#include "mtpfs.h"

static DECLARE_MUTEX (mtp_mount_mutex);

static int mtpfs_default_codepage = CONFIG_FAT_DEFAULT_CODEPAGE;
static char mtpfs_default_iocharset[] = "utf8";


void mtpfs_set_inode_info(struct inode *ino, PTPObjectInfo *object)
{

	if (object->ObjectFormat !=PTP_OFC_Association && object->AssociationType != PTP_AT_GenericFolder)
	{
        ino->i_size = object->ObjectCompressedSize;
        ino->i_ctime.tv_sec = object->CaptureDate;
        ino->i_mtime.tv_sec = ino->i_atime.tv_sec = object->ModificationDate;

#ifdef FEW_FILES
	printk("\nShow file info:\nfile name = %s\t",object->Filename);
	printk("storage_id = %d\n",object->StorageID);
	printk("object_format=0x%x\t\tPTP_OFC_EXIF_JPEG=0x%x\n",object->ObjectFormat,PTP_OFC_EXIF_JPEG);
	//printk("protection_status=%d\n",object->protection_status);
	printk("object_compressed_size= %d Bytes = %d kBytes\n",object->ObjectCompressedSize,object->ObjectCompressedSize/1024);
	printk("thumb_format=0x%x\t\tPTP_OFC_JFIF=0x%x\n",object->ThumbFormat,PTP_OFC_JFIF);
	printk("thumb_compressed_size= %d = %d kBytes\n",object->ThumbCompressedSize,object->ThumbCompressedSize/1024);
	printk("thumb_pix_width=%d\t\t",object->ThumbPixWidth);
	printk("thumb_pix_height=%d\n",object->ThumbPixHeight);
	printk("image_pix_width=%d\t\t",object->ImagePixWidth);
	printk("image_pix_height=%d\t",object->ImagePixHeight);
	printk("image_bit_depth=%d\n",object->ImageBitDepth);
	printk("parent_object=%d\t",object->ParentObject);
	printk("association_type=0x%x\t",object->AssociationType);
	printk("association_desc=%d\n",object->AssociationDesc);
	printk("sequence_number=%d\n",object->SequenceNumber);
	printk("ctime=%ld\t",(long)ino->i_ctime.tv_sec);
	printk("mtime=%ld\n",(long)ino->i_mtime.tv_sec);
	printk("sizeof(time_t)=%d\n",sizeof(time_t));
	//time(&(object->capture_date));
	//time(&(object->modification_date));
	//printk("david1013: capture_date=%s\n",ctime(&(object->capture_date)));
	//printk("david1013: modification_date=%s\n",ctime(&(object->modification_date)));
	//printk("david1013: capture_date=%s\n",object->capture_date);
	//printk("david1013: modification_date=%s\n",object->modification_date);
	
    //char    *keywords;
	printk("\n");
#endif	
	}
	else
	{
		PTPFSINO(ino)->type = MTP_INO_TYPE_DIR;
	}
	PTPFSINO(ino)->storage = object->StorageID;
}

//static int ptpfs_statfs(struct super_block *sb, struct statfs *buf)
static int mtpfs_get_statfs(struct super_block *sb)
{
	//printk(KERN_INFO "%s\n",  __FUNCTION__);
      
    LIBMTP_devicestorage_t *storage = NULL;
    LIBMTP_mtpdevice_t *mtp_device = PTPFSSB(sb)->device_info->mtp_device;
    
    if (mtp_device == NULL)
    {
        return -ENODEV;
    }    
    
    storage = mtp_device->storage;
    
    if (storage == NULL)
    {
        return -ENODEV;
    } 
    
    PTPFSSB(sb)->kstatfs_temp.f_type = MTPFS_MAGIC;
    PTPFSSB(sb)->kstatfs_temp.f_bsize = PAGE_CACHE_SIZE;
    PTPFSSB(sb)->kstatfs_temp.f_namelen = 255;

    PTPFSSB(sb)->kstatfs_temp.f_bsize  = 1024;
    PTPFSSB(sb)->kstatfs_temp.f_blocks = 0;
    PTPFSSB(sb)->kstatfs_temp.f_bfree  = 0;


    while(storage)
	{
        PTPFSSB(sb)->kstatfs_temp.f_blocks += storage->MaxCapacity;
	    PTPFSSB(sb)->kstatfs_temp.f_bfree  += storage->FreeSpaceInBytes;  
	    storage = storage->next;      
	}
	        
    PTPFSSB(sb)->kstatfs_temp.f_blocks /= 1024;
    PTPFSSB(sb)->kstatfs_temp.f_bfree  /= 1024;
    PTPFSSB(sb)->kstatfs_temp.f_bavail = PTPFSSB(sb)->kstatfs_temp.f_bfree;
    return 0;
}

// --------------------------------------------------------------------------------------------------
static void force_delete(struct inode *inode)
{
         /*
    * Kill off unused inodes ... iput() will unhash and
    * delete the inode if we set i_nlink to zero.
          */
	if (atomic_read(&inode->i_count) == 1)
		inode->i_nlink = 0;
}

void mtpfs_free_inode_data(struct inode *ino)
{
    int x;
    struct mtpfs_inode_data* mtpfs_data = PTPFSINO(ino);
    switch (mtpfs_data->type)
	{
		case MTP_INO_TYPE_DIR:
		case MTP_INO_TYPE_STGDIR:
		for (x = 0; x < mtpfs_data->data.dircache.num_files; x++)
		{
            kfree(mtpfs_data->data.dircache.file_info[x].filename);
		}
		kfree(mtpfs_data->data.dircache.file_info);
		mtpfs_data->data.dircache.file_info = NULL;
		mtpfs_data->data.dircache.num_files= 0;
		break;
	}
}

struct inode *mtpfs_get_inode(struct super_block *sb, int mode, int dev, int ino)
{
    //printk(KERN_INFO "%s -- %d\n",  __FUNCTION__, ino);
    struct inode * inode = new_inode(sb);

    if (inode)
	{
        inode->i_mode = mode;

        inode->i_uid = current->fsuid;
        inode->i_gid = current->fsgid;

        inode->i_blksize = PAGE_CACHE_SIZE;
        inode->i_blocks = 0;	
        inode->i_size = 0;
        inode->i_rdev = MTP_NODEV;
        inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
        inode->i_version = 1;
        inode->i_mapping->a_ops = &mtpfs_fs_aops;
        if (ino)
		{
            inode->i_ino = ino;
		}
	
		(void *) PTPFSINO(inode) = (struct mtpfs_inode_data *) kmalloc(sizeof(struct mtpfs_inode_data), GFP_KERNEL); 		

        memset(PTPFSINO(inode),0,sizeof(struct mtpfs_inode_data));
        insert_inode_hash(inode);
        switch (mode & S_IFMT)
		{
		case S_IFREG:
			inode->i_fop = &mtpfs_file_operations;
			break;
		case S_IFDIR:
			inode->i_op = &mtpfs_dir_inode_operations;			
			inode->i_fop = &mtpfs_dir_operations;			
			break;
		case S_IFLNK:
			inode->i_op = &page_symlink_inode_operations;
			break;
		default:
			init_special_inode(inode, mode, dev);
			break;
        }
    }
    return inode;
}

static void mtpfs_put_inode(struct inode *ino)
{
//    printk(KERN_INFO "%s - %ld   count: %d\n",  __FUNCTION__,ino->i_ino,ino->i_count);

    mtpfs_free_inode_data(ino);
    force_delete(ino);
}

static void mtpfs_put_super (struct super_block * sb)
{
    //printk(KERN_INFO "%s\n",  __FUNCTION__);
    printk("<mtp module> umount ptp device ST\n");


	kfree(PTPFSSB(sb));
	(void *) PTPFSSB(sb) = NULL; 
	 		
    printk("<mtp module> umount ptp device SP\n");

}

static int mtpfs_statfs(struct super_block *sb, struct kstatfs *buf)
{
    // DBG("mtpfs_statfs");
    
    buf->f_type = MTPFS_MAGIC;
    buf->f_bsize = PAGE_CACHE_SIZE;
    buf->f_namelen = 255;
    buf->f_bsize = 1024;
    buf->f_blocks = PTPFSSB(sb)->kstatfs_temp.f_blocks;
    buf->f_bfree  = PTPFSSB(sb)->kstatfs_temp.f_bfree;
    buf->f_bavail = PTPFSSB(sb)->kstatfs_temp.f_bavail;
    
    return 0;
}


struct super_operations mtpfs_ops = {
	statfs:		mtpfs_statfs,
	put_super:		mtpfs_put_super,
	put_inode:		mtpfs_put_inode,
	drop_inode:	generic_delete_inode,
};

// --------------------------------------------------------------------------------------------------
/*
static char * ___strtok;

static char * strtok(char * s,const char * ct)
{
        char *sbegin, *send;

        sbegin  = s ? s : ___strtok;
       if (!sbegin) {
            return NULL;
        }
        sbegin += strspn(sbegin,ct);
        if (*sbegin == '\0') {
            ___strtok = NULL;
            return( NULL );
        }
        send = strpbrk( sbegin, ct);
        if (send && *send != '\0')
            *send++ = '\0';
        ___strtok = send;
        return (sbegin);
}


static int mtpfs_parse_options(char *options, uid_t *uid, gid_t *gid)
{
	char *this_char, *value, *rest;

	this_char = NULL;
	
	if (options)
		this_char = strtok(options,",");
		
	for ( ; this_char; this_char = strtok(NULL,","))
	{
		if ((value = strchr(this_char,'=')) != NULL)		
			*value++ = 0;		
		else
		{
			printk(KERN_ERR 
			"mtpfs: No value for mount option '%s'\n",this_char);
			return 1;
		}

		if (!strcmp(this_char,"uid"))
		{
			if (!uid)
				continue;
			*uid = simple_strtoul(value,&rest,0);
			if (*rest)
				goto bad_val;
		}
		else if (!strcmp(this_char,"gid"))
		{
			if (!gid)
				continue;
			*gid = simple_strtoul(value,&rest,0);
			if (*rest)
				goto bad_val;
		}
		else
		{
			printk(KERN_ERR "mtpfs: Bad mount option %s\n",this_char);
			return 1;
		}
	}
	return 0;

	bad_val:
	printk(KERN_ERR "mtpfsf: Bad value '%s' for mount option '%s'\n",value, this_char);
	return 1;
}
*/

enum {
	Opt_uid, Opt_gid,
	// Opt_umask, Opt_dmask, Opt_fmask, 
	Opt_codepage, Opt_charset, 
	Opt_utf8_no, Opt_utf8_yes,
	Opt_uni_xl_no, Opt_uni_xl_yes, 	
	Opt_err,
};

static match_table_t mtpfs_tokens = {
	{Opt_uid, "uid=%u"},
	{Opt_gid, "gid=%u"},
/*	
	{Opt_umask, "umask=%o"},
	{Opt_dmask, "dmask=%o"},
	{Opt_fmask, "fmask=%o"},
*/	
    {Opt_charset, "iocharset=%s"},
	{Opt_codepage, "codepage=%u"},
	{Opt_utf8_no, "utf8=0"},		/* 0 or no or false */
	{Opt_utf8_no, "utf8=no"},
	{Opt_utf8_no, "utf8=false"},
	{Opt_utf8_yes, "utf8=1"},		/* empty or 1 or yes or true */
	{Opt_utf8_yes, "utf8=yes"},
	{Opt_utf8_yes, "utf8=true"},
	{Opt_utf8_yes, "utf8"},
	{Opt_err, NULL}
};

static int mtpfs_parse_options(char *options, struct mtpfs_mount_options *opts)
{
    char *p;
    char *iocharset;
    int option;
    substring_t args[MAX_OPT_ARGS];
    
    opts->fs_uid = current->uid;
	opts->fs_gid = current->gid;
	// opts->fs_fmask = opts->fs_dmask = current->fs->umask;
    opts->codepage = mtpfs_default_codepage;
	opts->iocharset = mtpfs_default_iocharset; 	
	opts->utf8 = 1;
	opts->unicode_xlate = 0; 
    
    if (!options)
        return 0;
                      
    while ((p = strsep(&options, ",")) != NULL) 
    {
		int token;
		if (!*p)
			continue;

		token = match_token(p, mtpfs_tokens, args);
		
		switch (token) 
		{
		    case Opt_uid:
			    if (match_int(&args[0], &option))
				    return 0;
			    opts->fs_uid = option;
			break;
		    case Opt_gid:
			    if (match_int(&args[0], &option))
				    return 0;
			    opts->fs_gid = option;
			break;
						
			case Opt_utf8_no:		// 0 or no or false 
			    opts->utf8 = 0;
			break;
		    case Opt_utf8_yes:		// empty or 1 or yes or true 
			    opts->utf8 = 1;
			break;
			
			case Opt_uni_xl_no:		/* 0 or no or false */
			    opts->unicode_xlate = 0;
			break;
			
		    case Opt_uni_xl_yes:		/* empty or 1 or yes or true */
			    opts->unicode_xlate = 1;
			break;
			
			case Opt_charset:
			    if (opts->iocharset != mtpfs_default_iocharset)
				    kfree(opts->iocharset);
			    iocharset = match_strdup(&args[0]);
			    if (!iocharset)
				    return -ENOMEM;
			    opts->iocharset = iocharset;
			break;
			
			case Opt_codepage:
			    if (match_int(&args[0], &option))
				    return 0;
			    opts->codepage = option;
			break;
						
			/* unknown option */
		    default:
			    printk(KERN_ERR "FAT: Unrecognized mount option \"%s\" "
			       "or missing value\n", p);
			    return -EINVAL;
			break;
	    }
    } // End of while	
    
    return 0;            
}

static int mtpfs_fill_super(struct super_block *sb, void *data, int silent)
{
    struct inode * inode;
    struct dentry * root;
    int mode   = S_IRWXUGO | S_ISVTX | S_IFDIR; 
    char buf[50];

    sb->s_blocksize = PAGE_CACHE_SIZE;
    sb->s_blocksize_bits = PAGE_CACHE_SHIFT;
    sb->s_magic = MTPFS_MAGIC;
    sb->s_flags |= MS_RDONLY;
    sb->s_op = &mtpfs_ops;

	(void *) PTPFSSB(sb) = kmalloc(sizeof(struct mtpfs_sb_info), GFP_KERNEL); 
    memset(PTPFSSB(sb), 0, sizeof(struct mtpfs_sb_info));
    
    PTPFSSB(sb)->device_info = kmalloc(sizeof(struct mtpfs_device_info), GFP_KERNEL); 
    memset(PTPFSSB(sb)->device_info, 0, sizeof(struct mtpfs_device_info));
    
    PTPFSSB(sb)->byteorder = PTP_DL_LE;
    
    if (mtpfs_parse_options (data,&PTPFSSB(sb)->options))
		return 1;
		
	if (PTPFSSB(sb)->options.iocharset)	
	    PTPFSSB(sb)->nls_io = load_nls(PTPFSSB(sb)->options.iocharset);	
        
    sprintf(buf, "cp%d", PTPFSSB(sb)->options.codepage);
	PTPFSSB(sb)->nls_codepage = load_nls(buf);
    
    PTPFSSB(sb)->fs_gid = PTPFSSB(sb)->options.fs_gid;
    PTPFSSB(sb)->fs_uid = PTPFSSB(sb)->options.fs_uid;
            
    inode = mtpfs_get_inode(sb, S_IFDIR | mode, 0, 0);
    if (!inode) goto error;

    inode->i_op = &mtpfs_root_dir_inode_operations;
    inode->i_fop = &mtpfs_root_file_operations;

    inode->i_uid = PTPFSSB(sb)->options.fs_uid;
    inode->i_gid = PTPFSSB(sb)->options.fs_gid;
    root = d_alloc_root(inode);
    
    if (!root)
	{
        iput(inode);
        goto error;
	}
	
    sb->s_root = root;	    
	
	//	fs/super.c  =>error return minus number, ex: -1~-34
    printk("<mtp module> mount mtp device SP and OK\n");
    #ifdef DAVID_DEBUG
    printk("<mtp module> mount ptp device SP and OK time=%lu\n",get_seconds());
    printk("<mtp module> manufacturer=%s\n",PTPFSSB(sb)->deviceinfo->manufacturer);
    printk("<mtp module> model=%s\n",PTPFSSB(sb)->deviceinfo->model);
    printk("<mtp module> device_version=%s\n",PTPFSSB(sb)->deviceinfo->device_version);
    printk("<mtp module> serial_number=%s\n\n\n",PTPFSSB(sb)->deviceinfo->serial_number);
    #endif
    return 0;			

	// suspend.. 	change mount type
error:
    /*
    down(&PTPFSSB(sb)->usb_device->sem);
    PTPFSSB(sb)->usb_device->open_count--;
    up(&PTPFSSB(sb)->usb_device->sem);
    up(&ptp_devices_mutex);
    */
    printk("<ptp module> mount ptp device SP and fails\n");
    return -ENXIO;

}
static struct super_block *mtpfs_get_sb(struct file_system_type *fst, int flags, const char *dev_name, void *data)
{
    int mtp_nr = 0;
    struct super_block *get_sb_nodev_temp;
    LIBMTP_mtpdevice_t *mtp_device = NULL;
    
    if (!dev_name)
        return ERR_PTR(-EINVAL);
                   
    down(&mtp_mount_mutex);
                                 
    if (dev_name[0] == 'm' && dev_name[1] == 't' && dev_name[2] == 'p') 
    {
        if (isdigit(dev_name[3])) 
        {
            char *endptr;

            mtp_nr = simple_strtoul(dev_name+3, &endptr, 0);
            
            if (!*endptr) 
            {
                mtp_device = Mtp_Get_Device(NULL,mtp_nr);                    
            }                
        }          
    }  
    else
    {
        mtp_device = Mtp_Get_Device_By_Kobj_Name((char *) dev_name);    
    }                   	
		
	if (mtp_device == NULL)
	{
	    up(&mtp_mount_mutex); 	    
	    return ERR_PTR(-ENODEV);
	}   
	
    // printk("dev_name = %s , mtp_device->index = %d , mtp_device = 0x%x\n",dev_name,mtp_device->index,mtp_device);
	
	// Open Session	
    if (Mtp_Usb_Device_Open(mtp_device) != LIBMTP_ERROR_NONE)
    {
        up(&mtp_mount_mutex); 
        Mtp_Put_Device(mtp_device);         
        return ERR_PTR(-ENODEV);    
    }     
    
	get_sb_nodev_temp = get_sb_nodev(fst, flags, data, mtpfs_fill_super);	
	
	if (get_sb_nodev_temp)	
	{    	
	    PTPFSSB(get_sb_nodev_temp)->device_info->mtp_device = mtp_device; 
	    mtpfs_get_statfs(get_sb_nodev_temp);
	}
			
	up(&mtp_mount_mutex);		
	return get_sb_nodev_temp;
}

static void mtpfs_kill_sb(struct super_block *sb)
{
    LIBMTP_mtpdevice_t *mtp_device = PTPFSSB(sb)->device_info->mtp_device;	        
    
	if (mtp_device)
	{    	
	    Mtp_Put_Device(mtp_device);
	}    
			 	    
	if (PTPFSSB(sb)->device_info)
	    kfree(PTPFSSB(sb)->device_info);
	kill_anon_super(sb);	
}

static struct file_system_type mtpfs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "ptpfs",
	.get_sb		= mtpfs_get_sb,
	.kill_sb	= mtpfs_kill_sb,
};

static int __init init_mtpfs_fs(void)
{
    int result;
    
    result = register_filesystem(&mtpfs_fs_type);
	if (result < 0)
	{
		printk("register_filesystem failed for the mtpfs driver. Error number %d\n", result);
		return -1;
	}
	printk("<mtp module> insert mtpfs module OK\n");

	return 0;
}

static void __exit exit_mtpfs_fs(void)
{
    unregister_filesystem(&mtpfs_fs_type);
}

MODULE_AUTHOR("Ching-Yuh Huang <cyhuang@realtek.com>");
MODULE_DESCRIPTION("MTP File System 1.0 driver - Copyright (c) 2011 Ching-Yuh Huang");
// MODULE_VERSION(MTPFS_VERSION);
// MODULE_LICENSE("GPL");

module_init(init_mtpfs_fs);
module_exit(exit_mtpfs_fs);


