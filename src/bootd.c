/*
 * $Id: bootd.c,v 1.4 2002/07/22 13:03:14 pajarola Exp $
 */

#define GLOBAL
#include "bootd.h"
#include "errno.h"

int
main(int argc, char *argv[])
{
	service_init();		/* initialize services (XXX could use static
				 * initializer) */
	conf_args(argc, argv);
	conf_file(c_file);
	if (c_directory != NULL) {
		if (chdir(c_directory) == -1) {
			log_msg(LOG_WARN | LOG_CONF, "chdir(%s): %s", c_directory, strerror(errno));
		}
	}
	service_start();	/* create the services */
	log_msg(LOG_INFO | LOG_DEV, "starting server loop");
	while (g_run) {
		listener_select();
	}
	util_stats();
	util_cleanup();
	return 0;
}
