AUTOMAKE_OPTIONS = dist-bzip2
lib_LTLIBRARIES = libdial.la
libdial_la_SOURCES = src/DialList.c src/DialMenu.c src/DialDisplay.c src/DialConfig.c src/dialsys.h
libdial_la_LDFLAGS = -version-info 2:1:0
AM_CPPFLAGS = $(DEPS_CFLAGS)
LIBS = $(DEPS_LIBS)
EXTRA_DIST = COPYING AUTHORS
include_HEADERS = src/dialsys.h
pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = pkgconfig/dial.pc

