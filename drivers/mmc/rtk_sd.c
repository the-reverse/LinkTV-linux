/******************************************************************************
 * $Id: rtk_sd.c 161767 2009-01-23 07:25:17Z ken.yu $
 * linux/drivers/mmc/rtk_sd.c
 * Overview: Realtek SD/MMC Driver
 * Copyright (c) 2009 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2009-02-05 Ken-Yu   create file
 *
 *******************************************************************************/
#include <linux/config.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/pnp.h>
#include <linux/highmem.h>
#include <linux/mmc/host.h>
#include <linux/mmc/protocol_mmc.h>
#include <linux/mmc/protocol_ms.h>
#include <linux/mmc/card.h>

#include <asm/io.h>
#include <asm/dma.h>
#include <asm/scatterlist.h>
#include <asm/addrspace.h>
#include <asm/mach-venus/platform.h>

/* Ken define: BEGIN */
#include <linux/rtk_cr.h>
#include "mmc.h"
#include "rtk_sd.h"
#include "mmc_debug.h"



#define BANNER      "Realtek Card-Reader Driver"
//#define VERSION     "$Id: rtk_sd.c 161767 2009-01-23 07:25:17Z ken.yu $"
#define VERSION     "$Id: rtk_sd.c 161767 2011-07-11 16:55 $"
#define DRIVER_NAME "rtk_sd"

#define BYTE_CNT            0x200
#define DMA_BUFFER_SIZE     128*512
#define RTK_NORMAL_SPEED    0x00
#define RTK_HIGH_SPEED      0x01
#define RTK_1_BITS          0x00
#define RTK_4_BITS          0x10
#define RTK_PHASE_HIGH      0x04
#define RTK_PHASE_LOW       0x02
#define RTK_PHASE_CLEAR     0x00

#define CRT_MUXPAD5         0xb8000364  //for mmc enviroment setting
#define CRT_PFUNC_NF0       0xb8000390
#define CRT_PFUNC_NF1       0xb8000394
#define CRT_PFUNC_NF2       0xb8000398
#define MUX_MMC_DET_MASK    0xffff3fff  //bit 14~15
#define MUX_MMC_WP_MASK     0xfffcffff  //bit 16~17
#define MUX_MMC_CLK_MASK    0xffcfffff  //bit 20~21

#define DET_SD_MOD          0x00008000  // bit 14~15[10]
#define DET_MS_MOD          0x0000c000  // bit 14~15[11]
#define WP_SD_MOD           0x00020000  // bit 16~17[10]
#define WP_MS_MOD           0x00030000  // bit 16~17[11]
#define CLK_SD_MOD          0x00100000  // bit 20~21[10]
#define CLK_MS_MOD          0x00200000  // bit 20~21[11]


static struct platform_device *rtksd_device;
static struct work_struct rtkcr_work;
u32 rtk_power_gpio;
u32 rtk_power_bits;

static DECLARE_MUTEX (sem);
u8 rtksd_get_rsp_len(u8 rsp_para);
int rtksd_set_speed(struct mmc_host *host,u8 para);
void rtksd_set_phase(struct mmc_host *host,u8 phase);
int rtksd_set_bits(struct mmc_host *host,u8 set_bit);
void rtkcr_set_clk(struct mmc_host *host,u8 mmc_clk);
static int SD_SendCMDGetRSP_Cmd(struct sd_cmd_pkt *cmd_info);
extern struct bus_type mmc_bus_type;
/* Ken define: END */

/*
 *  cmd_type: 1==>inner for controller used.wait 31.25ms
 *            0==>outter for communication wait card. wait 500ms
 *
 *
 */
#define R_W_CMD 2   //read/write command
#define INN_CMD 1   //command work chip inside
#define OTT_CMD 0   //command send to card

#define RTK_FAIL 3
#define RTK_RMOV 2
#define RTK_TOUT 1
#define RTK_SUCC 0

static DECLARE_MUTEX (sem_op_end);
/*****************************************************************
 Notice : if cmd_type is R_W_CMD, it's mean that command is send
          to card thought bus. So, if command GO bit not down
          the bus will be hold by card reader controller.
          To reset host is necessary when command fail
          about cmd_type is R_W_CMD.
 *****************************************************************/
static int rtkcr_wait_opt_end(struct mmc_host *host,u8 cmd_type)
{
    if(down_interruptible(&sem_op_end)){
        printk(KERN_WARNING "Wait OP semaphore fail\n");
        return -RTK_TOUT;
    }

    unsigned long timeend;
    unsigned int tmcount;
    int err;

    if(cmd_type==R_W_CMD){
        timeend=jiffies+(HZ*1);     //wait 1s
    }else if(cmd_type==INN_CMD){
        timeend=jiffies+(HZ>>5);    //wait 31.25ms
    }else{
        timeend=jiffies+(HZ>>1);    //wait 500ms
    }
    tmcount=0;
    err=-RTK_TOUT;
    cr_writel(0x01, CR_REG_MODE_CTR);
    while (time_before(jiffies, timeend) ){
        tmcount++;
        if((cr_readl(CR_REG_MODE_CTR)&0x01)==0){
            err=0;
            break;
        }

        //if remove evnet occur, stop waiting immidimately.
        if( host && (cmd_type==R_W_CMD) ){
            if(host->ins_event==EVENT_REMOV){
                err= -RTK_RMOV;
                break;
            }
            udelay(1);
        }else{

            udelay(1);
        }
    }
    if(err)
        printk(KERN_WARNING "rtk_opt_timeout!tmcount=%u\n",tmcount);

    up(&sem_op_end);
    return err;
}

void rtksd_pullup_pinout(void)
{
#ifndef HIGH_DRIVING
    printk(KERN_INFO "mmc: normal driving\n");
    cr_writel(0x30030300, CRT_PFUNC_NF0);  //SD_D3, SD_D0, SD_CD pull up
    cr_writel(0x33000000, CRT_PFUNC_NF1);  //SD_D1, SD_WP pull up
    cr_writel(0x00000303, CRT_PFUNC_NF2);  //SD_D2, SD_CMD pull up
#else
    printk(KERN_INFO "mmc: high driving\n");
    cr_writel(0x74474744, CRT_PFUNC_NF0);  //SD_D3, SD_D0, SD_CD pull up
    cr_writel(0x77444444, CRT_PFUNC_NF1);  //SD_D1, SD_WP pull up
    cr_writel(0x00000747, CRT_PFUNC_NF2);  //SD_D2, SD_CMD pull up
#endif
}

void rtkms_pullup_pinout(void)
{
    cr_writel(0x22000000, CRT_PFUNC_NF0);  //MS_D0, MS_D3 pull down
    cr_writel(0x03000000, CRT_PFUNC_NF1);  //MS_INS pull up
    cr_writel(0x00000202, CRT_PFUNC_NF2);  //MS_D2, MS_D1 pull down
}


void rtkcr_pulllow_pinout(void)
{
    cr_writel(0x0, CRT_PFUNC_NF0);  //SD_D3, SD_D0, SD_CD pull down
    cr_writel(0x0, CRT_PFUNC_NF1);  //SD_D1, SD_WP pull down
    cr_writel(0x0, CRT_PFUNC_NF2);  //SD_D2, SD_CMD pull down
}

#define POWER_ALL 0
#define POWER_MMC 1
#define POWER_MSP 2
static int rtkcr_host_power_off(struct mmc_host *host,u8 *filename)
{
    struct rtksd_port *sdport=NULL;

    if(host)
        sdport = mmc_priv(host);

    if(sdport==NULL)
        goto power_off;

    if(sdport->rtflags & RTKSD_FHOST_POWER){
        sdport->rtflags &= ~RTKSD_FHOST_POWER;

power_off:
        cr_writel(SD_POWERON>>8,    CR_REG_CMD_1);
        cr_writel(SD_POWERON,       CR_REG_CMD_2);
        cr_writel(1,        CR_REG_CMD_3);

        rtkcr_wait_opt_end(NULL,INN_CMD);

        rtkcr_pulllow_pinout();
    }
    return 0;
}

static int rtkcr_rst_host(struct mmc_host *host);
static int rtkcr_host_power_on(struct mmc_host *host,u8 *filename)
{
    int err;
    u8 retries;
    struct rtksd_port *sdport = NULL;

    retries=3;
    err=0;

    if(host)
        sdport = mmc_priv(host);

    if(sdport==NULL)
        goto re_poweron;

    if(sdport && (sdport->rtflags & RTKSD_FHOST_POWER))
        return err;

re_poweron:
    cr_writel(SD_POWERON>>8,    CR_REG_CMD_1);
    cr_writel(SD_POWERON,       CR_REG_CMD_2);
    cr_writel(0x00,             CR_REG_CMD_3);

    err=rtkcr_wait_opt_end(NULL,INN_CMD);

    if(err==RTK_SUCC){

        if(sdport)
            sdport->rtflags |= RTKSD_FHOST_POWER;

        rtksd_pullup_pinout();
    }else{  //if power control fail, pull pin low
        if(err==RTK_TOUT && retries ){
            if(host)
                rtkcr_rst_host(host);
            else
                rtkcr_rst_host(NULL);
            retries--;
            goto re_poweron;
        }
        printk(KERN_WARNING "%s(%d)rtk_host_power func fail\n",
                __func__,__LINE__);
        rtkcr_pulllow_pinout();
    }
    return err;
}

void rtkcr_card_power(struct mmc_host *host,u8 status,u8 *func_name)
{
    void __iomem *mmio = (void __iomem *) rtk_power_gpio;
    struct rtksd_port *sdport = NULL;
    unsigned int reginfo;

    if(host)
        sdport = mmc_priv(host);

    reginfo = cr_readl(CRT_MUXPAD5) & MUX_MMC_CLK_MASK;

    if(status==ON){ //power on

        if(sdport && (sdport->rtflags & RTKSD_FCARD_POWER)){
            mmcspec("Card alreay Power on\n");
        }else{
            mmcspec("Card Power on\n");
            if(sdport)
                sdport->rtflags |= RTKSD_FCARD_POWER;

            writel(readl(mmio) & ~(1<<rtk_power_bits),mmio);
            mdelay(3);
        }
        cr_writel(reginfo | CLK_SD_MOD, CRT_MUXPAD5);   //SD clk on
    }else{          //power off
        mmcspec("Card Power off\n");

        if(sdport)
            sdport->rtflags &= ~RTKSD_FCARD_POWER;

        cr_writel(reginfo, CRT_MUXPAD5);                //SD clk off
        cr_writel(0, CRT_PFUNC_NF0);
        cr_writel(0, CRT_PFUNC_NF1);
        cr_writel(0, CRT_PFUNC_NF2);
        writel(readl(mmio) | (1<<rtk_power_bits),mmio);
    }
}

void set_cmd_info(struct mmc_card *card,struct mmc_command * cmd,
                         struct sd_cmd_pkt * cmd_info,u32 opcode,u32 arg,u8 rsp_para)
{
    memset(cmd, 0, sizeof(struct mmc_command));
    memset(cmd_info, 0, sizeof(struct sd_cmd_pkt));

    cmd->opcode=opcode;
    cmd->arg=arg;
    cmd_info->cmd=cmd;
    cmd_info->mmc=card->host;
    cmd_info->rsp_para=rsp_para;
    cmd_info->rsp_len = rtksd_get_rsp_len(rsp_para);
}

#define STATE_IDLE  0
#define STATE_READY 1
#define STATE_IDENT 2
#define STATE_STBY  3
#define STATE_TRAN  4
#define STATE_DATA  5
#define STATE_RCV   6
#define STATE_PRG   7
#define STATE_DIS   8

