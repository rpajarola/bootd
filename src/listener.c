/*
 * $Id: listener.c,v 1.3 2002/07/22 13:03:14 pajarola Exp $
 */

#include "bootd.h"

/*
 * declarations for internal functions
 */

/* generic fd/socket functions */
static int      i_listener_ioctl_immediate(int fd, int on);
static int      i_listener_ioctl_shdrcmplt(int fd, int on);
static int      i_listener_ioctl_sseesent(int fd, int on);
static int      i_listener_fcntl_nonblock(int fd, int on);
/* link */
static int      i_listener_link_init_addr(listener_t * l, char *device);
static int      i_listener_link_init_pcap(listener_t * l, char *device);
static int      i_listener_link_init_lnet(listener_t * l, char *device);
static int      i_listener_link_init_filter(listener_t * l, char *device, char *filter);

static void     i_listener_link_cleanup_pcap(listener_t * l);
static void     i_listener_link_cleanup_lnet(listener_t * l);

static void     i_listener_link_pcap_callback(u_char * u, const struct pcap_pkthdr * pkthdr, const u_char * pkt);

static void     i_listener_link_receive(listener_t * l);
static void     i_listener_link_cleanup(listener_t * l);
static int      i_listener_link_send(listener_t * l, void *from, void *to, void *packet, int pktlen);


/* udp */
static void     i_listener_udp_receive(listener_t * l);
static void     i_listener_udp_cleanup(listener_t * l);
static int      i_listener_udp_send(listener_t * l, void *from, void *to, void *packet, int pktlen);
/* rpc/udp */
static void     i_listener_rpc_udp_receive(listener_t * l);
static void     i_listener_rpc_udp_cleanup(listener_t * l);


/*
 * public functions
 */

listener_t     *
listener_new_link(char *device, char *filter, listener_service service, char *desc)
{
	listener_t     *l;
	if (device == NULL) {
		/* ignore, return false */
		return 0;
	}
	if ((l = malloc(sizeof(listener_t))) == NULL) {
		/* error, return false */
		log_msg(LOG_ERR | LOG_DEV, "malloc(%d): %s", sizeof(listener_t), strerror(errno));
	}
	memset(l, 0, sizeof(listener_t));
	l->type = LISTEN_LINK;

	/* initialize libnet, libpcap for the specified device */
	if (!i_listener_link_init_lnet(l, device)) {
		i_listener_link_cleanup(l);
		return NULL;
	}
	if (!i_listener_link_init_addr(l, device)) {
		i_listener_link_cleanup(l);
		return NULL;
	}
	if (!i_listener_link_init_pcap(l, device)) {
		i_listener_link_cleanup(l);
		return NULL;
	}
	if (!i_listener_link_init_filter(l, device, filter)) {
		i_listener_link_cleanup(l);
		return NULL;
	}
	l->d.link.device = strdup(device);

	/* set up handlers */
	l->service = service;
	l->receive = (listener_handler) i_listener_link_receive;
	l->cleanup = (listener_handler) i_listener_link_cleanup;
	l->send = (listener_service) i_listener_link_send;

	/* create info and desc, log that we're listening */
	snprintf(l->info, sizeof(l->info), "link: %s", device);
	if (desc == NULL) {
		strncpy(l->desc, l->info, sizeof(l->desc)-1);
		log_msg(LOG_INFO | LOG_DEV, "listening on %s", l->info);
	} else {
		strncpy(l->desc, desc, sizeof(l->desc)-1);
		log_msg(LOG_INFO | LOG_DEV, "listening on %s (%s)", l->info, l->desc);
	}
	return l;
}

