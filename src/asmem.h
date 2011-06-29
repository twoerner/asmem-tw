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
	unsigned long memTotal;		/* total memory available */
	unsigned long memFree;		/* free memory */
	unsigned long memBuffers;	/* buffers memory */
	unsigned long memCached;	/* cached memory */
	unsigned long swapTotal;	/* total swap space */
	unsigned long swapFree;		/* free swap space */

	unsigned long memUsed;		/* the total of used memory */
	unsigned long swapUsed;		/* used swap space */
} AsmemMeminfo_t;

typedef struct {
	Pixmap pixmap;
	Pixmap mask;
	XpmAttributes attributes;
} XpmIcon_t;

#endif
