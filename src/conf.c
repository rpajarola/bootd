/*
 * $Id: conf.c,v 1.3 2002/07/22 13:03:14 pajarola Exp $
 */

#include "bootd.h"
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

/*
 * usefull functions
 */
static parse_entry_t *i_get_entry_single_value(parse_entry_t * root, char *name, int single);
static void     i_assure_no_entry(parse_entry_t * root, char *name);
/*
 * helpers for often used types return 0 if not found, fatal error if type
 * does not match
 */

static int      i_get_string(parse_entry_t * root, char *name, char **dst, int single);
static int      i_get_bool(parse_entry_t * root, char *name, int *dst, int single);
static int      i_get_int(parse_entry_t * root, char *name, int *dst, int single);
static int      i_get_ether(parse_entry_t * root, char *name, etheraddr_t * dst, int single);
static int      i_get_ip(parse_entry_t * root, char *name, ipaddr_t * dst, int single);
static int      i_get_service(parse_entry_t * root, char *name, int *dst);
static int      i_get_loglevel(parse_entry_t * root, char *name, int *dst, int single);
/*
 * evaluate configured options
 */
static void     i_eval_basic(parse_entry_t * root);
static void     i_eval_log_msg(parse_entry_t * root);
static void     i_eval_hosts(parse_entry_t * root);
static void     i_eval_host_files(parse_entry_t * root, host_t * h);
static void     i_eval_services(parse_entry_t * root);

void
usage()
{
	printf("usage: bootd [-c config] [-l minlevel] [-f logfile]\n");
	printf("       bootd -h\n");
	exit(1);
}

void
conf_args(int argc, char *argv[])
{
	int             ch;
	int             loglevel = -1;
	char           *logfile = NULL;
	while ((ch = getopt(argc, argv, "c:hl:f:")) != -1) {
		switch (ch) {
		case 'c':
			/* config file */
			c_file = optarg;
			break;
		case 'h':
			usage();
			/* notreached */
			break;
		case 'l':
			if (!str_get_loglevel(optarg, &loglevel)) {
				printf("log level: debug|info|warn|err\n");
				usage();
			}
			break;
		case 'f':
			logfile = optarg;
			break;
		default:
			usage();
			/* notreached */
			break;
		}
	}
	argc -= optind;
	argv += optind;
	if ((loglevel != -1) || (logfile != NULL)) {
		log_init(loglevel, logfile);
	}
	if (c_file != NULL) {
		log_msg(LOG_DEBUG | LOG_CONF, "conf_file: %s", c_file);
	}
}

void
conf_file(char *filename)
{
	parse_entry_t  *root;
	if (filename == NULL) {
		printf("no config file\n");
		return;
	}
	root = parse_file(filename);
	i_eval_log_msg(root);	/* get logging options */
	i_eval_basic(root);	/* get basic variables */
	i_eval_services(root);	/* get services */
	i_eval_hosts(root);	/* get hosts */
	i_assure_no_entry(root, NULL);
	parse_cleanup();
}

static parse_entry_t *
i_get_entry_single_value(parse_entry_t * root, char *name, int single)
{
	parse_entry_t  *e;
	if (name != NULL) {
		if ((e = parse_entry_get(root, name)) == NULL) {
			return NULL;
		}
		e->processed = 1;
		if (single) {
			i_assure_no_entry(root, name);
		}
		if (e->values_next != 1) {
			/* syntax error */
			parse_error(e, "expecting single value");
			/* notreached */
		}
	} else {
		e = root;
		if (e->values_next != 1) {
			return NULL;	/* not found */
		}
	}
	return e;
}

static void
i_assure_no_entry(parse_entry_t * root, char *name)
{
	parse_entry_t  *e;
	if ((e = parse_entry_get(root, name)) != NULL) {
		if (name == NULL) {
			parse_error(e, "unknown keyword: %s", e->name);
			/* notreached */
		} else {
			parse_error(e, "%s already set", name);
			/* notreached */
		}
	}
}

static int
i_get_string(parse_entry_t * root, char *name, char **dst, int single)
{
	parse_entry_t  *e;
	if ((e = i_get_entry_single_value(root, name, single)) != NULL) {
		if (*dst) {
			parse_error(root, "%s already set", name);
			/* notreached */
		}
		*dst = strdup(e->values[0]);
		return 1;
	}
	return 0;
}

static int
i_get_bool(parse_entry_t * root, char *name, int *dst, int single)
{
	parse_entry_t  *e;
	if ((e = i_get_entry_single_value(root, name, single)) != NULL) {
		if (str_get_bool(e->values[0], dst) == 0) {
			/* syntax error */
			parse_error(e, "expecting truth value, got \"%s\"", e->values[0]);
			/* notreached */
		}
		return 1;
	}
	return 0;
}

