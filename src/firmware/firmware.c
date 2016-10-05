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

#include "usb.h"

#include <xc.h>
#include <string.h>

#include "usb_config.h"
#include "usb_ch9.h"
#include "usb_hid.h"
#include "usb_dfu.h"

#include "oo_elis1024.h"
#include "mti_23k640.h"
#include "mti_tcn75a.h"
#include "mzt_mcdc04.h"

#include "ch-config.h"
#include "ch-errno.h"
#include "ch-flash.h"

static CHugConfig		 _cfg;
static ChError			 _last_error = CH_ERROR_NONE;
static ChCmd			 _last_error_cmd = CH_CMD_RESET;
static uint16_t			 _integration_time = 0x0;
static uint8_t			 _chug_buf[CH_EP0_TRANSFER_SIZE];
static uint16_t			 _heartbeat_cnt = 0;

#ifdef HAVE_MCDC04
MztMcdc04Context		 _mcdc04_ctx;
#endif

#define CH_SRAM_ADDRESS_WRDS		0x6000

void
chug_usb_dfu_set_success_callback(void *context)
{
	/* set the auto-boot flag to true */
	if (_cfg.flash_success != 0x01) {
		uint8_t rc;
		_cfg.flash_success = TRUE;
		rc = chug_config_write(&_cfg);
		if (rc != CH_ERROR_NONE)
			chug_errno_show(rc, FALSE);
	}
}

static void
chug_set_error(ChCmd cmd, ChError status)
{
	_last_error = status;
	_last_error_cmd = cmd;
}

static void
chug_set_leds_internal(uint8_t leds)
{
	/* the first few boards on the P&P machines had the
	 * LEDs soldered the wrong way around */
	if ((_cfg.pcb_errata & CH_PCB_ERRATA_SWAPPED_LEDS) > 0) {
		PORTEbits.RE0 = (leds & CH_STATUS_LED_GREEN);
		PORTEbits.RE1 = (leds & CH_STATUS_LED_RED) >> 1;
	} else {
		PORTEbits.RE0 = (leds & CH_STATUS_LED_RED) >> 1;
		PORTEbits.RE1 = (leds & CH_STATUS_LED_GREEN);
	}
}

static void
chug_set_leds(uint8_t leds)
{
	_heartbeat_cnt = 0xffff;
	chug_set_leds_internal(leds);
}

static uint8_t
chug_get_leds_internal(void)
{
	return (PORTEbits.RE1 << 1) + PORTEbits.RE0;
}

static void
chug_set_illuminants(uint8_t illuminants)
{
	PORTCbits.RC0 = (illuminants & CH_ILLUMINANT_A);
	PORTAbits.RA5 = (illuminants & CH_ILLUMINANT_UV) >> 1;
}

static uint8_t
chug_get_illuminants(void)
{
	return (PORTCbits.RC6 << 1) + PORTCbits.RC0;
}

static void
chug_heatbeat(uint8_t leds)
{
	/* disabled */
	if (_heartbeat_cnt == 0xffff)
		return;

	/* do pulse up -> down -> up */
	if (_heartbeat_cnt <= 0xff) {
		uint8_t j;
		uint8_t _duty = _heartbeat_cnt;
		if (_duty > 127)
			_duty = 0xff - _duty;
		for (j = 0; j < _duty * 2; j++)
			chug_set_leds_internal(leds);
		for (j = 0; j < 0xff - _duty * 2; j++)
			chug_set_leds_internal(0);
	}

	/* this is the 'pause' btween the bumps */
	if (_heartbeat_cnt++ > 0xc000)
		_heartbeat_cnt = 0;
}

#define HAVE_TESTS

