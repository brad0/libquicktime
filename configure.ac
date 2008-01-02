AC_INIT(src/lqt_quicktime.c)
AM_INIT_AUTOMAKE(libquicktime, 1.0.2)

LQT_VERSION=$VERSION

LQT_VERSION_MAJOR=`echo $VERSION | cut -d . -f 1`
LQT_VERSION_MINOR=`echo $VERSION | cut -d . -f 2`
LQT_VERSION_MICRO=`echo $VERSION | cut -d . -f 3 | cut -d p -f 1`

USER_CFLAGS=$CFLAGS

AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST
AM_MAINTAINER_MODE

AC_LIBTOOL_PICMODE

AH_TEMPLATE([LQT_VERSION],
            [Libquicktime version])

dnl LQT_VERSION=$VERSION
AC_DEFINE_UNQUOTED(LQT_VERSION, "$LQT_VERSION")
AC_SUBST(LQT_VERSION)
AC_SUBST(LQT_VERSION_MAJOR)
AC_SUBST(LQT_VERSION_MINOR)
AC_SUBST(LQT_VERSION_MICRO)


dnl
dnl Option for conversion to GPL
dnl

have_gpl="false"
AC_ARG_ENABLE(gpl, [  --enable-gpl            Change license to GPL. This enables some extra plugins],
              have_gpl="true")

dnl
dnl Libquicktime codec API version
dnl 

LQT_CODEC_API_VERSION="9"

if test "x$have_gpl" = "xtrue"; then
  LQT_CODEC_API_VERSION="($LQT_CODEC_API_VERSION|0x10000)"
fi

AH_TEMPLATE([LQT_CODEC_API_VERSION],
            [Libquicktime codec API version])

AC_DEFINE_UNQUOTED(LQT_CODEC_API_VERSION, $LQT_CODEC_API_VERSION)
AC_SUBST(LQT_CODEC_API_VERSION)

AC_DISABLE_STATIC

AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL

AC_PROG_CC
AC_LANG_C

AC_C_BIGENDIAN

# large file support
AC_SYS_LARGEFILE

AC_CACHE_SAVE

dnl
dnl Gettext
dnl

AM_GNU_GETTEXT([external])


PKG_PROG_PKG_CONFIG


dnl This is necessary for libtool assembler support

AS=gcc
ASFLAGS=

AC_SUBST(AS)
AC_SUBST(ASFLAGS)

AC_SUBST(LIBS)

AH_TEMPLATE([_GNU_SOURCE],
            [Define to enable GNU extensions of glibc, notably large file support])
           
AH_TEMPLATE([YUV_420_USE_YV12],
            [Define to 1 to use planar YUV format for 420 blocks in IEC PAL])

AH_TEMPLATE([ARCH_X86],
            [Define as 1 if host is an IA32])

AH_TEMPLATE([LINUX],
            [Define as 1 if host is Linux])

case "$host_os" in
	linux*)
		AC_DEFINE(LINUX)
		;;
	*)
		echo $host_os
		;;
esac

AC_DEFINE(_GNU_SOURCE)

use_asm=:
arch_x86=false

AC_ARG_ENABLE(asm,
[  --disable-asm           disable use of architecture specific assembly code],
[	if test "$enableval" = "no"; then
		use_asm=false
	fi
])

if $use_asm; then
	case "$host_cpu" in
		i?86)
			arch_x86=:
			AC_DEFINE(ARCH_X86)
			;;
		*)
			;;
	esac
fi

AM_CONDITIONAL(HOST_X86, $arch_x86)

dnl
dnl Extension for modules
dnl

AH_TEMPLATE([MODULE_EXT], [Extension for shared modules])

eval MODULE_EXT=$shrext_cmds

if test x$MODULE_EXT = x.dylib -o x$MODULE_EXT = x; then
MODULE_EXT=".so"
fi

AC_DEFINE_UNQUOTED(MODULE_EXT, ["$MODULE_EXT"])

dnl Checks for libraries.

AC_CHECK_LIB(dl, dlopen)
AC_CHECK_LIB(z, inflateEnd)
AC_CHECK_LIB(m, cos)

ICONV_LIBS=
AC_CHECK_LIB(iconv, libiconv_close, ICONV_LIBS="-liconv")
AC_SUBST(ICONV_LIBS)

AC_SEARCH_LIBS(pthread_create, [pthread c_r])

