/*
 * $Id: parse.c,v 1.2 2002/07/22 13:07:29 pajarola Exp $
 */

#define GENUTIL
#include "genutil.h"
#include <errno.h>
#include <stdarg.h>

static char    *parse_string_add(gen_tokenize_t * tokenize, char *s);	/* add a string to the
									 * string table */
static parse_entry_t *parse_entry_new(gen_tokenize_t * tokenize);	/* allocate new entry in
								 * table */
static parse_entry_t *parse_sub(gen_tokenize_t * tokenize);	/* return entry */
#define PARSE_TBL_TABLE_SIZE 256
#define PARSE_TBL_STRING_SIZE 256
#define PARSE_TBL_ENTRY_SIZE 256

typedef char    tbl_string_t[PARSE_TBL_STRING_SIZE];

static void   **l_tbl_table = NULL;
static tbl_string_t *l_tbl_string = NULL;
static parse_entry_t *l_tbl_entry = NULL;
static int      l_tbl_table_next = 0;
static int      l_tbl_string_next = 0;
static int      l_tbl_entry_next = 0;

parse_entry_t  *
parse_file(char *filename)
{
	gen_tokenize_t     *tokenize;
	parse_entry_t  *result;
	if (filename == NULL) {
		log_msg(LOG_INFO, "no config file\n");
	}
	l_tbl_table = malloc(sizeof(void *) * PARSE_TBL_TABLE_SIZE);
	memset(l_tbl_table, 0, sizeof(void *) * PARSE_TBL_TABLE_SIZE);

	tokenize = gen_tokenize_new(filename);
	result = parse_sub(tokenize);
	gen_tokenize_close(tokenize);
	return result;
}

void
parse_cleanup()
{
	int             i;
	if (l_tbl_table == NULL) {
		/* nothing to do */
		return;
	}
	for (i = 0; i < PARSE_TBL_TABLE_SIZE; i++) {
		if (l_tbl_table[i] != NULL) {
			free(l_tbl_table[i]);
		}
	}
	free(l_tbl_table);
	l_tbl_table = NULL;
	l_tbl_table_next = 0;
	l_tbl_string = NULL;
	l_tbl_string_next = 0;
	l_tbl_entry = NULL;
	l_tbl_entry_next = 0;
}
/*
 * this is highly inefficient, replace with some other mechanism... (btree,
 * hashtable, whatever)
 */

static char    *
parse_string_add(gen_tokenize_t * tokenize, char *s)
{
	char            tmp[PARSE_STRING_MAX];
	int             i;
	if (l_tbl_table == NULL) {
		gen_tokenize_error(tokenize, "l_tbl_table == NULL");
		/* notreached */
	}
	if ((l_tbl_string == NULL) || (l_tbl_string_next == PARSE_TBL_STRING_SIZE)) {
		if (l_tbl_table_next == PARSE_TBL_TABLE_SIZE) {
			/* XXX use realloc instead */
			gen_tokenize_error(tokenize, "running out of config entries, increase PARSE_TBL_TABLE_SIZE");
			/* notreached */
		}
		l_tbl_string = malloc(PARSE_STRING_MAX * PARSE_TBL_STRING_SIZE);
		l_tbl_string_next = 0;
		l_tbl_table[l_tbl_table_next++] = l_tbl_string;
	}
	if (l_tbl_string == NULL) {
		gen_tokenize_error(tokenize, "%s", strerror(errno));
		/* notreached */
	}
	if (strlen(s) >= PARSE_STRING_MAX) {
		strncpy(tmp, s, sizeof(tmp));
		tmp[sizeof(tmp) - 1] = 0;
		gen_tokenize_error(tokenize, "oversized token: %s", tmp);
		/* notreached */
	}
	for (i = 0; i < l_tbl_string_next; i++) {
		if (strcmp(s, l_tbl_string[i]) == 0) {
			return l_tbl_string[i];
		}
	}
	strcpy(l_tbl_string[l_tbl_string_next], s);
	return l_tbl_string[l_tbl_string_next++];
}

