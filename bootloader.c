/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011-2015 Richard Hughes <richard@hughsie.com>
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

//#define DEVICE_IS_COLORHUG2

#include <xc.h>
#include <string.h>

#include "usb_ch9.h"
#include "usb_config.h"
#include "usb_dfu.h"
#include "usb.h"

#include "ch-config.h"
#include "ch-errno.h"
#include "ch-flash.h"
#include "ch-sram.h"

#pragma config XINST	= OFF		/* turn off extended instruction set */
#pragma config STVREN	= ON		/* Stack overflow reset */
#pragma config WDTEN	= ON		/* Watch Dog Timer (WDT) */
#pragma config CP0	= OFF		/* Code protect */
#pragma config OSC	= HSPLL		/* HS oscillator, PLL enabled, HSPLL used by USB */
#pragma config IESO	= OFF		/* Internal External (clock) Switchover */
#pragma config FCMEN	= ON		/* Fail Safe Clock Monitor */
#pragma config T1DIG	= ON		/* secondary clock Source */
#pragma config LPT1OSC	= OFF		/* low power timer*/
#pragma config WDTPS	= 2048		/* Watchdog Timer Postscaler */
#pragma config DSWDTOSC	= INTOSCREF	/* DSWDT uses INTOSC/INTRC as reference clock */
#pragma config RTCOSC	= T1OSCREF	/* RTCC uses T1OSC/T1CKI as reference clock */
#pragma config DSBOREN	= OFF		/* Zero-Power BOR disabled in Deep Sleep */
#pragma config DSWDTEN	= OFF		/* Deep Sleep Watchdog Timer Enable */
#pragma config DSWDTPS	= 8192		/* Deep Sleep Watchdog Timer Postscale Select 1:8,192 (8.5 seconds) */
#pragma config IOL1WAY	= OFF		/* The IOLOCK bit (PPSCON<0>) can be set and cleared as needed */
#pragma config MSSP7B_EN = MSK7		/* 7 Bit address masking */
#pragma config WPFP	= PAGE_1	/* Write Protect Program Flash Page 0 */
#pragma config WPEND	= PAGE_0	/* Write/Erase protect Flash Memory pages */
#pragma config WPCFG	= OFF		/* Write/Erase Protection of last page Disabled */
#pragma config WPDIS	= OFF		/* Write Protect Disable */
#ifdef DEVICE_IS_COLORHUG2
#pragma config PLLDIV	= 6		/* (24 MHz crystal used on this board) */
#pragma config CPUDIV	= OSC2_PLL2	/* OSC1 = divide by 2 mode */
#else
#pragma config PLLDIV	= 3		/* (12 MHz crystal used on this board) */
#pragma config CPUDIV	= OSC1		/* OSC1 = divide by 1 mode */
#endif

/* flash the LEDs when in bootloader mode */
#ifdef DEVICE_IS_COLORHUG2
#define	BOOTLOADER_FLASH_INTERVAL	0x4000
#else
#define	BOOTLOADER_FLASH_INTERVAL	0x8000
#endif
static uint16_t _led_counter = 0x0;

/* we can flash the EEPROM or the FLASH */
static uint8_t _alt_setting = 0;
static uint8_t _did_upload_or_download = FALSE;
static uint8_t _do_reset = FALSE;
static CHugConfig _cfg;

#define LED_RED				0x02
#define LED_GREEN			0x01
#define CH_EEPROM_ADDR_WRDS		0x4000

#ifdef DEVICE_IS_COLORHUG2
uint8_t _buf_sram_is_crap[64];
#endif

/* This is the state machine used to switch between the different bootloader
 * and firmware modes:
 *
 *  [0. BOOTLOADER] -> [1. FIRMWARE]
 *      ^------------------/
 *
 * Rules for bootloader:
 *  - Run the firmware if !RCON.RI [RESET()] && !RCON.TO [WDT] && AUTO_BOOT=1
 *  - On USB reset, boot the firmware if we've read or written firmware
 *  - If state is dfuDNLD and AUTO_BOOT=1, set AUTO_BOOT=0
 *
 * Rules for firmware:
 *  - On USB reset in appDETACH, do reset() to get back to bootloader
 *  - If GetStatus is serviced and AUTO_BOOT=0, set AUTO_BOOT=1
 *
 */

