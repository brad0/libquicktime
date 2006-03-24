#include <config.h>
#include "funcprotos.h"
#include <quicktime/quicktime.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

#include <string.h>
#include <stdlib.h>
#include <math.h>


#include "pcm.h"

#include "ulaw_tables.h"
#include "alaw_tables.h"

#ifndef HAVE_LRINT
#define lrint(x) ((long int)(x))
#endif

typedef struct quicktime_pcm_codec_s
  {
  uint8_t * chunk_buffer;
  uint8_t * chunk_buffer_ptr;
  int chunk_buffer_size;
  int chunk_buffer_alloc;
    
  int block_align;

  int sample_buffer_size;
  int last_chunk_samples;

  void (*encode)(struct quicktime_pcm_codec_s*, int num_samples, void * input);
  void (*decode)(struct quicktime_pcm_codec_s*, int num_samples, void ** output);

  int initialized;
  } quicktime_pcm_codec_t;

/* 8 bit per sample, signedness and endian neutral */

static void encode_8(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  memcpy(codec->chunk_buffer_ptr, _input, num_samples);
  }

static void decode_8(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  uint8_t * output = (uint8_t *)(*_output);

  memcpy(output, codec->chunk_buffer_ptr, num_samples);
  codec->chunk_buffer_ptr += num_samples;

  output += num_samples;
  *_output = output;
  }

/* 16 bit per sample, without swapping */

static void encode_s16(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  memcpy(codec->chunk_buffer_ptr, _input, 2 * num_samples);
  }

static void decode_s16(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  uint8_t * output = (uint8_t *)(*_output);

  memcpy(output, codec->chunk_buffer_ptr, 2 * num_samples);
  codec->chunk_buffer_ptr += 2 * num_samples;
  
  output += 2 * num_samples;
  *_output = output;
  }

/* 16 bit per sample with swapping */

static void encode_s16_swap(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  uint8_t * input = (uint8_t*)_input;

  for(i = 0; i < num_samples; i++)
    {
    codec->chunk_buffer_ptr[0] = input[1];
    codec->chunk_buffer_ptr[1] = input[0];
    codec->chunk_buffer_ptr+=2;
    input+=2;
    }
  }

static void decode_s16_swap(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  int i;
  uint8_t * output = (uint8_t*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    output[0] = codec->chunk_buffer_ptr[1];
    output[1] = codec->chunk_buffer_ptr[0];
    codec->chunk_buffer_ptr+=2;
    output+=2;
    }
  *_output = output;
  }

/* 24 bit per sample (Big Endian) */

static void encode_s24_be(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  /* The uint32_t is intentional: Interpreting integers as unsigned has less pitfalls */
  uint32_t * input = (uint32_t*)_input;

  for(i = 0; i < num_samples; i++)
    {
    codec->chunk_buffer_ptr[0] = (*input & 0xff000000) >> 24;
    codec->chunk_buffer_ptr[1] = (*input & 0xff0000) >> 16;
    codec->chunk_buffer_ptr[2] = (*input & 0xff00) >> 8;
    codec->chunk_buffer_ptr+=3;
    input++;
    }
  
  }

static void decode_s24_be(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  int i;
  /* The uint32_t is intentional: Interpreting integers as unsigned has less pitfalls */
  uint32_t * output = (uint32_t*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    *output  = (uint32_t)(codec->chunk_buffer_ptr[0]) << 24;
    *output |= (uint32_t)(codec->chunk_buffer_ptr[1]) << 16;
    *output |= (uint32_t)(codec->chunk_buffer_ptr[2]) <<  8;
    codec->chunk_buffer_ptr+=3;
    output++;
    }
  *_output = output;
  }

/* 24 bit per sample (Little Endian) */

static void encode_s24_le(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  /* The uint32_t is intentional: Interpreting integers as unsigned has less pitfalls */
  uint32_t * input = (uint32_t*)_input;

  for(i = 0; i < num_samples; i++)
    {
    codec->chunk_buffer_ptr[2] = (*input & 0xff000000) >> 24;
    codec->chunk_buffer_ptr[1] = (*input & 0xff0000) >> 16;
    codec->chunk_buffer_ptr[0] = (*input & 0xff00) >> 8;
    codec->chunk_buffer_ptr+=3;
    input++;
    }

  }

