/*
 * $Id: host.h,v 1.2 2002/07/22 13:03:14 pajarola Exp $
 */

#ifndef _HOST_H
#define _HOST_H

#define FILE_ALIASES_MAX 10
typedef struct {
	void           *next;
	char           *path;	/* full pathname (including filename) in
				 * filesystem */
	char           *name;	/* official name */
	char           *server;	/* server name */
	char           *aliases[FILE_ALIASES_MAX];	/* recognized aliases */
	int             service;/* allowed services */
}               file_t;
#define SESSION_FD_MAX 8
typedef struct {
	int             service;/* service */
	int             atime;	/* last activity time */
	int             sid;	/* session id */
	int             fd[SESSION_FD_MAX];	/* file descriptors */
	listener_t     *l;	/* associated listener (for sending) */
	char            data[4096];	/* service specific session data */
}               session_t;

typedef struct {
	void           *next;
	char           *device;	/* device name */
	char           *name;	/* host name */
	int             service;/* allowed services */
	etheraddr_t     eth;	/* ethernet address */
	ipaddr_t        ip;	/* ip address */
	file_t         *files;	/* files */
	session_t      *session;/* active session */
}               host_t;
#define E_NOERROR	0
#define E_NOSESSION	1
#define E_BADSID	2

host_t         *host_new();
host_t         *host_clone(host_t * src, char *name, etheraddr_t * ether, ipaddr_t * ip);
void            host_delete(host_t * h);
void            host_add(host_t * h);

file_t         *host_file_new(host_t * h);
void            host_file_add(host_t * h, file_t * f);

int             host_session_add(host_t * h, int service, int sid);
void            host_session_delete(host_t * h, char *msg);
void            host_session_timeout(host_t * h);
int             host_session_check(host_t * h, int service, int sid, char *desc);
#endif
