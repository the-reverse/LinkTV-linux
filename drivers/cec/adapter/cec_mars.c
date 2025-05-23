/* ------------------------------------------------------------------------- 
   cec_mars.c  cec driver for Realtek Neptune/Mars           
   ------------------------------------------------------------------------- 
    Copyright (C) 2008 Kevin Wang <kevin_wang@realtek.com.tw>

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
----------------------------------------------------------------------------
Update List :
----------------------------------------------------------------------------
    1.0     |   20080911    |   Kevin Wang  | 1) create phase
----------------------------------------------------------------------------
    1.0a    |   20080917    |   Kevin Wang  | 1) porting to kernel land
----------------------------------------------------------------------------
    1.0b    |   20080918    |   Kevin Wang 
----------------------------------------------------------------------------    
    1) change the prototype of mar_cec_rcv_msg / mars_cec_send_message    
    2) fix TX control logic of CEC
    3) change TX to interrupt mode
    4) terminate all blocked tx/rx thread when chip is disabled
-----------------------------------------------------------------------------
    1.0c    |   20080925    |   Kevin Wang  | 1) Fix CEC timing
-----------------------------------------------------------------------------
    2.0     |   20081208    |   Kevin Wang  | 1) Using cmb architecture
-----------------------------------------------------------------------------
    2.1     |   20081217    |   Kevin Wang  | 1) Add Blocking Write
-----------------------------------------------------------------------------
    2.2     |   20081218    |   Kevin Wang  | 1) Using Work Queue to instead the timer
----------------------------------------------------------------------------*/
#include <linux/kernel.h>
#include <linux/config.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <asm/mach-venus/platform.h>
#include <asm/io.h>
#include "cec_mars.h"
#include "cec_mars_reg.h"
#include "cec_jupiter_reg.h"
#include "gcec_fixup.h"

static mars_cec m_cec;

#define TX_TIMEOUT      (HZ<<2)
//#define FPGA_MODE             // for fpga debug

#ifdef FPGA_MODE
//#define PRE_DIV         0x28
#define PRE_DIV         0x2A
#define RX_START0       0x8C
#define RX_START1       0xBC
#define TX_START0       0x94
#define TX_START1       0x20
//#define TX_DATA0        0x18
//#define TX_DATA1        0x24
#define TX_DATA0        0x1A
#define TX_DATA1        0x22    
#define TX_DATA2        0x24
#else
#define PRE_DIV         0x22
//#define RX_DATA1        0x52
#define RX_DATA1        0x50
//#define RX_START0       0x8C
//#define RX_START1       0xBF
#define RX_START0       0x8A
#define RX_START1       0xC1
#define TX_START0       0x94
#define TX_START1       0x20
#define TX_DATA0        0x18
#define TX_DATA1        0x24    
#define TX_DATA2        0x26
#endif

#define RST_TX_HIGH

//#define DBG_CHAR(x)     write_reg32(0xb801b200, x)
#define DBG_CHAR(x)     


////////////////////////////////////////////////////////////////////////
// CEC Wakeup Function
////////////////////////////////////////////////////////////////////////

#define __sleep     __attribute__       ((__section__ (".sleep.text")))
#define __sleepdata __attribute__       ((__section__ (".sleep.data")))


static unsigned long __sleepdata mars_cec_standby_config = 0;
static unsigned char __sleepdata mars_cec_standby_logical_addr = 0xf;
static unsigned short __sleepdata mars_cec_standby_physical_addr = 0xffff;


#define write_reg32(addr, val)      do {*((volatile unsigned int*) addr) = val; } while(0)
#define read_reg32(addr)                *((volatile unsigned int*) addr)


////////////////////////////////////////////////////////////////////////
// Register Map
////////////////////////////////////////////////////////////////////////
static unsigned long __sleepdata  CEC_ISR       = ISO_ISR;

static unsigned long __sleepdata  CEC_CR0       = ISO_CEC_CR0;                 
static unsigned long __sleepdata  CEC_CR1       = ISO_CEC_CR1;                 
static unsigned long __sleepdata  CEC_CR2       = ISO_CEC_CR2;                 
static unsigned long __sleepdata  CEC_CR3       = ISO_CEC_CR3;                 
static unsigned long __sleepdata  CEC_RT0       = ISO_CEC_RT0;                 
static unsigned long __sleepdata  CEC_RT1       = ISO_CEC_RT1;                 
static unsigned long __sleepdata  CEC_RX0       = ISO_CEC_RX0;                 
static unsigned long __sleepdata  CEC_RX1       = ISO_CEC_RX1;                 
static unsigned long __sleepdata  CEC_TX0       = ISO_CEC_TX0;                 
static unsigned long __sleepdata  CEC_TX1       = ISO_CEC_TX1;                 
static unsigned long __sleepdata  CEC_TX_FIFO   = ISO_CEC_TX_FIFO;                 
static unsigned long __sleepdata  CEC_RX_FIFO   = ISO_CEC_RX_FIFO;                 
static unsigned long __sleepdata  CEC_RX_START0 = ISO_CEC_RX_START0;                 
static unsigned long __sleepdata  CEC_RX_START1 = ISO_CEC_RX_START1;                 
static unsigned long __sleepdata  CEC_RX_DATA0  = ISO_CEC_RX_DATA0;                 
static unsigned long __sleepdata  CEC_RX_DATA1  = ISO_CEC_RX_DATA1;                 
static unsigned long __sleepdata  CEC_TX_START0 = ISO_CEC_TX_START0;                 
static unsigned long __sleepdata  CEC_TX_START1 = ISO_CEC_TX_START1;                 
static unsigned long __sleepdata  CEC_TX_DATA0  = ISO_CEC_TX_DATA0;                 
static unsigned long __sleepdata  CEC_TX_DATA1  = ISO_CEC_TX_DATA1;                 
static unsigned long __sleepdata  CEC_TX_DATA2  = ISO_CEC_TX_DATA2;            




