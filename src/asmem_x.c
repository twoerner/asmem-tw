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

struct asmem_state state;

/* nice idea from ascd */
typedef struct _XpmIcon {
    Pixmap pixmap;
    Pixmap mask;
    XpmAttributes attributes;
} XpmIcon;

XpmIcon backgroundXpm;
XpmIcon alphabetXpm;

/* X windows related global variables */
Display * mainDisplay = 0;      /* The display we are working on */
Window Root;                    /* The root window of X11 */
Window drawWindow;              /* A hidden window for drawing */
Window mainWindow;              /* Application window */
Window iconWindow;              /* Icon window */
XGCValues mainGCV;              /* graphics context values */
GC mainGC;                      /* Graphics context */
Atom wm_delete_window;
Atom wm_protocols;

Pixel back_pix, fore_pix;

/* background pixmap colors */
char bgpixmap_color[4][50];
char alphabet_color[4][50];

/* pixels we need */
Pixel pix[4][3];

/* last time we updated */
time_t last_time = 0;

/* requests for update */
int update_request = 0;

void draw_window(Window win)
{
	int points[3];
	unsigned int total;
	unsigned int percentage;
	unsigned long int free_mem;
	unsigned int available;
	int i;
	unsigned int tmp[6];
	int digits;
	XCopyArea(
			mainDisplay,
			backgroundXpm.pixmap,
			win,
			mainGC,
			0,
			0,
			backgroundXpm.attributes.width,
			backgroundXpm.attributes.height,
			0,
			0
			);
	if ( (state.fresh.total > (999999 * 1024)) 
			|| state.mb )
		total = state.fresh.total / 1024 / 1024;
	else
		total = state.fresh.total / 1024;
	digits = 0;
	for (i=0 ; i<6; ++i) {
		tmp[i] = total % 10;
		total = total / 10;
		++digits;
		if (total == 0)
			break;
	}
	for (i=0; i<digits; ++i) {
		XCopyArea(
				mainDisplay,
				alphabetXpm.pixmap,
				win,
				mainGC,
				tmp[i]*5,
				0,
				6,
				9,
				46-(i*5),
				2
				);
	}

	if ( state.standard_free )
		free_mem = state.fresh.free;
	else
		free_mem = state.fresh.free
			+state.fresh.buffers
			+state.fresh.cached;
	if ( state.show_used )
		free_mem = state.fresh.total - free_mem;
	if ( (state.fresh.total > (999999 * 1024))
			|| state.mb )
		available = free_mem / 1024 / 1024;
	else
		available = free_mem / 1024;
	digits = 0;
	for (i=0 ; i<6; ++i) {
		tmp[i] = available % 10;
		available = available / 10;
		++digits;
		if (available == 0)
			break;
	}
	for (i=0; i<digits; ++i) {
		XCopyArea(
				mainDisplay,
				alphabetXpm.pixmap,
				win,
				mainGC,
				tmp[digits-1-i]*5,
				0,
				6,
				9,
				2+(i*5),
				17
				);
	}

	if ( state.fresh.total )
		percentage = (int) ( ((float)free_mem) /
			((float)state.fresh.total) * 100);
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
	for (i=0; i<4; ++i) {
		XCopyArea(
				mainDisplay,
				alphabetXpm.pixmap,
				win,
				mainGC,
				tmp[i]*5,
				0,
				6,
				9,
				32+(i*5),
				17
				);
	}

	points[0] = ((float)(
			state.fresh.used -
			state.fresh.buffers -
			state.fresh.cached
			)) /
		((float)state.fresh.total) *
		46;
	points[1] = ((float)state.fresh.buffers) /
		((float)state.fresh.total) *
		46;
	points[2] = ((float)state.fresh.cached) /
		((float)state.fresh.total) *
		46;
	for (i=0; i<3; ++i) {
		mainGCV.foreground = pix[0][i];
		XChangeGC(
				mainDisplay,
				mainGC,
				GCForeground,
				&mainGCV
				);
		XFillRectangle(
				mainDisplay,
				win,
				mainGC,
				3,
				13+i,
				points[0],
				1
				);
	}
	for (i=0; i<3; ++i) {
		mainGCV.foreground = pix[1][i];
		XChangeGC(
				mainDisplay,
				mainGC,
				GCForeground,
				&mainGCV
				);
		XFillRectangle(
				mainDisplay,
				win,
				mainGC,
				3+points[0],
				13+i,
				points[1],
				1
				);
	}
	for (i=0; i<3; ++i) {
		mainGCV.foreground = pix[2][i];
		XChangeGC(
				mainDisplay,
				mainGC,
				GCForeground,
				&mainGCV
				);
		XFillRectangle(
				mainDisplay,
				win,
				mainGC,
				3+points[0]+points[1],
				13+i,
				points[2],
				1
				);
	}

	if ( (state.fresh.swap_total > (999999*1024))
			|| state.mb )
		total = state.fresh.swap_total / 1024 / 1024;
	else
		total = state.fresh.swap_total / 1024;
	digits = 0;
	for (i=0 ; i<6; ++i) {
		tmp[i] = total % 10;
		total = total / 10;
		++digits;
		if (total == 0)
			break;
	}
	for (i=0; i<digits; ++i) {
		XCopyArea(
				mainDisplay,
				alphabetXpm.pixmap,
				win,
				mainGC,
				tmp[i]*5,
				0,
				6,
				9,
				46-(i*5),
				27
				);
	}

	free_mem = state.fresh.swap_free;
	if ( state.show_used )
		free_mem = state.fresh.swap_total - free_mem;
	if ( (free_mem > (999999*1024))
			|| state.mb )
		available = free_mem / 1024 / 1024;
	else
		available = free_mem / 1024;
	digits = 0;
	for (i=0 ; i<6; ++i) {
		tmp[i] = available % 10;
		available = available / 10;
		++digits;
		if (available == 0)
			break;
	}
	for (i=0; i<digits; ++i) {
		XCopyArea(
				mainDisplay,
				alphabetXpm.pixmap,
				win,
				mainGC,
				tmp[digits-1-i]*5,
				0,
				6,
				9,
				2+(i*5),
				42
				);
	}

	if ( state.fresh.swap_total )
		percentage = (int) (
			((float)free_mem) /
			((float)state.fresh.swap_total) * 100);
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
	for (i=0; i<4; ++i) {
		XCopyArea(
				mainDisplay,
				alphabetXpm.pixmap,
				win,
				mainGC,
				tmp[i]*5,
				0,
				6,
				9,
				32+(i*5),
				42
				);
	}

	points[0] = ((float)state.fresh.swap_used) /
		((float)state.fresh.swap_total) *
		46;
	for (i=0; i<3; ++i) {
		mainGCV.foreground = pix[3][i];
		XChangeGC(
				mainDisplay,
				mainGC,
				GCForeground,
				&mainGCV
				);
		XFillRectangle(
				mainDisplay,
				win,
				mainGC,
				3,
				38+i,
				points[0],
				1
				);
	}
}

