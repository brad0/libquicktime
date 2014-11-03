/*******************************************************************************
 audio.c

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

#include <stdlib.h>
#include <string.h>

#include "lqt_private.h"

#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

#define LOG_DOMAIN "audio"

/***************************************************
 * Audio conversion functions
 ***************************************************/

#define RECLIP(val, min, max) (val<min?min:((val>max)?max:val))

/* Conversion Macros */

/* int8 -> */

#define INT8_TO_INT16(src, dst) dst = src * 0x0101

#define INT8_TO_FLOAT(src, dst) dst = (float)src / 128.0

/* uint8 -> */

#define UINT8_TO_INT16(src, dst) dst = (src - 128) * 0x0101

#define UINT8_TO_FLOAT(src, dst) dst = (float)src / 127.0 - 1.0

/* int16 -> */

#define INT16_TO_INT8(src, dst) dst = src >> 8

#define INT16_TO_UINT8(src, dst) dst = (src ^ 0x8000) >> 8

#define INT16_TO_INT16(src, dst) dst = src

#define INT16_TO_INT32(src, dst) dst = src * 0x00010001

#define INT16_TO_FLOAT(src, dst) dst = (float)src / 32767.0f

#define INT16_TO_DOUBLE(src, dst) dst = (double)src / 32767.0

/* int32 -> */

#define INT32_TO_INT16(src, dst) dst = src >> 16

#define INT32_TO_FLOAT(src, dst) dst = (float)src / 2147483647.0

/* float -> */

#define FLOAT_TO_INT8(src, dst) tmp = (int)(src * 127.0); dst = RECLIP(tmp, -128, 127)

#define FLOAT_TO_UINT8(src, dst) tmp = (int)((src + 1.0) * 127.0); dst = RECLIP(tmp, 0, 255)

#define FLOAT_TO_INT16(src, dst) tmp = (int)((src) * 32767.0); dst = RECLIP(tmp, -32768, 32767)

#define FLOAT_TO_INT32(src, dst) tmp = (int64_t)((src) * 2147483647.0); dst = RECLIP(tmp, -2147483648LL, 2147483647LL)

#define FLOAT_TO_FLOAT(src, dst) dst = src

#define FLOAT_TO_DOUBLE(src, dst) dst = src


#define DOUBLE_TO_INT16(src, dst) tmp = (int)((src) * 32767.0); dst = RECLIP(tmp, -32768, 32767)

#define DOUBLE_TO_FLOAT(src, dst) dst = src


/* Encoding */

