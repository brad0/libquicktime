AC_INIT(src/lqt_quicktime.c)
AM_INIT_AUTOMAKE(libquicktime, 0.9.4pre2)
AM_CONFIG_HEADER(config.h)
AM_CONFIG_HEADER(include/quicktime/lqt1394_config.h)

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

AC_DISABLE_STATIC

AC_LIBTOOL_DLOPEN
AM_PROG_LIBTOOL

AC_PROG_CC
AC_LANG_C

AC_C_BIGENDIAN

# large file support
AC_SYS_LARGEFILE

AC_CACHE_SAVE

dnl This is necessary for libtool assembler support

AS=gcc
ASFLAGS=

AC_SUBST(AS)
AC_SUBST(ASFLAGS)

dnl Build CFLAGS

COMMON_CFLAGS=" -finline-functions -Wall -Wno-unused -Winline"
PROFILE_CFLAGS=" -pg -fprofile-arcs -ftest-coverage"
DEBUG_CFLAGS=" -g3"

dnl Optimizing flags

OPTIMIZE_CFLAGS="-O3"

AC_TRY_CFLAGS([-funroll-all-loops],   [OPTIMIZE_CFLAGS="$OPTIMIZE_CFLAGS -funroll-all-loops"])
AC_TRY_CFLAGS([-fomit-frame-pointer], [OPTIMIZE_CFLAGS="$OPTIMIZE_CFLAGS -fomit-frame-pointer"])
AC_TRY_CFLAGS([-falign-loops=2],      [OPTIMIZE_CFLAGS="$OPTIMIZE_CFLAGS -falign-loops=2"])
AC_TRY_CFLAGS([-falign-jumps=2],      [OPTIMIZE_CFLAGS="$OPTIMIZE_CFLAGS -falign-jumps=2"])
AC_TRY_CFLAGS([-falign-functions=2],  [OPTIMIZE_CFLAGS="$OPTIMIZE_CFLAGS -falign-functions=2"])

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
AC_CHECK_LIB(z, inflateEnd)

AC_SEARCH_LIBS(pthread_create, [gthread pthread c_r])

dnl Check for X11
AC_PATH_XTRA
AC_SUBST(X_CFLAGS)
AC_SUBST(X_PRE_LIBS)
AC_SUBST(X_EXTRA_LIBS)
AC_SUBST(X_LIBS)
AM_CONDITIONAL(HAVE_X11, test "$no_x" != "yes")

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
AC_CHECK_FUNCS(fseeko, [have_fseeko="true"])
AM_CONDITIONAL(HAVE_FSEEKO, test x"$have_fseeko" = "xtrue")

AC_HEADER_STDC

dnl
dnl Check for optional libraries
dnl 



dnl
dnl Check for gtk
dnl This is neccesary for the utils/gtk subdirectory
dnl

GTK_REQUIRED="1.2.8"

have_gtk=false

AM_PATH_GTK($GTK_REQUIRED, have_gtk=true)

AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(GTK_REQUIRED)

AM_CONDITIONAL(HAVE_GTK, test x$have_gtk = xtrue)

dnl
dnl Check for libdv
dnl

have_libdv="false"

LIBDV_REQUIRED="0.102"

PKG_CHECK_MODULES(LIBDV, libdv >= $LIBDV_REQUIRED, have_libdv="true", have_libdv="false")
AC_SUBST(LIBDV_REQUIRED)
AC_SUBST(LIBDV_LIBS)
AC_SUBST(LIBDV_CFLAGS)

AM_CONDITIONAL(HAVE_LIBDV, test x$have_libdv = xtrue)


dnl
dnl Check for Vorbis
dnl 

VORBIS_REQUIRED="1.0"
 
have_vorbis=false
AH_TEMPLATE([HAVE_VORBIS], [Vorbis libraries are there])
  
XIPH_PATH_VORBIS(have_vorbis=true)
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

if test x$have_vorbis = xtrue; then
    AC_CHECK_LIB(mp3lame, lame_decode_init,
             [ have_lame="true" ], [], [ -lm -lvorbis])
else
    AC_CHECK_LIB(mp3lame, lame_decode_init,
             [ have_lame="true" ], [], [ -lm ])
fi

AM_CONDITIONAL(HAVE_LAME, test x$have_lame = x"true")
AC_SUBST(LAME_REQUIRED)

dnl
dnl Check for libjpeg
dnl

AH_TEMPLATE([HAVE_LIBJPEG], [Do we have libjpeg installed?])
JPEG_REQUIRED="6b"
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
AC_SUBST(JPEG_REQUIRED)

AM_CONDITIONAL(HAVE_LIBJPEG, test x$have_libjpeg = xtrue)

dnl
dnl Check for quicktime1394 junk
dnl

CINELERRA_FIREWIRE_LIBS=""

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

