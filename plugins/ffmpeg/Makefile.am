CFLAGS = @CFLAGS@
CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64

pkglib_LTLIBRARIES = lqt_ffmpeg.la

lqt_ffmpeg_la_SOURCES = ffmpeg.c lqt_ffmpeg.c audio.c video.c
lqt_ffmpeg_la_LDFLAGS = -avoid-version -module
lqt_ffmpeg_la_LIBADD = @MODULE_LIBADD@ -lavcodec -lm

noinst_HEADERS = ffmpeg.h

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/
