/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>

#include "asmem.h"
#include "background.xpm"
#include "alphabet.xpm"

#include "config.h"

/*
 * default check and update intervals in microseconds
 *      x11 events - every 1/100 th of a second (in mks)
 *      Memory status - every second (in sec)
 */
#define X11_INTERVAL 10000L
#define CHK_INTERVAL 1
#define BUFFER_LENGTH 400

/* ------------------------------------------------------------------------- */
// prototypes
/* ------------------------------------------------------------------------- */
// general
static void defaults (void);
static void usage (void);
static void version (void);
static void parse_cmdline (int argc, char *argv[]);
static char* safe_copy (char *dest_p, const char *src_p, unsigned short maxlen);
static void cleanup (void);

// file handling
static int get_num (char *marker_p);
static bool read_meminfo (void);
static bool open_meminfo (void);
static void close_meminfo (void);

// x11
static Pixel x11_get_colour (char *colourName_p, Display *dpy_p, Window win);
static XColor x11_parse_colour (char *colourName_p, Display *dpy_p, Window win);
static char* x11_darken_char_colour (char *colourName_p, float rate, Display *dpy_p, Window win);
static Pixel x11_darken_colour (char *colourName_p, float rate, Display *dpy_p, Window win);
static char* x11_lighten_char_colour (char *colourName_p, float rate, Display *dpy_p, Window win);
static Pixel x11_lighten_colour (char *colourName_p, float rate, Display *dpy_p, Window win);
static void x11_draw_window (Window win);
static void x11_check_events (void);
static void x11_update (void);
static void x11_initialize (int argc, char *argv[]);

/* ------------------------------------------------------------------------- */
// globals
/* ------------------------------------------------------------------------- */
static AsmemState_t state_G;
static bool withdrawn_G = false;
static bool iconic_G = false;
static bool pushedIn_G = true;
static char displayName_G[50];
static char mainGeometry_G[50];
static FILE *procMeminfoFile_pG = NULL;
static char tmpChar_G[50];
static bool verbose_G = false;

/* X windows related global variables */
static Display *dpy_pG = 0; /* The display we are working on */
static Window rootWin_G; /* The root window of X11 */
static Window drawWin_G; /* A hidden window for drawing */
static Window mainWin_G; /* Application window */
static Window iconWin_G; /* Icon window */
static XGCValues mainGCV_G; /* graphics context values */
static GC mainGC_G; /* Graphics context */
static Atom wmDelWin_G;
static Atom wmProtocols_G;
static Pixel bgPix_G, fgPix_G;

/* pixmap stuff */
static char pgPixColour_G[4][50];
static char alphaColour_G[4][50];
static XpmIcon_t backgroundXpm_G;
static XpmIcon_t alphabetXpm_G;
static Pixel pix_G[4][3];

/* update stuff */
static time_t lastTime_G = 0;
static int updateRequest_G = 0;

/* ------------------------------------------------------------------------- */
// meat and potatoes
/* ------------------------------------------------------------------------- */
int
main (int argc, char *argv[])
{
	defaults ();
	parse_cmdline (argc, argv);
	x11_initialize (argc, argv);
	while (1) {
		x11_update ();
		usleep (X11_INTERVAL);
	}

	return 0;
}

/* ------------------------------------------------------------------------- */
// helpers
/* ------------------------------------------------------------------------- */
static void
defaults (void)
{
	state_G.updateInterval = CHK_INTERVAL;
	state_G.standardFree = false;
	state_G.mb = false;
	state_G.showUsed = false;
	safe_copy (state_G.procMemFilename, PROC_MEM, 256);
	withdrawn_G = false;
	iconic_G = false;
	pushedIn_G = true;
	safe_copy (displayName_G, "", 50);
	safe_copy (mainGeometry_G, "", 50);
	safe_copy (state_G.bgColor, "#303030", 50);
	safe_copy (state_G.fgColor, "#20b2aa", 50);
	safe_copy (state_G.memoryColor, "#4141d7", 50);
	safe_copy (state_G.bufferColor, "#aa80aa", 50);
	safe_copy (state_G.cacheColor, "#bebebe", 50);
	safe_copy (state_G.swapColor, "#ffa649", 50);
}

