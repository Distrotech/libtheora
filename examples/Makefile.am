## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include 

noinst_PROGRAMS = player_example encoder_example

LDFLAGS = -all-static
LDADD = ../lib/libtheora.la

player_example_SOURCES = player_example.c
encoder_example_SOURCES = encoder_example.c

debug:
        $(MAKE) all CFLAGS="@DEBUG@"

profile:
        $(MAKE) all CFLAGS="@PROFILE@"
