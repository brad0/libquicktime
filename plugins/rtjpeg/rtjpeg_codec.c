/*******************************************************************************
 rtjpeg_codec.c

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
#include <quicktime/colormodels.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 16

static int delete_codec(quicktime_codec_t *codec_base)
  {
  quicktime_rtjpeg_codec_t *codec = codec_base->priv;
  if(codec->compress_struct)
    RTjpeg_close(codec->compress_struct);
  if(codec->rows)
    lqt_rows_free(codec->rows);
  if(codec->write_buffer)
    free(codec->write_buffer);

  if(codec->decompress_struct)
    RTjpeg_close(codec->decompress_struct);
  if(codec->read_buffer)
    free(codec->read_buffer);
  free(codec);
  return 0;
  }

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
  {
  int result = 0;
  int t;
  int buffer_size;
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  //	quicktime_trak_t *trak = vtrack->track;
  quicktime_rtjpeg_codec_t *codec = vtrack->codec->priv;

  if(!row_pointers)
    {
    vtrack->stream_cmodel = BC_YUV420P;
    return 0;
    }
        
  if(!codec->decompress_struct) {
  codec->decompress_struct = RTjpeg_init();
  if(!codec->decompress_struct)
    return -1;
  codec->qt_height = quicktime_video_height(file, track);
  codec->qt_width  = quicktime_video_width(file, track);
  codec->jpeg_height = BLOCK_SIZE * ((codec->qt_height + BLOCK_SIZE - 1) / (BLOCK_SIZE));
  codec->jpeg_width  = BLOCK_SIZE * ((codec->qt_width  + BLOCK_SIZE - 1) / (BLOCK_SIZE));
  t = RTJ_YUV420;
  RTjpeg_set_format(codec->decompress_struct, &t);

  codec->rows = lqt_rows_alloc(codec->jpeg_width, codec->jpeg_height,
                               vtrack->stream_cmodel, &codec->rowspan,
                               &codec->rowspan_uv);
                
  }

  buffer_size = lqt_read_video_frame(file, &codec->read_buffer,
                                     &codec->read_buffer_alloc,
                                     vtrack->current_position, NULL, track);
  if(buffer_size <= 0)
    result = -1;
        
  if(buffer_size > 0) {
  if(!result)
    RTjpeg_decompress(codec->decompress_struct, codec->read_buffer, codec->rows);
  }
        
  lqt_rows_copy(row_pointers, codec->rows, codec->qt_width, codec->qt_height,
                codec->rowspan, codec->rowspan_uv,
                vtrack->stream_row_span, vtrack->stream_row_span_uv, vtrack->stream_cmodel);

        
  return result;
  }

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
  {
  int result = 0;
  int i;
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  quicktime_trak_t *trak = vtrack->track;
  quicktime_rtjpeg_codec_t *codec = vtrack->codec->priv;

  if(!row_pointers)
    {
    vtrack->stream_cmodel = BC_YUV420P;
    return 0;
    }
        
  if(!codec->compress_struct) {
  codec->compress_struct = RTjpeg_init();
  if(!codec->compress_struct)
    return -1;
  codec->qt_height = trak->tkhd.track_height;
  codec->qt_width  = trak->tkhd.track_width;
  codec->jpeg_height = BLOCK_SIZE * ((codec->qt_height + BLOCK_SIZE - 1) / (BLOCK_SIZE));
  codec->jpeg_width  = BLOCK_SIZE * ((codec->qt_width  + BLOCK_SIZE - 1) / (BLOCK_SIZE));

  RTjpeg_set_size(codec->compress_struct, &codec->jpeg_width, &codec->jpeg_height);
  i = codec->Q;
  i *= 255;
  i /= 100;
  RTjpeg_set_quality(codec->compress_struct, &i);
  i = RTJ_YUV420;
  RTjpeg_set_format(codec->compress_struct, &i);
  RTjpeg_set_intra(codec->compress_struct, &codec->K, &codec->LQ, &codec->CQ);

  codec->rows = lqt_rows_alloc(codec->jpeg_width, codec->jpeg_height,
                               vtrack->stream_cmodel, &codec->rowspan,
                               &codec->rowspan_uv);
                
  codec->write_buffer = malloc(codec->jpeg_width * codec->jpeg_height * 3 / 2 + 100);
  if(!codec->write_buffer)
    return -1;
  }

  lqt_rows_copy(codec->rows, row_pointers, codec->qt_width, codec->qt_height,
                vtrack->stream_row_span, vtrack->stream_row_span_uv,
                codec->rowspan, codec->rowspan_uv,
                vtrack->stream_cmodel);
        
                
  i = RTjpeg_compress(codec->compress_struct, codec->write_buffer, codec->rows);

  lqt_write_frame_header(file, track,
                         vtrack->current_position,
                         -1, 0);
        
  result = !quicktime_write_data(file, 
                                 codec->write_buffer, 
                                 i);
  lqt_write_frame_footer(file, track);
  return result;
  }

static int set_parameter(quicktime_t *file, 
                         int track, 
                         const char *key, 
                         const void *value)
  {
  quicktime_rtjpeg_codec_t *codec = file->vtracks[track].codec->priv;

  if(!strcasecmp(key, "rtjpeg_quality"))
    codec->Q = *(int*)value;
  if(!strcasecmp(key, "rtjpeg_key_rate"))
    codec->K = *(int*)value;
  if(!strcasecmp(key, "rtjpeg_luma_quant"))
    codec->LQ = *(int*)value;
  if(!strcasecmp(key, "rtjpeg_chroma_quant"))
    codec->CQ = *(int*)value;
  return 0;
  }

void quicktime_init_codec_rtjpeg(quicktime_codec_t * codec_base,
                                 quicktime_audio_map_t *atrack,
                                 quicktime_video_map_t *vtrack)
  {
  quicktime_rtjpeg_codec_t *codec;

  codec = calloc(1, sizeof(*codec));
  if(!codec)
    return;
  codec->Q = 100;
  codec->K = 25;
  codec->LQ = 1;
  codec->CQ = 1;
	
  codec_base->priv = codec;
  codec_base->delete_codec = delete_codec;
  codec_base->decode_video = decode;
  codec_base->encode_video = encode;
  codec_base->set_parameter = set_parameter;
  }
