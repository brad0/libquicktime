#!/bin/sh
# Run this to generate all the initial makefiles, etc.

do_autogen () {
    echo "Doing autogen in $PWD... "
    autoheader
    aclocal
    autoconf
    automake --foreign --add-missing
}

do_autogen

#test -d libraw1394 && ( cd libraw1394; do_autogen )
#test -d libogg-1.0rc2 && ( cd libogg-1.0rc2; do_autogen )
#test -d libvorbis-1.0rc2 && ( cd libvorbis-1.0rc2; do_autogen )

echo "all done."
