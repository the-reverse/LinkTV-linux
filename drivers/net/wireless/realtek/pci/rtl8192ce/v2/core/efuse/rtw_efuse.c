/*
 * rtw_efuse.c
 *
 * Description :
 *
 * Author :
 *
* History :                                                          
*
*                                        
 *
 * Copyright 2008, Realtek Corp.
 *
 * The contents of this file is the sole property of Realtek Corp. It can not be
 * be used, copied or modified without written permission from Realtek Corp.
 *
 */
#define _RTW_EFUSE_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#include <rtw_efuse.h>
//------------------------------------------------------------------------------
//#define _PRE_EXECUTE_READ_CMD_
#if 0
#define PG_STATE_HEADER 	0x01
#define PG_STATE_DATA		0x20
#endif
//------------------------------------------------------------------------------
#define EF_FLAG						BIT(31)
#define REG_EFUSE_CTRL				0x0030
#define REG_EFUSE_TEST				0x0034
#define EFUSE_CTRL					REG_EFUSE_CTRL		// E-Fuse Control.
#define EFUSE_TEST					REG_EFUSE_TEST		// E-Fuse Test.

static int EFUSE_AVAILABLE_MAX_SIZE = EFUSE_MAX_SIZE - 3/*0x1FD*/; //reserve 3 bytes for HW stop read
#define EFUSE_CLK_CTRL	EFUSE_CTRL
//------------------------------------------------------------------------------
static void efuse_reg_ctrl(_adapter *padapter, u8 bPowerOn)
{
	struct mlme_priv *pmlmepriv = &padapter->mlmepriv;
	u8 tmpu8 = 0;

	if (_TRUE == bPowerOn)
	{
#if 0
		// -----------------SYS_FUNC_EN Digital Core Vdd enable ---------------------------------
		tmpu8 = read8(padapter, SYS_FUNC_EN + 1);
		if (!(tmpu8 & 0x20)) {
			tmpu8 |= 0x20;	// Loader Power Enable 
			write8(padapter, SYS_FUNC_EN + 1, tmpu8); // PWC_DV2LDR, 10250002:13
			msleep_os(10);
		}

		//EE Loader to Retention path1: attach 0x1[0]=0
		tmpu8 = read8(padapter, SYS_ISO_CTRL + 1);
		if (tmpu8 & 0x01) {
			tmpu8 &= 0xFE;
			write8(padapter, SYS_ISO_CTRL + 1, tmpu8); // iso_LDR2RP, 10250000:8
		}
#endif

		// -----------------e-fuse pwr & clk reg ctrl ---------------------------------
		// Enable LDOE25 Macro Block
		tmpu8 = read8(padapter, EFUSE_TEST + 3);
		tmpu8 |= 0x80;
		write8(padapter, EFUSE_TEST + 3, tmpu8);// 2.5v LDO, LDOE25_EN, 10250034:31
		mdelay_os(1);//for some platform , need some delay time
		// Change Efuse Clock for write action to 40MHZ
		//write8(padapter, EFUSE_CLK_CTRL, (read8(padapter, EFUSE_CLK_CTRL)|0x03));
		//write8(padapter, EFUSE_CLK_CTRL, 0x03);
		mdelay_os(10);//for some platform , need some delay time

#ifdef _PRE_EXECUTE_READ_CMD_
		if (efuse_one_byte_read(padapter, 0, &tmpu8) == _TRUE) {
			RT_TRACE(_module_rtl8712_efuse_c_,_drv_alert_,("efuse_reg_ctrl: read EFuse fail!!\n"));
		}
#endif
	}
	else {
		// -----------------e-fuse pwr & clk reg ctrl ---------------------------------
		// Disable LDOE25 Macro Block
		tmpu8 = read8(padapter, EFUSE_TEST + 3);
		tmpu8 &= 0x7F;
		write8(padapter, EFUSE_TEST + 3, tmpu8);

		// Change Efuse Clock for write action to 500K
		//write8(padapter, EFUSE_CLK_CTRL, read8(padapter, EFUSE_CLK_CTRL)&0xFE);
		//write8(padapter, EFUSE_CLK_CTRL, 0x02);
#if 0
		// -----------------SYS_FUNC_EN Digital Core Vdd disable ---------------------------------
		if (check_fwstate(pmlmepriv, WIFI_MP_STATE) == _FALSE) {
			write8(padapter, SYS_FUNC_EN+1,  read8(padapter,SYS_FUNC_EN+1)&0xDF);
		}
#endif
	}
}

/*
 * Before write E-Fuse, this function must be called.
 */
u8 efuse_reg_init(_adapter *padapter)
{
	//u8 value;

	efuse_reg_ctrl(padapter, _TRUE);

	// check if E-Fuse Clock Enable and E-Fuse Clock is 40M
	/*value = read8(padapter, EFUSE_CLK_CTRL);
	if (value != 0x03) {
		RT_TRACE(_module_rtl8712_efuse_c_, _drv_err_,("EFUSE_CLK_CTRL=0x%2x != 0x03", value));
		efuse_reg_uninit(padapter);
		return _FALSE;
	}*/

	return _TRUE;
}

