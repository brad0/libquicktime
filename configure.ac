AC_INIT([libquicktime],[1.2.4],[http://libquicktime.sourceforge.net])
AC_CONFIG_MACRO_DIRS([m4])
AM_INIT_AUTOMAKE([1.7])

LQT_VERSION=$VERSION
LQT_VERSION_MAJOR=`echo $VERSION | cut -d . -f 1`
LQT_VERSION_MINOR=`echo $VERSION | cut -d . -f 2`
LQT_VERSION_MICRO=`echo $VERSION | cut -d . -f 3 | cut -d p -f 1`

USER_CFLAGS=$CFLAGS

AC_CONFIG_HEADERS(config.h)
AC_CANONICAL_HOST
AC_LIBTOOL_PICMODE

AM_MAINTAINER_MODE

AH_TEMPLATE([LQT_VERSION], [Libquicktime version])

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

LQT_CODEC_API_VERSION="14"

AH_TEMPLATE([HAVE_GPL], [Enable GPL code])
AH_TEMPLATE([LQT_CODEC_API_VERSION], [Libquicktime codec API version])

if test "x$have_gpl" = "xtrue"; then
  LQT_CODEC_API_VERSION="($LQT_CODEC_API_VERSION|0x10000)"
  AC_DEFINE(HAVE_GPL)
fi

AC_DEFINE_UNQUOTED(LQT_CODEC_API_VERSION, $LQT_CODEC_API_VERSION)
AC_SUBST(LQT_CODEC_API_VERSION)

AC_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AC_PROG_CC
AC_PROG_MKDIR_P
AC_LANG_C
AC_C_BIGENDIAN
AC_SYS_LARGEFILE
AC_CACHE_SAVE

AM_PROG_LIBTOOL

dnl
dnl Doxygen
dnl

test_doxygen="true"
have_doxygen="false"

AC_ARG_WITH(doxygen,
    AC_HELP_STRING([--without-doxygen],
    [disable documentation generation]),
    test_doxygen="false")

if test "x$test_doxygen" = "xtrue"; then
   AC_CHECK_PROG(DOXYGEN, doxygen, "doxygen")
   if test -z "$DOXYGEN"; then
      AC_MSG_ERROR([Doxygen not found, use --without-doxygen to disable documentation generation])
   else
      have_doxygen="true"
  fi
fi

AM_CONDITIONAL(HAVE_DOXYGEN, test "x$have_doxygen" = "xtrue")

dnl
dnl Gettext
dnl
AM_GNU_GETTEXT_VERSION([0.17])
AM_GNU_GETTEXT([external])

PKG_PROG_PKG_CONFIG

dnl This is necessary for libtool assembler support
AS=gcc
ASFLAGS=
MODULE_LDFLAGS=""

AC_SUBST(AS)
AC_SUBST(ASFLAGS)
AC_SUBST(LIBS)

AH_TEMPLATE([_GNU_SOURCE], [Define to enable GNU extensions of glibc, notably large file support])
           
AH_TEMPLATE([YUV_420_USE_YV12], [Define to 1 to use planar YUV format for 420 blocks in IEC PAL])

AH_TEMPLATE([ARCH_X86], [Define as 1 if host is an IA32])
AH_TEMPLATE([LINUX], [Define as 1 if host is Linux])

case "$host_os" in
	linux*)
		AC_DEFINE(LINUX)
                MODULE_LDFLAGS="$MODULE_LDFLAGS -no-undefined"
		;;
	mingw*)
		MODULE_LDFLAGS="$MODULE_LDFLAGS -no-undefined"
		;;
	*)
		echo $host_os
		;;
esac

AC_SUBST(MODULE_LDFLAGS)

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

