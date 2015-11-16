/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __CH_ERRNO_H
#define __CH_ERRNO_H

#include <xc.h>

typedef enum {
	CHUG_ERRNO_NONE			= 0,	/* same as DFU */
	CHUG_ERRNO_ADDRESS		= 8,	/* same as DFU */
	CHUG_ERRNO_NO_FIRMWARE		= 10,	/* same as DFU */
	CHUG_ERRNO_UNKNOWN		= 14,	/* same as DFU */
	CHUG_ERRNO_SELF_TEST_SRAM	= 16,
	CHUG_ERRNO_SELF_TEST_EEPROM	= 17,
	CHUG_ERRNO_LAST
} CHugErrno;

#define LED_RED				0x02
#define LED_GREEN			0x01

void		 chug_errno_show	(CHugErrno	 errno,
					 uint8_t	 is_fatal);

#endif /* __CH_ERRNO_H */
