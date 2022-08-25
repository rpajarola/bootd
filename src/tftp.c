/*
 * $Id: tftp.c,v 1.4 2002/07/22 13:05:24 pajarola Exp $
 */

#include "bootd.h"

#define BF_FD	h->session->fd[0]
#define TFTP_FD	h->session->fd[1]

void tftp_log_msgpacket(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, void *packet, int pktlen);

void
tftp_handler(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, void *packet, int pktlen)
{
	struct tftp_header *req, *repl;
	struct tftp_header_s *reqs, *repls;
	char            buf[4096];
	file_t         *f;
	host_t         *h;
	listener_t     *nl;
	int             npktlen;
	int             sid, size;
	struct sockaddr_in sai_from, sai_to;
	char            desc[128];

	/* log_msg all packets */
	tftp_log_msgpacket(l, from, to, packet, pktlen);

	/* default to send it back to where it came from */
	memcpy(&sai_to, from, sizeof(struct sockaddr_in));
	memcpy(&sai_from, to, sizeof(struct sockaddr_in));

	req = (struct tftp_header *) packet;
	reqs = (struct tftp_header_s *) packet;
	repl = (struct tftp_header *) buf;
	repls = (struct tftp_header_s *) buf;

	/* check config */
	if ((h = util_ip2host((ipaddr_t*)&(from->sin_addr))) == NULL) {
		log_msg(LOG_INFO | LOG_BOOT,
		    "tftp request from host %s denied (client unknown)",
		    inet_ntoa(from->sin_addr));
		tftp_send_error(l, from, to, TFTP_E_ACCESS, "access denied, client unknown");
		return;
	}
	if ((h->service & SERVICE_TFTP) == 0) {
		log_msg(LOG_INFO | LOG_BOOT,
		    "tftp request from host %s denied (tftp not allowed for this host)",
		    h->name);
		tftp_send_error(l, from, to, TFTP_E_ACCESS, "access denied, tftp not allowed for this host");
		return;
	}
	sid = ntohs(from->sin_port);

	switch (ntohs(req->op)) {
	case TFTPOP_RRQ:
		/* read request */
		if ((f = util_name2file(h, SERVICE_TFTP,
			   tftp_get_filename(packet, pktlen), 0)) == NULL) {
			/* file does not exist */
			tftp_send_error(l, from, to, TFTP_E_NOENT, "file not configured");
			return;
		}
		host_session_delete(h, "new request");
		/* new request */

		/* take random port */
		sai_from.sin_port = 0;

		snprintf(desc, sizeof(desc), "tftp data %s", h->name);
		nl = listener_new_udp(&(sai_from), &(sai_to), (listener_service) tftp_handler, desc);
		listener_add(nl);
		if (host_session_add(h, SERVICE_TFTP, sid) == 0) {
			/* server too busy */
			tftp_send_error(l, from, to, TFTP_E_UNDEF, "server busy");
			return;
		} else if ((BF_FD = open(f->path, O_RDONLY)) == -1) {
			log_msg(LOG_WARN | LOG_BOOT, "open(%s, O_RDONLY): %s",
			    f->path, strerror(errno));
			BF_FD = 0;
			switch (errno) {
			case ENOENT:
				/* no such file or directory */
				tftp_send_error(l, from, to, TFTP_E_NOENT, strerror(errno));
				return;
				break;
			case EACCES:
			case EPERM:
				/* permission denied */
				tftp_send_error(l, from, to, TFTP_E_ACCESS, strerror(errno));
				return;
				break;
			default:
				/* anything else */
				tftp_send_error(l, from, to, TFTP_E_UNDEF, strerror(errno));
				return;
				break;
			}
		}
		log_msg(LOG_INFO | LOG_BOOT, "sending file %s", f->path);

		/* session created, file opened */
		h->session->sid = sid;
		h->session->l = nl;
		if (h->session->l == NULL) {
			tftp_send_error(l, from, to, TFTP_E_UNDEF, strerror(errno));
			return;
		}
		repls->op = htons(TFTPOP_DATA);
		repls->n = htons(1);

		/* send first block */
		size = util_read(BF_FD, repls->data, sizeof(repls->data), 0);
		if (size < 0) {
			tftp_send_error(l, from, to, TFTP_E_UNDEF, strerror(errno));
		}
		npktlen = 4 + size;
		if (nl->send(nl, to, from, repl, npktlen) != (npktlen)) {
			log_msg(LOG_WARN | LOG_BOOT, "error sending tftp reply");
			return;
		}
		tftp_log_msgpacket(nl, from, to, repl, npktlen);
		return;
		break;
	case TFTPOP_WRQ:
		/* writing files is not (yet) supported */
		tftp_send_error(l, from, to, TFTP_E_UNDEF, "writing files not supported");
		break;
	case TFTPOP_DATA:
		/* writing files not (yet) supported */
		tftp_send_error(l, from, to, TFTP_E_UNDEF, "writing files not supported");
		break;
	case TFTPOP_ACK:
		if (host_session_check(h, SERVICE_TFTP, sid, "tftp ack") != E_NOERROR) {
			if (ntohs(reqs->n) == 1) {
				/*
				 * this is (probably) an ack for an error on
				 * connect, ignore
				 */
				return;
			}
			tftp_send_error(l, from, to, TFTP_E_BADSID, "no session");
			return;
		} else {
			/* session ok, send next block */
			repls->op = htons(TFTPOP_DATA);
			repls->n = htons(ntohs(reqs->n) + 1);
			/* send first block */
			size = util_read(BF_FD, repls->data, sizeof(repls->data),
				      ntohs(reqs->n) * sizeof(repls->data));
			if (size < 0) {
				tftp_send_error(l, from, to, TFTP_E_UNDEF, strerror(errno));
			}
			npktlen = 4 + size;
			if (l->send(l, to, from, repl, npktlen) != npktlen) {
				log_msg(LOG_WARN | LOG_BOOT, "error sending tftp reply");
			}
			tftp_log_msgpacket(l, to, from, repl, npktlen);
			return;
			break;
		}
		break;
	case TFTPOP_ERROR:
		break;
	default:
		/* illegal tftp opcode */
		log_msg(LOG_INFO | LOG_BOOT,
		    "tftp request from host %s denied (unknown opcode %d)",
		    h->name, ntohs(req->op));
		return;
	}
}

