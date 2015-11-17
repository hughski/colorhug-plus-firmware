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

#include <glib.h>
#include <string.h>

#include "ch-client.h"

#define CHUG_DEFAULT_USB_TIMEOUT	5000

/**
 * chug_cmd_set_serial:
 * @dev: A #GUsbDevice
 * @value: XXXXXXXXXXXXXXXx
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Sets the serial number on the device
 *
 * Returns: %TRUE for success
 **/
gboolean
chug_cmd_set_serial (GUsbDevice *dev, guint16 value,
		     GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	return g_usb_device_control_transfer (dev,
					      G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
					      G_USB_DEVICE_REQUEST_TYPE_CLASS,
					      G_USB_DEVICE_RECIPIENT_INTERFACE,
					      CHUG_CMD_SET_SERIAL,
					      value,		/* wValue */
					      CHUG_INTERFACE,	/* idx */
					      NULL,		/* data */
					      0,		/* length */
					      NULL,		/* actual_length */
					      CHUG_DEFAULT_USB_TIMEOUT,
					      cancellable,
					      error);
}

/**
 * chug_cmd_get_serial:
 * @dev: A #GUsbDevice
 * @value: (out): serial number
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets the serial number from the device.
 *
 * Returns: %TRUE for success
 **/
gboolean
chug_cmd_get_serial (GUsbDevice *dev, guint16 *value,
		     GCancellable *cancellable, GError **error)
{
	guint8 buf[2];
	gsize actual_length;
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CHUG_CMD_GET_SERIAL,
					     0x00,		/* wValue */
					     CHUG_INTERFACE,	/* idx */
					     buf,		/* data */
					     sizeof(buf),	/* length */
					     &actual_length,
					     CHUG_DEFAULT_USB_TIMEOUT,
					     cancellable,
					     error);
	if (!ret)
		return FALSE;

	/* return result */
	if (actual_length != sizeof(buf)) {
		g_set_error (error,
			     G_USB_DEVICE_ERROR,
			     G_USB_DEVICE_ERROR_IO,
			     "Invalid size, got %li", actual_length);
		return FALSE;
	}
	if (value != NULL)
		memcpy(value, buf, sizeof(buf));
	return TRUE;
}

/**
 * chug_cmd_set_errata:
 * @dev: A #GUsbDevice
 * @value: #CHugPcbErrata, e.g. %CHUG_PCB_ERRATA_SWAPPED_LEDS
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Sets any PCB errata on the device
 *
 * Returns: %TRUE for success
 **/
gboolean
chug_cmd_set_errata (GUsbDevice *dev, CHugPcbErrata value,
		     GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	return g_usb_device_control_transfer (dev,
					      G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
					      G_USB_DEVICE_REQUEST_TYPE_CLASS,
					      G_USB_DEVICE_RECIPIENT_INTERFACE,
					      CHUG_CMD_SET_ERRATA,
					      value,		/* wValue */
					      CHUG_INTERFACE,	/* idx */
					      NULL,		/* data */
					      0,		/* length */
					      NULL,		/* actual_length */
					      CHUG_DEFAULT_USB_TIMEOUT,
					      cancellable,
					      error);
}

/**
 * chug_cmd_get_errata:
 * @dev: A #GUsbDevice
 * @value: (out): #CHugPcbErrata, e.g. %CHUG_PCB_ERRATA_SWAPPED_LEDS
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets any PCB errata from the device.
 *
 * Returns: %TRUE for success
 **/
gboolean
chug_cmd_get_errata (GUsbDevice *dev, CHugPcbErrata *value,
		     GCancellable *cancellable, GError **error)
{
	guint8 buf[1];
	gsize actual_length;
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CHUG_CMD_GET_ERRATA,
					     0x00,		/* wValue */
					     CHUG_INTERFACE,	/* idx */
					     buf,		/* data */
					     sizeof(buf),	/* length */
					     &actual_length,
					     CHUG_DEFAULT_USB_TIMEOUT,
					     cancellable,
					     error);
	if (!ret)
		return FALSE;

	/* return result */
	if (actual_length != sizeof(buf)) {
		g_set_error (error,
			     G_USB_DEVICE_ERROR,
			     G_USB_DEVICE_ERROR_IO,
			     "Invalid size, got %li", actual_length);
		return FALSE;
	}
	if (value != NULL)
		*value = buf[0];
	return TRUE;
}

/**
 * chug_cmd_set_integration:
 * @dev: A #GUsbDevice
 * @value: integration time in ms
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Sets the integration value for the next sample.
 *
 * Returns: %TRUE for success
 **/
gboolean
chug_cmd_set_integration (GUsbDevice *dev, guint16 value,
			  GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	return g_usb_device_control_transfer (dev,
					      G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
					      G_USB_DEVICE_REQUEST_TYPE_CLASS,
					      G_USB_DEVICE_RECIPIENT_INTERFACE,
					      CHUG_CMD_SET_INTEGRATION,
					      value,		/* wValue */
					      CHUG_INTERFACE,	/* idx */
					      NULL,		/* data */
					      0,		/* length */
					      NULL,		/* actual_length */
					      CHUG_DEFAULT_USB_TIMEOUT,
					      cancellable,
					      error);
}

