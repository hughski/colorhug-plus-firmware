/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
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

#ifndef __MTI_23K640_H
#define __MTI_23K640_H

#include <xc.h>
#include <stdint.h>

int8_t		 mti_23k640_self_test		(void);
void		 mti_23k640_write_byte		(uint16_t	 addr,
						 uint8_t	 data);
uint8_t		 mti_23k640_read_byte		(uint16_t	 addr);

void		 mti_23k640_dma_wait		(void);
void		 mti_23k640_wipe		(uint16_t	 addr,
						 uint16_t	 length);
uint8_t		 mti_23k640_dma_check		(void);

void		 mti_23k640_dma_from_cpu_prep	(void);
void		 mti_23k640_dma_from_cpu_exec	(const uint8_t	*addr_cpu,
						 uint16_t	 addr_ram,
						 uint16_t	 length);
void		 mti_23k640_dma_from_cpu	(const uint8_t	*addr_cpu,
						 uint16_t	 addr_ram,
						 uint16_t	 length);

void		 mti_23k640_dma_to_cpu_prep	(void);
void		 mti_23k640_dma_to_cpu_exec	(uint16_t	 addr_ram,
						 uint8_t	*addr_cpu,
						 uint16_t	 length);
void		 mti_23k640_dma_to_cpu		(uint16_t	 addr_ram,
						 uint8_t	*addr_cpu,
						 uint16_t	 length);

#endif /* __MTI_23K640_H */
