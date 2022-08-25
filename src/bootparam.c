/*
 * $Id: bootparam.c,v 1.5 2002/07/22 13:05:18 pajarola Exp $
 */

#include "bootd.h"

#include "generated_bootparam_prot.h"

void
bootparam_service_init()
{
	service_t      *s;

	s = service_new();
	s->name = "bootparam";
	s->id = SERVICE_BOOTPARM;
	s->d.rpc_udp.prognum = BOOTPARAMPROG;
	s->d.rpc_udp.versnum = BOOTPARAMVERS;
	service_add(s);
}

bp_whoami_res *
bootparamproc_whoami_1_svc(bp_whoami_arg arg1, struct svc_req *rqstp)
{
	listener_t *l;
	int fd;
	char            s_from[32];
	char		s_ip[32];
	host_t         *h;
	static bp_whoami_res result;
	ipaddr_t*pip;

	pip = (ipaddr_t*) & (arg1.client_address.bp_address_u);

	/* log request */
	str_sai(svc_getcaller(rqstp->rq_xprt), sizeof(s_from), s_from);
	str_ip(pip, sizeof(s_ip), s_ip);
	log_msg(LOG_INFO | LOG_BOOT, "bootparam whoami request from %s\n"
	    "  ipaddress %s", s_from, s_ip);

	/* check config */
	if ((h = util_ip2host(pip)) == NULL) {
		log_msg(LOG_INFO | LOG_BOOT,
		    "bootparam whoami request from host %s denied (client unknown)",
		    s_from);
		return NULL;
	}
	if ((h->service & SERVICE_BOOTPARM) == 0) {
		log_msg(LOG_INFO | LOG_BOOT,
		    "bootparam whoami request from host %s denied (bootparam not allowed for this host)",
		    h->name);
		return NULL;
	}
	fd = rqstp->rq_xprt->xp_port;
        if ((l = listener_search_by_fd(fd)) == NULL) {
                log_msg(LOG_WARN | LOG_DEV,
                    "bootparamprog: listener_search_by_fd() failed");
		return NULL;
        }

	/* fill in result */
	result.client_name = h->name;
	result.domain_name = "";/* XXX fix this? */
	result.router_address.address_type = IP_ADDR_TYPE;
	memcpy(&(result.router_address.bp_address_u), &(l->ip), sizeof(struct in_addr));

	/* log response */
	str_ip(&l->ip, sizeof(s_ip), s_ip);
	log_msg(LOG_INFO | LOG_BOOT, "bootparam whoami reply to %s\n"
	    "  client name %s, domain name %s, router address %s",
	    s_from,
	    result.client_name, result.domain_name, s_ip);
	return &result;
}

bp_getfile_res *
bootparamproc_getfile_1_svc(bp_getfile_arg arg1,  struct svc_req *rqstp)
{
	listener_t *l;
	int fd;
	char            s_from[32];
	char		s_ip[32];
	host_t         *h;
	file_t         *f;
	static bp_getfile_res result;

	/* log request */
	str_sai(svc_getcaller(rqstp->rq_xprt), sizeof(s_from), s_from);
	log_msg(LOG_INFO | LOG_BOOT, "bootparam getfile request from %s\n"
	    "  client_name %s, file_id %s",
	    s_from, arg1.client_name, arg1.file_id);

	/* check config */
	if ((h = util_name2host(arg1.client_name)) == NULL) {
		log_msg(LOG_INFO | LOG_BOOT,
		    "bootparam getfile request from host %s denied (client unknown)",
		    s_from);
		return NULL;
	}
	if ((h->service & SERVICE_BOOTPARM) == 0) {
		log_msg(LOG_INFO | LOG_BOOT,
		    "bootparam getfile request from host %s denied (bootparam not allowed for this host)",
		    h->name);
		return NULL;
	}
	/* fill in result */
	if ((f = util_name2file(h, SERVICE_BOOTPARM, arg1.file_id, 0)) == NULL) {
		return NULL;
	}
	fd = rqstp->rq_xprt->xp_port;
        if ((l = listener_search_by_fd(fd)) == NULL) {
                log_msg(LOG_WARN | LOG_DEV,
                    "bootparamprog: listener_search_by_fd() failed");
		return NULL;
        }
	result.server_name = f->server;	/* XXX fix this: server must be more
					 * clearly defined in config */
	result.server_path = f->path;
	result.server_address.address_type = IP_ADDR_TYPE;
	memcpy(&(result.server_address.bp_address_u), &(l->ip), sizeof(struct in_addr));	/* XXX fix this */

	/* log response */
	str_ip(&l->ip, sizeof(s_ip), s_ip);
	log_msg(LOG_INFO | LOG_BOOT, "bootparam getfile reply from %s to %s\n"
	    "  server name %s, server path %s, server address %s", s_from,
	    result.server_name, result.server_path, s_ip);
	return &result;
}
