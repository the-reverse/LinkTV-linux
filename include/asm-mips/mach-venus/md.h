#ifndef __KMD_H__
#define __KMD_H__

#include <linux/spinlock.h>
#include <linux/list.h>

typedef u64     MD_CMD_HANDLE;
typedef u64     MD_COPY_HANDLE;


typedef struct se_cmd_counter_t {
    u32 low;
    u32 high;
} md_cmd_counter;



typedef struct 
{
    unsigned char       flags;
    MD_CMD_HANDLE       request_id;
    struct list_head    list;
    dma_addr_t          dst;
    unsigned long       dst_len;    
    dma_addr_t          src;
    unsigned long       src_len;    
}MD_COMPLETION;

 
struct md_dev 
{    
    void*           CmdBuf;  /* Pointer to first quantum set */
    void*           CachedCmdBuf;
    void*           CmdBase;
    void*           CmdLimit;
    int             wrptr;    
    int             v_to_p_offset;
    int             size;
    md_cmd_counter  sw_counter;    
    
    spinlock_t	    lock;    
    unsigned long   spin_flags;        
    unsigned long   state;        
    
    struct list_head task_list;    
    struct list_head free_task_list;    
};



#if 1
#define DBG_PRINT(s, args...) printk(s, ## args)
#else
#define DBG_PRINT(s, args...)
#endif

//#define endian_swap_32(a) (((a)>>24) | (((a)>>8) & 0xFF00) | (((a)<<8) & 0xFF0000) | (((a)<<24) & 0xFF000000))
#define endian_swap_32(a) (a)


///////////////////////////////////////////////////////////////////////
// Low Level API
///////////////////////////////////////////////////////////////////////
MD_CMD_HANDLE md_write(const char *buf, size_t count);
bool md_checkfinish(MD_CMD_HANDLE handle);


///////////////////////////////////////////////////////////////////////
// Copy Request Management APIs
///////////////////////////////////////////////////////////////////////
int md_copy_sync(MD_COPY_HANDLE handle);


///////////////////////////////////////////////////////////////////////
// Asynchronous APIs
///////////////////////////////////////////////////////////////////////
MD_COPY_HANDLE md_copy_start(void* dst, void* src, int len, int dir);
MD_CMD_HANDLE  md_memset_start(void* lpDst, u8 data, int len);


///////////////////////////////////////////////////////////////////////
// Synchronous APIs
///////////////////////////////////////////////////////////////////////
int md_memcpy(void* lpDst, void* lpSrc, int len, bool forward);
int md_memset(void* lpDst, u8 data, int len);

///////////////////////////////////////////////////////////////////////
// MISC
///////////////////////////////////////////////////////////////////////
int md_is_valid_address(void* addr, unsigned long len);



#endif  // __KMD_H__
