/**
 * \file util.c
 *
 * This file contains generic utility functions such as can be
 * used for debugging for example.
 *
 * Copyright (C) 2005-2007 Linus Walleij <triad@df.lth.se>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/parser.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/slab.h>

/* MSVC does not have these */
/*
#ifndef _MSC_VER
#include <sys/time.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
*/

#include <linux/mtp/libmtp.h>
#include "util.h"

/**
 * This dumps out a number of bytes to a textual, hexadecimal
 * dump.
 *
 * @param f the file to dump to (e.g. stdout or stderr)
 * @param buf a pointer to the buffer containing the bytes to
 *            be dumped out in hex
 * @param n the number of bytes to dump from this buffer
 */
void data_dump (void *buf, uint32_t n)
{
  unsigned char *bp = (unsigned char *) buf;
  uint32_t i;
  
  for (i = 0; i < n; i++) {
    printk("%02x ", *bp);
    bp++;
  }
  printk("\n");
}

/**
 * This dumps out a number of bytes to a textual, hexadecimal
 * dump, and also prints out the string ASCII representation
 * for each line of bytes. It will also print the memory address
 * offset from a certain boundry.
 *
 * @param f the file to dump to (e.g. stdout or stderr)
 * @param buf a pointer to the buffer containing the bytes to
 *            be dumped out in hex
 * @param n the number of bytes to dump from this buffer
 * @param dump_boundry the address offset to start at (usually 0)
 */
void data_dump_ascii (void *buf, uint32_t n, uint32_t dump_boundry)
{
  uint32_t remain = n;
  uint32_t ln, lc;
  int i;
  unsigned char *bp = (unsigned char *) buf;
  
  lc = 0;
  while (remain) {
    printk("\t%04x:", dump_boundry-0x10);
    
    ln = ( remain > 16 ) ? 16 : remain;
    
    for (i = 0; i < ln; i++) {
      if ( ! (i%2) ) printk(" ");
      printk("%02x", bp[16*lc+i]);
    }
    
    if ( ln < 16 ) {
      int width = ((16-ln)/2)*5 + (2*(ln%2));
      printk("%*.*s", width, width, "");
    }
    
    printk("\t");
    for (i = 0; i < ln; i++) {
      unsigned char ch= bp[16*lc+i];
      printk("%c", ( ch >= 0x20 && ch <= 0x7e ) ? 
	      ch : '.');
    }
    printk("\n");
    
    lc++;
    remain -= ln;
    dump_boundry += ln;
  }
}

char *strdup(char *s)
{
    char *rv = kmalloc(strlen(s)+1, GFP_KERNEL);
    if (rv)
        strcpy(rv, s);
    return rv;
}

#ifndef HAVE_STRNDUP
char *strndup (const char *s, size_t n)
{
  size_t len = strlen (s);
  char *ret;

  if (len <= n)
    return strdup ((char *) s);

  ret = kmalloc(n + 1,GFP_KERNEL);
  strncpy(ret, s, n);
  ret[n] = '\0';
  return ret;
}
#endif

#ifndef __HAVE_ARCH_STRCASECMP
int strcasecmp(const char *s1, const char *s2)
{
    int c1, c2;

    do {
        c1 = tolower(*s1++);
        c2 = tolower(*s2++);
    } while (c1 == c2 && c1 != 0);
    return c1 - c2;
}
EXPORT_SYMBOL(strcasecmp);
#endif

/**
 * LIBMTP_realloc - reallocate memory. The contents will remain unchanged.
 * @p: object to reallocate memory for.
 * @new_size: how many bytes of memory are required.
 *
 * The contents of the object pointed to are preserved up to the
 * lesser of the new and old sizes.  If @p is %NULL, LIBMTP_realloc()
 * behaves exactly like kmalloc().  If @size is 0 and @p is not a
 * %NULL pointer, the object pointed to is freed.
 */

void *LIBMTP_realloc(const void *p, size_t new_size , size_t old_size)
{
    void *ret;

    // printk("%s : p = 0x%x , new_size = %d , old_size = %d\n",__func__,p,new_size,old_size);

    if (p == NULL)
    {
        ret = kmalloc(new_size, GFP_KERNEL); 
        
        if (ret)         
            memset(ret,0,new_size);
                    
        return ret;  
    }    

    if (unlikely(!new_size)) {
        if (p)
            kfree(p);
        return NULL;
    }

    ret = kmalloc(new_size, GFP_KERNEL);
    
    if (ret)
    {    
        memset(ret,0,new_size);
        memcpy(ret,p,old_size);
    }
    
    kfree(p);
        
    return ret;        
}

void *bsearch(const void *key, const void *base, size_t num, size_t width, int (*compare)(const void *, const void *))
{
  char *lo = (char *) base;
  char *hi = (char *) base + (num - 1) * width;
  char *mid;
  unsigned int half;
  int result;

  while (lo <= hi)
  {
    if ((half = num / 2))
    {
      mid = lo + (num & 1 ? half : (half - 1)) * width;
      if (!(result = (*compare)(key,mid)))
        return mid;
      else if (result < 0)
      {
        hi = mid - width;
        num = num & 1 ? half : half-1;
      }
      else    
      {
        lo = mid + width;
        num = half;
      }
    }
    else if (num)
      return ((*compare)(key,lo) ? NULL : lo);
    else
      break;
  }

  return NULL;
}

