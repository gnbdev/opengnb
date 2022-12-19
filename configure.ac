#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([opengnb], [1.3.0.c], [BUG-REPORT-ADDRESS])
AC_CONFIG_SRCDIR([src/gnb_binary.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR(build-aux)
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CANONICAL_HOST
AS_CASE([$host_os],
        [linux*], [LINUX=yes],
        [mingw*], [WINDOWS=yes],
        [darwin*], [DARWIN=yes],
        [freebsd*], [FREEBSD=yes],
        [openbsd*], [OPENBSD=yes],
        AC_MSG_ERROR([OS $host_os is not implemented])
        )
AM_CONDITIONAL([LINUX], [test "${LINUX}" = "yes"])
AM_CONDITIONAL([WINDOWS], [test "${WINDOWS}" = "yes"])
AM_CONDITIONAL([DARWIN], [test "${DARWIN}" = "yes"])
AM_CONDITIONAL([FREEBSD], [test "${FREEBSD}" = "yes"])
AM_CONDITIONAL([OPENBSD], [test "${OPENBSD}" = "yes"])
AC_CONFIG_FILES([
 Makefile
 libs/Makefile
 libs/ed25519/Makefile
 libs/libnatpmp/Makefile
 libs/miniupnpc/Makefile
 src/Makefile
 src/crypto/Makefile
])

AC_CHECK_TOOL([WINDRES], [windres], [echo])

# Checks for programs.
AC_PROG_CC
AC_PROG_RANLIB
AM_PROG_AR

AC_OUTPUT
