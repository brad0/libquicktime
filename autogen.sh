#!/bin/sh
# Run this to generate all the initial makefiles, etc.

function do_autogen
{
    echo "Doing autogen in $PWD... "
    aclocal
    if egrep -qs "A[CM]_CONFIG_HEADER" configure.in configure.ac > /dev/null 2> /dev/null; then
	autoheader
    fi
    automake --foreign --add-missing
    autoconf 
    echo " Done."
}

do_autogen

test -d libraw1394 && ( cd libraw1394; do_autogen )

test -d libogg-1.0rc2 && ( cd libogg-1.0rc2; do_autogen )

test -d libvorbis-1.0rc2 && ( cd libvorbis-1.0rc2; do_autogen )

