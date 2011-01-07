/*******************************************************************************
 dump_codecs.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2011 Members of the libquicktime project.

 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2.1 of the License, or (at your option)
 any later version.

 This library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this library; if not, write to the Free Software Foundation, Inc., 51
 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*******************************************************************************/ 

/*
 * This one is used for auto generating
 * the codecs for the webpage
 */

#include "lqt_private.h"
#include <stdio.h>

static char * header = "<h1>Codecs</h1>\n\
Note that this list might not reflect the last stable release.\n\
Instead, we try to keep this in sync with the CVS version.\n\
<h2>Audio codecs</h2>\n\
<ul>\n";

static char * middle = "</ul>\n\
<h2>Video codecs</h2>\n\
<ul>\n";

static char * footer =
"</ul>\n";

int main(int argc, char ** argv)
  {
  int i;
  lqt_codec_info_t ** codecs;
  lqt_registry_init();

  /* Print header */

  printf("%s", header);
  
  /* Print audio codecs */

  codecs = lqt_query_registry(1, 0, 1, 1);

  i = 0;
  while(codecs[i])
    {
    printf("<li><b>%s</b> ", codecs[i]->long_name);

    switch(codecs[i]->direction)
      {
      case LQT_DIRECTION_ENCODE:
        printf("Encode only\n");
        break;
      case LQT_DIRECTION_DECODE:
        printf("Decode only\n");
        break;
      case LQT_DIRECTION_BOTH:
        printf("Encode/Decode\n");
        break;
      }
    i++;
    }
  lqt_destroy_codec_info(codecs);
  printf("%s", middle);

  codecs = lqt_query_registry(0, 1, 1, 1);

  i = 0;
  while(codecs[i])
    {
    printf("<li><b>%s</b> ", codecs[i]->long_name);

    switch(codecs[i]->direction)
      {
      case LQT_DIRECTION_ENCODE:
        printf("Encode only\n");
        break;
      case LQT_DIRECTION_DECODE:
        printf("Decode only\n");
        break;
      case LQT_DIRECTION_BOTH:
        printf("Encode/Decode\n");
        break;
      }
    i++;
    }
  lqt_destroy_codec_info(codecs);
  printf("%s", footer);

  return 0;
  
  }
