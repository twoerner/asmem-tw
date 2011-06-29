/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#ifndef asmem__H
#define asmem__H

#include <X11/xpm.h>

// file to read for memory info
#define PROC_MEM "/proc/meminfo"

typedef struct {
	unsigned long total;		/* total memory available */
	unsigned long used;		/* the total of used memory */
	unsigned long free;		/* free memory */
	unsigned long buffers;		/* buffers memory */
	unsigned long cached;		/* cached memory */
	unsigned long swapTotal;	/* total swap space */
	unsigned long swapUsed;		/* used swap space */
	unsigned long swapFree;		/* free swap space */
} AsmemMeminfo_t;

typedef struct {
	Pixmap pixmap;
	Pixmap mask;
	XpmAttributes attributes;
} XpmIcon_t;

#endif
