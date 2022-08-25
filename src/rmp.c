/*
 * $Id: rmp.c,v 1.4 2002/07/22 13:05:22 pajarola Exp $
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>

#include "bootd.h"

#define boot_req ((struct rmp_boot_req *)((char *)req + sizeof(struct rmp_header)))
#define boot_repl ((struct rmp_boot_repl*)((char *)repl + sizeof(struct rmp_header)))
#define read_req ((struct rmp_read_req*)((char *)req + sizeof(struct rmp_header)))
#define read_repl ((struct rmp_read_repl*)((char *)repl + sizeof(struct rmp_header)))
#define BF_FD	h->session->fd[0]

void
rmp_handler(listener_t * l, void *from, void *to, void *packet, int pktlen)
{
	struct rmp_header *req, *repl;
	host_t         *h;
	file_t         *f;
	char            buf[4096];
	char           *filename;
	int             seq, size;
	int             npktlen;
	static int      rmp_sid = 1;

	if (pktlen < sizeof(struct rmp_header)) {
		/* too short! */
		return;
	}
	req = (struct rmp_header *) packet;
	repl = (struct rmp_header *) packet;

	seq = ntohs(req->seqh) << 16 | ntohs(req->seql);

	if (((memcmp(&(req->daddr), rmp_addr.octet, sizeof(etheraddr_t)) != 0) &&
	 (memcmp(&(req->daddr), l->eth.octet, sizeof(etheraddr_t)) != 0)) ||
	    (req->dsap != SAP_HP) ||
	    (ntohs(req->cntl) != CNTL_HP) ||
	    (ntohs(req->dxsap) != DXSAP_HP)) {
		/* not a an rmp packet or not for us */
		return;
	}
	rmp_logpacket(l, NULL, NULL, packet, pktlen);

	/* check config */
	if ((h = util_ether2host(&(req->saddr))) == NULL) {
		if ((req->type == RMPOP_BOOT_REQ) &&
		    ((h = util_name2host(rmp_machtype(boot_req->machtype))) != NULL)) {
			h = host_clone(h, NULL, &(req->saddr), NULL);
		} else {
			log_msg(LOG_INFO | LOG_BOOT,
			 "rmp request from host %s denied (client unknown)",
			  ether_ntoa((struct ether_addr *) & (req->saddr)));
			return;
		}
	}
	if ((h->service & SERVICE_RMP) == 0) {
		log_msg(LOG_INFO | LOG_BOOT,
		    "rmp request from host %s denied (rmp not allowed for this host)",
		    h->name);
		return;
	}
	/* set up reply */
	repl = (struct rmp_header *) buf;
	memcpy(repl->daddr.octet, req->saddr.octet, sizeof(struct ether_addr));
	memcpy(repl->saddr.octet, l->eth.octet, sizeof(struct ether_addr));
	repl->dsap = SAP_HP;
	repl->ssap = SAP_HP;
	repl->cntl = htons(CNTL_HP);
	repl->pad = 0;
	repl->dxsap = htons(SXSAP_HP);
	repl->sxsap = htons(DXSAP_HP);
	/* set type in handler */
	repl->retcode = RMP_E_OKAY;
	repl->seqh = req->seqh;
	repl->seql = req->seql;
	repl->session = req->session;

	/* must set at least repl->len, repl->type, */

	switch (req->type) {
	case RMPOP_BOOT_REQ:
		if (htons(boot_req->version) != RMP_VERSION) {
			log_msg(LOG_INFO | LOG_BOOT,
			    "rmp request from host %s denied (version %d not supported)",
			    ether_ntoa((struct ether_addr *) & (req->saddr)), ntohs(boot_req->version));
			return;
		}
		repl->type = RMPOP_BOOT_REPL;
		boot_repl->version = htons(RMP_VERSION);

		/* probe packet? */
		if (ntohs(req->session) == RMP_PROBESID) {
			repl->session = 0;
			if (seq == 0) {
				/* send server id */
				filename = l->name;
			} else {
				/* send file name */
				f = util_name2file(h, SERVICE_RMP, NULL, seq - 1);
				if (f) {
					filename = f->name;
				} else {
					filename = "";
					repl->retcode = RMP_E_NODFLT;
				}
			}
			boot_repl->flnm_size = strlen(filename);
			strcpy(boot_repl->flnm, filename);
			npktlen = RMP_BOOT_REPL + boot_repl->flnm_size;
			repl->len = htons(npktlen);
			rmp_logpacket(l, NULL, NULL, buf, npktlen);
			if (l->send(l, NULL, NULL, buf, npktlen) != npktlen) {
				log_msg(LOG_WARN | LOG_BOOT, "error sending rmp reply");
			}
		} else {
			/* normal boot request */
			filename = rmp_filename(boot_req->flnm_size, boot_req->flnm);
			f = util_name2file(h, SERVICE_RMP, filename, 0);
			boot_repl->flnm_size = 0;
			if (f == NULL) {
				/* file does not exist */
				repl->retcode = RMP_E_NOFILE;
			} else if (host_session_add(h, SERVICE_RMP, ++rmp_sid) == 0) {
				/* server too busy */
				repl->retcode = RMP_E_BUSY;
			} else {
				if ((BF_FD = open(f->path, O_RDONLY)) == -1) {
					log_msg(LOG_WARN | LOG_BOOT, "open(%s, O_RDONLY): %s", f->path, strerror(errno));
					BF_FD = 0;
					if (errno == ENOENT) {
						/* no such file or directory */
						repl->retcode = RMP_E_NOFILE;
					} else if ((errno == EMFILE) || (errno == ENFILE)) {
						/*
						 * file table overflow / too
						 * many open files
						 */
						repl->retcode = RMP_E_BUSY;
					} else {
						/* anything else */
						repl->retcode = RMP_E_OPENFILE;
					}
				} else {
					/* session created, file opened */
					repl->session = htons(h->session->sid);
					memcpy(boot_repl->flnm, boot_req->flnm, boot_req->flnm_size);
					boot_repl->flnm_size = boot_req->flnm_size;
				}
			}
			repl->len = htons(RMP_BOOT_REPL + boot_req->flnm_size);
			log_msg(LOG_INFO | LOG_BOOT, "sending file %s", f->path);
		}
		break;
	case RMPOP_READ_REQ:
		repl->type = RMPOP_READ_REPL;
		size = 0;
		switch (host_session_check(h, SERVICE_RMP, ntohs(req->session), "rmp read request")) {
		case E_NOSESSION:
			repl->retcode = RMP_E_ABORT;
			break;
		case E_BADSID:
			repl->retcode = RMP_E_BADSID;
			break;
		case E_NOERROR:
			/* session ok, send file */

			/* silently return less than requested if necessary */
			size = min(ntohs(read_req->size), RMP_READ_REPL_DATALEN);
			size = util_read(BF_FD, read_repl->data, size, (off_t) seq);
			if (size < 0) {
				repl->retcode = RMP_E_ABORT;
			} else if (size == 0) {
				repl->retcode = RMP_E_EOF;
			}
			break;
		default:
			repl->retcode = RMP_E_ABORT;
			break;
		}
		npktlen = RMP_READ_REPL + size;
		repl->len = htons(npktlen);
		rmp_logpacket(l, NULL, NULL, buf, npktlen);
		if (l->send(l, NULL, NULL, buf, npktlen) != npktlen) {
			log_msg(LOG_WARN | LOG_BOOT, "error sending rmp reply");
		}
		break;
	case RMPOP_BOOT_DONE:
		host_session_delete(h, "boot done");
		break;
	default:
		/* ignore */
		return;
	}
	return;
}

