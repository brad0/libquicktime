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

/* Functions for obtaining the size of a compressed frame from a given header */

/* For mp3: Taken from gmerlin */

static int mpeg_bitrates[5][16] = {
  /* MPEG-1 */
  { 0,  32000,  64000,  96000, 128000, 160000, 192000, 224000,    // I
       256000, 288000, 320000, 352000, 384000, 416000, 448000, 0},
  { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,    // II
       128000, 160000, 192000, 224000, 256000, 320000, 384000, 0 },
  { 0,  32000,  40000,  48000,  56000,  64000,  80000,  96000,    // III
       112000, 128000, 160000, 192000, 224000, 256000, 320000, 0 },
  
  /* MPEG-2 LSF */
  { 0,  32000,  48000,  56000,  64000,  80000,  96000, 112000,    // I
       128000, 144000, 160000, 176000, 192000, 224000, 256000, 0 },
  { 0,   8000,  16000,  24000,  32000,  40000,  48000,  56000,    
        64000,  80000,  96000, 112000, 128000, 144000, 160000, 0 } // II & III
};

static int mpeg_samplerates[3][3] = {
  { 44100, 48000, 32000 }, // MPEG1
  { 22050, 24000, 16000 }, // MPEG2
  { 11025, 12000, 8000 }   // MPEG2.5
};

#define VERSION_MPEG_1  0
#define VERSION_MPEG_2  1
#define VERSION_MPEG_25 2

#define MPEG_ID_MASK        0x00180000
#define MPEG_MPEG1          0x00180000
#define MPEG_MPEG2          0x00100000
#define MPEG_MPEG2_5        0x00000000

#define MPEG_LAYER_MASK     0x00060000
#define MPEG_LAYER_III      0x00020000
#define MPEG_LAYER_II       0x00040000
#define MPEG_LAYER_I        0x00060000
#define MPEG_PROTECTION     0x00010000
#define MPEG_BITRATE_MASK   0x0000F000
#define MPEG_FREQUENCY_MASK 0x00000C00
#define MPEG_PAD_MASK       0x00000200
#define MPEG_PRIVATE_MASK   0x00000100
#define MPEG_MODE_MASK      0x000000C0
#define MPEG_MODE_EXT_MASK  0x00000030
#define MPEG_COPYRIGHT_MASK 0x00000008
#define MPEG_HOME_MASK      0x00000004
#define MPEG_EMPHASIS_MASK  0x00000003

#define LAYER_I_SAMPLES       384
#define LAYER_II_III_SAMPLES 1152


#define IS_MPEG_AUDIO_HEADER(h) ((h&0xFFE00000)==0xFFE00000)

static int get_frame_bytes_mpeg(uint8_t * buffer)
  {
  int layer = 0;
  int mpeg_version = 0;

  int samplerate = 0;
  int samplerate_index;

  int bitrate = 0;
  int bitrate_index;

  int samples_per_frame;
  int ret;
  
  uint32_t header = buffer[3] | (buffer[2] << 8) | (buffer[1] << 16) | (buffer[0] << 24);

  if(!IS_MPEG_AUDIO_HEADER(header))
    return 0;

  switch(header & MPEG_ID_MASK)
    {
    case MPEG_MPEG1:
      mpeg_version = VERSION_MPEG_1;
      break;
    case MPEG_MPEG2:
      mpeg_version = VERSION_MPEG_2;
      break;
    case MPEG_MPEG2_5:
      mpeg_version = VERSION_MPEG_25;
      break;
    }

  switch(header & MPEG_LAYER_MASK)
    {
    case MPEG_LAYER_I:
      layer = 1;
      break;
    case MPEG_LAYER_II:
      layer = 2;
      break;
    case MPEG_LAYER_III:
      layer = 3;
      break;
    }
  bitrate_index = (header & MPEG_BITRATE_MASK) >> 12;

  switch(mpeg_version)
    {
    case VERSION_MPEG_1:
      switch(layer)
          {
          case 1:
            bitrate = mpeg_bitrates[0][bitrate_index];
            break;
          case 2:
            bitrate = mpeg_bitrates[1][bitrate_index];
            break;
          case 3:
            bitrate = mpeg_bitrates[2][bitrate_index];
            break;
          }
        break;
      case VERSION_MPEG_2:
      case VERSION_MPEG_25:
        switch(layer)
          {
          case 1:
            bitrate = mpeg_bitrates[3][bitrate_index];
            break;
          case 2:
          case 3:
            bitrate = mpeg_bitrates[4][bitrate_index];
            break;
          }
        break;
    default: // This won't happen, but keeps gcc quiet
      break;
    }

  samplerate_index = (header & MPEG_FREQUENCY_MASK) >> 10;
  
  switch(mpeg_version)
    {
    case VERSION_MPEG_1:
      samplerate = mpeg_samplerates[0][samplerate_index];
      break;
    case VERSION_MPEG_2:
      samplerate = mpeg_samplerates[1][samplerate_index];
      break;
    case VERSION_MPEG_25:
      samplerate = mpeg_samplerates[2][samplerate_index];
      break;
    }
  
  if(layer == 1)
    {
    ret = (12 * bitrate / samplerate);
    if(header & MPEG_PAD_MASK)
      ret++;
    ret *= 4;
    }
  else
    {
    ret = 144 * bitrate / samplerate;
    if(header & MPEG_PAD_MASK)
      ret++;
    }
  
  if(mpeg_version == VERSION_MPEG_2)
    ret /= 2;
  
  // For mpeg 2.5, this is not tested
  
  else if(mpeg_version == VERSION_MPEG_25)
    ret /= 4;
  
  return ret;
  }

