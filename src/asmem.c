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

extern struct asmem_state state;

/*
 * default check and update intervals in microseconds
 *      x11 events - every 1/100 th of a second (in mks)
 *      Memory status - every second (in sec)
 */
#define X11_INTERVAL    10000L
#define CHK_INTERVAL    1

int     withdrawn = 0;
int     iconic = 0;
int     pushed_in = 1;
char    display_name[50];
char    mainGeometry[50];

void defaults(void)
{
	state.update_interval = CHK_INTERVAL;
	state.standard_free = 0;
	state.mb = 0;
	state.show_used = 0;
	safecopy(state.proc_mem_filename, PROC_MEM, 256);
	withdrawn = 0;
	iconic = 0; 
	pushed_in = 1;
	safecopy(display_name, "", 50);
	safecopy(mainGeometry, "", 50);
	safecopy(state.bgcolor, "#303030", 50);
	safecopy(state.fgcolor, "#20b2aa", 50);
	safecopy(state.memory_color, "#4141d7", 50);
	safecopy(state.buffer_color, "#aa80aa", 50);
	safecopy(state.cache_color, "#bebebe", 50);
	safecopy(state.swap_color, "#ffa649", 50);
}

/* print the usage for the tool */
void usage(void) 
{
        printf("Usage : asmem [options ...]\n\n");
        printf("-V              print version and exit\n");
        printf("-h -H -help     print this message\n");
        printf("-u <secs>       the update interval in seconds\n");
        printf("-mb             the display is in MBytes\n");
        printf("-used           display used memory instead of free\n");
        printf("-display <name> the name of the display to use\n");
        printf("-position <xy>  position on the screen (geometry)\n");
        printf("-withdrawn      start in withdrawn shape (for WindowMaker)\n");
        printf("-iconic         start iconized\n");
        printf("-standout       standing out rather than being pushed in\n");
        printf("-asis           use free memory amount as read from device\n");
	printf("-dev <device>	use the specified file as stat device\n\n");
	printf("-bg <color>	background color\n");
	printf("-fg <color>	base foreground color\n");
	printf("-memory <color>	used memory bar color\n");
	printf("-buffer <color>	buffer memory bar color\n");
	printf("-cache <color>	cache memory bar color\n");
	printf("-swap <color>	used swap space bar color\n");
        printf("\n");
        exit(0);
}       

/* print the version of the tool */
void version(void)
{
        printf("asmem : AfterStep memory utilization monitor version 1.10\n");
}               

void    parsecmdline(int argc, char *argv[])
{
        char    *argument;
        int     i;

	/* parse the command line */
        for (i=1; i<argc; i++) {
                argument=argv[i];
                if (argument[0]=='-') {
                        if (!strncmp(argument,"-withdrawn",10)) {
                                withdrawn=1;
                        } else if (!strncmp(argument,"-iconic",7)) {
                                iconic=1;
                        } else if (!strncmp(argument,"-standout",9)) {
                                pushed_in=0;
                        } else if (!strncmp(argument,"-asis",5)) {
                                state.standard_free=1;
                        } else if (!strncmp(argument,"-free",5)) {
                                state.standard_free=1;
                        } else if (!strncmp(argument,"-mb",3)) {
                                state.mb=1;
                        } else if (!strncmp(argument,"-used",5)) {
                                state.show_used=1;
                        } else if (!strncmp(argument,"-u",2)) {
                                if (++i >= argc)
                                        usage();
                                state.update_interval = atoi(argv[i]);
                                if ( state.update_interval < 1 )  
                                        state.update_interval = CHK_INTERVAL;
                        } else if (!strncmp(argument,"-position",9)) {
                                if (++i >= argc)
                                        usage();
                                safecopy(mainGeometry, argv[i], 50);
                        } else if (!strncmp(argument,"-display",8)) {
                                if (++i >= argc)
                                        usage();
                                safecopy(display_name, argv[i], 50);
                        } else if (!strncmp(argument,"-dev",4)) {
                                if (++i >= argc)
                                        usage();
                                safecopy(state.proc_mem_filename,argv[i],256);
			} else if (!strncmp(argument,"-bg",3)) {
				if (++i >= argc)
					usage();
				safecopy(state.bgcolor, argv[i], 50);
			} else if (!strncmp(argument,"-fg",3)) {
				if (++i >= argc)
					usage();
				safecopy(state.fgcolor, argv[i], 50);
			} else if (!strncmp(argument,"-memory",7)) {
				if (++i >= argc)
					usage();
				safecopy(state.memory_color, argv[i], 50);
			} else if (!strncmp(argument,"-buffer",7)) {
				if (++i >= argc)
					usage();
				safecopy(state.buffer_color, argv[i], 50);
			} else if (!strncmp(argument,"-cache",6)) {
				if (++i >= argc)
					usage();
				safecopy(state.cache_color, argv[i], 50);
			} else if (!strncmp(argument,"-swap",5)) {
				if (++i >= argc)
					usage();
				safecopy(state.swap_color, argv[i], 50);
                        } else if (!strncmp(argument,"-V",2)) { 
                                version();
                                exit(0);
                        } else if (!strncmp(argument,"-H",2)) {
                                version();
                                usage();
                        } else if (!strncmp(argument,"-h",2)) {
                                version();
                                usage();
                        } else {
                                version();
                                usage();
                        }       
                } else {
                        version();
                        usage();
                }       
        }       
 
}

int main(int argc, char** argv)
{
	defaults();
	parsecmdline(argc, argv);
	asmem_initialize(argc, argv, 
			display_name, 
			mainGeometry, 
			withdrawn, 
			iconic, 
			pushed_in);
	while (1) {
		asmem_update();
		usleep(X11_INTERVAL);
	}

	return 0;
}

