/*
 * $Id: mtd-abi.h,v 1.11 2005/05/19 16:08:58 gleixner Exp $
 *
 * Portions of MTD ABI definition which are shared by kernel and user space 
 */

#ifndef __MTD_ABI_H__
#define __MTD_ABI_H__

#ifndef __KERNEL__ /* Urgh. The whole point of splitting this out into
		    separate files was to avoid #ifdef __KERNEL__ */
#define __user
#endif

struct erase_info_user {
	uint32_t start;
	uint32_t length;
};

struct mtd_oob_buf {
	uint32_t start;
	uint32_t length;
	unsigned char __user *ptr;
};

//===============Ken: 20081003================
struct mtd_data_buf {
	uint32_t start;
	uint32_t length;
	unsigned char __user *ptr;
};

struct mtd_data_oob {
	struct mtd_data_buf rtk_data;
	struct mtd_oob_buf rtk_oob;
};
//===============Ken: 20081003================

#define MTD_ABSENT		0
#define MTD_RAM			1
#define MTD_ROM			2
#define MTD_NORFLASH		3
#define MTD_NANDFLASH		4
#define MTD_PEROM		5
#define MTD_DATAFLASH		6
#define MTD_OTHER		14
#define MTD_UNKNOWN		15

#define MTD_CLEAR_BITS		1       // Bits can be cleared (flash)
#define MTD_SET_BITS		2       // Bits can be set
#define MTD_ERASEABLE		4       // Has an erase function
#define MTD_WRITEB_WRITEABLE	8       // Direct IO is possible
#define MTD_VOLATILE		16      // Set for RAMs
#define MTD_XIP			32	// eXecute-In-Place possible
#define MTD_OOB			64	// Out-of-band data (NAND flash)
#define MTD_ECC			128	// Device capable of automatic ECC
#define MTD_NO_VIRTBLOCKS	256	// Virtual blocks not allowed

// Some common devices / combinations of capabilities
#define MTD_CAP_ROM		0
#define MTD_CAP_RAM		(MTD_CLEAR_BITS|MTD_SET_BITS|MTD_WRITEB_WRITEABLE)
#define MTD_CAP_NORFLASH        (MTD_CLEAR_BITS|MTD_ERASEABLE)
#define MTD_CAP_NANDFLASH       (MTD_CLEAR_BITS|MTD_ERASEABLE|MTD_OOB)
#define MTD_WRITEABLE		(MTD_CLEAR_BITS|MTD_SET_BITS)


// Types of automatic ECC/Checksum available
#define MTD_ECC_NONE		0 	// No automatic ECC available
#define MTD_ECC_RS_DiskOnChip	1	// Automatic ECC on DiskOnChip
#define MTD_ECC_SW		2	// SW ECC for Toshiba & Samsung devices
#define MTD_ECC_RTK_HW		3	// Ken:  Realtek Nand HW IP ECC

/* ECC byte placement */
#define MTD_NANDECC_OFF		0	// Switch off ECC (Not recommended)
#define MTD_NANDECC_PLACE	1	// Use the given placement in the structure (YAFFS1 legacy mode)
#define MTD_NANDECC_AUTOPLACE	2	// Use the default placement scheme
#define MTD_NANDECC_PLACEONLY	3	// Use the given placement in the structure (Do not store ecc result on read)
#define MTD_NANDECC_AUTOPL_USR 	4	// Use the given autoplacement scheme rather than using the default

/* OTP mode selection */
#define MTD_OTP_OFF		0
#define MTD_OTP_FACTORY		1
#define MTD_OTP_USER		2

struct mtd_info_user {
	uint8_t type;
	uint32_t flags;
	uint32_t size;	 // Total size of the MTD
	uint32_t erasesize;
	uint32_t oobblock;  // Size of OOB blocks (e.g. 512)
	uint32_t oobsize;   // Amount of OOB data per block (e.g. 16)
	uint32_t ecctype;
	uint32_t eccsize;
};

struct region_info_user {
	uint32_t offset;		/* At which this region starts, 
					 * from the beginning of the MTD */
	uint32_t erasesize;		/* For this region */
	uint32_t numblocks;		/* Number of blocks in this region */
	uint32_t regionindex;
};

struct otp_info {
	uint32_t start;
	uint32_t length;
	uint32_t locked;
};

#define MEMGETINFO              _IOR('M', 1, struct mtd_info_user)
#define MEMERASE                _IOW('M', 2, struct erase_info_user)
#define MEMWRITEOOB             _IOWR('M', 3, struct mtd_oob_buf)
#define MEMREADOOB              _IOWR('M', 4, struct mtd_oob_buf)
#define MEMLOCK                 _IOW('M', 5, struct erase_info_user)
#define MEMUNLOCK               _IOW('M', 6, struct erase_info_user)
#define MEMGETREGIONCOUNT	_IOR('M', 7, int)
#define MEMGETREGIONINFO	_IOWR('M', 8, struct region_info_user)
#define MEMSETOOBSEL		_IOW('M', 9, struct nand_oobinfo)
#define MEMGETOOBSEL		_IOR('M', 10, struct nand_oobinfo)
#define MEMGETBADBLOCK		_IOW('M', 11, loff_t)
#define MEMSETBADBLOCK		_IOW('M', 12, loff_t)
#define OTPSELECT		_IOR('M', 13, int)
#define OTPGETREGIONCOUNT	_IOW('M', 14, int)
#define OTPGETREGIONINFO	_IOW('M', 15, struct otp_info)
#define OTPLOCK		_IOR('M', 16, struct otp_info)

//===============Ken: 20081003================
#define MEMWRITEDATAOOB		_IOWR('M', 17, struct mtd_data_oob)
#define MEMREADDATAOOB		_IOWR('M', 18, struct mtd_data_oob)
/* Ken: 20090210*/
#define MEMRELOADBBT		_IO('M', 19)
/* CMYu:20090409*/
#define RTKMEMERASE		_IO('M', 20)

//addby alexchang2131 0519-2010
#define MEMSETSYSINFO	_IOWR('M', 21, struct mtd_data_oob)
#define MEMGETSYSINFO   _IOWR('M', 22, struct mtd_data_oob)


struct nand_oobinfo {
	uint32_t useecc;
	uint32_t eccbytes;
	uint32_t oobfree[8][2];
	uint32_t eccpos[32];
};

#endif /* __MTD_ABI_H__ */
