AM_CFLAGS = @FAAD2_CFLAGS@

pkglib_LTLIBRARIES = lqt_faad2.la

lqt_faad2_la_SOURCES = faad2.c lqt_faad2.c
lqt_faad2_la_LDFLAGS = -avoid-version -module
lqt_faad2_la_LIBADD = @FAAD2_LIBS@ @MODULE_LIBADD@

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/quicktime
