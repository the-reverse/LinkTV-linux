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
#include "mmc_debug.h"

#include <linux/rtk_cr.h>


void rsp_rtk2mmc(u32 *raw)
{
    int i;
    u32 raw_tmp[4];

    for( i=0; i<3; i++){
        raw_tmp[i] = ( ((raw[i]>>8)&0xff)<<24 ) |  ( ((raw[i]>>16)&0xff)<<16 ) |
                            ( ((raw[i]>>24)&0xff)<<8 ) |  (raw[i+1]&0xff);
    }

    raw_tmp[3] =( ((raw[3]>>8)&0xff)<<24 ) | ( ((raw[3]>>16)&0xff)<<16 ) |
                         ( ((raw[3]>>24)&0xff)<<8 ) |  0xff;    //force CRC to 0xff

    memcpy(raw, raw_tmp, 16);

}