void efuse_reg_uninit(_adapter *padapter)
{
	efuse_reg_ctrl(padapter, _FALSE);
}
//------------------------------------------------------------------------------
u8 efuse_one_byte_read(_adapter *padapter, u16 addr, u8 *data)
{
	u8 tmpidx = 0, bResult;

	// -----------------e-fuse reg ctrl ---------------------------------				
	write8(padapter, EFUSE_CTRL+1, (u8)(addr&0xFF)); //address
	write8(padapter, EFUSE_CTRL+2, ((u8)((addr>>8)&0x03)) | (read8(padapter, EFUSE_CTRL+2)&0xFC));	

	//write8(padapter, EFUSE_CTRL+3, 0x72);//read cmd

	// use default program time 5000 ns
	write32(padapter, EFUSE_CTRL, read32(padapter, EFUSE_CTRL)&( ~EF_FLAG));//read cmd

	// wait for complete
	while (!(0x80 & read8(padapter, EFUSE_CTRL+3)) && (tmpidx < 100))
		tmpidx++;

	if (tmpidx < 100) {
		*data = read8(padapter, EFUSE_CTRL);
		RT_TRACE(_module_rtl8712_efuse_c_,_drv_info_,("====Read Efuse addr=0x%x value=0x%x=====\n", addr, *data));
		bResult = _TRUE;
	} else {
		*data = 0xff;
		RT_TRACE(_module_rtl8712_efuse_c_,_drv_err_,("====Read Efuse fail!!! addr=0x%x=====\n", addr));
		bResult = _FALSE;
	}
	return bResult;
}
//------------------------------------------------------------------------------
static u8 efuse_one_byte_write(_adapter *padapter, u16 addr, u8 data)
{
	u8 tmpidx = 0, bResult;

	// -----------------e-fuse reg ctrl ---------------------------------
	write8(padapter, EFUSE_CTRL+1, (u8)(addr&0xFF)); //address
	write8(padapter, EFUSE_CTRL+2, ((u8)((addr>>8)&0x03)) | (read8(padapter, EFUSE_CTRL+2)&0xFC));
	write8(padapter, EFUSE_CTRL, data); //data
	//write8(padapter, EFUSE_CTRL+3, 0xF2); //write cmd
	// use default program time 5000 ns
	write32(padapter, EFUSE_CTRL, read32(padapter, EFUSE_CTRL)|EF_FLAG);//write cmd	

	// wait for complete
	while ((0x80 &  read8(padapter, EFUSE_CTRL+3)) && (tmpidx < 100))
		tmpidx++;

	if (tmpidx < 100) {
		RT_TRACE(_module_rtl8712_efuse_c_,_drv_info_,("====Write Efuse addr=0x%x value=0x%x=====\n", addr, data));
		bResult = _TRUE;
	} else {
		RT_TRACE(_module_rtl8712_efuse_c_,_drv_err_,("====Write Efuse fail!! addr=0x%x value=0x%x=====\n", addr, data));
		bResult = _FALSE;
	}

	return bResult;	
}
//------------------------------------------------------------------------------
static u8 efuse_one_byte_rw(_adapter *padapter, u8 bRead, u16 addr, u8 *data)
{
	u8 tmpidx = 0, tmpv8 = 0, bResult;
	
	// -----------------e-fuse reg ctrl ---------------------------------				
	write8(padapter, EFUSE_CTRL+1, (u8)(addr&0xFF)); //address
	tmpv8 = ((u8)((addr>>8)&0x03)) | (read8(padapter, EFUSE_CTRL+2)&0xFC);
	write8(padapter, EFUSE_CTRL+2, tmpv8);

	if (_TRUE == bRead) {
		write8(padapter, EFUSE_CTRL+3,  0x72);//read cmd		

		while (!(0x80 & read8(padapter, EFUSE_CTRL+3)) && (tmpidx < 100))
			tmpidx++;

		if (tmpidx < 100) {		
			*data = read8(padapter, EFUSE_CTRL);
			bResult = _TRUE;
		} else {
			*data = 0;
			RT_TRACE(_module_rtl8712_efuse_c_,_drv_err_,("====Read Efuse Fail!! addr=0x%x =====\n", addr));		
			bResult = _FALSE;
		}
	} else {
		write8(padapter, EFUSE_CTRL, *data);//data
		write8(padapter, EFUSE_CTRL+3, 0xF2);//write cmd

		while ((0x80 & read8(padapter, EFUSE_CTRL+3)) && (tmpidx < 100))
			tmpidx++;

		if (tmpidx < 100) {
			RT_TRACE(_module_rtl8712_efuse_c_,_drv_info_,("====Write Efuse addr=0x%x value=0x%x =====\n", addr, *data));
			bResult = _TRUE;
		} else {
			RT_TRACE(_module_rtl8712_efuse_c_,_drv_err_,("====Write Efuse Fail!! addr =0x%x value=0x%x =====\n", addr, *data));
			bResult = _FALSE;
		}
	}

	return bResult;	
}
//------------------------------------------------------------------------------
static u8 efuse_is_empty(_adapter *padapter, u8 *empty)
{
	u8 value, ret = _TRUE;

	// read one byte to check if E-Fuse is empty
	if (efuse_one_byte_rw(padapter, _TRUE, 0, &value) == _TRUE) {
		if (0xFF == value) *empty = _TRUE;
		else *empty = _FALSE;
	} else {
		// read fail
		RT_TRACE(_module_rtl871x_mp_ioctl_c_,_drv_emerg_,
			("efuse_is_empty: fail!!=====\n"));
		ret = _FALSE;
	}

	return ret;
}
//------------------------------------------------------------------------------
void efuse_change_max_size(_adapter *padapter)
{	
	u16 pre_pg_data_saddr = 0x1FB;
#if 1
	u16 i;
	u16 pre_pg_data_size = 5;
	u8 pre_pg_data[5];

	for (i = 0; i < pre_pg_data_size; i++)
		efuse_one_byte_read(padapter, pre_pg_data_saddr+i, &pre_pg_data[i]);

	if ((pre_pg_data[0] == 0x03) && (pre_pg_data[1] == 0x00) && (pre_pg_data[2] == 0x00) && 
		(pre_pg_data[3] == 0x00) && (pre_pg_data[4] == 0x0C)) {
		EFUSE_AVAILABLE_MAX_SIZE -= pre_pg_data_size;//0x1F8;
	}
#else
	u8 efuse_data;

	efuse_one_byte_read(padapter, pre_pg_data_saddr,&efuse_data);
	if(efuse_data != 0xFF){
		EFUSE_AVAILABLE_MAX_SIZE = 0x1F8; //reserve :3 bytes
	}	
	RT_TRACE(_module_rtl8712_efuse_c_,_drv_alert_,("efuse_change_max_size , EFUSE_MAX_SIZE = %d\n",EFUSE_AVAILABLE_MAX_SIZE));
#endif
	
}

