AUTOMAKE_OPTIONS = dist-bzip2
lib_LTLIBRARIES = libdial.la
libdial_la_SOURCES = DialList.c DialMenu.c DialDisplay.c DialConfig.c dialsys.h
libdial_la_LDFLAGS = -version-info 1:2
AM_CPPFLAGS = $(DEPS_CFLAGS)
LIBS = $(DEPS_LIBS)
EXTRA_DIST = COPYING AUTHORS
nobase_include_HEADERS = dialsys.h
pkgconfigdir = $(libdir)/pkgconfig
nodist_pkgconfig_DATA = pkgconfig/dial.pc

