/*
 *  linux/drivers/mmc/mmc.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *
 *  SD support (c) 2004 Ian Molton.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/config.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/err.h>


#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/protocol_mmc.h>
#include <linux/mmc/protocol_ms.h>
#include <asm/scatterlist.h>
#include <linux/scatterlist.h>

#include "mmc.h"
#include "mmc_ops.h"
#include "ms_ops.h"
#include "mmc_debug.h"

#include <linux/rtk_cr.h>
#include <linux/crc7.h>

#ifdef CONFIG_MMC_DEBUG
#define DBG(x...)   printk(KERN_DEBUG x)
#else
#define DBG(x...)   do { } while (0)
#endif

#define CMD_RETRIES 3

/*
 * OCR Bit positions to 10s of Vdd mV.
 */

/*
static const unsigned short mmc_ocr_bit_to_vdd[] = {
    150,    155,    160,    165,    170,    180,    190,    200,
    210,    220,    230,    240,    250,    260,    270,    280,
    290,    300,    310,    320,    330,    340,    350,    360
};

static const unsigned int tran_exp[] = {
    10000,      100000,     1000000,    10000000,
    0,      0,      0,      0
};

static const unsigned char tran_mant[] = {
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};

static const unsigned int tacc_exp[] = {
    1,  10, 100,    1000,   10000,  100000, 1000000, 10000000,
};

static const unsigned int tacc_mant[] = {
    0,  10, 12, 13, 15, 20, 25, 30,
    35, 40, 45, 50, 55, 60, 70, 80,
};
*/

/**
 *  mmc_request_done - finish processing an MMC command
 *  @host: MMC host which completed command
 *  @mrq: MMC request which completed
 *
 *  MMC drivers should call this function when they have completed
 *  their processing of a command.  This should be called before the
 *  data part of the command has completed.
 */
void mmc_request_done(struct mmc_host *host, struct mmc_request *mrq)
{
    int err;
    mmcinfo("\n");

    if(host->card_type_pre!=CR_MS){
        struct mmc_command *cmd = mrq->cmd;
        err = mrq->cmd->error;

        DBG("MMC: req done (%02x): %d: %08x %08x %08x %08x\n", cmd->opcode,
            err, cmd->resp[0], cmd->resp[1], cmd->resp[2], cmd->resp[3]);

        if(host->ins_event==EVENT_REMOV){
            cmd->retries=0;
            mrq->done(mrq);
        }else{

            if (err && cmd->retries) {
                cmd->retries--;
                cmd->error = 0;
                host->ops->request(host, mrq);
            } else if (mrq->done) {
                mrq->done(mrq);
            }
        }
    }else{
        err = mrq->error;

        if(host->ins_event==EVENT_REMOV){
            mrq->done(mrq);

        }else{
            if (err){
                printk(KERN_WARNING "[%s] mrq->tpc=%#x error\n",
                        __FUNCTION__, mrq->tpc);
            }else if (mrq->done) {
                mrq->done(mrq);
            }
        }
    }
}

EXPORT_SYMBOL(mmc_request_done);

/**
 *  mmc_start_request - start a command on a host
 *  @host: MMC host to start command on
 *  @mrq: MMC request to start
 *
 *  Queue a command on the specified host.  We expect the
 *  caller to be holding the host lock with interrupts disabled.
 */
void mmc_start_request(struct mmc_host *host, struct mmc_request *mrq)
{
    //WARN_ON(host->card_busy == NULL);
    mmcinfo("\n");
    if(host->card_type_pre!=CR_MS){
        mrq->cmd->error = 0;
        mrq->cmd->mrq = mrq;

        if (mrq->data) {
            mrq->cmd->data = mrq->data;
            mrq->data->error = 0;
            mrq->data->mrq = mrq;
            if (mrq->stop) {
                mrq->data->stop = mrq->stop;
                mrq->stop->error = 0;
                mrq->stop->mrq = mrq;
            }
        }
    }else{
        if (mrq->data) {
            mrq->data->error = 0;
            mrq->data->mrq = mrq;
        }
    }

    host->ops->request(host, mrq);  //rtksd_request()/rtkms_request

}

EXPORT_SYMBOL(mmc_start_request);

static void mmc_wait_done(struct mmc_request *mrq)
{
    mmcinfo("\n");
    complete(mrq->done_data);
}

/**
 *      mmc_wait_for_req - start a request and wait for completion
 *      @host: MMC host to start command
 *      @mrq: MMC request to start
 *
 *      Start a new MMC custom command request for a host, and wait
 *      for the command to complete. Does not attempt to parse the
 *      response.
 */
int mmc_wait_for_req(struct mmc_host *host, struct mmc_request *mrq)
{
    DECLARE_COMPLETION(complete);
    mmcinfo("mrq->cmd->retries:%u\n",mrq->cmd->retries);
    mrq->done_data = &complete;
    mrq->done = mmc_wait_done;

    mmc_start_request(host, mrq);
    wait_for_completion(&complete);

    return 0;
}

EXPORT_SYMBOL(mmc_wait_for_req);

/*
static int cr_complete_cmd(struct mmc_host *host, struct mmc_card *card)
{
    //int err;
    struct mmc_command cmd;

    memset(&cmd, 0, sizeof(struct mmc_command));

    BUG_ON(!host);
    BUG_ON(card && (card->host != host));

    cmd.opcode = CMD12_STOP_TRANSMISSION;

    cmd.arg = 0;
    cmd.flags = MMC_RSP_R1B | MMC_CMD_AC;

    mmc_wait_for_cmd(host, &cmd, 0);

    return cmd.error;
}
*/
/*
 *  mmc_wait_for_cmd - start a command and wait for completion
 *  @host: MMC host to start command
 *  @cmd: MMC command to start
 *  @retries: maximum number of retries
 *
 *  Start a new MMC command for a host, and wait for the command
 *  to complete.  Return any error that occurred while the command
 *  was executing.  Do not attempt to parse the response.
 */
int mmc_wait_for_cmd(struct mmc_host *host, struct mmc_command *cmd, int retries)
{
    struct mmc_request mrq;

    BUG_ON(host->card_busy == NULL);
    mmcinfo("\n");

    memset(&mrq, 0, sizeof(struct mmc_request));

    cmd->retries = retries;
    mrq.cmd = cmd;

    mmc_wait_for_req(host, &mrq);

    return cmd->error;
}

EXPORT_SYMBOL(mmc_wait_for_cmd);

/*
 *  send SD/SDHC application command
 *  return val: cmd.error
 */
static int sd_app_cmd(struct mmc_host *host, struct mmc_card *card)
{
    struct mmc_command cmd;

    memset(&cmd, 0, sizeof(struct mmc_command));

    BUG_ON(!host);
    BUG_ON(card && (card->host != host));

    cmd.opcode = CMD55_APP_CMD;

    if (card) {
        cmd.arg = card->rca << 16;
        cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
    } else {
        cmd.arg = 0;
        cmd.flags = MMC_RSP_R1 | MMC_CMD_BCR;
    }

    mmc_wait_for_cmd(host, &cmd, 3);

    if(cmd.error){
        /* sd_app_cmd fail */
    }


    return cmd.error;
}

int mmc_send_status(struct mmc_card *card, u32 *status)
{
    //int err;
    struct mmc_command cmd;

    BUG_ON(!card);
    BUG_ON(!card->host);

    memset(&cmd, 0, sizeof(struct mmc_command));

    cmd.opcode = CMD13_SEND_STATUS;

    cmd.arg = card->rca << RCA_SHIFTER;
    cmd.flags = MMC_RSP_R1;

    mmc_wait_for_cmd(card->host, &cmd, CMD_RETRIES);

    /* NOTE: callers are required to understand the difference
     * between "native" and SPI format status words!
     */
    if (status)
        *status = cmd.resp[0];

    return cmd.error;;
}

#define R1_SWITCH_ERROR         (1 << 7)    /* sx, c */
#define MMC_STA_ERR_BIT         0xFDFFA000  //referencr mmc spec4.2 table23
int mmc_switch(struct mmc_card *card, u8 set, u8 index, u8 value)
{
    int err=0;
    struct mmc_command cmd;
    u32 status;
    unsigned long start;

    BUG_ON(!card);
    BUG_ON(!card->host);

    memset(&cmd, 0, sizeof(struct mmc_command));

    cmd.opcode  = CMD6_SWITCH;
    cmd.arg     =  (MMC_SWITCH_MODE_WRITE_BYTE << 24)
                 | (index << 16)
                 | (value << 8)
                 | set;
    cmd.flags = MMC_RSP_R1B;

    mmc_wait_for_cmd(card->host, &cmd, CMD_RETRIES);
    if (cmd.error)
        return cmd.error;

    /* Must check status to be sure of no errors */
    start=jiffies+HZ;
    do {
        if(time_after(jiffies, start))
            return -1;

        if(mmc_send_status(card, &status))
            return err;

    } while (R1_CURRENT_STATE(status) == 7);

    if (status & MMC_STA_ERR_BIT)
        printk(KERN_WARNING "unexpected status %#x after switch", status);

    if (status & R1_SWITCH_ERROR)
        return -EBADMSG;

    return 0;
}

