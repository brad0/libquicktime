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


#include "config.h"
#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include <quicktime/colormodels.h>
#include "rtjpeg_codec.h"

static char * fourccs_rtjpeg[]  = { "RTJ0", (char*)0 };

static lqt_parameter_info_static_t encode_parameters_rtjpeg[] = {
	{
		"rtjpeg_quality",
		"Quality setting",
		LQT_PARAMETER_INT,
		{ 100 },
		0,
		100,
		(char**)0
	},
	{
		"rtjpeg_key_rate",
		"Key frame interval",
		LQT_PARAMETER_INT,
		{ 25 },
		0,
		0,
		(char**)0
	},
	{
		"rtjpeg_luma_quant",
		"Luma quantiser",
		LQT_PARAMETER_INT,
		{ 1 },
		0,
		0,
		(char**)0
	},
	{
		"rtjpeg_chroma_quant",
		"Chroma quantiser",
		LQT_PARAMETER_INT,
		{ 1 },
		0,
		0,
		(char**)0
	},
	{ /* End of parameters */ }
};

static lqt_codec_info_static_t codec_info_rtjpeg = {
	name:        "rtjpeg",
	long_name:   "RTjpeg",
	description: "RTjpeg - real time lossy codec.",
	fourccs:     fourccs_rtjpeg,
	type:        LQT_CODEC_VIDEO,
	direction:   LQT_DIRECTION_BOTH,
	encoding_parameters: encode_parameters_rtjpeg,
	decoding_parameters: (lqt_parameter_info_static_t*)0,
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