AC_CHECK_HEADERS(fcntl.h sys/param.h sys/time.h unistd.h linux/videodev.h sys/soundcard.h soundcard.h stddef.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Checks for library functions.

# In case we are building for Windows via MinGW, we may need to use msvcrt's
# localtime_s - however for most MinGW builds, localtime_s isn't exported so
# check for _localtime32_s/_localtime64_s instead.
AC_CHECK_FUNCS(localtime_r, , [AC_CHECK_FUNCS([_localtime32_s _localtime64_s])])
AC_CHECK_FUNCS([getpagesize gettimeofday memalign posix_memalign lrint vasprintf])
AC_CHECK_FUNCS(fseeko, [have_fseeko="true"])
AM_CONDITIONAL(HAVE_FSEEKO, test x"$have_fseeko" = "xtrue")

AC_HEADER_STDC

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
 
dnl
dnl Check for lame
dnl

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
   AC_CHECK_LIB(GL, glBegin,have_gl="yes",have_gl="no")
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

AH_TEMPLATE([HAVE_ALSA], [Do we have Alsa?])

AC_ARG_WITH([alsa], AS_HELP_STRING([--without-alsa], [Build without alsa library (default: test)]))

have_libalsa="false"
if test "x$with_alsa" != "xno"; then
   PKG_CHECK_MODULES(ALSA, [alsa >= 0.9],
                           [have_alsa=true
			    AC_DEFINE(HAVE_ALSA)],
			   have_alsa=false)
   AC_SUBST(ALSA_CFLAGS)
fi
AM_CONDITIONAL(HAVE_ALSA, test x$have_alsa = xtrue)

dnl
dnl Check for sndio
dnl

AH_TEMPLATE([HAVE_SNDIO], [Do we have sndio?])

AC_ARG_WITH([sndio], AS_HELP_STRING([--without-sndio], [Build without sndio (default: test)]))

if test "x$with_sndio" != "xno"; then
   SNDIO_CFLAGS=""
   AC_CHECK_HEADER(sndio.h,have_sndio="true",have_sndio="false")

   if test x$have_sndio = xtrue; then
      AC_CHECK_LIB(sndio,sio_open,have_sndio="true",have_sndio="false")
   fi

   if test x$have_sndio = xtrue; then
      SNDIO_LIBS="-lsndio"
      AC_DEFINE(HAVE_SNDIO)
      AC_SUBST(SNDIO_CFLAGS)
      AC_SUBST(SNDIO_LIBS)
   fi
fi
AM_CONDITIONAL(HAVE_SNDIO, test x$have_sndio = xtrue)

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

dnl
dnl Check for libdv
dnl

AC_ARG_WITH([libdv], AS_HELP_STRING([--with-libdv], [Build without libdv support (default: without)]))

if test x$with_libdv = xyes; then
   PKG_CHECK_MODULES(LIBDV, [libdv >= 0.9], have_libdv="true", have_libdv="false")
   AC_SUBST(LIBDV_LIBS)
   AC_SUBST(LIBDV_CFLAGS)
fi
AM_CONDITIONAL(HAVE_LIBDV, test x$have_libdv = xtrue)

dnl
dnl Check for libjpeg
dnl

AH_TEMPLATE([HAVE_LIBJPEG], [Do we have libjpeg installed?])

have_libjpeg=false

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
  AC_SUBST(JPEG_LIBS)
  AC_SUBST(JPEG_CFLAGS)
fi
AM_CONDITIONAL(HAVE_LIBJPEG, test x$have_libjpeg = xtrue)

dnl
dnl libswscale
dnl

AH_TEMPLATE([HAVE_LIBSWSCALE], [Do we have libswscale installed?])

have_libswscale=false

AC_ARG_ENABLE(libswscale,
[AC_HELP_STRING([--disable-libswscale],[Disable libswscale (default: autodetect)])],
[case "${enableval}" in
   yes) test_libswscale=true ;;
   no)  test_libswscale=false ;;
esac],[test_libswscale=true])

if test x$test_libswscale = xtrue; then
   PKG_CHECK_MODULES(LIBSWSCALE, libswscale, 
                                 [have_libswscale="true"
				  AC_DEFINE(HAVE_LIBSWSCALE)],
				 have_libswscale="false")
   AC_SUBST(LIBSWSCALE_LIBS)
   AC_SUBST(LIBSWSCALE_CFLAGS)
