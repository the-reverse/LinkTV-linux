/* DO NOT TOUCH, AUTOGENERATED BY OFFSET.C */

#ifndef _MIPS_OFFSET_H
#define _MIPS_OFFSET_H

/* MIPS pt_regs offsets. */
#define PT_R0     24
#define PT_R1     28
#define PT_R2     32
#define PT_R3     36
#define PT_R4     40
#define PT_R5     44
#define PT_R6     48
#define PT_R7     52
#define PT_R8     56
#define PT_R9     60
#define PT_R10    64
#define PT_R11    68
#define PT_R12    72
#define PT_R13    76
#define PT_R14    80
#define PT_R15    84
#define PT_R16    88
#define PT_R17    92
#define PT_R18    96
#define PT_R19    100
#define PT_R20    104
#define PT_R21    108
#define PT_R22    112
#define PT_R23    116
#define PT_R24    120
#define PT_R25    124
#define PT_R26    128
#define PT_R27    132
#define PT_R28    136
#define PT_R29    140
#define PT_R30    144
#define PT_R31    148
#define PT_LO     152
#define PT_HI     156
#define PT_EPC    160
#define PT_BVADDR 164
#define PT_STATUS 168
#define PT_CAUSE  172
#define PT_SIZE   176

/* MIPS task_struct offsets. */
#define TASK_STATE         0
#define TASK_COUNTER       52
#define TASK_PRIORITY      56
#define TASK_FLAGS         4
#define TASK_SIGPENDING    8
#define TASK_MM            920

/* MIPS specific thread_struct offsets. */
#define THREAD_REG16   568
#define THREAD_REG17   572
#define THREAD_REG18   576
#define THREAD_REG19   580
#define THREAD_REG20   584
#define THREAD_REG21   588
#define THREAD_REG22   592
#define THREAD_REG23   596
#define THREAD_REG29   600
#define THREAD_REG30   604
#define THREAD_REG31   608
#define THREAD_STATUS  612
#define THREAD_FPU     616
#define THREAD_BVADDR  880
#define THREAD_ECODE   884
#define THREAD_TRAPNO  888
#define THREAD_PGDIR   892
#define THREAD_MFLAGS  896
#define THREAD_CURDS   900
#define THREAD_TRAMP   904
#define THREAD_OLDCTX  908

/* Linux mm_struct offsets. */
#define MM_COUNT      12
#define MM_PGD        8
#define MM_CONTEXT    32

/* Linux sigcontext offsets. */
#define SC_REGMASK    0
#define SC_STATUS     4
#define SC_PC         8
#define SC_REGS       16
#define SC_FPREGS     272
#define SC_OWNEDFP    528
#define SC_FPC_CSR    532
#define SC_FPC_EIR    536
#define SC_SSFLAGS    540
#define SC_MDHI       544
#define SC_MDLO       552
#define SC_CAUSE      560
#define SC_BADVADDR   564
#define SC_SIGSET     568

#endif /* !(_MIPS_OFFSET_H) */
