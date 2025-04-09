/*
 * Register Address for Venus/Neptune
 */
#define MIS_ISR				((volatile unsigned int *)0xb801b00c)
#define MIS_UMSK_ISR		((volatile unsigned int *)0xb801b008)
#define MIS_UMSK_ISR_GP0A	((volatile unsigned int *)0xb801b010)
#define MIS_UMSK_ISR_GP0DA	((volatile unsigned int *)0xb801b014)
#define MIS_UMSK_ISR_GP1A	((volatile unsigned int *)0xb801b038)
#define MIS_UMSK_ISR_GP1DA	((volatile unsigned int *)0xb801b03c)

#define MIS_GP0DIR		((volatile unsigned int *)0xb801b100)
#define MIS_GP1DIR		((volatile unsigned int *)0xb801b104)
#define MIS_GP0DATO		((volatile unsigned int *)0xb801b108)
#define MIS_GP1DATO		((volatile unsigned int *)0xb801b10c)
#define MIS_GP0DATI		((volatile unsigned int *)0xb801b110)
#define MIS_GP1DATI		((volatile unsigned int *)0xb801b114)
#define MIS_GP0IE		((volatile unsigned int *)0xb801b118)
#define MIS_GP1IE		((volatile unsigned int *)0xb801b11c)
#define MIS_GP0DP		((volatile unsigned int *)0xb801b120)
#define MIS_GP1DP		((volatile unsigned int *)0xb801b124)
#define MIS_GPDEB		((volatile unsigned int *)0xb801b128)

/*
 * Register Address for Mars
 */
#define MARS_MIS_ISR              ((volatile unsigned int *)0xB801B00C)
#define MARS_MIS_UMSK_ISR         ((volatile unsigned int *)0xB801B008)
#define MARS_MIS_UMSK_ISR_GP0A    ((volatile unsigned int *)0xB801B040)
#define MARS_MIS_UMSK_ISR_GP1A    ((volatile unsigned int *)0xB801B044)
#define MARS_MIS_UMSK_ISR_GP2A    ((volatile unsigned int *)0xB801B048)
#define MARS_MIS_UMSK_ISR_GP3A    ((volatile unsigned int *)0xB801B04C)
#define MARS_MIS_UMSK_ISR_GP0DA   ((volatile unsigned int *)0xB801B050)
#define MARS_MIS_UMSK_ISR_GP1DA   ((volatile unsigned int *)0xB801B054)
#define MARS_MIS_UMSK_ISR_GP2DA   ((volatile unsigned int *)0xB801B058)
#define MARS_MIS_UMSK_ISR_GP3DA   ((volatile unsigned int *)0xB801B05C)

#define MARS_MIS_GP0DIR	    ((volatile unsigned int *)0xB801B100)
#define MARS_MIS_GP1DIR     ((volatile unsigned int *)0xB801B104)
#define MARS_MIS_GP2DIR     ((volatile unsigned int *)0xB801B108)
#define MARS_MIS_GP3DIR     ((volatile unsigned int *)0xB801B10C)
#define MARS_MIS_GP0DATO    ((volatile unsigned int *)0xB801B110)
#define MARS_MIS_GP1DATO    ((volatile unsigned int *)0xB801B114)
#define MARS_MIS_GP2DATO    ((volatile unsigned int *)0xB801B118)
#define MARS_MIS_GP3DATO    ((volatile unsigned int *)0xB801B11C)
#define MARS_MIS_GP0DATI    ((volatile unsigned int *)0xB801B120)
#define MARS_MIS_GP1DATI    ((volatile unsigned int *)0xB801B124)
#define MARS_MIS_GP2DATI    ((volatile unsigned int *)0xB801B128)
#define MARS_MIS_GP3DATI    ((volatile unsigned int *)0xB801B12C)
#define MARS_MIS_GP0IE      ((volatile unsigned int *)0xB801B130)
#define MARS_MIS_GP1IE      ((volatile unsigned int *)0xB801B134)
#define MARS_MIS_GP2IE      ((volatile unsigned int *)0xB801B138)
#define MARS_MIS_GP3IE      ((volatile unsigned int *)0xB801B13C)
#define MARS_MIS_GP0DP      ((volatile unsigned int *)0xB801B140)
#define MARS_MIS_GP1DP      ((volatile unsigned int *)0xB801B144)
#define MARS_MIS_GP2DP      ((volatile unsigned int *)0xB801B148)
#define MARS_MIS_GP3DP      ((volatile unsigned int *)0xB801B14C)
#define MARS_MIS_GPDEB      ((volatile unsigned int *)0xB801B150)