static void decode_s24_le(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  int i;
  /* The uint32_t is intentional: Interpreting integers as unsigned has less pitfalls */
  uint32_t * output = (uint32_t*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    *output  = (uint32_t)(codec->chunk_buffer_ptr[2]) << 24;
    *output |= (uint32_t)(codec->chunk_buffer_ptr[1]) << 16;
    *output |= (uint32_t)(codec->chunk_buffer_ptr[0]) <<  8;
    codec->chunk_buffer_ptr+=3;
    output++;
    }
  *_output = output;
  }


/* 32 bit per sample, without swapping */

static void encode_s32(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  memcpy(codec->chunk_buffer_ptr, _input, 4 * num_samples);
  }

static void decode_s32(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  uint8_t * output = (uint8_t *)(*_output);

  memcpy(output, codec->chunk_buffer_ptr, 4 * num_samples);
  codec->chunk_buffer_ptr += 4 * num_samples;
  
  output += 4 * num_samples;
  *_output = output;
  }

/* 32 bit per sample with swapping */

static void encode_s32_swap(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  uint8_t * input = (uint8_t*)_input;

  for(i = 0; i < num_samples; i++)
    {
    codec->chunk_buffer_ptr[0] = input[3];
    codec->chunk_buffer_ptr[1] = input[2];
    codec->chunk_buffer_ptr[2] = input[1];
    codec->chunk_buffer_ptr[3] = input[0];
    codec->chunk_buffer_ptr+=4;
    input+=4;
    }
  }

static void decode_s32_swap(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  int i;
  uint8_t * output = (uint8_t*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    output[0] = codec->chunk_buffer_ptr[3];
    output[1] = codec->chunk_buffer_ptr[2];
    output[2] = codec->chunk_buffer_ptr[1];
    output[3] = codec->chunk_buffer_ptr[0];
    codec->chunk_buffer_ptr+=4;
    output+=4;
    }
  *_output = output;
  }

/* Floating point formats */

/* Sample read/write functions, taken from libsndfile */

static float
float32_be_read (unsigned char *cptr)
{       int             exponent, mantissa, negative ;
        float   fvalue ;

        negative = cptr [0] & 0x80 ;
        exponent = ((cptr [0] & 0x7F) << 1) | ((cptr [1] & 0x80) ? 1 : 0) ;
        mantissa = ((cptr [1] & 0x7F) << 16) | (cptr [2] << 8) | (cptr [3]) ;

        if (! (exponent || mantissa))
                return 0.0 ;

        mantissa |= 0x800000 ;
        exponent = exponent ? exponent - 127 : 0 ;

        fvalue = mantissa ? ((float) mantissa) / ((float) 0x800000) : 0.0 ;

        if (negative)
                fvalue *= -1 ;

        if (exponent > 0)
                fvalue *= (1 << exponent) ;
        else if (exponent < 0)
                fvalue /= (1 << abs (exponent)) ;

        return fvalue ;
} /* float32_be_read */

static float
float32_le_read (unsigned char *cptr)
{       int             exponent, mantissa, negative ;
        float   fvalue ;

        negative = cptr [3] & 0x80 ;
        exponent = ((cptr [3] & 0x7F) << 1) | ((cptr [2] & 0x80) ? 1 : 0) ;
        mantissa = ((cptr [2] & 0x7F) << 16) | (cptr [1] << 8) | (cptr [0]) ;

        if (! (exponent || mantissa))
                return 0.0 ;

        mantissa |= 0x800000 ;
        exponent = exponent ? exponent - 127 : 0 ;

        fvalue = mantissa ? ((float) mantissa) / ((float) 0x800000) : 0.0 ;

        if (negative)
                fvalue *= -1 ;

        if (exponent > 0)
                fvalue *= (1 << exponent) ;
        else if (exponent < 0)
                fvalue /= (1 << abs (exponent)) ;

        return fvalue ;
} /* float32_le_read */

