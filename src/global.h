/*
 * $Id: global.h,v 1.3 2002/07/22 13:03:14 pajarola Exp $
 */

#ifndef _GLOBAL_H
#define _GLOBAL_H

#if defined(GLOBAL)
#define EXTERN
#define INIT(i) = i
#else
#define EXTERN extern
#define INIT(i)
#endif

/* config */
EXTERN char    *c_file INIT(NULL);	/* config file */
EXTERN int c_resolve_ether INIT(1);	/* resolve ethernet names */
EXTERN int c_resolve_ip INIT(1);/* resolve ip names */
EXTERN char    *c_directory INIT(NULL);	/* base directory for file searches */
EXTERN char    *c_chroot INIT(NULL);	/* chroot to directory after reading
					 * config */
EXTERN char    *c_log_file INIT(NULL);	/* log file */
EXTERN int c_log_level INIT(LOG_DEBUG);	/* log level */
EXTERN int c_session_max INIT(5);	/* max # of active sessions */
EXTERN int c_session_timeout INIT(120);	/* session inactivity timeout */
/* globals */
EXTERN listener_t *g_listeners;	/* list of listeners */
EXTERN host_t  *g_hosts;	/* list of hosts */
EXTERN int g_n_sessions INIT(0);/* current # of active sessions */
EXTERN int g_run INIT(1);	/* set to 0 to exit cleanly */
#endif
