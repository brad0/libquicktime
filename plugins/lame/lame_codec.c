#include "funcprotos.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <lame/lame.h>
#include <quicktime/quicktime.h>

/*
 *  We decode all headers to figure out, how many COMPLETE frames we have encoded.
 */

/* The following code was ported from gmerlin_avdecoder (http://gmerlin.sourceforge.net) */

/* MPEG Audio header parsing code */

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

/* Header detection stolen from the mpg123 plugin of xmms */

static int header_check(uint32_t head)
{
        if ((head & 0xffe00000) != 0xffe00000)
                return 0;
        if (!((head >> 17) & 3))
                return 0;
        if (((head >> 12) & 0xf) == 0xf)
                return 0;
        if (!((head >> 12) & 0xf))
                return 0;
        if (((head >> 10) & 0x3) == 0x3)
                return 0;
        if (((head >> 19) & 1) == 1 &&
            ((head >> 17) & 3) == 3 &&
            ((head >> 16) & 1) == 1)
                return 0;
        if ((head & 0xffff0000) == 0xfffe0000)
                return 0;
        //        fprintf(stderr, "Head check ok %08x\n", head);
        return 1;
}

typedef enum
  {
    MPEG_VERSION_NONE = 0,
    MPEG_VERSION_1 = 1,
    MPEG_VERSION_2 = 2,
    MPEG_VERSION_2_5
  } mpeg_version_t;

#define CHANNEL_STEREO   0
#define CHANNEL_JSTEREO  1
#define CHANNEL_DUAL     2
#define CHANNEL_MONO     3

typedef struct
  {
  mpeg_version_t version;
  int layer;
  int bitrate;    /* -1: VBR */
  int samplerate;
  int frame_bytes;
  int channel_mode;
  int mode;
  int samples_per_frame;
  } mpeg_header;

static int decode_header(mpeg_header * h, uint8_t * ptr)
  {
  uint32_t header;
  int index;
  /* For calculation of the byte length of a frame */
  int pad;
  int slots_per_frame;
  h->frame_bytes = 0;
  header =
    ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24);
  if(!header_check(header))
    return 0;
  index = (header & MPEG_MODE_MASK) >> 6;
  switch(index)
    {
    case 0:
      h->channel_mode = CHANNEL_STEREO;
      break;
    case 1:
      h->channel_mode = CHANNEL_JSTEREO;
      break;
    case 2:
      h->channel_mode = CHANNEL_DUAL;
      break;
    case 3:
      h->channel_mode = CHANNEL_MONO;
      break;
    }
  /* Get Version */
  switch(header & MPEG_ID_MASK)
    {
    case MPEG_MPEG1:
      h->version = MPEG_VERSION_1;
        break;
    case MPEG_MPEG2:
      h->version = MPEG_VERSION_2;
      break;
    case MPEG_MPEG2_5:
      h->version = MPEG_VERSION_2_5;
      break;
    default:
      return 0;
    }
  /* Get Layer */
  switch(header & MPEG_LAYER_MASK)
    {
    case MPEG_LAYER_I:
      h->layer = 1;
      break;
    case MPEG_LAYER_II:
      h->layer = 2;
      break;
    case MPEG_LAYER_III:
      h->layer = 3;
      break;
    }
  index = (header & MPEG_BITRATE_MASK) >> 12;
  switch(h->version)
    {
    case MPEG_VERSION_1:
      switch(h->layer)
        {
        case 1:
          h->bitrate = mpeg_bitrates[0][index];
          break;
        case 2:
          h->bitrate = mpeg_bitrates[1][index];
          break;
        case 3:
          h->bitrate = mpeg_bitrates[2][index];
          break;
        }
      break;
    case MPEG_VERSION_2:
    case MPEG_VERSION_2_5:
      switch(h->layer)
        {
        case 1:
          h->bitrate = mpeg_bitrates[3][index];
          break;
        case 2:
        case 3:
          h->bitrate = mpeg_bitrates[4][index];
          break;
        }
      break;
    default: // This won't happen, but keeps gcc quiet
      return 0;
    }
  index = (header & MPEG_FREQUENCY_MASK) >> 10;
  switch(h->version)
    {
    case MPEG_VERSION_1:
      h->samplerate = mpeg_samplerates[0][index];
      break;
    case MPEG_VERSION_2:
      h->samplerate = mpeg_samplerates[1][index];
      break;
    case MPEG_VERSION_2_5:
      h->samplerate = mpeg_samplerates[2][index];
      break;
    default: // This won't happen, but keeps gcc quiet
      return 0;
    }
  pad = (header & MPEG_PAD_MASK) ? 1 : 0;
  if(h->layer == 1)
    {
    h->frame_bytes = ((12 * h->bitrate / h->samplerate) + pad) * 4;
    }
  else
    {
    slots_per_frame = ((h->layer == 3) &&
      ((h->version == MPEG_VERSION_2) ||
       (h->version == MPEG_VERSION_2_5))) ? 72 : 144;
    h->frame_bytes = (slots_per_frame * h->bitrate) / h->samplerate + pad;
    }
  // h->mode = (ptr[3] >> 6) & 3;
  h->samples_per_frame =
    (h->layer == 1) ? LAYER_I_SAMPLES : LAYER_II_III_SAMPLES;
  if(h->version != MPEG_VERSION_1)
    h->samples_per_frame /= 2;
  //  dump_header(h);
  return 1;
  }



