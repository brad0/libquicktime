CFLAGS = @CFLAGS@ @PNG_CFLAGS@


CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -include $(CONFIG_HEADER)

pkglib_LTLIBRARIES = lqt_png.la

lqt_png_la_SOURCES = qtpng.c lqt_png.c
lqt_png_la_LDFLAGS = -avoid-version -module
lqt_png_la_LIBADD = @PNG_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = qtpng.h

INCLUDES = -I../../include
