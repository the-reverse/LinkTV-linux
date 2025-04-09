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
#include <linux/ctype.h>
#include <linux/vmalloc.h>
#include <asm/uaccess.h>
#include <asm-mips/types.h>
//=======
#include <linux/seq_file.h>  
#include <linux/nls.h> 
#include <asm/unaligned.h> 
//=======
#include <linux/uio.h>
#include <linux/pagemap.h>
#include <linux/mtp/ptp.h>          
#include "mtpfs.h"
#include "mtp.h"

#ifndef HAVE_ARCH_PAGE_BUG
    #define PAGE_BUG(page) do { \
    printk("page BUG for page at %p\n", page); \
    BUG(); \
    } while (0)
#endif 

/*
 * Convert Unicode 16 to UTF8, translated Unicode, or ASCII.
 * If uni_xlate is enabled and we can't get a 1:1 conversion, use a
 * colon as an escape character since it is normally invalid on the vfat
 * filesystem. The following four characters are the hexadecimal digits
 * of Unicode value. This lets us do a full dump and restore of Unicode
 * filenames. We could get into some trouble with long Unicode names,
 * but ignore that right now.
 * Ahem... Stack smashing in ring 0 isn't fun. Fixed.
 */
static int uni16_to_x8(unsigned char *ascii, wchar_t *uni, int uni_xlate,
		       struct nls_table *nls)
{
	wchar_t *ip, ec;
	unsigned char *op, nc;
	int charlen;
	int k;

	ip = uni;
	op = ascii;

	while (*ip) {
		ec = *ip++;
		if ( (charlen = nls->uni2char(ec, op, NLS_MAX_CHARSET_SIZE)) > 0) {
			op += charlen;
		} else {
			if (uni_xlate == 1) {
				*op = ':';
				for (k = 4; k > 0; k--) {
					nc = ec & 0xF;
					op[k] = nc > 9	? nc + ('a' - 10)
							: nc + '0';
					ec >>= 4;
				}
				op += 5;
			} else {
				*op++ = '?';
			}
		}
		/* We have some slack there, so it's OK */
		if (op>ascii+256) {
			op = ascii + 256;
			break;
		}
	}
	*op = 0;
	return (op - ascii);
}


static bool inline mtpfs_is_photo_ext_name(const unsigned char *filename,int l)
{
    char buf[4];
    unsigned char *fname;
    unsigned char *photo_ext_name_table[] = {"jpeg","jpe","jpg","tiff","tif","bmp","png","gif","jps","mpo",NULL};
    unsigned char *photo_ext_name;
    int x;
       
    while (l)
    {
        l--;
        if (filename[l] == '.')
        {
            fname = (unsigned char *) &filename[l];
            l = 0;
            
            // printk("fname = %s\n",fname);
            
            if (strlen(fname) > 5 || strlen(fname) == 1)
            {
                return false;
            }
            fname++;
            for (x = 0; x < strlen(fname); x++)
            {
                buf[x] = __tolower(fname[x]);
            }
            buf[strlen(fname)] = 0;
                                    
            x = 0;            
            photo_ext_name = (unsigned char *) &photo_ext_name_table[x][0];
            
            while (photo_ext_name)
            {
                // printk("photo_ext_name = %s\n",photo_ext_name);
                if (!strcmp(buf, photo_ext_name))
                {
                    // printk("Get Filter Ext Name = %s , filename = %s\n",photo_ext_name,filename);
                    return true;
                } 
                
                x++;                
                photo_ext_name = (unsigned char *) &photo_ext_name_table[x][0];
            }
        }
    }  
    
    return false;  
}
                            
