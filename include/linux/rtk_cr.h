/******************************************************************************
 * include/linux/rtk_cr.h
 * Overview: Realtek Card Reader Controller Register Map
 * Copyright (c) 2009 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2009-03-16 CMYu   create file
 *
 *******************************************************************************/
#ifndef __RTK_CR_H
#define __RTK_CR_H

#include <linux/config.h>
#include <linux/sched.h>
#include <linux/mtd/mtd.h>
#include <asm/r4kcache.h>

extern void rtk_dump_cr_data (__u8 *buf, int start, int size, __u8 *func_name, int line, int timing);
/*
define cr read/write NAND HW registers
use them because standard readb/writeb have warning msgs in our gcc 2.96
ex: passing arg 2 of `writeb' makes pointer from integer without a cast
*/
#define rtk_readb(offset)       (*(volatile unsigned char *)(offset))
#define rtk_readw(offset)       (*(volatile unsigned short *)(offset))
#define cr_readl(offset)        (*(volatile unsigned int *)(offset))

#define rtk_writeb(val, offset) (*(volatile unsigned char *)(offset) = val)
#define rtk_writew(val, offset) (*(volatile unsigned short *)(offset) = val)
#define cr_writel(val, offset)  (*(volatile unsigned int *)(offset) = val)

#define WaitTimeOut(task_state,msecs)   set_current_state(task_state); \
                        schedule_timeout((msecs) * HZ / 1000)

#define CLEAR_CR_CARD_STATUS(reg_addr)      \
do {                                \
    cr_writel(0xffffffff, reg_addr);    \
} while (0)


#define CLEAR_ALL_CR_CARD_STATUS        \
do {                                \
    int i = 0;  \
    for(i=0; i<5; i++)  \
        CLEAR_CR_CARD_STATUS(0xb8010820+ i*4);  \
} while (0)

/*
    macro CR_FLUSH_CACHE is to flush the cache at address "addr",
    the length is "len"
*/
#define CR_FLUSH_CACHE(addr, len)       \
do {                                \
    unsigned long dc_lsize = current_cpu_data.dcache.linesz;    \
    unsigned long end, a;                               \
    a = (unsigned long ) addr & ~(dc_lsize - 1);        \
    end = ((unsigned long )addr + len - 1) & ~(dc_lsize - 1);   \
    while (1) {     \
        flush_dcache_line(a);   /* Hit_Writeback_Inv_D */   \
        if (a == end)   \
            break;  \
        a += dc_lsize;  \
    }   \
} while (0)


#define LONG2CHAR(num,i)    *((unsigned char *)&(num) + (i))
#define SHORT2CHAR(num,i)   *((unsigned char *)&(num) + (i))

#define ASSIGN_SHORT(num,hbyte,lbyte)   *((unsigned char *)&(num)) = (lbyte); \
                                        *((unsigned char *)&(num)+1) = (hbyte);
#define ASSIGN_LONG(num,byte3,byte2,byte1,byte0)    *((unsigned char *)&(num))  = (byte0); \
                                                    *((unsigned char *)&(num)+1) = (byte1); \
                                                    *((unsigned char *)&(num)+2) = (byte2); \
                                                    *((unsigned char *)&(num)+3) = (byte3);

// card operation type
#define READ_OP     0x00
#define WRITE_OP    0x01
#define CMP_OP      0x02


//CR INT STATUS
#define CIS_WRITE_PROTECTION    1<<5
#define CIS_CARD_DETECTION      1<<4
#define CIS_DECODE_ERROR        1<<2
#define CIS_DECODE_FINISH       1<<1
#define CIS_CARD_STATUS_CHANGE  1<<0

//CR DMS STATUS
#define CDS_DRQ_TMTOUT          1<<5
#define CDS_DES_MOD_FINISH      1<<4
#define CDS_FIFO_EMPTY          1<<3
#define CDS_FIFO_FULL           1<<2
#define CDS_DMA_ERROR           1<<1
#define CDS_DMA_FINISH          1<<0

