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

all: firmware.xdfu

SRC_H =								\
	$(top_srcdir)/src/ch-config.h				\
	$(top_srcdir)/src/ch-errno.h				\
	$(top_srcdir)/src/ch-flash.h				\
	$(top_srcdir)/src/ColorHug.h				\
	$(srcdir)/oo_elis1024.h					\
	$(srcdir)/mti_23k640.h					\
	$(srcdir)/mti_tcn75a.h					\
	$(srcdir)/mzt_mcdc04.h					\
	$(srcdir)/usb_config.h
SRC_C =								\
	$(top_srcdir)/src/ch-config.c				\
	$(top_srcdir)/src/ch-errno.c				\
	$(top_srcdir)/src/ch-flash.c				\
	$(top_srcdir)/src/m-stack/usb/src/usb.c			\
	$(top_srcdir)/src/m-stack/usb/src/usb_dfu.c		\
	$(top_srcdir)/src/m-stack/usb/src/usb_winusb.c		\
	$(srcdir)/firmware.c					\
	$(srcdir)/oo_elis1024.c					\
	$(srcdir)/mti_23k640.c					\
	$(srcdir)/mti_tcn75a.c					\
	$(srcdir)/mzt_mcdc04.c					\
	$(srcdir)/usb_descriptors.c
firmware_CFLAGS =						\
	-I$(srcdir)						\
	-I$(top_builddir)					\
	-I$(top_srcdir)/src					\
	-I$(top_srcdir)/src/m-stack/usb/include			\
	--codeoffset=0x4000					\
	--rom=0x4000-0x8000					\
	${CFLAGS}
firmware.hex: ${SRC_C} ${SRC_H}
	$(AM_V_GEN) $(CC) $(firmware_CFLAGS) ${SRC_C} -o$@
firmware.dfu: firmware.hex Makefile
	$(AM_V_GEN) ${DFUTOOL} convert dfu $< $@ 4000;		\
	${DFUTOOL} set-metadata $@ License "GPL-2.0+";		\
	${DFUTOOL} set-metadata $@ Copyright "Hughski Limited";	\
	${DFUTOOL} set-vendor $@ 273f;				\
	${DFUTOOL} set-product $@ 1002
firmware.xdfu: firmware.dfu Makefile
	$(AM_V_GEN) ${DFUTOOL} encrypt $< $@ xtea `cat ~/.config/fwkey`;\
	${DFUTOOL} set-metadata $@ CipherKind XTEA
install-firmware-unsigned: firmware.dfu Makefile
	sudo ${DFUTOOL} write $< ;
install-firmware: firmware.xdfu Makefile
	sudo ${DFUTOOL} write $<

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
	ch-common.h						\
	firmware.c						\
	mti_23k640.c						\
	mti_23k640.h						\
	mti_tcn75a.c						\
	mti_tcn75a.h						\
	mzt_mcdc04.c						\
	mzt_mcdc04.h						\
	usb_config.h						\
	usb_descriptors.c

-include $(top_srcdir)/git.mk
