#ifndef __RTW_EFUSE_H__
#define __RTW_EFUSE_H__

#include <drv_conf.h>
#include <osdep_service.h>


#define _REPEAT_THRESHOLD_	3

#define EFUSE_MAX_SIZE		512
#define EFUSE_MAP_MAX_SIZE	128

#define PGPKG_MAX_WORDS		4
#define PGPKT_DATA_SIZE		8	// PGPKG_MAX_WORDS*2; BYTES sizeof(u8)*8
#define MAX_PGPKT_SIZE		9	// 1 + PGPKT_DATA_SIZE; header + 2 * 4 words (BYTES)

#define GET_EFUSE_OFFSET(header)	((header & 0xF0) >> 4)
#define GET_EFUSE_WORD_EN(header)	(header & 0x0F)
#define MAKE_EFUSE_HEADER(offset, word_en)	(((offset & 0x0F) << 4) | (word_en & 0x0F))
//------------------------------------------------------------------------------
typedef struct PG_PKT_STRUCT{
	u8 offset;
	u8 word_en;
	u8 data[PGPKT_DATA_SIZE];
} PGPKT_STRUCT,*PPGPKT_STRUCT;
//------------------------------------------------------------------------------
u8	efuse_reg_init(_adapter *padapter);
void	efuse_reg_uninit(_adapter *padapter);
u16	efuse_get_current_size(_adapter *padapter);
int	efuse_get_max_size(_adapter *padapter);
void	efuse_change_max_size(_adapter *padapter);
u8	efuse_pg_packet_read(_adapter *padapter, u8 offset, u8 *data);
u8	efuse_pg_packet_write(_adapter *padapter, const u8 offset, const u8 word_en, const u8 *data);
u8	efuse_access(_adapter *padapter, u8 bRead, u16 start_addr, u16 cnts, u8 *data);
u8	efuse_map_read(_adapter *padapter, u16 addr, u16 cnts, u8 *data);
u8	efuse_map_write(_adapter *padapter, u16 addr, u16 cnts, u8 *data);
u8	efuse_one_byte_read(_adapter *padapter, u16 addr, u8 *data);

u8	EFUSE_Read1Byte(PADAPTER pAdapter, u16 Address);
void EFUSE_ShadowMapUpdate(PADAPTER pAdapter);
void	EFUSE_ShadowRead(PADAPTER pAdapter, u8 Type, 	u16 Offset, u32 *Value);

#endif
