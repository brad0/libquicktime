#include <config.h>
#include "funcprotos.h"
#include <quicktime/quicktime.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

#include <string.h>
#include <stdlib.h>


#include "pcm.h"

#include "ulaw_tables.h"

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

/* ulaw */

/* See ulaw_tables.h for the tables references here */

#define ENCODE_ULAW(src, dst) if(src >= 0) dst = ulaw_encode[src / 4]; else dst = 0x7F & ulaw_encode[src / -4] 
#define DECODE_ULAW(src, dst) dst = ulaw_decode [src]

static void encode_ulaw(quicktime_pcm_codec_t*codec, int num_samples, void * _input)
  {
  int i;
  int16_t * input = (uint16_t*)_input;
  
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
  int16_t * output = (uint16_t*)(*_output);
  
  for(i = 0; i < num_samples; i++)
    {
    DECODE_ULAW(codec->chunk_buffer_ptr[0], output[0]);
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
  /* defeat fwrite's return */
  if(result) 
    result = 0; 
  else 
    result = 1; 
  
  file->atracks[track].current_chunk++;		
  
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
  codec_base->fourcc = QUICKTIME_TWOS;
  codec_base->title = "Twos complement";
  codec_base->desc = "Twos complement";

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
  codec_base->fourcc = "SOWT";
  codec_base->title = "Twos complement (Little endian)";
  codec_base->desc = "Twos complement (Little endian)";
  codec_base->wav_id = 0x01;

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

void quicktime_init_codec_rawaudio(quicktime_audio_map_t *atrack)
  {
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_pcm_codec_t *codec;
  
  /* Init public items */
  codec_base->delete_acodec = delete_pcm;
  codec_base->decode_audio = decode_pcm;
  codec_base->encode_audio = encode_pcm;
  codec_base->fourcc = QUICKTIME_RAW;
  codec_base->title = "8 bit unsigned";
  codec_base->desc = "8 bit unsigned for video";
  codec_base->wav_id = 0x01;
  
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
  codec_base->fourcc = QUICKTIME_ULAW;
  codec_base->title = "uLaw";
  codec_base->desc = "uLaw";
  codec_base->wav_id = 0x07;
  
  /* Init private items */
  codec = calloc(1, sizeof(*codec));
  codec_base->priv = codec;
  
  codec->block_align = atrack->channels;
  codec->encode = encode_ulaw;
  codec->decode = decode_ulaw;
  atrack->sample_format = LQT_SAMPLE_INT16;
  }
