AC_INIT(src/lqt_quicktime.c)
dnl Every other copy of the package version number gets its value from here
AM_INIT_AUTOMAKE(libquicktime, 1.5.1.ap012502)
AM_CONFIG_HEADER(config.h)

AC_CANONICAL_HOST
AM_MAINTAINER_MODE

AC_LIBTOOL_PICMODE

dnl
dnl Libquicktime codec API version
dnl 

LQT_CODEC_API_VERSION="2"

AH_TEMPLATE([LQT_CODEC_API_VERSION],
            [Libquicktime codec API version])

AC_DEFINE(LQT_CODEC_API_VERSION, $LQT_CODEC_API_VERSION)
AC_SUBST(LQT_CODEC_API_VERSION)


dnl
dnl set GMERLIN to "true" for gmerlin compilation
dnl

dnl gmerlin=true

dnl Gmerlin does not need static libs

if test x$gmerlin = xtrue; then
AC_DISABLE_STATIC
AM_DISABLE_STATIC
else
AC_ENABLE_STATIC
fi

AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL
AC_PROG_RANLIB

AC_PROG_CC
AC_LANG_C

AC_CACHE_SAVE

dnl This is necessary for libtool assembler support

AS=gcc
ASFLAGS=

AC_SUBST(AS)
AC_SUBST(ASFLAGS)

dnl Build CFLAGS

COMMON_CFLAGS=" -finline-functions -W -Wall -Wno-unused -Winline"
PROFILE_CFLAGS=" -pg -fprofile-arcs -ftest-coverage"
DEBUG_CFLAGS=" -g3"
OPTIMIZE_CFLAGS=" -O3 -funroll-all-loops -fomit-frame-pointer -malign-loops=2 -malign-jumps=2 -malign-functions=2"

# -Wno-unused-variable -Wno-unused-parameter
# -std=gnu99 

have_debug=false
have_profile=false
compile_mode="optimize"

AH_TEMPLATE([NDEBUG],
            [Causes debug code to e removed])

AC_ARG_ENABLE(profile,
    [  --enable-profile        Turn on profile ],
     [case "${enableval}" in
       yes) have_profile=true ;;
       no) ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-profile) ;;
     esac],[])

AC_ARG_ENABLE(debug,
    [  --enable-debug          Turn on debugging (turn off optimizations)],
    [case "${enableval}" in
       yes) have_debug=true ;;
       no) ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-debug) ;;
     esac],[])

if test x$have_debug = xtrue; then
compile_mode="debug"
fi

if test x$have_profile = xtrue; then
compile_mode="profile"
fi

case $compile_mode in
"optimize") CFLAGS="$OPTIMIZE_CFLAGS $COMMON_CFLAGS"; AC_DEFINE(NDEBUG);;
"debug") CFLAGS="$DEBUG_CFLAGS $COMMON_CFLAGS" ;;
"profile") CFLAGS="$PROFILE_CFLAGS $COMMON_CFLAGS"; AC_DEFINE(NDEBUG);;
esac

dnl AC_ARG_ENABLE(optimization,
dnl     [  --disable-optimization  Turn off opimization ],
dnl      [case "${enableval}" in
dnl        yes) CFLAGS="$CFLAGS -O2" ;;
dnl        no) CFLAGS="$CFLAGS -g3 -O0" ;;
dnl        *) AC_MSG_ERROR(bad value ${enableval} for --disable-optimization) ;;
dnl      esac],[CFLAGS="$CFLAGS -O2"])

dnl AC_ARG_ENABLE(final,
dnl     [  --enable-final          Make a binary suitable for distribution  ],
dnl      [case "${enableval}" in
dnl        yes) CFLAGS="-O3 -W -Wall -Winline"
dnl 	AC_DEFINE(NDEBUG) ;;
dnl        no)  ;;
dnl        *) AC_MSG_ERROR(bad value ${enableval} for --disable-final) ;;
dnl      esac],[])

