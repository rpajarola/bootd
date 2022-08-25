/*
 * $Id: listener.h,v 1.2 2002/07/22 13:03:14 pajarola Exp $
 */

#ifndef _LISTENER_H
#define _LISTENER_H

typedef void    (*listener_handler) (void *l);
typedef int     (*listener_service) (void *l, void *from, void *to, void *packet, int pktlen);

typedef struct {
	void           *next;
	int             fd;	/* file descriptor */
	char            info[64];	/* technical (printable) description */
	char            desc[64];	/* user supplied (printable)
					 * description */
	ipaddr_t        ip;	/* local ip address */
	etheraddr_t     eth;	/* local ethernet address */
	char           *name;	/* local hostname (server name) */
	listener_service service;	/* user defined handler for each
					 * packet */

	listener_handler receive;	/* handler called to receive packet */
	listener_handler cleanup;	/* handler called on cleanup */
	listener_service send;	/* handler to send */

	int             type;	/* LISTEN_LINK, LISTEN_UDP, LISTEN_RPC_UDP */
	union {
		/* link level */
		struct {
			pcap_t         *pcap;	/* pcap handle */
#ifdef HAVE_LIBNET10
			struct libnet_link_int *lnet_link;	/* libnet link interface */
#endif
#ifdef HAVE_LIBNET11
			libnet_t *lnet;  /* libnet context */
#endif
			char           *device;
			char           *filter;
			int             dlt;	/* data link type */
		}               link;

		/* udp */
		struct {
			struct sockaddr_in from;	/* local sockaddr
							 * ('getsockname') */
			struct sockaddr_in to;	/* remote sockaddr
						 * ('getpeername') */
		}               udp;

		/* rpc */
		struct {
			struct sockaddr_in from;	/* local sockaddr
							 * ('getsockname') */
		}               rpc_udp;
	}               d;
}               listener_t;

struct listener_eth_hdr {
	/* 802.3 Ethernet header */
	etheraddr_t     daddr;
	etheraddr_t     saddr;
	u_int16_t       len;
	/* HP 802.2 LLC with extensions */
	u_int8_t        dsap;	/* 802.2 DSAP */
	u_int8_t        ssap;	/* 802.2 SSAP */
	u_int16_t       cntl;	/* 802.2 control field */
};
/*
 * create new link listener (send/receive link level frames). You have to
 * call listener_add to enable it filter and desc may be NULL
 */
listener_t     *listener_new_link(char *device, char *filter, listener_service service, char *desc);
/*
 * create new udp listener, create socket, bind to <from> (if !=NULL),
 * connect to <to> (if !=NULL). You have to call listener_add to enable it
 * from, to and desc may be null
 */
listener_t     *listener_new_udp(struct sockaddr_in * from, struct sockaddr_in * to, listener_service service, char *desc);
/*
 * create new rpc/udp listener. You have to call listener_add to enable it
 * desc may be null because rpc is handled differently, 'dispatch' is not of
 * type listener_handler
 */
listener_t     *listener_new_rpc_udp(struct sockaddr_in * from, rpcprog_t prognum, rpcvers_t versnum, void (*dispatch) (struct svc_req *, SVCXPRT *), char *desc);
/* add listener to list ('enable') */
void            listener_add(listener_t * l);
/* delete listener from list */
void            listener_delete(listener_t * l);
/* search listener */
listener_t     *listener_search_by_fd(int fd);
/*
 * select() all listeners, and call handlers as apropriate, return first
 * event.
 */
void            listener_select();
#endif
