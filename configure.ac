AC_INIT([bsp_viewer], [0.0.1], [andy_ball7@hotmail.com], [], [adreamingworld.com])

#AC_CANONICAL_HOST
#AC_CANONICAL_TARGET

# Put auxiliary files in a tidy place
AC_CONFIG_AUX_DIR(build-aux)

AM_INIT_AUTOMAKE([-Wall -Werror foreign])

AC_PROG_CC

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES([Makefile])

AC_CHECK_HEADERS([stdlib.h])

PKG_CHECK_MODULES([SDL], [sdl2])
PKG_CHECK_MODULES([IL], [IL])
PKG_CHECK_MODULES([GL], [gl])

AC_OUTPUT

