#ifndef	VENUS_SERIAL_FLASH_CONTROLLER
#define VENUS_SERIAL_FLASH_CONTROLLER



/*
	for HWSD, the serial flash address started from 0xbfcfffff,
	so for 16Mbit SST Serial Flash, the address ranges from 0xbfb0-0000 - 0xbfcf-ffff
*/
/* 2 MBytes */
#define FLASH_ADDR_END	0xbfd00000
//#define FLASH_SIZE		0x00200000
//#define FLASH_BASE		((volatile unsigned char *)(FLASH_ADDR_END-FLASH_SIZE))


#define MD_PP_DATA_SIZE_SHIFT	8	/* 256 bytes */
#define MD_PP_DATA_SIZE		(1 << MD_PP_DATA_SIZE_SHIFT)

#define SFC_OPCODE		((volatile unsigned int *)0xb801a800)
#define SFC_CTL			((volatile unsigned int *)0xb801a804)
#define SFC_SCK			((volatile unsigned int *)0xb801a808)
#define SFC_CE			((volatile unsigned int *)0xb801a80c)
#define SFC_WP			((volatile unsigned int *)0xb801a810)
#define SFC_POS_LATCH	((volatile unsigned int *)0xb801a814)
// Below: Jupiter Only
#define SFC_WAIT_WR		((volatile unsigned int *)0xb801a818)
#define SFC_EN_WR		((volatile unsigned int *)0xb801a81c)
#define SFC_FAST_RD		((volatile unsigned int *)0xb801a820)
#define SFC_SCK_TAP		((volatile unsigned int *)0xb801a824)


// System Bridge II
#define SB2_WRAPPER_CTRL	((volatile unsigned int *)0xb801a018)

// Move Data Engine
#define MD_FDMA_DDR_SADDR	((volatile unsigned int *)0xb800b088)
#define MD_FDMA_FL_SADDR	((volatile unsigned int *)0xb800b08c)
#define MD_FDMA_CTRL2		((volatile unsigned int *)0xb800b090)
#define MD_FDMA_CTRL1		((volatile unsigned int *)0xb800b094)

#define RDID_MANUFACTURER_ID_MASK	0x000000FF
#define RDID_DEVICE_ID_1_MASK		0x0000FF00
#define RDID_DEVICE_ID_2_MASK		0x00FF0000
#define RDID_DEVICE_EID_1_MASK		0x000000FF
#define RDID_DEVICE_EID_2_MASK		0x0000FF00

#define RDID_MANUFACTURER_ID(id)	(id & RDID_MANUFACTURER_ID_MASK)
#define RDID_DEVICE_ID_1(id)		((id & RDID_DEVICE_ID_1_MASK) >> 8)
#define RDID_DEVICE_ID_2(id)		((id & RDID_DEVICE_ID_2_MASK) >> 16)
#define RDID_DEVICE_EID_1(id)		(id & RDID_DEVICE_EID_1_MASK)
#define RDID_DEVICE_EID_2(id)		((id & RDID_DEVICE_EID_2_MASK) >> 8)

#define MANUFACTURER_ID_SPANSION	0x01
#define MANUFACTURER_ID_STM			0x20
#define MANUFACTURER_ID_PMC			0x7f
#define MANUFACTURER_ID_SST			0xbf
#define MANUFACTURER_ID_MXIC		0xc2
#define MANUFACTURER_ID_EON			0x1c
#define MANUFACTURER_ID_WINBOND		0xef //add by alexchang
#define MANUFACTURER_ID_ESMT		0x8c //add by alexchang


#define VENUS_SFC_ATTR_NONE				0x00
#define VENUS_SFC_ATTR_SUPPORT_MD_PP	0x01
#define VENUS_SFC_ATTR_SUPPORT_DUAL_IO	0x02

#define VENUS_SFC_SMALL_PAGE_WRITE_MASK	0x3
#define VENUS_SFC_ATTR_SUPPORT_4BYTE_ADDR_MODE 0x4

typedef struct venus_sfc_info {
	struct semaphore	venus_sfc_lock;
	u8			manufacturer_id;
	u8			device_id1;
	u8			device_id2;
	u8			attr;
	u32			erase_size;
	u32			erase_opcode;
	struct mtd_info 	*mtd_info;
}venus_sfc_info_t;


#define HD_SEM_REG	(0xB801A000)
#define DUMMY_REG	(0xB801A604) 
#define READ_REG(reg )(*(volatile unsigned long *)(reg))
#define WRITE_REG(reg,val)	(*(volatile unsigned long *)(reg) = val)

#if  defined(CONFIG_REALTEK_USE_HWSEM_AS_SENTINEL )
#define SYS_REG_TRY_LOCK(delay)\
		do {\
				local_irq_disable();\
				if(READ_REG(HD_SEM_REG)==1)	\
				{\
					if(READ_REG(DUMMY_REG)==1)\
					{\
						WRITE_REG(DUMMY_REG,0);\
						WRITE_REG(HD_SEM_REG,0);\
						break;\
					}\
					WRITE_REG(HD_SEM_REG,0);\
				}\
				local_irq_enable();\
				udelay(delay);\
		    } while (1)

#define SYS_REG_TRY_UNLOCK\
		WRITE_REG(DUMMY_REG,1);\
		local_irq_enable();\

								

#else
#define SYS_REG_TRY_LOCK(delay) 
#define SYS_REG_TRY_UNLOCK	
#endif

#define ENDIAN_SWAP_U16(data)\
		(data>>8) | (data<<8)

#define ENDIAN_SWAP_U32(data)\
		(data>>24) |\
	    ((data<<8) & 0x00FF0000) |\
	    ((data>>8) & 0x0000FF00) |\
	    (data<<24)

#define ENDIAN_SWAP_U64(data)\
		(data>>56) |\
	    ((data<<40) & 0x00FF000000000000) |\
	    ((data<<24) & 0x0000FF0000000000) |\
	    ((data<<8)  & 0x000000FF00000000) |\
	    ((data>>8)  & 0x00000000FF000000) |\
	    ((data>>24) & 0x0000000000FF0000) |\
	    ((data>>40) & 0x000000000000FF00) |\
	    (data<<56)

#define SFC_4KB_ERASE \
	{\
		printk("SFC_4KB_ERASE\n");\
		sfc_info->attr		|= VENUS_SFC_ATTR_SUPPORT_MD_PP;\
		sfc_info->erase_size	= 0x1000;\
		sfc_info->erase_opcode	= 0x00000020;\
	}

#define SFC_64KB_ERASE \
	{\
		printk("SFC_64KB_ERASE\n");\
		sfc_info->attr		|= VENUS_SFC_ATTR_SUPPORT_MD_PP;\
		sfc_info->erase_size	= 0x10000;\
		sfc_info->erase_opcode	= 0x000000d8;\
	}

#define SFC_256KB_ERASE \
	{\
		printk("SFC_256KB_ERASE\n");\
		sfc_info->attr		|= VENUS_SFC_ATTR_SUPPORT_MD_PP;\
		sfc_info->erase_size	= 0x40000;\
		sfc_info->erase_opcode	= 0x000000d8;\
	}

#if defined(CONFIG_USE_SFC_SYNC )
	#define SFC_SYNC	__asm__ __volatile__ ("sync;")
#else
    #define SFC_SYNC
#endif


#endif

