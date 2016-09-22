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

//gcc -Wall -o host host.c `pkg-config gusb colord colorhug --libs --cflags` && ./host

#include <stdlib.h>
#include <glib.h>

#include <colord.h>
#include <colorhug.h>

int
main (int argc, char **argv)
{
	ChPcbErrata errata = 0xff;
	gboolean ret;
	gdouble cal[4];
	gdouble temp = -1.f;
	guint16 integration = 0xffff;
	guint32 serial = 0xffff;
	g_autofree gchar *str = NULL;
	g_autoptr(CdColorXYZ) xyz = NULL;
	g_autoptr(CdSpectrum) sp = NULL;
	g_autoptr(GError) error = NULL;
	g_autoptr(GUsbContext) ctx = NULL;
	g_autoptr(GUsbDevice) dev = NULL;

	/* debugging */
	g_setenv ("G_MESSAGES_DEBUG", "all", TRUE);
	g_setenv ("G_DEBUG", "fatal-criticals", TRUE);

	ctx = g_usb_context_new (&error);
	g_assert_no_error (error);
	g_assert (ctx != NULL);
	g_usb_context_enumerate (ctx);

	g_debug ("finding device");
	dev = g_usb_context_find_by_vid_pid (ctx, 0x273f, 0x1002, &error);
	g_assert_no_error (error);
	g_assert (dev != NULL);

	g_debug ("open");
	ret = ch_device_open_full (dev, NULL, &error);
	g_assert_no_error (error);
	g_assert (ret);

	/* set the serial number and errata */
	if (0) {
		ret = ch_device_set_serial_number (dev, 999, NULL, &error);
		g_assert_no_error (error);
		g_assert (ret);
		ret = ch_device_set_pcb_errata (dev, CH_PCB_ERRATA_SWAPPED_LEDS, NULL, &error);
		g_assert_no_error (error);
		g_assert (ret);
		ret = ch_device_set_ccd_calibration (dev,
						    355, 0.37f, -0.000251235, 0.f,
						    NULL, &error);
		g_assert_no_error (error);
		g_assert (ret);
	}

	/* get the serial number */
	ret = ch_device_get_serial_number (dev, &serial, NULL, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (serial, ==, 999);

	/* get the errata */
	ret = ch_device_get_pcb_errata (dev, &errata, NULL, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (errata, ==, CH_PCB_ERRATA_SWAPPED_LEDS);

	/* set/get integration */
	ret = ch_device_set_integral_time (dev, 1204, NULL, &error);
	g_assert_no_error (error);
	g_assert (ret);
	ret = ch_device_get_integral_time (dev, &integration, NULL, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpint (integration, ==, 1204);

	/* take temp */
	ret = ch_device_get_temperature (dev, &temp, NULL, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_assert_cmpfloat (temp, >, 10);
	g_assert_cmpfloat (temp, <, 30);

	/* get spectral calibration */
	ret = ch_device_get_ccd_calibration (dev,
					     &cal[0], &cal[1], &cal[2], &cal[3],
					     NULL, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_print ("SPECTRAL_CAL=%.1f,%.3f,%.8f,%.8f\n",
		 cal[0], cal[1], cal[2], cal[3]);

if(0){
	/* get XYZ */
	xyz = ch_device_take_reading_xyz (dev, 0, NULL, &error);
	g_assert_no_error (error);
	g_assert (xyz != NULL);
	g_print ("XYZ=%.2f,%.2f,%.2f\n", xyz->X, xyz->Y, xyz->Z);
}

	/* take reading */
	g_print ("GO...");
	ret = ch_device_take_reading_spectral (dev, CH_SPECTRUM_KIND_RAW, NULL, &error);
	g_assert_no_error (error);
	g_assert (ret);
	g_print ("STOP\n");

	/* get spetrum */
	sp = ch_device_get_spectrum (dev, NULL, &error);
	if (sp == NULL)
	g_assert_no_error (error);
	g_assert (sp != NULL);
	str = cd_spectrum_to_string (sp, 60, 10);
	g_print ("%s\n", str);

	return 0;
}
