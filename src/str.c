/*
 * $Id: str.c,v 1.3 2002/07/22 13:07:29 pajarola Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>
#include <ctype.h>

#define GENUTIL
#include "genutil.h"

/* internal functions */

static int      str_is_true(char *w);	/* "yes", "yes", "ok", "1" */
static int      str_is_false(char *w);	/* "no", "false", "0" */

int
str_is_equal(char *w1, char *w2)
{
	if ((w1 == NULL) || (w2 == NULL)) {
		return 0;
	}
	return (strcmp(w1, w2) == 0);
}

int
str_is_equalnc(char *w1, char *w2)
{
	if ((w1 == NULL) || (w2 == NULL)) {
		return 0;
	}
	return (strcasecmp(w1, w2) == 0);
}

static int
str_is_true(char *w)
{
	if ((strcasecmp(w, "yes") == 0) ||
	    (strcasecmp(w, "true") == 0) ||
	    (strcasecmp(w, "ok") == 0) ||
	    (strcmp(w, "1") == 0)) {
		return 1;
	} else {
		return 0;
	}
}

static int
str_is_false(char *w)
{
	if ((strcasecmp(w, "no") == 0) ||
	    (strcasecmp(w, "false") == 0) ||
	    (strcmp(w, "0") == 0)) {
		return 1;
	} else {
		return 0;
	}
}

int
str_get_bool(char *w, int *dst)
{
	if ((w == NULL) || (dst == NULL)) {
		return 0;
	}
	if (str_is_true(w)) {
		*dst = 1;
		return 1;
	} else if (str_is_false(w)) {
		*dst = 0;
		return 1;
	} else {
		return 0;
	}
}

int
str_get_int(char *w, int *dst)
{
	char           *p;
	*dst = (int)strtol(w, &p, 10);
	return (p[0] == 0);
}

int
str_get_loglevel(char *w, int *dst)
{
	if ((w == NULL) || (dst == NULL)) {
		return 0;
	}
	if (str_is_equal(w, "debug")) {
		*dst = LOG_DEBUG;
		return 1;
	} else if (str_is_equal(w, "info")) {
		*dst = LOG_INFO;
		return 1;
	} else if (str_is_equal(w, "warn") | str_is_equal(w, "warning")) {
		*dst = LOG_WARN;
		return 1;
	} else if (str_is_equal(w, "err") | str_is_equal(w, "error")) {
		*dst = LOG_ERR;
		return 1;
	} else {
		return 0;
	}
}

int
str_get_eth(char *w, etheraddr_t * dst)
{
	struct ether_addr *r;
	if ((w == NULL) || (dst == NULL)) {
		return 0;
	}
	if ((r = ether_aton(w)) != NULL) {
		memcpy(dst, r, sizeof(etheraddr_t));
		return 1;
	} else {
		return 0;
	}
}

int
str_get_ip(char *w, ipaddr_t * dst)
{
	if ((w == NULL) || (dst == NULL)) {
		return 0;
	}
	if (inet_aton(w, (struct in_addr *) dst) == 1) {
		return 1;
	} else {
		return 0;
	}
}

int
str_get_hostport(char *w, int lhost, char *dsthost, int lport, char *dstport)
{
	char            host[256];
	char           *port;
	long            p;
	struct in_addr  ia;
	if ((w == NULL) || (dsthost == NULL) || (dstport == 0)) {
		return 0;
	}
	if (strchr(w, ' ')) {
		/* may not contain spaces */
		return 0;
	}
	strncpy(host, w, sizeof(host));
	if ((port = strchr(host, ':')) != NULL) {
		/* port given */
		*port = 0;
		if (strcmp(host, "*") == 0) {
			/* any host */
			host[0] = 0;
		}
		port++;
		if (strcmp(port, "*") == 0) {
			/* any port */
			port[0] = 0;
		}
	} else {
		/* only hostname given */
		port = host + strlen(host);
	}
	strncpy(dsthost, host, lhost);
	strncpy(dstport, port, lport);
	if (isdigit(dsthost[0])) {
		/* must be an ip address */
		if (inet_aton(w, &ia) != 1) {
			return 0;
		}
	}
	if (isdigit(dstport[0])) {
		/* must be number */
		p = strtol(dstport, &port, 10);
		if (port[0] != 0) {
			/* parse error */
			return 0;
		}
		if ((p < 0) || (p > 65535)) {
			/* out of range */
			return 0;
		}
		if (p == 0) {
			/* 'null' port */
			dstport[0] = 0;
		}
	}
	return 1;
}

