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
#include "md_reg.h"

///////////////////// MACROS /////////////////////////////////

static struct md_dev* dev = NULL;
static struct platform_device*  md_device = NULL;


///////////////////// MACROS /////////////////////////////////
#define _md_map_single(p_data, len, dir)      dma_map_single(&md_device->dev, p_data, len, dir)
#define _md_unmap_single(p_data, len, dir)    dma_unmap_single(&md_device->dev, p_data, len, dir)
#define md_lock(md)                           spin_lock_irqsave(&md->lock, md->spin_flags)
#define md_unlock(md)                         spin_unlock_irqrestore(&md->lock, md->spin_flags)

//#define dbg_char(x)                         writel((x) ,0xb801B200)
#define dbg_char(x)


static volatile unsigned int* IOA_SMQ_CNTL         = IOA_AMQ_CNTL_REG;
static volatile unsigned int* IOA_SMQ_INT_STATUS   = IOA_AMQ_INT_STATUS_REG;
static volatile unsigned int* IOA_SMQ_INT_ENABLE   = IOA_AMQ_INT_ENABLE_REG;
static volatile unsigned int* IOA_SMQ_CmdBase      = IOA_AMQ_CmdBase_REG;
static volatile unsigned int* IOA_SMQ_CmdLimit     = IOA_AMQ_CmdLimit_REG;
static volatile unsigned int* IOA_SMQ_CmdRdptr     = IOA_AMQ_CmdRdptr_REG;
static volatile unsigned int* IOA_SMQ_CmdWrptr     = IOA_AMQ_CmdWrptr_REG;
static volatile unsigned int* IOA_SMQ_CmdFifoState = IOA_AMQ_CmdFifoState_REG;
static volatile unsigned int* IOA_SMQ_INST_CNT     = IOA_AMQ_INST_CNT_REG;

//#define MD_COMPLETION_DBG(fmt, args...)             printk("[MD][CMP] " fmt, ## args)
#define MD_COMPLETION_DBG(fmt, args...)             


MD_COMPLETION* md_alloc_cmd_completion(    
    MD_CMD_HANDLE       request_id,
    dma_addr_t          dst,
    unsigned long       dst_len,
    dma_addr_t          src,
    unsigned long       src_len
    );

/*-------------------------------------------------------------------- 
 * Func : md_open 
 *
 * Desc : open md engine
 *
 * Parm : N/A
 *
 * Retn : N/A
 --------------------------------------------------------------------*/  
