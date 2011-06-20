/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>
#include <X11/Xatom.h>

#include "x_color.h"
#include "read_mem.h"
#include "background.xpm"
#include "alphabet.xpm"
#include "state.h"

AsmemState_t state_G;

/* nice idea from ascd */
typedef struct _XpmIcon {
	Pixmap pixmap;
	Pixmap mask;
	XpmAttributes attributes;
} XpmIcon_t;

static XpmIcon_t backgroundXpm_G;
static XpmIcon_t alphabetXpm_G;

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

/* background pixmap colors */
static char pgPixColour_G[4][50];
static char alphaColour_G[4][50];

/* pixels we need */
static Pixel pix_G[4][3];

/* last time we updated */
static time_t lastTime_G = 0;

/* requests for update */
static int updateRequest_G = 0;

static void
draw_window (Window win)
{
	int points[3];
	unsigned int total;
	unsigned int percentage;
	unsigned long int free_mem;
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

	if (state_G.standard_free)
		free_mem = state_G.fresh.free;
	else
		free_mem = state_G.fresh.free + state_G.fresh.buffers + state_G.fresh.cached;
	if (state_G.show_used)
		free_mem = state_G.fresh.total - free_mem;
	if ((state_G.fresh.total > (999999 * 1024)) || state_G.mb)
		available = free_mem / 1024 / 1024;
	else
		available = free_mem / 1024;
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
		percentage = (int)(((float)free_mem) / ((float)state_G.fresh.total) * 100);
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

	if ((state_G.fresh.swap_total > (999999*1024)) || state_G.mb)
		total = state_G.fresh.swap_total / 1024 / 1024;
	else
		total = state_G.fresh.swap_total / 1024;
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

	free_mem = state_G.fresh.swap_free;
	if (state_G.show_used)
		free_mem = state_G.fresh.swap_total - free_mem;
	if ((free_mem > (999999 * 1024)) || state_G.mb)
		available = free_mem / 1024 / 1024;
	else
		available = free_mem / 1024;
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

	if (state_G.fresh.swap_total)
		percentage = (int) (((float)free_mem) / ((float)state_G.fresh.swap_total) * 100);
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

	points[0] = ((float)state_G.fresh.swap_used) / ((float)state_G.fresh.swap_total) * 46;
	for (i=0; i<3; ++i) {
		mainGCV_G.foreground = pix_G[3][i];
		XChangeGC (dpy_pG, mainGC_G, GCForeground, &mainGCV_G);
		XFillRectangle (dpy_pG, win, mainGC_G, 3, 38 + i, points[0], 1);
	}
}

/*
 * This function clears up all X related
 * stuff and exits. It is called in case
 * of emergencies .
 */
static void
asmem_cleanup (void)
{
	if (dpy_pG)
		XCloseDisplay (dpy_pG);
	close_meminfo ();
	exit (0);
}

/*
 * This checks for X11 events. We distinguish the following:
 * - request to repaint the window
 * - request to quit (Close button)
 */
static void
check_x11_events (void)
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
#ifdef DEBUG
					printf ("caught wmDelWin_G, closing\n");
#endif
					asmem_cleanup ();
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

void
asmem_update (void)
{
	time_t currentTime;

	currentTime = time (0);
	if (abs (currentTime - lastTime_G) >= state_G.update_interval) {
		lastTime_G = currentTime;
		if (read_meminfo ())
			asmem_cleanup ();
		if (memcmp (&state_G.last, &state_G.fresh, sizeof (AsmemMeminfo_t))) {
			memcpy (&state_G.last, &state_G.fresh, sizeof (AsmemMeminfo_t));
			draw_window (drawWin_G);
			++updateRequest_G;
		}
	}

	check_x11_events ();
	if (updateRequest_G)
		asmem_redraw ();
}

void
asmem_initialize (int argc, char *argv[], char *displayName_p, char *mainGeometry_p,
		int withdrawn, int iconic, int pushedIn)
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

	dpy_pG = XOpenDisplay (displayName_p);
	if (!dpy_pG) {
		printf ("asmem : grrrr... can't open display %s. Sorry ...\n",
			XDisplayName (displayName_p));
		exit (1);
	}
	screen = DefaultScreen (dpy_pG);
	rootWin_G = RootWindow (dpy_pG, screen);
	bgPix_G = get_colour (state_G.bgcolor, dpy_pG, rootWin_G);
	fgPix_G = get_colour (state_G.fgcolor, dpy_pG, rootWin_G);
	color_depth = DefaultDepth (dpy_pG, screen);
#ifdef DEBUG
	printf ("asmem : detected color depth %d bpp, using %d bpp\n", color_depth, color_depth);
