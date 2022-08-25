/*
 * $Id: log.c,v 1.2 2002/07/22 13:07:29 pajarola Exp $
 */

#define GENUTIL
#include "genutil.h"
#include <errno.h>
#include <stdarg.h>
#include <time.h>

static int      l_modlevel2modnum(int level);

static char    *l_month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char    *l_level_names[] = {"DEBUG", "INFO", "WARN", "ERROR"};
static char    *l_cat_names[] = {"MISC", "CONF", "DEV", "BOOT", "RES", "XXX5", "XXX6", "XXX7"};
static char    *l_mod_names[LOG_MOD_MAX + 1] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", ""};

static int      l_minlevel = -1;
static int      l_minlevel_cat[LOG_CAT_MAX+1];
static int      l_minlevel_mod[LOG_MOD_MAX+1];

static int      l_fd = STDERR_FILENO;

static int
l_modlevel2modnum(int level)
{
	int             i;
	/* mod=0x1, 0x2, 0x4, 0x8, 0x10... */
	for (i = 0; i < LOG_MOD_MAX; i++) {
		if (level == (1 << i)) {
			return i + 1;
		}
	}
	return 0;
}

void
log_init(int level, char *filename)
{
	/* reset log masks */
	int             i;
	if (l_minlevel != -1) {
		return;
	}
	l_minlevel = LOG_DEBUG;
	for (i = 0; i <= LOG_CAT_MAX; i++) {
		l_minlevel_cat[i] = LOG_ERR;
	}
	/*
	 * XXX makes it crash for (i = 0; i <= LOG_MOD_MAX; i++) {
	 * l_minlevel_mod[i] = LOG_ERR; }
	 */
	log_set_level(level & LOG_LEVEL);
	log_set_file(filename);
}

void
log_set_file(char *filename)
{
	int             fd;
	if (filename != NULL) {
		if (str_is_equal(filename, "-") || (str_is_equal(filename, "stdout"))) {
			filename = "stdout";
			fd = STDOUT_FILENO;
		} else if (str_is_equal(filename, "stderr")) {
			filename = "stderr";
			fd = STDERR_FILENO;
		} else if ((fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0777)) == -1) {
			log_msg(LOG_WARN, "open(%s, WRONLY|CREAT|APPEND, 0777): %s", filename, strerror(errno));
		} else {
			log_msg(LOG_INFO, "logging to %s", filename);
			if ((l_fd != STDOUT_FILENO) && (l_fd != STDERR_FILENO)) {
				close(l_fd);
			}
			l_fd = fd;
		}
	}
}

void
log_set_level(int minlevel)
{
	if (minlevel & LOG_CAT) {
		l_minlevel_cat[(minlevel & LOG_CAT) >> LOG_CAT_SHIFT] = minlevel & LOG_LEVEL;
	} else if (minlevel & LOG_MOD) {
		l_minlevel_mod[(minlevel & LOG_MOD) >> LOG_MOD_SHIFT] = minlevel & LOG_LEVEL;
	} else {
		l_minlevel = minlevel;
	}
}

void
log_set_mod_name(int mod, char *name)
{
	l_mod_names[l_modlevel2modnum(mod)] = name;
}

void
log_msg(int what, char *fmt,...)
{
	char            buf[4096];
	struct timeval  tv;
	struct tm      *ptm;
	time_t          t;
	va_list         ap;
	int             i;
	if ((what & LOG_LEVEL) >= l_minlevel) {
		/* OK */
	} else if ((what & LOG_LEVEL) >= l_minlevel_cat[(what & LOG_CAT) >> LOG_CAT_SHIFT]) {
		/* OK */
	} else if ((what & LOG_LEVEL) >= l_minlevel_mod[(what & LOG_MOD) >> LOG_MOD_SHIFT]) {
		/* OK */
	} else {
		return;
	}

	gettimeofday(&tv, NULL);
	t = tv.tv_sec;
	ptm = localtime(&t);

	snprintf(buf, sizeof(buf), "%3s %02d %02d:%02d:%02d.%03ld %04d [%d] %-5s %-4s %s ",
		 l_month_names[ptm->tm_mon], ptm->tm_mday,
		 ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (long)tv.tv_usec / 1000,
		 1900 + ptm->tm_year,
		 getpid(), l_level_names[what & LOG_LEVEL], l_cat_names[(what & LOG_CAT) >> LOG_CAT_SHIFT], l_mod_names[l_modlevel2modnum(what & LOG_MOD)]);

	va_start(ap, fmt);
	vsnprintf(buf + strlen(buf), sizeof(buf), fmt, ap);
	va_end(ap);

	i = strlen(buf) - 1;
	if ((buf[i] == 10) || (buf[i] == 13)) {
		buf[i] = 0;
	}
	strcat(buf, "\n");
        if (write(l_fd, buf, strlen(buf)) == -1) {
	    /* do nothing. But it makes the compiler happy */
	}
	if ((what & LOG_LEVEL) == LOG_ERR) {
		exit(1);
	}
}