void md_open(void)
{
    u32 PhysAddr;
    int i;
    MD_COMPLETION* cmp;
	
    if (is_jupiter_cpu())	
    {
        // jupiter has only one md queue for SCPU
        IOA_SMQ_CNTL        = IOA_SMQ_CNTL_REG;
        IOA_SMQ_INT_STATUS  = IOA_SMQ_INT_STATUS_REG;
        IOA_SMQ_INT_ENABLE  = IOA_SMQ_INT_ENABLE_REG;
        IOA_SMQ_CmdBase     = IOA_SMQ_CmdBase_REG;
        IOA_SMQ_CmdLimit    = IOA_SMQ_CmdLimit_REG;
        IOA_SMQ_CmdRdptr    = IOA_SMQ_CmdRdptr_REG;
        IOA_SMQ_CmdWrptr    = IOA_SMQ_CmdWrptr_REG;
        IOA_SMQ_CmdFifoState= IOA_SMQ_CmdFifoState_REG;
        IOA_SMQ_INST_CNT    = IOA_SMQ_INST_CNT_REG;
    }    
    
    /* get correlated virtuall address of correlated io address */			

    //start MD
    writel(SMQ_GO | SMQ_CLR_WRITE_DATA | SMQ_CMD_SWAP, IOA_SMQ_CNTL);

    if (dev == NULL)
    {
        dev = (struct md_dev *)kmalloc(sizeof(struct md_dev), GFP_KERNEL);
        
        if (dev == NULL)        
            return ;
            
        memset(dev, 0, sizeof(struct md_dev));
    }
	dev->size = SMQ_COMMAND_ENTRIES * sizeof(u32);

    dev->CachedCmdBuf = kmalloc(dev->size, GFP_KERNEL);                 // cached virt address
    dev->CmdBuf       =(void*)(((unsigned long)dev->CachedCmdBuf & 0x0FFFFFFF) | 0xA0000000);  // non cached virt address
    PhysAddr          = virt_to_phys(dev->CachedCmdBuf);                
    
	DBG_PRINT("[user land] CmdBuf virt addr = %p/%p, phy addr=%p\n", dev->CachedCmdBuf, (void*)dev->CmdBuf, (void*)PhysAddr);

    dev->v_to_p_offset = (int32_t)((u8 *)dev->CmdBuf - (u32)PhysAddr);

    dev->wrptr    = 0;
	dev->CmdBase  = (void *)PhysAddr;
	dev->CmdLimit = (void *)(PhysAddr + SMQ_COMMAND_ENTRIES*sizeof(u32));
	spin_lock_init(&dev->lock);

    INIT_LIST_HEAD(&dev->task_list);
    INIT_LIST_HEAD(&dev->free_task_list);
    
    for (i=0; i<32; i++)
    {
        cmp = md_alloc_cmd_completion(0, 0, 0, 0, 0);
        if (cmp)
            list_add_tail(&cmp->list, &dev->free_task_list);
    } 
    
	*(volatile unsigned int *)IOA_SMQ_CmdRdptr = 1;

  	writel((u32)dev->CmdBase,  IOA_SMQ_CmdBase);
    writel((u32)dev->CmdLimit, IOA_SMQ_CmdLimit);
	writel((u32)dev->CmdBase,  IOA_SMQ_CmdRdptr);
    writel((u32)dev->CmdBase,  IOA_SMQ_CmdWrptr);
    writel(0,                  IOA_SMQ_INST_CNT);

	//enable interrupt
    //writel(SMQ_SET_WRITE_DATA | INT_SYNC, IOA_SMQ_INT_ENABLE);

    __sync();

    //start SE
    writel(SMQ_GO | SMQ_SET_WRITE_DATA | SMQ_CMD_SWAP, IOA_SMQ_CNTL);    	
}


 
/*-------------------------------------------------------------------- 
 * Func : md_close 
 *
 * Desc : close md engine
 *
 * Parm : N/A
 *
 * Retn : N/A
 --------------------------------------------------------------------*/ 
void md_close(void)
{
    int count = 0;
    
    while(readl(IOA_SMQ_CmdWrptr) != readl(IOA_SMQ_CmdRdptr))
    {
        msleep(100);
        if (count++ > 300) 
            return;
    }

	writel(SMQ_GO | SMQ_CLR_WRITE_DATA, IOA_SMQ_CNTL);

	if (dev) 
	{
        if (dev->CachedCmdBuf)
            kfree(dev->CachedCmdBuf);
                            
		kfree(dev);
        dev = NULL;
	}
    return;
}




/*-------------------------------------------------------------------- 
 * Func : md_check_hardware
 *
 * Desc : check hardware status
 *
 * Parm : N/A
 *
 * Retn : N/A
 --------------------------------------------------------------------*/
void md_check_hardware(void)
{
    unsigned long cntl = readl(IOA_SMQ_CNTL);
    unsigned long status = readl(IOA_SMQ_INT_STATUS);
    
	if ((cntl & BIT1)==0)
    {                
        if (cntl & BIT3)        // idel bit is seted
        {            
            printk("[MD] md status = idel, status = %08lx\n", status);
            // something error
            if (status & BIT2)
                printk("[MD] Fatal Error, smq command decoding error\n");
                
            if (status & BIT1)
                printk("[MD] Fatal Error, smq instrunction error\n");
                
            // do error handling            
        }
        
        printk("[MD] restart md\n");
        
        writel(SMQ_GO | SMQ_SET_WRITE_DATA | SMQ_CMD_SWAP, IOA_SMQ_CNTL);   // restart
    }        	      
}



/*-------------------------------------------------------------------- 
 * Func : WriteCmd 
 *
 * Desc : Write Command to Move Data Engine
 *
 * Parm : dev   :   md device
 *        pBuf  :   cmd
 *        nLen  :   cmd length in bytes
 *
 * Retn : N/A
 --------------------------------------------------------------------*/
