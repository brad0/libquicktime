if HAVE_GTK
gtk_subdirs=gtk
else
gtk_subdirs=
endif

SUBDIRS = $(gtk_subdirs)

if HAVE_X11
xbin := lqtplay lqtvrplay
xman := lqtplay.1
else
xbin :=
xman :=
endif
noinst_PROGRAMS := test_codec testqt dump_codecs
# bin_PROGRAMS := $(xbin) qtinfo qtstreamize qtdechunk qtrechunk qtyuv4toyuv qtdump qtrecover lqt_transcode
bin_PROGRAMS := $(xbin) qtinfo qtstreamize qtdechunk qtrechunk qtyuv4toyuv qtdump lqt_transcode
man1_MANS := $(xman)

lqt_transcode_SOURCES=lqt_transcode.c
lqt_transcode_LDADD=@UTIL_LIBADD@ -lm

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

dump_codecs_SOURCES=dump_codecs.c
dump_codecs_LDADD=@UTIL_LIBADD@

qtrechunk_SOURCES=rechunk.c
qtrechunk_LDADD=@UTIL_LIBADD@

# qtrecover_SOURCES=recover.c
# qtrecover_LDADD=@UTIL_LIBADD@

qtyuv4toyuv_SOURCES=yuv4toyuv.c
qtyuv4toyuv_LDADD=@UTIL_LIBADD@

lqtplay_SOURCES=lqtplay.c
lqtplay_LDADD=@UTIL_LIBADD@ @X_LIBS@ -lXaw -lXt @X_PRE_LIBS@ -lXext \
	-lXv -lGLU -lGL -lX11 @X_EXTRA_LIBS@ -lm @ALSA_LIBS@

lqtvrplay_SOURCES=lqtvrplay.c
lqtvrplay_LDADD=@UTIL_LIBADD@ @X_LIBS@ -lXaw -lXt @X_PRE_LIBS@ -lXext \
        -lXv -lGLU -lGL -lX11 @X_EXTRA_LIBS@ -lm

INCLUDES = -I$(top_srcdir)/include @X_CFLAGS@ @ALSA_CFLAGS@

EXTRA_DIST=lqtplay.1

#CFLAGS += -g3 -Werror
