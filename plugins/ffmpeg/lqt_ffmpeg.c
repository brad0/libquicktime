/* 
 * lqt_ffmpeg.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
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

/* WARNING: Don't even think about adding support
   for ffmpeg's PCM codecs because they will crash.
   
*/

#include "config.h"
#include <ctype.h>
#include <string.h>
#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include <quicktime/colormodels.h>
#include "ffmpeg.h"

#define MAX_FOURCCS 30
#define MAX_WAV_IDS 4

int ffmpeg_num_audio_codecs = -1;
int ffmpeg_num_video_codecs = -1;

#define ENCODE_PARAM_AUDIO \
	{\
	  "bit_rate_audio",\
          "Bit rate (kbps)",\
	  LQT_PARAMETER_INT,\
          { 128 },\
          0,\
          0,\
          (char**)0\
	}

#define ENCODE_PARAM_VIDEO_GENERAL \
        { \
        "general", \
        "General", \
        LQT_PARAMETER_SECTION, \
        }, \
        {\
          "flags_gray",\
          "Gray scale only mode",\
          LQT_PARAMETER_INT,\
          { 0 },\
          0,\
          1,\
          (char**)0\
        }, \
	{ \
	"strict_std_compliance", \
	"Standard compilance", \
	LQT_PARAMETER_INT, \
	{ 0 }, \
	0, \
	2, \
	(char**)0 \
	}, \
        {  \
        "aspect_ratio_info", \
        "Aspect Ratio", \
	LQT_PARAMETER_STRINGLIST, \
        {val_string:  "Square" }, \
        0, \
        0, \
        (char*[]){ "Square", "4:3", "16:9", (char*)0 } \
        }
  
#define ENCODE_PARAM_VIDEO_BITRATE \
        { \
        "bitrate", \
        "Bitrate", \
        LQT_PARAMETER_SECTION, \
        }, \
        {\
	  "bit_rate_video",\
          "Bit rate (kbps)",\
	  LQT_PARAMETER_INT,\
          { 800 },\
          0,\
          0,\
          (char**)0\
	},\
        { \
          "bit_rate_tolerance",\
          "Bitrate Tolerance (kbps)\n",\
          LQT_PARAMETER_INT,\
          { 4000 } \
        }, \
        { \
          "rc_min_rate",\
          "Minimum bitrate (kbps)\n",\
          LQT_PARAMETER_INT,\
          { 0 } \
        }, \
        { \
          "rc_max_rate",\
          "Maximum bitrate (kbps)\n",\
          LQT_PARAMETER_INT,\
          { 0 } \
        }, \
        { \
        "qcompress", \
        "Qscale change between easy and hard scenes", \
        LQT_PARAMETER_INT, \
        { 50 }, \
        0, \
        100, \
        (char**)0 \
        }, \
        { \
        "qblur", \
        "Qscale smoothing over time", \
        LQT_PARAMETER_INT, \
        { 50 }, \
        0, \
        100, \
        (char**)0 \
        }
  
#define ENCODE_PARAM_VIDEO_VBR \
        { \
        "vbr", \
        "VBR Options", \
        LQT_PARAMETER_SECTION, \
        }, \
        {\
          "qscale", \
          "VBR quantizer scale (0 = CBR)", \
          LQT_PARAMETER_INT, \
          { 0 }, \
          0, \
          31 \
        }, \
        {\
          "qmin", \
          "min quantiser scale (VBR)", \
          LQT_PARAMETER_INT, \
          { 2 }, \
          0, \
          31 \
        }, \
        {\
          "qmax", \
          "max quantiser scale (VBR)", \
          LQT_PARAMETER_INT, \
          { 31 }, \
          0, \
          31 \
        }, \
        {\
          "mb_qmin", \
          "min macroblock quantiser scale (VBR)", \
          LQT_PARAMETER_INT, \
          { 2 }, \
          0, \
          31 \
        }, \
        {\
          "mb_qmax", \
          "max macroblock quantiser scale (VBR)", \
          LQT_PARAMETER_INT, \
          { 31 }, \
          0, \
          31 \
        }, \
        {\
          "max_qdiff", \
          "max difference between the quantiser scale (VBR)", \
          LQT_PARAMETER_INT, \
          { 3 }, \
          0, \
          31 \
        } \

