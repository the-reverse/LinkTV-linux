#ifndef __GCEC_RX_H__
#define __GCEC_RX_H__

typedef enum {
    GCEC_ST_START = 0,
    GCEC_ST_BIT7,
    GCEC_ST_BIT6,
    GCEC_ST_BIT5,
    GCEC_ST_BIT4,
    GCEC_ST_BIT3,
    GCEC_ST_BIT2,
    GCEC_ST_BIT1,
    GCEC_ST_BIT0,
    GCEC_ST_EOM,
    GCEC_ST_ACK,
    GCEC_ST_MAX,  
}GCEC_ST;        

typedef enum {
    GCEC_SST_WAIT_START = 0,    // wait bit start
    GCEC_SST_WAIT_HIGH,         // wait bit rising
}GCEC_SST;        


#define MIS_UMSK_ISR_GP0A   0xB801b040
#define MIS_UMSK_ISR_GP0DA  0xB801B054
#define MIS_GP0DIR			0xB801B100
#define MIS_GP0DATO		    0xB801B110
#define MIS_GP0DATI		    0xB801B120
#define MIS_GP0IE			0xB801B130
#define MIS_GP0DP			0xB801B140

int gcec_rx_enable(unsigned short mask, void (*callback)(unsigned char* buff, unsigned char len, void* priv), void* priv);

int gcec_rx_disable(void);

int gcec_rx_standby_read(unsigned short mask, unsigned char* pbuff, unsigned char buff_len);

#ifdef GCEC_DBG_EN
#define gcec_dbg(fmt, args...)       printk("[GCEC] Dbg, " fmt, ## args)
#else
#define gcec_dbg(fmt, args...)      
#endif

#define gcec_info(fmt, args...)      printk("[GCEC] Info, " fmt, ## args)
#define gcec_warning(fmt, args...)   printk("[GCEC] Warning, " fmt, ## args)

#endif  //__GCEC_RX_H__