static void
usage (void)
{
	printf ("usage: asmem [options ...]\n\n");
	printf ("-V | --version             print version and exit\n");
	printf ("-h | -H | --help           print this message and exit\n");
	printf ("-v | --verbose             print debugging information\n");
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
		{"verbose", no_argument, NULL, 'v'},
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

		c = getopt_long (argc, argv, "VhHu:v", longOpts, NULL);
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

			case 'v':
				verbose_G = true;
				break;

			case 'u':
				state_G.updateInterval = atoi (optarg);
				if (state_G.updateInterval < 1)
					state_G.updateInterval = CHK_INTERVAL;
				break;

			case 0:
				state_G.mb = true;
				break;

			case 1:
				state_G.showUsed = true;
				break;

			case 2:
				safe_copy (displayName_G, optarg, sizeof (displayName_G));
				break;

			case 3:
				safe_copy (mainGeometry_G, optarg, sizeof (mainGeometry_G));
				break;

			case 4:
				withdrawn_G = true;
				break;

			case 5:
				iconic_G = true;
				break;

			case 6:
				pushedIn_G = false;
				break;

			case 7:
				state_G.standardFree = true;
				break;

			case 8:
				safe_copy (state_G.procMemFilename, optarg, sizeof (state_G.procMemFilename));
				break;

			case 9:
				safe_copy (state_G.bgColor, optarg, sizeof (state_G.bgColor));
				break;

			case 10:
				safe_copy (state_G.fgColor, optarg, sizeof (state_G.fgColor));
				break;

			case 11:
				safe_copy (state_G.memoryColor, optarg, sizeof (state_G.memoryColor));
				break;

			case 12:
				safe_copy (state_G.bufferColor, optarg, sizeof (state_G.bufferColor));
				break;

			case 13:
				safe_copy (state_G.cacheColor, optarg, sizeof (state_G.cacheColor));
				break;

			case 14:
				safe_copy (state_G.swapColor, optarg, sizeof (state_G.swapColor));
				break;
		}
	}
}

/*
 * Copies at most maxlen-1 characters from the source.
 * Makes sure that the destination string is zero-terminated.
 */
static char*
safe_copy (char *dest_p, const char *src_p, unsigned short maxlen)
{
	/* safety precaution */
	dest_p[maxlen-1] = 0;
	return strlen (src_p) < maxlen ? strcpy (dest_p, src_p) : strncpy (dest_p, src_p, maxlen-1);
}

static void
cleanup (void)
{
	if (dpy_pG)
		XCloseDisplay (dpy_pG);
	close_meminfo ();
}

/* ------------------------------------------------------------------------- */
// file routines
/* ------------------------------------------------------------------------- */
static int
get_num (char *marker_p)
{
	char thebuf[255];
	int done = 0;
	int theval;

	do {
		if (fgets (thebuf, sizeof (thebuf), procMeminfoFile_pG) == NULL) {
			printf ("file error\n");
			return (-1);
		}
		else
			if (strstr (thebuf, marker_p)) {
				sscanf (thebuf, "%*s %d %*s\n", &theval);
				return (theval);
			}
	} while (!done);
}

