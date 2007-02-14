/*******************************************************************************
 lqt_lame.c

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

extern void quicktime_init_codec_lame(quicktime_audio_map_t *atrack);


static char * fourccs_mp3[]  = { QUICKTIME_MP3, (char*)0 };

static lqt_parameter_info_static_t encode_parameters_lame[] =
  {
    {
      .name =        "mp3_bitrate_mode",
      .real_name =   "Bitrate mode",
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "CBR" },
      .stringlist_options = (char*[]){ "CBR", "ABR", "VBR", (char*)0 },
      .help_string = "CBR: Constant bitrate\nVBR: Variable bitrate\n"
                     "ABR: Average bitrate"
    },
    { 
      .name =        "mp3_bitrate",
      .real_name =   "Nominal Bitrate (ABR/CBR)",
      .type =        LQT_PARAMETER_INT,
      .val_default = { 256000 },
      .help_string = "Bitrate in bits per second. For CBR, this must be a "
                     "valid MP3 bitrate"
    },
    { 
      .name =        "mp3_bitrate_min",
      .real_name =   "Minimum Bitrate (ABR)",
      .type =        LQT_PARAMETER_INT,
      .val_default = { 64000 },
      .help_string = "Minimum ABR bitrate in bits per second. This must be a "
                     "valid MP3 bitrate"
    },
    { 
      .name =        "mp3_bitrate_max",
      .real_name =   "Maximum Bitrate (ABR)",
      .type =        LQT_PARAMETER_INT,
      .val_default = { 320000 },
      .help_string = "Maximum ABR bitrate in bits per second. This must be a "
                     "valid MP3 bitrate"
    },
    {
      .name =        "mp3_quality",
      .real_name =   "Quality (0 = best)",
      .type =        LQT_PARAMETER_INT,
      .val_default = { 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 9 },
      .help_string = "0: Slowest encoding, best quality\n"
                     "9: Fastest encoding, worst quality"
    },
    {
      .name =        "mp3_quality_vbr",
      .real_name =   "VBR Quality (0 = best)",
      .type =        LQT_PARAMETER_INT,
      .val_default = { 0 },
      .val_min =     { .val_int = 0 },
      .val_max =     { .val_int = 9 },
      .help_string = "VBR Quality level. 0: best, 9: worst"
    },
    { /* End of paramaters */ }
  };

static lqt_codec_info_static_t codec_info_lame =
  {
    .name =                "lame",
    .long_name =           "Lame mp3 encoder",
    .description =         "Lame mp3 encoder (see http://www.mp3dev.org)",
    .fourccs =             fourccs_mp3,
    .wav_ids =             (int[]){ 0x55, LQT_WAV_ID_NONE },
    .type =                LQT_CODEC_AUDIO,
    .direction =           LQT_DIRECTION_ENCODE,
    .encoding_parameters = encode_parameters_lame,
    .decoding_parameters = (lqt_parameter_info_static_t*)0,
    .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT | LQT_FILE_AVI
  };

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  if(!index)
    return &codec_info_lame;
  
  return (lqt_codec_info_static_t*)0;
  }

/*
 *   Return the actual codec constructor
 */

extern lqt_init_audio_codec_func_t get_audio_codec(int index)
  {
  if(index == 0)
    return quicktime_init_codec_lame;
  return (lqt_init_audio_codec_func_t)0;
  }
