/*
 *  linux/drivers/mmc/ms.h
 *
 *  Copyright (C) 2003 Russell King, All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
  *
 * Modification History:
 *    #000 2009-07-09 CMYu   modify from protocol.h for ms usage
 */
#ifndef _MS_H
#define _MS_H
/* core-internal functions */
void ms_init_card(struct ms_card *card, struct ms_host *host);
int ms_register_card(struct ms_card *card);
void ms_remove_card(struct ms_card *card);
#endif
