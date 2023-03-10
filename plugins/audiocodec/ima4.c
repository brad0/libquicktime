/*******************************************************************************
 ima4.c

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
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>
#include <stdlib.h>
#include <string.h>
#include "audiocodec.h"

#define LOG_DOMAIN "ima4"

/* Known by divine revelation */

#define BLOCK_SIZE 0x22
#define SAMPLES_PER_BLOCK 0x40


typedef struct
  {
  /* Starting information for all channels during encoding. */
  int *last_samples, *last_indexes;

  /* Buffer for samples */

  int16_t * sample_buffer; /* SAMPLES_PER_BLOCK * channels (interleaved) */
  int sample_buffer_size;  /* samples from last en/decode call */
 
  int packet_samples;
  
  //  int chunk_buffer_size;
  //  int chunk_buffer_alloc;
  //  uint8_t * chunk_buffer;
  uint8_t * pkt_ptr;

  int decode_initialized;
  int encode_initialized;

  lqt_packet_t pkt;
  
  } quicktime_ima4_codec_t;

static int ima4_step[89] = 
{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static int ima4_index[16] = 
{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};


/* ================================== private for ima4 */


static void ima4_decode_sample(int *predictor, int *nibble, int *index, int *step)
  {
  int difference, sign;

  /* Get new index value */
  *index += ima4_index[*nibble];

  if(*index < 0) *index = 0; 
  else 
    if(*index > 88) *index = 88;

  /* Get sign and magnitude from *nibble */
  sign = *nibble & 8;
  *nibble = *nibble & 7;

  /* Get difference */
  difference = *step >> 3;
  if(*nibble & 4) difference += *step;
  if(*nibble & 2) difference += *step >> 1;
  if(*nibble & 1) difference += *step >> 2;

  /* Predict value */
  if(sign) 
    *predictor -= difference;
  else 
    *predictor += difference;

  if(*predictor > 32767) *predictor = 32767;
  else
    if(*predictor < -32768) *predictor = -32768;

  /* Update the step value */
  *step = ima4_step[*index];
  }

static void ima4_decode_block(quicktime_audio_map_t *atrack, int16_t *output,
                              unsigned char *input, int channels)
  {
  int predictor;
  int index;
  int step;
  int nibble, nibble_count;
  unsigned char *input_end = input + BLOCK_SIZE;

  /* Get the chunk header */
  predictor = *input++ << 8;
  predictor |= *input++;

  index = predictor & 0x7f;
  if(index > 88) index = 88;

  predictor &= 0xff80;
  if(predictor & 0x8000) predictor -= 0x10000;
  step = ima4_step[index];

  /* Read the input buffer sequentially, one nibble at a time */
  nibble_count = 0;
  while(input < input_end)
    {
    nibble = nibble_count ? (*input++  >> 4) & 0x0f : *input & 0x0f;

    ima4_decode_sample(&predictor, &nibble, &index, &step);
    *output = predictor;
    output += channels; // TODO: Change this for planar output
    nibble_count ^= 1;
    }
  }

static void ima4_encode_sample(int *last_sample, int *last_index, int *nibble, int next_sample)
  {
  int difference, new_difference, mask, step;

  difference = next_sample - *last_sample;
  *nibble = 0;
  step = ima4_step[*last_index];
  new_difference = step >> 3;

  if(difference < 0)
    {
    *nibble = 8;
    difference = -difference;
    }

  mask = 4;
  while(mask)
    {
    if(difference >= step)
      {
      *nibble |= mask;
      difference -= step;
      new_difference += step;
      }

    step >>= 1;
    mask >>= 1;
    }

  if(*nibble & 8)
    *last_sample -= new_difference;
  else
    *last_sample += new_difference;

  if(*last_sample > 32767) *last_sample = 32767;
  else
    if(*last_sample < -32767) *last_sample = -32767;

  *last_index += ima4_index[*nibble];

  if(*last_index < 0) *last_index = 0;
  else
    if(*last_index > 88) *last_index= 88;
  }

static void ima4_encode_block(quicktime_audio_map_t *atrack, unsigned char *output,
                              int16_t *input, int step, int channel)
  {
  quicktime_ima4_codec_t *codec = atrack->codec->priv;
  int i, nibble_count = 0, nibble, header;

  /* Get a fake starting sample */
  header = codec->last_samples[channel];
  /* Force rounding. */
  if(header < 0x7fc0) header += 0x40;
  if(header < 0) header += 0x10000;
  header &= 0xff80;
  *output++ = (header & 0xff00) >> 8;
  *output++ = (header & 0x80) + (codec->last_indexes[channel] & 0x7f);

  for(i = 0; i < SAMPLES_PER_BLOCK; i++)
    {
    ima4_encode_sample(&codec->last_samples[channel], 
                       &codec->last_indexes[channel], 
                       &nibble, 
                       *input);

    if(nibble_count)
      *output++ |= (nibble << 4);
    else
      *output = nibble;

    input += step;
    nibble_count ^= 1;
    }
  }

/* Convert the number of samples in a chunk into the number of bytes in that */
/* chunk.  The number of samples in a chunk should end on a block boundary. */
static long ima4_samples_to_bytes(long samples, int channels)
  {
  return (samples / SAMPLES_PER_BLOCK) * BLOCK_SIZE * channels;
  }

#if 0
static int read_audio_chunk(quicktime_t * file, int track,
                            long chunk,
                            uint8_t ** buffer, int * buffer_alloc)
  {
  int bytes, samples, bytes_from_samples;

  bytes = lqt_read_audio_chunk(file, track, chunk, buffer, buffer_alloc, &samples);
  //  bytes_from_samples = ima4_samples_to_bytes(samples, file->atracks[track].channels);
  //  if(bytes > bytes_from_samples)
  //    return bytes_from_samples;
  //  else
    return bytes;
  }
#endif


/* =================================== public for ima4 */

static int delete_codec(quicktime_codec_t *codec_base)
  {
  quicktime_ima4_codec_t *codec = codec_base->priv;
  
  if(codec->last_samples) free(codec->last_samples);
  if(codec->last_indexes) free(codec->last_indexes);
  
  if(codec->sample_buffer)
    free(codec->sample_buffer);

  lqt_packet_free(&codec->pkt);
  
  free(codec);
  return 0;
  }

static int decode_packet(quicktime_t *file, int track, lqt_audio_buffer_t * buf)
  {
  int i;
  /* Decode ONE ima frame */
  quicktime_audio_map_t * atrack = &file->atracks[track];
  quicktime_ima4_codec_t *codec = atrack->codec->priv;

  if(!buf)
    return 0;

  /* Read packet */
  if(!codec->pkt_ptr || (codec->pkt_ptr - codec->pkt.data >= codec->pkt.data_len))
    {
    if(!quicktime_trak_read_packet(file, atrack->track, &codec->pkt))
      return 0;
    codec->pkt_ptr = codec->pkt.data;
    codec->packet_samples = codec->pkt.duration;
    }

  lqt_audio_buffer_alloc(buf, SAMPLES_PER_BLOCK, atrack->channels, 0, atrack->sample_format);
  
  /* Decode frame */
  for(i = 0; i < atrack->channels; i++)
    {
    ima4_decode_block(atrack,
                      buf->channels[0].i_16 + i,
                      codec->pkt_ptr, atrack->channels);
    codec->pkt_ptr += BLOCK_SIZE;
    }

  buf->size = SAMPLES_PER_BLOCK;
  if(codec->packet_samples < SAMPLES_PER_BLOCK)
    buf->size = codec->packet_samples;
  codec->packet_samples -= SAMPLES_PER_BLOCK;
  return buf->size;
  }

static void resync(quicktime_t *file, int track)
  {
  quicktime_audio_map_t * atrack = &file->atracks[track];
  quicktime_ima4_codec_t *codec = atrack->codec->priv;
  codec->pkt_ptr = NULL;
  }

static int encode(quicktime_t *file, void *_input, long samples, int track)
  {
  int result = 0;
  int64_t j;
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_ima4_codec_t *codec = track_map->codec->priv;
  quicktime_trak_t *trak = track_map->track;
  int16_t *input_ptr, *input;
  unsigned char *output_ptr;
  int samples_encoded, total_samples_copied,
    samples_copied, frames_encoded, old_buffer_size;
  
  if(codec->encode_initialized)
    {
    codec->encode_initialized = 1;
    trak->mdia.minf.stbl.stsd.table[0].sample_size = 16;
    }
  
  
  /* Get output size */
  codec->pkt.data_len = ima4_samples_to_bytes(samples + codec->sample_buffer_size,
                                       track_map->channels);

  lqt_packet_alloc(&codec->pkt, codec->pkt.data_len + track_map->channels * BLOCK_SIZE);
  
  if(!codec->last_samples)
    codec->last_samples = calloc(track_map->channels, sizeof(*(codec->last_samples)));
        
  if(!codec->last_indexes)
    codec->last_indexes = calloc(track_map->channels, sizeof(*(codec->last_samples)));
        
  if(!codec->sample_buffer)
    {
    codec->sample_buffer = malloc(sizeof(*(codec->sample_buffer)) * track_map->channels * SAMPLES_PER_BLOCK);
    }
        
  /* Encode from the input buffer to the read_buffer up to a multiple of  */
  /* blocks. */
  output_ptr = codec->pkt.data;
  
  input = (int16_t*)_input;
  input_ptr = input;
  samples_encoded = 0;
  frames_encoded = 0;
  total_samples_copied = 0;
  
  old_buffer_size = codec->sample_buffer_size;
  
  while(samples_encoded < samples + old_buffer_size)
    {
    /* Copy input to sample buffer */
    samples_copied = SAMPLES_PER_BLOCK - codec->sample_buffer_size;
    
    if(samples_copied > samples - total_samples_copied)
      samples_copied = samples - total_samples_copied;
   
    memcpy(codec->sample_buffer + track_map->channels * codec->sample_buffer_size,
           input_ptr,
           samples_copied * track_map->channels * 2);

    total_samples_copied += samples_copied;
    
    input_ptr += samples_copied * track_map->channels;

    codec->sample_buffer_size += samples_copied;

    if(codec->sample_buffer_size == SAMPLES_PER_BLOCK)
      {
      for(j = 0; j < track_map->channels; j++)
        {
        ima4_encode_block(track_map, output_ptr, codec->sample_buffer + j,
                          track_map->channels, j);
        output_ptr += BLOCK_SIZE;
        }
      samples_encoded += SAMPLES_PER_BLOCK;
      frames_encoded ++;
      codec->sample_buffer_size = 0;
      }
    else
      {
      break;
      }
    }
  
  /* Write to disk */
        
  /* The block division may result in 0 samples getting encoded. */
  /* Don't write 0 samples. */
  if(samples_encoded)
    {
    quicktime_write_chunk_header(file, trak);
    result = quicktime_write_data(file, codec->pkt.data, codec->pkt.data_len);
    trak->chunk_samples = samples_encoded;
    quicktime_write_chunk_footer(file, 
                                 trak);
    
    if(result) 
      result = 0; 
    else 
      result = 1; /* defeat fwrite's return */

    track_map->cur_chunk++;
    }
  return result;
  }

static int flush(quicktime_t *file, int track)
  {
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_ima4_codec_t *codec = track_map->codec->priv;
  quicktime_trak_t *trak = track_map->track;
  

  int result = 0;
  int i;
  unsigned char *output_ptr;
  
  if(codec->sample_buffer_size)
    {
    /* Zero out enough to get a block */
    i = codec->sample_buffer_size * track_map->channels;
    while(i < SAMPLES_PER_BLOCK * track_map->channels)
      codec->sample_buffer[i++] = 0;

    output_ptr = codec->pkt.data;
    for(i = 0; i < track_map->channels; i++)
      {
      ima4_encode_block(track_map, output_ptr, codec->sample_buffer + i, track_map->channels, i);
      output_ptr += BLOCK_SIZE;
      }
    codec->pkt.data_len = (int)(output_ptr - codec->pkt.data);
    
    quicktime_write_chunk_header(file, trak);
    result = quicktime_write_data(file, codec->pkt.data, codec->pkt.data_len);
    
    trak->chunk_samples = codec->sample_buffer_size;
    quicktime_write_chunk_footer(file, trak);
    track_map->cur_chunk++;
    return 1;
    }
  return 0;
  }

void quicktime_init_codec_ima4(quicktime_codec_t * codec_base,
                               quicktime_audio_map_t *atrack,
                               quicktime_video_map_t *vtrack)
  {
  quicktime_ima4_codec_t *codec;
  /* Set sample format */

  if(atrack)
    atrack->sample_format = LQT_SAMPLE_INT16;

  codec = calloc(1, sizeof(*codec));
  
  codec_base->priv = codec;

  codec_base->delete_codec = delete_codec;

  codec_base->decode_video = 0;
  codec_base->encode_video = 0;
  codec_base->resync = resync;
  codec_base->decode_audio_packet = decode_packet;

  codec_base->encode_audio = encode;
  codec_base->flush = flush;
  
  }
