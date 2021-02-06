localedir = $(datadir)/locale
AM_CFLAGS = @FAAC_CFLAGS@ -DLOCALE_DIR=\"$(localedir)\"

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_builddir)/include

pkglib_LTLIBRARIES = lqt_faac.la

lqt_faac_la_SOURCES = faac.c lqt_faac.c
lqt_faac_la_LDFLAGS = @MODULE_LDFLAGS@ -avoid-version -module
lqt_faac_la_LIBADD = @MODULE_LIBADD@ @FAAC_LIBS@

noinst_HEADERS = qtfaac.h
