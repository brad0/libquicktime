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

/*
 Damned if I know how to handle sample-granular access.  There is no 
 guaranteed 1:1 mapping between compressed and uncompressed samples.
 
 Linear access is easy, but the moment someone seeks, how can we possibly
 find  where to start again?
*/

int lqt_ffmpeg_decode_audio(quicktime_t *file, int16_t *output_i, float *output_f, long samples,
                            int track, int channel)
  {
  int result = -1;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  int channels = file->atracks[track].channels;
  int ad_length;
  
  if(!codec->init_dec)
    {
    memcpy(&(codec->ffcodec_enc), &(codec->params), sizeof(AVCodecContext));
          
    if(avcodec_open(&codec->ffcodec_dec, codec->ffc_dec) != 0)
      return -1;

    codec->sample_buffer_pos = 0;
    codec->init_dec = 1;
    }

  ad_length = (codec->sample_buffer_size / codec->ffcodec_dec.frame_size +
               codec->ffcodec_dec.frame_size - 1) * codec->ffcodec_dec.frame_size;
  
  if(codec->sample_buffer_size < ad_length)
    {
    codec->sample_buffer_size = ad_length;
    codec->sample_buffer = realloc(codec->sample_buffer, ad_length * channels * 2);
    }

    
  return result;  
  }

/*
 *   Encoding part
 */
        
static void interleave(short * dst, int16_t ** input_i, float ** input_f, int start, int end, int channels)
  {
  int i, j;
  if(input_i)
    {
    for(i = start; i < end; i++)
      {
      for(j = 0; j < channels; j++)
        *(dst++) = input_i[j][i];
      }
    }
  else if(input_f)
    {
    for(i = start; i < end; i++)
      {
      for(j = 0; j < channels; j++)
        *(dst++) = (int16_t)(input_f[j][i] * 16383.0);
      }
    }
  }


/*
 Untested, but it should work...   
*/

int lqt_ffmpeg_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f, int track, long samples)
  {
  int result = -1;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  quicktime_trak_t *trak = track_map->track;
  int channels = file->atracks[track].channels;
  longest offset;
  int size = 0;

  int interleave_end;
  int samples_done = 0;

  //  fprintf(stderr, "Encoding %d samples at %d bps %p\n", samples, codec->params.bit_rate, &codec->params);
  
  if(!codec->init_enc)
    {
    memcpy(&(codec->ffcodec_enc), &(codec->params), sizeof(AVCodecContext));

    codec->ffcodec_enc.sample_rate = trak->mdia.minf.stbl.stsd.table[0].sample_rate;
    codec->ffcodec_enc.channels = channels;
    
    if(avcodec_open(&codec->ffcodec_enc, codec->ffc_enc) != 0)
      {
      fprintf(stderr, "Avcodec open failed\n");
      return -1;
      }
    
    codec->init_enc = 1;
    
    codec->write_buffer_size = codec->ffcodec_enc.frame_size * 2 * channels * 2;
    codec->write_buffer = malloc(codec->write_buffer_size);
    if(!codec->write_buffer)
      return -1;
    codec->sample_buffer = malloc(codec->ffcodec_enc.frame_size * 2 * channels);
    if(!codec->sample_buffer)
      return -1;
    codec->sample_buffer_pos = 0;
    }
    
  /* While there is still data to encode... */
  while(samples_done < samples)
    {
    interleave_end = samples - samples_done < codec->ffcodec_enc.frame_size - codec->sample_buffer_pos
      ? samples - samples_done + codec->sample_buffer_pos : codec->ffcodec_enc.frame_size;

    interleave(&codec->sample_buffer[codec->sample_buffer_pos], input_i, input_f,
                codec->sample_buffer_pos, interleave_end, channels);

    samples_done += interleave_end - codec->sample_buffer_pos;
    
    /* If buffer is full, write a chunk. */
    if(interleave_end == codec->ffcodec_enc.frame_size)
      {
      size = avcodec_encode_audio(&codec->ffcodec_enc,
                                  codec->write_buffer,
                                  codec->write_buffer_size,
                                  codec->sample_buffer);
      if(size > 0)
        {
        //        fprintf(stderr, "Encoded size: %d\n", size);
        offset = quicktime_position(file);
        result = quicktime_write_data(file, codec->write_buffer, size);
        result = (result)?0:1;
        quicktime_update_tables(file, 
                                track_map->track,
                                offset,
                                track_map->current_chunk,
                                0,
                                codec->ffcodec_enc.frame_size,
                                0);
        file->atracks[track].current_chunk++;
        codec->sample_buffer_pos = 0;
        }
      }
    else
      codec->sample_buffer_pos = interleave_end;
    
    }
  return result;
  }