static int mtpfs_get_dir_data(struct inode *inode)
{
    int x;
    struct mtpfs_inode_data *mtpfs_data = PTPFSINO(inode);
    struct mtpfs_sb_info *mtpfs_sb = PTPFSSB(inode->i_sb);
    int uni_xlate = mtpfs_sb->options.unicode_xlate;
	int utf8 = mtpfs_sb->options.utf8;
	struct nls_table *nls_codepage = mtpfs_sb->nls_codepage;

    // printk(KERN_INFO "===== %s ===== ino : %ld \n",  __FUNCTION__, inode->i_ino);

    if (mtpfs_data->data.dircache.file_info == NULL)
    {
        PTPObjectHandles objects;
        objects.n = 0;
        objects.Handler = NULL;        

        mtpfs_data->data.dircache.num_files = 0;
        mtpfs_data->data.dircache.file_info = NULL;
                
        if (mtpfs_data->type == MTP_INO_TYPE_DIR)
        {
            if (mtpfs_ptp_getobjecthandles(PTPFSSB(inode->i_sb), mtpfs_data->storage, 0x000000, inode->i_ino, &objects) != PTP_RC_OK)
            {
                return 0;
            } 
        }
        else
        {
            if (mtpfs_ptp_getobjecthandles(PTPFSSB(inode->i_sb), inode->i_ino, 0x000000, 0xffffffff, &objects) != PTP_RC_OK)
            {
                return 0;
            }
        }
       
        int size = objects.n * sizeof(struct mtpfs_dirinode_fileinfo);
        struct mtpfs_dirinode_fileinfo *finfo = (struct mtpfs_dirinode_fileinfo *) kmalloc(size, GFP_KERNEL);
        mtpfs_data->data.dircache.file_info = finfo;
        
        if (finfo == NULL)
        {
            return 0;
        } 
        
        memset(mtpfs_data->data.dircache.file_info, 0, size);
        // printk("\n<mtp module> %s do mtpfs_ptp_getobjectinfo %d times inode=0x%p\n", __func__, objects.n, inode);
        
        for (x = 0; x < objects.n; x++)
        {
            uint16_t ret;
            
            PTPObjectInfo object;
            memset(&object, 0, sizeof(object));
            char *new_fname = NULL;
           
            /* 
            if ((x % 50) == 0)
                printk("<mtp module> %s do mtpfs_ptp_getobjectinfo %d times inode=0x%p x=%d\n", __func__, objects.n, inode, x);
            */    
            
            ret = mtpfs_ptp_getobjectinfo(PTPFSSB(inode->i_sb), objects.Handler[x], &object);
                            
            if (ret != PTP_RC_OK)
            {                                    
                if (ret == PTP_RC_AccessDenied)
                {
                    mtpfs_ptp_free_object_info(&object);
                    continue;    
                } 
                
                mtpfs_free_inode_data(inode);                               
                mtpfs_ptp_free_object_handles(&objects);                                                   
                return 0;
            } 

            if (mtpfs_data->type == MTP_INO_TYPE_STGDIR && (object.StorageID != inode->i_ino || object.ParentObject != 0))
            {
                continue;
            }            
            
            int mode = DT_REG;                        
                                        
            // +++ cyhuang (2011/06/08)             
            /* 
             * Fix bug for Creative ZEN Vision:M (idVendor = 0x041e, idProduct = 0x413e).
             * Creative ZEN Vision:M only use ObjectFormat = PTP_OFC_Association to be mark as directory.
             */   
            if (object.ObjectFormat == PTP_OFC_Association && ((object.AssociationType == PTP_AT_GenericFolder) || (object.AssociationType == PTP_AT_Undefined)))
            {    
                mode = DT_DIR;    
            }    
            // +++ cyhuang (2011/06/08)
            
            // Max length = (PTP_MAXSTRLEN * 3 + 1)            
            new_fname = kmalloc(object.Filename_len + 2,GFP_KERNEL);
            
            if (new_fname == NULL)
            {    
                mtpfs_free_inode_data(inode);  
                mtpfs_ptp_free_object_handles(&objects); 
                mtpfs_ptp_free_object_info(&object);
                return 0;
            }                               
                        
            memset(new_fname,0,object.Filename_len + 2);     
                                   
            if (utf8)  
            {               
                utf8_wcstombs(new_fname, (const wchar_t *) object.Filename, object.Filename_len);                          
            }
            else
            {
                uni16_to_x8(new_fname, (wchar_t *) object.Filename, uni_xlate, nls_codepage);
            }                          
            
            // PTP Device only allow photo file.           
            // if ((mtpfs_is_mtp_device(PTPFSSB(inode->i_sb)) == false) && (mode != DT_DIR))
            if (((mtpfs_ptp_operation_issupported(PTPFSSB(inode->i_sb),PTP_OC_GetPartialObject) == false) || (mtpfs_is_high_speed_device(PTPFSSB(inode->i_sb)) == false)) && (mode != DT_DIR))
            {                              
                if (mtpfs_is_photo_ext_name(new_fname,object.Filename_len) == false)
                {            
                    mtpfs_ptp_free_object_info(&object);
                    continue;
                }
            }   
            
                                                               
            finfo[mtpfs_data->data.dircache.num_files].filename = new_fname;             
            finfo[mtpfs_data->data.dircache.num_files].handle = objects.Handler[x];
            finfo[mtpfs_data->data.dircache.num_files].mode = mode;
            mtpfs_data->data.dircache.num_files++;
            // object.Filename = NULL;  
              
            mtpfs_ptp_free_object_info(&object);
        }
        // printk("<mtp module> %s do mtpfs_ptp_getobjectinfo %d times inode=0x%p end\n", __func__, objects.n, inode);
        mtpfs_ptp_free_object_handles(&objects);        
    }
    
    return 1;
}


static int mtpfs_readdir(struct file *filp, void *dirent, filldir_t filldir)
{

    // printk(KERN_INFO "===== %s ===== \n",  __FUNCTION__);  
    struct inode *inode = filp->f_dentry->d_inode;
    struct dentry *dentry = filp->f_dentry;
    struct mtpfs_inode_data *mtpfs_data = PTPFSINO(inode);
    int offset = filp->f_pos;
    int offset2 = filp->f_pos;
    int file_numbers;
    ino_t ino;
    int x;
    
    filp->f_version = inode->i_version;

    switch (offset)
    {
        case 0:
            if (filldir(dirent, ".", 1, offset, inode->i_ino, DT_DIR) < 0)
            //add "." and ".." to dirent
            {
                break;
            }
            offset++;
            filp->f_pos++;
            // fall through 
        case 1:
            spin_lock(&dcache_lock);
            ino = dentry->d_parent->d_inode->i_ino;
            spin_unlock(&dcache_lock); //same as parent_ino
            //			ino = parent_ino(dentry); 
            if (filldir(dirent, "..", 2, offset, ino, DT_DIR) < 0)
            {
                break;
            }
            filp->f_pos++;
            offset++;
        default:
            if (!mtpfs_get_dir_data(inode))
            {
                return filp->f_pos;
            }
            filp->f_pos += 2;

            file_numbers = mtpfs_data->data.dircache.num_files;

            if (offset2)
            {

                filp->f_pos -= 2;
                offset2 -= 4;
            }
            for (x = offset2; x < mtpfs_data->data.dircache.num_files; x++)
            {
                if (filldir(dirent, mtpfs_data->data.dircache.file_info[x].filename, strlen(mtpfs_data->data.dircache.file_info[x].filename), filp->f_pos, mtpfs_data->data.dircache.file_info[x].handle, mtpfs_data->data.dircache.file_info[x].mode) < 0)
                {
                    printk("\nFILLDIR SP call free inode data inode=0x%p STILL HAVE DATA NO FREE\n\n", inode);
                    return filp->f_pos;
                }
                filp->f_pos++;
            }
            //printk("\ndavid0923: %s FILLDIR SP DATA FINISHED WANT TO FREE\n",__func__);
            //printk("david0924: %s:%s(%d) f_pos=%d\n",__FILE__,__func__,__LINE__,filp->f_pos);
            //printk("david0924: %s:%s(%d) offset2=%d\n",__FILE__,__func__,__LINE__,offset2);
            if (offset2 == (filp->f_pos - 4))
            {
                // printk("%s FILLDIR SP DATA FINISHED REALLY FREE\n", __func__);
                mtpfs_free_inode_data(inode);
            }
            else
            {
                // printk("%s FILLDIR SP DATA FINISHED NO FREE\n", __func__);
            }
            return filp->f_pos;
    }
    return filp->f_pos;
}