int efuse_get_max_size(_adapter *padapter)
{
	return 	EFUSE_AVAILABLE_MAX_SIZE;
}
//------------------------------------------------------------------------------
static u8 calculate_word_cnts(const u8 word_en)
{
	u8 word_cnts = 0;
	u8 word_idx;

	for (word_idx = 0; word_idx < PGPKG_MAX_WORDS; word_idx++) {
		if (!(word_en & BIT(word_idx))) word_cnts++; // 0 : write enable		
	}
	return word_cnts;
}
//------------------------------------------------------------------------------
static void pgpacket_copy_data(const u8 word_en, const u8 *sourdata, u8 *targetdata)
{
	u8 tmpindex = 0;
	u8 word_idx, byte_idx;

	for (word_idx = 0; word_idx < PGPKG_MAX_WORDS; word_idx++) {
		if (!(word_en&BIT(word_idx))) {	
			byte_idx = word_idx * 2;
			targetdata[byte_idx] = sourdata[tmpindex++];
			targetdata[byte_idx + 1] = sourdata[tmpindex++];
		}
	}
}
//------------------------------------------------------------------------------
u16 efuse_get_current_size(_adapter *padapter)
{
	int bContinual = _TRUE;

	u16 efuse_addr = 0;
	u8 hoffset = 0, hworden = 0;
	u8 efuse_data, word_cnts = 0;

	while (bContinual && efuse_one_byte_read(padapter, efuse_addr, &efuse_data) &&
	       (efuse_addr < EFUSE_AVAILABLE_MAX_SIZE))
	{
		if (efuse_data != 0xFF) {
			hoffset = (efuse_data >> 4) & 0x0F;
			hworden =  efuse_data & 0x0F;							
			word_cnts = calculate_word_cnts(hworden);
			//read next header
			efuse_addr = efuse_addr + (word_cnts * 2) + 1;
		} else {
			bContinual = _FALSE ;			
		}
	}

	return efuse_addr;
}
//------------------------------------------------------------------------------
u8 efuse_pg_packet_read(_adapter *padapter, u8 offset, u8 *data)
{
#if 0
	u8 ReadState = PG_STATE_HEADER;	

	int bContinual = _TRUE;	
#endif
	u8 hoffset = 0, hworden = 0, word_cnts = 0;
	u16 efuse_addr = 0;
	u8 efuse_data;

	u8 tmpidx = 0;
	u8 tmpdata[PGPKT_DATA_SIZE];

	u8 ret = _TRUE;


	if (data == NULL) return _FALSE;
	if (offset > 0x0f) return _FALSE;

	_memset(data, 0xFF, sizeof(u8)*PGPKT_DATA_SIZE);
	_memset(tmpdata, 0xFF, sizeof(u8)*PGPKT_DATA_SIZE);		
	
#if 1
	while (efuse_addr < EFUSE_AVAILABLE_MAX_SIZE)
	{
		if (efuse_one_byte_read(padapter, efuse_addr, &efuse_data) == _TRUE)
		{
			if (efuse_data == 0xFF) break;

			hoffset = (efuse_data >> 4) & 0x0F;
			hworden =  efuse_data & 0x0F;									
			word_cnts = calculate_word_cnts(hworden);

			if (hoffset == offset) {
				_memset(tmpdata, 0xFF, PGPKT_DATA_SIZE);
				for (tmpidx = 0; tmpidx < word_cnts*2; tmpidx++) {
					if (efuse_one_byte_read(padapter, efuse_addr+1+tmpidx, &efuse_data) == _TRUE) {
						tmpdata[tmpidx] = efuse_data;
					} else ret = _FALSE;
				}
				pgpacket_copy_data(hworden, tmpdata, data);
			}

			efuse_addr += 1 + (word_cnts*2);
		} else {
			ret = _FALSE;
			break;
		}
	}
#else
	while (bContinual && (efuse_addr < EFUSE_AVAILABLE_MAX_SIZE))
	{
		//-------  Header Read -------------
		if (ReadState & PG_STATE_HEADER)
		{
			if (efuse_one_byte_read(padapter, efuse_addr, &efuse_data) && (efuse_data != 0xFF))
			{
				hoffset = (efuse_data >> 4) & 0x0F;
				hworden =  efuse_data & 0x0F;									
				word_cnts = calculate_word_cnts(hworden);
				
				if (hoffset == offset) {
					_memset(tmpdata, 0xFF, PGPKT_DATA_SIZE);
					for (tmpidx = 0; tmpidx < word_cnts*2; tmpidx++) {
						if (efuse_one_byte_read(padapter, efuse_addr+1+tmpidx, &efuse_data)) {
							tmpdata[tmpidx] = efuse_data;
						}
					}
					ReadState = PG_STATE_DATA;
				} else {//read next header	
					efuse_addr = efuse_addr + (word_cnts*2) + 1;
					ReadState = PG_STATE_HEADER; 
				}
			} else {
				bContinual = _FALSE ;
			}
		}		
		//-------  Data section Read -------------
		else if (ReadState & PG_STATE_DATA) {
			pgpacket_copy_data(hworden, tmpdata, data);
			efuse_addr = efuse_addr + (word_cnts*2) + 1;
			ReadState = PG_STATE_HEADER; 
		}
	}			

	if(	(data[0]==0xff) &&(data[1]==0xff) && (data[2]==0xff)  && (data[3]==0xff) &&
		(data[4]==0xff) &&(data[5]==0xff) && (data[6]==0xff)  && (data[7]==0xff))
		ret = _FALSE;
	else
		ret = _TRUE;
#endif

	return ret;
}
//------------------------------------------------------------------------------
static u8 pgpacket_write_data(_adapter *padapter, const u16 efuse_addr, const u8 word_en, const u8 *data)
{		
	u16 start_addr = efuse_addr;

	u8 badworden = 0x0F;

	u8 word_idx, byte_idx;

	u16 tmpaddr = 0;
	u8 tmpdata[PGPKT_DATA_SIZE];


	_memset(tmpdata, 0xff, PGPKT_DATA_SIZE);

	for (word_idx = 0; word_idx < PGPKG_MAX_WORDS; word_idx++) {
		if (!(word_en & BIT(word_idx))) {
			tmpaddr = start_addr;
			byte_idx = word_idx * 2;
			efuse_one_byte_write(padapter, start_addr++, data[byte_idx]);
			efuse_one_byte_write(padapter, start_addr++, data[byte_idx + 1]);

			efuse_one_byte_read(padapter, tmpaddr, &tmpdata[byte_idx]);
			efuse_one_byte_read(padapter, tmpaddr + 1, &tmpdata[byte_idx+1]);
			if ((data[byte_idx] != tmpdata[byte_idx]) || (data[byte_idx+1] != tmpdata[byte_idx+1])) {
				badworden &= (~BIT(word_idx));
			}
		}
	}

	return badworden;
}
//------------------------------------------------------------------------------
static u8 fix_header(_adapter *padapter, u8 header, u16 header_addr)
{
	PGPKT_STRUCT pkt;
	u8 offset, word_en, value;
	u16 addr;

	int i;

	u8 ret = _TRUE;

	pkt.offset = GET_EFUSE_OFFSET(header);
	pkt.word_en = GET_EFUSE_WORD_EN(header);

	addr = header_addr + 1 + calculate_word_cnts(pkt.word_en)*2;
	if (addr > EFUSE_AVAILABLE_MAX_SIZE)
		return _FALSE;

	// retrieve original data
	addr = 0;
	while (addr < header_addr) {
		if (efuse_one_byte_read(padapter, addr++, &value) == _FALSE) {
			ret = _FALSE;
			break;
		}

		offset = GET_EFUSE_OFFSET(value);
		word_en = GET_EFUSE_WORD_EN(value);

		if (pkt.offset == offset) {
			for (i = 0; i < PGPKG_MAX_WORDS; i++) {
				if (BIT(i) & word_en) {
					if (BIT(i) & pkt.word_en) {
						if (efuse_one_byte_read(padapter, addr, &value) == _TRUE)
							pkt.data[i*2] = value;
						else return _FALSE;

						if (efuse_one_byte_read(padapter, addr+1, &value) == _TRUE)
							pkt.data[i*2 + 1] = value;
						else return _FALSE;
					}
					addr += 2;
				}
			}
		} else
			addr += calculate_word_cnts(word_en)*2;
	}

	if (addr == header_addr) {
		addr++;
		// fill original data
		for (i = 0; i < PGPKG_MAX_WORDS; i++)
		{
			if (BIT(i) & pkt.word_en)
			{
				efuse_one_byte_write(padapter, addr, pkt.data[i*2]);
				efuse_one_byte_write(padapter, addr+1, pkt.data[i*2 + 1]);

				// additional check
				if (efuse_one_byte_read(padapter, addr, &value) == _FALSE)
					ret = _FALSE;
				else if (pkt.data[i*2] != value) {
					ret = _FALSE;
					if (0xFF == value) // write again
						efuse_one_byte_write(padapter, addr, pkt.data[i*2]);
				}

				if (efuse_one_byte_read(padapter, addr+1, &value) == _FALSE)
					ret = _FALSE;
				else if (pkt.data[i*2 + 1] != value) {
					ret = _FALSE;
					if (0xFF == value) // write again
						efuse_one_byte_write(padapter, addr+1, pkt.data[i*2 + 1]);
				}
			}
			addr += 2;
		}
	} 
	else ret = _FALSE;

	return ret;
}
//------------------------------------------------------------------------------
u8 efuse_pg_packet_write(_adapter *padapter, const u8 offset, const u8 word_en, const u8 *data)
{
#if 0
	u8 WriteState = PG_STATE_HEADER;

	int bContinual = _TRUE, bDataEmpty = _TRUE;

	u16 remain_size = 0;
	u16 tmp_addr = 0;

	u8 tmp_word_cnts = 0;
	u8 tmp_header, match_word_en, tmp_word_en;

	u8 word_idx;

	PGPKT_STRUCT target_pkt;
	PGPKT_STRUCT tmp_pkt;

	u8 originaldata[sizeof(u8) * PGPKT_DATA_SIZE];
	u8 tmpindex = 0, badworden = 0x0F;
#endif
	u8 pg_header = 0;
	u16 efuse_addr = 0, curr_size = 0;
	u8 efuse_data, target_word_cnts = 0;

	static int repeat_times = 0;
	int sub_repeat;

	u8 bResult = _TRUE;

	// check if E-Fuse Clock Enable and E-Fuse Clock is 40M
	//efuse_data = read8(padapter, EFUSE_CLK_CTRL);
	//if (efuse_data != 0x03) {
	//	RT_TRACE(_module_rtl8712_efuse_c_, _drv_err_,("efuse_pg_packet_write: EFUSE_CLK_CTRL=0x%2x != 0x03", efuse_data));
	//	return _FALSE;
	//}

#if 1
	pg_header = MAKE_EFUSE_HEADER(offset, word_en);

	target_word_cnts = calculate_word_cnts(word_en);

	repeat_times = 0;
	efuse_addr = 0;
	while (efuse_addr < EFUSE_AVAILABLE_MAX_SIZE)
	{
		curr_size = efuse_get_current_size(padapter);
		RT_TRACE(_module_rtl8712_efuse_c_, _drv_info_,
			 ("====efuse_pg_packet_write: max=%d current=%d cnts=%d=====\n",
			  EFUSE_AVAILABLE_MAX_SIZE, curr_size, (1+target_word_cnts*2)));
		if ((curr_size + 1 + target_word_cnts*2) > EFUSE_AVAILABLE_MAX_SIZE) {
			RT_TRACE(_module_rtl8712_efuse_c_, _drv_err_,
				 ("efuse_pg_packet_write: size not enough!!!!\n"));
			return _FALSE; //target_word_cnts + pg header(1 byte)
		}

		efuse_addr = curr_size; // current size is also the last address
		efuse_one_byte_write(padapter, efuse_addr, pg_header); // write header

		sub_repeat = 0;
		// check if what we read is what we write
		while (efuse_one_byte_read(padapter, efuse_addr, &efuse_data) == _FALSE) {
			if (++sub_repeat > _REPEAT_THRESHOLD_) {
				RT_TRACE(_module_rtl8712_efuse_c_, _drv_emerg_,
					 ("====efuse_pg_packet_write: can't read written header!!!!\n"));
				bResult = _FALSE; // continue to blind write
//				return bResult; // no rescue can be done, exit.
				break; // continue to blind write
			}
		}

		if ((sub_repeat > _REPEAT_THRESHOLD_) || (pg_header == efuse_data))
		{
			// write header ok OR can't check header(creep)
			u8 i;

			// go to next address
			efuse_addr++;

			for (i = 0; i < target_word_cnts*2; i++) {
				efuse_one_byte_write(padapter, efuse_addr+i, *(data+i));
				if (efuse_one_byte_read(padapter, efuse_addr+i, &efuse_data) == _FALSE)
					bResult = _FALSE;
				else if (*(data+i) != efuse_data) // write data fail
					bResult = _FALSE;
			}
			break;
		} else { // write header fail
			bResult = _FALSE;

			if (0xFF == efuse_data)
				return bResult; // not thing damaged.

			/* call rescue procedure */
			if (fix_header(padapter, efuse_data, efuse_addr) == _FALSE)
				return _FALSE; // rescue fail, face the music

			if (++repeat_times > _REPEAT_THRESHOLD_) { // fail to write too many times
				RT_TRACE(_module_rtl8712_efuse_c_, _drv_emerg_,
					 ("====efuse_pg_packet_write: can't write header for too many times(%d)!!!!\n", _REPEAT_THRESHOLD_));
				break;
			}
			// otherwise, take another risk...
		}
	}
#else
	if ((curr_size = efuse_get_current_size(padapter)) >= EFUSE_AVAILABLE_MAX_SIZE)
		return _FALSE;

	remain_size = EFUSE_AVAILABLE_MAX_SIZE - curr_size;

	target_word_cnts = calculate_word_cnts(word_en);
	RT_TRACE(_module_rtl8712_efuse_c_, _drv_info_,
		("====efuse_pg_packet_write max=%d remain=%d cnts=%d=====\n", EFUSE_AVAILABLE_MAX_SIZE, remain_size, (target_word_cnts*2+1)));
	if (remain_size < (target_word_cnts * 2 + 1)){
		RT_TRACE(_module_rtl8712_efuse_c_, _drv_err_,
			 ("====efuse size is not enough !!!!\n"));
		return _FALSE; //target_word_cnts + pg header(1 byte)
	}

	target_pkt.offset = offset;
	target_pkt.word_en = word_en;
	memset(target_pkt.data, 0xFF, sizeof(u8)*8);
	pgpacket_copy_data(word_en, data, target_pkt.data);

	while (bContinual && (efuse_addr < EFUSE_AVAILABLE_MAX_SIZE))
	{
		if (WriteState == PG_STATE_HEADER)
		{
			bDataEmpty = _TRUE;
			badworden = 0x0F;		
			//************  so *******************	
			if (efuse_one_byte_read(padapter, efuse_addr, &efuse_data) && (efuse_data != 0xFF))
			{
				tmp_header = efuse_data;

				tmp_pkt.offset = (tmp_header >> 4) & 0x0F;
				tmp_pkt.word_en = tmp_header & 0x0F;					
				tmp_word_cnts = calculate_word_cnts(tmp_pkt.word_en);

				//************  so-1 *******************
				if (tmp_pkt.offset != target_pkt.offset) {
					efuse_addr = efuse_addr + (tmp_word_cnts * 2) + 1; //Next pg_packet
					WriteState = PG_STATE_HEADER;
				} else {
					//************  so-2 *******************
					for (tmpindex = 0; tmpindex < (tmp_word_cnts*2); tmpindex++) {
						if (efuse_one_byte_read(padapter, (efuse_addr+1+tmpindex), &efuse_data) && (efuse_data != 0xFF)){
							bDataEmpty = _FALSE;
						}
					}	
					//************  so-2-1 *******************
					if (bDataEmpty == _FALSE) {
						efuse_addr = efuse_addr + (tmp_word_cnts*2) + 1; //Next pg_packet	
						WriteState = PG_STATE_HEADER;
					}
					else {//************  so-2-2 *******************
						match_word_en = 0x0F;
						for (word_idx = 0; word_idx < PGPKG_MAX_WORDS; word_idx++) {
							if (!((target_pkt.word_en & BIT(word_idx)) | (tmp_pkt.word_en & BIT(word_idx)))) {
								 match_word_en &= (~BIT(word_idx));
							}
						}
						//************  so-2-2-A *******************
						if ((match_word_en & 0x0F) != 0x0F)
						{
							badworden = pgpacket_write_data(padapter, efuse_addr + 1, tmp_pkt.word_en, target_pkt.data);
							
							//************  so-2-2-A-1 *******************
							//############################
							if (0x0F != (badworden & 0x0F)) {
								u8 reorg_offset = offset;
								u8 reorg_worden = badworden;								
								efuse_pg_packet_write(padapter, reorg_offset, reorg_worden, originaldata);
							}
							//############################

							tmp_word_en = 0x0F;
							for (word_idx = 0; word_idx < PGPKG_MAX_WORDS; word_idx++) {
								if ((target_pkt.word_en & BIT(word_idx))^(match_word_en & BIT(word_idx))) {
									tmp_word_en &= (~BIT(word_idx));
								}
							}
							//************  so-2-2-A-2 *******************
							if ((tmp_word_en & 0x0F) != 0x0F) {
								//reorganize other pg packet
								efuse_addr = efuse_get_current_size(padapter);
								//===========================
								target_pkt.offset = offset;
								target_pkt.word_en= tmp_word_en;
								//===========================
							} else {
								bContinual = _FALSE;
							}
							WriteState = PG_STATE_HEADER;
							repeat_times++;
							if (repeat_times > _REPEAT_THRESHOLD_) {
								bContinual = _FALSE;
								bResult = _FALSE;
							}
						}
						else {//************  so-2-2-B *******************
							//reorganize other pg packet
							efuse_addr = efuse_addr + (2*tmp_word_cnts) + 1;//next pg packet addr							
							//===========================
							target_pkt.offset = offset;
							target_pkt.word_en= target_pkt.word_en;
							//===========================
							WriteState = PG_STATE_HEADER;
						}
					}				
				}				
			}
			else { //************  s1: header == oxff  *******************
				curr_size = efuse_get_current_size(padapter);
				remain_size = EFUSE_AVAILABLE_MAX_SIZE - curr_size;
				RT_TRACE(_module_rtl8712_efuse_c_,_drv_alert_,("====efuse write header state remain_size =%d, target_word_cnts=%d=====\n", remain_size,(target_word_cnts*2+1)));
				if (remain_size < (target_word_cnts*2+1)) //target_word_cnts + pg header(1 byte)
				{
					RT_TRACE(_module_rtl8712_efuse_c_,_drv_alert_,("====efuse size isnot enough !!!!\n"));
					bContinual = _FALSE;
					bResult = _FALSE;	
				} else {
					pg_header = ((target_pkt.offset << 4) & 0xf0) | target_pkt.word_en;

					efuse_one_byte_write(padapter, efuse_addr, pg_header);
					efuse_one_byte_read(padapter, efuse_addr, &tmp_header);

					if (tmp_header == pg_header) { //************  s1-1 *******************								
						WriteState = PG_STATE_DATA;
					} else if (tmp_header == 0xFF) {//************  s1-3: if Write or read func doesn't work *******************		
						//efuse_addr doesn't change
						WriteState = PG_STATE_HEADER;
						repeat_times++;
						if (repeat_times > _REPEAT_THRESHOLD_) {
							bContinual = _FALSE;
							bResult = _FALSE;
						}
					}
					else {//************  s1-2 : fixed the header procedure *******************
						RT_TRACE(_module_rtl8712_efuse_c_, _drv_err_, ("====efuse write header fail write=0x%02x read=0x%02x!!\n", pg_header, tmp_header));
						tmp_pkt.offset = (tmp_header>>4) & 0x0F;
						tmp_pkt.word_en = tmp_header & 0x0F;
						tmp_word_cnts = calculate_word_cnts(tmp_pkt.word_en);

						//************  s1-2-A :cover the exist data *******************
						memset(originaldata, 0xff, sizeof(u8)*8);

						if (efuse_pg_packet_read(padapter, tmp_pkt.offset, originaldata))
						{	//check if data exist
							badworden = pgpacket_write_data(padapter, efuse_addr + 1, tmp_pkt.word_en, originaldata);
							//############################
							if (0x0F != (badworden & 0x0F)) {
								u8 reorg_offset = tmp_pkt.offset;
								u8 reorg_worden = badworden;
								efuse_pg_packet_write(padapter, reorg_offset, reorg_worden, originaldata);	
								efuse_addr = efuse_get_current_size(padapter);
							}
							//############################
							else {
								efuse_addr = efuse_addr + (tmp_word_cnts * 2) + 1; //Next pg_packet
							}
						}
						 //************  s1-2-B: wrong address*******************
						else {
							efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; //Next pg_packet
						}
						WriteState = PG_STATE_HEADER;
						repeat_times++;
						if (repeat_times > _REPEAT_THRESHOLD_) {
							bContinual = _FALSE;
							bResult = _FALSE;
						}
					}
				}
			}
		}
		//write data state
		else if (WriteState == PG_STATE_DATA) {//************  s1-1  *******************
			badworden = 0x0f;
			badworden = pgpacket_write_data(padapter, efuse_addr+1, target_pkt.word_en, target_pkt.data);
			if((badworden&0x0F)==0x0F){ //************  s1-1-A *******************
				bContinual = _FALSE;
			}
			else{//reorganize other pg packet //************  s1-1-B *******************
				efuse_addr = efuse_addr + (2*target_word_cnts) +1;//next pg packet addr

				//===========================
				target_pkt.offset = offset;
				target_pkt.word_en= badworden;
				target_word_cnts = calculate_word_cnts(target_pkt.word_en);
				//===========================
				WriteState=PG_STATE_HEADER;
				repeat_times++;
				if (repeat_times > _REPEAT_THRESHOLD_) {
					bContinual = _FALSE;
					bResult = _FALSE;
				}
			}
		}
	}
#endif

	return bResult;
}
//------------------------------------------------------------------------------
u8 efuse_access(_adapter *padapter, u8 bRead, u16 start_addr, u16 cnts, u8 *data)
{
	int i = 0;
	u8 res;

	if (start_addr > EFUSE_MAX_SIZE)
		return _FALSE;

	if ((bRead == _FALSE) && ((start_addr + cnts) > EFUSE_AVAILABLE_MAX_SIZE))
		return _FALSE;

	if ((_FALSE == bRead) && (efuse_reg_init(padapter) == _FALSE))
		return _FALSE;

	//-----------------e-fuse one byte read / write ------------------------------	
	for (i = 0; i < cnts; i++) {
		if ((start_addr + i) > EFUSE_MAX_SIZE) {
			res = _FALSE;
			break;
		}
		res = efuse_one_byte_rw(padapter, bRead, start_addr + i, data + i);
//		RT_TRACE(_module_rtl871x_mp_ioctl_c_,_drv_err_,("==>efuse_access addr:0x%02x value:0x%02x\n",data+i,*(data+i)));
		if ((_FALSE == bRead) && (_FALSE == res)) break;
	}

	if (_FALSE == bRead) efuse_reg_uninit(padapter);

	return res;
}	
//------------------------------------------------------------------------------
u8 efuse_map_read(_adapter *padapter, u16 addr, u16 cnts, u8 *data)
{
	u8 offset, ret = _TRUE;
	u8 pktdata[PGPKT_DATA_SIZE];
	int i, idx;

	if ((addr + cnts) > EFUSE_MAP_MAX_SIZE)
		return _FALSE;

	if ((efuse_is_empty(padapter, &offset) == _TRUE) && (offset == _TRUE)) {
		for (i = 0; i < cnts; i++)
			data[i] = 0xFF;
		return ret;
	}

	offset = (addr >> 3) & 0xF;
	ret = efuse_pg_packet_read(padapter, offset, pktdata);
	i = addr & 0x7;	// pktdata index
	idx = 0;	// data index

	do {
		for (; i < PGPKT_DATA_SIZE; i++) {
			data[idx++] = pktdata[i];
			if (idx == cnts) return ret;
		}

		offset++;
//		if (offset > 0xF) break; // no need to check
		if (efuse_pg_packet_read(padapter, offset, pktdata) == _FALSE)
			ret = _FALSE;
		i = 0;
	} while (1);

	return ret;
}
//------------------------------------------------------------------------------
u8 efuse_map_write(_adapter* padapter, u16 addr, u16 cnts, u8 *data)
{
	u8 offset, word_en, empty;
	u8 pktdata[PGPKT_DATA_SIZE], newdata[PGPKT_DATA_SIZE];
	int i, j, idx;

	if ((addr + cnts) > EFUSE_MAP_MAX_SIZE)
		return _FALSE;

	// check if E-Fuse Clock Enable and E-Fuse Clock is 40M
	//empty = read8(padapter, EFUSE_CLK_CTRL);
	//if (empty != 0x03) {
	//	RT_TRACE(_module_rtl8712_efuse_c_, _drv_err_,("efuse_map_write: EFUSE_CLK_CTRL=0x%2x != 0x03", empty));
	//	return _FALSE;
	//}

	if (efuse_is_empty(padapter, &empty) == _TRUE) {
		if (_TRUE == empty)
			_memset(pktdata, 0xFF, PGPKT_DATA_SIZE);
	} else
		return _FALSE;

	offset = (addr >> 3) & 0xF;
	if (empty == _FALSE)
		if (efuse_pg_packet_read(padapter, offset, pktdata) == _FALSE)
			return _FALSE;

	word_en = 0xF;
	_memset(newdata, 0xFF, PGPKT_DATA_SIZE);
	i = addr & 0x7;	// pktdata index
	j = 0;		// newdata index
	idx = 0;	// data index

	if (i & 0x1) {
		// odd start
		if (data[idx] != pktdata[i]) {
			word_en &= ~BIT(i >> 1);
			newdata[j++] = pktdata[i - 1];
			newdata[j++] = data[idx];
		}
		i++;
		idx++;
	}
	do {
		for (; i < PGPKT_DATA_SIZE; i += 2) {
			if ((cnts - idx) == 1) {
				if (data[idx] != pktdata[i]) {
					word_en &= ~BIT(i >> 1);
					newdata[j++] = data[idx];
					newdata[j++] = pktdata[1 + 1];
				}
				idx++;
				break;
			} else {
				if ((data[idx] != pktdata[i]) || (data[idx+1] != pktdata[i+1])) {
					word_en &= ~BIT(i >> 1);
					newdata[j++] = data[idx];
					newdata[j++] = data[idx + 1];
				}
				idx += 2;
			}
			if (idx == cnts) break;
		}

		if (word_en != 0xF)
			if (efuse_pg_packet_write(padapter, offset, word_en, newdata) == _FALSE)
				return _FALSE;

		if (idx == cnts) break;

		offset++;
		if (empty == _FALSE)
			if (efuse_pg_packet_read(padapter, offset, pktdata) == _FALSE)
				return _FALSE;
		i = 0;
		j = 0;
		word_en = 0xF;
		_memset(newdata, 0xFF, PGPKT_DATA_SIZE);
	} while (1);

	return _TRUE;
}

