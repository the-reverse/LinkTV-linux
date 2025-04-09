#include <asm/io.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/types.h>

#include <linux/mtp/libmtp.h>
#include <linux/mtp/usb.h>
#include <linux/usb_ch9.h>
#include "mtpfs.h"

// #define MTPFS_SESSION_DEBUG         1
// #undef MTPFS_SESSION_DEBUG

/*
 *
 *  MTP Compliance Test : 
 *                        OpenSession
 *                        CloseSession
 *                        GetDeviceInfo
 *                        GetStorageIDs
 *                        GetStorageInfo
 *                        GetObject
 *                        GetDevicePropDesc
 *                        GetDevicePropValue
 *                        SetDevicePropValue
 *                        DeleteObject
 *                        SendObject
 *                        GetNumObjects
 *                        GetObjectHandles
 *                        GetObjectInfo
 *                        SendObjectInfo
 *                        GetPartialObject
 *                        GetObjectPropsSupported
 *                        GetObjectPropDesc
 *                        GetObjectPropValue
 *                        SetObjectPropValue
 *                        GetObjectReferences
 *                        SetObjectReferences
 */

static DECLARE_MUTEX (mtpfs_session_mutex);

static void inline mtpfs_session_lock(PTPParams *params)
{    
    down(&mtpfs_session_mutex);   
}

static void inline mtpfs_session_unlock(PTPParams *params)
{    
    up(&mtpfs_session_mutex);       
}

static inline PTPParams *mtpfs_mtp_device_get_params(struct mtpfs_sb_info *sb)
{
    LIBMTP_mtpdevice_t *mtp_device = sb->device_info->mtp_device;
    
    if (mtp_device == NULL)
    {
        printk("%s [Error] : mtp_device is NULL\n",__func__);
        return NULL;
    }    
        
    return (PTPParams*) mtp_device->params;         
}

uint16_t mtpfs_ptp_getobjecthandles (struct mtpfs_sb_info *sb, uint32_t storage,
                                   uint32_t objectformatcode, uint32_t associationOH,
                                   PTPObjectHandles *objecthandles)
{
    uint16_t ret;
    PTPParams *params = mtpfs_mtp_device_get_params(sb);
    
    if (params == NULL)
    {
        printk("%s [Error] : params is NULL\n",__func__);
        return PTP_ERROR_IO;    
    }    
    
    mtpfs_session_lock(params);
    ret = ptp_getobjecthandles(params,storage,objectformatcode,associationOH,objecthandles);
    mtpfs_session_unlock(params);
    
    return ret;    
}

void mtpfs_ptp_free_object_handles(PTPObjectHandles *objecthandles)
{
    if (objecthandles->Handler) 
    {    
        kfree(objecthandles->Handler);  
        objecthandles->Handler = NULL;  
    }    
}

uint16_t mtpfs_ptp_getobjectinfo (struct mtpfs_sb_info *sb, uint32_t handle,PTPObjectInfo *objectinfo)
{
    uint16_t ret;
    PTPParams *params = mtpfs_mtp_device_get_params(sb);
    
    if (params == NULL)
    {
        printk("%s [Error] : params is NULL\n",__func__);
        return PTP_ERROR_IO;    
    }   
    
    mtpfs_session_lock(params);
    ret = ptp_getobjectinfo(params,handle,objectinfo); 
    mtpfs_session_unlock(params); 
    
    return ret;       
}                                

void mtpfs_ptp_free_object_info(PTPObjectInfo *object)
{
    ptp_free_objectinfo(object);
}

/*
uint16_t mtpfs_ptp_sendobjectinfo (struct mtpfs_sb_info *sb, uint32_t *store,
                                 uint32_t *parenthandle, uint32_t *handle,
                                 PTPObjectInfo *objectinfo)
{
    PTPParams *params = mtpfs_mtp_device_get_params(sb);
    
    if (params == NULL)
    {
        printk("%s [Error] : params is NULL\n",__func__);
        return PTP_ERROR_IO;    
    }
    
    return ptp_sendobjectinfo(params,store,parenthandle,handle,objectinfo);    
}
                                 
uint16_t mtpfs_ptp_sendobject(struct mtpfs_sb_info *sb, unsigned char *buf, uint32_t size)
{
    PTPParams *params = mtpfs_mtp_device_get_params(sb);
    
    if (params == NULL)
    {
        printk("%s [Error] : params is NULL\n",__func__);
        return PTP_ERROR_IO;    
    }
    
    return ptp_sendobject(params,buf,size); 
}

uint16_t mtpfs_ptp_deleteobject(struct mtpfs_sb_info *sb,uint32_t handle, uint32_t ofc)
{
    PTPParams *params = mtpfs_mtp_device_get_params(sb);
    
    if (params == NULL)
    {
        printk("%s [Error] : params is NULL\n",__func__);
        return PTP_ERROR_IO;    
    }
    
    return ptp_deleteobject(params,handle,ofc);    
}
*/

