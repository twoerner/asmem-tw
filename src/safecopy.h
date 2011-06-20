/*
 * Copyright (C) 2011  Trevor Woerner
 * 
 * This software is distributed under GPL. For details see LICENSE file.
 */

#ifndef safecopy__H
#define safecopy__H

/*
 * Copies at most maxlen-1 characters from the source.
 * Makes sure that the destination string is zero-terminated.
 */
char* safe_copy (char *dest_p, const char *src_p, unsigned short maxlen);

#endif