enum{
	VOLTAGE_V25						= 0x03,
	LDOE25_SHIFT						= 28 ,
};

#define REG_SYS_ISO_CTRL			0x0000
#define PWC_EV12V					BIT(15)
#define REG_SYS_FUNC_EN			0x0002
#define FEN_ELDR					BIT(12)
#define REG_SYS_CLKR				0x0008
#define ANA8M						BIT(1)
#define LOADER_CLK_EN				BIT(5)

#define		EFUSE_REAL_CONTENT_LEN		512
#define		EFUSE_MAP_LEN					128
#define		EFUSE_MAX_SECTION			16
#define		EFUSE_MAX_WORD_UNIT			4

static	void
efuse_PowerSwitch(
	PADAPTER	pAdapter,
	u8           bWrite,
	u8		PwrState)
{
	u8	tempval;
	u16	tmpV16;
	
	if(PwrState == _TRUE){
		// 1.2V Power: From VDDON with Power Cut(0x0000h[15]), defualt valid
		tmpV16 = read16(pAdapter,REG_SYS_ISO_CTRL);
		if( ! (tmpV16 & PWC_EV12V ) ){
			tmpV16 |= PWC_EV12V ;
			 write16(pAdapter,REG_SYS_ISO_CTRL,tmpV16);
		}
		// Reset: 0x0000h[28], default valid
		tmpV16 =  read16(pAdapter,REG_SYS_FUNC_EN);
		if( !(tmpV16 & FEN_ELDR) ){
			tmpV16 |= FEN_ELDR ;
			write16(pAdapter,REG_SYS_FUNC_EN,tmpV16);
		}
		
		// Clock: Gated(0x0008h[5]) 8M(0x0008h[1]) clock from ANA, default valid
		tmpV16 = read16(pAdapter,REG_SYS_CLKR);
		if( (!(tmpV16 & LOADER_CLK_EN) )  ||(!(tmpV16 & ANA8M) ) ){
			tmpV16 |= (LOADER_CLK_EN |ANA8M ) ;
			write16(pAdapter,REG_SYS_CLKR,tmpV16);
		}		
	}

	if (PwrState == _TRUE)
	{
		if(bWrite == _TRUE){
			// Enable LDO 2.5V before read/write action
			tempval = read8(pAdapter, EFUSE_TEST+3);
			tempval &= 0x0F;
			tempval |= (VOLTAGE_V25 << 4);
			write8(pAdapter, EFUSE_TEST+3, (tempval | 0x80));
		}
	}
	else
	{
		if(bWrite == _TRUE){
			// Disable LDO 2.5V after read/write action
			tempval = read8(pAdapter, EFUSE_TEST+3);
			write8(pAdapter, EFUSE_TEST+3, (tempval & 0x7F));
		}

		// Change Efuse Clock for write action to 500K
	}	
	
}	/* efuse_PowerSwitch */

