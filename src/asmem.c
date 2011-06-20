/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "safecopy.h"

#include "asmem_x.h"
#include "state.h"
#include "config.h"

extern AsmemState_t state_G;

/*
 * default check and update intervals in microseconds
 *      x11 events - every 1/100 th of a second (in mks)
 *      Memory status - every second (in sec)
 */
#define X11_INTERVAL 10000L
#define CHK_INTERVAL 1

static int withdrawn_G = 0;
static int iconic_G = 0;
static int pushedIn_G = 1;
static char displayName_G[50];
static char mainGeometry_G[50];

static void
defaults (void)
{
	state_G.update_interval = CHK_INTERVAL;
	state_G.standard_free = 0;
	state_G.mb = 0;
	state_G.show_used = 0;
	safe_copy (state_G.proc_mem_filename, PROC_MEM, 256);
	withdrawn_G = 0;
	iconic_G = 0;
	pushedIn_G = 1;
	safe_copy (displayName_G, "", 50);
	safe_copy (mainGeometry_G, "", 50);
	safe_copy (state_G.bgcolor, "#303030", 50);
	safe_copy (state_G.fgcolor, "#20b2aa", 50);
	safe_copy (state_G.memory_color, "#4141d7", 50);
	safe_copy (state_G.buffer_color, "#aa80aa", 50);
	safe_copy (state_G.cache_color, "#bebebe", 50);
	safe_copy (state_G.swap_color, "#ffa649", 50);
}

/* print the usage for the tool */
static void
usage (void)
{
	printf ("usage: asmem [options ...]\n\n");
	printf ("-V              print version and exit\n");
	printf ("-h -H -help     print this message\n");
	printf ("-u <secs>       the update interval in seconds\n");
	printf ("-mb             the display is in MBytes\n");
	printf ("-used           display used memory instead of free\n");
	printf ("-display <name> the name of the display to use\n");
	printf ("-position <xy>  position on the screen (geometry)\n");
	printf ("-withdrawn      start in withdrawn shape (for WindowMaker)\n");
	printf ("-iconic         start iconized\n");
	printf ("-standout       standing out rather than being pushed in\n");
	printf ("-asis           use free memory amount as read from device\n");
	printf ("-dev <device>   use the specified file as stat device\n\n");
	printf ("-bg <color>     background color\n");
	printf ("-fg <color>     base foreground color\n");
	printf ("-memory <color> used memory bar color\n");
	printf ("-buffer <color> buffer memory bar color\n");
	printf ("-cache <color>  cache memory bar color\n");
	printf ("-swap <color>   used swap space bar color\n");
	printf ("\n");
	exit (0);
}

/* print the version of the tool */
static void
version (void)
{
	printf ("%s\n", PACKAGE_STRING);
}

static void
parse_cmdline (int argc, char *argv[])
{
	char *arg_p;
	int  i;

	/* parse the command line */
	for (i=1; i<argc; i++) {
		arg_p = argv[i];
		if (arg_p[0] == '-') {
			if (!strncmp (arg_p, "-withdrawn", 10)) {
				withdrawn_G = 1;
			} else if (!strncmp (arg_p, "-iconic", 7)) {
				iconic_G = 1;
			} else if (!strncmp (arg_p, "-standout", 9)) {
				pushedIn_G = 0;
			} else if (!strncmp (arg_p, "-asis", 5)) {
				state_G.standard_free = 1;
			} else if (!strncmp (arg_p, "-free", 5)) {
				state_G.standard_free = 1;
			} else if (!strncmp (arg_p, "-mb", 3)) {
				state_G.mb = 1;
			} else if (!strncmp (arg_p, "-used", 5)) {
				state_G.show_used=1;
			} else if (!strncmp (arg_p, "-u", 2)) {
				if (++i >= argc)
					usage();
				state_G.update_interval = atoi (argv[i]);
				if (state_G.update_interval < 1)
					state_G.update_interval = CHK_INTERVAL;
			} else if (!strncmp (arg_p, "-position", 9)) {
				if (++i >= argc)
					usage ();
				safe_copy (mainGeometry_G, argv[i], 50);
			} else if (!strncmp (arg_p, "-display", 8)) {
				if (++i >= argc)
					usage ();
				safe_copy (displayName_G, argv[i], 50);
			} else if (!strncmp (arg_p, "-dev", 4)) {
				if (++i >= argc)
					usage ();
				safe_copy (state_G.proc_mem_filename, argv[i], 256);
			} else if (!strncmp (arg_p, "-bg", 3)) {
				if (++i >= argc)
					usage ();
				safe_copy (state_G.bgcolor, argv[i], 50);
			} else if (!strncmp (arg_p, "-fg", 3)) {
				if (++i >= argc)
					usage ();
				safe_copy (state_G.fgcolor, argv[i], 50);
			} else if (!strncmp (arg_p, "-memory", 7)) {
				if (++i >= argc)
					usage ();
				safe_copy (state_G.memory_color, argv[i], 50);
			} else if (!strncmp (arg_p, "-buffer", 7)) {
				if (++i >= argc)
					usage ();
				safe_copy (state_G.buffer_color, argv[i], 50);
			} else if (!strncmp (arg_p, "-cache", 6)) {
				if (++i >= argc)
					usage ();
				safe_copy (state_G.cache_color, argv[i], 50);
			} else if (!strncmp (arg_p, "-swap", 5)) {
				if (++i >= argc)
					usage ();
				safe_copy (state_G.swap_color, argv[i], 50);
			} else if (!strncmp (arg_p, "-V", 2)) {
				version ();
				exit (0);
			} else if (!strncmp (arg_p, "-H", 2)) {
				version ();
				usage ();
			} else if (!strncmp (arg_p, "-h", 2)) {
				version ();
				usage ();
			}
			else {
				version ();
				usage ();
			}
		}
		else {
			version ();
			usage ();
		}
	}
}

int
main (int argc, char *argv[])
{
	defaults ();
	parse_cmdline (argc, argv);
	asmem_initialize (argc, argv, displayName_G, mainGeometry_G, withdrawn_G, iconic_G, pushedIn_G);
	while (1) {
		asmem_update ();
		usleep (X11_INTERVAL);
	}

	return 0;
}
