/******************************************************************************
 * $Id: rtk_ms.c 161767 2009-01-23 07:25:17Z ken.yu $
 * linux/drivers/ms/rtk_ms.c
 * Overview: Realtek MS/MSPRO Driver
 * Copyright (c) 2009 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2009-07-06 CMYu   create file
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
#include <linux/ms/host.h>
#include <linux/ms/protocol.h>

#include <asm/io.h>
#include <asm/dma.h>
#include <asm/scatterlist.h>

/* Ken define: BEGIN */
#include <asm/mach-venus/platform.h>
#include <linux/ms/card.h>
#include <linux/rtk_cr.h>
#include <linux/rtk_cr_fun.h>
#include "rtk_ms.h"
#include "ms_debug.h"

#define BANNER  "Realtek MS/MSPRO Driver"
#define VERSION  "$Id: rtk_ms.c 161767 2009-07-10 07:25:17Z ken.yu $"
#define DRIVER_NAME "rtk_ms"

#define BYTE_CNT        0x200
#define DMA_BUFFER_SIZE 64*1024

#define IRQ_PWM 7
#define SW_TRANSFORM_ENDIAN 0

static struct platform_device *rtkms_device;
static struct work_struct rtkms_cr_work;
//int ms_SetInitPara(struct ms_info *ms_card);

/* Global Variables */
//int rtk_ms_card_type;
//int rtk_ms_card_exist;
//int rtk_ms_power = 0;
int rtk_ms_wp = 0;

u32 rtkms_power_gpio;
u32 rtkms_power_bits;

static DECLARE_MUTEX (sem);
/* Ken define: END */


#define R_W_CMD 2   //read/write command
#define INN_CMD 1   //command work chip inside
#define OTT_CMD 0   //command send to card

#define RTK_RMOV 2
#define RTK_TOUT 1
#define RTK_SUCC 0
/* same with mmc &&& */

/*
 * note: same with mmc.
 */
static int wait_opt_end(struct ms_host *host,u8 cmd_type)
{
    unsigned long timeend;
    u8 event=0;

    if(host)
        event=host->ins_event;

    if(cmd_type==R_W_CMD){
        timeend=jiffies+(HZ*1);     //wait 1s
    }else if(cmd_type==INN_CMD){
        //timeend=jiffies+(HZ>>5);    //wait 31.25ms
        timeend=jiffies+(HZ>>1);    //wait 31.25ms
    }else{
        timeend=jiffies+(HZ>>1);    //wait 500ms
    }

    while (cr_readl(CR_REG_MODE_CTR)&0x01){
        if (time_before(jiffies, timeend)){
            //msinfo("wait polling down\n");
            if(event==EVENT_REMOV){
                printk("remove event! terminate waiting! \n");
                return -RTK_RMOV;
            }
            mdelay(1);
        }else{
            msinfo("rtk_opt_timeout!\n");
            return -RTK_TOUT;
        }
    }
    return RTK_SUCC;
}

/*
 * note: same with mmc.
 */
void rtkms_pullup_pinout(void)
{
    cr_writel(0x22000000, 0xb8000390);  //MS_D0, MS_D3 pull down
    cr_writel(0x03000000, 0xb8000394);  //MS_INS pull up
    cr_writel(0x00000202, 0xb8000398);  //MS_D2, MS_D1 pull down
}

/*
 * note: same with mmc.
 */
void rtkms_pulllow_pinout(void)
{
    cr_writel(0x0, 0xb8000390);  //SD_D3, SD_D0, SD_CD pull down
    cr_writel(0x0, 0xb8000394);  //SD_D1, SD_WP pull down
    cr_writel(0x0, 0xb8000398);  //SD_D2, SD_CMD pull down
}

/*
 * note: same with mmc.
 */
static int rtkms_rst_host(struct ms_host *host);
static int rtkms_host_power(int status)
{
    int err;
    u8 retries=3;
    msinfo("\n");
re_poweron:
    cr_writel(MS_POWERON>>8,    CR_REG_CMD_1);
    cr_writel(MS_POWERON,       CR_REG_CMD_2);
    cr_writel(status,           CR_REG_CMD_3);

    cr_writel(0x01, CR_REG_MODE_CTR);

    err=wait_opt_end(NULL,INN_CMD);

    if(err==RTK_SUCC){
        if(status==ON){
            msinfo("power on and pull up\n");
            rtkms_pullup_pinout();
        }else{
            msinfo("power down and pull low\n");
            rtkms_pulllow_pinout();
        }
        //mdelay(10);
    }else{  //if power control fail, pull pin low
        if(status==ON && err==RTK_TOUT && retries ){
            rtkms_rst_host(NULL);
            retries--;
            goto re_poweron;
        }

        printk("%s(%d)rtkms_host_power func fail\n",__func__,__LINE__);
        rtkms_pulllow_pinout();
    }
    return err;
}

/*
 * note: same with mmc.
 */
void rtkms_card_power(u8 status)
{
#ifndef FPGA_BOARD

    void __iomem *mmio = (void __iomem *) rtkms_power_gpio;
    msinfo("\n");
    if(status==ON){ //power on
        msinfo("Card Power on\n");
        writel(readl(mmio) & ~(1<<rtkms_power_bits),mmio);
    }else{          //power off
        msinfo("Card Power off\n");
        writel(readl(mmio) | (1<<rtkms_power_bits),mmio);
    }
#endif
}

/*
 * if reset host,don't reset(turn off) card,
 * beacuse reset card will reset status of card to idle state.
 *
 * note: same with mmc.
 */
static int rtkms_rst_host(struct ms_host *host)
{
    //stop rapper
    unsigned long timeend;
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
    cr_writel(CARD_STOP_MS>>8,  CR_REG_CMD_1);
    cr_writel(CARD_STOP_MS,     CR_REG_CMD_2);

    cr_writel(0x01, CR_REG_MODE_CTR);
    if(wait_opt_end(NULL,INN_CMD)){
        printk(KERN_WARNING "%s(%d)stop IP fail\n",__func__,__LINE__);
        goto err_out;
    }

    mdelay(10);

    //rtk host power off
    cr_writel(MS_POWERON>>8,    CR_REG_CMD_1);
    cr_writel(MS_POWERON,       CR_REG_CMD_2);
    cr_writel(OFF,              CR_REG_CMD_3);
    cr_writel(0x01, CR_REG_MODE_CTR);

    if(wait_opt_end(NULL,INN_CMD)){
        printk(KERN_WARNING "%s(%d)host power off fail\n",__func__,__LINE__);
        goto err_out;
    }

    rtkms_pulllow_pinout();
    mdelay(10);

    //rtk host power on
    msinfo("mmc send power on cmd\n");
    cr_writel(MS_POWERON>>8,    CR_REG_CMD_1);
    cr_writel(MS_POWERON,       CR_REG_CMD_2);
    cr_writel(ON,               CR_REG_CMD_3);
    cr_writel(0x01, CR_REG_MODE_CTR);
    if(wait_opt_end(NULL,INN_CMD)){
        printk(KERN_WARNING "%s(%d)host power on fail\n",__func__,__LINE__);
        goto err_out;
    }

    rtkms_pullup_pinout();
    mdelay(10);
    return  RTK_SUCC;
err_out:
    printk(KERN_WARNING "%s(%d)host reset fail\n",__func__,__LINE__);
    return -RTK_TOUT;
}


/*
 * note: same with mmc.
 */
static int rtkms_polling(struct ms_host *host)
{
    int err;

    cr_writel( CARD_POLLING>>8, CR_REG_CMD_1 );
    cr_writel( CARD_POLLING,    CR_REG_CMD_2 );

    cr_writel(0x01, CR_REG_MODE_CTR);

    err=wait_opt_end(host,INN_CMD);

    if(err){
        if(err==-RTK_TOUT)
            rtkms_rst_host(host);
    }else{
        err = cr_readl(CR_CARD_STATUS_0)&0xff;
        CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_0);
    }
    return err;
}

/////////////////////////////

static void read_rsp(u8 *rsp, int byte_count)
{
    unsigned int reg_status, reg_num;
    int i;
    u8 temp[16];
    msinfo("\n");

    if ( !rsp ){
        printk("rsp is null pointer\n");
        return;
    }
    msinfo("*rsp:0x%p, byte_count:0x%x\n",rsp,byte_count);

    if(byte_count>16)
        byte_count=16;

    for ( i=0; i<((byte_count+3)/4); i++){

        reg_num = CR_CARD_STATUS_0 + i*4;
        reg_status = cr_readl(reg_num);
        msinfo("CR_CARD_STATUS_%x:0x%08x\n",i,reg_status);
        temp[i*4+0] = reg_status & 0xff;
        temp[i*4+1] = (reg_status >> 8) & 0xff;
        temp[i*4+2] = (reg_status >> 16) & 0xff;
        temp[i*4+3] = (reg_status >> 24) & 0xff;
        CLEAR_CR_CARD_STATUS(reg_num);
    }

    memcpy(rsp, temp, byte_count);

    for(i=0; i<byte_count; i++)
        msinfo("rsp[%x]:0x%08x\n",i,rsp[i]);

}

//#define RTKMS_0_BITS          0x00
//#define RTKMS_4_BITS          0x01

void rtkms_set_para(struct ms_host *host,u8 para)
{
    msinfo("\n");
    cr_writel(MS_SETPARAMETER >> 8, CR_REG_CMD_1);
    cr_writel(MS_SETPARAMETER ,     CR_REG_CMD_2);
    cr_writel(para,                 CR_REG_CMD_3);
    cr_writel(0xff,                 CR_REG_CMD_4);

    cr_writel(0x01, CR_REG_MODE_CTR);

    wait_opt_end(host,INN_CMD);
}

/*
u32 ms_check_cr_int_status (u32 status, u8 *func_name)
{
    msinfo("\n");
    if ( status&CIS_WRITE_PROTECTION ){
        printk("{%s} Write Protection\n", func_name);
        rtk_ms_wp = 1;
    }else
        rtk_ms_wp = 0;

    if ( !(status&CIS_CARD_DETECTION) ){
        printk("{%s} No Card Existed\n", func_name);
        rtk_ms_card_exist = 0;
        return -2;
    }

    if ( status&CIS_DECODE_ERROR ){
        printk("{%s} Command Parser Error\n", func_name);
        return -3;
    }

    if ( status&CIS_CARD_STATUS_CHANGE ){
        printk("{%s} Card Status Change\n", func_name);
    }

    if ( status&CIS_DECODE_FINISH ){
        printk("{%s} Command Parser Finish\n", func_name);
    }

    return 0;
}
*/

