
#ifndef __OSDEP_SERVICE_H_
#define __OSDEP_SERVICE_H_

#include <drv_conf.h>
#include <basic_types.h>
//#include <rtl871x_byteorder.h>

#define _SUCCESS	1
#define _FAIL		0

#undef _TRUE
#define _TRUE		1

#undef _FALSE
#define _FALSE		0


#ifdef PLATFORM_LINUX
	#include <linux/version.h>
	#include <linux/spinlock.h>
	#include <linux/compiler.h>
	#include <linux/kernel.h>
	#include <linux/errno.h>
	#include <linux/init.h>
	#include <linux/slab.h>
	#include <linux/module.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,5))
	#include <linux/kref.h>
#endif
	#include <linux/smp_lock.h>
	#include <linux/netdevice.h>
	#include <linux/skbuff.h>
	#include <linux/circ_buf.h>
	#include <asm/uaccess.h>
	#include <asm/byteorder.h>
	#include <asm/atomic.h>
	#include <asm/io.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26))
	#include <asm/semaphore.h>
#else
	#include <linux/semaphore.h>
#endif
	#include <linux/sem.h>
	#include <linux/sched.h>
	#include <linux/etherdevice.h>
	#include <linux/wireless.h>
	#include <net/iw_handler.h>
	#include <linux/if_arp.h>
	#include <linux/rtnetlink.h>
	#include <linux/delay.h>
	#include <linux/proc_fs.h>	// Necessary because we use the proc fs

#ifdef CONFIG_RTL8712_TCP_CSUM_OFFLOAD_TX
	#include <linux/in.h>
	#include <linux/ip.h>
	#include <linux/udp.h>
#endif

#ifdef CONFIG_USB_HCI
	#include <linux/usb.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21))
	#include <linux/usb_ch9.h>
#else
	#include <linux/usb/ch9.h>
#endif
#endif

#ifdef CONFIG_SDIO_HCI
	#include <linux/mmc/sdio_func.h>
	#include <linux/mmc/sdio_ids.h>
#endif

#ifdef CONFIG_PCI_HCI
	#include <linux/pci.h>
#endif

	
#ifdef CONFIG_USB_HCI
	typedef struct urb *  PURB;
#endif

	typedef struct 	semaphore _sema;
	typedef	spinlock_t	_lock;
        typedef struct semaphore	_rwlock;
	typedef struct timer_list _timer;

	struct	__queue	{
		struct	list_head	queue;	
		_lock	lock;
	};

	typedef	struct sk_buff	_pkt;
	typedef unsigned char	_buffer;
	
	typedef struct	__queue	_queue;
	typedef struct	list_head	_list;
	typedef	int	_OS_STATUS;
	//typedef u32	_irqL;
	typedef unsigned long _irqL;
	typedef	struct	net_device * _nic_hdl;
	
	typedef pid_t		_thread_hdl_;
	typedef int		thread_return;
	typedef void*	thread_context;

	#define thread_exit() complete_and_exit(NULL, 0)

	typedef void timer_hdl_return;
	typedef void* timer_hdl_context;
	typedef struct work_struct _workitem;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
	#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,22))
	#define skb_tail_pointer(skb)	skb->tail
#endif

__inline static _list *get_next(_list	*list)
{
	return list->next;
}	

__inline static _list	*get_list_head(_queue	*queue)
{
	return (&(queue->queue));
}

	
#define LIST_CONTAINOR(ptr, type, member) \
        ((type *)((char *)(ptr)-(SIZE_T)(&((type *)0)->member)))	

        
__inline static void _enter_critical(_lock *plock, _irqL *pirqL)
{
	spin_lock_irqsave(plock, *pirqL);
}

__inline static void _exit_critical(_lock *plock, _irqL *pirqL)
{
	spin_unlock_irqrestore(plock, *pirqL);
}

__inline static void _enter_critical_ex(_lock *plock, _irqL *pirqL)
{
	spin_lock_irqsave(plock, *pirqL);
}

__inline static void _exit_critical_ex(_lock *plock, _irqL *pirqL)
{
	spin_unlock_irqrestore(plock, *pirqL);
}

__inline static void _enter_critical_bh(_lock *plock, _irqL *pirqL)
{
	spin_lock_bh(plock);
}

__inline static void _exit_critical_bh(_lock *plock, _irqL *pirqL)
{
	spin_unlock_bh(plock);
}

__inline static void _enter_hwio_critical(_rwlock *prwlock, _irqL *pirqL)
{
	down(prwlock);
}


__inline static void _exit_hwio_critical(_rwlock *prwlock, _irqL *pirqL)
{
	up(prwlock);
}

__inline static void list_delete(_list *plist)
{
	list_del_init(plist);
}

__inline static void _init_timer(_timer *ptimer,_nic_hdl padapter,void *pfunc,void* cntx)
{
	//setup_timer(ptimer, pfunc,(u32)cntx);	
	ptimer->function = pfunc;
	ptimer->data = (u32)cntx;
	init_timer(ptimer);
}

