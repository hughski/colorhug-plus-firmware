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

#include "mti_23k640.h"

#include <spi.h>

/* this is for the 23K640 */
typedef enum {
	MTI_23K640_MODE_BYTE		= 0x01,
	MTI_23K640_MODE_PAGE		= 0x81,
	MTI_23K640_MODE_SEQUENTIAL		= 0x41
} ChSramMode;

/* this is for the 23K640 */
typedef enum {
	MTI_23K640_COMMAND_STATUS_WRITE	= 0x01,
	MTI_23K640_COMMAND_DATA_WRITE	= 0x02,
	MTI_23K640_COMMAND_DATA_READ	= 0x03,
	MTI_23K640_COMMAND_STATUS_READ	= 0x05
} ChSramCommand;

/**
 * mti_23k640_enable:
 *
 * This deasserts the SS2/SSDMA/_CS line.
 * We handle this manually, as we want to issue commands before starting
 * the DMA transfer.
 **/
static void
mti_23k640_enable (void)
{
	PORTDbits.RD3 = 0;
}

/**
 * mti_23k640_disable:
 *
 * This asserts the SS2/SSDMA/_CS line.
 **/
static void
mti_23k640_disable (void)
{
	PORTDbits.RD3 = 1;
}

/**
 * mti_23k640_set_mode:
 *
 * The SRAM has different modes to access and write the data.
 * byte: one byte can be transferred.
 * page: 32 bytes at a time within 1024 byte pages
 * sequential: 1024 bytes at a time over multiple pages
 **/
static void
mti_23k640_set_mode (ChSramMode mode)
{
	mti_23k640_enable();
	WriteSPI2(MTI_23K640_COMMAND_STATUS_WRITE);
	WriteSPI2(mode);
	mti_23k640_disable();
}

/**
 * mti_23k640_write_byte:
 *
 * This switches the SRAM to byte mode and reads one byte.
 * This blocks for the duration of the transfer.
 **/
void
mti_23k640_write_byte (uint16_t addr, uint8_t data)
{
	/* byte-write, high-addr, low-addr, data */
	mti_23k640_enable();
	WriteSPI2(MTI_23K640_COMMAND_DATA_WRITE);
	WriteSPI2(addr >> 8);
	WriteSPI2(addr & 0xff);
	WriteSPI2(data);
	mti_23k640_disable();
}

/**
 * mti_23k640_read_byte:
 * This blocks for the duration of the transfer.
 **/
uint8_t
mti_23k640_read_byte (uint16_t addr)
{
	uint8_t data;
	uint8_t addr_h = addr >> 8;

	/* byte-read, high-addr, low-addr, data */
	mti_23k640_enable();
	WriteSPI2(MTI_23K640_COMMAND_DATA_READ);
	WriteSPI2(addr >> 8);
	WriteSPI2(addr & 0xff);
	data = ReadSPI2();
	mti_23k640_disable();
	return data;
}

/**
 * mti_23k640_dma_wait:
 *
 * Hang the CPU waiting for the DMA transfer to finish,
 * then disable the SRAM.
 **/
void
mti_23k640_dma_wait (void)
{
	while (DMACON1bits.DMAEN)
		ClrWdt();
	mti_23k640_disable();
}

/**
 * CHugSramDmaCheck:
 *
 * Check to see if the DMA transfer has finished, and if so then
 * disable the SRAM.
 *
 * Return value: TRUE if the DMA transfer has finished.
 **/
uint8_t
CHugSramDmaCheck (void)
{
	if (DMACON1bits.DMAEN)
		return FALSE;
	mti_23k640_disable();
	return TRUE;
}

/**
 * mti_23k640_dma_from_cpu_prep:
 **/
void
mti_23k640_dma_from_cpu_prep (void)
{
	/* set the SPI DMA engine to send-only data mode*/
	DMACON1bits.DUPLEX1 = 0;
	DMACON1bits.DUPLEX0 = 1;
	DMACON1bits.RXINC = 0;
	DMACON1bits.TXINC = 1;

	/* change mode */
	mti_23k640_set_mode(MTI_23K640_MODE_SEQUENTIAL);
}

/**
 * mti_23k640_dma_from_cpu_exec:
 **/
void
mti_23k640_dma_from_cpu_exec (const uint8_t *addr_cpu, uint16_t addr_ram, uint16_t length)
{
	/* byte-write, high-addr, low-addr, data */
	mti_23k640_enable();
	WriteSPI2(MTI_23K640_COMMAND_DATA_WRITE);
	WriteSPI2(addr_ram >> 8);
	WriteSPI2(addr_ram & 0xff);

	/* actual bytes transferred is DMABC + 1 */
	DMABCH = (length - 1) >> 8;
	DMABCL = (length - 1) & 0xff;

	/* set transmit addr */
	TXADDRH = (uint16_t) addr_cpu >> 8;
	TXADDRL = (uint16_t) addr_cpu & 0xff;

	/* initiate a DMA transaction */
	DMACON1bits.DMAEN = 1;
}