static int
i_get_int(parse_entry_t * root, char *name, int *dst, int single)
{
	parse_entry_t  *e;
	if ((e = i_get_entry_single_value(root, name, single)) != NULL) {
		if (str_get_int(e->values[0], dst) == 0) {
			/* syntax error */
			parse_error(e, "expecting integer value, got \"%s\"", e->values[0]);
			/* notreached */
		}
		return 1;
	}
	return 0;
}

static int
i_get_ether(parse_entry_t * root, char *name, etheraddr_t * dst, int single)
{
	parse_entry_t  *e;
	if ((e = i_get_entry_single_value(root, name, single)) != NULL) {
		if (str_get_eth(e->values[0], dst)) {
			return 1;
		}
		if (c_resolve_ether == 0) {
			parse_error(e, "expecting ethernet address (resolving disabled), got \"%s\"", e->values[0]);
			/* notreached */
		}
		if (util_name2ether(e->values[0], dst)) {
			return 1;
		}
		/* XXX an unresolved ethernet address is fatal */
		parse_error(e, "expecting ethernet address (unknown hostname \"%s\")", e->values[0]);
		/* notreached */
	}
	return 0;
}

static int
i_get_ip(parse_entry_t * root, char *name, ipaddr_t * dst, int single)
{
	parse_entry_t  *e;
	if ((e = i_get_entry_single_value(root, name, single)) != NULL) {
		if (str_get_ip(e->values[0], dst)) {
			return 1;
		}
		if (c_resolve_ip == 0) {
			parse_error(e, "expecting ip address (resolving disabled), got \"%s\"", e->values[0]);
			/* notreached */
		}
		if (util_name2ip(e->values[0], dst)) {
			return 1;
		}
		/* XXX an unresolved ip address is fatal */
		parse_error(e, "expecting ip address (unknown hostname \"%s\")", e->values[0]);
		/* notreached */
	}
	return 0;
}

static int
i_get_service(parse_entry_t * root, char *name, int *dst)
{
	parse_entry_t  *e;
	int             i;
	int             result = 0;
	while ((e = parse_entry_get(root, name)) != NULL) {
		e->processed = 1;
		result = 1;
		for (i = 0; (i < PARSE_VALUES_MAX) && (e->values[i] != NULL); i++) {
			if (!str_get_service(e->values[i], dst)) {
				parse_error(e, "expecting { rarp | bootp | dhcp | mop | rmp | tftp | bootparam | all }");
				/* notreached */
			}
		}
	}
	return result;
}

static int
i_get_loglevel(parse_entry_t * root, char *name, int *dst, int single)
{
	parse_entry_t  *e;
	if ((e = i_get_entry_single_value(root, name, single)) != NULL) {
		if (str_get_loglevel(e->values[0], dst)) {
			return 1;
		}
		parse_error(e, "expecting log level (debug|info|warn|err)");
		/* notreached */
	}
	return 0;
}

static void
i_eval_basic(parse_entry_t * root)
{
	/* initialize the most basic stuff */
	i_get_string(root, "chroot", &c_chroot, 1);
	i_get_string(root, "directory", &c_directory, 1);
	i_get_bool(root, "resolve_ether", &c_resolve_ether, 1);
	i_get_bool(root, "resolve_ip", &c_resolve_ip, 1);
	i_get_int(root, "session_max", &c_session_max, 1);
	i_get_int(root, "session_timeout", &c_session_timeout, 1);
}

static void
i_eval_log_msg(parse_entry_t * root)
{
	parse_entry_t  *e_log;
	parse_entry_t  *e;
	char           *log_file = NULL;
	int             log_level = 0;
	if ((e_log = parse_entry_get(root, "log")) != NULL) {
		e_log->processed = 1;
		i_assure_no_entry(root, "log");

		if ((e = i_get_entry_single_value(e_log->sub, "file", 1)) != NULL) {
			log_file = e->values[0];
		}
		i_get_loglevel(e_log->sub, "level", &log_level, 1);

		i_assure_no_entry(e_log->sub, NULL);
	}
	log_init(log_level, log_file);
}


static void
i_eval_hosts(parse_entry_t * root)
{
	parse_entry_t  *e_host;
	host_t         *h;
	while ((e_host = parse_entry_get(root, "host")) != NULL) {
		e_host->processed = 1;
		h = host_new();
		i_get_string(e_host, NULL, &(h->name), 1);

		i_get_string(e_host->sub, "name", &(h->name), 1);
		i_get_ether(e_host->sub, "ether", &(h->eth), 1);
		i_get_ip(e_host->sub, "ip", &(h->ip), 1);
		i_get_service(e_host->sub, "service", &(h->service));
		i_eval_host_files(e_host->sub, h);
		i_assure_no_entry(e_host->sub, NULL);
		host_add(h);
	}
}

