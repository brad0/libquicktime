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
  int frame_bytes;
  int64_t offset;
  int num_samples;
  int samples_decoded = 0;
  int bytes_decoded;
  int bytes_used;
  int64_t chunk_size;
  offset = quicktime_chunk_to_offset(file, track_map->track, codec->current_chunk);
  num_samples = quicktime_chunk_samples(track_map->track, codec->current_chunk);
  
  /* Reallocate sample buffer */
  if(!num_samples)
    return 0;
  //  num_samples = 100000;
  if(codec->sample_buffer_size < codec->samples_in_buffer + num_samples)
    {
    codec->sample_buffer_size = codec->samples_in_buffer + num_samples;
    //    fprintf(stderr, "codec->sample_buffer_size: %d\n", codec->sample_buffer_size);
    codec->sample_buffer = realloc(codec->sample_buffer, 2 * codec->sample_buffer_size *
                                   track_map->channels);
    }
  
  /* Determine the chunk size */

  chunk_size = codec->chunk_sizes[codec->current_chunk-1];
  //  fprintf(stderr, "Chunk size: %lld, samples: %d\n", chunk_size, num_samples);
  if(codec->chunk_buffer_size < codec->bytes_in_chunk_buffer + chunk_size +
     FF_INPUT_BUFFER_PADDING_SIZE)
    {
    codec->chunk_buffer_size = codec->bytes_in_chunk_buffer +
      chunk_size + 100 + FF_INPUT_BUFFER_PADDING_SIZE;
    codec->chunk_buffer = realloc(codec->chunk_buffer, codec->chunk_buffer_size);
    }

  /* Read one chunk */

  offset = quicktime_chunk_to_offset(file, track_map->track,
                                     codec->current_chunk);
  quicktime_set_position(file, offset);
  
  result = !quicktime_read_data(file, codec->chunk_buffer, chunk_size);

  codec->bytes_in_chunk_buffer += chunk_size;
  
  memset(&(codec->chunk_buffer[codec->bytes_in_chunk_buffer]), 0, FF_INPUT_BUFFER_PADDING_SIZE);
  
  /* Decode this */

  /* decode an audio frame. return -1 if error, otherwise return the
   *number of bytes used. If no frame could be decompressed,
   *frame_size_ptr is zero. Otherwise, it is the decompressed frame
   *size in BYTES. */

  bytes_used = 0;
  while(1)
    {
    frame_bytes =
      avcodec_decode_audio(codec->com.ffcodec_dec,
                           &(codec->sample_buffer[track_map->channels * codec->samples_in_buffer]),
                           &bytes_decoded,
                           &(codec->chunk_buffer[bytes_used]), codec->bytes_in_chunk_buffer);
    if(frame_bytes < 0)
      {
      fprintf(stderr, "avcodec_decode_audio error\n");
      break;
      }
#if 0
    fprintf(stderr, "Samples decoded: %d, Bytes used: %d, chunk_size: %lld, chunk buffer: %d\n",
            bytes_decoded / (track_map->channels * 2),
            frame_bytes, chunk_size, codec->bytes_in_chunk_buffer);
#endif
        
    /* Incomplete frame, save the data for later use and exit here */

    bytes_used                   += frame_bytes;
    codec->bytes_in_chunk_buffer -= frame_bytes;

    /* This happens because ffmpeg adds FF_INPUT_BUFFER_PADDING_SIZE to the bytes returned */
    
    if(codec->bytes_in_chunk_buffer < 0)
      codec->bytes_in_chunk_buffer = 0;

    if(bytes_decoded <= 0)
      {
      if(codec->bytes_in_chunk_buffer > 0)
        //        memmove(codec->chunk_buffer, &(codec->chunk_buffer[bytes_used]), codec->bytes_in_chunk_buffer);
        codec->bytes_in_chunk_buffer = 0;
      break;
      }
    
    samples_decoded += (bytes_decoded / (track_map->channels * 2));
    codec->samples_in_buffer += (bytes_decoded / (track_map->channels * 2));
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
  int64_t total_samples;
  /* Initialize codec */
  
  if(!codec->com.init_dec)
    {
    codec->com.ffcodec_dec = avcodec_alloc_context();
    //    memcpy(&(codec->com.ffcodec_enc), &(codec->com.params), sizeof(AVCodecContext));
    
    if(avcodec_open(codec->com.ffcodec_dec, codec->com.ffc_dec) != 0)
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
      int samples_to_skip;
      quicktime_chunk_of_sample(&chunk_sample,
                                &(codec->current_chunk),
                                file->atracks[track].track,
                                file->atracks[track].current_position);
      samples_to_skip = file->atracks[track].current_position - chunk_sample;

      /* Decode this chunk and skip samples */
      codec->samples_in_buffer = 0;
      decode_chunk(file, codec, track_map);
      codec->current_chunk++;
      if(samples_to_skip > 0)
        {
        if(codec->samples_in_buffer - samples_to_skip > 0)
          memmove(codec->sample_buffer, &(codec->sample_buffer[samples_to_skip * channels]),
                  (codec->samples_in_buffer - samples_to_skip) * channels * sizeof(int16_t));
        codec->samples_in_buffer -= samples_to_skip;
        }
      }
    codec->current_position = file->atracks[track].current_position;
    }

  /* Lets not hang if we are at the end of the file */
  total_samples = quicktime_audio_length(file, track);
  if(samples + codec->current_position > total_samples)
    {
    samples = total_samples - codec->current_position;
    }
  
  /* Read new chunks until we have enough samples */
  
  while(codec->samples_in_buffer < samples)
    {
    if(codec->current_chunk >= file->atracks[track].track->mdia.minf.stbl.stco.total_entries)
      return 0;
    
    result = decode_chunk(file, codec, track_map);
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

    codec->chunk_buffer_size = (codec->com.ffcodec_enc->bit_rate * codec->com.ffcodec_enc->frame_size /
                                (codec->com.ffcodec_enc->sample_rate * 8)) + 1024;
    codec->chunk_buffer = malloc(codec->chunk_buffer_size);
    }

  /* Allocate sample buffer if necessary */

  if(!codec->sample_buffer_size < codec->samples_in_buffer + samples)
    {
    codec->sample_buffer_size = codec->samples_in_buffer + samples + 16;
    codec->sample_buffer = realloc(codec->sample_buffer,
                                   codec->sample_buffer_size * channels * sizeof(int16_t));
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
                                       codec->chunk_buffer_size,
                                       &(codec->sample_buffer[samples_done*channels]));
    if(frame_bytes > 0)
      {
      quicktime_write_chunk_header(file, trak, &chunk_atom);
      
      if(codec->com.ffcodec_enc->frame_size == 1)
        samples_encoded = codec->samples_in_buffer;
      else
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
