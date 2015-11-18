/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013 Richard Hughes <richard@hughsie.com>
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

#ifndef __MTI_TCN75A_H
#define __MTI_TCN75A_H

#include <stdint.h>

typedef enum {
	MTI_TCN75A_RESOLUTION_1_2C,
	MTI_TCN75A_RESOLUTION_1_4C,
	MTI_TCN75A_RESOLUTION_1_8C,
	MTI_TCN75A_RESOLUTION_1_16C
} MtiTcn75aResolution;

uint8_t	 mti_tcn75a_set_resolution	(MtiTcn75aResolution	 resolution);
uint8_t	 mti_tcn75a_get_temperature	(int32_t		*result);

#endif /* __MTI_TCN75A_H */
