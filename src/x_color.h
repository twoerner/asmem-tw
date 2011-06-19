/*
 * Copyright (c) 1998  Albert Dorofeev <Albert@mail.dma.be>
 * For the updates see http://bewoner.dma.be/Albert/
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#ifndef _x_color_h_
#define _x_color_h_

/*
 * It takes the given color, parses it in the context
 * of the given window and returns a pixel of that color.
 */
Pixel GetColor(char *ColorName, Display * disp, Window win);

/*
 * Performs the same actions as GetColor but
 * returns the complete XColor structure
 */
XColor ParseColor(char *ColorName, Display * disp, Window win);

/* darkens the given color using the supplied rate */
char *DarkenCharColor(char *ColorName, float rate, Display * disp, Window win);
Pixel DarkenColor(char *ColorName, float rate, Display * disp, Window win);

/* lightens the given color using the supplied rate */
char *LightenCharColor(char *ColorName, float rate, Display * disp, Window win);
Pixel LightenColor(char *ColorName, float rate, Display * disp, Window win);

#endif

