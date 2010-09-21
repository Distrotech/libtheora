#!/bin/sh
# Run this to set up the build system: configure, makefiles, etc.
# (based on the version in enlightenment's cvs)

package="theora"

ACLOCAL_FLAGS="-I m4"

olddir=`pwd`
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

cd "$srcdir"
DIE=0

/bin/echo "checking for autoconf... "
(autoconf --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have autoconf installed to compile $package."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

VERSIONGREP="sed -e s/.*[^0-9\.]\([0-9][0-9]*\.[0-9][0-9]*\).*/\1/"
VERSIONMKMAJ="sed -e s/\([0-9][0-9]*\)[^0-9].*/\\1/"
VERSIONMKMIN="sed -e s/.*[0-9][0-9]*\.//"

# do we need automake?
if test -r Makefile.am; then
  AM_OPTIONS=`fgrep AUTOMAKE_OPTIONS Makefile.am`
  AM_NEEDED=`echo $AM_OPTIONS | $VERSIONGREP`
  if test x"$AM_NEEDED" = "x$AM_OPTIONS"; then
    AM_NEEDED=""
  fi
  if test -z $AM_NEEDED; then
    /bin/echo -n "checking for automake... "
    AUTOMAKE=automake
    ACLOCAL=aclocal
    if ($AUTOMAKE --version < /dev/null > /dev/null 2>&1); then
      /bin/echo "yes"
    else
      /bin/echo "no"
      AUTOMAKE=
    fi
  else
    /bin/echo -n "checking for automake $AM_NEEDED or later... "
    majneeded=`echo $AM_NEEDED | $VERSIONMKMAJ`
    minneeded=`echo $AM_NEEDED | $VERSIONMKMIN`
    for am in automake-$AM_NEEDED automake$AM_NEEDED \
	automake automake-1.7 automake-1.8 automake-1.9 \
        automake-1.10 automake-1.11; do
      ($am --version < /dev/null > /dev/null 2>&1) || continue
      ver=`$am --version < /dev/null | head -n 1 | $VERSIONGREP`
      maj=`echo $ver | $VERSIONMKMAJ`
      min=`echo $ver | $VERSIONMKMIN`
      if test $maj -eq $majneeded -a $min -ge $minneeded; then
        AUTOMAKE=$am
        /bin/echo $AUTOMAKE
        break
      fi
    done
    test -z $AUTOMAKE &&  /bin/echo "no"
    /bin/echo -n "checking for aclocal $AM_NEEDED or later... "
    for ac in aclocal-$AM_NEEDED aclocal$AM_NEEDED \
	aclocal aclocal-1.7 aclocal-1.8 aclocal-1.9 aclocal-1.10 aclocal-1.11; do
      ($ac --version < /dev/null > /dev/null 2>&1) || continue
      ver=`$ac --version < /dev/null | head -n 1 | $VERSIONGREP`
      maj=`echo $ver | $VERSIONMKMAJ`
      min=`echo $ver | $VERSIONMKMIN`
      if test $maj -eq $majneeded -a $min -ge $minneeded; then
        ACLOCAL=$ac
        /bin/echo $ACLOCAL
        break
      fi
    done
    test -z $ACLOCAL && /bin/echo "no"
  fi
  test -z $AUTOMAKE || test -z $ACLOCAL && {
        echo
        echo "You must have automake installed to compile $package."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        exit 1
  }
fi

/bin/echo -n "checking for libtool... "
for LIBTOOLIZE in libtoolize glibtoolize nope; do
  ($LIBTOOLIZE --version) < /dev/null > /dev/null 2>&1 && break
done
if test x$LIBTOOLIZE = xnope; then
  /bin/echo "nope."
  LIBTOOLIZE=libtoolize
else
  /bin/echo $LIBTOOLIZE
fi
($LIBTOOLIZE --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have libtool installed to compile $package."
	echo "Download the appropriate package for your system,"
	echo "or get the source from one of the GNU ftp sites"
	echo "listed in http://www.gnu.org/order/ftp.html"
	DIE=1
}

if test "$DIE" -eq 1; then
        exit 1
fi

if test -z "$*"; then
        echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

/bin/echo "Generating configuration files for $package, please wait...."

/bin/echo "  $ACLOCAL $ACLOCAL_FLAGS"
$ACLOCAL $ACLOCAL_FLAGS || exit 1
/bin/echo "  $LIBTOOLIZE --automake --force"
$LIBTOOLIZE --automake --force || exit 1
/bin/echo "  autoheader"
autoheader || exit 1
/bin/echo "  $AUTOMAKE --add-missing $AUTOMAKE_FLAGS"
$AUTOMAKE --add-missing $AUTOMAKE_FLAGS || exit 1
/bin/echo "  autoconf"
autoconf || exit 1

cd $olddir
$srcdir/configure "$@" && /bin/echo
