/* 
   lqt_rtjpeg.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
   Based entirely upon lqt_png.c from libquicktime 
   (http://libquicktime.sf.net).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    
*/

#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include <quicktime/colormodels.h>
#include "rtjpeg.h"

static char * fourccs_rtjpeg[]  = { "RTJ0", (char*)0 };

static int encoding_colormodels_rtjpeg[] = {
	BC_YUV420P, 
	LQT_COLORMODEL_NONE
};

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
	encoding_colormodels: encoding_colormodels_rtjpeg,
	decoding_colormodel: BC_YUV420P
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

/* Not used if we have only one colormodel */
#if 0
int get_stream_colormodel(quicktime_t * file, int track, int codec_index,
                          int * exact)
{
	return BC_YUV420P;
}
#endif
