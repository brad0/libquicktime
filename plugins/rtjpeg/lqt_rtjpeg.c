/* 
 * lqt_rtjpeg.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
 * Based entirely upon lqt_png.c from libquicktime 
 * (http://libquicktime.sf.net).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 */


#include "lqt_private.h"
#include "rtjpeg_codec.h"
#include <quicktime/lqt_codecapi.h>

static char * fourccs_rtjpeg[]  = { "RTJ0", (char*)0 };

static lqt_parameter_info_static_t encode_parameters_rtjpeg[] = {
	{
          .name =        "rtjpeg_quality",
          .real_name =   "Quality setting",
          .type =        LQT_PARAMETER_INT,
          .val_default = { .val_int = 100 },
          .val_min =     { .val_int = 0 },
          .val_max =     { .val_int = 100 },
	},
	{
          .name =        "rtjpeg_key_rate",
          .real_name =   "Key frame interval",
          .type =        LQT_PARAMETER_INT,
          .val_default = { .val_int = 25 },
	},
	{
          .name =        "rtjpeg_luma_quant",
          .real_name =   "Luma quantiser",
          .type =        LQT_PARAMETER_INT,
          .val_default = { .val_int = 1 },
	},
	{
          .name =        "rtjpeg_chroma_quant",
          .real_name =   "Chroma quantiser",
          .type =        LQT_PARAMETER_INT,
          .val_default = { .val_int = 1 },
	},
	{ /* End of parameters */ }
};

static lqt_codec_info_static_t codec_info_rtjpeg = {
	.name =                "rtjpeg",
	.long_name =           "RTjpeg",
	.description =         "RTjpeg - real time lossy codec.",
	.fourccs =             fourccs_rtjpeg,
	.type =                LQT_CODEC_VIDEO,
	.direction =           LQT_DIRECTION_BOTH,
	.encoding_parameters = encode_parameters_rtjpeg,
	.decoding_parameters = (lqt_parameter_info_static_t*)0,
  .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT,
};

/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_static_t * get_codec_info(int index)
{
	if(!index)
		return &codec_info_rtjpeg;
	return (lqt_codec_info_static_t*)0;
}

/*
 *   Return the actual codec constructor
 */

extern lqt_init_video_codec_func_t get_video_codec(int index)
{
	if(index == 0)
		return quicktime_init_codec_rtjpeg;
	return (lqt_init_video_codec_func_t)0;
}