/*****************************************************************************\
 *                                                                           *
 * Realtek Register Helper funcitons                                         *
 *                                                                           *
\*****************************************************************************/
/*
static unsigned int rtkms_polling(void)
{
    u32 status;
    msinfo("\n");
    cr_writel( CARD_POLLING>>8, CR_REG_CMD_1 );
    cr_writel( CARD_POLLING,    CR_REG_CMD_2 );

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);
    //check command finishes
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,INN_CMD);

    status = cr_readl(CR_CARD_STATUS_0);

    CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_0);

    return status;
}
*/

/*
*******************************************************************************
*                         MS Power On Command
* FUNCTION MS_POWER
*******************************************************************************
*/
/**
  Send MS_POWERON command to RTS5121 through bulk pipe. This function would
  not handle error case. It returns error status to the upper function only.

  \param us         USB device.
  \param option         Power control.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
void MS_POWER(int status)
{
    msinfo("\n");
    cr_writel(MS_POWERON>>8,CR_REG_CMD_1);
    cr_writel(MS_POWERON,   CR_REG_CMD_2);
    cr_writel(status,       CR_REG_CMD_3);
    cr_writel(0,            CR_REG_CMD_4);  // Reserved
    //register mode enable, run cmd
    cr_writel(0x01,         CR_REG_MODE_CTR);
    //check command finishes
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,INN_CMD);
}
*/

/*
*******************************************************************************
*                          MS Set Command
* FUNCTION MS_SetCmd_Cmd
*******************************************************************************
*/
/**
  Send MS_SETCMD command to RTS5121 through bulk pipe. This function would not
  handle error case. It returns error status to the upper function only.

  \param us         USB device.
  \param read_start     Read start.
  \param read_count     Read count.
  \param write_start        Write start.
  \param write_count        Write count.
  \param cmd            Command.
  \param data           Data to be written.
  \param data_len       Data length.
  \param status         Status.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static u32 MS_SetCmd_Cmd(struct ms_info *ms_card, u8 read_start, u8 read_count,
    u8 write_start, u8 write_count, u8 cmd,
    u8 *data, int data_len, unsigned char *status)
{
    unsigned char stat[2];
    int i;
    msinfo("\n");
    if (data_len > 10) {
        printk("ERROR: data_len>10 \n");
        return -1;
    }

    cr_writel(MS_SETCMD>>8, CR_REG_CMD_1);
    cr_writel(MS_SETCMD,    CR_REG_CMD_2);
    cr_writel(read_start,   CR_REG_CMD_3);
    cr_writel(read_count,   CR_REG_CMD_4);
    cr_writel(write_start,  CR_REG_CMD_5);
    cr_writel(write_count,  CR_REG_CMD_6);

    for (i = 0; i < data_len; i++)
        cr_writel(data[i], CR_REG_CMD_7 + i*4);

    cr_writel(cmd,          CR_REG_CMD_17);
    cr_writel(NOT_CHECK_INT,CR_REG_CMD_18);

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);

    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,INN_CMD);

    read_rsp(stat, 2);

    if (status)
        *status = stat[1];

    return 0;
}
*/
/*
*******************************************************************************
*                          MS Set Command Automatically
* FUNCTION MS_SetCmd
*******************************************************************************
*/
/**
  Set command to MS card.
  This function calls MS_SetCmd_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param read_start     Read start.
  \param read_count     Read count.
  \param write_start        Write start.
  \param write_count        Write count.
  \param cmd            Command.
  \param data           Data to be written.
  \param data_len       Data length.
  \param status         Status.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static int MS_SetCmd(struct ms_info *ms_card, u8 read_start, u8 read_count,
    u8 write_start, u8 write_count, u8 cmd,
    u8 *data, int data_len, unsigned char *status)
{
    int rc;
    msinfo("\n");
    rc = MS_SetCmd_Cmd(ms_card, read_start, read_count, write_start,
        write_count, cmd, data, data_len, status);

    return rc;
}
*/
/*
int MS_Send_Vendor_Cmd(unsigned int cmd)
{
    msinfo("\n");
    cr_writel(cmd>>8, CR_REG_CMD_1);
    cr_writel(cmd, CR_REG_CMD_2);
    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);
    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,INN_CMD);

    return 0;
}
*/

/*
********************************************************************************
*                       Switch clock of card
* FUNCTION MS_Card_SwitchClock
********************************************************************************
*/
/**
  Switch the frequency of the working clock of specified card module.

  \param us     USB device.
  \param clock_freq Frequency of the working clock.

  \retval       Success or Fail
********************************************************************************
*/
/*
int MS_Card_SwitchClock(unsigned char clock)
{
    msinfo("\n");
    cr_writel(0x80, CR_REG_CMD_1);
    cr_writel(clock, CR_REG_CMD_2);
    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);
    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,INN_CMD);

    return 0;
}
*/

/*
*******************************************************************************
*                       Switch Clock
* FUNCTION ms_SwitchClock
*******************************************************************************
*/
/**
  Switch SD/MMC card module to a proper clock rate.
  This function is usually called in rwSD in fear that the clock rate has been
  changed by other card module.

  \param us         USB device.

  \retval           Success or fail.
*******************************************************************************
*/
/*
int ms_SwitchClock(struct ms_info *ms_card)
{
    msinfo("\n");
#if 0
    struct rts5121_device *rts5121_dev =
        (struct rts5121_device *)(us->extra);
    int rc;

    if (rts5121_dev->ms_card->bIsMSPro || rts5121_dev->ms_card->MS_4bit) {
        if (rts5121_dev->ssc_enable) {
            rc = SSC_SwitchClock(us, 80);
        } else {
            rc = MS_Card_SwitchClock(us, CLOCK_SELECT_80);
        }
        if (rc != USBSTOR_GOOD) {
            TraceMsg(us, MS, EMERG_FATAL_ERROR, __FUNCTION__,
                __FILE__, __LINE__, rc, 0);
            return USBSTOR_ERROR;
        }
    } else {
        if (rts5121_dev->ssc_enable) {
            rc = SSC_SwitchClock(us, 40);
        } else {
            rc = MS_Card_SwitchClock(us, CLOCK_SELECT_40);
        }
        if (rc != USBSTOR_GOOD) {
            TraceMsg(us, MS, EMERG_FATAL_ERROR, __FUNCTION__,
                __FILE__, __LINE__, rc, 0);
            return USBSTOR_ERROR;
        }
    }
#endif

    return 0;
}
*/
/*
static u32 MS_RegisterCheck_Cmd(struct ms_info *ms_card, u8 reg_idx, u16 *reg_val)
{
    unsigned int reg_status;
    msinfo("\n");
    //set MS_REGISTERCHECK cmd
    cr_writel(MS_REGISTERCHECK>>8, CR_REG_CMD_1);
    cr_writel(MS_REGISTERCHECK, CR_REG_CMD_2);
    cr_writel(reg_idx, CR_REG_CMD_3);
    cr_writel(0x00, CR_REG_CMD_4);  //Reserved

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);
    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,INN_CMD);

    if ( reg_idx==0 ){
        reg_status = cr_readl(CR_CARD_STATUS_0);
        CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_0);
    }else{
        reg_status = cr_readl(CR_CARD_STATUS_1);
        CLEAR_CR_CARD_STATUS(CR_CARD_STATUS_1);
    }

    *reg_val = reg_status & 0xffff;

    return 0;
}
*/
/*
int MS_RegisterCheck(struct ms_info *ms_card, u8 reg_idx, u16 *reg_val)
{
    int rc;
    msinfo("\n");
    rc = MS_RegisterCheck_Cmd(ms_card, reg_idx, reg_val);

    return rc;
}
*/

/*
*******************************************************************************
*                        MS Get Sector Number Command
* FUNCTION MS_GetSectorNum_Cmd
*******************************************************************************
*/
/**
  Send MS_GETSECTORNUM command to RTS5121 through bulk pipe. This function
  would not handle error case. It returns error status to the upper function
  only.

  \param us         USB device.
  \param INT_reg        INT register
  \param sectcount      Sector count.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static u32 MS_GetSectorNum_Cmd(struct ms_info *ms_card, u8 *INT_reg, u16 *sectcount)
{
    u16 value;
    unsigned char temp[2];
    msinfo("\n");
    cr_writel(MS_GETSECTORNUM>>8, CR_REG_CMD_1);
    cr_writel(MS_GETSECTORNUM, CR_REG_CMD_2);
    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);
    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,INN_CMD);

    read_rsp(temp, 1);

    ASSIGN_SHORT(value, temp[1], temp[0]);
    if (sectcount) {
        *sectcount = value & 0x7FF;
    }
    if (INT_reg) {
        *INT_reg = (u8) ((value & 0xF000) >> 12);
    }

    return 0;
}
*/

/*
*******************************************************************************
*                        MS Get Sector Number
* FUNCTION MS_GetSectorNum
*******************************************************************************
*/
/**
  Send this command to get sector count of hardware when error occurs in r/w
  process.
  This function calls MS_GetSectorNum_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param INT_reg        INT register.
  \param sectcount      Sector count.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
int MS_GetSectorNum(struct ms_info *ms_card, u8 *INT_reg, u16 *sectcount)
{
    int rc;
    msinfo("\n");
    rc = MS_GetSectorNum_Cmd(ms_card, INT_reg, sectcount);

    return rc;
}
*/
/*
static u32 MS_GetStatus_Cmd(struct ms_info *ms_card, u8 *status)
{
    msinfo("\n");
    cr_writel(MS_GETSTATUS>>8, CR_REG_CMD_1);
    cr_writel(MS_GETSTATUS, CR_REG_CMD_2);
    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);
    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,INN_CMD);

    read_rsp(status, 2);

    return 0;
}
*/
/*
int MS_GetStatus(struct ms_info *ms_card, u8 *status)
{
    int rc;
    msinfo("\n");
    rc = MS_GetStatus_Cmd(ms_card, status);

    return rc;
}
*/

