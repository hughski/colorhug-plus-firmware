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

#include <xc.h>
#include <string.h>

#include "usb_ch9.h"
#include "usb_config.h"
#include "usb_dfu.h"
#include "usb.h"

#include "ch-config.h"
#include "ch-errno.h"
#include "ch-flash.h"

#pragma config XINST	= OFF		/* turn off extended instruction set */
#pragma config STVREN	= ON		/* Stack overflow reset */
#pragma config WDTEN	= ON		/* Watch Dog Timer (WDT) */
#pragma config CP0	= ON		/* Code protect */
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
#pragma config WPCFG	= ON		/* Write/Erase Protection of last page */
#pragma config WPDIS	= OFF		/* Write Protect Disable */
#ifdef HAVE_24MHZ
#pragma config PLLDIV	= 6		/* (24 MHz crystal used on this board) */
#pragma config CPUDIV	= OSC2_PLL2	/* OSC1 = divide by 2 mode */
#else
#pragma config PLLDIV	= 3		/* (12 MHz crystal used on this board) */
#pragma config CPUDIV	= OSC1		/* OSC1 = divide by 1 mode */
#endif

/* flash the LEDs when in bootloader mode */
#ifdef HAVE_24MHZ
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

#define CH_STATUS_LED_RED		0x02
#define CH_STATUS_LED_GREEN		0x01
#define CH_EEPROM_ADDR_WRDS		0x8000

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

static void
chug_boot_runtime(void)
{
	uint16_t runcode_start = 0xffff;

	chug_flash_read(CH_EEPROM_ADDR_WRDS, (uint8_t *) &runcode_start, 2);
	if (runcode_start == 0xffff)
		chug_errno_show(CH_ERROR_DEVICE_DEACTIVATED, TRUE);
	asm("ljmp 0x8000");
	chug_errno_show(CH_ERROR_NOT_IMPLEMENTED, TRUE);
}

#define XTEA_DELTA		0x9e3779b9
#define XTEA_NUM_ROUNDS		32