static double
double64_be_read (unsigned char *cptr)
{       int             exponent, negative ;
        double  dvalue ;

        negative = (cptr [0] & 0x80) ? 1 : 0 ;
        exponent = ((cptr [0] & 0x7F) << 4) | ((cptr [1] >> 4) & 0xF) ;

        /* Might not have a 64 bit long, so load the mantissa into a double. */
        dvalue = (((cptr [1] & 0xF) << 24) | (cptr [2] << 16) | (cptr [3] << 8) | cptr [4]) ;
        dvalue += ((cptr [5] << 16) | (cptr [6] << 8) | cptr [7]) / ((double) 0x1000000) ;

        if (exponent == 0 && dvalue == 0.0)
                return 0.0 ;

        dvalue += 0x10000000 ;

        exponent = exponent - 0x3FF ;

        dvalue = dvalue / ((double) 0x10000000) ;

        if (negative)
                dvalue *= -1 ;

        if (exponent > 0)
                dvalue *= (1 << exponent) ;
        else if (exponent < 0)
                dvalue /= (1 << abs (exponent)) ;

        return dvalue ;
} /* double64_be_read */

static double
double64_le_read (unsigned char *cptr)
{       int             exponent, negative ;
        double  dvalue ;

        negative = (cptr [7] & 0x80) ? 1 : 0 ;
        exponent = ((cptr [7] & 0x7F) << 4) | ((cptr [6] >> 4) & 0xF) ;

        /* Might not have a 64 bit long, so load the mantissa into a double. */
        dvalue = (((cptr [6] & 0xF) << 24) | (cptr [5] << 16) | (cptr [4] << 8) | cptr [3]) ;
        dvalue += ((cptr [2] << 16) | (cptr [1] << 8) | cptr [0]) / ((double) 0x1000000) ;

        if (exponent == 0 && dvalue == 0.0)
                return 0.0 ;

        dvalue += 0x10000000 ;

        exponent = exponent - 0x3FF ;

        dvalue = dvalue / ((double) 0x10000000) ;

        if (negative)
                dvalue *= -1 ;

        if (exponent > 0)
                dvalue *= (1 << exponent) ;
        else if (exponent < 0)
                dvalue /= (1 << abs (exponent)) ;

        return dvalue ;
} /* double64_le_read */

void
float32_le_write (float in, unsigned char *out)
{       int             exponent, mantissa, negative = 0 ;

        memset (out, 0, sizeof (int)) ;

        if (in == 0.0)
                return ;

        if (in < 0.0)
        {       in *= -1.0 ;
                negative = 1 ;
                } ;

        in = frexp (in, &exponent) ;

        exponent += 126 ;

        in *= (float) 0x1000000 ;
        mantissa = (((int) in) & 0x7FFFFF) ;

        if (negative)
                out [3] |= 0x80 ;

        if (exponent & 0x01)
                out [2] |= 0x80 ;

        out [0] = mantissa & 0xFF ;
        out [1] = (mantissa >> 8) & 0xFF ;
        out [2] |= (mantissa >> 16) & 0x7F ;
        out [3] |= (exponent >> 1) & 0x7F ;

        return ;
} /* float32_le_write */

void
float32_be_write (float in, unsigned char *out)
{       int             exponent, mantissa, negative = 0 ;

        memset (out, 0, sizeof (int)) ;

        if (in == 0.0)
                return ;

        if (in < 0.0)
        {       in *= -1.0 ;
                negative = 1 ;
                } ;

        in = frexp (in, &exponent) ;

        exponent += 126 ;

        in *= (float) 0x1000000 ;
        mantissa = (((int) in) & 0x7FFFFF) ;

        if (negative)
                out [0] |= 0x80 ;

        if (exponent & 0x01)
                out [1] |= 0x80 ;

        out [3] = mantissa & 0xFF ;
        out [2] = (mantissa >> 8) & 0xFF ;
        out [1] |= (mantissa >> 16) & 0x7F ;
        out [0] |= (exponent >> 1) & 0x7F ;

        return ;
} /* float32_be_write */


void
double64_be_write (double in, unsigned char *out)
{       int             exponent, mantissa ;

        memset (out, 0, sizeof (double)) ;

        if (in == 0.0)
                return ;

        if (in < 0.0)
        {       in *= -1.0 ;
                out [0] |= 0x80 ;
                } ;

        in = frexp (in, &exponent) ;

        exponent += 1022 ;

        out [0] |= (exponent >> 4) & 0x7F ;
        out [1] |= (exponent << 4) & 0xF0 ;

        in *= 0x20000000 ;
        mantissa = lrint (floor (in)) ;

        out [1] |= (mantissa >> 24) & 0xF ;
        out [2] = (mantissa >> 16) & 0xFF ;
        out [3] = (mantissa >> 8) & 0xFF ;
        out [4] = mantissa & 0xFF ;

        in = fmod (in, 1.0) ;
        in *= 0x1000000 ;
        mantissa = lrint (floor (in)) ;

        out [5] = (mantissa >> 16) & 0xFF ;
        out [6] = (mantissa >> 8) & 0xFF ;
        out [7] = mantissa & 0xFF ;

        return ;
} /* double64_be_write */