/*
*******************************************************************************
*                         SD Set Parameter Command
* FUNCTION MS_SetParameter_Cmd
*******************************************************************************
*/
/**
  Send MS_SETPARAMETER command to RTS5121 through bulk pipe. This function
  would not handle error case. It returns error status to the upper function
  only.

  \param us         USB device.
  \param time_para      Time parameter.
  \param option         Extra option.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static u32 MS_SetParameter_Cmd(struct ms_info *ms_card, unsigned char para)
{
    msinfo("para:0x%x\n",para);
    cr_writel(MS_SETPARAMETER>>8,   CR_REG_CMD_1);
    cr_writel(MS_SETPARAMETER,      CR_REG_CMD_2);
    cr_writel(para,                 CR_REG_CMD_3);
    cr_writel(0xff,                 CR_REG_CMD_4);
    //register mode enable, run cmd
    cr_writel(0x01,                 CR_REG_MODE_CTR);
    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,INN_CMD);

    return 0;
}
*/

/*
*******************************************************************************
*                         SD Set Parameter
* FUNCTION MS_SetParameter
*******************************************************************************
*/
/**
  Set working mode of SD card module, including bit mode, speed mode, sample
  phase, and etc.
  This function calls MS_SetParameter_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param time_para      Time parameter.
  \param option         Extra option.

  \retval           Succeed or fail.
*******************************************************************************
*/
/* int MS_SetParameter(struct ms_info *ms_card)
{
    int rc;
    msinfo("\n");
    rc = MS_SetParameter_Cmd(ms_card,
            MS_4_BIT_MODE|SAMPLE_RISING|PUSH_TIME);
    if ( rc )
        return rc;
    return rc;
}
*/

/*
*******************************************************************************
*                        MS Normal Read Command
* FUNCTION MS_NormalRead_Cmd
*******************************************************************************
*/
/**
  Send MS_NORMALREAD command to RTS5121 through bulk pipe. This function would
  not handle error case. It returns error status to the upper function only.

  \param us         USB device.
  \param TPC            TPC code.
  \param option         Option.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
static u32 MS_NormalRead_Cmd(struct ms_info *ms_card, u8 TPC, u8 option,
    unsigned char *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    msinfo("\n");
    cr_writel((unsigned int) data,  CR_DMA_IN_ADDR);
    cr_writel(MS_NORMALREAD>>8,     CR_REG_CMD_1);
    cr_writel(MS_NORMALREAD,        CR_REG_CMD_2);
    cr_writel(TPC,                  CR_REG_CMD_3);
    cr_writel(option,               CR_REG_CMD_4);

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);
    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,R_W_CMD);

    read_rsp(stat, 2);

    if (status) {
        if (option & RW_AUTO_GET_INT) {
            *status = stat[1];
        } else {
            *status = stat[0];
        }
    }

    return 0;
}


/*
*******************************************************************************
*                        MS Normal Read
* FUNCTION MS_NormalRead
*******************************************************************************
*/
/**
  Send read TPC to MS card and read one page data from the media of MS card.
  Hardware will wait INT automatically. It is not for MS Pro card but MS card
  actually.
  This function calls MS_NormalRead_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param TPC            TPC code.
  \param wait_INT       Wait INT signal.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
int MS_NormalRead(struct ms_info *ms_card, u8 TPC, u8 wait_INT,
    unsigned char *data, unsigned char *status, int timeout)
{
    u32 rc;
    msinfo("\n");
    rc = MS_NormalRead_Cmd(ms_card, TPC, wait_INT, data, status, timeout);

    return rc;
}


/*
*******************************************************************************
*                        MS Normal Write Command
* FUNCTION MS_NormalWrite_Cmd
*******************************************************************************
*/
/**
  Send MS_NORMALWRITE command to RTS5121 through bulk pipe. This function
  would not handle error case. It returns error status to the upper function
  only.

  \param us         USB device.
  \param TPC            TPC code.
  \param option         Option.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
static u32 MS_NormalWrite_Cmd(struct ms_info *ms_card, u8 TPC, u8 option,
    unsigned char *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    msinfo("\n");
    cr_writel((unsigned int) data, CR_DMA_IN_ADDR);
    cr_writel(MS_NORMALWRITE>>8, CR_REG_CMD_1);
    cr_writel(MS_NORMALWRITE, CR_REG_CMD_2);
    cr_writel(TPC, CR_REG_CMD_3);
    cr_writel(option, CR_REG_CMD_4);

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);

    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,R_W_CMD);

    read_rsp(stat, 2);

    if (status) {
        if (option & RW_AUTO_GET_INT) {
            *status = stat[1];
        } else {
            *status = stat[0];
        }
    }

    return 0;
}


/*
*******************************************************************************
*                        MS Normal Write
* FUNCTION MS_NormalWrite
*******************************************************************************
*/
/**
  Send write TPC to MS card and write one page data to the media of MS card.
  Hardware will wait INT automatically. It is not for MS Pro card but MS card
  actually.
  This function calls MS_NormalWrite_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param TPC            TPC code.
  \param wait_INT       Wait INT signal.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
int MS_NormalWrite(struct ms_info *ms_card, u8 TPC, u8 wait_INT,
    unsigned char *data, unsigned char *status, int timeout)
{
    u32 rc;
    msinfo("\n");
    rc = MS_NormalWrite_Cmd(ms_card, TPC,
        wait_INT, data, status, timeout);

    return rc;
}


/*
*******************************************************************************
*                        MS Auto Read Command
* FUNCTION MS_AutoRead_Cmnd
*******************************************************************************
*/
/**
  Send MS_AUTOREAD command to RTS5121 through bulk pipe. This function would
  not handle error case. It returns error status to the upper function only.

  \param us         USB device.
  \param TPC            TPC code.
  \param sector_count       Sector count.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
static u32 MS_AutoRead_Cmd(struct ms_info *ms_card, u8 TPC, u16 sector_count,
    void *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    int total_bytes;
    msinfo("TPC:0x%x; sector_count:0x%x\n",TPC,sector_count);
    total_bytes = 512 * sector_count;

    cr_writel((unsigned int) data,  CR_DMA_IN_ADDR);
    cr_writel(MS_AUTOREAD>>8,       CR_REG_CMD_1);
    cr_writel(MS_AUTOREAD,          CR_REG_CMD_2);
    cr_writel(TPC,                  CR_REG_CMD_3);
    cr_writel(0,                    CR_REG_CMD_4);  // Reserved
    cr_writel(sector_count,         CR_REG_CMD_5);
    cr_writel(sector_count>>8,      CR_REG_CMD_6);

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);

    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,R_W_CMD);

    read_rsp(stat, 2);

    if (status) {
        *status = stat[0];
    }

    return 0;
}


/*
*******************************************************************************
*                        MS Auto Read
* FUNCTION MS_AutoRead
*******************************************************************************
*/
/**
  Send read TPC to MS card and read data from the media of MS card.
  This function calls MS_AutoRead_Cmnd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param TPC            TPC code.
  \param sector_count       Sector count.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
int MS_AutoRead(struct ms_info *ms_card, u8 TPC, u16 sector_count,
    void *data, unsigned char *status, int timeout)
{
    u32 rc;
    msinfo("\n");
    rc = MS_AutoRead_Cmd(ms_card, TPC, sector_count,
        data, status, timeout);

    return rc;
}


/*
*******************************************************************************
*                        MS Auto Write Command
* FUNCTION MS_AutoWrite_Cmd
*******************************************************************************
*/
/**
  Send MS_AUTOWRITE command to RTS5121 through bulk pipe. This function would
  not handle error case. It returns error status to the upper function only.

  \param us         USB device.
  \param TPC            TPC code.
  \param sector_count       Sector count.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
static u32 MS_AutoWrite_Cmd(struct ms_info *ms_card, u8 TPC, u16 sector_count,
    void *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    int total_bytes;
    msinfo("\n");
    total_bytes = 512 * sector_count;

    cr_writel((unsigned int) data,  CR_DMA_OUT_ADDR);
    cr_writel(MS_AUTOWRITE>>8,      CR_REG_CMD_1);
    cr_writel(MS_AUTOWRITE,         CR_REG_CMD_2);
    cr_writel(TPC,                  CR_REG_CMD_3);
    cr_writel(0x00,                 CR_REG_CMD_4);  // Reserved
    cr_writel(sector_count,         CR_REG_CMD_5);
    cr_writel(sector_count>>8,      CR_REG_CMD_6);

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);

    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,R_W_CMD);

    read_rsp(stat, 2);

    if (stat[0] & 0xC2) {
        return -1;
    }

    if (status) {
        *status = stat[0];
    }

    return 0;
}


/*
*******************************************************************************
*                        MS Auto Write
* FUNCTION MS_AutoWrite
*******************************************************************************
*/
/**
  Send write TPC to MS card and write data to the media of MS card.
  This function calls MS_AutoWrite_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param TPC            TPC code.
  \param sector_count       Sector count.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
int MS_AutoWrite(struct ms_info *ms_card, u8 TPC, u16 sector_count,
    void *data, unsigned char *status, int timeout)
{
    u32 rc;
    msinfo("\n");
    rc = MS_AutoWrite_Cmd(ms_card, TPC, sector_count,
        data, status, timeout);

    return rc;
}


/*
*******************************************************************************
*                        MS Multi Read Command
* FUNCTION MS_MultiRead_Cmd
*******************************************************************************
*/
/**
  Send MS_MULTIREAD command to RTS5121 through bulk pipe. This function would
  not handle error case. It returns error status to the upper function only.

  \param us         USB device.
  \param TPC            TPC code.
  \param option         Option.
  \param sect_count     Sector count.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static u32 MS_MultiRead_Cmd(struct ms_info *ms_card, u8 TPC, u8 option, u16 sect_count,
    unsigned char *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    msinfo("\n");
    cr_writel((unsigned int) data, CR_DMA_IN_ADDR);

    cr_writel(MS_MULTIREAD>>8, CR_REG_CMD_1);
    cr_writel(MS_MULTIREAD, CR_REG_CMD_2);
    cr_writel(TPC, CR_REG_CMD_3);
    cr_writel(option, CR_REG_CMD_4);
    cr_writel(sect_count, CR_REG_CMD_5);
    cr_writel(sect_count>>8, CR_REG_CMD_6);

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);

    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,R_W_CMD);

    read_rsp(stat, 2);

    if (status) {
        *status = stat[1];
    }

    return 0;
}
*/