static
u64 WriteCmd(
    struct md_dev*          dev, 
    u8*                     pBuf, 
    int                     nLen
    )
{
    int i;
    u8 *writeptr;
    u8 *pWptr;
    u8 *pWptrLimit;
    u64 counter64 = 0;
    
    md_lock(dev);    
    
    md_check_hardware();        // check hardware
        
    pWptrLimit = (u8 *)dev->CmdBuf + dev->size;
    pWptr      = (u8 *)dev->CmdBuf + dev->wrptr;

    writeptr = (u8*) readl(IOA_SMQ_CmdWrptr);        
    
    if ((dev->wrptr+nLen) >= dev->size)
    {
        //to cover md copy bus, see mantis 9801 & 9809
        while(1)
        {
            u32 rp = (u32)readl(IOA_SMQ_CmdRdptr);
            if((u32)writeptr == rp) 
                break;
            udelay(1);
        }
    }

    u8 *readptr;
    while(1)
    {        
        readptr = (u8 *)readl(IOA_SMQ_CmdRdptr);
        if(readptr <= writeptr)
        {
            readptr += dev->size; 
        }
        if((writeptr+nLen) < readptr)
        {
            break;
        }
        udelay(1);
    }

    //Start writing command words to the ring buffer.
    for(i=0; i<nLen; i+=sizeof(u32))
    {        
        *(u32 *)((u32)pWptr) = endian_swap_32(*(u32 *)(pBuf + i));
        
        pWptr += sizeof(u32);
        
        if (pWptr >= pWptrLimit)
            pWptr = (u8 *)dev->CmdBuf;
    }    

    dev->wrptr += nLen;
    if (dev->wrptr >= dev->size)
        dev->wrptr -= dev->size;

    //convert to physical address
    pWptr -= dev->v_to_p_offset;

    __sync();

    writel((u32)pWptr, IOA_SMQ_CmdWrptr);    

    dev->sw_counter.low++;    
    if (dev->sw_counter.low==0)    
        dev->sw_counter.high++;
            
    counter64 = dev->sw_counter.high;
    counter64 = (counter64 << 32) | dev->sw_counter.low;                      
    
    md_unlock(dev);    
    
    return counter64;
}




/*-------------------------------------------------------------------- 
 * Func : md_checkfinish 
 *
 * Desc : check if the specified command finished. 
 *
 * Parm : handle   :   md command handle 
 *
 * Retn : true : finished, 0 : not finished
 --------------------------------------------------------------------*/
bool md_checkfinish(MD_CMD_HANDLE handle)
{
	u32 sw_counter = (u32)handle & 0xFFFF;
	u32 hw_counter;
	
	if (handle==0)
	    return true;
	    
    hw_counter = readl(IOA_SMQ_INST_CNT) & 0xFFFF;
	
	if (hw_counter >= sw_counter)
	{
	    if((hw_counter - sw_counter) < 0x8000)
            return true;
	}
    else
	{
	    if((hw_counter + 0x10000 - sw_counter) < 0x8000)
		    return true;
	}
	
	md_check_hardware();        // command not complete... check hardware ...
	
    return false;
}



///////////////////////////////////////////////////////////////////////
// Copy Request Management APIs
///////////////////////////////////////////////////////////////////////



/*-------------------------------------------------------------------- 
 * Func : md_alloc_cmd_completion 
 *
 * Desc : alloc a completion cmd
 *
 * Parm : N/A
 *
 * Retn : 0 : success, <0 failed
 --------------------------------------------------------------------*/
MD_COMPLETION* md_alloc_cmd_completion(    
    MD_CMD_HANDLE       request_id,
    dma_addr_t          dst,
    unsigned long       dst_len,
    dma_addr_t          src,
    unsigned long       src_len
    )
{
    MD_COMPLETION* cmp;
        
    if (list_empty(&dev->free_task_list))            
    {        
        cmp = (MD_COMPLETION*) kmalloc(sizeof(MD_COMPLETION), GFP_KERNEL);    
        memset(cmp, 0, sizeof(MD_COMPLETION));
    }        
    else
    {
        cmp = list_entry(dev->free_task_list.next, MD_COMPLETION, list );  // get it from preallocate task list
        list_del(&cmp->list);        
        memset(cmp, 0, sizeof(MD_COMPLETION));        
        cmp->flags = 1;         // pool
    }        
            
    if (cmp)
    {                     
        INIT_LIST_HEAD(&cmp->list);
        
        cmp->request_id = request_id;
        cmp->dst        = dst;
        cmp->dst_len    = dst_len;
        cmp->src        = src;
        cmp->src_len    = src_len;                
    }
    
    return cmp;
}




