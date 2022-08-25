/*
 * $Id: service.h,v 1.2 2002/07/22 13:03:14 pajarola Exp $
 */

#ifndef _SERVICE_H
#define _SERVICE_H

#define SERVICES_MAX 16
#define SERVICE_BIND_MAX 16
typedef struct {
	void           *next;
	char           *name;	/* service name */
	int             id;	/* service id */
	void           *service;/* service routine */
	union {
		/* link level */
		struct {
			char           *(device[SERVICE_BIND_MAX]);	/* list of devices */
			char           *filter;	/* pcap filter expression for
						 * this service */
		}               link;

		/* udp */
		struct {
			struct sockaddr_in bindaddr[SERVICE_BIND_MAX];
			int             defport;	/* default port */
		}               udp;

		/* rpc/udp */
		struct {
			struct sockaddr_in bindaddr[SERVICE_BIND_MAX];
			int             prognum;
			int             versnum;
		}               rpc_udp;
	}               d;
	listener_handler receive;
	listener_handler cleanup;
	listener_handler send;
}               service_t;
#define SERVICE_ID_RARP	0
#define SERVICE_ID_DHCP	1
#define SERVICE_ID_MOP	2
#define SERVICE_ID_RMP	3
#define SERVICE_ID_TFTP	4
#define SERVICE_ID_BOOTPARM 5

#define SERVICE_RARP	(1<<SERVICE_ID_RARP)
#define SERVICE_DHCP	(1<<SERVICE_ID_DHCP)
#define SERVICE_MOP	(1<<SERVICE_ID_MOP)
#define SERVICE_RMP	(1<<SERVICE_ID_RMP)
#define SERVICE_TFTP	(1<<SERVICE_ID_TFTP)
#define SERVICE_BOOTPARM (1<<SERVICE_ID_BOOTPARM)
#define SERVICE_ALL	0xffff

#define LOG_RARP	(SERVICE_ID_RARP<<LOG_MOD_SHIFT)
#define LOG_DHCP	(SERVICE_ID_DHCP<<LOG_MOD_SHIFT)
#define LOG_MOP		(SERVICE_ID_MOP<<LOG_MOD_SHIFT)
#define LOG_RMP		(SERVICE_ID_RMP<<LOG_MOD_SHIFT)
#define LOG_TFTP	(SERVICE_ID_TFTP<<LOG_MOD_SHIFT)
#define LOG_BOOTPARM	(SERVICE_ID_BOOTPARM<<LOG_MOD_SHIFT)


#define LISTEN_LINK	(SERVICE_RARP | SERVICE_MOP | SERVICE_RMP)
#define LISTEN_UDP	(SERVICE_DHCP | SERVICE_TFTP)
#define LISTEN_RPC_UDP	(SERVICE_BOOTPARM)

#define E_NOERROR	0
#define E_NOSESSION	1
#define E_BADSID	2


service_t      *service_new();
void            service_add(service_t * s);
void            service_init();
service_t      *service_search_by_name(char *name);
service_t      *service_search_by_id(int id);
int             service_link_add(service_t * s, char *device);
int             service_udp_add(service_t * s, char *bind);

void            service_start();
#endif
