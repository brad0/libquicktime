localedir = $(datadir)/locale
AM_CFLAGS = @FAAC_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

pkglib_LTLIBRARIES = lqt_faac.la

lqt_faac_la_SOURCES = faac.c lqt_faac.c
lqt_faac_la_LDFLAGS = -avoid-version -module
lqt_faac_la_LIBADD = @FAAC_LIBS@ @MODULE_LIBADD@

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/quicktime
