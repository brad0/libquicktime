AC_INIT(src/lqt_quicktime.c)
dnl AC_INIT([Libquicktime], 0.9.0, [Libquicktime Devel List <libquicktime-devel@sourceforge.net>], [libquicktime])
dnl Every other copy of the package version number gets its value from here
AM_INIT_AUTOMAKE(libquicktime, 0.9.0)
AM_CONFIG_HEADER([config.h include/quicktime/lqt1394_config.h])

AC_CANONICAL_HOST
AM_MAINTAINER_MODE

AC_LIBTOOL_PICMODE

AH_TEMPLATE([LQT_VERSION],
            [Libquicktime version])

LQT_VERSION=$VERSION
AC_DEFINE_UNQUOTED(LQT_VERSION, "$LQT_VERSION")
AC_SUBST(LQT_VERSION)

dnl
dnl Libquicktime codec API version
dnl 

LQT_CODEC_API_VERSION="2"

AH_TEMPLATE([LQT_CODEC_API_VERSION],
            [Libquicktime codec API version])

AC_DEFINE_UNQUOTED(LQT_CODEC_API_VERSION, $LQT_CODEC_API_VERSION)
AC_SUBST(LQT_CODEC_API_VERSION)

AC_ENABLE_STATIC

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

# large file support
COMMON_FLAGS="$COMMON_FLAGS -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64"

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

AC_SEARCH_LIBS(pthread_create, [gthread pthread c_r])

dnl Check for X11
AC_PATH_XTRA
AC_SUBST(X_CFLAGS)
AC_SUBST(X_PRE_LIBS)
AC_SUBST(X_EXTRA_LIBS)
AC_SUBST(X_LIBS)
AM_CONDITIONAL(HAVE_X11, test "$no_x" != "yes")

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

dnl ENCORE_DIR=encore50
dnl DECORE_DIR=decore50

dnl AH_TEMPLATE([ENCORE_INCLUDE],
dnl            [Which encore library to use.])
dnl AH_TEMPLATE([DECORE_INCLUDE],
dnl            [Which decore library to use.])

dnl AC_DEFINE_UNQUOTED(ENCORE_INCLUDE, "$ENCORE_DIR/encore.h")
dnl AC_DEFINE_UNQUOTED(DECORE_INCLUDE, "$DECORE_DIR/decore.h")

AC_HEADER_STDC

dnl AC_ARG_ENABLE(complete-dist,
dnl [  --enable-complete-dist  Include the built copies of the library source if availiable. ])

dnl COMPLETE_DIST=""; 

dnl if test "x$enable_complete_dist" == "xyes"; then
dnl 	test -d libraw1394 && COMPLETE_DIST="$COMPLETE_DIST libraw1394";
dnl 	test -d libogg-1.0rc2 && COMPLETE_DIST="$COMPLETE_DIST libogg-1.0rc2";
dnl 	test -d libvorbis-1.0rc2 && COMPLETE_DIST="$COMPLETE_DIST libvorbis-1.0rc2";
dnl fi

dnl AC_SUBST(COMPLETE_DIST)

dnl
dnl Check for libdv
dnl

have_libdv=false

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

dnl
dnl Since libdv depends on glib but libquicktime does not,
dnl we check for it here (because we want to do AC_TRY_LINK).
dnl

have_glib=false
AM_PATH_GLIB([1.2.8], have_glib=true)

LIBS="$GLIB_LIBS -ldv"
CFLAGS=$GLIB_CFLAGS

