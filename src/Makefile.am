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

SUBDIRS =							\
	bootloader						\
	firmware						\
	firmware-wobble

# This is in _BYTES_ not words
#
# 0000
#  | <--- Bootloader (crossing 2 pages)
# 7bff
# 7c00
#  | <--- Config space
# 7fff
# 8000
#  | <--- Runtime program
# fff8

# get from the device
verify.hex: bootloader.hex firmware.hex
	${PK2CMD_DIR}/pk2cmd -pPIC18F46J50 -GF $@ -b ${PK2CMD_DIR}/ -r

%.dfu: %.hex
	$(AM_V_GEN) ${DFUTOOL} convert dfu $< $@ 8000

CLEANFILES =							\
	*.dfu							\
	*.hex

MAINATINERCLEANFILES =						\
	m-stack

EXTRA_DIST =							\
	ch-config.c						\
	ch-config.h						\
	ch-errno.c						\
	ch-errno.h						\
	ch-flash.c						\
	ch-flash.h						\
	ch-sram.c						\
	ch-sram.h						\
	m-stack

-include $(top_srcdir)/git.mk
