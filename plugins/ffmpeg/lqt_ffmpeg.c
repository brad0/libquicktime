/* 
   lqt_ffmpeg.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
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

#include <ctype.h>
#include <string.h>
#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include <quicktime/colormodels.h>
#include "ffmpeg.h"

#define MAX_CODECS 30
#define MAX_FOURCCS 500

struct CODECIDMAP {
	int id;
	char * fourcc;
} codecidmap[] = {
/* Tables from mplayers config... */
/* Video */
	{CODEC_ID_MPEG1VIDEO, "mpg1"},
	{CODEC_ID_MPEG1VIDEO, "MPG1"},
	{CODEC_ID_MPEG1VIDEO, "pim1"},
	{CODEC_ID_MPEG1VIDEO, "PIM1"},

	{CODEC_ID_MSMPEG4V3, "MPG3"},
	{CODEC_ID_MSMPEG4V3, "mpg3"},
	{CODEC_ID_MSMPEG4V3, "MP43"},
	{CODEC_ID_MSMPEG4V3, "mp43"},
	{CODEC_ID_MSMPEG4V3, "DIV5"},
	{CODEC_ID_MSMPEG4V3, "div5"},
	{CODEC_ID_MSMPEG4V3, "DIV6"},
	{CODEC_ID_MSMPEG4V3, "div6"},
	{CODEC_ID_MSMPEG4V3, "DIV3"},
	{CODEC_ID_MSMPEG4V3, "div3"},
	{CODEC_ID_MSMPEG4V3, "DIV4"},
	{CODEC_ID_MSMPEG4V3, "div4"},
	{CODEC_ID_MSMPEG4V3, "AP41"},
	{CODEC_ID_MSMPEG4V3, "ap41"},

	{CODEC_ID_MSMPEG4V2, "DIV2"},
	{CODEC_ID_MSMPEG4V2, "div2"},
	{CODEC_ID_MSMPEG4V2, "MP42"},
	{CODEC_ID_MSMPEG4V2, "mp42"},
	
	{CODEC_ID_MSMPEG4V1, "DIV1"},
	{CODEC_ID_MSMPEG4V1, "div1"},
	{CODEC_ID_MSMPEG4V1, "MPG4"},
	{CODEC_ID_MSMPEG4V1, "mpg4"},
	
	{CODEC_ID_WMV1, "WMV1"},
	{CODEC_ID_WMV1, "wmv1"},
	
	{CODEC_ID_MPEG4, "DIVX"},
	{CODEC_ID_MPEG4, "divx"},
	{CODEC_ID_MPEG4, "DIV1"},
	{CODEC_ID_MPEG4, "div1"},
	{CODEC_ID_MPEG4, "MP4S"},
	{CODEC_ID_MPEG4, "mp4s"},
	{CODEC_ID_MPEG4, "M4S2"},
	{CODEC_ID_MPEG4, "m4s2"},
	{CODEC_ID_MPEG4, "xvid"},
	{CODEC_ID_MPEG4, "XVID"},
	{CODEC_ID_MPEG4, "XviD"},
	{CODEC_ID_MPEG4, "DX50"},
	{CODEC_ID_MPEG4, "dx50"},
	{CODEC_ID_MPEG4, "mp4v"},
	{CODEC_ID_MPEG4, "MP4V"},
	
	{CODEC_ID_MJPEG, "MJPG"},
	{CODEC_ID_MJPEG, "mjpg"},
	{CODEC_ID_MJPEG, "JPEG"},
	{CODEC_ID_MJPEG, "jpeg"},
	
	{CODEC_ID_H263I, "I263"},
	{CODEC_ID_H263I, "i263"},
	
	{CODEC_ID_H263P, "H263"},
	{CODEC_ID_H263P, "h263"},
	{CODEC_ID_H263P, "U263"},
	{CODEC_ID_H263P, "u263"},

/*      Not used in Quicktime  */
/* 	{CODEC_ID_H263P, "viv1"}, */
/* 	{CODEC_ID_H263P, "VIV1"}, */

	
/* 	{CODEC_ID_RV10, "RV10"}, */
/* 	{CODEC_ID_RV10, "rv10"}, */
/* 	{CODEC_ID_RV10, "RV13"}, */
/* 	{CODEC_ID_RV10, "rv13"}, */

/* Audio */
	{CODEC_ID_MP3LAME, ".mp3"},
	{CODEC_ID_MP3LAME, ".MP3"},
	{CODEC_ID_MP3LAME, "ms\0\x55"},
	{CODEC_ID_MP3LAME, "MS\0\x55"},
	
	{CODEC_ID_MP2, ".mp2"},
	{CODEC_ID_MP2, ".MP2"},
	{CODEC_ID_MP2, "ms\0\x50"},
	{CODEC_ID_MP2, "MS\0\x50"},

	{CODEC_ID_AC3, ".ac3"},
	{CODEC_ID_AC3, ".AC3"},
};
#define NUMMAPS ((int)(sizeof(codecidmap)/sizeof(struct CODECIDMAP)))

