
EXTRA_DIST=autogen.sh

local_headers= \
	cmodel_permutation.h \
	codecs.h \
	divx.h \
	dv.h \
	fastjpg.h \
	fastjpgmacro.h \
	funcprotos.h \
	graphics.h \
	ima4.h \
	jpeg.h \
	jpeg_old.h \
	libdv.h \
	libmjpeg.h \
	mmx.h \
	qtpng.h \
	qtvorbis.h \
	raw.h \
	rawaudio.h \
	rtjpeg.h \
	rtjpeg_core.h \
	twos.h \
	ulaw.h \
	v308.h \
	v408.h \
	v410.h \
	workarounds.h \
	yuv2.h \
	yuv4.h \
	yv12.h

# The following must be commented out, because otherwise, the utils
# directory will go into make dist for gmerlin (which it shouldn't)

#
# For LIBQUICKTIME
#

SUBDIRS = libdv \
          decore50 \
          encore50 \
          @FIREWIRE_SUBDIRS@ \
          . \
          utils

include_HEADERS=quicktime.h qtprivate.h colormodels.h libdv.h 
noinst_HEADERS=$(local_headers)

#
# For GMERLIN
#

# SUBDIRS=libdv \
# 	decore50 \
# 	encore50
# noinst_HEADERS=quicktime.h qtprivate.h colormodels.h libdv.h $(local_headers)

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

DIST_SUBDIRS= libdv \
	decore50 \
	encore50 \
	utils \
	@COMPLETE_DIST@

lib_LTLIBRARIES= libquicktime.la

libquicktime_la_LIBADD= \
	libdv/libdv.la \
	decore50/libdecore50.la \
	encore50/libencore50.la \
	@FIREWIRE_LIBS@ \
	@VORBISENC_LIBS@ @VORBIS_LIBS@ @VORBISFILE_LIBS@ \
	@JPEG_LIBS@ @PNG_LIBS@

#	libogg-1.0rc2/src/libogg.la \


#CLAGS += @JPEG_CFLAGS@ -Ilibdv @VORBIS_CFLAGS@ @FIREWIRE_CFLAGS@
CFLAGS = @CFLAGS@ -Ilibdv @VORBIS_CFLAGS@ @FIREWIRE_CFLAGS@ @JPEG_CFLAGS@ @PNG_CFLAGS@

CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -include $(CONFIG_HEADER)

wc:
	cat *.c *.h | wc

backup: clean
	cd .. && \
	tar jcvf quicktime.tar.bz2 quicktime


# build only the files, which can be compiled

if HAVE_LIBJPEG
jpeg_sources = jpeg.c \
	libmjpeg.c \
	jpeg_old.c
else
jpeg_sources=
endif

if HAVE_LIBPNG
png_sources=qtpng.c
else
png_sources=
endif

if HAVE_VORBIS
vorbis_sources=vorbis.c
else
vorbis_sources=
endif

libquicktime_la_SOURCES= \
        $(jpeg_sources) \
        $(png_sources) \
        $(vorbis_sources) \
        atom.c \
	cmodel_default.c \
	cmodel_yuv420p.c \
	cmodel_yuv422.c \
	codecs.c \
	colormodels.c \
	ctab.c \
	dinf.c \
	divx.c \
	dref.c \
	dv.c \
	edts.c \
	elst.c \
	graphics.c \
	hdlr.c \
	ima4.c \
	libdv.c \
	matrix.c \
	mdat.c \
	mdhd.c \
	mdia.c \
	minf.c \
	moov.c \
	mvhd.c \
	plugin.c \
	quicktime.c \
	raw.c \
	rawaudio.c \
	smhd.c \
	stbl.c \
	stco.c \
	stsc.c \
	stsd.c \
	stsdtable.c \
	stss.c \
	stsz.c \
	stts.c \
	tkhd.c \
	trak.c \
	twos.c \
	udta.c \
	ulaw.c \
	util.c \
	v308.c \
	v408.c \
	v410.c \
	vmhd.c \
	workarounds.c \
	yuv2.c \
	yuv4.c \
	yv12.c 

#configure : configure.in
#	aclocal
#	autoconf

#Makefile.am : configure
#	aclocal
#	autoconf