static int mtpfs_ptp_get_data_from_session_buffer(PTPParams *params ,unsigned char *buf,uint32_t offset,uint32_t size)
{
    uint32_t addr;     
    unsigned char *session_buf = params->session_buf; 

    int len = params->obj_session.cur_offset - offset;
    int max_len = params->obj_session.cur_offset - params->obj_session.session_buf_offset;
    int session_buf_offset = offset - params->obj_session.session_buf_offset;        
    
	/*
    printk("===========================================================================\n");
    printk("params->obj_session.split_header_data = %d\n",params->obj_session.split_header_data);
    printk("params->obj_session.cur_offset = 0x%x , offset = 0x%x , len = 0x%x\n",params->obj_session.cur_offset,offset,len);
    printk("params->obj_session.session_buf_offset = 0x%x , session_buf_offset = 0x%x , max_len = 0x%x\n",params->obj_session.session_buf_offset,session_buf_offset,max_len);
    printk("==========================================================================\n");
    */
    
    if (session_buf && (size > 0) && (len > 0) && (session_buf_offset > 0) && (session_buf_offset < max_len) && (len <= max_len))
    {
        session_buf += session_buf_offset;                 
        
        if (len > size)
            len = size;    
            
        addr = (uint32_t) buf;    
                
        memcpy(buf,session_buf ,len);
            
        dma_cache_wback(addr, len);            
        return len;   
    }       
    return -1; 
}