/*------------------------------------------------------------------
 * Func : mars_cec_send_msg
 *
 * Desc : mars cec send msg
 *
 * Parm : pbuff : data to send
 *        len   : data length
 *         
 * Retn : 0 : not wakeup, 1 : wakeup  
 *------------------------------------------------------------------*/
int __sleep mars_cec_send_msg(unsigned char dst, unsigned char* pbuff, int len) 
{	                                                    
    write_reg32(CEC_TX_DATA2, 0x26);
    write_reg32(CEC_TX0, TX_RST);
    write_reg32(CEC_TX0, 0);        
    
    for(;len>0; len--)    
    {
        write_reg32(CEC_TX_FIFO, *(pbuff++));
    }   
    
    write_reg32(CEC_TX0, dst | TX_EN);      // write data out
        
    while((read_reg32(CEC_TX0) & TX_EN))    
        for (len=0; len<1000; len++);
    
	return 0;
}





/*------------------------------------------------------------------
 * Func : __mars_cec_check_wakeup
 *
 * Desc : mars cec check wakeup
 *
 * Parm : N/A
 *         
 * Retn : 0 : not wakeup, 1 : wakeup  
 *------------------------------------------------------------------*/
int __sleep __mars_cec_check_wakeup() 
{	   
    int ret = 0;              
    int len = read_reg32(CEC_RX1) & 0x1F;
    unsigned char init = read_reg32(CEC_RX0) & 0x0F;
    unsigned char broadcast = read_reg32(CEC_CR0) & 0x20;    
    unsigned short phy_addr;     
    unsigned char cmd[32];                         
    int i;
    for (i=0; i<len; i++)
        cmd[i] = read_reg32(CEC_RX_FIFO)& 0xFF;
    
    switch(cmd[0])
    {
    case CEC_MSG_IMAGE_VIEW_ON:
    case CEC_MSG_TEXT_VIEW_ON:
        if ((mars_cec_standby_config & STANBY_WAKEUP_BY_IMAGE_VIEW_ON) && len==1)
            ret = 1;
        break;
        
    case CEC_MSG_SET_STREAM_PATH:              // set stream path
        
        if ((mars_cec_standby_config & STANBY_WAKEUP_BY_SET_STREAM_PATH) && len==3)
        {
            phy_addr = (cmd[1]<<8) + cmd[2];
     
            if (phy_addr==mars_cec_standby_physical_addr)
                ret = 1;
        }
        break;
        
    case CEC_MSG_GIVE_DEVICE_POWER_STATUS:              // set stream path
        
        if (mars_cec_standby_config & STANBY_RESPONSE_GIVE_POWER_STATUS && len==1 && init!=0xF)
        {           
            cmd[0] = 0x90;  
            cmd[1] = 0x1;
            mars_cec_send_msg(init, cmd, 2);
        }
        break;

    case CEC_MSG_GIVE_PHYSICAL_ADDRESS:
        
        if (mars_cec_standby_config & STANBY_WAKEUP_BY_SET_STREAM_PATH && len==1 && !broadcast)
        {       
            cmd[0] = CEC_MSG_REPORT_PHYSICAL_ADDRESS;  
            cmd[1] = (mars_cec_standby_physical_addr >> 8) & 0xFF;
            cmd[2] = (mars_cec_standby_physical_addr) & 0xFF;
            cmd[3] = 0x04;
            mars_cec_send_msg(0xf, cmd, 4);
        }
        break;    
        
    default:
        // send feature abort         
        if (init!=0xF && !broadcast)
        {
            cmd[1] = cmd[0];
            cmd[0] = 0x00;      // FEATURE ABORT          
            cmd[2] = 1;         // not in correct mode to response
            mars_cec_send_msg(init, cmd, 3);        
        }
                    
    }
    
	return ret;
}



/*------------------------------------------------------------------
 * Func : mars_cec_wakeup_check
 *
 * Desc : mars cec wake up condition
 *
 * Parm : N/A
 *         
 * Retn : 0 : not wakeup, 1 : wakeup  
 *------------------------------------------------------------------*/
int __sleep mars_cec_wakeup_check(void) 
{	      
    int ret = 0;    
    
#ifdef CONFIG_GCEC_FIXUP_EN  

    int gcec_ret = 0;    
    //unsigned char cmd[5];
    
    if (mars_cec_standby_config &STANBY_RESPONSE_POOL)   
        gcec_ret = gcec_fixup_standy_work();                   // do gcec standby work
        
#endif    
        
    write_reg32(CEC_RX1, RX_INT);               // clear rx interrupt
    write_reg32(CEC_TX1, TX_INT);               // clear tx interrupt        
        
    
    if ((read_reg32(CEC_RX0) & RX_EN)==0)
    {                
        // rx stopped                
        if ((read_reg32(CEC_RX1) & RX_EOM) && __mars_cec_check_wakeup())
        {            
            write_reg32(CEC_RX0, RX_RST);   
            write_reg32(CEC_RX0, 0);            
            write_reg32(CEC_CR0, 0);               
            ret = 0xff;
            goto end_proc;            
        }
        
        // restart rx
        write_reg32(CEC_RX0, RX_RST);   
        write_reg32(CEC_RX0, 0);        
        write_reg32(CEC_RX0, RX_EN | RX_INT_EN);
    }            
    
    /*
#ifdef CONFIG_GCEC_FIXUP_EN  
     // GCEC state debug 
    if (mars_cec_standby_config &STANBY_RESPONSE_POOL && gcec_ret){            
        cmd[0] = 0x89;  
        cmd[1] = (unsigned char) (gcec_ret >> 24) & 0xFF;
        cmd[2] = (unsigned char) (gcec_ret >> 16) & 0xFF;
        cmd[3] = (unsigned char) (gcec_ret >> 8) & 0xFF;
        cmd[4] = (unsigned char) (gcec_ret) & 0xFF;
        mars_cec_send_msg(0xF, cmd, 5);
    }
#endif
    */    

end_proc:
        
    if (CEC_ISR==MIS_ISR)   // Mars CPU...
    {    
        // we should clear cec rx/tx interrupts manaually if CPU is mars        
        write_reg32(CEC_ISR, (CEC_TX_INT | CEC_RX_INT));    
    }
    
	return ret;
}