static int rtksd_wait_status(struct mmc_card *card,u8 state,u8 divider)
{
    struct mmc_command cmd;
    struct sd_cmd_pkt cmd_info;
    unsigned long timeend;
    int err;

    timeend=jiffies+HZ;

    do {

        memset(&cmd, 0, sizeof(struct mmc_command));
        memset(&cmd_info, 0, sizeof(struct sd_cmd_pkt));

        set_cmd_info(card,&cmd,&cmd_info,
                     CMD13_SEND_STATUS,
                     (card->rca)<<RCA_SHIFTER,
                     SD_R1|divider);
        err=SD_SendCMDGetRSP_Cmd(&cmd_info);

        if(err){
            break;
        }else{
            u8 cur_state=R1_CURRENT_STATE(cmd.resp[0]);

            if(state==cur_state )
                break;
            else
                err=-1;
        }

    }while(time_before(jiffies, timeend));   //0x00001E00; the 1E is card state window
    return err;
}
static int rtkcr_re_tune_on(struct mmc_host *host);
static int re_initial_card(struct mmc_card *card)
{
    struct sd_cmd_pkt cmd_info;
    struct mmc_command cmd;
    u32 arg=0;
    u8 i;
    int err=0;
    struct rtksd_port *sdport = mmc_priv(card->host);
    unsigned long timeend;

    sdport->rt_parameter=0;
    rtkcr_set_clk(card->host,CARD_SWITCHCLOCK_30MHZ);

    if(card->card_type & (CR_SD|CR_SDHC))
    {

        for(i=0;i<4;i++){
            cr_writel(0x48, CR_REG_CMD_1);
            cr_writel(0x00, CR_REG_CMD_2);
            cr_writel(0xff, CR_REG_CMD_3);
            cr_writel(0xff, CR_REG_CMD_4);
            cr_writel(0xff, CR_REG_CMD_5);
            cr_writel(0xff, CR_REG_CMD_6);
            cr_writel(0xff, CR_REG_CMD_7);
            cr_writel(0x90, CR_REG_CMD_8);

            rtkcr_wait_opt_end(card->host,INN_CMD);
        }
    }

    set_cmd_info(card,&cmd,&cmd_info,CMD0_GO_IDLE_STATE,
                 0x00000000,SD_R0|DIVIDE_128);
    err=SD_SendCMDGetRSP_Cmd(&cmd_info);
    if(err){
        goto err_out;
    }

    if(card->card_type & (CR_SD|CR_SDHC)){  //SD/SDHC

        if(card->scr.sda_vsn == 2){    //SD spec 2.0
            //CMD8_SEND_IF_COND
            set_cmd_info(card,&cmd,&cmd_info,CMD8_SEND_IF_COND,
                         0x000001AA,SD_R7|DIVIDE_128);
            err=SD_SendCMDGetRSP_Cmd(&cmd_info);

            if(err)
                goto err_out;
        }

        if ( mmc_card_blockaddr(card))
            arg=SD_HCS;

        timeend=jiffies+2*HZ;
        while (1){
            if(time_after(jiffies, timeend)){
                mmcspec("SD ACMD41_SD_APP_OP_COND time out\n");
                err=-1;
                goto err_out;
            }

            set_cmd_info(card,&cmd,&cmd_info,CMD55_APP_CMD,
                         0x00000000,SD_R1|DIVIDE_128);
            err=SD_SendCMDGetRSP_Cmd(&cmd_info);
            if(err)
                goto err_out;

            mdelay(1);
            set_cmd_info(card,&cmd,&cmd_info,ACMD41_SD_APP_OP_COND,
                         arg|RTL_HOST_VDD,SD_R3|DIVIDE_128);
            err=SD_SendCMDGetRSP_Cmd(&cmd_info);
            if(err)
                goto err_out;

            if (cmd.resp[0] & MMC_CARD_BUSY)
                break;

        }
        mdelay(1);
        set_cmd_info(card,&cmd,&cmd_info,CMD2_ALL_SEND_CID,
                         0x00000000,SD_R2|DIVIDE_128);
        err=SD_SendCMDGetRSP_Cmd(&cmd_info);
        if(err)
            goto err_out;

        set_cmd_info(card,&cmd,&cmd_info,CMD3_SEND_RELATIVE_ADDR,
                         0x00000000,SD_R6|DIVIDE_128);
        err=SD_SendCMDGetRSP_Cmd(&cmd_info);
        if(err)
            goto err_out;

        card->rca = cmd.resp[0]>>RCA_SHIFTER;
        mmcspec("card->rca:0x%x\n",card->rca);

        set_cmd_info(card,&cmd,&cmd_info,CMD7_SELECT_CARD,
                         (card->rca)<<RCA_SHIFTER,SD_R1b|DIVIDE_128);
        err=SD_SendCMDGetRSP_Cmd(&cmd_info);
        if(err)
            goto err_out;

        if(mmc_card_highspeed(card))
        {
            mmcspec("high-speed mode\n");
            //set_cmd_info(card,&cmd,&cmd_info,CMD6_SWITCH_FUNC,
            //             0x80fffff1,SD_R1);
            //err=SD_SendCMDGetRSP_Cmd(&cmd_info);
            rtkcr_set_clk(card->host,CARD_SWITCHCLOCK_48MHZ);
        }
        else
        {
            mmcspec("normal-speed mode\n");
            rtkcr_set_clk(card->host,CARD_SWITCHCLOCK_30MHZ);
        }

        /* support 4-bits */
        if(card->scr.bus_widths &0x04)
        {

            set_cmd_info(card,&cmd,&cmd_info,CMD55_APP_CMD,
                         card->rca<<RCA_SHIFTER,SD_R1);
            err=SD_SendCMDGetRSP_Cmd(&cmd_info);
            if(err)
                goto err_out;

            set_cmd_info(card,&cmd,&cmd_info,ACMD6_SET_BUS_WIDTH,
                         0x02,SD_R1);
            err=SD_SendCMDGetRSP_Cmd(&cmd_info);
            if(err)
                goto err_out;

            err=rtksd_set_bits(card->host,RTK_4_BITS);
            if(err)
                goto err_out;
            else
                mmcspec("SD/SDHC 4-bits mode\n");

        }

    }else{  /* MMC card */
        timeend=jiffies+HZ;
        while (1){
            if(time_after(jiffies, timeend)){
                mmcspec("SD ACMD41_SD_APP_OP_COND time out\n");
                err=-1;
                goto err_out;
            }

            set_cmd_info(card,&cmd,&cmd_info,CMD1_SEND_OP_COND,
                         RTL_HOST_VDD,SD_R3|DIVIDE_128);
            err=SD_SendCMDGetRSP_Cmd(&cmd_info);
            if(err)
                goto err_out;

            mdelay(1);

            if (cmd.resp[0] & MMC_CARD_BUSY)
                break;

        }

        mdelay(1);
        set_cmd_info(card,&cmd,&cmd_info,CMD2_ALL_SEND_CID,
                         0x00000000,SD_R2|DIVIDE_128);
        err=SD_SendCMDGetRSP_Cmd(&cmd_info);
        if(err)
            goto err_out;

        mmcspec("MMC:card->rca:0x%x\n",card->rca);
        set_cmd_info(card,&cmd,&cmd_info,CMD3_SEND_RELATIVE_ADDR,
                         (card->rca)<<RCA_SHIFTER,SD_R1|DIVIDE_128);
        err=SD_SendCMDGetRSP_Cmd(&cmd_info);
        if(err)
            goto err_out;

        set_cmd_info(card,&cmd,&cmd_info,CMD7_SELECT_CARD,
                         (card->rca)<<RCA_SHIFTER,SD_R1b|DIVIDE_128);
        err=SD_SendCMDGetRSP_Cmd(&cmd_info);
        if(err)
            goto err_out;

        /* speed up to 40MHz */
        rtkcr_set_clk(card->host,CARD_SWITCHCLOCK_40MHZ);

        arg=(MMC_SWITCH_MODE_WRITE_BYTE << 24)
            | (EXT_CSD_BUS_WIDTH << 16)
            | (1 << 8)
            | EXT_CSD_CMD_SET_NORMAL;
        set_cmd_info(card,&cmd,&cmd_info,CMD6_SWITCH,
                         arg,SD_R1b);
        err=SD_SendCMDGetRSP_Cmd(&cmd_info);
        if(err)
            goto err_out;

        timeend=jiffies+HZ;
        do {
            if(time_after(jiffies, timeend)){
                err=-1;
                goto err_out;
            }

            set_cmd_info(card,&cmd,&cmd_info,CMD13_SEND_STATUS,
                         (card->rca)<<RCA_SHIFTER,SD_R1);
            err=SD_SendCMDGetRSP_Cmd(&cmd_info);
            if(err)
                goto err_out;

        }while(((cmd.resp[0] & 0x00001E00) >> 9)==7);   //0x00001E00; the 1E is card state window

        if(cmd.resp[0]&0xFDFFA000){ //0xFDFFA000 is card status error bits
            err=-1;
            goto err_out;
        }

        if (cmd.resp[0] & (1<<7)){
            err=-1;
            goto err_out;
        }else{
            err=rtksd_set_bits(card->host,RTK_4_BITS);
            if(err)
                goto err_out;
            else
                mmcspec("MMC 4-bits mode\n");
        }
    }

err_out:

    if(err ){
        mmcspec("re_initial fail !!\n");

    }else{
        mmcspec("re_initial success\n");
    }
    return err;
}

/*
 * Reset host then hold host on trun-off.
 * if reset host,don't reset(turn off) card,
 * beacuse reset card will reset status of card to idle state.
 *
 * 1. Disable CardReader clock:0xb800000C [25]=0, delay 100us
 * 2. Reset CardReader: 0xb8000004[10] = 0, delay 100 us
 * 3.                   0xb8000004[10] = 1, dealy 100 us
 * 4. Enable CardReader clock:0xb800000C [25]=1,
 */
static int rtkcr_re_tune_on(struct mmc_host *host)
{
    unsigned int reginfo;


    struct rtksd_port *sdport = mmc_priv(host);

    spin_lock_bh(sdport->rtlock);
    sdport->reset_event=1;;
    spin_unlock_bh(sdport->rtlock);

    cr_writel(0x00, CR_INT_MASK);   /* disable interrupt */
    cr_writel(0xff, CR_INT_STATUS);/* clear interrupt */

    asm ("sync");
    reginfo=cr_readl(0xb800000C);
    cr_writel(reginfo&(~(1<<25)),0xb800000C);
    asm ("sync");
    udelay(100);
    reginfo=cr_readl(0xb8000004);
    cr_writel(reginfo&(~(1<<10)),0xb8000004);

    asm ("sync");
    udelay(100);
    reginfo=cr_readl(0xb8000004);
    cr_writel(reginfo|(1<<10),0xb8000004);

    asm ("sync");
    udelay(100);

    spin_lock_bh(sdport->rtlock);
    reginfo=cr_readl(0xb800000C);
    cr_writel(reginfo|(1<<25),0xb800000C);
    //cr_writel(0xff, CR_INT_STATUS);/* clear interrupt */
    cr_writel(0x01, CR_INT_MASK);   /* enable interrupt */
    //cr_writel(0x07, CR_INT_STATUS);/* clear interrupt */
    spin_unlock_bh(sdport->rtlock);
    asm ("sync");

    spin_lock_bh(sdport->rtlock);
    sdport->reset_event=0;
    spin_unlock_bh(sdport->rtlock);


    rtkcr_pulllow_pinout();
    return 0;
}

static int rtkcr_rst_host(struct mmc_host *host)
{
    unsigned long timeend;
    struct rtksd_port *sdport = NULL;

    if(host){
        sdport = mmc_priv(host);
        sdport->rtflags &= ~RTKSD_FHOST_POWER;
    }

    timeend=jiffies+(HZ>>5);  //wait 31.25ms
    cr_writel((1<<3),CR_REG_MODE_CTR);

    while (cr_readl(CR_REG_MODE_CTR) & (1<<3)){
        if (time_before(jiffies, timeend)){
            mdelay(1);
        }else{
            printk(KERN_WARNING "%s(%d)stop rapper fail\n",__func__,__LINE__);
            goto err_out;
        }
    }
    mdelay(10);

    //stop IP
    cr_writel(CARD_STOP_HOST>>8,  CR_REG_CMD_1);
    cr_writel(CARD_STOP_HOST,     CR_REG_CMD_2);

    if(rtkcr_wait_opt_end(NULL,INN_CMD)){
        printk(KERN_WARNING "%s(%d)stop IP fail\n",__func__,__LINE__);
        goto err_out;
    }

    mdelay(10);

    /* rtk host power off    <<can ingnore */
    if(host){
        if(host->card_type_pre!=CR_MS){
            cr_writel(SD_POWERON>>8,    CR_REG_CMD_1);
            cr_writel(SD_POWERON,       CR_REG_CMD_2);
        }else{
            cr_writel(MS_POWERON>>8,    CR_REG_CMD_1);
            cr_writel(MS_POWERON,       CR_REG_CMD_2);
        }
    }else{
        cr_writel(SD_POWERON>>8,    CR_REG_CMD_1);
        cr_writel(SD_POWERON,       CR_REG_CMD_2);
        cr_writel(OFF,              CR_REG_CMD_3);

        rtkcr_wait_opt_end(NULL,INN_CMD);

        cr_writel(MS_POWERON>>8,    CR_REG_CMD_1);
        cr_writel(MS_POWERON,       CR_REG_CMD_2);
    }

    cr_writel(OFF,  CR_REG_CMD_3);

    rtkcr_wait_opt_end(NULL,INN_CMD);
    rtkcr_pulllow_pinout();

    return  RTK_SUCC;
err_out:
    printk(KERN_WARNING "%s(%d)host reset fail\n",__func__,__LINE__);
    return -RTK_TOUT;
}

static int rtkcr_polling(struct mmc_host *host)
{
    int err;

    cr_writel( CARD_POLLING>>8, CR_REG_CMD_1 );
    cr_writel( CARD_POLLING,    CR_REG_CMD_2 );

    err=rtkcr_wait_opt_end(host,INN_CMD);

    if(err){
        if(err==-RTK_TOUT)
            rtkcr_rst_host(host);
    }else{
        err = cr_readl(CR_CARD_STATUS_0)&0xff;
        CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_0);
    }
    return err;
}

static void rtksd_read_rsp(u32 *rsp, int reg_count)
{
    unsigned int reginfo;

    mmcinfo("*rsp:0x%p\n",rsp);

    if ( reg_count==6 ){
        reginfo = cr_readl(CR_CARD_STATUS_1);
        rsp[0] = (reginfo & 0xff)<<24;
        reginfo = cr_readl(CR_CARD_STATUS_0);
        rsp[0] |= (((reginfo >> 24) & 0xff)<<16) | (((reginfo >> 16) & 0xff)<<8) | (reginfo & 0xff);

        mmcinfo("rsp[0]:0x%08x\n",rsp[0]);

        CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_0);
        CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_1);
    }else if(reg_count==16){
        reginfo = cr_readl(CR_CARD_STATUS_3);
        rsp[0]=(((reginfo>>16) & 0xff)<<24)|(((reginfo>>8) & 0xff)<<16)|((reginfo & 0xff)<<8);
        reginfo = cr_readl(CR_CARD_STATUS_2);
        rsp[0]|=(reginfo>>24) & 0xff;
        rsp[1]=(((reginfo>>16) & 0xff)<<24)|(((reginfo>>8) & 0xff)<<16)|((reginfo & 0xff)<<8);
        reginfo = cr_readl(CR_CARD_STATUS_1);
        rsp[1]|=(reginfo>>24) & 0xff;
        rsp[2]=(((reginfo>>16) & 0xff)<<24)|(((reginfo>>8) & 0xff)<<16)|((reginfo & 0xff)<<8);
        reginfo = cr_readl(CR_CARD_STATUS_0);
        rsp[2]|=(reginfo>>24) & 0xff;
        rsp[3]=(((reginfo>>16) & 0xff)<<24)|(((reginfo>>8) & 0xff)<<16)|((reginfo & 0xff)<<8)|0xff;

        mmcinfo("rsp[0]:0x%08x, rsp[1]:0x%08x, rsp[2]:0x%08x, rsp[3]:0x%08x\n"
            ,rsp[0],rsp[1],rsp[2],rsp[3]);

        CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_0);
        CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_1);
        CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_2);
        CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_3);
    }
}

