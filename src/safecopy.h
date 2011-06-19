#ifndef _safecopy_h_
#define _safecopy_h_

/*
 * Copyright (c) 1998  Albert Dorofeev <Albert@mail.dma.be>
 * For the updates see http://bewoner.dma.be/Albert/
 * 
 * This software is distributed under GPL. For details see LICENSE file.
 */

/*
 * Copies at most maxlen-1 characters from the source.
 * Makes sure that the destination string is zero-terminated.
 */
char *safecopy(char *dest, const char *src, unsigned short maxlen);

#endif