/*
*******************************************************************************
*                        MS Multi Read
* FUNCTION MS_MultiRead
*******************************************************************************
*/
/**
  Send read TPC to MS card and read multiple pages of data. If the sector is
  the last sector of block, send BLOCK_END command.
  This function calls MS_MultiRead_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param TPC            TPC code.
  \param option         Option.
  \param sect_count     Sector count.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static int MS_MultiRead(struct ms_info *ms_card, u8 TPC, u8 option, u16 sect_count,
    unsigned char *data, unsigned char *status, int timeout)
{
    u32 rc;
    msinfo("\n");
    rc = MS_MultiRead_Cmd(ms_card, TPC, option, sect_count,
        data, status, timeout);

    return rc;
}
*/

/*
*******************************************************************************
*                        MS Multi Write Command
* FUNCTION MS_MultiWrite_Cmd
*******************************************************************************
*/
/**
  Send MS_MULTIWRITE command to RTS5121 through bulk pipe. This function would
  not handle error case. It returns error status to the upper function only.

  \param us         USB device.
  \param TPC            TPC code.
  \param option         Option.
  \param sect_count     Sector count.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static u32 MS_MultiWrite_Cmd(struct ms_info *ms_card, u8 TPC, u8 option, u16 sect_count,
    unsigned char *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    msinfo("\n");
    cr_writel((unsigned int) data, CR_DMA_OUT_ADDR);

    cr_writel(MS_MULTIWRITE>>8, CR_REG_CMD_1);
    cr_writel(MS_MULTIWRITE, CR_REG_CMD_2);
    cr_writel(TPC, CR_REG_CMD_3);
    cr_writel(option, CR_REG_CMD_4);
    cr_writel(sect_count, CR_REG_CMD_5);
    cr_writel(sect_count>>8, CR_REG_CMD_6);

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);

    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,R_W_CMD);

    read_rsp(stat, 2);

    if (status)
        *status = stat[1];

    return 0;
}
*/

/*
*******************************************************************************
*                        MS Multi Write
* FUNCTION MS_MultiWrite
*******************************************************************************
*/
/**
  Send write TPC to MS card and write multiple pages of data. If the sector is
  the last sector of block, send BLOCK_END command.
  This function calls MS_MultiWrite_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param TPC            TPC code.
  \param option         Option.
  \param sect_count     Sector count.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static int MS_MultiWrite(struct ms_info *ms_card, u8 TPC, u8 option, u16 sect_count,
    unsigned char *data, unsigned char *status, int timeout)
{
    u32 rc;
    msinfo("\n");
    rc = MS_MultiWrite_Cmd(ms_card, TPC, option, sect_count,
        data, status, timeout);

    return rc;
}
*/

/*
*******************************************************************************
*                        MS Copy Page Command
* FUNCTION MS_CopyPage_Cmd
*******************************************************************************
*/
/**
  Send MS_COPYPAGE command to RTS5121 through bulk pipe. This function would
  not handle error case. It returns error status to the upper function only.

  \param us         USB device.
  \param old_phyblock       Old physical block.
  \param new_phyblock       New physical block.
  \param logblock       Logical block.
  \param bus_width      Bus width.
  \param startpage      Start page.
  \param pagecount      Page count.
  \param status         Status

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static u32 MS_CopyPage_Cmd(struct ms_info *ms_card, u16 old_phyblock, u16 new_phyblock,
    u16 logblock, u8 bus_width, u8 startpage, u8 pagecount, u16 *status)
{
    unsigned char stat[2];
    msinfo("\n");

    cr_writel(MS_COPYPAGE>>8,   CR_REG_CMD_1);
    cr_writel(MS_COPYPAGE,      CR_REG_CMD_2);
    cr_writel(old_phyblock,     CR_REG_CMD_3);
    cr_writel(old_phyblock>>8,  CR_REG_CMD_4);
    cr_writel(new_phyblock,     CR_REG_CMD_5);
    cr_writel(new_phyblock>>8,  CR_REG_CMD_6);
    cr_writel(logblock,         CR_REG_CMD_7);
    cr_writel(logblock>>8,      CR_REG_CMD_8);
    cr_writel(bus_width,        CR_REG_CMD_9);
    cr_writel(startpage,        CR_REG_CMD_10);
    cr_writel(pagecount,        CR_REG_CMD_11);
    cr_writel(0, CR_REG_CMD_12);     //Reserved

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);

    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,R_W_CMD);

    read_rsp(stat, 2);

    if (status) {
        ASSIGN_SHORT(*status, stat[1], stat[0]);
    }

    return 0;
}
*/

/*
*******************************************************************************
*                        MS Copy Page
* FUNCTION MS_CopyPage
*******************************************************************************
*/
/**
  Copy pages from old block to new block automatically.
  This function calls MS_CopyPage_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param old_phyblock       Old physical block.
  \param new_phyblock       New physical block.
  \param logblock       Logical block.
  \param bus_width      Bus width.
  \param startpage      Start page.
  \param pagecount      Page count.
  \param status         Status

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static int MS_CopyPage(struct ms_info *ms_card, u16 old_phyblock, u16 new_phyblock,
    u16 logblock, u8 bus_width, u8 startpage, u8 pagecount, u16 *status)
{
    u32 rc;
    msinfo("\n");
    rc = MS_CopyPage_Cmd(ms_card, old_phyblock, new_phyblock, logblock,
        bus_width, startpage, pagecount, status);

    return rc;
}
*/

/*
*******************************************************************************
*                           Get SD Clock
* FUNCTION GetSDClock
*******************************************************************************
*/
/**
  Get SD clock from transfer speed field of CIS response.

  \param ms_card    SD card structure.
  \param tran_speed Transfer speed.

  \retval       Success or fail.
*******************************************************************************
*/
/*
static int GetSDClock(struct ms_info *ms_card, unsigned char tran_speed)
{
    msinfo("\n");
#if 0
    if ((tran_speed & 0x07) == 0x02) {          // 10Mbits/s
        if((tran_speed & 0xf8) >= 0x30) {       // >25Mbits/s
            ms_card->ms_clock = CARD_SWITCHCLOCK_48MHZ;
        } else if ((tran_speed & 0xf8) == 0x28) {   // 20Mbits/s
            ms_card->ms_clock = CARD_SWITCHCLOCK_40MHZ;
        } else if ((tran_speed & 0xf8) == 0x20) {   // 15Mbits/s
            ms_card->ms_clock = CARD_SWITCHCLOCK_30MHZ;
        } else if ((tran_speed & 0xf8) >= 0x10) {   // 12Mbits/s
            ms_card->ms_clock = CARD_SWITCHCLOCK_24MHZ;
        } else if ((tran_speed & 0x08) >= 0x08) {   // 10Mbits/s
            ms_card->ms_clock = CARD_SWITCHCLOCK_20MHZ;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
#endif
    return 0;
}
*/

/*
*******************************************************************************
*                         Calculate SD/MMC Card Total Sector
* FUNCTION CalcTotalSector
*******************************************************************************
*/
/**
  Calculate SD/MMC Card total sector from CIS response.

  \param rsp        Response buffer.
  \param rsp_len    Response length.

  \retval       Total sector.
*******************************************************************************
*/
/*
static unsigned int CalcTotalSector(unsigned char *rsp, int rsp_len)
{
    unsigned char read_bl_len, c_size_mult;
    unsigned short c_size;
    unsigned int blocknr, block_len, mult, TotalSector;
    msinfo("\n");
    if (rsp_len < 16) {
        return 0;
    }

    read_bl_len = rsp[6] & 0x0f;
    c_size = ((unsigned short)(rsp[7] & 0x03) << 10) +
        ((unsigned short)rsp[8] << 2) +
        ((unsigned short)(rsp[9] & 0xc0) >> 6);
    c_size_mult = (unsigned char)((rsp[10] & 0x03) << 1) +
        ((unsigned char)(rsp[11] & 0x80) >> 7);
    mult = 1 << (c_size_mult + 2);
    blocknr = (c_size + 1) * mult;
    block_len = 1 << read_bl_len;
    TotalSector = blocknr * block_len / 512;

    return TotalSector;
}
*/

