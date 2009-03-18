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
#include <quicktime/colormodels.h>


static char * fourccs_dirac[]  = { "drac", (char*)0 };

lqt_parameter_info_static_t encode_parameters_schroedinger[] =
  {
    { 
      .name =        "rc",
      .real_name =   TRS("Rate control"),
      .type =        LQT_PARAMETER_SECTION,
    },
    { 
      .name =        "enc_rate_control",
      .real_name =   TRS("Rate control"),
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Constant noise threshold" },
      .stringlist_options = (char *[]){ TRS("Constant noise threshold"),
                                        TRS("Constant bitrate"),
                                        TRS("Low delay"), 
                                        TRS("Lossless"),
                                        TRS("Constant lambda"),
                                        TRS("Constant error"),
                                        (char *)0 },
    },
    {
      .name = "enc_bitrate",
      .real_name = TRS("Bitrate"),
      .type = LQT_PARAMETER_INT,
      .val_default = { .val_int =   13824000 },
    },
#if 0 /* Not used */
    {
      .name = "enc_min_bitrate",
      .real_name = TRS("Minimum bitrate"),
      .type = LQT_PARAMETER_INT,
      .val_default = { .val_int =   13824000 },
    },
    {
      .name = "enc_max_bitrate",
      .real_name = TRS("Maximum bitrate"),
      .type = LQT_PARAMETER_INT,
      .val_default = { .val_int =   13824000 },
    },
#endif
    {
      .name      = "enc_buffer_size",
      .real_name = TRS("Buffer size"),
      .type = LQT_PARAMETER_INT,
      .val_default = { .val_int =          0 },
    },
    {
      .name      = "enc_buffer_level",
      .real_name = TRS("Buffer level"),
      .type = LQT_PARAMETER_INT,
      .val_default = { .val_int =          0 },
    },
    {
      .name      = "enc_noise_threshold",
      .real_name = TRS("Noise Threshold"),
      .type = LQT_PARAMETER_FLOAT,
      .val_default = { .val_float =       25.0 },
      .val_min     = { .val_float =          0 },
      .val_max     = { .val_float =      100.0 },
      .num_digits  = 2,
    },
#if 0
    {
      .name      = "quality",
      .real_name = TRS("Quality"),
      .type = LQT_PARAMETER_FLOAT,
      .val_default = { .val_float =        7.0 },
      .val_min     = { .val_float =        0.0 },
      .val_max     = { .val_float =       10.0 },
      .num_digits  = 2,
    },
#endif
    { 
      .name =        "enc_frame_types",
      .real_name =   TRS("Frame types"),
      .type =        LQT_PARAMETER_SECTION,
    },
    {
      .name = "enc_gop_structure",
      .real_name = TRS("GOP Stucture"),
      .type = LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Adaptive" },
      .stringlist_options = (char *[]){ TRS("Adaptive"),
                                        TRS("Intra only"),
                                        TRS("Backref"), 
                                        TRS("Chained backref"),
                                        TRS("Biref"),
                                        TRS("Chained biref"),
                                        (char *)0 },
    },
    {
      .name = "enc_au_distance",
      .real_name = TRS("GOP size"),
      .type = LQT_PARAMETER_INT,
      .val_default = { .val_int = 30 },
    },
    { 
      .name =        "enc_me",
      .real_name =   TRS("Motion estimation"),
      .type =        LQT_PARAMETER_SECTION,
    },
    {
      .name =        "enc_mv_precision",
      .real_name =   TRS("MV Precision"),
      .type =        LQT_PARAMETER_INT,
      .val_default = { .val_int =     0 },
      .val_min     = { .val_int =     3 },
      .val_max     = { .val_int =     0 },
    },
    {
      .name =        "enc_motion_block_size",
      .real_name =   TRS("Block size"),
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Automatic" },
      .stringlist_options = (char *[]){ TRS("Automatic"),
                                        TRS("Small"),
                                        TRS("Medium"),
                                        TRS("Large"),
                                        (char *)0 },
      
    },
    {
      .name =        "enc_motion_block_overlap",
      .real_name =   TRS("Block overlap"),
      .type =        LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Automatic" },
      .stringlist_options = (char *[]){ TRS("Automatic"),
                                        TRS("None"),
                                        TRS("Partial"),
                                        TRS("Full"),
                                        (char *)0 },
    },
    { 
      .name =        "enc_wavelets",
      .real_name =   TRS("Wavelets"),
      .type =        LQT_PARAMETER_SECTION,
    },
    {
      .name = "enc_intra_wavelet",
      .real_name = TRS("Intra wavelet"),
      .type = LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "Deslauriers-Debuc (9,3)" },
      .stringlist_options = (char *[]){ TRS("Deslauriers-Debuc (9,3)"),
                                        TRS("LeGall (5,3)"),
                                        TRS("Deslauriers-Debuc (13,5)"),
                                        TRS("Haar (no shift)"),
                                        TRS("Haar (single shift)"),
                                        TRS("Fidelity"),
                                        TRS("Daubechies (9,7)"),
                                        (char *)0 },
    },
    {
      .name = "enc_inter_wavelet",
      .real_name = TRS("Inter wavelet"),
      .type = LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "LeGall (5,3)" },
      .stringlist_options = (char *[]){ TRS("Deslauriers-Debuc (9,3)"),
                                        TRS("LeGall (5,3)"),
                                        TRS("Deslauriers-Debuc (13,5)"),
                                        TRS("Haar (no shift)"),
                                        TRS("Haar (single shift)"),
                                        TRS("Fidelity"),
                                        TRS("Daubechies (9,7)"),
                                        (char *)0 },
    },
    { 
      .name =        "enc_filter",
      .real_name =   TRS("Filter"),
      .type =        LQT_PARAMETER_SECTION,
    },
    {
      .name = "enc_filtering",
      .real_name = TRS("Filter"),
      .type = LQT_PARAMETER_STRINGLIST,
      .val_default = { .val_string = "None" },
      .stringlist_options = (char *[]){ TRS("None"),
                                        TRS("Center weighted median"),
                                        TRS("Gaussian"),
                                        TRS("Add noise"),
                                        TRS("Adaptive Gaussian"),
                                        (char *)0 },
    },
    {
      .name = "enc_filter_value",
      .real_name = TRS("Filter value"),
      .type = LQT_PARAMETER_FLOAT,
      .val_default = { .val_float = 5.0 },
      .val_min =     { .val_float = 0.0 },
      .val_max =     { .val_float = 100.0 },
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
    .encoding_colormodels = (int[]) { BC_YUV420P, BC_YUV422P, BC_YUV444P,
                                      BC_YUVJ420P, BC_YUVJ422P, BC_YUVJ444P,
                                      LQT_COLORMODEL_NONE },
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
