/*******************************************************************************
 lqt_dv.c

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
#include "dv.h"
#include <quicktime/lqt_codecapi.h>
#include <quicktime/colormodels.h>
#include <libdv/dv.h>

// static char * fourccs_dv[]  = { QUICKTIME_DV, QUICKTIME_DV_AVID, QUICKTIME_DV_AVID_A, (char*)0 };

/* Now, we need the fourccs for encoding only */

static char * fourccs_dv_ntsc[] = { "dvc ", (char*)0 };
static char * fourccs_dv_pal[]  = { "dvcp", (char*)0 };


#if 0  
static lqt_parameter_info_static_t decode_parameters_dv[] =
  {
     { 
       .name =               "dv_decode_quality",
       .real_name =          TRS("Decoding Quality"),
       .type =               LQT_PARAMETER_INT,
       .val_default =        {DV_QUALITY_BEST},
       .val_min =            0,
       .val_max =            DV_QUALITY_BEST,
     },
	 { 
       .name =               "dv_clamp_luma",
       .real_name =          TRS("Clamp Luma Values"),
       .type =               LQT_PARAMETER_INT,
       .val_default =        { 0 },
       .val_min =            0,
       .val_max =            1,
     },
	 { 
       .name =               "dv_clamp_chroma",
       .real_name =          TRS("Clamp Chroma Values"),
       .type =               LQT_PARAMETER_INT,
       .val_default =        { 0 },
       .val_min =            0,
       .val_max =            1,
     },
	 { 
       .name =               "dv_add_ntsc_setup",
       .real_name =          TRS("Compensate for 7.5IRE NTSC setup"),
       .type =               LQT_PARAMETER_INT,
       .val_default =        { 0 },
       .val_min =            0,
       .val_max =            1,
     },
     { /* End of parameters */ }
  };
#endif

static lqt_parameter_info_static_t encode_parameters_dv[] =
  {
    { /* Set to one if the input is anamorphic 16x9 (stretched letter box).
         This is produced by many standard DV camcorders in 16x9 mode. */
      .name =               "dv_anamorphic16x9",
      .real_name =          TRS("Is Anamorphic 16x9"),
      .type =               LQT_PARAMETER_INT,
      .val_default =        { .val_int = 0 },
      .val_min =            { .val_int = 0 },
      .val_max =            { .val_int = 1 },
    },
    { 
      .name =               "dv_vlc_encode_passes",
      .real_name =          TRS("VLC Encode Passes"),
      .type =               LQT_PARAMETER_INT,
      .val_default =        { .val_int = 3 },
      .val_min =            { .val_int = 1 },
      .val_max =            { .val_int = 3 },
    },
    { 
      .name =               "dv_clamp_luma",
      .real_name =          TRS("Clamp Luma Values"),
      .type =               LQT_PARAMETER_INT,
      .val_default =        { .val_int = 0 },
      .val_min =            { .val_int = 0 },
      .val_max =            { .val_int = 1 },
    },
    { 
      .name =               "dv_clamp_chroma",
      .real_name =          TRS("Clamp Chroma Values"),
      .type =               LQT_PARAMETER_INT,
      .val_default =        { .val_int = 0 },
      .val_min =            { .val_int = 0 },
      .val_max =            { .val_int = 1 },
    },
    { 
      .name =               "dv_rem_ntsc_setup",
      .real_name =          TRS("Compensate for 7.5IRE NTSC setup"),
      .type =               LQT_PARAMETER_INT,
      .val_default =        { .val_int = 0 },
      .val_min =            { .val_int = 0 },
      .val_max =            { .val_int = 1 },
    },
    { /* End of parameters */ }
  };

static lqt_codec_info_static_t codec_info_dv_pal =
  {
    .name =                "dv_pal",
    .long_name =           TRS("Quasar DV Codec (PAL-Mode)"),
    .description =         TRS("Codec for digital video camaras. Based on libdv (http://libdv.sourceforge.net/)."),
    .fourccs =             fourccs_dv_pal,
    .type =                LQT_CODEC_VIDEO,
    .direction =           LQT_DIRECTION_ENCODE,
    .encoding_parameters = encode_parameters_dv,
    .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT | LQT_CODEC_OBSOLETE
  };

static lqt_codec_info_static_t codec_info_dv_ntsc =
  {
    .name =                "dv",
    .long_name =           TRS("Quasar DV Codec (NTSC-Mode)"),
    .description =         TRS("Codec for digital video camaras. Based on libdv (http://libdv.sourceforge.net/)."),
    .fourccs =             fourccs_dv_ntsc,
    .type =                LQT_CODEC_VIDEO,
    .direction =           LQT_DIRECTION_ENCODE,
    .encoding_parameters = encode_parameters_dv,
    .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT | LQT_CODEC_OBSOLETE
  };


/* These are called from the plugin loader */

LQT_EXTERN int get_num_codecs() { return 2; }

LQT_EXTERN lqt_codec_info_static_t * get_codec_info(int index)
  {
  if(index == 0)
    return &codec_info_dv_ntsc;
  else if(index == 1)
    return &codec_info_dv_pal;
  
  return (lqt_codec_info_static_t*)0;
  }


/*
 *   Return the actual codec constructor
 */

LQT_EXTERN lqt_init_codec_func_t get_codec(int index)
  {
  if(index < 2)
    return quicktime_init_codec_dv;
  return (lqt_init_codec_func_t)0;
  }
