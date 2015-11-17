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

#include "mti_23k640.h"
#include "mti_tcn75a.h"

#include "ch-common.h"
#include "ch-config.h"
#include "ch-errno.h"

static CHugConfig		 _cfg;
static uint8_t			 _chug_buf[CHUG_TRANSFER_SIZE];
static uint16_t			 _integration_time = 0x0;

/**
 * chug_usb_dfu_set_success_callback:
 **/
void
chug_usb_dfu_set_success_callback(void *context)
{
	/* set the auto-boot flag to true */
	if (_cfg.flash_success != 0x01) {
		uint8_t rc;
		_cfg.flash_success = TRUE;
		rc = chug_config_write(&_cfg);
		if (rc != CHUG_ERRNO_NONE)
			chug_errno_show(rc, FALSE);
	}
}

#define HAVE_TESTS

/**
 * main:
 **/
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

#ifdef HAVE_TESTS
	/* optional tests */
	if (mti_23k640_self_test() != 0)
		chug_errno_show(CHUG_ERRNO_SELF_TEST_SRAM, TRUE);
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
	}

	return 0;
}

static void
_send_data_stage_cb(bool transfer_ok, void *context)
{
	/* error */
	if (!transfer_ok) {
		chug_errno_show(CHUG_ERRNO_ADDRESS, FALSE);
		return;
	}
}

/**
 * chug_handle_get_temperature:
 **/
static int8_t
chug_handle_get_temperature(void)
{
#ifdef HAVE_TCN75A
	uint16_t tmp;
	uint8_t rc;
	rc = mti_tcn75a_get_temperature(&tmp);
	if (rc != CHUG_ERRNO_NONE)
		return -1;
	memcpy(_chug_buf, &tmp, 2);
	usb_send_data_stage(_chug_buf, 2, _send_data_stage_cb, NULL);
	return 0;
#else
	return -1;
#endif
}

/**
 * chug_handle_get_spectrum:
 **/
static int8_t
chug_handle_get_spectrum(const struct setup_packet *setup)
{
	/* raw from sensor */
	memset(_chug_buf, 0x10 + setup->wValue, CHUG_TRANSFER_SIZE);
	usb_send_data_stage(_chug_buf, CHUG_TRANSFER_SIZE, _send_data_stage_cb, NULL);
	return 0;
}

/**
 * _recieve_data_stage_cb:
 **/
static void
_recieve_spectrum_cb(bool transfer_ok, void *context)
{
	/* error */
	if (!transfer_ok) {
		chug_errno_show(CHUG_ERRNO_NO_FIRMWARE, FALSE);
		return;
	}
}

/**
 * chug_handle_set_spectrum:
 **/
static int8_t
chug_handle_set_spectrum(const struct setup_packet *setup)
{
	/* check size */
	if (setup->wLength != 64)
		return -1;

	/* dark calibration */
	if (setup->wValue == CHUG_SPECTRUM_KIND_DARK_CAL) {
		usb_start_receive_ep0_data_stage(_chug_buf, setup->wLength,
						 _recieve_spectrum_cb, NULL);
		return 0;
	}
	return -1;
}

/**
 * process_chug_setup_request:
 **/
int8_t
process_chug_setup_request(struct setup_packet *setup)
{
	if (setup->REQUEST.destination != DEST_INTERFACE)
		return -1;
	if (setup->REQUEST.type != REQUEST_TYPE_CLASS)
		return -1;
	if (setup->wIndex != CHUG_INTERFACE)
		return -1;

	/* process commands */
	switch (setup->bRequest) {

	/* device->host */
	case CHUG_CMD_GET_SERIAL:
		memcpy(_chug_buf, &_cfg.serial_number, 2);
		usb_send_data_stage(_chug_buf, 2, _send_data_stage_cb, NULL);
		return 0;
	case CHUG_CMD_GET_ERRATA:
		_chug_buf[0] = _cfg.pcb_errata;
		usb_send_data_stage(_chug_buf, 1, _send_data_stage_cb, NULL);
		return 0;
	case CHUG_CMD_GET_INTEGRATION:
		memcpy(_chug_buf, &_integration_time, 2);
		usb_send_data_stage(_chug_buf, 2, _send_data_stage_cb, NULL);
		return 0;
	case CHUG_CMD_GET_SPECTRUM:
		return chug_handle_get_spectrum(setup);
	case CHUG_CMD_GET_CALIBRATION:
		//FIXME
		return -1;
	case CHUG_CMD_GET_TEMPERATURE:
		return chug_handle_get_temperature();

	/* host->device */
	case CHUG_CMD_SET_SERIAL:
		_cfg.serial_number = setup->wValue;
		chug_config_write(&_cfg);
		usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
		return 0;
	case CHUG_CMD_SET_ERRATA:
		_cfg.pcb_errata = setup->wValue;
		chug_config_write(&_cfg);
		usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
		return 0;
	case CHUG_CMD_SET_INTEGRATION:
		_integration_time = setup->wValue;
		usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
		return 0;
	case CHUG_CMD_SET_SPECTRUM:
		return chug_handle_set_spectrum(setup);
	case CHUG_CMD_SET_CALIBRATION:
		//FIXME
		return -1;

	/* SLOW action */
	case CHUG_CMD_TAKE_READING:
		{
			uint32_t i;
			for (i = 0; i < 0xfffff; i++) {
				CLRWDT();
			}
		}
		usb_send_data_stage(NULL, 0, _send_data_stage_cb, NULL);
		return 0;

	}
	return -1;
}

/**
 * chug_unknown_setup_request_callback:
 **/
int8_t
chug_unknown_setup_request_callback(const struct setup_packet *setup)
{
	if (process_dfu_setup_request(setup) == 0)
		return 0;
	if (process_chug_setup_request((struct setup_packet *) setup) == 0)
		return 0;
	return -1;
}

/**
 * chug_usb_reset_callback:
 **/
void
chug_usb_reset_callback(void)
{
	/* reset back into DFU mode */
	if (usb_dfu_get_state() == DFU_STATE_APP_DETACH)
		RESET();
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