/**
 * chug_boot_runtime:
 **/
static void
chug_boot_runtime(void)
{
	uint16_t runcode_start = 0xffff;

	chug_flash_read(CH_EEPROM_ADDR_WRDS, (uint8_t *) &runcode_start, 2);
	if (runcode_start == 0xffff)
		chug_errno_show(CHUG_ERRNO_NO_FIRMWARE, TRUE);
	asm("ljmp 0x4000");
	chug_errno_show(CHUG_ERRNO_UNKNOWN, TRUE);
}

/**
 * chug_usb_dfu_write_callback:
 **/
int8_t
chug_usb_dfu_write_callback(uint16_t addr, const uint8_t *data, uint16_t len, void *context)
{
	int8_t rc;

	/* a USB reset will take us to firmware mode */
	_did_upload_or_download = TRUE;

	/* erase EEPROM and write */
	if (_alt_setting == 0x00) {

		/* set the auto-boot flag to false */
		if (_cfg.flash_success) {
			_cfg.flash_success = FALSE;
			chug_config_write(&_cfg);
		}

		/* we have to erase in chunks of 1024 bytes, e.g. every 16 blocks */
		if (addr % CH_FLASH_ERASE_BLOCK_SIZE == 0) {
			rc = chug_flash_erase(addr + CH_EEPROM_ADDR_WRDS,
					      CH_FLASH_ERASE_BLOCK_SIZE);
			if (rc != 0)
				return rc;
		}
		return chug_flash_write(addr + CH_EEPROM_ADDR_WRDS, data, len);
	}

#ifdef DEVICE_IS_COLORHUG2
	/* write to SRAM */
	if (_alt_setting == 0x01) {
//		chug_sram_dma_from_cpu(data, addr, len);
//		chug_sram_dma_wait();
		memcpy(_buf_sram_is_crap, data, 64);
		return 0;
	}
#endif

	/* invalid */
	return -1;
}

/**
 * chug_usb_dfu_read_callback:
 **/
int8_t
chug_usb_dfu_read_callback(uint16_t addr, uint8_t *data, uint16_t len, void *context)
{
	/* a USB reset will take us to firmware mode */
	_did_upload_or_download = TRUE;

	/* read from EEPROM */
	if (_alt_setting == 0x00)
		return chug_flash_read(addr + CH_EEPROM_ADDR_WRDS, data, len);

#ifdef DEVICE_IS_COLORHUG2
	/* read from SRAM */
	if (_alt_setting == 0x01) {
		memcpy(data, _buf_sram_is_crap, 64);
//		chug_sram_dma_to_cpu(addr, data, len);
//		chug_sram_dma_wait();
		return DFU_STATUS_OK;
	}
#endif

	/* invalid */
	return -1;
}

/**
 * main:
 **/