#undef  write_reg32
#undef  read_reg32
#define write_reg32(addr, val)      (writel((u32)(val), (volatile void __iomem*) (addr)))
#define read_reg32(addr)            ((u32) readl((volatile void __iomem*) (addr)))



/*------------------------------------------------------------------
 * Func : _read_rx_fifo
 *
 * Desc : read rx fifo
 *
 * Parm : cmb : cec message buffer
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
int _read_rx_fifo(cm_buff* cmb)
{    
    unsigned char* ptr;
    int len = read_reg32(CEC_RX1) & 0x1F;
    int i;
     
    if (cmb_tailroom(cmb) >= len)
    {        
        ptr = cmb_put(cmb, len);

        for (i=0; i<len; i++)
            *(ptr++) = (unsigned char) (read_reg32(CEC_RX_FIFO)& 0xFF);
                
        return len;
    }
    else
    {
        cec_warning("no more space to read data\n");
        return -ENOMEM;
    }
}



/*------------------------------------------------------------------
 * Func : _write_tx_fifo
 *
 * Desc : write data to tx fifo
 *
 * Parm : cmb
 *         
 * Retn : write length
 *------------------------------------------------------------------*/
int _write_tx_fifo(cm_buff* cmb)
{    
    unsigned char* ptr;
    int remain = 0x0F - (unsigned char)(read_reg32(CEC_TX1) & 0x0F);
    int i, len;
     
    len = cmb->len;
    
    if (len > remain)
        len = remain;
                        
    ptr = cmb->data;
    for (i=0; i<len; i++)        
    {        
        write_reg32(CEC_TX_FIFO, (unsigned long) *(ptr++));
    }
    
    cmb_pull(cmb, len);
                            
    return len;   
}



/*------------------------------------------------------------------
 * Func : _cmb_tx_complete
 *
 * Desc : complete a tx cmb
 *
 * Parm : p_this : handle of mars cec 
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
void _cmb_tx_complete(cm_buff* cmb)
{
    if (cmb->flags & NONBLOCK)
    {
        kfree_cmb(cmb);
    }
    else
    {
        if (cmb->status==WAIT_XMIT)
            cmb->status = XMIT_ABORT;
            
        complete(&cmb->complete);
    }
}



/*------------------------------------------------------------------
 * Func : mars_cec_rx_reset
 *
 * Desc : reset rx
 *
 * Parm : p_this : handle of mars cec  
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void mars_cec_rx_reset(mars_cec* p_this)
{       
    write_reg32(CEC_RX0, RX_RST);
    wmb();
    write_reg32(CEC_RX0, 0);        
    wmb();
    p_this->rcv.state = IDEL;
}



/*------------------------------------------------------------------
 * Func : mars_cec_tx_work
 *
 * Desc : mars cec tx function 
 *
 * Parm : p_this : handle of mars cec 
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
void mars_cec_tx_work(void* data)
{
    mars_cec* p_this = (mars_cec*) data;
    mars_cec_xmit* p_xmit = &p_this->xmit;    
    unsigned char dest;        
    unsigned long flags;                
    
    spin_lock_irqsave(&p_this->lock, flags);   
                
    if (p_xmit->state == XMIT)
    {
        if (read_reg32(CEC_TX0) & TX_EN)
        {                                                   
            // xmitting
            if (read_reg32(CEC_TX1) & TX_INT)
            {      
                if (p_xmit->cmb->len)
                {
                    _write_tx_fifo(p_xmit->cmb);
                    
                    if (p_xmit->cmb->len==0)                    
                        write_reg32(CEC_TX0, read_reg32(CEC_TX0) & ~TX_CONTINUOUS);                                                        
                }                            

                write_reg32(CEC_TX1, TX_INT);   // clear interrupt                                                                       
            }   
            
            if (time_after(jiffies, p_xmit->timeout))
            {                                    
                write_reg32(CEC_TX0, 0);                          
                
                p_xmit->cmb->status = XMIT_TIMEOUT;                
                _cmb_tx_complete(p_xmit->cmb);
                        
                p_xmit->cmb   = NULL;
                p_xmit->state = IDEL;
                
#ifdef RST_TX_HIGH                
                write_reg32(CEC_TX_DATA2, 1);                
                mars_cec_rx_reset(p_this);              // reset rx
                schedule_work(&p_this->rcv.work);       // restart rx                   
#endif
                cec_warning("cec tx timeout\n");
                
                cancel_delayed_work(&p_xmit->work);
            }                     
        }            
        else
        {
            cancel_delayed_work(&p_xmit->work);
            
            if ((read_reg32(CEC_TX1) & TX_EOM)==0)
            {
                p_xmit->cmb->status = XMIT_FAIL;                
                cec_warning("cec tx failed\n");
            }
            else
            {
                p_xmit->cmb->status = XMIT_OK;
                cec_tx_dbg("cec tx completed\n");
            }
            
            write_reg32(CEC_TX0, 0);
            _cmb_tx_complete(p_xmit->cmb);
            p_xmit->cmb   = NULL;
            p_xmit->state = IDEL;
            
#ifdef RST_TX_HIGH                        
            write_reg32(CEC_TX_DATA2, 1);
            mars_cec_rx_reset(p_this);            
            schedule_work(&p_this->rcv.work);       // restart rx
#endif            
        
        }                                                 
    }        
    
    if (p_xmit->state==IDEL)        
    {
        if (p_xmit->enable)
        {                                    
            p_xmit->cmb = cmb_dequeue(&p_this->tx_queue);
            
            if (p_xmit->cmb)
            {                            
                cec_tx_dbg("cec tx : cmb = %p, len=%d\n", p_xmit->cmb, p_xmit->cmb->len);
                dest = (p_xmit->cmb->data[0] & 0xf);                                
                cmb_pull(p_xmit->cmb, 1);          
                p_xmit->timeout = jiffies + TX_TIMEOUT;                                                                        

#ifdef RST_TX_HIGH                                    
                mars_cec_rx_reset(p_this);                // reset rx
                write_reg32(CEC_TX_DATA2, TX_DATA2);                
#endif

                // reset tx fifo
                write_reg32(CEC_TX0, TX_RST);
                wmb();
                write_reg32(CEC_TX0, 0);                                    
                wmb();                                
                                                                                                
                _write_tx_fifo(p_xmit->cmb);                
                                                   
                if (p_xmit->cmb->len==0)
                    write_reg32(CEC_TX0, dest | TX_EN | TX_INT_EN);
                else
                    write_reg32(CEC_TX0, dest | TX_EN | TX_INT_EN  | TX_CONTINUOUS);                                    
                             
                p_xmit->state = XMIT;
                
                cec_tx_dbg("cec tx start\n");
                
                schedule_delayed_work(&p_xmit->work, TX_TIMEOUT+1);      // queue delayed work for timeout detection
            }
        }
    }
    
    spin_unlock_irqrestore(&p_this->lock, flags);    
}



/*------------------------------------------------------------------
 * Func : mars_cec_tx_start
 *
 * Desc : start rx
 *
 * Parm : p_this : handle of mars cec  
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void mars_cec_tx_start(mars_cec* p_this)
{   
    unsigned long flags;    
    spin_lock_irqsave(&p_this->lock, flags);    
                
    if (!p_this->xmit.enable)
    {                        
        cec_info("cec tx start\n");
        p_this->xmit.enable = 1;
        p_this->xmit.state  = IDEL;
        p_this->xmit.cmb    = NULL;          
        
        INIT_WORK(&p_this->xmit.work, mars_cec_tx_work, (void*)p_this);
    }    
    
    spin_unlock_irqrestore(&p_this->lock, flags);
}


/*------------------------------------------------------------------
 * Func : mars_cec_tx_stop
 *
 * Desc : stop tx
 *
 * Parm : p_this : handle of mars cec  
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void mars_cec_tx_stop(mars_cec* p_this)
{   
    cm_buff* cmb;
    unsigned long flags;    
    spin_lock_irqsave(&p_this->lock, flags);    
                
    if (p_this->xmit.enable)
    {   
        cec_info("cec tx stop\n");
            
        mars_cec_rx_reset(p_this);        
                    
        if (p_this->xmit.cmb)
            _cmb_tx_complete(p_this->xmit.cmb);
            
        while((cmb = cmb_dequeue(&p_this->tx_queue)))
            _cmb_tx_complete(cmb);
                        
        p_this->xmit.enable = 0;
        p_this->xmit.state  = IDEL;
        p_this->xmit.cmb    = NULL;
    }    
    
    spin_unlock_irqrestore(&p_this->lock, flags);
}



/*------------------------------------------------------------------
 * Func : mars_cec_rx
 *
 * Desc : mars cec rx function 
 *
 * Parm : p_this : handle of mars cec 
 *         
 * Retn : N/A  
 *------------------------------------------------------------------*/
