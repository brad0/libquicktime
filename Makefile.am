
local_headers= \
	cmodel_permutation.h \
	codecs.h \
	fastjpg.h \
	fastjpgmacro.h \
	funcprotos.h \
	graphics.h \
	mmx.h \
	rtjpeg.h \
	rtjpeg_core.h \
	workarounds.h \
        lqt_codecinfo_private.h

include_headers = \
quicktime.h \
qtprivate.h \
colormodels.h \
lqt.h  \
lqt_codecinfo.h \
lqt_codecapi.h

# The following must be commented out, because otherwise, the utils
# directory will go into make dist for gmerlin (which it shouldn't)

#
# For LIBQUICKTIME
#

SUBDIRS = \
          @FIREWIRE_SUBDIRS@ \
          . \
          plugins \
          utils

include_HEADERS=$(include_headers) 
noinst_HEADERS=$(local_headers)

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

DIST_SUBDIRS= libdv \
        plugins \
	utils \
	@COMPLETE_DIST@

lib_LTLIBRARIES= libquicktime.la

#
# Library dependencies are emtpy no
# Maybe we need zlib to read compressed quicktime headers
#

libquicktime_la_LDFLAGS = -export-dynamic

#	libogg-1.0rc2/src/libogg.la \


#CLAGS += @JPEG_CFLAGS@ -Ilibdv @VORBIS_CFLAGS@ @FIREWIRE_CFLAGS@
CFLAGS = @CFLAGS@

CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -include $(CONFIG_HEADER)

wc:
	cat *.c *.h | wc

backup: clean
	cd .. && \
	tar jcvf quicktime.tar.bz2 quicktime


# build only the files, which can be compiled

# Different files for dynamic plugins and others

lqt_sources = lqt_codecs.c  lqt_plugin.c lqt_quicktime.c

# lqt_sources = codecs.c  plugin.c quicktime.c


libquicktime_la_SOURCES= \
        $(lqt_sources) \
        atom.c \
	cmodel_default.c \
	cmodel_yuv420p.c \
	cmodel_yuv422.c \
	colormodels.c \
	ctab.c \
	dinf.c \
	dref.c \
	edts.c \
	elst.c \
	graphics.c \
	hdlr.c \
	matrix.c \
	mdat.c \
	mdhd.c \
	mdia.c \
	minf.c \
	moov.c \
	mvhd.c \
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
	udta.c \
	util.c \
	vmhd.c \
	workarounds.c \
        lqt_codecfile.c \
        lqt_codecinfo.c \
        lqt_loader.c

#configure : configure.in
#	aclocal
#	autoconf

#Makefile.am : configure
#	aclocal
#	autoconf
