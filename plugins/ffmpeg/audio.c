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
#include "config.h"

#include "funcprotos.h"
#include <quicktime/quicktime.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

#include "funcprotos.h"
#include <quicktime/colormodels.h>

#include "ffmpeg.h"

int lqt_ffmpeg_delete_audio(quicktime_audio_map_t *vtrack)
  {
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  
  if(codec->com.init_enc)
    avcodec_close(codec->com.ffcodec_enc);
  if(codec->com.init_dec)
    avcodec_close(codec->com.ffcodec_dec);
  
  if(codec->sample_buffer) free(codec->sample_buffer);
  if(codec->chunk_buffer)  free(codec->chunk_buffer);
  
  free(codec);
  return 0;
  }

static void deinterleave(int16_t ** dst_i, float ** dst_f, int16_t * src,
                         int channels, int samples)
  {
  int i, j;
  if(dst_f)
    {
    for(i = 0; i < channels; i++)
      {
      if(dst_f[i])
        {
        for(j = 0; j < samples; j++)
          {
          dst_f[i][j] = (float)src[j*channels + i]/32767.0;
          }
        }
      }
    }
  if(dst_i)
    {
    for(i = 0; i < channels; i++)
      {
      if(dst_i[i])
        {
        for(j = 0; j < samples; j++)
          {
          dst_i[i][j] = src[j*channels + i];
          }
        }
      }

    }
  }

/* Decode the current chunk into the sample buffer */

static int decode_chunk(quicktime_t * file, int track)
  {
  int frame_bytes;
  int num_samples;
  int samples_decoded = 0;
  int bytes_decoded;
  int bytes_used;
  int64_t chunk_size;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

  //  fprintf(stderr, "Quicktime chunk samples...");
  num_samples = quicktime_chunk_samples(track_map->track, track_map->current_chunk);

  //  fprintf(stderr, "done %d\n", num_samples);

  if(!num_samples)
    {
    fprintf(stderr, "audio_ffmpeg: EOF\n");
    return 0;
    }
  /*
   *  For AVIs, chunk samples are not always 100% correct.
   *  Furthermore, there can be a complete mp3 frame from the last chunk!
   */

  num_samples += 8192;

  /* Reallocate sample buffer */
  
  if(codec->sample_buffer_alloc < codec->sample_buffer_end - codec->sample_buffer_start + num_samples)
    {
    
    codec->sample_buffer_alloc = codec->sample_buffer_end - codec->sample_buffer_start + num_samples;
    //    fprintf(stderr, "codec->sample_buffer_alloc: %d\n", codec->sample_buffer_alloc);
    codec->sample_buffer = realloc(codec->sample_buffer, 2 * codec->sample_buffer_alloc *
                                   track_map->channels);
    }

  /* Read chunk */

  chunk_size = lqt_append_audio_chunk(file,
                                      track, track_map->current_chunk,
                                      &(codec->chunk_buffer),
                                      &(codec->chunk_buffer_alloc),
                                      codec->bytes_in_chunk_buffer);

  track_map->current_chunk++;
  
  codec->bytes_in_chunk_buffer += chunk_size;
  
  /* Decode this */

  /* decode an audio frame. return -1 if error, otherwise return the
   *number of bytes used. If no frame could be decompressed,
   *frame_size_ptr is zero. Otherwise, it is the decompressed frame
   *size in BYTES. */

  bytes_used = 0;
  while(1)
    {
#if 0
    fprintf(stderr, "Avcodec decode audio %d\n",
            codec->bytes_in_chunk_buffer +
            FF_INPUT_BUFFER_PADDING_SIZE);
#endif
    /* BIG NOTE: We pass extra FF_INPUT_BUFFER_PADDING_SIZE for the buffer size
       because we know, that lqt_read_audio_chunk allocates 16 extra bytes for us */
#if 0
    fprintf(stderr, "decode_chunk: Sample buffer: %lld %lld %d, chunk_buffer: %d\n",
            codec->sample_buffer_start,
            codec->sample_buffer_end,
            (int)(codec->sample_buffer_end - codec->sample_buffer_start),
            codec->bytes_in_chunk_buffer);
#endif
    frame_bytes =
      avcodec_decode_audio(codec->com.ffcodec_dec,
                           &(codec->sample_buffer[track_map->channels *
                                                  (codec->sample_buffer_end - codec->sample_buffer_start)]),
                           &bytes_decoded,
                           &(codec->chunk_buffer[bytes_used]),
                           codec->bytes_in_chunk_buffer + FF_INPUT_BUFFER_PADDING_SIZE);
    if(frame_bytes < 0)
      {
      fprintf(stderr, "avcodec_decode_audio error\n");
      break;
      }
#if 0
    fprintf(stderr, "decode_chunk: Samples decoded: %d, Bytes used: %d, chunk_size: %lld, chunk buffer: %d\n",
            bytes_decoded / (track_map->channels * 2),
            frame_bytes, chunk_size, codec->bytes_in_chunk_buffer);
#endif

    bytes_used                   += frame_bytes;
    codec->bytes_in_chunk_buffer -= frame_bytes;
    
    /* Incomplete frame, save the data for later use and exit here */

    if(bytes_decoded < 0)
      {
      //      fprintf(stderr, "decode_chunk: Incomplete frame\n");
      
      if(codec->bytes_in_chunk_buffer > 0)
        memmove(codec->chunk_buffer,
                codec->chunk_buffer + bytes_used, codec->bytes_in_chunk_buffer);
      return 1;
      }
    
    /* This happens because ffmpeg adds FF_INPUT_BUFFER_PADDING_SIZE to the bytes returned */
    
    if(codec->bytes_in_chunk_buffer < 0)
      codec->bytes_in_chunk_buffer = 0;

    if(bytes_decoded <= 0)
      {
      if(codec->bytes_in_chunk_buffer > 0)
        codec->bytes_in_chunk_buffer = 0;
      break;
      }
    
    samples_decoded += (bytes_decoded / (track_map->channels * 2));
    codec->sample_buffer_end += (bytes_decoded / (track_map->channels * 2));

    if(!codec->bytes_in_chunk_buffer)
      break;

    }
#if 0
  fprintf(stderr, "Last samples decoded: %d, Bytes used: %d, samples decoded %d, chunk_size: %lld\n",
          bytes_decoded / (track_map->channels * 2),
          bytes_used, samples_decoded, chunk_size);
#endif
  //  track_map->current_chunk++;
  return samples_decoded;
  }