/* For ac3: Taken from a52dec */

static uint8_t ac3_halfrate[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3};

static int ac3_rate[] = { 32,  40,  48,  56,  64,  80,  96, 112,
                          128, 160, 192, 224, 256, 320, 384, 448,
                          512, 576, 640};

// static uint8_t ac3_lfeon[8] = {0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01};

static int get_frame_bytes_ac3(uint8_t * buf)
  {

  int frmsizecod;
  int bitrate;
  int half;
  //  int acmod;

  int bit_rate;
  //  int flags;
  int sample_rate;
  
  if ((buf[0] != 0x0b) || (buf[1] != 0x77))   /* syncword */
    return 0;
  
  if (buf[5] >= 0x60)         /* bsid >= 12 */
    return 0;
  half = ac3_halfrate[buf[5] >> 3];
  
  /* acmod, dsurmod and lfeon */
  //  acmod = buf[6] >> 5;
  //  flags = ((((buf[6] & 0xf8) == 0x50) ? A52_DOLBY : acmod) |
  //            ((buf[6] & lfeon[acmod]) ? A52_LFE : 0));
  
  frmsizecod = buf[4] & 63;
  if (frmsizecod >= 38)
    return 0;
  bitrate = ac3_rate [frmsizecod >> 1];
  bit_rate = (bitrate * 1000) >> half;
  
  switch (buf[4] & 0xc0)
    {
    case 0:
      sample_rate = 48000 >> half;
      return 4 * bitrate;
    case 0x40:
      sample_rate = 44100 >> half;
      return 2 * (320 * bitrate / 147 + (frmsizecod & 1));
    case 0x80:
      sample_rate = 32000 >> half;
    return 6 * bitrate;
    default:
      return 0;
    }
  }

static int chunk_len(quicktime_t *file, quicktime_ffmpeg_codec_t *codec,
                     longest offset, longest next_chunk)
  {
  int result = 0;
  unsigned char header[7];
  int accum = 0;
  
  while(offset < next_chunk)
    {
    quicktime_set_position(file, offset);
    result = !quicktime_read_data(file, (unsigned char*)&header, 7);
    
    if(result)
      {
      //      fprintf(stderr, "quicktime_read_data failed\n");
      return accum;
      }
    
// Decode size of frame

    if(codec->ffc_dec->id == CODEC_ID_AC3)  
      result = get_frame_bytes_ac3(header);
    else
      result = get_frame_bytes_mpeg(header);
    
// Invalid header
    if(!result) 
      {
      //      fprintf(stderr, "invalid header %d\n", accum);
      return accum;
      }
    
    else
      // Valid header
      {
      accum += result;
      offset += result;
      quicktime_set_position(file, offset + result);
      }
    }
  return accum;
  }

