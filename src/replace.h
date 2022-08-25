/*
 * $Id: replace.h,v 1.2 2002/07/22 13:07:29 pajarola Exp $
 */

#ifndef _REPLACE_H
#define _REPLACE_H

#include "config.h"

#ifndef HAVE_STRNCPY
char           *strncpy(char *dst, const char *src, size_t len);
#endif

#ifndef HAVE_STRLCPY
size_t          strlcpy(char *dst, const char *src, size_t size);
#endif

#ifndef HAVE_STRNCAT
char           *strncat(char *dst, const char *src, size_t count);
#endif

#ifndef HAVE_STRLCAT
size_t          strlcat(char *dst, const char *src, size_t size);
#endif

#endif