void mars_cec_rx_work(void* data)
{    
    mars_cec* p_this = (mars_cec*) data;
    mars_cec_rcv* p_rcv = &p_this->rcv;            
    unsigned long flags;           
     
    spin_lock_irqsave(&p_this->lock, flags);       
              
    if (!p_rcv->enable)
    {
        if (p_rcv->state==RCV)
        {
            cec_rx_dbg("force stop\n");            
            write_reg32(CEC_RX0, 0);
            kfree_cmb(p_rcv->cmb);
            p_rcv->cmb   = NULL;
            p_rcv->state = IDEL;
        }        
        goto end_proc;
    }    
                    
    if (p_rcv->state==RCV)
    {                   
        if (read_reg32(CEC_RX0) & RX_EN)
        {                           
            if (read_reg32(CEC_RX1) & RX_INT)
            {                                               
                if (_read_rx_fifo(p_rcv->cmb)<0)                                    
                {
                    cec_rx_dbg("read rx fifo failed, return to rx\n");                    
                    write_reg32(CEC_RX0, 0);
                    p_rcv->state = IDEL;
                }                            
                 
                write_reg32(CEC_RX1, RX_INT);                     
            }            
        }
        else
        {               
            cec_rx_dbg("rx_stop (0x%08x)\n", read_reg32(CEC_RX0));
            
            if ((read_reg32(CEC_RX1) & RX_EOM) && _read_rx_fifo(p_rcv->cmb))
            {                                                                                
                if (is_mars_cpu()||is_neptune_cpu())
                {
                    // mars cec can not identify the message is broadcasted or direct addressed
                    *cmb_push(p_rcv->cmb, 1) = (read_reg32(CEC_RX0) & 0xF)<<4 |
                                               (read_reg32(CEC_CR0) & 0xF);
                }
                else
                {
                //    printk("CEC_CR0 = %08x\n", read_reg32(CEC_CR0));

                    if (read_reg32(CEC_CR0) & 0x20)
                        *cmb_push(p_rcv->cmb, 1) = (read_reg32(CEC_RX0) & 0xF)<<4 | 0xF;
                    else
                        *cmb_push(p_rcv->cmb, 1) = (read_reg32(CEC_RX0) & 0xF)<<4 |
                                                   (read_reg32(CEC_CR0) & 0xF);
                }
                                         
                cmb_queue_tail(&p_this->rx_queue, p_rcv->cmb);                        
                p_rcv->cmb = NULL;
                
                wake_up_interruptible(&p_this->rcv.wq);
            }

            p_rcv->state = IDEL;            
        }         
    }
    
    if (p_rcv->state==IDEL)
    {           
        if (!p_rcv->enable)
            goto end_proc;
     
#ifdef RST_TX_HIGH                                
        if (read_reg32(CEC_TX_DATA2)!=1)
        {       
            cec_warning("[CEC] WARNING, runing rx when tx is running\n");     
            goto end_proc;
        }
#endif
        
        if (!p_rcv->cmb)   
        {            
            if (cmb_queue_len(&p_this->rx_free_queue))            
                p_rcv->cmb = cmb_dequeue(&p_this->rx_free_queue);
            else
            {
                cec_warning("[CEC] WARNING, rx over run\n");
                p_rcv->cmb = cmb_dequeue(&p_this->rx_queue);        // reclaim form rx fifo
            }
                
            if (p_rcv->cmb==NULL)
            {
                cec_warning("[CEC] FATAL, something wrong, no rx buffer left\n");
                goto end_proc;
            }
        }
                              
        cmb_purge(p_rcv->cmb);
        cmb_reserve(p_rcv->cmb, 1);
                
        write_reg32(CEC_RX0, RX_RST);                
        wmb();
        write_reg32(CEC_RX0, 0);        
        wmb();        
                                                                                                              
        write_reg32(CEC_RX0, RX_EN | RX_INT_EN);
        cec_rx_dbg("rx_restart (0x%08x)\n", read_reg32(CEC_RX0));                        
        p_rcv->state = RCV;        
    }                         
    
end_proc:

    spin_unlock_irqrestore(&p_this->lock, flags);                      
}




