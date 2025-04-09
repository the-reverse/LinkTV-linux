/* ------------------------------------------------------------------------- */
/* i2c-venus.c i2c-hw access for Realtek Venus DVR I2C                       */
/* ------------------------------------------------------------------------- */
/*   Copyright (C) 2005 Chih-pin Wu <wucp@realtek.com.tw>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    Version 1.0 written by wucp@realtek.com.tw
    Version 2.0 modified by Frank Ting(frank.ting@realtek.com.tw)(2007/06/21)   
-------------------------------------------------------------------------     
    1.4     |   20081016    | Multiple I2C Support
-------------------------------------------------------------------------     
    1.5     |   20090423    | Add Suspen/Resume Feature    
-------------------------------------------------------------------------*/    
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/pci.h>
#include <linux/wait.h>
#include <linux/i2c.h>
#include <linux/platform.h>
#include <linux/list.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
#include <asm/mach-venus/platform.h>
#include "../algos/i2c-algo-venus-k.h"
#include "i2c-venus-priv.h"


static int i2c_speed = SPD_MODE_SS;
module_param(i2c_speed, int, S_IRUGO);

#define VENUS_I2C_IRQ           3
#define VENUS_MASTER_7BIT_ADDR  0x24


typedef struct 
{
    struct i2c_adapter  adap;
    i2c_venus_algo      algo;
    struct list_head    list;
    
#ifdef CONFIG_PM 
    struct platform_device* p_dev;
#endif
    
}venus_i2c_adapter;


LIST_HEAD(venus_i2c_list);


////////////////////////////////////////////////////////////////////

#ifdef CONFIG_I2C_DEBUG_BUS
#define i2c_debug(fmt, args...)     printk(KERN_INFO fmt, ## args)
#else
#define i2c_debug(fmt, args...)
#endif

