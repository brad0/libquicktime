#!/bin/sh
# Run this to generate all the initial makefiles, etc.

do_autogen () {
    echo "Doing autogen in $PWD... "
    echo -n "aclocal..."
    aclocal -I .
    echo "done"
    echo -n "libtoolize..."
    libtoolize --automake --copy --force
    echo "done"
    echo -n "autoheader..."
    autoheader
    echo "done"
    echo -n "autoconf..."
    autoconf
    echo "done"
    echo -n "automake..."
    automake --foreign --add-missing
    echo "done"
}

#Run this twice - rerun with auto generated files...
do_autogen
do_autogen

echo "all done. You are now ready to run ./configure"
