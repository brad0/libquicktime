#!/bin/sh
if [ -f Makefile ] ; then
 make clean
 make distclean
fi

rm -f `find . -name "*~"`
rm -f `find . -name "Makefile.in"`
rm -rf configure config.* autom4te.cache
rm -f aclocal.m4
rm -f libquicktime.spec ltmain.sh
rm -f depcomp
rm -f install.sh
rm -f missing
rm -f mkinstalldirs
rm -f install-sh