void
double64_le_write (double in, unsigned char *out)
{       int             exponent, mantissa ;

        memset (out, 0, sizeof (double)) ;

        if (in == 0.0)
                return ;

        if (in < 0.0)
        {       in *= -1.0 ;
                out [7] |= 0x80 ;
                } ;

        in = frexp (in, &exponent) ;

        exponent += 1022 ;

        out [7] |= (exponent >> 4) & 0x7F ;
        out [6] |= (exponent << 4) & 0xF0 ;

        in *= 0x20000000 ;
        mantissa = lrint (floor (in)) ;

        out [6] |= (mantissa >> 24) & 0xF ;
        out [5] = (mantissa >> 16) & 0xFF ;
        out [4] = (mantissa >> 8) & 0xFF ;
        out [3] = mantissa & 0xFF ;

        in = fmod (in, 1.0) ;
        in *= 0x1000000 ;
        mantissa = lrint (floor (in)) ;

        out [2] = (mantissa >> 16) & 0xFF ;
        out [1] = (mantissa >> 8) & 0xFF ;
        out [0] = mantissa & 0xFF ;

        return ;
} /* double64_le_write */

/* 32 bit float (Big Endian) */

static void encode_fl32_be(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  float * input = (float*)_input;

  for(i = 0; i < num_samples; i++)
    {
    float32_be_write(*input, codec->chunk_buffer_ptr);
    codec->chunk_buffer_ptr+=4;
    input++;
    }
  
  }

static void decode_fl32_be(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  int i;
  float * output = (float*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    *output = float32_be_read(codec->chunk_buffer_ptr);
    codec->chunk_buffer_ptr+=4;
    output++;
    }
  *_output = output;
  }

/* 32 bit float (Little Endian) */

static void encode_fl32_le(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  float * input = (float*)_input;

  for(i = 0; i < num_samples; i++)
    {
    float32_le_write(*input, codec->chunk_buffer_ptr);
    codec->chunk_buffer_ptr+=4;
    input++;
    }

  }

static void decode_fl32_le(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  int i;
  float * output = (float*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    *output = float32_le_read(codec->chunk_buffer_ptr);
    codec->chunk_buffer_ptr+=4;
    output++;
    }
  *_output = output;
  }

/* 64 bit float (Big Endian) */

static void encode_fl64_be(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  float * input = (float*)_input;

  for(i = 0; i < num_samples; i++)
    {
    double64_be_write(*input, codec->chunk_buffer_ptr);
    codec->chunk_buffer_ptr+=8;
    input++;
    }
  
  }

static void decode_fl64_be(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  int i;
  float * output = (float*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    *output = double64_be_read(codec->chunk_buffer_ptr);
    codec->chunk_buffer_ptr+=8;
    output++;
    }
  *_output = output;
  }

/* 64 bit float (Little Endian) */

static void encode_fl64_le(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  float * input = (float*)_input;

  for(i = 0; i < num_samples; i++)
    {
    double64_le_write(*input, codec->chunk_buffer_ptr);
    codec->chunk_buffer_ptr+=8;
    input++;
    }

  }

static void decode_fl64_le(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  int i;
  float * output = (float*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    *output = double64_le_read(codec->chunk_buffer_ptr);
    codec->chunk_buffer_ptr+=8;
    output++;
    }
  *_output = output;
  }



/* ulaw */

/* See ulaw_tables.h for the tables references here */

#define ENCODE_ULAW(src, dst) if(src >= 0) dst = ulaw_encode[src / 4]; else dst = 0x7F & ulaw_encode[src / -4] 
#define DECODE_ULAW(src, dst) dst = ulaw_decode [src]