/**
 * chug_cmd_get_integration:
 * @dev: A #GUsbDevice
 * @value: (out) integration time in ms
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets the integration time used for taking the next samples.
 *
 * Returns: %TRUE for success
 **/
gboolean
chug_cmd_get_integration (GUsbDevice *dev, guint16 *value,
			  GCancellable *cancellable, GError **error)
{
	guint8 buf[2];
	gsize actual_length;
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CHUG_CMD_GET_INTEGRATION,
					     0x00,		/* wValue */
					     CHUG_INTERFACE,	/* idx */
					     buf,		/* data */
					     sizeof(buf),	/* length */
					     &actual_length,
					     CHUG_DEFAULT_USB_TIMEOUT,
					     cancellable,
					     error);
	if (!ret)
		return FALSE;

	/* return result */
	if (actual_length != sizeof(buf)) {
		g_set_error (error,
			     G_USB_DEVICE_ERROR,
			     G_USB_DEVICE_ERROR_IO,
			     "Invalid size, got %li", actual_length);
		return FALSE;
	}
	if (value != NULL)
		memcpy(value, buf, sizeof(buf));
	return TRUE;
}

/**
 * chug_cmd_get_temperature:
 * @dev: A #GUsbDevice
 * @value: (out): temperature in Celcius
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets the PCB board temperature from the device.
 *
 * Returns: %TRUE for success
 **/
gboolean
chug_cmd_get_temperature (GUsbDevice *dev, gdouble *value,
			  GCancellable *cancellable, GError **error)
{
	guint8 buf[2];
	gsize actual_length;
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CHUG_CMD_GET_TEMPERATURE,
					     0x00,		/* wValue */
					     CHUG_INTERFACE,	/* idx */
					     buf,		/* data */
					     sizeof(buf),	/* length */
					     &actual_length,
					     CHUG_DEFAULT_USB_TIMEOUT,
					     cancellable,
					     error);
	if (!ret)
		return FALSE;

	/* return result */
	if (actual_length != sizeof(buf)) {
		g_set_error (error,
			     G_USB_DEVICE_ERROR,
			     G_USB_DEVICE_ERROR_IO,
			     "Invalid size, got %li", actual_length);
		return FALSE;
	}
	if (value != NULL) {
		guint16 tmp;
		tmp = GUINT16_FROM_LE (*((guint16 *) buf));
		*value = (gdouble) tmp / (gdouble) 0xff;
	}
	return TRUE;
}

/**
 * chug_cmd_take_reading:
 * @dev: A #GUsbDevice
 * @value: a #CHugSpectrumKind, e.g. %CHUG_SPECTRUM_KIND_RAW
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Takes a reading from the device.
 *
 * Returns: %TRUE for success
 **/
gboolean
chug_cmd_take_reading (GUsbDevice *dev, CHugSpectrumKind value,
		       GCancellable *cancellable, GError **error)
{
	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	return g_usb_device_control_transfer (dev,
					      G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
					      G_USB_DEVICE_REQUEST_TYPE_CLASS,
					      G_USB_DEVICE_RECIPIENT_INTERFACE,
					      CHUG_CMD_TAKE_READING,
					      value,		/* wValue */
					      CHUG_INTERFACE,	/* idx */
					      NULL,		/* data */
					      0,		/* length */
					      NULL,		/* actual_length */
					      CHUG_DEFAULT_USB_TIMEOUT,
					      cancellable,
					      error);
}

/**
 * chug_cmd_get_spectrum:
 * @dev: A #GUsbDevice
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets the spectrum from the device. This queries the device multiple times
 * until the spectrum has been populated.
 *
 * Returns: %TRUE for success
 **/
gboolean
chug_cmd_get_spectrum (GUsbDevice *dev, GCancellable *cancellable, GError **error)
{
	guint16 i;
	gboolean ret;
	guint8 buf[CHUG_TRANSFER_SIZE];
	gsize actual_length;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	for (i = 0; i < 2048 / CHUG_TRANSFER_SIZE; i++) {
		ret = g_usb_device_control_transfer (dev,
						     G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
						     G_USB_DEVICE_REQUEST_TYPE_CLASS,
						     G_USB_DEVICE_RECIPIENT_INTERFACE,
						     CHUG_CMD_GET_SPECTRUM,
						     i,			/* wValue */
						     CHUG_INTERFACE,	/* idx */
						     buf,		/* data */
						     sizeof(buf),	/* length */
						     &actual_length,	/* actual_length */
						     CHUG_DEFAULT_USB_TIMEOUT,
						     cancellable,
						     error);
		if (!ret)
			return FALSE;
		if (actual_length != sizeof(buf)) {
			g_set_error (error,
				     G_USB_DEVICE_ERROR,
				     G_USB_DEVICE_ERROR_IO,
				     "Failed to get spectrum data, got %li",
				     actual_length);
			return FALSE;
		}
	}
	return TRUE;
}