/*
 * Register Address for Jupiter
 */
#define JUPITER_MIS_ISR              ((volatile unsigned int *)0xB801B00C)
#define JUPITER_MIS_UMSK_ISR         ((volatile unsigned int *)0xB801B008)
#define JUPITER_MIS_UMSK_ISR_GP0A    ((volatile unsigned int *)0xB801B040)
#define JUPITER_MIS_UMSK_ISR_GP1A    ((volatile unsigned int *)0xB801B044)
#define JUPITER_MIS_UMSK_ISR_GP2A    ((volatile unsigned int *)0xB801B048)
#define JUPITER_MIS_UMSK_ISR_GP3A    ((volatile unsigned int *)0xB801B04C)
#define JUPITER_MIS_UMSK_ISR_GP4A    ((volatile unsigned int *)0xB801B050)
#define JUPITER_MIS_UMSK_ISR_GP0DA   ((volatile unsigned int *)0xB801B054)
#define JUPITER_MIS_UMSK_ISR_GP1DA   ((volatile unsigned int *)0xB801B058)
#define JUPITER_MIS_UMSK_ISR_GP2DA   ((volatile unsigned int *)0xB801B05C)
#define JUPITER_MIS_UMSK_ISR_GP3DA   ((volatile unsigned int *)0xB801B060)
#define JUPITER_MIS_UMSK_ISR_GP4DA   ((volatile unsigned int *)0xB801B064)

#define JUPITER_MIS_GP0DIR		MARS_MIS_GP0DIR
#define JUPITER_MIS_GP1DIR		MARS_MIS_GP1DIR
#define JUPITER_MIS_GP2DIR		MARS_MIS_GP2DIR
#define JUPITER_MIS_GP3DIR		MARS_MIS_GP3DIR
#define JUPITER_MIS_GP0DATO		MARS_MIS_GP0DATO
#define JUPITER_MIS_GP1DATO		MARS_MIS_GP1DATO
#define JUPITER_MIS_GP2DATO		MARS_MIS_GP2DATO
#define JUPITER_MIS_GP3DATO		MARS_MIS_GP3DATO
#define JUPITER_MIS_GP0DATI		MARS_MIS_GP0DATI
#define JUPITER_MIS_GP1DATI		MARS_MIS_GP1DATI
#define JUPITER_MIS_GP2DATI		MARS_MIS_GP2DATI
#define JUPITER_MIS_GP3DATI		MARS_MIS_GP3DATI
#define JUPITER_MIS_GP0IE		MARS_MIS_GP0IE
#define JUPITER_MIS_GP1IE		MARS_MIS_GP1IE
#define JUPITER_MIS_GP2IE		MARS_MIS_GP2IE
#define JUPITER_MIS_GP3IE		MARS_MIS_GP3IE
#define JUPITER_MIS_GP0DP		MARS_MIS_GP0DP
#define JUPITER_MIS_GP1DP		MARS_MIS_GP1DP
#define JUPITER_MIS_GP2DP		MARS_MIS_GP2DP
#define JUPITER_MIS_GP3DP		MARS_MIS_GP3DP
#define JUPITER_MIS_GPDEB		MARS_MIS_GPDEB

