/*
 * $Id: service.c,v 1.2 2002/07/22 13:03:14 pajarola Exp $
 */

#include "bootd.h"

static int      l_service_link_device_add(service_t * s, device_t * d);
static int      l_service_udp_ip_add(service_t * s, ipaddr_t * ip, int port);
static int      l_service_udp_device_add(service_t * s, device_t * d, int port);
static void     l_service_start_link(device_t * d);
static void     l_service_start_udp(service_t * s);
static void     l_service_start_rpc_udp(service_t * s);

static service_t *l_services = NULL;

void
service_init()
{
	service_t      *s;
	/* HOOK service */

	/* rarp */
	s = service_new();
	s->name = "rarp";
	s->id = SERVICE_RARP;
	s->service = rarp_handler;
	s->d.link.filter = "(rarp)";
	service_add(s);

	/* dhcp */
	s = service_new();
	s->name = "dhcp";
	s->id = SERVICE_DHCP;
	s->service = dhcp_handler;
	s->d.udp.defport = htons(67);
	service_add(s);

	/* mop */
	s = service_new();
	s->name = "mop";
	s->id = SERVICE_MOP;
	//s->service = mop_handler;
	s->d.link.filter = "(ether dst 08:00:2b:60:01:00 || 08:00:2b:60:02:00 || 08:00:2b:90:00)";	/* XXX fill in correct
													 * filter */
	service_add(s);

	/* rmp */
	s = service_new();
	s->name = "rmp";
	s->id = SERVICE_RMP;
	s->service = rmp_handler;
	s->d.link.filter = "(ether dst 9:0:9:0:0:4)";
	service_add(s);

	/* tftp */
	s = service_new();
	s->name = "tftp";
	s->id = SERVICE_TFTP;
	s->service = tftp_handler;
	s->d.udp.defport = htons(69);
	service_add(s);

	bootparam_service_init();
}

service_t      *
service_new()
{
	service_t      *s;
	s = malloc(sizeof(service_t));
	memset(s, 0, sizeof(service_t));
	return s;
}

void
service_add(service_t * s)
{
	service_t      *n;
	int             i;
	if (l_services == NULL) {
		l_services = s;
		s->next = NULL;
	} else {
		for (n = l_services; n; n = n->next) {
			if (str_is_equal(n->name, s->name)) {
				log_msg(LOG_WARN | LOG_CONF, "duplicate entry for service %s (ignored)", s->name);
				return;
			}
			if (n->next == NULL) {
				n->next = s;
				for (i = 0; i < LOG_MOD_MAX; i++) {
					if ((s->id >> i) & 1) {
						break;
					}
				}
				i = s->id >> i;
				if ((i & (!1)) || (i >= LOG_MOD_MAX)) {
					log_msg(LOG_ERR | LOG_CONF, "invalid service id: %d", s->id);
				}
				log_set_mod_name(i, s->name);
				return;
			}
		}
	}
}

service_t      *
service_search_by_name(char *name)
{
	service_t      *n;
	for (n = l_services; n; n = n->next) {
		if (str_is_equal(n->name, name)) {
			return n;
		}
	}
	return NULL;
}

service_t      *
service_search_by_id(int id)
{
	service_t      *n;
	for (n = l_services; n; n = n->next) {
		if (n->id == id) {
			return n;
		}
	}
	return NULL;
}

int
service_link_add(service_t * s, char *device)
{
	device_t       *d;
	int             result;
	etheraddr_t     eth;
	if (str_is_equal(device, "*") || str_is_equal(device, "all")) {
		result = 1;
		for (d = device_search_by_name(NULL); d; d = d->next) {
			if (!l_service_link_device_add(s, d)) {
				result = 0;
			}
		}
		return result;
	}
	if ((d = device_search_by_name(device)) == NULL) {
		if (!(str_get_eth(device, &eth)) || (util_name2ether(device, &eth))) {
			return 0;
		}
		if ((d = device_search_by_eth(&eth)) == NULL) {
			return 0;
		}
	}
	return l_service_link_device_add(s, d);
}

static int
l_service_link_device_add(service_t * s, device_t * d)
{
	int             i;
	for (i = 0; i < SERVICE_BIND_MAX; i++) {
		if (s->d.link.device[i] == NULL) {
			s->d.link.device[i] = strdup(d->name);
			return 1;
		}
	}
	return 0;
}

