localedir = $(datadir)/locale
# libquicktime top-level Makefile.am

# aclocal Path
ACLOCAL_AMFLAGS = -I m4

SUBDIRS = doc src include plugins utils po

EXTRA_DIST = config.rpath libquicktime.spec autogen.sh lqt.m4 libquicktime.pc.in cpuinfo.sh Doxyfile

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA=libquicktime.pc

bin_SCRIPTS = lqt-config

# M4 macro file for inclusion with autoconf
m4datadir = $(datadir)/aclocal
m4data_DATA = lqt.m4


DIST_SUBDIRS= \
debian \
doc \
src \
include \
plugins \
utils