AVCodec * fcc_to_codec(char * fcc, int enc, int vid)
{
	AVCodec *codec;
	int i;
	for(i = 0; i < NUMMAPS; i++) {
		if(memcmp(codecidmap[i].fourcc, fcc, 4) == 0)
			break;
	}
	if(i == NUMMAPS)
		return NULL;
	avcodec_register_all();
	for(codec = first_avcodec; codec; codec = codec->next) {
		if((enc)&&(!codec->encode))
			continue;
		if((!enc)&&(!codec->decode))
			continue;
		if((vid)&&(codec->type != CODEC_TYPE_VIDEO))
			continue;
		if((!vid)&&(codec->type != CODEC_TYPE_AUDIO))
			continue;
		if(codec->id == codecidmap[i].id)
			return codec;
	}
	return NULL;
}

static char * fourccs_ffmpeg[MAX_FOURCCS];

/* Common to all */
static int encoding_colormodels_ffmpeg[] = {
	BC_YUV420P, 
	LQT_COLORMODEL_NONE
};

/* Common to all */
static lqt_parameter_info_static_t encode_parameters_ffmpeg[] = {
	{
		"bit_rate",
		"Bit rate (kbps)",
		LQT_PARAMETER_INT,
		{ 800 },
		4,
		24000,
		(char**)0
	},
	{
		"bit_rate_tolerance",
		"Bit rate tolernce",
		LQT_PARAMETER_INT,
		{ 1024 * 8 },
		4,
		24000,
		(char**)0
	},
	{
		"flags_hq",
		"High quality mode",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"flags_4mv",
		"4MV mode",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"flags_part",
		"Data partitioning mode",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"flags_gray",
		"Gay scale only mode",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"flags_pass",
		"Pass (0 = single pass)",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		2,
		(char**)0
	},
	{
		"flags_fix",
		"Fixed quality (VBR)",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"me_method",
		"Motion estimation method",
		LQT_PARAMETER_STRINGLIST,
		{val_string: "Zero"},
		0,
		100,
		((char *[]){"Zero", "Full", "Log", "Phods", "Epzs", "X1", (char *)0})
	},
	{
		"aspect_ratio_info",
		"Aspect ratio",
		LQT_PARAMETER_STRINGLIST,
		{val_string: "Square"},
		0,
		100,
		((char *[]){"Square", "4:3 (625)", "4:3 (525)", "16:9 (625)", "16:9 (525)", (char *)0})
	},
	{
		"gop_size",
		"GOP size",
		LQT_PARAMETER_INT,
		{ 250 },
		0,
		300,
		(char**)0
	},
	{
		"quality",
		"Quality",
		LQT_PARAMETER_INT,
		{ 1 },
		1,
		31,
		(char**)0
	},
	{
		"qcompress",
		"Qscale change between easy and hard scenes",
		LQT_PARAMETER_INT,
		{ 50 },
		0,
		100,
		(char**)0
	},
	{
		"qblur",
		"Qscale smoothing over time",
		LQT_PARAMETER_INT,
		{ 50 },
		0,
		100,
		(char**)0
	},
	{
		"qmin",
		"Minimum qscale",
		LQT_PARAMETER_INT,
		{ 3 },
		1,
		31,
		(char**)0
	},
	{
		"qmax",
		"Maximum qscale",
		LQT_PARAMETER_INT,
		{ 15 },
		1,
		31,
		(char**)0
	},
	{
		"max_qdiff",
		"Maximum qscale difference",
		LQT_PARAMETER_INT,
		{ 3 },
		1,
		31,
		(char**)0
	},
	{
		"max_b_frames",
		"Maximum B frames",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		FF_MAX_B_FRAMES,
		(char**)0
	},
	{
		"b_quant_factor",
		"Scale factor btw IPS and B frames",
		LQT_PARAMETER_INT,
		{ 2 },
		0,
		31,
		(char**)0
	},
	{
		"b_quant_offset",
		"Scale offset btw IPS and B frames",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		31,
		(char**)0
	},
	{
		"rc_strategy",
		"RC strategy",
		LQT_PARAMETER_INT,
		{ 2 },
		0,
		2,
		(char**)0
	},
	{
		"b_frame_strategy",
		"B Frame strategy",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"rtp_payload_size",
		"Packet size (0 = variable)",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1000000,
		(char**)0
	},
	{
		"workaround_bugs",
		"Enable bug worarounds",
		LQT_PARAMETER_INT,
		{ 1 },
		0,
		1,
		(char**)0
	},
	{
		"luma_elim_threshold",
		"Luma elimination threshold",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		99,
		(char**)0
	},
	{
		"chroma_elim_threshold",
		"Chroma elimination threshold",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		99,
		(char**)0
	},
	{
		"strict_std_compliance",
		"Comply strictly with standards",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"error_resilience",
		"Error resilience",
		LQT_PARAMETER_INT,
		{ 1 },
		0,
		1,
		(char**)0
	},
	{ /* End of parameters */ },
};

