/*
 * $Id: util.h,v 1.3 2002/07/22 16:08:32 pajarola Exp $
 */

#ifndef _UTIL_H
#define _UTIL_H

void            util_stats();
void            util_cleanup();

int             util_ether_valid(etheraddr_t * ether);
int             util_ip_valid(ipaddr_t * ip);

int             util_name2ether(char *name, etheraddr_t * ether);
int             util_name2ip(char *name, ipaddr_t * ip);
char           *util_ether2name(etheraddr_t * ether);
char           *util_ip2name(ipaddr_t * ip);

host_t         *util_name2host(char *name);
host_t         *util_ether2host(etheraddr_t * e);
host_t         *util_ip2host(ipaddr_t * i);

file_t         *util_name2file(host_t * h, int service, char *name, int no);

void            util_host_session_timeout_all();

int             util_read(int fd, char *buf, int len, off_t offs);
#define IP_HDR_SIZE(ip) (ip->ip_hl*4)
#define max(a,b) ((a) >= (b) ? (a) : (b))
#define min(a,b) ((a) >= (b) ? (b) : (a))

#endif
