#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([oshu], [0.1.0], [fmang+oshu@mg0.fr])
AC_CONFIG_SRCDIR([src/oshu.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile docs/Makefile])
AM_INIT_AUTOMAKE([foreign])

# Checks for programs.
AC_PROG_CC_C99
AC_CHECK_PROGS([DOXYGEN], [doxygen])

# Checks for libraries.
PKG_CHECK_MODULES([SDL2], [sdl2])
PKG_CHECK_MODULES([SDL2_image], [SDL2_image])
PKG_CHECK_MODULES([LAVF], [libavformat])
PKG_CHECK_MODULES([LAVC], [libavcodec])
PKG_CHECK_MODULES([LAVU], [libavutil])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$DOXYGEN"])
AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Doxyfile])])

AC_OUTPUT
