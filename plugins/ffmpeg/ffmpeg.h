/* 
   ffmpeg.h (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
   Based entirely upon qtpng.h from libquicktime 
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

#ifndef QUICKTIME_FFMPEG_H
#define QUICKTIME_FFMPEG_H

#include "ffmpeg/avcodec.h"
#include <quicktime/quicktime.h>

typedef struct
  {
  AVCodecContext params;
  
  /* Compression stuff */
  AVCodecContext ffcodec_enc;
  AVCodec * ffc_enc;
  int init_enc;
  unsigned char * encode_frame;
  unsigned char * write_buffer;
  int write_buffer_size;
  
  /* DeCompression stuff */
  AVCodecContext ffcodec_dec;
  AVCodec * ffc_dec;
  int init_dec;
  unsigned char * read_buffer;
  int read_buffer_size;
  int64_t last_frame;
  
  /* Audio Sample buffer */
  
  short * sample_buffer;

  /* Encoding only */
  
  int sample_buffer_pos;

  /* Decoding only */
  
  int sample_buffer_size; 
  int samples_in_buffer;
  
  /* Index of the first sample in buffer relative to start of stream */
  
  int64_t sample_buffer_offset;

  /* Buffer for the entire chunk */

  char * chunk_buffer;
  int chunk_buffer_size;

  char * chunk_buffer_ptr;
  int chunk_size;
    
  /* Buffer for a single frame */
  
  char * frame_buffer;
  int frame_buffer_size;
  int next_frame_bytes;

  /* Decoded frame */

  uint16_t * decoded_frame;
  int decoded_frame_pos;

  int samples_per_frame;
  
  /* Remember some stuff */

  int64_t last_sample_start;
  int64_t last_sample_num;
    
  } quicktime_ffmpeg_codec_t;

#endif

void quicktime_init_video_codec_ffmpeg(quicktime_video_map_t *vtrack, AVCodec *encoder, AVCodec *decoder);
void quicktime_init_audio_codec_ffmpeg(quicktime_audio_map_t *vtrack, AVCodec *encoder, AVCodec *decoder);

int lqt_ffmpeg_decode_video(quicktime_t *file, unsigned char **row_pointers,
                            int track);

int lqt_ffmpeg_encode_video(quicktime_t *file, unsigned char **row_pointers,
                            int track);

int lqt_ffmpeg_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f, int track,
                            long samples);

int lqt_ffmpeg_decode_audio(quicktime_t *file, int16_t *output_i, float *output_f, long samples,
                            int track, int channel);