/* Card clock */
#define CLOCK_SELECT_20     0x0
#define CLOCK_SELECT_24     0x1
#define CLOCK_SELECT_30     0x2         // default value
#define CLOCK_SELECT_40     0x3
#define CLOCK_SELECT_48     0x4
#define CLOCK_SELECT_60     0x5
#define CLOCK_SELECT_80     0x6
#define CLOCK_SELECT_96     0x7
#define CLOCK_SELECT_SSC    0x8

/*  ===== macros and funcitons for Realtek CR ===== */
/* for SD/MMC usage */
#define ON_SD               0x10
#define ON_MS               0x20
#define ON                  0
#define OFF                 1
#define HIGH              0
#define LOW               1
#define SUPPORT_VOLTAGE     0x003C0000
#define SD_RESP_TIMEOUT     3   /* time to wait for response */
#define CMD_DECODE_ERR      1<<2
#define CMD_DECODE_OK       1<<1

// SD v1.1 CMD6 SWITCH function
#define SD_CHECK_MODE       0x00
#define SD_SWITCH_MODE      0x80
#define SD_CHECK_SPEC_V1_1  0xFF

//MMC v4.0
#define MMC_52MHZ_SPEED         0x01
#define MMC_26MHZ_SPEED         0x02
#define NORMAL_SPEED            0x00
#define MMC_8BIT_BUS            0x10
#define MMC_4BIT_BUS            0x20

#define SD_NORMALSPEED_MODE     0x00
#define SD_HIGHSPEED_MODE       0x01

#define SD_1BIT_MODE            0x00
#define SD_4BIT_MODE            0x10
#define SD_8BIT_MODE            0x20

#define SAMPLE_HIGH             0x04
#define SAMPLE_NORMAL           0x02

#define NO_WAIT     0x00
#define WAIT_1FF    0x20
#define WAIT_3FF    0x40
#define WAIT_7FF    0x60
#define WAIT_FFF    0x80
#define WAIT_3FFF   0xA0
#define WAIT_7FFF   0xC0
#define WAIT_FFFF   0xC0

#define WAIT_BUSY   0x10

#define SD_READ_TIMEOUT     10000
#define SD_WRITE_TIMEOUT    10000

/* for MS/MSPRO usage */
/* Device Type definition */
#define TYPE_NONE       0
#define TYPE_MS         1
#define TYPE_MSPRO      2

#define CR_NON      0
#define CR_SD       (1<<0)          /* device is a SD card */
#define CR_SDHC     (1<<1)          /* device is a SDHC card */
#define CR_MMC      (1<<2)          /* device is a MMC card */
#define CR_MS       (1<<3)          /* device is a Memory Stick card */
#define CR_MS_PRO   (1<<4)          /* device is a Memory Stick Pro card */

/*  ===== MSPRO TPCs ===== */
#define MSPRO_TPC_READ_LONG_DATA    0x02
#define MSPRO_TPC_READ_SHORT_DATA   0x03
#define MSPRO_TPC_READ_REG          0x04
#define MSPRO_TPC_GET_INT           0x07
#define MSPRO_TPC_WRITE_LONG_DATA   0x0D
#define MSPRO_TPC_WRITE_SHORT_DATA  0x0C
#define MSPRO_TPC_WRITE_REG         0x0B
#define MSPRO_TPC_SET_RW_REG_ADRS   0x08
#define MSPRO_TPC_SET_CMD           0x0E
#define MSPRO_TPC_EX_SET_CMD        0x09

// Registers inside MS
#define MS_REG_IntReg           0x01
#define MS_REG_StatusReg0       0x02
#define MS_REG_StatusReg1       0x03

#define MS_REG_SystemParm       0x10
#define MS_REG_BlockAdrs        0x11
#define MS_REG_CMDParm          0x14
#define MS_REG_PageAdrs         0x15

#define MS_REG_OverwriteFlag    0x16
#define MS_REG_ManagemenFlag    0x17
#define MS_REG_LogicalAdrs      0x18
#define MS_REG_ReserveArea      0x1A

// Registers inside MSPRO
#define MSPRO_REG_INT           0x01
#define MSPRO_REG_Status        0x02

#define MSPRO_REG_Type          0x04
#define MSPRO_REG_Catagory      0x06
#define MSPRO_REG_Class         0x07

