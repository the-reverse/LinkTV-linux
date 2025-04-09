/*
 * \file util.h
 * Utilityfunctions.
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

#ifndef __MTP__UTIL__H
#define __MTP__UTIL__H

void data_dump(void *buf, uint32_t nbytes);
void data_dump_ascii (void *buf, uint32_t n, uint32_t dump_boundry);

char *strdup(char *s);
#ifndef HAVE_STRNDUP
char *strndup (const char *s, size_t n);
#endif

#ifndef __HAVE_ARCH_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#endif

void *LIBMTP_realloc(const void *p, size_t new_size , size_t old_size);
void *bsearch(const void *key, const void *base, size_t num, size_t width, int (*compare)(const void *, const void *));

/**
 * Info macro
 */
#define LIBMTP_INFO(format, args...) \
  do { \
    if (LIBMTP_debug != 0) \
      printk(KERN_INFO "LIBMTP %s[%d]: " format, __FUNCTION__, __LINE__, ##args); \
	else \
      printk(KERN_INFO format, ##args); \
  } while (0)

/**
 * Error macro
 */
#define LIBMTP_ERROR(format, args...) \
  do { \
    if (LIBMTP_debug != 0) \
      printk("LIBMTP %s[%d]: " format, __FUNCTION__, __LINE__, ##args); \
	else \
      printk(KERN_ERR format, ##args); \
  } while (0)


#endif //__MTP__UTIL__H
