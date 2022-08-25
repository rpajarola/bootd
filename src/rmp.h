/*
 * $Id: rmp.h,v 1.5 2002/07/22 13:05:22 pajarola Exp $
 */

#ifndef _RMP_H
#define _RMP_H

/*
 * HP proprietary Remote Maintenance Protocol
 */

#define RMP_MAX_PACKET	1514
#define RMP_MIN_PACKET	60

static          const etheraddr_t rmp_addr = {{0x9, 0x0, 0x9, 0x0, 0x0, 0x4}};

#define SAP_HP	0xF8
#define CNTL_HP 0x300
#define DXSAP_HP 0x608
#define SXSAP_HP 0x609

#define RMPOP_BOOT_REQ	1	/* boot request */
#define RMPOP_BOOT_REPL	129	/* bot reply */
#define RMPOP_READ_REQ	2	/* read request */
#define RMPOP_READ_REPL	130	/* read reply */
#define RMPOP_BOOT_DONE	3	/* boot done */

#define RMP_VERSION	2	/* protocol version */
#define RMP_PROBESID	0xffff	/* session ID of probes */
#define RMP_HOSTLEN	13	/* max length of server name */
#define RMP_MACHLEN	20	/* length of machine type field */

#define RMP_E_OKAY      0
#define RMP_E_EOF       2	/* read reply: returned end of file */
#define RMP_E_ABORT     3	/* abort operation */
#define RMP_E_BUSY      4	/* boot reply: server busy */
#define RMP_E_TIMEOUT   5	/* lengthen time out (not implemented) */
#define RMP_E_NOFILE    16	/* boot reply: file does not exist */
#define RMP_E_OPENFILE  17	/* boot reply: file open failed */
#define RMP_E_NODFLT    18	/* boot reply: default file does not exist */
#define RMP_E_OPENDFLT  19	/* boot reply: default file open failed */
#define RMP_E_BADSID    25	/* read reply: bad session ID */
#define RMP_E_BADPACKET 27	/* Bad packet detected */

#define RMP_H 14
#define RMP_BOOT_REQ (21+RMP_MACHLEN)
#define RMP_BOOT_REPL 21
#define RMP_READ_REQ 20
#define RMP_READ_REPL 18
#define RMP_BOOT_REQ_DATALEN (RMP_MAX_PACKET - RMP_H - RMP_BOOT_REQ)
#define RMP_BOOT_REPL_DATALEN (RMP_MAX_PACKET - RMP_H - RMP_BOOT_REPL)
#define RMP_READ_REQ_DATALEN (RMP_MAX_PACKET - RMP_H - RMP_READ_REQ)
#define RMP_READ_REPL_DATALEN (RMP_MAX_PACKET - RMP_H - RMP_READ_REPL)

struct rmp_header {

	/* 802.3 Ethernet header */
	etheraddr_t     daddr;
	etheraddr_t     saddr;
	u_int16_t       len;

	/* HP 802.2 LLC with extensions */
	u_int8_t        dsap;	/* 802.2 DSAP */
	u_int8_t        ssap;	/* 802.2 SSAP */
	u_int16_t       cntl;	/* 802.2 control field *//* 18 */
	u_int16_t       pad;	/* padding (must be zero) */
	u_int16_t       dxsap;	/* HP extended DSAP */
	u_int16_t       sxsap;	/* HP extended SSAP */
	u_int8_t        type;	/* packet type */
	u_int8_t        retcode;/* return code */
	u_int16_t       seqh;	/* sequence number or offset */
	u_int16_t       seql;
	u_int16_t       session;/* session id *//* 14 */
};

/* boot request */
struct rmp_boot_req {
	u_int16_t       version;/* protocol version */
	char            machtype[RMP_MACHLEN];	/* machine type */
	u_int8_t        flnm_size;	/* length of flnm */
	char            flnm[RMP_BOOT_REQ_DATALEN];	/* name of file */
};

/* boot reply */
struct rmp_boot_repl {
	u_int16_t       version;/* protocol version */
	u_int8_t        flnm_size;	/* length of flnm */
	char            flnm[RMP_BOOT_REPL_DATALEN];	/* name of file */
};

/* read request */
struct rmp_read_req {
	u_int16_t       size;	/* max no of data bytes to send */
};

/* read reply */
struct rmp_read_repl {
	char            data[RMP_READ_REPL_DATALEN];
};

void            rmp_handler(listener_t * l, void *from, void *to, void *packet, int pktlen);
void            rmp_logpacket(listener_t * l, void *from, void *to, void *packet, int pktlen);
char           *rmp_machtype(char machtype[RMP_MACHLEN]);
char           *rmp_filename(u_int8_t flnm_size, char *flnm);

#endif