int sd_switch(struct mmc_card *card, int mode, int group,u8 value, u8 *resp)
{
    struct mmc_request mrq;
    struct mmc_command cmd;
    struct mmc_data data;
    struct scatterlist sg;

    BUG_ON(!card);
    BUG_ON(!card->host);

    /* NOTE: caller guarantees resp is heap-allocated */

    mode = !!mode;
    value &= 0xF;

    memset(&mrq, 0, sizeof(struct mmc_request));
    memset(&cmd, 0, sizeof(struct mmc_command));
    memset(&data, 0, sizeof(struct mmc_data));

    mrq.cmd = &cmd;
    mrq.data = &data;

    cmd.opcode = CMD6_SWITCH_FUNC;
    cmd.arg = (mode << 31) | 0x00FFFFFF;
    cmd.arg &= ~(0xF << (group * 4));
    cmd.arg |= value << (group * 4);

    cmd.flags = MMC_RSP_R1 | MMC_CMD_ADTC;

    data.blksz = 64;
    data.blocks = 1;
    data.flags |= MMC_DATA_READ;
    data.sg = &sg;
    data.sg_len = 1;

    sg_init_one(&sg, resp, NORMAL_READ_BUF_SIZE);

    mmc_wait_for_req(card->host, &mrq);

    if (cmd.error || data.error)
        return -1;

    return 0;
}

static int sd_switch_hs(struct mmc_card *card)
{
    int err;
    u8 *status;
    struct mmc_host *host=card->host;

    err = -EIO;

    status = kmalloc(NORMAL_READ_BUF_SIZE, GFP_KERNEL);
    if (!status) {
        printk(KERN_ERR "could not allocate a buffer for "
            "switch capabilities.\n");
        return -ENOMEM;
    }

    err = sd_switch(card, 1, 0, 1, status);
    if (err)
        goto out;

    if ((status[16] & 0xF) != 1) {
        printk(KERN_WARNING "Problem switching card "
            "into high-speed mode!\n");
    } else {
        mmc_card_set_highspeed(card);
        host->ios.timing = MMC_TIMING_SD_HS;
        host->ops->set_ios(host, &host->ios);   //rtksd_set_ios
        msleep(30);

    }

out:
    kfree(status);

    return err;
}

static int sd_read_switch(struct mmc_card *card)
{
    int err;
    u8 *status;

    mmcinfo("\n");
    if (card->scr.sda_vsn < 1){  //SCR_SPEC_VER_1
        return 0;
    }

    if (!(card->csd.cmdclass & CCC_SWITCH)) {
        printk(KERN_WARNING "card lacks mandatory switch "
            "function, performance might suffer.\n");
        return 0;
    }

    err = -EIO;

    status = kmalloc(NORMAL_READ_BUF_SIZE, GFP_KERNEL);
    memset(status,0,NORMAL_READ_BUF_SIZE);
    if (!status) {
        printk(KERN_ERR "could not allocate a buffer for "
            "switch capabilities.\n");
        return -ENOMEM;
    }

    err = sd_switch(card, 0, 0, 1, status);
    if (err) {
        /* If the host or the card can't do the switch,
         * fail more gracefully. */
        if ((err != -EINVAL)
         && (err != -ENOSYS)
         && (err != -EFAULT))
            goto out;

        printk(KERN_WARNING "problem reading switch "
            "capabilities, performance might suffer.\n");
        err = 0;

        goto out;
    }
    msleep(10);
#ifdef SHOW_SWITCH_DATA
{
    u8 i;
    for(i=0;i<8;i++)
    {
        if(i==0)
            printk(KERN_INFO "switch data:\n");

        printk(KERN_INFO "{%03u ~ %03u} "
                         "0x%02x 0x%02x 0x%02x 0x%02x "
                         "0x%02x 0x%02x 0x%02x 0x%02x\n",
            i*8,i*8+7,
            status[i*8+0],status[i*8+1],status[i*8+2],status[i*8+3],
            status[i*8+4],status[i*8+5],status[i*8+6],status[i*8+7]);
    }
}
#endif

    if (status[13] & 0x02){
        printk(KERN_INFO "support high speed mode\n");
        sd_switch_hs(card);
    }

out:
    kfree(status);

    return err;
}

/*
 *  return value = 1 no support ext_csd
 *               = -1 read ext_csd fail
 *               = 0 success
 */
static int mmc_read_ext_csd(struct mmc_host *host,void * buf)
{
    struct mmc_card *card=host->card;
    struct mmc_csd *csd = &card->csd;
    int card_type = card->card_type;
    struct mmc_command cmd;
    struct mmc_request mrq;
    struct scatterlist sg;
    struct mmc_data data;

    mmcinfo("\n");

    if ( (card_type & (CR_SD|CR_SDHC))
    || ( (card_type==CR_MMC) && csd->mmca_vsn<4) ){
        /* Do not support EXT_CSD */
        return 1;
    }

    memset(&cmd, 0, sizeof(struct mmc_command));
    memset(&mrq, 0, sizeof(struct mmc_request));
    memset(&data, 0, sizeof(struct mmc_data));

    mrq.cmd     = &cmd;
    mrq.data    = &data;

    cmd.opcode  = CMD8_SEND_EXT_CSD;
    cmd.arg     = 0;
    cmd.flags   = MMC_RSP_R1|MMC_CMD_ADTC;
    cmd.retries = 0;

    data.blocks = 1;
    data.flags  |= MMC_DATA_READ;
    data.sg     = &sg;
    data.sg_len = 1;
    data.blksz = 512;

    sg_init_one(&sg, buf, 512);

    mmc_wait_for_req(host, &mrq);

    if (cmd.error || data.error) {

        printk(KERN_WARNING "read EXT_CSD fail\n");

        /* if support ext-csd, the ext-csd should be readed.*/
        mmc_card_set_dead(card);

        return -1;

    }

    return 0;

}

static int mmc_decode_scr(struct mmc_card *card)
{
    struct sd_scr *scr = &card->scr;
    unsigned int scr_struct;

    u32 tmp[4];
    tmp[3]=card->raw_scr[0];
    tmp[2]=card->raw_scr[1];

#ifdef SHOW_SCR
    u8 *pscr;

    pscr = &card->raw_scr;
    printk(KERN_INFO "SCR data:\n");
    printk(KERN_INFO "{000 ~ 007} 0x%02x 0x%02x 0x%02x 0x%02x "
                       "0x%02x 0x%02x 0x%02x 0x%02x\n",
            pscr[0],pscr[1],pscr[2],pscr[3],
            pscr[4],pscr[5],pscr[6],pscr[7]);

#endif

    scr_struct = UNSTUFF_BITS(tmp,60,4);
    if (scr_struct != 0) {
        printk(KERN_ERR "unrecognised SCR structure version %d\n",
            scr_struct);
        return -EINVAL;
    }

    scr->sda_vsn    = UNSTUFF_BITS(tmp,56,4);   //SD_SPEC>=1, support cmd6
    scr->bus_widths = UNSTUFF_BITS(tmp,48,4);

    mmcinfo("scr->sda_vsn:%u; scr->bus_widths:%u\n",scr->sda_vsn,scr->bus_widths);
    return 0;
}


int sd_app_send_scr(struct mmc_card *card, u32 *scr)
{
    struct mmc_request mrq;
    struct mmc_command cmd;
    struct mmc_data data;
    struct scatterlist sg;
    unsigned char *buf;

    mmcinfo("\n");
    buf = kmalloc(NORMAL_READ_BUF_SIZE, GFP_KERNEL);

    if(!buf)
        goto err_out;

    memset(buf, 0, NORMAL_READ_BUF_SIZE);

    BUG_ON(!card);
    BUG_ON(!card->host);
    BUG_ON(!scr);

    /* NOTE: caller guarantees scr is heap-allocated */

    if(sd_app_cmd(card->host, card))
        goto err_cmd;

    memset(&mrq, 0, sizeof(struct mmc_request));
    memset(&cmd, 0, sizeof(struct mmc_command));
    memset(&data, 0, sizeof(struct mmc_data));

    cmd.retries=3;
    mrq.cmd     = &cmd;
    mrq.data    = &data;
    cmd.opcode  = ACMD51_SEND_SCR;
    cmd.arg     = 0;
    cmd.flags   = MMC_RSP_R1 | MMC_CMD_ADTC;
    cmd.retries = 0;

    data.blksz = 8;
    data.blocks = 1;
    data.flags |= MMC_DATA_READ;
    data.sg = &sg;
    data.sg_len = 1;

    sg_init_one(&sg, buf, NORMAL_READ_BUF_SIZE);

    mmc_wait_for_req(card->host, &mrq);

    if (cmd.error || data.error)
        goto err_cmd;


    scr[0] = be32_to_cpu(*(unsigned int *)(buf+4));
    scr[1] = be32_to_cpu(*(unsigned int *)(buf+0));

    if(mmc_decode_scr(card)){
        printk(KERN_WARNING "decode SCR fail\n");
        goto err_cmd;

    }
    kfree(buf);
    return 0;


err_cmd:
    kfree(buf);

err_out:
    return -1;

}

/**
 *  __mmc_claim_host - exclusively claim a host
 *  @host: mmc host to claim
 *  @card: mmc card to claim host for
 *
 *  Claim a host for a set of operations.  If a valid card
 *  is passed and this wasn't the last card selected, select
 *  the card before returning.
 *
 *  Note: you should use mmc_card_claim_host or mmc_claim_host.
 */
/* for MMC CSD &EXT_CSD */
#define EXT_CSD_CMD_SET_NORMAL      (1<<0)
#define EXT_CSD_CMD_SET_SECURE      (1<<1)
#define EXT_CSD_CMD_SET_CPSECURE    (1<<2)

#define EXT_CSD_BUS_WIDTH       183 /* R/W */
#define EXT_CSD_HS_TIMING       185 /* R/W */
#define EXT_CSD_CARD_TYPE       196 /* RO */
#define EXT_CSD_REV             192 /* RO */
#define EXT_CSD_SEC_CNT         212 /* RO, 4 bytes */
#define EXT_CSD_S_A_TIMEOUT     217
/* for MMC CSD &EXT_CSD */