int
service_udp_add(service_t * s, char *bind)
{
	device_t       *d;
	ipaddr_t        ip;
	etheraddr_t     eth;
	int             port;
	char            buf_host[512];
	char            buf_port[512];
	struct servent *serv;
	if (!str_get_hostport(bind, sizeof(buf_host), buf_host, sizeof(buf_port), buf_port)) {
		return 0;
	}
	if (str_get_int(buf_port, &port)) {
		port = htons(port);
	} else if ((serv = getservbyname(buf_port, "udp")) == NULL) {
		return 0;
	} else {
		port = serv->s_port;
	}
	/*
	 * try 1) empty? 2) ip address 3) ethernet address 4) device name 5)
	 * host name
	 */
	if (buf_host[0] == 0) {
		ip.s_addr = 0;
		return l_service_udp_ip_add(s, &ip, port);
	}
	if (str_get_ip(buf_host, &ip)) {
		return l_service_udp_ip_add(s, &ip, port);
	} else if (str_get_eth(buf_host, &eth)) {
		if ((d = device_search_by_eth(&eth)) == NULL) {
			return 0;
		}
		return l_service_udp_device_add(s, d, port);
	} else if ((d = device_search_by_name(buf_host)) != NULL) {
		return l_service_udp_device_add(s, d, port);
	} else if (util_name2ip(buf_host, &ip)) {
		return l_service_udp_ip_add(s, &ip, port);
	}
	return 0;
}

static int
l_service_udp_ip_add(service_t * s, ipaddr_t * ip, int port)
{
	int             i;
	for (i = 0; i < SERVICE_BIND_MAX; i++) {
		if (s->d.udp.bindaddr[i].sin_family == 0) {
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
			s->d.udp.bindaddr[i].sin_len = sizeof(struct sockaddr_in);
#endif
			s->d.udp.bindaddr[i].sin_family = AF_INET;
			s->d.udp.bindaddr[i].sin_port = port;
			s->d.udp.bindaddr[i].sin_addr.s_addr = ip->s_addr;
			return 1;
		}
	}
	return 0;
}

static int
l_service_udp_device_add(service_t * s, device_t * d, int port)
{
	int             i, result;
	result = 1;
	for (i = 0; i < DEVICE_IP_ADDRS_MAX; i++) {
		if (util_ip_valid(&(d->ip[i]))) {
			if (!l_service_udp_ip_add(s, &(d->ip[i]), port)) {
				result = 0;
			}
		}
	}
	return result;
}
/* start all registered services */
void
service_start()
{
	service_t      *s;
	device_t       *d;
	int             i;
	char            eth_buf[32];
	for (s = l_services; s; s = s->next) {
		if (s->id & LISTEN_LINK) {
			/*
			 * special case, we start only one listener per
			 * interface for type link
			 */
			for (i = 0; s->d.link.device[i]; i++) {
				if (!(d = device_search_by_name(s->d.link.device[i]))) {
					/*
					 * XXX programming error, should
					 * already be checked
					 */
					log_msg(LOG_ERR | LOG_DEV, "unknown device: %s", s->d.link.device[i]);
					/* notreached */
				}
				if (d->filter[0] == 0) {
					str_eth(&(d->eth), sizeof(eth_buf), eth_buf);
					snprintf(d->filter, sizeof(d->filter), "(ether dst %s)", eth_buf);
				}
				snprintf(d->filter + strlen(d->filter), sizeof(d->filter) - strlen(d->filter),
					 " or %s", s->d.link.filter);
			}
		} else if (s->id & LISTEN_UDP) {
			l_service_start_udp(s);
		} else if (s->id & LISTEN_RPC_UDP) {
			l_service_start_rpc_udp(s);
		}
	}
	for (d = device_search_by_name(NULL); d; d = d->next) {
		if (d->filter[0] != 0) {
			l_service_start_link(d);
		}
	}
}

static void
l_service_start_link(device_t * d)
{
	listener_t     *l;
	if (strstr(d->filter, "or") != NULL) {
		/* service routine? */
		l = listener_new_link(d->name, d->filter, NULL, d->name);
		listener_add(l);
	}
}

static void
l_service_start_udp(service_t * s)
{
	listener_t     *l;
	int             i;
	for (i = 0; s->d.udp.bindaddr[i].sin_family; i++) {
		if (s->d.udp.bindaddr[i].sin_port == 0) {
			s->d.udp.bindaddr[i].sin_port = s->d.udp.defport;
		}
		l = listener_new_udp(&(s->d.udp.bindaddr[i]), NULL, s->service, s->name);
		listener_add(l);
	}
}

static void
l_service_start_rpc_udp(service_t * s)
{
	listener_t     *l;
	int             i;
	for (i = 0; s->d.rpc_udp.bindaddr[i].sin_family; i++) {
		l = listener_new_rpc_udp(&(s->d.rpc_udp.bindaddr[i]), s->d.rpc_udp.prognum, s->d.rpc_udp.versnum, s->service, s->name);
		listener_add(l);
	}
}