#define MSPRO_REG_SystemParm    0x10
#define MSPRO_REG_DataCount1    0x11
#define MSPRO_REG_DataCount0    0x12
#define MSPRO_REG_DataAddr3     0x13
#define MSPRO_REG_DataAddr2     0x14
#define MSPRO_REG_DataAddr1     0x15
#define MSPRO_REG_DataAddr0     0x16

#define MSPRO_REG_TPCParm       0x17
#define MSPRO_REG_CMDParm       0x18

//++ CMD over Memory Stick
// Flash CMD
#define MS_CMD_BLOCK_READ   0xAA
#define MS_CMD_BLOCK_WRITE  0x55
#define MS_CMD_BLOCK_END    0x33
#define MS_CMD_BLOCK_ERASE  0x99
#define MS_CMD_FLASH_STOP   0xCC

// Function CMD
#define MS_CMD_SLEEP        0x5A
#define MS_CMD_CLEAR_BUF    0xC3
#define MS_CMD_RESET        0x3C
//-- CMD over Memory Stick

//++ CMD over Memory Stick Pro
// Flash CMD
#define MSPRO_CMD_READ_DATA     0x20
#define MSPRO_CMD_WRITE_DATA    0x21
#define MSPRO_CMD_READ_ATRB     0x24
#define MSPRO_CMD_STOP          0x25
#define MSPRO_CMD_ERASE         0x26
#define MSPRO_CMD_SET_IBD       0x46
#define MSPRO_CMD_GET_IBD       0x47

// Function CMD
#define MSPRO_CMD_FORMAT    0x10
#define MSPRO_CMD_SLEEP     0x11

// INT signal
#define INT_CED             0x01
#define INT_ERR             0x02
#define INT_BREQ            0x04
#define INT_CMDNK           0x08

#define REG_INT_CED         0x80
#define REG_INT_ERR         0x40
#define REG_INT_BREQ        0x20
#define REG_INT_CMDNK       0x01

#define MS_MAX_RETRY_COUNT  20
#define ERASE_TIMEOUT       2000
#define USUAL_TIMEOUT       2000
#define WRITE_TIMEOUT       2000
#define READ_TIMEOUT        2000

#define PRO_READ_TIMEOUT    5000
#define PRO_WRITE_TIMEOUT   5000

// define for Pro_SystemParm Register
#define PARELLEL_IF         0x00
//#define PARELLEL_IF         0x80
#define SERIAL_IF           0x80

#define WP_FLAG             0x01
#define SLEEP_FLAG          0x02
#define ES_FLAG             0x80

#define MS_4_BIT_MODE       0x01

// define for StatusReg0 Register
#define BUF_FULL            0x10
#define BUF_EMPTY           0x20

// define for StatusReg1 Register
#define MEDIA_BUSY          0x80
#define FLASH_BUSY          0x40
#define DATA_ERROR          0x20
#define STS_UCDT            0x10
#define EXTRA_ERROR         0x08
#define STS_UCEX            0x04
#define FLAG_ERROR          0x02
#define STS_UCFG            0x01

// Transfer Protocol Command
#define MS_TPC_READ_PAGE_DATA   0x02
#define MS_TPC_READ_REG         0x04
#define MS_TPC_GET_INT          0x07
#define MS_TPC_WRITE_PAGE_DATA  0x0D
#define MS_TPC_WRITE_REG        0x0B
#define MS_TPC_SET_RW_REG_ADRS  0x08
#define MS_TPC_SET_CMD          0x0E

#define MS_EXTRA_SIZE           0x9
#define MS_MAX_UNUSED           0x10
//Max. number of defective block in one segment
#define MAX_DEFECTIVE_BLOCK     10

// define for OverwriteFlag Register
#define BLOCK_BOOT  0xC0
#define BLOCK_OK    0x80
#define PAGE_OK     0x60
#define DATA_COMPL  0x10

// define for ManagemenFlag Register
#define NOT_BOOT_BLOCK          0x4
#define NOT_TRANSLATION_TABLE   0x8