static void encode_ulaw(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  int16_t * input = (int16_t*)_input;
  
  for(i = 0; i < num_samples; i++)
    {
    ENCODE_ULAW(input[0], codec->chunk_buffer_ptr[0]);
    codec->chunk_buffer_ptr++;
    input++;
    }
  }

static void decode_ulaw(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  int i;
  /* The uint32_t is intentional: Interpreting integers as unsigned has less pitfalls */
  int16_t * output = (int16_t*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    DECODE_ULAW(codec->chunk_buffer_ptr[0], output[0]);
    codec->chunk_buffer_ptr++;
    output++;
    }
  *_output = output;
  }


/* alaw */

/* See alaw_tables.h for the tables references here */

#define ENCODE_ALAW(src, dst) if(src >= 0) dst = alaw_encode[src / 16]; else dst = 0x7F & alaw_encode[src / -16] 
#define DECODE_ALAW(src, dst) dst = alaw_decode [src]

static void encode_alaw(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  int16_t * input = (int16_t*)_input;
  
  for(i = 0; i < num_samples; i++)
    {
    ENCODE_ALAW(input[0], codec->chunk_buffer_ptr[0]);
    codec->chunk_buffer_ptr++;
    input++;
    }
  }

static void decode_alaw(quicktime_pcm_codec_t*codec, int num_samples, void ** _output)
  {
  int i;
  /* The uint32_t is intentional: Interpreting integers as unsigned has less pitfalls */
  int16_t * output = (int16_t*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    DECODE_ALAW(codec->chunk_buffer_ptr[0], output[0]);
    codec->chunk_buffer_ptr++;
    output++;
    }
  *_output = output;
  }


/* Generic decode function */

static int read_audio_chunk(quicktime_t * file, int track,
                            long chunk,
                            uint8_t ** buffer, int * buffer_alloc)
  {
  int bytes, samples, bytes_from_samples;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_pcm_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

  bytes = lqt_read_audio_chunk(file, track, chunk, buffer, buffer_alloc, &samples);
  //  fprintf(stderr, "Chunk: %d, samples: %d %d\n", chunk, samples, bytes);
  bytes_from_samples = samples * codec->block_align;
  if(bytes > bytes_from_samples)
    return bytes_from_samples;
  else
    return bytes;
  }

static int decode_pcm(quicktime_t *file, void * _output, long samples, int track)
  {
  int64_t chunk, chunk_sample;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_pcm_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  void * output;
  int64_t samples_to_skip = 0;
  int samples_in_chunk;
  int samples_decoded, samples_to_decode;

  if(!_output) /* Global initialization */
    {
    return 0;
    }
  
  if(!codec->initialized)
    {
    /* Read the first audio chunk */

    codec->chunk_buffer_size = read_audio_chunk(file,
                                                track, file->atracks[track].current_chunk,
                                                &(codec->chunk_buffer),
                                                &(codec->chunk_buffer_alloc));
    if(codec->chunk_buffer_size <= 0)
      return 0;
    codec->chunk_buffer_ptr = codec->chunk_buffer;
    codec->initialized = 1;
    }
  
  if(file->atracks[track].current_position != file->atracks[track].last_position)
    {
    /* Seeking happened */
    quicktime_chunk_of_sample(&chunk_sample, &chunk,
                              file->atracks[track].track,
                              file->atracks[track].current_position);

    /* Read the chunk */
    
    if(file->atracks[track].current_chunk != chunk)
      {
      file->atracks[track].current_chunk = chunk;
      codec->chunk_buffer_size = read_audio_chunk(file,
                                                  track, file->atracks[track].current_chunk,
                                                  &(codec->chunk_buffer),
                                                  &(codec->chunk_buffer_alloc));
      if(codec->chunk_buffer_size <= 0)
        return 0;
      
      codec->chunk_buffer_ptr = codec->chunk_buffer;
      }
    else
      {
      codec->chunk_buffer_ptr = codec->chunk_buffer;
      }
    
    /* Skip samples */
    
    samples_to_skip = file->atracks[track].current_position - chunk_sample;
    if(samples_to_skip < 0)
      {
      fprintf(stderr, "pcm: Cannot skip backwards\n");
      samples_to_skip = 0;
      }
    codec->chunk_buffer_ptr = codec->chunk_buffer + samples_to_skip * codec->block_align;
    }

  samples_decoded = 0;

  output = _output;
  
  while(samples_decoded < samples)
    {
    /* Get new chunk if necessary */
    if(codec->chunk_buffer_ptr - codec->chunk_buffer >= codec->chunk_buffer_size)
      {
      file->atracks[track].current_chunk++;
      codec->chunk_buffer_size = read_audio_chunk(file,
                                                  track, file->atracks[track].current_chunk,
                                                  &(codec->chunk_buffer),
                                                  &(codec->chunk_buffer_alloc));
      if(codec->chunk_buffer_size <= 0)
        break;
      codec->chunk_buffer_ptr = codec->chunk_buffer;
      }

    /* Decode */

    samples_in_chunk = ((codec->chunk_buffer_size-(int)(codec->chunk_buffer_ptr - codec->chunk_buffer)))
      /codec->block_align;
    
    samples_to_decode = samples - samples_decoded;
    
    if(samples_to_decode > samples_in_chunk)
      samples_to_decode = samples_in_chunk;
    codec->decode(codec, samples_to_decode * track_map->channels, &output);
    samples_decoded += samples_to_decode;
    }
  file->atracks[track].last_position = file->atracks[track].current_position + samples_decoded;
  return samples_decoded;
  }