/* Read the next needed chunk into the chunk buffer */

#define END_BYTES 4

static int fill_chunk_buffer(quicktime_t *file, quicktime_ffmpeg_codec_t *codec,
                             quicktime_audio_map_t *track_map, longest current_chunk)
  {
  int result = 0;
  longest offset1, offset2;
      
  offset1 = quicktime_chunk_to_offset(track_map->track, current_chunk);
  offset2 = quicktime_chunk_to_offset(track_map->track, current_chunk + 1);
    
  codec->chunk_size = chunk_len(file, codec, offset1, offset2);

  //  fprintf(stderr, "fill_chunk_buffer: %d Bytes, diff: %d\n", codec->chunk_size,
  //          offset2 - offset1);
  
  if(codec->chunk_buffer_size < codec->chunk_size + END_BYTES)
    {
    codec->chunk_buffer = realloc(codec->chunk_buffer, codec->chunk_size + END_BYTES);
    codec->chunk_buffer_size = codec->chunk_size + END_BYTES;
    }

  quicktime_set_position(file, offset1);
  quicktime_read_data(file, codec->chunk_buffer, codec->chunk_size);

  codec->chunk_buffer_ptr = codec->chunk_buffer;
    
  return result;
  }

/* Decode one frame into the sample buffer */

static int decode_frame(quicktime_t *file, quicktime_ffmpeg_codec_t *codec,
                         quicktime_audio_map_t *track_map, longest sample)
  {
  int frame_bytes, result = 0;
  int frame_size;
  longest chunk_sample, current_chunk;
  
  if(!codec->chunk_buffer ||
     (codec->chunk_buffer_ptr >= codec->chunk_buffer + codec->chunk_size - 4))
    {
    quicktime_chunk_of_sample(&chunk_sample, &current_chunk, track_map->track,
                              sample);
    
    fill_chunk_buffer(file, codec, track_map, current_chunk);
    }
  //  else
  //    fprintf(stderr, "No new chunk needed\n");
  
  if(codec->ffc_dec->id == CODEC_ID_AC3)  
    frame_bytes = get_frame_bytes_ac3(codec->chunk_buffer_ptr);
  else
    frame_bytes = get_frame_bytes_mpeg(codec->chunk_buffer_ptr);
    
  result = avcodec_decode_audio(&(codec->ffcodec_dec), codec->decoded_frame, &frame_size,
                                 codec->chunk_buffer_ptr, frame_bytes + END_BYTES);
  codec->decoded_frame_pos = codec->samples_per_frame;

  //  fprintf(stderr,
  //          "Decode frame: %d bytes (from header) %d bytes uncompressed\n",
  //          frame_bytes, frame_size);
  
  codec->chunk_buffer_ptr += frame_bytes;
  return result;
  }

