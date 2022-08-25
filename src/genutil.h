/*
 * $Id: genutil.h,v 1.7 2002/07/22 13:07:28 pajarola Exp $
 */

#ifndef _GENUTIL_H
#define _GENUTIL_H

#ifdef GENUTIL

#include "config.h"

/*
 * do includes only if compiling genutil
 */

/* ANSI headers */
#ifdef HAVE_STDIO_H
#include <stdio.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
#include <stdarg.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif

/* POSIX.1 headers */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

/* network header files */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_IF_ETHER_H
#include <netinet/if_ether.h>
#endif
#ifdef HAVE_NETINET_ETHER_H
#include <netinet/ether.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include <pcap.h>

#endif

typedef struct {
	u_int8_t        octet[6];
}               etheraddr_t;

typedef struct {
	uint32_t        s_addr;
}               ipaddr_t;

/*
 * all functions are tolerant to NULL values
 */


/**
 *
 * string functions
 *
 */

/* return 1 if true, 0 if false or NULL */
int             str_is_equal(char *w1, char *w2);	/* 1=equal, 0=not equal */
int             str_is_equalnc(char *w1, char *w2);	/* 1=equal, 0=not equal
							 * (ignore case) */
/**
 * interpret string *w, put result in *dst return 1 if string is meaningfull
 * in this context, 0 if not no resolving is done on ethernet addresses/ip
 * addresses/services
 */
int             str_get_bool(char *w, int *dst);
int             str_get_int(char *w, int *dst);
int             str_get_loglevel(char *w, int *dst);
int             str_get_eth(char *w, etheraddr_t * dst);
int             str_get_ip(char *w, ipaddr_t * dst);
int             str_get_hostport(char *w, int lhost, char *dsthost, int lport, char *dstport);

/**
 * give string value of var.
 * undefined if dst is NULL (log warning)
 */
char           *str_bool(int b, int l, char *dst);	/* "true"/"false" */
char           *str_eth(etheraddr_t * eth, int l, char *dst);	/* "xx:xx:xx:xx:xx:xx" */
char           *str_ip(ipaddr_t * ip, int l, char *dst);	/* "xxx.xxx.xxx.xxx" */
char           *str_dlt(int dlt, int l, char *dst);	/* eg "Ethernet" */
char           *str_af(int af, int l, char *dst);	/* eg "inet" */
char           *str_sai(struct sockaddr_in * sai, int l, char *dst);	/* "xxx.xxx.xxx.xxx:xxxxx
									 * " */

char           *str_do_quote(char *w, int l, char *r);	/* quote string w, put
							 * result in r */
/**
 *
 * log functions
 *
 */

/* severities */
#define LOG_DEBUG	(0<<0)
#define LOG_INFO	(1<<0)
#define LOG_WARN	(2<<0)
#define LOG_ERR		(3<<0)
#define LOG_LEVEL	(3<<0)

/* categories */
#define LOG_CAT_SHIFT	(8)
#define LOG_CAT_MAX	(8)
#define LOG_MISC	(0<<LOG_CAT_SHIFT)
#define LOG_CONF	(1<<LOG_CAT_SHIFT)
#define LOG_DEV		(2<<LOG_CAT_SHIFT)
#define LOG_BOOT	(3<<LOG_CAT_SHIFT)
#define LOG_RES		(4<<LOG_CAT_SHIFT)
#define LOG_CAT		(7<<LOG_CAT_SHIFT)

/* modules */
#define LOG_MOD_SHIFT	(16)
#define LOG_MOD_MAX	(16)
#define LOG_MOD		(15<<LOG_MOD_SHIFT)

/*
 * log_init resets all loglevels to default, and sets the global loglevel and
 * logfile name to the value specified.
 * 
 * log_set_file changes the name of the logfile log_set_level changes the global
 * loglevel, or the per category/module loglevel (using eg
 * log_set_level(LOG_DEV|LOG_DEBUG).* set one category/module at a time
 * log_set_mod_name sets the name of a module (default: "")
 */
void            log_init(int minlevel, char *fileame);
void            log_set_file(char *fileame);
void            log_set_level(int minlevel);
void            log_set_mod_name(int mod, char *name);
void            log_msg(int what, char *fmt,...);
/**
 *
 * tokenize functions
 *
 */

/*
 * give possibility to override lookahead/bufsize
 */
#ifndef TOKENIZE_LOOKAHEAD
#define TOKENIZE_LOOKAHEAD 4
#endif
#ifndef TOKENIZE_BUFSIZE
#define TOKENIZE_BUFSIZE 4096
#endif

/*
 * single token
 */
typedef struct {
	int             x, y;
	char            b[TOKENIZE_BUFSIZE];
}               gen_token_t;
/*
 * tokenizer state
 */
typedef struct {
	char           *fname;	/* filename */
	int             fd;	/* file descriptor */
	int             s, e;
	int             x, y;
	char            b[TOKENIZE_BUFSIZE];
	int             ts, te;
	gen_token_t         t[TOKENIZE_LOOKAHEAD + 1];
}               gen_tokenize_t;
/*
 * most of these functions call log(LOG_ERR | LOG_CONF, ...) on error (ie, in
 * case of error, they exit the program)
 */

gen_tokenize_t     *gen_tokenize_new(char *filename);	/* allocate tokenizer */
void            gen_tokenize_close(gen_tokenize_t * tokenize);	/* close tokenizer */
int             gen_tokenize_next(gen_tokenize_t * tokenize, int l, char *w);	/* get next token,
									 * return 0 if eof  */
int             gen_tokenize_ahead(gen_tokenize_t * tokenize, int n, int l, char *w);	/* get nth token ahead,
										 * 0 on eof */
/*
 * return value if meaningfull in this context, gen_tokenize_error if not
 */
void            gen_tokenize_get(gen_tokenize_t * tokenize, int l, char *w);	/* get next token,
									 * gen_tokenize_error if eof */
void            gen_tokenize_expect(gen_tokenize_t * tokenize, char *w);	/* gen_tokenize_error if
									 * next token is not w */
void            gen_tokenize_error(gen_tokenize_t * tokenize, char *fmt,...);	/* print error with
									 * filename/line number,
									 * terminate */
void            gen_tokenize_warn(gen_tokenize_t * tokenize, char *fmt,...);	/* print error with
									 * filename/line number */
/**
 *
 * tokenize functions
 *
 */

/*
 * give possibility to override string_max/values_max
 */
#ifndef PARSE_STRING_MAX
#define PARSE_STRING_MAX 256
#endif
#ifndef PARSE_VALUES_MAX
#define PARSE_VALUES_MAX 32
#endif

typedef struct {
	char           *name;
	char           *values[PARSE_VALUES_MAX];
	int             values_next;	/* next free index in values_sidx */
	void           *next;
	void           *sub;
	int             processed;	/* != 0 -> entry has been processed */
	int             line;	/* line in parse file */
	char           *filename;	/* name of parse file */
}               parse_entry_t;

parse_entry_t  *parse_file(char *filename);	/* parse file */
void            parse_cleanup();/* deallocate structures */
parse_entry_t  *parse_entry_get(parse_entry_t * root, char *name);	/* get entry in current *
									 * context */
void            parse_error(parse_entry_t * e, char *fmt,...);	/* print error with
								 * filename/line number,
								 * terminate */
void            parse_warn(parse_entry_t * e, char *fmt,...);	/* print error with
								 * filename/line number */
#endif
