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

#define MAX_FOURCCS 30

int ffmpeg_num_codecs = -1;

struct CODECIDMAP {
	int id;
	int index;
	AVCodec *encoder;
	AVCodec *decoder;
	char *short_name;
	char *name;
	char *fourccs[MAX_FOURCCS];
} codecidmap[] = {
/* Tables from mplayers config... */
/* Video */
	{ CODEC_ID_MPEG1VIDEO,
	  -1, NULL, NULL,
	  "mpg1",
	  "Mpeg 1 Video",
	  {"mpg1", "MPG1", "pim1", "PIM1", (char *)0} },
	{ CODEC_ID_MPEG4,
	  -1, NULL, NULL,
	  "mpg4",
	  "Mpeg 4 Video (DivX)",
	  {"DIVX", "divx", "DIV1", "div1", "MP4S", "mp4s", "M4S2", "m4s2", "xvid", "XVID", "XviD", "DX50", "dx50", "mp4v", "MP4V", (char *)0} },
	{ CODEC_ID_MSMPEG4V1,
	  -1, NULL, NULL,
	  "msmpeg4v1",
	  "MSMpeg 4v1",
	  {"DIV1", "div1", "MPG4", "mpg4", (char *)0} },
	{ CODEC_ID_MSMPEG4V2,
	  -1, NULL, NULL,
	  "msmpeg4v2",
	  "MSMpeg 4v2",
	  {"DIV2", "div2", "MP42", "mp42", (char *)0} },
	{ CODEC_ID_MSMPEG4V3,
	  -1, NULL, NULL,
	  "msmpeg4v3",
	  "MSMpeg 4v3",
	  {"MPG3", "mpg3", "MP43", "mp43", "DIV5", "div5", "DIV6", "div6", "DIV3", "div3", "DIV4", "div4", "AP41", "ap41", (char *)0} },
	{ CODEC_ID_WMV1,
	  -1, NULL, NULL,
	  "wmv1",
	  "WMV1",
	  {"WMV1", "wmv1", (char *)0} },
	{ CODEC_ID_H263,
	  -1, NULL, NULL,
	  "h263",
	  "H263",
	  {"H263", "h263", (char *)0} },
	{ CODEC_ID_H263P,
	  -1, NULL, NULL,
	  "h263p",
	  "H263+",
	  {"U263", "u263", (char *)0} },
	{ CODEC_ID_H263I,
	  -1, NULL, NULL,
	  "i263",
	  "I263",
	  {"I263", "i263", "viv1", "VIV1", (char *)0} },
	{ CODEC_ID_RV10,
	  -1, NULL, NULL,
	  "rv10",
	  "Real Video 10",
	  {"RV10", "rv10", "RV13", "rv13", (char *)0} },
	{ CODEC_ID_MJPEG,
	  -1, NULL, NULL,
	  "mjpg",
	  "MJPEG",
	  {"MJPG", "mjpg", "JPEG", "jpeg", "dmb1", (char *)0} },

/* Audio */
	{ CODEC_ID_MP2,
	  -1, NULL, NULL,
	  "mp2",
	  "Mpeg Layer 2 Audio",
	  {".mp2", ".MP2", "ms\0\x50", "MS\0\x50", (char *)0} },
	{ CODEC_ID_MP3LAME,
	  -1, NULL, NULL,
	  "mp3",
	  "Mpeg Layer 3 Audio",
	  {".mp3", ".MP3", "ms\0\x55", "MS\0\x55"} },
	{ CODEC_ID_AC3,
	  -1, NULL, NULL,
	  "ac3",
	  "AC3 Audio",
	  {".ac3", ".AC3", (char *)0} },
};
#define NUMMAPS ((int)(sizeof(codecidmap)/sizeof(struct CODECIDMAP)))