static bool
read_meminfo (void)
{
	int result;

	fflush (procMeminfoFile_pG);

	result = fseek (procMeminfoFile_pG, 0L, SEEK_SET);
	if (result < 0) {
		perror ("fseek()");
		return false;
	}

	state_G.fresh.total = get_num ("MemTotal") * 1024;
	state_G.fresh.free = get_num ("MemFree") * 1024;
	state_G.fresh.shared = 0;   /* this is always 0 */
	state_G.fresh.buffers = get_num ("Buffers") * 1024;
	state_G.fresh.cached = get_num ("Cached") * 1024;
	state_G.fresh.swapTotal = get_num ("SwapTotal") * 1024;
	state_G.fresh.swapFree = get_num ("SwapFree") * 1024;
	state_G.fresh.swapUsed = state_G.fresh.swapTotal - state_G.fresh.swapFree;
	state_G.fresh.used = state_G.fresh.total - state_G.fresh.free;

	return true;
}

static bool
open_meminfo (void)
{
	int result;
	if ((procMeminfoFile_pG = fopen (state_G.procMemFilename, "r")) == NULL) {
		perror ("fopen()");
		return false;
	}
	return true;
}

static void
close_meminfo (void)
{
	fclose (procMeminfoFile_pG);
}

/* ------------------------------------------------------------------------- */
// x11
/* ------------------------------------------------------------------------- */
/*
 * Note: this function was originally taken out of ascd.
 *
 * It takes the given color, parses it in the context
 * of the given window and returns a pixel of that color.
 */
static Pixel
x11_get_colour (char *colourName_p, Display *dpy_p, Window win)
{
	XColor colour;
	XWindowAttributes attr;

	XGetWindowAttributes (dpy_p, win, &attr);
	colour.pixel = 0;

	if (!XParseColor (dpy_p, attr.colormap, colourName_p, &colour))
		printf ("asmem: can't parse %s\n", colourName_p);
	else if (!XAllocColor (dpy_p, attr.colormap, &colour))
		printf ("asmem: can't allocate %s\n", colourName_p);

	return colour.pixel;
}

/*
 * Performs the same actions as x11_get_colour but
 * returns the complete XColor structure
 */
static XColor
x11_parse_colour (char *colourName_p, Display *dpy_p, Window win)
{
	XColor colour;
	XWindowAttributes attr;

	XGetWindowAttributes (dpy_p, win, &attr);
	colour.pixel = 0;

	if (!XParseColor (dpy_p, attr.colormap, colourName_p, &colour))
		printf ("asmem: can't parse %s\n", colourName_p);
	else if (!XAllocColor (dpy_p, attr.colormap, &colour))
		printf ("asmem: can't allocate %s\n", colourName_p);

	return colour;
}

/* darkens the given color using the supplied rate */
static char*
x11_darken_char_colour (char *colourName_p, float rate, Display *dpy_p, Window win)
{
	XColor tmpColour;

	if (verbose_G)
		printf ("darkening %s ->", colourName_p);
	tmpColour = x11_parse_colour (colourName_p, dpy_p, win);
	if (verbose_G)
		printf (" #%x %x %x ", tmpColour.red, tmpColour.green, tmpColour.blue);
	tmpColour.red = tmpColour.red / 257 / rate;
	tmpColour.green = tmpColour.green / 257 / rate;
	tmpColour.blue = tmpColour.blue / 257 / rate;
	sprintf (tmpChar_G, "#%.2x%.2x%.2x", (int)tmpColour.red, (int)tmpColour.green, (int)tmpColour.blue);
	if (verbose_G)
		printf ("-> %s\n", tmpChar_G);

	return tmpChar_G;
}

/* darkens the given color using the supplied rate */
static Pixel
x11_darken_colour (char *colourName_p, float rate, Display *dpy_p, Window win)
{
	return x11_get_colour (x11_darken_char_colour (colourName_p, rate, dpy_p, win), dpy_p, win);
}

