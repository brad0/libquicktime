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

static void deinterleave(int16_t *dst_i, float *dst_f, int16_t * src,
                         int channel, int num_channels, int length)
  {
  int i;
  if(dst_f)
    {
    for(i = 0; i < length; i++)
      {
      dst_f[i] = (float)src[i * num_channels + channel]/16383.0;
      }
    }
  else if(dst_i)
    {
    for(i = 0; i < length; i++)
      {
      dst_i[i] = src[i * num_channels + channel];
      }
    }
  }

/* Decode the current chunk into the sample buffer */

static int decode_chunk(quicktime_t * file, quicktime_ffmpeg_audio_codec_t *codec,
                        quicktime_audio_map_t *track_map)
  {
  int result;
  int64_t offset;
  int num_samples;
  int samples_decoded = 0;
  int bytes_decoded;
  int bytes_used;
  int64_t chunk_size;
  offset = quicktime_chunk_to_offset(file, track_map->track, codec->current_chunk);
  num_samples = quicktime_chunk_samples(track_map->track, codec->current_chunk);
  
  /* Reallocate sample buffer */
  fprintf(stderr, "Num samples: %d\n", num_samples);
  if(!num_samples)
    //    return 0;
    num_samples = 100000;
  if(codec->sample_buffer_size < codec->samples_in_buffer + num_samples)
    {
    codec->sample_buffer_size = codec->samples_in_buffer + num_samples;
    //    fprintf(stderr, "codec->sample_buffer_size: %d\n", codec->sample_buffer_size);
    codec->sample_buffer = realloc(codec->sample_buffer, 2 * codec->sample_buffer_size *
                                   track_map->channels);
    }
  
  /* Determine the chunk size */

  chunk_size = codec->chunk_sizes[codec->current_chunk-1];
  fprintf(stderr, "Chunk size: %d\n", chunk_size);
  if(codec->chunk_buffer_size < chunk_size)
    {
    codec->chunk_buffer_size = chunk_size + 100;
    codec->chunk_buffer = realloc(codec->chunk_buffer, codec->chunk_buffer_size);
    }

  /* Read one chunk */

  offset = quicktime_chunk_to_offset(file, track_map->track, codec->current_chunk);
  quicktime_set_position(file, offset);
  
  result = !quicktime_read_data(file, codec->chunk_buffer, chunk_size);

  /* Decode this */

  /* decode an audio frame. return -1 if error, otherwise return the
   *number of bytes used. If no frame could be decompressed,
   *frame_size_ptr is zero. Otherwise, it is the decompressed frame
   *size in BYTES. */

  bytes_used = 0;
  while(chunk_size - bytes_used)
    {
    bytes_used +=
      avcodec_decode_audio(&(codec->com.ffcodec_dec),
                           &(codec->sample_buffer[track_map->channels * codec->samples_in_buffer]),
                           &bytes_decoded,
                           &(codec->chunk_buffer[bytes_used]), chunk_size - bytes_used);
    if(bytes_decoded < 0)
      break;

    samples_decoded += (bytes_decoded / (track_map->channels * 2));
    codec->samples_in_buffer += (bytes_decoded / (track_map->channels * 2));

    fprintf(stderr, "Samples decoded: %d, Bytes used: %d, chunk_size: %lld\n",
            bytes_decoded / (track_map->channels * 2),
            bytes_used, chunk_size);
    }
#if 0
  fprintf(stderr, "Last samples decoded: %d, Bytes used: %d, samples decoded %d, chunk_size: %lld\n",
          bytes_decoded / (track_map->channels * 2),
          bytes_used, samples_decoded, chunk_size);
#endif
  return result;
  }


/*
 Damned if I know how to handle sample-granular access.  There is no 
 guaranteed 1:1 mapping between compressed and uncompressed samples.
 
 Linear access is easy, but the moment someone seeks, how can we possibly
 find  where to start again?
*/

