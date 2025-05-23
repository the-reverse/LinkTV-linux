#ifndef __RTW_EEPROM_H__
#define __RTW_EEPROM_H__

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#define	RTL8712_EEPROM_ID			0x8712
#define	EEPROM_MAX_SIZE			256
#define	CLOCK_RATE					50			//100us		

//- EEPROM opcodes
#define EEPROM_READ_OPCODE		06
#define EEPROM_WRITE_OPCODE		05
#define EEPROM_ERASE_OPCODE		07
#define EEPROM_EWEN_OPCODE		19      // Erase/write enable
#define EEPROM_EWDS_OPCODE		16      // Erase/write disable

//Country codes
#define USA							0x555320
#define EUROPE						0x1 //temp, should be provided later	
#define JAPAN						0x2 //temp, should be provided later

#ifdef CONFIG_SDIO_HCI
#define eeprom_cis0_sz	17
#define eeprom_cis1_sz	50
#endif

#define	EEPROM_CID_DEFAULT			0x0
#define	EEPROM_CID_ALPHA				0x1
#define	EEPROM_CID_Senao				0x3
#define	EEPROM_CID_NetCore				0x5
#define	EEPROM_CID_CAMEO				0X8
#define	EEPROM_CID_SITECOM				0x9
#define	EEPROM_CID_COREGA				0xB
#define	EEPROM_CID_EDIMAX_BELKIN		0xC
#define	EEPROM_CID_SERCOMM_BELKIN		0xE
#define	EEPROM_CID_CAMEO1				0xF
#define	EEPROM_CID_WNC_COREGA		0x12
#define	EEPROM_CID_CLEVO				0x13
#define	EEPROM_CID_WHQL				0xFE // added by chiyoko for dtm, 20090108

//
// Customer ID, note that: 
// This variable is initiailzed through EEPROM or registry, 
// however, its definition may be different with that in EEPROM for 
// EEPROM size consideration. So, we have to perform proper translation between them.
// Besides, CustomerID of registry has precedence of that of EEPROM.
// defined below. 060703, by rcnjko.
//
typedef enum _RT_CUSTOMER_ID
{
	RT_CID_DEFAULT = 0,
	RT_CID_8187_ALPHA0 = 1,
	RT_CID_8187_SERCOMM_PS = 2,
	RT_CID_8187_HW_LED = 3,
	RT_CID_8187_NETGEAR = 4,
	RT_CID_WHQL = 5,
	RT_CID_819x_CAMEO  = 6, 
	RT_CID_819x_RUNTOP = 7,
	RT_CID_819x_Senao = 8,
	RT_CID_TOSHIBA = 9,	// Merge by Jacken, 2008/01/31.
	RT_CID_819x_Netcore = 10,
	RT_CID_Nettronix = 11,
	RT_CID_DLINK = 12,
	RT_CID_PRONET = 13,
	RT_CID_COREGA = 14,
	RT_CID_819x_ALPHA = 15,
	RT_CID_819x_Sitecom = 16,
	RT_CID_CCX = 17, // It's set under CCX logo test and isn't demanded for CCX functions, but for test behavior like retry limit and tx report. By Bruce, 2009-02-17.
	RT_CID_819x_Lenovo = 18,
	RT_CID_819x_QMI = 19,
	RT_CID_819x_Edimax_Belkin = 20,		
	RT_CID_819x_Sercomm_Belkin = 21,			
	RT_CID_819x_CAMEO1 = 22,
	RT_CID_819x_MSI = 23,
	RT_CID_819x_Acer = 24,
	RT_CID_819x_AzWave_ASUS = 25,
	RT_CID_819x_AzWave = 26, // For AzWave in PCIe, The ID is AzWave use and not only Asus
	RT_CID_819x_HP = 27,
	RT_CID_819x_WNC_COREGA = 28,
	RT_CID_819x_Arcadyan_Belkin = 29,
	RT_CID_819x_SAMSUNG = 30,
	RT_CID_819x_CLEVO = 31,
	RT_CID_819x_DELL = 32,
	RT_CID_819x_PRONETS = 33,
}RT_CUSTOMER_ID, *PRT_CUSTOMER_ID;

struct eeprom_priv 
{    
	u8		bautoload_fail_flag;
	//u8		bempty;
	//u8		sys_config;
	u8		mac_addr[6];	//PermanentAddress
	//u8		config0;
	u16		channel_plan;
	//u8		country_string[3];	
	//u8		tx_power_b[15];
	//u8		tx_power_g[15];
	//u8		tx_power_a[201];

	u8		EepromOrEfuse;

	u8		efuse_eeprom_data[EEPROM_MAX_SIZE];

#ifdef CONFIG_SDIO_HCI
	u8		sdio_setting;	
	u32		ocr;
	u8		cis0[eeprom_cis0_sz];
	u8		cis1[eeprom_cis1_sz];	
#endif
};


extern void eeprom_write16(_adapter *padapter, u16 reg, u16 data);
extern u16 eeprom_read16(_adapter *padapter, u16 reg);
extern void read_eeprom_content(_adapter *padapter);
extern void eeprom_read_sz(_adapter * padapter, u16 reg,u8* data, u32 sz); 

extern void read_eeprom_content_by_attrib(_adapter *	padapter	);

#endif  //__RTL871X_EEPROM_H__