#define ENCODE_PARAM_VIDEO_TEMPORAL \
        { \
        "temporal_compression", \
        "Temporal Compression", \
        LQT_PARAMETER_SECTION \
        }, \
	{\
	"me_method",\
	"Motion estimation method",\
	LQT_PARAMETER_STRINGLIST,\
	{val_string: "Zero"},\
        0, \
        0, \
	((char *[]){"Zero", "Phods", "Log", "X1", "Epzs", "Full", (char *)0})\
	},\
	{\
	"mb_decision",\
	"MB decision mode",\
	LQT_PARAMETER_STRINGLIST,\
	{val_string: "Simple"},\
        0, \
        0, \
	((char *[]){"Simple", "Fewest bits", "Rate distoration", (char *)0})\
	},\
        { \
        "gop_size", \
        "GOP size (0 = intra only)", \
        LQT_PARAMETER_INT, \
        { val_int: 250 }, \
        0, \
        300, \
        (char**)0 \
        } \
 
#define ENCODE_PARAM_VIDEO_MPEG4 \
        { \
        "mpeg4", \
        "MPEG-4 Options", \
        LQT_PARAMETER_SECTION \
        }, \
	{\
	"flags_4mv",\
	"Use four motion vector by macroblock",\
	LQT_PARAMETER_INT,\
	{ 0 },\
	0,\
	1,\
	(char**)0\
	}, \
        { \
        "flags_part", \
        "Data partitioning mode", \
        LQT_PARAMETER_INT, \
        { 0 }, \
        0, \
        1, \
        (char**)0 \
        }


#define ENCODE_PARAM_VIDEO_H263P \
        { \
        "h263p", \
        "H263+ Options", \
        LQT_PARAMETER_SECTION \
        }, \
        { \
        "flags_h263p_aic", \
        "Advanced intra coding", \
        LQT_PARAMETER_INT, \
        { 0 }, \
        0, \
        1 \
        }, \
        { \
        "flags_h263p_umv", \
        "Unlimited Motion Vector", \
        LQT_PARAMETER_INT, \
        { 0 }, \
        0, \
        1 \
        }

#define DECODE_PARAM_AUDIO

#define DECODE_PARAM_VIDEO \
        {\
	  "workaround_bugs",\
	  "Enable bug worarounds",\
	  LQT_PARAMETER_INT,\
	  { 1 },\
	  0,\
	  1,\
	  (char**)0\
	},\
        {\
          "flags_gray",\
          "Gray scale only mode",\
          LQT_PARAMETER_INT,\
          { 0 },\
          0,\
          1,\
          (char**)0\
        }, \
        { \
          "error_resilience", \
          "Error resilience", \
          LQT_PARAMETER_STRINGLIST, \
          { val_string: "Careful" }, \
          0, \
          0, \
          (char *[]){"None", "Careful", "Compilant", "Agressive", "Very Agressive", (char *)0 } \
        }

static lqt_parameter_info_static_t encode_parameters_video[] = {
  ENCODE_PARAM_VIDEO_GENERAL,
  ENCODE_PARAM_VIDEO_BITRATE,
  ENCODE_PARAM_VIDEO_VBR,
  { /* End of parameters */ }
};

static lqt_parameter_info_static_t encode_parameters_mpegvideo[] = {
  ENCODE_PARAM_VIDEO_GENERAL,
  ENCODE_PARAM_VIDEO_BITRATE,
  ENCODE_PARAM_VIDEO_VBR,
  ENCODE_PARAM_VIDEO_TEMPORAL,
  { /* End of parameters */ }
};

static lqt_parameter_info_static_t encode_parameters_mpeg4[] = {
  ENCODE_PARAM_VIDEO_GENERAL,
  ENCODE_PARAM_VIDEO_BITRATE,
  ENCODE_PARAM_VIDEO_VBR,
  ENCODE_PARAM_VIDEO_TEMPORAL,
  ENCODE_PARAM_VIDEO_MPEG4,
  { /* End of parameters */ }
};

static lqt_parameter_info_static_t encode_parameters_h263p[] = {
  ENCODE_PARAM_VIDEO_GENERAL,
  ENCODE_PARAM_VIDEO_BITRATE,
  ENCODE_PARAM_VIDEO_VBR,
  ENCODE_PARAM_VIDEO_TEMPORAL,
  ENCODE_PARAM_VIDEO_H263P,
  { /* End of parameters */ }
};


