/*
 *  include/asm-s390/ebcdic.h
 *    EBCDIC -> ASCII, ASCII -> EBCDIC conversion routines.
 *
 *  S390 version
 *    Copyright (C) 1999 IBM Deutschland Entwicklung GmbH, IBM Corporation
 *    Author(s): Martin Schwidefsky <schwidefsky@de.ibm.com>
 */

#ifndef _EBCDIC_H
#define _EBCDIC_H

#ifndef _S390_TYPES_H
#include <types.h>
#endif

extern __u8 _ascebc_500[];   /* ASCII -> EBCDIC 500 conversion table */
extern __u8 _ebcasc_500[];   /* EBCDIC 500 -> ASCII conversion table */
extern __u8 _ascebc[];   /* ASCII -> EBCDIC conversion table */
extern __u8 _ebcasc[];   /* EBCDIC -> ASCII conversion table */
extern __u8 _ebc_tolower[]; /* EBCDIC -> lowercase */
extern __u8 _ebc_toupper[]; /* EBCDIC -> uppercase */

extern __inline__ void
codepage_convert(const __u8 *codepage, volatile __u8 * addr, unsigned long nr)
{
        static const __u16 tr_op[] = { 0xDC00, 0x1000,0x3000 };
        __asm__ __volatile__(
                "   lgr   1,%0\n"
                "   lgr   2,%1\n"
                "   lgr   3,%2\n"
                "   aghi  2,-256\n"
                "   jm    1f\n"
                "0: tr    0(256,1),0(3)\n"
                "   aghi  1,256\n"
                "   aghi  2,-256\n"
                "   jp    0b\n"
                "1: aghi  2,255\n"
                "   jm    2f\n"
                "   ex    2,%3\n"
                "2:"
                : /* no output */ 
                : "a" (addr), "d" (nr), "a" (codepage), "m" (tr_op[0])
                : "cc", "memory", "1", "2", "3" );
}

#define ASCEBC(addr,nr) codepage_convert(_ascebc, addr, nr)
#define EBCASC(addr,nr) codepage_convert(_ebcasc, addr, nr)
#define ASCEBC_500(addr,nr) codepage_convert(_ascebc_500, addr, nr)
#define EBCASC_500(addr,nr) codepage_convert(_ebcasc_500, addr, nr)
#define EBC_TOLOWER(addr,nr) codepage_convert(_ebc_tolower, addr, nr)
#define EBC_TOUPPER(addr,nr) codepage_convert(_ebc_toupper, addr, nr)

#endif

