/*
 * $Id: replace.c,v 1.2 2002/07/22 13:07:29 pajarola Exp $
 */

#include <sys/types.h>

#ifndef HAVE_STRLCPY
size_t
strncpy(char *dst, const char *src, size_t size)
{
	int             i;
	for (i = 0; i < (size - 1); i++) {
		dst[i] = src[i];
		if (src[i] == 0) {
			break;
		}
	}
	if (src[i] != 0) {
		dst[size] = 0;
		while (src[i] != 0) {
			i++;
		}
	}
	return i;
}
#endif

#ifndef HAVE_STRNCAT
char           *
strncat(char *dst, const char *src, size_t count)
{
	int             i, j;
	for (i = 0;; i++) {
		if (dst[i] == 0) {
			break;
		}
	}
	for (j = 0; j < count; j++) {
		dst[i + j] = src[j];
		if (src[j] == 0) {
			break;
		}
	}
	dst[j] = 0;
	return dst;
}
#endif

#ifndef HAVE_STRLCAT
size_t
strlcat(char *dst, const char *src, size_t size)
{
	int             i, j;
	for (i = 0; i < (size - 1); i++) {
		if (dst[i] == 0) {
			break;
		}
	}
	if (dst[i] != 0) {
		/* oops, no space to append src */
		return size;
	}
	for (j = 0; (i + j) < (size - 1); j++) {
		dst[i + j] = src[j];
		if (dst[i + j] == 0) {
			break;
		}
	}
	if (src[i + j] != 0) {
		dst[size] = 0;
		while (src[i + j] != 0) {
			j++;
		}
	}
	return (i + j);
}
#endif