static lqt_parameter_info_static_t encode_parameters_audio[] = {
  ENCODE_PARAM_AUDIO,
  { /* End of parameters */ }
};

static lqt_parameter_info_static_t decode_parameters_video[] = {
  DECODE_PARAM_VIDEO,
  { /* End of parameters */ }
};

static lqt_parameter_info_static_t decode_parameters_mpeg4[] = {
  DECODE_PARAM_VIDEO,
  { /* End of parameters */ }
};
#if 0
static lqt_parameter_info_static_t decode_parameters_rle[] = {
  DECODE_PARAM_VIDEO,
  { /* End of parameters */ }
};
#endif

static lqt_parameter_info_static_t decode_parameters_audio[] = {
  //  DECODE_PARAM_AUDIO,
  { /* End of parameters */ }
};

struct CODECIDMAP {
	int id;
	int index;
	AVCodec *encoder;
	AVCodec *decoder;
        lqt_parameter_info_static_t * encode_parameters;
        lqt_parameter_info_static_t * decode_parameters;
	char *short_name;
	char *name;
	char *fourccs[MAX_FOURCCS];
        int   wav_ids[MAX_WAV_IDS];
        int * encoding_colormodels;
/*
 *  We explicitely allow, if encoding is allowed.
 *  This prevents the spreading of broken files
 */
        int   do_encode; 
  };

struct CODECIDMAP codecidmap_v[] = {
/* Tables from mplayers config... */
/* Video */
        {
          id: CODEC_ID_MPEG1VIDEO,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_mpegvideo,
          decode_parameters: decode_parameters_video,
	  short_name: "mpg1",
	  name: "Mpeg 1 Video",
	  fourccs: {"mpg1", "MPG1", "pim1", "PIM1", (char *)0} },
        {
          id: CODEC_ID_MPEG4,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_mpeg4,
          decode_parameters: decode_parameters_mpeg4,
	  short_name: "mpg4",
	  name: "Mpeg 4 Video (DivX)",
	  fourccs: {"mp4v", "divx", "DIV1", "div1", "MP4S", "mp4s", "M4S2",
                    "m4s2", "xvid", "XVID", "XviD", "DX50", "dx50", "DIVX",
                    "MP4V", (char *)0 },
          do_encode: 1 },
	{
          id: CODEC_ID_MSMPEG4V1,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_mpeg4,
          decode_parameters: decode_parameters_mpeg4,
	  short_name: "msmpeg4v1",
	  name: "MSMpeg 4v1",
	  fourccs: {"DIV1", "div1", "MPG4", "mpg4", (char *)0} },
	{
          id: CODEC_ID_MSMPEG4V2,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_mpeg4,
          decode_parameters: decode_parameters_mpeg4,
	  short_name: "msmpeg4v2",
	  name: "MSMpeg 4v2",
	  fourccs: {"DIV2", "div2", "MP42", "mp42", (char *)0} },
	{
          id: CODEC_ID_MSMPEG4V3,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_mpeg4,
          decode_parameters: decode_parameters_mpeg4,
	  short_name: "msmpeg4v3",
	  name: "MSMpeg 4v3",
	  fourccs: {"DIV3", "mpg3", "MP43", "mp43", "DIV5", "div5", "DIV6",
                    "MPG3", "div6", "div3", "DIV4", "div4", "AP41", "ap41",
                    (char *)0},
          do_encode: 1,
        },
#if 0
	{
          id: CODEC_ID_WMV1,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_mpegvideo,
          decode_parameters: decode_parameters_video,
	  short_name: "wmv1",
	  name: "WMV1",
	  fourccs: {"WMV1", "wmv1", (char *)0} },
#endif
	{
          id: CODEC_ID_H263,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_mpegvideo,
          decode_parameters: decode_parameters_video,
	  short_name: "h263",
	  name: "H263",
	  fourccs: {"H263", "h263", "U263", "u263", (char *)0},
          do_encode: 1, },
	{
          id: CODEC_ID_H263P,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_h263p,
          decode_parameters: decode_parameters_video,
	  short_name: "h263p",
	  name: "H263+",
	  fourccs: {"U263", "u263", (char *)0},
          do_encode: 1, },
	{
          id: CODEC_ID_H263I,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_mpegvideo,
          decode_parameters: decode_parameters_video,
	  short_name: "i263",
	  name: "I263",
	  fourccs: {"I263", "i263", "viv1", "VIV1", (char *)0},
        },
#if 0
	{
          id: CODEC_ID_RV10,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_mpegvideo,
          decode_parameters: decode_parameters_video,
	  short_name: "rv10",
	  name: "Real Video 10",
	  fourccs: {"RV10", "rv10", "RV13", "rv13", (char *)0} },
#endif
	{
          id: CODEC_ID_SVQ1,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_mpegvideo,
          decode_parameters: decode_parameters_video,
	  short_name: "svq1",
	  name: "Sorenson Video 1",
	  fourccs: {"SVQ1", (char *)0} },
	{
          id: CODEC_ID_SVQ3,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_mpegvideo,
          decode_parameters: decode_parameters_video,
	  short_name: "svq3",
	  name: "Sorenson Video 3",
	  fourccs: {"SVQ3", (char *)0} },
	{
          id: CODEC_ID_MJPEG,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "mjpg",
	  name: "MJPEG",
	  fourccs: {"MJPG", "mjpg", "JPEG", "jpeg", "dmb1", "AVDJ", (char *)0},
          do_encode: 1, },
	{
          id: CODEC_ID_8BPS,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "8BPS",
	  name: "Quicktime Planar RGB (8BPS)",
	  fourccs: {"8BPS", (char *)0} },
	{
          id: CODEC_ID_INDEO3,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "indeo",
	  name: "Intel Indeo 3",
	  fourccs: {"IV31", "IV32", (char *)0} },
	{
          id: CODEC_ID_RPZA,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "rpza",
	  name: "Apple Video",
	  fourccs: {"rpza", (char *)0} },
	{
          id: CODEC_ID_SMC,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "smc",
	  name: "Apple Graphics",
	  fourccs: {"smc ", (char *)0} },
	{
          id: CODEC_ID_CINEPAK,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "cinepak",
	  name: "Cinepak",
	  fourccs: {"cvid", (char *)0} },
	{
          id: CODEC_ID_CYUV,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "cyuv",
	  name: "Creative YUV",
	  fourccs: {"CYUV", (char *)0} },
	{
          id: CODEC_ID_QTRLE,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "rle",
	  name: "RLE",
	  fourccs: {"rle ", (char *)0} },
        {
          id: CODEC_ID_MSRLE,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          //          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "wrle",
	  name: "Microsoft RLE",
	  fourccs: {"WRLE", (char *)0} },
        {
          id: CODEC_ID_DVVIDEO,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "dv_ntsc",
	  name: "DV (NTSC)",
	  fourccs: {"dvc ", (char *)0 },
          do_encode: 1 },
        {
          id: CODEC_ID_DVVIDEO,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_video,
          decode_parameters: decode_parameters_video,
	  short_name: "dv_pal",
	  name: "DV (PAL)",
	  fourccs: {"dvcp", "dvpp", (char *)0 },
          do_encode: 1 },
};