dnl LIBS="$LIBS $SIGC_LIBS"
dnl CFLAGS="$CFLAGS $SIGC_CFLAGS"
dnl CFLAGS="$CFLAGS $SIGC_CFLAGS $SIGC_CFLAGS"

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

dnl Checks for libraries.

AC_CHECK_LIB(dl, dlopen)

AC_CHECK_LIB(raw1394, raw1394_write)
AC_CHECK_LIB(avc1394, avc1394_send_command)
AC_CHECK_LIB(rom1394, main)

AC_SEARCH_LIBS(pthread_create, [gthread pthread])

dnl
dnl Check for gtk
dnl THis is neccesary for the utils/gtk subdirectory
dnl

have_gtk=false

AM_PATH_GTK(1.2.8, have_gtk=true)

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

AM_CONDITIONAL(HAVE_GTK, test x$have_gtk = xtrue)


AC_SUBST(LIBS)

dnl CPPFLAGS="$CPPFLAGS"
dnl LDFLAGS="$LDFLAGS"

AC_CHECK_HEADERS(fcntl.h sys/time.h unistd.h linux/videodev.h sys/soundcard.h stddef.h)

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_INLINE
AC_TYPE_OFF_T
AC_TYPE_SIZE_T
AC_HEADER_TIME

dnl Checks for library functions.
AC_FUNC_MMAP
AC_CHECK_FUNCS(gettimeofday)

ENCORE_DIR=encore50
DECORE_DIR=decore50

AH_TEMPLATE([ENCORE_INCLUDE],
            [Which encore library to use.])
AH_TEMPLATE([DECORE_INCLUDE],
            [Which decore library to use.])

AC_DEFINE_UNQUOTED(ENCORE_INCLUDE, "$ENCORE_DIR/encore.h")
AC_DEFINE_UNQUOTED(DECORE_INCLUDE, "$DECORE_DIR/decore.h")

AC_HEADER_STDC

AC_ARG_ENABLE(complete-dist,
[  --enable-complete-dist  Include the built copies of the library source if availiable. ])

COMPLETE_DIST=""; 

if test "x$enable_complete_dist" == "xyes"; then
	test -d libraw1394 && COMPLETE_DIST="$COMPLETE_DIST libraw1394";
	test -d libogg-1.0rc2 && COMPLETE_DIST="$COMPLETE_DIST libogg-1.0rc2";
	test -d libvorbis-1.0rc2 && COMPLETE_DIST="$COMPLETE_DIST libvorbis-1.0rc2";
fi

AC_SUBST(COMPLETE_DIST)

if test x$gmerlin = xtrue; then

enable_firewire=no
FIREWIRE_CFLAGS=""
FIREWIRE_LIBS=""
FIREWIRE_SUBDIRS=""

else

AH_TEMPLATE([HAVE_FIREWIRE],
            [tell libdv.c to support firewire])

AC_MSG_CHECKING(whether to enable firewire support)

AC_ARG_ENABLE(firewire,
 [  --disable-firewire      disable ieee1394 (firewire) in the library. This prevents 
			  Broadcast 2000 and Cinelerra from building. This code will 
			  not work with recent versions of the linux ieee1394 driver. ],
 [case "${enableval}" in
       yes) enable_firewire=yes ;;
       no) enable_firewire=no ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-firewire) ;;
 esac], [enable_firewire=yes])

AC_MSG_RESULT($enable_firewire)