/*
 *  if mmc spec >=4,the card support EXT_CSD
 *  The case card may be to support High-speed & 4-bits mode
 *  It's not error, if transfer to High_Speed & 4-bits mode fail.
 *  only return error when read SCR fail.
 */
static int mmc_set_card(struct mmc_host *host)
{
    struct mmc_card *card=host->card;
    int err;
    unsigned char *ext_csd=NULL;

    mmcinfo("MMC spec %u\n",card->csd.mmca_vsn);

    if(card->csd.mmca_vsn>=4){

        /* read EXT_CSD */
        ext_csd = kmalloc(512, GFP_KERNEL);

        if(ext_csd==NULL)
            goto ram_out;

        if(mmc_read_ext_csd(host,ext_csd))
            goto err_out;

#ifdef SHOW_EXT_CSD
        {
            u8 i;
            for(i=0;i<64;i++)
            {
                if(i==0)
                    printk(KERN_INFO "ext_csd:\n");
                printk(KERN_INFO "{%03u ~ %03u} 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
                i*8,i*8+7,
                ext_csd[i*8+0],ext_csd[i*8+1],ext_csd[i*8+2],ext_csd[i*8+3],
                ext_csd[i*8+4],ext_csd[i*8+5],ext_csd[i*8+6],ext_csd[i*8+7]);
            }
        }
#endif
        card->ext_csd.rev = ext_csd[EXT_CSD_REV];
        mmcinfo("card->ext_csd.rev:0x%x\n",card->ext_csd.rev);
        if (card->ext_csd.rev > 3) {
            printk(KERN_ERR "unrecognised EXT_CSD structure "
                "version %d\n",card->ext_csd.rev);
            //goto err_out;
            card->ext_csd.hs_max_dtr = 26000000;    //High speed @ 52MHz
            goto skip1;
        }

        if (card->ext_csd.rev >= 2) {
            card->ext_csd.sectors =
            ext_csd[EXT_CSD_SEC_CNT + 0] << 0 |
            ext_csd[EXT_CSD_SEC_CNT + 1] << 8 |
            ext_csd[EXT_CSD_SEC_CNT + 2] << 16 |
            ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
            mmcinfo("card->ext_csd.sectors:0x%x\n",card->ext_csd.sectors);
            if (card->ext_csd.sectors){
                /* mmc_card_set_blockaddr */
                mmc_card_set_blockaddr(card);

            }
        }

        switch (ext_csd[EXT_CSD_CARD_TYPE]) {
        case EXT_CSD_CARD_TYPE_52 | EXT_CSD_CARD_TYPE_26:
            card->ext_csd.hs_max_dtr = 52000000;    //High speed @ 52MHz
            break;
        case EXT_CSD_CARD_TYPE_26:
            card->ext_csd.hs_max_dtr = 26000000;    //High speed @ 26MHz
            break;
        default:
            /* MMC v4 spec says this cannot happen */
            printk(KERN_WARNING "card is mmc v4 but doesn't "
                "support any high-speed modes.\n");
            goto out;
        }

        if (card->ext_csd.rev >= 3) {   //have not in spec 4.2, is this what?
            u8 sa_shift = ext_csd[EXT_CSD_S_A_TIMEOUT];

            /* Sleep / awake timeout in 100ns units */
            if (sa_shift > 0 && sa_shift <= 0x17)
                card->ext_csd.sa_timeout =
                        1 << ext_csd[EXT_CSD_S_A_TIMEOUT];
        }

        if(card->ext_csd.hs_max_dtr == 52000000){
            /* change to high speed */
            err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
                             EXT_CSD_HS_TIMING, 1);
            //if (err && err != -EBADMSG)
            if (err)
                goto out;

            mmc_card_set_highspeed(card);

            host->ios.timing = MMC_TIMING_MMC_HS;
            host->ops->set_ios(host, &host->ios);   //rtksd_set_ios
            mdelay(10);

        }

skip1:
        /* if necessary, run bus_test */
        //host->ops->fake_proc(host);

        /* change to 4 bits mode */
        err = mmc_switch(card, EXT_CSD_CMD_SET_NORMAL,
                         EXT_CSD_BUS_WIDTH, 1);

        //if (err && err != -EBADMSG)
        if (err)
           goto out;

        host->ios.bus_width = MMC_BUS_WIDTH_4;
        host->ops->set_ios(host, &host->ios);   //rtksd_set_ios
        mdelay(10);

    }

out:
    kfree(ext_csd);
    return 0;

err_out:
    kfree(ext_csd);
ram_out:
    return -1;
}

/*
 *  It's not error, if transfer to High_Speed & 4-bits mode fail.
 *  only return error when read SCR fail.
 */
static int sd_set_card(struct mmc_host *host)
{
    mmcinfo("\n");
    struct mmc_card *card=host->card;
    struct mmc_command cmd;

    if(sd_app_send_scr(card, card->raw_scr)==0){

        if(card->scr.sda_vsn>=1){
            /* support CMD6 (switch function cmd) */
            sd_read_switch(card);
            mdelay(10);
        }

        if(card->scr.bus_widths & SD_SCR_BUS_WIDTH_4){
            /* transfer to 4-BITs mode */
            if(sd_app_cmd(host,card)==0){

                memset(&cmd, 0, sizeof(struct mmc_command));
                cmd.opcode = ACMD6_SET_BUS_WIDTH;
                cmd.arg = 0x02;
                cmd.flags = MMC_RSP_R1;
                mmc_wait_for_cmd(host, &cmd, 0);
                if(cmd.error==0){
                    /* 4-BITs mode success */
                    host->ios.bus_width = MMC_BUS_WIDTH_4;
                    host->ops->set_ios(host, &host->ios);   //rtksd_set_ios
                    mdelay(10);
                }else
                    printk(KERN_WARNING "problem in transfer 4-bit\n");
            }else{
                printk(KERN_WARNING "problem in transfer 4-bit\n");
            }
        }
    }else{
        goto err_out;
    }

    return 0;
err_out:
    return -1;
}

static int _mmc_select_card(struct mmc_host *host, struct mmc_card *card)
{
    struct mmc_command cmd;

    BUG_ON(!host);

    memset(&cmd, 0, sizeof(struct mmc_command));

    cmd.opcode = CMD7_SELECT_CARD;

    if (card) {
        cmd.arg = card->rca << 16;
        cmd.flags = MMC_RSP_R1 | MMC_CMD_AC;
    } else {
        cmd.arg = 0;
        cmd.flags = MMC_RSP_NONE | MMC_CMD_AC;

        if (host->card_selected)
            host->card_selected = NULL;

    }

    mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
    return cmd.error;
}

int mmc_select_card(struct mmc_card *card)
{
    BUG_ON(!card);

    return _mmc_select_card(card->host, card);
}

int mmc_deselect_cards(struct mmc_host *host)
{
    return _mmc_select_card(host, NULL);
}

int __mmc_claim_host(struct mmc_host *host, struct mmc_card *card)
{

    DECLARE_WAITQUEUE(wait, current);
    unsigned long flags;
    int err = 0;

    mmcinfo("\n");

    add_wait_queue(&host->wq, &wait);
    spin_lock_irqsave(&host->lock, flags);
    while (1) {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if (host->card_busy == NULL){
            break;
        }
        spin_unlock_irqrestore(&host->lock, flags);
        schedule();
        spin_lock_irqsave(&host->lock, flags);
    }
    set_current_state(TASK_RUNNING);
    host->card_busy = card;
    spin_unlock_irqrestore(&host->lock, flags);
    remove_wait_queue(&host->wq, &wait);

    return err;
}

EXPORT_SYMBOL(__mmc_claim_host);


/**
 *  mmc_release_host - release a host
 *  @host: mmc host to release
 *
 *  Release a MMC host, allowing others to claim the host
 *  for their operations.
 */
void mmc_release_host(struct mmc_host *host)
{

    unsigned long flags;
    mmcinfo("\n");
        BUG_ON(host->card_busy == NULL);

    spin_lock_irqsave(&host->lock, flags);
    host->card_busy = NULL;
    spin_unlock_irqrestore(&host->lock, flags);

    wake_up(&host->wq);
}

EXPORT_SYMBOL(mmc_release_host);

/*
 * Ensure that no card is selected.
 */
/*
static void mmc_deselect_cards(struct mmc_host *host)
{

    struct mmc_command cmd;

    mmcinfo("\n");


    if (host->card_selected) {
        host->card_selected = NULL;
        memset(&cmd, 0, sizeof(struct mmc_command));
        cmd.opcode = CMD7_DESELECT_CARD;
        cmd.arg = 0;
        cmd.flags = MMC_RSP_NONE;

        mmc_wait_for_cmd(host, &cmd, 0);
    }
}
*/

static inline void mmc_delay(unsigned int ms)
{
    mmcinfo("\n");
    if (ms < HZ / 1000) {
        mdelay(ms);
    } else {
        msleep_interruptible (ms);
    }
}

/*
 * Mask off any voltages we don't support and select
 * the lowest voltage
 */
static u32 mmc_select_voltage(struct mmc_host *host, u32 ocr)
{
    int bit;
    mmcinfo("ocr=0x%x\n",ocr);

    ocr &= host->ocr_avail;

    bit = ffs(ocr);
    if (bit) {
        bit -= 1;
        ocr = 3 << bit;
        host->ios.vdd = bit;
        host->ops->set_ios(host, &host->ios);   //rtksd_set_ios
    } else {
        ocr = 0;
    }

    return ocr;
}


/*
 * Given the decoded CSD structure, decode the raw CID to our CID structure.
 */