struct CODECIDMAP codecidmap_a[] = {
         /* Audio */
        {
          id: CODEC_ID_MP3,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          decode_parameters: decode_parameters_audio,
	  short_name: "mp3",
	  name: "MPEG-1/2 audio layer 1/2/3",
	  fourccs: {".mp2", ".MP2", "ms\0\x50", "MS\0\x50",".mp3", ".MP3", "ms\0\x55", "MS\0\x55", (char *)0},
          wav_ids: { 0x50, 0x55, LQT_WAV_ID_NONE },
        },
	{
          id: CODEC_ID_MP2,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_audio,
          decode_parameters: decode_parameters_audio,
	  short_name: "mp2",
	  name: "Mpeg Layer 2 Audio",
	  fourccs: {".mp2", ".MP2", (char *)0 },
          wav_ids: { 0x55, LQT_WAV_ID_NONE },
          do_encode: 1,
        },
	{
          id: CODEC_ID_AC3,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          encode_parameters: encode_parameters_audio,
          decode_parameters: decode_parameters_audio,
	  short_name: "ac3",
	  name: "AC3 Audio",
	  fourccs: {".ac3", ".AC3", (char *)0},
          wav_ids: { 0x2000, LQT_WAV_ID_NONE },
          do_encode: 1,
        },
#if 0 /* Sounds ugly */
	{
          id: CODEC_ID_ADPCM_MS,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          short_name: "adpcm (ms)",
	  name: "McRowsoft ADPCM",
	  fourccs: {"ms\0\x02", "MS\0\x02", (char*)0},
          wav_ids: { 0x02, LQT_WAV_ID_NONE },
        },
#endif
#if 0 /* Sounds ugly */
        {
          id: CODEC_ID_ADPCM_IMA_WAV,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          short_name: "ima adpcm (wav)",
	  name: "ADPCM ima WAV",
	  fourccs: {"ms\0\x11", "MS\0\x11", (char*)0},
          wav_ids: { 0x11, LQT_WAV_ID_NONE },
          
        },
#endif
#if 0 /* Not needed anymore, we have a native codec */
	{
          id: CODEC_ID_PCM_ALAW,
	  index: -1,
          encoder: NULL,
          decoder: NULL,
          short_name: "alaw",
	  name: "Alaw",
	  fourccs: { "alaw", (char*)0},
          wav_ids: { 0x06, LQT_WAV_ID_NONE },
        },
#endif
};