/*------------------------------------------------------------------
 * Func : mars_cec_rx_start
 *
 * Desc : start rx
 *
 * Parm : p_this : handle of mars cec  
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void mars_cec_rx_start(mars_cec* p_this)
{   
    unsigned long flags;    
    spin_lock_irqsave(&p_this->lock, flags);    
                
    if (!p_this->rcv.enable)
    {                       
        cec_info("rx start\n");
        
        p_this->rcv.enable = 1;
        p_this->rcv.state  = IDEL;
        p_this->rcv.cmb    = NULL;      
        
        INIT_WORK(&p_this->rcv.work, mars_cec_rx_work, (void*)p_this);
                
        schedule_work(&p_this->rcv.work);                  
    }    
    
    spin_unlock_irqrestore(&p_this->lock, flags);
}



/*------------------------------------------------------------------
 * Func : mars_cec_rx_stop
 *
 * Desc : stop rx
 *
 * Parm : p_this : handle of mars cec  
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
static 
void mars_cec_rx_stop(mars_cec* p_this)
{   
    cm_buff* cmb;
    unsigned long flags;    
        
    spin_lock_irqsave(&p_this->lock, flags);    
    
    if (p_this->rcv.enable)
    {           
        cec_info("rx stop\n");        
        
        write_reg32(CEC_RX0, RX_RST);
        wmb();
        write_reg32(CEC_RX0, 0);        
        wmb();
                    
        if (p_this->rcv.cmb)
        {
            cmb_purge(p_this->rcv.cmb);
            cmb_reserve(p_this->rcv.cmb, 1);
            cmb_queue_tail(&p_this->rx_free_queue, p_this->rcv.cmb);
        }    
            
        while((cmb = cmb_dequeue(&p_this->rx_queue)))
        {
            cmb_purge(cmb);
            cmb_reserve(cmb, 1);
            cmb_queue_tail(&p_this->rx_free_queue, cmb);
        }
        
        p_this->rcv.enable = 0;
        p_this->rcv.state  = IDEL;
        p_this->rcv.cmb    = NULL;
        
        wake_up_interruptible(&p_this->rcv.wq);
    }    
    spin_unlock_irqrestore(&p_this->lock, flags);
}



/*------------------------------------------------------------------
 * Func : mars_cec_isr
 *
 * Desc : isr of mars cec
 *
 * Parm : p_this : handle of mars cec 
 *        dev_id : handle of the mars_cec
 *        regs   :
 *         
 * Retn : IRQ_NONE, IRQ_HANDLED
 *------------------------------------------------------------------*/
static 
irqreturn_t mars_cec_isr(
    int                     this_irq, 
    void*                   dev_id, 
    struct pt_regs*         regs
    )
{            
    unsigned long event = read_reg32(CEC_ISR) & (CEC_TX_INT | CEC_RX_INT);    
    
    if (!event)
        return IRQ_NONE;                    
        
    //DBG_CHAR('i');            
    
    if (event & CEC_TX_INT)                    
        mars_cec_tx_work((void*) dev_id);    
    
    if (event & CEC_RX_INT)    
        mars_cec_rx_work((void*) dev_id);
            
    write_reg32(CEC_ISR, event );      
                      
    return IRQ_HANDLED;    
}



/*------------------------------------------------------------------
 * Func : mars_cec_init
 *
 * Desc : init a mars cec adapter
 *
 * Parm : N/A
 *         
 * Retn : handle of cec module 
 *------------------------------------------------------------------*/