static struct dentry *mtpfs_lookup(struct inode *dir, struct dentry *dentry, struct nameidata *nd)
{

    // printk(KERN_INFO "===== %s =====\n",  __FUNCTION__);

    int x;
    struct mtpfs_inode_data *mtpfs_data = PTPFSINO(dir);

    if (!mtpfs_get_dir_data(dir))
    {
        d_add(dentry, NULL);
        return NULL;
    } if (mtpfs_data->type != MTP_INO_TYPE_DIR && mtpfs_data->type != MTP_INO_TYPE_STGDIR)
    {
        d_add(dentry, NULL);
        return NULL;
    }
    for (x = 0; x < mtpfs_data->data.dircache.num_files; x++)
    {
        if (!memcmp(mtpfs_data->data.dircache.file_info[x].filename, dentry->d_name.name, dentry->d_name.len))
        {
            uint16_t ret;
            
            PTPObjectInfo objectinfo;
            memset(&objectinfo, 0, sizeof(objectinfo));
            
            ret = mtpfs_ptp_getobjectinfo(PTPFSSB(dir->i_sb), mtpfs_data->data.dircache.file_info[x].handle, &objectinfo);
            
            if (ret != PTP_RC_OK)
            {
                d_add(dentry, NULL);
                
                if (ret == PTP_RC_AccessDenied)
                {
                    mtpfs_ptp_free_object_info(&objectinfo);                    
                }
                
                return NULL;
            } 

            int mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH;
            
            // +++ cyhuang (2011/06/08)             
            /* 
             * Fix bug for Creative ZEN Vision:M (idVendor = 0x041e, idProduct = 0x413e).
             * Creative ZEN Vision:M only use ObjectFormat = PTP_OFC_Association to be mark as directory.
             */             
            if (objectinfo.ObjectFormat == PTP_OFC_Association && ((objectinfo.AssociationType == PTP_AT_GenericFolder) || (objectinfo.AssociationType == PTP_AT_Undefined)))
            // +++ cyhuang (2011/06/08)      
            {
                mode |= S_IFDIR;
            }
            else
            {
                mode |= S_IFREG;
            }
            struct inode *newi = mtpfs_get_inode(dir->i_sb, mode, 0, mtpfs_data->data.dircache.file_info[x].handle);
            PTPFSINO(newi)->parent = dir;
            mtpfs_set_inode_info(newi, &objectinfo);
            //atomic_inc(&newi->i_count);    // New dentry reference 
            d_add(dentry, newi);
            mtpfs_ptp_free_object_info(&objectinfo); 
            return NULL;
        }
    }
    d_add(dentry, NULL);
    return NULL;

}

#if defined(CONFIG_MTPFS_DIRECT_IO)
#define MTPFS_DIO_PAGES                        (64)

extern void page_cache_release_dvr(struct page *page);

enum mtpfs_dio_flag
{
    MTPFS_DIO_USER      = 1,
    MTPFS_DIO_KERNEL    = 2
};

struct mtpfs_dio_buffer
{    
    unsigned char *addr;
    size_t size;    
};

struct mtpfs_dio_iovec
{
    unsigned int flag;
    struct mtpfs_dio_buffer *buf;
    int nr_segs;
};

static void inline mtpfs_pages_lock(struct page **pages,int nr_pages)
{
    int i;
     
    for (i=0;i<nr_pages;i++)
    {                    
        lock_page(pages[i]);   
    }     
}

static void inline mtpfs_pages_unlock(struct page **pages,int nr_pages)
{
    int i;
     
    for (i=0;i<nr_pages;i++)
    {
        unlock_page(pages[i]);   
    }     
}

static void inline mtpfs_pages_dio_unmap(struct page **pages,int nr_pages)
{
    int i;  
    
    // printk("%s\n",__func__); 
    
    mtpfs_pages_unlock(pages,nr_pages);       
    
    for (i=0;i<nr_pages;i++)
    {        
        if (! PageReserved(pages[i]))
            SetPageDirty(pages[i]);
                          
        kunmap_atomic(pages[i], KM_USER0);         
        // kunmap(pages[i]);  
        page_cache_release_dvr(pages[i]);   
    }     
}

