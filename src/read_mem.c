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

extern AsmemState_t state_G;

#define BUFFER_LENGTH 400
static int fd;
static FILE *file_pG;
static char buf[BUFFER_LENGTH];

static void
error_handle (int place, const char *message_p)
{
	int errorNum;

	errorNum = errno;
	/* if that was an interrupt - quit quietly */
	if (errorNum == EINTR) {
		printf ("asmem: Interrupted.\n");
		return;
	}
	switch (place) {
		case 1: /* opening the /proc/meminfo file */
			switch (errorNum) {
				case ENOENT :
					printf ("asmem: The file %s does not exist. Weird system it is.\n", state_G.procMemFilename);
					break;
				case EACCES :
					printf ("asmem: You do not have permissions to read %s\n", state_G.procMemFilename);
					break;
				default :
					printf ("asmem: cannot open %s. Error %d: %s\n", state_G.procMemFilename, errno, strerror (errno));
					break;
			}
			break;

		default: /* catchall for the rest */
			printf ("asmem: %s: Error %d: %s\n", message_p, errno, strerror (errno));
	}
}

#ifdef DEBUG
#define verb_debug () { \
	printf ("+- Total : %ld, used : %ld, free : %ld \n", state_G.fresh.total, state_G.fresh.used, state_G.fresh.free); \
	printf ("|  Shared : %ld, buffers : %ld, cached : %ld \n", state_G.fresh.shared, state_G.fresh.buffers, state_G.fresh.cached); \
	printf ("+- Swap total : %ld, used : %ld, free : %ld \n", state_G.fresh.swapTotal, state_G.fresh.swapUsed, state_G.fresh.swapFree); \
}
#else
#define verb_debug ()
#endif /* DEBUG */

static int
get_num (FILE *file_p, char *marker_p)
{
	char thebuf[255];
	int done = 0;
	int theval;

	do {
		if (fgets (thebuf, sizeof (thebuf), file_p) == NULL) {
			printf ("file error\n");
			return (-1);

		} else
			if (strstr (thebuf, marker_p)) {
				sscanf (thebuf, "%*s %d %*s\n", &theval);
				return (theval);
			}
	} while (!done);
}

int
read_meminfo (void)
{
	int result;

	fflush (file_pG);

	result = fseek (file_pG, 0L, SEEK_SET);
	if (result < 0) {
		error_handle (2, "seek");
		return -1;
	}

	state_G.fresh.total = get_num (file_pG, "MemTotal") * 1024;
	state_G.fresh.free = get_num (file_pG, "MemFree") * 1024;
	state_G.fresh.shared = 0;   /* this is always 0 */
	state_G.fresh.buffers = get_num (file_pG, "Buffers") * 1024;
	state_G.fresh.cached = get_num (file_pG, "Cached") * 1024;
	state_G.fresh.swapTotal = get_num (file_pG, "SwapTotal") * 1024;
	state_G.fresh.swapFree = get_num (file_pG, "SwapFree") * 1024;
	state_G.fresh.swapUsed = state_G.fresh.swapTotal - state_G.fresh.swapFree;
	state_G.fresh.used = state_G.fresh.total - state_G.fresh.free;

	return 0;
}

int
open_meminfo (void)
{
	int result;
	if ((file_pG = fopen (state_G.procMemFilename, "r")) == NULL) {
		error_handle (1, "");
		return -1;
	}
	return 0;
}

int
close_meminfo (void)
{
	fclose (file_pG);
	return 0;
}
