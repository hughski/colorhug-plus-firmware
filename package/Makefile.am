# Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
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

all: $(VENDOR)-$(PROJECT_NAME)-${VERSION}.cab

CAB_FILES=							\
	../src/firmware/firmware.dfu				\
	../src/firmware/firmware.xdfu				\
	firmware.metainfo.xml

check: firmware.metainfo.xml
	appstream-util validate-relax $<

$(VENDOR)-$(PROJECT_NAME)-${VERSION}.cab: $(CAB_FILES)
	$(AM_V_GEN) ${GCAB} --create --nopath $@ $(CAB_FILES)

CLEANFILES =							\
	*.cab

EXTRA_DIST =							\
	firmware.metainfo.xml.in

-include $(top_srcdir)/git.mk
