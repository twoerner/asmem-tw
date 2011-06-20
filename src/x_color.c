/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/xpm.h>


/*
 * Note: this function was originally taken out of ascd.
 *
 * It takes the given color, parses it in the context
 * of the given window and returns a pixel of that color.
 */
Pixel
get_colour (char *colourName_p, Display *dpy_p, Window win)
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
 * Performs the same actions as get_colour but
 * returns the complete XColor structure
 */
static XColor
parse_colour (char *colourName_p, Display *dpy_p, Window win)
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

static char tmp_char[50];
/* darkens the given color using the supplied rate */
char*
darken_char_colour (char *colourName_p, float rate, Display *dpy_p, Window win)
{
	XColor tmpColour;
#ifdef DEBUG
	printf ("darkening %s ->", colourName_p);
#endif
	tmpColour = parse_colour (colourName_p, dpy_p, win);
#ifdef DEBUG
	printf (" #%x %x %x ", tmpColour.red, tmpColour.green, tmpColour.blue);
#endif
	tmpColour.red = tmpColour.red / 257 / rate;
	tmpColour.green = tmpColour.green / 257 / rate;
	tmpColour.blue = tmpColour.blue / 257 / rate;
	sprintf (tmp_char, "#%.2x%.2x%.2x", (int)tmpColour.red, (int)tmpColour.green, (int)tmpColour.blue);
#ifdef DEBUG
	printf ("-> %s\n", tmp_char);
#endif
	return tmp_char;
}

/* darkens the given color using the supplied rate */
Pixel
darken_colour (char *colourName_p, float rate, Display *dpy_p, Window win)
{
	return get_colour (darken_char_colour (colourName_p, rate, dpy_p, win), dpy_p, win);
}

/* lightens the given color using the supplied rate */
char*
lighten_char_colour (char *colourName_p, float rate, Display *dpy_p, Window win)
{
	XColor tmpColour;
#ifdef DEBUG
	printf ("lightening %s ->", colourName_p);
#endif
	tmpColour = parse_colour (colourName_p, dpy_p, win);
#ifdef DEBUG
	printf (" #%x %x %x ", tmpColour.red, tmpColour.green, tmpColour.blue);
#endif
	tmpColour.red = tmpColour.red / 257 * rate;
	tmpColour.green = tmpColour.green / 257 * rate;
	tmpColour.blue = tmpColour.blue / 257 * rate;
	if (tmpColour.red > 255)
		tmpColour.red = 255;
	if (tmpColour.green > 255)
		tmpColour.green = 255;
	if (tmpColour.blue > 255)
		tmpColour.blue = 255;
	sprintf (tmp_char, "#%.2x%.2x%.2x", (int)tmpColour.red, (int)tmpColour.green, (int)tmpColour.blue);
#ifdef DEBUG
	printf ("-> %s\n", tmp_char);
#endif
	return tmp_char;
}

/* lightens the given color using the supplied rate */
Pixel
lighten_colour (char *colourName_p, float rate, Display *dpy_p, Window win)
{
	return get_colour (lighten_char_colour (colourName_p, rate, dpy_p, win), dpy_p, win);
}
