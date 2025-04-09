/*
 *  linux/include/linux/ms/ms.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Modification History:
 *    #000 2009-07-09 CMYu   modify from mmc.h for ms usage
 */
#ifndef MS_H
#define MS_H

#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/device.h>

//========== BEGIN: copy from memstick=============
struct ms_status_register {
	unsigned char reserved;
	unsigned char interrupt;
#define MEMSTICK_INT_CMDNAK 0x01
#define MEMSTICK_INT_IOREQ  0x08
#define MEMSTICK_INT_IOBREQ 0x10
#define MEMSTICK_INT_BREQ   0x20
#define MEMSTICK_INT_ERR    0x40
#define MEMSTICK_INT_CED    0x80

	unsigned char status0;
#define MEMSTICK_STATUS0_WP  0x01
#define MEMSTICK_STATUS0_SL  0x02
#define MEMSTICK_STATUS0_BF  0x10
#define MEMSTICK_STATUS0_BE  0x20
#define MEMSTICK_STATUS0_FB0 0x40
#define MEMSTICK_STATUS0_MB  0x80

	unsigned char status1;
#define MEMSTICK_STATUS1_UCFG 0x01
#define MEMSTICK_STATUS1_FGER 0x02
#define MEMSTICK_STATUS1_UCEX 0x04
#define MEMSTICK_STATUS1_EXER 0x08
#define MEMSTICK_STATUS1_UCDT 0x10
#define MEMSTICK_STATUS1_DTER 0x20
#define MEMSTICK_STATUS1_FB1  0x40
#define MEMSTICK_STATUS1_MB   0x80
} __attribute__((packed));


struct ms_id_register {
	unsigned char type;
	unsigned char if_mode;
	unsigned char category;
	unsigned char class;
} __attribute__((packed));


struct ms_param_register {
	unsigned char system;
#define MEMSTICK_SYS_PAM  0x08
#define MEMSTICK_SYS_BAMD 0x80

	unsigned char block_address_msb;
	unsigned short block_address;
	unsigned char cp;
#define MEMSTICK_CP_BLOCK     0x00
#define MEMSTICK_CP_PAGE      0x20
#define MEMSTICK_CP_EXTRA     0x40
#define MEMSTICK_CP_OVERWRITE 0x80

	unsigned char page_address;
} __attribute__((packed));


struct ms_extra_data_register {
	unsigned char  overwrite_flag;
#define MEMSTICK_OVERWRITE_UDST  0x10
#define MEMSTICK_OVERWRITE_PGST1 0x20
#define MEMSTICK_OVERWRITE_PGST0 0x40
#define MEMSTICK_OVERWRITE_BKST  0x80

	unsigned char  management_flag;
#define MEMSTICK_MANAGEMENT_SYSFLG 0x04
#define MEMSTICK_MANAGEMENT_ATFLG  0x08
#define MEMSTICK_MANAGEMENT_SCMS1  0x10
#define MEMSTICK_MANAGEMENT_SCMS0  0x20

	unsigned short logical_address;
} __attribute__((packed));


struct ms_register {
	struct ms_status_register     status;
	struct ms_id_register         id;
	unsigned char                 reserved[8];
	struct ms_param_register      param;
	struct ms_extra_data_register extra_data;
} __attribute__((packed));


struct mspro_param_register {
	unsigned char  system;
#define MEMSTICK_SYS_PAR4   0x00
#define MEMSTICK_SYS_PAR8   0x40
#define MEMSTICK_SYS_SERIAL 0x80

	unsigned short data_count;
	unsigned int   data_address;
	unsigned char  tpc_param;
} __attribute__((packed));


struct mspro_io_info_register {
	unsigned char version;
	unsigned char io_category;
	unsigned char current_req;
	unsigned char card_opt_info;
	unsigned char rdy_wait_time;
} __attribute__((packed));


struct mspro_io_func_register {
	unsigned char func_enable;
	unsigned char func_select;
	unsigned char func_intmask;
	unsigned char transfer_mode;
} __attribute__((packed));


struct mspro_io_cmd_register {
	unsigned short tpc_param;
	unsigned short data_count;
	unsigned int   data_address;
} __attribute__((packed));


struct mspro_register {
	struct ms_status_register     status;
	struct ms_id_register         id;
	unsigned char                 reserved0[8];
	struct mspro_param_register   param;
	unsigned char                 reserved1[8];
	struct mspro_io_info_register io_info;
	struct mspro_io_func_register io_func;
	unsigned char                 reserved2[7];
	struct mspro_io_cmd_register  io_cmd;
	unsigned char                 io_int;
	unsigned char                 io_int_func;
} __attribute__((packed));


struct ms_register_addr {
	unsigned char r_offset;
	unsigned char r_length;
	unsigned char w_offset;
	unsigned char w_length;
} __attribute__((packed));


struct memstick_device_id {
	unsigned char match_flags;
#define MEMSTICK_MATCH_ALL            0x01

	unsigned char type;
#define MEMSTICK_TYPE_LEGACY          0xff
#define MEMSTICK_TYPE_DUO             0x00
#define MEMSTICK_TYPE_PRO             0x01

	unsigned char category;
#define MEMSTICK_CATEGORY_STORAGE     0xff
#define MEMSTICK_CATEGORY_STORAGE_DUO 0x00
#define MEMSTICK_CATEGORY_IO          0x01
#define MEMSTICK_CATEGORY_IO_PRO      0x10

	unsigned char class;
#define MEMSTICK_CLASS_FLASH          0xff
#define MEMSTICK_CLASS_DUO            0x00
#define MEMSTICK_CLASS_ROM            0x01
#define MEMSTICK_CLASS_RO             0x02
#define MEMSTICK_CLASS_WP             0x03
};


struct mspro_sys_info {
	unsigned char  class;
	unsigned char  reserved0;
	unsigned short block_size;
	unsigned short block_count;
	unsigned short user_block_count;
	unsigned short page_size;
	unsigned char  reserved1[2];
	unsigned char  assembly_date[8];
	unsigned int   serial_number;
	unsigned char  assembly_maker_code;
	unsigned char  assembly_model_code[3];
	unsigned short memory_maker_code;
	unsigned short memory_model_code;
	unsigned char  reserved2[4];
	unsigned char  vcc;
	unsigned char  vpp;
	unsigned short controller_number;
	unsigned short controller_function;
	unsigned short start_sector;
	unsigned short unit_size;
	unsigned char  ms_sub_class;
	unsigned char  reserved3[4];
	unsigned char  interface_type;
	unsigned short controller_code;
	unsigned char  format_type;
	unsigned char  reserved4;
	unsigned char  device_type;
	unsigned char  reserved5[7];
	unsigned char  mspro_id[16];
	unsigned char  reserved6[16];
} __attribute__((packed));

//========== END: copy from memstick============

#endif
