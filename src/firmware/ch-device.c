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
#include <colorhug.h>

#include "ch-device.h"

/**
 * ch_offset_float_to_double:
 **/
static gdouble
ch_offset_float_to_double (gint32 tmp)
{
	return (gdouble) tmp / (gdouble) 0xffff;
}

/**
 * ch_device_check_status:
 **/
static gboolean
ch_device_check_status (GUsbDevice *dev, GCancellable *cancellable, GError **error)
{
	ChError status;
	ChCmd cmd;

	/* hit hardware */
	if (!ch_device_get_error (dev, &status, &cmd, cancellable, error))
		return FALSE;

	/* formulate error */
	if (status != CH_ERROR_NONE) {
		g_set_error (error,
			     G_USB_DEVICE_ERROR,
			     G_USB_DEVICE_ERROR_IO,
			     "Failed, %s(0x%02x) status was %s(0x%02x)",
			     ch_command_to_string (cmd), cmd,
			     ch_strerror (status), status);
		return FALSE;
	}
	return TRUE;
}

/**
 * ch_device_set_serial_number:
 * @dev: A #GUsbDevice
 * @value: serial number
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Sets the serial number on the device
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_set_serial_number (GUsbDevice *dev, guint16 value,
		     GCancellable *cancellable, GError **error)
{
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CH_CMD_SET_SERIAL_NUMBER,
					     value,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     NULL,		/* data */
					     0,			/* length */
					     NULL,		/* actual_length */
					     CH_DEVICE_USB_TIMEOUT,
					     cancellable,
					     error);
	return ret;
}

/**
 * ch_device_get_serial_number:
 * @dev: A #GUsbDevice
 * @value: (out): serial number
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets the serial number from the device.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_get_serial_number (GUsbDevice *dev, guint16 *value,
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
					     CH_CMD_GET_SERIAL_NUMBER,
					     0x00,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     buf,		/* data */
					     sizeof(buf),	/* length */
					     &actual_length,
					     CH_DEVICE_USB_TIMEOUT,
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
 * ch_device_set_pcb_errata:
 * @dev: A #GUsbDevice
 * @value: #ChPcbErrata, e.g. %CH_PCB_ERRATA_SWAPPED_LEDS
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Sets any PCB errata on the device
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_set_pcb_errata (GUsbDevice *dev, ChPcbErrata value,
			  GCancellable *cancellable, GError **error)
{
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CH_CMD_SET_PCB_ERRATA,
					     value,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     NULL,		/* data */
					     0,			/* length */
					     NULL,		/* actual_length */
					     CH_DEVICE_USB_TIMEOUT,
					     cancellable,
					     error);
	return ret;
}

/**
 * ch_device_get_pcb_errata:
 * @dev: A #GUsbDevice
 * @value: (out): #ChPcbErrata, e.g. %CH_PCB_ERRATA_SWAPPED_LEDS
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets any PCB errata from the device.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_get_pcb_errata (GUsbDevice *dev, ChPcbErrata *value,
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
					     CH_CMD_GET_PCB_ERRATA,
					     0x00,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     buf,		/* data */
					     sizeof(buf),	/* length */
					     &actual_length,
					     CH_DEVICE_USB_TIMEOUT,
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
 * ch_device_set_ccd_calibration:
 * @dev: A #GUsbDevice
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Sets any PCB wavelength_cal on the device
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_set_ccd_calibration (GUsbDevice *dev,
			       gdouble start_nm,
			       gdouble c0,
			       gdouble c1,
			       gdouble c2,
			       GCancellable *cancellable,
			       GError **error)
{
	gboolean ret;
	gint32 buf[4];

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* format data */
	buf[0] = start_nm * (gdouble) 0xffff;
	buf[1] = c0 * (gdouble) 0xffff;
	buf[2] = c1 * (gdouble) 0xffff * 1000.f;
	buf[3] = c2 * (gdouble) 0xffff * 1000.f;

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CH_CMD_SET_CCD_CALIBRATION,
					     0,			/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     (guint8 *) buf,	/* data */
					     sizeof(buf),	/* length */
					     NULL,		/* actual_length */
					     CH_DEVICE_USB_TIMEOUT,
					     cancellable,
					     error);
	if (!ret)
		return FALSE;

	/* check status */
	return ch_device_check_status (dev, cancellable, error);
}

