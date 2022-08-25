/*
 * $Id: tokenize.c,v 1.2 2002/07/22 13:07:29 pajarola Exp $
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/errno.h>

#define GENUTIL
#include "genutil.h"

static void     gen_tokenize_tokenize(gen_tokenize_t * tokenize);
static int      gen_tokenize_read(gen_tokenize_t * tokenize);

gen_tokenize_t     *
gen_tokenize_new(char *filename)
{
	gen_tokenize_t     *tokenize;
	if (filename == NULL) {
		log_msg(LOG_ERR | LOG_CONF, "tokenize open %s: filename is NULL\n", filename);
		/* notreached */
	}
	tokenize = malloc(sizeof(gen_tokenize_t));
	memset(tokenize, 0, sizeof(gen_tokenize_t));
	tokenize->fname = strdup(filename);
	tokenize->ts = -1;
	if ((tokenize->fd = open(filename, O_RDONLY)) == -1) {
		log_msg(LOG_ERR | LOG_CONF, "tokenize open %s: %s\n", filename, strerror(errno));
		/* notreached */
	}
	return tokenize;
}

void
gen_tokenize_close(gen_tokenize_t * tokenize)
{
	if (tokenize != NULL) {
		if (tokenize->fname != NULL) {
			free(tokenize->fname);
		}
		free(tokenize);
	}
}

static int
gen_tokenize_read(gen_tokenize_t * tokenize)
{
	if ((tokenize->s < 0) || (tokenize->e <= tokenize->s)) {
		/* there is no more data in tokenize->b -> read from file */
		tokenize->s = 0;
		tokenize->e = read(tokenize->fd, tokenize->b, 4096);
		switch (tokenize->e) {
		case 0:
			/* no more data from file */
			return 0;
		case -1:
			/* error */
			log_msg(LOG_ERR | LOG_CONF, "read %s: %s\n", tokenize->fname, strerror(errno));
			/* notreached */
		}
	}
	return 1;
}

static void
gen_tokenize_tokenize(gen_tokenize_t * tokenize)
{
	gen_token_t        *tok;
	int             t;
	int             esc, quo, com;
	char            c;
	if (tokenize == NULL) {
		return;
	}
	while (tokenize->ts != tokenize->te) {
		tok = &(tokenize->t[tokenize->te]);
		esc = quo = com = 0;
		tok->b[0] = 0;
		for (t = 0; t < TOKENIZE_BUFSIZE;) {
			/* get next character */
			if (gen_tokenize_read(tokenize) == 0) {
				return;
			}
			c = tokenize->b[tokenize->s++];

			/* update x/y position, handle comment */
			if ((c == 10) || (c == 13)) {
				tokenize->x = 0;
				tokenize->y++;
				if (com) {
					com = 0;
					continue;
				}
			} else {
				tokenize->x++;
			}
			if (t == 0) {
				tok->x = tokenize->x;
				tok->y = tokenize->y;
			}
			if (com) {
				continue;
			}
			if (c == 0) {
				if (t == 0) {
					continue;
				} else {
					tok->b[t] = 0;
					t = TOKENIZE_BUFSIZE + 1;
					continue;
				}
			}
			if (esc) {
				/* don't interpret next character */
				tok->b[t++] = c;
				esc = 0;
				continue;
			}
			if ((quo == 2) && (c == '"')) {
				quo = 0;
				continue;
			}
			if ((quo == 1) && (c == '\'')) {
				quo = 0;
				continue;
			}
			if (quo) {
				tok->b[t++] = c;
				continue;
			}
			switch (c) {
			case '\\':	/* escape */
				esc = 1;
				break;
			case '"':	/* double quote */
				quo = 2;
				break;
			case '\'':	/* single quote */
				quo = 1;
				break;
			case 13:	/* white space */
			case 10:
			case ' ':
			case '	':
				if (t != 0) {
					tok->b[t] = 0;
					t = TOKENIZE_BUFSIZE + 1;
					continue;
				}
				break;
			case '#':	/* comment */
				com = 1;
				break;
			case ';':	/* statement delimiter */
				if (t == 0) {
					tok->b[t++] = ';';
					tok->b[t] = 0;
					t = TOKENIZE_BUFSIZE + 1;
					continue;
				} else {
					tokenize->s--;
					tokenize->x--;
					tok->b[t] = 0;
					t = TOKENIZE_BUFSIZE + 1;
					continue;
				}
				break;
			default:
				tok->b[t++] = c;
			}
		}
		if ((++tokenize->te) > TOKENIZE_LOOKAHEAD) {
			tokenize->te = 0;
		}
		if (tokenize->ts == -1) {
			tokenize->ts = 0;
		}
		tok->b[t] = 0;
		t = TOKENIZE_BUFSIZE + 1;
		continue;
	}
}

int
gen_tokenize_next(gen_tokenize_t * tokenize, int l, char *w)
{
	if (gen_tokenize_ahead(tokenize, 0, l, w)) {
		if ((++tokenize->ts) > TOKENIZE_LOOKAHEAD) {
			tokenize->ts = 0;
		}
		if (tokenize->ts == tokenize->te) {
			tokenize->ts = -1;
		}
		return 1;
	} else {
		return 0;
	}
}

int
gen_tokenize_ahead(gen_tokenize_t * tokenize, int n, int l, char *w)
{
	int             max;
	gen_tokenize_tokenize(tokenize);
	if (tokenize->ts == -1) {
		return 0;
	}
	max = tokenize->te - tokenize->ts;
	if (max <= 0) {
		max += TOKENIZE_LOOKAHEAD;
	}
	if (n > max) {
		return 0;
	}
	n = (n + tokenize->ts) % (TOKENIZE_LOOKAHEAD + 1);
	strncpy(w, tokenize->t[n].b, l);
	return 1;
}

void
gen_tokenize_get(gen_tokenize_t * tokenize, int l, char *w)
{
	if (gen_tokenize_next(tokenize, l, w) == 0) {
		gen_tokenize_error(tokenize, "unexpected end of file");
		/* notreached */
	}
}

void
gen_tokenize_expect(gen_tokenize_t * tokenize, char *w)
{
	char            b[4096];
	gen_tokenize_get(tokenize, sizeof(b), b);
	if (!str_is_equal(b, w)) {
		str_do_quote(w, sizeof(b), b);
		gen_tokenize_error(tokenize, "expecting %s, got \"%s\"", w, b);
		/* notreached */
	}
}

void
gen_tokenize_error(gen_tokenize_t * tokenize, char *fmt,...)
{
	va_list         ap;
	char            msg[4096];
	snprintf(msg, sizeof(msg), "%s:%d: ", tokenize->fname, tokenize->y + 1);
	va_start(ap, fmt);
	vsnprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), fmt, ap);
	va_end(ap);
	log_msg(LOG_ERR | LOG_CONF, msg);
}

void
gen_tokenize_warn(gen_tokenize_t * tokenize, char *fmt,...)
{
	va_list         ap;
	char            msg[4096];
	snprintf(msg, sizeof(msg), "%s:%d: ", tokenize->fname, tokenize->y + 1);
	va_start(ap, fmt);
	vsnprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), fmt, ap);
	va_end(ap);
	log_msg(LOG_WARN | LOG_CONF, msg);
}