/* Generic encode function */

static int encode_pcm(quicktime_t *file, void * input, long samples, int track)
  {
  int result;
  quicktime_atom_t chunk_atom;
  
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_pcm_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  quicktime_trak_t *trak = track_map->track;
  
  /* Allocate chunk buffer */

  if(codec->chunk_buffer_alloc < samples * codec->block_align)
    {
    codec->chunk_buffer_alloc = samples * codec->block_align + 1024;
    codec->chunk_buffer = realloc(codec->chunk_buffer, codec->chunk_buffer_alloc);
    }

  codec->chunk_buffer_ptr = codec->chunk_buffer;
  codec->encode(codec, samples * track_map->channels, input);

  quicktime_write_chunk_header(file, trak, &chunk_atom);
  result = quicktime_write_data(file, codec->chunk_buffer, samples * codec->block_align);
  quicktime_write_chunk_footer(file, trak, track_map->current_chunk, &chunk_atom, samples);
  file->atracks[track].current_chunk++;		
  
  /* defeat fwrite's return */
  if(result) 
    result = 0; 
  else 
    result = 1; 
  
  
  return result;
  }

/* Destructor */

static int delete_pcm(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;
  if(codec->chunk_buffer)
    {
    free(codec->chunk_buffer);
    }
  free(codec);
  return 0;
  }


void quicktime_init_codec_twos(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->decode_audio = decode_pcm;
  codec_base->encode_audio = encode_pcm;
  
  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;
  
  switch(atrack->track->mdia.minf.stbl.stsd.table[0].sample_size)
    {
    case 8:
      codec->block_align = atrack->channels;
      atrack->sample_format = LQT_SAMPLE_INT8;
      codec->encode = encode_8;
      codec->decode = decode_8;
      break;
    case 16:
      codec->block_align = 2 * atrack->channels;
      atrack->sample_format = LQT_SAMPLE_INT16;
#ifdef WORDS_BIGENDIAN
      codec->encode = encode_s16;
      codec->decode = decode_s16;
#else
      codec->encode = encode_s16_swap;
      codec->decode = decode_s16_swap;
#endif
      break;
    case 24:
      codec->block_align = 3 * atrack->channels;
      atrack->sample_format = LQT_SAMPLE_INT32;
      codec->encode = encode_s24_be;
      codec->decode = decode_s24_be;
      break;
    }
  }


void quicktime_init_codec_sowt(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->decode_audio = decode_pcm;
  codec_base->encode_audio = encode_pcm;
  //  codec_base->wav_id = 0x01;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;
  
  switch(atrack->track->mdia.minf.stbl.stsd.table[0].sample_size)
    {
    case 8:
      codec->block_align = atrack->channels;
      atrack->sample_format = LQT_SAMPLE_UINT8;
      codec->encode = encode_8;
      codec->decode = decode_8;
      break;
    case 16:
      codec->block_align = 2 * atrack->channels;
      atrack->sample_format = LQT_SAMPLE_INT16;
#ifdef WORDS_BIGENDIAN
      codec->encode = encode_s16_swap;
      codec->decode = decode_s16_swap;
#else
      codec->encode = encode_s16;
      codec->decode = decode_s16;
#endif
      break;
    case 24:
      codec->block_align = 3 * atrack->channels;
      atrack->sample_format = LQT_SAMPLE_INT32;
      codec->encode = encode_s24_le;
      codec->decode = decode_s24_le;
      break;
    }
  }

