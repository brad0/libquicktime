/*******************************************************************************
 test_codec.c

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

/***************************************************
 * This program prints informations about all
 * installed libquicktime codecs
 ***************************************************/

#include "lqt_private.h"
#include "lqt_codecinfo_private.h"
#include <stdio.h>

int main()
  {
  int num;
  int i;

  /* Initialize codecs */

  fprintf(stderr, "Libquicktime codec test, Codec API Version %d\n",
          lqt_get_codec_api_version());
  
  lqt_registry_init();
  
#if 1
  
  num = lqt_get_num_audio_codecs();

  for(i = 0; i < num; i++)
    {
    lqt_dump_codec_info(lqt_get_audio_codec_info(i));
    }

  num = lqt_get_num_video_codecs();

  for(i = 0; i < num; i++)
    {
    lqt_dump_codec_info(lqt_get_video_codec_info(i));
    }

#endif
  return 0;
  }
