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

//gcc -Wall -o host host.c ch-client.c `pkg-config gusb --libs --cflags` && ./host

#include <stdlib.h>
#include <glib.h>

#include "ch-common.h"
#include "ch-client.h"

/**
 * main:
 **/
int
main (int argc, char **argv)
{
	guint16 serial = 0xffff;
	guint16 integration = 0xffff;
	gdouble temp = -1.f;
	CHugPcbErrata errata = 0xff;
	g_autoptr(GUsbDevice) dev = NULL;
	g_autoptr(GUsbContext) ctx = NULL;
	g_autoptr(GError) error = NULL;

	ctx = g_usb_context_new (&error);
	if (ctx == NULL)
		g_error ("failed to get devices: %s", error->message);
	g_usb_context_enumerate (ctx);

	g_debug ("finding device");
	dev = g_usb_context_find_by_vid_pid (ctx, 0x273f, 0x1002, &error);
	if (dev == NULL)
		g_error ("failed to find: %s", error->message);

	g_debug ("open");
	if (!g_usb_device_open (dev, &error))
		g_error ("%s", error->message);

	g_debug ("claim interface");
	if (!g_usb_device_claim_interface (dev, CHUG_INTERFACE, 0, &error))
		g_error ("%s", error->message);

	/* set the serial number and errata */
	if (0) {
		if (!chug_cmd_set_serial (dev, 999, NULL, &error))
			g_error ("%s", error->message);
		if (!chug_cmd_set_errata (dev, CHUG_PCB_ERRATA_SWAPPED_LEDS, NULL, &error))
			g_error ("%s", error->message);
	}

	/* get the serial number */
	if (!chug_cmd_get_serial (dev, &serial, NULL, &error))
		g_error ("%s", error->message);
	g_print ("serial=%i\n", serial);

	/* get the errata */
	if (!chug_cmd_get_errata (dev, &errata, NULL, &error))
		g_error ("%s", error->message);
	g_print ("errata=0x%02x\n", errata);

	/* set/get integration */
	if (!chug_cmd_set_integration (dev, 1204, NULL, &error))
		g_error ("%s", error->message);
	if (!chug_cmd_get_integration (dev, &integration, NULL, &error))
		g_error ("%s", error->message);
	g_print ("integration=%i\n", integration);

	/* take reading */
	g_print ("GO...");
	if (!chug_cmd_take_reading (dev, CHUG_SPECTRUM_KIND_RAW, NULL, &error))
		g_error ("%s", error->message);
	g_print ("STOP\n");

	if (!chug_cmd_get_spectrum (dev, NULL, &error))
		g_error ("%s", error->message);

	/* take temp */
	if (!chug_cmd_get_temperature (dev, &temp, NULL, &error))
		g_error ("%s", error->message);
	g_print ("TEMP=%.2f\n", temp);

	return 0;
}