char           *
str_bool(int b, int l, char *dst)
{
	if (dst == NULL) {
		log_msg(LOG_WARN, "str_bool: NULL dst");
		return dst;
	}
	if (b) {
		strncpy(dst, "true", l);
	} else {
		strncpy(dst, "false", l);
	}
	return dst;
}

char           *
str_eth(etheraddr_t * eth, int l, char *dst)
{
	if (dst == NULL) {
		log_msg(LOG_WARN, "str_eth: NULL dst");
		return dst;
	}
	if (eth == NULL) {
		strncpy(dst, "00:00:00:00:00:00", l);
	} else {
		strncpy(dst, ether_ntoa((struct ether_addr *) eth), l);
	}
	return dst;
}

char           *
str_ip(ipaddr_t * ip, int l, char *dst)
{
	if (dst == NULL) {
		log_msg(LOG_WARN, "str_ip: NULL dst");
		return dst;
	}
	if (ip == NULL) {
		strncpy(dst, "0.0.0.0", l);
	} else {
		strncpy(dst, inet_ntoa(*(struct in_addr *) ip), l);
	}
	return dst;
}

char           *
str_dlt(int dlt, int l, char *dst)
{
	if (dst == NULL) {
		log_msg(LOG_WARN, "str_dlt: NULL dst");
		return dst;
	}
	switch (dlt) {
	case DLT_NULL:
		strncpy(dst, "no link-layer encapsulation", l);
		break;
	case DLT_EN10MB:
		strncpy(dst, "Ethernet", l);
		break;
	case DLT_EN3MB:
		strncpy(dst, "experimental Ethernet (3Mb)", l);
		break;
	case DLT_AX25:
		strncpy(dst, "Amateur Radio AX.25", l);
		break;
	case DLT_PRONET:
		strncpy(dst, "Proteon ProNET Token Ring", l);
		break;
	case DLT_CHAOS:
		strncpy(dst, "Chaos", l);
		break;
	case DLT_IEEE802:
		strncpy(dst, "IEEE 802 Networks", l);
		break;
	case DLT_ARCNET:
		strncpy(dst, "ARCNET", l);
		break;
	case DLT_SLIP:
		strncpy(dst, "Serial Line IP", l);
		break;
	case DLT_PPP:
		strncpy(dst, "Point-to-point Protocol", l);
		break;
	case DLT_FDDI:
		strncpy(dst, "FDDI", l);
		break;
	case DLT_ATM_RFC1483:
		strncpy(dst, "LLC/SNAP encapsulated atm", l);
		break;
	case DLT_RAW:
		strncpy(dst, "raw IP", l);
		break;
	default:
		strncpy(dst, "unknown datalink type", l);
		break;
	}
	return dst;
}