#define JUPITER_ISO_ISR                 ((volatile unsigned int *)0xB801BD00)
#define JUPITER_ISO_UMSK_ISR            ((volatile unsigned int *)0xB801BD04)
#define JUPITER_ISO_UMSK_ISR_GP         ((volatile unsigned int *)0xB801BD08)
#define JUPITER_ISO_GPDIR               ((volatile unsigned int *)0xB801BD70)
#define JUPITER_ISO_GPDATO              ((volatile unsigned int *)0xB801BD74)
#define JUPITER_ISO_GPDATI              ((volatile unsigned int *)0xB801BD78)
#define JUPITER_ISO_GPIE                ((volatile unsigned int *)0xB801BD7C)
#define JUPITER_ISO_GPDP                ((volatile unsigned int *)0xB801BD80)
#define JUPITER_ISO_GPDEB               ((volatile unsigned int *)0xB801BD84)

#define VENUS_GPIO_IRQ		3       /* HW1 +2 = 3 */

#define VENUS_GPIO_IOC_MAGIC                    'r'
#define VENUS_GPIO_ENABLE_INT                   _IOW(VENUS_GPIO_IOC_MAGIC, 1, int)
#define VENUS_GPIO_DISABLE_INT                  _IOW(VENUS_GPIO_IOC_MAGIC, 2, int)
#define VENUS_GPIO_SET_ASSERT                   _IOW(VENUS_GPIO_IOC_MAGIC, 3, int)
#define VENUS_GPIO_SET_DEASSERT                 _IOW(VENUS_GPIO_IOC_MAGIC, 4, int)
#define VENUS_GPIO_SET_SW_DEBOUNCE              _IOW(VENUS_GPIO_IOC_MAGIC, 5, int) 
#define VENUS_GPIO_CHOOSE_GPDEB1                _IOW(VENUS_GPIO_IOC_MAGIC, 6, int)
#define VENUS_GPIO_CHOOSE_GPDEB2                _IOW(VENUS_GPIO_IOC_MAGIC, 7, int)
#define VENUS_GPIO_CHOOSE_GPDEB3                _IOW(VENUS_GPIO_IOC_MAGIC, 8, int)
#define VENUS_GPIO_CHOOSE_GPDEB4                _IOW(VENUS_GPIO_IOC_MAGIC, 9, int)
#define VENUS_GPIO_RESET_INT_STATUS             _IO(VENUS_GPIO_IOC_MAGIC, 10)
#define VENUS_GPIO_CHOOSE_GPDEB5                _IOW(VENUS_GPIO_IOC_MAGIC,11, int)
#define VENUS_GPIO_CHOOSE_GPDEB6                _IOW(VENUS_GPIO_IOC_MAGIC,12, int)
#define VENUS_GPIO_CHOOSE_GPDEB7                _IOW(VENUS_GPIO_IOC_MAGIC,13, int)
#define VENUS_GPIO_CHOOSE_GPDEB8                _IOW(VENUS_GPIO_IOC_MAGIC,14, int)
#define VENUS_ISO_GPIO_ENABLE_INT               _IOW(VENUS_GPIO_IOC_MAGIC,15, int)
#define VENUS_ISO_GPIO_DISABLE_INT              _IOW(VENUS_GPIO_IOC_MAGIC,16, int)
#define VENUS_ISO_GPIO_SET_ASSERT               _IOW(VENUS_GPIO_IOC_MAGIC,17, int)
#define VENUS_ISO_GPIO_SET_DEASSERT             _IOW(VENUS_GPIO_IOC_MAGIC,18, int)
#define VENUS_ISO_GPIO_SET_SW_DEBOUNCE          _IOW(VENUS_GPIO_IOC_MAGIC,19, int)
#define VENUS_ISO_GPIO_CHOOSE_GPDEB1            _IOW(VENUS_GPIO_IOC_MAGIC,20, int)
#define VENUS_ISO_GPIO_RESET_INT_STATUS         _IOW(VENUS_GPIO_IOC_MAGIC,21, int)

#define VENUS_GPIO_IOC_MAXNR	14

/* For Neptune IDE control. */
#define	ATA_PRIMARY_DIR		((volatile unsigned int *)0xb8000110)