/**
 * mti_23k640_dma_from_cpu:
 *
 * DMA up to 1024 bytes of data from the CPU->SRAM.
 * This method does not block and leaves the SRAM enabled.
 **/
void
mti_23k640_dma_from_cpu (const uint8_t *addr_cpu, uint16_t addr_ram, uint16_t length)
{
	mti_23k640_dma_from_cpu_prep();
	mti_23k640_dma_from_cpu_exec(addr_cpu, addr_ram, length);
}

/**
 * mti_23k640_dma_to_cpu_prep:
 **/
void
mti_23k640_dma_to_cpu_prep (void)
{
	/* set the SPI DMA engine to receive-only data mode */
	DMACON1bits.DUPLEX1 = 0;
	DMACON1bits.DUPLEX0 = 0;
	DMACON1bits.RXINC = 1;
	DMACON1bits.TXINC = 0;

	/* change mode */
	mti_23k640_set_mode(MTI_23K640_MODE_SEQUENTIAL);
}

/**
 * mti_23k640_dma_to_cpu_exec:
 **/
void
mti_23k640_dma_to_cpu_exec (uint16_t addr_ram, uint8_t *addr_cpu, uint16_t length)
{
	/* byte-read, high-addr, low-addr, data */
	mti_23k640_enable();
	WriteSPI2(MTI_23K640_COMMAND_DATA_READ);
	WriteSPI2(addr_ram >> 8);
	WriteSPI2(addr_ram & 0xff);

	/* actual bytes transferred is DMABC + 1 */
	DMABCH = (length - 1) >> 8;
	DMABCL = (length - 1) & 0xff;

	/* set recieve addr */
	RXADDRH = (uint16_t) addr_cpu >> 8;
	RXADDRL = (uint16_t) addr_cpu & 0xff;

	/* initiate a DMA transaction */
	DMACON1bits.DMAEN = 1;
}

/**
 * mti_23k640_dma_to_cpu:
 *
 * DMA up to 1024 bytes of data from the SRAM->CPU.
 * This method does not block and leaves the SRAM enabled.
 **/
void
mti_23k640_dma_to_cpu (uint16_t addr_ram, uint8_t *addr_cpu, uint16_t length)
{
	mti_23k640_dma_to_cpu_prep();
	mti_23k640_dma_to_cpu_exec(addr_ram, addr_cpu, length);
}

/**
 * mti_23k640_wipe:
 *
 * Wipe the SRAM so that it contains known data.
 **/
void
mti_23k640_wipe (uint16_t addr, uint16_t length)
{
	uint8_t buffer[8];
	uint16_t i;

	/* use 0xff as 'clear' */
	for (i = 0; i < 8; i++)
		buffer[i] = 0xff;

	/* blit this to the entire area specified */
	mti_23k640_dma_from_cpu_prep();
	for (i = 0; i < length / 8; i++) {
		ClrWdt();
		mti_23k640_dma_from_cpu_exec(buffer, addr + i * 8, 8);
		mti_23k640_dma_wait();
	}
}

/**
 * mti_23k640_self_test:
 **/
int8_t
mti_23k640_self_test(void)
{
	uint8_t sram_dma[4];
	uint8_t sram_tmp;

	/* check SRAM */
	mti_23k640_write_byte(0x0000, 0xde);
	mti_23k640_write_byte(0x0001, 0xad);
	mti_23k640_write_byte(0x0002, 0xbe);
	mti_23k640_write_byte(0x0003, 0xef);
	sram_tmp = mti_23k640_read_byte(0x0000);
	if (sram_tmp != 0xde)
		return -1;
	sram_tmp = mti_23k640_read_byte(0x0001);
	if (sram_tmp != 0xad)
		return -1;

	/* test DMA to and from SRAM */
	sram_dma[0] = 0xde;
	sram_dma[1] = 0xad;
	sram_dma[2] = 0xbe;
	sram_dma[3] = 0xef;
	mti_23k640_dma_from_cpu(sram_dma, 0x0010, 3);
	mti_23k640_dma_wait();
	sram_dma[0] = 0x00;
	sram_dma[1] = 0x00;
	sram_dma[2] = 0x00;
	sram_dma[3] = 0x00;
	mti_23k640_dma_to_cpu(0x0010, sram_dma, 3);
	mti_23k640_dma_wait();

	if (sram_dma[0] != 0xde &&
	    sram_dma[1] != 0xad &&
	    sram_dma[2] != 0xbe &&
	    sram_dma[3] != 0xef)
		return -1;

	/* success */
	return 0;
}
