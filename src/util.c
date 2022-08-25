/*
 * $Id: util.c,v 1.3 2002/07/22 13:03:14 pajarola Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>

#include "bootd.h"

static int      i_file_match(file_t * f, int service, char *name);

void
util_stats()
{
	listener_t     *l;
	struct pcap_stat ps;
	int             recv, drop;

	recv = 0;
	drop = 0;
	for (l = g_listeners; l != NULL; l = l->next) {
		/* statistics only usefull for pcap */
		if (l->type & LISTEN_LINK) {
			pcap_stats(l->d.link.pcap, &ps);
			recv += ps.ps_recv;
			drop += ps.ps_drop;
		}
	}
	log_msg(LOG_INFO | LOG_MISC, "exiting\n"
	    "  %d bytes received by filter\n"
	    "  %d packets dropped by kernel",
	    recv, drop);
}

void
util_cleanup()
{
	/* do nothing so far */
}

int
util_ether_valid(etheraddr_t * ether)
{
	int             i;

	if (ether == NULL) {
		return 0;
	}
	for (i = 0; i < 6; i++) {
		if (ether->octet[i] != 0) {
			return 1;
		}
	}
	return 0;
}

int
util_ip_valid(ipaddr_t * ip)
{
	if (ip == NULL) {
		return 0;
	}
	return ip->s_addr != 0;
}

int
util_name2ether(char *name, etheraddr_t * ether)
{
	host_t         *h;
	int             i;
	char            b[64];

	if (str_get_eth(name, ether)) {
		return 1;
	}
	if (((h = util_name2host(name)) != NULL) && (util_ether_valid(&(h->eth)))) {
		memcpy(ether, h->eth.octet, sizeof(etheraddr_t));
		return 1;
	}
	if (c_resolve_ether == 0) {
		log_msg(LOG_WARN | LOG_RES, "expecting ethernet address (resolving disabled), got %s", name);
		return 0;
	}
	if (ether_hostton(name, (struct ether_addr *) ether) != 0) {
		for (i = 0; i < strlen(name); i++) {
			b[i] = toupper(name[i]);
		}
		b[strlen(name)] = 0;
		if (ether_hostton(b, (struct ether_addr *) ether) != 0) {
			log_msg(LOG_ERR | LOG_RES, "expecting ethernet address (unknown hostname %s)", name);
		}
	}
	return 1;
}

int
util_name2ip(char *name, ipaddr_t * ip)
{
	struct hostent *he;
	host_t         *h;

	if (str_get_ip(name, ip)) {
		return 1;
	}
	if (((h = util_name2host(name)) != NULL) && (util_ip_valid(&(h->ip)))) {
		ip->s_addr = h->ip.s_addr;
		return 1;
	}
	if (c_resolve_ip == 0) {
		log_msg(LOG_ERR | LOG_RES, "expecting ip address (resolving disabled), got %s", name);
	}
	if ((he = gethostbyname(name)) == NULL) {
		log_msg(LOG_ERR | LOG_RES, "expecting ip address (unknown hostname %s)", name);
	}
	if (he->h_addrtype != AF_INET) {
		log_msg(LOG_ERR | LOG_RES, "expecting ip address (got non ip address for %s)", name);
	}
	memcpy(ip, he->h_addr, sizeof(ipaddr_t));
	return 1;
}

char           *
util_ether2name(etheraddr_t * ether)
{
	static char     name[64];
	host_t         *h;

	if ((h = util_ether2host(ether)) != NULL) {
		return h->name;
	}
	if (c_resolve_ether) {
		if (ether_ntohost(name, (struct ether_addr *) ether) == 0) {
			return name;
		}
	}
	return str_eth(ether, sizeof(name), name);
}

char           *
util_ip2name(ipaddr_t * ip)
{
	static char     name[64];
	struct hostent *he;
	host_t         *h;

	if ((h = util_ip2host(ip)) != NULL) {
		return h->name;
	}
	if (c_resolve_ip) {
		if ((he = gethostbyaddr((char *)ip, sizeof(ipaddr_t), AF_INET)) != NULL) {
			return he->h_name;
		}
	}
	return str_ip(ip, sizeof(name), name);
}

host_t         *
util_name2host(char *name)
{
	host_t         *h;

	for (h = g_hosts; h != NULL; h = h->next) {
		if (strcasecmp(name, h->name) == 0) {
			return h;
		}
	}
	return NULL;
}

host_t         *
util_ether2host(etheraddr_t * ether)
{
	host_t         *h;
	for (h = g_hosts; h != NULL; h = h->next) {
		if (memcmp(h->eth.octet, ether->octet, sizeof(etheraddr_t)) == 0) {
			return h;
		}
	}
	return NULL;
}

host_t         *
util_ip2host(ipaddr_t * ip)
{
	host_t         *h;
	for (h = g_hosts; h != NULL; h = h->next) {
		if (ip->s_addr == h->ip.s_addr) {
			return h;
		}
	}
	return NULL;
}

file_t         *
util_name2file(host_t * h, int service, char *name, int no)
{
	file_t         *n;
	int             i;

	if (h == 0) {
		log_msg(LOG_WARN | LOG_RES, "trying to search file for null host");
		return NULL;
	}
	/* look for best matches first */
	for (i = 1; i <= 4; i++) {
		for (n = h->files; n; n = n->next) {
			if (i_file_match(n, service, name) == i) {
				if (no--) {
					continue;
				}
				return n;
			}
		}
	}
	return NULL;
}

static int
i_file_match(file_t * n, int service, char *name)
{
	int             i;

	if ((n->service & service) == 0) {
		/* service not allowed, no match */
		return 0;
	}
	if (name == NULL) {
		/* no name specified, match anything */
		return 1;
	}
	if (str_is_equal(name, n->path)) {
		/* name matches full path */
		return 1;
	}
	if (str_is_equal(name, n->name)) {
		/* name matches file name */
		return 2;
	}
	for (i = 0; i < FILE_ALIASES_MAX; i++) {
		if (str_is_equal(name, n->aliases[i])) {
			/* name matches an alias */
			return 3;
		}
	}
	if (str_is_equal(n->path, "*") || str_is_equal(n->name, "*")) {
		/* wildcard entry */
		return 4;
	}
	for (i = 0; i < FILE_ALIASES_MAX; i++) {
		if (str_is_equal(n->aliases[i], "*")) {
			/* wildcard entry */
			return 4;
		}
	}
	/* no match */
	return 0;
}

void
util_host_session_timeout_all()
{
	host_t         *n;
	for (n = g_hosts; n != NULL; n = n->next) {
		host_session_timeout(n);
	}
}

int
util_read(int fd, char *buf, int len, off_t offs)
{
	int             result;
	if ((result = lseek(fd, offs, SEEK_SET)) < 0) {
		log_msg(LOG_WARN | LOG_DEV, "lseek(%d, %d, SEEK_SET): %s",
		    fd, offs, strerror(errno));
		return result;
	}
	if ((result = read(fd, buf, len)) <= 0) {
		if (result < 0) {
			log_msg(LOG_WARN | LOG_DEV, "read(%d, buf, %d): %s",
			    fd, len, strerror(errno));
		}
	}
	return result;
}