void
tftp_log_msgpacket(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, void *packet, int pktlen)
{
	struct tftp_header *tftp;
	struct tftp_header_s *tftp_s;
	char           *s_op, *s_filename, *s_mode, *s_error;
	char            text[128];
	int             level;

	tftp = (struct tftp_header *) packet;
	tftp_s = (struct tftp_header_s *) packet;

	level = LOG_INFO;
	switch (ntohs(tftp->op)) {
	case TFTPOP_RRQ:
		s_op = "RRQ";
		s_filename = tftp_get_filename(packet, pktlen);
		s_mode = tftp_get_mode(packet, pktlen);
		if (s_filename == NULL) {
			s_filename = "null";
		}
		if (s_mode == NULL) {
			s_mode = "null";
		}
		snprintf(text, sizeof(text), "filename %s, mode %s", s_filename, s_mode);
		text[sizeof(text)-1] = 0;
		break;
	case TFTPOP_WRQ:
		s_op = "WRQ";
		s_filename = tftp_get_filename(packet, pktlen);
		s_mode = tftp_get_mode(packet, pktlen);
		if (s_filename == NULL) {
			s_filename = "null";
		}
		if (s_mode == NULL) {
			s_mode = "null";
		}
		snprintf(text, sizeof(text), "filename %s, mode %s", s_filename, s_mode);
		text[sizeof(text)-1] = 0;
		break;
	case TFTPOP_DATA:
		s_op = "DATA";
		snprintf(text, sizeof(text), "block %d, datalen %d", ntohs(tftp_s->n), pktlen - 2);
		text[sizeof(text)-1] = 0;
		if (ntohs(tftp_s->n) != 1) {
			level = LOG_DEBUG;
		}
		break;
	case TFTPOP_ACK:
		s_op = "ACK";
		snprintf(text, sizeof(text), "block %d", ntohs(tftp_s->n));
		text[sizeof(text)-1] = 0;
		if (ntohs(tftp_s->n) != 1) {
			level = LOG_DEBUG;
		}
		break;
	case TFTPOP_ERROR:
		s_op = "ERROR";
		s_error = tftp_get_errmsg(packet, pktlen);
		if (s_error == NULL) {
			s_error = "null";
		}
		snprintf(text, sizeof(text), "%s (%d)", s_error, ntohs(tftp_s->n));
		text[sizeof(text)-1] = 0;
		break;
	default:
		s_op = "unknown";
		snprintf(text, sizeof(text), "datalen %d", pktlen);
		text[sizeof(text)-1] = 0;
		break;
	}
	log_msg(level | LOG_BOOT,
	    "tftp %s (0x%x)\n"
	    "  from %s:%d to %s:%d\n"
	    "  %s",
	    s_op, ntohs(tftp->op), inet_ntoa(from->sin_addr), ntohs(from->sin_port),
	    inet_ntoa(to->sin_addr), ntohs(to->sin_port), text);
}