LIBAVC1394_REQUIRED="0.3.1"
LIBRAW1394_REQUIRED="0.9.0"

AC_SUBST(LIBAVC1394_REQUIRED)
AC_SUBST(LIBRAW1394_REQUIRED)

have_libavc1394=false
have_librom1394=false

if test "x$enable_firewire" = xyes; then

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
        else
	  CINELERRA_FIREWIRE_LIBS="$FIREWIRE_LIBS"
	fi

	OLD_LIBS=$LIBS
	OLD_CFLAGS=$CFLAGS
	LIBS=$FIREWIRE_LIBS
	CFLAGS=$FIREWIRE_CFLAGS

	if test "x$enable_firewire" = xyes; then
	  AC_CHECK_LIB(avc1394, avc1394_send_command, 
			[LIBS="-lavc1394 $LIBS"; have_libavc1394=true; AC_DEFINE(HAVE_LIBAVC1394, 1) ], 
			[AC_MSG_WARN([libavc1394 is not available, so firewire support will be disabled.])
			enable_firewire=no])
	fi

	if test "x$enable_firewire" = xyes; then
	  AC_CHECK_LIB(rom1394, main, 
			[LIBS="-lrom1394 $LIBS"; have_librom1394=true; AC_DEFINE(HAVE_LIBROM1394, 1)], 
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
dnl Multithread support is now optional and
dnl disabled by default
dnl

have_mt_jpeg=false

AH_TEMPLATE([HAVE_MT_JPEG], [Multithreaded jpeg supported?])

AC_ARG_ENABLE(mt-jpeg,
    [  --enable-mt-jpeg        Enable multithreaded jpeg codec ],
     [case "${enableval}" in
       yes) have_mt_jpeg=true ;;
       no) ;;
       *) AC_MSG_ERROR(bad value ${enableval} for --enable-mt-jpeg) ;;
     esac],[])

if test x$have_mt_jpeg = xtrue; then
AC_DEFINE(HAVE_MT_JPEG)
fi

dnl
dnl Check for libavcodec (ffmpeg)
dnl

AH_TEMPLATE([HAVE_LIBAVCODEC],
            [Do we have libavcodec installed?])

have_libavcodec=false

AVCODEC_BUILD="4712"

dnl PKG_CHECK_MODULES(AVCODEC, avcodec_acl = $AVCODEC_REQUIRED, have_libavcodec="true", have_libavcodec="false")
LQT_PATH_AVCODEC($AVCODEC_BUILD , have_libavcodec="true", have_libavcodec="false")

AVCODEC_REQUIRED=$AVCODEC_VERSION

AM_CONDITIONAL(HAVE_LIBAVCODEC, test x$have_libavcodec = xtrue)

AC_SUBST(AVCODEC_REQUIRED)
AC_SUBST(AVCODEC_LIBS)
AC_SUBST(AVCODEC_CFLAGS)

dnl
dnl Check for libpng
dnl 

AH_TEMPLATE([HAVE_LIBPNG], [Enable png codec])

have_libpng=false
PNG_REQUIRED="1.0.8"

OLD_CFLAGS=$CFLAGS
OLD_LIBS=$LIBS

LIBS="-lpng -lm -lz"
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
AC_SUBST(PNG_REQUIRED)

AM_CONDITIONAL(HAVE_LIBPNG, test x$have_libpng = xtrue)

#CPU Optimisation added by Justin Schoeman (justin@suntiger.ee.up.ac.za)

AC_MSG_WARN(Doing a few not well tested checks for CPU optimisations.)
AC_MSG_WARN(Please report any problems to justin@suntiger.ee.up.ac.za)

RTJPEG_CFLAGS=$CFLAGS

#Check for MMX-capable compiler
AC_ARG_ENABLE(mmx, [  --enable-mmx            compile for MMX CPU [default=yes]])
if test x${enable_mmx-yes} = xyes ; then
  AC_MSG_CHECKING(for MMX-capable compiler)
  AC_TRY_RUN([
    #include "plugins/rtjpeg/mmx.h"
    main()
    { movq_r2r(mm0, mm1); return 0; }
  ],
  [
    # program could be run
    HAVE_MMX="yes"
    AC_MSG_RESULT(yes)
  ],
    # program could not be run
    HAVE_MMX="no"
    AC_MSG_RESULT(no)
  ,
    # cross compiling (untested)
  [
    case "x${host_cpu}" in 
         xi586 | xi686 | xk6 | xk7 | xathlon)
            HAVE_MMX="yes"
            AC_MSG_RESULT(yes) ;;
        *)
            HAVE_MMX="no"
            AC_MSG_RESULT(not supported on ${host_cpu}) ;;
    esac
  ])
fi

# Set X86_MMX compile flag

