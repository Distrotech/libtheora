## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include 

if HAVE_SDL
player_example = player_example
else
player_example = 
endif

if HAVE_PORTAUDIO
splayer = splayer
else
splayer =
endif

noinst_PROGRAMS = encoder_example dump_video $(player_example) $(splayer)

LDFLAGS = -all-static 
LDADD = ../lib/libtheora.la -logg -lm

dump_video_SOURCES = dump_video.c
dump_video_LDADD = $(LDADD)

player_example_SOURCES = player_example.c
player_example_LDADD = $(LDADD) $(SDL_LIBS)  -lvorbis -lpthread

splayer_SOURCES = splayer.c
splayer_LDADD = $(LDADD) $(SDL_LIBS)  -lvorbis -lpthread /usr/local/lib/libportaudio.a

encoder_example_SOURCES = encoder_example.c
encoder_example_LDADD = $(LDADD) -lvorbisenc

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"

EXTRA_DIST = $(encoder_example_SOURCES)
