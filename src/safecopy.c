/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#include <string.h>

/*
 * Copies at most maxlen-1 characters from the source.
 * Makes sure that the destination string is zero-terminated.
 */
char*
safe_copy (char *dest_p, const char *src_p, unsigned short maxlen)
{
	/* safety precaution */
	dest_p[maxlen-1] = 0;
	return strlen (src_p) < maxlen ? strcpy (dest_p, src_p) : strncpy (dest_p, src_p, maxlen-1);
}
