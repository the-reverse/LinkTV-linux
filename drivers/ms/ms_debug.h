#define MS_DEBUG//////////////////////


#ifdef MS_DEBUG
#define msinfo(fmt, args...) \
           printk(KERN_INFO "%s(%d):" fmt, __func__ ,__LINE__,## args)
#else
#define msinfo(fmt, args...)
#endif

//#define SHOW_DEV_INFO_ENTRY