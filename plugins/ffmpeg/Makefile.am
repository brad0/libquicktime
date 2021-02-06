localedir = $(datadir)/locale
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include
if HAVE_LIBSWSCALE
libswscale_cflags = @LIBSWSCALE_CFLAGS@
libswscale_libs   = @LIBSWSCALE_LIBS@
else
libswscale_cflags =
libswscale_libs   =
endif

AM_CFLAGS = @LIBAVCODEC_CFLAGS@ $(libswscale_cflags) -DLOCALE_DIR=\"$(localedir)\"


pkglib_LTLIBRARIES = lqt_ffmpeg.la

lqt_ffmpeg_la_SOURCES = lqt_ffmpeg.c audio.c video.c params.c
lqt_ffmpeg_la_LDFLAGS = @MODULE_LDFLAGS@ -avoid-version -module
lqt_ffmpeg_la_LIBADD = @MODULE_LIBADD@ @LIBAVCODEC_LIBS@ $(libswscale_libs)

noinst_HEADERS = ffmpeg.h params.h

