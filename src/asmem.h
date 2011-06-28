/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#ifndef asmem__H
#define asmem__H

#include <stdbool.h>

/* file to read for stat info */
#define PROC_MEM "/proc/meminfo"

/* The structure defines what memory information we use */
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
	long int updateInterval;	/* interval (sec) to check the statistics */
	bool showUsed;		 	/* show used instead of free */
	char procMemFilename[256];	/* the file to read for the memory info */
	char bgColor[50];
	char fgColor[50];
	char memoryColor[50];
	char bufferColor[50];
	char cacheColor[50];
	char swapColor[50];
	AsmemMeminfo_t last;		/* the old data */
	AsmemMeminfo_t fresh;		/* the new data */
} AsmemState_t;

typedef struct {
	Pixmap pixmap;
	Pixmap mask;
	XpmAttributes attributes;
} XpmIcon_t;

#endif
