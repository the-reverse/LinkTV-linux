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
#include <asm/mach-venus/jupiter.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include "cec_mars.h"
#include "cec_mars_reg.h"
#include "cec_jupiter_reg.h"           
#include "gcec_fixup.h"

#ifdef CONFIG_DEFAULT_GCEC_ISO_GPIO
#define DEFAULT_GPIO_TYPE   1
#else
#define DEFAULT_GPIO_TYPE   0
#endif
#define DEFAULT_GPIO_CH    CONFIG_DEFAULT_GCEC_GPIO

#define __sleep     __attribute__       ((__section__ (".sleep.text")))
#define __sleepdata __attribute__       ((__section__ (".sleep.data")))

static unsigned char enable = 0;
static unsigned char state  = 0;
static unsigned char sstate = 0;
static unsigned long cycle_per_us = 10000;
static unsigned char iso     = DEFAULT_GPIO_TYPE;
static unsigned long gpio_ch = DEFAULT_GPIO_CH;

static unsigned char __sleepdata init_rdy = 0;
static unsigned long __sleepdata header = 0;
static unsigned long __sleepdata CEC_GPDIR  = MIS_GP0DIR;
static unsigned long __sleepdata CEC_GPDATO = MIS_GP0DATO;    
static unsigned long __sleepdata CEC_GPDATI = MIS_GP0DATI;
static unsigned long __sleepdata CEC_GPIE   = MIS_GP0IE;
static unsigned long __sleepdata CEC_GPDP   = MIS_GP0DP;
static unsigned long __sleepdata CEC_GPMASK = 0x1;
static unsigned long __sleepdata GCEC_ISR   = MIS_ISR;
static unsigned long __sleepdata GCEC_ISRMASK = (0x3UL << 19);

#define MIN_START_PERIOD    4300
#define MAX_START_PERIOD    4700

#define MIN_START_RISING    3500
#define MAX_START_RISING    3900

#define MIN_BIT_PERIOD      2050
#define MAX_BIT_PERIOD      2750

#define MIN_ONE_RISING      400
#define MAX_ONE_RISING      800

#define MIN_ZERO_RISING     1300
#define MAX_ZERO_RISING     1700

#define write_reg32(addr, val)      do {*((volatile unsigned int*) addr) = val; } while(0)
#define read_reg32(addr)                *((volatile unsigned int*) addr)


unsigned int read_counter(void)
{
    if (read_c0_cause() & 0x08000000)
        return 0xffffffff;
    else
        return read_c0_count() / cycle_per_us;
}

void reset_counter(void)
{
	write_c0_count(0);
	write_c0_compare(0xffffffff);
	
	if (read_c0_cause() & 0x08000000)
	    write_c0_cause(read_c0_cause() & ~0x08000000);
}

         
         
int enable_counter(unsigned char on)
{
	if (on) 
    {
        gcec_dbg("enable time counter\n");
	    write_c0_cause(read_c0_cause() & ~0x08000000);
	    write_c0_count(0);
	   	write_c0_cause(read_c0_cause() & ~0x08000000);
	    write_c0_compare(0xffffffff);
	    udelay(16);
	    cycle_per_us = read_c0_count() >> 4;
	    write_c0_count(0);
    }
    else
    {
        gcec_dbg("disable time counter\n");
        write_c0_cause(read_c0_cause() | 0x08000000);
        write_c0_count(0);                  
    }
    return 0;
}



/*------------------------------------------------------------------
 * Func : gcec_isr
 *
 * Desc : isr of cec gpio. this GPIO is used to fix bug of mars/jupiter 
 *        cec controller can not ack polling message as if it's init address
 *        is exactly the same with our logical address.
 *
 * Parm : p_this : handle of mars cec 
 *        dev_id : handle of the mars_cec
 *        regs   :
 *         
 * Retn : IRQ_NONE, IRQ_HANDLED
 *------------------------------------------------------------------*/