listener_t     *
listener_new_udp(struct sockaddr_in * from, struct sockaddr_in * to, listener_service service, char *desc)
{
	listener_t     *l;
	char            from_str[32];	/* 23 would be enough */
	char            to_str[32];
	socklen_t       s_inl;
	if ((l = malloc(sizeof(listener_t))) == NULL) {
		log_msg(LOG_ERR | LOG_DEV, "malloc(%d): %s", sizeof(listener_t), strerror(errno));
	}
	memset(l, 0, sizeof(listener_t));
	l->type = LISTEN_UDP;

	str_sai(from, sizeof(from_str), from_str);
	str_sai(to, sizeof(to_str), to_str);

	/* create socket */
	if ((l->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log_msg(LOG_WARN | LOG_DEV, "socket(AF_INET, SOCK_DGRAM): %s",
		    strerror(errno));
		free(l);
		return NULL;
	}
	i_listener_fcntl_nonblock(l->fd, 1);

	/* bind to local addr, use from if specified */
	if (from == NULL) {
		from = &(l->d.udp.from);	/* l->d.udp.from has been set
						 * to zero */
	}
	if (bind(l->fd, (struct sockaddr *) from, sizeof(struct sockaddr_in)) < 0) {
		log_msg(LOG_WARN | LOG_DEV, "bind(%s): %s",
		    from_str, strerror(errno));
		close(l->fd);
		free(l);
		return NULL;
	}
	/* connect to remote addr if specified */
	if (to != NULL) {
		memcpy(&(l->d.udp.to), to, sizeof(struct sockaddr_in));
		if (connect(l->fd, (struct sockaddr *) to, sizeof(struct sockaddr_in)) < 0) {
			log_msg(LOG_WARN | LOG_DEV, "connect(%s): %s",
			    to_str, strerror(errno));
			close(l->fd);
			free(l);
			return NULL;
		}
	}
	s_inl = sizeof(struct sockaddr_in);
	if (getsockname(l->fd, (struct sockaddr *) & (l->d.udp.from), &s_inl) == -1) {
		log_msg(LOG_WARN | LOG_DEV, "getsockname(): %s", strerror(errno));
	}
	str_sai(&(l->d.udp.from), sizeof(from_str), from_str);

	/* set up handlers */
	l->service = service;
	l->receive = (listener_handler) i_listener_udp_receive;
	l->cleanup = (listener_handler) i_listener_udp_cleanup;
	l->send = (listener_service) i_listener_udp_send;


	/* create info and desc, log that we're listening */
	snprintf(l->info, sizeof(l->info), "udp: %s <-> %s", from_str, to_str);
	if (desc == NULL) {
		strncpy(l->desc, l->info, sizeof(l->desc)-1);
		log_msg(LOG_INFO | LOG_DEV, "listening on %s", l->info);
	} else {
		strncpy(l->desc, desc, sizeof(l->desc)-1);
		log_msg(LOG_INFO | LOG_DEV, "listening on %s (%s)", l->info, l->desc);
	}
	return l;
}

listener_t     *
                listener_new_rpc_udp(struct sockaddr_in * from, rpcprog_t prognum, rpcvers_t versnum, void (*dispatch) (struct svc_req *, SVCXPRT *), char *desc){
	listener_t     *l;
	SVCXPRT        *xprt;
	socklen_t       s_inl;
	char            from_str[32];	/* 23 would be enough */
#ifdef HAVE_SUN_RPC_NEW
	struct netconfig *netconf;
#endif

	if ((l = malloc(sizeof(listener_t))) == NULL) {
		log_msg(LOG_ERR | LOG_DEV, "malloc(%d): %s", sizeof(listener_t), strerror(errno));
	}
	memset(l, 0, sizeof(listener_t));
	l->type = LISTEN_RPC_UDP;

	str_sai(&(l->d.udp.from), sizeof(from_str), from_str);

	/* create socket */
	if ((l->fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		log_msg(LOG_WARN | LOG_DEV, "socket(AF_INET, SOCK_DGRAM): %s",
		    strerror(errno));
		free(l);
		return NULL;
	}
	i_listener_fcntl_nonblock(l->fd, 1);

	/* bind to local addr, use from if specified */
	if (from == NULL) {
		from = &(l->d.rpc_udp.from);	/* l->d.udp.from has been set
						 * to zero */
	}
	if (bind(l->fd, (struct sockaddr *) from, sizeof(struct sockaddr_in)) < 0) {
		log_msg(LOG_WARN | LOG_DEV, "bind(%s): %s",
		    from_str, strerror(errno));
		close(l->fd);
		free(l);
		return NULL;
	}
#ifdef HAVE_SUN_RPC_NEW
	/* create the service */
	if ((xprt = svc_dg_create(l->fd, 0, 0)) == NULL) {
		log_msg(LOG_WARN | LOG_DEV, "svc_dg_create() failed");
		close(l->fd);
		free(l);
		return 0;
	}
	/* make sure nothing is registered for this service */
	svc_unreg(prognum, versnum);

	netconf = getnetconfigent("udp");
	if (!svc_reg(xprt, prognum, versnum, dispatch, netconf)) {
		log_msg(LOG_WARN | LOG_DEV, "svc_reg(%d, %d, udp) failed", prognum, versnum);
		svc_destroy(xprt);
		close(l->fd);
		free(l);
		return 0;
	}
#else
	/* create the service */
	if ((xprt = svcudp_bufcreate(l->fd, 0, 0)) == NULL) {
		log_msg(LOG_WARN | LOG_DEV, "svcudp_bufcreate() failed");
		close(l->fd);
		free(l);
		return 0;
	}
	/* make sure nothing is registered for this service */
	svc_unregister(prognum, versnum);

	if (!svc_register(xprt, prognum, versnum, dispatch, IPPROTO_UDP)) {
		log_msg(LOG_WARN | LOG_DEV, "svc_register(%d, %d, udp) failed", prognum, versnum);
		svc_destroy(xprt);
		close(l->fd);
		free(l);
		return 0;
	}
#endif

	/* set up handlers */
	l->service = NULL;	/* this is handled differently!!! */
	l->receive = (listener_handler) i_listener_rpc_udp_receive;
	l->cleanup = (listener_handler) i_listener_rpc_udp_cleanup;
	l->send = NULL;		/* this is handled differently!!! */

	/* create info and desc, log that we're listening */
	s_inl = sizeof(struct sockaddr_in);
	if (getsockname(l->fd, (struct sockaddr *) & (l->d.rpc_udp.from), &s_inl) == -1) {
		log_msg(LOG_WARN | LOG_DEV, "getsockname(): %s", strerror(errno));
	}
	str_sai(&(l->d.udp.from), sizeof(from_str), from_str);
	snprintf(l->info, sizeof(l->info), "rpc/udp: %s", from_str);
	if (desc == NULL) {
		strncpy(l->desc, l->info, sizeof(l->desc)-1);
		log_msg(LOG_INFO | LOG_DEV, "listening on %s", l->info);
	} else {
		strncpy(l->desc, desc, sizeof(l->desc)-1);
		log_msg(LOG_INFO | LOG_DEV, "listening on %s (%s)", l->info, l->desc);
	}
	return l;
}

void
listener_add(listener_t * l)
{
	listener_t     *n;
	/* sanity check l */
	if (l == NULL) {
		log_msg(LOG_WARN | LOG_CONF, "trying to add null listener");
		return;
	}
	/* validate l->fd ? */
	/*
	 * the service handler is optional, it is the responsibility of the
	 * receive handler to detect that it is not set
	 */
	if (l->receive == NULL) {
		log_msg(LOG_WARN | LOG_CONF, "trying to add listener without receive handler");
		return;
	}
	if (l->cleanup == NULL) {
		log_msg(LOG_WARN | LOG_CONF, "trying to add listener without cleanup handler");
		return;
	}
	/*
	 * just to annoy the programmer if he was too lazy to fill in these
	 * values
	 */
	if (!util_ip_valid(&(l->ip))) {
		log_msg(LOG_DEBUG | LOG_CONF, "adding listener without ip address");
	}
	if (!util_ether_valid(&(l->eth))) {
		log_msg(LOG_DEBUG | LOG_CONF, "adding listener without ethernet adress");
	}
	if (l->name == NULL) {
		log_msg(LOG_DEBUG | LOG_CONF, "adding listener without hostname");
		l->name[0] = 0;
	}
	/*
	 * insert the listener into the linked list this whole thing is not
	 * performance critical...
	 */
	l->next = NULL;
	if (g_listeners == NULL) {
		g_listeners = l;
	} else {
		for (n = g_listeners; n != NULL; n = n->next) {
			if (l->fd == n->fd) {
				/*
				 * this should not happen ever. this may
				 * cause a memory leak
				 */
				log_msg(LOG_WARN | LOG_CONF,
				    "duplicate entry for listener fd %d (ignored)", l->fd);
				return;
			}
			if (n->next == NULL) {
				n->next = l;
				return;
			}
		}
	}
}

void
listener_delete(listener_t * l)
{
	listener_t     *n;
	if (l == NULL) {
		/* nothing serious, happens from time to time */
		log_msg(LOG_DEBUG | LOG_CONF, "trying to delete null listener");
		return;
	}
	if (g_listeners == NULL) {
		/* there are no registered listeners */
		log_msg(LOG_WARN | LOG_CONF, "deleteing unregistered listener");
		((listener_handler) l->cleanup) (l);
		return;
	}
	if (g_listeners == l) {
		g_listeners = l->next;
		((listener_handler) l->cleanup) (l);
	} else {
		for (n = g_listeners; n->next != NULL; n = n->next) {
			if (n->next == l) {
				n->next = l->next;
				((listener_handler) l->cleanup) (l);
				return;
			}
		}
	}
	log_msg(LOG_WARN | LOG_CONF, "deleteing unregistered listener");
	((listener_handler) l->cleanup) (l);
}

listener_t
*
listener_search_by_fd(int fd)
{
	listener_t     *n;
	for (n = g_listeners; n != NULL; n = n->next) {
		if (n->fd == fd) {
			return n;
		}
	}
	return NULL;
}

void
listener_select()
{
	listener_t     *l;
	fd_set          fds;
	int             fd, maxfd;
	/* init fd_set */
	FD_ZERO(&fds);
	maxfd = 0;
	for (l = g_listeners; l != NULL; l = l->next) {
		fd = l->fd;
		if (fd >= 0) {
			FD_SET(fd, &fds);
			if (fd > maxfd) {
				maxfd = fd;
			}
		}
	}
	if (select(maxfd + 1, &fds, NULL, NULL, NULL) == -1) {
		if (errno == EINTR) {
			/* we have been interrupted, ignore */
			return;
		}
		log_msg(LOG_WARN | LOG_DEV, "select: %s", strerror(errno));
		return;
	}
	for (l = g_listeners; l != NULL; l = l->next) {
		fd = l->fd;
		if (fd >= 0) {
			if (FD_ISSET(fd, &fds)) {
				((listener_handler) l->receive) (l);
			}
		}
	}
}



/*
 * internal functions
 */


static int
i_listener_ioctl_immediate(int fd, int on)
{
#ifdef BIOCIMMEDIATE
	if (ioctl(fd, BIOCIMMEDIATE, &on) < 0) {
		log_msg(LOG_WARN | LOG_DEV, "ioctl(BIOCIMMEDIATE): %s", strerror(errno));
		return 0;
	}
	return 1;
#else
	log_msg(LOG_WARN | LOG_DEV, "ioctl(BIOCIMMEDIATE) not supported");
	return 0;
#endif
}

static int
i_listener_ioctl_shdrcmplt(int fd, int on)
{
#ifdef BIOCSHDRCMPLT
	if (ioctl(fd, BIOCSHDRCMPLT, &on) < 0) {
		log_msg(LOG_WARN | LOG_DEV, "ioctl(BIOCSHDRCMPLT): %s", strerror(errno));
		return 0;
	}
	return 1;
#else
	log_msg(LOG_WARN | LOG_DEV, "ioctl(BIOCSHDRCMPLT) not supported");
	return 0;
#endif
}

static int
i_listener_ioctl_sseesent(int fd, int on)
{
#ifdef BIOCSSEESENT
	if (ioctl(fd, BIOCSSEESENT, &on) < 0) {
		log_msg(LOG_WARN | LOG_DEV, "ioctl(BIOCSSEESENT): %s", strerror(errno));
		return 0;
	}
	return 1;
#else
	log_msg(LOG_WARN | LOG_DEV, "ioctl(BIOCSSEESENT) not supported");
	return 0;
#endif
}

static int
i_listener_fcntl_nonblock(int fd, int on)
{
#ifdef O_NDELAY
#ifndef O_NONBLOCK
#define O_NONBLOCK O_NDELAY
#endif
#endif
#ifdef O_NONBLOCK
	int             flags;
	if ((flags = fcntl(fd, F_GETFL, 0)) == -1) {
		log_msg(LOG_WARN, "fcntl(F_GETFL): %s", strerror(errno));
		return 0;
	}
	if (on) {
		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
			log_msg(LOG_WARN, "fcntl(F_SETFL, O_NONBLOCK): %s", strerror(errno));
			return 0;
		}
	} else {
		if (fcntl(fd, F_SETFL, flags & (!O_NONBLOCK)) == -1) {
			log_msg(LOG_WARN, "fntl(F_SETFL, !O_NONBLOCK): %s", strerror(errno));
			return 0;
		}
	}
	return 1;
#else
	log_msg(LOG_WARN | LOG_DEV, "fcntl(O_NONBLOCK) not supported");
	return 0;
#endif
}

static int
i_listener_link_init_addr(listener_t * l, char *device)
{
#ifdef HAVE_LIBNET10
	struct ether_addr *eth;
#endif
#ifdef HAVE_LIBNET11
	struct libnet_ether_addr *eth;
#endif
	char            ebuf[PCAP_ERRBUF_SIZE];
	char           *ch;

	if (l == NULL) {
		log_msg(LOG_ERR | LOG_DEV, "i_listener_link_init_addr: lnet not initialized");
	}
#ifdef HAVE_LIBNET10
	eth = libnet_get_hwaddr(l->d.link.lnet_link, device, ebuf);
#endif
#ifdef HAVE_LIBNET11
	eth = libnet_get_hwaddr(l->d.link.lnet);
	ebuf[0] = 0; /* XXX */
#endif
	if (eth == NULL) {
		log_msg(LOG_WARN | LOG_DEV, "libnet_get_hwaddr(%s): %s", device, ebuf);
	} else {
		memcpy(&(l->eth), eth, sizeof(etheraddr_t));
	}

	/*
	 * XXX should somehow get all ip addresses associated with this
	 * interface
	 */
#ifdef HAVE_LIBNET10
	l->ip.s_addr = ntohl(libnet_get_ipaddr(l->d.link.lnet_link, device, ebuf));
#endif
#ifdef HAVE_LIBNET11
	l->ip.s_addr = ntohl(libnet_get_ipaddr4(l->d.link.lnet));
#endif
	if (l->ip.s_addr == 0) {
		log_msg(LOG_WARN | LOG_DEV, "libnet_get_ipaddr(%s): %s", device, ebuf);
		l->ip.s_addr = 0;
	}
	/*
	 * XXX should resolve ip address belonging to this interface instead
	 * of just using hostname
	 */
	gethostname(ebuf, sizeof(ebuf));
	if ((ch = strchr(ebuf, '.')) != NULL) {
		*ch = 0;
	}
	l->name = strdup(ebuf);
	return 1;
}

static int
i_listener_link_init_pcap(listener_t * l, char *device)
{
	char            ebuf[PCAP_ERRBUF_SIZE];
	char            dlt_str[32];
	if ((l == NULL) || (device == NULL)) {
		log_msg(LOG_WARN | LOG_DEV, "i_listener_link_init_pcap(): invalid params");
		return 0;
	}
#define XXX_MY_ETHER_MAX_LEN 1518
	if ((l->d.link.pcap = pcap_open_live(device, XXX_MY_ETHER_MAX_LEN, 1, 1, ebuf)) == NULL) {
		/* log error, return false */
		log_msg(LOG_WARN | LOG_DEV, "pcap_open_live(%s): %s", device, ebuf);
		return 0;
	}
	if ((l->d.link.dlt = pcap_datalink(l->d.link.pcap)) != DLT_EN10MB) {
		/* XXX should extend support to other link types */
		str_dlt(l->d.link.dlt, sizeof(dlt_str), dlt_str);
		log_msg(LOG_WARN | LOG_DEV, "%s datalink type %s not supported",
		    device, dlt_str);
		pcap_close(l->d.link.pcap);
		l->d.link.pcap = NULL;
		l->d.link.dlt = 0;
		return 0;
	}
	/* XXX not sure if this works everywhere */
	l->fd = pcap_fileno(l->d.link.pcap);
	i_listener_ioctl_immediate(l->fd, 1);
	i_listener_ioctl_shdrcmplt(l->fd, 1);
	i_listener_ioctl_sseesent(l->fd, 0);
	i_listener_fcntl_nonblock(l->fd, 1);

	return 1;
}

static int
i_listener_link_init_lnet(listener_t * l, char *device)
{
	char            ebuf[PCAP_ERRBUF_SIZE];
	if ((l == NULL) || (device == NULL)) {
		log_msg(LOG_WARN | LOG_DEV, "i_listener_link_init_lnet(): invalid params");
		return 0;
	}
#ifdef HAVE_LIBNET10
	l->d.link.lnet_link = libnet_open_link_interface(device, ebuf);
	if (l->d.link.lnet_link == NULL) {
		log_msg(LOG_WARN | LOG_DEV, "libnet_open_link_interface(%s): %s", device, ebuf);
		return 0;
	}
#endif
#ifdef HAVE_LIBNET11
	l->d.link.lnet = libnet_init(LIBNET_LINK, device, ebuf);
	if (l->d.link.lnet == NULL) {
		log_msg(LOG_WARN | LOG_DEV, "libnet_open_link_interface(%s): %s", device, ebuf);
		return 0;
	}
#endif
	return 1;
}

static int
i_listener_link_init_filter(listener_t * l, char *device, char *filter)
{
	char            ebuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32     net, mask;
	struct bpf_program f;
	log_msg(LOG_INFO | LOG_DEV, "setting pcap filter:\n  %s", filter);

	if ((l == NULL) || (device == NULL)) {
		log_msg(LOG_WARN | LOG_DEV, "i_listener_link_init_filter(): invalid params");
		return 0;
	}
	if (filter == NULL) {
		/* no filter */
		return 1;
	}
	if (pcap_lookupnet(device, &net, &mask, ebuf) == -1) {
		log_msg(LOG_WARN | LOG_DEV, "pcap_lookupnet(%s): %s", device, ebuf);
		return 0;
	}
	if (pcap_compile(l->d.link.pcap, &f, filter, 1, mask) == -1) {
		log_msg(LOG_WARN | LOG_DEV, "pcap_compile: %s", pcap_geterr(l->d.link.pcap));
		return 0;
	}
	if (pcap_setfilter(l->d.link.pcap, &f) == -1) {
		log_msg(LOG_WARN | LOG_DEV, "pcap_setfilter(%s): %s", device, ebuf);
		/* XXX */
#ifdef HAVE_PCAP_FREECODE
		pcap_freecode(&f);
#endif
		return 0;
	}
#ifdef HAVE_PCAP_FREECODE
	pcap_freecode(&f);
#endif
	return 1;
}

static void
i_listener_link_cleanup_pcap(listener_t * l)
{
	if ((l != NULL) && (l->d.link.pcap != NULL)) {
		pcap_close(l->d.link.pcap);
		l->d.link.pcap = NULL;
		l->fd = 0;
	}
}

static void
i_listener_link_cleanup_lnet(listener_t * l)
{
	if (l == NULL) {
		return;
	}
#ifdef HAVE_LIBNET10
	if (l->d.link.lnet_link != NULL) {
		libnet_close_link_interface(l->d.link.lnet_link);
		l->d.link.lnet_link = NULL;
	}
#endif
#ifdef HAVE_LIBNET11
	if (l->d.link.lnet != NULL) {
		libnet_destroy(l->d.link.lnet);
		l->d.link.lnet = NULL;
	}
#endif
}

static void
i_listener_link_pcap_callback(u_char * u, const struct pcap_pkthdr * pkthdr, const u_char * pkt)
{
	struct listener_eth_hdr *ether;
	listener_t     *l;
	char            from[64];
	char            to[64];
	ether = (struct listener_eth_hdr *) pkt;

	str_eth(&(ether->saddr), sizeof(from), from);
	str_eth(&(ether->saddr), sizeof(to), to);

	l = (listener_t *) u;

	/* 802.3 packet */
	switch (ntohs(ether->len)) {
	case ETHERTYPE_REVARP:
		log_msg(LOG_DEBUG | LOG_DEV, "lnet recv revarp %s -> %s (%d)", from, to, pkthdr->len);
		rarp_handler(l, NULL, NULL, (void *)pkt, pkthdr->len);
		break;
	default:
		if (ntohs(ether->len) < 1536) {
			/* Ethernet II packet */
			switch (ether->dsap) {
			case SAP_HP:
				log_msg(LOG_DEBUG | LOG_DEV, "lnet recv sap_hp %s -> %s (%d)", from, to, pkthdr->len);
				rmp_handler(l, NULL, NULL, (void *)pkt, pkthdr->len);
				break;
			default:
				/*
				 * do not log unhandled packets, there are
				 * just too many  of them
				 */
				break;
			}
			break;
		}
	}
}

/*
 * well, i don't know what they thought here... they could have made it
 * easier to write 'interactive' applications. can't seem avoid this 1ms
 * delay
 */
static void
i_listener_link_receive(listener_t * l)
{
	/* XXX */
	pcap_loop(l->d.link.pcap, 1, i_listener_link_pcap_callback, (u_char *) l);
}

static void
i_listener_link_cleanup(listener_t * l)
{
	if (l == NULL) {
		return;
	}
	if (l->name != NULL) {
		free(l->name);
	}
	i_listener_link_cleanup_lnet(l);
	i_listener_link_cleanup_pcap(l);
	if (l->d.link.device != NULL) {
		free(l->d.link.device);
	}
	if (l->d.link.filter != NULL) {
		free(l->d.link.filter);
	}
	free(l);
}
/* from/to is ignored because it is contained in packet */

static int
i_listener_link_send(listener_t * l, void *from, void *to, void *packet, int pktlen)
{
	struct listener_eth_hdr *eth;
	char            from_str[32];	/* 17 would be enough */
	char            to_str[32];
	int             result;
	/*
	 * minimal packet length (ETHER_MIN_LEN is 64, but this includes the
	 * crc, which is not inclued here)
	 */
	if (pktlen < 62) {
		log_msg(LOG_WARN | LOG_DEV, "link_send: short packet (len=%d)", pktlen);
		return -1;
	}
	if (l == NULL) {
		log_msg(LOG_WARN | LOG_DEV, "link_send: invalid params");
		return -1;
	}
	eth = (void *)packet;
	str_eth(&(eth->saddr), sizeof(from_str), from_str);
	str_eth(&(eth->saddr), sizeof(to_str), to_str);
#ifdef HAVE_LIBNET10
	result = libnet_write_link_layer(l->d.link.lnet_link, l->d.link.device, packet, pktlen);
#elif HAVE_LIBNET11
	result = libnet_write_link(l->d.link.lnet, packet, pktlen);
#else
	result = -1;
#endif
	if (result == -1) {

		log_msg(LOG_DEBUG | LOG_DEV, "link_send (%s) %s -> %s (%d bytes) (failed)",
		    l->desc, from_str, to_str, pktlen);
	} else {
		log_msg(LOG_DEBUG | LOG_DEV, "link_send (%s) %s -> %s (%d bytes)",
		    l->desc, from_str, to_str, pktlen);
	}
	return result;
}

static void
i_listener_udp_receive(listener_t * l)
{
	char            buf[4096];
	socklen_t       s_inl;
	struct sockaddr_in sai_from;
	struct sockaddr_in sai_to;
	char            from_str[32];
	char            to_str[32];
	int             len;
	s_inl = sizeof(struct sockaddr_in);
	if ((len = recvfrom(l->fd, buf, sizeof(buf), 0,
			    (struct sockaddr *) & sai_from, &s_inl)) == -1) {
		if ((errno == EAGAIN) || (errno == EINTR)) {
			/* there was no data, or we were interrupted */
			return;
		}
		log_msg(LOG_WARN | LOG_DEV, "recvfrom(%s): %s", l->desc, strerror(errno));
		return;
	}
	s_inl = sizeof(struct sockaddr_in);
	if (getsockname(l->fd, (struct sockaddr *) & sai_to, &s_inl) == -1) {
		log_msg(LOG_WARN | LOG_DEV, "getsockname(): %s", strerror(errno));
		return;
	}
	str_sai(&sai_from, sizeof(from_str), from_str);
	str_sai(&sai_to, sizeof(to_str), to_str);
	log_msg(LOG_DEBUG | LOG_DEV, "udp recv %s -> %s (%d)", from_str, to_str, len);

	if (l->service != NULL) {
		((listener_service) l->service) (l, &sai_from, &sai_to, buf, len);
	} else {
		/* programming error! */
		log_msg(LOG_WARN | LOG_DEV, "%s request from host %s to %s rejected (no service handler)",
		    l->desc, from_str, to_str);
	}
}

static void
i_listener_udp_cleanup(listener_t * l)
{
	if (l == NULL) {
		return;
	}
	if (l->name != NULL) {
		free(l->name);
	}
	close(l->fd);
	free(l);
}

static int
i_listener_udp_send(listener_t * l, void *from, void *to, void *packet, int pktlen)
{
	char            from_str[32];	/* 22 would be enough */
	char            to_str[32];
	struct sockaddr_in *sai_to;
	struct sockaddr_in *sai_from;
	int             result;
	if (l == NULL) {
		log_msg(LOG_WARN | LOG_DEV, "udp_send: invalid params");
		return -1;
	}
	/* from is ignored and taken from l */
	sai_from = &(l->d.udp.from);
	sai_to = to;

	if (sai_to != NULL) {
		if ((sai_to->sin_addr.s_addr == l->d.udp.to.sin_addr.s_addr) && (sai_to->sin_port == l->d.udp.to.sin_port)) {
			/*
			 * to has been specified, but is the same as the
			 * value in l -> ignore
			 */
			sai_to = NULL;
		} else if ((sai_to->sin_addr.s_addr == INADDR_ANY) && (sai_to->sin_port == 0)) {
			/*
			 * to has been specified, but is unspecified ->
			 * ignore
			 */
			sai_to = NULL;
		}
	}
	if (sai_to == NULL) {
		if ((result = send(l->fd, packet, pktlen, 0)) >= 0) {
			/* we actually sent it to this address */
			sai_to = &(l->d.udp.to);
		}
	} else {
		result = sendto(l->fd, packet, pktlen, 0, (struct sockaddr *) sai_to, sizeof(struct sockaddr_in));
	}

	str_sai(sai_from, sizeof(from_str), from_str);
	str_sai(sai_to, sizeof(to_str), to_str);
	if (result < 0) {
		/* failed */
		log_msg(LOG_DEBUG | LOG_DEV, "udp_send (%s) %s -> %s (%d bytes) (failed)", l->desc, from_str, to_str, pktlen);
	} else {
		log_msg(LOG_DEBUG | LOG_DEV, "udp_send (%s) %s -> %s (%d bytes)", l->desc, from_str, to_str, pktlen);
	}
	return result;
}

static void
i_listener_rpc_udp_receive(listener_t * l)
{
	if (!FD_ISSET(l->fd, &svc_fdset)) {
		log_msg(LOG_WARN | LOG_DEV, "listener_rcp_handler: %d is not an rpc fd", l->fd);
		return;
	}
#ifdef HAVE_SUN_RPC_NEW
	svc_getreq_common(l->fd);
#else
	svc_getreqset(&svc_fdset);
#endif
}

static void
i_listener_rpc_udp_cleanup(listener_t * l)
{
	if (l == NULL) {
		return;
	}
	if (l->name != NULL) {
		free(l->name);
	}
	close(l->fd);
	free(l);
}