static void inline mtpfs_buffer_build_dio_iovec(unsigned char *buffer,size_t size,struct mtpfs_dio_iovec *_dio_iovec)
{
    if (buffer == NULL)
        return;
        
    _dio_iovec->buf = kmalloc(sizeof(struct mtpfs_dio_buffer),GFP_KERNEL);
    _dio_iovec->flag = MTPFS_DIO_KERNEL;
    _dio_iovec->buf->addr = buffer;
    _dio_iovec->buf->size = size;
    _dio_iovec->nr_segs = 1;               
}

static void inline mtpfs_pages_build_dio_iovec(struct page **pages,int nr_pages, unsigned int start_offset,size_t size,struct mtpfs_dio_iovec *_dio_iovec)
{
    int i;
    unsigned char *buffer;
    
    if ((pages == NULL) || (pages[0] == NULL))
        return;
    
    _dio_iovec->buf = kmalloc(sizeof(struct mtpfs_dio_buffer) * nr_pages,GFP_KERNEL);
    memset(_dio_iovec->buf,0,sizeof(struct mtpfs_dio_buffer) * nr_pages);
    
    buffer = kmap_atomic(pages[0], KM_USER0);
    // buffer = kmap(pages[0]);  
	buffer += start_offset;

    _dio_iovec->flag = MTPFS_DIO_USER;
    
    _dio_iovec->buf[0].addr = buffer;
    _dio_iovec->buf[0].size = PAGE_SIZE - start_offset;
    
    if (_dio_iovec->buf[0].size > size)
        _dio_iovec->buf[0].size = size;    
    
    _dio_iovec->nr_segs = 1; 
    
    size -= _dio_iovec->buf[0].size;
    
    for (i=1;i<nr_pages;i++)
    {
        int rsize = 0;
        
        buffer = kmap_atomic(pages[i], KM_USER0);
        // buffer = kmap(pages[i]);
        
        /*
        printk("buffer = 0x%x , _dio_iovec->nr_segs = %d\n",buffer,_dio_iovec->nr_segs);
        printk("_dio_iovec->buf[_dio_iovec->nr_segs-1].addr = 0x%x\n",_dio_iovec->buf[_dio_iovec->nr_segs-1].addr);
        printk("_dio_iovec->buf[_dio_iovec->nr_segs-1].size = 0x%lx\n",_dio_iovec->buf[_dio_iovec->nr_segs-1].size);
        */
        
        if (size > PAGE_SIZE)
            rsize = PAGE_SIZE;
        else
            rsize = size;    
        
        
        if ((_dio_iovec->buf[_dio_iovec->nr_segs-1].addr + _dio_iovec->buf[_dio_iovec->nr_segs-1].size) == buffer)
        {
            _dio_iovec->buf[_dio_iovec->nr_segs-1].size += rsize;    
        }
        else
        {            
            _dio_iovec->buf[_dio_iovec->nr_segs].addr = buffer;
            _dio_iovec->buf[_dio_iovec->nr_segs].size = rsize;
            _dio_iovec->nr_segs++;      
        }                        
        
        size -= rsize;
        
        if (size <= 0)
            break;
    }

}