static 
void gcec_isr(
    void*                   dev_id, 
    unsigned char           assert
    )
{
    unsigned long time_interval;
    unsigned long tmax;
    unsigned long tmin;
           
    if (!enable)
        return;
        
    //---start     
    if (state >= GCEC_ST_MAX) 
    {
        state = GCEC_ST_START;
        sstate = GCEC_SST_WAIT_START;
    }
    
    time_interval = read_counter();
    
    if (sstate==GCEC_SST_WAIT_START)
    {
        if (assert==0)     // low level
        {
            switch(state)
            {
            case GCEC_ST_START:
                sstate = GCEC_SST_WAIT_HIGH;
                break;
            
            case GCEC_ST_ACK:
                // send ack bit
                write_reg32(CEC_GPDIR, read_reg32(CEC_GPDIR) | CEC_GPMASK);      // set to output mod
                udelay(1500);           // wait 1500 ms
                write_reg32(CEC_GPDIR, read_reg32(CEC_GPDIR) & ~CEC_GPMASK);     // set input mode
                break;
               
            default:
                
                if (state==GCEC_ST_INIT3)
                {
                    tmax = MAX_START_PERIOD;
                    tmin = MIN_START_PERIOD;
                }
                else
                {
                    tmax = MAX_BIT_PERIOD;
                    tmin = MIN_BIT_PERIOD;                
                }
                
                if (time_interval >= tmin && time_interval <= tmax)
                    sstate = GCEC_SST_WAIT_HIGH;   
                else
                {
                    gcec_dbg("gcec wait start failed, (state=%d) time_interval=%d (t: %d-%d)\n", state, time_interval, tmin, tmax);
                }
                break;
            }
        }
        
        if (sstate!=GCEC_SST_WAIT_HIGH)
            state=GCEC_ST_START;        // reset state
        
        reset_counter();
    }
    else
    {
        if (assert)     // high level
        {
            if (state==GCEC_ST_START)  // check bit periodic is correct or not
            {
                tmin = MIN_START_RISING;
                tmax = MAX_START_RISING;
            }
            else
            {
                if ((header & (0x1 << (GCEC_ST_EOM - state))))
                {
                    tmin = MIN_ONE_RISING;
                    tmax = MAX_ONE_RISING;
                }
                else
                {
                    tmin = MIN_ZERO_RISING;
                    tmax = MAX_ZERO_RISING;
                }
            }
            
            if (time_interval >= tmin && time_interval <= tmax)
            {
                state++;
                sstate = GCEC_SST_WAIT_START;
            }
            else
            {
                gcec_dbg("gcec wait high failed, (state=%d) time_interval=%d (t: %d-%d)\n", state, time_interval, tmin, tmax);
            }
        }
        
        if (sstate!=GCEC_SST_WAIT_START) {
            state  = GCEC_ST_START;        // reset state
            sstate = GCEC_SST_WAIT_START;
            reset_counter();
         }
    }
}


#ifdef CONFIG_REALTEK_USE_EXTERNAL_TIMER_INTERRUPT


/*------------------------------------------------------------------
 * Func : gcec_fixup_init
 *
 * Desc : init gcec fixup
 *
 * Parm : iso : iso GPIO or not
 *        cpio_ch : gpio channel
 *         
 * Retn : 0 : success, others : failed
 *------------------------------------------------------------------*/