__inline static void _set_timer(_timer *ptimer,u32 delay_time)
{	
	mod_timer(ptimer , (jiffies+(delay_time*HZ/1000)));	
}

__inline static void _cancel_timer(_timer *ptimer,u8 *bcancelled)
{
	del_timer_sync(ptimer); 	
	*bcancelled=  _TRUE;//TRUE ==1; FALSE==0
}

__inline static void _init_workitem(_workitem *pwork, void *pfunc, PVOID cntx)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20))
	INIT_WORK(pwork, pfunc);
#else
	INIT_WORK(pwork, pfunc,pwork);
#endif
}

__inline static void _set_workitem(_workitem *pwork)
{
	schedule_work(pwork);
}

//
// Global Mutex: can only be used at PASSIVE level.
//

#define ACQUIRE_GLOBAL_MUTEX(_MutexCounter)                              \
{                                                               \
	while (atomic_inc_return((atomic_t *)&(_MutexCounter)) != 1)\
	{                                                           \
		atomic_dec((atomic_t *)&(_MutexCounter));        \
		msleep(10);                          \
	}                                                           \
}

#define RELEASE_GLOBAL_MUTEX(_MutexCounter)                              \
{                                                               \
	atomic_dec((atomic_t *)&(_MutexCounter));        \
}

#endif	


#ifdef PLATFORM_OS_XP

	#include <ndis.h>
	#include <ntddk.h>
	#include <ntddsd.h>
	#include <ntddndis.h>
	#include <ntdef.h>

#ifdef CONFIG_USB_HCI
	#include <usb.h>
	#include <usbioctl.h>
	#include <usbdlib.h>
#endif

	typedef KSEMAPHORE 	_sema;
	typedef	LIST_ENTRY	_list;
	typedef NDIS_STATUS _OS_STATUS;
	

	typedef NDIS_SPIN_LOCK	_lock;

	typedef KMUTEX 			_rwlock;

	typedef KIRQL	_irqL;

	// USB_PIPE for WINCE , but handle can be use just integer under windows
	typedef NDIS_HANDLE  _nic_hdl;


	typedef NDIS_MINIPORT_TIMER    _timer;

	struct	__queue	{
		LIST_ENTRY	queue;	
		_lock	lock;
	};

	typedef	NDIS_PACKET	_pkt;
	typedef NDIS_BUFFER	_buffer;
	typedef struct	__queue	_queue;
	
	typedef PKTHREAD _thread_hdl_;
	typedef void	thread_return;
	typedef void* thread_context;

	typedef NDIS_WORK_ITEM _workitem;

	#define thread_exit() PsTerminateSystemThread(STATUS_SUCCESS);

	#define HZ			10000000
	#define SEMA_UPBND	(0x7FFFFFFF)   //8192
	
__inline static _list *get_next(_list	*list)
{
	return list->Flink;
}	

__inline static _list	*get_list_head(_queue	*queue)
{
	return (&(queue->queue));
}
	

#define LIST_CONTAINOR(ptr, type, member) CONTAINING_RECORD(ptr, type, member)
     

__inline static _enter_critical(_lock *plock, _irqL *pirqL)
{
	NdisAcquireSpinLock(plock);	
}

__inline static _exit_critical(_lock *plock, _irqL *pirqL)
{
	NdisReleaseSpinLock(plock);	
}


__inline static _enter_critical_ex(_lock *plock, _irqL *pirqL)
{
	NdisDprAcquireSpinLock(plock);	
}

__inline static _exit_critical_ex(_lock *plock, _irqL *pirqL)
{
	NdisDprReleaseSpinLock(plock);	
}

__inline static void _enter_critical_bh(_lock *plock, _irqL *pirqL)
{
	NdisDprAcquireSpinLock(plock);
}

__inline static void _exit_critical_bh(_lock *plock, _irqL *pirqL)
{
	NdisDprReleaseSpinLock(plock);
}

__inline static _enter_hwio_critical(_rwlock *prwlock, _irqL *pirqL)
{
	KeWaitForSingleObject(prwlock, Executive, KernelMode, FALSE, NULL);
}


__inline static _exit_hwio_critical(_rwlock *prwlock, _irqL *pirqL)
{
	KeReleaseMutex(prwlock, FALSE);
}


__inline static void list_delete(_list *plist)
{
	RemoveEntryList(plist);
	InitializeListHead(plist);	
}

__inline static void _init_timer(_timer *ptimer,_nic_hdl padapter,void *pfunc,PVOID cntx)
{
	NdisMInitializeTimer(ptimer, padapter, pfunc, cntx);
}

__inline static void _set_timer(_timer *ptimer,u32 delay_time)
{	
 	NdisMSetTimer(ptimer,delay_time);	
}

__inline static void _cancel_timer(_timer *ptimer,u8 *bcancelled)
{
	NdisMCancelTimer(ptimer,bcancelled);
}

__inline static void _init_workitem(_workitem *pwork, void *pfunc, PVOID cntx)
{

	NdisInitializeWorkItem(pwork, pfunc, cntx);
}

