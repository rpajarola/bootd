/*
 * $Id: dhcp.h,v 1.4 2002/07/22 13:05:21 pajarola Exp $
 */

#ifndef _DHCP_H
#define _DHCP_H

/*
 * BOOTstrap Protocol as specified in RFC951, Dynamic Host Configuration
 * Protocol as specified somewhere else...
 */

#define DHCPOP_BOOTREQUEST	1
#define DHCPOP_BOOTREPLY	2

/* generic dhcp header */
struct dhcp_header {
	u_int8_t        op;	/* packet op code */
	u_int8_t        htype;	/* hardware address type */
	u_int8_t        hlen;	/* hardware address length */
	u_int8_t        hops;	/* hop count (for gateways) */
	u_int32_t       xid;	/* transaction id */
	u_int16_t       secs;	/* seconds elapsed since boot */
	u_int16_t       flags;	/* flags (bit1: broadcast) */
	u_int32_t       ciaddr;	/* client ip address */
	u_int32_t       yiaddr;	/* 'your' ip address */
	u_int32_t       siaddr;	/* server ip address */
	u_int32_t       giaddr;	/* gateway ip address */
	char            chaddr[16];	/* client hardware address */
	char            sname[64];	/* server hostname, 0 terminated */
	char            file[128];	/* boot file name, 0 terminated */
	char            vend[64];	/* vendor specific area */
};

void            dhcp_handler(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, void *packet, int pktlen);
void            dhcp_logpacket(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, void *packet, int pktlen);

#endif
