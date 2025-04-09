/*
 *  linux/drivers/ms/ms.c
 *
 *  Copyright (C) 2003-2004 Russell King, All Rights Reserved.
 *
 *  SD support (c) 2004 Ian Molton.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modification History:
 *    #000 2009-07-09 CMYu   modify from mmc.c for ms usage
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

#include <linux/ms/card.h>
#include <linux/ms/host.h>
#include <linux/ms/protocol.h>

#include <linux/rtk_cr.h>
#include "ms.h"
#include "ms_debug.h"

#define CMD_RETRIES 3

#define BEFORE_CALL 0
#define AFTER_CALL  1

int ms_register_card(struct ms_card *card);
void ms_remove_card(struct ms_card *card);

void ms_request_done(struct ms_host *host, struct ms_request *mrq)
{
    msinfo("\n");
    int err = mrq->error;

    if (err){
        printk("[%s] mrq->tpc=%#x error\n", __FUNCTION__, mrq->tpc);
    }else if (mrq->done) {
        mrq->done(mrq);
    }
}

EXPORT_SYMBOL(ms_request_done);


void ms_start_request(struct ms_host *host, struct ms_request *mrq)
{
    msinfo("\n");
    WARN_ON(host->card_busy == NULL);
    if (mrq->data) {
        mrq->data->error = 0;
        mrq->data->mrq = mrq;
    }
    host->ops->request(host, mrq);  //rtkms_request
}
EXPORT_SYMBOL(ms_start_request);


static void ms_wait_done(struct ms_request *mrq)
{
    msinfo("\n");
    complete(mrq->done_data);
}


int ms_wait_for_req(struct ms_host *host, struct ms_request *mrq)
{
    DECLARE_COMPLETION(complete);
    msinfo("\n");
    mrq->done_data = &complete;
    mrq->done = ms_wait_done;
    ms_start_request(host, mrq);
    wait_for_completion(&complete);

    return 0;
}

EXPORT_SYMBOL(ms_wait_for_req);


int __ms_claim_host(struct ms_host *host, struct ms_card *card)
{

    DECLARE_WAITQUEUE(wait, current);
    unsigned long flags;
    int err = 0;
    msinfo("\n");
    add_wait_queue(&host->wq, &wait);
    spin_lock_irqsave(&host->lock, flags);
    while (1) {
        set_current_state(TASK_UNINTERRUPTIBLE);
        if (host->card_busy == NULL)
            break;
        spin_unlock_irqrestore(&host->lock, flags);
        schedule();
        spin_lock_irqsave(&host->lock, flags);
    }
    set_current_state(TASK_RUNNING);
    host->card_busy = card;
    spin_unlock_irqrestore(&host->lock, flags);
    remove_wait_queue(&host->wq, &wait);

    if(host->ins_event==EVENT_REMOV){
        msinfo("\n");
        err = MS_ERR_RMOVE;
        goto out;
    }


    if (card != (void *)-1 && host->card_selected != card) {
        host->card_selected = card;
        msinfo("card selected!\n");
    }
out:
    msinfo("\n");
    return err;
}

EXPORT_SYMBOL(__ms_claim_host);

void ms_release_host(struct ms_host *host)
{
    unsigned long flags;
    msinfo("\n");
    BUG_ON(host->card_busy == NULL);

    spin_lock_irqsave(&host->lock, flags);
    host->card_busy = NULL;
    spin_unlock_irqrestore(&host->lock, flags);

    wake_up(&host->wq);
}

EXPORT_SYMBOL(ms_release_host);


static inline void ms_delay(unsigned int ms)
{
    if (ms < HZ / 1000) {
        //yield();  //CMYu, 20090610
        mdelay(ms);
    } else {
        msleep_interruptible (ms);
    }
}


#define UNSTUFF_BITS(resp,start,size)                   \
    ({                              \
        const int __size = size;                \
        const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1; \
        const int __off = 3 - ((start) / 32);           \
        const int __shft = (start) & 31;            \
        u32 __res;                      \
                                    \
        __res = resp[__off] >> __shft;              \
        if (__size + __shft > 32)               \
            __res |= resp[__off-1] << ((32 - __shft) % 32); \
        __res & __mask;                     \
    })


int memstick_set_rw_addr(struct ms_card *card, struct ms_request *mrq)
{
    msinfo("\n");
    BUG_ON(card->host->card_busy == NULL);

    mrq->reg_rw_addr = &(card->reg_rw_addr);

    mrq->tpc = MSPRO_TPC_SET_RW_REG_ADRS;

    ms_wait_for_req(card->host, mrq);

    return 0;
}
EXPORT_SYMBOL(memstick_set_rw_addr);


int memstick_read_dev_id(struct ms_card *card, struct ms_request *mrq)
{
    struct ms_id_register id_reg;
    msinfo("\n");

    BUG_ON(card->host->card_busy == NULL);

    mrq->id = &(card->id);
    mrq->tpc = MSPRO_TPC_READ_REG;

    ms_wait_for_req(card->host, mrq);
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
EXPORT_SYMBOL(memstick_read_dev_id);


/* Procudure to Switch the Interface Mode */
//int process_switch_interface(struct ms_card *card, struct ms_request *mrq)
int process_switch_interface(struct ms_card *card)
{
    struct ms_request mrq;
    unsigned int i;
    unsigned int j;

    msinfo("\n");

    memset(&mrq, 0, sizeof(struct ms_request));
    //card->reg_rw_addr.r_offset = MSPRO_REG_Status;
    //card->reg_rw_addr.r_length = 6;
    card->reg_rw_addr.r_offset = 0;
    card->reg_rw_addr.r_length = 0;
    card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
    card->reg_rw_addr.w_length = 1;

    for(i=0; i<3; i++){
        msinfo("\n");
        if (memstick_set_rw_addr(card, &mrq)==0){
            i=0;
            break;
        }
        printk("[%s] memstick_set_rw_addr() fails, try_cnt=%d\n", __FUNCTION__, i);
        msleep(10);
    }

    if(i==0){
        msleep(10);
        memset(&mrq, 0, sizeof(struct ms_request));
        mrq.tpc         = MSPRO_TPC_WRITE_REG;
        //mrq.option      = 0x80;
        mrq.byte_count  = 1;
        mrq.reg_data[0] = PARELLEL_IF;
        for(i=0; i<3; i++){
            msinfo("set card to parallel mode\n");
            ms_wait_for_req(card->host, &mrq);
            if (mrq.error){
                printk("[%s] memstick_read_dev_id() fails, try_cnt=%d\n", __FUNCTION__, i);
                msleep(10);
            }else{
                i=0;
                break;
            }
        }
    }

    msleep(10);
    if(i<3){
        //card->host->ios.set_parameter_flag = 1;
        //card->host->ops->set_ios(card->host, &card->host->ios);   //rtkms_set_ios
        //card->host->ios.set_parameter_flag = 0;

        card->host->ios.bus_width = MS_BUS_WIDTH_4;
        card->host->ops->set_ios(card->host, &card->host->ios);   //rtkms_set_ios

        msinfo("\n");
        return 0;
    }
    msinfo("\n");
    return -1;

}
//EXPORT_SYMBOL(process_switch_interface);


