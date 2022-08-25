/*
 * $Id: tftp.h,v 1.3 2002/07/04 12:02:08 pajarola Exp $
 */

#ifndef _TFTP_H
#define _TFTP_H

/*
 * Trivial File Transfer Protocol Rev 2 as specified in RFC1350, RFC1785
 * (Option Negotiation), RFC2347 (Option Extension), RFC2348 (Blocksize
 * Option), RFC 2349 (Timeout Interval and Transfer Size Options) RFC906
 * (Bootstrap Loading using TFTP)
 */

#define TFTPOP_RRQ	01
#define TFTPOP_WRQ	02
#define TFTPOP_DATA	03
#define TFTPOP_ACK	04
#define TFTPOP_ERROR	05

#define TFTPMODE_NETASCII	1
#define TFTPMODE_OCTET		2
#define TFTPMODE_MAIL		3
#define TFTPMODE_UNSUPP		4

#define TFTP_E_UNDEF		0
#define TFTP_E_NOENT		1
#define TFTP_E_ACCESS		2
#define TFTP_E_NOSPC		3
#define TFTP_E_INVAL		4
#define TFTP_E_BADSID		5
#define TFTP_E_EXIST		6
#define TFTP_E_NOUSER		7

/* generic tftp header */
struct tftp_header {
	u_int16_t       op;
	char            data[514];
};

/* tftp header with u_int16_t after op */
struct tftp_header_s {
	u_int16_t       op;
	u_int16_t       n;
	char            data[512];
};

void            tftp_handler(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, void *packet, int pktlen);
void            tftp_logpacket(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, void *packet, int pktlen);
void            tftp_send_error(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, int err, char *msg);
char           *tftp_get_filename(void *packet, int pktlen);
char           *tftp_get_mode(void *packet, int pktlen);
char           *tftp_get_errmsg(void *packet, int pktlen);

#endif
