# libquicktime top-level Makefile.am

if HAVE_FIREWIRE
FIREWIRE_DIR=quicktime1394
else
FIREWIRE_DIR=
endif

SUBDIRS = \
	  $(FIREWIRE_DIR) \
	  src \
	  include \
          plugins \
          utils

EXTRA_DIST = libquicktime.spec autogen.sh lqt.m4

bin_SCRIPTS = lqt-config

# M4 macro file for inclusion with autoconf
m4datadir = $(datadir)/aclocal
m4data_DATA = lqt.m4


DIST_SUBDIRS= \
	src \
	include \
	plugins \
	quicktime1394 \
	utils
