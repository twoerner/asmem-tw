/*
 * Copyright (c) 1998  Albert Dorofeev <Albert@mail.dma.be>
 * For the updates see http://bewoner.dma.be/Albert/
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
Pixel GetColor(char *ColorName, Display * disp, Window win)
{   
    XColor Color;
    XWindowAttributes Attributes;
    
    XGetWindowAttributes(disp,win,&Attributes);
    Color.pixel = 0;

    if (!XParseColor (disp, Attributes.colormap, ColorName, &Color))
         printf("asmem: can't parse %s\n", ColorName);
    else if(!XAllocColor (disp, Attributes.colormap, &Color))
         printf("asmem: can't allocate %s\n", ColorName);
    
    return Color.pixel;
}

/*
 * Performs the same actions as GetColor but
 * returns the complete XColor structure
 */
XColor ParseColor(char *ColorName, Display * disp, Window win)
{
    XColor Color;
    XWindowAttributes Attributes;

    XGetWindowAttributes(disp,win,&Attributes);
    Color.pixel = 0;

    if (!XParseColor (disp, Attributes.colormap, ColorName, &Color))
         printf("asmem: can't parse %s\n", ColorName);
    else if(!XAllocColor (disp, Attributes.colormap, &Color))
         printf("asmem: can't allocate %s\n", ColorName);

    return Color;
}

static char tmp_char[50];
/* darkens the given color using the supplied rate */
char *DarkenCharColor(char *ColorName, float rate, Display * disp, Window win)
{
        XColor tmp_color;
#ifdef DEBUG 
        printf("darkening %s ->", ColorName);
#endif  
        tmp_color = ParseColor(ColorName, disp, win);
#ifdef DEBUG
        printf(" #%x %x %x ", tmp_color.red, tmp_color.green, tmp_color.blue);
#endif  
        tmp_color.red = tmp_color.red / 257 / rate;
        tmp_color.green = tmp_color.green / 257 / rate;
        tmp_color.blue = tmp_color.blue / 257 / rate;
        sprintf(tmp_char, "#%.2x%.2x%.2x", 
                (int) tmp_color.red,
                (int) tmp_color.green,
                (int) tmp_color.blue);
#ifdef DEBUG    
        printf("-> %s\n", tmp_char);
#endif  
        return tmp_char;
}       

/* darkens the given color using the supplied rate */
Pixel DarkenColor(char *ColorName, float rate, Display * disp, Window win)
{
        return GetColor(
		DarkenCharColor(ColorName,rate,disp,win), 
		disp, win);
}       

/* lightens the given color using the supplied rate */
char *LightenCharColor(char *ColorName, float rate, Display * disp, Window win)
{
        XColor tmp_color;
#ifdef DEBUG 
        printf("lightening %s ->", ColorName);
#endif  
        tmp_color = ParseColor(ColorName, disp, win);
#ifdef DEBUG
        printf(" #%x %x %x ", tmp_color.red, tmp_color.green, tmp_color.blue);
#endif  
        tmp_color.red = tmp_color.red / 257 * rate;
        tmp_color.green = tmp_color.green / 257 * rate;
        tmp_color.blue = tmp_color.blue / 257 * rate;
        if (tmp_color.red > 255) tmp_color.red = 255;
        if (tmp_color.green > 255) tmp_color.green = 255;
        if (tmp_color.blue > 255) tmp_color.blue = 255;
        sprintf(tmp_char, "#%.2x%.2x%.2x", 
                (int) tmp_color.red,
                (int) tmp_color.green,
                (int) tmp_color.blue);
#ifdef DEBUG    
        printf("-> %s\n", tmp_char);
#endif  
        return tmp_char;
}       

/* lightens the given color using the supplied rate */
Pixel LightenColor(char *ColorName, float rate, Display * disp, Window win)
{
        return GetColor(
		LightenCharColor(ColorName,rate,disp,win), 
		disp, win);
}