/*-----------------------------------------------------------------------------
 * Function:	EFUSE_Read1Byte
 *
 * Overview:	Copy from WMAC fot EFUSE read 1 byte.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 09/23/2008 	MHC		Copy from WMAC.
 *
 *---------------------------------------------------------------------------*/
u8	
EFUSE_Read1Byte(	
	PADAPTER	pAdapter, 
	u16		Address)
{
	u8	data;
	u8	Bytetemp = {0x00};
	u8	temp = {0x00};
	u32	k=0;

	if (Address < EFUSE_REAL_CONTENT_LEN)	//E-fuse 512Byte
	{
		//Write E-fuse Register address bit0~7
		temp = Address & 0xFF;	
		write8(pAdapter, EFUSE_CTRL+1, temp);	
		Bytetemp = read8(pAdapter, EFUSE_CTRL+2);	
		//Write E-fuse Register address bit8~9
		temp = ((Address >> 8) & 0x03) | (Bytetemp & 0xFC);	
		write8(pAdapter, EFUSE_CTRL+2, temp);	

		//Write 0x30[31]=0
		Bytetemp = read8(pAdapter, EFUSE_CTRL+3);
		temp = Bytetemp & 0x7F;
		write8(pAdapter, EFUSE_CTRL+3, temp);

		//Wait Write-ready (0x30[31]=1)
		Bytetemp = read8(pAdapter, EFUSE_CTRL+3);
		while(!(Bytetemp & 0x80))
		{				
			Bytetemp = read8(pAdapter, EFUSE_CTRL+3);
			k++;
			if(k==1000)
			{
				k=0;
				break;
			}
		}
		data=read8(pAdapter, EFUSE_CTRL);
		return data;
	}
	else
		return 0xFF;
	
}	/* EFUSE_Read1Byte */

