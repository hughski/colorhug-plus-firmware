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

#include "usb_config.h"
#include "usb.h"
#include "usb_ch9.h"
#include "usb_hid.h"
#include "usb_dfu.h"

/* Configuration Packet */
struct configuration_1_packet {
	struct configuration_descriptor		config;
	struct interface_descriptor		interface_dfu;
	struct dfu_functional_descriptor	dfu_runtime;
	struct interface_descriptor		interface;
	struct hid_descriptor			hid;
	struct endpoint_descriptor		ep;
	struct endpoint_descriptor		ep1_out;
};

/* Device Descriptor */
const struct device_descriptor chug_device_descriptor =
{
	sizeof(struct device_descriptor),
	DESC_DEVICE,
	0x0200,					/* USB 2.0, 0x0110 = USB 1.1 */
	0x00,					/* Device class */
	0x00,					/* Device Subclass */
	0x00,					/* Protocol */
	EP_0_LEN,				/* bMaxPacketSize0 */
	0x273f,					/* VID */
	0x1002,					/* PID */
	0x0001,					/* firmware version */
	1,					/* Manufacturer string index */
	2,					/* Product string index */
	0,					/* Serial string index */
	NUMBER_OF_CONFIGURATIONS
};

/* HID Report descriptor */
static const uint8_t chug_report_descriptor[] = {
   0x05, 0x01,					/* USAGE_PAGE (Generic Desktop) */
    0x09, 0x02,					/* USAGE (Mouse) */
    0xa1, 0x01,					/* COLLECTION (Application) */
    0x09, 0x01,					/*   USAGE (Pointer) */
    0xa1, 0x00,					/*   COLLECTION (Physical) */
    0x05, 0x09,					/*     USAGE_PAGE (Button) */
    0x19, 0x01,					/*     USAGE_MINIMUM (Button 1) */
    0x29, 0x03,					/*     USAGE_MAXIMUM (Button 3) */
    0x15, 0x00,					/*     LOGICAL_MINIMUM (0) */
    0x25, 0x01,					/*     LOGICAL_MAXIMUM (1) */
    0x95, 0x03,					/*     REPORT_COUNT (3) */
    0x75, 0x01,					/*     REPORT_SIZE (1) */
    0x81, 0x02,					/*     INPUT (Data,Var,Abs) */
    0x95, 0x01,					/*     REPORT_COUNT (1) */
    0x75, 0x05,					/*     REPORT_SIZE (5) */
    0x81, 0x03,					/*     INPUT (Cnst,Var,Abs) */
    0x05, 0x01,					/*     USAGE_PAGE (Generic Desktop) */
    0x09, 0x30,					/*     USAGE (X) */
    0x09, 0x31,					/*     USAGE (Y) */
    0x15, 0x81,					/*     LOGICAL_MINIMUM (-127) */
    0x25, 0x7f,					/*     LOGICAL_MAXIMUM (127) */
    0x75, 0x08,					/*     REPORT_SIZE (8) */
    0x95, 0x02,					/*     REPORT_COUNT (2) */
    0x81, 0x06,					/*     INPUT (Data,Var,Rel) */
    0xc0,					/*   END_COLLECTION */
    0xc0					/* END_COLLECTION */
};