static ssize_t mtpfs_direct_IO(int rw, struct kiocb *iocb, const struct iovec *iov, loff_t offset, unsigned long nr_segs)
{       
    struct file *file = iocb->ki_filp;
    struct mtpfs_sb_info *sb_info = PTPFSSB(file->f_dentry->d_sb); 
    struct inode *inode = file->f_mapping->host;   
    int read_size = 0;
    int err = 0;
    uint16_t ret; 
    int seg,i;
    ssize_t total_size = 0;        
    struct mtpfs_dio_iovec _dio_iovec;   
    
    if (offset >= i_size_read(inode))
        return -EFAULT; 
    
    // printk("%s : inode->i_ino = 0x%x , offset = 0x%llx , iov->iov_len = 0x%x , i_size_read(inode) = 0x%llx\n",__func__ , inode->i_ino , offset,iov->iov_len,i_size_read(inode));    
    // printk("\nnr_segs = %d\n",nr_segs);

    for (seg = 0; seg < nr_segs; seg++) 
    {
        int nr_pages = 0;
        unsigned long user_addr = (unsigned long) iov[seg].iov_base;
		size_t size = iov[seg].iov_len;
		unsigned char *buffer = NULL;
		struct page *pages[MTPFS_DIO_PAGES];	/* page buffer */
		                
        read_size = min((int)(file->f_dentry->d_inode->i_size - offset), (int) size);
                            
        if (!(user_addr & CAC_BASE))         
        {          
            size_t cal_size = read_size;              
            unsigned int cal_offset = user_addr & (PAGE_SIZE - 1); // user_addr % PAGE_SIZE;
            /*                        
            if ((cal_size > PAGE_SIZE) && (user_addr & (PAGE_SIZE-1))) 
            {
                nr_pages++;
                cal_size -= PAGE_SIZE - (user_addr & (PAGE_SIZE - 1));
            }
            */
            
            /*
            if ((cal_offset) && (cal_size >= PAGE_SIZE))
            {
                nr_pages++;
                cal_size -= (PAGE_SIZE - cal_offset);
            } 
            */   
                        
            nr_pages += (cal_size + cal_offset + PAGE_SIZE - 1)/PAGE_SIZE;
                                                
            // printk("nr_pages = %d\n",nr_pages);
            
            if (nr_pages > MTPFS_DIO_PAGES)
            {
                printk("%s [Error] : Too much pages (nr_pages = %d)\n",__func__ ,nr_pages);
                return  -EFAULT;        
            }    
            
            err = !access_ok(VERIFY_READ, (void __user*) user_addr, read_size);
        
            if (err)
            {
                printk("access error : %d\n", err);
                return  -EFAULT;
            }
                               
            // From User Space Address
            down_read(&current->mm->mmap_sem);
            // get_user_pages() return nr_pages 
	        ret = get_user_pages(
		                         current,			/* Task for fault acounting */
		                         current->mm,			/* whose pages? */
		                         user_addr,		/* Where from? */
		                         nr_pages,			/* How many pages? */
		                         1,		/* Write to memory? (dio->rw == READ) */
		                         0,				/* force (?) */
		                         &pages[0],
		                         NULL);				/* vmas */
	        up_read(&current->mm->mmap_sem);
	        
	        mtpfs_pages_lock(pages,nr_pages);
	        
	        mtpfs_pages_build_dio_iovec(pages,nr_pages,cal_offset,read_size,&_dio_iovec);
        }
        else
        {            
            // From Kernel Space Address 
            buffer = (unsigned char *) user_addr;                          
            mtpfs_buffer_build_dio_iovec(buffer,read_size,&_dio_iovec);
        }        
    
                    
        // printk("%s : buffer = 0x%x , read_size = %d , _dio_iovec.nr_segs = %d\n",__func__,buffer,read_size,_dio_iovec.nr_segs);   
        
        for (i=0;i<_dio_iovec.nr_segs;i++) 
        {                                    
            int retry = 0;
            
            for (retry = 0 ; retry < MTPFS_SESSION_MAX_ERR_RETRY ; retry++)
            {
                ret = mtpfs_ptp_getobject_session(sb_info,inode->i_ino,_dio_iovec.buf[i].addr,offset,_dio_iovec.buf[i].size);
                if ((ret == PTP_RC_OK) || (ret == PTP_NRC_GETOBJECT))
                    break;
                                    
                // printk("Retry = %d , ret = 0x%x\n",retry,ret);    
            }
                                                      
            // ret = mtpfs_ptp_getobject_session(sb_info,inode->i_ino,_dio_iovec.buf[i].addr,offset,_dio_iovec.buf[i].size);
            
            if ((ret != PTP_RC_OK) && (ret != PTP_NRC_GETOBJECT))
            {   
                kfree(_dio_iovec.buf);   
                                 
                if (_dio_iovec.flag == MTPFS_DIO_USER)
                {    
                    mtpfs_pages_dio_unmap(pages,nr_pages);                    
                }    
                                                 
                return -EFAULT;
            }
            
            offset += _dio_iovec.buf[i].size;
            // size -= _dio_iovec.buf[i].size;            
            total_size += _dio_iovec.buf[i].size;
            
            // printk("total_size = 0x%x , _dio_iovec.buf[%d].size = 0x%x\n",total_size,i,_dio_iovec.buf[i].size);
                          
        } // End of for
                
        kfree(_dio_iovec.buf);  
                 
        if (_dio_iovec.flag == MTPFS_DIO_USER)
        {    
            mtpfs_pages_dio_unmap(pages,nr_pages);           
        }    
                           
    } // End of for   
        
    return total_size; 
}
#endif // CONFIG_MTPFS_DIRECT_IO

#if defined(CONFIG_MTPFS_FS_READ_AHEAD)
static uint16_t mtpfs_file_read_ahead(struct file *filp,unsigned char *buffer,loff_t offset,uint32_t size)
{    
    struct mtpfs_sb_info *sb_info = PTPFSSB(filp->f_dentry->d_sb); 
    struct inode *inode = filp->f_dentry->d_inode; 
    struct mtpfs_file_buffer *file_buf = NULL;
    uint16_t ret; 
    int retry = 0;
    
    file_buf = (struct mtpfs_file_buffer *) filp->private_data;
    
    if (file_buf && (file_buf->i_ino == inode->i_ino) && ((offset >= file_buf->offset) && ((offset - file_buf->offset + size) <= file_buf->size)))
    {
        // printk("Hit : file_buf->offset = 0x%llx , offset = 0x%llx\n",file_buf->offset,offset);
        memcpy(buffer,file_buf->data + ((uint32_t) (offset - file_buf->offset)),size);
        
        /*
        if (inode->i_ino == 0x220C7)
            data_dump_ascii(buffer,size,16);
        */
        return PTP_RC_OK;
    }    
     
    if (file_buf == NULL)
    {         
        file_buf = kmalloc(sizeof(struct mtpfs_file_buffer) + MTPFS_MAX_READ_AHEAD_SIZE,GFP_KERNEL);                
    
        if (file_buf == NULL)
        {        
            return PTP_RC_GeneralError;
        }
        
        filp->private_data = (void *) file_buf;
    }    
                
    file_buf->i_ino = inode->i_ino;   
    file_buf->offset = offset;
    
    if ((offset + MTPFS_MAX_READ_AHEAD_SIZE) <= i_size_read(inode))
        file_buf->size = MTPFS_MAX_READ_AHEAD_SIZE;
    else
        file_buf->size =  (uint32_t) (i_size_read(inode) - offset);   
    
    // ret = mtpfs_ptp_getobject_session(sb_info,inode->i_ino,file_buf->data,file_buf->offset,file_buf->size); 
                
    for (retry = 0 ; retry < MTPFS_SESSION_MAX_ERR_RETRY ; retry++)
    {
        // printk("file_buf->offset = 0x%x , file_buf->size = 0x%x , i_size_read(inode) = 0x%x\n",(uint32_t) file_buf->offset,(uint32_t) file_buf->size,(uint32_t) i_size_read(inode)); 
        ret = mtpfs_ptp_getobject_session(sb_info,inode->i_ino,file_buf->data,(uint32_t) file_buf->offset,(uint32_t) file_buf->size); 
        
        if ((ret == PTP_RC_OK) || (ret == PTP_NRC_GETOBJECT))
            break;
                                                
        // printk("Retry = %d , ret = 0x%x\n",retry,ret);    
    }
    
    if ((ret != PTP_RC_OK) && (ret != PTP_NRC_GETOBJECT))
    {    
        kfree(file_buf); 
        filp->private_data = NULL;       
        return PTP_RC_GeneralError;      
    }
    
    /*    
    if (filp->private_data)
    {    
        kfree(filp->private_data);        
    }    
    
    filp->private_data = (void *) file_buf;  
    */
                          
    memcpy(buffer,file_buf->data + ((uint32_t) (offset - file_buf->offset)),size);
        
    /*    
    if (inode->i_ino == 0x220C7)
        data_dump_ascii(buffer,size,16);  
    */
    return ret;
}
#endif