static lqt_parameter_info_static_t decode_parameters_ffmpeg[] = {
	{
		"flags_gray",
		"Gay scale only mode",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"workaround_bugs",
		"Enable bug worarounds",
		LQT_PARAMETER_INT,
		{ 1 },
		0,
		1,
		(char**)0
	},
	{
		"error_resilience",
		"Error resilience",
		LQT_PARAMETER_INT,
		{ 1 },
		0,
		1,
		(char**)0
	},
	{ /* End of parameters */ },
};

static lqt_parameter_info_static_t encode_parameters_ffmpeg_audio[] = {
	{
		"bit_rate",
		"Bit rate (kbps)",
		LQT_PARAMETER_INT,
		{ 800 },
		4,
		24000,
		(char**)0
	},
	{
		"bit_rate_tolerance",
		"Bit rate tolernce",
		LQT_PARAMETER_INT,
		{ 1024 * 8 },
		4,
		24000,
		(char**)0
	},
	{
		"flags_hq",
		"High quality mode",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"flags_part",
		"Data partitioning mode",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"flags_pass",
		"Pass (0 = single pass)",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		2,
		(char**)0
	},
	{
		"flags_fix",
		"Fixed quality (VBR)",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"gop_size",
		"GOP size",
		LQT_PARAMETER_INT,
		{ 250 },
		0,
		300,
		(char**)0
	},
	{
		"quality",
		"Quality",
		LQT_PARAMETER_INT,
		{ 1 },
		1,
		31,
		(char**)0
	},
	{
		"qcompress",
		"Qscale change between easy and hard scenes",
		LQT_PARAMETER_INT,
		{ 50 },
		0,
		100,
		(char**)0
	},
	{
		"qblur",
		"Qscale smoothing over time",
		LQT_PARAMETER_INT,
		{ 50 },
		0,
		100,
		(char**)0
	},
	{
		"qmin",
		"Minimum qscale",
		LQT_PARAMETER_INT,
		{ 3 },
		1,
		31,
		(char**)0
	},
	{
		"qmax",
		"Maximum qscale",
		LQT_PARAMETER_INT,
		{ 15 },
		1,
		31,
		(char**)0
	},
	{
		"max_qdiff",
		"Maximum qscale difference",
		LQT_PARAMETER_INT,
		{ 3 },
		1,
		31,
		(char**)0
	},
	{
		"max_b_frames",
		"Maximum B frames",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		FF_MAX_B_FRAMES,
		(char**)0
	},
	{
		"b_quant_factor",
		"Scale factor btw IPS and B frames",
		LQT_PARAMETER_INT,
		{ 2 },
		0,
		31,
		(char**)0
	},
	{
		"b_quant_offset",
		"Scale offset btw IPS and B frames",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		31,
		(char**)0
	},
	{
		"rc_strategy",
		"RC strategy",
		LQT_PARAMETER_INT,
		{ 2 },
		0,
		2,
		(char**)0
	},
	{
		"b_frame_strategy",
		"B Frame strategy",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"rtp_payload_size",
		"Packet size (0 = variable)",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1000000,
		(char**)0
	},
	{
		"workaround_bugs",
		"Enable bug worarounds",
		LQT_PARAMETER_INT,
		{ 1 },
		0,
		1,
		(char**)0
	},
	{
		"strict_std_compliance",
		"Comply strictly with standards",
		LQT_PARAMETER_INT,
		{ 0 },
		0,
		1,
		(char**)0
	},
	{
		"error_resilience",
		"Error resilience",
		LQT_PARAMETER_INT,
		{ 1 },
		0,
		1,
		(char**)0
	},
	{ /* End of parameters */ },
};

static lqt_parameter_info_static_t decode_parameters_ffmpeg_audio[] = {
	{
		"workaround_bugs",
		"Enable bug worarounds",
		LQT_PARAMETER_INT,
		{ 1 },
		0,
		1,
		(char**)0
	},
	{
		"error_resilience",
		"Error resilience",
		LQT_PARAMETER_INT,
		{ 1 },
		0,
		1,
		(char**)0
	},
	{ /* End of parameters */ },
};