void rtksd_set_para(u8 para)
{
    cr_writel(SD_SETPARAMETER >> 8, CR_REG_CMD_1);
    cr_writel(SD_SETPARAMETER ,     CR_REG_CMD_2);
    cr_writel(para,                 CR_REG_CMD_3);
    cr_writel(0x10,                 CR_REG_CMD_4);  //enable wait busy

}

int rtksd_set_bits(struct mmc_host *host,u8 set_bit)
{
    struct rtksd_port *sdport = mmc_priv(host);
    u8 old_bits;
    int err=0;

    old_bits = (sdport->rt_parameter) & 0xf0;
    if(old_bits!=set_bit)
    {
        sdport->rt_parameter =((sdport->rt_parameter) & 0x0f)|set_bit;

        rtksd_set_para(sdport->rt_parameter);
        err=rtkcr_wait_opt_end(host,INN_CMD);
        if(err)
            sdport->rt_parameter =((sdport->rt_parameter) & 0x0f)|old_bits;
    }
    return err;
}

void rtksd_set_phase(struct mmc_host *host,u8 phase)
{
    struct rtksd_port *sdport = mmc_priv(host);
    u8 old_phase=(sdport->rt_parameter)&0x06;

    mmcinfo("\n");
    if(old_phase!=phase)
    {
        sdport->rt_parameter = ((sdport->rt_parameter) & 0xf9)|phase;
        rtksd_set_para(sdport->rt_parameter);

        if(rtkcr_wait_opt_end(host,INN_CMD))
            sdport->rt_parameter = ((sdport->rt_parameter) & 0xf9)|old_phase;

    }
}

int rtksd_set_speed(struct mmc_host *host,u8 para)
{
    struct rtksd_port *sdport = mmc_priv(host);
    u8 old_para=(sdport->rt_parameter)&0x01;
    int err=0;

    if(old_para!=para){
        sdport->rt_parameter = ((sdport->rt_parameter) & 0xfe)|para;
        rtksd_set_para(sdport->rt_parameter);
        err= rtkcr_wait_opt_end(host,INN_CMD);

        if(err)
            sdport->rt_parameter = ((sdport->rt_parameter) & 0xfe)|old_para;
    }
    return err;
}

/*
 * it's no matter if clock setting fail,
 * Host can send cmd with low speed clock even card have work on high speed mode,
 *
 */
//0=20MHz, 1=24, 2=30, 3=40, 4=48, 5=60, 6=80, 7=96
const char *const clk_tlb[8] = {
    "10MHz",
    "12MHz",
    "15MHz",
    "20MHz",
    "24MHz",
    "30MHz",
    "40MHz",
    "48MHz"
};
void rtkcr_set_clk(struct mmc_host *host,u8 mmc_clk)
{
    u8 old_clk;
    struct rtksd_port *sdport = mmc_priv(host);

    if(mmc_clk != sdport->act_host_clk){
        old_clk=sdport->act_host_clk;
        sdport->act_host_clk=mmc_clk;
        printk(KERN_INFO "MMC:clk change to %s\n",clk_tlb[sdport->act_host_clk]);
        cr_writel(0x80,     CR_REG_CMD_1);
        cr_writel(sdport->act_host_clk,  CR_REG_CMD_2);

        if(rtkcr_wait_opt_end(host,INN_CMD)){
            sdport->act_host_clk=old_clk;
        }else{
            if(sdport->act_host_clk>CARD_SWITCHCLOCK_48MHZ){
                if(rtksd_set_speed(host,RTK_HIGH_SPEED))
                {
                    sdport->act_host_clk=old_clk;
                    cr_writel(0x80,     CR_REG_CMD_1);
                    cr_writel(sdport->act_host_clk,  CR_REG_CMD_2);  //0=20MHz, 1=24, 2=30, 3=40, 4=48, 5=60, 6=80, 7=96
                }
            }else{
                rtksd_set_speed(host,RTK_NORMAL_SPEED);
            }
        }
    }
}


u8 rtksd_get_rsp_len(u8 rsp_para)
{
    switch (rsp_para & 0x3) {
    case 0:
        return 0;
    case 1:
        return 6;
    case 2:
        return 16;
    default:
        return 0;
    }
}

u32 rtksd_get_cmd_timeout(u8 rsp_para)
{
    if (rsp_para & 0x04)
        return 500;
    else
        return 300;
}

/******************************************************************************
  Send SD_SENDCMDGETRSP command to RTS5121 through bulk pipe. This function
  would not handle error case. It returns error status to the upper function
  only.

  \param us         USB device.
  \param cmd_idx        Command index.
  \param sd_arg         SD argument.
  \param rsp_para       Response parameter.
  \param rsp_len        Response length.
  \param rsp            Response buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************/
static int SD_SendCMDGetRSP_Cmd(struct sd_cmd_pkt *cmd_info)
{
    u32 cr_int_status       = 0;
    u8 cmd_idx              = cmd_info->cmd->opcode;
    u32 sd_arg              = cmd_info->cmd->arg;
    u8 rsp_para             = cmd_info->rsp_para;
    u8 rsp_len             = cmd_info->rsp_len;
    u32 *rsp                = (u32 *)&cmd_info->cmd->resp;
    struct mmc_host *host=cmd_info->mmc;
    int err;

    mmcinfo("cmd_idx=%u\nsd_arg=0x%x; rsp_para=0x%x\n",
                cmd_idx,sd_arg,rsp_para);

    if(rsp == NULL) {
        BUG_ON(1);
    }

    cr_writel(SD_SENDCMDGETRSP>>8,  CR_REG_CMD_1);
    cr_writel(SD_SENDCMDGETRSP,     CR_REG_CMD_2);
    cr_writel(0x40| cmd_idx,        CR_REG_CMD_3);
    cr_writel(sd_arg>>24,           CR_REG_CMD_4);
    cr_writel(sd_arg>>16,           CR_REG_CMD_5);
    cr_writel(sd_arg>>8,            CR_REG_CMD_6);
    cr_writel(sd_arg,               CR_REG_CMD_7);
    cr_writel(rsp_para,             CR_REG_CMD_8);

    err=rtkcr_wait_opt_end(host,R_W_CMD);

    if(err == RTK_SUCC){
        cr_writel((CIS_DECODE_FINISH|CIS_DECODE_ERROR),CR_INT_STATUS);

        if(cr_int_status & CIS_DECODE_ERROR){
            err = -RTK_FAIL;
            printk(KERN_WARNING "cmd parser fail\n");
            printk(KERN_WARNING "CR_INT_STATUS:0x%08x\n",cr_int_status);

            rtkcr_rst_host(host);
            mdelay(10);
            rtkcr_host_power_on(host,(u8 *)__FUNCTION__);

        }else{
            rtksd_read_rsp(rsp, rsp_len);
        }

    }else if(err == -RTK_TOUT){
        printk(KERN_WARNING "SD_SendCMDGetRSP_Cmd fail\n");
        rtkcr_rst_host(host);
        mdelay(10);
        rtkcr_host_power_on(host,(u8 *)__FUNCTION__);
    }else{
        rtkcr_rst_host(host);
    }
    return err;
}


/********************************************************************************
 *
 * Send command to SD/MMC card and get response from card afterward. The
 * response returned from SD/MMC card is up to command type, i.e. response type
 * that is described in argument. If the response is 6 bytes, response data has
 * 6 bytes, and 1st, 2nd, 3rd, 4th, 6th byte is useful but 5th byte is dummy
 * byte. If the response is 17 bytes, the response data has 16 bytes. If no
 * response needed, no data would be returned.
 * It should be noted that, those commands sent by Flash Card Controller in this
 * vendor command are nothing with r/w SD/MMC card. In other words, there no
 * data on SD/MMC card's data bus.
 * This function calls SD_SendCMDGetRSP_Cmd to send the command to RTS5121,
 * and handles the error case according to the returned value of that function.
 *
 * \param us         USB device.
 * \param cmd_idx        Command index.
 * \param sd_arg         SD argument.
 * \param rsp_para       Response parameter.
 * \param rsp_len        Response length.
 * \param rsp            Response buffer.
 *
 * \retval           Succeed or fail.
 *******************************************************************************/
int SD_SendCMDGetRSP(struct sd_cmd_pkt * cmd_info)
{
    int rc;

    rc = SD_SendCMDGetRSP_Cmd(cmd_info);

    return rc;
}

void duplicate_pkt(struct sd_cmd_pkt* sour,struct sd_cmd_pkt* dist)
{

    dist->mmc         = sour->mmc;
    dist->cmd         = sour->cmd;
    dist->data        = sour->data;

    dist->dma_buffer  = sour->dma_buffer;
    dist->byte_count  = sour->byte_count;
    dist->block_count = sour->block_count;

    dist->rsp_para    = sour->rsp_para;
    dist->rsp_len     = sour->rsp_len;
    dist->flags       = sour->flags;
    dist->timeout     = sour->timeout;
}

static int SD_Stream_Cmd(u16 cmdcode,struct sd_cmd_pkt *cmd_info)
{
    u32 cr_int_status = 0;
    u8 cmd_idx      = cmd_info->cmd->opcode;
    u32 sd_arg      = cmd_info->cmd->arg;
    u8 rsp_para     = cmd_info->rsp_para;
    u8 rsp_len     = cmd_info->rsp_len;
    u32 *rsp        = (u32 *)&cmd_info->cmd->resp;
    u16 byte_count  = cmd_info->byte_count;
    u16 block_count = cmd_info->block_count;
    void *data      = cmd_info->dma_buffer;
    struct mmc_host *host=cmd_info->mmc;
    unsigned int reginfo;
    int err;

    /* for SD_NORMALWRITE/SD_NORMALREAD can't check CRC16 issue */
    if((cmdcode==SD_NORMALWRITE) || (cmdcode==SD_NORMALREAD))
    {
        rsp_para &=~CHECK_CRC16;
        byte_count=512;
    }

    if(data == NULL) {
        BUG_ON(1);
    }
    if(rsp == NULL) {
        BUG_ON(1);
    }

#ifdef TEST_POWER_RESCYCLE
    if(cmd_info->mmc->card)
    {
        cmd_info->mmc->card->test_count++;
    }
#endif

    if(cmd_info->cmd->data->flags & MMC_DATA_READ){

        cr_writel((unsigned int) data, CR_DMA_IN_ADDR);
    }else{

        cr_writel((unsigned int) data, CR_DMA_OUT_ADDR);
    }

    cr_writel(0x1f,CR_DMA_STATUS);  //clear DMA status before DMA transfer
    cr_writel((CIS_DECODE_FINISH|CIS_DECODE_ERROR),CR_INT_STATUS);

    mmcinfo("cmd_idx=%u\nsd_arg=0x%x;rsp_para=0x%x;byte_count=0x%x;block_count=0x%x\n",
            cmd_idx,sd_arg,rsp_para,byte_count,block_count);

    cr_writel(cmdcode>>8,       CR_REG_CMD_1);
    cr_writel(cmdcode,          CR_REG_CMD_2);
    cr_writel(0x40| cmd_idx,    CR_REG_CMD_3);
    cr_writel(sd_arg>>24,       CR_REG_CMD_4);
    cr_writel(sd_arg>>16,       CR_REG_CMD_5);
    cr_writel(sd_arg>>8,        CR_REG_CMD_6);
    cr_writel(sd_arg,           CR_REG_CMD_7);
    cr_writel(rsp_para,         CR_REG_CMD_8);
    cr_writel(byte_count,       CR_REG_CMD_9);
    cr_writel(byte_count>>8,    CR_REG_CMD_10);
    cr_writel(block_count,      CR_REG_CMD_11);
    cr_writel(block_count>>8,   CR_REG_CMD_12);

#ifdef TEST_POWER_RESCYCLE
    if(cmd_info->mmc->card){
        if(cmd_info->mmc->card->test_count==150){
            rtkcr_card_power(NULL,OFF,(u8 *)__FUNCTION__);
            /* execute power recycle */
            cmd_info->mmc->card->test_count=0;
        }
    }
#endif

    err=rtkcr_wait_opt_end(host,R_W_CMD);

    if(err == RTK_SUCC){
        reginfo=cr_readl(CR_DMA_STATUS);
        cr_writel(reginfo,CR_DMA_STATUS);

        cr_writel((CIS_DECODE_FINISH|CIS_DECODE_ERROR),CR_INT_STATUS);

        if(reginfo & (CDS_DRQ_TMTOUT|CDS_DMA_ERROR) || cr_int_status & CIS_DECODE_ERROR){
            err = -RTK_FAIL;
            printk(KERN_WARNING "DMA error\n");
            printk(KERN_WARNING"CR_DMA_STATUS:0x%08x; CR_INT_STATUS:0x%08x\n",
                    reginfo,cr_int_status);
        }

        if(cmdcode==SD_AUTOREAD1 || cmdcode==SD_AUTOWRITE1 ||
           cmdcode==SD_AUTOREAD2 || cmdcode==SD_AUTOWRITE2)
        {}
        else
            rtksd_read_rsp(rsp, rsp_len);

    }else if(err == -RTK_TOUT){
        mmcspec("cmd_idx=%u, sd_arg=0x%x, rsp_para=0x%x, byte_count0x%x, block_count=%x\n",
                 cmd_idx,sd_arg, rsp_para,byte_count,block_count);
        if(host->card && mmc_card_ident_rdy(host->card)){

            rtkcr_card_power(NULL,OFF,(u8 *)__FUNCTION__);
            mdelay(100);
            rtkcr_card_power(NULL,ON,(u8 *)__FUNCTION__);
            mdelay(10);

            rtkcr_re_tune_on(host);
            rtkcr_rst_host(host);
            mdelay(10);
            rtkcr_host_power_on(host,(u8 *)__FUNCTION__);
            mdelay(10);
            re_initial_card(host->card);

        }else{

            rtkcr_rst_host(host);
            rtkcr_host_power_on(host,(u8 *)__FUNCTION__);
        }
    }else{
        rtkcr_rst_host(host);
    }
    return err;
}

