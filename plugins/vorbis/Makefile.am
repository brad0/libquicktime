AM_CFLAGS = @VORBIS_CFLAGS@

pkglib_LTLIBRARIES = lqt_vorbis.la

lqt_vorbis_la_SOURCES = vorbis.c lqt_vorbis.c
lqt_vorbis_la_LDFLAGS = -avoid-version -module
lqt_vorbis_la_LIBADD = @VORBISENC_LIBS@ @VORBIS_LIBS@ @VORBISFILE_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = qtvorbis.h

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/
