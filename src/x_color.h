/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#ifndef x_colour__H
#define x_colour__H

#include <X11/Xlib.h>

/*
 * It takes the given color, parses it in the context
 * of the given window and returns a pixel of that color.
 */
Pixel get_colour (char *colourName_p, Display *dpy_p, Window win);

/*
 * Performs the same actions as GetColor but
 * returns the complete XColor structure
 */
XColor parse_colour (char *colourName_p, Display *dpy_p, Window win);

/* darkens the given color using the supplied rate */
char* darken_char_colour (char *colourName_p, float rate, Display *dpy_p, Window win);
Pixel darken_colour (char *colourName_p, float rate, Display *dpy_p, Window win);

/* lightens the given color using the supplied rate */
char* lighten_char_colour (char *colourName_p, float rate, Display *dpy_p, Window win);
Pixel lighten_colour (char *colourName_p, float rate, Display *dpy_p, Window win);

#endif
