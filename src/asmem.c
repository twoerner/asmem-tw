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
#include <poll.h>
#include <getopt.h>
#include <libgen.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>

#include "asmem.h"
#include "background.xpm"
#include "alphabet.xpm"

#include "config.h"

// update frequency [ms]
#define DEFAULT_INTERVAL 2000
static int updateInterval_G = DEFAULT_INTERVAL;

#define STRSZ 32
#define FNAMESZ 256
#define WIDTH_PADDING 6
#define MAXDIGITS 10

#define VERBOSE(fmt, ...) \
	if (verbose_G) { \
		char *fname_p = basename (__FILE__); \
		char *newfmt_p = (char*)malloc (strlen (fmt) + 32); \
		if (newfmt_p) { \
			strcpy (newfmt_p, "[%s:%s():%u] "); \
			strcat (newfmt_p, fmt); \
			fprintf (stderr, newfmt_p, fname_p, __func__, __LINE__, ##__VA_ARGS__); \
			free (newfmt_p); \
		} \
	}

/* ------------------------------------------------------------------------- */
// prototypes
/* ------------------------------------------------------------------------- */
// general
static void set_defaults (void);
static void print_usage (void);
static void print_version (void);
static void parse_cmdline (int argc, char *argv[]);
static char* safe_copy (char *dest_p, const char *src_p, unsigned short maxlen);
static void cleanup (void);

// file handling
static unsigned long get_num (char *marker_p);
static bool read_meminfo (void);
static bool open_meminfo (void);
static void close_meminfo (void);
static void meminfo_update (void);

// x11
static Pixel x11_get_colour (char *colourName_p, Window win);
static XColor x11_parse_colour (char *colourName_p, Window win);
static char* x11_darken_char_colour (char *colourName_p, float rate, Window win);
static Pixel x11_darken_colour (char *colourName_p, float rate, Window win);
static char* x11_lighten_char_colour (char *colourName_p, float rate, Window win);
static Pixel x11_lighten_colour (char *colourName_p, float rate, Window win);
static void x11_draw_offscreen_win (void);
static void x11_draw_main_win_from_offscreen (void);
static void x11_check_events (void);
static void x11_initialize (int argc, char *argv[]);

/* ------------------------------------------------------------------------- */
// globals
/* ------------------------------------------------------------------------- */
static AsmemMeminfo_t last_G;
static AsmemMeminfo_t fresh_G;
static char displayName_G[STRSZ];
static char mainGeometry_G[STRSZ];
static FILE *procMeminfoFile_pG = NULL;
static char procMemFilename_G[FNAMESZ];
static char tmpChar_G[STRSZ];
static bool verbose_G = false;
static bool visible_G = true;

static char bgColour_G[STRSZ];
static char fgColour_G[STRSZ];
static char memoryColour_G[STRSZ];
static char bufferColour_G[STRSZ];
static char cacheColour_G[STRSZ];
static char swapColour_G[STRSZ];

static Display *dpy_pG = 0;
static Window rootWin_G;
static Window drawWin_G;
static Window mainWin_G;
static Window iconWin_G;
static XGCValues mainGCV_G;
static GC mainGC_G;
static Atom wmDelWin_G;
static Atom wmProtocols_G;
static Pixel bgPix_G, fgPix_G;

static char pgPixColour_G[4][STRSZ];
static char alphaColour_G[4][STRSZ];
static XpmIcon_t backgroundXpm_G;
static XpmIcon_t alphabetXpm_G;

// first index, what the colour is for
#define cMEM 0 // memory
#define cBUF 1 // buffer
#define cCHE 2 // cache
#define cSWP 3 // swap
// second index, the colour's hue
#define cLGT 0 // light
#define cREG 1 // regular
#define cDRK 2 // dark
static Pixel pix_G[4][3];

/* ------------------------------------------------------------------------- */
// meat and potatoes
/* ------------------------------------------------------------------------- */
int
main (int argc, char *argv[])
{
	int xfd;
	int timeout;

	set_defaults ();
	parse_cmdline (argc, argv);
	x11_initialize (argc, argv);

	xfd = ConnectionNumber (dpy_pG);
	if (xfd == 0) {
		printf ("warning: can't obtain connection number, redraws timed with updates\n");
		while (1) {
			meminfo_update ();
			usleep ((useconds_t)updateInterval_G * 1000);
		}
	}
	else {
		int rtn;
		struct pollfd fds[1];

		memset (fds, 0, sizeof (fds));
		fds[0].fd = xfd;
		fds[0].events = POLLIN;

		while (1) {
			timeout = updateInterval_G;
			if (!visible_G)
				timeout = -1;

			rtn = poll (fds, 1, timeout);
			if (rtn == -1) {
				perror ("poll()");
				continue;
			}
			if (rtn == 0) {
				meminfo_update ();
				continue;
			}
			x11_check_events ();
		}
	}

	cleanup ();
	return 0;
}

/* ------------------------------------------------------------------------- */
// helpers
/* ------------------------------------------------------------------------- */
static void
set_defaults (void)
{
	safe_copy (procMemFilename_G, PROC_MEM, sizeof (procMemFilename_G));
	safe_copy (displayName_G, "", sizeof (displayName_G));
	safe_copy (mainGeometry_G, "", sizeof (mainGeometry_G));
	safe_copy (bgColour_G, "#303030", sizeof (bgColour_G));
	safe_copy (fgColour_G, "#20b2aa", sizeof (fgColour_G));
	safe_copy (memoryColour_G, "#4141d7", sizeof (memoryColour_G));
	safe_copy (bufferColour_G, "#aa80aa", sizeof (bufferColour_G));
	safe_copy (cacheColour_G, "#bebebe", sizeof (cacheColour_G));
	safe_copy (swapColour_G, "#ffa649", sizeof (swapColour_G));
}

static void
print_usage (void)
{
	printf ("usage: asmem [options ...]\n\n");
	printf ("-V | --version             print version and exit\n");
	printf ("-h | -H | --help           print this message and exit\n");
	printf ("-v | --verbose             print debugging information\n");
	printf ("-u | --update <secs>       the update interval in seconds\n");
	printf ("--display <name>           the name of the display to use\n");
	printf ("--position <xy>            position on the screen (geometry)\n");
	printf ("--dev <device>             use the specified file as stat device\n");
	printf ("--bg <colour>              background colour\n");
	printf ("--fg <colour>              base foreground colour\n");
	printf ("--memory <colour>          used memory bar colour\n");
	printf ("--buffer <colour>          buffer memory bar colour\n");
	printf ("--cache <colour>           cache memory bar colour\n");
	printf ("--swap <colour>            used swap space bar colour\n");
	printf ("\n");
}

static void
print_version (void)
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
		{"display", required_argument, NULL, 0},
		{"position", required_argument, NULL, 1},
		{"dev", required_argument, NULL, 2},
		{"bg", required_argument, NULL, 3},
		{"fg", required_argument, NULL, 4},
		{"memory", required_argument, NULL, 5},
		{"buffer", required_argument, NULL, 6},
		{"cache", required_argument, NULL, 7},
		{"swap", required_argument, NULL, 8},
		{NULL, 0, NULL, 0},
	};

	while (1) {
		int c;

		c = getopt_long (argc, argv, "VhHu:v", longOpts, NULL);
		if (c == -1)
			break;

		switch (c) {
			case 'V':
				print_version ();
				exit (0);

			case 'h':
			case 'H':
				print_usage ();
				exit (0);

			case 'v':
				verbose_G = true;
				break;

			case 'u':
				updateInterval_G = atoi (optarg);
				if (updateInterval_G < 1)
					updateInterval_G = DEFAULT_INTERVAL;
				break;

			case 0:
				safe_copy (displayName_G, optarg, sizeof (displayName_G));
				break;

			case 1:
				safe_copy (mainGeometry_G, optarg, sizeof (mainGeometry_G));
				break;

			case 2:
				safe_copy (procMemFilename_G, optarg, sizeof (procMemFilename_G));
				break;

			case 3:
				safe_copy (bgColour_G, optarg, sizeof (bgColour_G));
				break;

			case 4:
				safe_copy (fgColour_G, optarg, sizeof (fgColour_G));
				break;

			case 5:
				safe_copy (memoryColour_G, optarg, sizeof (memoryColour_G));
				break;

			case 6:
				safe_copy (bufferColour_G, optarg, sizeof (bufferColour_G));
				break;

			case 7:
				safe_copy (cacheColour_G, optarg, sizeof (cacheColour_G));
				break;

			case 8:
				safe_copy (swapColour_G, optarg, sizeof (swapColour_G));
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
static unsigned long
get_num (char *marker_p)
{
	char buf[255];
	unsigned long val;

	while (true) {
		if (fgets (buf, sizeof (buf), procMeminfoFile_pG) == NULL) {
			perror ("fgets()");
			return (unsigned long)-1;
		}

		if (strstr (buf, marker_p)) {
			sscanf (buf, "%*s %lu %*s\n", &val);
			VERBOSE ("get_num() marker_p:%s val:%lu\n", marker_p, val);
			return val;
		}
	}

	return (unsigned long)-1;
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

	fresh_G.memTotal = get_num ("MemTotal") / 1000;
	fresh_G.memFree = get_num ("MemFree") / 1000;
	fresh_G.memBuffers = get_num ("Buffers") / 1000;
	fresh_G.memCached = get_num ("Cached") / 1000;
	fresh_G.swapTotal = get_num ("SwapTotal") / 1000;
	fresh_G.swapFree = get_num ("SwapFree") / 1000;

	fresh_G.memUsed = fresh_G.memTotal - fresh_G.memFree;
	fresh_G.swapUsed = fresh_G.swapTotal - fresh_G.swapFree;

	return true;
}

static bool
open_meminfo (void)
{
	if ((procMeminfoFile_pG = fopen (procMemFilename_G, "r")) == NULL) {
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

static void
meminfo_update (void)
{
	static bool firstTime = true;
	bool different;

	if (!read_meminfo ()) {
		cleanup ();
		exit (1);
	}

	if (firstTime) {
		firstTime = false;
		different = true;
	}
	else
		different = memcmp (&last_G, &fresh_G, sizeof (AsmemMeminfo_t));

	if (different) {
		memcpy (&last_G, &fresh_G, sizeof (AsmemMeminfo_t));
		x11_draw_offscreen_win ();
		x11_draw_main_win_from_offscreen ();
	}
}

/* ------------------------------------------------------------------------- */
// x11
/* ------------------------------------------------------------------------- */
/*
 * Note: this function was originally taken out of ascd.
 *
 * It takes the given colour, parses it in the context
 * of the given window and returns a pixel of that colour.
 */
static Pixel
x11_get_colour (char *colourName_p, Window win)
{
	XColor colour;
	XWindowAttributes attr;

	XGetWindowAttributes (dpy_pG, win, &attr);
	colour.pixel = 0;

	if (!XParseColor (dpy_pG, attr.colormap, colourName_p, &colour))
		printf ("asmem: can't parse %s\n", colourName_p);
	else if (!XAllocColor (dpy_pG, attr.colormap, &colour))
		printf ("asmem: can't allocate %s\n", colourName_p);

	return colour.pixel;
}

/*
 * Performs the same actions as x11_get_colour() but
 * returns the complete XColor structure
 */
static XColor
x11_parse_colour (char *colourName_p, Window win)
{
	XColor colour;
	XWindowAttributes attr;

	XGetWindowAttributes (dpy_pG, win, &attr);
	colour.pixel = 0;

	if (!XParseColor (dpy_pG, attr.colormap, colourName_p, &colour))
		printf ("asmem: can't parse %s\n", colourName_p);
	else if (!XAllocColor (dpy_pG, attr.colormap, &colour))
		printf ("asmem: can't allocate %s\n", colourName_p);

	return colour;
}

/* darkens the given colour using the supplied rate */
static char*
x11_darken_char_colour (char *colourName_p, float rate, Window win)
{
	XColor tmpColour;

	VERBOSE ("darkening %s ->", colourName_p);
	tmpColour = x11_parse_colour (colourName_p, win);
	VERBOSE (" #%x %x %x ", tmpColour.red, tmpColour.green, tmpColour.blue);
	tmpColour.red = tmpColour.red / 257 / rate;
	tmpColour.green = tmpColour.green / 257 / rate;
	tmpColour.blue = tmpColour.blue / 257 / rate;
	sprintf (tmpChar_G, "#%.2x%.2x%.2x", (int)tmpColour.red, (int)tmpColour.green, (int)tmpColour.blue);
	VERBOSE ("-> %s\n", tmpChar_G);

	return tmpChar_G;
}

/* darkens the given colour using the supplied rate */
static Pixel
x11_darken_colour (char *colourName_p, float rate, Window win)
{
	return x11_get_colour (x11_darken_char_colour (colourName_p, rate, win), win);
}

/* lightens the given colour using the supplied rate */
static char*
x11_lighten_char_colour (char *colourName_p, float rate, Window win)
{
	XColor tmpColour;

	VERBOSE ("lightening %s ->", colourName_p);
	tmpColour = x11_parse_colour (colourName_p, win);
	VERBOSE (" #%x %x %x ", tmpColour.red, tmpColour.green, tmpColour.blue);
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
	VERBOSE ("-> %s\n", tmpChar_G);

	return tmpChar_G;
}

/* lightens the given colour using the supplied rate */
static Pixel
x11_lighten_colour (char *colourName_p, float rate, Window win)
{
	return x11_get_colour (x11_lighten_char_colour (colourName_p, rate, win), win);
}

static void
x11_draw_offscreen_win (void)
{
	int points[3];
	unsigned val;
	unsigned tmp[MAXDIGITS];
	unsigned i, digitCnt;
	int winWidth;

	VERBOSE ("\n");

	// figure out the window width (based on xpm size)
	winWidth = (int)backgroundXpm_G.attributes.width - WIDTH_PADDING;

	// paint background
	XCopyArea (dpy_pG, backgroundXpm_G.pixmap, drawWin_G, mainGC_G, 0, 0, backgroundXpm_G.attributes.width, backgroundXpm_G.attributes.height, 0, 0);

	// string of total memory
	val = fresh_G.memTotal;
	digitCnt = 0;
	for (i=0; i<MAXDIGITS; ++i) {
		tmp[i] = val % 10;
		val /= 10;
		++digitCnt;
		if (val == 0)
			break;
	}
	for (i=0; i<digitCnt; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, drawWin_G, mainGC_G, tmp[i] * 5, 0, 6, 9, winWidth - (i * 5), 2);

	// string of memory used
	val = fresh_G.memTotal - fresh_G.memFree;
	digitCnt = 0;
	for (i=0; i<MAXDIGITS; ++i) {
		tmp[i] = val % 10;
		val /= 10;
		++digitCnt;
		if (val == 0)
			break;
	}
	for (i=0; i<digitCnt; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, drawWin_G, mainGC_G, tmp[digitCnt-1-i] * 5, 0, 6, 9, 2 + (i * 5), 17);

	// string of percentage memory used
	val = (int)((((float)fresh_G.memTotal) - ((float)fresh_G.memFree)) / ((float)fresh_G.memTotal) * 100);
	if (val >= 100)
		tmp[0] = val / 100;
	else
		tmp[0] = 13;
	if (val >= 10)
		tmp[1] = val % 100 / 10;
	else
		tmp[1] = 10;
	tmp[2] = val % 100 % 10;
	tmp[3] = 11;
	for (i=0; i<4; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, drawWin_G, mainGC_G, tmp[i]*5, 0, 6, 9, 32 + (i * 5), 17);

	// draw the memory bar
	points[0] = ((float)(fresh_G.memUsed - fresh_G.memBuffers - fresh_G.memCached)) / ((float)fresh_G.memTotal) * winWidth;
	points[1] = ((float)fresh_G.memBuffers) / ((float)fresh_G.memTotal) * winWidth;
	points[2] = ((float)fresh_G.memCached) / ((float)fresh_G.memTotal) * winWidth;
	for (i=0; i<3; ++i) {
		mainGCV_G.foreground = pix_G[cMEM][i];
		XChangeGC (dpy_pG, mainGC_G, GCForeground, &mainGCV_G);
		XFillRectangle (dpy_pG, drawWin_G, mainGC_G, 3, 13 + i, points[0], 1);
	}
	for (i=0; i<3; ++i) {
		mainGCV_G.foreground = pix_G[cBUF][i];
		XChangeGC (dpy_pG, mainGC_G, GCForeground, &mainGCV_G);
		XFillRectangle (dpy_pG, drawWin_G, mainGC_G, 3 + points[0], 13 + i, points[1], 1);
	}
	for (i=0; i<3; ++i) {
		mainGCV_G.foreground = pix_G[cCHE][i];
		XChangeGC (dpy_pG, mainGC_G, GCForeground, &mainGCV_G);
		XFillRectangle (dpy_pG, drawWin_G, mainGC_G, 3 + points[0] + points[1], 13 + i, points[2], 1);
	}

	// string of swap total
	val = fresh_G.swapTotal;
	digitCnt = 0;
	for (i=0; i<MAXDIGITS; ++i) {
		tmp[i] = val % 10;
		val /= 10;
		++digitCnt;
		if (val == 0)
			break;
	}
	for (i=0; i<digitCnt; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, drawWin_G, mainGC_G, tmp[i] * 5, 0, 6, 9, winWidth - (i * 5), 27);

	// string of swap used
	val = fresh_G.swapTotal - fresh_G.swapFree;
	digitCnt = 0;
	for (i=0; i<MAXDIGITS; ++i) {
		tmp[i] = val % 10;
		val /= 10;
		++digitCnt;
		if (val == 0)
			break;
	}
	for (i=0; i<digitCnt; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, drawWin_G, mainGC_G, tmp[digitCnt-1-i] * 5, 0, 6, 9, 2 + (i * 5), 42);

	// string of percentage swap used
	val = (int) ((((float)fresh_G.swapTotal) - ((float)fresh_G.swapFree)) / ((float)fresh_G.swapTotal) * 100);
	if (val >= 100)
		tmp[0] = val / 100;
	else
		tmp[0] = 13;
	if (val >= 10)
		tmp[1] = val % 100 / 10;
	else
		tmp[1] = 10;
	tmp[2] = val % 100 % 10;
	tmp[3] = 11;
	for (i=0; i<4; ++i)
		XCopyArea (dpy_pG, alphabetXpm_G.pixmap, drawWin_G, mainGC_G, tmp[i] * 5, 0, 6, 9, 32 + (i * 5), 42);

	// draw swap bar
	points[0] = ((float)fresh_G.swapUsed) / ((float)fresh_G.swapTotal) * winWidth;
	for (i=0; i<3; ++i) {
		mainGCV_G.foreground = pix_G[cSWP][i];
		XChangeGC (dpy_pG, mainGC_G, GCForeground, &mainGCV_G);
		XFillRectangle (dpy_pG, drawWin_G, mainGC_G, 3, 38 + i, points[0], 1);
	}
}

static void
x11_check_events (void)
{
	XEvent event;

	VERBOSE ("\n");

	while (XPending (dpy_pG)) {
		XNextEvent (dpy_pG, &event);
		switch (event.type) {
			case Expose:
				if (event.xexpose.count == 0)
					x11_draw_main_win_from_offscreen ();
				break;

			case ClientMessage:
				if ((event.xclient.message_type == wmProtocols_G) && (event.xclient.data.l[0] == wmDelWin_G)) {
					VERBOSE ("caught wmDelWin_G, closing\n");
					cleanup ();
					exit (0);
				}
				break;

			case VisibilityNotify:
				VERBOSE ("visibility state: %d\n", event.xvisibility.state);
				visible_G = ((event.xvisibility.state == VisibilityFullyObscured)? false : true);
				break;

			default:
				if (event.type != NoExpose)
					printf ("unhandled X11 event: %d\n", event.type);
				break;
		}
	}
}

static void
x11_draw_main_win_from_offscreen (void)
{
	VERBOSE ("\n");
	XCopyArea (dpy_pG, drawWin_G, mainWin_G, mainGC_G, 0, 0, backgroundXpm_G.attributes.width, backgroundXpm_G.attributes.height, 0, 0);
	XCopyArea (dpy_pG, drawWin_G, iconWin_G, mainGC_G, 0, 0, backgroundXpm_G.attributes.width, backgroundXpm_G.attributes.height, 0, 0);
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
	int colourDepth;
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
	bgPix_G = x11_get_colour (bgColour_G, rootWin_G);
	fgPix_G = x11_get_colour (fgColour_G, rootWin_G);
	colourDepth = DefaultDepth (dpy_pG, screen);
	VERBOSE ("asmem : detected colour depth %d bpp, using %d bpp\n", colourDepth, colourDepth);

	// adjust the background pixmap
	sprintf (pgPixColour_G[3], "# c %s", fgColour_G);
	sprintf (pgPixColour_G[2], "q c %s", x11_darken_char_colour (bgColour_G, 1.2, rootWin_G));
	sprintf (pgPixColour_G[1], "c c %s", bgColour_G);
	sprintf (pgPixColour_G[0], ". c %s", x11_lighten_char_colour (bgColour_G, 2.5, rootWin_G));
	for (tmp=0; tmp<4; ++tmp)
		background[tmp+1] = pgPixColour_G[tmp];

	backgroundXpm_G.attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);
	status = XpmCreatePixmapFromData (dpy_pG, rootWin_G, background, &backgroundXpm_G.pixmap, &backgroundXpm_G.mask, &backgroundXpm_G.attributes);
	if (status != XpmSuccess) {
		printf ("asmem : (%d) not enough free colour cells for background.\n", status);
		cleanup ();
		exit (1);
	}
	VERBOSE ("bg pixmap %d x %d\n", backgroundXpm_G.attributes.width, backgroundXpm_G.attributes.height);

	sprintf (alphaColour_G[0], ". c %s", bgColour_G);
	sprintf (alphaColour_G[1], "# c %s", fgColour_G);
	sprintf (alphaColour_G[2], "a c %s", x11_darken_char_colour (bgColour_G, 1.4, rootWin_G));
	sprintf (alphaColour_G[3], "c c %s", x11_darken_char_colour (fgColour_G, 1.6, rootWin_G));
	for (tmp=0; tmp<4; ++tmp)
		alphabet[tmp+1] = alphaColour_G[tmp];
	alphabetXpm_G.attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);
	status = XpmCreatePixmapFromData (dpy_pG, rootWin_G, alphabet, &alphabetXpm_G.pixmap, &alphabetXpm_G.mask, &alphabetXpm_G.attributes);
	if (status != XpmSuccess) {
		printf ("asmem : (%d) not enough free colour cells for alphabet.\n", status);
		XCloseDisplay (dpy_pG);
		exit (1);
	}

	if (strlen (mainGeometry_G)) {
		// check the user-specified size
		result = XParseGeometry (mainGeometry_G, &SizeHints.x, &SizeHints.y, &SizeHints.width, &SizeHints.height);
		if (result & XNegative)
			x_negative = 1;
		if (result & YNegative)
			y_negative = 1;
	}

	SizeHints.flags= USSize | USPosition;
	SizeHints.x = 0;
	SizeHints.y = 0;
	XWMGeometry (dpy_pG, screen, mainGeometry_G, NULL, 1, & SizeHints, &SizeHints.x, &SizeHints.y, &SizeHints.width, &SizeHints.height, &gravity);
	SizeHints.min_width = SizeHints.max_width = SizeHints.width = backgroundXpm_G.attributes.width;
	SizeHints.min_height = SizeHints.max_height = SizeHints.height= backgroundXpm_G.attributes.height;
	SizeHints.flags |= PMinSize|PMaxSize;

	// correct the offsets if the X/Y are negative
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

	drawWin_G = XCreatePixmap (dpy_pG, rootWin_G, SizeHints.width, SizeHints.height, colourDepth);
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

	status = XSelectInput (dpy_pG, mainWin_G, ExposureMask | VisibilityChangeMask);
	status = XSelectInput (dpy_pG, iconWin_G, ExposureMask);

	// creating GC
	mainGCV_G.foreground = fgPix_G;
	mainGCV_G.background = bgPix_G;
	mainGCV_G.graphics_exposures = False;
	mainGCV_G.line_style = LineSolid;
	mainGCV_G.fill_style = FillSolid;
	mainGCV_G.line_width = 1;
	mainGC_G = XCreateGC (dpy_pG, mainWin_G, GCForeground|GCBackground|GCLineWidth|GCLineStyle|GCFillStyle, &mainGCV_G);

	status = XSetCommand (dpy_pG, mainWin_G, argv, argc);

	// set up the event for quitting the window
	wmDelWin_G = XInternAtom (dpy_pG, "WM_DELETE_WINDOW", False);
	wmProtocols_G = XInternAtom (dpy_pG, "WM_PROTOCOLS", False);
	status = XSetWMProtocols (dpy_pG, mainWin_G, &wmDelWin_G, 1);
	status = XSetWMProtocols (dpy_pG, iconWin_G, &wmDelWin_G, 1);

	WmHints.flags = StateHint | IconWindowHint;
	WmHints.initial_state = NormalState;
	WmHints.icon_window = iconWin_G;
	XSetWMHints (dpy_pG, mainWin_G, &WmHints);

	// finally show the window
	status = XMapWindow (dpy_pG, mainWin_G);

	// get colours while waiting for Expose
	pix_G[cMEM][cLGT] = x11_lighten_colour (memoryColour_G, 1.4, mainWin_G);
	pix_G[cMEM][cREG] = x11_get_colour (memoryColour_G, mainWin_G);
	pix_G[cMEM][cDRK] = x11_darken_colour (memoryColour_G, 1.4, mainWin_G);
	pix_G[cBUF][cLGT] = x11_lighten_colour (bufferColour_G, 1.4, mainWin_G);
	pix_G[cBUF][cREG] = x11_get_colour (bufferColour_G, mainWin_G);
	pix_G[cBUF][cDRK] = x11_darken_colour (bufferColour_G, 1.4, mainWin_G);
	pix_G[cCHE][cLGT] = x11_lighten_colour (cacheColour_G, 1.4, mainWin_G);
	pix_G[cCHE][cREG] = x11_get_colour (cacheColour_G, mainWin_G);
	pix_G[cCHE][cDRK] = x11_darken_colour (cacheColour_G, 1.4, mainWin_G);
	pix_G[cSWP][cLGT] = x11_lighten_colour (swapColour_G, 1.4, mainWin_G);
	pix_G[cSWP][cREG] = x11_get_colour (swapColour_G, mainWin_G);
	pix_G[cSWP][cDRK] = x11_darken_colour (swapColour_G, 1.4, mainWin_G);

	if (!open_meminfo ()) {
		cleanup ();
		exit (1);
	}
	if (!read_meminfo ()) {
		cleanup ();
		exit (1);
	}

	// wait for the Expose event now
	XNextEvent (dpy_pG, &Event);

	// we've got Expose -> draw the parts of the window
	meminfo_update ();
	x11_draw_main_win_from_offscreen ();
	XFlush (dpy_pG);
}
