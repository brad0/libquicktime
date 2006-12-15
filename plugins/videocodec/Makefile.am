
pkglib_LTLIBRARIES = lqt_videocodec.la

lqt_videocodec_la_SOURCES = \
raw.c \
v210.c \
v308.c \
v408.c \
v410.c \
yuv2.c \
yuv4.c \
yv12.c \
videocodec.c \
lqt_videocodec.c

lqt_videocodec_la_LDFLAGS = -avoid-version -module
lqt_videocodec_la_LIBADD = @MODULE_LIBADD@

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/quicktime

noinst_HEADERS = videocodec.c
