#include <stdio.h>
#include <stdlib.h>
#include <funcprotos.h>
#include <string.h>
#include <lame/lame.h>
#include <quicktime/quicktime.h>

// Mp3 framesize detection (ported from gmerlin)

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

#define IS_MPEG_AUDIO_HEADER(h) ((h&0xFFE00000)==0xFFE00000)

#define MPEG_VERSION_1   0
#define MPEG_VERSION_2   1
#define MPEG_VERSION_2_5 2


int get_frame_size(const unsigned char * data)
  {
  unsigned long header;
  int bitrate_index, bitrate = 0;
  int frequency_index, samplerate = 0;
  int layer = 0;
  int mpeg_version = 0;
  int ret;
  header = data[3] | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
  bitrate_index = (header & MPEG_BITRATE_MASK) >> 12;
  if(!IS_MPEG_AUDIO_HEADER(header))
    return 0;

  switch(header & MPEG_ID_MASK)
    {
    case MPEG_MPEG1:
      mpeg_version = MPEG_VERSION_1;
      break;
    case MPEG_MPEG2:
      mpeg_version = MPEG_VERSION_2;
      break;
    case MPEG_MPEG2_5:
      mpeg_version = MPEG_VERSION_2_5;
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
  frequency_index = (header & MPEG_FREQUENCY_MASK) >> 10;
  
  switch(mpeg_version)
    {
    case MPEG_VERSION_1:
      switch(layer)
        {
        case 1:
          bitrate = mpeg_bitrates[0][bitrate_index];
          samplerate = mpeg_samplerates[0][frequency_index];
          break;
        case 2:
          bitrate = mpeg_bitrates[1][bitrate_index];
          samplerate = mpeg_samplerates[1][frequency_index];
          break;
        case 3:
          bitrate = mpeg_bitrates[2][bitrate_index];
          samplerate = mpeg_samplerates[2][frequency_index];
          break;
        }
      break;
    case MPEG_VERSION_2:
    case MPEG_VERSION_2_5:
      {
      switch(layer)
        {
        case 1:
          bitrate = mpeg_bitrates[3][bitrate_index];
          samplerate = mpeg_samplerates[0][frequency_index]/2;
          break;
        case 2:
          bitrate = mpeg_bitrates[4][bitrate_index];
          samplerate = mpeg_samplerates[1][frequency_index]/2;
        case 3:
          bitrate = mpeg_bitrates[5][bitrate_index];
          samplerate = mpeg_samplerates[1][frequency_index]/2;
          break;
        }
      if(mpeg_version == MPEG_VERSION_2_5)
        samplerate /= 2;
      }
    default: // This won't happen, but keeps gcc quiet
      break;
    }

  if(!bitrate)
    return 0;
  if(layer == 1)
    {
    ret = (12 * bitrate / samplerate);
    if(header & MPEG_PAD_MASK)
      ret++;
    ret *= 4;
    }
  else
    {
    ret = 144 * bitrate /samplerate;
    if(header & MPEG_PAD_MASK)
      ret++;
    }
  if(mpeg_version == MPEG_VERSION_2)
    ret /= 2;

  // For mpeg 2.5, this is not tested
  
  else if(mpeg_version == MPEG_VERSION_2_5)
    ret /= 4;
  return ret;
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

  unsigned char *encoder_output;
  int encoder_output_alloc;
  
  int samples_per_frame;
  int stereo;
  int64_t frames_encoded;
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
  if(codec->encoder_output)
    free(codec->encoder_output);
  //  if(codec->encoded_header)  mpeg3_delete_layer(codec->encoded_header);
  free(codec);
  return 0;
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
  quicktime_atom_t chunk_atom;

  if(!codec->encode_initialized)
    {
    codec->encode_initialized = 1;
    codec->lame_global = lame_init();
    lame_set_brate(codec->lame_global, codec->bitrate / 1000);
    lame_set_quality(codec->lame_global, 0);
    lame_set_in_samplerate(codec->lame_global, 
                           trak->mdia.minf.stbl.stsd.table[0].sample_rate);
    lame_set_out_samplerate(codec->lame_global, 
                            trak->mdia.minf.stbl.stsd.table[0].sample_rate);
    
    /* Also support Mono streams */
    
    codec->stereo = (trak->mdia.minf.stbl.stsd.table[0].channels == 1) ? 0 : 1;
    lame_set_num_channels(codec->lame_global, (codec->stereo ? 2 : 1));

    lame_set_scale(codec->lame_global, 1.0);
    
    if((result = lame_init_params(codec->lame_global)) < 0)
      printf(" lame_init_params returned %d\n", result);
    //    codec->encoded_header = mpeg3_new_layer();
    codec->samples_per_frame = lame_get_framesize(codec->lame_global);
    }

  /* Reallocate output if necessary */

  encoded_size = (5*samples)/4 + 7200;
  
  if(codec->encoder_output_alloc < encoded_size)
    {
    codec->encoder_output_alloc = encoded_size;
    codec->encoder_output       = realloc(codec->encoder_output, codec->encoder_output_alloc);
    }
  if(input_f)
    {
    result = lame_encode_buffer_float(codec->lame_global,
                                      input_f[0],
                                      codec->stereo ? input_f[1] : input_f[0],
                                      samples,
                                      codec->encoder_output,
                                      codec->encoder_output_alloc);
    }
  else if(input_i)
    {
    result = lame_encode_buffer(codec->lame_global,
                                input_i[0],
                                codec->stereo ? input_i[1] : input_i[0],
                                samples,
                                codec->encoder_output,
                                codec->encoder_output_alloc);
    }

  if(result > 0)
    {
    /* Check, how many frames were encoded */

    frames = lame_get_frameNum(codec->lame_global) - codec->frames_encoded;

    //   fprintf(stderr, "Encoded %d frames, %d bytes\n", frames, result);
    
    quicktime_write_chunk_header(file, trak, &chunk_atom);
    result = !quicktime_write_data(file, codec->encoder_output, result);
    // Knows not to save the chunksizes for audio
    quicktime_write_chunk_footer(file, 
                                 trak, 
                                 track_map->current_chunk,
                                 &chunk_atom, 
                                 frames * codec->samples_per_frame);
    track_map->current_chunk++;
    codec->frames_encoded += frames;
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
    samples_left = lame_get_mf_samples_to_encode(codec->lame_global);
    
    result = lame_encode_flush(codec->lame_global,
                               codec->encoder_output, 
                               codec->encoder_output_alloc);
    /* Check if more frames arrived */

    if(samples_left && result)
      {
      quicktime_write_chunk_header(file, trak, &chunk_atom);
      result = !quicktime_write_data(file, codec->encoder_output, result);
      // Knows not to save the chunksizes for audio
      quicktime_write_chunk_footer(file, 
                                   trak, 
                                   track_map->current_chunk,
                                   &chunk_atom, 
                                   samples_left);
      
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
  }