/*-------------------------------------------------------------------- 
 * Func : md_release_cmd_completion 
 *
 * Desc : alloc a completion cmd
 *
 * Parm : N/A
 *
 * Retn : 0 : success, <0 failed
 --------------------------------------------------------------------*/
void md_release_cmd_completion(    
    MD_COMPLETION*      cmp    
    )
{
    if (cmp->flags)           
    {
        INIT_LIST_HEAD(&cmp->list);                   // reinit list
        list_add_tail(&cmp->list, &dev->free_task_list);   // move to free list    
    }        
    else        
        kfree(cmp);    
}



/*-------------------------------------------------------------------- 
 * Func : md_add_cmd_completion 
 *
 * Desc : add a completion cmd
 *
 * Parm : N/A
 *
 * Retn : 0 : success, <0 failed
 --------------------------------------------------------------------*/
int md_add_cmd_completion(    
    MD_CMD_HANDLE       request_id,
    dma_addr_t          dst,
    unsigned long       dst_len,
    dma_addr_t          src,
    unsigned long       src_len
    )
{
    MD_COMPLETION* cmp = md_alloc_cmd_completion(request_id, dst, dst_len, src, src_len);
    
    if (cmp)
    {
        md_lock(dev);
        list_add_tail(&cmp->list, &dev->task_list);      
          
        MD_COMPLETION_DBG("add complete req = %llu, src=%08x, len=%lu, dst=%08x, len=%lu\n", 
                cmp->request_id, cmp->src, cmp->src_len, cmp->dst, cmp->dst_len);
     
        md_unlock(dev); 

        return 0;
    }
    
    return -1;
}




/*-------------------------------------------------------------------- 
 * Func : md_cmd_complete 
 *
 * Desc : do completion
 *
 * Parm : N/A
 *
 * Retn : MD_CMD 
 --------------------------------------------------------------------*/
void md_cmd_complete(
    MD_COMPLETION*      cmp
    )
{                
    MD_COMPLETION_DBG("complete req = %llu, src=%08x, len=%lu, dst=%08x, len=%lu\n", 
            cmp->request_id, cmp->src, cmp->src_len, cmp->dst, cmp->dst_len);
    
    if (cmp->src && cmp->src_len)
        _md_unmap_single(cmp->src, cmp->src_len, DMA_TO_DEVICE);
    
    if (cmp->dst && cmp->dst_len)    
        _md_unmap_single(cmp->dst, cmp->dst_len, DMA_FROM_DEVICE);
        
    md_release_cmd_completion(cmp);
}





