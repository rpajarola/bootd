/*
 * $Id: rarp.h,v 1.4 2002/07/22 13:05:22 pajarola Exp $
 */

#ifndef _RARP_H
#define _RARP_H

/*
 * Reverse Address Resolution Protocol as specified in RFC903
 * 
 * the code in rarp_updatetable has been derived from the FreeBSD rarpd source
 * (/usr/src/usr.sbin/rarpd/rarpd.c)
 */

struct rarp_header {

	/* Ethernet Header */
	etheraddr_t daddr;
	etheraddr_t saddr;
	u_int16_t       type;

	/* RARP header */
	u_short         hrd;
	u_short         pro;
	u_char          hln;
	u_char          pln;
	u_short         op;
	u_char          sha[6];
	u_char          spa[4];
	u_char          tha[6];
	u_char          tpa[4];
};

void            rarp_handler(listener_t * l, void *from, void *to, void *packet, int pktlen);
void            rarp_logpacket(listener_t * l, void *from, void *to, void *packet, int pktlen);
void            rarp_updatetable(ipaddr_t *ip, etheraddr_t *eth);

#endif