int mars_cec_init(mars_cec* p_this)
{            
    cm_buff*  cmb = NULL;
    int i;
    
    if (!p_this->status.init)
    {              
        if (is_jupiter_cpu())
        {
            cec_info("Open Jupiter CEC\n");      
            CEC_ISR       = ISO_ISR;    
            CEC_CR0       = ISO_CEC_CR0;                 
            CEC_CR1       = ISO_CEC_CR1;                 
            CEC_CR2       = ISO_CEC_CR2;                 
            CEC_CR3       = ISO_CEC_CR3;                 
            CEC_RT0       = ISO_CEC_RT0;                 
            CEC_RT1       = ISO_CEC_RT1;                 
            CEC_RX0       = ISO_CEC_RX0;                 
            CEC_RX1       = ISO_CEC_RX1;                 
            CEC_TX0       = ISO_CEC_TX0;                 
            CEC_TX1       = ISO_CEC_TX1;                 
            CEC_TX_FIFO   = ISO_CEC_TX_FIFO;                 
            CEC_RX_FIFO   = ISO_CEC_RX_FIFO;                 
            CEC_RX_START0 = ISO_CEC_RX_START0;                 
            CEC_RX_START1 = ISO_CEC_RX_START1;                 
            CEC_RX_DATA0  = ISO_CEC_RX_DATA0;                 
            CEC_RX_DATA1  = ISO_CEC_RX_DATA1;                 
            CEC_TX_START0 = ISO_CEC_TX_START0;                 
            CEC_TX_START1 = ISO_CEC_TX_START1;                 
            CEC_TX_DATA0  = ISO_CEC_TX_DATA0;                 
            CEC_TX_DATA1  = ISO_CEC_TX_DATA1;                 
            CEC_TX_DATA2  = ISO_CEC_TX_DATA2;
        }
        else if (is_mars_cpu() || is_neptune_cpu())
        {                 
            if (is_mars_cpu())
                cec_info("Open Mars CEC\n");   
            else
                cec_info("Open Neptune CEC\n");   

            CEC_ISR       = MIS_ISR;       
            CEC_CR0       = MIS_CEC_CR0;                 
            CEC_CR1       = MIS_CEC_CR1;                 
            CEC_CR2       = MIS_CEC_CR2;                 
            CEC_CR3       = MIS_CEC_CR3;                 
            CEC_RT0       = MIS_CEC_RT0;                 
            CEC_RT1       = MIS_CEC_RT1;                 
            CEC_RX0       = MIS_CEC_RX0;                 
            CEC_RX1       = MIS_CEC_RX1;                 
            CEC_TX0       = MIS_CEC_TX0;                 
            CEC_TX1       = MIS_CEC_TX1;                 
            CEC_TX_FIFO   = MIS_CEC_TX_FIFO;                 
            CEC_RX_FIFO   = MIS_CEC_RX_FIFO;                 
            CEC_RX_START0 = MIS_CEC_RX_START0;                 
            CEC_RX_START1 = MIS_CEC_RX_START1;                 
            CEC_RX_DATA0  = MIS_CEC_RX_DATA0;                 
            CEC_RX_DATA1  = MIS_CEC_RX_DATA1;                 
            CEC_TX_START0 = MIS_CEC_TX_START0;                 
            CEC_TX_START1 = MIS_CEC_TX_START1;                 
            CEC_TX_DATA0  = MIS_CEC_TX_DATA0;                 
            CEC_TX_DATA1  = MIS_CEC_TX_DATA1;                 
            CEC_TX_DATA2  = MIS_CEC_TX_DATA2;
        }
        else
        {
            return -1;
        }
        write_reg32(CEC_CR0, 0x0f);        // Disable CEC
        write_reg32(CEC_RX0, 0);           // RX Disable 
        write_reg32(CEC_TX0, 0);           // TX Disable
        write_reg32(CEC_RT1, 5);           // Retry times = 5
        write_reg32(CEC_CR2, PRE_DIV);        
        write_reg32(CEC_RX_DATA1,  RX_DATA1);   // update rx data 1
        write_reg32(CEC_RX_START0, RX_START0);   
        write_reg32(CEC_RX_START1, RX_START1);   
        write_reg32(CEC_TX_START0, TX_START0);   
        write_reg32(CEC_TX_START1, TX_START1);   
        write_reg32(CEC_TX_DATA0,  TX_DATA0);   
        write_reg32(CEC_TX_DATA1,  TX_DATA1);          
#ifdef RST_TX_HIGH                    
        write_reg32(CEC_TX_DATA2, 1);
#else
        write_reg32(CEC_TX_DATA2,  TX_DATA2);   
#endif                       
        
        spin_lock_init(&p_this->lock);        
        p_this->xmit.state      = IDEL;    
        p_this->xmit.cmb        = NULL;    
        p_this->xmit.timeout    = 0;        
        
        p_this->rcv.state       = IDEL;  
        p_this->rcv.cmb         = NULL;  
        
        init_waitqueue_head(&p_this->rcv.wq);                
        
        cmb_queue_head_init(&p_this->tx_queue);
        cmb_queue_head_init(&p_this->rx_queue);
        cmb_queue_head_init(&p_this->rx_free_queue);
        
        for (i=0; i< RX_RING_LENGTH; i++)
        {
            cmb = alloc_cmb(RX_BUFFER_SIZE);
            if (cmb)
                cmb_queue_tail(&p_this->rx_free_queue, cmb);            
        }            

        if (request_irq(MISC_IRQ, mars_cec_isr, SA_INTERRUPT | SA_SHIRQ, "MARS CEC", p_this) < 0) 
        {
            cec_warning("cec : open mars cec failed, unable to request irq#%d\n", MISC_IRQ);	    		
            return -1;
        }
        
        p_this->status.init = 1;
    }
  
    return 0;            
}




/*------------------------------------------------------------------
 * Func : mars_cec_enable
 *
 * Desc : enable ces module
 *
 * Parm : p_this   : handle of mars cec adapter
 *        on_off   : 0 : disable, others enable
 *         
 * Retn : 0 for success, others failed
 *------------------------------------------------------------------*/
int mars_cec_enable(mars_cec* p_this, unsigned char on_off)
{    
    if (on_off)                
    {                          
        if (!p_this->status.enable)
        {
            cec_info("mars cec enabled\n");                                              
            write_reg32(CEC_CR0, (read_reg32(CEC_CR0) & 0x3F) | CEC_MODE_NORMAL);
            mars_cec_rx_start(p_this);
            mars_cec_tx_start(p_this);
            p_this->status.enable = 1;
            
#ifdef CONFIG_GCEC_FIXUP_EN            
            if ((read_reg32(CEC_CR0) & 0xF)!=0xF)          
                gcec_fixup_enable(read_reg32(CEC_CR0) & 0xF);
            else
                gcec_fixup_disable();
#endif                
        }
    }
    else
    {
        if (p_this->status.enable)
        {
            cec_info("mars cec disabled\n");                   
            write_reg32(CEC_CR0, read_reg32(CEC_CR0) & 0x3F);            
            p_this->status.enable = 0;                        
            mars_cec_rx_stop(p_this);
            mars_cec_tx_stop(p_this);            
            
#ifdef CONFIG_GCEC_FIXUP_EN                     
            gcec_fixup_disable();
#endif            
        }
    }          
    
    return 0;
}