char           *
str_af(int af, int l, char *dst)
{
	if (dst == NULL) {
		log_msg(LOG_WARN, "str_af: NULL dst");
		return dst;
	}
	switch (af) {
#ifdef AF_UNSPEC
	case AF_UNSPEC:
		strncpy(dst, "unspecified", l);
		break;
#endif
#ifdef AF_LOCAL
	case AF_LOCAL:
		strncpy(dst, "local unix/pipe/portal", l);
		break;
#endif
#ifdef AF_INET
	case AF_INET:
		strncpy(dst, "inet", l);
		break;
#endif
#ifdef AF_IMPLINK
	case AF_IMPLINK:
		strncpy(dst, "arpanet imp", l);
		break;
#endif
#ifdef AF_PUP
	case AF_PUP:
		strncpy(dst, "PUP", l);
		break;
#endif
#ifdef AF_CHAOS
	case AF_CHAOS:
		strncpy(dst, "MIT CHAOS", l);
		break;
#endif
#ifdef AF_NS
	case AF_NS:
		strncpy(dst, "Xerox NS", l);
		break;
#endif
#ifdef AF_ISO
	case AF_ISO:
		strncpy(dst, "ISO", l);
		break;
#endif
#ifdef AF_ECMA
	case AF_ECMA:
		strncpy(dst, "ECMA", l);
#endif
#ifdef AF_CCITT
	case AF_CCITT:
		strncpy(dst, "CCITT x.25", l);
		break;
#endif
#ifdef AF_SNA
	case AF_SNA:
		strncpy(dst, "SNA", l);
		break;
#endif
#ifdef AF_DECnet
	case AF_DECnet:
		strncpy(dst, "DECnet", l);
		break;
#endif
#ifdef AF_DLI
	case AF_DLI:
		strncpy(dst, "DEC dli", l);
		break;
#endif
#ifdef AF_LAT
	case AF_LAT:
		strncpy(dst, "LAT", l);
		break;
#endif
#ifdef AF_APPLETALK
	case AF_APPLETALK:
		strncpy(dst, "Apple Talk", l);
		break;
#endif
#ifdef AF_LINK
	case AF_LINK:
		strncpy(dst, "Link Layer Interface", l);
		break;
#endif
#ifdef AF_IPX
	case AF_IPX:
		strncpy(dst, "IPX", l);
		break;
#endif
#ifdef AF_ISDN
	case AF_ISDN:
		strncpy(dst, "ISDN", l);
		break;
#endif
#ifdef AF_INET6
	case AF_INET6:
		strncpy(dst, "IPv6", l);
		break;
#endif
#ifdef AF_NATM
	case AF_NATM:
		strncpy(dst, "native ATM", l);
		break;
#endif
#ifdef AF_ATM
	case AF_ATM:
		strncpy(dst, "ATM", l);
		break;
#endif
#ifdef AF_SLOW
	case AF_SLOW:
		strncpy(dst, "802.3ad slow protocol", l);
		break;
#endif
#ifdef AF_SCLUSTER
	case AF_SCLUSTER:
		strncpy(dst, "Sitara cluster protocol", l);
		break;
#endif
	default:
		strncpy(dst, "unknown protocol", l);
		break;
	}
	return dst;
}

char           *
str_sai(struct sockaddr_in * sai, int l, char *dst)
{
	if (dst == NULL) {
		log_msg(LOG_WARN, "str_sai: NULL dst");
		return dst;
	}
	if (sai == NULL) {
		strncpy(dst, "*:*", l);
	} else if (sai->sin_addr.s_addr != INADDR_ANY) {
		if (sai->sin_port == 0) {
			snprintf(dst, l, "%s:*", inet_ntoa(sai->sin_addr));
		} else {
			snprintf(dst, l, "%s:%d", inet_ntoa(sai->sin_addr), ntohs(sai->sin_port));
		}
	} else {
		if (sai->sin_port == 0) {
			strncpy(dst, "*:*", l);
		} else {
			snprintf(dst, l, "*:%d", ntohs(sai->sin_port));
		}
	}
	return dst;
}

char           *
str_do_quote(char *w, int l, char *dst)
{
	char           *d;
	if ((dst == NULL) || (l < 3)) {
		/* need dest and at least 3 bytes total length */
		return dst;
	}
	if (w == NULL) {
		strcpy(dst, "\"\"");
		return dst;
	}
	d = dst;
	*(d++) = '\"';

	while ((*w != 0) && ((d + 2) < (dst + l))) {
		switch (*w) {
		case '"':
		case '\\':
			*(d++) = '\\';
		default:
			*(d++) = *(w++);
		}
	}
	*(d++) = '\"';
	*(d++) = 0;
	return dst;
}