// Header
#define HEADER_ID0          0
#define HEADER_ID1          1
// System Entry
#define DISABLED_BLOCK0     0x170 + 4
#define DISABLED_BLOCK1     0x170 + 5
#define DISABLED_BLOCK2     0x170 + 6
#define DISABLED_BLOCK3     0x170 + 7
// Boot & Attribute Information
#define BLOCK_SIZE_0        0x1a0 + 2
#define BLOCK_SIZE_1        0x1a0 + 3
#define BLOCK_COUNT_0       0x1a0 + 4
#define BLOCK_COUNT_1       0x1a0 + 5
#define EBLOCK_COUNT_0      0x1a0 + 6
#define EBLOCK_COUNT_1      0x1a0 + 7
#define PAGE_SIZE_0         0x1a0 + 8
#define PAGE_SIZE_1         0x1a0 + 9
#define MS_Device_Type      0x1D8
#define MS_4bit_Support     0x1D3

#define setPS_NG            1
#define setPS_Error         0

#define SAMPLE_FALLING      0x80
#define SAMPLE_RISING       0x00

#define PUSH_TIME           0x40

//#define MS_BUSY_TIME      0x02
#define MS_BUSY_TIME        0x00

#define WAIT_INT            0x80
#define RW_AUTO_GET_INT     0x40
#define NOT_CHECK_INT       0x20

#define SEND_BLOCK_END      0x20

/* ===== MMC/SD Responses Parameters ===== */
// SD response type
#define SD_R0           0x04
#define SD_R1           0x05
#define SD_R1b          0x0D
#define SD_R2           0x06
#define SD_R3           0x01
#define SD_R6           0x05
#define SD_R7           0x05    //CMYu adds it.

#define CHECK_CRC7      0x04
#define CHECK_CRC16     0x40
//#define CHECK_CRC16   0x00
#define NOT_CALC_CRC7   0x80

// clock divider
#define DIVIDE_NON      0x00
#define DIVIDE_256      0x10
#define DIVIDE_128      0x20


/* ===== Realtek Card Reader Command Sets ===== */
#define CARD_POLLING            0xC000

#define CARD_STOP_SD            0x8304
#define CARD_STOP_MS            0x8308
#define CARD_STOP_HOST          0x830C

#define CARD_OUTPUTENABLE_SD    0x8404
#define CARD_OUTPUTENABLE_MS    0x8408

#define SD_POWERON          0x5F00
#define SD_SETPARAMETER     0x5100
#define SD_SENDCMDGETRSP    0x4800
#define SD_NORMALREAD       0x4C00
#define SD_NORMALWRITE      0x4000
#define SD_AUTOREAD1        0x4D00
#define SD_AUTOREAD2        0x4E00
#define SD_AUTOREAD3        0x4500
#define SD_AUTOREAD4        0x4600
#define SD_AUTOWRITE1       0x4900
#define SD_AUTOWRITE2       0x4A00
#define SD_AUTOWRITE3       0x4100
#define SD_AUTOWRITE4       0x4200
#define SD_REGISTERCHECK    0x5200
#define SD_GETSTATUS        SD_REGISTERCHECK

#define MS_POWERON          0x7F00
#define MS_SETPARAMETER     0x7100
#define MS_GETSTATUS        0x7200
#define MS_GETSECTORNUM     0x7300
#define MS_SETCMD           0x6600
#define MS_COPYPAGE         0x6700
#define MS_READBYTES        0x6000
#define MS_WRITEBYTES       0x6400
#define MS_NORMALREAD       0x6100
#define MS_NORMALWRITE      0x6500
#define MS_AUTOREAD         0x6800
#define MS_AUTOWRITE        0x6C00
#define MS_MULTIREAD        0x6200
#define MS_MULTIWRITE       0x6300
#define MS_REGISTERCHECK    0x7400


/* ===== Realtek Card Reader Register Sets ===== */
#define CR_BASE_ADDR        0xb8010800
#define CR_DMA_CTRL         ( CR_BASE_ADDR  + 0 )           //0xb8010800
#define CR_DMA_STATUS       ( CR_BASE_ADDR  + 0x4 )         //0xb8010804
#define CR_DMA_PTR_STATUS   ( CR_BASE_ADDR  + 0x8 )         //0xb8010808
#define CR_DMA_IN_ADDR      ( CR_BASE_ADDR  + 0xc )         //0xb801080c
#define CR_DMA_OUT_ADDR     ( CR_BASE_ADDR  + 0x10 )        //0xb8010810
#define CR_INT_STATUS       ( CR_BASE_ADDR  + 0x14 )        //0xb8010814
#define CR_INT_MASK         ( CR_BASE_ADDR  + 0x18 )        //0xb8010818
#define CR_REG_MODE_CTR     ( CR_BASE_ADDR  + 0x1c )        //0xb801081c

