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

#include <xc.h>

#include "oo_elis1024.h"
#include "mti_23k640.h"

#define PIN_CLK			PORTCbits.RC6
#define PIN_DATA		PORTDbits.RD1
#define PIN_RM			PORTDbits.RD0
#define PIN_RST			PORTEbits.RE2
#define PIN_SHT			PORTAbits.RA1

void
oo_elis1024_set_standby(void)
{
	PIN_RST = 1;
	PIN_DATA = 1;
	PIN_CLK = 0;
}

static void
oo_elis1024_wait_us(uint16_t cnt)
{
	cnt /= 2;
	for (;cnt > 0; cnt--)
		CLRWDT();
}

static void
oo_elis1024_wait_ms(uint16_t cnt)
{
	for (;cnt > 0; cnt--)
		oo_elis1024_wait_us(996);
}

/*
 * @integration_time: in ms
 *
 * The ELIS is always run with M0=M1=0 which puts it in 1024 pixel mode and
 * is used with Frame Mode Timing, where RM=0
 */
uint8_t
oo_elis1024_take_sample(uint16_t integration_time, uint16_t offset)
{
	uint16_t i;
	uint16_t dma_buf = 0x0;

	/* we read the pixels backwards */
	offset += 1023 * 2;

	/* device reset */
	PIN_DATA = 0;
	PIN_CLK = 0;
	PIN_RST = 1;
	PIN_RM = 0;

	/* PIN_RST needs to be high for at least 200ns */
	for (i = 0; i < 10; i++) {
		PIN_CLK = 1;
		oo_elis1024_wait_us(10);
		PIN_CLK = 0;
		oo_elis1024_wait_us(10);
	}

	/* start integration */
	PIN_RST = 0;
	if (integration_time > 0) {
		PIN_SHT = 1;
		oo_elis1024_wait_ms(integration_time);
		PIN_SHT = 0;
	} else {
		PIN_SHT = 1;
		PIN_SHT = 0;
	}

	/* get first pixel from device */
	PIN_DATA = 1;

	/* DMA to the SRAM while the ADC is in operation */
	mti_23k640_dma_from_cpu_prep();

	/* wait Td then get data */
	oo_elis1024_wait_us(10);
	for (i = 0; i < 1024; i++) {
		PIN_CLK = 1;

		/* start ADC sample */
		ADCON0bits.GO = 1;
		while (ADCON0bits.GO);
		mti_23k640_dma_wait();

		//FIXME: about half way throughout acquisition
		PIN_CLK = 0;

		/* this is high for the first clock cycle */
		PIN_DATA = 0;

		/* save to SRAM */
		dma_buf = ADRES;
		mti_23k640_dma_from_cpu_exec((const uint8_t *) &dma_buf,
					     offset, sizeof(uint16_t));
		offset -= 2;
	}

	/* wait for the last write to complete */
	mti_23k640_dma_wait();
	return 0;
}
