AC_INIT([bsp_viewer], [1.0], [andy_ball7@hotmail.com], [], [adreamingworld.com])
AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])

AC_CHECK_HEADERS([stdlib.h])

PKG_CHECK_MODULES([SDL], [sdl2])
PKG_CHECK_MODULES([IL], [IL])
PKG_CHECK_MODULES([GL], [gl])

AC_OUTPUT