/*------------------------------------------------------------------
 * Func : i2c_venus_xfer
 *
 * Desc : start i2c xfer (read/write)
 *
 * Parm : p_msg : i2c messages 
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
void i2c_venus_dump_msg(const struct i2c_msg* p_msg)
{
    printk("msg->addr  = %02x\n",p_msg->addr);
    printk("msg->flags = %04x\n",p_msg->flags);
    printk("msg->len   = %d  \n",p_msg->len);
    printk("msg->buf   = %p  \n",p_msg->buf);    
}


#define IsReadMsg(x)        (x.flags & I2C_M_RD)
#define IsGPIORW(x)         (x.flags & I2C_GPIO_RW)
#define IsSameTarget(x,y)   ((x.addr == y.addr) && !((x.flags ^ y.flags) & (~I2C_M_RD)))

/*------------------------------------------------------------------
 * Func : i2c_venus_xfer
 *
 * Desc : start i2c xfer (read/write)
 *
 * Parm : adapter : i2c adapter
 *        msgs    : i2c messages
 *        num     : nessage counter
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
static int 
i2c_venus_xfer(
    void*                   dev_id, 
    struct i2c_msg*         msgs, 
    int                     num
    )
{
    venus_i2c* p_this = (venus_i2c*) dev_id;
    int ret = 0;
    int i;
        
    for (i = 0; i < num; i++) 
    {                    
        ret = p_this->set_tar(p_this, msgs[i].addr, ADDR_MODE_7BITS);
        
        if (ret<0)
            goto err_occur;
        
        if (IsReadMsg(msgs[i]))            
        {                   
            if (IsGPIORW(msgs[i]))
                ret = p_this->gpio_read(p_this, NULL, 0, msgs[i].buf, msgs[i].len);
            else
                ret = p_this->read(p_this, NULL, 0, msgs[i].buf, msgs[i].len);
        }                            
        else
        {             
            if ((i < (num-1)) && IsReadMsg(msgs[i+1]) && IsSameTarget(msgs[i], msgs[i+1]))
            {
                // Random Read = Write + Read (same addr)                
                if (IsGPIORW(msgs[i]))
                    ret = p_this->gpio_read(p_this, msgs[i].buf, msgs[i].len, msgs[i+1].buf, msgs[i+1].len);                        
                else                    
                    ret = p_this->read(p_this, msgs[i].buf, msgs[i].len, msgs[i+1].buf, msgs[i+1].len);                        
                i++;
            }
            else
            {   
                // Single Write
                if (IsGPIORW(msgs[i]))
                    ret = p_this->gpio_write(p_this, msgs[i].buf, msgs[i].len, (i==(num-1)) ? WAIT_STOP : NON_STOP);
                else                    
                    ret = p_this->write(p_this, msgs[i].buf, msgs[i].len, (i==(num-1)) ? WAIT_STOP : NON_STOP);
            }            
        }
        
        if (ret < 0)        
            goto err_occur;                          
    }     

    return i;
    
////////////////////    
err_occur:        
    
    if (ret==-ETXABORT && (msgs[i].flags & I2C_M_NO_ABORT_MSG))    
        return -EACCES; 
        
    printk("-----------------------------------------\n");        
          
    switch(ret)
    {
    case -ECMDSPLIT:
        printk("[I2C%d] Xfer fail - MSG SPLIT (%d/%d)\n", p_this->id, i,num);
        break;                                            
                                        
    case -ETXABORT:             
            
        printk("[I2C%d] Xfer fail - TXABORT (%d/%d), Reason=%04x\n",p_this->id, i,num, p_this->get_tx_abort_reason(p_this));        
        break;                  
                                
    case -ETIMEOUT:              
        printk("[I2C%d] Xfer fail - TIMEOUT (%d/%d)\n", p_this->id, i,num);        
        break;                  
                                
    case -EILLEGALMSG:           
        printk("[I2C%d] Xfer fail - ILLEGAL MSG (%d/%d)\n",p_this->id, i,num);
        break;
    
    case -EADDROVERRANGE:    
        printk("[I2C%d] Xfer fail - ADDRESS OUT OF RANGE (%d/%d)\n",p_this->id, i,num);
        break;
        
    default:        
        printk("[I2C%d] Xfer fail - Unkonwn Return Value (%d/%d)\n", p_this->id, i,num);
        break;
    }    
    
    i2c_venus_dump_msg(&msgs[i]);
    
    printk("-----------------------------------------\n");        
        
    ret = -EACCES;        
    return ret;
}



/*------------------------------------------------------------------
 * Func : i2c_venus_set_speed
 *
 * Desc : set speed of venus i2c
 *
 * Parm : dev_id : i2c adapter
 *        KHz    : speed of i2c adapter 
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
static int 
i2c_venus_set_speed(
    void*                   dev_id, 
    int                     KHz
    )
{
    venus_i2c* p_this = (venus_i2c*) dev_id;            
    
    if (p_this)
        return p_this->set_spd(p_this, KHz);
            
    return -1;          
}

    

//////////////////////////////////////////////////////////////////////////////////////////////
// Platform Device Interface
//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef CONFIG_PM

#define VENUS_I2C_NAME          "Venus_I2C"

static int venus_i2c_probe(struct device *dev)
{
    struct platform_device  *pdev = to_platform_device(dev);
    return strncmp(pdev->name,VENUS_I2C_NAME, strlen(VENUS_I2C_NAME));
}


static int venus_i2c_remove(struct device *dev)
{
    // we don't need to do anything for it...
    return 0;
}
 
static void venus_i2c_shutdown(struct device *dev)
{
    // we don't need to do anything for it...    
}

static int venus_i2c_suspend(struct device *dev, pm_message_t state, u32 level)
{
    struct platform_device* pdev   = to_platform_device(dev);  
    venus_i2c*              p_this = (venus_i2c*)  pdev->id;       
    
    if (p_this && level==SUSPEND_POWER_DOWN)
        p_this->suspend(p_this);                    
        
    return 0;    
}


static int venus_i2c_resume(struct device *dev, u32 level)
{    
    struct platform_device* pdev   = to_platform_device(dev);  
    venus_i2c*              p_this = (venus_i2c*) pdev->id;       
    
    if (p_this && level==RESUME_POWER_ON)
        p_this->resume(p_this);
        
    return 0;    
}


static struct device_driver venus_i2c_drv = 
{
    .name     = VENUS_I2C_NAME,
    .bus      = &platform_bus_type, 
    .probe    = venus_i2c_probe,
    .remove   = venus_i2c_remove,
    .shutdown = venus_i2c_shutdown,
    .suspend  = venus_i2c_suspend,
    .resume   = venus_i2c_resume,
};

#endif

//////////////////////////////////////////////////////////////////////////////////////////////
// Device Attribute
//////////////////////////////////////////////////////////////////////////////////////////////




static int venus_i2c_show_speed(struct device *dev, struct device_attribute *attr, char* buf)
{    
    struct i2c_adapter* p_adap = dev_to_i2c_adapter(dev);
    i2c_venus_algo*     p_algo = (i2c_venus_algo*) p_adap->algo_data;
    venus_i2c*          p_this = (venus_i2c*) p_algo->dev_id;       
        
    if (p_this)    
        return sprintf(buf, "%d\n", p_this->spd);    
                    
    return 0;    
}



static int venus_i2c_store_speed(struct device *dev, struct device_attribute *attr, char* buf,  size_t count)
{    
    struct i2c_adapter* p_adap = dev_to_i2c_adapter(dev);
    i2c_venus_algo*     p_algo = (i2c_venus_algo*) p_adap->algo_data;
    venus_i2c*          p_this = (venus_i2c*) p_algo->dev_id;       
    int                 spd;
                
    if (p_this && sscanf(buf,"%d\n", &spd)==1)     
        p_algo->set_speed(p_this, spd);
        
    return count;    
}


DEVICE_ATTR(speed, S_IRUGO | S_IWUGO, venus_i2c_show_speed, venus_i2c_store_speed);



//////////////////////////////////////////////////////////////////////////////////////////////
// Module
//////////////////////////////////////////////////////////////////////////////////////////////



/*------------------------------------------------------------------
 * Func : i2c_venus_register_adapter
 *
 * Desc : register i2c_adapeter 
 *
 * Parm : p_phy       : pointer of i2c phy
 *         
 * Retn : 0 : success, others failed
 *------------------------------------------------------------------*/  
