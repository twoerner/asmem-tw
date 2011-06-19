/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "state.h"

#include "config.h"

extern struct asmem_state state;

#define BUFFER_LENGTH 400
int fd;
FILE *fp;
char buf[BUFFER_LENGTH];

void error_handle( int place, const char * message )
{
	int error_num;
	error_num = errno;
	/* if that was an interrupt - quit quietly */
	if (error_num == EINTR) {
		printf("asmem: Interrupted.\n");
		return;
	}
	switch ( place )
	{
	case 1: /* opening the /proc/meminfo file */
		switch (error_num)
		{
		case ENOENT :
			printf("asmem: The file %s does not exist. "
			"Weird system it is.\n", state.proc_mem_filename);
			break;
		case EACCES :
			printf("asmem: You do not have permissions "
			"to read %s\n", state.proc_mem_filename);
			break;
		default :
			printf("asmem: cannot open %s. Error %d: %s\n",
				state.proc_mem_filename, errno,
				strerror(errno));
			break;
		}
		break;
	default: /* catchall for the rest */
		printf("asmem: %s: Error %d: %s\n",
			message, errno, strerror(errno));
	}
}

#ifdef DEBUG
#define verb_debug() { \
       printf("+- Total : %ld, used : %ld, free : %ld \n", \
                       state.fresh.total, \
                       state.fresh.used,\
                       state.fresh.free);\
       printf("|  Shared : %ld, buffers : %ld, cached : %ld \n",\
                       state.fresh.shared,\
                       state.fresh.buffers,\
                       state.fresh.cached);\
       printf("+- Swap total : %ld, used : %ld, free : %ld \n",\
                       state.fresh.swap_total,\
                       state.fresh.swap_used,\
                       state.fresh.swap_free);\
       }
#else
#define verb_debug()
#endif /* DEBUG */

/* default /proc/meminfo (Linux) method ... */

int getnum(FILE *fp, char *marker)
{
	char thebuf[255];
	int done = 0;
	int theval;

	do {
		if (fgets(thebuf, sizeof(thebuf), fp) == NULL) {
			printf("file error\n");
			return (-1);
		
		} else
			if (strstr(thebuf, marker)) {
				sscanf(thebuf, "%*s %d %*s\n",
					&theval);
				return (theval);
			}
	}while (!done);
}

int read_meminfo(void)
{
	int result;

	fflush(fp);

	result = fseek(fp, 0L, SEEK_SET);

	if ( result < 0 ) {
		error_handle(2, "seek");
		return -1;
	}

	state.fresh.total = getnum(fp, "MemTotal") * 1024;
	state.fresh.free = getnum(fp, "MemFree") * 1024;
	/* state.fresh.shared = getnum(fp, "MemShared") * 1024; */
	state.fresh.shared = 0;   /* this is always 0 */
	state.fresh.buffers = getnum(fp, "Buffers") * 1024;
	state.fresh.cached = getnum(fp, "Cached") * 1024;
	state.fresh.swap_total = getnum(fp, "SwapTotal") * 1024;
	state.fresh.swap_free = getnum(fp, "SwapFree") * 1024;
	state.fresh.swap_used = state.fresh.swap_total - state.fresh.swap_free;
	state.fresh.used = state.fresh.total - state.fresh.free;

	return 0;
}

int open_meminfo(void)
{
	int result;
	if ((fp = fopen(state.proc_mem_filename, "r")) == NULL) {
		error_handle(1, "");
		return -1;
	}
	return 0;
}

int close_meminfo(void)
{
	fclose(fp);
	return 0;
}

