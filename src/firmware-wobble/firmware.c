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

#include "ch-config.h"
#include "ch-errno.h"

static CHugConfig _cfg;

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
		if (rc != CH_ERROR_NONE)
			chug_errno_show(rc, FALSE);
	}
}

/**
 * chug_unknown_setup_request_callback:
 **/
int
main(void)
{
	uint8_t hid_interfaces[] = { 0x00 };
	uint8_t dfu_interfaces[] = { 0x01 };
	uint16_t pll_startup = 600;
	OSCTUNEbits.PLLEN = 1;
	while (pll_startup--);

/* Configure interrupts, per architecture */
#ifdef USB_USE_INTERRUPTS
	INTCONbits.PEIE = 1;
	INTCONbits.GIE = 1;
#endif

	/* read config */
	chug_config_read(&_cfg);
	usb_dfu_set_state(DFU_STATE_APP_IDLE);
	usb_init();

	/* we have to tell the USB stack which interfaces to use */
	hid_set_interface_list(hid_interfaces, 1);
	dfu_set_interface_list(dfu_interfaces, 1);

	/* Setup mouse movement. This implementation sends back data for every
	 * IN packet, but sends no movement for all but every delay-th frame.
	 * Adjusting delay will slow down or speed up the movement, which is
	 * also dependent upon the rate at which the host sends IN packets,
	 * which varies between implementations.
	 *
	 * In real life, you wouldn't want to send back data that hadn't
	 * changed, but since there's no real hardware to poll, and since this
	 * example is about showing the HID class, and not about creative ways
	 * to do timing, we send back data every frame. The interested reader
	 * may want to modify it to use the start-of-frame callback for
	 * timing.
	 */
#define WIDTH	2
	uint8_t x_count = WIDTH;
	uint8_t delay = 7;
	int8_t x_direc = 1;

	while (1) {
		/* clear watchdog */
		CLRWDT();

		if (usb_is_configured() &&
		    !usb_in_endpoint_halted(1) &&
		    !usb_in_endpoint_busy(1)) {

			unsigned char *buf = usb_get_in_buffer(1);
			buf[0] = 0x0;
			buf[1] = (--delay)? 0: x_direc;
			buf[2] = 0;
			usb_send_in_buffer(1, 3);

			if (delay == 0) {
				if (--x_count == 0) {
					x_count = WIDTH;
					x_direc *= -1;
				}
				delay = 7;
			}
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
	if (process_hid_setup_request(setup) == 0)
		return 0;
	if (process_dfu_setup_request(setup) == 0)
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