/* The following ones are for ENCODING */

void quicktime_init_codec_in24_little(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_stsd_table_t *table = &(atrack->track->mdia.minf.stbl.stsd.table[0]);
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->encode_audio = encode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;
  quicktime_set_enda(atrack->track, 1);
  
  codec->block_align = 3 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_INT32;
  codec->encode = encode_s24_le;
  /* Initialize version 1 stsd */
  quicktime_set_stsd_audio_v1(table, 1, 3, 3 * atrack->channels, 2);

  /* Set frma and enda */
  quicktime_set_frma(atrack->track, "in24");
  quicktime_set_enda(atrack->track, 1);
  
  }

void quicktime_init_codec_in24_big(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_stsd_table_t *table = &(atrack->track->mdia.minf.stbl.stsd.table[0]);
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->encode_audio = encode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;

  codec->block_align = 3 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_INT32;
  codec->encode = encode_s24_be;
  /* Initialize version 1 stsd */
  quicktime_set_stsd_audio_v1(table, 1, 3, 3 * atrack->channels, 2);
  }

/* The following one is for DECODING */

void quicktime_init_codec_in24(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->decode_audio = decode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;

  codec->block_align = 3 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_INT32;

  if(quicktime_get_enda(atrack->track))
    codec->decode = decode_s24_le;
  else
    codec->decode = decode_s24_be;
  }

/* The following ones are for ENCODING */

void quicktime_init_codec_in32_little(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_stsd_table_t *table = &(atrack->track->mdia.minf.stbl.stsd.table[0]);
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->encode_audio = encode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;
  quicktime_set_enda(atrack->track, 1);
  
  codec->block_align = 4 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_INT32;
#ifdef WORDS_BIGENDIAN
  codec->encode = encode_s32_swap;
#else
  codec->encode = encode_s32;
#endif
  /* Initialize version 1 stsd */
  quicktime_set_stsd_audio_v1(table, 1, 4, 4 * atrack->channels, 2);

  /* Set frma and enda */
  quicktime_set_frma(atrack->track, "in32");
  quicktime_set_enda(atrack->track, 1);
  }

void quicktime_init_codec_in32_big(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_stsd_table_t *table = &(atrack->track->mdia.minf.stbl.stsd.table[0]);
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->encode_audio = encode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;

  codec->block_align = 4 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_INT32;
#ifdef WORDS_BIGENDIAN
  codec->encode = encode_s32;
#else
  codec->encode = encode_s32_swap;
#endif
  /* Initialize version 1 stsd */
  quicktime_set_stsd_audio_v1(table, 1, 4, 4 * atrack->channels, 2);
  }

/* The following one is for DECODING */

void quicktime_init_codec_in32(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->decode_audio = decode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;

  codec->block_align = 4 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_INT32;

#ifdef WORDS_BIGENDIAN
  if(quicktime_get_enda(atrack->track))
    codec->decode = decode_s32_swap;
  else
    codec->decode = decode_s32;
#else
  if(quicktime_get_enda(atrack->track))
    codec->decode = decode_s32;
  else
    codec->decode = decode_s32_swap;
#endif
  }

/* Floating point */

/* The following ones are for ENCODING */

void quicktime_init_codec_fl32_little(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_stsd_table_t *table = &(atrack->track->mdia.minf.stbl.stsd.table[0]);
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->encode_audio = encode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;
  quicktime_set_enda(atrack->track, 1);
  
  codec->block_align = 4 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_FLOAT;
  codec->encode = encode_fl32_le;
  /* Initialize version 1 stsd */
  quicktime_set_stsd_audio_v1(table, 1, 4, 4 * atrack->channels, 2);

  /* Set frma and enda */
  quicktime_set_frma(atrack->track, "fl32");
  quicktime_set_enda(atrack->track, 1);

  }