/*
*******************************************************************************
*                        MS Write Bytes Command
* FUNCTION MS_WriteBytes_Cmd
*******************************************************************************
*/
/**
  Send MS_WRITEBYTES command to RTS5121 through bulk pipe. This function would
  not handle error case. It returns error status to the upper function only.

  \param us         USB device.
  \param TPC            TPC code.
  \param option         Option.
  \param byte_count     Byte count.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
static u32 MS_WriteBytes_Cmd(struct ms_info *ms_card, u8 TPC, u8 option, u8 byte_count,
    unsigned char *data, unsigned char *status, int timeout)
{
    unsigned char stat[2];
    int i;
    u8 bytecount;
    u8 fillcnt;
    msinfo("TPC:0x%x, option:0x%x, byte_cnt:0x%x\n"
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

    if(TPC==0x0e){
        msinfo("cmd:0x%x\n",data[0]);
    }

    for (i=0; i<bytecount; i++){
        cr_writel(data[i], CR_REG_CMD_7+i*4);
    }

    fillcnt=20-(bytecount+6);   //fill 0 to CR_REG_CMD_XX from CR_REG_CMD_XX to CR_REG_CMD_20.
    if(fillcnt){
        for (i=0; i<fillcnt; i++){
            cr_writel(0, CR_REG_CMD_7+(i+bytecount)*4);
        }
    }

#if RTK_DEBUG
    rtk_cr_dump_cmd_reg(16);
#endif

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);

    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,R_W_CMD);

    read_rsp(stat, 2);
    msinfo("stat[0]:0x%x,stat[1]:0x%x\n",stat[0],stat[1]);
    if (status) {
        if (option & RW_AUTO_GET_INT) {
            msinfo("RW_AUTO_GET_INT\n");
            *status = stat[1];
        } else {
            msinfo("Non RW_AUTO_GET_INT\n");
            *status = stat[0];
        }
    }
    msinfo("status:0x%x\n",*status);

    return 0;
}


/*
*******************************************************************************
*                        MS Write Bytes
* FUNCTION MS_WriteBytes
*******************************************************************************
*/
/**
  Send write TPC to MS card and write data to MS card internal registers. If
  byte_count is larger than the count of data to be written the reset data is
  set to 0xFF and written to internal registers.
  This function calls MS_WriteBytes_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param TPC            TPC code.
  \param wait_INT       Wait INT signal.
  \param byte_count     Byte count.
  \param data           Data buffer.
  \param status         Status buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
int MS_WriteBytes(struct ms_info *ms_card, u8 TPC, u8 wait_INT, u8 byte_count,
    unsigned char *data, unsigned char *status, int timeout)
{
    u32 rc;
    msinfo("\n");
    rc = MS_WriteBytes_Cmd(ms_card, TPC, wait_INT, byte_count,
        data, status, timeout);

    return rc;
}


/*
*******************************************************************************
*                        MS Read Bytes Command
* FUNCTION MS_ReadBytes_Cmd
*******************************************************************************
*/
/**
  Send MS_READBYTES command to RTS5121 through bulk pipe. This function would
  not handle error case. It returns error status to the upper function only.

  \param us         USB device.
  \param TPC            TPC code.
  \param byte_count     Byte count.
  \param data           Data buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
static u32 MS_ReadBytes_Cmd(struct ms_info *ms_card, u8 TPC, u8 byte_count,
    unsigned char *data, int timeout)
{
    //int buf_size;
    int rtn_cnt;
    int i;
    unsigned char *buf;
    msinfo("\n");

    if (byte_count % 2) {
        rtn_cnt = byte_count + 1;
    } else {
        rtn_cnt = byte_count;
    }

    buf = (unsigned char *)kmalloc(rtn_cnt, GFP_KERNEL);

    if ( !buf ) {
        printk("[%s] Allocation buf fails\n", __FUNCTION__);
        return -ENOMEM;
    }

    cr_writel(MS_READBYTES>>8,  CR_REG_CMD_1);
    cr_writel(MS_READBYTES,     CR_REG_CMD_2);
    cr_writel(TPC,              CR_REG_CMD_3);
    cr_writel(0,                CR_REG_CMD_4); // Reserved
    cr_writel(byte_count,       CR_REG_CMD_5);
    cr_writel(0,                CR_REG_CMD_6);  // Reserved

    //register mode enable, run cmd
    cr_writel(0x01, CR_REG_MODE_CTR);

    //check command finish
    //while (cr_readl(CR_REG_MODE_CTR) & 0x01);
    wait_opt_end(NULL,R_W_CMD);

    read_rsp(buf, rtn_cnt);

    if (data != NULL) {
        for (i=0; i<byte_count; i++) {
            msinfo("buf[%x]:0x%x\n",i,buf[i]);
            data[i] = buf[i];
        }
        if (rtn_cnt > byte_count) {
            msinfo("\n");
            data[byte_count-1] = buf[rtn_cnt-1];
        }
    }

    kfree(buf);
    return 0;
}


/*
*******************************************************************************
*                        MS Read Bytes
* FUNCTION MS_ReadBytes
*******************************************************************************
*/
/**
  Send read TPC to MS card and read data from MS card internal registers.
  This function calls MS_ReadBytes_Cmd to send the command to RTS5121, and
  handles the error case according to the returned value of that function.

  \param us         USB device.
  \param TPC            TPC code.
  \param byte_count     Byte count.
  \param data           Data buffer.
  \param timeout        Timeout.

  \retval           Succeed or fail.
*******************************************************************************
*/
int MS_ReadBytes(struct ms_info *ms_card, u8 TPC, u8 byte_count,
    unsigned char *data, int timeout)
{
    u32 rc;
    msinfo("\n");
    rc = MS_ReadBytes_Cmd(ms_card, TPC, byte_count, data, timeout);

    return rc;
}


/*
*******************************************************************************
*                      Set Register Address And Size
* FUNCTION MS_SetRWRegAdrs
*******************************************************************************
*/
/**
  This function sends command SET_RW_REG_ADRS to MS/MS Pro card internal
  controller to set start address and count of the register to registers which
  is used by command READ_REG and WRITE_REG.

  \param us         USB device.
  \param readstart      Start address of the register to be read.
  \param readcount      Size of the register to be read.
  \param writestart     Start address of the register to be written.
  \param writecount     Size of the register to be written.

  \retval           Succeed or fail.
*******************************************************************************
*/
int MS_SetRWRegAdrs(struct ms_info *ms_card,
    unsigned char readstart,  unsigned char readcount,
    unsigned char writestart, unsigned char writecount)
{
    unsigned char data[4], status;
    int rc;
    msinfo("R-star:%x,R-cnt:%x,W-star:%x,W-cnt:%x\n",
            readstart,readcount,writestart,writecount);
    data[0] = readstart;
    data[1] = readcount;
    data[2] = writestart;
    data[3] = writecount;

    rc = MS_WriteBytes(ms_card, MSPRO_TPC_SET_RW_REG_ADRS,
        0, 4, data, &status, USUAL_TIMEOUT);
    if (rc) {
        printk("MS_WriteBytes() fails, rc=%d\n", rc);
        return -1;
    }

    return 0;
}


static int mspro_SetRWCmd(struct ms_info *ms_card,
    unsigned int startsect, unsigned short sectcount, int op)
{
    int rc, i;
    unsigned char temp[7];
    msinfo("\n");
    if (op == READ_OP) {
        temp[0] = MSPRO_CMD_READ_DATA;
    } else {
        temp[0] = MSPRO_CMD_WRITE_DATA;
    }

    temp[1] = SHORT2CHAR(sectcount, 1);
    temp[2] = SHORT2CHAR(sectcount, 0);
    temp[3] = LONG2CHAR(startsect, 3);
    temp[4] = LONG2CHAR(startsect, 2);
    temp[5] = LONG2CHAR(startsect, 1);
    temp[6] = LONG2CHAR(startsect, 0);

    i = 0;
    while (i < MS_MAX_RETRY_COUNT) {
        i ++;
        rc = MS_WriteBytes(ms_card, MSPRO_TPC_EX_SET_CMD,
            0x80, 7, temp, NULL, PRO_READ_TIMEOUT);
        if ( !rc ){
            break;
        }
    }

    if ( rc ) {
        printk("[%s] MS_WriteBytes() fails\n", __FUNCTION__);
    }

    return rc;
}


/*
*******************************************************************************
*                   Read Multiple Sectors from MS Pro
* FUNCTION mspro_ReadMultipleSector
*******************************************************************************
*/
/**
  Read multiple sectors from MS Pro card to USB SIE through DMA.

  \param srb            SCSI command.
  \param us         USB device.

  \retval           Sector count returned.
*******************************************************************************
*/
int mspro_ReadMultipleSector(struct ms_info *ms_card, unsigned int startsect,
    unsigned short sectcount, void *data)
{
    unsigned char status;
    int rc;
    msinfo("\n");
    rc = mspro_SetRWCmd(ms_card, startsect, sectcount, READ_OP);
    if (rc) {
        printk("[%s] mspro_SetRWCmd() fails\n", __FUNCTION__);
        return -1;
    }

    rc = MS_AutoRead(ms_card, MSPRO_TPC_READ_LONG_DATA,
        sectcount, data, &status, 10000);
    if (rc) {
        printk("[%s] MS_AutoRead() fails\n", __FUNCTION__);
        return -1;
    }

    ms_card->MSProINT = status;
    ms_card->MSProPreOP = READ_OP;
    ms_card->MSProPreAddr = startsect;

    return sectcount;
}


/*
*******************************************************************************
*                   Write Multiple Sectors to MS Pro
* FUNCTION mspro_WriteMultipleSector
*******************************************************************************
*/
/**
  Write multiple sectors to MS Pro card from USB SIE through DMA.

  \param srb            SCSI command.
  \param us         USB device.

  \retval           Sector count returned.
*******************************************************************************
*/
/*
int mspro_WriteMultipleSector(struct ms_info *ms_card, unsigned int startsect,
    unsigned short sectcount, void *data)
{
    unsigned char status;
    int rc;
    msinfo("\n");
    rc = mspro_SetRWCmd(ms_card, startsect, sectcount, WRITE_OP);
    if (rc) {
        printk("[%s] mspro_SetRWCmd() fails\n", __FUNCTION__);
        return -1;
    }

    rc = MS_AutoWrite(ms_card, MSPRO_TPC_WRITE_LONG_DATA,
        sectcount, data, &status, 10000);
    if (rc) {
        printk("[%s] MS_AutoWrite() fails\n", __FUNCTION__);
        return -1;
    }

    ms_card->MSProINT = status;
    ms_card->MSProPreOP = WRITE_OP;
    ms_card->MSProPreAddr = startsect;

    return sectcount;
}
*/

/*
*******************************************************************************
*                   Read the Value of Status0/1 Register
* FUNCTION ms_ReadStatus0_1
*******************************************************************************
*/
/**
  Read the value of status 0/1 register. It is frequently used to check if
  the error is uncorrectable. It should be noted that status buffer must point
  to a two bytes long area at least.

  \param us         USB device.
  \param status         Status buffer.

  \retval           Success or fail.
*******************************************************************************
*/
/*
int MS_ReadStatus0_1(struct ms_info *ms_card, unsigned char *status)
{
    int rc;
    msinfo("\n");
    if (status == NULL) {
        return -1;
    }

    rc = MS_SetRWRegAdrs(ms_card,MS_REG_StatusReg0, 2, 0, 0);
    if ( rc ) {
        printk("MS_SetRWRegAdrs() StatusReg0 fails\n");
        return -1;
    }

    rc = MS_ReadBytes(ms_card, MS_TPC_READ_REG, 2, status, USUAL_TIMEOUT);
    if (rc) {
        printk("MS_ReadBytes() READ_REG fails\n");
        return -1;
    }

    //check StatusReg1
    if (status[1] & (STS_UCDT | STS_UCEX | STS_UCFG | EXTRA_ERROR)) {
        printk("status[1] fails\n");
        return -1;
    }

    return 0;
}
*/

