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

#ifndef __CH_DEVICE2_H
#define __CH_DEVICE2_H

#include <gusb.h>
#include <colord.h>

#include <colorhug.h>

// FIXME: add to spec
typedef enum {
	CH_SPECTRUM_KIND_RAW		= 0x00,
	CH_SPECTRUM_KIND_DARK_CAL	= 0x01,
	CH_SPECTRUM_KIND_TEMP_CAL	= 0x02,
	CH_SPECTRUM_KIND_LAST
} ChSpectrumKind;

gboolean	 ch_device_open_full		(GUsbDevice	*dev,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_set_serial_number	(GUsbDevice	*dev,
						 guint16	 value,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_get_serial_number	(GUsbDevice	*dev,
						 guint16	*value,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_set_pcb_errata	(GUsbDevice	*dev,
						 ChPcbErrata	 value,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_get_pcb_errata	(GUsbDevice	*dev,
						 ChPcbErrata	*value,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_set_ccd_calibration	(GUsbDevice	*dev,
						 gdouble	 nm_start,
						 gdouble	 c0,
						 gdouble	 c1,
						 gdouble	 c2,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_get_ccd_calibration	(GUsbDevice	*dev,
						 gdouble	*nm_start,
						 gdouble	*c0,
						 gdouble	*c1,
						 gdouble	*c2,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_set_integral_time	(GUsbDevice	*dev,
						 guint16	 value,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_get_integral_time	(GUsbDevice	*dev,
						 guint16	*value,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_get_temperature	(GUsbDevice	*dev,
						 gdouble	*value,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_get_error		(GUsbDevice	*dev,
						 ChError	*status,
						 ChCmd		*cmd,
						 GCancellable	*cancellable,
						 GError		**error);
gboolean	 ch_device_take_reading_spectral(GUsbDevice	*dev,
						 ChSpectrumKind value,
						 GCancellable	*cancellable,
						 GError		**error);
CdColorXYZ	*ch_device_take_reading_xyz	(GUsbDevice	*dev,
						 GCancellable	*cancellable,
						 GError		**error);
CdSpectrum	*ch_device_get_spectrum		(GUsbDevice	*dev,
						 GCancellable	*cancellable,
						 GError		**error);

#endif /* __CH_DEVICE2_H */
