#ifndef __GCEC_FIXUP_H__
#define __GCEC_FIXUP_H__

typedef enum {
    GCEC_ST_START = 0,
    GCEC_ST_INIT3,
    GCEC_ST_INIT2,
    GCEC_ST_INIT1,
    GCEC_ST_INIT0,
    GCEC_ST_DEST3,
    GCEC_ST_DEST2,
    GCEC_ST_DEST1,
    GCEC_ST_DEST0,
    GCEC_ST_EOM,
    GCEC_ST_ACK,
    GCEC_ST_MAX,  
}GCEC_ST;        

typedef enum {
    GCEC_SST_WAIT_START = 0,
    GCEC_SST_WAIT_HIGH,
}GCEC_SST;        


#define MIS_UMSK_ISR_GP0A   0xB801b040
#define MIS_UMSK_ISR_GP0DA  0xB801B054
#define MIS_GP0DIR			0xB801B100
#define MIS_GP0DATO		    0xB801B110
#define MIS_GP0DATI		    0xB801B120
#define MIS_GP0IE			0xB801B130
#define MIS_GP0DP			0xB801B140

int gcec_fixup_enable(unsigned char addr);

int gcec_fixup_disable(void);


#ifdef GCEC_DBG_EN
#define gcec_dbg(fmt, args...)       printk("[GCEC] Dbg, " fmt, ## args)
#else
#define gcec_dbg(fmt, args...)      
#endif

#define gcec_info(fmt, args...)      printk("[GCEC] Info, " fmt, ## args)
#define gcec_warning(fmt, args...)   printk("[GCEC] Warning, " fmt, ## args)

#endif  //__GCEC_FIXUP_H__