typedef struct
  {
  // mp3 encoder
  lame_global_flags *lame_global;
  //        mpeg3_layer_t *encoded_header;
  int encode_initialized;
  float **input;
  int input_size;
  int input_allocated;
  int bitrate;
  int quality;
  
  unsigned char *encoder_output;
  int encoder_output_alloc;
  int encoder_output_size;
  
  int samples_per_frame;
  int stereo;

  /* Floating point input must be scaled to +/- 32768 */
  
  float * input_buffer[2];
  int input_buffer_alloc;
  
  } quicktime_mp3_codec_t;

static int delete_codec(quicktime_audio_map_t *atrack)
  {
  quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;
  //  if(codec->mp3) mpeg3_delete_layer(codec->mp3);
  //  if(codec->mp3_header)  mpeg3_delete_layer(codec->mp3_header);
  //  if(codec->packet_buffer) free(codec->packet_buffer);
  /*   if(codec->output) */
  /*     { */
  /*     int i; */
  /*     for(i = 0; i < atrack->channels; i++) */
  /*       free(codec->output[i]); */
  /*     free(codec->output); */
  /*     } */

  
  if(codec->lame_global)
    {
    lame_close(codec->lame_global);
    }
  if(codec->input)
    {
    int i;
    for(i = 0; i < atrack->channels; i++)
      {
      free(codec->input[i]);
      }
    free(codec->input);
    }

  if(codec->input_buffer[0])
    free(codec->input_buffer[0]);
  if(codec->input_buffer[1])
    free(codec->input_buffer[1]);
  
  
  if(codec->encoder_output)
    free(codec->encoder_output);
  //  if(codec->encoded_header)  mpeg3_delete_layer(codec->encoded_header);
  free(codec);
  return 0;
  }

static int write_data(quicktime_t *file, quicktime_audio_map_t *track_map,
                       quicktime_mp3_codec_t *codec)
  {
  mpeg_header h;
  quicktime_atom_t chunk_atom;
  int result;
  /* We write each mp3 frame into an own chunk. This increases the chance,
     that M$ software will be able to play our files */
  while(codec->encoder_output_size > 4)
    {
    if(!decode_header(&h, codec->encoder_output))
      {
      fprintf(stderr, "Ouch: lame created non mp3 data\n");
      break;
      }

    if(codec->encoder_output_size >= h.frame_bytes)
      {
      quicktime_write_chunk_header(file, track_map->track, &chunk_atom);
      result = !quicktime_write_data(file, codec->encoder_output, h.frame_bytes);
      quicktime_write_chunk_footer(file, 
                                   track_map->track, 
                                   track_map->current_chunk,
                                   &chunk_atom, 
                                   h.samples_per_frame);
      track_map->current_chunk++;
#if 0        
      fprintf(stderr, "Encoded %d samples, %d bytes %d remaining\n",
              h.samples_per_frame, h.frame_bytes, codec->encoder_output_size - h.frame_bytes);
#endif   
      /* Adjust mp3 buffer */

      if(codec->encoder_output_size > h.frame_bytes)
        {
        memmove(codec->encoder_output, codec->encoder_output + h.frame_bytes,
                codec->encoder_output_size - h.frame_bytes);
        }
      codec->encoder_output_size -= h.frame_bytes;
      }
    else
      break;
    }

  return result;
  }