/*
*******************************************************************************
*                        Parse Boot Block
* FUNCTION ms_ParseBootBlock
*******************************************************************************
*/
/**
  Read boot block from MS card, and parse it to get relevant parameters,
  including capacity, write protected or not, etc.

  \param us         USB device.
  \param blockno        Block number.
  \param totalsector        Total sector.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
static int ms_ParseBootBlock(struct ms_info *ms_card,
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
        rc = MS_ReadPage(ms_card, blockno, (unsigned char)i, data);
        if (rc ) {
            printk("[%d] MS_ReadPage() fails\n", __LINE__);
            kfree(data);
            return -1;
        }
    }

    rc = MS_ReadPage(ms_card, blockno, 0, data);
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
            ms_card->write_protect_flag = 1;
        }
        ASSIGN_SHORT(temp, data[BLOCK_SIZE_0], data[BLOCK_SIZE_1]);
        if (temp == 0x0010) {    // 16KB
            ms_card->MSBlockShift = 5;
            ms_card->MSPageOff = 0x1f;
        } else if (temp == 0x0008) {   // 8KB
            ms_card->MSBlockShift = 4;
            ms_card->MSPageOff = 0x0f;
        }
        ASSIGN_SHORT(ms_card->MSTotalBlock,
            data[BLOCK_COUNT_0], data[BLOCK_COUNT_1]);
        ASSIGN_SHORT(temp, data[EBLOCK_COUNT_0], data[EBLOCK_COUNT_1]);
        if (!ms_card->MS_4bit) {
            if (data[MS_4bit_Support] == 1) {
                rc = MS_SetRWRegAdrs(ms_card, 0, 0, MS_REG_SystemParm, 1);
                if (rc) {
                    printk("[%d] ms_SetRWRegAdrs() fails\n", __LINE__);
                    kfree(data);
                    return -1;
                }
                data[0] = 0x88;
                rc = MS_WriteBytes(ms_card, MS_TPC_WRITE_REG,
                    0, 1, data, NULL, USUAL_TIMEOUT);
                if (rc) {
                    printk("[%d] MS_WriteBytes() fails\n", __LINE__);
                    kfree(data);
                    return -1;
                }
                ms_card->MS_4bit = 1;
                rc = MS_SetParameter(ms_card);
                if (rc) {
                    printk("[%d] ms_SetParameter() fails\n", __LINE__);
                    kfree(data);
                    return -1;
                }
            }
        }
        capacity = ((unsigned int)temp-2) << ms_card->MSBlockShift;
    } else {
        printk("[%d] MS_ReadPage() fails\n", __LINE__);
        kfree(data);
        return -1;
    }

    ms_card->MSBootBlock = (unsigned char)blockno;
    if (totalsector) {
        *totalsector = capacity;
    }

    kfree(data);
    return 0;
}
*/

/*
*******************************************************************************
*                        MS Card Size
* FUNCTION MS_CardSize
*******************************************************************************
*/
/**
  Read boot block to get block size and total block count to calculate the
  number of total sectors. If the boot block of MS card is  defective we will
  use the backup boot block instead.

  \param us         USB device.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
int MS_CardSize(struct ms_info *ms_card)
{
    int rc, i = 0;
    unsigned char extra[MS_EXTRA_SIZE];
    unsigned int totalsector = 0;
    msinfo("\n");

    rc = MS_SetParameter(ms_card);
    if (rc){
        printk("MS_SetParameter() fails\n");
        return -1;
    }

    while (i < (MAX_DEFECTIVE_BLOCK + 2)) {
        rc = MS_ReadExtraData(ms_card, i, 0, extra);
        if (rc) {
            i++;
            continue;
        }
        if ((extra[0] & BLOCK_OK) && (!(extra[1] & NOT_BOOT_BLOCK))) {
            rc = ms_ParseBootBlock(ms_card, i, &totalsector);
            if ( !rc ) {
                break;
            } else if (rc) {
                i ++;
                continue;
            } else {
                return 0;
            }
        }
        i++;
    }

    if (i == (MAX_DEFECTIVE_BLOCK + 2)) {
        printk("MS_SetParameter() fails\n");
        return -1;
    }

    return totalsector;
}
*/

/*
*******************************************************************************
*                        MS Pro Card Size
* FUNCTION MS_CardSizePro
*******************************************************************************
*/
/**
  Read attribute information area to get block size and total block count, then
  calculate the number of total sectors according to them two.

  \param us         USB device.
  \param capacity       Pointer to capacity.

  \retval           Succeed or fail.
*******************************************************************************
*/
/*
int MSPRO_CardSize(struct ms_info *ms_card, unsigned int *capacity)
{
    int rc, i = 0;
    unsigned char temp[7] = {PARELLEL_IF, 0, 0x40, 0, 0, 0, 0x00};
    unsigned char *data, status, ClassCode, DeviceType, SubClass;
    unsigned short totalblock = 0, blocksize = 0;
    unsigned int totalsector = 0, SysInfoAddr, SysInfoSize;

    msinfo("\n");
    data = kmalloc(32 * 1024, GFP_KERNEL);
    if ( !data ) {
        printk("[%d] Allocation data fails\n", __LINE__);
        return -ENOMEM;
    }

    while (i < MS_MAX_RETRY_COUNT) {
        i ++;
        rc = MS_SetRWRegAdrs(ms_card, MSPRO_REG_INT, 2, MSPRO_REG_SystemParm, 7);
        if ( !rc ) {
            break;
        }
    }
    if (i < MS_MAX_RETRY_COUNT) {
        i = 0;
    } else {
        printk("[%s] MS_SetRWRegAdrs() fails\n", __FUNCTION__);
        kfree(data);
        return -1;
    }

    while (i < MS_MAX_RETRY_COUNT) {
        i ++;
        rc = MS_WriteBytes(ms_card, MSPRO_TPC_WRITE_REG, 0, 7,
            temp, &status, PRO_WRITE_TIMEOUT);
        if ( !rc ) {
            break;
        }
    }
    if (i < MS_MAX_RETRY_COUNT) {
        i = 0;
    } else {
        printk("MS_WriteBytes() MSPRO_TPC_WRITE_REG fails\n");
        kfree(data);
        return -1;
    }

    temp[0] = MSPRO_CMD_READ_ATRB;
    while (i < MS_MAX_RETRY_COUNT) {
        i ++;
        rc = MS_WriteBytes(ms_card, MSPRO_TPC_SET_CMD, WAIT_INT, 1,
            temp, &status, PRO_WRITE_TIMEOUT);
        if ( !rc ) {
            break;
        }
    }
    if (i < MS_MAX_RETRY_COUNT) {
        i = 0;
    } else {
        printk("MS_WriteBytes() MSPRO_TPC_SET_CMD fails\n");
        kfree(data);
        return -1;
    }

    if ( !(status & INT_BREQ) ){
        printk("[%d] INT_BREQ errors, status=%x\n", __LINE__, status);
        kfree(data);
        return -1;
    }

    while (i < MS_MAX_RETRY_COUNT) {
        i ++;
        rc = MS_AutoRead(ms_card, MSPRO_TPC_READ_LONG_DATA, 0x40,
            data, NULL, PRO_READ_TIMEOUT);
        if ( !rc ) {
            break;
        }
    }
    if (i < MS_MAX_RETRY_COUNT) {
        i = 0;
    } else {
        printk("MS_AutoRead() fails\n");
        kfree(data);
        return -1;
    }

    if ((data[0] != 0xa5) && (data[1] != 0xc3)) {
        printk("Signature code is wrong !! data[0]=%#x, data[1]=%#x\n",
            data[0], data[1]);
        kfree(data);
        return -1;
    }

    if ((data[4] < 1) || (data[4] > 12)) {
        printk("Device Informantion Entry Count errors!! data[4]=%d\n", data[4]);
        kfree(data);
        return -1;
    }

    for (i = 0; i < data[4]; i ++) {
        int CurrentAddrOff = 16 + i * 12;

        if (data[CurrentAddrOff + 8] == 0x10) {
            ASSIGN_LONG(SysInfoAddr, data[CurrentAddrOff+0],
                data[CurrentAddrOff+1], data[CurrentAddrOff+2],
                data[CurrentAddrOff+3]);
            ASSIGN_LONG(SysInfoSize, data[CurrentAddrOff+4],
                data[CurrentAddrOff+5], data[CurrentAddrOff+6],
                data[CurrentAddrOff+7]);
            if (SysInfoSize != 96)  {  // Size shall equal to 96
                printk("SysInfoSize=%d error\n", SysInfoSize);
                kfree(data);
                return -1;
            }
            if (SysInfoAddr < 0x1A0) {
                printk("SysInfoAddr=%d error\n", SysInfoAddr);
                kfree(data);
                return -1;
            }
            if ((SysInfoSize + SysInfoAddr) > 0x8000) {
                printk("SysInfoSize+SysInfoAddr=%d error\n", SysInfoSize+SysInfoAddr);
                // Address + Size can't be over than 0x8000
                kfree(data);
                return -1;
            }
            break;
        }
    }

    if (i == data[4]) {
        printk("Can not find System Information Entry error, data[4]=%d\n", data[4]);
        kfree(data);
        return -1;
    }

    ClassCode =  data[SysInfoAddr + 0];
    DeviceType = data[SysInfoAddr + 56];
    SubClass = data[SysInfoAddr + 46];
    ASSIGN_SHORT(totalblock, data[SysInfoAddr + 6], data[SysInfoAddr + 7]);
    ASSIGN_SHORT(blocksize, data[SysInfoAddr + 2], data[SysInfoAddr + 3]);
    totalsector = ((unsigned int)totalblock) * blocksize;

    kfree(data);

    if (ClassCode != 0x02) { // Class Code != 0x02
        printk("Error: NOT MSPRO\n");
        return -1;
    }
    if (DeviceType != 0x00) {
        if ((DeviceType == 0x01) || (DeviceType == 0x02)
            || (DeviceType == 0x03)) {
            ms_card->write_protect_flag = 1;
        } else {
            printk("DeviceType erroe\n");
            return -1;
        }
    }

    if (SubClass & 0xc0) {
        printk("SubClass erroe\n");
        return -1;
    }

    rc = ms_SetInitPara(ms_card);
    if (rc) {
        printk("ms_SetInitPara() fails\n");
        return -1;
    }

    if (capacity) {
        *capacity = totalsector;
    }

    return 0;
}
*/

/*
*******************************************************************************
*                       Set Init Parameter
* FUNCTION ms_SetInitPara
*******************************************************************************
*/
/**
  Switch SD/MMC card module to a proper clock rate, and give it a proper time
  parameter according to the card's type and bus width.

  \param us         USB device.

  \retval           Success or fail.
*******************************************************************************
*/
/*int ms_SetInitPara(struct ms_info *ms_card)
{
    int rc;
    msinfo("\n");
    rc = MS_SetParameter(ms_card);
    if ( rc ){
        printk("MS_SetParameter fails\n");
        return -1;
    }
    return 0;
}
*/