/*
 * This function clears up all X related
 * stuff and exits. It is called in case
 * of emergencies .
 */             
void asmem_cleanup()
{
        if ( mainDisplay ) {
                XCloseDisplay(mainDisplay);
        }
	close_meminfo();
        exit(0);
}

/* 
 * This checks for X11 events. We distinguish the following:
 * - request to repaint the window
 * - request to quit (Close button)
 */
void CheckX11Events()
{
        XEvent Event;
        while (XPending(mainDisplay)) { 
                XNextEvent(mainDisplay, &Event);
                switch(Event.type)
                {
                case Expose:
                        if(Event.xexpose.count == 0) {
				++update_request;
			}
                        break;
                case ClientMessage:
                        if ((Event.xclient.message_type == wm_protocols)
                          && (Event.xclient.data.l[0] == wm_delete_window))
			{
#ifdef DEBUG
				printf("caught wm_delete_window, closing\n");
#endif
                                asmem_cleanup();
			}
                        break;
                }
        }
}

void asmem_redraw()
{
	XCopyArea(
			mainDisplay,
			drawWindow,
			mainWindow,
			mainGC,
			0,
			0,
			backgroundXpm.attributes.width,
			backgroundXpm.attributes.height,
			0,
			0
			);
	XCopyArea(
			mainDisplay,
			drawWindow,
			iconWindow,
			mainGC,
			0,
			0,
			backgroundXpm.attributes.width,
			backgroundXpm.attributes.height,
			0,
			0
			);

	update_request = 0;
}

