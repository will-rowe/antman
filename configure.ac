AC_INIT([antman], m4_normalize(m4_include([VERSION])), [w.p.m.rowe@gmail.com])

# Initialize Automake
AM_INIT_AUTOMAKE([-Wall -Werror gnu subdir-objects std-options 1.14 foreign])
AM_SILENT_RULES([yes])
AM_PROG_AR
LT_INIT
AC_CONFIG_MACRO_DIRS([m4])

# Configure language
AC_LANG(C)

# Checks for header files
AC_CHECK_HEADERS([libfswatch/c/libfswatch.h], [], [AC_MSG_ERROR([Unable to find the fswatch headers - supply location with CFLAGS or make sure it is installed (brew install fswatch).])])

# Checks for libraries
AC_CHECK_LIB([pthread], [pthread_create])
AC_CHECK_LIB([fswatch], [fsw_init_session], [], [AC_MSG_ERROR([Unable to find the fswatch library - supply with LDFLAGS.])])
AC_CHECK_LIB([fswatch], [fsw_start_monitor], [], [AC_MSG_ERROR([Unable to find the fswatch library - supply with LDFLAGS.])])

# Add some defines for automake to give to antman
AC_SUBST([PROG_NAME], ["antman"])
AC_SUBST([CONFIG_LOCATION], ["/tmp/.antman.config"])
AC_SUBST([DEFAULT_WATCH_DIR], ["/var/lib/MinKNOW/data/reads"])

# Donzo
AC_CONFIG_FILES([Makefile src/Makefile src/unit-tests/Makefile])

AC_OUTPUT()