//
//	Description:
//		Execute E-Fuse read byte operation.
//		Refered from SD1 Richard.
//
//	Assumption:
//		1. Boot from E-Fuse and successfully auto-load.
//		2. PASSIVE_LEVEL (USB interface)
//
//	Created by Roger, 2008.10.21.
//
static	void
ReadEFuseByte(
		PADAPTER	pAdapter,
		u16 		_offset, 
		u8 		*pbuf) 
{
	u32  value32;
	u8 	readbyte;
	u16 	retry;
	

	//Write Address
	write8(pAdapter, EFUSE_CTRL+1, (_offset & 0xff));  		
	readbyte = read8(pAdapter, EFUSE_CTRL+2);
	write8(pAdapter, EFUSE_CTRL+2, ((_offset >> 8) & 0x03) | (readbyte & 0xfc));  		

	//Write bit 32 0
	readbyte = read8(pAdapter, EFUSE_CTRL+3);		
	write8(pAdapter, EFUSE_CTRL+3, (readbyte & 0x7f));  	
	
	//Check bit 32 read-ready
	retry = 0;
	value32 = read32(pAdapter, EFUSE_CTRL);
	//while(!(((value32 >> 24) & 0xff) & 0x80)  && (retry<10))
	while(!(((value32 >> 24) & 0xff) & 0x80)  && (retry<10000))
	{
		value32 = read32(pAdapter, EFUSE_CTRL);
		retry++;
	}

	// 20100205 Joseph: Add delay suggested by SD1 Victor.
	// This fix the problem that Efuse read error in high temperature condition.
	// Designer says that there shall be some delay after ready bit is set, or the
	// result will always stay on last data we read.
	udelay_os(50);
	value32 = read32(pAdapter, EFUSE_CTRL);
	
	*pbuf = (u8)(value32 & 0xff);
}


