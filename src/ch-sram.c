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

#include "ch-sram.h"

/* this is for the 23K640 */
typedef enum {
	CH_SRAM_MODE_BYTE		= 0x01,
	CH_SRAM_MODE_PAGE		= 0x81,
	CH_SRAM_MODE_SEQUENTIAL		= 0x41
} ChSramMode;

/* this is for the 23K640 */
typedef enum {
	CH_SRAM_COMMAND_STATUS_WRITE	= 0x01,
	CH_SRAM_COMMAND_DATA_WRITE	= 0x02,
	CH_SRAM_COMMAND_DATA_READ	= 0x03,
	CH_SRAM_COMMAND_STATUS_READ	= 0x05
} ChSramCommand;

/**
 * chug_sram_enable:
 *
 * This deasserts the SS2/SSDMA/_CS line.
 * We handle this manually, as we want to issue commands before starting
 * the DMA transfer.
 **/
static void
chug_sram_enable (void)
{
	PORTDbits.RD3 = 0;
}

/**
 * chug_sram_disable:
 *
 * This asserts the SS2/SSDMA/_CS line.
 **/
static void
chug_sram_disable (void)
{
	PORTDbits.RD3 = 1;
}

/**
 * chug_sram_set_mode:
 *
 * The SRAM has different modes to access and write the data.
 * byte: one byte can be transferred.
 * page: 32 bytes at a time within 1024 byte pages
 * sequential: 1024 bytes at a time over multiple pages
 **/
static void
chug_sram_set_mode (ChSramMode mode)
{
	chug_sram_enable();
	SSP2BUF = CH_SRAM_COMMAND_STATUS_WRITE;
	while(!SSP2STATbits.BF);
	SSP2BUF = mode;
	while(!SSP2STATbits.BF);
	chug_sram_disable();
}

/**
 * chug_sram_write_byte:
 *
 * This switches the SRAM to byte mode and reads one byte.
 * This blocks for the duration of the transfer.
 **/
void
chug_sram_write_byte (uint16_t address, uint8_t data)
{
	/* change mode */
	chug_sram_set_mode(CH_SRAM_MODE_BYTE);

	/* byte-write, high-addr, low-addr, data */
	chug_sram_enable();
	SSP2BUF = CH_SRAM_COMMAND_DATA_WRITE;
	while(!SSP2STATbits.BF);
	SSP2BUF = address >> 8;
	while(!SSP2STATbits.BF);
	SSP2BUF = address & 0xff;
	while(!SSP2STATbits.BF);
	SSP2BUF = data;
	while(!SSP2STATbits.BF);
	chug_sram_disable();
}

/**
 * chug_sram_read_byte:
 * This blocks for the duration of the transfer.
 **/
uint8_t
chug_sram_read_byte (uint16_t address)
{
	uint8_t data;

	/* change mode */
	chug_sram_set_mode(CH_SRAM_MODE_BYTE);

	/* byte-read, high-addr, low-addr, data */
	chug_sram_enable();
	SSP2BUF = CH_SRAM_COMMAND_DATA_READ;
	while(!SSP2STATbits.BF);
	SSP2BUF = address >> 8;
	while(!SSP2STATbits.BF);
	SSP2BUF = address & 0xff;
	while(!SSP2STATbits.BF);
	SSP2BUF = 0xff;
	while(!SSP2STATbits.BF);
	data = SSP2BUF;
	chug_sram_disable();

	return data;
}

/**
 * chug_sram_dma_wait:
 *
 * Hang the CPU waiting for the DMA transfer to finish,
 * then disable the SRAM.
 **/
void
chug_sram_dma_wait (void)
{
	while (DMACON1bits.DMAEN)
		ClrWdt();
	chug_sram_disable();
}

/**
 * chug_sram_dma_check:
 *
 * Check to see if the DMA transfer has finished, and if so then
 * disable the SRAM.
 *
 * Return value: TRUE if the DMA transfer has finished.
 **/
uint8_t
chug_sram_dma_check (void)
{
	if (DMACON1bits.DMAEN)
		return FALSE;
	chug_sram_disable();
	return TRUE;
}

/**
 * chug_sram_dma_from_cpu_prep:
 **/
void
chug_sram_dma_from_cpu_prep (void)
{
	/* set the SPI DMA engine to send-only data mode*/
	DMACON1bits.DUPLEX1 = 0;
	DMACON1bits.DUPLEX0 = 1;
	DMACON1bits.RXINC = 0;
	DMACON1bits.TXINC = 1;

	/* change mode */
	chug_sram_set_mode(CH_SRAM_MODE_SEQUENTIAL);
}

/**
 * chug_sram_dma_from_cpu_exec:
 **/