int
main(void)
{
	uint8_t dfu_interfaces[] = { 0x01 };

#ifdef HAVE_TCN75A
	/* set up TCN75A */
	SSP1ADD = 0x3e;
	OpenI2C1(MASTER, SLEW_ON);
	mti_tcn75a_set_resolution(MTI_TCN75A_RESOLUTION_1_16C);
#endif

#ifdef HAVE_SRAM
	/* set up SPI2 on remappable pins */
	RPINR21 = 19;			/* RP19 = SDI2 */
	RPINR22 = 12;			/* RP12 = SCK2 (input and output) */
	RPOR12 = 0x0a;			/* RP12 = SCK2 */
	RPOR13 = 0x09;			/* RP13 = SDO2 */
	RPOR20 = 0x0c;			/* RP20 = SS2 (SSDMA) */

	/* turn on the SPI bus */
	OpenSPI2(SPI_FOSC_4,MODE_00,SMPMID);

	/* set up the DMA controller */
	DMACON1bits.SSCON0 = 0;		/* SSDMA (_CS) is not DMA controlled */
	DMACON1bits.SSCON1 = 0;
	DMACON1bits.DLYINTEN = 0;	/* don't interrupt after each byte */
	DMACON2bits.INTLVL = 0x0;	/* interrupt only when complete */
	DMACON2bits.DLYCYC = 0x02;	/* minimum delay between bytes */

	/* clear base SRAM memory */
	mti_23k640_wipe(0x0000, 0x2000);
#endif

#ifdef HAVE_ELIS1024
	/* set up the ADC */
	ADCON0bits.VCFG1 = 0;		/* reference is VSS, no hardware VRef- */
	ADCON0bits.VCFG0 = 0;		/* reference is VDD, no hardware VRef+ */
	ADCON0bits.CHS = 0b0000;	/* input (AN0) */
	ADCON0bits.ADON = 1;		/* enable module */
	ADCON1bits.ACQT = 0b111;	/* A/D Acquisition Time Select (FIXME: can we disable this?) */
	ADCON1bits.ADCS = 0b010;	/* A/D Conversion Clock Select (Fosc/32) */
	ANCON1bits.VBGEN = 0;		/* enable band gap reference */
	ANCON0bits.PCFG0 = 0;		/* AN0 = analog */

	/* perform ADC calibration */
	ADCON1bits.ADCAL = 1;
	ADCON0bits.GO = 1;
	while (ADCON0bits.GO);
	ADCON1bits.ADCAL = 0;

	/* power down sensor */
	oo_elis1024_set_standby();
#endif

	/* ensure both UV and A illuminants turned off */
	chug_set_illuminants(CH_ILLUMINANT_NONE);

#ifdef HAVE_MCDC04
	/* set up MCDC04 */
	mzt_mcdc04_init(&_mcdc04_ctx);
	mzt_mcdc04_set_tint(&_mcdc04_ctx, MZT_MCDC04_TINT_512);
	mzt_mcdc04_set_iref(&_mcdc04_ctx, MZT_MCDC04_IREF_20);
	mzt_mcdc04_set_div(&_mcdc04_ctx, MZT_MCDC04_DIV_DISABLE);
#endif

#ifdef HAVE_TESTS
	/* optional tests */
	if (mti_23k640_self_test() != 0)
		chug_errno_show(CH_ERROR_SRAM_FAILED, TRUE);
#endif

	/* read config */
	chug_config_read(&_cfg);
	usb_dfu_set_state(DFU_STATE_APP_IDLE);
	usb_init();

	/* we have to tell the USB stack which interfaces to use */
	dfu_set_interface_list(dfu_interfaces, 1);

	while (1) {
		/* clear watchdog */
		CLRWDT();
		usb_service();
		chug_heatbeat(CH_STATUS_LED_RED);
	}

	return 0;
}

static int8_t
_send_data_stage_cb(bool transfer_ok, void *context)
{
	/* error */
	if (!transfer_ok) {
		chug_errno_show(CH_ERROR_INVALID_ADDRESS, FALSE);
		return -1;
	}
	return 0;
}

