/*******************************************************************************
 lqt_schroedinger.c

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
#include "schroedinger.h"


static char * fourccs_dirac[]  = { "drac", (char*)0 };

static lqt_parameter_info_static_t encode_parameters_schroedinger[] =
  {
    { 
      .name =        "dummy",
      .real_name =   TRS("Dummy"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 95 },
      .val_min =     { .val_int = 1 },
      .val_max =     { .val_int = 100 },
     },
     { /* End of parameters */ }
  };


static lqt_codec_info_static_t codec_info_schroedinger =
  {
    .name =                "schroedinger",
    .long_name =           TRS("Dirac video"),
    .description =         TRS("Dirac codec based on libschroedinger"),
    .fourccs =             fourccs_dirac,
    .type =                LQT_CODEC_VIDEO,
    .direction =           LQT_DIRECTION_BOTH,
    .encoding_parameters = encode_parameters_schroedinger,
    .decoding_parameters = (lqt_parameter_info_static_t*)0,
    .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT,
    
  };

/* These are called from the plugin loader */

LQT_EXTERN int get_num_codecs() { return 1; }

LQT_EXTERN lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return &codec_info_schroedinger;
    }
  return (lqt_codec_info_static_t*)0;
  }
     

LQT_EXTERN lqt_init_video_codec_func_t get_video_codec(int index)
  {
  if((index == 0) || (index == 1))
    return quicktime_init_codec_schroedinger;
  return (lqt_init_video_codec_func_t)0;
  }