if test x$have_glib = xtrue; then
AC_MSG_CHECKING(for libdv with encoder)
AC_TRY_LINK([#include <stdio.h>
             #include <libdv/dv.h>],
            [dv_decoder_t *decoder; dv_encoder_t *encoder;
	     unsigned char *data, **frame;
	     int *pitches;
             decoder = dv_decoder_new( FALSE, TRUE, TRUE );
             encoder = dv_encoder_new( FALSE, FALSE, FALSE );
	     dv_decode_full_frame( decoder, data,
				   e_dv_color_rgb, frame,
				   pitches );
	     dv_encode_full_frame( encoder, frame, e_dv_color_yuv, data );
	     dv_decoder_free( decoder ); dv_encoder_free( encoder );],
	    [have_libdv=true])


case $have_libdv in
  true) dnl AC_DEFINE(HAVE_LIBDV)
        AC_MSG_RESULT(yes)
	LIBDV_LIBS=-ldv;
	LIBDV_CFLAGS=$CFLAGS;;
  false) AC_MSG_RESULT(no); LIBDV_LIBS=""; LIBDV_CFLAGS="";;
  * ) AC_MSG_RESULT("Somethings wrong: $have_libdv") ;;
esac

fi

CFLAGS=$OLD_CFLAGS
LIBS=$OLD_LIBS

AC_SUBST(LIBDV_LIBS)
AC_SUBST(LIBDV_CFLAGS)

AM_CONDITIONAL(HAVE_LIBDV, test x$have_libdv = xtrue)

dnl
dnl Check for quicktime1394 junk
dnl

AH_TEMPLATE([HAVE_FIREWIRE],
            [tell libdv.c to support firewire])
AH_TEMPLATE([HAVE_LIBRAW1394],
            [Define if you have libraw1394 (-lraw1394).])
AH_TEMPLATE([HAVE_LIBROM1394],
            [Define if you have librom1394 (-lrom1394).])
AH_TEMPLATE([HAVE_LIBAVC1394],
            [Define if you have the `avc1394` library (-lavc1394).])

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

if test x$have_libdv != xtrue; then
  enable_firewire="no (I need libdv)"
fi

AC_MSG_RESULT($enable_firewire)

if test "x$enable_firewire" = xyes; then
dnl 	FIREWIRE_SUBDIRS=""
dnl 	FIREWIRE_LIBS="unknown"
 
dnl 	if test -d libraw1394; then
 
dnl 	AC_ARG_WITH(internal-raw1394,
dnl [  --with-internal-raw1394 Use libraw1394 from the distribution. The default is to use
dnl                           versions installed on the system if possible. ],
dnl 	     [case "${withval}" in
dnl 	       yes) FIREWIRE_SUBDIRS="libraw1394"
dnl 		    FIREWIRE_LIBS="libraw1394/src/libraw1394.la"
dnl 		    FIREWIRE_CFLAGS="" ;;
dnl 	       no) FIREWIRE_LIBS="unknown";;
dnl 	       *) AC_MSG_ERROR(bad value ${enableval} for --disable-final) ;;
dnl 	     esac],[ FIREWIRE_LIBS="unknown" ])

dnl 	fi

dnl 	if test "x$FIREWIRE_LIBS" = "xunknown"; then
dnl 	fi

	FIREWIRE_LIBS="$GLIB_LIBS $LIBDV_LIBS"
	FIREWIRE_CFLAGS="$GLIB_CFLAGS $LIBDV_CFLAGS"

	AC_LIB_RAW1394([0.9], 
			 [ FIREWIRE_CFLAGS="$FIREWIRE_CFLAGS $LIBRAW1394_CPPFLAGS $LIBRAW1394_CFLAGS"; 
			 FIREWIRE_LIBS="$FIREWIRE_LIBS $LIBRAW1394_LIBS"
			 AC_DEFINE(HAVE_LIBRAW1394, 1)
			 have_libraw1394=true ],
			[ have_libraw1394=false ])

	if test "x$have_libraw1394" != "xtrue"; then
	  enable_firewire=no
	  AC_MSG_WARN([libraw1394 is not available, so firewire support will be disabled.])
	fi

	OLD_LIBS=$LIBS
	OLD_CFLAGS=$CFLAGS
	LIBS=$FIREWIRE_LIBS
	CFLAGS=$FIREWIRE_CFLAGS

	if test "x$enable_firewire" = xyes; then
	  AC_CHECK_LIB(avc1394, avc1394_send_command, 
			[LIBS="-lavc1394 $LIBS"; AC_DEFINE(HAVE_LIBAVC1394, 1) ], 
			[AC_MSG_WARN([libavc1394 is not available, so firewire support will be disabled.])
			enable_firewire=no])
	fi

	if test "x$enable_firewire" = xyes; then
	  AC_CHECK_LIB(rom1394, main, 
			[LIBS="-lrom1394 $LIBS"; AC_DEFINE(HAVE_LIBROM1394, 1)], 
			[AC_MSG_WARN([librom1394 is not available, so firewire support will be disabled.])
			enable_firewire=no])
	fi

	FIREWIRE_LIBS=$LIBS
	FIREWIRE_CFLAGS=$CFLAGS

	LIBS=$OLD_LIBS
	CFLAGS=$OLD_CFLAGS

	if test "x$enable_firewire" = xyes; then
	  AC_DEFINE(HAVE_FIREWIRE, 1)
	fi
