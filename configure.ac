# Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
AC_PREREQ(2.63)
AC_INIT([colorhug-plus-firmware],[0.1],[http://www.hughski.com])
AM_INIT_AUTOMAKE([1.9 no-dist-gzip dist-xz tar-ustar foreign])
AC_PROG_INSTALL

# enable nice build output on automake1.11
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

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
AC_SUBST(CFLAGS)

AC_CONFIG_FILES([
Makefile
src/Makefile
src/bootloader/Makefile
src/firmware/Makefile
src/firmware-wobble/Makefile
])
AC_OUTPUT