static void mmc_decode_cid(struct mmc_card *card)
{
    u32 *resp = card->raw_cid;
    u8 *pcid = (u8 *)card->raw_cid;
    u8 *tpcid;
    u32 CIDTMP[4];
    u8 crc;
    memset(&card->cid, 0, sizeof(struct mmc_cid));

    CIDTMP[0]=be32_to_cpu(resp[0]);
    CIDTMP[1]=be32_to_cpu(resp[1]);
    CIDTMP[2]=be32_to_cpu(resp[2]);
    CIDTMP[3]=be32_to_cpu(resp[3]);

    tpcid=(u8 *)CIDTMP;

#ifdef CONFIG_CRC7
    crc=crc7(0, (u8 *)tpcid,15);
    printk("crc7=%x\n",crc);
    tpcid[15]=pcid[12]=(crc<<1)|0x01;
#else
    printk("No crc7 support\n");
    tpcid[15]=pcid[12]=0xff;
#endif

#ifdef SHOW_CID
    int i;

    /*for(i=0;i<2;i++)
    {
        if(i==0)
            printk("CID data:\n");
        printk("{%03u ~ %03u} 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
            15-(i*8),15-(i*8+7),
            pcid[i*8+3],pcid[i*8+2],pcid[i*8+1],pcid[i*8+0],
            pcid[i*8+7],pcid[i*8+6],pcid[i*8+5],pcid[i*8+4]);
    }*/

    for(i=0;i<2;i++)
    {
        if(i==0)
            printk("CID data:\n");
        printk("{%03u ~ %03u} 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
            15-(i*8),15-(i*8+7),
            tpcid[i*8+0],tpcid[i*8+1],tpcid[i*8+2],tpcid[i*8+3],
            tpcid[i*8+4],tpcid[i*8+5],tpcid[i*8+6],tpcid[i*8+7]);
    }
#endif

    //CMYu, 2009.06.17_1
    if( card->card_type & (CR_SD|CR_SDHC)) {
        card->cid.manfid        = UNSTUFF_BITS(resp, 120, 8);
        card->cid.oemid         = UNSTUFF_BITS(resp, 104, 16);
        card->cid.prod_name[0]  = UNSTUFF_BITS(resp, 96, 8);
        card->cid.prod_name[1]  = UNSTUFF_BITS(resp, 88, 8);
        card->cid.prod_name[2]  = UNSTUFF_BITS(resp, 80, 8);
        card->cid.prod_name[3]  = UNSTUFF_BITS(resp, 72, 8);
        card->cid.prod_name[4]  = UNSTUFF_BITS(resp, 64, 8);
        card->cid.hwrev         = UNSTUFF_BITS(resp, 60, 4);
        card->cid.fwrev         = UNSTUFF_BITS(resp, 56, 4);
        card->cid.serial        = UNSTUFF_BITS(resp, 24, 32);
        card->cid.year          = UNSTUFF_BITS(resp, 12, 8);
        card->cid.month         = UNSTUFF_BITS(resp, 8, 4);

        card->cid.year += 2000; /* SD cards year offset */
        return;
    }

    /*
     * The selection of the format here is guesswork based upon
     * information people have sent to date.
     */
    switch (card->csd.mmca_vsn) {
    case 0: /* MMC v1.? */
    case 1: /* MMC v1.4 */
        card->cid.manfid        = UNSTUFF_BITS(resp, 104, 24);
        card->cid.prod_name[0]  = UNSTUFF_BITS(resp, 96, 8);
        card->cid.prod_name[1]  = UNSTUFF_BITS(resp, 88, 8);
        card->cid.prod_name[2]  = UNSTUFF_BITS(resp, 80, 8);
        card->cid.prod_name[3]  = UNSTUFF_BITS(resp, 72, 8);
        card->cid.prod_name[4]  = UNSTUFF_BITS(resp, 64, 8);
        card->cid.prod_name[5]  = UNSTUFF_BITS(resp, 56, 8);
        card->cid.prod_name[6]  = UNSTUFF_BITS(resp, 48, 8);
        card->cid.hwrev         = UNSTUFF_BITS(resp, 44, 4);
        card->cid.fwrev         = UNSTUFF_BITS(resp, 40, 4);
        card->cid.serial        = UNSTUFF_BITS(resp, 16, 24);
        card->cid.month         = UNSTUFF_BITS(resp, 12, 4);
        card->cid.year          = UNSTUFF_BITS(resp, 8, 4) + 1997;
        break;

    case 2: /* MMC v2.x ? */
    case 3: /* MMC v3.x ? */
        card->cid.manfid        = UNSTUFF_BITS(resp, 120, 8);
        card->cid.oemid         = UNSTUFF_BITS(resp, 104,16);
        card->cid.prod_name[0]  = UNSTUFF_BITS(resp, 96, 8);
        card->cid.prod_name[1]  = UNSTUFF_BITS(resp, 88, 8);
        card->cid.prod_name[2]  = UNSTUFF_BITS(resp, 80, 8);
        card->cid.prod_name[3]  = UNSTUFF_BITS(resp, 72, 8);
        card->cid.prod_name[4]  = UNSTUFF_BITS(resp, 64, 8);
        card->cid.prod_name[5]  = UNSTUFF_BITS(resp, 56, 8);
        card->cid.serial        = UNSTUFF_BITS(resp, 16, 32);
        card->cid.month         = UNSTUFF_BITS(resp, 12, 4);
        card->cid.year          = UNSTUFF_BITS(resp,  8, 4) + 1997;
        break;

    case 4: /* CMYu, 20090703, MMC v4.x ? */
        card->cid.manfid        = UNSTUFF_BITS(resp, 120, 8);
        card->cid.oemid         = UNSTUFF_BITS(resp, 104, 16);
        card->cid.prod_name[0]  = UNSTUFF_BITS(resp, 96, 8);
        card->cid.prod_name[1]  = UNSTUFF_BITS(resp, 88, 8);
        card->cid.prod_name[2]  = UNSTUFF_BITS(resp, 80, 8);
        card->cid.prod_name[3]  = UNSTUFF_BITS(resp, 72, 8);
        card->cid.prod_name[4]  = UNSTUFF_BITS(resp, 64, 8);
        card->cid.prod_name[5]  = UNSTUFF_BITS(resp, 56, 8);
        card->cid.prv           = UNSTUFF_BITS(resp, 48, 8);
        card->cid.serial        = UNSTUFF_BITS(resp, 16,32);
        card->cid.month         = UNSTUFF_BITS(resp, 12, 4);
        card->cid.year          = UNSTUFF_BITS(resp,  8, 4) + 1997;
        break;

    default:
        printk("%s: card has unknown MMCA version %d\n",
            card->host->host_name, card->csd.mmca_vsn);
        mmc_card_set_bad(card);
        break;
    }
}

/*
 * Given a 128-bit response, decode to our card CSD structure.
 */
static void mmc_decode_csd(struct mmc_card *card)
{

    struct mmc_csd *csd = &card->csd;
    unsigned int e, m, csd_struct;
    u32 *resp = card->raw_csd;
    int card_type = card->card_type;
    mmcinfo("\n");
    /*
     * We only understand CSD structure v1.1 and v2.
     * v2 has extra information in bits 15, 11 and 10.
     */
    csd_struct = UNSTUFF_BITS(resp, 126, 2);    //CSD version
    mmcinfo("CSD_STRUCT:%d\n",csd_struct);

    if ( ((card_type & (CR_SD|CR_SDHC)) && csd_struct >= 2)
    || ( (card_type==CR_MMC) && csd_struct > 3) ){
        printk(KERN_WARNING "%s: unrecognised CSD structure version %d\n",
            card->host->host_name, csd_struct);
        mmc_card_set_bad(card);
        return;
    }

    csd->ver = csd_struct;

    if ( card_type & CR_MMC){
        csd->mmca_vsn = UNSTUFF_BITS(resp, 122, 4);
        mmcinfo("SPEC_VERS:%d\n",csd->mmca_vsn);
    }

    m = UNSTUFF_BITS(resp, 99, 4);  //timevalue of TRAN_SPEED
    e = UNSTUFF_BITS(resp, 96, 3);  //Transfer rate unit of TRAN_SPEED
    csd->max_dtr      = tran_exp[e] * tran_mant[m];
    csd->cmdclass     = UNSTUFF_BITS(resp, 84, 12);

    if ( (card_type & (CR_SD|CR_SDHC)) && csd_struct==1){   //SD2.0, High Capacity
        mmcinfo("csd_struct:%u\n",csd_struct);

        mmc_card_set_blockaddr(card);

        csd->tacc_ns     = 0;
        csd->tacc_clks   = 0;
        m = UNSTUFF_BITS(resp, 48, 22);
        csd->capacity = (1 + m) * 1024;
        mmcinfo("C_SIZE:%u\n",m);
        mmcinfo("READ_BL_LEN:%u\n",UNSTUFF_BITS(resp, 80, 4));
        csd->read_blkbits   = 9;
        csd->read_partial   = 0;
        csd->write_misalign = 0;
        csd->read_misalign  = 0;
        csd->r2w_factor     = 4;
        csd->write_blkbits  = 9;
        csd->write_partial  = 0;
    }else{                              //SD1.01 - 1.10; SD 2.00 Stadard Capcity; MMC
        mmcinfo("csd_struct:%u\n",csd_struct);
        m = UNSTUFF_BITS(resp, 115, 4); //timevalue of TAAC
        e = UNSTUFF_BITS(resp, 112, 3); //timeout of TAAC
        csd->tacc_ns     = (tacc_exp[e] * tacc_mant[m] + 9) / 10;
        csd->tacc_clks   = UNSTUFF_BITS(resp, 104, 8) * 100;

        e = UNSTUFF_BITS(resp, 47, 3);
        m = UNSTUFF_BITS(resp, 62, 12);
        csd->capacity = (1 + m) << (e + 2);

        csd->read_blkbits   = UNSTUFF_BITS(resp, 80, 4);
        csd->read_partial   = UNSTUFF_BITS(resp, 79, 1);
        csd->write_misalign = UNSTUFF_BITS(resp, 78, 1);
        csd->read_misalign  = UNSTUFF_BITS(resp, 77, 1);
        csd->r2w_factor     = UNSTUFF_BITS(resp, 26, 3);
        csd->write_blkbits  = UNSTUFF_BITS(resp, 22, 4);
        csd->write_partial  = UNSTUFF_BITS(resp, 21, 1);
    }

    mmcinfo("read_blkbits:%u\n"     ,csd->read_blkbits);
    mmcinfo("read_partial:0x%x\n"   ,csd->read_partial);
    mmcinfo("capacity:0x%x\n"       ,csd->capacity);

}

