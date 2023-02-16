#
# Process this file with autoconf to produce a configure script.
#
AC_PREREQ([2.69])
AC_INIT([libdial],[2.3],[chris@theknight.co.uk])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([src/DialDisplay.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
LT_INIT

# Checks for libraries.
PKG_CHECK_MODULES([DEPS], [gtk+-3.0 gthread-2.0])
AC_CHECK_LIB(m, lrint, [DEPS_LIBS="$DEPS_LIBS -lm"]) 

AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

AC_CHECK_HEADERS([langinfo.h ctype.h math.h stdarg.h stdbool.h stdint.h stdio.h stdlib.h string.h time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([nl_langinfo])
REVISION=1
AC_SUBST([REVISION])

AC_CONFIG_FILES([Makefile libdial.spec pkgconfig/dial.pc])
AC_CONFIG_FILES([buildrpm.sh],[chmod +x buildrpm.sh])
AC_CONFIG_FILES([builddeb.sh],[chmod +x builddeb.sh])
AC_CONFIG_FILES([debian/control debian/changelog])
AC_OUTPUT
