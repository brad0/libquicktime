dnl
dnl AC_LIB_RAW1394_FLAGS
dnl This just unconditionally sets the options.  It should offer an option for
dnl explicitly giving the path to libraw1394 on the configure command line.
dnl
AC_DEFUN([AC_LIB_RAW1394_FLAGS], [
LIBRAW1394_CPPFLAGS=""
LIBRAW1394_CFLAGS=""
LIBRAW1394_LIBS="-lraw1394"

AC_SUBST(LIBRAW1394_CPPFLAGS)
AC_SUBST(LIBRAW1394_CFLAGS)
AC_SUBST(LIBRAW1394_LIBS)
])

dnl
dnl AC_LIB_RAW1394_HEADERS([ACTION_IF_FOUND[,ACTION_IF_NOT_FOUND]])
dnl
AC_DEFUN([AC_LIB_RAW1394_HEADERS], [
AC_REQUIRE([AC_LIB_RAW1394_FLAGS])

ac_libraw1394_save_cppflags=$CPPFLAGS
CPPFLAGS="$LIBRAW1394_CPPFLAGS $CPPFLAGS"

ac_libraw1394_headers=no
AC_CHECK_HEADER(libraw1394/raw1394.h, ac_libraw1394_headers=yes)

CPPFLAGS=$ac_libraw1394_save_cppflags

if test $ac_libraw1394_headers = yes ; then
	ifelse([$1], , :, $1)
else
	ifelse([$2], , :, $2)
fi
])


dnl
dnl AC_LIB_RAW1394_LIBVERSION(MINIMUMVERSION[,ACTION_IF_FOUND[,ACTION_IF_NOT_FOUND]])
dnl
AC_DEFUN([AC_LIB_RAW1394_LIBVERSION], [
AC_REQUIRE([AC_PROG_CC])
AC_REQUIRE([AC_LIB_RAW1394_FLAGS])

ac_libraw1394_save_cppflags=$CPPFLAGS
ac_libraw1394_save_cflags=$CFLAGS
ac_libraw1394_save_libs=$LIBS
CPPFLAGS="$LIBRAW1394_CPPFLAGS $CPPFLAGS"
CFLAGS="$LIBRAW1394_CFLAGS $CFLAGS"
LIBS="$LIBRAW1394_LIBS $LIBS"

ac_libraw1394_versiontest_success=no
ac_libraw1394_ver_symbol=`echo __libraw1394_version_$1 | sed 's/\./_/g'`

AC_TRY_LINK([], [{
	extern char $ac_libraw1394_ver_symbol;
	$ac_libraw1394_ver_symbol++;
}], ac_libraw1394_versiontest_success=yes)

CPPFLAGS=$ac_libraw1394_save_cppflags
CFLAGS=$ac_libraw1394_save_cflags
LIBS=$ac_libraw1394_save_libs

if test $ac_libraw1394_versiontest_success = yes; then
	ifelse([$2], , :, $2)
else
	ifelse([$3], , :, $3)
fi
])


dnl
dnl AC_LIB_RAW1394_RUNTEST(MINIMUMVERSION[,ACTION_IF_FOUND
dnl                        [,ACTION_IF_NOT_FOUND[,ACTION_IF_CROSS_COMPILING]]])
AC_DEFUN([AC_LIB_RAW1394_RUNTEST], [
ac_libraw1394_save_cppflags=$CPPFLAGS
ac_libraw1394_save_cflags=$CFLAGS
ac_libraw1394_save_libs=$LIBS
CPPFLAGS="$LIBRAW1394_CPPFLAGS $CPPFLAGS"
CFLAGS="$LIBRAW1394_CFLAGS $CFLAGS"
LIBS="$LIBRAW1394_LIBS $LIBS"

dnl This program compares two version strings and returns with code 0 if
dnl req_ver <= lib_ver, returns 1 otherwise.
dnl 
dnl "1.23" < "1.23.1"   (missing fields assumed zero)
dnl "1.23pre" <> "1.23" (undefined, do not use text as version)
dnl "1.21" > "1.3"      (no implicit delimiters)
AC_TRY_RUN([
#include <stdlib.h>
#include <libraw1394/raw1394.h>

int main()
{
        char *req_ver, *lib_ver;
        unsigned int req_i, lib_i;

        req_ver = "$1";
        lib_ver = raw1394_get_libversion();

        while (1) {
                req_i = strtoul(req_ver, &req_ver, 10);
                lib_i = strtoul(lib_ver, &lib_ver, 10);

                if (req_i > lib_i) exit(1);
                if (req_i < lib_i) exit(0);

                if (*req_ver != '.' || *lib_ver != '.') exit(0);

                req_ver++;
                lib_ver++;
        }
}
], ac_libraw1394_run=yes, ac_libraw1394_run=no, ac_libraw1394_run=cross)


CPPFLAGS=$ac_libraw1394_save_cppflags
CFLAGS=$ac_libraw1394_save_cflags
LIBS=$ac_libraw1394_save_libs

if test $ac_libraw1394_run = yes; then
	ifelse([$2], , :, $2)
elif test $ac_libraw1394_run = no; then
	ifelse([$3], , :, $3)
else
	ifelse([$4], ,
               AC_MSG_ERROR([no default for cross compiling in libraw1394 runtest macro]),
               [$4])
fi
])