fi

AM_CONDITIONAL(HAVE_FIREWIRE, test "x$enable_firewire" = xyes)

HAVE_FIREWIRE=0
if test "x$enable_firewire" = xyes; then
	HAVE_FIREWIRE=1
fi
AC_SUBST(HAVE_FIREWIRE)

AC_SUBST(FIREWIRE_LIBS)
AC_SUBST(FIREWIRE_CFLAGS)
AC_SUBST(FIREWIRE_SUBDIRS)

dnl
dnl Check for Vorbis
dnl 

have_vorbis=false
AH_TEMPLATE([HAVE_VORBIS], [Vorbis libraries are there])

AM_PATH_VORBIS(have_vorbis=true)
AM_CONDITIONAL(HAVE_VORBIS, test x$have_vorbis = xtrue)

if test x$have_vorbis = xtrue; then
AC_DEFINE(HAVE_VORBIS)
fi

dnl
dnl Check for libjpeg
dnl

AH_TEMPLATE([HAVE_LIBJPEG],
            [Do we have libjpeg installed?])

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

AC_SUBST(JPEG_LIBS)
AC_SUBST(JPEG_CFLAGS)

AM_CONDITIONAL(HAVE_LIBJPEG, test x$have_libjpeg = xtrue)

dnl
dnl Check for libpng
dnl 

AH_TEMPLATE([HAVE_LIBPNG], [Enable png codec])

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

AC_SUBST(PNG_CFLAGS)
AC_SUBST(PNG_LIBS)

AM_CONDITIONAL(HAVE_LIBPNG, test x$have_libpng = xtrue)

dnl jpeg/Makefile

if test "x$prefix" = xNONE; then
   prefix="${ac_default_prefix}"
fi
if test "x$exec_prefix" = xNONE; then
   exec_prefix="${prefix}"
fi
plugin_dir="$exec_prefix/lib/$PACKAGE"


AH_TEMPLATE([PLUGIN_DIR], [Directory for plugins])

PLUGIN_DIR=$plugin_dir
AC_DEFINE_UNQUOTED(PLUGIN_DIR, ["$plugin_dir"])
AC_SUBST(PLUGIN_DIR)

# -L is required so that linker can find libquicktime when DESTDIR is used.
# libtool bug, documented at: http://www.geocrawler.com/mail/thread.php3?subject=install+phase+fails&list=404
MODULE_LIBADD="-L$prefix/lib ../../src/libquicktime.la"
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

LQT1394_CFLAGS="$FIREWIRE_CFLAGS"
LQT1394_LIBS="$FIREWIRE_LIBS -lquicktime1394"

AC_SUBST(LQT1394_CFLAGS)
AC_SUBST(LQT1394_LIBS)

dnl Write all the makefiles and other stuff

AC_OUTPUT([Makefile \
src/Makefile \
quicktime1394/Makefile \
include/Makefile \
include/quicktime/Makefile \
include/quicktime/lqt_version.h \
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
plugins/ferguson/Makefile \
plugins/xanim/Makefile \
plugins/xanim/libxanimcodecs/Makefile \
utils/Makefile \
utils/gtk/Makefile \
lqt-config], [chmod a+x lqt-config])