static int mtpfs_file_readpage(struct file *filp, struct page *page)
{
// #if !defined(CONFIG_MTPFS_FS_READ_AHEAD)    
    struct mtpfs_sb_info *sb_info = PTPFSSB(filp->f_dentry->d_sb);    
// #endif    
    loff_t offset = ((loff_t) page->index) << PAGE_CACHE_SHIFT; 
    struct inode *inode = filp->f_dentry->d_inode;     
    int size;
    unsigned char *buffer;
    uint16_t ret;  
#if !defined(CONFIG_MTPFS_FS_READ_AHEAD)    
    int retry = 0;
#endif    
             
    // printk("%s : inode->i_ino = 0x%x ,offset = 0x%llx , i_size_read(inode) = 0x%llx\n",__func__,inode->i_ino , offset,i_size_read(inode));
    
    if (offset >= i_size_read(inode))
        goto error;
    
    if (!PageLocked(page))
        PAGE_BUG(page);
    
    // Map to buffer    
    buffer = kmap_atomic(page, KM_USER0);    
    clear_page(buffer);     
    
    size = min((size_t)(i_size_read(inode) - offset), (size_t) PAGE_SIZE);
    // printk("size = %d\n",size);        
    
#if defined(CONFIG_MTPFS_FS_READ_AHEAD)    
    ret = mtpfs_file_read_ahead(filp,buffer,offset,size);
    
    if ((ret != PTP_RC_OK) && (ret != PTP_NRC_GETOBJECT))
    {    
        ret = mtpfs_ptp_getobject_session(sb_info,inode->i_ino,buffer,offset,size);     
    }
            
#else  
    // ret = mtpfs_ptp_getobject_session(sb_info,inode->i_ino,buffer,offset,size); 
    
    for (retry = 0 ; retry < MTPFS_SESSION_MAX_ERR_RETRY ; retry++)
    {
        ret = mtpfs_ptp_getobject_session(sb_info,inode->i_ino,buffer,offset,size);  
        
        if ((ret == PTP_RC_OK) || (ret == PTP_NRC_GETOBJECT))
            break;
                                                
        // printk("Retry = %d , ret = 0x%x\n",retry,ret);    
    }
    
#endif  
    
    if ((ret != PTP_RC_OK) && (ret != PTP_NRC_GETOBJECT))
    {    
        goto error;       
    }    
                
    // Unmap buffer & flush page
    kunmap_atomic(buffer, KM_USER0);
    flush_dcache_page(page);
    if (!PageError(page))
        SetPageUptodate(page);
    unlock_page(page);
    return 0;
error:     
    SetPageError(page);
    unlock_page(page);
    return -EFAULT;
}

/*
static ssize_t mtpfs_write(struct file *filp, const char *buf, size_t count, loff_t *ppos)
{
    //printk(KERN_INFO "%s   pos: %d   count:%d\n",  __FUNCTION__,(int)*ppos,count);
#if 0    
    struct ptp_data_buffer *data = (struct ptp_data_buffer*)filp->private_data;

    struct ptp_block *blocks = kmalloc(sizeof(struct ptp_block)*(data->num_blocks + 1), GFP_KERNEL);

    if (data->blocks)
        memcpy(blocks, data->blocks, sizeof(struct ptp_block) *data->num_blocks);
    blocks[data->num_blocks].block_size = count;
    blocks[data->num_blocks].block = kmalloc(count, GFP_KERNEL);
    memcpy(blocks[data->num_blocks].block, buf, count);
    if (data->blocks)
        kfree(data->blocks);
    data->blocks = blocks;
    data->num_blocks++;
    *ppos += count;
     return count;
#endif // #if 0    
    return -EFAULT;
} 
*/