static int fill_sample_buffer(quicktime_t *file, quicktime_ffmpeg_codec_t *codec,
                               quicktime_audio_map_t *track_map, int samples, int channels)
  {
  /* Copy samples from last frame */

  int samples_done = 0;
  int result = 0;

  //  fprintf(stderr, "fill sample buffer %d %d... ", samples, codec->samples_per_frame);
  
  if(codec->decoded_frame_pos < codec->samples_per_frame)
    {
/*     fprintf(stderr, "Copy remaining %d samples\n", */
/*             codec->samples_per_frame - codec->decoded_frame_pos); */
    memcpy(codec->sample_buffer, &(codec->decoded_frame[codec->decoded_frame_pos * channels]),
           2 * channels * (codec->samples_per_frame - codec->decoded_frame_pos));
    samples_done += codec->samples_per_frame - codec->decoded_frame_pos;
    codec->decoded_frame_pos = codec->samples_per_frame;
    }

  while(samples_done < samples)
    {
    result = decode_frame(file, codec, track_map, track_map->current_position + samples_done);

    if(samples - samples_done < codec->samples_per_frame)
      {
      memcpy(&(codec->sample_buffer[channels * samples_done]),
             codec->decoded_frame,
             2 * channels * (samples - samples_done));

      codec->decoded_frame_pos = samples - samples_done;
      samples_done += samples - samples_done;
      }
    else
      {
      memcpy(&(codec->sample_buffer[channels * samples_done]),
             codec->decoded_frame,
             2 * channels * codec->samples_per_frame);
      samples_done += codec->samples_per_frame;
      }
    }

  codec->samples_in_buffer = samples_done;
  codec->sample_buffer_offset = track_map->current_position;

  //  fprintf(stderr, "done %d\n", codec->samples_in_buffer);
  
  return result;
  }

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
  longest chunk_sample, current_chunk; /* For seeking only */
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  int channels = file->atracks[track].channels;

  /* Initialize codec */
  
  if(!codec->init_dec)
    {
    memcpy(&(codec->ffcodec_enc), &(codec->params), sizeof(AVCodecContext));
          
    if(avcodec_open(&codec->ffcodec_dec, codec->ffc_dec) != 0)
      {
      fprintf(stderr, "Avcodec open failed\n");
      return -1;
      }
    
    codec->sample_buffer_offset = 0;
    codec->init_dec = 1;

    codec->decoded_frame = malloc(codec->samples_per_frame * 2 * channels);
    codec->decoded_frame_pos = codec->samples_per_frame;
    }

  /* Check whether to enlarge sample buffer */
 
  if(codec->sample_buffer_size < samples)
    {
    codec->sample_buffer_size = samples;
    codec->sample_buffer = realloc(codec->sample_buffer, samples * channels * 2);
    }
  
  /* Copy samples, we might have in the sample buffer */
  /* This is the case if we read the second channel */
  
  if((track_map->current_position == codec->sample_buffer_offset) &&
     (samples == codec->samples_in_buffer))
    {
    //    fprintf(stderr, "New channel\n");
    deinterleave(output_i, output_f, codec->sample_buffer,
                 channel, channels, samples);
    return result;
    }

  if(file->atracks[track].current_position !=
     codec->sample_buffer_offset + codec->samples_in_buffer)
    {
    /* Now, we must completely reset the stream position */

    /* 1st step: Get the proper chunk */
    
    quicktime_chunk_of_sample(&chunk_sample, &current_chunk, track_map->track,
                              file->atracks[track].current_position);
    //   fprintf(stderr, "Samples in chunk: %d\n", quicktime_chunk_samples(track_map->track, current_chunk));

    fill_chunk_buffer(file, codec, track_map, current_chunk);

    /* Next step: Skip unused frames at the beginning of the chunk */

    while((file->atracks[track].current_position - chunk_sample) / codec->samples_per_frame)
      {
      if(codec->ffc_dec->id == CODEC_ID_AC3)  
        codec->chunk_buffer_ptr += get_frame_bytes_ac3(codec->chunk_buffer_ptr);
      else
        codec->chunk_buffer_ptr += get_frame_bytes_mpeg(codec->chunk_buffer_ptr);
      chunk_sample += codec->samples_per_frame;
      }

    /* Last step: Skip samples in the decoded stream */

    if(file->atracks[track].current_position - chunk_sample)
      {
      decode_frame(file, codec, track_map, track_map->current_position);
      codec->sample_buffer_pos = file->atracks[track].current_position - chunk_sample;
      }
    
    }

  /* Read the next frames sequentially */
  
  result = fill_sample_buffer(file, codec, track_map, samples, channels);

  deinterleave(output_i, output_f, codec->sample_buffer,
               channel, channels, samples);

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
  int result = -1;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  quicktime_trak_t *trak = track_map->track;
  int channels = file->atracks[track].channels;
  longest offset;
  int size = 0;

  int interleave_length;
  int samples_done = 0;

  //  fprintf(stderr, "Samples %d\n", samples);
  
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
        offset = quicktime_position(file);
        result = quicktime_write_data(file, codec->write_buffer, size);
        result = (result)?0:1;
        quicktime_update_tables(file, 
                                track_map->track,
                                offset,
                                track_map->current_chunk,
                                0,
                                codec->samples_per_frame,
                                0);
        file->atracks[track].current_chunk++;
        codec->sample_buffer_pos = 0;
        }
      }
    
    }
  return result;
  }