char           *
tftp_get_filename(void *packet, int pktlen)
{
	struct tftp_header *tftp;
	static char     buf[512];
	int             i;

	tftp = (struct tftp_header *) packet;

	switch (ntohs(tftp->op)) {
	case TFTPOP_RRQ:
	case TFTPOP_WRQ:
		for (i = 0; i < pktlen - 2; i++) {
			buf[i] = tftp->data[i];
		}
		buf[i] = 0;
		return buf;
		break;
	}
	return NULL;
}

char           *
tftp_get_mode(void *packet, int pktlen)
{
	struct tftp_header *tftp;
	static char     buf[512];
	int             i, j;

	tftp = (struct tftp_header *) packet;

	switch (ntohs(tftp->op)) {
	case TFTPOP_RRQ:
	case TFTPOP_WRQ:
		for (i = 0, j = -1; i < pktlen - 2; i++) {
			if (tftp->data[i] == 0) {
				if (j == -1) {
					j = 0;
				} else {
					buf[j] = 0;
					return buf;
				}
			} else {
				if (j != -1) {
					buf[j++] = tftp->data[i];
				}
			}
		}
		if (j == -1) {
			return NULL;
		} else {
			buf[j] = 0;
			return buf;
		}
		break;
	default:
		return NULL;
	}
}

char           *
tftp_get_errmsg(void *packet, int pktlen)
{
	struct tftp_header *tftp;
	static char     buf[512];
	int             i;
	tftp = (struct tftp_header *) packet;
	switch (ntohs(tftp->op)) {
	case TFTPOP_ERROR:
		for (i = 0; i < pktlen - 4; i++) {
			buf[i] = tftp->data[i + 2];
		}
		buf[i] = 0;
		return buf;
		break;
	}
	return NULL;
}

void
tftp_send_error(listener_t * l, struct sockaddr_in * from, struct sockaddr_in * to, int err, char *msg)
{
	struct tftp_header_s repl;
	int             i;

	repl.op = htons(TFTPOP_ERROR);
	repl.n = htons(err);
	if (msg != NULL) {
		i = strlen(msg);
		strncpy(repl.data, msg, sizeof(repl.data));
	} else {
		i = 0;
		repl.data[0] = 0;
	}
	tftp_log_msgpacket(l, to, from, &repl, 5 + i);
	if (l->send(l, to, from, &repl, 5 + i) != (5 + i)) {
		log_msg(LOG_WARN | LOG_BOOT, "error sending tftp reply");
	}
}
