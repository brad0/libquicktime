# The following must be commented out, because otherwise, the utils
# directory will go into make dist for gmerlin (which it shouldn't)

#
# For LIBQUICKTIME
#

SUBDIRS = \
          @FIREWIRE_SUBDIRS@ \
          . \
	  src \
	  include \
          plugins \
          utils

EXTRA_DIST = libquicktime.spec

#
# For GMERLIN
#

# SUBDIRS=plugins
# noinst_HEADERS=$(include_headers) $(local_headers)

# ENDIF

# @JPEG_SUBDIRS@ \


#DIST_SUBDIRS=jpeg \
#	libdv \
#	decore50 \
#	encore50 \
#	libraw1394 \
#	libogg-1.0rc2 \
#	libvorbis-1.0rc2 \
#	utils

DIST_SUBDIRS=src \
	include \
	plugins \
	utils \
	@COMPLETE_DIST@

wc:
	cat *.c *.h | wc

backup: clean
	cd .. && \
	tar jcvf quicktime.tar.bz2 quicktime


#configure : configure.in
#	aclocal
#	autoconf

#Makefile.am : configure
#	aclocal
#	autoconf