int SD_Stream(u16 cmdcode,struct sd_cmd_pkt *cmd_info)
{
#ifdef TEST_DSC_MOD
    int err=0;
    unsigned int i;
    u32 chk_sta_time;
    struct scatterlist *sg;
    int dir;
    unsigned int dma_nents=0;
    unsigned int dma_leng;
    unsigned int dma_addr;
    unsigned int old_arg;
    u8 one_blk=0;
    struct mmc_host *host=cmd_info->mmc;

    if(cmd_info->data->flags & MMC_DATA_READ){
        dir=DMA_FROM_DEVICE;
    }else{
        err=rtksd_wait_status(host->card,STATE_TRAN,0);
        if(err)
            goto err_out;

        if(mmc_card_one_blk(host->card))
            one_blk = 1;

        dir=DMA_TO_DEVICE;

    }

    cmd_info->data->bytes_xfered=0;
    dma_nents = dma_map_sg( mmc_dev(cmd_info->mmc), cmd_info->data->sg,
                            cmd_info->data->sg_len,  dir);
    sg = cmd_info->data->sg;

#ifdef SHOW_MMC_PRD
    printk(KERN_INFO "sg_len:%u\n",cmd_info->data->sg_len);
    printk(KERN_INFO "sg:0x%p; dma_nents:%u\n",sg,dma_nents);
#endif
    old_arg=cmd_info->cmd->arg;

    for(i=0; i<dma_nents; i++,sg++){
        dma_addr=sg_dma_address(sg);
        dma_leng=sg_dma_len(sg);
#ifdef SHOW_MMC_PRD
        printk(KERN_INFO "dma_addr:0x%x; dma_leng:0x%x\n",
                dma_addr,dma_leng);

        if ( mmc_card_blockaddr(host->card))
            printk(KERN_INFO "arg:0x%x blk\n",cmd_info->cmd->arg);
        else
            printk(KERN_INFO "arg:0x%x byte\n",cmd_info->cmd->arg);
#endif
        if(one_blk)
        {
            u8 i,blk_cnt;
            struct sd_cmd_pkt tmp_pkt;

            blk_cnt = dma_leng/BYTE_CNT;
            duplicate_pkt(cmd_info,&tmp_pkt);

            tmp_pkt.byte_count  = BYTE_CNT;
            tmp_pkt.block_count = 1;
            tmp_pkt.dma_buffer  = (unsigned char *)dma_addr;

            if(tmp_pkt.cmd->opcode == CMD25_WRITE_MULTIPLE_BLOCK){
                tmp_pkt.cmd->opcode = CMD24_WRITE_BLOCK;
                cmdcode = SD_AUTOWRITE2;
            }

            if(tmp_pkt.cmd->opcode == CMD18_READ_MULTIPLE_BLOCK){
                tmp_pkt.cmd->opcode = CMD17_READ_SINGLE_BLOCK;
                cmdcode = SD_AUTOREAD2;
            }

            for(i=0;i<blk_cnt;i++){
                mmcmsg1("\n");

                err = SD_Stream_Cmd(cmdcode,&tmp_pkt);

                if(err == 0){
                    if(cmd_info->data->flags & MMC_DATA_WRITE){
                        chk_sta_time = 2;
                        while(chk_sta_time){

                            err = rtksd_wait_status(host->card,STATE_TRAN,0);
                            chk_sta_time--;
                            if(err)
                                break;
                        }
                    }

                    if(host->card && mmc_card_blockaddr(host->card))
                        tmp_pkt.cmd->arg += 1;
                    else
                        tmp_pkt.cmd->arg += BYTE_CNT;

                    tmp_pkt.dma_buffer += BYTE_CNT;
                    tmp_pkt.data->bytes_xfered += BYTE_CNT;

                }else{
                    break;
                }
            }
            dma_cache_wback(KSEG0ADDR(dma_addr), dma_leng);  //<===
        }
        else
        {
            cmd_info->byte_count =BYTE_CNT;
            cmd_info->block_count=dma_leng/BYTE_CNT;
            cmd_info->dma_buffer=(unsigned char *)dma_addr;

            if(cmd_info->data->flags & MMC_DATA_READ)
                dma_cache_inv(KSEG0ADDR(dma_addr), dma_leng);  //<===

            err=SD_Stream_Cmd(cmdcode,cmd_info);

            if(cmd_info->data->flags & MMC_DATA_WRITE)
                dma_cache_wback(KSEG0ADDR(dma_addr), dma_leng);  //<===

            if(err==0){

                if(cmd_info->data->flags & MMC_DATA_WRITE){
                    chk_sta_time = 2;
                    while(chk_sta_time){
                        err = rtksd_wait_status(host->card,STATE_TRAN,0);
                        chk_sta_time--;
                        if(err)
                            break;
                    }
                }

                if ( mmc_card_blockaddr(host->card))
                    cmd_info->cmd->arg+=cmd_info->block_count;
                else
                    cmd_info->cmd->arg+=dma_leng;

                cmd_info->data->bytes_xfered+=dma_leng;

            }
        }
        if(err){
            cmd_info->cmd->arg=old_arg;
            break;
        }
    }
    dma_unmap_sg( mmc_dev(cmd_info->mmc), cmd_info->data->sg,
                            cmd_info->data->sg_len,  dir);

err_out:
    return err;
#else
    return SD_Stream_Cmd(cmdcode,cmd_info);
#endif

}

static inline void rtkcr_sg_to_dma(struct rtksd_port *sdport, struct mmc_data *data)
{
    unsigned int len, i, size;
    struct scatterlist *sg;
    unsigned char *dmabuf = sdport->dma_buffer;
    unsigned char *sgbuf;

    size = sdport->size;

    sg = data->sg;
    len = data->sg_len;

    for (i = 0; i < len; i++){
        sgbuf = kmap_atomic(sg[i].page, KM_BIO_SRC_IRQ) + sg[i].offset;

        if (size < sg[i].length)
            memcpy(dmabuf, sgbuf, size);
        else
            memcpy(dmabuf, sgbuf, sg[i].length);

        kunmap_atomic(sgbuf, KM_BIO_SRC_IRQ);
        dmabuf += sg[i].length;

        if (size < sg[i].length)
            size = 0;
        else
            size -= sg[i].length;

        if (size == 0)
            break;
    }
    sdport->size -= size;
}

static inline void rtkcr_dma_to_sg(struct rtksd_port *sdport, struct mmc_data *data)
{
    unsigned int len, i, size;
    struct scatterlist *sg;
    unsigned char *dmabuf = sdport->dma_buffer;
    unsigned char *sgbuf;
    mmcinfo("\n");
    size = sdport->size;
    sg = data->sg;
    len = data->sg_len;

    for (i = 0; i < len; i++){
        sgbuf = kmap_atomic(sg[i].page, KM_BIO_SRC_IRQ) + sg[i].offset;

        if (size < sg[i].length){
            memcpy(sgbuf, dmabuf, size);
        }else{
            memcpy(sgbuf, dmabuf, sg[i].length);
        }
        kunmap_atomic(sgbuf, KM_BIO_SRC_IRQ);
        dmabuf += sg[i].length;
        if (size < sg[i].length)
            size = 0;
        else
            size -= sg[i].length;
        if (size == 0)
            break;
    }

    BUG_ON(size != 0);
    sdport->size -= size;

}

#ifndef TEST_DSC_MOD
static void rtksd_prepare_data(struct rtksd_port *sdport, struct mmc_data *data)
{
    sdport->size = data->blocks * data->blksz;

    BUG_ON(sdport->size > DMA_BUFFER_SIZE);
    if (sdport->size > DMA_BUFFER_SIZE){
        data->error = MMC_ERR_INVALID;
        printk(KERN_WARNING "[%s] sdport->size > 64 KB\n", __FUNCTION__);
        return;
    }

    data->error = MMC_ERR_NONE;
}
#endif
static void rtksd_send_command(struct rtksd_port *sdport, struct mmc_command *cmd)
{
    unsigned char rsp[16];
    u32 rsp_type;
    unsigned char *rsp_tmp;
    u32 clock_divider;
    int rc = 0;
    struct sd_cmd_pkt cmd_info;

    mmcinfo("cmd->opcode:%u\n",cmd->opcode);

    if ( !sdport || !cmd ){
        printk(KERN_ERR"sdport or cmd is null\n");
        return ;
    }

    cmd_info.cmd=cmd;
    cmd_info.mmc=sdport->mmc;
    if ( mmc_resp_type(cmd)==MMC_RSP_R1 ){
        rsp_type = SD_R1;
    }else if ( mmc_resp_type(cmd)== MMC_RSP_R1B ){
        rsp_type = SD_R1b;
    }else if ( mmc_resp_type(cmd)== MMC_RSP_R2 ){
        rsp_type = SD_R2;
    }else if ( mmc_resp_type(cmd)== MMC_RSP_R3 ){
        rsp_type = SD_R3;
    }else if ( mmc_resp_type(cmd)== MMC_RSP_R6 ){
        rsp_type = SD_R6;
    }else if ( mmc_resp_type(cmd)== SD_R7 ){
        rsp_type = SD_R7;
    }else{
        rsp_type = SD_R0;
    }

    if (cmd->data){

#ifndef TEST_DSC_MOD
        CR_FLUSH_CACHE(sdport->dma_buffer, DMA_BUFFER_SIZE);
#endif

#ifdef TEST_DSC_MOD
        cmd_info.data = cmd->data;
#endif
        //mmcinfo("sdport->dma_buffer:0x%x; sdport->size:0x%x\n",
        //        (u32)sdport->dma_buffer,sdport->size);

        clock_divider = DIVIDE_NON;
        if ( cmd->data->flags & MMC_DATA_READ ){
            if(cmd->opcode==18 || cmd->opcode==17||cmd->opcode==11){

                cmd_info.byte_count=BYTE_CNT;
                cmd_info.block_count=sdport->size/BYTE_CNT;
#ifndef TEST_DSC_MOD
                cmd_info.dma_buffer=sdport->dma_buffer;
#endif
                cmd_info.rsp_para=rsp_type|CHECK_CRC16|clock_divider;
                cmd_info.rsp_len=rtksd_get_rsp_len(cmd_info.rsp_para);
                cmd_info.timeout=rtksd_get_cmd_timeout(cmd_info.rsp_para);

                if(cmd->opcode==17)
                    rc = SD_Stream(SD_AUTOREAD2,&cmd_info);
                else
                    rc = SD_Stream(SD_AUTOREAD1,&cmd_info);
            }else{
                u32 opcode=cmd->opcode;

                cmd_info.byte_count=sdport->size;
                cmd_info.block_count=1;
#ifndef TEST_DSC_MOD
                cmd_info.dma_buffer=sdport->dma_buffer;
#endif
                switch(opcode){
                    case 27:
                    case 26:
                    case 51:
                    case 30:
                    case 13:
                    case 22:
                        cmd_info.rsp_para=rsp_type|CHECK_CRC16|clock_divider;
                        break;
                    default:
                        cmd_info.rsp_para=rsp_type|CHECK_CRC16|clock_divider;
                        break;
                }
                cmd_info.rsp_len=rtksd_get_rsp_len(cmd_info.rsp_para);
                cmd_info.timeout=rtksd_get_cmd_timeout(cmd_info.rsp_para);

                rc = SD_Stream(SD_NORMALREAD,&cmd_info);
            }

        }else if (cmd->data->flags & MMC_DATA_WRITE ){

            if(sdport->wp==0){
#ifndef TEST_DSC_MOD
                cmd_info.byte_count=BYTE_CNT;
                cmd_info.block_count=sdport->size/BYTE_CNT;
                cmd_info.dma_buffer=sdport->dma_buffer;
#endif
                cmd_info.rsp_para=rsp_type|CHECK_CRC16|clock_divider;
                cmd_info.rsp_len=rtksd_get_rsp_len(cmd_info.rsp_para);
                cmd_info.timeout=rtksd_get_cmd_timeout(cmd_info.rsp_para);

                if(cmd->opcode==24){
                    rc = SD_Stream(SD_AUTOWRITE2,&cmd_info);
                }else{ //cmd->opcode==25 || cmd->opcode==20
                    rc = SD_Stream(SD_AUTOWRITE1,&cmd_info);
                }
            }else{
                rc=-1;
            }
        }else{
            printk(KERN_ERR "error: cmd->data->flags=%d\n", cmd->data->flags);
            cmd->error = MMC_ERR_INVALID;
        }

    }else{

        if (cmd->opcode == 0){

            rsp_tmp = NULL;
        }else{

            rsp_tmp = rsp;
        }

        if(1){
            struct mmc_host *host=sdport->mmc;
            if(host->card_selected)
                clock_divider=DIVIDE_NON;
            else
                clock_divider=DIVIDE_128;
        }

        cmd_info.rsp_para=rsp_type|clock_divider;
        cmd_info.rsp_len=rtksd_get_rsp_len(cmd_info.rsp_para);
        cmd_info.timeout=rtksd_get_cmd_timeout(cmd_info.rsp_para);

        rc = SD_SendCMDGetRSP(&cmd_info);

    }
    if (rc){
        if(rc== -RTK_RMOV)
            cmd->retries==0;

        cmd->error = MMC_ERR_FAILED;
    }
}

