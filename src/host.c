/*
 * $Id: host.c,v 1.3 2002/07/22 13:03:14 pajarola Exp $
 */

#include "bootd.h"

host_t         *
host_new()
{
	host_t         *h;
	h = malloc(sizeof(host_t));
	memset(h, 0, sizeof(host_t));
	h->service = 0;
	return h;
}

host_t         *
host_clone(host_t * src, char *name, etheraddr_t * ether, ipaddr_t * ip)
{
	host_t         *h;

	h = malloc(sizeof(host_t));
	memcpy(h, src, sizeof(host_t));
	h->name = name;
	if (ether != NULL) {
		memcpy(&(h->eth), ether, sizeof(etheraddr_t));
	} else {
		memset(&(h->eth), 0, sizeof(etheraddr_t));
	}
	if (ip != NULL) {
		memcpy(&(h->ip), ip, sizeof(ipaddr_t));
	} else {
		memset(&(h->ip), 0, sizeof(ipaddr_t));
	}
	host_add(h);
	return (h);
}

void
host_add(host_t * h)
{
	host_t         *n;
	file_t         *f;
	int             has_name, has_ip, has_ether;
	char            service_str[512];
	char            ether_str[32];
	char            ip_str[32];
	char            files[2048];
	int             i;
	/* sanity check h */
	if (h == NULL) {
		log_msg(LOG_WARN | LOG_CONF, "trying to add null host");
		return;
	}
	has_name = h->name != NULL;
	has_ip = util_ip_valid(&(h->ip));
	has_ether = util_ether_valid(&(h->eth));

	if (!has_name) {
		if (has_ip) {
			h->name = strdup(util_ip2name(&(h->ip)));
		} else if (has_ether) {
			h->name = strdup(util_ether2name(&(h->eth)));
		} else {
			log_msg(LOG_WARN | LOG_CONF, "host has neither name nor ip nor ethernet address");
			return;
		}
	}
	if (!has_ip) {
		if (!util_name2ip(h->name, &(h->ip))) {
			log_msg(LOG_WARN | LOG_CONF, "unable to determine ip address for host %s", h->name);
			return;
		}
	}
	if (!has_ether) {
		if (!util_name2ether(h->name, &(h->eth))) {
			log_msg(LOG_WARN | LOG_CONF, "unable to determine ethernet address for host %s",
			    h->name);
			return;
		}
	}
	if (h->service == 0) {
		h->service = SERVICE_ALL;
	}
	files[0] = 0;
	for (f = h->files; f != NULL; f = f->next) {
		if (f->service == 0) {
			f->service = h->service;
		}
		strcat(files, "\n  file ");
		strcat(files, f->path);
		strcat(files, "\n    name ");
		strcat(files, f->name);
		for (i = 0; (i < FILE_ALIASES_MAX) && (f->aliases[i] != NULL); i++) {
			strcat(files, " | ");
			strcat(files, f->aliases[i]);
		}
		if (f->server != NULL) {
			strcat(files, "\n    server ");
			strcat(files, f->server);
		}
		str_service(f->service, sizeof(service_str), service_str);
		strcat(files, "\n    service ");
		strcat(files, service_str);
	}
	str_service(h->service, sizeof(service_str), service_str);
	str_eth(&(h->eth), sizeof(ether_str), ether_str);
	str_ip(&(h->ip), sizeof(ip_str), ip_str);
	log_msg(LOG_DEBUG | LOG_CONF, "host %s\n  ether: %s\n  ip: %s\n  service: %s%s",
	    h->name, ether_str, ip_str, service_str, files);
	if (g_hosts != NULL) {
		for (n = g_hosts; n; n = n->next) {
			if (strcasecmp(h->name, n->name) == 0) {
				log_msg(LOG_WARN | LOG_CONF, "duplicate entry for host %s (ignored)", h->name);
				return;
			}
			if (n->next == NULL) {
				n->next = h;
				return;
			}
		}
	} else {
		g_hosts = h;
	}
}