fi
AM_CONDITIONAL(HAVE_LIBSWSCALE, test x$have_libswscale = xtrue)

dnl
dnl Check for libavcodec (ffmpeg)
dnl

AH_TEMPLATE([HAVE_LIBAVCODEC], [Do we have libavcodec installed?])

have_libavcodec=false

AC_ARG_WITH([ffmpeg], AS_HELP_STRING([--without-ffmpeg], [Build without ffmpeg library (default: test)]))

if test "x$with_ffmpeg" != "xno"; then
   PKG_CHECK_MODULES(LIBAVCODEC, [libavcodec < 59.0.0], 
                                 [have_libavcodec="true"
				  AC_DEFINE(HAVE_LIBAVCODEC)],
				 have_libavcodec="false")
   AC_SUBST(LIBAVCODEC_LIBS)
   AC_SUBST(LIBAVCODEC_CFLAGS)
fi

AM_CONDITIONAL(HAVE_LIBAVCODEC, test x$have_libavcodec = xtrue)

dnl
dnl Check for libpng
dnl 

AH_TEMPLATE([HAVE_LIBPNG], [Enable png codec])

AC_ARG_WITH(libpng,
            AC_HELP_STRING([--without-libpng], [Do not use libpng.]),
            [], [with_libpng=yes])

have_libpng=false
if test x$with_libpng != xno ; then
   PKG_CHECK_MODULES(LIBPNG, [libpng], [have_libpng=true], [have_libpng=false])
   AC_SUBST(LIBPNG_CFLAGS)
   AC_SUBST(LIBPNG_LIBS)
fi
AM_CONDITIONAL(HAVE_LIBPNG, test x$have_libpng = xtrue)

dnl
dnl Check for libschroedinger
dnl 

AH_TEMPLATE([HAVE_SCHROEDINGER], [Enable dirac codec])

AC_ARG_WITH(schroedinger,
            AC_HELP_STRING([--without-schroedinger], [Do not use schroedinger.]),
            [], [with_schroedinger=yes])

have_schroedinger=false
if test x$with_schroedinger != xno ; then
   PKG_CHECK_MODULES(SCHROEDINGER, [schroedinger-1.0 >= 1.0.5],
                                  [have_schroedinger=true
				   AC_DEFINE(HAVE_SCHROEDINGER)],
				   [have_schroedinger=false])
   AC_SUBST(SCHROEDINGER_CFLAGS)
   AC_SUBST(SCHROEDINGER_LIBS)
fi
AM_CONDITIONAL(HAVE_SCHROEDINGER, test x$have_schroedinger = xtrue)

dnl
dnl faac
dnl

have_faac="false"
AH_TEMPLATE([HAVE_FAAC], [Enable faac])

if test $have_gpl = "true"; then
   AC_ARG_WITH([faac], AS_HELP_STRING([--without-faac], [Build without faac library (default: test)]))

   if test "x$with_faac" != "xno"; then
      PKG_CHECK_MODULES(FAAC, [faac >= 1.24],
                              [have_faac=true
			       AC_DEFINE(HAVE_FAAC)], 
			      [have_faac=false])
      AC_SUBST(FAAC_CFLAGS)
      AC_SUBST(FAAC_LIBS)
   fi
fi
AM_CONDITIONAL(HAVE_FAAC, test x$have_faac = xtrue)

dnl
dnl FAAD2
dnl

have_faad2="false"
AH_TEMPLATE([HAVE_FAAD2], [Enable FAAD2])

if test $have_gpl = "true"; then
   AC_ARG_WITH([faad2], AS_HELP_STRING([--without-faad2], [Build without faad2 library (default: test)]))

   if test "x$with_faad2" != "xno"; then
      PKG_CHECK_MODULES(FAAD2, [faad2 >= 2.0],
                               [have_faad2=true
			        AC_DEFINE(HAVE_FAAD2)], 
			       [have_faad2=false])
   AC_SUBST(FAAD2_CFLAGS)
   AC_SUBST(FAAD2_LIBS)
   fi
