## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = lib include examples

EXTRA_DIST = COPYING autogen.sh 

dist-hook:
        rm -rf `find $(distdir)/macos -name CVS`

debug:
        $(MAKE) all CFLAGS="@DEBUG@"

profile:
        $(MAKE) all CFLAGS="@PROFILE@"