/*-------------------------------------------------------------------- 
 * Func : md_check_copy_tasks 
 *
 * Desc : check a copy request
 *
 * Parm : 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
void md_check_copy_tasks(void)
{
    struct list_head* cur;
    struct list_head* next;
    MD_COMPLETION* cmp = NULL;    
    
    md_lock(dev);
    
    list_for_each_safe(cur, next, &dev->task_list)
    {
        cmp = list_entry(cur, MD_COMPLETION, list);
        
        MD_COMPLETION_DBG("check tastk (%llu)\n", cmp->request_id);
                
        if (md_checkfinish(cmp->request_id)==0)        
            break;                            
                
        list_del(&cmp->list);   // detatch it form the task list
        
        md_cmd_complete(cmp);
    }
    
    md_unlock(dev);      
}



/*-------------------------------------------------------------------- 
 * Func : md_copy_sync 
 *
 * Desc : sync md copy request
 *
 * Parm : md_copy_t
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
int md_copy_sync(MD_COPY_HANDLE handle)
{                   
    unsigned int aaa = 0;                     
    
    //md_check_copy_tasks();
    
    if (!list_empty(&dev->task_list))
    {
        MD_COMPLETION_DBG("sync copy (%llu)\n", handle);
        
        while(md_checkfinish(handle)==0)                 
        {
            if (aaa++ > 100000)        
            {
                printk("[MD] WARNING, command not complete, req_id = %llu, hw_counter=%lu", handle, (unsigned long) readl(IOA_SMQ_INST_CNT) & 0xFFFF);
                aaa = 0;
            }                
            udelay(1);        
        }

        md_check_copy_tasks();
    }

    return 0;        
}




///////////////////////////////////////////////////////////////////////
// Asynchronous APIs
///////////////////////////////////////////////////////////////////////




/*-------------------------------------------------------------------- 
 * Func : md_copy_start 
 *
 * Desc : doing memory copy via md
 *
 * Parm : req   :   md_copy_request 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
MD_COPY_HANDLE md_copy_start( 
    void*                   dst,
    void*                   src,
    int                     len,
    int                     dir
    )
{               
    unsigned long   tmp;
    unsigned long   remain = len;
    dma_addr_t      dma_dst;
    dma_addr_t      dma_src;
    dma_addr_t      dst_tmp;
    dma_addr_t      src_tmp;
    MD_COPY_HANDLE  request_id = 0;
    u32             cmd[4];                     
                
    dma_dst = _md_map_single(dst, len, DMA_FROM_DEVICE);    
    dma_src = _md_map_single(src, len, DMA_TO_DEVICE);
           
    dst_tmp = dma_dst;
    src_tmp = dma_src;    
    
    while(remain)
    {
        //to cover md copy bus, see mantis 9801 & 9809
        int bytes_to_next_8bytes_align = (8 - ((u32)src & 0x7)) & 0x7;
    
        if(((remain - bytes_to_next_8bytes_align) & 0xFF) == 8)                 
            tmp = 8;                 
        else        
            tmp = len;        
            
        cmd[0] = MOVE_DATA_SS;
        
        if(!dir) 
            cmd[0] |= 1 << 7;    //backward
 
        cmd[1] = (u32)dst_tmp;
        cmd[2] = (u32)src_tmp;
        cmd[3] = (u32)tmp;
 
        request_id = md_write((const char *)cmd, sizeof(cmd));
        remain  -= tmp;
        dst_tmp += tmp;
        src_tmp += tmp;
    }    

    md_add_cmd_completion(request_id, dma_dst, len, dma_src, len);  
        
    return request_id;
}



/*-------------------------------------------------------------------- 
 * Func : md_memset_start 
 *
 * Desc : doing memory set via md
 *
 * Parm : dst   :   Destination 
 *        data    :   data to set
 *        len     :   number of bytes 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
MD_CMD_HANDLE md_memset_start(
    void*                   dst, 
    u8                      data, 
    int                     len
    )
{    
    MD_COPY_HANDLE request_id = 0;    
    dma_addr_t dma_dst = _md_map_single(dst, len, DMA_FROM_DEVICE);
    u32 cmd[4];    
    
	cmd[0] = MOVE_DATA_SS | (1 << 6);
	cmd[1] = (u32)dma_dst;
	cmd[2] = (u32)data;
	cmd[3] = (u32)len;

    request_id =  (MD_CMD_HANDLE) md_write((const char *)cmd, sizeof(cmd)); 
    
    md_add_cmd_completion(request_id, dma_dst, len, 0, 0);  
    
    return request_id; 
}




///////////////////////////////////////////////////////////////////////
// Synchronous APIs
///////////////////////////////////////////////////////////////////////



/*-------------------------------------------------------------------- 
 * Func : md_memcpy 
 *
 * Desc : doing memory copy via md
 *
 * Parm : lpDst   :   Destination
 *        lpSrc   :   Source
 *        len     :   number of bytes
 *        forward :   1 : Src to Dest, 0 Dest to Src
 *
 * Retn : 0 : succeess, others failed
 --------------------------------------------------------------------*/
int md_memcpy(
    void*                   dst, 
    void*                   src, 
    int                     len, 
    bool                    forward
    )
{            
    MD_CMD_HANDLE mdh;
    
    if ((mdh= md_copy_start(dst, src, len, forward)))    
    {        
        md_copy_sync(mdh);      // wait complete         
        return 0;
    }    
    
    return -1; 	
}