fi
AM_CONDITIONAL(HAVE_FAAD2, test x$have_faad2 = xtrue)

dnl
dnl Check for x264
dnl

have_x264="false"
AH_TEMPLATE([HAVE_X264], [Enable X264])

if test $have_gpl = "true"; then
   AC_ARG_WITH([x264], AS_HELP_STRING([--without-x264], [Build without x264 library (default: test)]))

   if test "x$with_x264" != "xno"; then
      PKG_CHECK_MODULES(X264,[x264 >= 0.48],have_x264="true",have_x264="false")
      AC_SUBST(X264_LIBS)
      AC_SUBST(X264_CFLAGS)
   fi
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
lqt_test_cflags="-finline-functions -Wall -Winline -Wmissing-declarations -Wdeclaration-after-statement"
for i in $lqt_test_cflags; do
  LQT_TRY_CFLAGS($i, COMMON_CFLAGS="$COMMON_CFLAGS $i") ;
done

dnl
dnl GCC Visibility
dnl

AH_TEMPLATE([HAVE_GCC_VISIBILITY], [GCC Visibility support])

AC_ARG_WITH([visibility], AS_HELP_STRING([--without-visibility], [Build without gcc visibility (default: test)]))

if test "x$with_visibility" != "xno"; then
LQT_TRY_CFLAGS("-fvisibility=hidden",
               COMMON_CFLAGS="$COMMON_CFLAGS -fvisibility=hidden";
	       AC_DEFINE(HAVE_GCC_VISIBILITY))
fi

if test "x${USER_CFLAGS}" = "x"; then
  LQT_OPT_CFLAGS($host_cpu, ["-O3 -funroll-all-loops -fomit-frame-pointer"])
  CFLAGS="$OPT_CFLAGS"
fi

AH_TEMPLATE([NDEBUG], [Causes debug code to e removed])

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

dnl Absolute src path for doxygen

TOP_SRCDIR=`cd ${srcdir} && pwd` 
AC_SUBST(TOP_SRCDIR)

dnl Write all the makefiles and other stuff

AC_OUTPUT([Makefile 
po/Makefile.in \
libquicktime.pc \
doc/Makefile \
doc/Doxyfile \
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
plugins/schroedinger/Makefile \
plugins/x264/Makefile \
utils/Makefile \
utils/gtk/Makefile])

AC_MSG_NOTICE([])
AC_MSG_NOTICE([ libquicktime ${VERSION} build configuration:])
AC_MSG_NOTICE([])

if test x$have_libdv = "xtrue"; then
AC_MSG_NOTICE([  -libdv                 : enabled])
else
AC_MSG_NOTICE([  -libdv                 : disabled (default) / missing])
fi
AC_MSG_NOTICE([  -vorbis                : ${have_vorbis}])
AC_MSG_NOTICE([  -lame                  : ${have_lame}])
AC_MSG_NOTICE([  -libjpeg               : ${have_libjpeg}])
AC_MSG_NOTICE([  -libpng                : ${have_libpng}])
AC_MSG_NOTICE([  -nschroedinger         : ${have_schroedinger}])
AC_MSG_NOTICE([  -libavcodec            : ${have_libavcodec}])
AC_MSG_NOTICE([  -libswscale            : ${have_libswscale}])
AC_MSG_NOTICE([  -gtk >= $GTK_REQUIRED          : ${have_gtk}])
AC_MSG_NOTICE([  -Alsa                  : ${have_alsa}])
AC_MSG_NOTICE([  -sndio                 : ${have_sndio}])
AC_MSG_NOTICE([  -GPL plugins           : ${have_gpl}])
if test x$have_gpl = "xtrue"; then
AC_MSG_NOTICE([       faac              : ${have_faac}])
AC_MSG_NOTICE([       faad2             : ${have_faad2}])
AC_MSG_NOTICE([       x264              : ${have_x264}])
fi
