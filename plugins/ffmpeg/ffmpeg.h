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

#include <ffmpeg/avcodec.h>
#include <quicktime/quicktime.h>
#include <quicktime/lqt.h>

typedef struct
  {
  AVCodecContext params;
  
  /* Compression stuff */
  AVCodecContext * ffcodec_enc;
  AVCodec * ffc_enc;
  int init_enc;

  /* DeCompression stuff */
  AVCodecContext * ffcodec_dec;
  AVCodec * ffc_dec;
  int init_dec;
  int bitrate;
  } quicktime_ffmpeg_codec_common_t;

typedef struct
  {
  quicktime_ffmpeg_codec_common_t com;
  int64_t current_chunk;

  /* Interleaved samples as avcodec needs them */
    
  int16_t * sample_buffer;
  int sample_buffer_size; 
  int samples_in_buffer;

  /* Buffer for the entire chunk */

  char * chunk_buffer;
  int chunk_buffer_size;
  int bytes_in_chunk_buffer;
    
  /* Decoder specific stuff */
  
  int64_t * chunk_sizes;
  int64_t current_position; /* Start of sample buffer */
  int num_samples;          /* Number of samples decoded the last time */
  
  } quicktime_ffmpeg_audio_codec_t;

typedef struct
  {
  quicktime_ffmpeg_codec_common_t com;

  unsigned char * encode_buffer;
  unsigned char * write_buffer;
  int write_buffer_size;
  
  unsigned char * read_buffer;
  int read_buffer_size;
  int64_t last_frame;

  AVFrame * frame;
  uint8_t * frame_buffer;

  /* Colormodel */

  int lqt_colormodel;
  int do_imgconvert;
    
  unsigned char ** tmp_buffer;
  unsigned char ** row_pointers;
    
  /* Quality must be passed to the individual frame */

  int qscale;

  AVPaletteControl palette;
  } quicktime_ffmpeg_video_codec_t;



void quicktime_init_video_codec_ffmpeg(quicktime_video_map_t *vtrack,
                                       AVCodec *encoder, AVCodec *decoder);
void quicktime_init_audio_codec_ffmpeg(quicktime_audio_map_t *vtrack,
                                       AVCodec *encoder, AVCodec *decoder);

int lqt_ffmpeg_decode_video(quicktime_t *file,
                            unsigned char **row_pointers,
                            int track);

int lqt_ffmpeg_encode_video(quicktime_t *file,
                            unsigned char **row_pointers,
                            int track);

int lqt_ffmpeg_encode_audio(quicktime_t *file, int16_t **input_i,
                            float **input_f, int track,
                            long samples);

int lqt_ffmpeg_decode_audio(quicktime_t *file, int16_t *output_i,
                            float *output_f, long samples,
                            int track, int channel);

int lqt_ffmpeg_delete_audio(quicktime_audio_map_t *atrack);
int lqt_ffmpeg_delete_video(quicktime_video_map_t *vtrack);


int lqt_ffmpeg_get_lqt_colormodel(enum PixelFormat fmt, int * exact);
enum PixelFormat lqt_ffmpeg_get_ffmpeg_colormodel(int colormodel);


#endif
