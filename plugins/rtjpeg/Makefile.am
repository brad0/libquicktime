localedir = $(datadir)/locale
AM_CFLAGS = -DLOCALE_DIR=\"$(localedir)\"
pkglib_LTLIBRARIES = lqt_rtjpeg.la
AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include

lqt_rtjpeg_la_SOURCES = rtjpeg_codec.c lqt_rtjpeg.c RTjpeg.c
lqt_rtjpeg_la_LDFLAGS = @MODULE_LDFLAGS@ -avoid-version -module
lqt_rtjpeg_la_LIBADD = @MODULE_LIBADD@

noinst_HEADERS = RTjpeg.h rtjpeg_codec.h mmx.h

