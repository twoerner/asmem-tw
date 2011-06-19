dnl Copyright (C) 2011  Trevor Woerner

dnl AC_PREREQ(2.57)
AC_INIT([asmem-tw], 0.1.0, twoerner@gmail.com, asmem-tw)
AC_CONFIG_SRCDIR(src/asmem.c)
AC_CONFIG_AUX_DIR(cfg)
AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-bzip2 1.9])
AM_CONFIG_HEADER(cfg/config.h)

SUBDIRS="src doc"

dnl **********************************
dnl checks for programs
dnl **********************************
AC_PROG_CC
AC_PROG_CPP

AC_PROG_MAKE_SET

AC_PROG_INSTALL
AC_PROG_LN_S

dnl **********************************
dnl checks for libraries
dnl **********************************
AC_PATH_XTRA
AC_CHECK_LIB(X11, XOpenDisplay, ,AC_MSG_ERROR([Can not find X11]) ,)
AC_CHECK_LIB(Xpm, XpmCreatePixmapFromData, ,AC_MSG_ERROR([Can not find Xpm]) ,)

dnl **********************************
dnl checks for header files
dnl **********************************
AC_HEADER_STDC
AC_CHECK_HEADERS(stdio.h string.h stdlib.h)
AC_CHECK_HEADERS(unistd.h math.h time.h)
AC_CHECK_HEADERS(X11/Xlib.h X11/xpm.h X11/Xatom.h)

dnl **********************************
dnl other stuff
dnl **********************************
if test x$HAVE_CHECK = xtrue; then
	SUBDIRS="$SUBDIRS tests"
fi
AC_SUBST(SUBDIRS)

dnl **********************************
dnl output
dnl **********************************
AC_OUTPUT(Makefile
cfg/Makefile
doc/Makefile
src/Makefile)
