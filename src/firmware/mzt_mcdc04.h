/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
 *
 * Multi-Channel Programmable Analog Current Integrator with Digital Converter
 * Manufactured by MAZeT GmbH, designed for the JENCOLOR sensor
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

#ifndef __MZT_MCDC04_H
#define __MZT_MCDC04_H

#include "ch-errno.h"

/* Integration Time */
typedef enum {
	MZT_MCDC04_TINT_1,	/* ms, 10 bit precision */
	MZT_MCDC04_TINT_2,	/* ms, 11 bit precision */
	MZT_MCDC04_TINT_4,	/* ms, 12 bit precision */
	MZT_MCDC04_TINT_8,	/* ms, 13 bit precision */
	MZT_MCDC04_TINT_16,	/* ms, 14 bit precision */
	MZT_MCDC04_TINT_32,	/* ms, 15 bit precision */
	MZT_MCDC04_TINT_64,	/* ms, 16 bit precision */
	MZT_MCDC04_TINT_128,	/* ms, 17 bit precision */
	MZT_MCDC04_TINT_256,	/* ms, 18 bit precision */
	MZT_MCDC04_TINT_512,	/* ms, 19 bit precision */
	MZT_MCDC04_TINT_1024	/* ms, 20 bit precision */
} MztMcdc04Tint;

typedef enum {
	MZT_MCDC04_IREF_20,	/* nA */
	MZT_MCDC04_IREF_80,	/* nA */
	MZT_MCDC04_IREF_320,	/* nA */
	MZT_MCDC04_IREF_1280,	/* nA */
	MZT_MCDC04_IREF_5120	/* nA */
} MztMcdc04Iref;

typedef enum {
	MZT_MCDC04_DIV_2,
	MZT_MCDC04_DIV_4,
	MZT_MCDC04_DIV_8,
	MZT_MCDC04_DIV_16,
	MZT_MCDC04_DIV_DISABLE
} MztMcdc04Div;

typedef struct {
	MztMcdc04Tint		tint;
	MztMcdc04Iref		iref;
	MztMcdc04Div		div;
} MztMcdc04Context;

void		 mzt_mcdc04_init		(MztMcdc04Context	*ctx);
void		 mzt_mcdc04_set_tint		(MztMcdc04Context	*ctx,
						 MztMcdc04Tint		 tint);
void		 mzt_mcdc04_set_iref		(MztMcdc04Context	*ctx,
						 MztMcdc04Iref		 iref);
void		 mzt_mcdc04_set_div		(MztMcdc04Context	*ctx,
						 MztMcdc04Div		 div);
ChError	 mzt_mcdc04_write_config	(MztMcdc04Context	*ctx);
ChError	 mzt_mcdc04_take_readings_raw	(MztMcdc04Context	*ctx,
						 int32_t		*x,
						 int32_t		*y,
						 int32_t		*z);
ChError	 mzt_mcdc04_take_readings	(MztMcdc04Context	*ctx,
						 int32_t		*x,
						 int32_t		*y,
						 int32_t		*z);
ChError	 mzt_mcdc04_take_readings_auto	(MztMcdc04Context	*ctx,
						 int32_t		*x,
						 int32_t		*y,
						 int32_t		*z);

#endif /* __MZT_MCDC04_H */