/* lightens the given color using the supplied rate */
static char*
x11_lighten_char_colour (char *colourName_p, float rate, Display *dpy_p, Window win)
{
	XColor tmpColour;

	if (verbose_G)
		printf ("lightening %s ->", colourName_p);
	tmpColour = x11_parse_colour (colourName_p, dpy_p, win);
	if (verbose_G)
		printf (" #%x %x %x ", tmpColour.red, tmpColour.green, tmpColour.blue);
	tmpColour.red = tmpColour.red / 257 * rate;
	tmpColour.green = tmpColour.green / 257 * rate;
	tmpColour.blue = tmpColour.blue / 257 * rate;
	if (tmpColour.red > 255)
		tmpColour.red = 255;
	if (tmpColour.green > 255)
		tmpColour.green = 255;
	if (tmpColour.blue > 255)
		tmpColour.blue = 255;
	sprintf (tmpChar_G, "#%.2x%.2x%.2x", (int)tmpColour.red, (int)tmpColour.green, (int)tmpColour.blue);
	if (verbose_G)
		printf ("-> %s\n", tmpChar_G);

	return tmpChar_G;
}

/* lightens the given color using the supplied rate */
static Pixel
x11_lighten_colour (char *colourName_p, float rate, Display *dpy_p, Window win)
{
	return x11_get_colour (x11_lighten_char_colour (colourName_p, rate, dpy_p, win), dpy_p, win);
}