/* Template */
static lqt_codec_info_static_t codec_info_ffmpeg = {
	name:        "ffmpeg",
	long_name:   "ffmpeg",
	description: "ffmpeg libavcodec codec library",
	fourccs:     fourccs_ffmpeg,
	type:        0,
	direction:   0,
	encoding_parameters: NULL,
	decoding_parameters: NULL,
	encoding_colormodels: encoding_colormodels_ffmpeg,
	decoding_colormodel: BC_YUV420P
};

/* These are called from the plugin loader */

extern int get_num_codecs()
{
	return 4;
}

extern lqt_codec_info_static_t * get_codec_info(int index)
{
	AVCodec *codec;
	int i = 0;
	int j;
	int enc;
	int vid;

	switch(index) {
		case 0:
			enc = 1;
			vid = 1;
			codec_info_ffmpeg.name = "ffmpeg_ve";
			codec_info_ffmpeg.long_name = "ffmpeg video encoder";
			codec_info_ffmpeg.description = "ffmpeg libavcodec codec library - video encoders";
			codec_info_ffmpeg.type = LQT_CODEC_VIDEO;
			codec_info_ffmpeg.direction = LQT_DIRECTION_ENCODE;
			codec_info_ffmpeg.encoding_parameters = encode_parameters_ffmpeg;
			codec_info_ffmpeg.decoding_parameters = NULL;
			break;
		case 1:
			enc = 0;
			vid = 1;
			codec_info_ffmpeg.name = "ffmpeg_vd";
			codec_info_ffmpeg.long_name = "ffmpeg video decoder";
			codec_info_ffmpeg.description = "ffmpeg libavcodec codec library - video decoders";
			codec_info_ffmpeg.type = LQT_CODEC_VIDEO;
			codec_info_ffmpeg.direction = LQT_DIRECTION_DECODE;
			codec_info_ffmpeg.encoding_parameters = NULL;
			codec_info_ffmpeg.decoding_parameters = decode_parameters_ffmpeg;
			break;
		case 2:
			enc = 1;
			vid = 0;
			codec_info_ffmpeg.name = "ffmpeg_ae";
			codec_info_ffmpeg.long_name = "ffmpeg audio encoder";
			codec_info_ffmpeg.description = "ffmpeg libavcodec codec library - audio encoders";
			codec_info_ffmpeg.type = LQT_CODEC_AUDIO;
			codec_info_ffmpeg.direction = LQT_DIRECTION_ENCODE;
			codec_info_ffmpeg.encoding_parameters = encode_parameters_ffmpeg_audio;
			codec_info_ffmpeg.decoding_parameters = NULL;
			break;
		case 3:
			enc = 0;
			vid = 0;
			codec_info_ffmpeg.name = "ffmpeg_ad";
			codec_info_ffmpeg.long_name = "ffmpeg audio decoder";
			codec_info_ffmpeg.description = "ffmpeg libavcodec codec library - audio decoders";
			codec_info_ffmpeg.type = LQT_CODEC_AUDIO;
			codec_info_ffmpeg.direction = LQT_DIRECTION_DECODE;
			codec_info_ffmpeg.encoding_parameters = NULL;
			codec_info_ffmpeg.decoding_parameters = decode_parameters_ffmpeg_audio;
			break;
		default:
			return (lqt_codec_info_static_t*)0;
	}
	
	avcodec_register_all();
	for(codec = first_avcodec; codec; codec = codec->next) {
		if((vid)&&(codec->type != CODEC_TYPE_VIDEO))
			continue;
		if((!vid)&&(codec->type != CODEC_TYPE_AUDIO))
			continue;
		if((enc)&&(!codec->encode))
			continue;
		if((!enc)&&(!codec->decode))
			continue;
		for(j = 0; j < NUMMAPS; j++) {
			if(codecidmap[j].id == codec->id)
				fourccs_ffmpeg[i++] = codecidmap[j].fourcc;
			if(i == (MAX_FOURCCS - 1))
				i--;
		}
	}
	fourccs_ffmpeg[i] = (char *)0;
	if(i == 0)
		return (lqt_codec_info_static_t*)0;
	return &codec_info_ffmpeg;
}

/*
 *   Return the actual codec constructor
 */

extern lqt_init_video_codec_func_t get_video_codec(int index)
{
	switch(index) {
		case 0:
			return quicktime_init_codec_ffmpeg_video_enc;
		case 1:
			return quicktime_init_codec_ffmpeg_video_dec;
		case 2:
			return quicktime_init_codec_ffmpeg_audio_enc;
		case 3:
			return quicktime_init_codec_ffmpeg_audio_dec;
		default:
			break;
	}
	return (lqt_init_video_codec_func_t)0;
}

int get_stream_colormodel(quicktime_t * file, int track, int codec_index,
                          int * exact)
{
	return BC_YUV420P;
}
