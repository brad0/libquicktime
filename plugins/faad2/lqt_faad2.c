/*******************************************************************************
 lqt_faad2.c

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

#include "lqt_private.h"
#include <quicktime/lqt_codecapi.h>
#include "faad2.h"

static char * fourccs_faad2[]     = { "mp4a", (char*)0 };

static lqt_codec_info_static_t codec_info_faad2 =
  {
    .name =                "faad2",
    .long_name =           TRS("MPEG-2/4 AAC decoder"),
    .description =         TRS("MPEG-2/4 AAC decoder (faad2 based)"),
    .fourccs =             fourccs_faad2,
    .type =                LQT_CODEC_AUDIO,
    .direction =           LQT_DIRECTION_DECODE,
    .encoding_parameters = (lqt_parameter_info_static_t*)0,
    .decoding_parameters = (lqt_parameter_info_static_t*)0
  };


/* These are called from the plugin loader */

LQT_EXTERN int get_num_codecs() { return 1; }

LQT_EXTERN lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return &codec_info_faad2;
      break;
    }  
  return (lqt_codec_info_static_t*)0;
  }
     
/*
 *   Return the actual codec constructor
 */

LQT_EXTERN lqt_init_codec_func_t get_codec(int index)
  {
  return quicktime_init_codec_faad2;
  }