#define NUMMAPS_A ((int)(sizeof(codecidmap_a)/sizeof(struct CODECIDMAP)))
#define NUMMAPS_V ((int)(sizeof(codecidmap_v)/sizeof(struct CODECIDMAP)))

// static void ffmpeg_map_init(void) __attribute__ ((constructor));

static void ffmpeg_map_init(void)
  {
  int i;
  //  fprintf(stderr, "Init map...");
  if(ffmpeg_num_video_codecs >= 0)
    {
    //    fprintf(stderr, "already initialized\n");
    return;
    }
  //  else
  //    fprintf(stderr, "initializing\n");
    
  avcodec_register_all();
  avcodec_init();
  ffmpeg_num_video_codecs = 0;
  ffmpeg_num_audio_codecs = 0;
  
  for(i = 0; i < NUMMAPS_V; i++)
    {
    if(codecidmap_v[i].do_encode)
      codecidmap_v[i].encoder = avcodec_find_encoder(codecidmap_v[i].id);
    codecidmap_v[i].decoder = avcodec_find_decoder(codecidmap_v[i].id);

    if(codecidmap_v[i].encoder || codecidmap_v[i].decoder)
      {
      codecidmap_v[i].index = ffmpeg_num_video_codecs++;
#if 0
      fprintf(stderr, "Found video codec %s %p %p %d %s\n",
              codecidmap_v[i].name, codecidmap_v[i].encoder, codecidmap_v[i].decoder,
              codecidmap_v[i].index,
              (codecidmap_v[i].encoder?codecidmap_v[i].encoder->name:codecidmap_v[i].decoder->name));
#endif
      }
    }
  for(i = 0; i < NUMMAPS_A; i++)
    {
    if(codecidmap_a[i].do_encode)
      codecidmap_a[i].encoder = avcodec_find_encoder(codecidmap_a[i].id);
    codecidmap_a[i].decoder = avcodec_find_decoder(codecidmap_a[i].id);

    if(codecidmap_a[i].encoder || codecidmap_a[i].decoder)
      {
      codecidmap_a[i].index = ffmpeg_num_audio_codecs++ + ffmpeg_num_video_codecs;
#if 0
      fprintf(stderr, "Found audio codec %s %p %p %d %s\n",
              codecidmap_a[i].name, codecidmap_a[i].encoder, codecidmap_a[i].decoder,
              codecidmap_a[i].index,
              (codecidmap_a[i].encoder?codecidmap_a[i].encoder->name:codecidmap_a[i].decoder->name));
#endif
      }
    }
  }

/* Template */

static char ffmpeg_name[256];
static char ffmpeg_long_name[256];
static char ffmpeg_description[256];

static lqt_codec_info_static_t codec_info_ffmpeg = {
	name:        ffmpeg_name,
	long_name:   ffmpeg_long_name,
	description: ffmpeg_description,
	fourccs:     NULL,
        wav_ids:     NULL,
	type:        0,
	direction:   0,
	encoding_parameters: NULL,
	decoding_parameters: NULL,
};

/* These are called from the plugin loader */

extern int get_num_codecs()
{
//        fprintf(stderr, "Get num codecs\n");
	ffmpeg_map_init();
	return ffmpeg_num_video_codecs + ffmpeg_num_audio_codecs;
}

