/*
 * $Id: bootd.h,v 1.7 2002/07/22 13:03:14 pajarola Exp $
 */

#ifndef _BOOTD_H
#define _BOOTD_H

#include "config.h"

#ifndef HAVE_UINT
typedef unsigned int uint;
#endif
#ifndef HAVE_U_INT
typedef unsigned int u_int;
#endif
#ifndef HAVE_U_SHORT
typedef unsigned short u_short;
#endif
#ifndef HAVE_U_CHAR
typedef unsigned char u_char;
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif

/* POSIX.1 headers */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_LINUX_SYSCTL_H
#include <linux/sysctl.h>
#else
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#endif

#include <netinet/in.h>
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif
#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif
#include <net/if.h>
#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#include <net/route.h>
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif
#include <pcap.h>

#ifdef LIBNET_GLIBC_HACK
#define __GLIBC__ 1
#include <libnet.h>
#undef __GLIBC__
#else
#include <libnet.h>
#endif

#include <rpc/rpc.h>

#ifndef HAVE_RPCPROG_T
typedef uint32_t rpcprog_t;
typedef uint32_t rpcvers_t;
#endif

#include "genutil.h"
#include "ngenutil.h"
#include "listener.h"
#include "device.h"
#include "service.h"
#include "host.h"
#include "util.h"
#include "conf.h"
#include "global.h"

#include "rarp.h"
#include "rmp.h"
#include "tftp.h"
#include "bootparam.h"
#include "dhcp.h"

#define PREFIX "/server/netboot/src/bootd"
#define CONF "/etc/bootd.conf"
#endif
