/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>

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
	printf ("-V | --version             print version and exit\n");
	printf ("-h | -H | --help           print this message and exit\n");
	printf ("-u | --update <secs>       the update interval in seconds\n");
	printf ("--mb                       the display is in MBytes\n");
	printf ("--used                     display used memory instead of free\n");
	printf ("--display <name>           the name of the display to use\n");
	printf ("--position <xy>            position on the screen (geometry)\n");
	printf ("--withdrawn                start in withdrawn shape (for WindowMaker)\n");
	printf ("--iconic                   start iconized\n");
	printf ("--standout                 standing out rather than being pushed in\n");
	printf ("--asis                     use free memory amount as read from device\n");
	printf ("--dev <device>             use the specified file as stat device\n");
	printf ("--bg <color>               background color\n");
	printf ("--fg <color>               base foreground color\n");
	printf ("--memory <color>           used memory bar color\n");
	printf ("--buffer <color>           buffer memory bar color\n");
	printf ("--cache <color>            cache memory bar color\n");
	printf ("--swap <color>             used swap space bar color\n");
	printf ("\n");
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
	struct option longOpts[] = {
		{"version", no_argument, NULL, 'V'},
		{"help", no_argument, NULL, 'h'},
		{"update", required_argument, NULL, 'u'},
		{"mb", no_argument, NULL, 0},
		{"used", no_argument, NULL, 1},
		{"display", required_argument, NULL, 2},
		{"position", required_argument, NULL, 3},
		{"withdrawn", no_argument, NULL, 4},
		{"iconic", no_argument, NULL, 5},
		{"standout", no_argument, NULL, 6},
		{"asis", no_argument, NULL, 7},
		{"dev", required_argument, NULL, 8},
		{"bg", required_argument, NULL, 9},
		{"fg", required_argument, NULL, 10},
		{"memory", required_argument, NULL, 11},
		{"buffer", required_argument, NULL, 12},
		{"cache", required_argument, NULL, 13},
		{"swap", required_argument, NULL, 14},
		{NULL, 0, NULL, 0},
	};

	while (1) {
		int c;

		c = getopt_long (argc, argv, "VhHu:", longOpts, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'V':
				version ();
				exit (0);

			case 'h':
			case 'H':
				usage ();
				exit (0);

			case 'u':
				state_G.update_interval = atoi (optarg);
				if (state_G.update_interval < 1)
					state_G.update_interval = CHK_INTERVAL;
				break;

			case 0:
				state_G.mb = 1;
				break;

			case 1:
				state_G.show_used = 1;
				break;

			case 2:
				safe_copy (displayName_G, optarg, sizeof (displayName_G));
				break;

			case 3:
				safe_copy (mainGeometry_G, optarg, sizeof (mainGeometry_G));
				break;

			case 4:
				withdrawn_G = 1;
				break;

			case 5:
				iconic_G = 1;
				break;

			case 6:
				pushedIn_G = 0;
				break;

			case 7:
				state_G.standard_free = 1;
				break;

			case 8:
				safe_copy (state_G.proc_mem_filename, optarg, sizeof (state_G.proc_mem_filename));
				break;

			case 9:
				safe_copy (state_G.bgcolor, optarg, sizeof (state_G.bgcolor));
				break;

			case 10:
				safe_copy (state_G.fgcolor, optarg, sizeof (state_G.fgcolor));
				break;

			case 11:
				safe_copy (state_G.memory_color, optarg, sizeof (state_G.memory_color));
				break;

			case 12:
				safe_copy (state_G.buffer_color, optarg, sizeof (state_G.buffer_color));
				break;

			case 13:
				safe_copy (state_G.cache_color, optarg, sizeof (state_G.cache_color));
				break;

			case 14:
				safe_copy (state_G.swap_color, optarg, sizeof (state_G.swap_color));
				break;
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
