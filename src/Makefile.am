lib_LTLIBRARIES= libquicktime.la

#
# Library dependencies are emtpy no
# Maybe we need zlib to read compressed quicktime headers
#

libquicktime_la_LDFLAGS = -export-dynamic

#CLAGS += @JPEG_CFLAGS@ -Ilibdv @VORBIS_CFLAGS@ @FIREWIRE_CFLAGS@
CFLAGS = @CFLAGS@

CFLAGS += -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
#CFLAGS += -include $(CONFIG_HEADER)

# build only the files, which can be compiled

# Different files for dynamic plugins and others

lqt_sources = lqt_codecs.c  lqt_plugin.c lqt_quicktime.c

# lqt_sources = codecs.c  plugin.c quicktime.c

libquicktime_la_SOURCES= \
$(lqt_sources) \
atom.c \
cmodel_default.c \
cmodel_yuv420p.c \
cmodel_yuv422.c \
colormodels.c \
ctab.c \
dinf.c \
dref.c \
edts.c \
elst.c \
graphics.c \
hdlr.c \
matrix.c \
mdat.c \
mdhd.c \
mdia.c \
minf.c \
moov.c \
mvhd.c \
smhd.c \
stbl.c \
stco.c \
stsc.c \
stsd.c \
stsdtable.c \
stss.c \
stsz.c \
stts.c \
tkhd.c \
trak.c \
udta.c \
util.c \
vmhd.c \
workarounds.c \
lqt_codecfile.c \
lqt_color.c \
lqt_codecinfo.c \
lqt_loader.c

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include/