static void set_codec_info(struct CODECIDMAP * map)
  {
  // char * capabilities = (char*)0;

  codec_info_ffmpeg.fourccs = map->fourccs;
  codec_info_ffmpeg.wav_ids = map->wav_ids;

  if(map->encoder && map->decoder)
    {
    codec_info_ffmpeg.direction = LQT_DIRECTION_BOTH;
    codec_info_ffmpeg.encoding_parameters = map->encode_parameters;
    codec_info_ffmpeg.decoding_parameters = map->decode_parameters;
    //    capabilities = "Codec";
    }
  else if(map->encoder)
    {
    codec_info_ffmpeg.direction = LQT_DIRECTION_ENCODE;
    codec_info_ffmpeg.encoding_parameters = map->encode_parameters;
    codec_info_ffmpeg.decoding_parameters = NULL;
      
    //    capabilities = "Encoder";
    }
  else if(map->decoder)
    {
    codec_info_ffmpeg.direction = LQT_DIRECTION_DECODE;
    codec_info_ffmpeg.encoding_parameters = NULL;
    codec_info_ffmpeg.decoding_parameters = map->decode_parameters;
    //    capabilities = "Decoder";
    }

  snprintf(ffmpeg_name, 256, "ffmpeg_%s", map->short_name);
  //  snprintf(ffmpeg_long_name, 50, "FFMPEG %s %s", map->name, capabilities);
  //  snprintf(ffmpeg_description, 100, "FFMPEG %s %s", map->name, capabilities);

  snprintf(ffmpeg_long_name, 256, "FFMPEG %s", map->name);
  snprintf(ffmpeg_description, 256, "FFMPEG %s", map->name);

  
  if((map->encoder && (map->encoder->type == CODEC_TYPE_VIDEO)) ||
     (map->decoder && (map->decoder->type == CODEC_TYPE_VIDEO))){
       codec_info_ffmpeg.type = LQT_CODEC_VIDEO;
  } else {
       codec_info_ffmpeg.type = LQT_CODEC_AUDIO;
  }
  }

static struct CODECIDMAP * find_codec(int index)
  {
  int i;
  //  fprintf(stderr, "Find codec\n");
  for(i = 0; i < NUMMAPS_V; i++)
    {
    if(codecidmap_v[i].index == index)
      return &codecidmap_v[i];
    }
  for(i = 0; i < NUMMAPS_A; i++)
    {
    if(codecidmap_a[i].index == index)
      return &codecidmap_a[i];
    }
  return (struct CODECIDMAP *)0;
  }

