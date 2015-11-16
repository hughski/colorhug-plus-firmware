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
		if (rc != CHUG_ERRNO_NONE)
			chug_errno_show(rc, FALSE);
	}
}

/**
 * chug_unknown_setup_request_callback:
 **/
int
main(void)
{
	uint8_t dfu_interfaces[] = { 0x01 };
	uint16_t pll_startup = 600;
	OSCTUNEbits.PLLEN = 1;
	while (pll_startup--);

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

int8_t
chug_unknown_setup_request_callback(const struct setup_packet *setup)
{
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
