/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
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

#include <xc.h>
#include <i2c.h>

#include "ColorHug.h"

#include "mzt_mcdc04.h"

#define MZT_MCDC04_SLAVE_ADDRESS_READ		0b11101001
#define MZT_MCDC04_SLAVE_ADDRESS_WRITE		0b11101000

typedef enum {
	MZT_MCDC04_CONFIG_ADDR_OSR		= 0x00,	/* wo */
	MZT_MCDC04_CONFIG_ADDR_AGEN		= 0x02,	/* ro */
	MZT_MCDC04_CONFIG_ADDR_CREGL		= 0x06,	/* rw */
	MZT_MCDC04_CONFIG_ADDR_CREGH		= 0x07,	/* rw */
	MZT_MCDC04_CONFIG_ADDR_OPTREG		= 0x08,	/* rw */
	MZT_MCDC04_CONFIG_ADDR_BREAK		= 0x09,	/* rw */
	MZT_MCDC04_CONFIG_ADDR_EDGES		= 0x0a	/* rw */
} Mcdc04ConfigAddr;

typedef enum {
	MZT_MCDC04_MEASURE_ADDR_OUT0		= 0x00,	/* ro */
	MZT_MCDC04_MEASURE_ADDR_OUT1		= 0x01,	/* ro */
	MZT_MCDC04_MEASURE_ADDR_OUT2		= 0x02,	/* ro */
	MZT_MCDC04_MEASURE_ADDR_OUT3		= 0x03,	/* ro */
	MZT_MCDC04_MEASURE_ADDR_OUTINT		= 0x04	/* ro */
} Mcdc04MeasureAddr;

/**
 * mzt_mcdc04_init:
 **/
void
mzt_mcdc04_init(MztMcdc04Context *ctx)
{
	ctx->tint = MZT_MCDC04_TINT_512;
	ctx->iref = MZT_MCDC04_IREF_20;
	ctx->div = MZT_MCDC04_DIV_DISABLE;
}

/**
 * mzt_mcdc04_set_tint:
 **/
void
mzt_mcdc04_set_tint(MztMcdc04Context *ctx, MztMcdc04Tint tint)
{
	ctx->tint = tint;
}

/**
 * mzt_mcdc04_set_iref:
 **/
void
mzt_mcdc04_set_iref(MztMcdc04Context *ctx, MztMcdc04Iref iref)
{
	ctx->iref = iref;
}

/**
 * mzt_mcdc04_set_iref:
 **/
void
mzt_mcdc04_set_div(MztMcdc04Context *ctx, MztMcdc04Div div)
{
	ctx->div = div;
}

/**
 * mzt_mcdc04_write_config:
 * @ctx: A #MztMcdc04Context
 *
 * Writes the context settings to the device.
 *
 * Returns: a #ChError, e.g. #CH_ERROR_OVERFLOW_SENSOR
 **/
ChError
mzt_mcdc04_write_config(MztMcdc04Context *ctx)
{
	uint8_t tmp;
	uint8_t rc;

	/* start */
	StartI2C1();

	/* send slave address */
	rc = WriteI2C1(MZT_MCDC04_SLAVE_ADDRESS_WRITE);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_ADDRESS;
		goto out;
	}

	/* send address pointer */
	rc = WriteI2C1(MZT_MCDC04_CONFIG_ADDR_OSR);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_CONFIG;
		goto out;
	}

	/* go to configuration mode */
	rc = WriteI2C1(0b00000010);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_CONFIG;
		goto out;
	}

	/* restart */
	RestartI2C1();

	/* send slave address */
	rc = WriteI2C1(MZT_MCDC04_SLAVE_ADDRESS_WRITE);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_ADDRESS;
		goto out;
	}

	/* set address pointer */
	rc = WriteI2C1(MZT_MCDC04_CONFIG_ADDR_CREGL);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_CONFIG;
		goto out;
	}

	/* send CREGL */
	tmp = 0b10000000;		/* DIR: 	anodes to input pins */
	tmp |= ctx->iref << 4;		/* R: 		ADC Reference Current */
	tmp |= ctx->tint;		/* T: 		Integration Time */
	WriteI2C1(tmp);

	/* send CREGH */
	tmp  = 0b0000000;		/* ENTM:	disable access to OUTINT */
	tmp |= 0b0100000;		/* SB:		Standby Enable */
	tmp |= 0b0001000;		/* MODE:	CMD measurement mode */
	if (ctx->div != MZT_MCDC04_DIV_DISABLE) {
		tmp |= ctx->div << 1;	/* DIV:		Divide higher bits */
		tmp |= 0b00000001;	/* ENDIV:	Enable divider */
	}
	WriteI2C1(tmp);

	/* send OPTREG */
	WriteI2C1(0x00);		/* ZERO:	Offset value */

	/* send BREAK */
	WriteI2C1(0x20);		/* BREAK:	1us to 255us */

	/* send EDGES */
	WriteI2C1(0x01); 		/* EDGES:	Only valid in SYND mode */