dnl
dnl AC_LIB_RAW1394(MINIMUMVERSION[,ACTION_IF_FOUND[,ACTION_IF_NOT_FOUND]])
dnl
dnl Versions before 0.9 can't be checked, so this will always fail if the
dnl installed libraw1394 is older than 0.9 as if the library weren't found.
dnl
AC_DEFUN([AC_LIB_RAW1394], [

AC_LIB_RAW1394_FLAGS
AC_LIB_RAW1394_HEADERS(ac_libraw1394_found=yes, ac_libraw1394_found=no)

if test $ac_libraw1394_found = yes ; then

AC_MSG_CHECKING(for libraw1394 version >= [$1])
AC_LIB_RAW1394_RUNTEST([$1], , ac_libraw1394_found=no,
                       AC_LIB_RAW1394_LIBVERSION([$1], , ac_libraw1394_found=no))

if test $ac_libraw1394_found = yes ; then
	AC_MSG_RESULT(yes)
	$2
else
	AC_MSG_RESULT(no)
	$3
fi

fi

])
# Configure paths for GTK+
# Owen Taylor     97-11-3

dnl AM_PATH_GTK([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND [, MODULES]]]])
dnl Test for GTK, and define GTK_CFLAGS and GTK_LIBS
dnl
AC_DEFUN([AM_PATH_GTK],
[dnl 
dnl Get the cflags and libraries from the gtk-config script
dnl
AC_ARG_WITH(gtk-prefix,[  --with-gtk-prefix=PFX   Prefix where GTK is installed (optional)],
            gtk_config_prefix="$withval", gtk_config_prefix="")
AC_ARG_WITH(gtk-exec-prefix,[  --with-gtk-exec-prefix=PFX Exec prefix where GTK is installed (optional)],
            gtk_config_exec_prefix="$withval", gtk_config_exec_prefix="")
AC_ARG_ENABLE(gtktest, [  --disable-gtktest       Do not try to compile and run a test GTK program],
		    , enable_gtktest=yes)

  for module in . $4
  do
      case "$module" in
         gthread) 
             gtk_config_args="$gtk_config_args gthread"
         ;;
      esac
  done

  if test x$gtk_config_exec_prefix != x ; then
     gtk_config_args="$gtk_config_args --exec-prefix=$gtk_config_exec_prefix"
     if test x${GTK_CONFIG+set} != xset ; then
        GTK_CONFIG=$gtk_config_exec_prefix/bin/gtk-config
     fi
  fi
  if test x$gtk_config_prefix != x ; then
     gtk_config_args="$gtk_config_args --prefix=$gtk_config_prefix"
     if test x${GTK_CONFIG+set} != xset ; then
        GTK_CONFIG=$gtk_config_prefix/bin/gtk-config
     fi
  fi

  AC_PATH_PROG(GTK_CONFIG, gtk-config, no)
  min_gtk_version=ifelse([$1], ,0.99.7,$1)
  AC_MSG_CHECKING(for GTK - version >= $min_gtk_version)
  no_gtk=""
  if test "$GTK_CONFIG" = "no" ; then
    no_gtk=yes
  else
    GTK_CFLAGS=`$GTK_CONFIG $gtk_config_args --cflags`
    GTK_LIBS=`$GTK_CONFIG $gtk_config_args --libs`
    gtk_config_major_version=`$GTK_CONFIG $gtk_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    gtk_config_minor_version=`$GTK_CONFIG $gtk_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    gtk_config_micro_version=`$GTK_CONFIG $gtk_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_gtktest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $GTK_CFLAGS"
      LIBS="$GTK_LIBS $LIBS"
dnl
dnl Now check if the installed GTK is sufficiently new. (Also sanity
dnl checks the results of gtk-config to some extent
dnl
      rm -f conf.gtktest
      AC_TRY_RUN([
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

int 
main ()
{
  int major, minor, micro;
  char *tmp_version;

  system ("touch conf.gtktest");

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = g_strdup("$min_gtk_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_gtk_version");
     exit(1);
   }

  if ((gtk_major_version != $gtk_config_major_version) ||
      (gtk_minor_version != $gtk_config_minor_version) ||
      (gtk_micro_version != $gtk_config_micro_version))
    {
      printf("\n*** 'gtk-config --version' returned %d.%d.%d, but GTK+ (%d.%d.%d)\n", 
             $gtk_config_major_version, $gtk_config_minor_version, $gtk_config_micro_version,
             gtk_major_version, gtk_minor_version, gtk_micro_version);
      printf ("*** was found! If gtk-config was correct, then it is best\n");
      printf ("*** to remove the old version of GTK+. You may also be able to fix the error\n");
      printf("*** by modifying your LD_LIBRARY_PATH enviroment variable, or by editing\n");
      printf("*** /etc/ld.so.conf. Make sure you have run ldconfig if that is\n");
      printf("*** required on your system.\n");
      printf("*** If gtk-config was wrong, set the environment variable GTK_CONFIG\n");
      printf("*** to point to the correct copy of gtk-config, and remove the file config.cache\n");
      printf("*** before re-running configure\n");
    } 
#if defined (GTK_MAJOR_VERSION) && defined (GTK_MINOR_VERSION) && defined (GTK_MICRO_VERSION)
  else if ((gtk_major_version != GTK_MAJOR_VERSION) ||
	   (gtk_minor_version != GTK_MINOR_VERSION) ||
           (gtk_micro_version != GTK_MICRO_VERSION))
    {
      printf("*** GTK+ header files (version %d.%d.%d) do not match\n",
	     GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION);
      printf("*** library (version %d.%d.%d)\n",
	     gtk_major_version, gtk_minor_version, gtk_micro_version);
    }
#endif /* defined (GTK_MAJOR_VERSION) ... */
  else
    {
      if ((gtk_major_version > major) ||
        ((gtk_major_version == major) && (gtk_minor_version > minor)) ||
        ((gtk_major_version == major) && (gtk_minor_version == minor) && (gtk_micro_version >= micro)))
      {
        return 0;
       }
     else
      {
        printf("\n*** An old version of GTK+ (%d.%d.%d) was found.\n",
               gtk_major_version, gtk_minor_version, gtk_micro_version);
        printf("*** You need a version of GTK+ newer than %d.%d.%d. The latest version of\n",
	       major, minor, micro);
        printf("*** GTK+ is always available from ftp://ftp.gtk.org.\n");
        printf("***\n");
        printf("*** If you have already installed a sufficiently new version, this error\n");
        printf("*** probably means that the wrong copy of the gtk-config shell script is\n");
        printf("*** being found. The easiest way to fix this is to remove the old version\n");
        printf("*** of GTK+, but you can also set the GTK_CONFIG environment to point to the\n");
        printf("*** correct copy of gtk-config. (In this case, you will have to\n");
        printf("*** modify your LD_LIBRARY_PATH enviroment variable, or edit /etc/ld.so.conf\n");
        printf("*** so that the correct libraries are found at run-time))\n");
      }
    }
  return 1;
}
],, no_gtk=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_gtk" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$GTK_CONFIG" = "no" ; then
       echo "*** The gtk-config script installed by GTK could not be found"
       echo "*** If GTK was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the GTK_CONFIG environment variable to the"
       echo "*** full path to gtk-config."
     else
       if test -f conf.gtktest ; then
        :
       else
          echo "*** Could not run GTK test program, checking why..."
          CFLAGS="$CFLAGS $GTK_CFLAGS"
          LIBS="$LIBS $GTK_LIBS"
          AC_TRY_LINK([
#include <gtk/gtk.h>
#include <stdio.h>
],      [ return ((gtk_major_version) || (gtk_minor_version) || (gtk_micro_version)); ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding GTK or finding the wrong"
          echo "*** version of GTK. If it is not finding GTK, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"
          echo "***"
          echo "*** If you have a RedHat 5.0 system, you should remove the GTK package that"
          echo "*** came with the system with the command"
          echo "***"
          echo "***    rpm --erase --nodeps gtk gtk-devel" ],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means GTK was incorrectly installed"
          echo "*** or that you have moved GTK since it was installed. In the latter case, you"
          echo "*** may want to edit the gtk-config script: $GTK_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     GTK_CFLAGS=""
     GTK_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(GTK_CFLAGS)
  AC_SUBST(GTK_LIBS)
  rm -f conf.gtktest
])
dnl AC_TRY_CFLAGS (CFLAGS, [ACTION-IF-WORKS], [ACTION-IF-FAILS])
dnl check if $CC supports a given set of cflags
AC_DEFUN([AC_TRY_CFLAGS],
    [AC_MSG_CHECKING([if $CC supports $1 flags])
    SAVE_CFLAGS="$CFLAGS"
    CFLAGS="$1"
    AC_TRY_COMPILE([],[],[ac_cv_try_cflags_ok=yes],[ac_cv_try_cflags_ok=no])
    CFLAGS="$SAVE_CFLAGS"
    AC_MSG_RESULT([$ac_cv_try_cflags_ok])
    if test x"$ac_cv_try_cflags_ok" = x"yes"; then
        ifelse([$2],[],[:],[$2])
    else
        ifelse([$3],[],[:],[$3])
    fi])

# Configure paths for libavcodec
# Burkhard Plaum, 2004-08-12

dnl Compile an avcodec test program and figure out the version

AC_DEFUN([ACL_CHECK_AVCODEC],[
AC_MSG_CHECKING([for build ID in libavcodec, libs: $AVCODEC_LIBS])
CFLAGS_save=$CFLAGS
LIBS_save=$LIBS
CFLAGS="$CFLAGS $AVCODEC_CFLAGS"
LIBS="$LIBS $AVCODEC_LIBS"
avcodec_ok="false"
AC_TRY_RUN([
    #include <ffmpeg/avcodec.h>
    #include <stdio.h>
    int main()
    {
    FILE * output;
    if(LIBAVCODEC_BUILD < $1)
      return -1;
    output=fopen("avcodec_version", "w");
    fprintf(output, LIBAVCODEC_VERSION);
    fclose(output);
    return 0;
    }
  ],
  [
    # program could be run
    if test "x$AVCODEC_VERSION" = "x"; then 
      AVCODEC_VERSION=`cat avcodec_version`
    fi
    rm -f avcodec_version
    avcodec_ok="true"
    AC_MSG_RESULT(ok)
  ],
  [
    # program could not be run
    AC_MSG_RESULT(failed)
  ])
CFLAGS="$CFLAGS_save"
LIBS="$LIBS_save"
])

dnl ACL_PATH_AVCODEC(BUILD_ID [, ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]])
dnl Test for libavcodec, and define AVCODEC_CFLAGS, AVCODEC_LIBS and
dnl AVCODEC_VERSION

AC_DEFUN([ACL_PATH_AVCODEC],[
AC_ARG_WITH(avcodec,[  --with-avcodec=PFX   Prefix where libavcodec is installed (optional)], avcodec_prefix="$withval", avcodec_prefix="")
dnl We need the _save variables because PKG_CHECK_MODULES will change
dnl the other variables
AVCODEC_CFLAGS_save=""
AVCODEC_LIBS_save=""
avcodec_done="false"

dnl
dnl First preference: configure options
dnl

if test "x$avcodec_prefix" != x; then
AVCODEC_CFLAGS="-I$avcodec_prefix/include"
AVCODEC_LIBS="-L$avcodec_prefix/lib -lavcodec"
ACL_CHECK_AVCODEC([$1])
  if test "x$avcodec_ok" = "xtrue"; then
    avcodec_done="true"
  fi
fi

dnl
dnl Second Peference: ffmpeg_acl
dnl

if test "x$avcodec_done" = "xfalse"; then
  PKG_CHECK_MODULES(AVCODEC_ACL, avcodec_acl >= "0.4.8acl", have_avcodec_acl="true", have_avcodec_acl="false")
  if test x"$have_avcodec_acl" = "xtrue"; then
        AVCODEC_CFLAGS=$AVCODEC_ACL_CFLAGS
        AVCODEC_LIBS=$AVCODEC_ACL_LIBS
        ACL_CHECK_AVCODEC([$1])
    if test "x$avcodec_ok" = "xtrue"; then
      avcodec_done="true"
      AVCODEC_VERSION=`pkg-config --modversion avcodec_acl`
      fi
  fi
fi

dnl
dnl Third Perference: Autodetect
dnl

if test "x$avcodec_done" = "xfalse"; then
  AVCODEC_CFLAGS=""
  AVCODEC_LIBS="-lavcodec"
  ACL_CHECK_AVCODEC([$1])
  if test "x$avcodec_ok" = "xtrue"; then
    avcodec_done="true"
  fi
fi

if test "x$avcodec_done" = "xtrue"; then
  ifelse([$2], , :, [$2])
else
  ifelse([$3], , :, [$3])
fi
])