static void rtkcr_request_end(struct rtksd_port* sdport, struct mmc_request* mrq)
{
    mmcinfo("\n");
    sdport->mrq = NULL;
    up(&sem);

    mmc_request_done(sdport->mmc, mrq);

    if (down_interruptible (&sem)) {
        printk(KERN_WARNING "%s : user breaking\n",__FUNCTION__);
        return ;
    }
}

static void rtksd_request(struct mmc_host *host, struct mmc_request *mrq)
{
    struct rtksd_port *sdport = mmc_priv(host);
    struct mmc_command *cmd;
    struct mmc_card  *card;


    if (down_interruptible (&sem)) {
        printk(KERN_WARNING "%s : user breaking\n",__FUNCTION__);
        return ;
    }

    BUG_ON(sdport->mrq != NULL);
    cmd = mrq->cmd;
    sdport->mrq = mrq;
    sdport->size=0;

    if (!(sdport->rtflags & RTKSD_FCARD_DETECTED)){
        cmd->error = MMC_ERR_RMOVE;
        goto done;
    }

    if (cmd->data) {

        BUG_ON(host->card_selected == NULL);
        if(host->card_selected)
        {
            card=host->card_selected;

            if(card->card_type & CR_MMC){

                switch (cmd->opcode) {
                    case 18:
                    case 25:
                    case 17:
                    case 24:
                    case 11:
                    case 20:
                    case 56:
                    case 8:
                    case 26:
                    case 27:
                    case 30:
                    case 42:
                        break;
                    default:
                        printk(KERN_WARNING "Data command %d is not "
                            "supported by this controller.\n", cmd->opcode);
                        cmd->error = -EINVAL;
                        goto done;
                }

            }else if(card->card_type & (CR_SD|CR_SDHC)){

                switch (cmd->opcode) {
                    case 18:
                    case 25:
                    case 17:
                    case 24:
                    case 20:
                    case 56:
                    case 27:
                    case 30:
                    case 42:
                    case 13:
                    case 22:
                    case 51:
                    case 6:
                        break;
                    default:
                        printk(KERN_WARNING "Data command %d is not "
                            "supported by this controller.\n", cmd->opcode);
                        cmd->error = -EINVAL;
                        goto done;
                }

            }else{
                BUG_ON(1);
            }
        }
#ifndef TEST_DSC_MOD
        rtksd_prepare_data(sdport, cmd->data);

        if (cmd->data->error != MMC_ERR_NONE){
            printk(KERN_WARNING "request data has something wrong!!\n");
            goto done;
        }

        if (cmd->data->flags & MMC_DATA_WRITE){
            rtkcr_sg_to_dma(sdport, cmd->data);
        }
#endif
    }

    if ( sdport && cmd ){
        rtksd_send_command(sdport, cmd);
    }

    if ( cmd->error != MMC_ERR_NONE ){
#ifdef MMC_DEBUG
        printk(KERN_WARNING "rtksd_send_command() CMD %d fails!!\n",
                cmd->opcode);
#endif
        goto done;
    }

    if ( cmd->data ){
#ifndef TEST_DSC_MOD
        if (cmd->data->flags & MMC_DATA_READ)
            rtkcr_dma_to_sg(sdport, cmd->data);

        cmd->data->bytes_xfered = sdport->size;
#endif
    }

done:
    rtkcr_request_end(sdport, mrq);
    up(&sem);
}

static void rtksd_set_ios(struct mmc_host *host, struct mmc_ios *ios)
{
    struct rtksd_port *sdport = mmc_priv(host);

    spin_lock_bh(sdport->rtlock);

    if (ios->bus_mode == MMC_BUSMODE_PUSHPULL){

        if (ios->bus_width == MMC_BUS_WIDTH_4){
            printk(KERN_INFO "MMC:4 bits mode\n");
            rtksd_set_bits(host,RTK_4_BITS);
        }else{
            printk(KERN_INFO "MMC:1 bits mode\n");
            rtksd_set_bits(host,RTK_1_BITS);
        }

        if(ios->timing & (MMC_TIMING_SD_HS|MMC_TIMING_MMC_HS))
        {
            printk(KERN_INFO "MMC:high speed mode\n");
            rtkcr_set_clk(host,CARD_SWITCHCLOCK_96MHZ);
        }else{
            printk(KERN_INFO "MMC:normal speed mode\n");
            rtkcr_set_clk(host,CARD_SWITCHCLOCK_48MHZ);
        }
    }else{  //MMC_BUSMODE_OPENDRAIN
        rtksd_set_bits(host,RTK_1_BITS);
        rtkcr_set_clk(host,CARD_SWITCHCLOCK_30MHZ);  //default clock
    }

    if (ios->power_mode == MMC_POWER_OFF){
        rtkcr_host_power_off(host,(u8 *)__FUNCTION__);
        rtkcr_card_power(host,OFF,(u8 *)__FUNCTION__);
    }else{  //MMC_POWER_UP or MMC_POWER_NO
        /* have any setting for power up mode ? */
        rtkcr_host_power_on(host,(u8 *)__FUNCTION__);
        mdelay(10);
        rtkcr_card_power(host,ON,(u8 *)__FUNCTION__);

        if(ios->power_mode == MMC_POWER_ON){
            u8 i;
            mdelay(10);

            for(i=0;i<4;i++){
                cr_writel(0x48, CR_REG_CMD_1);
                cr_writel(0x00, CR_REG_CMD_2);
                cr_writel(0xff, CR_REG_CMD_3);
                cr_writel(0xff, CR_REG_CMD_4);
                cr_writel(0xff, CR_REG_CMD_5);
                cr_writel(0xff, CR_REG_CMD_6);
                cr_writel(0xff, CR_REG_CMD_7);
                cr_writel(0x90, CR_REG_CMD_8);

                rtkcr_wait_opt_end(host,INN_CMD);
            }
        }
    }
    spin_unlock_bh(sdport->rtlock);

}

void set_str_info(struct mmc_card *card,
                    struct mmc_command* cmd,
                    struct mmc_data *data,
                    struct sd_cmd_pkt * cmd_info,
                    u32 opcode,
                    u32 arg,
                    u16 byte_count,
                    u16 block_count,
                    unsigned char * dma_buf)
{
    memset(cmd_info, 0, sizeof(struct sd_cmd_pkt));
    memset(cmd, 0, sizeof(struct mmc_command));
    memset(data, 0, sizeof(struct mmc_data));

    mmcinfo("opcode=%u\n",opcode);
    switch (opcode) {
        case 8:
        case 14:
        case 17:
        case 18:
            data->flags = MMC_DATA_READ;
            break;
        case 20:
        case 24:
        case 25:
        case 26:
        case 27:
        case 30:
        case 42:
            data->flags = MMC_DATA_WRITE;
            break;
        default:
            BUG_ON(1);
            break;
    }

    cmd->data    = data;
    cmd->opcode  = opcode;
    cmd->arg     = arg;

    cmd_info->cmd           = cmd;
    cmd_info->mmc           = card->host;
    cmd_info->rsp_para      = SD_R1;
    cmd_info->rsp_len       = rtksd_get_rsp_len(SD_R1);
    cmd_info->byte_count    = byte_count;
    cmd_info->block_count   = block_count;
    cmd_info->dma_buffer    = dma_buf;

}

static u32 SD_NormalWrite_Cmnd(struct mmc_host *host, u16 byte_count,
    u8 data0, u8 data1, unsigned char *status)
{
    u32 rsp;
    u32 reginfo;
    int err;
    u32 cr_int_status = 0;

    cr_writel(SD_NORMALWRITE>>8,    CR_REG_CMD_1);
    cr_writel(SD_NORMALWRITE,       CR_REG_CMD_2);
    cr_writel(byte_count,           CR_REG_CMD_3);
    cr_writel(byte_count>>8,        CR_REG_CMD_4);
    cr_writel(data0,                CR_REG_CMD_5);
    cr_writel(data1,                CR_REG_CMD_6);

    err=rtkcr_wait_opt_end(host,R_W_CMD);

    if(err == RTK_SUCC){
        reginfo=cr_readl(CR_DMA_STATUS);
        cr_writel(reginfo,CR_DMA_STATUS);

        cr_writel((CIS_DECODE_FINISH|CIS_DECODE_ERROR),CR_INT_STATUS);

        if(reginfo & (CDS_DRQ_TMTOUT|CDS_DMA_ERROR) || cr_int_status & CIS_DECODE_ERROR){
            err = -RTK_FAIL;
            printk("DMA error\n");
            printk("CR_DMA_STATUS:0x%08x; CR_INT_STATUS:0x%08x\n",reginfo,cr_int_status);
        }

        rtksd_read_rsp(&rsp, 6); //rsp_len=6

        if(status){
            *status = (unsigned char)rsp;
        }

    }else if(err == -RTK_TOUT){
        mmcspec("cmd_idx=%u, sd_arg=0x%x, rsp_para=0x%x, byte_count0x%x, block_count=%x\n",
                 cmd_idx,sd_arg, rsp_para,byte_count,block_count);
        if(host->card && mmc_card_ident_rdy(host->card)){
            rtkcr_card_power(NULL,OFF,(u8 *)__FUNCTION__);
            mdelay(100);
            rtkcr_card_power(NULL,ON,(u8 *)__FUNCTION__);
            mdelay(10);

            rtkcr_re_tune_on(host);
            rtkcr_rst_host(host);
            mdelay(10);
            rtkcr_host_power_on(host,(u8 *)__FUNCTION__);
            mdelay(10);
            re_initial_card(host->card);

        }else{

            rtkcr_rst_host(host);
            rtkcr_host_power_on(host,(u8 *)__FUNCTION__);
        }
    }else{
        rtkcr_rst_host(host);
    }
    return 0;
}


int SD_NormalWrite(struct mmc_host *host,u16 byte_count, u8 data0, u8 data1,
    unsigned char *status)
{
    u32 errCode;

    errCode = SD_NormalWrite_Cmnd(host, byte_count, data0, data1, status);

    return errCode;
}

int rtksd_bustest(struct mmc_host *host,u8 bit_width)
{
    struct sd_cmd_pkt cmd_info;
    struct mmc_command cmd;
    struct mmc_data data;

    struct mmc_card *card = host->card;
    int err = 0;
    unsigned char *buf;

    /* send cmd19 */
    set_cmd_info(card,&cmd,&cmd_info,
                 CMD19_BUSTEST_W,
                 0x00000000,
                 SD_R1);
    err = SD_SendCMDGetRSP_Cmd(&cmd_info);
    if(err)
        goto err_out;

    if(bit_width == EXT_CSD_BUS_WIDTH_1){
        rtksd_set_bits(host,RTK_1_BITS);
        err = SD_NormalWrite(host, 1, 0x80, 0x00, NULL);    //1-bits
    }else if(bit_width == EXT_CSD_BUS_WIDTH_4){
        rtksd_set_bits(host,RTK_4_BITS);
        err = SD_NormalWrite(host, 1, 0x5a, 0x00, NULL);    //4-bits
    }else if(bit_width == EXT_CSD_BUS_WIDTH_8){
        printk(KERN_WARNING "Don't suooprt 8-bits width\n");
        BUG_ON(1);
        //rtksd_set_bits(host,RTK_8_BITS);
        //err = SD_NormalWrite(host, 2, 0x55, 0xaa, NULL);    //8-bits
    }else{
        printk(KERN_WARNING "unknow bus width\n");
        BUG_ON(1);
    }


    if(err)
        goto err_out;

    /* send cmd14 */
    buf = (unsigned char *)kmalloc(512, GFP_KERNEL);

    set_str_info(card,&cmd,&data,&cmd_info,
                    CMD14_BUSTEST_R,
                    0x00000000,8,1,buf);
    err = SD_Stream_Cmd(SD_NORMALREAD,&cmd_info);
    if(err)
        goto kfree_out;

    if(bit_width == EXT_CSD_BUS_WIDTH_1){
        if(buf[0]==0x40)
            goto kfree_out;
    }else if(bit_width == EXT_CSD_BUS_WIDTH_4){
        if(buf[0]==0xa5)
            goto kfree_out;
    }else if(bit_width == EXT_CSD_BUS_WIDTH_4){
        if(buf[0]==0xaa && buf[1]==0x55)
            goto kfree_out;
    }else{
        BUG_ON(1);
    }
    err = -1;

kfree_out:
    kfree(buf);
err_out:
    return err;

}

void rtksd_fake_proc(struct mmc_host *host)
{
    struct mmc_card *card = host->card;

    int err=0;
    u8 test_clk;
    u8 limit_clk;

    if(card->ext_csd.hs_max_dtr > 26000000)
        limit_clk = CARD_SWITCHCLOCK_96MHZ;
    else
        limit_clk = CARD_SWITCHCLOCK_48MHZ;

    test_clk = CARD_SWITCHCLOCK_20MHZ;

speed_up:
    rtkcr_set_clk(host,test_clk);  //default clock

    err = rtksd_bustest(host,EXT_CSD_BUS_WIDTH_4);

    if(err || test_clk==limit_clk)
        goto end_out;

    test_clk++;
    goto speed_up;

end_out:
    if(err)
        test_clk--;
    return;
}