/* Configuration Packet Instance */
static const struct configuration_1_packet configuration_1 =
{
	{
	/* Members from struct configuration_descriptor */
	sizeof(struct configuration_descriptor),
	DESC_CONFIGURATION,
	sizeof(configuration_1),
	0x02,					/* bNumInterfaces */
	0x01,					/* bConfigurationValue */
	0x02,					/* iConfiguration */
	0b10000000,
	100/2,					/* 100/2 indicates 100mA */
	},

	{
	/* DFU Runtime Descriptor (runtime) */
	sizeof(struct interface_descriptor),
	DESC_INTERFACE,
	0x01,					/* InterfaceNumber */
	0x00,					/* AlternateSetting */
	0x00,					/* bNumEndpoints (num besides endpoint 0) */
	DFU_INTERFACE_CLASS,			/* bInterfaceClass */
	DFU_INTERFACE_SUBCLASS,			/* bInterfaceSubclass */
	DFU_INTERFACE_PROTOCOL_RUNTIME,		/* bInterfaceProtocol */
	0x00,					/* iInterface */
	},

	{
	/* DFU Functional Descriptor (runtime) */
	sizeof(struct dfu_functional_descriptor),
	DESC_DFU_FUNCTIONAL_DESCRIPTOR,		/* bDescriptorType */
	0,					/* bmAttributes */
	0x00,					/* wDetachTimeOut (ms) */
	0x64,					/* wTransferSize */
	0x0101,					/* bcdDFUVersion */
	},

	{
	/* Members from struct interface_descriptor */
	sizeof(struct interface_descriptor),
	DESC_INTERFACE,
	0x00,					/* InterfaceNumber */
	0x00,					/* AlternateSetting */
	0x02,					/* bNumEndpoints (num besides endpoint 0) */
	HID_INTERFACE_CLASS,			/* bInterfaceClass 3=HID, 0xFF=VendorDefined */
	0x00,					/* bInterfaceSubclass (0=NoBootInterface for HID) */
	0x00,					/* bInterfaceProtocol */
	0x00,					/* iInterface */
	},

	{
	/* Members from struct hid_descriptor */
	sizeof(struct hid_descriptor),
	DESC_HID,
	0x0101,					/* bcdHID */
	0x00,					/* bCountryCode */
	0x01,					/* bNumDescriptors */
	DESC_REPORT,				/* bDescriptorType2 */
	sizeof(chug_report_descriptor),		/* wDescriptorLength */
	},

	{
	/* Members of the Endpoint Descriptor (EP1 IN) */
	sizeof(struct endpoint_descriptor),
	DESC_ENDPOINT,
	0x01 | 0x80,				/* endpoint #1 0x80=IN */
	EP_INTERRUPT,				/* bmAttributes */
	EP_1_IN_LEN,				/* wMaxPacketSize */
	1,					/* bInterval in ms. */
	},

	{
	/* Members of the Endpoint Descriptor (EP1 OUT) */
	sizeof(struct endpoint_descriptor),
	DESC_ENDPOINT,
	0x01 /*| 0x00*/,			/* endpoint #1 0x00=OUT */
	EP_INTERRUPT,				/* bmAttributes */
	EP_1_OUT_LEN,				/* wMaxPacketSize */
	1,					/* bInterval in ms. */
	},

};

/* String Descriptors */
static const struct {uint8_t bLength;uint8_t bDescriptorType; uint16_t lang; } str00 = {
	sizeof(str00),
	DESC_STRING,
	0x0409					/* US English */
};
static const struct {uint8_t bLength;uint8_t bDescriptorType; uint16_t chars[12]; } vendor_string = {
	sizeof(vendor_string),
	DESC_STRING,
	{'H','u','g','h','s','k','i',' ','L','t','d','.'}
};
static const struct {uint8_t bLength;uint8_t bDescriptorType; uint16_t chars[9]; } product_string = {
	sizeof(product_string),
	DESC_STRING,
	{'C','o','l','o','r','H','u','g','+'}
};

/* Get String function */
int16_t usb_application_get_string(uint8_t string_number, const void **ptr)
{
	if (string_number == 0) {
		*ptr = &str00;
		return sizeof(str00);
	} else if (string_number == 1) {
		*ptr = &vendor_string;
		return sizeof(vendor_string);
	} else if (string_number == 2) {
		*ptr = &product_string;
		return sizeof(product_string);
	} else if (string_number == 3) {
		/* FIXME: read a serial number out of EEPROM */
		return -1;
	}

	return -1;
}

/* Configuration Descriptor List
 */
const struct configuration_descriptor *usb_application_config_descs[] =
{
	(struct configuration_descriptor*) &configuration_1,
};

/* HID Descriptor Function */
int16_t usb_application_get_hid_descriptor(uint8_t interface, const void **ptr)
{
	/* two-step assignment avoids an incorrect error in XC8 on PIC16 */
	const void *p = &configuration_1.hid;
	*ptr = p;
	return sizeof(configuration_1.hid);
}

/* HID Report Descriptor Function */
int16_t usb_application_get_hid_report_descriptor(uint8_t interface, const void **ptr)
{
	*ptr = chug_report_descriptor;
	return sizeof(chug_report_descriptor);
}
