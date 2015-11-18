# Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
AC_PREREQ(2.63)

m4_define([fwver_major], [0])
m4_define([fwver_minor], [2])

AC_INIT([colorhug-plus-firmware],[fwver_major.fwver_minor],[http://www.hughski.com])
AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-xz tar-ustar foreign])
AC_CONFIG_HEADERS([config.h])
AC_PROG_INSTALL

# enable nice build output on automake1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

# export these for the firmware
AC_DEFINE(FWVER_MAJOR,fwver_major,[Firmware major version])
AC_DEFINE(FWVER_MINOR,fwver_minor,[Firmware minor version])

# .hex -> .dfu
AC_PATH_PROG(DFUTOOL, dfu-tool)
if test x$DFUTOOL == x; then
	AC_MSG_ERROR([dfu-tool required, install fwupd])
fi

# .dfu -> .cab
AC_PATH_PROG(GCAB, gcab)
if test x$GCAB == x; then
	AC_MSG_ERROR([gcab required, install gcab])
fi

# FIXME: not hardcoded
PK2CMD_DIR="../../../../pk2cmd/pk2cmd"
AC_SUBST(PK2CMD_DIR)

# FIXME: not hardcoded
CC="/opt/microchip/xc8/v1.34/bin/xc8"
AC_SUBST(CC)

# common to bootloader and firmware
CFLAGS="--chip=18F46J50 "
CFLAGS+="--asmlist "
CFLAGS+="--opt=+speed "
CFLAGS+="-w3 "
CFLAGS+="-nw=3004 "
CFLAGS+="-DHAVE_SRAM "
CFLAGS+="-DHAVE_24MHZ "
CFLAGS+="-DHAVE_TCN75A "
CFLAGS+="-DHAVE_MCDC04 "
AC_SUBST(CFLAGS)

AC_CONFIG_FILES([
Makefile
package/firmware.metainfo.xml
package/Makefile
src/Makefile
src/bootloader/Makefile
src/firmware/Makefile
src/firmware-wobble/Makefile
])
AC_OUTPUT