out:
	StopI2C1();
	return rc;
}

/**
 * mzt_mcdc04_take_readings_raw:
 **/
ChError
mzt_mcdc04_take_readings_raw(MztMcdc04Context *ctx,
			     int32_t *x, int32_t *y, int32_t *z)
{
	uint16_t tmp;
	uint32_t i;
	uint8_t rc;

	/* start */
	StartI2C1();

	/* send slave address */
	rc = WriteI2C1(MZT_MCDC04_SLAVE_ADDRESS_WRITE);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_ADDRESS;
		goto out;
	}

	/* send address pointer */
	rc = WriteI2C1(MZT_MCDC04_CONFIG_ADDR_OSR);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_CONFIG;
		goto out;
	}

	/* start measurement */
	rc = WriteI2C1(0b10000011);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_CONFIG;
		goto out;
	}

	/* stop */
	StopI2C1();

	/* wait for READY pin */
	for (i = 0x80000; i > 0; i--) {
		if (PORTAbits.RA0)
			break;
	}
	if (i == 0x0) {
		rc = CH_ERROR_OVERFLOW_SENSOR;
		goto out;
	}

	/* start */
	StartI2C1();

	/* send slave address */
	rc = WriteI2C1(MZT_MCDC04_SLAVE_ADDRESS_WRITE);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_ADDRESS;
		goto out;
	}

	/* send address pointer */
	rc = WriteI2C1(MZT_MCDC04_MEASURE_ADDR_OUT1);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_CONFIG;
		goto out;
	}

	/* reset the bus */
	RestartI2C1();

	/* send slave address */
	rc = WriteI2C1(MZT_MCDC04_SLAVE_ADDRESS_READ);
	if (rc != 0x00) {
		rc = CH_ERROR_I2C_SLAVE_ADDRESS;
		goto out;
	}

	/* read the XYZ:X color */
	tmp = ReadI2C1();
	AckI2C1();
	tmp |= ((uint16_t) ReadI2C1()) << 8;
	AckI2C1();
	if (x != NULL)
		*x = tmp;

	/* read the XYZ:Y color */
	tmp = ReadI2C1();
	AckI2C1();
	tmp |= ((uint16_t) ReadI2C1()) << 8;
	AckI2C1();
	if (y != NULL)
		*y = tmp;

	/* read the XYZ:Z color */
	tmp = ReadI2C1();
	AckI2C1();
	tmp |= ((uint16_t) ReadI2C1()) << 8;
	NotAckI2C1();
	if (z != NULL)
		*z = tmp;
out:
	StopI2C1();
	return rc;
}

/**
 * mzt_mcdc04_errata_01:
 *
 * Work around a possible device errata:
 *
 * The typical sensor response for X (600nm), Y (550nm), Z (445nm) is provided
 * in datasheet table 1. Normalised to Y=1.0, we have:
 *
 * [ 1.03 : 1.00 : 0.82 ]
 *
 * At matching frequencies, the CIE 2° observer normalized to Y=1.0 is:
 *
 * [ 1.06 : 1.00 : 1.78 ]
 *
 * To convert the deviceXYZ reading to a CIEXYZ value the channels have to be
 * scaled by [ 1.03 : 1.00 : 2.16 ]
 *
 * We also have to scale with a large constant factor to ensure the calibration
 * matrix does not have a huge unity component.
 * We do this as a uint32 without floating point to prevent loss of precision.
 * Any small fractional remander will be taken care of by the factory matrix.
 **/
