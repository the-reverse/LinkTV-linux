/******************************************************************************
 * $Id: rtk_nand.c 264228 2009-08-31 11:16:32Z ken.yu $
 * linux/drivers/mmc/rtk_sd.h
 * Overview: Realtek SD/MMC driver
 * Copyright (c) 2009 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2009-02-05 CMYu   create file
 *
 *******************************************************************************/

/*
struct ms_info {
	char write_protect_flag;
	unsigned int capacity;

	unsigned char   MSProSequentialFlag;
	unsigned int    MSProPreAddr;
	unsigned char   MSProPreOP;
	unsigned char   MSProINT;

};
*/
struct sd_cmd_pkt {

    struct mmc_host     *mmc;       /* MMC structure */
    struct mmc_command  *cmd;
    struct mmc_data     *data;

    unsigned char       *dma_buffer;
    u16                 byte_count;
    u16                 block_count;

    u8                  rsp_para;
    u8                  rsp_len;
    u32                 flags;
    u32                 timeout;
};

struct ms_cmd_pkt {
    struct mmc_host *mmc;       /* MMC structure */
    unsigned int flags;
    u8 TPC;
    u8 option;
    u8 reg_byte_count;
    unsigned char *status;
    u16 sector_count;
    u32 timeout;
    unsigned char *dma_buffer;
    struct mmc_data *data;
};

struct rtksd_port {
    struct mmc_host *mmc;           /* MMC structure */
    spinlock_t * rtlock;                /* Mutex */

    int rtflags;                      /* Driver states */
#define RTKSD_FCARD_DETECTED    (1<<0)      /* Card is detected */
#define RTKSD_FIGNORE_DETECT    (1<<1)      /* Ignore card detection */
#define RTKSD_FCARD_POWER       (1<<2)      /* Card is power on */
#define RTKSD_FHOST_POWER       (1<<3)      /* Host is power on */
#define RTKSD_BUS_BIT4          (1<<4)      /* 4-bit bus width */
#define RTKSD_BUS_BIT8          (1<<5)      /* 8-bit bus width */
//#define RTKSD_SD_DETECT    (1<<6)      /* MMC class card detected */
//#define RTKSD_MS_DETECT    (1<<7)      /* MS class card detected */
    u8 reset_event;
    u8 rt_parameter;
    u8 fail_count;
    int hp_event;
#define HP_NON_ENENT    0
#define HP_INS_ENENT    1
#define HP_RMV_ENENT    2

    int act_host_clk;

#define CARD_SWITCHCLOCK_20MHZ  0x00
#define CARD_SWITCHCLOCK_24MHZ  0x01
#define CARD_SWITCHCLOCK_30MHZ  0x02
#define CARD_SWITCHCLOCK_40MHZ  0x03
#define CARD_SWITCHCLOCK_48MHZ  0x04
#define CARD_SWITCHCLOCK_60MHZ  0x05
#define CARD_SWITCHCLOCK_80MHZ  0x06
#define CARD_SWITCHCLOCK_96MHZ  0x07

    struct mmc_request *mrq;            /* Current request */
    int size;                           /* Total size of transfer */
    int wp;
    unsigned char *dma_buffer;          /* ISA DMA buffer */
    //unsigned int dma_len;
//    struct ms_info ms_card;
};