static parse_entry_t *
parse_entry_new(gen_tokenize_t * tokenize)
{
	parse_entry_t  *result;
	if (l_tbl_table == NULL) {
		gen_tokenize_error(tokenize, "l_tbl_table == NULL");
		/* notreached */
	}
	if ((l_tbl_entry == NULL) || (l_tbl_entry_next == PARSE_TBL_ENTRY_SIZE)) {
		if (l_tbl_table_next == PARSE_TBL_TABLE_SIZE) {
			/* XXX use realloc instead */
			gen_tokenize_error(tokenize, "running out of config entries, increase PARSE_TBL_TABLE_SIZE");
			/* notreached */
		}
		l_tbl_entry = malloc(sizeof(parse_entry_t) * PARSE_TBL_ENTRY_SIZE);
		l_tbl_entry_next = 0;
		l_tbl_table[l_tbl_table_next++] = l_tbl_entry;
	}
	if (l_tbl_entry == NULL) {
		gen_tokenize_error(tokenize, "%s", strerror(errno));
		/* notreached */
	}
	result = &(l_tbl_entry[l_tbl_entry_next++]);
	memset(result, 0, sizeof(parse_entry_t));
	return result;
}

parse_entry_t  *
parse_entry_get(parse_entry_t * root, char *name)
{
	char            tmp[PARSE_STRING_MAX];
	parse_entry_t  *result;
	if (root == NULL) {
		return NULL;
	}
	if ((name != NULL) && (strlen(name) >= PARSE_STRING_MAX)) {
		/* this usually is a programming error!!! */
		strncpy(tmp, name, sizeof(tmp));
		parse_error(root, "oversize name: %s", tmp);
		/* notreached */
	}
	for (result = root; result != NULL; result = result->next) {
		if (result->processed == 0) {
			if (name == NULL) {
				return result;
			} else if (strcmp(name, result->name) == 0) {
				return result;
			}
		}
	}
	/* not finding an entry is not an error, return NULL */
	return NULL;
}

static parse_entry_t *
parse_sub(gen_tokenize_t * tokenize)
{
	char            w[PARSE_STRING_MAX];
	parse_entry_t  *e;
	parse_entry_t **prev_next;
	parse_entry_t  *result;
	prev_next = &result;
	while (gen_tokenize_next(tokenize, sizeof(w), w) && (!str_is_equal(w, "}"))) {
		/*
		 * allocate new entry and set previous entrie's next, get
		 * pointer to new entry and set prev_next for next iteration
		 */
		e = *prev_next = parse_entry_new(tokenize);
		prev_next = (parse_entry_t**)&(e->next);

		e->name = parse_string_add(tokenize, w);
		e->values_next = 0;
		e->next = NULL;	/* default: no next */
		e->sub = NULL;	/* default: no sub */
		e->processed = 0;	/* this node has not yet been
					 * processed */
		e->line = tokenize->y;	/* line in config file (for error
					 * msgs) */
		e->filename = parse_string_add(tokenize, tokenize->fname);	/* filename of config
										 * file (for error msgs) */

		/* fill in values */
		gen_tokenize_next(tokenize, sizeof(w), w);
		while ((!str_is_equal(w, "{")) && (!str_is_equal(w, ";"))) {
			if (str_is_equal(w, ",") || str_is_equal(w, "|")) {
				/* accept blanks, ',', and '|' as separators */
				continue;
			}
			if (e->values_next >= PARSE_VALUES_MAX) {
				gen_tokenize_error(tokenize, "maximum number of values (%d) exceeded", PARSE_VALUES_MAX);
			}
			e->values[e->values_next++] = parse_string_add(tokenize, w);
			gen_tokenize_next(tokenize, sizeof(w), w);
		}
		if (str_is_equal(w, "{")) {
			/* sub node */
			e->sub = parse_sub(tokenize);
		}
	}
	return result;
}

void
parse_error(parse_entry_t * e, char *fmt,...)
{
	va_list         ap;
	char            msg[4096];
	snprintf(msg, sizeof(msg), "%s:%d: ", e->filename, e->line);
	va_start(ap, fmt);
	vsnprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), fmt, ap);
	va_end(ap);
	log_msg(LOG_ERR | LOG_CONF, msg);
}

void
parse_warn(parse_entry_t * e, char *fmt,...)
{
	va_list         ap;
	char            msg[4096];
	snprintf(msg, sizeof(msg), "%s:%d: ", e->filename, e->line);
	va_start(ap, fmt);
	vsnprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), fmt, ap);
	va_end(ap);
	log_msg(LOG_WARN | LOG_CONF, msg);
}
