CFLAGS = @CFLAGS@

CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -include $(CONFIG_HEADER)

pkglib_LTLIBRARIES = lqt_mjpeg.la

lqt_mjpeg_la_SOURCES = \
jpeg.c \
libmjpeg.c \
lqt_mjpeg.c

lqt_mjpeg_la_LDFLAGS = -avoid-version -module
lqt_mjpeg_la_LIBADD = @JPEG_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = \
jpeg.h \
libmjpeg.h

INCLUDES = -I$(top_srcdir)/include
