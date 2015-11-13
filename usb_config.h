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

#ifndef __USB_CONFIG_H
#define __USB_CONFIG_H

/* automatically send the descriptors to bind the WinUSB driver on Windows */
#define AUTOMATIC_WINUSB_SUPPORT
#define MICROSOFT_OS_DESC_VENDOR_CODE 0x50

#ifdef COLORHUG_BOOTLOADER
 #include "usb_config_bootloader.h"
#else
 #include "usb_config_firmware.h"
#endif

#endif /* __USB_CONFIG_H */