void
chug_sram_dma_from_cpu_exec (const uint8_t *address_cpu, uint16_t address_ram, uint16_t length)
{
	/* byte-write, high-addr, low-addr, data */
	chug_sram_enable();
	SSP2BUF = CH_SRAM_COMMAND_DATA_WRITE;
	while(!SSP2STATbits.BF);
	SSP2BUF = address_ram >> 8;
	while(!SSP2STATbits.BF);
	SSP2BUF = address_ram & 0xff;
	while(!SSP2STATbits.BF);

	/* actual bytes transferred is DMABC + 1 */
	DMABCH = (length - 1) >> 8;
	DMABCL = (length - 1) & 0xff;

	/* set transmit address */
	TXADDRH = (uint16_t) address_cpu >> 8;
	TXADDRL = (uint16_t) address_cpu & 0xff;

	/* initiate a DMA transaction */
	DMACON1bits.DMAEN = 1;
}

/**
 * chug_sram_dma_from_cpu:
 *
 * DMA up to 1024 bytes of data from the CPU->SRAM.
 * This method does not block and leaves the SRAM enabled.
 **/
void
chug_sram_dma_from_cpu (const uint8_t *address_cpu, uint16_t address_ram, uint16_t length)
{
	chug_sram_dma_from_cpu_prep();
	chug_sram_dma_from_cpu_exec(address_cpu, address_ram, length);
}

/**
 * chug_sram_dma_to_cpu_prep:
 **/
void
chug_sram_dma_to_cpu_prep (void)
{
	/* set the SPI DMA engine to receive-only data mode */
	DMACON1bits.DUPLEX1 = 0;
	DMACON1bits.DUPLEX0 = 0;
	DMACON1bits.RXINC = 1;
	DMACON1bits.TXINC = 0;

	/* change mode */
	chug_sram_set_mode(CH_SRAM_MODE_SEQUENTIAL);
}

/**
 * chug_sram_dma_to_cpu_exec:
 **/
void
chug_sram_dma_to_cpu_exec (uint16_t address_ram, uint8_t *address_cpu, uint16_t length)
{
	/* byte-read, high-addr, low-addr, data */
	chug_sram_enable();
	SSP2BUF = CH_SRAM_COMMAND_DATA_READ;
	while(!SSP2STATbits.BF);
	SSP2BUF = address_ram >> 8;
	while(!SSP2STATbits.BF);
	SSP2BUF = address_ram & 0xff;
	while(!SSP2STATbits.BF);

	/* actual bytes transferred is DMABC + 1 */
	DMABCH = (length - 1) >> 8;
	DMABCL = (length - 1) & 0xff;

	/* set recieve address */
	RXADDRH = (uint16_t) address_cpu >> 8;
	RXADDRL = (uint16_t) address_cpu & 0xff;

	/* initiate a DMA transaction */
	DMACON1bits.DMAEN = 1;
}

/**
 * chug_sram_dma_to_cpu:
 *
 * DMA up to 1024 bytes of data from the SRAM->CPU.
 * This method does not block and leaves the SRAM enabled.
 **/
void
chug_sram_dma_to_cpu (uint16_t address_ram, uint8_t *address_cpu, uint16_t length)
{
	chug_sram_dma_to_cpu_prep();
	chug_sram_dma_to_cpu_exec(address_ram, address_cpu, length);
}

/**
 * chug_sram_wipe:
 *
 * Wipe the SRAM so that it contains known data.
 **/
void
chug_sram_wipe (uint16_t address, uint16_t length)
{
	uint8_t buffer[8];
	uint16_t i;

	/* use 0xff as 'clear' */
	for (i = 0; i < 8; i++)
		buffer[i] = 0xff;

	/* blit this to the entire area specified */
	chug_sram_dma_from_cpu_prep();
	for (i = 0; i < length / 8; i++) {
		ClrWdt();
		chug_sram_dma_from_cpu_exec(buffer, address + i * 8, 8);
		chug_sram_dma_wait();
	}
}

/**
 * chug_sram_self_test:
 **/
int8_t
chug_sram_self_test(void)
{
	uint8_t sram_dma[4];
	uint8_t sram_tmp;

	/* check SRAM */
	chug_sram_write_byte(0x0000, 0xde);
	chug_sram_write_byte(0x0001, 0xad);
	chug_sram_write_byte(0x0002, 0xbe);
	chug_sram_write_byte(0x0003, 0xef);
	sram_tmp = chug_sram_read_byte(0x0000);
	if (sram_tmp != 0xde)
		return -1;
	sram_tmp = chug_sram_read_byte(0x0001);
	if (sram_tmp != 0xad)
		return -1;

	/* test DMA to and from SRAM */
	sram_dma[0] = 0xde;
	sram_dma[1] = 0xad;
	sram_dma[2] = 0xbe;
	sram_dma[3] = 0xef;
	chug_sram_dma_from_cpu(sram_dma, 0x0010, 3);
	chug_sram_dma_wait();
	sram_dma[0] = 0x00;
	sram_dma[1] = 0x00;
	sram_dma[2] = 0x00;
	sram_dma[3] = 0x00;
	chug_sram_dma_to_cpu(0x0010, sram_dma, 3);
	chug_sram_dma_wait();

	if (sram_dma[0] != 0xde &&
	    sram_dma[1] != 0xad &&
	    sram_dma[2] != 0xbe &&
	    sram_dma[3] != 0xef)
		return -1;

	/* success */
	return 0;
}