/**
 * ch_device_set_crypto_key:
 * @dev: A #GUsbDevice
 * @keys: a set of XTEA keys
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Sets the firmware signing keys on the device.
 *
 * IMPORTANT: This can only be called once until the device is unlocked.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_set_crypto_key (GUsbDevice *dev,
			  guint32 keys[4],
			  GCancellable *cancellable,
			  GError **error)
{
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CH_CMD_SET_CRYPTO_KEY,
					     0,			/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     (guint8 *) keys,	/* data */
					     sizeof(guint32) * 4, /* length */
					     NULL,		/* actual_length */
					     CH_DEVICE_USB_TIMEOUT,
					     cancellable,
					     error);
	if (!ret)
		return FALSE;

	/* check status */
	return ch_device_check_status (dev, cancellable, error);
}

/**
 * ch_device_get_ccd_calibration:
 * @dev: A #GUsbDevice
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets any PCB wavelength_cal from the device.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_get_ccd_calibration (GUsbDevice *dev,
			       gdouble *start_nm,
			       gdouble *c0,
			       gdouble *c1,
			       gdouble *c2,
			       GCancellable *cancellable,
			       GError **error)
{
	gboolean ret;
	gint32 buf[4];
	gsize actual_length;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CH_CMD_GET_CCD_CALIBRATION,
					     0x00,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     (guint8 *) buf,	/* data */
					     sizeof(buf),	/* length */
					     &actual_length,
					     CH_DEVICE_USB_TIMEOUT,
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
	if (start_nm != NULL) {
		*start_nm = ch_offset_float_to_double (buf[0]);
		*c0 = ch_offset_float_to_double (buf[1]);
		*c1 = ch_offset_float_to_double (buf[2]) / 1000.f;
		*c2 = ch_offset_float_to_double (buf[3]) / 1000.f;
	}

	/* check status */
	return ch_device_check_status (dev, cancellable, error);
}

/**
 * ch_device_set_integral_time:
 * @dev: A #GUsbDevice
 * @value: integration time in ms
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Sets the integration value for the next sample.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_set_integral_time (GUsbDevice *dev, guint16 value,
			     GCancellable *cancellable, GError **error)
{
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CH_CMD_SET_INTEGRAL_TIME,
					     value,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     NULL,		/* data */
					     0,			/* length */
					     NULL,		/* actual_length */
					     CH_DEVICE_USB_TIMEOUT,
					     cancellable,
					     error);
	return ret;
}

/**
 * ch_device_get_integral_time:
 * @dev: A #GUsbDevice
 * @value: (out) integration time in ms
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets the integration time used for taking the next samples.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_get_integral_time (GUsbDevice *dev, guint16 *value,
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
					     CH_CMD_GET_INTEGRAL_TIME,
					     0x00,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     buf,		/* data */
					     sizeof(buf),	/* length */
					     &actual_length,
					     CH_DEVICE_USB_TIMEOUT,
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
 * ch_device_get_temperature:
 * @dev: A #GUsbDevice
 * @value: (out): temperature in Celcius
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets the PCB board temperature from the device.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_get_temperature (GUsbDevice *dev, gdouble *value,
			   GCancellable *cancellable, GError **error)
{
	gint32 buf[1];
	gsize actual_length;
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CH_CMD_GET_TEMPERATURE,
					     0x00,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     (guint8 *) buf,	/* data */
					     sizeof(buf),	/* length */
					     &actual_length,
					     CH_DEVICE_USB_TIMEOUT,
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
		*value = ch_offset_float_to_double (buf[0]);
	return TRUE;
}

