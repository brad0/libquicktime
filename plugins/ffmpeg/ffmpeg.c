/* 
   ffmpeg.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
   Based entirely upon qtpng.c from libquicktime 
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

#include <string.h>
#include <quicktime/colormodels.h>
#include <funcprotos.h>
#include "ffmpeg.h"

static int delete_acodec(quicktime_audio_map_t *vtrack)
  {
  quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  
  if(codec->init_enc)
    avcodec_close(&codec->ffcodec_enc);
  if(codec->init_dec)
    avcodec_close(&codec->ffcodec_dec);
  
  if(codec->encode_frame) free(codec->encode_frame);
  if(codec->write_buffer) free(codec->write_buffer);
  
  if(codec->read_buffer) free(codec->read_buffer);
  
  if(codec->sample_buffer) free(codec->sample_buffer);
  
  free(codec);
  return 0;
  }

static int delete_vcodec(quicktime_video_map_t *vtrack)
{
	quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	
	if(codec->init_enc)
		avcodec_close(&codec->ffcodec_enc);
	if(codec->init_dec)
		avcodec_close(&codec->ffcodec_dec);

	if(codec->encode_frame) free(codec->encode_frame);
	if(codec->write_buffer) free(codec->write_buffer);

	if(codec->read_buffer) free(codec->read_buffer);
	
	free(codec);
	
	return 0;
}


static int set_parameter_video(quicktime_t *file, 
                               int track, 
                               char *key, 
                               void *value)
{
	quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;

        
#define INTPARM(x, y) \
	if(!strcasecmp(key, #x)) { \
          { \
          codec->params.x = (*(int *)value) * y; \
          } \
	}
	
	INTPARM(bit_rate, 1000) else 
	INTPARM(bit_rate_tolerance, 1) else
	INTPARM(gop_size, 1) else
	INTPARM(quality, 1) else
	INTPARM(qcompress, 0.01) else
	INTPARM(qblur, 0.01) else
	INTPARM(qmin, 1) else
	INTPARM(qmax, 1) else
	INTPARM(max_qdiff, 1) else
	INTPARM(max_b_frames, 1) else
	INTPARM(b_quant_factor, 1) else
	INTPARM(b_quant_offset, 1) else
	INTPARM(rc_strategy, 1) else
	INTPARM(b_frame_strategy, 1) else
	INTPARM(rtp_payload_size, 1) else
	INTPARM(workaround_bugs, 1) else
	INTPARM(luma_elim_threshold, 1) else
	INTPARM(chroma_elim_threshold, 1) else
	INTPARM(strict_std_compliance, 1) else
	INTPARM(error_resilience, 1) else
	if(!strcasecmp(key, "flags_hq")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_HQ;
		else
			codec->params.flags &= ~CODEC_FLAG_HQ;
	} else
	if(!strcasecmp(key, "flags_4mv")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_4MV;
		else
			codec->params.flags &= ~CODEC_FLAG_4MV;
	} else
	if(!strcasecmp(key, "flags_part")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_PART;
		else
			codec->params.flags &= ~CODEC_FLAG_PART;
	} else
	if(!strcasecmp(key, "flags_gray")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_GRAY;
		else
			codec->params.flags &= ~CODEC_FLAG_GRAY;
	} else
	if(!strcasecmp(key, "flags_fix")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_QSCALE;
		else
			codec->params.flags &= ~CODEC_FLAG_QSCALE;
	} else
	if(!strcasecmp(key, "flags_pass")) {
		codec->params.flags &= ~(CODEC_FLAG_PASS1 | CODEC_FLAG_PASS2);
		if(*(int *)value == 1) {
			codec->params.flags |= CODEC_FLAG_PASS1;
		} else if(*(int *)value == 2) {
			codec->params.flags |= CODEC_FLAG_PASS1;
		}
	} else
	if(!strcasecmp(key, "me_method")) {
		if(!strcasecmp((char *)value, "Zero")) {
			codec->params.me_method = ME_ZERO;
		} else if(!strcasecmp((char *)value, "Full")) {
			codec->params.me_method = ME_FULL;
		} else if(!strcasecmp((char *)value, "Log")) {
			codec->params.me_method = ME_LOG;
		} else if(!strcasecmp((char *)value, "Phods")) {
			codec->params.me_method = ME_PHODS;
		} else if(!strcasecmp((char *)value, "Epzs")) {
			codec->params.me_method = ME_EPZS;
		} else if(!strcasecmp((char *)value, "X1")) {
			codec->params.me_method = ME_X1;
		}
	} else
	if(!strcasecmp(key, "aspect_ratio_info")) {
		if(!strcasecmp((char *)value, "Square")) {
			codec->params.aspect_ratio_info = FF_ASPECT_SQUARE;
		} else if(!strcasecmp((char *)value, "4:3 (625)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_4_3_625;
		} else if(!strcasecmp((char *)value, "4:3 (525)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_4_3_525;
		} else if(!strcasecmp((char *)value, "16:9 (625)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_16_9_625;
		} else if(!strcasecmp((char *)value, "16:9 (525)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_16_9_525;
		}
	} else
	{
		fprintf(stderr, "Unknown key: %s\n", key);
		return -1;
	}
	return 0;
}

static int set_parameter_audio(quicktime_t *file, 
                               int track, 
                               char *key, 
                               void *value)
{
	quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;

        
#define INTPARM(x, y) \
	if(!strcasecmp(key, #x)) { \
          { \
          codec->params.x = (*(int *)value) * y; \
          } \
	}
	
	INTPARM(bit_rate, 1000) else 
	INTPARM(bit_rate_tolerance, 1) else
	INTPARM(gop_size, 1) else
	INTPARM(quality, 1) else
	INTPARM(qcompress, 0.01) else
	INTPARM(qblur, 0.01) else
	INTPARM(qmin, 1) else
	INTPARM(qmax, 1) else
	INTPARM(max_qdiff, 1) else
	INTPARM(max_b_frames, 1) else
	INTPARM(b_quant_factor, 1) else
	INTPARM(b_quant_offset, 1) else
	INTPARM(rc_strategy, 1) else
	INTPARM(b_frame_strategy, 1) else
	INTPARM(rtp_payload_size, 1) else
	INTPARM(workaround_bugs, 1) else
	INTPARM(luma_elim_threshold, 1) else
	INTPARM(chroma_elim_threshold, 1) else
	INTPARM(strict_std_compliance, 1) else
	INTPARM(error_resilience, 1) else
	if(!strcasecmp(key, "flags_hq")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_HQ;
		else
			codec->params.flags &= ~CODEC_FLAG_HQ;
	} else
	if(!strcasecmp(key, "flags_4mv")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_4MV;
		else
			codec->params.flags &= ~CODEC_FLAG_4MV;
	} else
	if(!strcasecmp(key, "flags_part")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_PART;
		else
			codec->params.flags &= ~CODEC_FLAG_PART;
	} else
	if(!strcasecmp(key, "flags_gray")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_GRAY;
		else
			codec->params.flags &= ~CODEC_FLAG_GRAY;
	} else
	if(!strcasecmp(key, "flags_fix")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_QSCALE;
		else
			codec->params.flags &= ~CODEC_FLAG_QSCALE;
	} else
	if(!strcasecmp(key, "flags_pass")) {
		codec->params.flags &= ~(CODEC_FLAG_PASS1 | CODEC_FLAG_PASS2);
		if(*(int *)value == 1) {
			codec->params.flags |= CODEC_FLAG_PASS1;
		} else if(*(int *)value == 2) {
			codec->params.flags |= CODEC_FLAG_PASS1;
		}
	} else
	if(!strcasecmp(key, "me_method")) {
		if(!strcasecmp((char *)value, "Zero")) {
			codec->params.me_method = ME_ZERO;
		} else if(!strcasecmp((char *)value, "Full")) {
			codec->params.me_method = ME_FULL;
		} else if(!strcasecmp((char *)value, "Log")) {
			codec->params.me_method = ME_LOG;
		} else if(!strcasecmp((char *)value, "Phods")) {
			codec->params.me_method = ME_PHODS;
		} else if(!strcasecmp((char *)value, "Epzs")) {
			codec->params.me_method = ME_EPZS;
		} else if(!strcasecmp((char *)value, "X1")) {
			codec->params.me_method = ME_X1;
		}
	} else
	if(!strcasecmp(key, "aspect_ratio_info")) {
		if(!strcasecmp((char *)value, "Square")) {
			codec->params.aspect_ratio_info = FF_ASPECT_SQUARE;
		} else if(!strcasecmp((char *)value, "4:3 (625)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_4_3_625;
		} else if(!strcasecmp((char *)value, "4:3 (525)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_4_3_525;
		} else if(!strcasecmp((char *)value, "16:9 (625)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_16_9_625;
		} else if(!strcasecmp((char *)value, "16:9 (525)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_16_9_525;
		}
	} else
	{
		fprintf(stderr, "Unknown key: %s\n", key);
		return -1;
	}
	return 0;
}


static int reads_colormodel(quicktime_t *file, 
                            int colormodel, 
                            int track)
{
	return (colormodel == BC_YUV420P);
}

static int writes_colormodel(quicktime_t *file, 
                             int colormodel, 
                             int track)
{
	return (colormodel == BC_YUV420P);
}


void quicktime_init_video_codec_ffmpeg(quicktime_video_map_t *vtrack, AVCodec *encoder, AVCodec *decoder)
{
	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
	quicktime_ffmpeg_codec_t *codec;

	avcodec_init();

	codec = calloc(1, sizeof(quicktime_ffmpeg_codec_t));
	if(!codec)
	  return;

	codec->ffc_enc = encoder;
	codec->ffc_dec = decoder;
	
	((quicktime_codec_t*)vtrack->codec)->priv = (void *)codec;
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_vcodec;

        if(encoder)
          ((quicktime_codec_t*)vtrack->codec)->encode_video = lqt_ffmpeg_encode_video;
	if(decoder)
          ((quicktime_codec_t*)vtrack->codec)->decode_video = lqt_ffmpeg_decode_video;

        ((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter_video;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = reads_colormodel;
	((quicktime_codec_t*)vtrack->codec)->writes_colormodel = writes_colormodel;
}

void quicktime_init_audio_codec_ffmpeg(quicktime_audio_map_t *atrack, AVCodec *encoder, AVCodec *decoder)
{
	char *compressor = atrack->track->mdia.minf.stbl.stsd.table[0].format;
	quicktime_ffmpeg_codec_t *codec;

	avcodec_init();

	codec = calloc(1, sizeof(quicktime_ffmpeg_codec_t));
	if(!codec)
          return;

	codec->ffc_enc = encoder;
	codec->ffc_dec = decoder;
	
	((quicktime_codec_t*)atrack->codec)->priv = (void *)codec;
	((quicktime_codec_t*)atrack->codec)->delete_acodec = delete_acodec;
	if(encoder)
          ((quicktime_codec_t*)atrack->codec)->encode_audio = lqt_ffmpeg_encode_audio;
	if(decoder)
          ((quicktime_codec_t*)atrack->codec)->decode_audio = lqt_ffmpeg_decode_audio;
	((quicktime_codec_t*)atrack->codec)->set_parameter = set_parameter_audio;
}