//
//	Description:
//		1. Execute E-Fuse read byte operation according as map offset and 
//		    save to E-Fuse table.
//		2. Refered from SD1 Richard.
//
//	Assumption:
//		1. Boot from E-Fuse and successfully auto-load.
//		2. PASSIVE_LEVEL (USB interface)
//
//	Created by Roger, 2008.10.21.
//
//	2008/12/12 MH 	1. Reorganize code flow and reserve bytes. and add description.
//					2. Add efuse utilization collect.
//	2008/12/22 MH	Read Efuse must check if we write section 1 data again!!! Sec1
//					write addr must be after sec5.
//
static void
ReadEFuse(
	PADAPTER	pAdapter,
	u16		 _offset,
	u16 		_size_byte,
	u8      	*pbuf
	)
{
	u8  	efuseTbl[EFUSE_MAP_LEN];
	u8  	rtemp8[1];
	u16 	eFuse_Addr = 0;
	u8  	offset, wren;
	u16  i, j;
	u16 	eFuseWord[EFUSE_MAX_SECTION][EFUSE_MAX_WORD_UNIT];
	u16	efuse_utilized = 0;
	u8	efuse_usage = 0;

	//
	// Do NOT excess total size of EFuse table. Added by Roger, 2008.11.10.
	//
	if((_offset + _size_byte)>EFUSE_MAP_LEN)
	{// total E-Fuse table is 128bytes
		//RT_TRACE(COMP_EFUSE, DBG_LOUD, 
		//("ReadEFuse(): Invalid offset(%#x) with read bytes(%#x)!!\n",_offset, _size_byte));
		return;
	}

	// 0. Refresh efuse init map as all oxFF.
	for (i = 0; i < EFUSE_MAX_SECTION; i++)
		for (j = 0; j < EFUSE_MAX_WORD_UNIT; j++)
			eFuseWord[i][j] = 0xFFFF;


	//
	// 1. Read the first byte to check if efuse is empty!!!
	// 
	//
	ReadEFuseByte(pAdapter, eFuse_Addr, rtemp8);	
	if(*rtemp8 != 0xFF)
	{
		efuse_utilized++;
		//RTPRINT(FEEPROM, EFUSE_READ_ALL, ("Addr=%d\n", eFuse_Addr));
		eFuse_Addr++;
	}

	//
	// 2. Read real efuse content. Filter PG header and every section data.
	//
	while((*rtemp8 != 0xFF) && (eFuse_Addr < EFUSE_REAL_CONTENT_LEN))
	{
		// Check PG header for section num.
		offset = ((*rtemp8 >> 4) & 0x0f);

		if(offset < EFUSE_MAX_SECTION)
		{
			// Get word enable value from PG header
			wren = (*rtemp8 & 0x0f);
			//RTPRINT(FEEPROM, EFUSE_READ_ALL, ("Offset-%d Worden=%x\n", offset, wren));

			for(i=0; i<EFUSE_MAX_WORD_UNIT; i++)
			{
				// Check word enable condition in the section				
				if(!(wren & 0x01))
				{
					//RTPRINT(FEEPROM, EFUSE_READ_ALL, ("Addr=%d\n", eFuse_Addr));
					ReadEFuseByte(pAdapter, eFuse_Addr, rtemp8);	eFuse_Addr++;
					efuse_utilized++;
					eFuseWord[offset][i] = (*rtemp8 & 0xff);
					

					if(eFuse_Addr >= EFUSE_REAL_CONTENT_LEN) 
						break;

					//RTPRINT(FEEPROM, EFUSE_READ_ALL, ("Addr=%d\n", eFuse_Addr));
					ReadEFuseByte(pAdapter, eFuse_Addr, rtemp8);	eFuse_Addr++;
					efuse_utilized++;
					eFuseWord[offset][i] |= (((u16)*rtemp8 << 8) & 0xff00);

					if(eFuse_Addr >= EFUSE_REAL_CONTENT_LEN) 
						break;
				}
				
				wren >>= 1;
				
			}
		}

		//RTPRINT(FEEPROM, EFUSE_READ_ALL, ("Addr=%d\n", eFuse_Addr));
		// Read next PG header
		ReadEFuseByte(pAdapter, eFuse_Addr, rtemp8);	
		if(*rtemp8 != 0xFF && (eFuse_Addr < 512))
		{
			efuse_utilized++;
			eFuse_Addr++;
		}
	}

	//
	// 3. Collect 16 sections and 4 word unit into Efuse map.
	//
	for(i=0; i<EFUSE_MAX_SECTION; i++)
	{
		for(j=0; j<EFUSE_MAX_WORD_UNIT; j++)
		{
			efuseTbl[(i*8)+(j*2)]=(eFuseWord[i][j] & 0xff);
			efuseTbl[(i*8)+((j*2)+1)]=((eFuseWord[i][j] >> 8) & 0xff);
		}
	}

	//
	// 4. Copy from Efuse map to output pointer memory!!!
	//
	for(i=0; i<_size_byte; i++)
	{		
		pbuf[i] = efuseTbl[_offset+i];
	}

	//
	// 5. Calculate Efuse utilization.
	//
	//priv->EfuseUsedBytes = efuse_utilized;
	efuse_usage = (u8)((efuse_utilized*100)/EFUSE_REAL_CONTENT_LEN);
	//priv->EfuseUsedPercentage = efuse_usage;	
	//priv->rtllib->SetHwRegHandler(dev, HW_VAR_EFUSE_BYTES, (u8*)&efuse_utilized);
	//priv->rtllib->SetHwRegHandler(dev, HW_VAR_EFUSE_USAGE, (u8*)&efuse_usage);
}

