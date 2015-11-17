# Copyright (C) 2012 t-lo <thilo@thilo-fromm.de>
# Copyright (C) 2012-2015 Richard Hughes <richard@hughsie.com>
#
# Licensed under the GNU General Public License Version 2
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

all: bootloader.dfu

SRC_H =								\
	$(top_srcdir)/src/ch-config.h				\
	$(top_srcdir)/src/ch-errno.h				\
	$(top_srcdir)/src/ch-flash.h				\
	$(srcdir)/usb_config.h
SRC_C =								\
	$(top_srcdir)/src/ch-config.c				\
	$(top_srcdir)/src/ch-errno.c				\
	$(top_srcdir)/src/ch-flash.c				\
	$(top_srcdir)/src/m-stack/usb/src/usb.c			\
	$(top_srcdir)/src/m-stack/usb/src/usb_dfu.c		\
	$(top_srcdir)/src/m-stack/usb/src/usb_winusb.c		\
	$(srcdir)/bootloader.c					\
	$(srcdir)/usb_descriptors.c
bootloader_CFLAGS =						\
	${CFLAGS}						\
	-I$(srcdir)						\
	-I$(top_builddir)					\
	-I$(top_srcdir)/src					\
	-I$(top_srcdir)/src/m-stack/usb/include			\
	--rom=0x0000-0x3dff					\
	-DCOLORHUG_BOOTLOADER
bootloader.hex: ${SRC_C} ${SRC_H}
	$(AM_V_GEN) $(CC) $(bootloader_CFLAGS) ${SRC_C} -o$@
install-bootloader: bootloader.hex
	sudo ${PK2CMD_DIR}/pk2cmd -pPIC18F46J50 -f $< -b ${PK2CMD_DIR}/ -m -r

%.dfu: %.hex
	$(AM_V_GEN) ${DFUTOOL} convert dfu $< $@ 8000

CLEANFILES =							\
	*.as							\
	*.cmf							\
	*.cof							\
	*.d							\
	*.dfu							\
	*.hex							\
	*.hxl							\
	*.lst							\
	*.obj							\
	*.p1							\
	*.pre							\
	*.rlf							\
	*.sdb							\
	*.sym

EXTRA_DIST =							\
	bootloader.c						\
	usb_config.h						\
	usb_descriptors.c

-include $(top_srcdir)/git.mk