static void
mzt_mcdc04_errata_01(int32_t *x, int32_t *y, int32_t *z)
{
	*x *= 33;
	*y *= 32;
	*z *= 69;
}

/**
 * mzt_mcdc04_take_readings:
 * @ctx: A #MztMcdc04Context that has been set up
 * @x: a #int32_t, or %NULL
 * @y: a #int32_t, or %NULL
 * @z: a #int32_t, or %NULL
 *
 * Takes a reading from the ADC using a previously set up context.
 *
 * Returns: a #ChError, e.g. #CH_ERROR_OVERFLOW_SENSOR
 **/
ChError
mzt_mcdc04_take_readings(MztMcdc04Context *ctx,
		         int32_t *x, int32_t *y, int32_t *z)
{
	ChError rc;
	rc = mzt_mcdc04_take_readings_raw(ctx, x, y, z);
	if (rc != CH_ERROR_NONE)
		return rc;
	mzt_mcdc04_errata_01(x, y, z);
	return CH_ERROR_NONE;
}

/**
 * mzt_mcdc04_take_readings_auto:
 * @ctx: A #MztMcdc04Context
 * @x: a #int32_t
 * @y: a #int32_t
 * @z: a #int32_t
 *
 * Takes a reading from the ADC using an adaptive algorithm.
 *
 * This does repeated measurements, adjusting the Tint and Iref so that the
 * reading falls into the middle 3/4 of the FSD.
 *
 * We never go below 256ms to avoid integration errors with either a 50Hz
 * refresh on a CRT tube or PWM from a LED backlight.
 *
 * Returns: a #ChError, e.g. #CH_ERROR_OVERFLOW_SENSOR
 **/
ChError
mzt_mcdc04_take_readings_auto(MztMcdc04Context *ctx,
			      int32_t *x, int32_t *y, int32_t *z)
{
	uint8_t i;
	uint8_t rc;
	uint32_t scale = 1;
	uint32_t overflow;
	uint32_t overflow_8th;

	/* set most sensitive initially */
	ctx->iref = MZT_MCDC04_IREF_20;

	/* this is probably the smallest we want to go with a 50Hz refresh */
	ctx->tint = MZT_MCDC04_TINT_256;
	ctx->div = MZT_MCDC04_DIV_DISABLE;

	/* try to get a reading in the middle 3/4 of FSD */
	for (i = 0; i < 4; i++) {

		/* take reading */
		rc = mzt_mcdc04_write_config(ctx);
		if (rc != CH_ERROR_NONE)
			return rc;
		rc = mzt_mcdc04_take_readings_raw(ctx, x, y, z);
		if (rc != CH_ERROR_NONE)
			return rc;

		/* the FSD is also the overflow value */
		overflow = (((uint32_t) 1) << (ctx->tint + 10)) - 1;
		if (overflow > 0xffff)
			overflow = 0xffff;

		/* all channels are in the bottom 1/8th */
		overflow_8th = overflow / 8;
		if (*x < overflow_8th &&
		    *y < overflow_8th &&
		    *z < overflow_8th) {
			ctx->tint += 2;
			if (ctx->tint > MZT_MCDC04_TINT_512)
				ctx->tint = MZT_MCDC04_TINT_512;
			continue;
		}

		/* any channel is overflow or top 1/8th */
		if (*x >= overflow - overflow_8th ||
		    *y >= overflow - overflow_8th ||
		    *z >= overflow - overflow_8th) {
			ctx->iref++;
			continue;
		}

		/* success */
		break;
	}

	/* calculate scale value */
	scale *= (uint32_t) 1 << (MZT_MCDC04_TINT_1024 - ctx->tint);
	scale *= (uint32_t) 1 << (ctx->iref * 2);

	/* scale value to absolute deviceXYZ */
	*x *= scale;
	*y *= scale;
	*z *= scale;

	/* work around a possible device errata */
	mzt_mcdc04_errata_01(x, y, z);

	return rc;
}