int
main(void)
{
	/* enable the PLL and wait 2+ms until the PLL locks
	 * before enabling USB module */
	uint16_t pll_startup_counter = 1200;
	OSCTUNEbits.PLLEN = 1;
	while (pll_startup_counter--);

	/* default all pins to digital */
	ANCON0 = 0xFF;
	ANCON1 = 0xFF;

	/* set RA0 to input (READY)
	 * set RA1 to input (unused)
	 * set RA2 to input (unused)
	 * set RA3 to input (unused)
	 * set RA4 to input (missing)
	 * set RA5 to input (frequency counter),
	 * (RA6 is "don't care" in OSC2 mode)
	 * set RA7 to input (OSC1, HSPLL in) */
	TRISA = 0b11111111;

	/* set RB0 to input (h/w revision),
	 * set RB1 to input (h/w revision),
	 * set RB2 to input (h/w revision),
	 * set RB3 to input (h/w revision),
	 * set RB4 to input (SCL),
	 * set RB5 to input (SDA),
	 * set RB6 to input (PGC),
	 * set RB7 to input (PGD) */
	TRISB = 0b11111111;

	/* set RC0 to input (unused),
	 * set RC1 to output (SCK2),
	 * set RC2 to output (SDO2)
	 * set RC3 to input (unused)
	 * set RC4 to input (unused)
	 * set RC5 to input (unused)
	 * set RC6 to input (unused
	 * set RC7 to input (unused) */
	TRISC = 0b11111001;

	/* set RD0 to input (unused),
	/* set RD1 to input (unused),
	 * set RD2 to input (SDI2),
	 * set RD3 to output (SS2) [SSDMA?],
	 * set RD4-RD7 to input (unused) */
	TRISD = 0b11110111;

	/* set RE0, RE1 output (LEDs) others input (unused) */
	TRISE = 0x3c;

	/* assign remappable input and outputs */
	RPINR21 = 19;			/* RP19 = SDI2 */
	RPINR22 = 12;			/* RP12 = SCK2 (input and output) */
	RPOR12 = 0x0a;			/* RP12 = SCK2 */
	RPOR13 = 0x09;			/* RP13 = SDO2 */
	RPOR20 = 0x0c;			/* RP20 = SS2 (SSDMA) */

#ifdef DEVICE_IS_COLORHUG2
	/* turn on the SPI bus */
	SSP2STATbits.CKE = 1;		/* enable SMBus-specific inputs */
	SSP2STATbits.SMP = 0;		/* enable slew rate for HS mode */
	SSP2CON1bits.SSPEN = 1;		/* enables the serial port */
	SSP2CON1bits.SSPM = 0x0;	/* SPI master mode, clk = Fosc / 4 */

	/* set up the DMA controller */
	DMACON1bits.SSCON0 = 0;		/* SSDMA (_CS) is not DMA controlled */
	DMACON1bits.SSCON1 = 0;
	DMACON1bits.DLYINTEN = 0;	/* don't interrupt after each byte */
	DMACON2bits.INTLVL = 0x0;	/* interrupt only when complete */
	DMACON2bits.DLYCYC = 0x02;	/* minimum delay between bytes */

	/* clear base SRAM memory */
	chug_sram_wipe(0x0000, 0xffff);
#endif

	/* set up the I2C controller */
	SSP1ADD = 0x3e;
	OpenI2C1(MASTER, SLEW_ON);

	/* set the LED state initially */
	PORTE = LED_GREEN;

/* Configure interrupts, per architecture */
#ifdef USB_USE_INTERRUPTS
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;
#endif

	/* read config and boot to firmware mode if all okay */
	chug_config_read(&_cfg);
	if (RCONbits.NOT_TO && RCONbits.NOT_RI && _cfg.flash_success == 0x01) {
		PORTE = 0x03;
		chug_boot_runtime();
	}

	/* set to known initial status suitable for a bootloader */
	usb_dfu_set_state(DFU_STATE_DFU_IDLE);

#ifdef HAVE_TESTS
	/* optional tests */
	if (chug_config_self_test() != 0)
		chug_errno_show(CHUG_ERRNO_SELF_TEST_EEPROM, TRUE);
	if (chug_sram_self_test() != 0)
		chug_errno_show(CHUG_ERRNO_SELF_TEST_SRAM, TRUE);
#endif

	usb_init();

	while (1) {
		/* clear watchdog */
		CLRWDT();

		/* boot back into firmware */
		if (_do_reset)
			chug_boot_runtime();

		/* flash the LEDs */
		if (--_led_counter == 0) {
			PORTE ^= 0x03;
			_led_counter = BOOTLOADER_FLASH_INTERVAL;
		}
#ifndef USB_USE_INTERRUPTS
		usb_service();
#endif
	}

	return 0;
}

/**
 * chug_unknown_setup_request_callback:
 **/
int8_t
chug_unknown_setup_request_callback(const struct setup_packet *setup)
{
	return process_dfu_setup_request(setup);
}

/**
 * chug_set_interface_callback:
 **/
int8_t
chug_set_interface_callback(uint8_t interface, uint8_t alt_setting)
{
	_alt_setting = alt_setting;
	return 0;
}

/**
 * chug_usb_reset_callback:
 **/
void
chug_usb_reset_callback(void)
{
	/* boot into the firmware if we ever did reading or writing */
	if (_did_upload_or_download)
		_do_reset = TRUE;
}

/**
 * isr:
 **/
void interrupt high_priority
isr()
{
#ifdef USB_USE_INTERRUPTS
	usb_service();
#endif
}