/**
 * ch_device_get_error:
 * @dev: A #GUsbDevice
 * @status: (out): a #ChError, e.g. %CH_ERROR_INVALID_ADDRESS
 * @cmd: (out): a #ChCmd, e.g. %CH_CMD_TAKE_READING_SPECTRAL
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets the status for the last operation.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_get_error (GUsbDevice *dev, ChError *status, ChCmd *cmd,
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
					     CH_CMD_GET_ERROR,
					     0x00,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     buf,		/* data */
					     sizeof(buf),	/* length */
					     &actual_length,
					     CH_DEVICE_USB_TIMEOUT,
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
	if (status != NULL)
		*status = buf[0];
	if (cmd != NULL)
		*cmd = buf[1];
	return TRUE;
}

/**
 * ch_device_open_full:
 * @dev: A #GUsbDevice
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Opens the device ready for use.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_open_full (GUsbDevice *dev, GCancellable *cancellable, GError **error)
{
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* open */
	if (!g_usb_device_open (dev, error))
		return FALSE;

	/* claim interface */
	if (!g_usb_device_claim_interface (dev, CH_USB_INTERFACE, 0, error))
		return FALSE;

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CH_CMD_CLEAR_ERROR,
					     0x00,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     NULL,		/* data */
					     0,			/* length */
					     NULL,		/* actual_length */
					     CH_DEVICE_USB_TIMEOUT,
					     cancellable,
					     error);
	if (!ret)
		return FALSE;

	/* check status */
	return ch_device_check_status (dev, cancellable, error);
}

/**
 * ch_device_fixup_error:
 **/
static gboolean
ch_device_fixup_error (GUsbDevice *dev, GCancellable *cancellable, GError **error)
{
	ChError status = 0xff;
	ChCmd cmd = 0xff;

	/* do we match not supported */
	if (error == NULL)
		return FALSE;
	if (!g_error_matches (*error,
			      G_USB_DEVICE_ERROR,
			      G_USB_DEVICE_ERROR_NOT_SUPPORTED))
		return FALSE;

	/* can we get a error enum from the device */
	if (!ch_device_get_error (dev, &status, &cmd, cancellable, NULL))
		return FALSE;

	/* add what we tried to do */
	g_prefix_error (error,
			"Failed [%s(%02x):%s(%02x)]: ",
			ch_command_to_string (cmd), cmd,
			ch_strerror (status), status);
	return FALSE;
}

/**
 * ch_device_take_reading_spectral:
 * @dev: A #GUsbDevice
 * @value: a #ChSpectrumKind, e.g. %CH_SPECTRUM_KIND_RAW
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Takes a reading from the device.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: %TRUE for success
 *
 * Since: 1.3.1
 **/
gboolean
ch_device_take_reading_spectral (GUsbDevice *dev, ChSpectrumKind value,
				 GCancellable *cancellable, GError **error)
{
	gboolean ret;

	g_return_val_if_fail (G_USB_DEVICE (dev), FALSE);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     0x51,		//FIXME: I have no idea
					     //CH_CMD_TAKE_READING_SPECTRAL,
					     value,		/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     NULL,		/* data */
					     0,			/* length */
					     NULL,		/* actual_length */
					     CH_DEVICE_USB_TIMEOUT,
					     cancellable,
					     error);
	if (!ret)
		return ch_device_fixup_error (dev, cancellable, error);

	/* check status */
	return ch_device_check_status (dev, cancellable, error);
}

/**
 * ch_device_take_reading_xyz:
 * @dev: A #GUsbDevice
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Takes a reading from the device and returns the XYZ value.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: a #CdColorXYZ, or %NULL for error
 *
 * Since: 1.3.1
 **/