static inline void rtkms_sg_to_dma(struct rtkms_port *msport, struct ms_data *data)
{
    unsigned int len, i, size;
    struct scatterlist *sg;
    char *dmabuf = msport->dma_buffer;
    char *sgbuf;
    msinfo("\n");
    size = msport->size;

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

    msport->size -= size;
}


static inline void rtkms_dma_to_sg(struct rtkms_port *msport, struct ms_data *data)
{
    unsigned int len, i, size;
    struct scatterlist *sg;
    char *dmabuf = msport->dma_buffer;
    char *sgbuf;
    msinfo("\n");
    size = msport->size;

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
    msport->size -= size;
}


static void rtkms_request_end(struct rtkms_port *msport, struct ms_request *mrq)
{
    msinfo("\n");
    msport->mrq = NULL;

    up(&sem);

    ms_request_done(msport->ms, mrq);

    if (down_interruptible (&sem)) {
        printk("%s : user breaking\n",__FUNCTION__);
        return ;
    }
}


/*
 * Common routines
 */
/*
static void rtkms_init_device(struct rtkms_port *msport)
{
    int card_status;
    struct ms_host 	*host=msport->ms;
    msinfo("\n");
    //MS_POWER(OFF);
    //rtkms_host_power(OFF);

    card_status = rtkms_polling(host);
    if(card_status<0)
        return;

    if ( card_status & 0x08 ){
        rtk_ms_card_exist = 1;
    }else{
        rtk_ms_card_exist = 0;
    }

}
*/

/*****************************************************************************\
 *                                                                           *
 * MS layer callbacks                                                       *
 *                                                                           *
\*****************************************************************************/

static void rtkms_request(struct ms_host *host, struct ms_request *mrq)
{
    int rc;
    struct rtkms_port *msport = ms_priv(host);
    struct ms_data *data = 0;
    struct ms_card *card;
    unsigned char temp[8];
    unsigned char *data_buf = 0;
    msinfo("\n");
    /* try to grap the semaphore and see if the device is available */
    if (down_interruptible (&sem)) {
        printk("%s : user breaking\n",__FUNCTION__);
        return ;
    }

    BUG_ON(msport->mrq != NULL);
    if ( mrq->data ){
        msinfo("\n");
        data = mrq->data;
        msport->size = mrq->sector_cnt * 512;
    }
    msport->mrq = mrq;

    mrq->error = 0;

    if ( data && (data->flags&MS_DATA_WRITE) &&
        (mrq->tpc==MSPRO_TPC_WRITE_LONG_DATA) ){
        msinfo("\n");
        rtkms_sg_to_dma(msport, data);
#if SW_TRANSFORM_ENDIAN
        msinfo("SW_TRANSFORM_ENDIAN\n");
        transform_data_format(msport->dma_buffer, mrq->sector_cnt*512);
#endif
    }

    msinfo("mrq->tpc:0x%x  ",mrq->tpc);
    switch(mrq->tpc){
        case MSPRO_TPC_GET_INT:
            printk("MSPRO_TPC_GET_INT\n");
            rc =MS_ReadBytes(&(msport->ms_card), MSPRO_TPC_GET_INT,
                mrq->byte_count, mrq->reg_data, USUAL_TIMEOUT);
            break;

        case MSPRO_TPC_READ_REG:
            printk("MSPRO_TPC_READ_REG\n");
            rc =MS_ReadBytes(&(msport->ms_card), MSPRO_TPC_READ_REG,
                mrq->reg_rw_addr->r_length, mrq->reg_data, USUAL_TIMEOUT);
            if (mrq->reg_data[0] & WP_FLAG) {
                rtk_ms_wp= 1;
            } else {
                rtk_ms_wp = 0;
            }
            break;

        case MSPRO_TPC_WRITE_REG:
            printk("MSPRO_TPC_WRITE_REG\n");
            rc = MS_WriteBytes(&(msport->ms_card), MSPRO_TPC_WRITE_REG, mrq->option,
                 mrq->byte_count, mrq->reg_data, &(mrq->status), USUAL_TIMEOUT);
            break;

        case MSPRO_TPC_READ_LONG_DATA:
            printk("MSPRO_TPC_READ_LONG_DATA\n");

            if ( data && (data->flags&MS_DATA_READ) ){
                msinfo("\n");
                data_buf = msport->dma_buffer;
            }else{
                msinfo("\n");
                data_buf = mrq->long_data;
            }

            CR_FLUSH_CACHE((unsigned long) data_buf, mrq->sector_cnt*512);

            if ( host->card->card_type & CR_MS_PRO ){
                msinfo("\n");
                rc = MS_AutoRead(&(msport->ms_card), MSPRO_TPC_READ_LONG_DATA, mrq->sector_cnt,
                    data_buf, &(mrq->status), mrq->timeout);
            }else if( host->card->card_type & CR_MS ){
                msinfo("No support MS-PRO\n");
                BUG_ON(1);
                rc = MS_NormalRead(&(msport->ms_card), MS_TPC_READ_PAGE_DATA, mrq->option,
                    data_buf, &(mrq->status), mrq->timeout);
            }else{
                msinfo("unknow Card Type\n");
                BUG_ON(1);
            }

#if SW_TRANSFORM_ENDIAN
            msinfo("SW_TRANSFORM_ENDIAN\n");
            if ( data && (data->flags&MS_DATA_READ) ){
                msinfo("\n");
                transform_data_format(data_buf, mrq->sector_cnt*512);
            }else{
                msinfo("\n");
                transform_data_format(data_buf, 16+400+96+48);
                rtk_cr_ms_dump_data (data_buf, 0, 16+400+96+48, (__u8 *)__FUNCTION__, __LINE__, AFTER_CALL);
                transform_data_format(data_buf+656, 16);
                rtk_cr_ms_dump_data (data_buf, 656, 16, (__u8 *)__FUNCTION__, __LINE__, AFTER_CALL);
            }
#endif
            break;

        case MSPRO_TPC_WRITE_LONG_DATA:
            printk("MSPRO_TPC_WRITE_LONG_DATA\n");
            if ( rtk_ms_wp )
                return ;

            CR_FLUSH_CACHE((unsigned long) msport->dma_buffer, mrq->sector_cnt*512);

            rc = MS_AutoWrite(&(msport->ms_card), MSPRO_TPC_WRITE_LONG_DATA, mrq->sector_cnt,
                msport->dma_buffer, &(mrq->status), mrq->timeout);
            break;

        case MSPRO_TPC_SET_CMD:
            printk("MSPRO_TPC_SET_CMD\n");
            rc = MS_WriteBytes(&(msport->ms_card), MSPRO_TPC_SET_CMD,
                mrq->option, mrq->byte_count, mrq->reg_data, &(mrq->status), USUAL_TIMEOUT);
            break;

        case MSPRO_TPC_SET_RW_REG_ADRS:
            printk("MSPRO_TPC_SET_RW_REG_ADRS\n");
            rc =MS_SetRWRegAdrs(&(msport->ms_card), mrq->reg_rw_addr->r_offset, mrq->reg_rw_addr->r_length,
                mrq->reg_rw_addr->w_offset, mrq->reg_rw_addr->w_length);
            break;

        default:
            printk("DO NOT support TPC %x !!\n", mrq->tpc);
    }

    if (rc){
        mrq->error = rc;
    }


    if ( cr_readl(CR_INT_STATUS) & 0x04 ){
        printk("[%s] command parser error!!, mrq->tpc=%#x\n", __FUNCTION__, mrq->tpc);
        cr_writel( cr_readl(CR_INT_STATUS)|0x04, CR_INT_STATUS );
        mrq->error = -2;
    }

    if ( data && (data->flags&MS_DATA_READ) &&
        (mrq->tpc==MSPRO_TPC_READ_LONG_DATA) ){
        msinfo("\n");
        rtkms_dma_to_sg(msport, data);
        data->bytes_xfered = msport->size;
    }

    if ( data && (data->flags&MS_DATA_WRITE) &&
        (mrq->tpc==MSPRO_TPC_WRITE_LONG_DATA) ){
        msinfo("\n");
        data->bytes_xfered = msport->size;
    }

done:
    msinfo("\n");
    rtkms_request_end(msport, mrq);
    up(&sem);
}

void rtkms_set_clk(struct ms_host *host,u8 ms_clk)   //0=20MHz, 1=24, 2=30, 3=40, 4=48, 5=60, 6=80, 7=96
{
    cr_writel(0x80,     CR_REG_CMD_1);
    cr_writel(ms_clk,   CR_REG_CMD_2);  //0=20MHz, 1=24, 2=30, 3=40, 4=48, 5=60, 6=80, 7=96

    cr_writel(0x01, CR_REG_MODE_CTR);

    wait_opt_end(host,INN_CMD);
}


static void rtkms_set_ios(struct ms_host *host, struct ms_ios *ios)
{
    struct rtkms_port *msport = ms_priv(host);
    int rc = 0;
    msinfo("\n");
    spin_lock_bh(&msport->rtlock);

    if (ios->bus_width == MS_BUS_WIDTH_4){
        if((msport->rtflags & RTKMS_BUS_BIT4)==0){
            printk("MS-PRO:PARALLEL mode\n");
            rtkms_set_para(host,MS_4_BIT_MODE|SAMPLE_RISING|PUSH_TIME);
            msport->rtflags|=RTKMS_BUS_BIT4;
        }
    }

    if (ios->bus_mode == MS_BUSMODE_PUSHPULL){
        msinfo("\n");
        if(msport->act_host_clk!=CARD_SWITCHCLOCK_96MHZ){
            msinfo("\n");
            rtkms_set_clk(host,CARD_SWITCHCLOCK_96MHZ);
            msport->act_host_clk=CARD_SWITCHCLOCK_96MHZ;
        }
    }else{  //MS_BUSMODE_OPENDRAIN
        msinfo("\n");
        if(msport->act_host_clk!=CARD_SWITCHCLOCK_30MHZ){
            msinfo("\n");
            rtkms_set_clk(host,CARD_SWITCHCLOCK_30MHZ);  //default clock
            msport->act_host_clk=CARD_SWITCHCLOCK_30MHZ;
        }
    }

    if (ios->power_mode == MS_POWER_OFF){
        msinfo("\n");
        if(msport->rtflags & RTKMS_FHOST_POWER){
            msinfo("\n");
            rtkms_host_power(OFF);
            msport->rtflags &= ~RTKMS_FHOST_POWER;
        }
        if(msport->rtflags & RTKMS_FCARD_POWER){
            msinfo("\n");
            rtkms_card_power(OFF);
            msport->rtflags &= ~RTKMS_FCARD_POWER;
        }
    }else{  //MS_POWER_UP or MS_POWER_NO
        /* have any setting for power up mode ? */
        msinfo("\n");

        if(!(msport->rtflags & RTKMS_FHOST_POWER)){
            msinfo("\n");
            rtkms_host_power(ON);
            msport->rtflags |= RTKMS_FHOST_POWER;
        }
        if(!(msport->rtflags & RTKMS_FCARD_POWER)){
            msinfo("\n");
            rtkms_card_power(ON);
            msport->rtflags |= RTKMS_FCARD_POWER;
        }
    }

    spin_unlock_bh(&msport->rtlock);
    msinfo("\n");
}


