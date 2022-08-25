/*
 * $Id: rarp.c,v 1.6 2002/07/22 16:08:32 pajarola Exp $
 */

#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif
#ifdef HAVE_NET_IF_TYPES_H
#include <net/if_types.h>
#endif

#include "bootd.h"

void
rarp_handler(listener_t * l, void *from, void *to, void *packet, int pktlen)
{
	struct rarp_header *req, *repl;
	host_t         *h;
	char            buf[4096];
	int             npktlen;

	if (pktlen < (LIBNET_ETH_H + sizeof(struct libnet_arp_hdr))) {
		/* too short! */
		return;
	}
	req = (struct rarp_header *) packet;

	if ((ntohs(req->type) != ETHERTYPE_REVARP) ||
	    (ntohs(req->hrd) != ARPHRD_ETHER) ||
	    (ntohs(req->pro) != ETHERTYPE_IP) ||
	    (req->hln != 6) ||
	    (req->pln != 4)
		) {
		/* not a rarp packet */
		return;
	}
	rarp_logpacket(l, NULL, NULL, packet, pktlen);

	/* check config */
	if ((h = util_ether2host((etheraddr_t*)& (req->tha))) == NULL) {
		str_eth(&(req->saddr), sizeof(buf), buf);
		log_msg(LOG_INFO | LOG_BOOT,
		    "rarp request from host %s denied (client unknown)", buf);
		return;
	}
	if ((h->service & SERVICE_RARP) == 0) {
		log_msg(LOG_INFO | LOG_BOOT,
		    "rarp request from host %s denied (rarp not allowed for this host)",
		    h->name);
		return;
	}
	/* set up reply */
	repl = (struct rarp_header *) buf;
	memcpy(repl->daddr.octet, req->saddr.octet, sizeof(struct libnet_ether_addr));
	memcpy(repl->saddr.octet, l->eth.octet, sizeof(struct libnet_ether_addr));
	repl->type = htons(ETHERTYPE_REVARP);
	repl->hrd = htons(ARPHRD_ETHER);
	repl->pro = htons(ETHERTYPE_IP);
	repl->hln = 6;
	repl->pln = 4;

	/* must set repl->op, repl->sha, repl->spa, repl->tha, repl->tpa */

	switch (ntohs(req->op)) {
	case ARPOP_REVREQUEST:
		repl->op = htons(ARPOP_REVREPLY);
		memcpy(repl->sha, l->eth.octet, sizeof(struct libnet_ether_addr));
		memcpy(repl->spa, &(l->ip), sizeof(struct in_addr));
		memcpy(repl->tha, req->tha, sizeof(struct libnet_ether_addr));
		memcpy(repl->tpa, &(h->ip), sizeof(struct in_addr));

		rarp_updatetable(&(h->ip), &(h->eth));
		npktlen = max(60, sizeof(struct rarp_header));
		rarp_logpacket(l, NULL, NULL, buf, npktlen);
		if (l->send(l, NULL, NULL, buf, npktlen) != npktlen) {
			log_msg(LOG_WARN | LOG_BOOT, "error sending rarp reply\n");
		}
		break;
	default:
		/* not a rarp request */
		return;
	}
}

/*
 * put entry into arp table (most netbooting hosts wouldn't answer an arp
 * request before the os is loaded)
 */

#ifdef SIOCSARP

void
rarp_updatetable(ipaddr_t *ip, etheraddr_t *eth)
{
	struct arpreq   request;
	struct sockaddr_in *sin;
	int             s;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		log_msg(LOG_WARN | LOG_DEV, "socket(AF_INET, SOCK_DGRAM, 0): %s", strerror(errno));
		return;
	}
	request.arp_flags = 0;
	sin = (struct sockaddr_in *) & request.arp_pa;
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = ip->s_addr;
	request.arp_ha.sa_family = AF_UNSPEC;
	memcpy(request.arp_ha.sa_data, eth, sizeof(struct libnet_ether_addr));
	if (ioctl(s, SIOCSARP, (caddr_t) & request) == -1) {
		log_msg(LOG_WARN | LOG_DEV, "SIOCSARP: %s", strerror(errno));
	}
	close(s);
}