file_t         *
host_file_new(host_t * h)
{
	file_t         *f;
	f = malloc(sizeof(file_t));
	memset(f, 0, sizeof(file_t));
	f->service = 0;
	return f;
}

void
host_file_add(host_t * h, file_t * f)
{
	file_t         *n;
	int             has_name, has_path, n_alias;
	char           *ch;
	/* sanity check fi */
	if (f == NULL) {
		log_msg(LOG_WARN | LOG_CONF, "trying to add null file");
		return;
	}
	has_name = f->name != NULL;
	has_path = !str_is_equal(f->path, "{");
	for (n_alias = 0; (f->aliases[n_alias] != NULL) && (n_alias < FILE_ALIASES_MAX); n_alias++) {
	}
	if (!has_name) {
		if (n_alias) {
			f->name = strdup(f->aliases[0]);
		} else if (has_path) {
			if ((ch = strrchr(f->path, '/')) != NULL) {
				ch++;
				f->name = strdup(ch);
			} else {
				f->name = strdup(f->path);
			}
		} else {
			log_msg(LOG_WARN | LOG_CONF, "file has no name");
			return;
		}
	}
	if (!has_path) {
		f->path = strdup(f->name);
	}
	if (h->files != NULL) {
		for (n = h->files; n; n = n->next) {
			if (strcasecmp(f->name, n->name) == 0) {
				log_msg(LOG_WARN | LOG_CONF, "duplicate entry for file %s (ignored)"
				    ,
				    f->name);
				return;
			}
			if (n->next == NULL) {
				n->next = f;
				return;
			}
		}
	} else {
		h->files = f;
	}
}

void
host_session_delete(host_t * h, char *msg)
{
	int             i;
	char            service_str[512];
	if ((h == NULL) || (h->session == NULL)) {
		return;
	}
	for (i = 0; i < SESSION_FD_MAX; i++) {
		if (h->session->fd[i]) {
			listener_delete(listener_search_by_fd(h->session->fd[i]));
		}
	}
	str_service(h->session->service, sizeof(service_str), service_str);
	log_msg(LOG_INFO | LOG_BOOT, "%s session for host %s deleted (%s)",
	    service_str, h->name, msg);
	free(h->session);
	h->session = NULL;
	g_n_sessions--;
}


void
host_session_timeout(host_t * h)
{
	if ((h == NULL) || (h->session == NULL)) {
		return;
	}
	if ((h->session->atime + c_session_timeout) < time(NULL)) {
		/* session timed out */
		host_session_delete(h, "timeout");
	}
}

int
host_session_add(host_t * h, int service, int sid)
{
	char            service_str[512];
	/* clear any active session for this host */
	util_host_session_timeout_all();
	host_session_delete(h, "new request");

	if (g_n_sessions < c_session_max) {
		g_n_sessions++;
		h->session = malloc(sizeof(session_t));
		memset(h->session, 0, sizeof(session_t));
		h->session->service = service;
		h->session->atime = time(NULL);
		h->session->sid = sid;
		str_service(service, sizeof(service_str), service_str);
		log_msg(LOG_INFO | LOG_BOOT, "%s session for host %s created",
		    service_str, h->name);
		return 1;
	} else {
		str_service(service, sizeof(service_str), service_str);
		log_msg(LOG_INFO | LOG_BOOT, "%s session for host %s not created (too many active sessions)",
		    service_str, h->name);
		return 0;
	}
}

int
host_session_check(host_t * h, int service, int sid, char *desc)
{
	if ((h->session == NULL) || (h->session->service != service)) {
		log_msg(LOG_WARN | LOG_BOOT, "%s from %s for nonexistant session", desc, h->name);
		return E_NOSESSION;
	}
	if (h->session->sid != sid) {
		log_msg(LOG_WARN | LOG_BOOT, "%s from %s with bad session id", desc, h->name);
		return E_BADSID;
	}
	h->session->atime = time(NULL);
	return 0;
}