static struct ms_host_ops rtkms_ops = {
    .request    = rtkms_request,
    .set_ios    = rtkms_set_ios,
};

static void chk_card_insert(struct rtkms_port *msport)
{
    int reginfo;
    struct ms_host *host=msport->ms;

    reginfo = cr_readl(CR_INT_STATUS);
    msinfo("\n");
    if(reginfo & 0x10){
        reginfo = rtkms_polling(host);
        if(reginfo<0){
            goto non_card;
        }

        msinfo("polling status:0x%x\n",reginfo);
        if ( reginfo & MS_CARD_EXIST ){
            msinfo("card detect~\n");
            msport->rtflags |= RTKMS_FCARD_DETECTED;

        }else{
            msinfo("no card inside~\n");
            goto non_card;
        }
    }else{
non_card:
        msinfo("no card detect~\n");
        msport->rtflags &= ~RTKMS_FCARD_DETECTED;
    }
}


/*****************************************************************************\
 *                                                                           *
 * Interrupt handling                                                        *
 *                                                                           *
\*****************************************************************************/

/*
 * Tasklets
 */
/*
inline static struct ms_data* rtkms_get_data(struct rtkms_port* msport)
{
    WARN_ON(!msport->mrq);
    msinfo("\n");
    if (!msport->mrq)
        return NULL;
    return msport->mrq->data;
}
*/
/*
 * note: same with mmc.
 */
void rtk_check_cr_param(u32 *pparam, u32 len, u8 *ptr)
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

/* get card power GPIO address and control bit
 *
 *
 * note: same with mmc.
 */
int rtkms_get_gpio(void)
{
#ifndef FPGA_BOARD
    unsigned char *cr_param;
    void __iomem *mmio;

    cr_param=(char *)parse_token(platform_info.system_parameters,"cr_pw");
    if(cr_param){
        rtk_check_cr_param(&rtkms_power_gpio,4,cr_param);
        rtk_check_cr_param(&rtkms_power_bits,1,cr_param+5);

        mmio = (void __iomem *) (rtkms_power_gpio+0xb8010000);
        writel(readl(mmio) | (1<<rtkms_power_bits),mmio); //enable GPIO output

        if((rtkms_power_gpio & 0xf00) ==0x100){
            rtkms_power_gpio+=0xb8010010;
        }else if((rtkms_power_gpio & 0xf00) ==0xd00){
            rtkms_power_gpio+=0xb8010004;
        }else{
            printk(KERN_ERR "wrong GPIO of card's power.\n");
            return -1;
        }
        msinfo("power_gpio = 0x%x\n",rtkms_power_gpio);
        msinfo("power_bits = %d\n",rtkms_power_bits);
        return 0;
    }
    printk(KERN_ERR "Can't find GPIO of card's power.\n");
    return -1;
#else
    return 0;
#endif

}

/*
 * Allocate/free MS structure.
 */
static int __devinit rtkms_init_ms(struct device *dev)
{
    struct ms_host *host;
    struct rtkms_port *msport;

    msinfo("\n");
    host = ms_alloc_host(sizeof(struct rtkms_port), dev);
    if (!host)
        return -ENOMEM;

    msport = ms_priv(host);
    msport->ms = host;
    msport->rtlock=&host->lock;

    host->ops = &rtkms_ops;
    host->f_min = 375000;
    host->f_max = 24000000;
    host->ocr_avail = MS_VDD_32_33|MS_VDD_33_34;

    sema_init(&host->ms_sem, 1);
    spin_lock_init(&msport->rtlock);

    host->max_hw_segs = 1;
    host->max_phys_segs = 1;
    host->max_sectors = 128;
    host->max_seg_size = host->max_sectors * 512;

    /* interrupt setting  mark first ****/
    //if (request_irq(4, rtk_cr_interrupt, SA_SHIRQ, DRIVER_NAME, host)){
    //    msinfo("IRQ request fail\n");
    //    return -1;
    //}
    //INIT_WORK(&rtk_cr_work, rtk_cr_work_fn, host);
    //
    //cr_writel(0xff, CR_INT_STATUS); /* clear int status */
    //cr_writel(0x01, CR_INT_MASK);   /* enable interrupt */
    /* interrupt setting  mark first &&&*/
    dev_set_drvdata(dev, host);

    return 0;
}

static void __devexit rtkms_free_ms(struct device* dev)
{
    struct ms_host* host;
    msinfo("\n");
    host = dev_get_drvdata(dev);
    if (!host)
        return;

    ms_free_host(host);

    dev_set_drvdata(dev, NULL);
}

static int  __devexit rtkms_device_remove(struct device* dev)
{
    struct ms_host *host = dev_get_drvdata(dev);
    struct rtkms_port *msport;
    msinfo("\n");
    if ( !host )
        return -1;

    msport = ms_priv(host);

    ms_remove_host(host);

    if ( msport->dma_buffer )
        kfree(msport->dma_buffer);

    rtkms_free_ms(dev);
    return 0;
}


static int rtkms_request_dma(struct rtkms_port *msport)
{
    msport->dma_buffer = (unsigned char *)kmalloc(DMA_BUFFER_SIZE, GFP_KERNEL);
    if ( !msport->dma_buffer ){
        printk(KERN_ERR "Allocate host->dma_buffer fails\n");
        return -ENOMEM;
    }
    return 0;
}


static int __devinit rtkms_device_probe(struct device *dev)
{
    struct rtkms_port *msport = NULL;
    struct ms_host *host = NULL;
    int rc = 0;

    msinfo("\n");
    rc = rtkms_init_ms(dev);
    if (rc){
        printk(KERN_WARNING "MS controller initial fails\n");
        goto error_out;
    }

    msinfo("\n");
    if ( (host = dev_get_drvdata(dev)) == NULL ){
        printk("[%s] dev_get_drvdata() fails\n", __FUNCTION__);
        rtkms_free_ms(dev);
        goto error_out;
    }
    msinfo("\n");
    if ( (msport = ms_priv(host)) == NULL ){
        printk("[%s] ms_priv() fails\n", __FUNCTION__);
        rtkms_free_ms(dev);
        goto error_out;
    }

    msinfo("\n");
    chk_card_insert(msport);

    /*
     * Allocate DMA.
     */
    rc = rtkms_request_dma(msport);
    if (rc){
        printk(KERN_WARNING "rtkms_request_dma() fails\n");
        goto error_out;
    }

    if(msport->rtflags & RTKMS_FCARD_DETECTED){
        msinfo("\n");
        ms_add_host(host);
    }

    return 0;
error_out:
    return -1;
}


/*
 * Power management
 */
/*
#ifdef CONFIG_PM
static int rtkms_suspend(struct device *dev, pm_message_t state, u32 level)
{
    printk("[%s] Not yet supported\n", __FUNCTION__);
    return 0;
}

static int rtkms_resume(struct device *dev, u32 level)
{
    printk("[%s] Not yet supported\n", __FUNCTION__);
    return 0;
}
#else
#define rtkms_suspend NULL
#define rtkms_resume NULL
#endif
*/

static void display_version (void)
{
    const __u8 *revision;
    const __u8 *date;
    char *running = (__u8 *)VERSION;

    strsep(&running, " ");
    strsep(&running, " ");
    revision = strsep(&running, " ");
    date = strsep(&running, " ");
    printk(BANNER " Rev:%s (%s)\n", revision, date);
}

#ifdef CONFIG_PM
static int rtkms_suspend(struct device *dev, pm_message_t state, u32 level)
{
    printk("%s Not yet supported\n", __FUNCTION__);

    return 0;
}

static int rtkms_resume(struct device *dev, u32 level)
{
    printk("%s Not yet supported\n", __FUNCTION__);

    return 0;
}
#else
#define rtkms_suspend NULL
#define rtkms_resume NULL
#endif


static struct device_driver rtkms_driver = {
    .name       = DRIVER_NAME,
    .bus        = &platform_bus_type,
    .probe      = rtkms_device_probe,
    .remove     = rtkms_device_remove,
#ifdef CONFIG_PM
    .suspend    = rtkms_suspend,
    .resume     = rtkms_resume,
#endif
};

static int __init rtkms_init(void)
{
    int rc = 0;

    display_version();
    rc = rtkms_get_gpio();
    if (rc < 0)
        goto POWER_ERR;

    rtkms_card_power(OFF);
    rtkms_host_power(OFF);
    mdelay(100);

    rc = driver_register(&rtkms_driver);
    if (rc < 0)
        goto EXIT;

    rtkms_device = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);

    if ( IS_ERR(rtkms_device) ){
        rc = PTR_ERR(rtkms_device);
        goto EXIT;
    }


EXIT:
    if (rc < 0){
        platform_device_unregister(rtkms_device);
        driver_unregister(&rtkms_driver);
POWER_ERR:
        printk(KERN_INFO "Realtek MS/MSPRO Controller Driver installation fails.\n\n");
        return -ENODEV;
    }else{
        printk(KERN_INFO "Realtek MS/MSPRO Controller Driver is successfully installing.\n\n");
        return 0;
    }
}


static void __exit rtkms_exit(void)
{
    platform_device_unregister(rtkms_device);
    driver_unregister(&rtkms_driver);
    rtkms_host_power(OFF);
}

module_init(rtkms_init);
module_exit(rtkms_exit);

MODULE_AUTHOR("CMYu<Ken.Yu@realtek.com>");
MODULE_DESCRIPTION("Realtek MS/MSPRO Controller Driver");