#else	/* ifdef SIOCSARP */
#ifdef PF_ROUTE

#define ARPSECS (20*60)		/* as per netinet/if_ether.c */

void
rarp_updatetable(ipaddr_t *ip, etheraddr_t *eth)
{
	int             cc;
	char            buf[4096];
	char		s_af[32];

	struct rt_msghdr *prt;
	struct sockaddr_inarp sa, *psa;
	struct sockaddr_dl dl, *pdl;

	static int      seq;
	pid_t           pid;
	int             rfd;

	/* initialize sa and dl */
	memset(&sa, 0, sizeof(sa));
	sa.sin_len = sizeof(struct sockaddr_inarp);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = ip->s_addr;

	memset(&dl, 0, sizeof(struct sockaddr_dl));
	dl.sdl_len = sizeof(struct sockaddr_dl);
	dl.sdl_family = AF_LINK;
	dl.sdl_type = IFT_ETHER;
	dl.sdl_alen = 6;
	memcpy(dl.sdl_data, eth, sizeof(struct libnet_ether_addr));

	pid = getpid();
	if ((rfd = socket(PF_ROUTE, SOCK_RAW, 0)) == -1) {
		log_msg(LOG_WARN | LOG_DEV, "socket(PF_ROUTE, SOCK_RAW): %s", strerror(errno));
		return;
	}
	memset(buf, 0, sizeof(buf));
	prt = (void *)&(buf[0]);
	psa = (void *)&(buf[sizeof(struct rt_msghdr)]);
	pdl = (void *)&(buf[sizeof(struct rt_msghdr) + sizeof(struct sockaddr_inarp)]);

	/* get the type and interface index */
	prt->rtm_version = RTM_VERSION;
	prt->rtm_addrs = RTA_DST;
	prt->rtm_type = RTM_GET;
	prt->rtm_seq = ++seq;
	prt->rtm_msglen = sizeof(struct rt_msghdr) + sizeof(struct sockaddr_inarp);
	memcpy(psa, &sa, sizeof(struct sockaddr_inarp));

	if ((write(rfd, buf, prt->rtm_msglen) == -1) && (errno != ESRCH)) {
		log_msg(LOG_WARN | LOG_DEV, "rtmsg get write: %s", strerror(errno));
		close(rfd);
		return;
	}
	do {
		cc = read(rfd, buf, sizeof(buf));
	} while (cc > 0 && (prt->rtm_seq != seq || prt->rtm_pid != pid));

	if (cc == -1) {
		log_msg(LOG_WARN | LOG_DEV, "rtmsg get read: %s", strerror(errno));
		close(rfd);
		return;
	}
	if (pdl->sdl_family == AF_INET) {
		log_msg(LOG_WARN | LOG_DEV, "wrong link family (%s), address %s not local",
		    str_af(pdl->sdl_family, sizeof(s_af), s_af), inet_ntoa(*(struct in_addr*)ip));
		close(rfd);
		return;
	}
	if (pdl->sdl_family != AF_LINK) {
		log_msg(LOG_WARN | LOG_DEV, "wrong link family (%s) for %s",
		    str_af(pdl->sdl_family, sizeof(s_af), s_af), inet_ntoa(*(struct in_addr*)ip));
		close(rfd);
		return;
	}
	dl.sdl_type = pdl->sdl_type;
	dl.sdl_index = pdl->sdl_index;

	/* set new arp entry */
	memset(buf, 0, sizeof(buf));
	prt->rtm_version = RTM_VERSION;
	prt->rtm_addrs = RTA_DST | RTA_GATEWAY;
	prt->rtm_inits = RTV_EXPIRE;
	prt->rtm_rmx.rmx_expire = time(0) + ARPSECS;
	prt->rtm_flags = RTF_HOST | RTF_STATIC;
	prt->rtm_type = RTM_ADD;
	prt->rtm_seq = ++seq;
	memcpy(psa, &sa, sizeof(struct sockaddr_inarp));
	memcpy(pdl, &dl, sizeof(struct sockaddr_dl));
	prt->rtm_msglen = sizeof(struct rt_msghdr) + sizeof(struct sockaddr_inarp) + sizeof(struct sockaddr_dl);
	if ((write(rfd, prt, prt->rtm_msglen) == -1) && (errno != EEXIST)) {
		log_msg(LOG_WARN | LOG_DEV, "rtmsg add write: %s", strerror(errno));
		close(rfd);
		return;
	}
	do {
		cc = read(rfd, buf, sizeof(buf));
	} while (cc > 0 && (prt->rtm_seq != seq || prt->rtm_pid != pid));

	close(rfd);
	if (cc == -1) {
		log_msg(LOG_WARN | LOG_DEV, "rtmsg add read: %s", strerror(errno));
	}
}