static int mtpfs_release(struct inode *ino, struct file *filp)
{
    void *data= filp->private_data;
    // printk("<mtp module> %s release inode=0x%p filp=0x%p\n", __func__, ino, filp);
        
    if (data)
    {
        /*
        switch (filp->f_flags & O_ACCMODE)
        {
        case O_RDWR:
        case O_WRONLY:
            {
                struct ptp_object_info object;
                memset(&object,0,sizeof(object));
                if (mtpfs_ptp_getobjectinfo(PTPFSSB(ino->i_sb),ino->i_ino,&object)!=PTP_RC_OK)
                {
                    mtpfs_ptp_free_data_buffer(data);
                    kfree(data);
                    return -EPERM;
                }

                mtpfs_ptp_deleteobject(PTPFSSB(ino->i_sb),ino->i_ino,0);
                int count = 0;
                int x;
                for (x = 0; x < data->num_blocks; x++)
                {
                    count += data->blocks[x].block_size;
                }


                int storage = object.storage_id;
                int parent = object.parent_object;
                int handle = ino->i_ino;
                object.object_compressed_size=count;
                int ret = mtpfs_ptp_sendobjectinfo(PTPFSSB(ino->i_sb), &storage, &parent, &handle, &object);
                ret = mtpfs_ptp_sendobject(PTPFSSB(ino->i_sb),data,count);

                ino->i_size = count;
                ino->i_mtime = CURRENT_TIME;
                mtpfs_free_inode_data(ino);//uncache
                mtpfs_free_inode_data(PTPFSINO(ino) -> parent);//uncache

                //the inode number will change.  Kill this inode so next dir lookup will create a new one
                dput(filp->f_dentry);
                //iput(ino);
            }
            break;
        }
        */
        
        // ptp_free_data_buffer(data);
        if (data)
            kfree(data);
        filp->private_data = NULL;
    }
    return 0;
}