static int encode(quicktime_t *file, 
                  int16_t **input_i, 
                  float **input_f, 
                  int track, 
                  long samples)
  {

  int result = 0;
  int frames;
  int encoded_size;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_trak_t *trak = track_map->track;
  quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  int new_size = codec->input_size + samples;
  int i, j;
  int frames_end = 0;

  if(!codec->encode_initialized)
    {
    codec->encode_initialized = 1;
    codec->lame_global = lame_init();
    lame_set_brate(codec->lame_global, codec->bitrate / 1000);
    lame_set_VBR(codec->lame_global, vbr_off);
    lame_set_quality(codec->lame_global, codec->quality);
    lame_set_in_samplerate(codec->lame_global, 
                           trak->mdia.minf.stbl.stsd.table[0].sample_rate);
    lame_set_out_samplerate(codec->lame_global, 
                            trak->mdia.minf.stbl.stsd.table[0].sample_rate);

    lame_set_bWriteVbrTag(codec->lame_global, 0);
    
    /* Also support Mono streams */
    
    codec->stereo = (trak->mdia.minf.stbl.stsd.table[0].channels == 1) ? 0 : 1;
    lame_set_num_channels(codec->lame_global, (codec->stereo ? 2 : 1));
    
    //    lame_set_padding_type(codec->lame_global, PAD_ALL); // PAD_NO | PAD_ALL | PAD_ADJUST
    
    if((result = lame_init_params(codec->lame_global)) < 0)
      printf(" lame_init_params returned %d\n", result);
    //    codec->encoded_header = mpeg3_new_layer();
    codec->samples_per_frame = lame_get_framesize(codec->lame_global);

    /* Initialize AVI header */

    if(trak->strl)
      {
      /* strh stuff */
      trak->strl->dwRate = codec->bitrate / 8;
      trak->strl->dwScale = 1;
      trak->strl->dwSampleSize = 1;
      
      /* WAVEFORMATEX stuff */
      
      trak->strl->nBlockAlign = 1;
      trak->strl->nAvgBytesPerSec =  codec->bitrate / 8;
      trak->strl->wBitsPerSample = 0;
      }
    }

  /* Reallocate output if necessary */

  encoded_size = (5*samples)/4 + 7200;
  
  if(codec->encoder_output_alloc < encoded_size + codec->encoder_output_size)
    {
    codec->encoder_output_alloc = encoded_size + codec->encoder_output_size + 16;
    codec->encoder_output       = realloc(codec->encoder_output, codec->encoder_output_alloc);
    }
  if(input_f)
    {
    if(codec->input_buffer_alloc < samples)
      {
      codec->input_buffer_alloc = samples + 16;
      codec->input_buffer[0] =
        realloc(codec->input_buffer[0], codec->input_buffer_alloc * sizeof(float));
      
      if(codec->stereo)
        codec->input_buffer[1] =
          realloc(codec->input_buffer[1], codec->input_buffer_alloc * sizeof(float));
      }

    for(i = 0; i < samples; i++)
      codec->input_buffer[0][i] = input_f[0][i] * 32767.0;

    if(codec->stereo)
      {
      for(i = 0; i < samples; i++)
        codec->input_buffer[1][i] = input_f[1][i] * 32767.0;
      }

    result = lame_encode_buffer_float(codec->lame_global,
                                      codec->input_buffer[0],
                                      codec->stereo ? codec->input_buffer[1] : codec->input_buffer[0],
                                      samples,
                                      codec->encoder_output + codec->encoder_output_size,
                                      codec->encoder_output_alloc - codec->encoder_output_size);
    }
  else if(input_i)
    {
    result = lame_encode_buffer(codec->lame_global,
                                input_i[0],
                                codec->stereo ? input_i[1] : input_i[0],
                                samples,
                                codec->encoder_output + codec->encoder_output_size,
                                codec->encoder_output_alloc - codec->encoder_output_size);
    }

  if(result > 0)
    {
    codec->encoder_output_size += result;
    result = write_data(file, track_map, codec);
    }
  
  return result;
  }



static int set_parameter(quicktime_t *file, int track, 
                         char *key, void *value)
  {
  quicktime_audio_map_t *atrack = &(file->atracks[track]);
  quicktime_mp3_codec_t *codec =
    ((quicktime_codec_t*)atrack->codec)->priv;

  if(!strcasecmp(key, "mp3_bitrate"))
    codec->bitrate = *(int*)value;
  else if(!strcasecmp(key, "mp3_quality"))
    codec->quality = *(int*)value;

  return 0;
  }

static void flush(quicktime_t *file, int track)
  {
  int result = 0;
  int samples_left;
  quicktime_atom_t chunk_atom;
  int64_t offset = quicktime_position(file);
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_trak_t *trak = track_map->track;
  quicktime_mp3_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  
  if(codec->encode_initialized)
    {
    //    samples_left = lame_get_mf_samples_to_encode(codec->lame_global);
    
    result = lame_encode_flush_nogap(codec->lame_global,
                                     codec->encoder_output + codec->encoder_output_size, 
                                     codec->encoder_output_alloc);
    /* Check if more frames arrived */

    if(result > 0)
      {
      codec->encoder_output_size += result;
      write_data(file, track_map, codec);
      }
    }
  }

void quicktime_init_codec_lame(quicktime_audio_map_t *atrack)
  {
  quicktime_mp3_codec_t *codec;
  
  /* Init public items */
  ((quicktime_codec_t*)atrack->codec)->priv = calloc(1, sizeof(quicktime_mp3_codec_t));
  ((quicktime_codec_t*)atrack->codec)->delete_acodec = delete_codec;
  //  ((quicktime_codec_t*)atrack->codec)->decode_audio = decode;
  ((quicktime_codec_t*)atrack->codec)->encode_audio = encode;
  ((quicktime_codec_t*)atrack->codec)->set_parameter = set_parameter;
  ((quicktime_codec_t*)atrack->codec)->flush = flush;
  
  codec = ((quicktime_codec_t*)atrack->codec)->priv;
  codec->bitrate = 256000;
  codec->quality = 0;
  }
