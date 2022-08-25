#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_INIT([bootd],[1.0],[rp@servium..ch])
AM_INIT_AUTOMAKE([-Wall -Werror gnu subdir-objects])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS_ONCE([
    arpa/inet.h
    fcntl.h
    getopt.h
    linux/sysctl.h
    net/if.h
    net/if_dl.h
    net/if_var.h
    net/if_types.h
    net/route.h
    netinet/if_ether.h
    netinet/ether.h
    netinet/in.h
    netinet/in.h
    sys/socket.h
    sys/sysctl.h
    sys/time.h
    time.h
    unistd.h

    ])

PKG_PREREQ()
PKG_PROG_PKG_CONFIG()

# BOOTD_CHECK_LIB(NAME, VERSION, FUNC, CANDIDATES)
# ------------------------------------------------
AC_DEFUN([BOOTD_CHECK_LIB], [
  # name version func candidates
  if test -n "$bootd_cv_have_$1"; then
    AC_MSG_CHECKING([$1])
    AC_MSG_RESULT([(cached) $bootd_cv_have_$1])
  else
    AC_CACHE_VAL(bootd_cv_have_$1, [
      PKG_CHECK_MODULES([$1], [$2], [], [true])
      LIBS="$LIBS $$1_LIBS"
      CFLAGS="$CFLAGS $$1_CFLAGS"
      AC_SEARCH_LIBS([$3], [$4], [
        bootd_cv_have_$1=yes], [bootd_cv_have_$1=no])
    ])
  fi
])

# Older libnet versions use libnet-config instead of pkg-config
# Only use if libnet_CFLAGS is not set, and pkg-config libnet doesn't exist
if test -z "$libnet_CFLAGS"; then
  AC_CACHE_VAL(pkg_cv_libnet_CFLAGS, [
    PKG_CHECK_EXISTS([libnet >= 1.2], [], [
      AC_PATH_PROGS(libnet_config, [libnet-config /opt/local/bin/libnet-config])
      libnet_CFLAGS=`$ac_cv_path_libnet_config --cflags`
      libnet_CFLAGS="$libnet_CFLAGS `$ac_cv_path_libnet_config --defines`"
      libnet_LIBS=`$ac_cv_path_libnet_config --libs`
    ])])
fi

BOOTD_CHECK_LIB([libnet], [libnet >= 1.2], [libnet_init_packet], [net])
BOOTD_CHECK_LIB([libpcap], [libpcap], [pcap_create], [pcap])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_TYPE_UINT32_T

AC_CHECK_TYPES([rpcprog_t], [], [], [[#include <rpc/rpc.h>]])
AC_CHECK_TYPES([uint u_int u_short u_char])
AC_CHECK_FUNC([libnet_write_link_layer], [AC_DEFINE([HAVE_LIBNET10], [1],
                                              [Define if libnet is <=1.0])])
AC_CHECK_FUNC([libnet_write_link], [AC_DEFINE([HAVE_LIBNET11], [1],
                                              [Define if libnet is >=1.1])])

AC_PATH_PROGS(rpcgen_path, [rpcgen])
RPCGEN_PATH=$rpcgen_path
AC_SUBST([RPCGEN_PATH])

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([gethostbyaddr gethostbyname gethostname gettimeofday inet_ntoa memset select socket strcasecmp strchr strdup strerror strrchr strstr strtol])
AC_REPLACE_FUNCS([strlcpy])

AC_OUTPUT
