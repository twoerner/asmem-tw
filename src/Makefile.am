## Copyright (C) 2011  Trevor Woerner

########################
## src/Makefile.am
########################
SUBDIRS =
AM_LDFLAGS = $(X_LIBS)
#AM_CFLAGS = $(X_CFLAGS) -Wall -Werror -Wextra -Wconversion -Wreturn-type -Wstrict-prototypes
AM_CFLAGS = $(X_CFLAGS) -Wall -Wextra -Wconversion -Wreturn-type -Wstrict-prototypes

bin_PROGRAMS = asmem
asmem_SOURCES = asmem.c asmem_x.c asmem_x.h state.h \
	read_mem.c read_mem.h safecopy.c safecopy.h x_color.c x_color.h \
	alphabet.xpm background.xpm
