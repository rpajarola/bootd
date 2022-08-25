/*
 * $Id: ngenutil.c,v 1.2 2002/07/22 13:03:14 pajarola Exp $
 */

#include "bootd.h"


int
str_get_service(char *w, int *dst)
{
	service_t      *s;
	if ((w == NULL) || (dst == NULL)) {
		return 0;
	}
	if (str_is_equalnc(w, "all")) {
		*dst = SERVICE_ALL;
		return 1;
	} else if (str_is_equalnc(w, "none")) {
		*dst = 0;
		return 1;
	} else {
		if ((s = service_search_by_name(w)) != NULL) {
			*dst |= s->id;
			return 1;
		}
	}
	return 0;
}

void
str_service(int service, int l, char *dst)
{
	char            buf[512];
	int             i, j;
	service_t      *s;
	buf[0] = 0;

	if (service == SERVICE_ALL) {
		strncpy(dst, "all", l);
		return;
	}
	for (i = 0; i < LOG_MOD_MAX; i++) {
		if ((j = service & (1 << i))) {
			if ((s = service_search_by_id(j)) == NULL) {
				if (buf[0] == 0) {
					snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%d", j);
				} else {
					snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " %d", j);
				}
			} else {
				if (buf[0] == 0) {
					snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%s", s->name);
				} else {
					snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), " %s", s->name);
				}
			}
		}
	}
	strncpy(dst, buf, l);
}
