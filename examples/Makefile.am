## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include 

if HAVE_SDL
player_example = player_example
else
player_example = 
endif

noinst_PROGRAMS = encoder_example $(player_example)

LDFLAGS = -all-static 
LDADD = ../lib/libtheora.la -logg -lvorbis -lm

player_example_SOURCES = player_example.c
player_example_LDADD = $(LDADD) $(SDL_LIBS) -lpthread

encoder_example_SOURCES = encoder_example.c
encoder_example_LDADD = $(LDADD) -lvorbisenc

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

EXTRA_DIST = $(player_example_SOURCES)
