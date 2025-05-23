#ifndef __BASIC_TYPES_H__
#define __BASIC_TYPES_H__

#include <drv_conf.h>


#define SUCCESS	0
#define FAIL	(-1)

#ifndef TRUE
	#define _TRUE	1
#else
	#define _TRUE	TRUE	
#endif
		
#ifndef FALSE		
	#define _FALSE	0
#else
	#define _FALSE	FALSE	
#endif

#ifdef PLATFORM_WINDOWS

	typedef signed char s8;
	typedef unsigned char u8;

	typedef signed short s16;
	typedef unsigned short u16;

	typedef signed long s32;
	typedef unsigned long u32;
	
	typedef unsigned int	uint;
	typedef	signed int		sint;


	typedef signed long long s64;
	typedef unsigned long long u64;

	#ifdef NDIS50_MINIPORT
	
		#define NDIS_MAJOR_VERSION       5
		#define NDIS_MINOR_VERSION       0

	#endif

	#ifdef NDIS51_MINIPORT

		#define NDIS_MAJOR_VERSION       5
		#define NDIS_MINOR_VERSION       1

	#endif

	typedef NDIS_PROC proc_t;

	typedef LONG atomic_t;

#endif


#ifdef PLATFORM_LINUX

	#include <linux/types.h>
	#define IN
	#define OUT
	#define VOID void
	#define NDIS_OID uint
	#define NDIS_STATUS uint

	typedef	signed int sint;

	#ifndef	PVOID
	typedef void * PVOID;
	//#define PVOID	(void *)
	#endif

        #define UCHAR u8
	#define USHORT u16
	#define UINT u32
	#define ULONG u32	

	typedef void (*proc_t)(void*);

	typedef 	__kernel_size_t	SIZE_T;	
	#define FIELD_OFFSET(s,field)	((int)&((s*)(0))->field)
	
#endif

#define MEM_ALIGNMENT_OFFSET	(sizeof (SIZE_T))
#define MEM_ALIGNMENT_PADDING	(sizeof(SIZE_T) - 1)


//port from fw by thomas
// TODO: Belows are Sync from SD7-Driver. It is necessary to check correctness

/*
 *	Call endian free function when
 *		1. Read/write packet content.
 *		2. Before write integer to IO.
 *		3. After read integer from IO.
*/

//
// Byte Swapping routine.
//
#define EF1Byte	
#define EF2Byte 	le16_to_cpu
#define EF4Byte	le32_to_cpu

//
// Read LE format data from memory
//
#define ReadEF1Byte(_ptr)		EF1Byte(*((u8 *)(_ptr)))
#define ReadEF2Byte(_ptr)		EF2Byte(*((u16 *)(_ptr)))
#define ReadEF4Byte(_ptr)		EF4Byte(*((u32 *)(_ptr)))

//
// Write LE data to memory
//
#define WriteEF1Byte(_ptr, _val)	(*((u8 *)(_ptr)))=EF1Byte(_val)
#define WriteEF2Byte(_ptr, _val)	(*((u16 *)(_ptr)))=EF2Byte(_val)
#define WriteEF4Byte(_ptr, _val)	(*((u32 *)(_ptr)))=EF4Byte(_val)									

//
//	Example:
//		BIT_LEN_MASK_32(0) => 0x00000000
//		BIT_LEN_MASK_32(1) => 0x00000001
//		BIT_LEN_MASK_32(2) => 0x00000003
//		BIT_LEN_MASK_32(32) => 0xFFFFFFFF
//
#define BIT_LEN_MASK_32(__BitLen) \
	(0xFFFFFFFF >> (32 - (__BitLen)))
//
//	Example:
//		BIT_OFFSET_LEN_MASK_32(0, 2) => 0x00000003
//		BIT_OFFSET_LEN_MASK_32(16, 2) => 0x00030000
//
#define BIT_OFFSET_LEN_MASK_32(__BitOffset, __BitLen) \
	(BIT_LEN_MASK_32(__BitLen) << (__BitOffset)) 

//
//	Description:
//		Return 4-byte value in host byte ordering from
//		4-byte pointer in litten-endian system.
//
#define LE_P4BYTE_TO_HOST_4BYTE(__pStart) \
	(EF4Byte(*((u32 *)(__pStart))))

//
//	Description:
//		Translate subfield (continuous bits in little-endian) of 4-byte value in litten byte to
//		4-byte value in host byte ordering.
//
#define LE_BITS_TO_4BYTE(__pStart, __BitOffset, __BitLen) \
	( \
		( LE_P4BYTE_TO_HOST_4BYTE(__pStart) >> (__BitOffset) ) \
		& \
		BIT_LEN_MASK_32(__BitLen) \
	)

