if HAVE_GTK
gtk_subdirs=gtk
else
gtk_subdirs=
endif

SUBDIRS = $(gtk_subdirs)

noinst_PROGRAMS=test_codec testqt

bin_PROGRAMS=qtinfo qtstreamize qtdechunk qtrechunk qtyuv4toyuv qtdump qtrecover

testqt_SOURCES=testqt.c
testqt_LDADD=@UTIL_LIBADD@ -lm

test_codec_SOURCES=test_codec.c
test_codec_LDADD=@UTIL_LIBADD@

qtinfo_SOURCES=qtinfo.c
qtinfo_LDADD=@UTIL_LIBADD@

qtstreamize_SOURCES=qtstreamize.c
qtstreamize_LDADD=@UTIL_LIBADD@

qtdump_SOURCES=dump.c
qtdump_LDADD=@UTIL_LIBADD@

qtdechunk_SOURCES=dechunk.c
qtdechunk_LDADD=@UTIL_LIBADD@

qtrechunk_SOURCES=rechunk.c
qtrechunk_LDADD=@UTIL_LIBADD@

qtrecover_SOURCES=recover.c
qtrecover_LDADD=@UTIL_LIBADD@

qtyuv4toyuv_SOURCES=yuv4toyuv.c
qtyuv4toyuv_LDADD=@UTIL_LIBADD@


INCLUDES = -I$(top_srcdir)/include

#CFLAGS += -g3 -Werror
