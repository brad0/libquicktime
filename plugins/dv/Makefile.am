CFLAGS = @CFLAGS@ @LIBDV_CFLAGS@ @FIREWIRE_CFLAGS@

CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -include $(CONFIG_HEADER)
#CFLAGS += -g3

pkglib_LTLIBRARIES = lqt_dv.la

lqt_dv_la_SOURCES = dv.c lqt_dv.c
lqt_dv_la_LDFLAGS = -avoid-version -module -lm

lqt_dv_la_LIBADD = @FIREWIRE_LIBS@ @LIBDV_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = dv.h

INCLUDES = -I$(top_srcdir)/include