int gcec_fixup_init()
{
    unsigned char g_group = 0;
    
    if (gpio_ch==0xFF) 
        goto invalid_gpio;
    
    if (is_jupiter_cpu())
    {
        if (iso)
        {
            if (gpio_ch >= 10)
            {
                gcec_warning("gcec_fixup_init failed - invalid iso gpio number %d\n", gpio_ch);
                goto invalid_gpio;
            }
            
            GCEC_ISR     = ISO_ISR;
            GCEC_ISRMASK = (0x1UL << 19);
            CEC_GPDIR    = JUPITER_ISO_GPDIR;
            CEC_GPDATO   = JUPITER_ISO_GPDATO; 
            CEC_GPDATI   = JUPITER_ISO_GPDATI;
            CEC_GPIE     = JUPITER_ISO_GPIE;
            CEC_GPDP     = JUPITER_ISO_GPDP;
            CEC_GPMASK   = 0x00000001 << (gpio_ch);
        }
        else
        {
            if (gpio_ch >= 128)
            {
                gcec_warning("gcec_fixup_init failed - invalid gpio number %d\n", gpio_ch);
                goto invalid_gpio;
            }
            
            g_group      = gpio_ch >> 5;
            GCEC_ISR     = MIS_ISR;
            GCEC_ISRMASK = (0x1UL << 19);
            CEC_GPDIR    = MIS_GP0DIR  + (g_group<<2);
            CEC_GPDATO   = MIS_GP0DATO + (g_group<<2); 
            CEC_GPDATI   = MIS_GP0DATI + (g_group<<2);
            CEC_GPIE     = MIS_GP0IE   + (g_group<<2);
            CEC_GPDP     = MIS_GP0DP   + (g_group<<2);
            CEC_GPMASK   = 0x00000001 << (gpio_ch & 0x1F);
        }
    }
    else
    {
        gcec_warning("gcec_fixup_init failed - gcec only works on jupiter cpu\n");
        goto invalid_gpio;
    }    
    
    write_reg32(CEC_GPDIR,  read_reg32(CEC_GPDIR)  & ~CEC_GPMASK);    // set to input mode
    write_reg32(CEC_GPDATO, read_reg32(CEC_GPDATO) & ~CEC_GPMASK);    // set to out val to 0
    write_reg32(CEC_GPDP,   read_reg32(CEC_GPDP)   |  CEC_GPMASK);    // set assert falg to high level
    write_reg32(CEC_GPIE,   read_reg32(CEC_GPIE)   & ~CEC_GPMASK);    // disable interrupt
        
    header   = 0;
    state    = GCEC_ST_START;
    sstate   = 0;
    enable   = 0;
    init_rdy = 1;
                
    gcec_info("gcec_module_init with %s(%ld)\n", (iso) ? "ISO GPIO" : "GPIO", gpio_ch);

    if (request_kgpio_irq(iso, gpio_ch, gcec_isr,  "GCEC", (unsigned long) gcec_isr) < 0)     
        gcec_warning("gcec_fixup_init failed : alloc irq %d failed\n", MISC_IRQ);
                    
    return 0;

//----------------
invalid_gpio:    
    iso = 0;
    gpio_ch = 0xff;
    return -1;       
}


/*------------------------------------------------------------------
 * Func : gcec_fixup_uninit
 *
 * Desc : uninit gcec fixup
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void gcec_fixup_uninit()
{
    free_kgpio_irq(iso, gpio_ch, (unsigned long) gcec_isr);
}

#else


/*------------------------------------------------------------------
 * Func : gcec_fixup_init
 *
 * Desc : init gcec fixup
 *
 * Parm : iso : iso GPIO or not
 *        cpio_ch : gpio channel
 *         
 * Retn : 0 : success, others : failed
 *------------------------------------------------------------------*/
int gcec_fixup_init()
{
    gcec_warning("gcec_fixup_enable failed - c0 counter has been occupied\n");
    return -1;
}



/*------------------------------------------------------------------
 * Func : gcec_fixup_uninit
 *
 * Desc : uninit gcec fixup
 *
 * Parm : N/A
 *         
 * Retn : N/A
 *------------------------------------------------------------------*/
void gcec_fixup_uninit()
{
}
    
#endif    



/*------------------------------------------------------------------
 * Func : gcec_fixup_enable
 *
 * Desc : enable gcec fixup
 *
 * Parm : addr : address to monitoring
 *         
 * Retn : 0 : success, others : failed
 *------------------------------------------------------------------*/
int gcec_fixup_enable(unsigned char addr)
{
    if (!init_rdy)
        return -1;
        
    if (!enable)
    {   
        gcec_dbg("gcec_fixup_enabled\n");
                
        write_reg32(CEC_GPDIR,  read_reg32(CEC_GPDIR)  & ~CEC_GPMASK);    // set to input mode
        write_reg32(CEC_GPDATO, read_reg32(CEC_GPDATO) & ~CEC_GPMASK);    // set to out val to 0
        write_reg32(CEC_GPDP,   read_reg32(CEC_GPDP)   |  CEC_GPMASK);    // set assert falg to high level
        write_reg32(CEC_GPIE,   read_reg32(CEC_GPIE)   |  CEC_GPMASK);    // enable interrupt
        
        header   = (((addr<<4) | addr)<<1) | 1;
        state    = GCEC_ST_START;
        sstate   = 0;
        enable   = 1;
        enable_counter(1);
    }
    
    return 0;  
}




/*------------------------------------------------------------------
 * Func : gcec_fixup_disable
 *
 * Desc : disable gcec fixup
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others : failed
 *------------------------------------------------------------------*/