/*------------------------------------------------------------------
 * Func : mars_cec_set_logical_addr
 *
 * Desc : set logical address of mars ces
 *
 * Parm : p_this    : handle of mars cec adapter
 *        log_addr : logical address  
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int mars_cec_set_logical_addr(
    mars_cec*               p_this,
    unsigned char           log_addr    
    )
{         
    unsigned long val = read_reg32(CEC_CR0);
        
    if (log_addr>0x0F) 
    {
        cec_warning("cec : illegal logical address %02x\n", log_addr);
        return -EFAULT;
    }    
        
    val = val & ~0xF;
    write_reg32(CEC_CR0, val | log_addr);
    
    cec_info("cec : logical address = %02x\n", log_addr);    
    
#ifdef CONFIG_GCEC_FIXUP_EN
    if (log_addr!=0xF)
        gcec_fixup_enable(log_addr);
    else
        gcec_fixup_disable();
#endif    

    if (log_addr != 0xF)
        mars_cec_standby_logical_addr = log_addr;         // save the latest vaild logical address
    
    return 0;            
}    



/*------------------------------------------------------------------
 * Func : mars_cec_set_physical_addr
 *
 * Desc : set physical address of mars ces
 *
 * Parm : p_this    : handle of mars cec adapter
 *        phy_addr  : physical address  
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int mars_cec_set_physical_addr(
    mars_cec*               p_this,
    unsigned short          phy_addr    
    )
{         
    cec_info("cec : physcial address = %04x\n", phy_addr);    
    
    if (phy_addr!= 0xffff)
        mars_cec_standby_physical_addr = phy_addr;      // we always keep latest valid physical address during standby
        
    return 0;            
}    




/*------------------------------------------------------------------
 * Func : mars_cec_set_stanby_mode
 *
 * Desc : set standy mode condition of mars ces
 *
 * Parm : p_this  : handle of mars cec adapter
 *        mode    : cec standby mode
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
int mars_cec_set_stanby_mode(
    mars_cec*               p_this,
    unsigned long           mode
    )
{         
    mars_cec_standby_config = mode;
    return 0;            
}    



/*------------------------------------------------------------------
 * Func : mars_cec_xmit_msg
 *
 * Desc : xmit message
 *
 * Parm : p_this   : handle of mars cec 
 *        cmb      : msg to xmit
 *         
 * Retn : 0 : for success, others : fail
 *------------------------------------------------------------------*/
int mars_cec_xmit_message(mars_cec* p_this, cm_buff* cmb, unsigned long flags)
{    
    int ret = 0;
    
    spin_lock(&p_this->lock);   
        
    if (!p_this->xmit.enable) {
        spin_unlock(&p_this->lock); 
        return -1;
    }
        
#ifdef CONFIG_GCEC_FIXUP_EN        
    gcec_fixup_disable();           // fix up disable
#endif
    
    cmb->flags  = flags;
    cmb->status = WAIT_XMIT;
    
    cmb_queue_tail(&p_this->tx_queue, cmb);
    
    if (p_this->xmit.state==IDEL)
        schedule_work(&p_this->xmit.work);  // restart tx
    
    spin_unlock(&p_this->lock);   
    
    if ((cmb->flags & NONBLOCK)==0)
    {                
        wait_for_completion(&cmb->complete);    
        switch(cmb->status)    
        {
        case XMIT_OK:
            cec_tx_dbg("cec : xmit message success\n");
            break;
                            
        case XMIT_TIMEOUT:            
            cec_warning("cec : xmit message timeout\n");
            break;
            
        case XMIT_ABORT:
            cec_warning("cec : xmit message abort\n");
            break;        
        
        case XMIT_FAIL:
            cec_warning("cec : xmit message failed\n");
            break;
        }
            
        ret = (cmb->status==XMIT_OK) ? 0 : -1;
        kfree_cmb(cmb);
    }                    
    
#ifdef CONFIG_GCEC_FIXUP_EN
    if ((read_reg32(CEC_CR0) & 0xF)!=0xF)
        gcec_fixup_enable(read_reg32(CEC_CR0) & 0xF);   // re enable fixup
    else
        gcec_fixup_disable();
#endif                     
    
    return ret;
}




/*------------------------------------------------------------------
 * Func : mars_cec_read_message
 *
 * Desc : read message
 *
 * Parm : p_this : handle of cec device
 *        flags  : flag of current read operation
 * 
 * Retn : cec message
 *------------------------------------------------------------------*/
static cm_buff* mars_cec_read_message(mars_cec* p_this, unsigned char flags)
{                
    if (!(flags & NONBLOCK) && p_this->status.enable && !cmb_queue_len(&p_this->rx_queue))
        wait_event_interruptible(p_this->rcv.wq, (!p_this->status.enable || cmb_queue_len(&p_this->rx_queue)));    // wait message        
    
    if (p_this->status.enable)
    {
        cm_buff* cmb = cmb_dequeue(&p_this->rx_queue);
    
        if (cmb)
        {   
            cec_rx_dbg("got msg %p\n", cmb);
            cmb_queue_tail(&p_this->rx_free_queue, alloc_cmb(RX_BUFFER_SIZE));
            return cmb;
        }        
    }
        
    return NULL;
}


/*------------------------------------------------------------------
 * Func : mars_cec_uninit
 *
 * Desc : uninit a mars cec adapter
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void mars_cec_uninit(mars_cec* p_this)
{         
    cec_info("mars cec closed\n");    
    free_irq(MISC_IRQ ,p_this);
    write_reg32(CEC_CR0, 0x0f);
    write_reg32(CEC_RX0, 0);   
    write_reg32(CEC_TX0, 0);        
    mars_cec_enable(p_this, 0);
    p_this->status.init = 0;
}    


#ifdef CONFIG_PM
  extern int  register_cec_wakeup_ops( int (*ops)(void) ) ;
  extern void unregister_cec_wakeup_ops( int (*ops)(void) ) ;
#else
  #define register_cec_wakeup_ops(x)
  #define unregister_cec_wakeup_ops(x) 
#endif

/*------------------------------------------------------------------
 * Func : mars_cec_suspend
 *
 * Desc : suspend a mars cec adapter
 *
 * Parm : p_this : handle of mars cec adapter
 * 
 * Retn : 0
 *------------------------------------------------------------------*/