static struct mmc_host_ops rtksd_ops = {
    .request    = rtksd_request,
    .set_ios    = rtksd_set_ios,
    .fake_proc  = rtksd_fake_proc,
};

/* for MS use ********************************************************* */
#define MS_ERR_CRC      0x80
#define MS_ERR_TIMEOUT  0x40
#define MS_STA_CMDNK    0x08
#define MS_STA_BREQ     0x04
#define MS_STA_ERR      0x02
#define MS_STA_CDE      0x01
#define MS_ERR_STATE    (MS_ERR_CRC|MS_ERR_TIMEOUT|MS_STA_ERR)

static void rtkms_read_rsp(u8 *rsp, int byte_count)
{
    unsigned int reg_status, reg_num;
    int i;
    u8 temp[16];
    mmcinfo("\n");

    if ( !rsp ){
        printk(KERN_WARNING "rsp is null pointer\n");
        return;
    }

    if(byte_count>16)
        byte_count=16;

    for ( i=0; i<((byte_count+3)/4); i++){
        reg_num = CR_CARD_STATUS_0 + i*4;
        reg_status = cr_readl(reg_num);

        temp[i*4+0] = reg_status & 0xff;
        temp[i*4+1] = (reg_status >>  8) & 0xff;
        temp[i*4+2] = (reg_status >> 16) & 0xff;
        temp[i*4+3] = (reg_status >> 24) & 0xff;
        CLEAR_CR_CARD_STATUS(reg_num);
    }

    memcpy(rsp, temp, byte_count);

#ifdef SHOW_MS_RSP
    for(i=0; i<byte_count; i++)
        printk(KERN_WARNING "rsp[%x]:0x%08x\n",i,rsp[i]);
#endif
}

/*
static u32 ms_auto_cmd(u8 TPC, u16 sector_count,
    void *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    u32 cr_int_status=0;

    mmcinfo("\n");

    cr_writel((unsigned int) data,  CR_DMA_OUT_ADDR);
    cr_writel(MS_AUTOWRITE>>8,      CR_REG_CMD_1);
    cr_writel(MS_AUTOWRITE,         CR_REG_CMD_2);
    cr_writel(TPC,                  CR_REG_CMD_3);
    cr_writel(0x00,                 CR_REG_CMD_4);  // Reserved
    cr_writel(sector_count,         CR_REG_CMD_5);
    cr_writel(sector_count>>8,      CR_REG_CMD_6);

    //check command finish
    rtkcr_wait_opt_end(NULL,R_W_CMD);

    cr_int_status = cr_readl(CR_INT_STATUS);
    cr_writel((CIS_DECODE_FINISH|CIS_DECODE_ERROR),CR_INT_STATUS);

    rtkms_read_rsp(stat, 2);

    //if (stat[0] & 0xC2) {
    if (stat[0] & (MS_ERR_CRC|MS_ERR_TIMEOUT|MS_STA_ERR)) {
        return -1;
    }

    if (status) {
        *status = stat[0];
    }

    return 0;
}
*/

static u32 MSP_ReadBytes_Cmd(u8 TPC, u8 byte_count,
                            unsigned char *data, int timeout)
{
    int rtn_cnt;
    int i;
    unsigned char *buf;
    u32 cr_int_status=0;
    mmcinfo("\n");

    if (byte_count % 2) {
        rtn_cnt = byte_count + 1;
    } else {
        rtn_cnt = byte_count;
    }

    buf = (unsigned char *)kmalloc(rtn_cnt, GFP_KERNEL);

    if ( !buf ) {
        printk(KERN_WARNING "[%s] Allocation buf fails\n", __FUNCTION__);
        return -ENOMEM;
    }

    cr_writel(MS_READBYTES>>8,  CR_REG_CMD_1);
    cr_writel(MS_READBYTES,     CR_REG_CMD_2);
    cr_writel(TPC,              CR_REG_CMD_3);
    cr_writel(0,                CR_REG_CMD_4); // Reserved
    cr_writel(byte_count,       CR_REG_CMD_5);
    cr_writel(0,                CR_REG_CMD_6);  // Reserved

    /* check command finish */
    rtkcr_wait_opt_end(NULL,R_W_CMD);

    cr_int_status = cr_readl(CR_INT_STATUS);
    cr_writel((CIS_DECODE_FINISH|CIS_DECODE_ERROR),CR_INT_STATUS);

    rtkms_read_rsp(buf, rtn_cnt);

    if (data != NULL) {
        for (i=0; i<byte_count; i++) {

            data[i] = buf[i];
        }
        if (rtn_cnt > byte_count) {

            data[byte_count-1] = buf[rtn_cnt-1];
        }
    }

    kfree(buf);
    return 0;
}

int MSP_ReadBytes(u8 TPC, u8 byte_count,
    unsigned char *data, int timeout)
{
    u32 rc;
    mmcinfo("\n");

    rc = MSP_ReadBytes_Cmd(TPC, byte_count, data, timeout);

    return rc;
}

static u32 MSP_WriteBytes_Cmd(u8 TPC, u8 option, u8 byte_count,
    unsigned char *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    int i;
    u8 bytecount;
    u8 fillcnt;
    u32 cr_int_status=0;

    mmcinfo("TPC:0x%x, option:0x%x, byte_cnt:0x%x\n"
            ,TPC,option,byte_count);

    if (byte_count > 10) {
        bytecount = 10;
    } else {
        bytecount = byte_count;
    }

    cr_writel(MS_WRITEBYTES>>8, CR_REG_CMD_1);
    cr_writel(MS_WRITEBYTES,    CR_REG_CMD_2);
    cr_writel(TPC,              CR_REG_CMD_3);
    cr_writel(option,           CR_REG_CMD_4);
    cr_writel(byte_count,       CR_REG_CMD_5);
    cr_writel(0,                CR_REG_CMD_6);  // Reserved

    for (i=0; i<bytecount; i++){
        cr_writel(data[i], CR_REG_CMD_7+i*4);
    }
    fillcnt=20-(bytecount+6);   //fill 0 to CR_REG_CMD_XX from CR_REG_CMD_XX to CR_REG_CMD_20.
    if(fillcnt){
        for (i=0; i<fillcnt; i++){
            cr_writel(0, CR_REG_CMD_7+(i+bytecount)*4);
        }
    }
    /* check command finish */
    rtkcr_wait_opt_end(NULL,R_W_CMD);
    cr_int_status = cr_readl(CR_INT_STATUS);
    cr_writel((CIS_DECODE_FINISH|CIS_DECODE_ERROR),CR_INT_STATUS);
    rtkms_read_rsp(stat, 2);

    if (status) {
        if (option & RW_AUTO_GET_INT) {
            /* RW_AUTO_GET_INT */
            *status = stat[1];
        } else {
            /* Non RW_AUTO_GET_INT */
            *status = stat[0];
        }
    }

    return 0;
}

int MSP_WriteBytes(u8 TPC, u8 wait_INT, u8 byte_count,
    unsigned char *data, unsigned char *status, int timeout)
{
    u32 rc;
    mmcinfo("\n");

    rc = MSP_WriteBytes_Cmd(TPC, wait_INT, byte_count,data, status, timeout);

    return rc;
}

static u32 MSP_AutoRead_Cmd(u8 TPC, u16 sector_count,
    void *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    int total_bytes;
    u32 cr_int_status=0;

    mmcinfo("TPC:0x%x; sector_count:0x%x\n",TPC,sector_count);
    total_bytes = 512 * sector_count;

    cr_writel((unsigned int) data,  CR_DMA_IN_ADDR);
    cr_writel(MS_AUTOREAD>>8,       CR_REG_CMD_1);
    cr_writel(MS_AUTOREAD,          CR_REG_CMD_2);
    cr_writel(TPC,                  CR_REG_CMD_3);
    cr_writel(0,                    CR_REG_CMD_4);  // Reserved
    cr_writel(sector_count,         CR_REG_CMD_5);
    cr_writel(sector_count>>8,      CR_REG_CMD_6);

    /* check command finish */
    rtkcr_wait_opt_end(NULL,R_W_CMD);

    cr_int_status = cr_readl(CR_INT_STATUS);
        cr_writel((CIS_DECODE_FINISH|CIS_DECODE_ERROR),CR_INT_STATUS);

    rtkms_read_rsp(stat, 2);

    if (status) {
        *status = stat[0];
    }

    return 0;
}


int MSP_AutoRead(u8 TPC, u16 sector_count,
    void *data, unsigned char *status, int timeout)
{
    u32 rc;
    mmcinfo("\n");

    rc = MSP_AutoRead_Cmd(TPC, sector_count,data, status, timeout);

    return rc;

}

/* Mark for MS use only, can't use at MS-Pro *** */
/*
static u32 MSP_NormalRead_Cmd(u8 TPC, u8 option,
    unsigned char *data, unsigned char *status, int timeout)

{
    unsigned char stat[2];
    u32 cr_int_status=0;
    mmcinfo("\n");
    cr_writel((unsigned int) data,  CR_DMA_IN_ADDR);
    cr_writel(MS_NORMALREAD>>8,     CR_REG_CMD_1);
    cr_writel(MS_NORMALREAD,        CR_REG_CMD_2);
    cr_writel(TPC,                  CR_REG_CMD_3);
    cr_writel(option,               CR_REG_CMD_4);

    //check command finish
    rtkcr_wait_opt_end(NULL,R_W_CMD);

    cr_int_status = cr_readl(CR_INT_STATUS);
    cr_writel((CIS_DECODE_FINISH|CIS_DECODE_ERROR),CR_INT_STATUS);

    rtkms_read_rsp(stat, 2);

    if (status) {
        if (option & RW_AUTO_GET_INT) {
            *status = stat[1];
        } else {
            *status = stat[0];
        }
    }

    return 0;
}

int MSP_NormalRead(u8 TPC, u8 wait_INT,
    unsigned char *data, unsigned char *status, int timeout)
{
    u32 rc;
    mmcinfo("\n");

    rc = MSP_NormalRead_Cmd(TPC, wait_INT, data, status, timeout);

    return rc;
}
*/
/* Mark for MS use only, can't use at MS-Pro &&& */

static u32 MSP_AutoWrite_Cmd(u8 TPC, u16 sector_count,
    void *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    u32 cr_int_status=0;
    mmcinfo("\n");

    cr_writel((unsigned int) data,  CR_DMA_OUT_ADDR);
    cr_writel(MS_AUTOWRITE>>8,      CR_REG_CMD_1);
    cr_writel(MS_AUTOWRITE,         CR_REG_CMD_2);
    cr_writel(TPC,                  CR_REG_CMD_3);
    cr_writel(0x00,                 CR_REG_CMD_4);  // Reserved
    cr_writel(sector_count,         CR_REG_CMD_5);
    cr_writel(sector_count>>8,      CR_REG_CMD_6);

    //check command finish
    rtkcr_wait_opt_end(NULL,R_W_CMD);

    cr_int_status = cr_readl(CR_INT_STATUS);
    cr_writel((CIS_DECODE_FINISH|CIS_DECODE_ERROR),CR_INT_STATUS);

    rtkms_read_rsp(stat, 2);

    if (stat[0] & 0xC2) {
        return -1;
    }

    if (status) {
        *status = stat[0];
    }

    return 0;
}

int MSP_AutoWrite(u8 TPC, u16 sector_count,
    void *data, unsigned char *status, int timeout)
{
    u32 rc;
    mmcinfo("\n");

    rc = MSP_AutoWrite_Cmd(TPC, sector_count,
        data, status, timeout);


    return rc;
}

int MSP_SetRWRegAdrs(unsigned char readstart,  unsigned char readcount,
                     unsigned char writestart, unsigned char writecount)
{
    mmcinfo("\n");

    unsigned char data[4], status;
    int rc;

    data[0] = readstart;
    data[1] = readcount;
    data[2] = writestart;
    data[3] = writecount;

    rc = MSP_WriteBytes(MSPRO_TPC_SET_RW_REG_ADRS,
                        0, 4, data, &status, USUAL_TIMEOUT);
    if (rc) {
        printk(KERN_WARNING "MS_WriteBytes() fails, rc=%d\n", rc);
        return -1;
    }

    return 0;

}

