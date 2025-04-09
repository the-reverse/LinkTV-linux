#ifndef __JUPITER_CEC_REG_H__
#define __JUPITER_CEC_REG_H__
                                                                                     
#define ISO_ISR                 0xb801bd00
//*** ISO_ISR ***
#define CEC_RX_INT             (0x00000001<<23)
#define CEC_TX_INT             (0x00000001<<22)
                                                                 
#define ISO_CEC_CR0             0xb801BE80
#define ISO_CEC_CR1             0xb801BE84
#define ISO_CEC_CR2             0xb801BE88
#define ISO_CEC_CR3             0xb801BE8C

#define ISO_CEC_RT0             0xb801BE90                       
#define ISO_CEC_RT1             0xb801BE94                       
#define ISO_CEC_RX0             0xb801BE98                       
#define ISO_CEC_RX1             0xb801BE9C                       
#define ISO_CEC_TX0             0xb801BEA0                       
#define ISO_CEC_TX1             0xb801BEA4                       
#define ISO_CEC_TX_FIFO         0xb801BEA8                       
#define ISO_CEC_RX_FIFO         0xb801BEAC                       
#define ISO_CEC_RX_START0       0xb801BEB0                       
#define ISO_CEC_RX_START1       0xb801BEB4                       
#define ISO_CEC_RX_DATA0        0xb801BEB8                       
#define ISO_CEC_RX_DATA1        0xb801BEBC                       
#define ISO_CEC_TX_START0       0xb801BEC0                       
#define ISO_CEC_TX_START1       0xb801BEC4                       
#define ISO_CEC_TX_DATA0        0xb801BEC8                       
#define ISO_CEC_TX_DATA1        0xb801BECC                       
#define ISO_CEC_TX_DATA2        0xb801BED0                       

#endif  //__JUPITER_CEC_REG_H__

