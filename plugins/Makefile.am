# These are always here

base_subdirs = audiocodec videocodec opendivx ferguson rtjpeg xanim svq

if HAVE_LIBJPEG
jpeg_subdirs = mjpeg
else
jpeg_subdirs=
endif

if HAVE_LIBDV
dv_subdirs = dv
else
dv_subdirs=
endif

if HAVE_LIBPNG
png_subdirs=png
else
png_subdirs=
endif

if HAVE_VORBIS
vorbis_subdirs=vorbis
else
vorbis_subdirs=
endif

SUBDIRS = $(base_subdirs) $(jpeg_subdirs) $(dv_subdirs) \
	$(png_subdirs) $(vorbis_subdirs)