int mspro_read_sys_info(struct ms_card *card, unsigned char *data, int CurrentAddrOff)
{
    int i = 0;
    unsigned char ClassCode, DeviceType, SubClass;
    unsigned short totalblock = 0, blocksize = 0;
    unsigned int totalsector = 0, SysInfoAddr, SysInfoSize;
    unsigned int serial_number;
    unsigned short Start_Sector = 0;
    msinfo("\n");
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

    /* Confirm System Information   */
    if (ClassCode != 0x02) {
        printk("##### Error: NOT MSPRO\n");
        goto ERROR;
    }
    if (DeviceType != 0x00) {
        if ((DeviceType == 0x01) || (DeviceType == 0x02)
            || (DeviceType == 0x03)) {
            card->write_protect_flag = 1;
        } else {
            printk("##### DeviceType erroe\n");
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


int mspro_read_dev_info(struct ms_card *card, unsigned char *data, int CurrentAddrOff)
{
    unsigned int DevInfoAddr, DevInfoSize;
    unsigned short cylinders;
    unsigned short heads;
    unsigned short sectors_per_track;
    msinfo("\n");
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

void mspro_read_model_name(struct ms_card *card, unsigned char *data, int CurrentAddrOff)
{
    int i;
    unsigned int ModNameAddr, ModNameSize;
    msinfo("Model Name\n");
    ASSIGN_LONG(ModNameAddr,data[CurrentAddrOff+0],
                            data[CurrentAddrOff+1],
                            data[CurrentAddrOff+2],
                            data[CurrentAddrOff+3]);
    ASSIGN_LONG(ModNameSize,data[CurrentAddrOff+4],
                            data[CurrentAddrOff+5],
                            data[CurrentAddrOff+6],
                            data[CurrentAddrOff+7]);

    for ( i=0; i<0x30; i++){
        printk("%c",data[ModNameAddr+i]);
    }
    printk("\n");

}



void transform_data_format(unsigned char *data, int size)
{
    int i;
    unsigned char temp[4];
    msinfo("\n");
    for ( i=0; i<size; i+=4){
        memcpy(temp, data+ i, 4);
        data[i+0] = temp[3];
        data[i+1] = temp[2];
        data[i+2] = temp[1];
        data[i+3] = temp[0];
    }
}


//int process_read_attributes(struct ms_card *card, struct ms_request *mrq)
int process_read_attributes(struct ms_card *card)
{
    int i = 0;
    int try_cnt = 0;
    unsigned char atrb_sct_cnt=0x02;
    unsigned char temp[7] = {PARELLEL_IF, 0, atrb_sct_cnt, 0, 0, 0, 0x00};
    struct ms_request mrq;
    unsigned char *atrb_data;
    msinfo("\n");
    //data = kmalloc(32 * 1024, GFP_KERNEL);
    atrb_data = kmalloc(atrb_sct_cnt * 512, GFP_KERNEL);
    //data = kmalloc(512, GFP_KERNEL);
    if ( !atrb_data ) {
        printk("[%d] Allocation data fails\n", __LINE__);
        return -ENOMEM;
    }

    //memset(atrb_data, 0, 32*1024);
    memset(&mrq, 0, sizeof(struct ms_request));
    memset(atrb_data, 0, atrb_sct_cnt*512);

TRY_AGAIN_SETRW:
    msinfo("SET_R/W_REG_ADRS TPC\n");
    card->reg_rw_addr.r_offset = MSPRO_REG_Status;
    card->reg_rw_addr.r_length = 2;
    card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
    card->reg_rw_addr.w_length = 7;

    if (memstick_set_rw_addr(card, &mrq)){
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk("[%s] memstick_set_rw_addr() exceeds try_cnt=%d\n", __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_SETRW;
    }
    try_cnt = 0;


TRY_AGAIN_SYS_PARA:

    msinfo("WRITE_REG TPC\n");
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

    ms_wait_for_req(card->host, &mrq);

    if (mrq.error) {
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk("[%s] Writing System Parameter exceeds try_cnt=%d\n", __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_SYS_PARA;
    }
    try_cnt = 0;


TRY_AGAIN_SET_READ_ATRB:
    msinfo("SET_CMD TPC\n");
    memset(&mrq, 0, sizeof(struct ms_request));
    mrq.reg_data[0] = MSPRO_CMD_READ_ATRB;
    mrq.tpc = MSPRO_TPC_SET_CMD;
    mrq.option |= WAIT_INT;
    //mrq.option = 0xc0;
    mrq.byte_count = 1;
    ms_wait_for_req(card->host, &mrq);
    if (mrq.error) {
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk("[%s] MSPRO_TPC_SET_CMD exceeds try_cnt=%d\n", __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_SET_READ_ATRB;
    }

    if ( !(mrq.status & INT_BREQ) ){
        printk("[%d] INT_BREQ errors, mrq.status=%x\n", __LINE__, mrq.status);
        goto ERROR;
    }

    try_cnt = 0;

TRY_AGAIN_READ_ATRB:
    msinfo("Read ATRB\n");
    memset(&mrq, 0, sizeof(struct ms_request));
    mrq.long_data = atrb_data;
    mrq.tpc = MSPRO_TPC_READ_LONG_DATA;
    mrq.sector_cnt = atrb_sct_cnt;
    //mrq.sector_cnt = 0x08;
    mrq.timeout = PRO_READ_TIMEOUT;
    mrq.endian = BIG;
    ms_wait_for_req(card->host, &mrq);
    if (mrq.error) {
        printk("[%s] MSPRO_CMD_READ_ATRB failstry_cnt=%d\n", __FUNCTION__, try_cnt);
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk("[%s] MSPRO_CMD_READ_ATRB exceeds try_cnt=%d\n", __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_READ_ATRB;
    }
    try_cnt = 0;

    if ((atrb_data[0] != 0xa5) && (atrb_data[1] != 0xc3)) {
        printk("Signature code is wrong !! data[0]=%#x, data[1]=%#x\n",
            atrb_data[0], atrb_data[1]);
        goto ERROR;
    }

    if ((atrb_data[4] < 1) || (atrb_data[4] > 12)) {
        printk("Device Informantion Entry Count errors!! data[4]=%d\n", atrb_data[4]);
        goto ERROR;
    }

#ifdef SHOW_DEV_INFO_ENTRY
    printk("Device Information Entry\n");
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

        printk("ID: 0x%02x; Address:0x%08x; Size:0x%08x\n",
                atrb_data[CurrentAddrOff+8],
                SysInfoAddr,SysInfoSize);
    }
#endif

    for (i = 0; i < atrb_data[4]; i ++) {
        int CurrentAddrOff = 16 + i * 12;

        if (atrb_data[CurrentAddrOff + 8] == 0x10) { //system information

            if ( mspro_read_sys_info(card, atrb_data, CurrentAddrOff) ){
                printk("mspro_read_sys_info() fails\n");
                goto ERROR;
            }
        }

        if (atrb_data[CurrentAddrOff + 8] == 0x30) { //identify device information
            if ( mspro_read_dev_info(card, atrb_data, CurrentAddrOff) ){
                printk("mspro_read_dev_info() fails\n");
                goto ERROR;
            }
        }


        if (atrb_data[CurrentAddrOff + 8] == 0x15) { //system information
            mspro_read_model_name(card, atrb_data, CurrentAddrOff);
        }
    }
    msinfo("\n");
    kfree(atrb_data);
    return 0;

ERROR:
    msinfo("\n");
    kfree(atrb_data);
    return -4;
}
EXPORT_SYMBOL(process_read_attributes);

/*
static int mspro_wait_for_ced(struct ms_card *card, struct ms_request *mrq)
{
    msinfo("\n");
    mrq->tpc = MSPRO_TPC_GET_INT;
    ms_wait_for_req(card->host, mrq);

    if (!mrq->error) {
        if (mrq->status & (REG_INT_CMDNK | REG_INT_ERR)){
            mrq->error = -EFAULT;
            return -1;
        }else if (mrq->status & REG_INT_CED)
            return 0;
    }
}
EXPORT_SYMBOL(mspro_wait_for_ced);
*/

static int process_identify_media_type(struct ms_card *card)
{
    //struct ms_card *card=host->card;
    struct ms_request mrq;
    unsigned int i;
    unsigned int j;

    msinfo("\n");
    memset(&mrq, 0, sizeof(struct ms_request));

    card->reg_rw_addr.r_offset = MSPRO_REG_Status;
    card->reg_rw_addr.r_length = 6;
    card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
    card->reg_rw_addr.w_length = 1;

    for(i=0; i<3; i++){
        msinfo("TRY_AGAIN_SETRW:\n");
        if (memstick_set_rw_addr(card, &mrq)==0){
            i=0;
            break;
        }else{
            printk("[%s] memstick_set_rw_addr() fails, try_cnt=%d\n", __FUNCTION__, i);
            msleep(10);
        }
    }

    if(i==0){
        msleep(10);
        //memset(&mrq, 0, sizeof(struct ms_request));
        for(i=0; i<3; i++){
            msinfo("TRY_READ_DEV_ID:\n");
            if (memstick_read_dev_id(card, &mrq)){
                printk("[%s] memstick_read_dev_id() fails, try_cnt=%d\n", __FUNCTION__, i);
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

/*
static int process_identify_media_type(struct ms_card *card)
{
    //struct ms_card *card=host->card;
    struct ms_request mrq;
    unsigned int i;
    unsigned int j;
    //int err;
    msinfo("\n");
    memset(&mrq, 0, sizeof(struct ms_request));

    card->reg_rw_addr.r_offset = MSPRO_REG_Status;
    card->reg_rw_addr.r_length = 6;
    card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
    card->reg_rw_addr.w_length = 1;

    for(i=0; i<3; i++){
        msinfo("TRY_AGAIN_SETRW:\n");
        if (memstick_set_rw_addr(card, &mrq)){
            printk("[%s] memstick_set_rw_addr() fails, try_cnt=%d\n", __FUNCTION__, i);
        }else{
            msleep(10);
            i=3;
            for(j=0; j<3; j++){
                msinfo("TRY_READ_DEV_ID:\n");
                if (memstick_read_dev_id(card, &mrq)){
                    printk("[%s] memstick_read_dev_id() fails, try_cnt=%d\n", __FUNCTION__, i);
                }else{
                    break;
                }
            }
        }
    }

    if(j==3)
        return -1;

    return 0;
}
*/

static struct ms_card *ms_alloc_card(struct ms_host *host)
{

    struct ms_card *card;
    //struct ms_request mrq;
    //int try_cnt = 0;
    msinfo("\n");
    //memset(&mrq, 0, sizeof(struct ms_request));

    card = kmalloc(sizeof(struct ms_card), GFP_KERNEL);
    if (!card)
        return ERR_PTR(-ENOMEM);

    /* Initialise a MS card structure of Sysfs */
    ms_init_card_sysfs(card, host);
    msinfo("\n");
    return card;
/*
#if 0
    card->reg_rw_addr.r_offset = offsetof(struct mspro_register, id);
    card->reg_rw_addr.r_length = sizeof(struct ms_id_register);
    card->reg_rw_addr.w_offset = offsetof(struct mspro_register, param);
    card->reg_rw_addr.w_length = sizeof(struct mspro_param_register);
#else
    card->reg_rw_addr.r_offset = MSPRO_REG_Status;
    card->reg_rw_addr.r_length = 6;
    card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
    card->reg_rw_addr.w_length = 1;
#endif
TRY_AGAIN_SETRW:
    msinfo("TRY_AGAIN_SETRW:\n");
    if (memstick_set_rw_addr(card, &mrq)){
        printk("[%s] memstick_set_rw_addr() fails, try_cnt=%d\n", __FUNCTION__, try_cnt);
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            msinfo("\n");
            goto err_out;
        }
        msinfo("\n");
        goto TRY_AGAIN_SETRW;
    }
    try_cnt = 0;
    //msleep(150);
    msleep(10);
TRY_READ_DEV_ID:
    msinfo("TRY_READ_DEV_ID:\n");
    if (memstick_read_dev_id(card, &mrq)){
        printk("[%s] memstick_read_dev_id() fails, try_cnt=%d\n", __FUNCTION__, try_cnt);
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            goto err_out;
        }
        goto TRY_READ_DEV_ID;
    }
    try_cnt = 0;
*/
/*
    if(process_identify_media_type(card)==0){
        msinfo("\n");
        //host->card = card;
        return card;
    }else{
        msinfo("\n");
        kfree(card);
        return -1;
    }
*/
}


static void ms_power_up(struct ms_host *host)
{

    int bit = fls(host->ocr_avail) - 1;
    msinfo("\n");
    host->ios.vdd = bit;
    host->ios.bus_mode = MS_BUSMODE_OPENDRAIN;

    host->ios.clock = host->f_min;
    host->ios.power_mode = MS_POWER_ON;
    host->ops->set_ios(host, &host->ios);   //rtkms_set_ios

    ms_delay(2);
}

static void ms_power_off(struct ms_host *host)
{
    msinfo("\n");
    host->ios.clock = 0;
    host->ios.vdd = 0;
    host->ios.bus_mode = MS_BUSMODE_OPENDRAIN;
    host->ios.power_mode = MS_POWER_OFF;
    host->ops->set_ios(host, &host->ios);   //rtkms_set_ios
}


//static int ms_discover_cards(struct ms_host *host)
static void ms_discover_cards(struct ms_host *host)
{
    struct ms_card *card;
    msinfo("\n");
    card = ms_alloc_card(host);
    if (IS_ERR(card)) {
        printk("[%s] ms_alloc_card() fails\n", __FUNCTION__);
        return;
    }

    msinfo("card->card_type:0x%x\n",card->card_type);
    host->card=card;
    //card->card_type = TYPE_NONE;
    msinfo("host:0x%p\n",host);
    msinfo("card:0x%p\n",host->card);

    if(process_identify_media_type(card)==0){
        msinfo("\n");
        if (card->id.type == 0x01){     //Identify MSPRO Card
            msinfo("\n");
            if (card->id.category == 0x00){
                msinfo("\n");
                if (card->id.class == 0x00){
                    msinfo("\n");
                    card->card_type = CR_MS_PRO;   //before read MS card, we need to know MS or MS-pro, Beacuse method of reading different.
                    //////////
                    //host->ios.card_type = host->type = card->card_type; //硂场蛤ㄧΑ程场,ユ传竚穦Τ拜肈,и临⊿т,繷ㄓ,俱瞶attributeêㄧΑ
                    //host->ops->set_ios(host, &host->ios);   //rtkms_set_ios
                    //host->ios.card_type = card->card_type;
                    if ( mspro_startup_card(host) ){
                        printk("mspro_startup_card() fails\n");
                        card->card_type = CR_NON;
                    }
                    //////////

                }else{
                    //card->card_type = CR_NON;
                    printk("[%s] Class Register mismatch, card->id.class=%#x\n", __FUNCTION__, card->id.class);
                }
            }else{
                //card->card_type = CR_NON;
                printk("[%s] Category Register mismatch, card->id.category=%#x\n", __FUNCTION__, card->id.category);
            }
        }else{
            //no support Memoery-Stick
            //card->card_type = CR_NON;
            /*
            if (card->id.type == 0x00){ //Identify MS Card
                msinfo("\n");
                if (card->id.category == 0x00){
                    msinfo("\n");
                    if (card->id.class == 0x00){
                        msinfo("\n");
                        card->card_type = CR_MS;
                        if ( ms_init_card(host) ){
                            printk("ms_startup_card() fails\n");
                            card->card_type = CR_NON;

                        }
                    }else{
                        msinfo("\n");
                        card->card_type = CR_NON;
                        printk("[%s] Class Register mismatch, card->id.class=%#x\n", __FUNCTION__, card->id.class);
                    }
                }else{
                    card->card_type = CR_NON;
                    printk("[%s] Category Register mismatch, card->id.category=%#x\n", __FUNCTION__, card->id.category);
                }
            }else{
                printk("[%s] Type Register mismatch, card->id.type=%#x\n", __FUNCTION__, card->id.type);
                card->card_type = CR_NON;
            }
            */
        }
    }else{

        msinfo("\n");
        //card->card_type = CR_NON;
    }

    //if(card->card_type == CR_NON)
    msinfo("card->card_type:0x%x\n",card->card_type);


    if(card->card_type & (CR_MS|CR_MS_PRO))
    {
        msinfo("\n");
        /* setup rtk ms host system parameter */
        //host->ios.card_type = host->type = card->card_type;
        //host->ops->set_ios(host, &host->ios);   //rtkms_set_ios
        //list_add(&card->node, &host->cards);
        return;

    }else{
        msinfo("\n");
        ms_card_set_dead(card);
        return;

    }
}


/* A.4 Procudure to Confirm CPU Startup */
//static int process_confirm_cpu_startup(struct ms_card *card, struct ms_request *mrq)
static int process_confirm_cpu_startup(struct ms_card *card)
{
    int try_cnt = 0;
    struct ms_request mrq;

    msinfo("\n");
TRY_AGAIN_GET_INT_CED:
    memset(&mrq, 0, sizeof(struct ms_request));
    mrq.tpc = MSPRO_TPC_GET_INT;
    mrq.byte_count = 1;
    mrq.timeout = USUAL_TIMEOUT;
    mrq.reg_data[0] = 0;

    ms_wait_for_req(card->host, &mrq);

    if ( mrq.error || !(mrq.reg_data[0] & REG_INT_CED) ) {
        printk("[%s] MSPRO_TPC_GET_INT fails, try_cnt=%d\n", __FUNCTION__, try_cnt);
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            return -1;
        }
        goto TRY_AGAIN_GET_INT_CED;
    }
    try_cnt = 0;


    mrq.tpc = MSPRO_TPC_GET_INT;
    mrq.byte_count = 1;
    mrq.timeout = USUAL_TIMEOUT;
    mrq.reg_data[0] = 0;

    ms_wait_for_req(card->host, &mrq);

    if (mrq.reg_data[0] & REG_INT_ERR) {
        if (mrq.reg_data[0] & REG_INT_CMDNK) {
            card->write_protect_flag = 1;
        } else {
            printk("[%s] Media Error\n", __FUNCTION__);
            return -2;
        }
    }else
        printk("[%s] CPU start is completed\n", __FUNCTION__);

    return 0;
}

/*
static u32 MS_SetCmd(struct ms_card *card, struct ms_request *mrq, u8 read_start,
    u8 read_count,  u8 write_start, u8 write_count, u8 cmd,     u8 *data, int data_len,
    unsigned char *status)
{
    unsigned char stat[2];
    int i = 0, try_cnt = 0;
    msinfo("\n");
    if (data_len > 10) {
        printk("ERROR: data_len>10 \n");
        return -1;
    }

TRY_AGAIN_SETRW:
    card->reg_rw_addr.r_offset = read_start;
    card->reg_rw_addr.r_length = read_count;
    card->reg_rw_addr.w_offset = write_start;
    card->reg_rw_addr.w_length = write_count;

    if (memstick_set_rw_addr(card, mrq)){
        printk("[%s] memstick_set_rw_addr() fails, try_cnt=%d\n", __FUNCTION__, try_cnt);
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk("[%s] memstick_set_rw_addr() exceeds try_cnt=%d\n", __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_SETRW;
    }
    try_cnt = 0;


TRY_AGAIN_SYS_PARA:
    mrq->tpc = MSPRO_TPC_WRITE_REG;
    mrq->option = 0;
    mrq->byte_count = card->reg_rw_addr.w_length;
    mrq->timeout = PRO_WRITE_TIMEOUT;
    for ( i=0; i<write_count; i++)
        mrq->reg_data[i] = data[i];

    ms_wait_for_req(card->host, mrq);

    if (mrq->error) {
        printk("[%s] Writing System Parameter fails, try_cnt=%d\n", __FUNCTION__, try_cnt);
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk("[%s] Writing System Parameter exceeds try_cnt=%d\n", __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_SYS_PARA;
    }
    try_cnt = 0;


TRY_AGAIN_SET_CMD:
    mrq->reg_data[0] = cmd;
    mrq->tpc = MSPRO_TPC_SET_CMD;
    mrq->option |= WAIT_INT;
    mrq->byte_count = 1;
    ms_wait_for_req(card->host, mrq);
    if (mrq->error) {
        printk("[%s] MSPRO_TPC_SET_CMD failstry_cnt=%d\n", __FUNCTION__, try_cnt);
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            printk("[%s] MSPRO_TPC_SET_CMD exceeds try_cnt=%d\n", __FUNCTION__, try_cnt);
            goto ERROR;
        }
        goto TRY_AGAIN_SET_CMD;
    }

    if ( !(mrq->status & INT_BREQ) ){
        printk("[%d] INT_BREQ errors, mrq->status=%x\n", __LINE__, mrq->status);
        goto ERROR;
    }
    try_cnt = 0;


TRY_AGAIN_GET_INT_CED:
    mrq->tpc = MSPRO_TPC_GET_INT;
    mrq->byte_count = 1;
    mrq->timeout = USUAL_TIMEOUT;
    mrq->reg_data[0] = 0;

    ms_wait_for_req(card->host, mrq);

    if ( mrq->error || !(mrq->reg_data[0] & REG_INT_CED) ) {
        printk("[%s] MSPRO_TPC_GET_INT fails, try_cnt=%d\n", __FUNCTION__, try_cnt);
        try_cnt++;
        if ( try_cnt > MS_MAX_RETRY_COUNT ){
            return -1;
        }
        goto TRY_AGAIN_GET_INT_CED;
    }

    return 0;

ERROR:
    return -1;
}
*/
/*
int MS_ReadExtraData(struct ms_card *card, struct ms_request *mrq,
    unsigned int blockno, unsigned char pageno, unsigned char *extra)
{
    unsigned char temp[6], status0_1[2];
    int i = 0, rc;
    unsigned char status;
    msinfo("\n");

    if (card->MS_4bit) {
        temp[0] = 0x88;
    } else {
        temp[0] = 0x80;
    }
    temp[1] = 0;
    temp[2] = LONG2CHAR(blockno, 1);
    temp[3] = LONG2CHAR(blockno, 0);
    temp[4] = 0x40;
    temp[5] = pageno;

    while (i < MS_MAX_RETRY_COUNT) {
        i++;
        rc = MS_SetCmd(card, mrq, MS_REG_OverwriteFlag, MS_EXTRA_SIZE,
            MS_REG_SystemParm, 6, MS_CMD_BLOCK_READ, temp, 6, &status);
        if(rc) {
            continue;
        } else {
            break;
        }
    }
    if (i < MS_MAX_RETRY_COUNT) {
        i = 0;
    } else {
        printk("[%s] MS_SetCmd() fails\n", __FUNCTION__);
        return -1;
    }

    mrq->tpc = MSPRO_TPC_READ_REG;
    card->reg_rw_addr.r_length = MS_EXTRA_SIZE;
    ms_wait_for_req(card->host, mrq);

    if (mrq->error) {
        printk("[%s] MSPRO_TPC_READ_REG fails...\n", __FUNCTION__);
        return -1;
    }

    memcpy(extra, mrq->reg_data, MS_EXTRA_SIZE);

    return 0;
}
*/
/*
int MS_ReadPage(struct ms_card *card, struct ms_request *mrq, unsigned int blockno,
    unsigned char pageno, unsigned char *data)
{
    unsigned char temp[6], status0_1[2];
    int rc, error_flag = 0;
    unsigned char status;
    msinfo("\n");
    if (card->MS_4bit) {
        temp[0] = 0x88;
    } else {
        temp[0] = 0x80;
    }
    temp[1] = 0;
    temp[2] = LONG2CHAR(blockno, 1);
    temp[3] = LONG2CHAR(blockno, 0);
    temp[4] = 0x20;
    temp[5] = pageno;

    rc = MS_SetCmd(card, mrq, MS_REG_OverwriteFlag, MS_EXTRA_SIZE,
        MS_REG_SystemParm, 6, MS_CMD_BLOCK_READ, temp, 6, &status);

    if (rc) {
        printk("[%s] MS_SetCmd() MS_EXTRA_SIZE fails\n", __FUNCTION__);
        return -1;
    }

    mrq->long_data = data;
    mrq->tpc = MS_TPC_READ_PAGE_DATA;
    mrq->option = 0;
    mrq->timeout = PRO_READ_TIMEOUT;
    ms_wait_for_req(card->host, mrq);
    if (mrq->error) {
        printk("[%s] MS_NormalRead() fails, rc=%d\n", __FUNCTION__, rc);
        return -1;
    }

    return 0;
}
*/
/*
static int MS_ParseBootBlock(struct ms_card *card, struct ms_request *mrq,
    int blockno, unsigned int *totalsector)
{
    int rc, i;
    unsigned int capacity;
    unsigned char *data;
    unsigned short temp;
    msinfo("\n");

    data = (unsigned char *) kmalloc(512, GFP_KERNEL);
    if ( !data ) {
        printk("[%d] Allocation data fails\n", __LINE__);
        return -ENOMEM;
    }

    for (i=0; i<3; i++) {
        rc = MS_ReadPage(card, mrq, blockno, (unsigned char)i, data);
        if (rc ) {
            printk("[%d] MS_ReadPage() fails\n", __LINE__);
            kfree(data);
            return -1;
        }
    }

    rc = MS_ReadPage(card, mrq, blockno, 0, data);

    if ( !rc ) {
        if (data[HEADER_ID0] != 0x00 || data[HEADER_ID1] != 0x01) {
            printk("[%d] Block ID fails, data[HEADER_ID0]=%#x, data[HEADER_ID1]=%#x\n",
                __LINE__, data[HEADER_ID0], data[HEADER_ID1]);
            kfree(data);
            return -1;
        }
        if (data[PAGE_SIZE_0] != 0x02 || data[PAGE_SIZE_1] != 0x00) {
            printk("[%d] PAGE_SIZE_0 fails, data[PAGE_SIZE_0]=%#x, data[PAGE_SIZE_1]=%#x\n",
                __LINE__, data[PAGE_SIZE_0], data[PAGE_SIZE_1]);
            kfree(data);
            return -1;
        }
        if ((data[MS_Device_Type] == 1) || (data[MS_Device_Type] == 3)) {
            card->write_protect_flag = 1;
        }
        ASSIGN_SHORT(temp, data[BLOCK_SIZE_0], data[BLOCK_SIZE_1]);
        if (temp == 0x0010) {    // 16KB
            card->MSBlockShift = 5;
            card->MSPageOff = 0x1f;
        } else if (temp == 0x0008) {   // 8KB
            card->MSBlockShift = 4;
            card->MSPageOff = 0x0f;
        }
        ASSIGN_SHORT(card->MSTotalBlock,
            data[BLOCK_COUNT_0], data[BLOCK_COUNT_1]);
        ASSIGN_SHORT(temp, data[EBLOCK_COUNT_0], data[EBLOCK_COUNT_1]);
        if (!card->MS_4bit) {
            if (data[MS_4bit_Support] == 1) {
                card->reg_rw_addr.r_offset = 0;
                card->reg_rw_addr.r_length = 0;
                card->reg_rw_addr.w_offset = MS_REG_SystemParm;
                card->reg_rw_addr.w_length = 1;

                if (memstick_set_rw_addr(card, mrq)){
                    printk("[%s] memstick_set_rw_addr() fails \n", __FUNCTION__);
                    kfree(data);
                    return -1;
                }

                mrq->tpc = MS_TPC_WRITE_REG;
                mrq->byte_count = 1;
                mrq->reg_data[0] = 0x88;    //Parallel I/F

                ms_wait_for_req(card->host, mrq);
                if (mrq->error) {
                    printk("[%d] MS_TPC_WRITE_REG fails\n", __LINE__);
                    kfree(data);
                    return -1;
                }
                card->MS_4bit = 1;
            }
        }
        capacity = ((unsigned int)temp-2) << card->MSBlockShift;
    } else {
        printk("[%d] MS_ReadPage() fails\n", __LINE__);
        kfree(data);
        return -1;
    }

    card->MSBootBlock = (unsigned char)blockno;
    if (totalsector) {
        *totalsector = capacity;
    }

    kfree(data);
    return 0;
}
*/
/*
static int ms_startup_card(struct ms_host *host)
{
    struct ms_request mrq;
    int rc = 0, try_cnt = 0;
    msinfo("\n");
    memset(&mrq, 0, sizeof(struct ms_request));

    struct ms_card *card=host->card;

    int i = 0;
    unsigned char extra[MS_EXTRA_SIZE];
    unsigned int totalsector = 0;

    //list_for_each_entry(card, &host->cards, node) {
        while (i < (MAX_DEFECTIVE_BLOCK + 2)) {
            rc = MS_ReadExtraData(card, &mrq, i, 0, extra);
            if (rc) {
                i++;
                continue;
            }
#if 1
            if ((extra[0] & BLOCK_OK) && (!(extra[1] & NOT_BOOT_BLOCK))) {
                rc = MS_ParseBootBlock(card, &mrq, i, &totalsector);
                if ( !rc ) {
                    break;
                } else if (rc) {
                    i++;
                    continue;
                } else {
                    return 0;
                }
            }
#endif
            i++;
        }
    //}

    host->ios.set_parameter_flag = 1;
    host->ops->set_ios(host, &host->ios);   //rtkms_set_ios
    host->ios.set_parameter_flag = 0;

    return rc;
}
*/
/*
static int process_confirm_cpu_startup(struct ms_card *card)
{
        //struct ms_card *card=host->card;
    struct ms_request mrq;
    unsigned int i;
    unsigned int j;
    //int err;
    msinfo("\n");

    card->reg_rw_addr.r_offset = MSPRO_REG_Status;
    card->reg_rw_addr.r_length = 6;
    card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
    card->reg_rw_addr.w_length = 1;

    for(i=0; i<3; i++){
        msinfo("TRY_AGAIN_SETRW:\n");
        memset(&mrq, 0, sizeof(struct ms_request));
        if (memstick_set_rw_addr(card, &mrq)){
            printk("[%s] memstick_set_rw_addr() fails, try_cnt=%d\n", __FUNCTION__, i);
        }else{
            msleep(10);
            i=3;
            if ( ms_cpu_startup(card, &mrq)){
                msinfo("ms_cpu_startup() fails\n");
            }else{
                return 0;
            }
        }
    }
    return -1;
}
*/

static int mspro_startup_card(struct ms_host *host)
{
    struct ms_request mrq;
    int rc = 0;
    int try_cnt = 0;
    struct ms_card *card=host->card;

    msinfo("\n");

    //list_for_each_entry(card, &host->cards, node) {
/*
TRY_AGAIN_SETRW:
        msinfo("TRY_AGAIN_SETRW:%d\n", try_cnt);
        card->reg_rw_addr.r_offset = MSPRO_REG_Status;
        card->reg_rw_addr.r_length = 6;
        card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
        card->reg_rw_addr.w_length = 1;

        if (memstick_set_rw_addr(card, &mrq)){
            msinfo("memstick_set_rw_addr() fails, try_cnt=%d\n",try_cnt);
            try_cnt++;
            if ( try_cnt > MS_MAX_RETRY_COUNT ){
                msinfo("\n");
                return -1;
            }
            goto TRY_AGAIN_SETRW;
        }
        try_cnt = 0;
        msinfo("\n");
*/
        //memset(&mrq, 0, sizeof(struct ms_request));
        //if ( process_confirm_cpu_startup(card, &mrq)){
        if ( process_confirm_cpu_startup(card)){
            msinfo("confirm cpu startup fails\n");
            return -1;
        }

        //if(process_confirm_cpu_startup(card)){
        //    msinfo("\n");
        //    return -1;
        //}

/*
TRY_AGAIN_SETRW:
        msinfo("TRY_AGAIN_SETRW:%d\n", try_cnt);
        memset(&mrq, 0, sizeof(struct ms_request));
        card->reg_rw_addr.r_offset = MSPRO_REG_Status;
        card->reg_rw_addr.r_length = 6;
        card->reg_rw_addr.w_offset = MSPRO_REG_SystemParm;
        card->reg_rw_addr.w_length = 1;

        if (memstick_set_rw_addr(card, &mrq)){
            msinfo("memstick_set_rw_addr() fails, try_cnt=%d\n",try_cnt);
            try_cnt++;
            if ( try_cnt > MS_MAX_RETRY_COUNT ){
                msinfo("\n");
                return -1;
            }
            goto TRY_AGAIN_SETRW;
        }
        try_cnt = 0;
*/
        msinfo("\n");
//TRY_AGAIN_SWITCH:

        //msinfo("TRY_AGAIN_SWITCH:%d\n", try_cnt);
        //if (process_switch_interface(card, &mrq)){
        if (process_switch_interface(card)){
            msinfo("process switch interface mode fails, try_cnt=%d\n", try_cnt);
            return -1;
            //try_cnt++;
            //if ( try_cnt > MS_MAX_RETRY_COUNT ){
            //    msinfo("\n");
            //    return -3;
            //}
            //goto TRY_AGAIN_SWITCH;
        }
        //try_cnt = 0;

        //host->ios.set_parameter_flag = 1;
        //host->ops->set_ios(host, &host->ios);   //rtkms_set_ios
        //host->ios.set_parameter_flag = 0;

        //rc = process_read_attributes(card, &mrq);
        rc = process_read_attributes(card);
        if (rc){
            msinfo("process_read_attributes() fails, rc=%d\n", rc);
            return rc;
        }
    //}
    msinfo("\n");
    return rc;
}


static int ms_setup(struct ms_host *host)
{
    int rc = 0;
    msinfo("\n");
//    if (host->ios.power_mode != MS_POWER_ON) {
//        msinfo("\n");
        ms_power_up(host);          //Pcocedure to turn on the power
//    } else {
//        msinfo("\n");
//        host->ios.bus_mode = MS_BUSMODE_OPENDRAIN;
//        host->ios.clock = host->f_min;
//        host->ops->set_ios(host, &host->ios);   //rtkms_set_ios
//    }

//    if ( !host->ios.card_present ){
//        msinfo("\n");
//        printk(KERN_WARNING "No MS/MSPRO card presents in slot !!\n");
//        return ;
//    }

    /*
    if ( ms_discover_cards(host) ){
        msinfo("\n");
        printk("ms_discover_cards() fails\n");
        goto err_out;
    }
    */
    ms_discover_cards(host);
/*
    if ( host->type == CR_MS_PRO ){
        msinfo("\n");
        rc = mspro_startup_card(host);
        if ( rc ){
            printk("mspro_startup_card() fails, rc=%d\n", rc);
            return ;
        }
    }else if ( host->type == CR_MS ){
        printk("no support Memory Stick Card!\n");
        return ;

        //if ( ms_init_card(host) ){
        //    printk("ms_startup_card() fails\n");
        //    return ;
        //}
    }else{
        msinfo("\n");
        printk(KERN_ERR "Unknown Media Type\n");
        return ;
    }
*/
    ///////////////
    if(host->card && !ms_card_dead(host->card)){
        msinfo("\n");
        host->ios.bus_mode = MS_BUSMODE_PUSHPULL;
        host->ops->set_ios(host, &host->ios);   //rtksd_set_ios
        return 0;
    }else{
 err_out:
        msinfo("\n");
        kfree(host->card);
        return -1;
    }
    ///////////////
}


void ms_detect_change(struct ms_host *host)
{
    msinfo("\n");
    schedule_work(&host->detect);   //ms_rescan
    schedule();
}

EXPORT_SYMBOL(ms_detect_change);




static void ms_rescan(void *data)
{
    struct ms_host *host = data;
    //struct list_head *l, *n;
    struct ms_card *card;
    msinfo("\n");
    ms_claim_host(host);

    msinfo("\n");
    if(ms_setup(host))
    {
        ms_release_host(host);
        ms_power_off(host);
        goto terminate;
    }

    ms_release_host(host);

    //list_for_each_safe(l, n, &host->cards) {
        //struct ms_card *card = ms_list_to_card(l);
        card=host->card;
        if (!ms_card_present(card) && !ms_card_dead(card)) {
            if (ms_register_card(card))
                ms_card_set_dead(card);
            else
                ms_card_set_present(card);
        }

        if (ms_card_dead(card)) {
            //list_del(&card->node);
            host->card=NULL;
            ms_remove_card(card);
            ms_power_off(host);
        }
    //}

    //if (list_empty(&host->cards))
        //ms_power_off(host);

    if(0){  //switch clock 0=20MHz, 1=24, 2=30, 3=40, 4=48, 5=60, 6=80, 7=96
        cr_writel(0x80, CR_REG_CMD_1);
        cr_writel(3,    CR_REG_CMD_2);
        cr_writel(0x01, CR_REG_MODE_CTR);
        while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    }
terminate:
    msinfo("\n");
}


struct ms_host *ms_alloc_host(int extra, struct device *dev)
{

    struct ms_host *host;
    msinfo("\n");
    host = kmalloc(sizeof(struct ms_host) + extra, GFP_KERNEL);
    if (host) {
        memset(host, 0, sizeof(struct ms_host) + extra);

        spin_lock_init(&host->lock);
        init_waitqueue_head(&host->wq);
//      INIT_LIST_HEAD(&host->cards);
        INIT_WORK(&host->detect, ms_rescan, host);

        host->dev = dev;

        //host->max_hw_segs = 1;
        //host->max_phys_segs = 1;
        //host->max_sectors = 1 << (PAGE_CACHE_SHIFT - 9);
        //host->max_seg_size = PAGE_CACHE_SIZE;
    }else{
        printk("ms_alloc_host() fails\n");
        return NULL;
    }

    return host;
}

EXPORT_SYMBOL(ms_alloc_host);

int ms_add_host(struct ms_host *host)
{

    static unsigned int host_num;
    msinfo("\n");
    snprintf(host->host_name, sizeof(host->host_name),
         "ms%d", host_num++);

    ms_power_off(host);

    ms_detect_change(host);

    return 0;
}

EXPORT_SYMBOL(ms_add_host);


void ms_remove_host(struct ms_host *host)
{

    //struct list_head *l, *n;
    struct ms_card *card=host->card;
    msinfo("\n");
    //list_for_each_safe(l, n, &host->cards) {
    //  struct ms_card *card = ms_list_to_card(l);
        ms_remove_card(card);
    //}

    ms_power_off(host);
}

EXPORT_SYMBOL(ms_remove_host);


void ms_free_host(struct ms_host *host)
{
    msinfo("\n");
    flush_scheduled_work();
    kfree(host);
}

EXPORT_SYMBOL(ms_free_host);

#ifdef CONFIG_PM

int ms_suspend_host(struct ms_host *host, pm_message_t state)
{
    ms_claim_host(host);
    ms_power_off(host);
    ms_release_host(host);

    return 0;
}

EXPORT_SYMBOL(ms_suspend_host);

int ms_resume_host(struct ms_host *host)
{
    ms_detect_change(host);

    return 0;
}

EXPORT_SYMBOL(ms_resume_host);

#endif

MODULE_LICENSE("GPL");
