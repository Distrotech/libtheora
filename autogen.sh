#!/bin/sh
# Run this to set up the build system: configure, makefiles, etc.
# (based on the version in enlightenment's cvs)

package="theora"

olddir=`pwd`
srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

cd "$srcdir"
DIE=0

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have autoconf installed to compile $package."
        echo "Download the appropriate package for your distribution,"
        echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
        DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
        echo
        echo "You must have automake installed to compile $package."
	echo "Download the appropriate package for your system,"
	echo "or get the source from one of the GNU ftp sites"
	echo "listed in http://www.gnu.org/order/ftp.html"
        DIE=1
}

(libtoolize --version) < /dev/null > /dev/null 2>&1 || {
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

echo "Generating configuration files for $package, please wait...."

test -f aclocal.m4 && rm aclocal.m4
echo -n "looking for our m4 macros... "
aclocaldirs="$srcdir `aclocal --print-ac-dir` \
        /usr/local/share/aclocal* /usr/local/aclocal* \
        /sw/share/aclocal* /usr/share/aclocal*"
for dir in $aclocaldirs; do
  test -d "$dir" && if ! test -z "`ls $dir | grep .m4`"; then
    if test ! -z "`fgrep XIPH_PATH_OGG $dir/*.m4`"; then
      echo $dir
      ACLOCAL_FLAGS="-I $dir $ACLOCAL_FLAGS"
      break
    fi
  fi
done
if test -z "$ACLOCAL_FLAGS"; then
  echo "nope."
  echo
  echo "Please install the ogg and vorbis libraries, or add the"
  echo "location of ogg.m4 to ACLOCAL_FLAGS in the environment."
  echo
  exit 1
fi


echo "  aclocal $ACLOCAL_FLAGS"
aclocal $ACLOCAL_FLAGS
echo "  autoheader"
autoheader
echo "  libtoolize --automake"
libtoolize --automake
echo "  automake --add-missing $AUTOMAKE_FLAGS"
automake --add-missing $AUTOMAKE_FLAGS 
echo "  autoconf"
autoconf

cd $olddir
$srcdir/configure "$@" && echo