int mars_cec_suspend(mars_cec* p_this)
{
    cec_info("mars cec suspended (la=%x, pa=%04x, standby_mode=%08x)\n",
            mars_cec_standby_logical_addr, 
            mars_cec_standby_physical_addr, 
            mars_cec_standby_config);
    
    if (mars_cec_standby_config)    
    {
        register_cec_wakeup_ops(mars_cec_wakeup_check);

        write_reg32(CEC_CR0, 0x40 | mars_cec_standby_logical_addr);        // Enable CEC
        write_reg32(CEC_RX0, RX_RST);           // RX Disable 
        write_reg32(CEC_RX0, 0);           // RX Disable 
        write_reg32(CEC_TX0, TX_RST);           // RX Disable 
        write_reg32(CEC_TX0, 0);           // TX Disable
        write_reg32(CEC_RT1, 5);           // Retry times = 5
        write_reg32(CEC_CR2, PRE_DIV);        
        write_reg32(CEC_RX_START0, RX_START0);   
        write_reg32(CEC_RX_START1, RX_START1);   
        write_reg32(CEC_TX_START0, TX_START0);   
        write_reg32(CEC_TX_START1, TX_START1);   
        write_reg32(CEC_TX_DATA0,  TX_DATA0);   
        write_reg32(CEC_TX_DATA1,  TX_DATA1);                             
        write_reg32(CEC_TX_DATA2, 0x26);
        write_reg32(CEC_RX0, RX_RST);
        write_reg32(CEC_RX0, 0);                        
        write_reg32(CEC_RX0, 0x90);     // enabla RX interrupt
        
        cec_info("standby mode cec controller enabled\n");
        
#ifdef CONFIG_GCEC_FIXUP_EN
        if (mars_cec_standby_config &STANBY_RESPONSE_POOL)
            gcec_fixup_suspend(mars_cec_standby_logical_addr);       // enter suspend mode
#endif        
    }        
        
    return 0;
}


/*------------------------------------------------------------------
 * Func : mars_cec_resume
 *
 * Desc : suspend a mars cec adapter
 *
 * Parm : p_this : handle of mars cec adapter
 * 
 * Retn : 0
 *------------------------------------------------------------------*/
int mars_cec_resume(mars_cec* p_this)
{
    cec_info("mars cec resume\n");       
    
    if (mars_cec_standby_config)
    {
        write_reg32(CEC_RX0, RX_RST);
        write_reg32(CEC_RX0, 0);        // disable cec
        unregister_cec_wakeup_ops(mars_cec_wakeup_check);        
        
#ifdef CONFIG_GCEC_FIXUP_EN         
        gcec_fixup_disable();
#endif        
    }
        
    return 0;
}


////////////////////////////////////////////////////////////////////////
// CEC Driver Interface                                               //
////////////////////////////////////////////////////////////////////////


static int ops_probe(cec_device* dev)
{    
    if (dev->id!=ID_MARS_CEC_CONTROLLER)
        return -ENODEV;                
            
    if (mars_cec_init(&m_cec)==0)    
        cec_set_drvdata(dev, &m_cec);
      
    return 0; 
}


static void ops_remove(cec_device* dev)
{        
    mars_cec_uninit((mars_cec*) cec_get_drvdata(dev));    
}


static int ops_enable(cec_device* dev, unsigned char on_off)
{    
    return mars_cec_enable((mars_cec*) cec_get_drvdata(dev), on_off);        
}


static int ops_set_logical_addr(cec_device* dev, unsigned char log_addr)
{      
    return mars_cec_set_logical_addr((mars_cec*) cec_get_drvdata(dev), log_addr);       
}    


static int ops_set_physical_addr(cec_device* dev, unsigned short phy_addr)
{      
    return mars_cec_set_physical_addr((mars_cec*) cec_get_drvdata(dev), phy_addr);       
}    


static int ops_xmit(cec_device* dev, cm_buff* cmb, unsigned long flags)
{
    return mars_cec_xmit_message((mars_cec*) cec_get_drvdata(dev), cmb, flags);        
}


static cm_buff* ops_read(cec_device* dev, unsigned char flags)
{
    return mars_cec_read_message((mars_cec*) cec_get_drvdata(dev), flags);     
}


static int ops_set_stanby_mode(cec_device* dev, unsigned long flags)
{       
    return mars_cec_set_stanby_mode((mars_cec*) cec_get_drvdata(dev), flags);    
}


static int ops_suspend(cec_device* dev)
{       
    return mars_cec_suspend((mars_cec*) cec_get_drvdata(dev));    
}


static int ops_resume(cec_device* dev)
{    
    return mars_cec_resume((mars_cec*) cec_get_drvdata(dev));
}



static cec_device mars_cec_controller = 
{
    .id   = ID_MARS_CEC_CONTROLLER,
    .name = "mars_cec"    
};


static cec_driver mars_cec_driver = 
{    
    .name     = "mars_cec",
    .probe    = ops_probe,
    .remove   = ops_remove,        
    .suspend  = ops_suspend,
    .resume   = ops_resume,
    .enable   = ops_enable,
    .xmit     = ops_xmit,
    .read     = ops_read,
    .set_logical_addr = ops_set_logical_addr,    
    .set_physical_addr = ops_set_physical_addr,
    .set_stanby_mode = ops_set_stanby_mode,
};




/*------------------------------------------------------------------
 * Func : cec_core_init
 *
 * Desc : cec dev init function
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static int __init mars_cec_module_init(void)
{                        
    cec_info("mars cec module init\n");        
                                
    if (register_cec_driver(&mars_cec_driver)!=0)
        return -EFAULT;                        
        
    register_cec_device(&mars_cec_controller);          // register cec device
                            
    return 0;
}



/*------------------------------------------------------------------
 * Func : cec_core_exit
 *
 * Desc : cec dev exit function
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static void __exit mars_cec_module_exit(void)
{            
    unregister_cec_driver(&mars_cec_driver);
}


module_init(mars_cec_module_init);
module_exit(mars_cec_module_exit);
