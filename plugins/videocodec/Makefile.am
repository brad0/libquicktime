
pkglib_LTLIBRARIES = lqt_videocodec.la

lqt_videocodec_la_SOURCES = \
raw.c \
v308.c \
v408.c \
v410.c \
yuv2.c \
yuv4.c \
yv12.c \
lqt_videocodec.c

lqt_videocodec_la_LDFLAGS = -avoid-version -module
lqt_videocodec_la_LIBADD = @MODULE_LIBADD@

noinst_HEADERS = \
raw.h \
v308.h \
v408.h \
v410.h \
yuv2.h \
yuv4.h \
yv12.h

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/