static void
chug_xtea_encode (const uint32_t key[4], uint8_t *data, uint16_t length)
{
	uint32_t sum;
	uint32_t *tmp = (uint32_t *) data;
	uint32_t v0;
	uint32_t v1;
	uint8_t i;
	uint8_t j;

	for (j = 0; j < length / 4; j += 2) {
		sum = 0;
		v0 = tmp[j];
		v1 = tmp[j+1];
		for (i = 0; i < XTEA_NUM_ROUNDS; i++) {
			v0 += (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
			sum += XTEA_DELTA;
			v1 += (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
		}
		tmp[j] = v0;
		tmp[j+1] = v1;
	}
}

static void
chug_xtea_decode (const uint32_t key[4], uint8_t *data, uint16_t length)
{
	uint32_t sum;
	uint32_t *tmp = (uint32_t *) data;
	uint32_t v0;
	uint32_t v1;
	uint8_t i;
	uint8_t j;

	for (j = 0; j < length / 4; j += 2) {
		v0 = tmp[j];
		v1 = tmp[j+1];
		sum = XTEA_DELTA * XTEA_NUM_ROUNDS;
		for (i = 0; i < XTEA_NUM_ROUNDS; i++) {
			v1 -= (((v0 << 4) ^ (v0 >> 5)) + v0) ^ (sum + key[(sum >> 11) & 3]);
			sum -= XTEA_DELTA;
			v0 -= (((v1 << 4) ^ (v1 >> 5)) + v1) ^ (sum + key[sum & 3]);
		}
		tmp[j] = v0;
		tmp[j+1] = v1;
	}
}

int8_t
chug_usb_dfu_write_callback(uint16_t addr, uint8_t *data, uint16_t len, void *context)
{
	int8_t rc;

	/* a USB reset will take us to firmware mode */
	_did_upload_or_download = TRUE;

	/* erase EEPROM and write */
	if (_alt_setting == 0x00) {

		/* decrypt */
		if (chug_config_has_signing_key(&_cfg)) {
			uint32_t *key = _cfg.signing_key;
			chug_xtea_decode(key, data, len);
		}

		/* check this looks like a valid firmware by checking the
		 * interrupt vector values; these will be 0x1200 for proper
		 * firmware or 0x0000 if the firmware doesn't handle them */
		if (addr == 0x0000) {
			uint16_t *vectors = (uint16_t *) (data + 4);
			if ((vectors[0] != 0x0000 && vectors[0] != 0x1200) ||
			    (vectors[1] != 0x0000 && vectors[1] != 0x1200)) {
				usb_dfu_set_status(DFU_STATUS_ERR_FILE);
				return -1;
			}
		}

		/* set the auto-boot flag to false */
		if (_cfg.flash_success) {
			_cfg.flash_success = FALSE;
			chug_config_write(&_cfg);
		}

		/* we have to erase in chunks of 1024 bytes, e.g. every 16 blocks */
		if (addr % CH_FLASH_ERASE_BLOCK_SIZE == 0) {
			rc = chug_flash_erase(addr + CH_EEPROM_ADDR_WRDS,
					      CH_FLASH_ERASE_BLOCK_SIZE);
			if (rc != 0) {
				usb_dfu_set_status(DFU_STATUS_ERR_ERASE);
				return -1;
			}
		}

		/* write */
		rc = chug_flash_write(addr + CH_EEPROM_ADDR_WRDS, data, len);
		if (rc != 0) {
			usb_dfu_set_status(DFU_STATUS_ERR_WRITE);
			return -1;
		}
		return 0;
	}

	/* invalid */
	return -1;
}

int8_t
chug_usb_dfu_read_callback(uint16_t addr, uint8_t *data, uint16_t len, void *context)
{
	uint8_t rc;

	/* invalid */
	if (_alt_setting != 0x00)
		return -1;

	/* a USB reset will take us to firmware mode */
	_did_upload_or_download = TRUE;

	/* read from EEPROM */
	rc = chug_flash_read(addr + CH_EEPROM_ADDR_WRDS, data, len);
	if (rc != CH_ERROR_NONE)
		return -1;

	/* re-encrypt */
	if (chug_config_has_signing_key(&_cfg)) {
		uint32_t *key = _cfg.signing_key;
		chug_xtea_encode(key, data, len);
	}
	return 0;
}

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

	/* set RA0 to input (ELIS_VIDEO)
	 * set RA1 to output (ELIS_SHT)
	 * set RA2 to input (AN2[ref-])
	 * set RA3 to input (AN3[ref+])
	 * set RA4 to input (missing)
	 * set RA5 to output (LED3),
	 * (RA6 is "don't care" in OSC2 mode)
	 * set RA7 to input (OSC1, HSPLL in) */
	TRISA = 0b11011101;

	/* set RB0 to input (h/w revision)
	 * set RB1 to input (h/w revision)
	 * set RB2 to input (h/w revision)
	 * set RB3 to input (h/w revision)
	 * set RB4 to input (SCL)
	 * set RB5 to input (SDA & Unlock)
	 * set RB6 to input (PGC)
	 * set RB7 to input (PGD) */
	TRISB = 0b11111111;

	/* set RC0 to output (LED2)
	 * set RC1 to output (SCK2)
	 * set RC2 to output (SDO2)
	 * set RC3 to input (unused)
	 * set RC4 to input (USB-)
	 * set RC5 to input (USB+)
	 * set RC6 to output (ELIS_CLK)
	 * set RC7 to input (unused) */
	TRISC = 0b10111000;

	/* set RD0 to output (ELIS_RM)
	 * set RD1 to output (ELIS_DATA)
	 * set RD2 to input (SDI2)
	 * set RD3 to output (SS2) [SSDMA?]
	 * set RD4-RD7 to input (unused) */
	TRISD = 0b11110100;

	/* set RE0 to output (LED0)
	 * set RE1 to output (LED1)
	 * set RE2 to output (ELIS_RST) */
	TRISE = 0b11111000;

	/* set the LED state initially */
	PORTE = CH_STATUS_LED_GREEN;

/* Configure interrupts, per architecture */
#ifdef USB_USE_INTERRUPTS
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;
#endif

	/* read config */
	chug_config_read(&_cfg);

	/* if the unlock pins are jumpered then clear the signing keys */
	if (PORTBbits.RB5 == 0) {
		_cfg.signing_key[0] = 0x0;
		_cfg.signing_key[1] = 0x0;
		_cfg.signing_key[2] = 0x0;
		_cfg.signing_key[3] = 0x0;
		chug_config_write(&_cfg);
		PORTE = 0x03;
		while(1);
	}

	/* boot to firmware mode if all okay */
	if (RCONbits.NOT_TO && RCONbits.NOT_RI && _cfg.flash_success == 0x01) {
		PORTE = 0x03;
		chug_boot_runtime();
	}

	/* set to known initial status suitable for a bootloader */
	usb_dfu_set_state(DFU_STATE_DFU_IDLE);

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

int8_t
chug_unknown_setup_request_callback(const struct setup_packet *setup)
{
	return process_dfu_setup_request(setup);
}

void
chug_usb_reset_callback(void)
{
	/* boot into the firmware if we ever did reading or writing */
	if (_did_upload_or_download)
		_do_reset = TRUE;
}

void interrupt high_priority
isr()
{
#ifdef USB_USE_INTERRUPTS
	usb_service();
#endif
}