/*-------------------------------------------------------------------- 
 * Func : md_memset 
 *
 * Desc : doing memory set via md
 *
 * Parm : lpDst   :   Destination 
 *        data    :   data to set
 *        len     :   number of bytes 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
int md_memset(
    void*                   dst, 
    u8                      data, 
    int                     len
    )
{    
    MD_CMD_HANDLE mdh;
    
    if ((mdh = md_memset_start(dst, data, len)))    
    {        
        md_copy_sync(mdh);      // wait complete
        return 0;
    }        
    
    return -1;        
}



///////////////////////////////////////////////////////////////////////
// MISC APIs
///////////////////////////////////////////////////////////////////////




/*-------------------------------------------------------------------- 
 * Func : md_check_physical_alignment 
 *
 * Desc : check if this buff is physical alignment
 *
 * Parm : buff    :   buff      (kernel virtual address)
 *        len     :   length    
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
int md_check_physical_alignment(
    void*                   buff,     
    int                     len)
{   
    int pc = (len + ((u32)buff & PAGE_SIZE)) >> PAGE_SHIFT;
    int i;
    u32 phy_addr = (u32) virt_to_phys(buff);
    
    for (i=1; i<pc; i++)     
    {
        if ((u32) virt_to_phys(buff) != phy_addr)
            return 0;

        buff     += PAGE_SIZE;
        phy_addr += PAGE_SIZE;        
    }
    
    return 1;    
}




/*-------------------------------------------------------------------- 
 * Func : md_is_valid_address 
 *
 * Desc : check address for md access
 *
 * Parm : addr   :   Destination 
 *        len    :   number of bytes 
 *
 * Retn : 1 for valid address, 0 for invalid address space
 --------------------------------------------------------------------*/
int md_is_valid_address(
    void*                   addr, 
    unsigned long           len
    )
{    
    u32 address = (u32) addr;    

    
    if (address > 0x7FFFFFFF && address < 0xC0000000)        
    {
        // unmapped kernel address (0x8000_0000 ~ 0xBFFF_FFFF)                                    
        return md_check_physical_alignment(addr, len);
    }        
    else
    {                
        struct vm_area_struct* vma; 
        
        vma = find_extend_vma(current->mm, address);
        if (vma && (vma->vm_flags & VM_DVR))
            return 1;           // memory allocated by pli
    }        

    return 0;    
}




/*-------------------------------------------------------------------- 
 * Func : md_write 
 *
 * Desc : send command to MD
 *
 * Parm : lpDst   :   Destination 
 *        data    :   data to set
 *        len     :   number of bytes 
 *
 * Retn : MD_CMD_HANDLE
 --------------------------------------------------------------------*/
MD_CMD_HANDLE md_write(const char *buf, size_t count)
{
    return WriteCmd(dev, (u8 *)buf, count);
}




//////////////////////////////////////////////////////////////////////////////////////////////
// Platform Device Interface
//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_PM

#define MD_DEV_NAME         "MD"

static int md_drv_probe(struct device *dev)
{
    struct platform_device	*pdev = to_platform_device(dev);    
    return strncmp(pdev->name,MD_DEV_NAME, strlen(MD_DEV_NAME));
}


static int md_drv_remove(struct device *dev)
{    
    return 0;
}


static int md_drv_suspend(struct device *p_dev, pm_message_t state, u32 level)
{                             
    md_lock(dev); 	                        
             
    dev->state = 1;
    
    md_unlock(dev);  
    
    while(!list_empty(&dev->task_list))
    {   
        md_check_copy_tasks();
        udelay(10);
    }
    
    dev->sw_counter.low = (unsigned long) readl(IOA_SMQ_INST_CNT);

    
    
    return 0;    
}


static int md_drv_resume(struct device *p_dev, u32 level)
{             
    md_lock(dev); 	                        
             
    dev->state = 0;
    
    writel(dev->sw_counter.low, IOA_SMQ_INST_CNT);
    
    md_check_hardware();        
    
    md_unlock(dev);        
                    
    return 0;    
}


static struct device_driver md_drv = 
{
    .name     =	MD_DEV_NAME,
	.bus	  =	&platform_bus_type,	
    .probe    = md_drv_probe,
    .remove   = md_drv_remove,    
    .suspend  = md_drv_suspend,
    .resume   = md_drv_resume,
};

#endif



/***************************************************************************
     ------------------- module init / exit stubs ----------------
****************************************************************************/

static int __init md_init(void)
{
    md_open();
        
    md_device = platform_device_register_simple("MD", 0, NULL, 0);      
        
#ifdef CONFIG_PM        
    driver_register(&md_drv);
#endif             
    
    return 0;	
}


static void __exit md_exit(void)
{
    platform_device_unregister(md_device);
    
#ifdef CONFIG_PM            
    driver_unregister(&md_drv);    
#endif           
   
	md_close();		   
}



module_init(md_init);
module_exit(md_exit);