#else				/* ifdef PF_ROUTE */

void
rarp_updatetable(struct in_addr * i, struct libnet_ether_addr * eth)
{
	/* XXX live without... */
	log_msg(LOG_WARN | LOG_DEV, "updating RARP table not supported, booting may fail");
}

#endif
#endif				/* falback */

void
rarp_logpacket(listener_t * l, void *from, void *to, void *packet, int pktlen)
{
	struct rarp_header *rarp;
	char            s_daddr[32], s_saddr[32];
	char            s_sha[32], s_spa[32];
	char            s_tha[32], s_tpa[32];
	char           *s_hrd, *s_pro, *s_op;

	rarp = (struct rarp_header *) packet;
	str_eth(&(rarp->daddr), sizeof(s_daddr), s_daddr);
	str_eth(&(rarp->saddr), sizeof(s_saddr), s_saddr);
	str_eth((etheraddr_t*)&(rarp->sha), sizeof(s_sha), s_sha);
	str_eth((etheraddr_t*)&(rarp->tha), sizeof(s_tha), s_tha);
	str_ip((ipaddr_t*)&(rarp->spa), sizeof(s_spa), s_spa);
	str_ip((ipaddr_t*)&(rarp->tpa), sizeof(s_tpa), s_tpa);

	switch (ntohs(rarp->hrd)) {
	case ARPHRD_ETHER:
		s_hrd = "ether";
		break;
	case ARPHRD_IEEE802:
		s_hrd = "token-ring";
		break;
#ifdef ARPHRD_FRELAY
	case ARPHRD_FRELAY:
		s_hrd = "frame relay";
		break;
#endif
	default:
		s_hrd = "unknown";
	}

	switch (ntohs(rarp->pro)) {
	case ETHERTYPE_IP:
		s_pro = "IP";
		break;
#ifdef ETHERTYPE_IPV6
	case ETHERTYPE_IPV6:
		s_pro = "IPv6";
		break;
#endif
	default:
		s_pro = "unknown";
	}

	switch (ntohs(rarp->op)) {
	case ARPOP_REVREQUEST:
		s_op = "rev request";
		break;
	case ARPOP_REVREPLY:
		s_op = "rev reply";
		break;
	default:
		s_op = "unknown request";
	}


	log_msg(LOG_INFO | LOG_BOOT,
	    "rarp %s (0x%x)\n"
	    "  from %s to %s\n"
	    "  hardware %s (length %d), protocol %s (length %d)\n"
	    "  source %s %s\n"
	    "  target %s %s\n",

	    s_op, ntohs(rarp->op), s_saddr, s_daddr,
	    s_hrd, rarp->hln, s_pro, rarp->pln,
	    s_sha, s_spa, s_tha, s_tpa);

	/* check that rarp->saddr == rarp->sha */
	/* check that rarp->sha == rarp->tha */
}
