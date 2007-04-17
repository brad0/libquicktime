#!/bin/sh
# Run this to generate all the initial makefiles, etc.
./make_potfiles
autoreconf -f -i
echo "all done. You are now ready to run ./configure"