void ffmpeg_map_init(void)
{
	AVCodec *codec;
	int i;
	if(ffmpeg_num_codecs >= 0)
		return;
	avcodec_register_all();
	avcodec_init();
	ffmpeg_num_codecs = 0;
	for(codec = first_avcodec; codec; codec = codec->next) {
		for(i = 0; i < NUMMAPS; i++) {
			if(codec->id == codecidmap[i].id) {
				if(codecidmap[i].index < 0)
					codecidmap[i].index = ffmpeg_num_codecs++;
				if(codec->encode)
					codecidmap[i].encoder = codec;
				if(codec->decode)
					codecidmap[i].decoder = codec;
				break;
			}
		}
	}
}

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

static char ffmpeg_name[50];
static char ffmpeg_long_name[50];
static char ffmpeg_description[100];

static lqt_codec_info_static_t codec_info_ffmpeg = {
	name:        ffmpeg_name,
	long_name:   ffmpeg_long_name,
	description: ffmpeg_description,
	fourccs:     NULL,
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
	ffmpeg_map_init();
	return ffmpeg_num_codecs;
}

extern lqt_codec_info_static_t * get_codec_info(int index)
{
	int i;
	
	ffmpeg_map_init();
	for(i = 0; i < ffmpeg_num_codecs; i++) {
		if(codecidmap[i].index == index) {
			if(codecidmap[i].encoder && codecidmap[i].decoder) {
				snprintf(ffmpeg_name, 50, "ffmpeg_%s", codecidmap[i].short_name);
				snprintf(ffmpeg_long_name, 50, "FFMPEG %s Codec", codecidmap[i].name);
				snprintf(ffmpeg_description, 100, "FFMPEG %s Codec", codecidmap[i].name);
				codec_info_ffmpeg.direction = LQT_DIRECTION_BOTH;
				if(codecidmap[i].encoder->type == CODEC_TYPE_VIDEO) {
					codec_info_ffmpeg.type = LQT_CODEC_VIDEO;
					codec_info_ffmpeg.encoding_parameters = encode_parameters_ffmpeg;
					codec_info_ffmpeg.decoding_parameters = decode_parameters_ffmpeg;
				} else {
					codec_info_ffmpeg.type = LQT_CODEC_AUDIO;
					codec_info_ffmpeg.encoding_parameters = encode_parameters_ffmpeg_audio;
					codec_info_ffmpeg.decoding_parameters = decode_parameters_ffmpeg_audio;
				}
			} else if(codecidmap[i].encoder) {
				snprintf(ffmpeg_name, 50, "ffmpeg_%s_enc", codecidmap[i].short_name);
				snprintf(ffmpeg_long_name, 50, "FFMPEG %s Encoder", codecidmap[i].name);
				snprintf(ffmpeg_description, 100, "FFMPEG %s Encoder", codecidmap[i].name);
				codec_info_ffmpeg.direction = LQT_DIRECTION_ENCODE;
				if(codecidmap[i].encoder->type == CODEC_TYPE_VIDEO) {
					codec_info_ffmpeg.type = LQT_CODEC_VIDEO;
					codec_info_ffmpeg.encoding_parameters = encode_parameters_ffmpeg;
					codec_info_ffmpeg.decoding_parameters = NULL;
				} else {
					codec_info_ffmpeg.type = LQT_CODEC_AUDIO;
					codec_info_ffmpeg.encoding_parameters = encode_parameters_ffmpeg_audio;
					codec_info_ffmpeg.decoding_parameters = NULL;
				}
			} else if(codecidmap[i].decoder) {
				snprintf(ffmpeg_name, 50, "ffmpeg_%s_dec", codecidmap[i].short_name);
				snprintf(ffmpeg_long_name, 50, "FFMPEG %s Decoder", codecidmap[i].name);
				snprintf(ffmpeg_description, 100, "FFMPEG %s Decoder", codecidmap[i].name);
				codec_info_ffmpeg.direction = LQT_DIRECTION_DECODE;
				if(codecidmap[i].decoder->type == CODEC_TYPE_VIDEO) {
					codec_info_ffmpeg.type = LQT_CODEC_VIDEO;
					codec_info_ffmpeg.encoding_parameters = NULL;
					codec_info_ffmpeg.decoding_parameters = decode_parameters_ffmpeg;
				} else {
					codec_info_ffmpeg.type = LQT_CODEC_AUDIO;
					codec_info_ffmpeg.encoding_parameters = NULL;
					codec_info_ffmpeg.decoding_parameters = decode_parameters_ffmpeg_audio;
				}
			} else {
				return NULL;
			}
			codec_info_ffmpeg.fourccs = codecidmap[i].fourccs;
			return &codec_info_ffmpeg;
		}
	}
	return NULL;
}

