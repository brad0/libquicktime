libquicktimeincludedir = $(includedir)/quicktime

CFLAGS = @CFLAGS@

pkglib_LTLIBRARIES = lqt_lame.la

lqt_lame_la_SOURCES = \
lame_codec.c \
lqt_lame.c

lqt_lame_la_LDFLAGS = -avoid-version -module
lqt_lame_la_LIBADD = @MODULE_LIBADD@ -lmp3lame

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/
