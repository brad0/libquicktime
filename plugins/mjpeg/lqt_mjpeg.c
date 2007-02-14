/*******************************************************************************
 lqt_mjpeg.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2007 Members of the libquicktime project.

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

void quicktime_init_codec_jpeg(quicktime_video_map_t *vtrack);

/*
 *  We cheat here: Actually, this is one codec but we tell the
 *  outer world, that we are 2 codecs because mjpa and jpeg are
 *  to different to be one codec with 2 fourccs
 */

static char * fourccs_jpeg[]  = { QUICKTIME_JPEG, (char*)0 };
static char * fourccs_mjpa[]  = { QUICKTIME_MJPA, (char*)0 };

static lqt_parameter_info_static_t encode_parameters_jpeg[] =
  {
    { 
      .name =        "jpeg_quality",
      .real_name =   "Quality",
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 95 },
      .val_min =     { .val_int = 1 },
      .val_max =     { .val_int = 100 },
     },
     { 
       .name =        "jpeg_usefloat",
       .real_name =   "Use float",
       .type =        LQT_PARAMETER_INT,
       .val_default = { .val_int = 0 },
       .val_min =     { .val_int = 0 },
       .val_max =     { .val_int = 1 },
     },
     { /* End of parameters */ }
  };


static lqt_codec_info_static_t codec_info_jpeg =
  {
    .name =                "jpeg",
    .long_name =           "Jpeg photo",
    .description =         "This format writes a seperate JPEG photo for \
every frame in YUV 4:2:0",
    .fourccs =             fourccs_jpeg,
    .type =                LQT_CODEC_VIDEO,
    .direction =           LQT_DIRECTION_BOTH,
    .encoding_parameters = encode_parameters_jpeg,
    .decoding_parameters = (lqt_parameter_info_static_t*)0,
    .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT,
    
  };

static lqt_codec_info_static_t codec_info_mjpa =
  {
    .name =                "mjpa",
    .long_name =           "Motion Jpeg A",
    .description =         "MJPA stores each frame as two JPEGs interlaced \
and in YUV 4:2:2",
    .fourccs =             fourccs_mjpa,
    .type =                LQT_CODEC_VIDEO,
    .direction =           LQT_DIRECTION_BOTH,
    .encoding_parameters = encode_parameters_jpeg,
    .decoding_parameters = (lqt_parameter_info_static_t*)0,
    .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT,
  };


/* These are called from the plugin loader */

extern int get_num_codecs() { return 2; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return &codec_info_jpeg;
    case 1:
      return &codec_info_mjpa;
    }
  return (lqt_codec_info_static_t*)0;
  }
     

extern lqt_init_video_codec_func_t get_video_codec(int index)
  {
  if((index == 0) || (index == 1))
    return quicktime_init_codec_jpeg;
  return (lqt_init_video_codec_func_t)0;
  }
