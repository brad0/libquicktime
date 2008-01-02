#!/bin/sh
# Run this to generate all the initial makefiles, etc.
./make_potfiles
autoreconf -f -i

if test $? != 0; then
  echo "autogen.sh failed"
else
  echo "all done. You are now ready to run ./configure"
fi