AC_SUBST(LIBS)

AC_CHECK_HEADERS(fcntl.h sys/time.h unistd.h linux/videodev.h sys/soundcard.h soundcard.h stddef.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Checks for library functions.

AC_FUNC_MMAP
AC_CHECK_FUNCS([gettimeofday memalign posix_memalign lrint vasprintf])
AC_CHECK_FUNCS(fseeko, [have_fseeko="true"])
AM_CONDITIONAL(HAVE_FSEEKO, test x"$have_fseeko" = "xtrue")

AC_HEADER_STDC

dnl
dnl Check for optional libraries
dnl 

dnl
dnl Check for Vorbis
dnl 

VORBIS_REQUIRED="1.0"
 
have_vorbis=false
AH_TEMPLATE([HAVE_VORBIS], [Vorbis libraries are there])
  
if test "x$with_vorbis" != "xno"; then
XIPH_PATH_VORBIS(have_vorbis=true)
fi
AM_CONDITIONAL(HAVE_VORBIS, test x$have_vorbis = xtrue)
  
if test x$have_vorbis = xtrue; then
AC_DEFINE(HAVE_VORBIS)
fi
 
AC_SUBST(VORBIS_REQUIRED)

dnl
dnl Check for lame
dnl

LAME_REQUIRED="3.93"
have_lame="false"

AC_ARG_WITH([lame], AS_HELP_STRING([--without-lame], [Build without lame library (default: test)]))

if test "x$with_lame" != "xno"; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

if test x$have_vorbis = xtrue; then
LIBS="$LIBS -lmp3lame -lvorbis -lm"
else
LIBS="$LIBS -lmp3lame -lm"
fi

dnl CFLAGS="$CFLAGS"

AC_MSG_CHECKING(for lame)
have_lame="false"

  AC_TRY_RUN([
    #include <lame/lame.h>
    #include <stdio.h>
    main()
    {
    int version_major;
    int version_minor;
    const char * version;
    version = get_lame_version();
    fprintf(stderr, "lame version: %s\n", version);
    if(sscanf(version, "%d.%d", &version_major,
              &version_minor) < 2)
      return -1;
    if((version_major != 3) || (version_minor < 93))
      return 1;
    return 0;
    }
  ],
  [
    # program could be run
    have_lame="true"
    AC_MSG_RESULT(yes)
    LAME_CFLAGS=$CFLAGS
    LAME_LIBS=$LIBS

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(LAME_CFLAGS)
AC_SUBST(LAME_LIBS)
AC_SUBST(LAME_REQUIRED)

fi

AM_CONDITIONAL(HAVE_LAME, test x$have_lame = xtrue)

dnl
dnl Check for X11
dnl

AC_PATH_XTRA
AC_SUBST(X_CFLAGS)
AC_SUBST(X_PRE_LIBS)
AC_SUBST(X_EXTRA_LIBS)
AC_SUBST(X_LIBS)

dnl
dnl Check for other headers which might be missing
dnl

if test "$no_x" != "yes"; then
saveCFLAGS=$CFLAGS
CFLAGS="$CFLAGS $X_CFLAGS"
AC_CHECK_HEADER([X11/Xaw/Simple.h],[],no_x="yes")
CFLAGS=$saveCFLAGS
fi

AM_CONDITIONAL(HAVE_X11, test "$no_x" != "yes")

dnl
dnl OpenGL
dnl

AH_TEMPLATE([HAVE_GL], [Do we have OpenGL?])

AC_ARG_WITH([opengl], AS_HELP_STRING([--without-opengl], [Build without opengl library (default: test)]))

if test "x$with_opengl" != "xno"; then

have_gl="yes"

AC_CHECK_HEADER([GL/gl.h],[],have_gl="no")

if test "$have_gl" = "yes"; then
AC_CHECK_LIB(GL, glBegin,[],have_gl="no")
fi

if test "$have_gl" = "yes"; then
AC_CHECK_HEADER([GL/glx.h],[],have_gl="no")
fi

if test "x$have_gl" = "xyes"; then
AC_DEFINE(HAVE_GL,1)
GL_LIBS="-lGL"
fi

fi

AC_SUBST(GL_LIBS)

dnl
dnl Check for Alsa
dnl

have_libalsa="false"

AH_TEMPLATE([HAVE_ALSA], [Do we have Alsa?])

AC_ARG_WITH([alsa], AS_HELP_STRING([--without-alsa], [Build without alsa library (default: test)]))

if test "x$with_alsa" != "xno"; then
ALSA_REQUIRED="0.9"
PKG_CHECK_MODULES(ALSA, alsa >= $ALSA_REQUIRED, have_alsa="true", have_alsa="false")
fi

AM_CONDITIONAL(HAVE_ALSA, test x$have_alsa = xtrue)

if test x$have_alsa = xtrue; then
AC_DEFINE(HAVE_ALSA,1)
fi

ALSA_CFLAGS="$ALSA_CFLAGS"
AC_SUBST(ALSA_CFLAGS)

# NetBSD and OpenBSD have an OSS audio compatability library 
AH_TEMPLATE([HAVE_LIBOSSAUDIO], [Have libossaudio, for NetBSD and OpenBSD])
AC_CHECK_LIB(ossaudio, _oss_ioctl, have_libossaudio=true)
if test x$have_libossaudio = xtrue; then
AC_DEFINE(HAVE_LIBOSSAUDIO,1)
OSSAUDIO_LIBS=-lossaudio
AC_SUBST(OSSAUDIO_LIBS)
fi

dnl
dnl Check for gtk
dnl This is neccesary for the utils/gtk subdirectory
dnl
GTK_REQUIRED="2.4.0"
AC_ARG_WITH([gtk], AS_HELP_STRING([--without-gtk], [Build without gtk library (default: test)]))

if test "x$with_gtk" != "xno"; then
PKG_CHECK_MODULES(GTK, gtk+-2.0 >= $GTK_REQUIRED, have_gtk="true", have_gtk="false")
fi

AM_CONDITIONAL(HAVE_GTK, test x$have_gtk = xtrue)
AC_SUBST(GTK_REQUIRED)

dnl Always be in sync with the newest gtk
GTK_CFLAGS="$GTK_CFLAGS -DGTK_DISABLE_DEPRECATED"

dnl
dnl Check for libdv
dnl

have_libdv="false"

LIBDV_REQUIRED="0.102"

AC_ARG_WITH([libdv], AS_HELP_STRING([--without-libdv], [Build without libdv support (default: test)]))

if test "x$with_libdv" != "xno"; then
PKG_CHECK_MODULES(LIBDV, libdv >= $LIBDV_REQUIRED, have_libdv="true", have_libdv="false")
fi

AC_SUBST(LIBDV_REQUIRED)
AC_SUBST(LIBDV_LIBS)
AC_SUBST(LIBDV_CFLAGS)

AM_CONDITIONAL(HAVE_LIBDV, test x$have_libdv = xtrue)

dnl
dnl Check for libjpeg
dnl

AH_TEMPLATE([HAVE_LIBJPEG],
            [Do we have libjpeg installed?])

have_libjpeg=false
JPEG_REQUIRED="6b"

AC_ARG_WITH(libjpeg,
[AC_HELP_STRING([--without-libjpeg],[Disable libjpeg (default: autodetect)])],
[case "${withval}" in
   yes) test_libjpeg=true ;;
   no)  test_libjpeg=false ;;
esac],[test_libjpeg=true])

if test x$test_libjpeg = xtrue; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS
LIBS="$LIBS -ljpeg"
CFLAGS="$CFLAGS"

AC_MSG_CHECKING(for libjpeg)
AC_TRY_LINK([#include <stdio.h>
             #include <jpeglib.h>],
            [struct jpeg_decompress_struct cinfo;
             jpeg_create_decompress(&cinfo);],
            [have_libjpeg=true])
case $have_libjpeg in
  true) AC_DEFINE(HAVE_LIBJPEG)
        AC_MSG_RESULT(yes)
        JPEG_LIBS=$LIBS;
        JPEG_CFLAGS=$CFLAGS;;
  false) AC_MSG_RESULT(no); JPEG_LIBS=""; JPEG_CFLAGS="";;
  * ) AC_MSG_RESULT("Somethings wrong: $have_libjpeg") ;;