//
//	Description:
//		Mask subfield (continuous bits in little-endian) of 4-byte value in litten byte oredering  
//		and return the result in 4-byte value in host byte ordering.
//
#define LE_BITS_CLEARED_TO_4BYTE(__pStart, __BitOffset, __BitLen) \
	( \
		LE_P4BYTE_TO_HOST_4BYTE(__pStart) \
		& \
		( ~BIT_OFFSET_LEN_MASK_32(__BitOffset, __BitLen) ) \
	)

//
//	Description:
//		Set subfield of little-endian 4-byte value to specified value.	
//
#define SET_BITS_TO_LE_4BYTE(__pStart, __BitOffset, __BitLen, __Value) \
	*((u32 *)(__pStart)) = \
		EF4Byte( \
			LE_BITS_CLEARED_TO_4BYTE(__pStart, __BitOffset, __BitLen) \
			| \
			( (((u32)__Value) & BIT_LEN_MASK_32(__BitLen)) << (__BitOffset) ) \
		);

		
#define BIT_LEN_MASK_16(__BitLen) \
		(0xFFFF >> (16 - (__BitLen)))
		
#define BIT_OFFSET_LEN_MASK_16(__BitOffset, __BitLen) \
	(BIT_LEN_MASK_16(__BitLen) << (__BitOffset))
	
#define LE_P2BYTE_TO_HOST_2BYTE(__pStart) \
	(EF2Byte(*((u16 *)(__pStart))))
	
#define LE_BITS_TO_2BYTE(__pStart, __BitOffset, __BitLen) \
	( \
		( LE_P2BYTE_TO_HOST_2BYTE(__pStart) >> (__BitOffset) ) \
		& \
		BIT_LEN_MASK_16(__BitLen) \
	)
	
#define LE_BITS_CLEARED_TO_2BYTE(__pStart, __BitOffset, __BitLen) \
	( \
		LE_P2BYTE_TO_HOST_2BYTE(__pStart) \
		& \
		( ~BIT_OFFSET_LEN_MASK_16(__BitOffset, __BitLen) ) \
	)

#define SET_BITS_TO_LE_2BYTE(__pStart, __BitOffset, __BitLen, __Value) \
	*((u16 *)(__pStart)) = \
		EF2Byte( \
			LE_BITS_CLEARED_TO_2BYTE(__pStart, __BitOffset, __BitLen) \
			| \
			( (((u16)__Value) & BIT_LEN_MASK_16(__BitLen)) << (__BitOffset) ) \
		);
			
#define BIT_LEN_MASK_8(__BitLen) \
		(0xFF >> (8 - (__BitLen)))

#define BIT_OFFSET_LEN_MASK_8(__BitOffset, __BitLen) \
	(BIT_LEN_MASK_8(__BitLen) << (__BitOffset))

#define LE_P1BYTE_TO_HOST_1BYTE(__pStart) \
	(EF1Byte(*((u8 *)(__pStart))))

#define LE_BITS_TO_1BYTE(__pStart, __BitOffset, __BitLen) \
	( \
		( LE_P1BYTE_TO_HOST_1BYTE(__pStart) >> (__BitOffset) ) \
		& \
		BIT_LEN_MASK_8(__BitLen) \
	)

#define LE_BITS_CLEARED_TO_1BYTE(__pStart, __BitOffset, __BitLen) \
	( \
		LE_P1BYTE_TO_HOST_1BYTE(__pStart) \
		& \
		( ~BIT_OFFSET_LEN_MASK_8(__BitOffset, __BitLen) ) \
	)

#define SET_BITS_TO_LE_1BYTE(__pStart, __BitOffset, __BitLen, __Value) \
	*((u8 *)(__pStart)) = \
		EF1Byte( \
			LE_BITS_CLEARED_TO_1BYTE(__pStart, __BitOffset, __BitLen) \
			| \
			( (((u8)__Value) & BIT_LEN_MASK_8(__BitLen)) << (__BitOffset) ) \
		);

// Get the N-bytes aligment offset from the current length
#define N_BYTE_ALIGMENT(__Value, __Aligment) ((__Aligment == 1) ? (__Value) : (((__Value + __Aligment - 1) / __Aligment) * __Aligment))

typedef unsigned char	BOOLEAN,*PBOOLEAN;

#endif //__BASIC_TYPES_H__