static void
x11_draw_window (Window win)
{
	int points[3];
	unsigned int total;
	unsigned int percentage;
	unsigned long int freeMem;
	unsigned int available;
	int i;
	unsigned int tmp[6];
	int digits;

	XCopyArea (dpy_pG, backgroundXpm_G.pixmap, win, mainGC_G, 0, 0, backgroundXpm_G.attributes.width, backgroundXpm_G.attributes.height, 0, 0);
	if ((state_G.fresh.total > (999999 * 1024)) || state_G.mb)
		total = state_G.fresh.total / 1024 / 1024;
	else
		total = state_G.fresh.total / 1024;
	digits = 0;
	for (i=0; i<6; ++i) {
		tmp[i] = total % 10;
		total = total / 10;
		++digits;
		if (total == 0)
			break;
	}
	for (i=0; i<digits; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, win, mainGC_G, tmp[i] * 5, 0, 6, 9, 46 - (i * 5), 2);

	if (state_G.standardFree)
		freeMem = state_G.fresh.free;
	else
		freeMem = state_G.fresh.free + state_G.fresh.buffers + state_G.fresh.cached;
	if (state_G.showUsed)
		freeMem = state_G.fresh.total - freeMem;
	if ((state_G.fresh.total > (999999 * 1024)) || state_G.mb)
		available = freeMem / 1024 / 1024;
	else
		available = freeMem / 1024;
	digits = 0;
	for (i=0; i<6; ++i) {
		tmp[i] = available % 10;
		available = available / 10;
		++digits;
		if (available == 0)
			break;
	}
	for (i=0; i<digits; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, win, mainGC_G, tmp[digits-1-i] * 5, 0, 6, 9, 2 + (i * 5), 17);

	if (state_G.fresh.total)
		percentage = (int)(((float)freeMem) / ((float)state_G.fresh.total) * 100);
	else
		percentage = 0;
	if (percentage >= 100)
		tmp[0] = percentage / 100;
	else
		tmp[0] = 13;
	if (percentage >= 10)
		tmp[1] = percentage % 100 / 10;
	else
		tmp[1] = 10;
	tmp[2] = percentage % 100 % 10;
	tmp[3] = 11;
	for (i=0; i<4; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, win, mainGC_G, tmp[i]*5, 0, 6, 9, 32 + (i * 5), 17);

	points[0] = ((float)(state_G.fresh.used - state_G.fresh.buffers - state_G.fresh.cached)) / ((float)state_G.fresh.total) * 46;
	points[1] = ((float)state_G.fresh.buffers) / ((float)state_G.fresh.total) * 46;
	points[2] = ((float)state_G.fresh.cached) / ((float)state_G.fresh.total) * 46;
	for (i=0; i<3; ++i) {
		mainGCV_G.foreground = pix_G[0][i];
		XChangeGC (dpy_pG, mainGC_G, GCForeground, &mainGCV_G);
		XFillRectangle (dpy_pG, win, mainGC_G, 3, 13 + i, points[0], 1);
	}
	for (i=0; i<3; ++i) {
		mainGCV_G.foreground = pix_G[1][i];
		XChangeGC (dpy_pG, mainGC_G, GCForeground, &mainGCV_G);
		XFillRectangle (dpy_pG, win, mainGC_G, 3 + points[0], 13 + i, points[1], 1);
	}
	for (i=0; i<3; ++i) {
		mainGCV_G.foreground = pix_G[2][i];
		XChangeGC (dpy_pG, mainGC_G, GCForeground, &mainGCV_G);
		XFillRectangle (dpy_pG, win, mainGC_G, 3 + points[0] + points[1], 13 + i, points[2], 1);
	}

	if ((state_G.fresh.swapTotal > (999999*1024)) || state_G.mb)
		total = state_G.fresh.swapTotal / 1024 / 1024;
	else
		total = state_G.fresh.swapTotal / 1024;
	digits = 0;
	for (i=0; i<6; ++i) {
		tmp[i] = total % 10;
		total = total / 10;
		++digits;
		if (total == 0)
			break;
	}
	for (i=0; i<digits; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, win, mainGC_G, tmp[i] * 5, 0, 6, 9, 46 - (i * 5), 27);

	freeMem = state_G.fresh.swapFree;
	if (state_G.showUsed)
		freeMem = state_G.fresh.swapTotal - freeMem;
	if ((freeMem > (999999 * 1024)) || state_G.mb)
		available = freeMem / 1024 / 1024;
	else
		available = freeMem / 1024;
	digits = 0;
	for (i=0; i<6; ++i) {
		tmp[i] = available % 10;
		available = available / 10;
		++digits;
		if (available == 0)
			break;
	}
	for (i=0; i<digits; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, win, mainGC_G, tmp[digits-1-i] * 5, 0, 6, 9, 2 + (i * 5), 42);

	if (state_G.fresh.swapTotal)
		percentage = (int) (((float)freeMem) / ((float)state_G.fresh.swapTotal) * 100);
	else
		percentage = 0;
	if (percentage >= 100)
		tmp[0] = percentage / 100;
	else
		tmp[0] = 13;
	if (percentage >= 10)
		tmp[1] = percentage % 100 / 10;
	else
		tmp[1] = 10;
	tmp[2] = percentage % 100 % 10;
	tmp[3] = 11;
	for (i=0; i<4; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, win, mainGC_G, tmp[i] * 5, 0, 6, 9, 32 + (i * 5), 42);

	points[0] = ((float)state_G.fresh.swapUsed) / ((float)state_G.fresh.swapTotal) * 46;
	for (i=0; i<3; ++i) {
		mainGCV_G.foreground = pix_G[3][i];
		XChangeGC (dpy_pG, mainGC_G, GCForeground, &mainGCV_G);
		XFillRectangle (dpy_pG, win, mainGC_G, 3, 38 + i, points[0], 1);
	}
}

/*
 * This checks for X11 events. We distinguish the following:
 * - request to repaint the window
 * - request to quit (Close button)
 */
static void
x11_check_events (void)
{
	XEvent event;

	while (XPending (dpy_pG)) {
		XNextEvent (dpy_pG, &event);
		switch (event.type) {
			case Expose:
				if (event.xexpose.count == 0)
					++updateRequest_G;
				break;
			case ClientMessage:
				if ((event.xclient.message_type == wmProtocols_G) && (event.xclient.data.l[0] == wmDelWin_G)) {
					if (verbose_G)
						printf ("caught wmDelWin_G, closing\n");
					cleanup ();
					exit (0);
				}
				break;
		}
	}
}

