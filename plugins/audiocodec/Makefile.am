
pkglib_LTLIBRARIES = lqt_audiocodec.la

lqt_audiocodec_la_SOURCES = \
ima4.c \
rawaudio.c \
twos.c \
ulaw.c \
lqt_audiocodec.c

lqt_audiocodec_la_LDFLAGS = -avoid-version -module
lqt_audiocodec_la_LIBADD = @MODULE_LIBADD@

noinst_HEADERS = \
ima4.h \
rawaudio.h \
twos.h \
ulaw.h

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/