/*
 * Allocate a new MMC card, and assign a unique RCA.
 */
void mmc_init_card_sysfs(struct mmc_card *card, struct mmc_host *host);
static struct mmc_card * mmc_alloc_card(struct mmc_host *host)
{
    struct mmc_card *card;

    mmcinfo("\n");
    card = kmalloc(sizeof(struct mmc_card), GFP_KERNEL);

    if (!card){
        return ERR_PTR(-ENOMEM);
    }
    mmc_init_card_sysfs(card, host);
    return card;
}

/*
 * Tell attached cards to go to IDLE state
 */
static void mmc_idle_cards(struct mmc_host *host)
{
    struct mmc_command cmd;
    mmcinfo("\n");

    memset(&cmd, 0, sizeof(struct mmc_command));
    cmd.opcode = CMD0_GO_IDLE_STATE;
    cmd.arg = 0;
    cmd.flags = MMC_RSP_NONE;

    mmc_wait_for_cmd(host, &cmd, 0);

    mmc_delay(1);
}

 /*
 * allocate a unique RCA
 */
static unsigned int mmc_alloc_rca(struct mmc_host *host, unsigned int *frca) {
    unsigned int rca = *frca;
    struct mmc_card *card=host->card;
    mmcinfo("rca:0x%x\n",rca);

    if (card->rca == rca) {
        rca++;
    }

    return rca;
}

/*
 * Apply power to the MMC stack.  This is a two-stage process.
 * First, we enable power to the card without the clock running.
 * We then wait a bit for the power to stabilise.  Finally,
 * enable the bus drivers and clock to the card.
 *
 * We must _NOT_ enable the clock prior to power stablising.
 *
 * If a host does all the power sequencing itself, ignore the
 * initial MMC_POWER_UP stage.
 */
static void mmc_power_up(struct mmc_host *host)
{

    int bit = fls(host->ocr_avail) - 1;
    mmcinfo("\n");
    host->ios.vdd = bit;
    host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;

    host->ios.clock = host->f_min;
    host->ios.power_mode = MMC_POWER_ON;
    host->ops->set_ios(host, &host->ios);   //rtksd_set_ios
    if(host->card_type_pre==CR_SD){
        host->ios.power_mode = MMC_POWER_UP;
    }

    mmc_delay(30);
}

static void mmc_power_off(struct mmc_host *host)
{
    mmcinfo("\n");
    host->ios.clock = 0;
    host->ios.vdd = 0;
    host->ios.bus_mode = MMC_BUSMODE_OPENDRAIN;
    host->ios.power_mode = MMC_POWER_OFF;

    host->ios.bus_width= MMC_BUS_WIDTH_1;
    host->ios.timing =  MMC_TIMING_LEGACY;

    if(host->ops)
        host->ops->set_ios(host, &host->ios);   //rtksd_set_ios
}

/*
 * u32 ocr --- use for set host cmd argument for VDD window only
 * u32 *rocr - use for either set host cmd argument for VDD window
 *             or get back card's response about OCR.
 */
static int sd_send_op_cond( struct mmc_host *host, u32 ocr, u32 *rocr,
                            int init_opcode, int *is_sd)
{
    struct mmc_command cmd;
    int err = 0;
    unsigned long start;

    mmcinfo("\n");

    start=jiffies+2*HZ;

    while (time_before(jiffies, start)){

        if(host->ins_event==EVENT_REMOV){
            err=MMC_ERR_RMOVE;
            printk(KERN_WARNING "%s(%d)card remove \n",__func__,__LINE__);
            break;
        }

        if(init_opcode == ACMD41_SD_APP_OP_COND) {
            if (sd_app_cmd(host, NULL))
                continue;
        }
        memset(&cmd, 0, sizeof(struct mmc_command));
        cmd.opcode = init_opcode;
        if ( rocr ){
            cmd.arg = *rocr;

        }else{
            cmd.arg = ocr;

        }

        if(cmd.opcode == CMD1_SEND_OP_COND){
            cmd.arg |= SD_HCS;
        }

        cmd.flags = MMC_RSP_R3;
        mmc_wait_for_cmd(host, &cmd, 0);

        if (cmd.error == MMC_ERR_NONE){
            if (cmd.resp[0] & MMC_CARD_BUSY){

                if(rocr){   //get card's OCR
                    *rocr = cmd.resp[0];
                }

                if(is_sd) {
                    if(init_opcode == ACMD41_SD_APP_OP_COND){
                        if (cmd.resp[0] & SD_CCS)
                            *is_sd = CR_SDHC;   //SDHC
                        else
                            *is_sd = CR_SD;     //SD
                    }else{
                        *is_sd = CR_MMC;        //MMC
                    }
                }
                err=MMC_ERR_NONE;
                break;
            }
            /* wait for NCC */
            err=MMC_ERR_TIMEOUT;
            mdelay(1);  //wait for NCC

        }else{  //card did not recognize cmd
            err = MMC_ERR_FAILED;
            break;
        }
    }

    return err;
}

/*
 * Create a mmc_card entry for each discovered card, assigning
 * it an RCA, and save the raw CID for decoding later.
 */
static void mmc_discover_cards(struct mmc_host *host, int is_sd)
{

    struct mmc_card *card;
    unsigned int first_rca = 1;
    unsigned int err;

    mmcinfo("\n");

    while (1) {
        struct mmc_command cmd;
        memset(&cmd, 0, sizeof(struct mmc_command));
        cmd.opcode = CMD2_ALL_SEND_CID;
        cmd.arg = 0;
        cmd.flags = MMC_RSP_R2;

        mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);

        if (cmd.error != MMC_ERR_NONE) {
            printk(KERN_ERR "%s: error requesting CID: %d\n",
                host->host_name, cmd.error);
            break;
        }

        card = mmc_alloc_card(host);

        if (IS_ERR(card)) {
            err = PTR_ERR(card);
            break;
        }
        host->card=card;

        memcpy(card->raw_cid, cmd.resp, sizeof(card->raw_cid));
        first_rca=card->rca = 1;//rca;

        memset(&cmd, 0, sizeof(struct mmc_command));
        if(is_sd & (CR_SD|CR_SDHC)){  //SD;SDHC
            cmd.opcode = CMD3_SEND_RELATIVE_ADDR;
            cmd.arg = 0; // ignored
            cmd.flags = MMC_RSP_R1;
        }else if(is_sd & CR_MMC){      //MMC
            card->rca = mmc_alloc_rca(host, &first_rca);
            cmd.opcode = CMD3_SET_RELATIVE_ADDR;
            cmd.arg = card->rca << RCA_SHIFTER;
            cmd.flags = MMC_RSP_R1;
        }else{
            /* non recognized card type */
            mmc_card_set_dead(card);
            break;
        }

        mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
        if (cmd.error != MMC_ERR_NONE)
                mmc_card_set_dead(card);

        if(is_sd & (CR_SD|CR_SDHC)){
            card->rca = cmd.resp[0]>>RCA_SHIFTER;
            first_rca = card->rca;
        }
        break;
    }
}

static void mmc_read_csds(struct mmc_host *host, int is_sd)
{
    struct mmc_card *card=host->card;
    mmcinfo("\n");
    struct mmc_command cmd;
    //int err;

    if(mmc_card_present(card) && mmc_card_dead(card) )
        return;

    //CMYu, 20090621
    card->card_type = is_sd;
    memset(&cmd, 0, sizeof(struct mmc_command));
    cmd.opcode = CMD9_SEND_CSD;
    cmd.arg = card->rca << RCA_SHIFTER;
    cmd.flags = MMC_RSP_R2;

    mmc_wait_for_cmd(host, &cmd, CMD_RETRIES);
    if (cmd.error != MMC_ERR_NONE) {
        mmc_card_set_dead(card);
        return;
    }

    memcpy(card->raw_csd, cmd.resp, sizeof(card->raw_csd));

#ifdef SHOW_CSD
    printk("CSD:\n");
    printk("0x%08x 0x%08x 0x%08x 0x%08x\n",
    cmd.resp[0],cmd.resp[1],cmd.resp[2],cmd.resp[3]);
#endif

    mmc_decode_csd(card);
    mmc_decode_cid(card);
}


/*
static unsigned int mmc_calculate_clock(struct mmc_host *host)
{

    struct mmc_card *card=host->card;
    unsigned int max_dtr = host->f_max;
    mmcinfo("\n");
    //list_for_each_entry(card, &host->cards, node)
        if (!mmc_card_dead(card) && max_dtr > card->csd.max_dtr)
            max_dtr = card->csd.max_dtr;

    DBG("MMC: selected %d.%03dMHz transfer rate\n",
        max_dtr / 1000000, (max_dtr / 1000) % 1000);

    return max_dtr;
}
*/