if test x$enable_firewire = xyes; then
 	AC_DEFINE(HAVE_FIREWIRE, 1)
 	FIREWIRE_SUBDIRS=""
 	FIREWIRE_LIBS="unknown"
 
 	if test -d libraw1394; then
 
 	AC_ARG_WITH(internal-raw1394,
 [  --with-internal-raw1394 Use libraw1394 from the distribution. The default is to use
                           versions installed on the system if possible. ],
 	     [case "${withval}" in
 	       yes) FIREWIRE_SUBDIRS="libraw1394"
 		    FIREWIRE_LIBS="libraw1394/src/libraw1394.la"
 		    FIREWIRE_CFLAGS="" ;;
 	       no) FIREWIRE_LIBS="unknown";;
 	       *) AC_MSG_ERROR(bad value ${enableval} for --disable-final) ;;
 	     esac],[ FIREWIRE_LIBS="unknown" ])

 	fi

 	if test "x$FIREWIRE_LIBS" = "xunknown"; then
 		FIREWIRE_LIBS=""
 		AC_LIB_RAW1394([0.9], [
 				 FIREWIRE_CFLAGS="$LIBRAW1394_CPPFLAGS $LIBRAW1394_CFLAGS"; 
 				 FIREWIRE_LIBS="$LIBRAW1394_LIBS" ],
 	 [  FIREWIRE_SUBDIRS="libraw1394"
 	    FIREWIRE_LIBS="libraw1394/src/libraw1394.la"
 	    FIREWIRE_CFLAGS="" ] )
 	fi
 
fi

fi

AC_SUBST(FIREWIRE_LIBS)
AC_SUBST(FIREWIRE_CFLAGS)
AC_SUBST(FIREWIRE_SUBDIRS)

dnl
dnl Check for libdv
dnl

have_libdv=false

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

LIBS=-ldv
CFLAGS=""

AC_MSG_CHECKING(for libdv with encoder)
AC_TRY_LINK([#include <stdio.h>
             #include <libdv/dv.h>],
            [dv_decoder_t *decoder;
	     unsigned char *data, **frame;
	     int *pitches;
             decoder = dv_decoder_new();
	     dv_decode_full_frame( decoder, data,
				   e_dv_color_rgb, frame,
				   pitches );
	     dv_encode_full_frame( frame, data, e_dv_color_yuv,
				   0, 0, 3, 0, DV_DCT_AUTO );],
	    [have_libdv=true])


case $have_libdv in
  true) dnl AC_DEFINE(HAVE_LIBDV)
        AC_MSG_RESULT(yes)
	LIBDV_LIBS=$LIBS;
	LIBDV_CFLAGS=$CFLAGS;;
  false) AC_MSG_RESULT(no); LIBDV_LIBS=""; LIBDV_CFLAGS="";;
  * ) AC_MSG_RESULT("Somethings wrong: $have_libdv") ;;
esac

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(LIBDV_LIBS)
AC_SUBST(LIBDV_CFLAGS)

AM_CONDITIONAL(HAVE_LIBDV, test x$have_libdv = xtrue)

dnl
dnl Check for Vorbis
dnl 

have_vorbis=false
AH_TEMPLATE([HAVE_VORBIS], [Vorbis libraries are there])

AM_PATH_VORBIS(have_vorbis=true)
AM_CONDITIONAL(HAVE_VORBIS, test x$have_vorbis = xtrue)

if test x$have_vorbis = true; then
AC_DEFINE(HAVE_VORBIS)
fi

dnl
dnl Check for libjpeg
dnl

AH_TEMPLATE([HAVE_LIBJPEG],
            [Do we have libjpeg installed?])

if test x$gmerlin = xtrue; then
  JPEG_CFLAGS=-I../jpeg-mmx
  JPEG_LIBS=../jpeg-mmx/libjpeg_mmx.la
  have_libjpeg=true
  AC_DEFINE(HAVE_LIBJPEG)
