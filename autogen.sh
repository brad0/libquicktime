#!/bin/sh
# Run this to generate all the initial makefiles, etc.

do_autogen () {
    echo "Doing autogen in $PWD... "
    libtoolize --copy --force
    autoheader
    aclocal
    autoconf
    automake --foreign --add-missing
}

#Run this twice - rerun with auto generated files...
# do_autogen
do_autogen

echo "all done. You are now ready to run ./configure"
