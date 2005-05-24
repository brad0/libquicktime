# libquicktime top-level Makefile.am

# aclocal Path
ACLOCAL_AMFLAGS = -I m4

if HAVE_FIREWIRE
FIREWIRE_DIR=quicktime1394
else
FIREWIRE_DIR=
endif

SUBDIRS = \
$(FIREWIRE_DIR) \
doc \
src \
include \
plugins \
utils

EXTRA_DIST = libquicktime.spec autogen.sh lqt.m4 libquicktime.pc.in

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
quicktime1394 \
utils
