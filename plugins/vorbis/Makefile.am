CFLAGS = @CFLAGS@ @VORBIS_CFLAGS@

CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
#CFLAGS += -include $(CONFIG_HEADER)

pkglib_LTLIBRARIES = lqt_vorbis.la

lqt_vorbis_la_SOURCES = vorbis.c lqt_vorbis.c
lqt_vorbis_la_LDFLAGS = -avoid-version -module
lqt_vorbis_la_LIBADD = @VORBISENC_LIBS@ @VORBIS_LIBS@ @VORBISFILE_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = qtvorbis.h

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/