static void
asmem_redraw (void)
{
	XCopyArea (dpy_pG, drawWin_G, mainWin_G, mainGC_G, 0, 0, backgroundXpm_G.attributes.width, backgroundXpm_G.attributes.height, 0, 0);
	XCopyArea (dpy_pG, drawWin_G, iconWin_G, mainGC_G, 0, 0, backgroundXpm_G.attributes.width, backgroundXpm_G.attributes.height, 0, 0);

	updateRequest_G = 0;
}

static void
x11_update (void)
{
	time_t currentTime;

	currentTime = time (0);
	if (abs (currentTime - lastTime_G) >= state_G.updateInterval) {
		lastTime_G = currentTime;
		if (!read_meminfo ()) {
			cleanup ();
			exit (1);
		}
		if (memcmp (&state_G.last, &state_G.fresh, sizeof (AsmemMeminfo_t))) {
			memcpy (&state_G.last, &state_G.fresh, sizeof (AsmemMeminfo_t));
			x11_draw_window (drawWin_G);
			++updateRequest_G;
		}
	}

	x11_check_events ();
	if (updateRequest_G)
		asmem_redraw ();
}

static void
x11_initialize (int argc, char *argv[])
{
	int screen;
	Status status;
	XWindowAttributes winAttr;
	XSizeHints SizeHints;
	XTextProperty title;
	char *appName_p = "asmem";
	XClassHint classHint;
	int gravity;
	XWMHints WmHints;
	XEvent Event;
	int color_depth;
	int tmp;
	int result;
	int x_negative = 0;
	int y_negative = 0;

	dpy_pG = XOpenDisplay (displayName_G);
	if (!dpy_pG) {
		printf ("asmem : grrrr... can't open display %s. Sorry ...\n",
			XDisplayName (displayName_G));
		exit (1);
	}
	screen = DefaultScreen (dpy_pG);
	rootWin_G = RootWindow (dpy_pG, screen);
	bgPix_G = x11_get_colour (state_G.bgColor, dpy_pG, rootWin_G);
	fgPix_G = x11_get_colour (state_G.fgColor, dpy_pG, rootWin_G);
	color_depth = DefaultDepth (dpy_pG, screen);
	if (verbose_G)
		printf ("asmem : detected color depth %d bpp, using %d bpp\n", color_depth, color_depth);

	/* adjust the background pixmap */
	sprintf (pgPixColour_G[3], "# c %s", state_G.fgColor);
	if (pushedIn_G) {
		sprintf (pgPixColour_G[0], ". c %s", x11_darken_char_colour (state_G.bgColor, 1.6, dpy_pG, rootWin_G));
		sprintf (pgPixColour_G[1], "c c %s", state_G.bgColor);
		sprintf (pgPixColour_G[2], "q c %s", x11_lighten_char_colour (state_G.bgColor, 2.8, dpy_pG, rootWin_G));
	}
	else {
		sprintf (pgPixColour_G[2], "q c %s", x11_darken_char_colour (state_G.bgColor, 1.2, dpy_pG, rootWin_G));
		sprintf (pgPixColour_G[1], "c c %s", state_G.bgColor);
		sprintf (pgPixColour_G[0], ". c %s", x11_lighten_char_colour (state_G.bgColor, 2.5, dpy_pG, rootWin_G));
	}
	for (tmp=0; tmp<4; ++tmp)
		background[tmp+1] = pgPixColour_G[tmp];

	backgroundXpm_G.attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);
	status = XpmCreatePixmapFromData (dpy_pG, rootWin_G, background, &backgroundXpm_G.pixmap, &backgroundXpm_G.mask, &backgroundXpm_G.attributes);
	if (status != XpmSuccess) {
		printf ("asmem : (%d) not enough free color cells for background.\n", status);
		cleanup ();
		exit (1);
	}
	if (verbose_G)
		printf ("bg pixmap %d x %d\n", backgroundXpm_G.attributes.width, backgroundXpm_G.attributes.height);

	sprintf (alphaColour_G[0], ". c %s", state_G.bgColor);
	sprintf (alphaColour_G[1], "# c %s", state_G.fgColor);
	sprintf (alphaColour_G[2], "a c %s", x11_darken_char_colour (state_G.bgColor, 1.4, dpy_pG, rootWin_G));
	sprintf (alphaColour_G[3], "c c %s", x11_darken_char_colour (state_G.fgColor, 1.6, dpy_pG, rootWin_G));
	for (tmp=0; tmp<4; ++tmp)
		alphabet[tmp+1] = alphaColour_G[tmp];
	alphabetXpm_G.attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);
	status = XpmCreatePixmapFromData (dpy_pG, rootWin_G, alphabet, &alphabetXpm_G.pixmap, &alphabetXpm_G.mask, &alphabetXpm_G.attributes);
	if (status != XpmSuccess) {
		printf ("asmem : (%d) not enough free color cells for alphabet.\n", status);
		XCloseDisplay (dpy_pG);
		exit (1);
	}

	if (strlen (mainGeometry_G)) {
		/* Check the user-specified size */
		result = XParseGeometry (mainGeometry_G, &SizeHints.x, &SizeHints.y, &SizeHints.width, &SizeHints.height);
		if (result & XNegative)
			x_negative = 1;
		if (result & YNegative)
			y_negative = 1;
	}

	SizeHints.flags= USSize|USPosition;
	SizeHints.x = 0;
	SizeHints.y = 0;
	XWMGeometry (dpy_pG, screen, mainGeometry_G, NULL, 1, & SizeHints, &SizeHints.x, &SizeHints.y, &SizeHints.width, &SizeHints.height, &gravity);
	SizeHints.min_width = SizeHints.max_width = SizeHints.width = backgroundXpm_G.attributes.width;
	SizeHints.min_height = SizeHints.max_height = SizeHints.height= backgroundXpm_G.attributes.height;
	SizeHints.flags |= PMinSize|PMaxSize;

	/* Correct the offsets if the X/Y are negative */
	SizeHints.win_gravity = NorthWestGravity;
	if (x_negative) {
		SizeHints.x -= SizeHints.width;
		SizeHints.win_gravity = NorthEastGravity;
	}
	if (y_negative) {
		SizeHints.y -= SizeHints.height;
		if (x_negative)
			SizeHints.win_gravity = SouthEastGravity;
		else
			SizeHints.win_gravity = SouthWestGravity;
	}
	SizeHints.flags |= PWinGravity;

	drawWin_G = XCreatePixmap (dpy_pG, rootWin_G, SizeHints.width, SizeHints.height, color_depth);
	mainWin_G = XCreateSimpleWindow (dpy_pG, rootWin_G, SizeHints.x, SizeHints.y, SizeHints.width, SizeHints.height, 0, fgPix_G, bgPix_G);
	iconWin_G = XCreateSimpleWindow (dpy_pG, rootWin_G, SizeHints.x, SizeHints.y, SizeHints.width, SizeHints.height, 0, fgPix_G, bgPix_G);
	XSetWMNormalHints (dpy_pG, mainWin_G, &SizeHints);
	status = XClearWindow (dpy_pG, mainWin_G);

	status = XGetWindowAttributes (dpy_pG, mainWin_G, &winAttr);
	status = XSetWindowBackgroundPixmap (dpy_pG, mainWin_G, backgroundXpm_G.pixmap);
	status = XSetWindowBackgroundPixmap (dpy_pG, iconWin_G, backgroundXpm_G.pixmap);

	status = XStringListToTextProperty (&appName_p, 1, &title);
	XSetWMName (dpy_pG, mainWin_G, &title);
	XSetWMName (dpy_pG, iconWin_G, &title);

	classHint.res_name = "asmem" ;
	classHint.res_class = "ASMEM";
	XSetClassHint (dpy_pG, mainWin_G, &classHint);
	XStoreName (dpy_pG, mainWin_G, "asmem");
	XSetIconName (dpy_pG, mainWin_G, "asmem");

	status = XSelectInput (dpy_pG, mainWin_G, ExposureMask);
	status = XSelectInput (dpy_pG, iconWin_G, ExposureMask);

	/* Creating Graphics context */
	mainGCV_G.foreground = fgPix_G;
	mainGCV_G.background = bgPix_G;
	mainGCV_G.graphics_exposures = False;
	mainGCV_G.line_style = LineSolid;
	mainGCV_G.fill_style = FillSolid;
	mainGCV_G.line_width = 1;
	mainGC_G = XCreateGC (dpy_pG, mainWin_G, GCForeground|GCBackground|GCLineWidth|GCLineStyle|GCFillStyle, &mainGCV_G);

	status = XSetCommand (dpy_pG, mainWin_G, argv, argc);

	/* Set up the event for quitting the window */
	wmDelWin_G = XInternAtom (dpy_pG, "WM_DELETE_WINDOW", False);
	wmProtocols_G = XInternAtom (dpy_pG, "WM_PROTOCOLS", False);
	status = XSetWMProtocols (dpy_pG, mainWin_G, &wmDelWin_G, 1);
	status = XSetWMProtocols (dpy_pG, iconWin_G, &wmDelWin_G, 1);

	WmHints.flags = StateHint | IconWindowHint;
	WmHints.initial_state = withdrawn_G ? WithdrawnState : iconic_G ? IconicState : NormalState;
	WmHints.icon_window = iconWin_G;
	if (withdrawn_G) {
		WmHints.window_group = mainWin_G;
		WmHints.flags |= WindowGroupHint;
	}
	if (iconic_G || withdrawn_G) {
		WmHints.icon_x = SizeHints.x;
		WmHints.icon_y = SizeHints.y;
		WmHints.flags |= IconPositionHint;
	}
	XSetWMHints (dpy_pG, mainWin_G, &WmHints);

	/* Finally show the window */
	status = XMapWindow (dpy_pG, mainWin_G);

	/* Get colors while waiting for Expose */
	pix_G[0][0] = x11_lighten_colour (state_G.memoryColor, 1.4, dpy_pG, mainWin_G);
	pix_G[0][1] = x11_get_colour (state_G.memoryColor, dpy_pG, mainWin_G);
	pix_G[0][2] = x11_darken_colour (state_G.memoryColor, 1.4, dpy_pG, mainWin_G);
	pix_G[1][0] = x11_lighten_colour (state_G.bufferColor, 1.4, dpy_pG, mainWin_G);
	pix_G[1][1] = x11_get_colour (state_G.bufferColor, dpy_pG, mainWin_G);
	pix_G[1][2] = x11_darken_colour (state_G.bufferColor, 1.4, dpy_pG, mainWin_G);
	pix_G[2][0] = x11_lighten_colour (state_G.cacheColor, 1.4, dpy_pG, mainWin_G);
	pix_G[2][1] = x11_get_colour (state_G.cacheColor, dpy_pG, mainWin_G);
	pix_G[2][2] = x11_darken_colour (state_G.cacheColor, 1.4, dpy_pG, mainWin_G);
	pix_G[3][0] = x11_lighten_colour (state_G.swapColor, 1.4, dpy_pG, mainWin_G);
	pix_G[3][1] = x11_get_colour (state_G.swapColor, dpy_pG, mainWin_G);
	pix_G[3][2] = x11_darken_colour (state_G.swapColor, 1.4, dpy_pG, mainWin_G);

	if (!open_meminfo ()) {
		cleanup ();
		exit (1);
	}
	if (!read_meminfo ()) {
		cleanup ();
		exit (1);
	}

	/* wait for the Expose event now */
	XNextEvent (dpy_pG, &Event);
	/* We 've got Expose -> draw the parts of the window. */
	asmem_redraw ();
	XFlush (dpy_pG);
}