esac

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

fi

AC_SUBST(JPEG_LIBS)
AC_SUBST(JPEG_CFLAGS)
AC_SUBST(JPEG_REQUIRED)
AM_CONDITIONAL(HAVE_LIBJPEG, test x$have_libjpeg = xtrue)



dnl
dnl Check for libavcodec (ffmpeg)
dnl

AH_TEMPLATE([HAVE_LIBAVCODEC],
            [Do we have libavcodec installed?])

have_libavcodec=false

AVCODEC_BUILD="3277056"

AC_ARG_WITH([ffmpeg], AS_HELP_STRING([--without-ffmpeg], [Build without ffmpeg library (default: test)]))

if test "x$with_ffmpeg" != "xno"; then

dnl PKG_CHECK_MODULES(AVCODEC, avcodec_acl = $AVCODEC_REQUIRED, have_libavcodec="true", have_libavcodec="false")
ACL_PATH_AVCODEC($AVCODEC_BUILD , have_libavcodec="true", have_libavcodec="false")

fi

AVCODEC_REQUIRED=$AVCODEC_VERSION

AM_CONDITIONAL(HAVE_LIBAVCODEC, test x$have_libavcodec = xtrue)

AC_SUBST(AVCODEC_REQUIRED)
AC_SUBST(AVCODEC_LIBS)
AC_SUBST(AVCODEC_CFLAGS)