void quicktime_init_codec_fl32_big(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_stsd_table_t *table = &(atrack->track->mdia.minf.stbl.stsd.table[0]);
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->encode_audio = encode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;

  codec->block_align = 4 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_FLOAT;

  codec->encode = encode_fl32_be;
  /* Initialize version 1 stsd */
  quicktime_set_stsd_audio_v1(table, 1, 4, 4 * atrack->channels, 2);
  }

/* The following one is for DECODING */

void quicktime_init_codec_fl32(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->decode_audio = decode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;

  codec->block_align = 4 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_FLOAT;

  if(quicktime_get_enda(atrack->track))
    codec->decode = decode_fl32_le;
  else
    codec->decode = decode_fl32_be;
  }

/* The following ones are for ENCODING */

void quicktime_init_codec_fl64_little(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_stsd_table_t *table = &(atrack->track->mdia.minf.stbl.stsd.table[0]);
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->encode_audio = encode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;
  quicktime_set_enda(atrack->track, 1);
  
  codec->block_align = 8 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_FLOAT;
  codec->encode = encode_fl64_le;

  /* Initialize version 1 stsd */
  quicktime_set_stsd_audio_v1(table, 1, 8, 8 * atrack->channels, 2);

  /* Set frma and enda */
  quicktime_set_frma(atrack->track, "fl64");
  quicktime_set_enda(atrack->track, 1);
  }

void quicktime_init_codec_fl64_big(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_stsd_table_t *table = &(atrack->track->mdia.minf.stbl.stsd.table[0]);
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->encode_audio = encode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;

  codec->block_align = 8 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_FLOAT;

  codec->encode = encode_fl64_be;

  /* Initialize version 1 stsd */
  quicktime_set_stsd_audio_v1(table, 1, 8, 8 * atrack->channels, 2);
  
  }

/* The following one is for DECODING */

void quicktime_init_codec_fl64(quicktime_audio_map_t *atrack)
  {
  quicktime_pcm_codec_t *codec;
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->decode_audio = decode_pcm;

  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;

  codec->block_align = 8 * atrack->channels;
  atrack->sample_format = LQT_SAMPLE_FLOAT;

  if(quicktime_get_enda(atrack->track))
    codec->decode = decode_fl64_le;
  else
    codec->decode = decode_fl64_be;
  }


/* raw */

void quicktime_init_codec_rawaudio(quicktime_audio_map_t *atrack)
  {
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_pcm_codec_t *codec;
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->decode_audio = decode_pcm;
  codec_base->encode_audio = encode_pcm;
  
  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;

  switch(atrack->track->mdia.minf.stbl.stsd.table[0].sample_size)
    {
    case 8:
      codec->block_align = atrack->channels;
      atrack->sample_format = LQT_SAMPLE_INT8;
      codec->encode = encode_8;
      codec->decode = decode_8;
      break;
    case 16:
      codec->block_align = 2 * atrack->channels;
      atrack->sample_format = LQT_SAMPLE_INT16;
#ifdef WORDS_BIGENDIAN
      codec->encode = encode_s16;
      codec->decode = decode_s16;
#else
      codec->encode = encode_s16_swap;
      codec->decode = decode_s16_swap;
#endif
      break;
    case 24:
      codec->block_align = 3 * atrack->channels;
      atrack->sample_format = LQT_SAMPLE_INT32;
      codec->encode = encode_s24_le;
      codec->decode = decode_s24_le;
      break;
    }
  }

void quicktime_init_codec_ulaw(quicktime_audio_map_t *atrack)
  {
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_pcm_codec_t *codec;
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->decode_video = 0;
  codec_base->encode_video = 0;
  codec_base->decode_audio = decode_pcm;
  codec_base->encode_audio = encode_pcm;
  
  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;
  
  codec->block_align = atrack->channels;
  codec->encode = encode_ulaw;
  codec->decode = decode_ulaw;
  atrack->sample_format = LQT_SAMPLE_INT16;
  }

void quicktime_init_codec_alaw(quicktime_audio_map_t *atrack)
  {
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_pcm_codec_t *codec;
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->decode_video = 0;
  codec_base->encode_video = 0;
  codec_base->decode_audio = decode_pcm;
  codec_base->encode_audio = encode_pcm;
  
  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;
  
  codec->block_align = atrack->channels;
  codec->encode = encode_alaw;
  codec->decode = decode_alaw;
  atrack->sample_format = LQT_SAMPLE_INT16;
  }