//CMYu, 20090618, add SD CMD8 to check SD 2.0
static int sd_send_if_cond(struct mmc_host *host, u32 cmd_idx, u32 arg, u32 *voltage)
{
    struct mmc_command cmd;
    mmcinfo("\n");

    memset(&cmd, 0, sizeof(struct mmc_command));
    cmd.opcode = cmd_idx;
    cmd.arg = arg;
    cmd.flags = 0x05;     //should be R7
    mmc_wait_for_cmd(host, &cmd, 0);

    if( cmd.error != MMC_ERR_NONE)
        return -1;

    /*
     * (cmd.resp[0]&0xff) == 0xAA) is check test pattern;
     * (((cmd.resp[0]>>8)&0x0f) == 0x01) is check voltage accepted Table 4-35
     */
    if( ((cmd.resp[0]&0xff) == 0xAA) && (((cmd.resp[0]>>8)&0x0f) == 0x01) ){
        *voltage = *voltage | SD_HCS;
        mmc_delay(1);
        return MMC_ERR_NONE;
    }else{
        printk(KERN_WARNING"CMD8 fails, NOT SD 2.0 card\n");
        return MMC_ERR_FAILED;
    }
}

static int mmc_setup(struct mmc_host *host)
{
    //int i;
    int err, is_sd = 0;

    mmcinfo("mmc_host addr:0x%p\n",host);

    u32 ocr = RTL_HOST_VDD;   //rtl supported OCR VDD voltage window

    mmc_power_up(host);

    mmc_idle_cards(host);       //cmd0

    err = sd_send_op_cond(host, 0, &ocr, CMD1_SEND_OP_COND, &is_sd);    //chk MMC first

    if (err != MMC_ERR_NONE){   //SD card did not recognize CMD1.
        mmc_idle_cards(host);

        //CMYu, 20090618, add SD CMD8 to check SD 2.0
        err = sd_send_if_cond(host, CMD8_SEND_IF_COND, 0x000001AA, &ocr);

        if (err != MMC_ERR_NONE){   //SD under Physical Spec .2.0
            printk(KERN_WARNING "sd_send_if_cond() fails, "
                                "go in idle state\n");
            mmc_idle_cards(host);
        }

        is_sd = 0;

        err = sd_send_op_cond(host, 0, &ocr, ACMD41_SD_APP_OP_COND, &is_sd);

        if (err != MMC_ERR_NONE){
            printk(KERN_WARNING "sd_send_op_cond() fails\n");
            goto err_out;
        }
    }

    /*
     * Since we're changing the OCR value, we seem to
     * need to tell some cards to go back to the idle
     * state.  We wait 1ms to give cards time to
     * respond.
     */
    host->ocr = mmc_select_voltage(host, ocr);
    if (host->ocr==0){
        mmc_idle_cards(host);
        goto err_out;
    }

    /*
     * Send the selected OCR multiple times... until the cards
     * all get the idea that they should be ready for CMD2.
     * (My SanDisk card seems to need this.)
     */
    mmc_discover_cards(host, is_sd);    //create link between card and host

    /*
     * Ok, now switch to push-pull mode.
     */
    if(host->card && !mmc_card_dead(host->card)){
        host->ios.bus_mode = MMC_BUSMODE_PUSHPULL;
        host->ops->set_ios(host, &host->ios);   //rtksd_set_ios
        mmc_read_csds(host, is_sd);
        if(mmc_card_dead(host->card) | mmc_card_bad(host->card) ){
            goto err_out;
        }else{

            if(mmc_select_card(host->card)){
                goto err_out;
            }else{
                /* Card selected */
                host->card_selected=host->card;
                host->ios.bus_width =0;
                if(host->card->card_type & CR_MMC){
                    if(mmc_set_card(host))
                        goto err_out;
                }else if(host->card->card_type & (CR_SDHC|CR_SD)){
                    if(sd_set_card(host))
                        goto err_out;
                }else{
                    printk(KERN_WARNING "mmc:card type wrong!\n");
                    goto err_out;
                }
                mmc_card_set_ident_rdy(host->card);
            }
        }
        return 0;
    }else{
 err_out:
        return -1;
    }
}


/**
 *  mmc_detect_change - process change of state on a MMC socket
 *  @host: host which changed state.
 *
 *  All we know is that card(s) have been inserted or removed
 *  from the socket(s).  We don't know which socket or cards.
 */
void mmc_detect_change(struct mmc_host *host)
{
    mmcinfo("\n");
    schedule_work(&host->detect);   //mmc_rescan
    schedule(); //CMYu adds it, 20090617
}

EXPORT_SYMBOL(mmc_detect_change);
/* MS_PRO flow ****************************************************** */
int msp_read_dev_id(struct mmc_card *card, struct mmc_request *mrq)
{
    struct ms_id_register id_reg;
    mmcinfo("\n");

    BUG_ON(card->host->card_busy == NULL);

    mrq->id = &(card->id);
    mrq->tpc = MSPRO_TPC_READ_REG;

    mmc_wait_for_req(card->host, mrq);
    memcpy(&id_reg, mrq->reg_data+2, sizeof(id_reg));

    card->id.match_flags = MEMSTICK_MATCH_ALL;
    card->id.type        = id_reg.type;
    card->id.category    = id_reg.category;
    card->id.class       = id_reg.class;

    if (mrq->reg_data[0] & WP_FLAG) {
        card->write_protect_flag = 1;
    } else {
        card->write_protect_flag = 0;
    }

    return 0;

}

int msp_set_rw_addr(struct mmc_card *card, struct mmc_request *mrq)
{
    mmcinfo("\n");
    BUG_ON(card->host->card_busy == NULL);

    mrq->reg_rw_addr = &(card->reg_rw_addr);

    mrq->tpc = MSPRO_TPC_SET_RW_REG_ADRS;

    mmc_wait_for_req(card->host, mrq);

    return 0;
}

static int proc_identify_media_type(struct mmc_card *card)
{
    struct mmc_request mrq;
    unsigned int i;

    mmcinfo("\n");
    memset(&mrq, 0, sizeof(struct mmc_request));

    card->reg_rw_addr.r_offset = MSPRO_REG_Status;
    card->reg_rw_addr.r_length = 6;
    card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
    card->reg_rw_addr.w_length = 1;

    for(i=0; i<3; i++){
        /* TRY_AGAIN_SETRW */
        if (msp_set_rw_addr(card, &mrq)==0){
            i=0;
            break;
        }else{
            printk(KERN_WARNING "[%s] msp_set_rw_addr() fails, try_cnt=%d\n",
                    __FUNCTION__, i);
            msleep(10);
        }
    }

    if(i==0){
        msleep(10);
        for(i=0; i<3; i++){
            /* TRY_READ_DEV_ID */
            if (msp_read_dev_id(card, &mrq)){
                printk(KERN_WARNING "[%s] msp_read_dev_id() fails, try_cnt=%d\n", __FUNCTION__, i);
                msleep(10);
            }else{
                i=0;
                break;
            }
        }
    }

    if(i<3)
        return 0;

    return -1;
}

static int proc_confirm_cpu_startup(struct mmc_card *card)
{
    int try_cnt = 0;
    struct mmc_request mrq;

    mmcinfo("\n");
TRY_AGAIN_GET_INT_CED:

    memset(&mrq, 0, sizeof(struct mmc_request));
    mrq.tpc = MSPRO_TPC_GET_INT;
    mrq.byte_count = 1;
    mrq.timeout = USUAL_TIMEOUT;
    mrq.reg_data[0] = 0;

    mmc_wait_for_req(card->host, &mrq);

    if ( mrq.error || !(mrq.reg_data[0] & REG_INT_CED) ) {
        printk(KERN_WARNING "[%s] MSPRO_TPC_GET_INT fails, try_cnt=%d\n",
               __FUNCTION__, try_cnt);
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            return -1;
        }
        msleep(100);
        goto TRY_AGAIN_GET_INT_CED;
    }
    try_cnt = 0;

    msleep(10);
    mrq.tpc = MSPRO_TPC_GET_INT;
    mrq.byte_count = 1;
    mrq.timeout = USUAL_TIMEOUT;
    mrq.reg_data[0] = 0;

    mmc_wait_for_req(card->host, &mrq);

    if (mrq.reg_data[0] & REG_INT_ERR) {
        if (mrq.reg_data[0] & REG_INT_CMDNK) {
            card->write_protect_flag = 1;
        } else {
            printk(KERN_WARNING "[%s] Media Error\n",
                   __FUNCTION__);
            return -2;
        }
    }else
        printk(KERN_INFO "[%s] CPU start is completed\n",
               __FUNCTION__);

    return 0;

}

int proc_switch_interface(struct mmc_card *card)
{

    struct mmc_request mrq;
    unsigned int i;

    mmcinfo("\n");

    if(PARELLEL_IF==0x80){
        printk(KERN_INFO "Do not support Parallel Mode!\n");
        return 0;
    }
    memset(&mrq, 0, sizeof(struct mmc_request));
    //card->reg_rw_addr.r_offset = MSPRO_REG_Status;
    //card->reg_rw_addr.r_length = 6;
    card->reg_rw_addr.r_offset = 0;
    card->reg_rw_addr.r_length = 0;
    card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
    card->reg_rw_addr.w_length = 1;

    for(i=0; i<3; i++){
        if (msp_set_rw_addr(card, &mrq)==0){
            i=0;
            break;
        }
        printk(KERN_WARNING "[%s] msp_set_rw_addr() fails, try_cnt=%d\n", __FUNCTION__, i);
        msleep(10);
    }

    if(i==0){
        msleep(10);
        memset(&mrq, 0, sizeof(struct mmc_request));
        mrq.tpc         = MSPRO_TPC_WRITE_REG;
        //mrq.option      = 0x80;
        mrq.byte_count  = 1;
        mrq.reg_data[0] = PARELLEL_IF;
        for(i=0; i<3; i++){
            /* set card to parallel mode */
            mmc_wait_for_req(card->host, &mrq);
            if (mrq.error){
                printk(KERN_WARNING "[%s] memstick_read_dev_id() fails, try_cnt=%d\n", __FUNCTION__, i);
                msleep(10);
            }else{
                i=0;
                break;
            }
        }
    }

    msleep(10);
    if(i<3){
        card->host->ios.bus_width = MMC_BUS_WIDTH_4;
        card->host->ops->set_ios(card->host, &card->host->ios);   //rtkms_set_ios

        return 0;
    }
    return -1;

}