if test "x$HAVE_MMX" = "xyes" ; then
  AC_MSG_WARN(Building with MMX optimisations - do not)
  AC_MSG_WARN(execute the binary on a non-MMX machine.)
  RTJPEG_CFLAGS="-DMMX $RTJPEG_CFLAGS"
fi

# Set CPU specific compiler options

if test "x$RPM_OPT_FLAGS" = "x" ; then
    # detect Athlon CPU
    # the idea is to use -mcpu=athlon to get the important optimization
    # but keep -march=${host_cpu} from the command line. -march=i686
    # should not have a negative impact on athlons, but -march=i586 or
    # worse has.
    AC_MSG_CHECKING(for Athlon CPU)
    if grep Athlon /proc/cpuinfo > /dev/null ; then
         AC_MSG_RESULT(yes)
         AC_TRY_CFLAGS([-march=athlon], [RTJPEG_CFLAGS="$RTJPEG_CFLAGS -march=athlon"],
           AC_TRY_CFLAGS([-march=i686], [RTJPEG_CFLAGS="$RTJPEG_CFLAGS -march=i686"]))
    else
         AC_MSG_RESULT(no)
         AC_MSG_CHECKING(for K6 CPU)
         if grep -i k6 /proc/cpuinfo > /dev/null ; then
              AC_MSG_RESULT(yes)
              AC_TRY_CFLAGS([-march=k6], [RTJPEG_CFLAGS="$RTJPEG_CFLAGS -march=k6"],
                AC_TRY_CFLAGS([-march=i686], [RTJPEG_CFLAGS="$RTJPEG_CFLAGS -march=i686"]))
         else
              AC_MSG_RESULT(no)
              AC_TRY_CFLAGS([-march=pentium], 
                            [RTJPEG_CFLAGS="$RTJPEG_CFLAGS -march=pentium"])
         fi
    fi
    
    # basic optimization
    AC_TRY_CFLAGS([-malign-double -fschedule-insns2],
                  [RTJPEG_CFLAGS="$RTJPEG_CFLAGS -malign-double -fschedule-insns2"])
    AC_TRY_CFLAGS([-O6],
                  [RTJPEG_CFLAGS="$RTJPEG_CFLAGS -O6"],
		  AC_TRY_CFLAGS([-O3],[RTJPEG_CFLAGS="$RTJPEG_CFLAGS -O3"]))
else
    # use optimization as required for RPM host CPU
    RTJPEG_CFLAGS="$RTJPEG_CFLAGS $RPM_OPT_FLAGS"
    RTJPEG_CFLAGS="$RTJPEG_CFLAGS"
fi

AC_SUBST(RTJPEG_CFLAGS)

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
# MODULE_LIBADD="-L$prefix/lib ../../src/libquicktime.la"
# The above is OK iff the added library is already installed. The
# current
# libtool version does not support linking libraries with uninstalled
# libraries as occurs when Debian packages are created for the 1st time.
# Linking against old libraries may not be desirable!
MODULE_LIBADD= 
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
AC_SUBST(CINELERRA_FIREWIRE_LIBS)

dnl Write all the makefiles and other stuff

AC_OUTPUT([Makefile \
libquicktime.spec \
doc/Makefile \
debian/Makefile \
src/Makefile \
quicktime1394/Makefile \
include/Makefile \
include/quicktime/Makefile \
include/quicktime/lqt_version.h \
plugins/Makefile \
plugins/dv/Makefile \
plugins/lame/Makefile \
plugins/mjpeg/Makefile \
plugins/audiocodec/Makefile \
plugins/videocodec/Makefile \
plugins/vorbis/Makefile \
plugins/png/Makefile \
plugins/opendivx/Makefile \
plugins/opendivx/decore50/Makefile \
plugins/opendivx/encore50/Makefile \
plugins/rtjpeg/Makefile \
plugins/ffmpeg/Makefile \
utils/Makefile \
utils/gtk/Makefile \
lqt-config], [chmod a+x lqt-config])

echo "Configuration: "
$have_gtk

echo -n "libdv:      "
if test x$have_libdv = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "vorbis:     "
if test x$have_vorbis = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "lame:       "
if test x$have_lame = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi


echo -n "libjpeg:    "
if test x$have_libjpeg = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "libpng:     "
if test x$have_libpng = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "libavcodec: "
if test x$have_libavcodec = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "libraw1394: "
if test x$have_libraw1394 = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "libavc1394: "
if test x$have_libavc1394 = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "librom1394: "
if test x$have_librom1394 = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

echo -n "Firewire:   "
if test "x$enable_firewire" = xyes; then
echo "Enabled"
else
echo "Disabled"
fi


echo -n "gtk-1.x.y   "
if test x$have_gtk = "xtrue"; then
echo "Yes"
else
echo "Missing"
fi