void asmem_update()
{
	time_t cur_time;
	cur_time = time(0);
	if ( abs(cur_time - last_time) >= state.update_interval ) {
		last_time = cur_time;
		if (read_meminfo())
			asmem_cleanup();
		if (memcmp(&state.last, &state.fresh, sizeof(struct meminfo))) {
			memcpy(&state.last, &state.fresh, sizeof(struct meminfo));
			draw_window(drawWindow);
			++update_request;
		}
	}
	CheckX11Events();
	if ( update_request ) {
		asmem_redraw();
	}
}

void asmem_initialize(
			int argc, char** argv,
			char * display_name,
			char * mainGeometry,
                        int withdrawn,
                        int iconic,
                        int pushed_in)
{
	int screen;
	Status status;
	XWindowAttributes winAttr;
	XSizeHints SizeHints;
	XTextProperty title;
	char * app_name = "asmem";
	XClassHint classHint;
	int gravity;
	XWMHints WmHints;
	XEvent Event;
	int color_depth;
	int tmp;
	int result;
	int x_negative = 0;
	int y_negative = 0;

	mainDisplay = XOpenDisplay(display_name);
	if ( ! mainDisplay ) {
		printf("asmem : grrrr... can't open display %s. Sorry ...\n",
			XDisplayName(display_name));
		exit(1);
	}
	screen = DefaultScreen(mainDisplay);
	Root = RootWindow(mainDisplay, screen);
	back_pix = GetColor(state.bgcolor, mainDisplay, Root);
	fore_pix = GetColor(state.fgcolor, mainDisplay, Root);
	color_depth = DefaultDepth(mainDisplay, screen);
#ifdef DEBUG    
        printf("asmem : detected color depth %d bpp, using %d bpp\n",
                color_depth, color_depth);
#endif          
	/* adjust the background pixmap */
	sprintf(bgpixmap_color[3], "# c %s", state.fgcolor);
	if (pushed_in) {
		sprintf(bgpixmap_color[0],
			". c %s",
			DarkenCharColor(state.bgcolor, 1.6, mainDisplay, Root));
		sprintf(bgpixmap_color[1],
			"c c %s",
			state.bgcolor);
		sprintf(bgpixmap_color[2],
			"q c %s",
			LightenCharColor(state.bgcolor, 2.8, mainDisplay, Root));
	} else {
		sprintf(bgpixmap_color[2],
			"q c %s",
			DarkenCharColor(state.bgcolor, 1.2, mainDisplay, Root));
		sprintf(bgpixmap_color[1],
			"c c %s",
			state.bgcolor);
		sprintf(bgpixmap_color[0],
			". c %s",
			LightenCharColor(state.bgcolor, 2.5, mainDisplay, Root));
	}
	for (tmp = 0; tmp < 4; ++tmp)
		background[tmp+1] = bgpixmap_color[tmp];

	backgroundXpm.attributes.valuemask |=
                (XpmReturnPixels | XpmReturnExtensions);
        status = XpmCreatePixmapFromData(
                mainDisplay,                    /* display */
                Root,                           /* window */
                background,                     /* xpm */ 
                &backgroundXpm.pixmap,          /* resulting pixmap */
                &backgroundXpm.mask,
                &backgroundXpm.attributes
                );
        if(status != XpmSuccess) {
                printf("asmem : (%d) not enough free color cells for background.\n", status);   
		asmem_cleanup();
        }       
#ifdef DEBUG
	printf("bg pixmap %d x %d\n", 
		backgroundXpm.attributes.width,
		backgroundXpm.attributes.height);
#endif
	sprintf(alphabet_color[0], ". c %s", state.bgcolor);
	sprintf(alphabet_color[1], "# c %s", state.fgcolor);
	sprintf(alphabet_color[2], "a c %s", 
			DarkenCharColor(state.bgcolor, 1.4, mainDisplay, Root));
	sprintf(alphabet_color[3], "c c %s",
			DarkenCharColor(state.fgcolor, 1.6, mainDisplay, Root));
	for (tmp = 0; tmp < 4; ++tmp)
		alphabet[tmp+1] = alphabet_color[tmp];
	alphabetXpm.attributes.valuemask |=
		(XpmReturnPixels | XpmReturnExtensions);
	status = XpmCreatePixmapFromData(
			mainDisplay,
			Root,
			alphabet,
			&alphabetXpm.pixmap,            /* resulting pixmap */
			&alphabetXpm.mask,
			&alphabetXpm.attributes
			);
	if(status != XpmSuccess) {
		printf("asmem : (%d) not enough free color cells for alphabet.\n", status);
		XCloseDisplay(mainDisplay);
		exit(1);
	}

	if (strlen(mainGeometry)) {
		/* Check the user-specified size */
		result = XParseGeometry(
			mainGeometry,
			&SizeHints.x,
			&SizeHints.y,
			&SizeHints.width,
			&SizeHints.height
			);
		if (result & XNegative) 
			x_negative = 1;
		if (result & YNegative) 
			y_negative = 1;
	}

        SizeHints.flags= USSize|USPosition;
        SizeHints.x = 0; 
        SizeHints.y = 0; 
        XWMGeometry(
                mainDisplay,
                screen,
                mainGeometry,
                NULL,
                1,
                & SizeHints,
                &SizeHints.x,
                &SizeHints.y,
                &SizeHints.width,
                &SizeHints.height,
                &gravity
                );
        SizeHints.min_width =
        SizeHints.max_width =
        SizeHints.width = backgroundXpm.attributes.width;
        SizeHints.min_height =
        SizeHints.max_height =
        SizeHints.height= backgroundXpm.attributes.height;
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

	drawWindow = XCreatePixmap(
			mainDisplay,
			Root,
			SizeHints.width,
			SizeHints.height,
			color_depth
			);

        mainWindow = XCreateSimpleWindow(
                mainDisplay,            /* display */
                Root,                   /* parent */
                SizeHints.x,            /* x */
                SizeHints.y,            /* y */
                SizeHints.width,        /* width */
                SizeHints.height,       /* height */
                0,                      /* border_width */
                fore_pix,               /* border */
                back_pix                /* background */
                );
                
        iconWindow = XCreateSimpleWindow(
                mainDisplay,            /* display */
                Root,                   /* parent */
                SizeHints.x,            /* x */
                SizeHints.y,            /* y */
                SizeHints.width,        /* width */
                SizeHints.height,       /* height */
                0,                      /* border_width */
                fore_pix,               /* border */
                back_pix                /* background */
                );
                
        XSetWMNormalHints(mainDisplay, mainWindow, &SizeHints);
        status = XClearWindow(mainDisplay, mainWindow);
        
        status = XGetWindowAttributes(
                mainDisplay,    /* display */
                mainWindow,     /* window */
                & winAttr       /* window_attributes_return */
                );
        status = XSetWindowBackgroundPixmap(
                mainDisplay,            /* display */
                mainWindow,             /* window */
                backgroundXpm.pixmap    /* background_pixmap */
                );
        status = XSetWindowBackgroundPixmap(
                mainDisplay,            /* display */
                iconWindow,             /* window */
                backgroundXpm.pixmap    /* background_pixmap */
                );
                
        status = XStringListToTextProperty(&app_name, 1, &title);
        XSetWMName(mainDisplay, mainWindow, &title);
        XSetWMName(mainDisplay, iconWindow, &title);
        
        classHint.res_name = "asmem" ;
        classHint.res_class = "ASMEM";
        XSetClassHint(mainDisplay, mainWindow, &classHint);
        XStoreName(mainDisplay, mainWindow, "asmem");
        XSetIconName(mainDisplay, mainWindow, "asmem");

        status = XSelectInput(
                mainDisplay,    /* display */
                mainWindow,     /* window */
                ExposureMask    /* event_mask */
                );
                
        status = XSelectInput(
                mainDisplay,    /* display */
                iconWindow,     /* window */
                ExposureMask    /* event_mask */
                );
                
        /* Creating Graphics context */
        mainGCV.foreground = fore_pix;
        mainGCV.background = back_pix;
        mainGCV.graphics_exposures = False;
        mainGCV.line_style = LineSolid;
        mainGCV.fill_style = FillSolid;
        mainGCV.line_width = 1;
        mainGC = XCreateGC(
                mainDisplay,
                mainWindow, 
                GCForeground|GCBackground|GCLineWidth|
                        GCLineStyle|GCFillStyle,
                &mainGCV
                );
                
        status = XSetCommand(mainDisplay, mainWindow, argv, argc);
        
        /* Set up the event for quitting the window */
        wm_delete_window = XInternAtom(
                mainDisplay, 
                "WM_DELETE_WINDOW",     /* atom_name */
                False                   /* only_if_exists */
                );
        wm_protocols = XInternAtom(
                mainDisplay,
                "WM_PROTOCOLS",         /* atom_name */
                False                   /* only_if_exists */
                );
        status = XSetWMProtocols(
                mainDisplay, 
                mainWindow,
                &wm_delete_window,
                1
                );
        status = XSetWMProtocols(
                mainDisplay, 
                iconWindow,
                &wm_delete_window,
                1
                );
                
        WmHints.flags = StateHint | IconWindowHint;
        WmHints.initial_state = 
                withdrawn ? WithdrawnState :
                        iconic ? IconicState : NormalState;
        WmHints.icon_window = iconWindow;
        if ( withdrawn ) {
                WmHints.window_group = mainWindow;
                WmHints.flags |= WindowGroupHint;
        }       
        if ( iconic || withdrawn ) {
                WmHints.icon_x = SizeHints.x;
                WmHints.icon_y = SizeHints.y;
                WmHints.flags |= IconPositionHint;
        }       
        XSetWMHints(
                mainDisplay,
                mainWindow, 
                &WmHints);
                
        /* Finally show the window */
        status = XMapWindow(
                mainDisplay,
                mainWindow
                );
                
	/* Get colors while waiting for Expose */
	pix[0][0] = LightenColor(state.memory_color, 1.4, mainDisplay, mainWindow);
	pix[0][1] = GetColor(state.memory_color, mainDisplay, mainWindow);
	pix[0][2] = DarkenColor(state.memory_color, 1.4, mainDisplay, mainWindow);
	pix[1][0] = LightenColor(state.buffer_color, 1.4, mainDisplay, mainWindow);
	pix[1][1] = GetColor(state.buffer_color, mainDisplay, mainWindow);
	pix[1][2] = DarkenColor(state.buffer_color, 1.4, mainDisplay, mainWindow);
	pix[2][0] = LightenColor(state.cache_color, 1.4, mainDisplay, mainWindow);
	pix[2][1] = GetColor(state.cache_color, mainDisplay, mainWindow);
	pix[2][2] = DarkenColor(state.cache_color, 1.4, mainDisplay, mainWindow);
	pix[3][0] = LightenColor(state.swap_color, 1.4, mainDisplay, mainWindow);
	pix[3][1] = GetColor(state.swap_color, mainDisplay, mainWindow);
	pix[3][2] = DarkenColor(state.swap_color, 1.4, mainDisplay, mainWindow);

	if ( open_meminfo() ) {
		/* The /proc/meminfo file reading problems */
		asmem_cleanup();
	}
	if ( read_meminfo() ) {
		asmem_cleanup();
	}

        /* wait for the Expose event now */
        XNextEvent(mainDisplay, &Event); 
        /* We 've got Expose -> draw the parts of the window. */
        asmem_redraw(); 
        XFlush(mainDisplay);
}