int lqt_ffmpeg_decode_audio(quicktime_t *file, int16_t **output_i,
                            float **output_f, long samples,
                            int track)
  {
  int result = 0;
  int64_t chunk_sample; /* For seeking only */
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  int channels = file->atracks[track].channels;
  //  int64_t total_samples;

  int samples_to_skip;
  int samples_to_move;

  //  fprintf(stderr, "ffmpeg decode audio %lld\n", track_map->current_position);
  
  /* Initialize codec */
  if(!codec->com.init_dec)
    {
    codec->com.ffcodec_dec = avcodec_alloc_context();
    //    memcpy(&(codec->com.ffcodec_enc), &(codec->com.params), sizeof(AVCodecContext));
    
    if(avcodec_open(codec->com.ffcodec_dec, codec->com.ffc_dec) != 0)
      {
      fprintf(stderr, "Avcodec open failed\n");
      return 0;
      }
    
    //    codec->sample_buffer_offset = 0;
    codec->com.init_dec = 1;
    }

  /* Check if we have to reposition the stream */
  
  if(track_map->last_position != track_map->current_position)
    {

    if((track_map->current_position < codec->sample_buffer_start) || 
       (track_map->current_position + samples >= codec->sample_buffer_end))
      {
      quicktime_chunk_of_sample(&chunk_sample,
                                &(track_map->current_chunk),
                                track_map->track,
                                track_map->current_position);

      fprintf(stderr, "Seek: pos: %ld, last_pos: %lld, chunk: %lld, chunk_sample: %lld\n",
              track_map->last_position,
              track_map->current_position,
              track_map->current_chunk, chunk_sample);
            
      codec->sample_buffer_start = chunk_sample;
      codec->sample_buffer_end   = chunk_sample;
      decode_chunk(file, track);
      }
    }
  
  /* Flush unneeded samples */
  
  if(track_map->current_position > codec->sample_buffer_start)
    {
    samples_to_skip = track_map->current_position - codec->sample_buffer_start;

    //    fprintf(stderr, "Flush\n");
    
    if(codec->sample_buffer_end > track_map->current_position)
      {
      samples_to_move = codec->sample_buffer_end - track_map->current_position;

      //      fprintf(stderr, "Memmove...");
      memmove(codec->sample_buffer,
              &(codec->sample_buffer[samples_to_skip * channels]),
              samples_to_move * channels * sizeof(int16_t));
      //      fprintf(stderr, "done\n");
      }
    codec->sample_buffer_start = track_map->current_position;
    }
  
#if 0
  /* Lets not hang if we are at the end of the file */
  total_samples = quicktime_audio_length(file, track);
  if(samples + codec->current_position > total_samples)
    {
    samples = total_samples - codec->current_position;
    }
#endif

  /* Read new chunks until we have enough samples */
  while(codec->sample_buffer_end - codec->sample_buffer_start < samples)
    {
#if 0
    fprintf(stderr, "Samples: %lld -> %lld %lld, %d\n",
            codec->sample_buffer_start, codec->sample_buffer_end,
            codec->sample_buffer_end - codec->sample_buffer_start, samples);
#endif
    if(track_map->current_chunk >= track_map->track->mdia.minf.stbl.stco.total_entries)
      return 0;
    
    result = decode_chunk(file, track);
    }

  /* Deinterleave into the buffer */
  
  deinterleave(output_i, output_f, codec->sample_buffer,
               channels, samples);

  track_map->last_position = track_map->current_position + samples;

  return samples;
  // #endif
  }