CdColorXYZ *
ch_device_take_reading_xyz (GUsbDevice *dev, GCancellable *cancellable, GError **error)
{
	CdColorXYZ *value;
	gboolean ret;
	gsize actual_length;
	gint32 buf[3];

	g_return_val_if_fail (G_USB_DEVICE (dev), NULL);

	/* hit hardware */
	ret = g_usb_device_control_transfer (dev,
					     G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
					     G_USB_DEVICE_REQUEST_TYPE_CLASS,
					     G_USB_DEVICE_RECIPIENT_INTERFACE,
					     CH_CMD_TAKE_READING_XYZ,
					     0,			/* wValue */
					     CH_USB_INTERFACE,	/* idx */
					     (guint8 *) buf,	/* data */
					     sizeof(buf),	/* length */
					     &actual_length,	/* actual_length */
					     CH_DEVICE_USB_TIMEOUT,
					     cancellable,
					     error);
	if (!ret)
		return NULL;

	/* return result */
	if (actual_length != sizeof(buf)) {
		g_set_error (error,
			     G_USB_DEVICE_ERROR,
			     G_USB_DEVICE_ERROR_IO,
			     "Invalid size, got %li", actual_length);
		return NULL;
	}

	/* check status */
	if (!ch_device_check_status (dev, cancellable, error))
		return NULL;

	/* parse */
	value = cd_color_xyz_new ();
	value->X = ch_offset_float_to_double (buf[0]);
	value->Y= ch_offset_float_to_double (buf[1]);
	value->Z = ch_offset_float_to_double (buf[2]);
	return value;
}

/**
 * ch_device_get_spectrum:
 * @dev: A #GUsbDevice
 * @cancellable: a #GCancellable, or %NULL
 * @error: a #GError, or %NULL
 *
 * Gets the spectrum from the device. This queries the device multiple times
 * until the spectrum has been populated.
 *
 * The spectrum is also set up with the correct start and wavelength
 * calibration coefficients.
 *
 * NOTE: This uses the ColorHug HID-less protocol and only works with ColorHug+.
 *
 * Returns: a #CdSpectrum, or %NULL for error
 *
 * Since: 1.3.1
 **/
CdSpectrum *
ch_device_get_spectrum (GUsbDevice *dev, GCancellable *cancellable, GError **error)
{
	gboolean ret;
	gdouble cal[4];
	gint32 buf[CH_EP0_TRANSFER_SIZE / sizeof(gint32)];
	gsize actual_length;
	guint16 i;
	guint16 j;
	g_autoptr(CdSpectrum) sp = NULL;

	g_return_val_if_fail (G_USB_DEVICE (dev), NULL);

	/* populate ahead of time for each chunk */
	sp = cd_spectrum_new ();

	/* hit hardware */
	for (i = 0; i < 1024 * sizeof(gint32) / CH_EP0_TRANSFER_SIZE; i++) {
		ret = g_usb_device_control_transfer (dev,
						     G_USB_DEVICE_DIRECTION_DEVICE_TO_HOST,
						     G_USB_DEVICE_REQUEST_TYPE_CLASS,
						     G_USB_DEVICE_RECIPIENT_INTERFACE,
						     CH_CMD_READ_SRAM,
						     i,			/* wValue */
						     CH_USB_INTERFACE,	/* idx */
						     (guint8 *) buf,	/* data */
						     sizeof(buf),	/* length */
						     &actual_length,	/* actual_length */
						     CH_DEVICE_USB_TIMEOUT,
						     cancellable,
						     error);
		if (!ret)
			return NULL;
		if (actual_length != sizeof(buf)) {
			g_set_error (error,
				     G_USB_DEVICE_ERROR,
				     G_USB_DEVICE_ERROR_IO,
				     "Failed to get spectrum data, got %li",
				     actual_length);
			return NULL;
		}

		/* add data */
		for (j = 0; j < CH_EP0_TRANSFER_SIZE / sizeof(gint32); j++) {
			gdouble tmp = ch_offset_float_to_double (buf[j]);
			cd_spectrum_add_value (sp, tmp);
		}
	}

	/* check status */
	if (!ch_device_check_status (dev, cancellable, error))
		return NULL;

	/* add the coefficients */
	if (!ch_device_get_ccd_calibration (dev,
					    &cal[0], &cal[1], &cal[2], &cal[3],
					    cancellable, error))
		return NULL;
	cd_spectrum_set_start (sp, cal[0]);
	cd_spectrum_set_wavelength_cal (sp, cal[1], cal[2], cal[3]);

	/* return copy */
	return cd_spectrum_dup (sp);
}