int gcec_fixup_disable(void)
{
    if (!init_rdy)
        return -1;
        
    if (enable)
    {
        gcec_dbg("[CEC] gcec_fixup_disabled\n");
        write_reg32(CEC_GPDIR,  read_reg32(CEC_GPDIR)  & ~CEC_GPMASK);    // set to input mode
        write_reg32(CEC_GPDATO, read_reg32(CEC_GPDATO) & ~CEC_GPMASK);    // set to out val to 0
        write_reg32(CEC_GPDP,   read_reg32(CEC_GPDP)   |  CEC_GPMASK);    // set assert falg to high level
        write_reg32(CEC_GPIE,   read_reg32(CEC_GPIE)   & ~CEC_GPMASK);    // disable interrupt
        
        enable = 0;
        state  = GCEC_ST_START;
        sstate = 0;
        enable_counter(0);
    }
    
    return 0;
}




int __sleep gcec_wait_signal(unsigned char lvl, unsigned int min, unsigned int max)
{
    unsigned long clv = 0;
	int i;
	
    while(read_c0_count() < max)
	{
	    clv = read_reg32(CEC_GPDATI) & CEC_GPMASK;
	    if ((lvl && clv) || (!lvl && !clv))
	        return (read_c0_count()< min) ? read_c0_count() : 0;
	            
        for(i=0; i<100; i++);          	            
	}
	
	return read_c0_count(); 
}



#define STANDBY_CLK_MHZ             27                 
#define STANDBY_ST_RISING_MIN       ((3500 * STANDBY_CLK_MHZ) >> 1)         
#define STANDBY_ST_RISING_MAX       ((3900 * STANDBY_CLK_MHZ) >> 1)
#define STANDBY_ST_FALLING_MIN      ((4300 * STANDBY_CLK_MHZ) >> 1)        
#define STANDBY_ST_FALLING_MAX      ((4700 * STANDBY_CLK_MHZ) >> 1)
#define STANDBY_DA0_RISING_MIN      ((1300 * STANDBY_CLK_MHZ) >> 1)
#define STANDBY_DA0_RISING_NORMAL   ((1500 * STANDBY_CLK_MHZ) >> 1)    
#define STANDBY_DA0_RISING_MAX      ((1700 * STANDBY_CLK_MHZ) >> 1)
#define STANDBY_DA1_RISING_MIN      (( 400 * STANDBY_CLK_MHZ) >> 1)        
#define STANDBY_DA1_RISING_MAX      (( 800 * STANDBY_CLK_MHZ) >> 1)
#define STANDBY_DA_FALLING_MIN      ((2050 * STANDBY_CLK_MHZ) >> 1)        
#define STANDBY_DA_FALLING_MAX      ((2750 * STANDBY_CLK_MHZ) >> 1)



/*------------------------------------------------------------------
 * Func : gcec_fixup_standy_work
 *
 * Desc : 
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others : failed
 *------------------------------------------------------------------*/
int __sleep gcec_fixup_standy_work(void)
{   
    int i; 
    int ret = 0;
    
    if (header==0)  // do nothing if header is 0 (like : no iso gpio associated)
        return -1;
    
    if ((read_reg32(GCEC_ISR) & GCEC_ISRMASK)==0)
        return ret;
    
    // write_reg32(GCEC_ISR, GCEC_ISRMASK);        // clear interrupt 

    if ((read_reg32(CEC_GPDATI) & CEC_GPMASK)==0)   // exit if not data low ?
    {            
        write_c0_count(0);
        write_c0_compare(0xffffffff);        
    	write_c0_cause(read_c0_cause() & ~0x08000000);      // enable cpu counter
        
        // check start
        ret = gcec_wait_signal(1, 0, STANDBY_ST_RISING_MAX);
        if (ret)             // wait start high
            return (ret & 0xFFFFFF)<<8 | 0x0F;
        
        write_c0_count(0);
        
        ret = gcec_wait_signal(0, STANDBY_ST_FALLING_MIN - STANDBY_ST_RISING_MAX, STANDBY_ST_FALLING_MAX - STANDBY_ST_RISING_MIN);
        
        if (ret)             // wait start low
            return (ret & 0xFFFFFF)<<8 | 0x1F;
        
        // check header
        for (i=8; i>=0; i--)
        {           
            write_c0_count(0); 

            if ((header >> i) & 0x1)                
                ret = gcec_wait_signal(1, STANDBY_DA1_RISING_MIN, STANDBY_DA1_RISING_MAX);
            else
                ret = gcec_wait_signal(1, STANDBY_DA0_RISING_MIN, STANDBY_DA0_RISING_MAX);
            
            if (ret)        
                return ((ret & 0xFFFFFF)<<8) | i;
            
            ret = gcec_wait_signal(0, STANDBY_DA_FALLING_MIN, STANDBY_DA_FALLING_MAX);
            
            if (ret)
                return ((ret & 0xFFFFFF)<<8) | 0x10 | i;
        }
                    
        // send ack        
        write_c0_count(0);
        
        ret = gcec_wait_signal(1, STANDBY_DA1_RISING_MIN, STANDBY_DA1_RISING_MIN);
        
        write_reg32(CEC_GPDATO, read_reg32(CEC_GPDATO) & ~CEC_GPMASK);    // set to out val to 0
        write_reg32(CEC_GPDIR,  read_reg32(CEC_GPDIR)  |  CEC_GPMASK);    // set to output mode
        
        while(read_c0_count() < STANDBY_DA0_RISING_NORMAL) 
            for (i=0; i<100; i++);                      // add delay       
        
        write_reg32(CEC_GPDIR,  read_reg32(CEC_GPDIR)  & ~CEC_GPMASK);    // set to input mode    
end_proc:     
            
        write_c0_count(0);
        write_c0_cause(read_c0_cause() | 0x08000000);      // diable cpu counter
    }
    
    return 0xA;
}