/*
 *   Encoding part
 */
        
static void interleave(int16_t * sample_buffer, int16_t ** input_i, float ** input_f,
                       int num_samples, int channels)
  {
  int i, j;
  if(input_i)
    {
    for(i = 0; i < num_samples; i++)
      {
      for(j = 0; j < channels; j++)
        *(sample_buffer++) = input_i[j][i];
      }
    }
  else if(input_f)
    {
    for(i = 0; i < num_samples; i++)
      {
      for(j = 0; j < channels; j++)
        *(sample_buffer++) = (int16_t)(input_f[j][i] * 16383.0);
      }
    }
  }


/*
  Untested, but it should work...   
*/

int lqt_ffmpeg_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f,
                            int track, long samples)
  {
  int result = -1;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  quicktime_trak_t *trak = track_map->track;
  int channels = file->atracks[track].channels;
  quicktime_atom_t chunk_atom;
  int frame_bytes;
  int samples_done = 0;
  int samples_encoded;
  /* Initialize encoder */
    
  if(!codec->com.init_enc)
    {
    codec->com.ffcodec_enc = avcodec_alloc_context();
    codec->com.ffcodec_enc->sample_rate = trak->mdia.minf.stbl.stsd.table[0].sample_rate;
    codec->com.ffcodec_enc->channels = channels;

#define SP(x) codec->com.ffcodec_enc->x = codec->com.params.x
    SP(bit_rate);
#undef SP
    if(avcodec_open(codec->com.ffcodec_enc, codec->com.ffc_enc) != 0)
      {
      fprintf(stderr, "Avcodec open failed\n");
      return -1;
      }
    
    codec->com.init_enc = 1;

    /* One frame is: bitrate * frame_samples / (samplerate * 8) + 1024 */
#if 0		// PATCH 1
    codec->chunk_buffer_alloc = (codec->com.ffcodec_enc->bit_rate * codec->com.ffcodec_enc->frame_size /
                                (codec->com.ffcodec_enc->sample_rate * 8)) + 1024;
#else
    codec->chunk_buffer_alloc = ( codec->com.ffcodec_enc->frame_size
									* sizeof( int16_t )
									* codec->com.ffcodec_enc->channels
								);
#endif
    codec->chunk_buffer = malloc(codec->chunk_buffer_alloc);
    }

  /* Allocate sample buffer if necessary */

//         PATCH 2 
  if(codec->sample_buffer_alloc < (codec->samples_in_buffer + samples))
    {
    codec->sample_buffer_alloc = codec->samples_in_buffer + samples + 16;
    codec->sample_buffer = realloc(codec->sample_buffer,
                                   codec->sample_buffer_alloc * channels * sizeof(int16_t));
    }

  /* Interleave */

  interleave(&(codec->sample_buffer[codec->samples_in_buffer * channels]),
             input_i, input_f, samples, channels);

  codec->samples_in_buffer += samples;
  
  /* Encode */
  
  //  fprintf(stderr, "codec->samples_in_buffer: %d, codec->com.ffcodec_enc->frame_size %d\n",
  //          codec->samples_in_buffer, codec->com.ffcodec_enc->frame_size);
  while(codec->samples_in_buffer >= codec->com.ffcodec_enc->frame_size)
    {
    //    fprintf(stderr, "avcodec_encode_audio %d...", samples_done);
    
    frame_bytes = avcodec_encode_audio(codec->com.ffcodec_enc, codec->chunk_buffer,
                                       codec->chunk_buffer_alloc,
                                       &(codec->sample_buffer[samples_done*channels]));
    if(frame_bytes > 0)
      {
      quicktime_write_chunk_header(file, trak, &chunk_atom);
#if 0    // PATCH 3  
      if(codec->com.ffcodec_enc->frame_size == 1)
        samples_encoded = codec->samples_in_buffer;
      else
#endif
        samples_encoded = codec->com.ffcodec_enc->frame_size;

      //      fprintf(stderr, "Done %d->%d\n", samples_encoded, frame_bytes);

      samples_done              += samples_encoded;
      codec->samples_in_buffer  -= samples_encoded;
      
      result = !quicktime_write_data(file, codec->chunk_buffer, frame_bytes);
      quicktime_write_chunk_footer(file,
                                   trak, 
                                   file->atracks[track].current_chunk,
                                   &chunk_atom, 
                                   samples_encoded);
      file->atracks[track].current_chunk++;
      }
    }
  if(codec->samples_in_buffer && samples_done)
    memmove(codec->sample_buffer, &(codec->sample_buffer[samples_done*channels]),
            codec->samples_in_buffer * sizeof(int16_t) * channels);
  return result;
  }