/*
struct mime_map
{
    char *ext;
    int type;
};
const struct mime_map mime_map[] = 
{
    {
        "txt", PTP_OFC_Text
    } , 
    {
        "mp3", PTP_OFC_MP3
    }
    , 
    {
        "mpg", PTP_OFC_MPEG
    }
    , 
    {
        "wav", PTP_OFC_WAV
    }
    , 
    {
        "avi", PTP_OFC_AVI
    }
    , 
    {
        "asf", PTP_OFC_ASF
    }
    , 
    {
        "jpg", PTP_OFC_EXIF_JPEG
    }
    , 
    {
        "tif", PTP_OFC_TIFF
    }
    , 
    {
        "bmp", PTP_OFC_BMP
    }
    , 
    {
        "gif", PTP_OFC_GIF
    }
    , 
    {
        "pcd", PTP_OFC_PCD
    }
    , 
    {
        "pct", PTP_OFC_PICT
    }
    , 
    {
        "png", PTP_OFC_PNG
    }
    , 
    {
        0, 0
    }
    , 
};

static inline int get_format(unsigned char *filename, int l)
{
    char buf[4];
    int x;

    //printk(KERN_INFO "===== %s =====\n",  __FUNCTION__);
    while (l)
    {
        l--;
        if (filename[l] == '.')
        {
            filename = &filename[l];
            l = 0;
            if (strlen(filename) > 4 || strlen(filename) == 1)
            {
                return PTP_OFC_Undefined;
            }
            filename++;
            for (x = 0; x < strlen(filename); x++)
            {
                buf[x] = __tolower(filename[x]);
            }
            buf[strlen(filename)] = 0;
            const struct mime_map *map = mime_map;
            while (map->ext)
            {
                if (!strcmp(buf, map->ext))
                {
                    return map->type;
                } map++;
            }
        }
    }
    return PTP_OFC_Undefined;
}

static int mtpfs_create(struct inode *dir, struct dentry *d, int i,struct nameidata *ni)
{
    //printk(KERN_INFO "%s   %s %d\n",__FUNCTION__,d->d_name.name,i);
    __u32 storage;
    __u32 parent;
    __u32 handle;
    PTPObjectInfo objectinfo;
    memset(&objectinfo, 0, sizeof(objectinfo));

    storage = PTPFSINO(dir)->storage;
    if (PTPFSINO(dir)->type == MTP_INO_TYPE_STGDIR)
    {
        parent = 0xffffffff;
    } 
    else
    {
        parent = dir->i_ino;
    }

    objectinfo.Filename = (unsigned char*)d->d_name.name;
    objectinfo.ObjectFormat = get_format((unsigned char*)d->d_name.name, d->d_name.len);
    objectinfo.ObjectCompressedSize = 0;

    int ret = mtpfs_ptp_sendobjectinfo(PTPFSSB(dir->i_sb), &storage, &parent, &handle, &objectinfo);

    if (ret == PTP_RC_OK)
    {
        unsigned char buf[10];

        ret = mtpfs_ptp_sendobject(PTPFSSB(dir->i_sb), buf, 0);

        if (handle == 0)
        {
            //problem - it didn't return the right handle - need to do something find it
            PTPObjectHandles objects;
            objects.n = 0;
            objects.Handler = NULL;

            mtpfs_get_dir_data(dir);
            if (PTPFSINO(dir)->type == MTP_INO_TYPE_DIR)
            {
                mtpfs_ptp_getobjecthandles(PTPFSSB(dir->i_sb), PTPFSINO(dir)->storage, 0x000000, dir->i_ino, &objects);
            } 
            else
            {
                mtpfs_ptp_getobjecthandles(PTPFSSB(dir->i_sb), dir->i_ino, 0x000000, 0xffffffff, &objects);
            }

            int x, y;
            for (x = 0; x < objects.n && !handle; x++)
            {
                for (y = 0; y < PTPFSINO(dir)->data.dircache.num_files; y++)
                {
                    if (PTPFSINO(dir)->data.dircache.file_info[y].handle == objects.Handler[x])
                    {
                        objects.Handler[x] = 0;
                        y = PTPFSINO(dir)->data.dircache.num_files;
                    }
                }
                if (objects.Handler[x])
                {
                    handle = objects.Handler[x];
                }
            }
            mtpfs_ptp_free_object_handles(&objects);
        }


        int mode = S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | S_IFREG;
        struct inode *newi = mtpfs_get_inode(dir->i_sb, mode, 0, handle);
        PTPFSINO(newi)->parent = dir;
        mtpfs_set_inode_info(newi, &objectinfo);
        atomic_inc(&newi->i_count); // New dentry reference 
        d_instantiate(d, newi);

        objectinfo.Filename = NULL;
        mtpfs_ptp_free_object_info(&objectinfo);

        mtpfs_free_inode_data(dir); //uncache
        dir->i_version++;
        newi->i_mtime = CURRENT_TIME;
        return 0;
    }
    objectinfo.Filename = NULL;
    mtpfs_ptp_free_object_info(&objectinfo);
    return  -EPERM;


}

static int mtpfs_mkdir(struct inode *ino, struct dentry *d, int i)
{
    //printk(KERN_INFO "%s   %s %d\n",__FUNCTION__,d->d_name.name,i);

    __u32 storage;
    __u32 parent;
    __u32 handle;
    PTPObjectInfo objectinfo;
    memset(&objectinfo, 0, sizeof(objectinfo));


    storage = PTPFSINO(ino)->storage;
    if (PTPFSINO(ino)->type == MTP_INO_TYPE_STGDIR)
    {
        parent = 0xffffffff;
    } 
    else
    {
        parent = ino->i_ino;
    }

    objectinfo.Filename = (unsigned char*)d->d_name.name;
    objectinfo.ObjectFormat = PTP_OFC_Association;
    objectinfo.AssociationType = PTP_AT_GenericFolder;

    int ret = mtpfs_ptp_sendobjectinfo(PTPFSSB(ino->i_sb), &storage, &parent, &handle, &objectinfo);
    
    mtpfs_ptp_free_object_info(&objectinfo);

    if (ret == PTP_RC_OK)
    {
        mtpfs_free_inode_data(ino); //uncache
        ino->i_version++;
        return 0;
    }
    return  -EPERM;
}

int mtpfs_unlink(struct inode *dir, struct dentry *d)
{
    //printk(KERN_INFO "%s   %s\n",__FUNCTION__,d->d_name.name);
    int x;

    struct mtpfs_inode_data *mtpfs_data = PTPFSINO(dir);

    if (!mtpfs_get_dir_data(dir))
    {
        return  - EPERM;
    } if (mtpfs_data->type != MTP_INO_TYPE_DIR && mtpfs_data->type != MTP_INO_TYPE_STGDIR)
    {
        return  - EPERM;
    }


    for (x = 0; x < mtpfs_data->data.dircache.num_files; x++)
    {

        if (!memcmp(mtpfs_data->data.dircache.file_info[x].filename, d->d_name.name, d->d_name.len))
        {
            int ret = mtpfs_ptp_deleteobject(PTPFSSB(dir->i_sb), mtpfs_data->data.dircache.file_info[x].handle, 0);
            if (ret == PTP_RC_OK)
            {
                mtpfs_free_inode_data(dir); //uncache
                dir->i_version++;
                return 0;

            }
        }
    }
    return  -EPERM;
}


static int mtpfs_rmdir(struct inode *ino, struct dentry *d)
{

    //	printk(KERN_INFO "===== %s =====\n",  __FUNCTION__);
    return mtpfs_unlink(ino, d);
} 
*/

static int mtpfs_open(struct inode *ino, struct file *filp)
{
    clear_bit(AS_LIMIT_SIZE, &filp->f_mapping->flags);
    return 0;
} 


struct address_space_operations mtpfs_fs_aops = 
{
    readpage: mtpfs_file_readpage, 
#if defined(CONFIG_MTPFS_DIRECT_IO)        
    direct_IO: mtpfs_direct_IO, 
#endif //        
    //	commit_write:	simple_commit_write,		//
};
struct file_operations mtpfs_dir_operations = 
{
    read: generic_read_dir, 
    readdir: mtpfs_readdir, 
};
struct file_operations mtpfs_file_operations = 
{
    read: generic_file_read, 
    // write: mtpfs_write, 
    open: mtpfs_open, 
    release: mtpfs_release, 
    // llseek:	mtp_file_llseek,
    llseek: generic_file_llseek,

};
struct inode_operations mtpfs_dir_inode_operations = 
{
    lookup: mtpfs_lookup, 
/*        
    mkdir: mtpfs_mkdir, 
    rmdir: mtpfs_rmdir, 
    unlink: mtpfs_unlink, 
    create: mtpfs_create, 
*/    
};

/*
struct file_operations mtpfs_file_operations = {
write:      mtpfs_file_write,
mmap:       generic_file_mmap,
fsync:      mtpfs_sync_file,
};
struct inode_operations mtpfs_dir_inode_operations = {
create:     mtpfs_create,
rename:     mtpfs_rename,
};
 */