static void rtkms_request(struct mmc_host *host, struct mmc_request *mrq)
{
    int rc=0;
    struct rtksd_port *sdport = mmc_priv(host);
    struct mmc_data *data = 0;
    unsigned char *data_buf = 0;
    mmcinfo("\n");

    /* try to grap the semaphore and see if the device is available */
    if (down_interruptible (&sem)) {
        printk(KERN_WARNING "%s : user breaking\n",__FUNCTION__);
        return ;
    }

    BUG_ON(sdport->mrq != NULL);
    if ( mrq->data ){
        data = mrq->data;
        sdport->size = mrq->sector_cnt * 512;
    }
    sdport->mrq = mrq;

    mrq->error = 0;

    if ( data && (data->flags&MMC_DATA_WRITE) &&
        (mrq->tpc==MSPRO_TPC_WRITE_LONG_DATA) )
    {
        rtkcr_sg_to_dma(sdport, data);
#if SW_TRANSFORM_ENDIAN
        mmcinfo("SW_TRANSFORM_ENDIAN\n");
        transform_data_format(sdport->dma_buffer, mrq->sector_cnt*512);
#endif
    }

    switch(mrq->tpc){
        case MSPRO_TPC_GET_INT:
            rc =MSP_ReadBytes( MSPRO_TPC_GET_INT,
                               mrq->byte_count,
                               mrq->reg_data, USUAL_TIMEOUT);
            break;

        case MSPRO_TPC_READ_REG:
            rc =MSP_ReadBytes( MSPRO_TPC_READ_REG,
                               mrq->reg_rw_addr->r_length,
                               mrq->reg_data, USUAL_TIMEOUT);
            if (mrq->reg_data[0] & WP_FLAG) {
                host->card->state |= MMC_STATE_WP;
                //rtk_ms_wp= 1;
            } else {
                //rtk_ms_wp = 0;
            }
            break;

        case MSPRO_TPC_WRITE_REG:
            rc = MSP_WriteBytes(MSPRO_TPC_WRITE_REG,
                                mrq->option, mrq->byte_count,
                                mrq->reg_data, &(mrq->status), USUAL_TIMEOUT);
            break;

        case MSPRO_TPC_READ_LONG_DATA:
            if ( data && (data->flags&MMC_DATA_READ) ){
                data_buf = sdport->dma_buffer;
            }else{
                data_buf = mrq->long_data;
            }

            CR_FLUSH_CACHE((unsigned long) data_buf, mrq->sector_cnt*512);

            if ( host->card->card_type & CR_MS_PRO ){

                rc = MSP_AutoRead(MSPRO_TPC_READ_LONG_DATA, mrq->sector_cnt,
                                  data_buf, &(mrq->status), mrq->timeout);
            }else if( host->card->card_type & CR_MS ){
                printk(KERN_WARNING "No support MS-PRO\n");
                BUG_ON(1);

            }else{
                printk(KERN_WARNING "unknow Card Type\n");
                BUG_ON(1);
            }

#if SW_TRANSFORM_ENDIAN
            mmcinfo("SW_TRANSFORM_ENDIAN\n");
            if ( data && (data->flags&MMC_DATA_READ) ){
                transform_data_format(data_buf, mrq->sector_cnt*512);
            }else{
                transform_data_format(data_buf, 16+400+96+48);
                rtk_cr_ms_dump_data (data_buf, 0, 16+400+96+48, (__u8 *)__FUNCTION__, __LINE__, AFTER_CALL);
                transform_data_format(data_buf+656, 16);
                rtk_cr_ms_dump_data (data_buf, 656, 16, (__u8 *)__FUNCTION__, __LINE__, AFTER_CALL);
            }
#endif
            break;

        case MSPRO_TPC_WRITE_LONG_DATA:
            if(host->card->state & MMC_STATE_WP)
                return ;

            CR_FLUSH_CACHE((unsigned long) sdport->dma_buffer, mrq->sector_cnt*512);

            rc = MSP_AutoWrite( MSPRO_TPC_WRITE_LONG_DATA, mrq->sector_cnt,
                                sdport->dma_buffer, &(mrq->status), mrq->timeout);
            break;

        case MSPRO_TPC_SET_CMD:
            rc = MSP_WriteBytes(MSPRO_TPC_SET_CMD, mrq->option, mrq->byte_count,
                                mrq->reg_data, &(mrq->status), USUAL_TIMEOUT);
            break;

        case MSPRO_TPC_SET_RW_REG_ADRS:
            rc =MSP_SetRWRegAdrs(mrq->reg_rw_addr->r_offset, mrq->reg_rw_addr->r_length,
                                 mrq->reg_rw_addr->w_offset, mrq->reg_rw_addr->w_length);
            break;

        default:
            printk(KERN_WARNING "DO NOT support TPC %x !!\n", mrq->tpc);
    }

    if (rc){
        mrq->error = rc;
    }

    if ( cr_readl(CR_INT_STATUS) & CIS_DECODE_ERROR ){
        printk("[%s] command parser error!!, mrq->tpc=%#x\n", __FUNCTION__, mrq->tpc);
        cr_writel( CIS_DECODE_ERROR, CR_INT_STATUS );
        mrq->error = -2;
    }

    if ( data && (data->flags&MMC_DATA_READ) &&
        (mrq->tpc==MSPRO_TPC_READ_LONG_DATA) ){
        rtkcr_dma_to_sg(sdport, data);
        data->bytes_xfered = sdport->size;
    }

    if ( data && (data->flags&MMC_DATA_WRITE) &&
        (mrq->tpc==MSPRO_TPC_WRITE_LONG_DATA) ){
        data->bytes_xfered = sdport->size;
    }

    rtkcr_request_end(sdport, mrq);
    up(&sem);
}

void rtkms_set_para(struct mmc_host *host,u8 para)
{
    mmcinfo("\n");
    cr_writel(MS_SETPARAMETER >> 8, CR_REG_CMD_1);
    cr_writel(MS_SETPARAMETER ,     CR_REG_CMD_2);
    cr_writel(para,                 CR_REG_CMD_3);
    cr_writel(0xff,                 CR_REG_CMD_4);

    rtkcr_wait_opt_end(host,INN_CMD);
}

static void rtkms_set_ios(struct mmc_host *host, struct mmc_ios *ios)
{
    struct rtksd_port *sdport = mmc_priv(host);

    spin_lock_bh(&sdport->rtlock);

    if (ios->bus_width == MMC_BUS_WIDTH_4){
        if((sdport->rtflags & MMC_BUS_WIDTH_4)==0){
            printk("MS-PRO:PARALLEL mode\n");
            rtkms_set_para(host,MS_4_BIT_MODE|SAMPLE_RISING|PUSH_TIME);
            sdport->rtflags|=MMC_BUS_WIDTH_4;
        }
    }

    if (ios->bus_mode == MMC_BUSMODE_PUSHPULL){     //normal speed
        rtkcr_set_clk(host,CARD_SWITCHCLOCK_48MHZ);
    }else{  //MS_BUSMODE_OPENDRAIN
        rtkcr_set_clk(host,CARD_SWITCHCLOCK_30MHZ);  //default clock
    }

    if (ios->power_mode == MMC_POWER_OFF){
        rtkcr_host_power_off(host,(u8 *)__FUNCTION__);
        rtkcr_card_power(host,OFF,(u8 *)__FUNCTION__);
    }else{  //MS_POWER_UP or MS_POWER_NO
        rtkcr_host_power_on(host,(u8 *)__FUNCTION__);
        rtkcr_card_power(host,ON,(u8 *)__FUNCTION__);
    }

    spin_unlock_bh(&sdport->rtlock);

}


static struct mmc_host_ops rtkms_ops = {
    .request    = rtkms_request,
    .set_ios    = rtkms_set_ios,
};
/* for MS use &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& */
static void rtkcr_chk_card_insert(struct rtksd_port *sdport)
{
    int reginfo;
    struct mmc_host *host=sdport->mmc;

    reginfo = cr_readl(CR_INT_STATUS);

    if(reginfo & 0x10){
        reginfo = rtkcr_polling(host);
        if(reginfo<0){
            goto non_card;
        }

        if ( reginfo & SD_CARD_EXIST ){
            if ( rtkcr_polling(host) & SD_CARD_WP ){
                sdport->wp=1;
            }else{
                sdport->wp=0;
            }

            sdport->rtflags |= RTKSD_FCARD_DETECTED;
            host->card_type_pre = CR_SD;
            mmcspec("MMC/SD/SDHC card detect~ \n");
        }else if( reginfo & MS_CARD_EXIST ){
            mmcspec("MS_PRO card detect~\n");
            goto non_card;

        }else{
            mmcspec("no card inside~\n");
            goto non_card;
        }

    }else{
non_card:
        mmcspec("no card detect~\n");
        sdport->rtflags &= ~RTKSD_FCARD_DETECTED;
    }
}

void rtkcr_clr_port_sta(struct rtksd_port *sdport)
{
    sdport->rtflags=0;
    sdport->act_host_clk=0;
    sdport->rt_parameter=0;
    sdport->size=0;
    sdport->wp=0;

}

unsigned long card_event_start;
static void rtkcr_work_fn(void* ptr)
{
    struct mmc_host *host=ptr;
    struct rtksd_port *sdport = mmc_priv(host);
    struct mmc_card *card = host->card;

    if(down_interruptible(&host->mmc_sema))
    {
        printk("Work Func semaphore fail\n");
        return;
    }

    if(host->ins_event==EVENT_INSER ){      //card insert
        if(host->card==NULL){
            if(time_before(jiffies, card_event_start)){
                schedule_delayed_work(&rtkcr_work,HZ>>5);
                up(&host->mmc_sema);
                return;
            }
            mmcspec("Start detect card.\n");
            rtkcr_chk_card_insert(sdport);

            if(sdport->rtflags & RTKSD_FCARD_DETECTED){
                mmcspec("card insert~~  \n");

                if(host->card_type_pre!= CR_MS){
                    host->ops = &rtksd_ops;
                }else{
                    goto non_card_out;
                    //host->ops = &rtkms_ops;
                }
                schedule_work(&host->detect);   //mmc_rescan
                host->ins_event=EVENT_NON;
            }else{
                goto non_card_out;
            }
        }
    }else{

non_card_out:

        sdport->rtflags &= ~RTKSD_FCARD_DETECTED;

        if(card){
            host->ins_event=EVENT_REMOV;
            rtkcr_clr_port_sta(sdport);
            schedule_work(&host->detect);   //mmc_rescan
        }else{
            host->ins_event=EVENT_NON;
        }

        if(host->card_selected){
            host->card_selected = NULL;
        }
    }

    up(&host->mmc_sema);
}

//#define SHOW_INT_STATUS
irqreturn_t rtkcr_interrupt (int irq, void *dev_instance, struct pt_regs *regs)
{
    unsigned int reginfo;
    unsigned int handled = 0;
    struct mmc_host *host=dev_instance;

    reginfo=cr_readl(CR_INT_STATUS);

    if(reginfo&0x01){

#ifdef  SHOW_INT_STATUS
        //mmcinfo("dev_instance addr :0x%p\n",dev_instance);
        //mmcinfo("mmc_host addr :0x%p\n",host);
        printk("%s(%u)CR_INT_STATUS:0x%x\n",__func__,__LINE__,reginfo);
#endif
        cr_writel(CIS_CARD_STATUS_CHANGE, CR_INT_STATUS);

        if((reginfo>>4)&0x01){
            host->ins_event=EVENT_INSER;
            card_event_start=jiffies+3*HZ;
        }else{
            card_event_start=jiffies;
            host->ins_event=EVENT_REMOV;
        }
        schedule_work(&rtkcr_work);

        handled=1;  //cr_sta_chg_handler(host);
    }

    return IRQ_RETVAL(handled);
}

static char *rtkcr_parse_token(const char *parsed_string, const char *token)
{
    const char *ptr = parsed_string;
    const char *start, *end, *value_start, *value_end;
    char *ret_str;

    while(1) {
        value_start = value_end = 0;
        for(;*ptr == ' ' || *ptr == '\t'; ptr++);
        if(*ptr == '\0')        break;
        start = ptr;
        for(;*ptr != ' ' && *ptr != '\t' && *ptr != '=' && *ptr != '\0'; ptr++) ;
        end = ptr;
        if(*ptr == '=') {
            ptr++;
            if(*ptr == '"') {
                ptr++;
                value_start = ptr;
                for(; *ptr != '"' && *ptr != '\0'; ptr++);
                if(*ptr != '"' || (*(ptr+1) != '\0' && *(ptr+1) != ' ' && *(ptr+1) != '\t')) {
                    printk("system_parameters error! Check your parameters     .");
                    break;
                }
            } else {
                value_start = ptr;
                for(;*ptr != ' ' && *ptr != '\t' && *ptr != '\0' && *ptr != '"'; ptr++) ;
                if(*ptr == '"') {
                    printk("system_parameters error! Check your parameters.");
                    break;
                }
            }
            value_end = ptr;
        }

        if(!strncmp(token, start, end-start)) {
            if(value_start) {
                ret_str = kmalloc(value_end-value_start+1, GFP_KERNEL);
                strncpy(ret_str, value_start, value_end-value_start);
                ret_str[value_end-value_start] = '\0';
                return ret_str;
            } else {
                ret_str = kmalloc(1, GFP_KERNEL);
                strcpy(ret_str, "");
                return ret_str;
            }
        }
    }

    return (char*)NULL;
}

void rtkcr_chk_param(u32 *pparam, u32 len, u8 *ptr)
{
    unsigned int value,i;
    for(i=0;i<len;i++){
        value = ptr[i] - '0';
        if((value >= 0) && (value <=9))
        {
            *pparam+=value<<(4*(len-1-i));
            continue;
        }

        value = ptr[i] - 'a';
        if((value >= 0) && (value <=5))
        {
            value+=10;
            *pparam+=value<<(4*(len-1-i));
            continue;
        }

        value = ptr[i] - 'A';
        if((value >= 0) && (value <=5))
        {
            value+=10;
            *pparam+=value<<(4*(len-1-i));
            continue;
        }
    }
}

/* get card power GPIO address and control bit */
int rtkcr_get_gpio(void)
{
    unsigned char *cr_param;
    void __iomem *mmio;
    //u32 power_bits_tmp=0;

    cr_param=(char *)rtkcr_parse_token(platform_info.system_parameters,"cr_pw");
    if(cr_param){
        rtkcr_chk_param(&rtk_power_gpio,4,cr_param);
        rtkcr_chk_param(&rtk_power_bits,2,cr_param+5);

        mmio = (void __iomem *) (rtk_power_gpio+0xb8010000);
        writel(readl(mmio) | (1<<rtk_power_bits),mmio); //enable GPIO output

        if((rtk_power_gpio & 0xf00) ==0x100){
            rtk_power_gpio+=0xb8010010;
        }else if((rtk_power_gpio & 0xf00) ==0xd00){
            rtk_power_gpio+=0xb8010004;
        }else{
            printk(KERN_ERR "wrong GPIO of card's power.\n");
            return -1;
        }
        mmcinfo("power_gpio = 0x%x\n",rtk_power_gpio);
        mmcinfo("power_bits = %d\n",rtk_power_bits);
        return 0;
    }
    printk(KERN_ERR "Can't find GPIO of card's power.\n");
    return -1;

}

