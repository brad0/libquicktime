/*******************************************************************************
 lqt_rtjpeg.c

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
#include "rtjpeg_codec.h"
#include <quicktime/lqt_codecapi.h>

static char * fourccs_rtjpeg[]  = { "RTJ0", (char*)0 };

static lqt_parameter_info_static_t encode_parameters_rtjpeg[] = {
	{
          .name =        "rtjpeg_quality",
          .real_name =   TRS("Quality setting"),
          .type =        LQT_PARAMETER_INT,
          .val_default = { .val_int = 100 },
          .val_min =     { .val_int = 0 },
          .val_max =     { .val_int = 100 },
	},
	{
          .name =        "rtjpeg_key_rate",
          .real_name =   TRS("Key frame interval"),
          .type =        LQT_PARAMETER_INT,
          .val_default = { .val_int = 25 },
	},
	{
          .name =        "rtjpeg_luma_quant",
          .real_name =   TRS("Luma quantiser"),
          .type =        LQT_PARAMETER_INT,
          .val_default = { .val_int = 1 },
	},
	{
          .name =        "rtjpeg_chroma_quant",
          .real_name =   TRS("Chroma quantiser"),
          .type =        LQT_PARAMETER_INT,
          .val_default = { .val_int = 1 },
	},
	{ /* End of parameters */ }
};

static lqt_codec_info_static_t codec_info_rtjpeg = {
	.name =                "rtjpeg",
	.long_name =           TRS("RTjpeg"),
	.description =         TRS("RTjpeg - real time lossy codec."),
	.fourccs =             fourccs_rtjpeg,
	.type =                LQT_CODEC_VIDEO,
	.direction =           LQT_DIRECTION_BOTH,
	.encoding_parameters = encode_parameters_rtjpeg,
	.decoding_parameters = (lqt_parameter_info_static_t*)0,
  .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT,
};

/* These are called from the plugin loader */

LQT_EXTERN int get_num_codecs() { return 1; }

LQT_EXTERN lqt_codec_info_static_t * get_codec_info(int index)
{
	if(!index)
		return &codec_info_rtjpeg;
	return (lqt_codec_info_static_t*)0;
}

/*
 *   Return the actual codec constructor
 */

LQT_EXTERN lqt_init_codec_func_t get_codec(int index)
  {
  if(index == 0)
    return quicktime_init_codec_rtjpeg;
  return (lqt_init_codec_func_t)0;
  }