static int8_t
chug_handle_get_temperature(void)
{
#ifdef HAVE_TCN75A
	int32_t tmp;
	uint8_t rc;
	rc = mti_tcn75a_get_temperature(&tmp);
	if (rc != CH_ERROR_NONE) {
		chug_set_error(CH_CMD_GET_TEMPERATURE, rc);
		return -1;
	}
	memcpy(_chug_buf, &tmp, sizeof(int32_t));
	usb_send_data_stage(_chug_buf, sizeof(int32_t),
			    _send_data_stage_cb, NULL);
	return 0;
#else
	chug_set_error(CH_CMD_GET_TEMPERATURE, CH_ERROR_NOT_IMPLEMENTED);
	return -1;
#endif
}

static int8_t
chug_handle_read_sram(const struct setup_packet *setup)
{
#ifdef HAVE_SRAM
	if (setup->wLength > sizeof(_chug_buf)) {
		chug_set_error(CH_CMD_READ_SRAM, CH_ERROR_INVALID_LENGTH);
		return -1;
	}
	mti_23k640_dma_to_cpu(setup->wValue, _chug_buf, setup->wLength);
	mti_23k640_dma_wait();
#else
	memset(_chug_buf, 0x00, setup->wLength);
#endif
	usb_send_data_stage(_chug_buf, setup->wLength, _send_data_stage_cb, NULL);
	return 0;
}

static int8_t
_recieve_data_sram_cb(bool transfer_ok, void *context)
{
	const struct setup_packet *setup = (const struct setup_packet *) context;

	/* error */
	if (!transfer_ok) {
		chug_errno_show(CH_ERROR_DEVICE_DEACTIVATED, FALSE);
		return -1;
	}
#ifdef HAVE_SRAM
	mti_23k640_dma_from_cpu(_chug_buf, setup->wValue, setup->wLength);
	mti_23k640_dma_wait();
#endif
	return 0;
}

static int8_t
chug_handle_write_sram(const struct setup_packet *setup)
{
	/* check size */
	if (setup->wLength > sizeof(_chug_buf)) {
		chug_set_error(CH_CMD_WRITE_SRAM, CH_ERROR_INVALID_LENGTH);
		return -1;
	}

	/* receive data */
	usb_start_receive_ep0_data_stage(_chug_buf, setup->wLength,
					 _recieve_data_sram_cb, setup);
	return 0;
}

static int8_t
_recieve_spectral_calibration_cb(bool transfer_ok, void *context)
{
	/* error */
	if (!transfer_ok) {
		chug_errno_show(CH_ERROR_DEVICE_DEACTIVATED, FALSE);
		return -1;
	}

	/* save to EEPROM */
	memcpy(_cfg.wavelength_cal, _chug_buf, sizeof(int32_t) * 4);
	chug_config_write(&_cfg);
	return 0;
}

static int8_t
chug_handle_set_wavelength_calibration(const struct setup_packet *setup)
{
	/* check size */
	if (setup->wLength != sizeof(int32_t) * 4) {
		chug_set_error(CH_CMD_SET_CCD_CALIBRATION, CH_ERROR_INVALID_LENGTH);
		return -1;
	}
	usb_start_receive_ep0_data_stage(_chug_buf, setup->wLength,
					 _recieve_spectral_calibration_cb, NULL);
	return 0;
}

static int8_t
_recieve_crypto_key_cb(bool transfer_ok, void *context)
{
	/* error */
	if (!transfer_ok) {
		chug_errno_show(CH_ERROR_DEVICE_DEACTIVATED, FALSE);
		return -1;
	}

	/* save to EEPROM */
	memcpy(_cfg.signing_key, _chug_buf, sizeof(uint32_t) * 4);
	chug_config_write(&_cfg);
	return 0;
}

static int8_t
chug_handle_set_crypto_key(const struct setup_packet *setup)
{
	/* check size */
	if (setup->wLength != sizeof(uint32_t) * 4) {
		chug_set_error(CH_CMD_SET_CRYPTO_KEY, CH_ERROR_INVALID_LENGTH);
		return -1;
	}

	/* already set */
	if (chug_config_has_signing_key(&_cfg)) {
		chug_set_error(CH_CMD_SET_CRYPTO_KEY, CH_ERROR_WRONG_UNLOCK_CODE);
		return -1;
	}

	usb_start_receive_ep0_data_stage(_chug_buf, setup->wLength,
					 _recieve_crypto_key_cb, NULL);
	return 0;
}