static void efuse_ReadAllMap(
	IN		PADAPTER	pAdapter, 
	IN OUT	u8		*Efuse)
{
#if 0
	int i, offset;	

	//
	// We must enable clock and LDO 2.5V otherwise, read all map will be fail!!!!
	//
	//efuse_PowerSwitch(pAdapter, _TRUE);
	//ReadEFuse(pAdapter, 0, 128, Efuse);
	//efuse_PowerSwitch(pAdapter, _FALSE);

	//efuse_reg_init(pAdapter);
	
	for(i=0, offset=0 ; i<128; i+=8, offset++)
	{
		efuse_pg_packet_read(pAdapter, offset, Efuse+i);			
	}
	
	//efuse_reg_uninit(pAdapter);
#else
	//
	// We must enable clock and LDO 2.5V otherwise, read all map will be fail!!!!
	//
	efuse_PowerSwitch(pAdapter, _FALSE, _TRUE);

	ReadEFuse(pAdapter, 0, 128, Efuse);

	efuse_PowerSwitch(pAdapter, _FALSE, _FALSE);
#endif
}// efuse_ReadAllMap
//------------------------------------------------------------------------------

void EFUSE_ShadowMapUpdate(
	IN		PADAPTER	pAdapter)
{	
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(pAdapter);
		
	if (pEEPROM->bautoload_fail_flag == _TRUE)
	{			
		_memset(pEEPROM->efuse_eeprom_data, 0xff, 128);
	}
	else
	{
		efuse_ReadAllMap(pAdapter, pEEPROM->efuse_eeprom_data);
	}
	
	//PlatformMoveMemory((PVOID)&pHalData->EfuseMap[EFUSE_MODIFY_MAP][0], 
	//(PVOID)&pHalData->EfuseMap[EFUSE_INIT_MAP][0], HWSET_MAX_SIZE);
	
}// EFUSE_ShadowMapUpdate

/*-----------------------------------------------------------------------------
 * Function:	efuse_ShadowRead1Byte
 *			efuse_ShadowRead2Byte
 *			efuse_ShadowRead4Byte
 *
 * Overview:	Read from efuse init map by one/two/four bytes !!!!!
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/12/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
static	VOID
efuse_ShadowRead1Byte(
	IN		PADAPTER	pAdapter,
	IN		u16		Offset,
	IN OUT	u8		*Value	)
{
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(pAdapter);
	
	*Value = pEEPROM->efuse_eeprom_data[Offset];

}	// EFUSE_ShadowRead1Byte

//---------------Read Two Bytes
static	VOID
efuse_ShadowRead2Byte(
	IN		PADAPTER	pAdapter,
	IN		u16		Offset,
	IN OUT	u16		*Value	)
{
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(pAdapter);
	
	*Value = pEEPROM->efuse_eeprom_data[Offset];
	*Value |= pEEPROM->efuse_eeprom_data[Offset+1]<<8;
	
}	// EFUSE_ShadowRead2Byte

//---------------Read Four Bytes
static	VOID
efuse_ShadowRead4Byte(
	IN		PADAPTER	pAdapter,
	IN		u16		Offset,
	IN OUT	u32		*Value	)
{
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(pAdapter);
	
	*Value = pEEPROM->efuse_eeprom_data[Offset];
	*Value |= pEEPROM->efuse_eeprom_data[Offset+1]<<8;
	*Value |= pEEPROM->efuse_eeprom_data[Offset+2]<<16;
	*Value |= pEEPROM->efuse_eeprom_data[Offset+3]<<24;
	
}	// efuse_ShadowRead4Byte


/*-----------------------------------------------------------------------------
 * Function:	EFUSE_ShadowRead
 *
 * Overview:	Read from efuse init map !!!!!
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/12/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
void
EFUSE_ShadowRead(
	IN		PADAPTER	pAdapter,
	IN		u8		Type,
	IN		u16		Offset,
	IN OUT	u32		*Value	)
{
	if (Type == 1)
		efuse_ShadowRead1Byte(pAdapter, Offset, (u8 *)Value);
	else if (Type == 2)
		efuse_ShadowRead2Byte(pAdapter, Offset, (u16 *)Value);
	else if (Type == 4)
		efuse_ShadowRead4Byte(pAdapter, Offset, (u32 *)Value);
	
}	// EFUSE_ShadowRead

#if 0
/*-----------------------------------------------------------------------------
 * Function:	efuse_ShadowWrite1Byte
 *			efuse_ShadowWrite2Byte
 *			efuse_ShadowWrite4Byte
 *
 * Overview:	Write efuse modify map by one/two/four byte.
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/12/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
static	VOID
efuse_ShadowWrite1Byte(
	IN		PADAPTER	pAdapter,
	IN		u16		Offset,
	IN 		u8		Value	)
{
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(pAdapter);

	pEEPROM->efuse_eeprom_data[Offset] = Value;

}	// efuse_ShadowWrite1Byte

//---------------Write Two Bytes
static	VOID
efuse_ShadowWrite2Byte(
	IN		PADAPTER	pAdapter,
	IN		u16		Offset,
	IN 		u32		Value	)
{
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(pAdapter);
	
	pEEPROM->efuse_eeprom_data[Offset] = Value&0x00FF;
	pEEPROM->efuse_eeprom_data[Offset+1] = Value>>8;

}	// efuse_ShadowWrite1Byte

//---------------Write Four Bytes
static	VOID
efuse_ShadowWrite4Byte(
	IN		PADAPTER	pAdapter,
	IN		u16		Offset,
	IN 		u32		Value	)
{
	EEPROM_EFUSE_PRIV *pEEPROM = GET_EEPROM_EFUSE_PRIV(pAdapter);

	pEEPROM->efuse_eeprom_data[Offset] = (u8)(Value&0x000000FF);
	pEEPROM->efuse_eeprom_data[Offset+1] = (u8)((Value>>8)&0x0000FF);
	pEEPROM->efuse_eeprom_data[Offset+2] = (u8)((Value>>16)&0x00FF);
	pEEPROM->efuse_eeprom_data[Offset+3] = (u8)((Value>>24)&0xFF);

}	// efuse_ShadowWrite1Byte

/*-----------------------------------------------------------------------------
 * Function:	EFUSE_ShadowWrite
 *
 * Overview:	Write efuse modify map for later update operation to use!!!!!
 *
 * Input:       NONE
 *
 * Output:      NONE
 *
 * Return:      NONE
 *
 * Revised History:
 * When			Who		Remark
 * 11/12/2008 	MHC		Create Version 0.
 *
 *---------------------------------------------------------------------------*/
extern	VOID
EFUSE_ShadowWrite(
	IN		PADAPTER	pAdapter,
	IN		u8		Type,
	IN		u16		Offset,
	IN OUT	u32		Value	)
{
#if (MP_DRIVER == 0)
	return;
#endif

	if (Type == 1)
		efuse_ShadowWrite1Byte(pAdapter, Offset, (u8)Value);
	else if (Type == 2)
		efuse_ShadowWrite2Byte(pAdapter, Offset, (u16)Value);
	else if (Type == 4)
		efuse_ShadowWrite4Byte(pAdapter, Offset, (u32)Value);
	
}	// EFUSE_ShadowWrite
 #endif

