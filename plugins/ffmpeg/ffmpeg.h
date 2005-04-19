/* 
 * ffmpeg.h (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
 * Based entirely upon qtpng.h from libquicktime 
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

  /* We decode the first frame during the init() function to
     obtain the stream colormodel */

  int have_frame;

  int encode_colormodel;
  
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

int lqt_ffmpeg_decode_audio(quicktime_t * file, int16_t ** output_i,
                            float ** output_f, long samples,
                            int track);

int lqt_ffmpeg_delete_audio(quicktime_audio_map_t *atrack);
int lqt_ffmpeg_delete_video(quicktime_video_map_t *vtrack);

int lqt_ffmpeg_set_parameter_audio(quicktime_t *file, 
                                   int track, 
                                   char *key, 
                                   void *value);

int lqt_ffmpeg_get_lqt_colormodel(enum PixelFormat fmt, int * exact);
enum PixelFormat lqt_ffmpeg_get_ffmpeg_colormodel(int colormodel);


#endif