dnl
dnl libswscale
dnl

AH_TEMPLATE([HAVE_LIBSWSCALE],
            [Do we have libswscale installed?])

have_libswscale=false

LIBSWSCALE_REQUIRED="0.5.0"

AC_ARG_ENABLE(libswscale,
[AC_HELP_STRING([--disable-libswscale],[Disable libswscale (default: autodetect)])],
[case "${enableval}" in
   yes) test_libswscale=true ;;
   no)  test_libswscale=false ;;
esac],[test_libswscale=true])

if test x$test_libswscale = xtrue; then

PKG_CHECK_MODULES(LIBSWSCALE, libswscale >= $LIBSWSCALE_REQUIRED,
                  have_libswscale="true", have_libswscale="false")
fi

AC_SUBST(LIBSWSCALE_REQUIRED)
AC_SUBST(LIBSWSCALE_LIBS)
AC_SUBST(LIBSWSCALE_CFLAGS)

AM_CONDITIONAL(HAVE_LIBSWSCALE, test x$have_libswscale = xtrue)

if test "x$have_libswscale" = "xtrue"; then
AC_DEFINE([HAVE_LIBSWSCALE])
fi

dnl
dnl Check for libpng
dnl 

AH_TEMPLATE([HAVE_LIBPNG], [Enable png codec])

have_libpng=false
PNG_REQUIRED="1.0.8"

AC_ARG_WITH([libpng], AS_HELP_STRING([--without-libpng], [Build without png library (default: test)]))

if test "x$with_libpng" != "xno"; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

LIBS="$LIBS -lpng -lm -lz"
CFLAGS="$CFLAGS"

AC_MSG_CHECKING(for libpng)
AC_TRY_LINK([#include <png.h>],
            [png_structp png_ptr;
             png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
	                                       (png_voidp)0,
						NULL, NULL);],
	    [have_libpng=true])

case $have_libpng in
  true) AC_DEFINE(HAVE_LIBPNG)
        AC_MSG_RESULT(yes)
	PNG_LIBS=$LIBS ;
	PNG_CFLAGS=$CFLAGS ;;
  false) AC_MSG_RESULT(no); PNG_LIBS=""; PNG_CFLAGS="";;
esac
  
CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

fi

AC_SUBST(PNG_CFLAGS)
AC_SUBST(PNG_LIBS)
AC_SUBST(PNG_REQUIRED)

AM_CONDITIONAL(HAVE_LIBPNG, test x$have_libpng = xtrue)

dnl
dnl faac
dnl

have_faac="false"
FAAC_REQUIRED="1.24"
AH_TEMPLATE([HAVE_FAAC], [Enable faac])

if test $have_gpl = "true"; then

AC_ARG_WITH([faac], AS_HELP_STRING([--without-faac], [Build without faac library (default: test)]))

if test "x$with_faac" != "xno"; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

LIBS="$LIBS -lfaac -lm"
CFLAGS="$CFLAGS"