static int8_t
chug_handle_take_reading_spectral(const struct setup_packet *setup)
{
	ChError rc;
	uint16_t offset = 0;

	chug_set_leds(0);
	rc = oo_elis1024_take_sample(_integration_time, offset);
	if (rc != CH_ERROR_NONE) {
		chug_set_error(CH_CMD_TAKE_READING_SPECTRAL, rc);
		return -1;
	}
	usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
	return 0;
}

static int8_t
chug_flash_load_sram(uint16_t addr, uint16_t len)
{
	uint16_t i;
	uint8_t rc;
	const uint16_t buflen = sizeof(_chug_buf);

	/* copy from EEPROM to SRAM */
	for (i = 0; i < len; i += buflen) {
		rc = chug_flash_read(addr + i, _chug_buf, buflen);
		if (rc != 0) {
			chug_set_error(CH_CMD_LOAD_SRAM, rc);
			return -1;
		}
		mti_23k640_dma_from_cpu(_chug_buf, i, buflen);
		mti_23k640_dma_wait();
	}
	usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
	return 0;
}

static int8_t
chug_flash_save_sram (uint16_t addr, uint16_t len)
{
	uint16_t i;
	uint8_t rc;
	const uint16_t buflen = sizeof(_chug_buf);

	/* clear EEPROM */
	rc = chug_flash_erase(addr, len);
	if (rc != 0) {
		chug_set_error(CH_CMD_SAVE_SRAM, rc);
		return -1;
	}

	/* copy from SRAM to EEPROM */
	for (i = 0; i < len; i += buflen) {
		mti_23k640_dma_to_cpu(i, _chug_buf, buflen);
		mti_23k640_dma_wait();
		rc = chug_flash_write(addr + i, _chug_buf, buflen);
		if (rc != 0) {
			chug_set_error(CH_CMD_SAVE_SRAM, rc);
			return -1;
		}
	}
	usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
	return 0;
}

static int8_t
chug_handle_take_reading_xyz(const struct setup_packet *setup)
{
#ifdef HAVE_MCDC04
	ChError rc;
	int32_t *buf = (int32_t *) _chug_buf;

	/* get integer readings */
	rc = mzt_mcdc04_take_readings_auto(&_mcdc04_ctx,
					   &buf[0], &buf[1], &buf[2]);
	if (rc != CH_ERROR_NONE) {
		chug_set_error(CH_CMD_TAKE_READING_XYZ, rc);
		return -1;
	}
	usb_send_data_stage(_chug_buf, sizeof(int32_t) * 3,
			    _send_data_stage_cb, NULL);
	return 0;
#else
	chug_set_error(CH_CMD_TAKE_READING_XYZ, CH_ERROR_NOT_IMPLEMENTED);
	return -1;
#endif
}

