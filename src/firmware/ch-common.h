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

#ifndef __CH_COMMON_H
#define __CH_COMMON_H

#define CHUG_TRANSFER_SIZE	64
#define CHUG_INTERFACE		0x00

typedef enum {
	/* read */
	CHUG_CMD_GET_SERIAL		= 0x00,
	CHUG_CMD_GET_ERRATA		= 0x01,
	CHUG_CMD_GET_INTEGRATION	= 0x02,
	CHUG_CMD_GET_SPECTRUM		= 0x03,
	CHUG_CMD_GET_CALIBRATION	= 0x04,

	/* write */
	CHUG_CMD_SET_SERIAL		= 0x10,
	CHUG_CMD_SET_ERRATA		= 0x11,
	CHUG_CMD_SET_INTEGRATION	= 0x12,
	CHUG_CMD_SET_SPECTRUM		= 0x13,
	CHUG_CMD_SET_CALIBRATION	= 0x14,

	/* read only */
	CHUG_CMD_GET_TEMPERATURE	= 0x20,

	/* action */
	CHUG_CMD_TAKE_READING		= 0x30,
	CHUG_CMD_LAST
} CHugCmd;

typedef enum {
	CHUG_SPECTRUM_KIND_RAW		= 0x00,
	CHUG_SPECTRUM_KIND_DARK_CAL	= 0x01,
	CHUG_SPECTRUM_KIND_TEMP_CAL	= 0x02,
	CHUG_SPECTRUM_KIND_LAST
} CHugSpectrumKind;

typedef enum {
	CHUG_PCB_ERRATA_NONE		= 0,
	CHUG_PCB_ERRATA_SWAPPED_LEDS	= 1 << 0,
	CHUG_PCB_ERRATA_LAST
} CHugPcbErrata;

#endif /* __CH_COMMON_H */