/*------------------------------------------------------------------
 * Func : gcec_fixup_suspend
 *
 * Desc : enter gcec suspend mode
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others : failed
 *------------------------------------------------------------------*/
int gcec_fixup_suspend(unsigned char addr)
{
    if (!init_rdy)
        return 0;
    
    if (iso)
    {                       
        gcec_fixup_disable();       // disable kgpio irq first
                   
        header = (((addr<<4) | addr)<<1) | 1;
        
        printk("GCEC fixup suspend(pattern=%02x)\n", header>>1);
        
        // set up interrupt
        write_reg32(CEC_GPDIR,  read_reg32(CEC_GPDIR)  & ~CEC_GPMASK);    // set to input mode
        write_reg32(CEC_GPDATO, read_reg32(CEC_GPDATO) & ~CEC_GPMASK);    // set to out val to 0
        write_reg32(CEC_GPDP,   read_reg32(CEC_GPDP)   & ~CEC_GPMASK);    // set assert falg to low active
        write_reg32(CEC_GPIE,   read_reg32(CEC_GPIE)   |  CEC_GPMASK);    // enable interrupt
        
        enable = 1;
    }
    else
    {
        header = 0;
        printk("GCEC fixup can not work without ISO GPIO\n");
    }
    
    return 0;
}




/*------------------------------------------------------------------
 * Func : gcec_fixup_suspend
 *
 * Desc : enter gcec suspend mode
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others : failed
 *------------------------------------------------------------------*/
int gcec_fixup_resume(void)
{   
    if (!init_rdy)
        return 0;

    printk("GCEC fixup resume\n");
             
    if (enable)
    {    
        // disable gcec
        gcec_fixup_disable();
    }
    
    return 0;
}



/*------------------------------------------------------------------
 * Func : gcec_module_init
 *
 * Desc : gcec init function
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static int __init gcec_module_init(void)
{                        
    char* param;
    
    param = parse_token(platform_info.system_parameters, "GCEC_GPIO");
    
    if (param>0)
    {
        if (*param=='I')
        {
            iso = 1;
            param++;
        }    
        else
            iso = 0;
        
        if (sscanf(param, "%u", &gpio_ch)!=1)
        {
            gcec_warning("unknown GCEC_GPIO parameter %s\n", (iso) ? param -1 : param);
            iso = DEFAULT_GPIO_TYPE;
            gpio_ch = DEFAULT_GPIO_CH;                
        }
    }
    
    gcec_fixup_init();
    
    return 0;
}



/*------------------------------------------------------------------
 * Func : gcec_module_exit
 *
 * Desc : gcec module exit function
 *
 * Parm : N/A
 *         
 * Retn : 0 : success, others fail  
 *------------------------------------------------------------------*/
static void __exit gcec_module_exit(void)
{            
    gcec_fixup_uninit();    
}


module_init(gcec_module_init);
module_exit(gcec_module_exit);