#define CR_CARD_STATUS_0    ( CR_BASE_ADDR  + 0x20 )        //0xb8010820
#define CR_CARD_STATUS_1    ( CR_BASE_ADDR  + 0x24 )        //0xb8010824
#define CR_CARD_STATUS_2    ( CR_BASE_ADDR  + 0x28 )        //0xb8010828
#define CR_CARD_STATUS_3    ( CR_BASE_ADDR  + 0x2c )        //0xb801082c
#define CR_CARD_STATUS_4    ( CR_BASE_ADDR  + 0x30 )        //0xb8010830

#define CR_REG_CMD_1        ( CR_BASE_ADDR  + 0x34 )        //0xb8010834
#define CR_REG_CMD_2        ( CR_BASE_ADDR  + 0x38 )        //0xb8010838
#define CR_REG_CMD_3        ( CR_BASE_ADDR  + 0x3c )        //0xb801083c
#define CR_REG_CMD_4        ( CR_BASE_ADDR  + 0x40 )        //0xb8010840
#define CR_REG_CMD_5        ( CR_BASE_ADDR  + 0x44 )        //0xb8010844
#define CR_REG_CMD_6        ( CR_BASE_ADDR  + 0x48 )        //0xb8010848
#define CR_REG_CMD_7        ( CR_BASE_ADDR  + 0x4c )        //0xb801084c
#define CR_REG_CMD_8        ( CR_BASE_ADDR  + 0x50 )        //0xb8010850
#define CR_REG_CMD_9        ( CR_BASE_ADDR  + 0x54 )        //0xb8010854
#define CR_REG_CMD_10       ( CR_BASE_ADDR  + 0x58 )        //0xb8010858
#define CR_REG_CMD_11       ( CR_BASE_ADDR  + 0x5c )        //0xb801085c
#define CR_REG_CMD_12       ( CR_BASE_ADDR  + 0x60 )        //0xb8010860
#define CR_REG_CMD_13       ( CR_BASE_ADDR  + 0x64 )        //0xb8010864
#define CR_REG_CMD_14       ( CR_BASE_ADDR  + 0x68 )        //0xb8010868
#define CR_REG_CMD_15       ( CR_BASE_ADDR  + 0x6c )        //0xb801086c
#define CR_REG_CMD_16       ( CR_BASE_ADDR  + 0x70 )        //0xb8010870
#define CR_REG_CMD_17       ( CR_BASE_ADDR  + 0x74 )        //0xb8010874
#define CR_REG_CMD_18       ( CR_BASE_ADDR  + 0x78 )        //0xb8010878
#define CR_REG_CMD_19       ( CR_BASE_ADDR  + 0x7c )        //0xb801087c
#define CR_REG_CMD_20       ( CR_BASE_ADDR  + 0x80 )        //0xb8010880

#define CR_DES_MODE_CTRL    ( CR_BASE_ADDR  + 0x84 )        //0xb8010884
#define DES_CMD_WR_PORT     ( CR_BASE_ADDR  + 0x88 )        //0xb8010888
#define CR_SIE_TIMING_ADJUST    ( CR_BASE_ADDR  + 0x8c )    //0xb801088c

#define RCA_SHIFTER 16

#define NORMAL_READ_BUF_SIZE   512      //no matter FPGA & QA

#define SD_CARD_WP    (1<<5)
#define MS_CARD_EXIST (1<<3)
#define SD_CARD_EXIST (1<<2)
//alex add at 2010/6/23*****
/*
#define R_W_CMD 2   //read/write command
#define INN_CMD 1   //command work chip inside
#define OTT_CMD 0   //command send to card

#define RTK_RMOV 2
#define RTK_TOUT 1
#define RTK_SUCC 0
*/
//extern spinlock_t		rtk_cr_lock;		/* rtk hardware register locker between MMC & MS_PRO */
//alex add at 2010/6/23&&&&&

#endif  /* __RTK_CR_H */