#endif
	/* adjust the background pixmap */
	sprintf (pgPixColour_G[3], "# c %s", state_G.fgcolor);
	if (pushedIn) {
		sprintf (pgPixColour_G[0], ". c %s", darken_char_colour (state_G.bgcolor, 1.6, dpy_pG, rootWin_G));
		sprintf (pgPixColour_G[1], "c c %s", state_G.bgcolor);
		sprintf (pgPixColour_G[2], "q c %s", lighten_char_colour (state_G.bgcolor, 2.8, dpy_pG, rootWin_G));
	}
	else {
		sprintf (pgPixColour_G[2], "q c %s", darken_char_colour (state_G.bgcolor, 1.2, dpy_pG, rootWin_G));
		sprintf (pgPixColour_G[1], "c c %s", state_G.bgcolor);
		sprintf (pgPixColour_G[0], ". c %s", lighten_char_colour (state_G.bgcolor, 2.5, dpy_pG, rootWin_G));
	}
	for (tmp=0; tmp<4; ++tmp)
		background[tmp+1] = pgPixColour_G[tmp];

	backgroundXpm_G.attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);
	status = XpmCreatePixmapFromData (dpy_pG, rootWin_G, background, &backgroundXpm_G.pixmap, &backgroundXpm_G.mask, &backgroundXpm_G.attributes);
	if (status != XpmSuccess) {
		printf ("asmem : (%d) not enough free color cells for background.\n", status);
		asmem_cleanup ();
	}
#ifdef DEBUG
	printf ("bg pixmap %d x %d\n", backgroundXpm_G.attributes.width, backgroundXpm_G.attributes.height);
#endif
	sprintf (alphaColour_G[0], ". c %s", state_G.bgcolor);
	sprintf (alphaColour_G[1], "# c %s", state_G.fgcolor);
	sprintf (alphaColour_G[2], "a c %s", darken_char_colour (state_G.bgcolor, 1.4, dpy_pG, rootWin_G));
	sprintf (alphaColour_G[3], "c c %s", darken_char_colour (state_G.fgcolor, 1.6, dpy_pG, rootWin_G));
	for (tmp=0; tmp<4; ++tmp)
		alphabet[tmp+1] = alphaColour_G[tmp];
	alphabetXpm_G.attributes.valuemask |= (XpmReturnPixels | XpmReturnExtensions);
	status = XpmCreatePixmapFromData (dpy_pG, rootWin_G, alphabet, &alphabetXpm_G.pixmap, &alphabetXpm_G.mask, &alphabetXpm_G.attributes);
	if (status != XpmSuccess) {
		printf ("asmem : (%d) not enough free color cells for alphabet.\n", status);
		XCloseDisplay (dpy_pG);
		exit (1);
	}

	if (strlen (mainGeometry_p)) {
		/* Check the user-specified size */
		result = XParseGeometry (mainGeometry_p, &SizeHints.x, &SizeHints.y, &SizeHints.width, &SizeHints.height);
		if (result & XNegative)
			x_negative = 1;
		if (result & YNegative)
			y_negative = 1;
	}

	SizeHints.flags= USSize|USPosition;
	SizeHints.x = 0;
	SizeHints.y = 0;
	XWMGeometry (dpy_pG, screen, mainGeometry_p, NULL, 1, & SizeHints, &SizeHints.x, &SizeHints.y, &SizeHints.width, &SizeHints.height, &gravity);
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
	WmHints.initial_state = withdrawn ? WithdrawnState : iconic ? IconicState : NormalState;
	WmHints.icon_window = iconWin_G;
	if (withdrawn) {
		WmHints.window_group = mainWin_G;
		WmHints.flags |= WindowGroupHint;
	}
	if (iconic || withdrawn) {
		WmHints.icon_x = SizeHints.x;
		WmHints.icon_y = SizeHints.y;
		WmHints.flags |= IconPositionHint;
	}
	XSetWMHints (dpy_pG, mainWin_G, &WmHints);

	/* Finally show the window */
	status = XMapWindow (dpy_pG, mainWin_G);

	/* Get colors while waiting for Expose */
	pix_G[0][0] = lighten_colour (state_G.memory_color, 1.4, dpy_pG, mainWin_G);
	pix_G[0][1] = get_colour (state_G.memory_color, dpy_pG, mainWin_G);
	pix_G[0][2] = darken_colour (state_G.memory_color, 1.4, dpy_pG, mainWin_G);
	pix_G[1][0] = lighten_colour (state_G.buffer_color, 1.4, dpy_pG, mainWin_G);
	pix_G[1][1] = get_colour (state_G.buffer_color, dpy_pG, mainWin_G);
	pix_G[1][2] = darken_colour (state_G.buffer_color, 1.4, dpy_pG, mainWin_G);
	pix_G[2][0] = lighten_colour (state_G.cache_color, 1.4, dpy_pG, mainWin_G);
	pix_G[2][1] = get_colour (state_G.cache_color, dpy_pG, mainWin_G);
	pix_G[2][2] = darken_colour (state_G.cache_color, 1.4, dpy_pG, mainWin_G);
	pix_G[3][0] = lighten_colour (state_G.swap_color, 1.4, dpy_pG, mainWin_G);
	pix_G[3][1] = get_colour (state_G.swap_color, dpy_pG, mainWin_G);
	pix_G[3][2] = darken_colour (state_G.swap_color, 1.4, dpy_pG, mainWin_G);

	if (open_meminfo ())
		asmem_cleanup ();
	if (read_meminfo ())
		asmem_cleanup ();

	/* wait for the Expose event now */
	XNextEvent (dpy_pG, &Event);
	/* We 've got Expose -> draw the parts of the window. */
	asmem_redraw ();
	XFlush (dpy_pG);
}
