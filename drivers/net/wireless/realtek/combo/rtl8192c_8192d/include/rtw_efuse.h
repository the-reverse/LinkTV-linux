/******************************************************************************
 *
 * Copyright(c) 2007 - 2010 Realtek Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
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
u8	efuse_GetCurrentSize(PADAPTER padapter, u16 *size);
u16	efuse_GetMaxSize(PADAPTER padapter);
u8	efuse_access(PADAPTER padapter, u8 bRead, u16 start_addr, u16 cnts, u8 *data);
u8	efuse_map_read(PADAPTER padapter, u16 addr, u16 cnts, u8 *data);
u8	efuse_map_write(PADAPTER padapter, u16 addr, u16 cnts, u8 *data);

u8	EFUSE_Read1Byte(PADAPTER pAdapter, u16 Address);
void EFUSE_ShadowMapUpdate(PADAPTER pAdapter);
void	EFUSE_ShadowRead(PADAPTER pAdapter, u8 Type, 	u16 Offset, u32 *Value);

#endif
