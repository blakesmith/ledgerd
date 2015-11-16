#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([LEDGERD], [0.0.1])
AM_INIT_AUTOMAKE([foreign subdir-objects serial-tests])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

LT_INIT([disable-static])

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AC_PROG_LIBTOOL

AX_PTHREAD([],[mandatory])

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

# Checks for packages which use pkg-config.
# PKG_CHECK_MODULES([libev], [libev >= 4.11])

# Define variables for unit test using gtest
GTEST_VERSION="1.7.0"
GTEST_CPPFLAGS="-DGTEST_HAS_PTHREAD=1"
GTEST_CXXFLAGS="$PTHREAD_CFLAGS"
GTEST_LDFLAGS=""
GTEST_LIBS="$PTHREAD_LIBS"
AC_SUBST([GTEST_VERSION])
AC_SUBST([GTEST_CPPFLAGS])
AC_SUBST([GTEST_CXXFLAGS])
AC_SUBST([GTEST_LDFLAGS])
AC_SUBST([GTEST_LIBS])

# Other tools used in this package's various make scripts
AC_PATH_PROG([PERL], [perl])      # Perl
AC_PATH_PROG([TAR], [tar gtar])   # tar
AC_PATH_PROG([XARGS], [xargs])    # xargs
AC_PATH_PROG([CHMOD], [chmod])    # chmod
AC_PATH_PROG([CAT], [cat])        # cat
AC_PATH_PROG([ECHO], [echo])      # echo
AC_PATH_PROG([CP], [cp])          # cp
AC_PATH_PROG([DATE], [date])      # date
AC_PATH_PROG([MV], [mv])          # mv
AC_PATH_PROG([TEST], [test])

AC_CONFIG_FILES([Makefile
                 lib/Makefile
                 lib/gtest/Makefile
                 src/Makefile
                 src/lib/Makefile
                 src/paxos/Makefile
                 test/Makefile
                 test/lib/Makefile
                 test/paxos/Makefile])
AC_OUTPUT

AC_MSG_RESULT([
Configure Information:
    C Compiler      :   $CC
    DEFS            :   $DEFS
    CPPFLAGS        :   $CPPFLAGS
    CXXFLAGS        :   $CXXFLAGS
    CFLAGS          :   $CFLAGS

  Linker            :   $LD
    LDFLAGS         :   $LDFLAGS
    LIBS            :   $LIBS

Supported Build Features:
  Documents             : $(if test "x$DX_FLAG_doc" = "x1"; then echo yes; else echo no; fi)
    html                :   $(if test "x$DX_FLAG_html" = "x1"; then echo yes; else echo no; fi)
    man                 :   $(if test "x$DX_FLAG_man" = "x1"; then echo yes; else echo no; fi)
    pdf                 :   $(if test "x$DX_FLAG_pdf" = "x1"; then echo yes; else echo no; fi)
    ps                  :   $(if test "x$DX_FLAG_ps" = "x1"; then echo yes; else echo no; fi)
])
