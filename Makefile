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


VENDOR		= hughski
PROJECT_NAME	= colorhug+
VERSION		= 0.1

PK2CMD_DIR 	= ../../pk2cmd/pk2cmd
CC		= /opt/microchip/xc8/v1.34/bin/xc8
DFU_TOOL	= /home/hughsie/Code/fwupd/libdfu/dfu-tool

.DEFAULT_GOAL := all

all: bootloader.hex $(VENDOR)-$(PROJECT_NAME)-$(VERSION).cab

# common to bootloader and firmware
CFLAGS  +=							\
	-I.							\
	-Im-stack/usb/include					\
	--chip=18F46J50						\
	--asmlist						\
	--opt=+speed						\
	-w3							\
	-nw=3004

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
verify.hex: Makefile bootloader.hex firmware.hex
	${PK2CMD_DIR}/pk2cmd -pPIC18F46J50 -GF $@ -b ${PK2CMD_DIR}/ -r
verify.dfu: verify.hex Makefile
	${DFU_TOOL} convert $< $@ 273f 1004 ffff 8000

# bootloader
SRC_BOOTLOADER_H =						\
	ch-config.h						\
	ch-errno.h						\
	ch-flash.h						\
	ch-sram.c						\
	usb_config.h						\
	usb_config_bootloader.h
SRC_BOOTLOADER_C =						\
	bootloader.c						\
	ch-config.c						\
	ch-errno.c						\
	ch-flash.c						\
	ch-sram.c						\
	m-stack/usb/src/usb.c					\
	m-stack/usb/src/usb_dfu.c				\
	m-stack/usb/src/usb_winusb.c				\
	usb_descriptors_bootloader.c
bootloader_CFLAGS =						\
	${CFLAGS}						\
	--rom=0x0000-0x3dff					\
	-DCOLORHUG_BOOTLOADER
bootloader.hex: Makefile ${SRC_BOOTLOADER_C} ${SRC_BOOTLOADER_H}
	$(CC) $(bootloader_CFLAGS) ${SRC_BOOTLOADER_C} -o$@
bootloader.dfu: bootloader.hex
	${DFU_TOOL} convert $< $@ 273f 1004 ffff 8000
install-bootloader: bootloader.hex
	${PK2CMD_DIR}/pk2cmd -pPIC18F46J50 -f $< -b ${PK2CMD_DIR}/ -m -r

# runtime firmware
SRC_FIRMWARE_H =						\
	ch-config.h						\
	ch-errno.h						\
	ch-flash.h						\
	ch-sram.c						\
	usb_config.h						\
	usb_config_firmware.h
SRC_FIRMWARE_C =						\
	ch-config.c						\
	ch-errno.c						\
	ch-flash.c						\
	ch-sram.c						\
	firmware.c						\
	m-stack/usb/src/usb.c					\
	m-stack/usb/src/usb_dfu.c				\
	m-stack/usb/src/usb_hid.c				\
	m-stack/usb/src/usb_winusb.c				\
	usb_descriptors_firmware.c
firmware_CFLAGS =						\
	--codeoffset=0x4000					\
	--rom=0x4000-0x8000					\
	${CFLAGS}
firmware.hex: Makefile ${SRC_FIRMWARE_C} ${SRC_FIRMWARE_H}
	$(CC) $(firmware_CFLAGS) ${SRC_FIRMWARE_C} -o$@
firmware.dfu: firmware.hex Makefile
	${DFU_TOOL} convert dfu $< $@ 273f 1004 ffff 8000
install: firmware.dfu Makefile
	${DFU_TOOL} --reset --alt 0 download $<

# LVFS package
CAB_FILES=							\
	firmware.dfu						\
	firmware.metainfo.xml
check: firmware.metainfo.xml
	appstream-util validate-relax $<
%.cab: $(CAB_FILES)
	gcab --create --nopath $@ $(CAB_FILES)

clean:
	rm -f *.as
	rm -f *.bin
	rm -f *.cab
	rm -f *.cmf
	rm -f *.cof
	rm -f *.d
	rm -f funclist
	rm -f *.hex
	rm -f *.hxl
	rm -f *.mum
	rm -f *.o
	rm -f *.obj
	rm -f *.p1
	rm -f *.pre
	rm -f *.lst
	rm -f *.rlf
	rm -f *.sdb
	rm -f *.sym
