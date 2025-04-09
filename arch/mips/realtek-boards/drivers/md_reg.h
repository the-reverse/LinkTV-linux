#ifndef SMQ_REG_H
#define SMQ_REG_H


#define BIT0 0x01
#define BIT1 0x02
#define BIT2 0x04
#define BIT3 0x08
#define BIT4 0x10
#define BIT5 0x20
#define BIT6 0x40
#define BIT7 0x80

#define SMQ_COMMAND_ENTRIES 4096
#define INST_CNT_MASK 0xFFFF

#define SMQ_REG_BASE        0xB800B000  //TBD

#define IOA_SMQ_CNTL_REG         ((volatile unsigned int *)0xB800B000)
#define IOA_SMQ_INT_STATUS_REG   ((volatile unsigned int *)0xB800B004)
#define IOA_SMQ_INT_ENABLE_REG   ((volatile unsigned int *)0xB800B008)
#define IOA_SMQ_CmdBase_REG      ((volatile unsigned int *)0xB800B00C)
#define IOA_SMQ_CmdLimit_REG     ((volatile unsigned int *)0xB800B010)
#define IOA_SMQ_CmdRdptr_REG     ((volatile unsigned int *)0xB800B014)
#define IOA_SMQ_CmdWrptr_REG     ((volatile unsigned int *)0xB800B018)
#define IOA_SMQ_CmdFifoState_REG ((volatile unsigned int *)0xB800B01C)
#define IOA_SMQ_INST_CNT_REG     ((volatile unsigned int *)0xB800B020)

#define IOA_AMQ_CNTL_REG         ((volatile unsigned int *)0xB800B060)
#define IOA_AMQ_INT_STATUS_REG   ((volatile unsigned int *)0xB800B064)
#define IOA_AMQ_INT_ENABLE_REG   ((volatile unsigned int *)0xB800B068)
#define IOA_AMQ_CmdBase_REG      ((volatile unsigned int *)0xB800B06C)
#define IOA_AMQ_CmdLimit_REG     ((volatile unsigned int *)0xB800B070)
#define IOA_AMQ_CmdRdptr_REG     ((volatile unsigned int *)0xB800B074)
#define IOA_AMQ_CmdWrptr_REG     ((volatile unsigned int *)0xB800B078)
#define IOA_AMQ_CmdFifoState_REG ((volatile unsigned int *)0xB800B07C)
#define IOA_AMQ_INST_CNT_REG     ((volatile unsigned int *)0xB800B080)


#define SMQ_CLR_WRITE_DATA 0
#define SMQ_SET_WRITE_DATA 1

#define MOVE_DATA_SS 0x05
#define MOVE_DATA_SB 0x15
#define MOVE_DATA_BS 0x25

//se control bits
typedef enum {
    SMQ_GO=BIT1,
    SMQ_CMD_SWAP=BIT2,
    SMQ_IDLE=BIT3,
} SE_CTRL;

#endif

