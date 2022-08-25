/*
 * $Id: device.h,v 1.2 2002/07/22 13:03:14 pajarola Exp $
 */

#ifndef _DEVICE_H
#define _DEVICE_H

#define DEVICE_IP_ADDRS_MAX 16

typedef struct {
	void           *next;
	char           *name;	/* device name */
	int             flags;	/* device flags */
	etheraddr_t     eth;	/* ethernet address (assume max 1 ethernet
				 * address per device) */
	ipaddr_t        ip[DEVICE_IP_ADDRS_MAX];	/* ip addresses */
	ipaddr_t        nm[DEVICE_IP_ADDRS_MAX];	/* netmasks */
	ipaddr_t        bc[DEVICE_IP_ADDRS_MAX];	/* broadcast addresses */
	char            filter[4096];
}               device_t;

void            device_init();
device_t       *device_search_by_name(char *name);
device_t       *device_search_by_eth(etheraddr_t * eth);
device_t       *device_search_by_ip(ipaddr_t * ip);
#endif