/*
 *   Return the actual codec constructor
 */

/* 
   This is where it gets ugly - make sure there are enough dummys to 
   handle all codecs!
*/

#define IFUNC(x) \
void quicktime_init_codec_ffmpeg ## x(quicktime_video_map_t *vtrack) \
{ \
	int i; \
	for(i = 0; i < ffmpeg_num_codecs; i++) { \
		if(codecidmap[i].index == x) { \
			quicktime_init_codec_ffmpeg(vtrack, \
				codecidmap[i].encoder, \
				codecidmap[i].decoder); \
		} \
	} \
}
IFUNC(0)
IFUNC(1)
IFUNC(2)
IFUNC(3)
IFUNC(4)
IFUNC(5)
IFUNC(6)
IFUNC(7)
IFUNC(8)
IFUNC(9)
IFUNC(10)
IFUNC(11)
IFUNC(12)
IFUNC(13)
IFUNC(14)
IFUNC(15)
IFUNC(16)
IFUNC(17)
IFUNC(18)
IFUNC(19)
IFUNC(20)
IFUNC(21)
IFUNC(22)
IFUNC(23)
IFUNC(24)
IFUNC(25)
IFUNC(26)
IFUNC(27)
IFUNC(28)
IFUNC(29)
#define MAXFUNC 29

#undef IFUNC

extern lqt_init_video_codec_func_t get_video_codec(int index)
{
	ffmpeg_map_init();
	if(index > MAXFUNC) {
		fprintf(stderr, "lqt_ffmpeg error: Insufficient dummy calls - please report!\n");
		return NULL;
	}
	switch(index) {
		case 0: return quicktime_init_codec_ffmpeg0;
		case 1: return quicktime_init_codec_ffmpeg1;
		case 2:	return quicktime_init_codec_ffmpeg2;
		case 3:	return quicktime_init_codec_ffmpeg3;
		case 4:	return quicktime_init_codec_ffmpeg4;
		case 5:	return quicktime_init_codec_ffmpeg5;
		case 6:	return quicktime_init_codec_ffmpeg6;
		case 7:	return quicktime_init_codec_ffmpeg7;
		case 8:	return quicktime_init_codec_ffmpeg8;
		case 9:	return quicktime_init_codec_ffmpeg9;
		case 10: return quicktime_init_codec_ffmpeg10;
		case 11: return quicktime_init_codec_ffmpeg11;
		case 12: return quicktime_init_codec_ffmpeg12;
		case 13: return quicktime_init_codec_ffmpeg13;
		case 14: return quicktime_init_codec_ffmpeg14;
		case 15: return quicktime_init_codec_ffmpeg15;
		case 16: return quicktime_init_codec_ffmpeg16;
		case 17: return quicktime_init_codec_ffmpeg17;
		case 18: return quicktime_init_codec_ffmpeg18;
		case 19: return quicktime_init_codec_ffmpeg19;
		case 20: return quicktime_init_codec_ffmpeg20;
		case 21: return quicktime_init_codec_ffmpeg21;
		case 22: return quicktime_init_codec_ffmpeg22;
		case 23: return quicktime_init_codec_ffmpeg23;
		case 24: return quicktime_init_codec_ffmpeg24;
		case 25: return quicktime_init_codec_ffmpeg25;
		case 26: return quicktime_init_codec_ffmpeg26;
		case 27: return quicktime_init_codec_ffmpeg27;
		case 28: return quicktime_init_codec_ffmpeg28;
		case 29: return quicktime_init_codec_ffmpeg29;
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