AC_MSG_CHECKING(for faac)
AC_TRY_RUN([
    #include <inttypes.h>
    #include <faac.h>
    main()
    {
    int samplerate = 44100, num_channels = 2;
    unsigned long input_samples, output_bytes;
    faacEncHandle enc;
    enc = faacEncOpen(samplerate,
                      num_channels,
                      &input_samples,
                      &output_bytes);

    return 0;
    }
  ],
  [
    # program could be run
    have_faac="true"
    AC_MSG_RESULT(yes)
    FAAC_CFLAGS=$CFLAGS
    FAAC_LIBS=$LIBS

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

fi

fi
AC_SUBST(FAAC_CFLAGS)
AC_SUBST(FAAC_LIBS)
AC_SUBST(FAAC_REQUIRED)

AM_CONDITIONAL(HAVE_FAAC, test x$have_faac = xtrue)

if test x$have_faac = xtrue; then
AC_DEFINE(HAVE_FAAC)
fi


dnl
dnl FAAD2
dnl
FAAD2_REQUIRED="2.0"
AH_TEMPLATE([HAVE_FAAD2], [Enable FAAD2])
AH_TEMPLATE([HAVE_NEAACDEC_H], [Use new header file for faad2])

have_faad2="false"

AC_ARG_WITH([faad2], AS_HELP_STRING([--without-faad2], [Build without faad2 library (default: test)]))

if test $have_gpl = "true"; then

if test "x$with_faad2" != "xno"; then

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

CFLAGS="$CFLAGS"
LIBS="$LIBS -lfaad -lm"

AC_MSG_CHECKING(for neaacdec.h usability for faad2)


AC_TRY_RUN([
    #include <neaacdec.h>
    #include <stdio.h>
    main()
    {
    int faad_major;
    int faad_minor;
    faacDecHandle dec;

    if(sscanf(FAAD2_VERSION, "%d.%d", &faad_major, &faad_minor) < 2)
      return -1;
    dec = faacDecOpen();
    if(!dec)
      return -1;
    return 0;
    }
  ],
  [
    # program could be run
    have_faad2="true"
    AC_MSG_RESULT(yes)
    FAAD2_CFLAGS=$CFLAGS
    FAAD2_LIBS=$LIBS
    AC_DEFINE(HAVE_NEAACDEC_H)
  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

if test "x$have_faad2" != "xtrue"; then

AC_MSG_CHECKING(for faad.h usability for faad2)

AC_TRY_RUN([
    #include <faad.h>
    #include <stdio.h>
    main()
    {
    int faad_major;
    int faad_minor;
    faacDecHandle dec;

    if(sscanf(FAAD2_VERSION, "%d.%d", &faad_major, &faad_minor) < 2)
      return -1;
    dec = faacDecOpen();
    if(!dec)
      return -1;
    return 0;
    }
  ],
  [
    # program could be run
    have_faad2="true"
    AC_MSG_RESULT(yes)
    FAAD2_CFLAGS=$CFLAGS
    FAAD2_LIBS=$LIBS
    

  ],
    # program could not be run
    AC_MSG_RESULT(no)
)

fi

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

fi

fi

AC_SUBST(FAAD2_CFLAGS)
AC_SUBST(FAAD2_LIBS)
AC_SUBST(FAAD2_REQUIRED)
AM_CONDITIONAL(HAVE_FAAD2, test x$have_faad2 = xtrue)

if test x$have_faad2 = xtrue; then
AC_DEFINE(HAVE_FAAD2)
fi

dnl
dnl Check for x264
dnl
AH_TEMPLATE([HAVE_X264], [Enable X264])

have_x264="false"

AC_ARG_WITH([x264], AS_HELP_STRING([--without-x264], [Build without x264 library (default: test)]))

if test $have_gpl = "true"; then
X264_REQUIRED="0.48"

if test "x$with_x264" != "xno"; then
PKG_CHECK_MODULES(X264, x264 >= $X264_REQUIRED, have_x264="true", have_x264="false")
fi
AC_SUBST(X264_REQUIRED)
AC_SUBST(X264_LIBS)
AC_SUBST(X264_CFLAGS)

fi
AM_CONDITIONAL(HAVE_X264, test x$have_x264 = xtrue)

dnl
dnl Plugin directory
dnl

if test "x$prefix" = xNONE; then
  prefix="${ac_default_prefix}"
fi
if test "x$exec_prefix" = xNONE; then
   exec_prefix="${prefix}"
fi
plugin_dir="$libdir/$PACKAGE"

AH_TEMPLATE([PLUGIN_DIR], [Directory for plugins])

PLUGIN_DIR=`eval echo $plugin_dir`
AC_DEFINE_UNQUOTED(PLUGIN_DIR, "$PLUGIN_DIR")
AC_SUBST(PLUGIN_DIR)

dnl
dnl Build CFLAGS
dnl

COMMON_CFLAGS=""
lqt_test_cflags="-finline-functions -Wall -Winline"
for i in $lqt_test_cflags; do
  LQT_TRY_CFLAGS($i, COMMON_CFLAGS="$COMMON_CFLAGS $i") ;
done

if test "x${USER_CFLAGS}" = "x"; then

  dnl Optimizing flags

  LQT_OPT_CFLAGS($host_cpu, ["-O3 -funroll-all-loops -fomit-frame-pointer"])

  CFLAGS="$OPT_CFLAGS"

fi

AH_TEMPLATE([NDEBUG],
            [Causes debug code to e removed])

if test x$LQT_DEBUG != xtrue; then
AC_DEFINE(NDEBUG)
fi

CFLAGS="$CFLAGS $COMMON_CFLAGS"

# -L is required so that linker can find libquicktime when DESTDIR is used.
# libtool bug, documented at: http://www.geocrawler.com/mail/thread.php3?subject=install+phase+fails&list=404
# MODULE_LIBADD="-L$prefix/lib ../../src/libquicktime.la"
# The above is OK iff the added library is already installed. The
# current
# libtool version does not support linking libraries with uninstalled
# libraries as occurs when Debian packages are created for the 1st time.
# Linking against old libraries may not be desirable!
MODULE_LIBADD=../../src/libquicktime.la 
AC_SUBST(MODULE_LIBADD)

UTIL_LIBADD=../src/libquicktime.la
AC_SUBST(UTIL_LIBADD)

GTKUTIL_LIBADD=../../src/libquicktime.la
AC_SUBST(GTKUTIL_LIBADD)

dnl Define flags for application to use.

LQT_CFLAGS=""
LQT_LIBS="-lquicktime $LIBS"

AC_SUBST(LQT_CFLAGS)
AC_SUBST(LQT_LIBS)

dnl Write all the makefiles and other stuff

AC_OUTPUT([Makefile 
po/Makefile.in \
libquicktime.spec \
libquicktime.pc \
doc/Makefile \
debian/Makefile \
src/Makefile \
include/Makefile \
include/quicktime/Makefile \
include/quicktime/lqt_version.h \
plugins/Makefile \
plugins/dv/Makefile \
plugins/faad2/Makefile \
plugins/faac/Makefile \
plugins/lame/Makefile \
plugins/mjpeg/Makefile \
plugins/audiocodec/Makefile \
plugins/videocodec/Makefile \
plugins/vorbis/Makefile \
plugins/png/Makefile \
plugins/rtjpeg/Makefile \
plugins/ffmpeg/Makefile \
plugins/x264/Makefile \
utils/Makefile \
utils/gtk/Makefile \
lqt-config], [chmod a+x lqt-config])

echo "Configuration: "
$have_gtk

echo -n "libdv:      "
if test x$have_libdv = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://libdv.sourceforge.net)"
fi

echo -n "vorbis:     "
if test x$have_vorbis = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://www.vorbis.com/)"
fi

echo -n "lame:       "
if test x$have_lame = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://www.mp3dev.org/)"
fi


echo -n "libjpeg:    "
if test x$have_libjpeg = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://www.ijg.org/)"
fi

echo -n "libpng:     "
if test x$have_libpng = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://www.libpng.org/pub/png/libpng.html)"
fi

echo -n "libavcodec: "
if test x$have_libavcodec = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://www.ffmpeg.org/)"
fi

echo -n "libswscale: "
if test x$have_libswscale = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://www.ffmpeg.org/)"
fi

echo -n "gtk >= $GTK_REQUIRED: "
if test x$have_gtk = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://www.gtk.org/)"
fi

echo -n "Alsa        "
if test x$have_alsa = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://www.alsa-project.org/)"
fi  

echo -n "GPL plugins "
if test x$have_gpl = "xtrue"; then
echo "Enabled"
else
echo "Disabled"
fi  

if test x$have_gpl = "xtrue"; then

echo -n "faac        "
if test x$have_faac = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://www.audiocoding.com/)"
fi  

echo -n "faad2       "
if test x$have_faad2 = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://www.audiocoding.com/)"
fi  

echo -n "x264        "
if test x$have_x264 = "xtrue"; then
echo "Yes"
else
echo "Missing (Go to http://developers.videolan.org/x264.html)"
fi  

fi
