#
# Process this file with autoconf to produce a configure script.
#
AC_PREREQ([2.69])
AC_INIT([gauge],[3.10],[chris@theknight.co.uk])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([src/Gauge.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

# Checks for libraries.
PKG_CHECK_MODULES([DEPS], [gtk+-3.0 libcurl libxml-2.0 glib-2.0 json-glib-1.0 zlib dial])
AC_CHECK_LIB(sensors, sensors_init, [DEPS_LIBS="$DEPS_LIBS -lsensors"]) 
AC_CHECK_LIB(m, lrint, [DEPS_LIBS="$DEPS_LIBS -lm"]) 
AC_CHECK_LIB(pthread, pthread_create, [DEPS_LIBS="$DEPS_LIBS -lpthread"]) 

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([langinfo.h ctype.h math.h stdarg.h stdbool.h stdint.h stdio.h stdlib.h string.h time.h sensors/sensors.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([nl_langinfo])
REVISION=1
AC_SUBST([REVISION])

AC_CONFIG_FILES([Makefile gauge.spec])
AC_CONFIG_FILES([buildpkg.sh],[chmod +x buildpkg.sh])
AC_CONFIG_FILES([debian/control debian/changelog])
AC_OUTPUT