char           *
rmp_machtype(char machtype[RMP_MACHLEN])
{
	static char     result[RMP_MACHLEN + 1];
	int             i;

	for (i = 0; ((i < RMP_MACHLEN) && (machtype[i] != 0x20) && (machtype[i] != 0)); i++) {
		result[i] = machtype[i];
	}
	result[i] = 0;
	return result;
}

char           *
rmp_filename(u_int8_t flnm_size, char *flnm)
{
	static char     result[1024];
	int             i;

	for (i = 0; i < flnm_size; i++) {
		result[i] = flnm[i];
	}
	result[i] = 0;
	return result;
}

void
rmp_logpacket(listener_t * l, void *from, void *to, void *packet, int pktlen)
{
	struct rmp_header *req, *repl;
	char            s_eth[32], d_eth[32];
	int             seq;
	char           *spec;
	char           *retcode;
	char           *file;

	req = (struct rmp_header *) packet;
	repl = (struct rmp_header *) packet;

	strcpy(s_eth, ether_ntoa((struct ether_addr *) & (req->saddr)));
	strcpy(d_eth, ether_ntoa((struct ether_addr *) & (req->daddr)));
	seq = ntohs(req->seqh) << 16 | ntohs(req->seql);

	switch (req->retcode) {
	case RMP_E_OKAY:
		retcode = "OK ";
		break;
	case RMP_E_EOF:
		retcode = "EOF ";
		break;
	case RMP_E_ABORT:
		retcode = "ABORT ";
		break;
	case RMP_E_BUSY:
		retcode = "BUSY ";
		break;
	case RMP_E_TIMEOUT:
		retcode = "LENGTHEN TIMEOUT ";
		break;
	case RMP_E_NOFILE:
		retcode = "NO FILE ";
		break;
	case RMP_E_OPENFILE:
		retcode = "OPEN FAILED ";
		break;
	case RMP_E_NODFLT:
		retcode = "NO DEFAULT FILE ";
		break;
	case RMP_E_OPENDFLT:
		retcode = "DEFAULT OPEN FAILED ";
		break;
	case RMP_E_BADSID:
		retcode = "BADSID ";
		break;
	case RMP_E_BADPACKET:
		retcode = "BADPACKET ";
		break;
	default:
		retcode = "";
	}

	switch (req->type) {
	case RMPOP_BOOT_REQ:
		if (ntohs(req->session) == RMP_PROBESID) {
			if (seq == 0) {
				spec = "boot request (send server id)";
			} else {
				spec = "boot request (send filename)";
			}
		} else {
			spec = "boot request";
		}
		log_msg(LOG_INFO | LOG_BOOT,
		    "rmp %s (0x%x) %d bytes\n  from %s to %s\n  retcode %s(%d), sequence %d, session %d\n"
		    "  version %d, machtype \"%s\", filename \"%s\"",
		    spec, req->type, ntohs(req->len), s_eth, d_eth, retcode, req->retcode, seq,
		    ntohs(req->session),
		 ntohs(boot_req->version), rmp_machtype(boot_req->machtype),
		    rmp_filename(boot_req->flnm_size, boot_req->flnm));
		break;

	case RMPOP_BOOT_REPL:
		if (ntohs(repl->session) == 0) {
			if (seq == 0) {
				spec = "boot reply (send server id)";
				file = "server";
			} else {
				spec = "boot reply (send filename)";
				file = "filename";
			}
		} else {
			spec = "boot reply";
			file = "filename";
		}
		log_msg(LOG_INFO | LOG_BOOT,
		    "rmp %s (0x%x) %d bytes\n  from %s to %s\n  retcode %s(%d), sequence %d, session %d\n"
		    "  version %d, %s \"%s\"",
		    spec, repl->type, ntohs(repl->len), s_eth, d_eth, retcode, repl->retcode, seq,
		    ntohs(repl->session),
		    ntohs(boot_repl->version),
		 file, rmp_filename(boot_repl->flnm_size, boot_repl->flnm));
		break;
	case RMPOP_READ_REQ:
		log_msg(seq ? LOG_DEBUG : LOG_INFO | LOG_BOOT,
		    "rmp read request (0x%x) %d bytes\n  from %s to %s\n  retcode %s(%d), sequence %d, session %d\n"
		    "  size %d",
		    req->type, ntohs(req->len), s_eth, d_eth, retcode, req->retcode, seq,
		    ntohs(req->session),
		    ntohs(read_req->size));
		break;
	case RMPOP_READ_REPL:
		log_msg(seq ? LOG_DEBUG : LOG_INFO | LOG_BOOT,
		    "rmp read reply (0x%x) %d bytes\nfrom %s to %s\n  retcode %s(%d), sequence %d, session %d\n",
		    repl->type, ntohs(req->len), s_eth, d_eth, retcode, repl->retcode, seq,
		    ntohs(repl->session));
		break;
	case RMPOP_BOOT_DONE:
		log_msg(LOG_INFO | LOG_BOOT,
		    "rmp boot done (0x%x) %d bytes\n  from %s to %s\n  retcode %s(%d), sequence %d, session %d",
		    req->type, ntohs(req->len), s_eth, d_eth, retcode, req->retcode, seq,
		    ntohs(req->session));
		break;
	default:
		log_msg(LOG_INFO | LOG_BOOT,
		    "rmp unknown request (0x%x) %d bytes\n  from %s to %s\n  retcode %s(%d), sequence %d, session %d",
		    req->type, ntohs(req->len), s_eth, d_eth, retcode, req->retcode, seq, ntohs(req->session));
		break;
	}
}
