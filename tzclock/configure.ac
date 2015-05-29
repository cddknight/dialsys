#
# Process this file with autoconf to produce a configure script.
#
AC_PREREQ(2.59)
AC_INIT(TzClock, 3.1.6, chris@theknight.co.uk)
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([src/TzClock.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O

REQUIRES=""

# Checks for libraries.
PKG_CHECK_MODULES([DEPS], [gtk+-3.0 libnotify dial],
	[REQUIRES="gtk3-devel libnotify-devel"], 
	[PKG_CHECK_MODULES([DEPS], [gtk+-2.0 >= 2.10.0 libgnomeui-2.0 dial],
		[REQUIRES="gtk2-devel cairo-devel libgnomeui-devel"])])

AC_SUBST(REQUIRES)
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([langinfo.h ctype.h math.h stdarg.h stdbool.h stdint.h stdio.h stdlib.h string.h time.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([nl_langinfo])
REVISION=1
AC_SUBST([REVISION])

AC_CONFIG_FILES([Makefile tzclock.spec])
AC_CONFIG_FILES([buildrpm.sh],[chmod +x buildrpm.sh])
AC_CONFIG_FILES([builddeb.sh],[chmod +x builddeb.sh])
AC_CONFIG_FILES([debian/control debian/changelog])
AC_OUTPUT
