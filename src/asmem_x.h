/*
 * Copyright (C) 2011  Trevor Woerner
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#ifndef asmem_x__H
#define asmem_x__H

void asmem_initialize (int argc, char *argv[], char *displayName_p, char *mainGeometry_p,
		int withdrawn, int iconic, int pushedIn);
void asmem_update (void);
void asmem_redraw (void);
void asmem_cleanup (void);

#endif
