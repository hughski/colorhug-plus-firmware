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

SUBDIRS = src

MAINTAINERCLEANFILES =						\
	$(srcdir)/aclocal.m4					\
	$(srcdir)/INSTALL					\
	$(srcdir)/ChangeLog					\
	$(srcdir)/install-sh					\
	$(srcdir)/missing

distclean-local:
	if test $(srdcir) = .; then :; else \
		rm -f ChangeLog; \
	fi

ChangeLog:
	@echo Creating $@
	@if test -d "$(srcdir)/.git"; then \
	  (GIT_DIR=$(top_srcdir)/.git ./missing --run git log df83d9ae25cddddea927883c630e66e3a247949c.. --stat -M -C --name-status  --date=short --no-color) | fmt --split-only > $@.tmp \
	  && mv -f $@.tmp $@ \
	  || ($(RM) $@.tmp; \
	      echo Failed to generate ChangeLog, your ChangeLog may be outdated >&2; \
	      (test -f $@ || echo git-log is required to generate this file >> $@)); \
	else \
	  test -f $@ || \
	  (echo A git checkout and git-log is required to generate ChangeLog >&2 && \
	  echo A git checkout and git-log is required to generate this file >> $@); \
	fi

.PHONY: ChangeLog

VENDOR		= hughski
PROJECT_NAME	= colorhug+

all: $(VENDOR)-$(PROJECT_NAME)-${VERSION}.cab

# LVFS package
CAB_FILES=							\
	src/firmware/firmware.dfu				\
	firmware.metainfo.xml
check: firmware.metainfo.xml
	appstream-util validate-relax $<
$(VENDOR)-$(PROJECT_NAME)-${VERSION}.cab: $(CAB_FILES)
	$(AM_V_GEN) ${GCAB} --create --nopath $@ $(CAB_FILES)

CLEANFILES =							\
	*.cab							\
	*.tar.xz

EXTRA_DIST =							\
	firmware.metainfo.xml

-include $(top_srcdir)/git.mk
