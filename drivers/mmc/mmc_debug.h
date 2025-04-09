//#define MMC_DEBUG//////////////////////
#ifdef MMC_DEBUG
#define mmcinfo(fmt, args...) \
           printk(KERN_INFO "mmc:%s(%d):" fmt, __func__ ,__LINE__,## args)
#else
#define mmcinfo(fmt, args...)
#endif

//#define MMC_SPEC//////////////////////
#ifdef MMC_SPEC
#define mmcspec(fmt, args...) \
           printk(KERN_INFO "mmc:%s(%d):" fmt, __func__ ,__LINE__,## args)
#else
#define mmcspec(fmt, args...)
#endif

//#define MMC_MSG1//////////////////////
#ifdef MMC_MSG1
#define mmcmsg1(fmt, args...) \
           printk(KERN_INFO "mmc:%s(%d):" fmt, __func__ ,__LINE__,## args)
#else
#define mmcmsg1(fmt, args...)
#endif

//#define SHOW_CSD
//#define SHOW_EXT_CSD
//#define SHOW_SWITCH_DATA
#define SHOW_CID
//#define SHOW_SCR
//#define SHOW_MS_RSP
//#define SHOW_MMC_PRD

#define TEST_DSC_MOD
//#define MEM_COPY_METHOD

//#define TEST_POWER_RESCYCLE   /* for test re_initial fow */
//#define HIGH_DRIVING          /* pull high driving force */
