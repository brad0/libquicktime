localedir = $(datadir)/locale
AM_CFLAGS = @PNG_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

pkglib_LTLIBRARIES = lqt_png.la

lqt_png_la_SOURCES = qtpng.c lqt_png.c
lqt_png_la_LDFLAGS = -avoid-version -module
lqt_png_la_LIBADD = @PNG_LIBS@ @MODULE_LIBADD@

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/quicktime
