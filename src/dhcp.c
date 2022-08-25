/*
 * $Id: dhcp.c,v 1.4 2002/07/22 13:05:20 pajarola Exp $
 */

#include "bootd.h"

void
dhcp_handler(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, void *packet, int pktlen)
{
	struct dhcp_header *req, *repl;
	char            buf[4096];
	host_t         *h;

	req = (struct dhcp_header *) packet;
	repl = (struct dhcp_header *) buf;

	dhcp_logpacket(l, from, to, packet, pktlen);

	/* sanity checking */
	if (req->hops > 16) {
		log_msg(LOG_DEV | LOG_INFO, "dhcp request: max hop count exceeded");
		return;
	}
	if ((strlen(req->sname) > sizeof(req->sname)) ||
	    (strlen(req->file) > sizeof(req->file))) {
		/*
		 * of those strings is not 0 terminated
		 */
		log_msg(LOG_DEV | LOG_WARN, "malformed dhcp packet");
		return;
	}
	if ((req->sname[0] != 0) && (strcmp(req->sname, l->name) != 0)) {
		/*
		 * client specified a server name, and server name does not
		 * match, log and discard. we do not forward such packets
		 */
		log_msg(LOG_DEV | LOG_INFO, "dhcp request for other server (%s)", req->sname);
		return;
	}
	/* from myip:bootpd to hisip:bootpc */
	/* XXX set pinfo accordingly */

	/* check config */
	if ((h = util_ether2host((etheraddr_t*)req->chaddr)) == NULL) {
		str_eth((etheraddr_t*) req->chaddr, sizeof(buf), buf);
		log_msg(LOG_INFO | LOG_BOOT,
		    "dhcp request from host %s denied (client unknown)", buf);
		return;
	}
	switch (req->op) {
	case DHCPOP_BOOTREQUEST:
		/* bootp boot request */
		if ((h->service & SERVICE_DHCP) == 0) {
			log_msg(LOG_INFO | LOG_BOOT,
			    "bootp request from host %s denied (bootp not allowed for this host)",
			    h->name);
			return;
		}
		dhcp_logpacket(l, to, from, repl, 0);	/* XXX length? */
		return;
		break;
	default:
		/* illegal dhcp opcode */
		log_msg(LOG_INFO | LOG_BOOT,
		    "dhcp request from host %s denied (unknown opcode %d)",
		    h->name, ntohs(req->op));
		return;
	}
}

void
dhcp_logpacket(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, void *packet, int pktlen)
{
	struct dhcp_header *dhcp;
	dhcp = (struct dhcp_header *) packet;

	switch (ntohs(dhcp->op)) {
	case DHCPOP_BOOTREQUEST:
		break;
	}
}