int lqt_ffmpeg_decode_audio(quicktime_t *file, int16_t *output_i, float *output_f, long samples,
                            int track, int channel)
  {
  int result = 0;
  int samples_decoded;
  int64_t chunk_sample, current_chunk; /* For seeking only */
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  int channels = file->atracks[track].channels;

  /* Initialize codec */
  
  if(!codec->com.init_dec)
    {
    memcpy(&(codec->com.ffcodec_enc), &(codec->com.params), sizeof(AVCodecContext));
          
    if(avcodec_open(&codec->com.ffcodec_dec, codec->com.ffc_dec) != 0)
      {
      fprintf(stderr, "Avcodec open failed\n");
      return -1;
      }
    
    //    codec->sample_buffer_offset = 0;
    codec->com.init_dec = 1;
    /* Build table of contents */
    codec->chunk_sizes = lqt_get_chunk_sizes(file, file->atracks[track].track);
    codec->current_chunk = 1;
    }

  /* Check if we have to reposition the stream */
  
  if(codec->current_position != file->atracks[track].current_position)
    {
    /* Sequential reading happened -> flush sample buffer */
    
    if(codec->current_position + codec->num_samples == file->atracks[track].current_position)
      {
      if(codec->samples_in_buffer - codec->num_samples > 0)
        {
        memmove(codec->sample_buffer, &(codec->sample_buffer[codec->num_samples * channels]),
                (codec->samples_in_buffer - codec->num_samples) * channels * sizeof(int16_t));
        }
      codec->samples_in_buffer -= codec->num_samples;
      }
    else /* Seeking happened */
      {
      
      }
    codec->current_position = file->atracks[track].current_position;
    }
  
  /* Read new chunks until we have enough samples */
  
  while(codec->samples_in_buffer < samples)
    {
    decode_chunk(file, codec, track_map);
    codec->current_chunk++;
    }

  /* Deinterleave into the buffer */

  deinterleave(output_i, output_f, codec->sample_buffer,
               channel, channels, samples);
  
  codec->num_samples = samples;
  return result;
  }

/*
 *   Encoding part
 */
        
static void interleave(short * dst, int16_t ** input_i, float ** input_f,
                       int start, int length, int channels)
  {
  int i, j;
  if(input_i)
    {
    for(i = start; i < start + length; i++)
      {
      for(j = 0; j < channels; j++)
        *(dst++) = input_i[j][i];
      }
    }
  else if(input_f)
    {
    for(i = start; i < start + length; i++)
      {
      for(j = 0; j < channels; j++)
        *(dst++) = (int16_t)(input_f[j][i] * 16383.0);
      }
    }
  }


/*
  Untested, but it should work...   
*/

int lqt_ffmpeg_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f,
                            int track, long samples)
  {
#if 0
  int result = -1;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  quicktime_trak_t *trak = track_map->track;
  int channels = file->atracks[track].channels;
  int64_t offset;
  int size = 0;

  int interleave_length;
  int samples_done = 0;
  quicktime_atom_t chunk_atom;

  //  fprintf(stderr, "Samples %d\n", samples);
  
  if(!codec->com.init_enc)
    {
    memcpy(&(codec->com.ffcodec_enc), &(codec->com.params), sizeof(AVCodecContext));

    codec->com.ffcodec_enc.sample_rate = trak->mdia.minf.stbl.stsd.table[0].sample_rate;
    codec->com.ffcodec_enc.channels = channels;
    
    if(avcodec_open(&codec->com.ffcodec_enc, codec->com.ffc_enc) != 0)
      {
      fprintf(stderr, "Avcodec open failed\n");
      return -1;
      }
    
    codec->com.init_enc = 1;
    
    codec->write_buffer_size = codec->samples_per_frame * 2 * channels * 2;
    codec->write_buffer = malloc(codec->write_buffer_size);
    if(!codec->write_buffer)
      return -1;

    codec->sample_buffer = malloc(codec->samples_per_frame * 2 * channels);
    if(!codec->sample_buffer)
      return -1;
    codec->sample_buffer_pos = 0;
    }
    
  /* While there is still data to encode... */
  while(samples_done < samples)
    {
    interleave_length =
      samples - samples_done < codec->samples_per_frame - codec->sample_buffer_pos
      ? samples - samples_done  : codec->samples_per_frame- codec->sample_buffer_pos;

    //    fprintf(stderr, "Interleaving: %d %d\n", samples_done, interleave_length);
    
    interleave(&codec->sample_buffer[codec->sample_buffer_pos * channels], input_i, input_f,
                samples_done, interleave_length, channels);
    
    codec->sample_buffer_pos += interleave_length;
    samples_done += interleave_length;
    
    /* If buffer is full, write a chunk. */
    if(codec->sample_buffer_pos == codec->samples_per_frame)
      {
      //      fprintf(stderr, "Encoding\n");
      size = avcodec_encode_audio(&codec->ffcodec_enc,
                                  codec->write_buffer,
                                  codec->write_buffer_size,
                                  codec->sample_buffer);
      if(size > 0)
        {
        //        fprintf(stderr, "Encoded size: %d\n", size);

        quicktime_write_chunk_header(file, trak, &chunk_atom);
        result = !quicktime_write_data(file, codec->write_buffer, size);
        quicktime_write_chunk_footer(file, 
                                     trak, 
                                     file->atracks[track].current_chunk,
                                     &chunk_atom, 
                                     codec->samples_per_frame);
        
        file->atracks[track].current_chunk++;
        codec->sample_buffer_pos = 0;
        }
      }
    
    }

  return result;
#endif
  return 1;
  }