static void encode_int16_to_int8(int16_t ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j;
  int8_t * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((int8_t*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      INT16_TO_INT8(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

static void encode_int16_to_uint8(int16_t ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j;
  uint8_t * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((uint8_t*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      INT16_TO_UINT8(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

static void encode_int16_to_int16(int16_t ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j;
  int16_t * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((int16_t*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      INT16_TO_INT16(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

static void encode_int16_to_int32(int16_t ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j;
  int32_t * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((int32_t*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      INT16_TO_INT32(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

static void encode_int16_to_float(int16_t ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j;
  float * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((float*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      INT16_TO_FLOAT(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

static void encode_int16_to_double(int16_t ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j;
  double * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((double*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      INT16_TO_DOUBLE(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }


static void encode_float_to_int8(float ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j, tmp;
  int8_t * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((int8_t*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      FLOAT_TO_INT8(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

static void encode_float_to_uint8(float ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j, tmp;
  uint8_t * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((uint8_t*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      FLOAT_TO_UINT8(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

static void encode_float_to_int16(float ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j, tmp;
  int16_t * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((int16_t*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      FLOAT_TO_INT16(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

static void encode_float_to_int32(float ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j;
  int64_t tmp;
  int32_t * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((int32_t*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      FLOAT_TO_INT32(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

static void encode_float_to_float(float ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j;
  float * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((float*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      FLOAT_TO_FLOAT(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

static void encode_float_to_double(float ** in, void * _out, int num_channels, int num_samples)
  {
  int i, j;
  double * out;
  for(i = 0; i < num_channels; i++)
    {
    out = ((double*)_out) + i;
    for(j = 0; j < num_samples; j++)
      {
      FLOAT_TO_DOUBLE(in[i][j], (*out));
      out+=num_channels;
      }
    }
  }

void lqt_convert_audio_encode(quicktime_t * file, int16_t ** in_int, float ** in_float, void * out,
                              int num_channels, int num_samples,
                              lqt_sample_format_t stream_format)
  {
  switch(stream_format)
    {
    case LQT_SAMPLE_INT8:
      if(in_int)
        encode_int16_to_int8(in_int, out, num_channels, num_samples);
      else if(in_float)
        encode_float_to_int8(in_float, out, num_channels, num_samples);
      break;
    case LQT_SAMPLE_UINT8:
      if(in_int)
        encode_int16_to_uint8(in_int, out, num_channels, num_samples);
      else if(in_float)
        encode_float_to_uint8(in_float, out, num_channels, num_samples);
      break;
    case LQT_SAMPLE_INT16:
      if(in_int)
        encode_int16_to_int16(in_int, out, num_channels, num_samples);
      else if(in_float)
        encode_float_to_int16(in_float, out, num_channels, num_samples);
      break;
    case LQT_SAMPLE_INT32:
      if(in_int)
        encode_int16_to_int32(in_int, out, num_channels, num_samples);
      else if(in_float)
        encode_float_to_int32(in_float, out, num_channels, num_samples);
      break;
    case LQT_SAMPLE_FLOAT:
      if(in_int)
        encode_int16_to_float(in_int, out, num_channels, num_samples);
      else if(in_float)
        encode_float_to_float(in_float, out, num_channels, num_samples);
      break;
    case LQT_SAMPLE_DOUBLE:
      if(in_int)
        encode_int16_to_double(in_int, out, num_channels, num_samples);
      else if(in_float)
        encode_float_to_double(in_float, out, num_channels, num_samples);
      break;
    case LQT_SAMPLE_UNDEFINED:
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "Cannot encode samples: Stream format undefined");
      break;
    }
  }

/* Sample format */

static struct
  {
  lqt_sample_format_t format;
  const char * name;
  int size;
  }
sample_formats[] =
  {
    { LQT_SAMPLE_UNDEFINED, "Undefined",  0 }, /* If this is returned, we have an error */
    { LQT_SAMPLE_INT8, "8 bit signed",    1 },
    { LQT_SAMPLE_UINT8, "8 bit unsigned", 1 },
    { LQT_SAMPLE_INT16, "16 bit signed",  2 },
    { LQT_SAMPLE_INT32, "32 bit signed",  4 },
    { LQT_SAMPLE_FLOAT, "Floating point", sizeof(float)     }, /* Float is ALWAYS machine native */
    { LQT_SAMPLE_DOUBLE, "Double precision", sizeof(double) } /* Double is ALWAYS machine native */
  };

const char * lqt_sample_format_to_string(lqt_sample_format_t format)
  {
  int i;
  for(i = 0; i < sizeof(sample_formats)/sizeof(sample_formats[0]); i++)
    {
    if(sample_formats[i].format == format)
      return sample_formats[i].name;
    }
  return sample_formats[0].name;
  }

int lqt_sample_format_bytes(lqt_sample_format_t format)
  {
  int i;
  for(i = 0; i < sizeof(sample_formats)/sizeof(sample_formats[0]); i++)
    {
    if(sample_formats[i].format == format)
      return sample_formats[i].size;
    }
  return 0;
  }

void lqt_audio_buffer_alloc(lqt_audio_buffer_t * buf, int num_samples, int num_channels, int planar,
                            lqt_sample_format_t fmt)
  {
  int bytes, i;

  if(!buf->channels)
    buf->channels = calloc(num_channels, sizeof(*buf->channels));
  
  if(buf->alloc >= num_samples)
    return;
  
  bytes = lqt_sample_format_bytes(fmt) * num_samples;
  
  if(planar)
    {
    for(i = 0; i < num_channels; i++)
      buf->channels[i].u_8 = realloc(buf->channels[i].u_8, bytes);
    }
  else
    buf->channels[0].u_8 = realloc(buf->channels[0].u_8, bytes * num_channels);
  }

/* En- decode functions */

/* Decode raw samples */

int lqt_decode_audio_raw(quicktime_t *file,  void * output, long samples, int track)
  {
  int result;
  quicktime_audio_map_t * atrack;
  int samples_read = 0;
  int samples_to_copy, bytes_to_copy;
  uint8_t * output_ptr = output;
  int sample_size;

  atrack = &file->atracks[track];

  sample_size = lqt_sample_format_bytes(atrack->sample_format);
  
  if(atrack->eof)
    return 0;
  
  while(samples_read < samples)
    {
    if(!atrack->buf.size || (atrack->buf.pos >= atrack->buf.size))
      {
      atrack->buf.pos = 0;
      result = atrack->codec->decode_audio_packet(file, track, &atrack->buf);
      if(!result)
        break;
      }

    samples_to_copy = samples - samples_read;

    if(samples_to_copy > atrack->buf.size - atrack->buf.pos)
      samples_to_copy = atrack->buf.size - atrack->buf.pos;

    if(atrack->planar)
      {
      int i, j;
      /* Interleave */

      for(i = 0; i < samples_to_copy; i++)
        {
        for(j = 0; j < atrack->channels; j++)
          {
          memcpy(output_ptr, atrack->buf.channels[j].u_8 + atrack->buf.pos * sample_size,
                 sample_size);
          output_ptr += sample_size;
          }
        atrack->buf.pos++;
        }          
      }
    else
      {
      bytes_to_copy = samples_to_copy * sample_size * atrack->channels;
      memcpy(output_ptr,
             atrack->buf.channels[0].u_8 + atrack->buf.pos * sample_size * atrack->channels,
             bytes_to_copy);
      output_ptr += bytes_to_copy;
      atrack->buf.pos += samples_to_copy;
      }
    samples_read += samples_to_copy;
    }
  atrack->current_position += samples_read;
    
  if(samples_read < samples)
    atrack->eof = 1;
  return samples_read;
  }

int lqt_encode_audio_raw(quicktime_t *file,  void * input, long samples, int track)
  {
  int result;
  quicktime_audio_map_t * atrack;
  if(!samples)
    return 0;
  atrack = &file->atracks[track];
  lqt_start_encoding(file);
  file->atracks[track].current_position += samples;
  result = atrack->codec->encode_audio(file, input, samples, track);
  if(file->io_error)
    return 0;
  else
    return samples;
  }

/* Compatibility function for old decoding API */

static void dec_int8_to_int16(void * _in, void * _out,
                              int num_samples, int advance)
  {
  int i;
  int8_t * in = _in;
  int16_t * out = _out;
  
  for(i = 0; i < num_samples; i++)
    {
    INT8_TO_INT16((*in), *out);
    in += advance;
    out++;
    }
  }

static void dec_uint8_to_int16(void * _in, void * _out,
                              int num_samples, int advance)
  {
  int i;
  uint8_t * in = _in;
  int16_t * out = _out;
  
  for(i = 0; i < num_samples; i++)
    {
    UINT8_TO_INT16((*in), *out);
    in += advance;
    out++;
    }
  }

static void dec_int16_to_int16(void * _in, void * _out,
                               int num_samples, int advance)
  {
  int i;
  int16_t * in = _in;
  int16_t * out = _out;
  
  for(i = 0; i < num_samples; i++)
    {
    INT16_TO_INT16((*in), *out);
    in += advance;
    out++;
    }
  }

static void dec_int32_to_int16(void * _in, void * _out,
                               int num_samples, int advance)
  {
  int i;
  int32_t * in = _in;
  int16_t * out = _out;
  
  for(i = 0; i < num_samples; i++)
    {
    INT32_TO_INT16((*in), *out);
    in += advance;
    out++;
    }
  }

static void dec_float_to_int16(void * _in, void * _out,
                               int num_samples, int advance)
  {
  int i;
  float * in = _in;
  int16_t * out = _out;
  int tmp;
  for(i = 0; i < num_samples; i++)
    {
    FLOAT_TO_INT16((*in), *out);
    in += advance;
    out++;
    }
  }

static void dec_double_to_int16(void * _in, void * _out,
                                int num_samples, int advance)
  {
  int i;
  double * in = _in;
  int16_t * out = _out;
  int tmp;

  for(i = 0; i < num_samples; i++)
    {
    DOUBLE_TO_INT16((*in), *out);
    in += advance;
    out++;
    }
  }


static void dec_int8_to_float(void * _in, void * _out,
                              int num_samples, int advance)
  {
  int i;
  int8_t * in = _in;
  float * out = _out;
  
  for(i = 0; i < num_samples; i++)
    {
    INT8_TO_FLOAT((*in), *out);
    in += advance;
    out++;
    }
  }

static void dec_uint8_to_float(void * _in, void * _out,
                              int num_samples, int advance)
  {
  int i;
  uint8_t * in = _in;
  float * out = _out;
  
  for(i = 0; i < num_samples; i++)
    {
    UINT8_TO_FLOAT((*in), *out);
    in += advance;
    out++;
    }
  }

static void dec_int16_to_float(void * _in, void * _out,
                               int num_samples, int advance)
  {
  int i;
  int16_t * in = _in;
  float * out = _out;
  
  for(i = 0; i < num_samples; i++)
    {
    INT16_TO_FLOAT((*in), *out);
    in += advance;
    out++;
    }
  }

static void dec_int32_to_float(void * _in, void * _out,
                               int num_samples, int advance)
  {
  int i;
  int32_t * in = _in;
  float * out = _out;
  
  for(i = 0; i < num_samples; i++)
    {
    INT32_TO_FLOAT((*in), *out);
    in += advance;
    out++;
    }
  }

static void dec_float_to_float(void * _in, void * _out,
                               int num_samples, int advance)
  {
  int i;
  float * in = _in;
  float * out = _out;
  
  for(i = 0; i < num_samples; i++)
    {
    FLOAT_TO_FLOAT((*in), *out);
    in += advance;
    out++;
    }
  }

static void dec_double_to_float(void * _in, void * _out,
                                int num_samples, int advance)
  {
  int i;
  double * in = _in;
  float * out = _out;
  
  for(i = 0; i < num_samples; i++)
    {
    DOUBLE_TO_FLOAT((*in), *out);
    in += advance;
    out++;
    }
  }


static int decode_audio_old(quicktime_t *file, 
                            int16_t ** output_i, 
                            float ** output_f, 
                            long samples, 
                            int track)
  {
  int result;
  quicktime_audio_map_t * atrack;
  
  int i;
  int samples_read = 0;
  int samples_to_copy;
  uint8_t * src;
  void (*convert)(void*, void*, int, int);
  int sample_size;
  atrack = &file->atracks[track];

  if(atrack->eof)
    return 0;

  sample_size = lqt_sample_format_bytes(atrack->sample_format);
    
  if(atrack->sample_format == LQT_SAMPLE_UNDEFINED)
    atrack->codec->decode_audio_packet(file, track, NULL);

    
  /* Find converter function */

  switch(atrack->sample_format)
    {
    case LQT_SAMPLE_INT8:
      if(output_i)
        convert = dec_int8_to_int16;
      else if(output_f)
        convert = dec_int8_to_float;
      break;
    case LQT_SAMPLE_UINT8:
      if(output_i)
        convert = dec_uint8_to_int16;
      else if(output_f)
        convert = dec_uint8_to_float;
      break;
    case LQT_SAMPLE_INT16:
      if(output_i)
        convert = dec_int16_to_int16;
      else if(output_f)
        convert = dec_int16_to_float;
      break;
    case LQT_SAMPLE_INT32:
      if(output_i)
        convert = dec_int32_to_int16;
      else if(output_f)
        convert = dec_int32_to_float;
      break;
    case LQT_SAMPLE_FLOAT: /* Float is ALWAYS machine native */
      if(output_i)
        convert = dec_float_to_int16;
      else if(output_f)
        convert = dec_float_to_float;
      break;
    case LQT_SAMPLE_DOUBLE: /* Float is ALWAYS machine native */
      if(output_i)
        convert = dec_double_to_int16;
      else if(output_f)
        convert = dec_double_to_float;
      break;
    case LQT_SAMPLE_UNDEFINED:
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "Cannot decode samples: Stream format undefined");
      break;
    }
    
  while(samples_read < samples)
    {
    if(!atrack->buf.size || (atrack->buf.pos >= atrack->buf.size))
      {
      atrack->buf.pos = 0;
      result = atrack->codec->decode_audio_packet(file, track, &atrack->buf);
      if(!result)
        break;
      }

    samples_to_copy = samples - samples_read;

    if(samples_to_copy > atrack->buf.size - atrack->buf.pos)
      samples_to_copy = atrack->buf.size - atrack->buf.pos;

    if(atrack->planar)
      {
      for(i = 0; i < atrack->channels; i++)
        {
        src = atrack->buf.channels[i].u_8 + sample_size * atrack->buf.pos;
        if(output_i)
          convert(src, output_i[i] + samples_read, samples_to_copy, 1);
        else if(output_f)
          convert(src, output_f[i] + samples_read, samples_to_copy, 1);
        }
      }
    else
      {
      for(i = 0; i < atrack->channels; i++)
        {
        src = atrack->buf.channels[0].u_8 + sample_size * (atrack->buf.pos * atrack->channels);
          
        if(output_i && output_i[i])
          convert(src, output_i[i] + samples_read, samples_to_copy, atrack->channels);
        else if(output_f && output_f[i])
          convert(src, output_f[i] + samples_read, samples_to_copy, atrack->channels);
        }
      }
    atrack->buf.pos += samples_to_copy;
    samples_read += samples_to_copy;
    }

  if(samples_read < samples)
    atrack->eof = 1;
  atrack->current_position += samples_read;
  return samples_read;
  }

int quicktime_decode_audio(quicktime_t *file, 
                           int16_t *output_i, 
                           float *output_f, 
                           long samples, 
                           int channel)
  {
  float   ** channels_f;
  int16_t ** channels_i;

  int quicktime_track, quicktime_channel;
  int result = 1;

  quicktime_channel_location(file, &quicktime_track,
                             &quicktime_channel, channel);

  if(file->atracks[quicktime_track].eof)
    return 1;
        
  if(output_i)
    {
    channels_i = calloc(quicktime_track_channels(file, quicktime_track), sizeof(*channels_i));
    channels_i[quicktime_channel] = output_i;
    }
  else
    channels_i = NULL;
        
  if(output_f)
    {
    channels_f = calloc(quicktime_track_channels(file, quicktime_track), sizeof(*channels_f));
    channels_f[quicktime_channel] = output_f;
    }
  else
    channels_f = NULL;
        
  result = decode_audio_old(file, channels_i, channels_f, samples, quicktime_track);
  file->atracks[quicktime_track].current_position += result;

  if(channels_i)
    free(channels_i);
  else if(channels_f)
    free(channels_f);
  return ((result < 0) ? 1 : 0);
  }

/*
 * Same as quicktime_decode_audio, but it grabs all channels at
 * once. Or if you want only some channels you can leave the channels
 * you don't want = NULL in the poutput array. The poutput arrays
 * must contain at least lqt_total_channels(file) elements.
 */

int lqt_decode_audio(quicktime_t *file, 
                     int16_t **poutput_i, 
                     float **poutput_f, 
                     long samples)
  {
  int result = 1;
  int i = 0;
  int16_t **output_i;
  float   **output_f;

  int total_tracks = quicktime_audio_tracks(file);
  int track_channels;

  if(poutput_i)
    output_i = poutput_i;
  else
    output_i = (int16_t**)0;

  if(poutput_f)
    output_f = poutput_f;
  else
    output_f = (float**)0;
        
  for( i=0; i < total_tracks; i++ )
    {
    track_channels = quicktime_track_channels(file, i);

    if(file->atracks[i].eof)
      return 1;
                    
    result = decode_audio_old(file, output_i, output_f, samples, i);
    if(output_f)
      output_f += track_channels;
    if(output_i)
      output_i += track_channels;

    //  file->atracks[i].current_position += samples;
    }
  return result;
  }

int lqt_decode_audio_track(quicktime_t *file, 
                           int16_t **poutput_i, 
                           float **poutput_f, 
                           long samples,
                           int track)
  {
  int result = 1;

  if(file->atracks[track].eof)
    return 1;

  result = !decode_audio_old(file, poutput_i, poutput_f, samples, track);
  
  // file->atracks[track].current_position += samples;
  
  return result;
  }

static int encode_audio_old(quicktime_t *file, 
                            int16_t **input_i, 
                            float **input_f, 
                            long samples,
                            int track)
  {
  quicktime_audio_map_t * atrack;
  atrack = &file->atracks[track];
  lqt_start_encoding(file);
  
  if(!samples)
    return 0;
  
  if(atrack->sample_format == LQT_SAMPLE_UNDEFINED)
    atrack->codec->encode_audio(file, (void*)0, 0, track);
  
  /* (Re)allocate sample buffer */

  if(atrack->sample_buffer_alloc < samples)
    {
    atrack->sample_buffer_alloc = samples + 1024;
    atrack->sample_buffer = realloc(atrack->sample_buffer,
                                    atrack->sample_buffer_alloc *
                                    atrack->channels *
                                    lqt_sample_format_bytes(atrack->sample_format));
    }

  /* Convert */

  lqt_convert_audio_encode(file, input_i, input_f, atrack->sample_buffer, atrack->channels, samples,
                           atrack->sample_format);

  /* Encode */

  file->atracks[track].current_position += samples;
  
  return atrack->codec->encode_audio(file, atrack->sample_buffer, samples, track);
  
  }

int lqt_encode_audio_track(quicktime_t *file, 
                           int16_t **input_i, 
                           float **input_f, 
                           long samples,
                           int track)
  {
  int result = 1;

  result = encode_audio_old(file, input_i, input_f, samples, track);
  return result;
  
  }

int quicktime_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f, long samples)
  {
  return lqt_encode_audio_track(file, input_i, input_f, samples, 0);
  }

int quicktime_set_audio_position(quicktime_t *file, int64_t sample, int track)
  {
  quicktime_audio_map_t * atrack;
  if((track < 0) || (track >= file->total_atracks))
    {
    lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN,
            "quicktime_set_audio_position: No track %d", track);
    return 1;
    }

  atrack = file->atracks + track;

  /* Clear EOF */
  atrack->eof = 0;
  
  atrack->track->idx_pos = lqt_packet_index_seek(&atrack->track->idx, sample);
  
  if(atrack->preroll)
    {
    int64_t seek_sample = atrack->track->idx.entries[atrack->track->idx_pos].pts - atrack->preroll;
    if(seek_sample < 0)
      seek_sample = 0;
    atrack->track->idx_pos = lqt_packet_index_seek(&atrack->track->idx, seek_sample);
    }

  /* Reset the decoder */
  if(atrack->codec->resync)
    atrack->codec->resync(file, track);
  
  atrack->current_position = atrack->track->idx.entries[atrack->track->idx_pos].pts;

  /* Skip to the desired sample */
  while(1)
    {
    if(!atrack->codec->decode_audio_packet(file, track, &atrack->buf))
      break; // Unexpected EOF

    if(file->atracks[track].current_position + atrack->buf.size > sample)
      {
      atrack->buf.pos = sample - file->atracks[track].current_position;
      file->atracks[track].current_position += atrack->buf.pos;
      return 0;
      }
    file->atracks[track].current_position += atrack->buf.size;
    }
  return 1;
  }

long quicktime_audio_position(quicktime_t *file, int track)
  {
  return file->atracks[track].current_position;
  }

int lqt_total_channels(quicktime_t *file)
  {
  int i = 0, total_channels = 0;
  for( i=0; i < file->total_atracks; i++ )
    total_channels += file->atracks[i].channels;
  
  return total_channels;
  }

int quicktime_has_audio(quicktime_t *file)
  {
  if(quicktime_audio_tracks(file)) return 1;
  return 0;
  }

long quicktime_sample_rate(quicktime_t *file, int track)
  {
  if(file->total_atracks)
    return file->atracks[track].samplerate;
  return 0;
  }

int quicktime_audio_bits(quicktime_t *file, int track)
  {
  if(file->total_atracks)
    return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].sample_size;

  return 0;
  }

char* quicktime_audio_compressor(quicktime_t *file, int track)
  {
  return file->atracks[track].track->mdia.minf.stbl.stsd.table[0].format;
  }

int quicktime_track_channels(quicktime_t *file, int track)
  {
  if(track < file->total_atracks)
    return file->atracks[track].channels;

  return 0;
  }

int quicktime_channel_location(quicktime_t *file,
                               int *quicktime_track,
                               int *quicktime_channel, int channel)
  {
  int current_channel = 0, current_track = 0;
  *quicktime_channel = 0;
  *quicktime_track = 0;
  for(current_channel = 0, current_track = 0; current_track < file->total_atracks; )
    {
    if(channel >= current_channel)
      {
      *quicktime_channel = channel - current_channel;
      *quicktime_track = current_track;
      }

    current_channel += file->atracks[current_track].channels;
    current_track++;
    }
  return 0;
  }

lqt_sample_format_t lqt_get_sample_format(quicktime_t * file, int track)
  {
  quicktime_audio_map_t * atrack;

  if(track < 0 || track > file->total_atracks)
    return LQT_SAMPLE_UNDEFINED;

  atrack = &file->atracks[track];

/* atrack->codec can be NULL if the codec is unknown as in 'drms' */
  if(atrack->sample_format == LQT_SAMPLE_UNDEFINED && atrack->codec != NULL)
    {
    if(file->wr)
      {
      atrack->codec->encode_audio(file, (void*)0, 0, track);
      }
    else
      {
      atrack->codec->decode_audio_packet(file, track, NULL);
      }
    }
  
  return atrack->sample_format;
  }

void lqt_init_vbr_audio(quicktime_t * file, int track)
  {
  quicktime_trak_t * trak = file->atracks[track].track;
  trak->mdia.minf.stbl.stsd.table[0].compression_id = -2;
  trak->mdia.minf.stbl.stsz.sample_size = 0;
  trak->mdia.minf.is_audio_vbr = 1;

  if(trak->strl)
    {
    trak->strl->strh.dwRate = quicktime_sample_rate(file, track);
    trak->strl->strh.dwScale = 0;
    trak->strl->strh.dwSampleSize = 0;
    
    trak->strl->strf.wf.f.WAVEFORMAT.nBlockAlign = 0;
    trak->strl->strf.wf.f.WAVEFORMAT.nAvgBytesPerSec =  18120; // Probably doesn't matter
    trak->strl->strf.wf.f.PCMWAVEFORMAT.wBitsPerSample = 0;
    }
  }

LQT_EXTERN void lqt_set_audio_bitrate(quicktime_t * file, int track, int bitrate)
  {
  quicktime_trak_t * trak = file->atracks[track].track;

  if(!trak->strl)
    return;
  
  /* strh stuff */
  trak->strl->strh.dwRate = bitrate / 8;
  trak->strl->strh.dwScale = 1;
  trak->strl->strh.dwSampleSize = 1;
  /* WAVEFORMATEX stuff */
  
  trak->strl->strf.wf.f.WAVEFORMAT.nBlockAlign = 1;
  trak->strl->strf.wf.f.WAVEFORMAT.nAvgBytesPerSec =  bitrate / 8;
  trak->strl->strf.wf.f.PCMWAVEFORMAT.wBitsPerSample = 0;
  }


void lqt_start_audio_vbr_frame(quicktime_t * file, int track)
  {
  quicktime_audio_map_t * atrack = &file->atracks[track];
  
  /* Make chunk at maximum 10 VBR packets large */
  if((file->write_trak == atrack->track) &&
     (atrack->track->chunk_samples >= 10))
    {
    quicktime_write_chunk_footer(file, file->write_trak);
    quicktime_write_chunk_header(file, atrack->track);
    }
  atrack->vbr_frame_start = quicktime_position(file);
  
  }

void lqt_finish_audio_vbr_frame(quicktime_t * file, int track, int num_samples)
  {
  int size;
  quicktime_stsz_t * stsz;
  quicktime_stts_t * stts;
  long vbr_frames_written;
  quicktime_audio_map_t * atrack = &file->atracks[track];
  
  stsz = &atrack->track->mdia.minf.stbl.stsz;
  stts = &atrack->track->mdia.minf.stbl.stts;

  vbr_frames_written = stsz->total_entries;
  
  /* Update stsz */

  size = quicktime_position(file) - atrack->vbr_frame_start;
  
  quicktime_update_stsz(stsz, vbr_frames_written, size);
  
  if(atrack->track->strl)
    {
    quicktime_strl_t * strl = atrack->track->strl;
    
    if(size > strl->strf.wf.f.WAVEFORMAT.nBlockAlign)
      strl->strf.wf.f.WAVEFORMAT.nBlockAlign = size;

    if(!strl->strh.dwScale)
      strl->strh.dwScale = num_samples;

    strl->strh.dwLength++;
    }
  
  /* Update stts */
  
  quicktime_update_stts(stts, vbr_frames_written, num_samples);
  
  atrack->track->chunk_samples++;
  }

/* Check if VBR reading should be enabled */

int lqt_audio_is_vbr(quicktime_t * file, int track)
  {
  return file->atracks[track].track->mdia.minf.is_audio_vbr;
  }

long quicktime_audio_length(quicktime_t *file, int track)
  {
  long ret;
  if(track < 0 || track > file->total_atracks)
    return -1;
  ret = file->atracks[track].track->idx.max_pts;
  return ret;
  }

int64_t lqt_last_audio_position(quicktime_t * file, int track)
  {
  return file->atracks[track].current_position;
  }

int quicktime_audio_tracks(quicktime_t *file)
  {
  int i, result = 0;
  quicktime_minf_t *minf;
  for(i = 0; i < file->moov.total_tracks; i++)
    {
    minf = &file->moov.trak[i]->mdia.minf;
    if(minf->is_audio)
      result++;
    }
  return result;
  }

int lqt_set_audio_codec(quicktime_t *file, int track,
                        lqt_codec_info_t * info)
  {
  quicktime_stsd_t * stsd;
  
  stsd = &file->atracks[track].track->mdia.minf.stbl.stsd;
  
  quicktime_stsd_set_audio_codec(stsd, info->fourccs[0]);
  
  quicktime_init_audio_map(file, &file->atracks[track],
                           file->wr,
                           info);
  lqt_set_default_audio_parameters(file, track);
  return 0;
  }

int lqt_add_audio_track_internal(quicktime_t *file,
                                 int channels, long sample_rate, int bits,
                                 lqt_codec_info_t * codec_info,
                                 const lqt_compression_info_t * ci)
  {
  quicktime_trak_t *trak;
  char * compressor = codec_info ? codec_info->fourccs[0] : NULL;
  
  file->atracks = realloc(file->atracks,
                          (file->total_atracks+1)*sizeof(*file->atracks));

  memset(&file->atracks[file->total_atracks], 0, sizeof(*file->atracks));
  
  if(ci)
    lqt_compression_info_copy(&file->atracks[file->total_atracks].ci, ci);
  
  trak = quicktime_add_track(file);
  quicktime_trak_init_audio(file, trak, channels,
                            sample_rate, bits, compressor);
  file->atracks[file->total_atracks].track = trak;
  
  file->total_atracks++;
  
  if(codec_info)
    return lqt_set_audio_codec(file, file->total_atracks-1,
                               codec_info);
  return 0;
  }

int lqt_add_audio_track(quicktime_t *file,
                        int channels, long sample_rate, int bits,
                        lqt_codec_info_t * codec_info)
  {
  return lqt_add_audio_track_internal(file,
                                      channels,
                                      sample_rate,
                                      bits,
                                      codec_info, NULL);
  }

int lqt_set_audio(quicktime_t *file, int channels,
                  long sample_rate,  int bits,
                  lqt_codec_info_t * codec_info)
  {
  lqt_add_audio_track(file, channels, sample_rate, bits, codec_info);
  return 0;
  }

int quicktime_set_audio(quicktime_t *file, 
                        int channels,
                        long sample_rate,
                        int bits,
                        char *compressor)
  {
  lqt_codec_info_t ** info;
  info = lqt_find_audio_codec(compressor, 1);
  lqt_set_audio(file, channels, sample_rate, bits, *info);
  lqt_destroy_codec_info(info);
  return 1;   /* Return the number of tracks created */
  }

void lqt_set_audio_io_mode(quicktime_t *file, int track, lqt_track_io_mode_t mode)
  {
  if(file->atracks[track].io_mode == mode)
    return;
  }

/* Called by codecs */
void lqt_audio_set_sbr(quicktime_audio_map_t *atrack)
  {
  int i;
  if(atrack->ci.flags & LQT_COMPRESSION_SBR)
    return;
  
  for(i = 0; i < atrack->track->idx.num_entries; i++)
    {
    atrack->track->idx.entries[i].pts *= 2;
    atrack->track->idx.entries[i].dts *= 2;
    atrack->track->idx.entries[i].duration *= 2;
    }
  atrack->track->idx.min_packet_duration *= 2; // Minimum packet duration
  atrack->track->idx.max_packet_duration *= 2; // Maximum packet duration
  atrack->track->idx.max_pts *= 2;
  atrack->samplerate *= 2;
  
  atrack->ci.flags |= LQT_COMPRESSION_SBR;
  }