uint16_t mtpfs_ptp_getobject_session (struct mtpfs_sb_info *sb, uint32_t handle, unsigned char *buf, uint32_t offset,uint32_t size)
{
    PTPParams *params = mtpfs_mtp_device_get_params(sb);
    PTP_USB *ptp_usb;
    uint16_t ret = 0;                         
    
    if ((params == NULL) || (size == 0))
    {
        printk("%s [Error] : params is NULL or size is zero\n",__func__);
        return PTP_ERROR_IO;    
    }        
      
    // printk("%s : handle = 0x%x , offset = 0x%x , size = 0x%x , buf = 0x%x\n",__func__,handle,offset,size,buf);

    ptp_usb = (PTP_USB *) params->data;  
        
    // printk("params->obj_session.cur_offset = 0x%x, offset = 0x%x\n",params->obj_session.cur_offset,offset);
                       
    // cyhuang (2011/06/08) :
    /* Change from mtp_is_mtp_device() to ptp_operation_issupported() check GetPartialObject command support. */        
    if (ptp_operation_issupported(params,PTP_OC_GetPartialObject) == 1)     
    {    
        int rlen;             
        
        ret = ptp_getpartialobject_new(params,handle,offset,size, buf,&rlen);
        // printk("PTP_OC_GetPartialObject : offset = 0x%x , size = 0x%x , ret = 0x%x , rlen = 0x%x\n",offset,size,ret,rlen);    
        return ret;
    }         
    
    down(&mtpfs_session_mutex);        
            
    if (params->obj_session.cur_offset > offset)
    {
        int len = params->obj_session.cur_offset - offset;       
        
        /*            
        printk("1 : params->obj_session.cur_offset = 0x%x , offset = 0x%x\n",params->obj_session.cur_offset,offset);
        printk("1 : len = 0x%x\n",len);
        */
                        
        if (len >= params->obj_session.session_buf_size)
        {    
            ret = ptp_getobject_session(params,handle,buf,size,true);
        
            if (ret != PTP_RC_OK)
                goto err;
        }          
    }
         
    if (params->obj_session.cur_offset < offset)
    {
        int  skip_len;        
                
        // printk("2 : params->obj_session.cur_offset = 0x%x , offset = 0x%x\n",params->obj_session.cur_offset,offset);
               
        while (params->obj_session.cur_offset < offset)
        {           
            uint32_t ori_offset = params->obj_session.cur_offset;             
            skip_len = offset - params->obj_session.cur_offset; 
                
            /*                             
            printk("2 : params->obj_session.cur_offset = 0x%x , offset = 0x%x\n",params->obj_session.cur_offset,offset);
            printk("2 : skip_len = 0x%x\n",skip_len);
            */
                                    
#if defined(CONFIG_USB_MTP_MAX_TRANSFER_SIZE_ENABLE) 
            if (skip_len > MTP_USB_MAX_TRANSFER_SIZE) 
                skip_len = MTP_USB_MAX_TRANSFER_SIZE;                                                                             
#else            
            if (skip_len > CONTEXT_BLOCK_SIZE)
                skip_len = CONTEXT_BLOCK_SIZE; 
#endif                
            
            ret = ptp_getobject_session(params,handle,NULL,skip_len,false); 
                        
            if (ret == PTP_RC_OK)
            {
                int len = params->obj_session.cur_offset - offset;
                
                if ((len <= 0) || (size > len))
                {
                    /*
                    printk("2-0 : Reset");
                    printk("2-0 : params->obj_session.cur_offset = 0x%x , offset = 0x%x, len = %d ,size = %d\n",params->obj_session.cur_offset,offset,len,size);    
                    */
                    goto err;
                } 
                
                len = mtpfs_ptp_get_data_from_session_buffer(params ,buf,offset,size);
                
                ptp_getobject_session(params,handle,buf,size,true);
                goto finish;
            }                  
                                                           
            if ((ret != PTP_RC_OK) && (ret != PTP_NRC_GETOBJECT))
            {                    
                ptp_getobject_session(params,handle,buf,size,true);
                // printk("2-1 : Reset\n");  
                goto err;
            }     
                       
            // if (skip_len == (offset - params->obj_session.cur_offset))
            if (((params->obj_session.cur_offset - ori_offset) < skip_len) || ((params->obj_session.cur_offset - ori_offset - skip_len) >= ptp_usb->inep_maxpacket))
            {      
                /* 
                printk("2-2 : Reset ---- \n");                          
                printk("2-2 : params->obj_session.cur_offset = 0x%x , ori_offset = 0x%x\n",params->obj_session.cur_offset,ori_offset);
                printk("2-2 : offset = 0x%x , skip_len = 0x%x\n",offset,skip_len);
                */
                ptp_getobject_session(params,handle,buf,size,true);                 
                goto err;
            } 
           
        }                             
    }    
        
    if (params->obj_session.cur_offset > offset)
    {
        int len;
        
        len = mtpfs_ptp_get_data_from_session_buffer(params ,buf,offset,size);
        // printk("3 : params->obj_session.cur_offset = 0x%x , offset = 0x%x ,len = 0x%x\n",params->obj_session.cur_offset,offset,len);
        
        
        if (len < 0)
        {    
            ptp_getobject_session(params,handle,buf,size,true); 
            goto err; 
        }    
        
        offset += len;
        size -= len;
        buf += len;
                
        if (size == 0)
        {                
            ret = PTP_RC_OK;
            goto finish;
        }                
    }       
   
    if (params->obj_session.cur_offset != offset)
    {
        printk("BUG : params->obj_session.cur_offset = 0x%x , offset = 0x%x\n",(u32) params->obj_session.cur_offset,(u32) offset);                   
        BUG_ON(1);                                       
    }       
    
    ret = ptp_getobject_session(params,handle,buf,size,false);      
    // printk("4 : ret = 0x%x\n",ret);
    
    if (ret == PTP_RC_OK)
    {
        ptp_getobject_session(params,handle,buf,size,true);
        goto finish;    
    }    
    
    if ((ret != PTP_RC_OK) && (ret != PTP_NRC_GETOBJECT))
    {                    
        ptp_getobject_session(params,handle,buf,size,true);  
        // printk("4 : Reset\n");     
    }    
       
finish :       
    up(&mtpfs_session_mutex);         
    return ret; 

err :     
    up(&mtpfs_session_mutex);
    return ret;            
} 

/*
bool mtpfs_is_mtp_device(struct mtpfs_sb_info *sb)
{
    PTPParams *params = mtpfs_mtp_device_get_params(sb); 
    return mtp_is_mtp_device(params);   
}
*/

bool mtpfs_is_high_speed_device(struct mtpfs_sb_info *sb)
{
    PTPParams *params = mtpfs_mtp_device_get_params(sb); 
    PTP_USB *ptp_usb;    
    
    if (params == NULL)
    {
        printk("%s [Error] : params is NULL\n",__func__);
        return PTP_ERROR_IO;    
    }        
          
    ptp_usb = (PTP_USB *) params->data; 
    
    if (ptp_usb->pusb_dev->speed == USB_SPEED_HIGH)
    {
        // printk("=========== High Speed ===========\n");    
        return true;
    }    
        
    return false;    
    
}

bool mtpfs_ptp_operation_issupported(struct mtpfs_sb_info *sb,uint16_t operation)  
{
    PTPParams *params = mtpfs_mtp_device_get_params(sb); 
    
    if (params == NULL)
    {
        printk("%s [Error] : params is NULL\n",__func__);
        return PTP_ERROR_IO;    
    }
    
    if (ptp_operation_issupported(params,operation) == 1)
        return true;
        
    return false;        
}