else
  have_libjpeg=false

  OLD_CFLAGS=$CFLAGS
  OLD_LIBS=$LIBS

  LIBS=-ljpeg-mmx
  CFLAGS=""

  AC_MSG_CHECKING(for jpeg-mmx)
  AC_TRY_LINK([#include <stdio.h>
               #include <jpeglib.h>],
              [struct jpeg_decompress_struct cinfo;
               jpeg_create_decompress(&cinfo);],
	       [have_libjpeg=true])

  if test x$have_libjpeg = xfalse; then
    AC_MSG_RESULT(no);
    LIBS=-ljpeg
    CFLAGS=""
    AC_MSG_CHECKING(for libjpeg)
    AC_TRY_LINK([#include <stdio.h>
                 #include <jpeglib.h>],
                [struct jpeg_decompress_struct cinfo;
	         jpeg_create_decompress(&cinfo);],
	         [have_libjpeg=true])
  fi

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

AM_CONDITIONAL(HAVE_LIBJPEG, test x$have_libjpeg = xtrue)

dnl
dnl Check for libpng
dnl 

AH_TEMPLATE([HAVE_LIBPNG], [Enable png codec])

if test x$gmerlin = xtrue; then
  PNG_CFLAGS=""
  PNG_LIBS="-lpng -lz"
  have_libpng=true
  AC_DEFINE(HAVE_LIBPNG)
else
  have_libpng=false

  OLD_CFLAGS=$CFLAGS
  OLD_LIBS=$LIBS

  LIBS="-lpng -lz"
  CFLAGS=""

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

AM_CONDITIONAL(HAVE_LIBPNG, test x$have_libpng = xtrue)

dnl AC_LIB_RAW1394(1.0, [LIBRAW1394="-lraw1394"], [AC_CONFIG_SUBDIRS(libraw1394) LIBRAW1394="libraw1394/libraw1394.la"])

dnl AC_CONFIG_SUBDIRS($VORBIS_SUBDIRS $FIREWIRE_SUBDIRS)

dnl jpeg/Makefile

AM_CONDITIONAL(GMERLIN, test x$gmerlin = xtrue)

if test x$gmerlin = xtrue; then
  if test "x$prefix" = xNONE; then
     prefix="${ac_default_prefix}"
  fi
  if test "x$exec_prefix" = xNONE; then
     exec_prefix="${prefix}"
  fi
  libdir=$exec_prefix/lib/gmerlin
  plugin_dir="$exec_prefix/lib/gmerlin/$PACKAGE"
else
  if test "x$prefix" = xNONE; then
     prefix="${ac_default_prefix}"
  fi
  if test "x$exec_prefix" = xNONE; then
     exec_prefix="${prefix}"
  fi
     plugin_dir="$exec_prefix/lib/$PACKAGE"
fi

AH_TEMPLATE([PLUGIN_DIR], [Directory for plugins])

AC_DEFINE_UNQUOTED(PLUGIN_DIR, ["$plugin_dir"])

# -L is required so that linker can find libquicktime when DESTDIR is used.
# libtool bug, documented at: http://www.geocrawler.com/mail/thread.php3?subject=install+phase+fails&list=404
MODULE_LIBADD="-L$prefix/lib ../../src/libquicktime.la"
AC_SUBST(MODULE_LIBADD)

UTIL_LIBADD=../src/libquicktime.la
AC_SUBST(UTIL_LIBADD)

GTKUTIL_LIBADD=../../src/libquicktime.la
AC_SUBST(GTKUTIL_LIBADD)

dnl
dnl LIBQUICKTIME
dnl

AC_OUTPUT([Makefile \
src/Makefile \
include/Makefile \
include/quicktime/Makefile \
include/quicktime/version.h \
plugins/Makefile \
plugins/dv/Makefile \
plugins/mjpeg/Makefile \
plugins/audiocodec/Makefile \
plugins/videocodec/Makefile \
plugins/vorbis/Makefile \
plugins/png/Makefile \
plugins/opendivx/Makefile \
plugins/opendivx/decore50/Makefile \
plugins/opendivx/encore50/Makefile \
utils/Makefile \
utils/gtk/Makefile])

dnl
dnl GMERLIN
dnl

dnl AC_OUTPUT([Makefile libdv/Makefile decore50/Makefile encore50/Makefile])

dnl
dnl ENDIF
dnl
