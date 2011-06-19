/*
 * Copyright (c) 1999  Albert Dorofeev <Albert@mail.dma.be>
 * For the updates see http://bewoner.dma.be/Albert/
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#ifndef _state_h_
#define _state_h_

/* file to read for stat info */
#define PROC_MEM "/proc/meminfo"

/* The structure defines what memory information we use */
struct meminfo {
	unsigned long total;	/* total memory available */
	unsigned long used;	/* the total of used memory */
	unsigned long free;	/* free memory */
	unsigned long shared;	/* shared memory */
	unsigned long buffers;	/* buffers memory */
	unsigned long cached;	/* cached memory */
	unsigned long swap_total;	/* total swap space */
	unsigned long swap_used;	/* used swap space */
	unsigned long swap_free;	/* free swap space */
};

struct asmem_state {
	long int update_interval; /* interval (sec) to check the statistics */
	unsigned char standard_free;	/* use free memory as is */
	unsigned char mb; 		/* display in MBytes */
	unsigned char show_used; 	/* show used instead of free */
	char proc_mem_filename[256]; /* the file to read for the memory info */
	char bgcolor[50];
	char fgcolor[50];
	char memory_color[50];
	char buffer_color[50];
	char cache_color[50];
	char swap_color[50];
	struct meminfo last;	/* the old data */
	struct meminfo fresh;	/* the new data */
};

#endif

