/*
 * Copyright (c) 1999  Albert Dorofeev <Albert@mail.dma.be>
 * For the updates see http://bewoner.dma.be/Albert/
 *
 * This software is distributed under GPL. For details see LICENSE file.
 */

#ifndef _asmem_x_h_
#define _asmem_x_h_

void asmem_initialize(  
                        int argc, char** argv,
                        char * display_name,
			char * mainGeometry,
                        int withdrawn,
                        int iconic,
                        int pushed_in);
void asmem_update(void);
void asmem_redraw(void);
void asmem_cleanup(void);

#endif

