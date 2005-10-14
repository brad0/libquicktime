#!/bin/sh
# Run this to generate all the initial makefiles, etc.

autoreconf -f -i
echo "all done. You are now ready to run ./configure"