static int 
__init i2c_venus_register_adapter(
    venus_i2c*              p_phy
    ) 
{        
    venus_i2c_adapter *p_adp = kmalloc(sizeof(venus_i2c_adapter), GFP_KERNEL);
    
    if (p_adp)
    {
        memset(p_adp, 0, sizeof(venus_i2c_adapter));               
        
        // init i2c_adapter data structure
        p_adp->adap.owner      = THIS_MODULE;
        p_adp->adap.class      = I2C_CLASS_HWMON;
        p_adp->adap.id         = 0x00; 
    	sprintf(p_adp->adap.name, "%s I2C %d bus", p_phy->model_name, p_phy->id);	
        p_adp->adap.algo_data  = &p_adp->algo;
        
        // init i2c_algo data structure    
        p_adp->algo.dev_id     = (void*) p_phy;
        p_adp->algo.masterXfer = i2c_venus_xfer;
        p_adp->algo.set_speed  = i2c_venus_set_speed;
        
#ifdef CONFIG_PM
        p_adp->p_dev = platform_device_register_simple(VENUS_I2C_NAME, (unsigned int) p_phy, NULL, 0);       
#endif
           
        // add bus
        i2c_venus_add_bus(&p_adp->adap);
        
        // add to list
        INIT_LIST_HEAD(&p_adp->list);
        
        list_add_tail(&p_adp->list, &venus_i2c_list);
        
        device_create_file(&p_adp->adap.dev, &dev_attr_speed);                  
    }
    
    return (p_adp) ? 0 : -1;
}


/*------------------------------------------------------------------
 * Func : i2c_venus_unregister_adapter
 *
 * Desc : remove venus i2c adapeter 
 *
 * Parm : p_adap      : venus_i2c_adapter data structure
 *         
 * Retn : 0 : success, others failed
 *------------------------------------------------------------------*/  