extern lqt_codec_info_static_t * get_codec_info(int index)
{
	struct CODECIDMAP * map;
        ffmpeg_map_init();
	map = find_codec(index);

        if(map)
          {
          set_codec_info(map);
          return &codec_info_ffmpeg;
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
void quicktime_init_video_codec_ffmpeg ## x(quicktime_video_map_t *vtrack) \
{ \
	int i; \
	for(i = 0; i < ffmpeg_num_video_codecs; i++) { \
		if(codecidmap_v[i].index == x) { \
                     quicktime_init_video_codec_ffmpeg(vtrack,       \
				codecidmap_v[i].encoder, \
				codecidmap_v[i].decoder); \
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
#undef IFUNC
#define MAX_VIDEO_FUNC 29
#define IFUNC(x) \
void quicktime_init_audio_codec_ffmpeg ## x(quicktime_audio_map_t *atrack) \
{ \
	int i; \
	for(i = 0; i < ffmpeg_num_audio_codecs; i++) { \
		if(codecidmap_a[i].index == x) { \
                      quicktime_init_audio_codec_ffmpeg(atrack,       \
				codecidmap_a[i].encoder, \
				codecidmap_a[i].decoder); \
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
#define MAX_AUDIO_FUNC 29

     
#undef IFUNC

extern lqt_init_video_codec_func_t get_video_codec(int index)
{
//        fprintf(stderr, "Get video codec %d\n", index);
	ffmpeg_map_init();
	if(index > MAX_VIDEO_FUNC) {
		fprintf(stderr, "lqt_ffmpeg error: Insufficient dummy calls - please report!\n");
		return NULL;
	}
	switch(index) {
		case 0: return quicktime_init_video_codec_ffmpeg0;
		case 1: return quicktime_init_video_codec_ffmpeg1;
		case 2:	return quicktime_init_video_codec_ffmpeg2;
		case 3:	return quicktime_init_video_codec_ffmpeg3;
		case 4:	return quicktime_init_video_codec_ffmpeg4;
		case 5:	return quicktime_init_video_codec_ffmpeg5;
		case 6:	return quicktime_init_video_codec_ffmpeg6;
		case 7:	return quicktime_init_video_codec_ffmpeg7;
		case 8:	return quicktime_init_video_codec_ffmpeg8;
		case 9:	return quicktime_init_video_codec_ffmpeg9;
		case 10: return quicktime_init_video_codec_ffmpeg10;
		case 11: return quicktime_init_video_codec_ffmpeg11;
		case 12: return quicktime_init_video_codec_ffmpeg12;
		case 13: return quicktime_init_video_codec_ffmpeg13;
		case 14: return quicktime_init_video_codec_ffmpeg14;
		case 15: return quicktime_init_video_codec_ffmpeg15;
		case 16: return quicktime_init_video_codec_ffmpeg16;
		case 17: return quicktime_init_video_codec_ffmpeg17;
		case 18: return quicktime_init_video_codec_ffmpeg18;
		case 19: return quicktime_init_video_codec_ffmpeg19;
		case 20: return quicktime_init_video_codec_ffmpeg20;
		case 21: return quicktime_init_video_codec_ffmpeg21;
		case 22: return quicktime_init_video_codec_ffmpeg22;
		case 23: return quicktime_init_video_codec_ffmpeg23;
		case 24: return quicktime_init_video_codec_ffmpeg24;
		case 25: return quicktime_init_video_codec_ffmpeg25;
		case 26: return quicktime_init_video_codec_ffmpeg26;
		case 27: return quicktime_init_video_codec_ffmpeg27;
		case 28: return quicktime_init_video_codec_ffmpeg28;
		case 29: return quicktime_init_video_codec_ffmpeg29;
		default:
			break;
	}
	return (lqt_init_video_codec_func_t)0;
}

extern lqt_init_audio_codec_func_t get_audio_codec(int index)
{
//        fprintf(stderr, "Get audio codec %d\n", index);
	ffmpeg_map_init();
	if(index > MAX_AUDIO_FUNC) {
		fprintf(stderr, "lqt_ffmpeg error: Insufficient dummy calls - please report!\n");
		return NULL;
	}
	switch(index) {
		case 0: return quicktime_init_audio_codec_ffmpeg0;
		case 1: return quicktime_init_audio_codec_ffmpeg1;
		case 2:	return quicktime_init_audio_codec_ffmpeg2;
		case 3:	return quicktime_init_audio_codec_ffmpeg3;
		case 4:	return quicktime_init_audio_codec_ffmpeg4;
		case 5:	return quicktime_init_audio_codec_ffmpeg5;
		case 6:	return quicktime_init_audio_codec_ffmpeg6;
		case 7:	return quicktime_init_audio_codec_ffmpeg7;
		case 8:	return quicktime_init_audio_codec_ffmpeg8;
		case 9:	return quicktime_init_audio_codec_ffmpeg9;
		case 10: return quicktime_init_audio_codec_ffmpeg10;
		case 11: return quicktime_init_audio_codec_ffmpeg11;
		case 12: return quicktime_init_audio_codec_ffmpeg12;
		case 13: return quicktime_init_audio_codec_ffmpeg13;
		case 14: return quicktime_init_audio_codec_ffmpeg14;
		case 15: return quicktime_init_audio_codec_ffmpeg15;
		case 16: return quicktime_init_audio_codec_ffmpeg16;
		case 17: return quicktime_init_audio_codec_ffmpeg17;
		case 18: return quicktime_init_audio_codec_ffmpeg18;
		case 19: return quicktime_init_audio_codec_ffmpeg19;
		case 20: return quicktime_init_audio_codec_ffmpeg20;
		case 21: return quicktime_init_audio_codec_ffmpeg21;
		case 22: return quicktime_init_audio_codec_ffmpeg22;
		case 23: return quicktime_init_audio_codec_ffmpeg23;
		case 24: return quicktime_init_audio_codec_ffmpeg24;
		case 25: return quicktime_init_audio_codec_ffmpeg25;
		case 26: return quicktime_init_audio_codec_ffmpeg26;
		case 27: return quicktime_init_audio_codec_ffmpeg27;
		case 28: return quicktime_init_audio_codec_ffmpeg28;
		case 29: return quicktime_init_audio_codec_ffmpeg29;
		default:
			break;
	}
	return (lqt_init_audio_codec_func_t)0;
}