int msp_read_sys_info(struct mmc_card *card, unsigned char *data, int CurrentAddrOff)
{

    int i = 0;
    unsigned char ClassCode, DeviceType, SubClass;
    unsigned short totalblock = 0, blocksize = 0;
    unsigned int totalsector = 0, SysInfoAddr, SysInfoSize;
    unsigned int serial_number;
    unsigned short Start_Sector = 0;
    mmcinfo("\n");

    ASSIGN_LONG(SysInfoAddr,data[CurrentAddrOff+0],
                            data[CurrentAddrOff+1],
                            data[CurrentAddrOff+2],
                            data[CurrentAddrOff+3]);

    ASSIGN_LONG(SysInfoSize,data[CurrentAddrOff+4],
                            data[CurrentAddrOff+5],
                            data[CurrentAddrOff+6],
                            data[CurrentAddrOff+7]);

    if (SysInfoSize != 96)  {
        goto ERROR;
    }
    if (SysInfoAddr < 0x1A0) {
        goto ERROR;
    }
    if ((SysInfoSize + SysInfoAddr) > 0x8000) {
        goto ERROR;
    }

    ClassCode =  data[SysInfoAddr + 0];
    ASSIGN_SHORT(blocksize, data[SysInfoAddr + 2], data[SysInfoAddr + 3]);
    ASSIGN_SHORT(totalblock,data[SysInfoAddr + 6], data[SysInfoAddr + 7]);
    ASSIGN_LONG(serial_number,  data[SysInfoAddr+20],
                                data[SysInfoAddr+21],
                                data[SysInfoAddr+22],
                                data[SysInfoAddr+23]);
    ASSIGN_SHORT(Start_Sector,  data[SysInfoAddr + 42],data[SysInfoAddr + 43]);
    SubClass    = data[SysInfoAddr + 46];
    DeviceType  = data[SysInfoAddr + 56];
    totalsector = ((unsigned int)totalblock) * blocksize;

    card->mspro_sysinfo.class           = ClassCode;
    card->mspro_sysinfo.block_size      = blocksize;
    card->mspro_sysinfo.block_count     = totalblock;
    card->mspro_sysinfo.interface_type  = data[SysInfoAddr + 51];
    card->mspro_sysinfo.start_sector    = Start_Sector;
    card->mspro_sysinfo.device_type     = DeviceType;
    card->mspro_sysinfo.serial_number   = serial_number;

    for( i=0; i<16; i++){
        card->mspro_sysinfo.mspro_id[i] = data[SysInfoAddr + 64 + i];
    }

    /* Confirm System Information */
    if (ClassCode != 0x02) {
        printk(KERN_WARNING "##### Error: NOT MSPRO\n");
        goto ERROR;
    }
    if (DeviceType != 0x00) {
        if ((DeviceType == 0x01) || (DeviceType == 0x02)
            || (DeviceType == 0x03)) {
            card->write_protect_flag = 1;
        } else {
            printk(KERN_WARNING "##### DeviceType erroe\n");
            goto ERROR;
        }
    }

    if (SubClass & 0xc0) {
        goto ERROR;
    }

    card->capacity = totalsector;
    return 0;

ERROR:
    return -1;

}

int msp_read_dev_info(struct mmc_card *card, unsigned char *data, int CurrentAddrOff)
{

    unsigned int DevInfoAddr, DevInfoSize;
    unsigned short cylinders;
    unsigned short heads;
    unsigned short sectors_per_track;
    mmcinfo("\n");
    ASSIGN_LONG(DevInfoAddr,data[CurrentAddrOff+0],
                            data[CurrentAddrOff+1],
                            data[CurrentAddrOff+2],
                            data[CurrentAddrOff+3]);
    ASSIGN_LONG(DevInfoSize,data[CurrentAddrOff+4],
                            data[CurrentAddrOff+5],
                            data[CurrentAddrOff+6],
                            data[CurrentAddrOff+7]);

    ASSIGN_SHORT(cylinders, data[DevInfoAddr + 0], data[DevInfoAddr + 1]);
    ASSIGN_SHORT(heads,     data[DevInfoAddr + 2], data[DevInfoAddr + 3]);
    ASSIGN_SHORT(sectors_per_track, data[DevInfoAddr + 8], data[DevInfoAddr + 9]);

    card->cylinders = cylinders;
    card->heads     = heads;
    card->sectors_per_track = sectors_per_track;

    return 0;

}

void msp_read_model_name(struct mmc_card *card, unsigned char *data, int CurrentAddrOff)
{

    int i;
    unsigned int ModNameAddr, ModNameSize;
    mmcinfo("Model Name\n");
    ASSIGN_LONG(ModNameAddr,data[CurrentAddrOff+0],
                            data[CurrentAddrOff+1],
                            data[CurrentAddrOff+2],
                            data[CurrentAddrOff+3]);
    ASSIGN_LONG(ModNameSize,data[CurrentAddrOff+4],
                            data[CurrentAddrOff+5],
                            data[CurrentAddrOff+6],
                            data[CurrentAddrOff+7]);

    for ( i=0; i<0x30; i++){
        printk(KERN_INFO "%c",data[ModNameAddr+i]);
    }
    printk(KERN_INFO "\n");

}

