CFLAGS = @CFLAGS@ @FIREWIRE_CFLAGS@ -Ilibdv 

CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS += -include $(CONFIG_HEADER)

SUBDIRS = libdv

pkglib_LTLIBRARIES = lqt_dv.la

lqt_dv_la_SOURCES = dv.c libdv.c lqt_dv.c
lqt_dv_la_LDFLAGS = -avoid-version -module -lm

lqt_dv_la_LIBADD = libdv/libdv.la @FIREWIRE_LIBS@ @MODULE_LIBADD@

noinst_HEADERS = dv.h libdv.h

