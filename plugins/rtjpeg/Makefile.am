AM_CFLAGS = @RTJPEG_CFLAGS@

pkglib_LTLIBRARIES = lqt_rtjpeg.la

lqt_rtjpeg_la_SOURCES = rtjpeg_codec.c lqt_rtjpeg.c RTjpeg.c
lqt_rtjpeg_la_LDFLAGS = -avoid-version -module
lqt_rtjpeg_la_LIBADD = @MODULE_LIBADD@

noinst_HEADERS = RTjpeg.h rtjpeg_codec.h mmx.h

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/

# This is broken for newer libtools
# RTjpeg.lo:	RTjpeg.c RTjpeg.h
#	gcc @RTJPEG_CFLAGS@ -c RTjpeg.c -o RTjpeg.lo
