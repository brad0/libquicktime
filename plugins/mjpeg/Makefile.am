libquicktimeincludedir = $(includedir)/quicktime

libquicktimeinclude_HEADERS = \
libmjpeg.h jpeg.h

CFLAGS = @CFLAGS@

lib_LTLIBRARIES = libquicktime_mjpeg.la

pkglib_LTLIBRARIES = lqt_mjpeg.la

libquicktime_mjpeg_la_SOURCES = libmjpeg.c

lqt_mjpeg_la_SOURCES = \
jpeg.c \
lqt_mjpeg.c

lqt_mjpeg_la_LDFLAGS = -avoid-version -module
lqt_mjpeg_la_LIBADD = libquicktime_mjpeg.la @JPEG_LIBS@ @MODULE_LIBADD@

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/
