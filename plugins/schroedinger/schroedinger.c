/*******************************************************************************
 schroedinger.c

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

#include <string.h>

#define LQT_LIBQUICKTIME /* Hack: This prevents multiple compilation of
                            get_codec_api_version() */
#include "lqt_private.h"
#include "schroedinger.h"
#include <quicktime/colormodels.h>

/* Colormodel stuff */

typedef struct
  {
  int colormodel;
  SchroChromaFormat chroma_format;
  SchroFrameFormat  frame_format;
  SchroSignalRange  signal_range;
  int bits;
  } pixel_format_t;

static const pixel_format_t
pixel_format_map[] =
  {
    { BC_YUV420P, SCHRO_CHROMA_420, SCHRO_FRAME_FORMAT_U8_420, SCHRO_SIGNAL_RANGE_8BIT_VIDEO, 8 },
    { BC_YUV422P, SCHRO_CHROMA_422, SCHRO_FRAME_FORMAT_U8_422, SCHRO_SIGNAL_RANGE_8BIT_VIDEO, 8 },
    { BC_YUV444P, SCHRO_CHROMA_444, SCHRO_FRAME_FORMAT_U8_444, SCHRO_SIGNAL_RANGE_8BIT_VIDEO, 8 },
    { BC_YUVJ420P, SCHRO_CHROMA_420, SCHRO_FRAME_FORMAT_U8_420, SCHRO_SIGNAL_RANGE_8BIT_FULL, 8 },
    { BC_YUVJ422P, SCHRO_CHROMA_422, SCHRO_FRAME_FORMAT_U8_422, SCHRO_SIGNAL_RANGE_8BIT_FULL, 8 },
    { BC_YUVJ444P, SCHRO_CHROMA_444, SCHRO_FRAME_FORMAT_U8_444, SCHRO_SIGNAL_RANGE_8BIT_FULL, 8 },
  };

static const int num_pixel_formats = sizeof(pixel_format_map)/sizeof(pixel_format_map[0]);

static const pixel_format_t * pixelformat_from_schro(SchroVideoFormat *format)
  {
  int i;
  SchroSignalRange signal_range;
  
  signal_range = schro_video_format_get_std_signal_range(format);
  
  for(i = 0; i < num_pixel_formats; i++)
    {
    if((pixel_format_map[i].signal_range == signal_range) &&
       (pixel_format_map[i].chroma_format == format->chroma_format))
      return &pixel_format_map[i];
    }
  return NULL;
  }

static const pixel_format_t * pixelformat_from_lqt(int cmodel)
  {
  int i;
  for(i = 0; i < num_pixel_formats; i++)
    {
    if(pixel_format_map[i].colormodel == cmodel)
      return &pixel_format_map[i];
    }
  return NULL;
  }

SchroChromaFormat lqt_schrodinger_get_chroma_format(int cmodel)
  {
  const pixel_format_t * p;
  p = pixelformat_from_lqt(cmodel);
  if(p)
    return p->chroma_format;
  return 0;
  }

SchroSignalRange lqt_schrodinger_get_signal_range(int cmodel)
  {
  const pixel_format_t * p;
  p = pixelformat_from_lqt(cmodel);
  if(p)
    return p->signal_range;
  return 0;
  }


int lqt_schrodinger_get_colormodel(SchroVideoFormat *format)
  {
  const pixel_format_t * pfmt = pixelformat_from_schro(format);
  if(pfmt)
    return pfmt->colormodel;
  else
    return LQT_COLORMODEL_NONE;
  }

SchroFrameFormat
lqt_schrodinger_get_frame_format(SchroVideoFormat *format)
  {
  const pixel_format_t * pfmt = pixelformat_from_schro(format);
  if(pfmt)
    return pfmt->frame_format;
  else
    return 0;
  }

int lqt_schroedinger_delete(quicktime_codec_t *codec_base)
  {
  schroedinger_codec_t *codec = codec_base->priv;
  
  if(codec->dec)
    schro_decoder_free(codec->dec);
  if(codec->dec_buffer)
    free(codec->dec_buffer);
  if(codec->enc_buffer)
    free(codec->enc_buffer);
  
  if(codec->enc)
    schro_encoder_free(codec->enc);
    
  free(codec);
  return 0;
  }

static int set_parameter_schroedinger(quicktime_t *file, 
                               int track, 
                               const char *key, 
                               const void *value)
  {
  if(!strncmp(key, "enc_", 4))
    return lqt_schroedinger_set_enc_parameter(file, track, key, value);
  return 0;
  }

void quicktime_init_codec_schroedinger(quicktime_codec_t * codec_base,
                                       quicktime_audio_map_t *atrack,
                                       quicktime_video_map_t *vtrack)
  {
  schroedinger_codec_t *codec;
  
  // char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
  
  schro_init();

  codec = calloc(1, sizeof(*codec));
  if(!codec)
    return;

  /* Need to create the encoder immediately */
  codec->enc = schro_encoder_new();
  
  codec_base->priv = (void *)codec;
  codec_base->delete_codec = lqt_schroedinger_delete;
  codec_base->flush = lqt_schroedinger_flush;
  codec_base->resync = lqt_schroedinger_resync;
  
  codec_base->encode_video = lqt_schroedinger_encode_video;
  
  codec_base->decode_video = lqt_schroedinger_decode_video;
  codec_base->set_parameter = set_parameter_schroedinger;
  
  }

