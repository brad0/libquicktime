localedir = $(datadir)/locale
AM_CFLAGS = @X264_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include
pkglib_LTLIBRARIES = lqt_x264.la

lqt_x264_la_SOURCES = x264.c lqt_x264.c
lqt_x264_la_LDFLAGS = @MODULE_LDFLAGS@ -avoid-version -module
lqt_x264_la_LIBADD = @MODULE_LIBADD@ @X264_LIBS@

noinst_HEADERS = qtx264.h
