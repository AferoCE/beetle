AC_INIT([beetle], [1.0], [bug-automake@gnu.org])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
])

AC_SUBST([BUILD_NUMBER])
AM_CONDITIONAL([BUILD_NUMBER_SET], [test ! -z "$BUILD_NUMBER"])
LT_INIT
AC_OUTPUT
