/*******************************************************************************
 lqt_faac.c

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

#include "qtfaac.h"

static char * fourccs_faac[]     = { "mp4a", (char*)0 };


static lqt_parameter_info_static_t encode_parameters_faac[] =
  {
    {
      .name =        "faac_bitrate",
      .real_name =   TRS("Bitrate (kbps, 0 = VBR)"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int = 0 }
    },
    {
      .name =        "quality",
      .real_name =   TRS("VBR Quality"),
      .type =        LQT_PARAMETER_INT,
      .val_min =     { .val_int = 10 },
      .val_max =     { .val_int = 500 },
      .val_default = { .val_int = 100 },
    },
    {
      .name        = "object_type",
      .real_name   = TRS("Object type"),
      .type        = LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Low" },
      .stringlist_options = (char*[]){ TRS("Low"),
                                       TRS("Main"),
                                       TRS("SSR"),
                                       TRS("LTP"),
                                       (char*)0 },
    },
    { /* End of parameters */ }
  };

static lqt_codec_info_static_t codec_info_faac =
  {
    .name =                "faac",
    .long_name =           TRS("MPEG-2/4 AAC encoder"),
    .description =         TRS("MPEG-2/4 AAC encoder (faac based)"),
    .fourccs =             fourccs_faac,
    .type =                LQT_CODEC_AUDIO,
    .direction =           LQT_DIRECTION_ENCODE,
    .encoding_parameters = encode_parameters_faac,
    .decoding_parameters = (lqt_parameter_info_static_t*)0,
    .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT | LQT_FILE_MP4 |
    LQT_FILE_M4A | LQT_FILE_3GP,
    .compression_id      = LQT_COMPRESSION_AAC,
  };

/* These are called from the plugin loader */

LQT_EXTERN int get_num_codecs() { return 1; }

LQT_EXTERN lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return &codec_info_faac;
      break;
    }  
  return (lqt_codec_info_static_t*)0;
  }
     
/*
 *   Return the actual codec constructor
 */

LQT_EXTERN lqt_init_codec_func_t get_codec(int index)
  {
  return quicktime_init_codec_faac;
  }