__inline static void _set_workitem(_workitem *pwork)
{
	NdisScheduleWorkItem(pwork);
}


#define ATOMIC_INIT(i)  { (i) }

//
// Global Mutex: can only be used at PASSIVE level.
//

#define ACQUIRE_GLOBAL_MUTEX(_MutexCounter)                              \
{                                                               \
    while (NdisInterlockedIncrement((PULONG)&(_MutexCounter)) != 1)\
    {                                                           \
        NdisInterlockedDecrement((PULONG)&(_MutexCounter));        \
        NdisMSleep(10000);                          \
    }                                                           \
}

#define RELEASE_GLOBAL_MUTEX(_MutexCounter)                              \
{                                                               \
    NdisInterlockedDecrement((PULONG)&(_MutexCounter));              \
}

#endif


#ifdef PLATFORM_OS_CE
#include <osdep_ce_service.h>
#endif

#include <rtw_byteorder.h>

#ifndef BIT
	#define BIT(x)	( 1 << (x))
#endif

extern u8*	_malloc(u32 sz);
extern void	_mfree(u8 *pbuf, u32 sz);
extern void	_memcpy(void* dec, void* sour, u32 sz);
extern int	_memcmp(void *dst, void *src, u32 sz);
extern void	_memset(void *pbuf, int c, u32 sz);

extern void	_init_listhead(_list *list);
extern u32	is_list_empty(_list *phead);
extern void	list_insert_tail(_list *plist, _list *phead);
extern void	list_delete(_list *plist);
extern void	_init_sema(_sema *sema, int init_val);
extern void	_free_sema(_sema	*sema);
extern void	_up_sema(_sema	*sema);
extern u32	_down_sema(_sema *sema);
extern void	_rwlock_init(_rwlock *prwlock);
extern void	_spinlock_init(_lock *plock);
extern void	_spinlock_free(_lock *plock);
extern void	_spinlock(_lock	*plock);
extern void	_spinunlock(_lock	*plock);
extern void	_spinlock_ex(_lock	*plock);
extern void	_spinunlock_ex(_lock	*plock);
extern void	_init_queue(_queue	*pqueue);
extern u32	_queue_empty(_queue	*pqueue);
extern u32	end_of_queue_search(_list *queue, _list *pelement);
extern u32	get_current_time(void);

extern void	sleep_schedulable(int ms);

extern void	msleep_os(int ms);
extern void	usleep_os(int us);
extern void	mdelay_os(int ms);
extern void	udelay_os(int us);



__inline static unsigned char _cancel_timer_ex(_timer *ptimer)
{
#ifdef PLATFORM_LINUX
	return del_timer_sync(ptimer);
#endif

#ifdef PLATFORM_WINDOWS
	u8 bool;
	
	_cancel_timer(ptimer, &bool);
	
	return bool;
#endif
}

__inline static void thread_enter(void *context)
{
#ifdef PLATFORM_LINUX
	//struct net_device *pnetdev = (struct net_device *)context;
	//daemonize("%s", pnetdev->name);
	daemonize("%s", "RTKTHREAD");
	allow_signal(SIGTERM);
#endif
}

__inline static void flush_signals_thread(void) 
{
#ifdef PLATFORM_LINUX
	if (signal_pending (current)) 
	{
		flush_signals(current);
	}
#endif
}

__inline static _OS_STATUS res_to_status(sint res)
{


#if defined (PLATFORM_LINUX) || defined (PLATFORM_MPIXEL)
	return res;
#endif

#ifdef PLATFORM_WINDOWS

	if (res == _SUCCESS)
		return NDIS_STATUS_SUCCESS;
	else
		return NDIS_STATUS_FAILURE;

#endif	
	
}

__inline static u32 _RND4(u32 sz)
{

	u32	val;

	val = ((sz >> 2) + ((sz & 3) ? 1: 0)) << 2;
	
	return val;

}

__inline static u32 _RND8(u32 sz)
{

	u32	val;

	val = ((sz >> 3) + ((sz & 7) ? 1: 0)) << 3;
	
	return val;

}

__inline static u32 _RND128(u32 sz)
{

	u32	val;

	val = ((sz >> 7) + ((sz & 127) ? 1: 0)) << 7;
	
	return val;

}

__inline static u32 _RND256(u32 sz)
{

	u32	val;

	val = ((sz >> 8) + ((sz & 255) ? 1: 0)) << 8;
	
	return val;

}

__inline static u32 _RND512(u32 sz)
{

	u32	val;

	val = ((sz >> 9) + ((sz & 511) ? 1: 0)) << 9;
	
	return val;

}

__inline static u32 bitshift(u32 bitmask)
{
	u32 i;

	for (i = 0; i <= 31; i++)
		if (((bitmask>>i) &  0x1) == 1) break;

	return i;
}

#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

//#ifdef __GNUC__
#ifdef PLATFORM_LINUX
#define STRUCT_PACKED __attribute__ ((packed))
#else
#define STRUCT_PACKED
#endif


#endif