int8_t
process_chug_setup_request(struct setup_packet *setup)
{
	int32_t tmp;

	if (setup->REQUEST.destination != DEST_INTERFACE)
		return -1;
	if (setup->REQUEST.type != REQUEST_TYPE_CLASS)
		return -1;
	if (setup->wIndex != CH_USB_INTERFACE)
		return -1;

	/* process commands */
	switch (setup->bRequest) {

	/* device->host */
	case CH_CMD_GET_SERIAL_NUMBER:
		memcpy(_chug_buf, &_cfg.serial_number, 2);
		usb_send_data_stage(_chug_buf, 2, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_GET_LEDS:
		_chug_buf[0] = chug_get_leds_internal();
		usb_send_data_stage(_chug_buf, 1, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_GET_ILLUMINANTS:
		_chug_buf[0] = chug_get_illuminants();
		usb_send_data_stage(_chug_buf, 1, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_GET_PCB_ERRATA:
		_chug_buf[0] = _cfg.pcb_errata;
		usb_send_data_stage(_chug_buf, 1, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_GET_INTEGRAL_TIME:
		memcpy(_chug_buf, &_integration_time, 2);
		usb_send_data_stage(_chug_buf, 2, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_READ_SRAM:
		return chug_handle_read_sram(setup);
	case CH_CMD_GET_CCD_CALIBRATION:
		memcpy(_chug_buf, _cfg.wavelength_cal, sizeof(int32_t) * 4);
		usb_send_data_stage(_chug_buf, sizeof(int32_t) * 4,
				    _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_GET_ADC_CALIBRATION_POS:
		tmp = 0xbfff; /* 0.75 * 0xffff */
		memcpy(_chug_buf, &tmp, sizeof(int32_t));
		usb_send_data_stage(_chug_buf, sizeof(int32_t),
				    _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_GET_ADC_CALIBRATION_NEG:
		tmp = 0x1999; /* 0.10 * 0xffff */
		memcpy(_chug_buf, &tmp, sizeof(int32_t));
		usb_send_data_stage(_chug_buf, sizeof(int32_t),
				    _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_GET_TEMPERATURE:
		return chug_handle_get_temperature();
	case CH_CMD_GET_ERROR:
		_chug_buf[0] = _last_error;
		_chug_buf[1] = _last_error_cmd;
		usb_send_data_stage(_chug_buf, 2, _send_data_stage_cb, NULL);
		return 0;

	/* host->device */
	case CH_CMD_SET_SERIAL_NUMBER:
		_cfg.serial_number = setup->wValue;
		chug_config_write(&_cfg);
		usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_SET_LEDS:
		chug_set_leds(setup->wValue);
		usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_SET_ILLUMINANTS:
		chug_set_illuminants(setup->wValue);
		usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_SET_PCB_ERRATA:
		_cfg.pcb_errata = setup->wValue;
		chug_config_write(&_cfg);
		usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_SET_INTEGRAL_TIME:
		_integration_time = setup->wValue;
		usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_WRITE_SRAM:
		return chug_handle_write_sram(setup);
	case CH_CMD_SET_CCD_CALIBRATION:
		return chug_handle_set_wavelength_calibration(setup);
	case CH_CMD_SET_CRYPTO_KEY:
		return chug_handle_set_crypto_key(setup);

	/* actions */
	case CH_CMD_CLEAR_ERROR:
		chug_set_error(CH_CMD_LAST, CH_ERROR_NONE);
		usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
		return 0;
	case CH_CMD_TAKE_READING_SPECTRAL:
		return chug_handle_take_reading_spectral(setup);
	case CH_CMD_TAKE_READING_XYZ:
		return chug_handle_take_reading_xyz(setup);
	case CH_CMD_LOAD_SRAM:
		/* read the 0x2000 (8k) bytes of shadow memory from eeprom */
		return chug_flash_load_sram(CH_SRAM_ADDRESS_WRDS, 0x2000);
	case CH_CMD_SAVE_SRAM:
		/* write int8_t 0x2000 (8k) bytes of shadow memory to eeprom */
		return chug_flash_save_sram(CH_SRAM_ADDRESS_WRDS, 0x2000);
	default:
		chug_set_error(setup->bRequest, CH_ERROR_UNKNOWN_CMD);
	}
	return -1;
}

int8_t
chug_unknown_setup_request_callback(const struct setup_packet *setup)
{
	if (process_dfu_setup_request(setup) == 0)
		return 0;
	if (process_chug_setup_request((struct setup_packet *) setup) == 0)
		return 0;
	return -1;
}

void
chug_usb_reset_callback(void)
{
	/* reset back into DFU mode */
	if (usb_dfu_get_state() == DFU_STATE_APP_DETACH)
		RESET();
}

void interrupt high_priority
isr()
{
#ifdef USB_USE_INTERRUPTS
	usb_service();
#endif
}