static void
i_eval_host_files(parse_entry_t * root, host_t * h)
{
	parse_entry_t  *e_file;
	parse_entry_t  *e;
	file_t         *f;
	int             i;
	while ((e_file = parse_entry_get(root, "file")) != NULL) {
		e_file->processed = 1;
		f = host_file_new(h);
		i_get_string(e_file, NULL, &(f->path), 1);
		i_get_string(e_file->sub, "path", &(f->path), 1);
		i_get_string(e_file->sub, "name", &(f->name), 1);
		i_get_service(e_file->sub, "service", &(f->service));
		for (i = 0; i < FILE_ALIASES_MAX; i++) {
			i_get_string(e_file->sub, "alias", &(f->aliases[i]), 0);
		}
		if ((e = parse_entry_get(e_file->sub, "alias"))) {
			parse_error(e, "too many aliases");
		}
		i_get_string(e_file->sub, "server", &(f->server), 1);
		i_assure_no_entry(e_file->sub, NULL);
		host_file_add(h, f);
	}
}

static void
i_eval_services(parse_entry_t * root)
{
	parse_entry_t  *e_service;
	parse_entry_t  *e;
	service_t      *s;
	int             service, i;
	char            buf[512];
	while ((e_service = parse_entry_get(root, "service")) != NULL) {
		e_service->processed = 1;
		if (e_service->values_next != 1) {
			parse_error(e_service, "expecting single value");
			/* notreached */
		}
		service = 0;
		if (!str_get_service(e_service->values[0], &service)) {
			parse_error(e_service, "expecting service");
			/* notreached */
		}
		if ((s = service_search_by_id(service)) == NULL) {
			parse_error(e_service, "service not found: %d", service);
			/* notreached */
		}
		if (s->id & LISTEN_LINK) {
			/* link layer */
			while ((e = parse_entry_get(e_service->sub, "device")) != NULL) {
				e->processed = 1;
				for (i = 0; i < e->values_next; i++) {
					if (!service_link_add(s, e->values[i])) {
						parse_warn(e, "invalid device: %s", e->values[i]);
					}
				}
			}
		} else if ((s->id & LISTEN_UDP) || (s->id & LISTEN_RPC_UDP)) {
			/* udp */
			while ((e = parse_entry_get(e_service->sub, "bind")) != NULL) {
				e->processed = 1;
				for (i = 0; i < e->values_next; i++) {
					if (!service_udp_add(s, e->values[i])) {
						parse_warn(e, "invalid bind specification: %s", e->values[i]);
					}
				}
			}
		}
		/* HOOK service */
		switch (service) {
		case SERVICE_RARP:
			break;
		case SERVICE_DHCP:
			break;
		case SERVICE_MOP:
			/* not supported */
			log_msg(LOG_WARN | LOG_CONF, "mop protocol not supported");
			break;
		case SERVICE_RMP:
			break;
		case SERVICE_TFTP:
			break;
		case SERVICE_BOOTPARM:
			break;
		default:
			break;
		}
		if (s->id & LISTEN_LINK) {
			buf[0] = 0;
			for (i = 0; (i < SERVICE_BIND_MAX) && (s->d.link.device[i]); i++) {
				snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " %s", s->d.link.device[i]);
			}
			log_msg(LOG_DEBUG | LOG_CONF, "%s:%s %s", s->name, buf, s->d.link.filter);
		} else if (s->id & LISTEN_UDP) {
			buf[0] = 0;
			for (i = 0; (i < SERVICE_BIND_MAX) && (s->d.udp.bindaddr[i].sin_family != 0); i++) {
				str_sai(&(s->d.udp.bindaddr[i]), sizeof(buf) - strlen(buf), buf + strlen(buf));
			}
			log_msg(LOG_DEBUG | LOG_CONF, "%s:%s", s->name, buf);
		} else if (s->id & LISTEN_RPC_UDP) {
			buf[0] = 0;
			for (i = 0; (i < SERVICE_BIND_MAX) && (s->d.rpc_udp.bindaddr[i].sin_family != 0); i++) {
				str_sai(&(s->d.rpc_udp.bindaddr[i]), sizeof(buf) - strlen(buf), buf + strlen(buf));
			}
			log_msg(LOG_DEBUG | LOG_CONF, "%s:%s", s->name, buf);
		}
	}
}

void
conf_get_loglevel(gen_tokenize_t * tokenize, int *loglevel)
{
	char            w[32];
	for (gen_tokenize_get(tokenize, sizeof(w), w); !str_is_equal(w, ";"); gen_tokenize_get(tokenize, sizeof(w), w)) {
		if (!str_get_loglevel(w, loglevel)) {
			gen_tokenize_error(tokenize, "expecting { debug | info | warn | err }");
			/* notreached */
		}
	}
}

void
conf_host_file_add(host_t * h, file_t * f)
{
	file_t         *n;
	int             has_name, has_path, n_alias;
	char           *ch;
	/* sanity check h */
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
				log_msg(LOG_WARN | LOG_CONF, "duplicate entry for file %s (ignored)",
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
