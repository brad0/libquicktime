AM_CFLAGS = @AVCODEC_CFLAGS@

pkglib_LTLIBRARIES = lqt_ffmpeg.la

lqt_ffmpeg_la_SOURCES = ffmpeg.c lqt_ffmpeg.c audio.c video.c params.c
lqt_ffmpeg_la_LDFLAGS = -avoid-version -module
lqt_ffmpeg_la_LIBADD = @MODULE_LIBADD@ @AVCODEC_LIBS@

noinst_HEADERS = ffmpeg.h params.h

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/quicktime