/*
 * Allocate/free MMC structure.
 */
struct mmc_host *rtk_host;
static int __devinit rtkcr_init_dev(struct device *dev)
{
    struct mmc_host *host;
    struct rtksd_port *sdport;

    mmcinfo("\n");
    /*
     * Allocate MMC structure.
     */
    host = mmc_alloc_host(sizeof(struct rtksd_port), dev);  //create mmc_host and  rtksd_port
    rtk_host=host;

    if (!host)
        return -ENOMEM;

    sdport = mmc_priv(host);
    sdport->mmc = host;
    sdport->rtlock=&host->lock;

    /* Set host parameters. */
    //host->ops = &rtksd_ops;   //until detect MMC/MS then asigned
    host->f_min = 375000;
    host->f_max = 24000000;
    host->ocr_avail = RTL_HOST_VDD;//MMC_VDD_32_33|MMC_VDD_33_34;

    sema_init(&host->mmc_sema, 1);
    spin_lock_init(sdport->rtlock);

    /*
     * Maximum number of segments. Worst case is one sector per segment
     * so this will be 64kB/512.
     */
#ifdef TEST_DSC_MOD
    host->max_hw_segs   = 128;
    host->max_phys_segs = 128;
    host->max_sectors   = 128;    /*Maximum number of sectors in one transfer. Also limited by 64kB buffer.*/
#else
    host->max_hw_segs   = 128;
    host->max_phys_segs = 128;
    host->max_sectors   = 128;    /*Maximum number of sectors in one transfer. Also limited by 64kB buffer.*/
#endif

    host->max_seg_size = host->max_sectors * 512;   /*Maximum segment size. Could be one segment with the maximum number of segments.*/
    host->irq = 4;

    /* interrupt setting */
    if (request_irq(host->irq, rtkcr_interrupt, SA_SHIRQ, DRIVER_NAME, host)){
        printk("IRQ request fail\n");
        return -1;
    }

    INIT_WORK(&rtkcr_work, rtkcr_work_fn, host);

    cr_writel(0xff, CR_DMA_STATUS); /* clear dma int status */
    cr_writel(0x00, CR_DMA_CTRL);   /* disable dma interrupt */
    cr_writel(0xff, CR_INT_STATUS); /* clear int status */
    cr_writel(0x01, CR_INT_MASK);   /* enable interrupt */

    /* dev->driver_data = host; */
    dev_set_drvdata(dev, host);

    return 0;
}

static void __devexit rtkcr_free_dev(struct device* dev)
{
    struct mmc_host* host;

    host = dev_get_drvdata(dev);
    if (!host)
        return;

    mmc_free_host(host);

    dev_set_drvdata(dev, NULL);
}

static int  __devexit rtkcr_device_remove(struct device* dev)
{
    struct mmc_host* host = dev_get_drvdata(dev);

    mmcinfo("\n");

    if ( !host ){
        return -1;
    }

    cr_writel(0x00, CR_INT_MASK);   /* disable interrupt */

    cancel_delayed_work(&rtkcr_work);
    flush_scheduled_work();
    free_irq(host->irq, host);
    mmc_remove_host(host);

#ifdef MEM_COPY_METHOD
    if(1){
        struct rtksd_port* sdport;
        sdport = mmc_priv(host);
        if ( sdport->dma_buffer ){

            kfree(sdport->dma_buffer);
        }
    }
#endif
    rtkcr_free_dev(dev);
    return 0;
}

/*
 * Allocate/free DMA buffer
 */

#ifdef MEM_COPY_METHOD
static int rtkcr_request_dma(struct rtksd_port *sdport)
{
    mmcinfo("\n");
    /* We need to allocate a special buffer to be able to DMA to it. */
    sdport->dma_buffer = (unsigned char *)kmalloc(DMA_BUFFER_SIZE, GFP_KERNEL);
    if ( !sdport->dma_buffer ){
        printk(KERN_ERR "Allocate sdport->dma_buffer fails\n");
        return -ENOMEM;
    }
    return 0;
}
#endif
/*
 * Non-PnP
 */
static int __devinit rtkcr_device_probe(struct device *dev)
{
    struct rtksd_port *sdport = NULL;
    struct mmc_host *host = NULL;
    int rc = 0;

    mmcinfo("\n");

    rc = rtkcr_init_dev(dev);
    if (rc){
        printk(KERN_WARNING "MMC controller initial fails\n");
        goto error_out;
    }

    if ( (host = dev_get_drvdata(dev)) == NULL ){
        printk("[%s] dev_get_drvdata() fails\n", __FUNCTION__);
        rtkcr_free_dev(dev);
        goto error_out;
    }

    if ( (sdport = mmc_priv(host)) == NULL ){
        printk("[%s] mmc_priv() fails\n", __FUNCTION__);
        rtkcr_free_dev(dev);
        goto error_out;
    }
    if(1){  //alex
        unsigned int reginfo;
        reginfo=cr_readl(0xb800030C);   //pull up function for SD/MMC write protect pad
        cr_writel(reginfo|0x09, 0xb800030C);
    }

    rtkcr_chk_card_insert(sdport);

    /*
     * Allocate DMA buffer.
     */

#ifndef TEST_DSC_MOD
    rc = rtkcr_request_dma(sdport);
    if (rc){
        printk(KERN_WARNING "rtkcr_request_dma() fails\n");
        goto error_out;
    }
#endif

    if(sdport->rtflags & RTKSD_FCARD_DETECTED){
        if(host->card_type_pre != CR_MS){
            host->ops = &rtksd_ops;
        }else{
            BUG_ON(1);
            host->ops = &rtkms_ops;
        }

        mmc_add_host(host);
    }

    return 0;
error_out:
    return -1;
}

static void rtkcr_display_version (void)
{
    const __u8 *revision;
    const __u8 *date;
    const __u8 *time;
    char *running = (__u8 *)VERSION;

    strsep(&running, " ");
    strsep(&running, " ");
    revision = strsep(&running, " ");
    date = strsep(&running, " ");
    time = strsep(&running, " ");
    printk(BANNER " Rev:%s (%s %s)\n", revision, date, time);

}

#ifdef CONFIG_PM
static int rtkcr_suspend(struct device *dev, pm_message_t state, u32 level)
{
    //struct mmc_host * host=rtk_host;
    unsigned int reginfo;

    cr_writel(0x00, CR_INT_MASK);   /* disable interrupt */
    reginfo=cr_readl(CR_INT_STATUS);
    reginfo=reginfo&0xfe;
    cr_writel(reginfo, CR_INT_STATUS); /* clear int status */
    mdelay(10);
    //mmcspec("host->ins_event=%u\n",host->ins_event);
    return 0;
}

static int rtkcr_resume(struct device *dev, u32 level)
{
    struct mmc_host * host=rtk_host;
    //unsigned int reginfo;

    cr_writel(0x01, CR_INT_MASK);   /* enable interrupt */
    host->ins_event=EVENT_INSER;
    schedule_work(&rtkcr_work);
    return 0;
}
#else
#define rtkcr_suspend NULL
#define rtkcr_resume NULL
#endif


static struct device_driver rtksd_driver = {
    .name       = DRIVER_NAME,
    .bus        = &platform_bus_type,
    .probe      = rtkcr_device_probe,
    .remove     = rtkcr_device_remove,
#ifdef CONFIG_PM
    .suspend    = rtkcr_suspend,
    .resume     = rtkcr_resume,
#endif
};

static ssize_t
show_cr_getCID_field(struct bus_type *bus, char *buf)
{
    struct mmc_host * host=rtk_host;
    struct mmc_card *card=host->card;

    if(host && card){
        u8 *pcid = (u8 *)card->raw_cid;

        return sprintf(buf, "0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n",
            pcid[3] ,pcid[2] ,pcid[1] ,pcid[0],
            pcid[7] ,pcid[6] ,pcid[5] ,pcid[4],
            pcid[11],pcid[10],pcid[9] ,pcid[8],
            pcid[15],pcid[14],pcid[13],pcid[12]);
    }
    return sprintf(buf, "0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 \n");
}

static ssize_t
store_cr_getCID_field(struct bus_type *bus, const char *buf, size_t count)
{
    return 1;
}
static BUS_ATTR(cr_getCID, S_IRUGO | S_IWUSR,
                show_cr_getCID_field, store_cr_getCID_field);

static ssize_t
show_cr_chkwp_field(struct bus_type *bus, char *buf)
{
    struct mmc_host * host=rtk_host;
    struct rtksd_port *sdport = mmc_priv(host);
    if(sdport->wp == 1)
        return sprintf(buf, "1\n");
    else
        return sprintf(buf, "0\n");
}

static ssize_t
store_cr_chkwp_field(struct bus_type *bus, const char *buf, size_t count)
{
    return 1;
}
static BUS_ATTR(cr_chkwp, S_IRUGO | S_IWUSR,
                show_cr_chkwp_field, store_cr_chkwp_field);


static ssize_t
show_cr_offline_field(struct bus_type *bus, char *buf)
{
    struct mmc_host * host=rtk_host;
    unsigned int reginfo;
    cr_writel(0x00, CR_INT_MASK);   // disable interrupt
    reginfo=cr_readl(CR_INT_STATUS);
    reginfo=reginfo&0xfe;
    cr_writel(reginfo, CR_INT_STATUS); // clear int status
    host->ins_event=EVENT_REMOV;
    schedule_work(&rtkcr_work);
    return sprintf(buf, "off-line card\n");
}

static ssize_t
store_cr_offline_field(struct bus_type *bus, const char *buf, size_t count)
{
    return 1;
}

static BUS_ATTR(cr_offline, S_IRUGO | S_IWUSR,
                show_cr_offline_field, store_cr_offline_field);

static ssize_t
show_cr_rescan_field(struct bus_type *bus, char *buf)
{
    struct mmc_host * host=rtk_host;
    unsigned int reginfo;
    cr_writel(0x00, CR_INT_MASK);   /* disable interrupt */
    reginfo=cr_readl(CR_INT_STATUS);
    reginfo=reginfo&0xfe;
    cr_writel(reginfo, CR_INT_STATUS); /* clear int status */
    host->ins_event=EVENT_REMOV;
    schedule_work(&rtkcr_work);
    cr_writel(0x01, CR_INT_MASK);   /* enable interrupt */
    host->ins_event=EVENT_INSER;
    schedule_work(&rtkcr_work);
    return sprintf(buf, "re-scan card\n");
}
static ssize_t
store_cr_rescan_field(struct bus_type *bus, const char *buf, size_t count)
{
    return 1;
}
static BUS_ATTR(cr_rescan, S_IRUGO | S_IWUSR,
                show_cr_rescan_field, store_cr_rescan_field);

static int __init rtkcr_init(void)
{
    int rc = 0;
    unsigned int intmod;

    rtkcr_display_version();
    rc = rtkcr_get_gpio();

    intmod=cr_readl(CRT_MUXPAD5) & MUX_MMC_DET_MASK;
    cr_writel(intmod|DET_SD_MOD,CRT_MUXPAD5);

    intmod=cr_readl(CRT_MUXPAD5) & MUX_MMC_WP_MASK;
    cr_writel(intmod|WP_SD_MOD,CRT_MUXPAD5);

    if (rc < 0)
        goto err_power_fail;

    rtkcr_card_power(NULL,OFF,(u8 *)__FUNCTION__);
    rtkcr_host_power_off(NULL,(u8 *)__FUNCTION__);

    bus_create_file(&mmc_bus_type, &bus_attr_cr_rescan);
    bus_create_file(&mmc_bus_type, &bus_attr_cr_offline);
    bus_create_file(&mmc_bus_type, &bus_attr_cr_chkwp);
    bus_create_file(&mmc_bus_type, &bus_attr_cr_getCID);

    rc = driver_register(&rtksd_driver);
    if (rc < 0)
        goto err_driver_fail;

    rtksd_device = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);

    if ( IS_ERR(rtksd_device) ){
        rc = PTR_ERR(rtksd_device);
        goto err_device_fail;
    }
    printk(KERN_INFO "Realtek SD/MMC Controller Driver is successfully installing.\n\n");

    return 0;

err_device_fail:
    platform_device_unregister(rtksd_device);

err_driver_fail:
    driver_unregister(&rtksd_driver);
    bus_remove_file(&mmc_bus_type, &bus_attr_cr_rescan);
    bus_remove_file(&mmc_bus_type, &bus_attr_cr_offline);
    bus_remove_file(&mmc_bus_type, &bus_attr_cr_chkwp);
    bus_remove_file(&mmc_bus_type, &bus_attr_cr_getCID);

err_power_fail:
    printk(KERN_INFO "Realtek SD/MMC Controller Driver installation fails.\n\n");
    return -ENODEV;
}

static void __exit rtkcr_exit(void)
{
    platform_device_unregister(rtksd_device);
    driver_unregister(&rtksd_driver);
    rtkcr_host_power_off(NULL,(u8 *)__FUNCTION__);
    bus_remove_file(&mmc_bus_type, &bus_attr_cr_rescan);
    bus_remove_file(&mmc_bus_type, &bus_attr_cr_offline);
    bus_remove_file(&mmc_bus_type, &bus_attr_cr_chkwp);
    bus_remove_file(&mmc_bus_type, &bus_attr_cr_getCID);
}

module_init(rtkcr_init);
module_exit(rtkcr_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("CMYu<Ken.Yu@realtek.com>");
MODULE_DESCRIPTION("Realtek SD/MMC Controller Driver");

