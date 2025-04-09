#ifndef _MTPFS_H_
#define _MTPFS_H_

#include <linux/types.h>
#include <linux/statfs.h>
#include <linux/mtp/libmtp.h>
#include <linux/mtp/ptp.h>
#include <linux/mtp/usb.h>

#define MTPFS_MAGIC		0x244D5450 // $MTP

#define MTP_NODEV               0

#define MTP_INO_TYPE_DIR        1
#define MTP_INO_TYPE_STGDIR     2

#if defined(CONFIG_MTPFS_FS_READ_AHEAD)
#define MTPFS_MAX_READ_AHEAD_SIZE           (MTP_USB_MAX_TRANSFER_SIZE)
#endif

#define MTPFS_SESSION_MAX_ERR_RETRY            (1) 

enum 
{
    MTPFS_PAGE_READ_FLAG = 0,
    MTPFS_DIRECT_IO_FLAG = 1
};

struct mtpfs_sb_info;

#define PTPFSSB(x) ((struct mtpfs_sb_info *)(x->s_fs_info))
#define PTPFSINO(x) ((struct mtpfs_inode_data *)(x->u.generic_ip))

struct mtpfs_file_buffer
{
	unsigned long i_ino;
    loff_t offset;
    unsigned long size;
    unsigned char data[0];
};

struct mtpfs_mount_options 
{
    uid_t fs_uid;
    gid_t fs_gid;    
    unsigned short fs_fmask;
    unsigned short fs_dmask;
    unsigned short codepage;  /* Codepage for shortname conversions */
    char *iocharset;          /* Charset used for filename input/display */
    unsigned char utf8:1,          /* Use of UTF8 character set (Default) */   
                  unicode_xlate:1; /* create escape sequences for unhandled Unicode */ 
};    

struct mtpfs_device_info
{
    /* stucture lock */
    struct semaphore sem;

    /* users using this block */
    int open_count;

    /*  the mtp device */
    LIBMTP_mtpdevice_t *mtp_device;
    __u8    minor;

    /*  store kobject_name to check which device will be mounted (fill_super) */
    char *kobj_name;

};

struct mtpfs_sb_info
{
    /* ptp transaction ID */
    __u32 transaction_id;
    /* ptp session ID */
    __u32 session_id;

    uid_t fs_uid;
    gid_t fs_gid;

    /* data layer byteorder */
    __u8  byteorder;

    struct mtpfs_mount_options options;
    struct nls_table *nls_codepage;
    struct nls_table *nls_io;
    struct mtpfs_device_info *device_info;

    // unsigned char *buffer;      // store the last page data. If offset is the same, we can use it directly.
    struct kstatfs kstatfs_temp;
};


struct mtpfs_dirinode_fileinfo
{
    char *filename;
    int handle;
    int mode;
};


struct mtpfs_inode_data
{
    int type;
    __u32 storage;
    struct inode *parent;

    union
    {
        struct
        {
            int num_files;
            struct mtpfs_dirinode_fileinfo *file_info;
        } dircache;
    } data;
};

static inline bool mtp_is_mtp_device(PTPParams *params)
{
    if (params->deviceinfo.VendorExtensionID != 0x00000006)
        return false;
    else
        return true;    
}

// root.c
extern struct inode_operations mtpfs_root_dir_inode_operations;
extern struct file_operations mtpfs_root_file_operations;

// inode.c
extern struct address_space_operations mtpfs_fs_aops ;
extern struct file_operations mtpfs_dir_operations;
extern struct file_operations mtpfs_file_operations;
extern struct inode_operations mtpfs_dir_inode_operations;

// super.c
extern struct inode *mtpfs_get_inode(struct super_block *sb, int mode, int dev, int ino);
extern void mtpfs_free_inode_data(struct inode *ino);
extern void mtpfs_set_inode_info(struct inode *ino, PTPObjectInfo *object);
#endif // _MTPFS_H_