int proc_read_attributes(struct mmc_card *card)
{
    int i = 0;
    int try_cnt = 0;
    unsigned char atrb_sct_cnt=0x02;
    unsigned char temp[7] = {PARELLEL_IF, 0, atrb_sct_cnt, 0, 0, 0, 0x00};
    struct mmc_request mrq;
    unsigned char *atrb_data;
    mmcinfo("\n");

    atrb_data = kmalloc(atrb_sct_cnt * 512, GFP_KERNEL);

    if ( !atrb_data ) {
        printk(KERN_WARNING "[%d] Allocation data fails\n", __LINE__);
        return -ENOMEM;
    }

    memset(&mrq, 0, sizeof(struct mmc_request));
    memset(atrb_data, 0, atrb_sct_cnt*512);

TRY_AGAIN_SETRW:
    card->reg_rw_addr.r_offset = MSPRO_REG_Status;
    card->reg_rw_addr.r_length = 2;
    card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
    card->reg_rw_addr.w_length = 7;

    if (msp_set_rw_addr(card, &mrq)){
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk(KERN_WARNING "[%s] msp_set_rw_addr() exceeds try_cnt=%d\n",
                    __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_SETRW;
    }
    try_cnt = 0;

TRY_AGAIN_SYS_PARA:
    mrq.tpc = MSPRO_TPC_WRITE_REG;
    mrq.option = 0;
    //mrq.option = 0x80;
    mrq.byte_count = card->reg_rw_addr.w_length;
    mrq.timeout = PRO_WRITE_TIMEOUT;
    mrq.reg_data[0] = temp[0];
    mrq.reg_data[1] = temp[1];
    mrq.reg_data[2] = temp[2];
    mrq.reg_data[3] = temp[3];
    mrq.reg_data[4] = temp[4];
    mrq.reg_data[5] = temp[5];
    mrq.reg_data[6] = temp[6];

    mmc_wait_for_req(card->host, &mrq);

    if (mrq.error) {
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk(KERN_WARNING "[%s] Writing System Parameter exceeds try_cnt=%d\n", __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_SYS_PARA;
    }
    try_cnt = 0;

TRY_AGAIN_SET_READ_ATRB:
    memset(&mrq, 0, sizeof(struct mmc_request));
    mrq.reg_data[0] = MSPRO_CMD_READ_ATRB;
    mrq.tpc = MSPRO_TPC_SET_CMD;
    mrq.option |= WAIT_INT;
    //mrq.option = 0xc0;
    mrq.byte_count = 2;
    mmc_wait_for_req(card->host, &mrq);
    if (mrq.error) {
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk(KERN_WARNING "[%s] MSPRO_TPC_SET_CMD exceeds try_cnt=%d\n",
                   __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_SET_READ_ATRB;
    }

    if ( !(mrq.status & INT_BREQ) ){
        printk(KERN_WARNING "[%d] INT_BREQ errors, mrq.status=%x\n",
               __LINE__, mrq.status);
        goto ERROR;
    }

    try_cnt = 0;

TRY_AGAIN_READ_ATRB:
    memset(&mrq, 0, sizeof(struct mmc_request));
    mrq.long_data = atrb_data;
    mrq.tpc = MSPRO_TPC_READ_LONG_DATA;
    mrq.sector_cnt = atrb_sct_cnt;
    //mrq.sector_cnt = 0x08;
    mrq.timeout = PRO_READ_TIMEOUT;
    mrq.endian = BIG;
    mmc_wait_for_req(card->host, &mrq);
    if (mrq.error) {
        printk(KERN_WARNING "[%s] MSPRO_CMD_READ_ATRB failstry_cnt=%d\n",
               __FUNCTION__, try_cnt);
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk(KERN_WARNING "[%s] MSPRO_CMD_READ_ATRB exceeds try_cnt=%d\n",
                   __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_READ_ATRB;
    }
    try_cnt = 0;

    if ((atrb_data[0] != 0xa5) && (atrb_data[1] != 0xc3)) {
        printk(KERN_WARNING "Signature code is wrong !! data[0]=%#x, data[1]=%#x\n",
            atrb_data[0], atrb_data[1]);
        goto ERROR;
    }

    if ((atrb_data[4] < 1) || (atrb_data[4] > 12)) {
        printk(KERN_WARNING "Device Informantion Entry Count errors!! data[4]=%d\n", atrb_data[4]);
        goto ERROR;
    }

#ifdef SHOW_DEV_INFO_ENTRY
    printk(KERN_INFO "Device Information Entry\n");
    for (i = 0; i < atrb_data[4]; i ++) {
        int CurrentAddrOff = 16 + i * 12;
        unsigned int SysInfoAddr, SysInfoSize;
        ASSIGN_LONG(SysInfoAddr,atrb_data[CurrentAddrOff+0],
                                atrb_data[CurrentAddrOff+1],
                                atrb_data[CurrentAddrOff+2],
                                atrb_data[CurrentAddrOff+3]);
        ASSIGN_LONG(SysInfoSize,atrb_data[CurrentAddrOff+4],
                                atrb_data[CurrentAddrOff+5],
                                atrb_data[CurrentAddrOff+6],
                                atrb_data[CurrentAddrOff+7]);

        printk(KERN_INFO "ID: 0x%02x; Address:0x%08x; Size:0x%08x\n",
                atrb_data[CurrentAddrOff+8],
                SysInfoAddr,SysInfoSize);
    }
#endif

    for (i = 0; i < atrb_data[4]; i ++) {
        int CurrentAddrOff = 16 + i * 12;

        if (atrb_data[CurrentAddrOff + 8] == 0x10) { //system information

            if ( msp_read_sys_info(card, atrb_data, CurrentAddrOff) ){
                printk(KERN_WARNING "msp_read_sys_info() fails\n");
                goto ERROR;
            }
        }

        if (atrb_data[CurrentAddrOff + 8] == 0x30) { //identify device information
            if ( msp_read_dev_info(card, atrb_data, CurrentAddrOff) ){
                printk(KERN_WARNING "msp_read_dev_info() fails\n");
                goto ERROR;
            }
        }

        if (atrb_data[CurrentAddrOff + 8] == 0x15) { //system information
            msp_read_model_name(card, atrb_data, CurrentAddrOff);
        }
    }
    kfree(atrb_data);
    return 0;

ERROR:
    kfree(atrb_data);
    return -4;
}



static int mspro_startup_card(struct mmc_host *host)
{
    struct mmc_card *card=host->card;
    int rc = 0;

    mmcinfo("\n");

    if ( proc_confirm_cpu_startup(card)){
        /* confirm cpu startup fails */
        return -1;
    }

    if (proc_switch_interface(card)){
        /* process switch interface mode fails! */
        return -1;
    }
    rc = proc_read_attributes(card);
    if (rc){
        /* process_read_attributes() fails */
        return rc;
    }

    return rc;

}

static void msp_discover_cards(struct mmc_host *host)
{
    struct mmc_card *card;
    mmcinfo("\n");
    card = mmc_alloc_card(host);
    if (IS_ERR(card)) {
        printk("[%s] ms_alloc_card() fails\n", __FUNCTION__);
        return;
    }

    host->card=card;

    if(proc_identify_media_type(card)==0){

        if (card->id.type == 0x01){     //Identify MSPRO Card

            if (card->id.category == 0x00){

                if (card->id.class == 0x00){

                    card->card_type = CR_MS_PRO;   //before read MS card, we need to know MS or MS-pro, Beacuse method of reading different.

                    if ( mspro_startup_card(host) ){
                        printk("mspro_startup_card() fails\n");
                        card->card_type = CR_NON;
                    }

                }else{
                    printk("[%s] Class Register mismatch, card->id.class=%#x\n", __FUNCTION__, card->id.class);
                }
            }else{
                printk("[%s] Category Register mismatch, card->id.category=%#x\n", __FUNCTION__, card->id.category);
            }
        }
    }

    if(card->card_type & (CR_MS|CR_MS_PRO))
    {
        return;
    }else{
        mmc_card_set_dead(card);
        return;
    }
}


static int msp_setup(struct mmc_host *host)
{
    //int rc = 0;
    mmcinfo("\n");

    mmc_power_up(host);          //Pcocedure to turn on the power

    msp_discover_cards(host);

    if(host->card && !mmc_card_dead(host->card)){
        host->ios.bus_mode = MMC_BUSMODE_PUSHPULL;
        host->ops->set_ios(host, &host->ios);   //rtkms_set_ios
        return 0;
    }else{
        kfree(host->card);
        return -1;
    }

}
/* MS_PRO flow &&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& */

/**
 *  mmc_rescan - scan MMC socket to see if SD or MMC in slot
 */
static void mmc_rescan(void *data)
{
    struct mmc_host *host = data;
    struct mmc_card *card;
    int err=0;

    mmcinfo("\n");

    if(host->ins_event==EVENT_REMOV)
    {
        BUG_ON(!host);
        BUG_ON(!host->card);

        mmc_remove_card(host->card);

        mmc_power_off(host);
        mmc_claim_host(host);
        host->ops = NULL;
        mmc_release_host(host);
        host->ins_event=EVENT_NON;
        goto terminate;
    }

    mmc_claim_host(host);
    if(host->card_type_pre!=CR_MS){
        err=mmc_setup(host);
    }else{
        err=msp_setup(host);
    }

    if(err)
    {
        mmc_release_host(host);
        mmc_power_off(host);
        goto terminate;
    }
    /*
    if(mmc_setup(host))
    {
        mmc_release_host(host);
        mmc_power_off(host);
        goto terminate;
    }
    */

    /*
     * (Re-)calculate the fastest clock rate which the
     * attached cards and the host support.
     */
    //host->ios.clock = mmc_calculate_clock(host);
    //host->ops->set_ios(host, &host->ios);   //rtksd_set_ios
    mmc_release_host(host);
    card = host->card;
    /*
     * If this is a new and good card, register it.
     */
    if (!mmc_card_present(card) && !mmc_card_dead(card)) {
        if (mmc_register_card(card)){
            mmc_card_set_dead(card);
        }else{
            mmc_card_set_present(card);
        }
    }

    /*
     * If this card is dead, destroy it.
     */
    if (mmc_card_dead(card)) {
        //host->card=NULL;  MS-PRO有加,之後再來看
        mmc_remove_card(card);
        mmc_power_off(host);
    }

    /*
     * If we discover that there are no cards on the
     * bus, turn off the clock and power down.
     */
terminate:
    return;
}


/**
 *  mmc_alloc_host - initialise the per-host structure.
 *  @extra: sizeof private data structure
 *  @dev: pointer to host device model structure
 *
 *  Initialise the per-host structure.
 */
struct mmc_host *mmc_alloc_host(int extra, struct device *dev)
{

    struct mmc_host *host;
    mmcinfo("\n");
    host = kmalloc(sizeof(struct mmc_host) + extra, GFP_KERNEL);
    if (host) {
        memset(host, 0, sizeof(struct mmc_host) + extra);

        spin_lock_init(&host->lock);
        init_waitqueue_head(&host->wq);
        INIT_WORK(&host->detect, mmc_rescan, host);

        host->dev = dev;

    }else{
        printk("mmc_alloc_host() fails\n");
        return NULL;
    }

    return host;
}

EXPORT_SYMBOL(mmc_alloc_host);

/**
 *  mmc_add_host - initialise host hardware
 *  @host: mmc host
 */
int mmc_add_host(struct mmc_host *host)
{

    static unsigned int host_num;
    mmcinfo("\n");
    snprintf(host->host_name, sizeof(host->host_name),
         "mmc%d", host_num++);

    mmc_power_off(host);
    mmc_detect_change(host);

    return 0;
}

EXPORT_SYMBOL(mmc_add_host);

/**
 *  mmc_remove_host - remove host hardware
 *  @host: mmc host
 *
 *  Unregister and remove all cards associated with this host,
 *  and power down the MMC bus.
 */
void mmc_remove_host(struct mmc_host *host)
{
    mmcinfo("\n");
    //struct mmc_card *card = host->card;

    if(host->card){
        mmc_remove_card(host->card);
    }

    mmc_power_off(host);
}

EXPORT_SYMBOL(mmc_remove_host);

/**
 *  mmc_free_host - free the host structure
 *  @host: mmc host
 *
 *  Free the host once all references to it have been dropped.
 */
void mmc_free_host(struct mmc_host *host)
{
    flush_scheduled_work();
    mmcinfo("\n");
    kfree(host);
}

EXPORT_SYMBOL(mmc_free_host);

#ifdef CONFIG_PM

/**
 *  mmc_suspend_host - suspend a host
 *  @host: mmc host
 *  @state: suspend mode (PM_SUSPEND_xxx)
 */
int mmc_suspend_host(struct mmc_host *host, pm_message_t state)
{
    mmcinfo("\n");
    mmc_claim_host(host);
    mmc_deselect_cards(host);
    mmc_power_off(host);
    mmc_release_host(host);

    return 0;
}

EXPORT_SYMBOL(mmc_suspend_host);

/**
 *  mmc_resume_host - resume a previously suspended host
 *  @host: mmc host
 */
int mmc_resume_host(struct mmc_host *host)
{
    mmcinfo("\n");
    mmc_detect_change(host);

    return 0;
}

EXPORT_SYMBOL(mmc_resume_host);

#endif

MODULE_LICENSE("GPL");