static 
void i2c_venus_unregister_adapter(
    venus_i2c_adapter*      p_adap
    )
{
    i2c_venus_algo* p_algo = NULL;
    
    if (p_adap)
    {
        list_del(&p_adap->list);                                        // remove if from the list
        
        p_algo = (i2c_venus_algo*) p_adap->adap.algo_data;
    
        device_remove_file(&p_adap->adap.dev, &dev_attr_speed);         // remove device attribute
    
        i2c_venus_del_bus(&p_adap->adap);                               // remove i2c bus
    
        destroy_venus_i2c_handle((venus_i2c*)p_algo->dev_id);        
        
#ifdef CONFIG_PM         
        if (p_adap->p_dev)
            platform_device_unregister(p_adap->p_dev);  
#endif        
        kfree(p_adap);
    }
}




#ifdef CONFIG_I2C_VENUS_GPIO_I2C


/*------------------------------------------------------------------
 * Func : get_g2c_gpio_config
 *
 * Desc : get config for GPIO i2c
 *
 * Parm : N/A
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/ 
static int __init get_g2c_gpio_config()
{
    int sda = CONFIG_I2C_VENUS_GPIO_I2C0_SDA;
    int scl = CONFIG_I2C_VENUS_GPIO_I2C0_SCL;
    
    char* param= parse_token(platform_info.system_parameters, "G2C0_GPIO");
    
    if (param>0)
    { 
        if (sscanf(param, "%u:%u", &sda, &scl)!=2)
        {
            printk("get_g2c_gpio_config failed, unknown G2C0_GPIO parameter %s\n", param);
            return 0xffff;          
        }
    }
    
    return ((sda << 8) | scl);
}

#endif



/*------------------------------------------------------------------
 * Func : i2c_venus_module_init
 *
 * Desc : init venus i2c module
 *
 * Parm : N/A
 *         
 * Retn : 0 success, otherwise fail
 *------------------------------------------------------------------*/  
static int 
__init i2c_venus_module_init(void) 
{        
    int i;
    venus_i2c* p_this;
    
    for (i=0; i<MAX_I2C_CNT; i++)
    {                            
        p_this = create_venus_i2c_handle(i, VENUS_MASTER_7BIT_ADDR, 
                    ADDR_MODE_7BITS, SPD_MODE_SS, VENUS_I2C_IRQ);

        if (p_this==NULL)
            continue;

        if (p_this->init(p_this)<0 || i2c_venus_register_adapter(p_this)<0)
            destroy_venus_i2c_handle(p_this);
    }

#ifdef CONFIG_I2C_VENUS_GPIO_I2C0
    
    i=get_g2c_gpio_config();
 
    if (i!=0xffff)
    {
        p_this = create_venus_g2c_handle(MAX_I2C_CNT, (i >>8) & 0xFF, i & 0xFF);
        
        if (p_this)
        {
            if (p_this->init(p_this)<0 || i2c_venus_register_adapter(p_this)<0)
                destroy_venus_i2c_handle(p_this);                        
        }
    }
    else
    {
        printk("[G2C] Open GPIO I2C failed, no valid GPIO specified\n");
    }
#endif

    
#ifdef CONFIG_PM        
    driver_register(&venus_i2c_drv);
#endif                        

    return 0;
}



/*------------------------------------------------------------------
 * Func : i2c_venus_module_exit
 *
 * Desc : exit venus i2c module
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/  
static void 
__exit i2c_venus_module_exit(void)
{   
    venus_i2c_adapter* cur = NULL;
    venus_i2c_adapter* next = NULL;
    
    list_for_each_entry_safe(cur, next, &venus_i2c_list, list)
    {
        i2c_venus_unregister_adapter(cur);
    }     

#ifdef CONFIG_PM            
    driver_unregister(&venus_i2c_drv);    
#endif           
}



MODULE_AUTHOR("Kevin-Wang <kevin_wang@realtek.com.tw>");
MODULE_DESCRIPTION("I2C-Bus adapter routines for Realtek Venus/Neptune DVR");
MODULE_LICENSE("GPL");

module_init(i2c_venus_module_init);
module_exit(i2c_venus_module_exit);

