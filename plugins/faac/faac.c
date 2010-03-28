/*******************************************************************************
 faac.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2010 Members of the libquicktime project.

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
#include <faac.h>
#include <string.h>
#include <stdlib.h>
#include "qtfaac.h"

#define LOG_DOMAIN "faac"

typedef struct
  {
  float * sample_buffer;
  int sample_buffer_size;
  int samples_per_frame;

  uint8_t * chunk_buffer;
  int chunk_buffer_size;
  
  int initialized;
  faacEncHandle enc;

  int chunk_started;
  quicktime_atom_t chunk_atom;

  int encoder_delay;
  
  /* Configuration stuff */
  int bitrate;
  int quality;
  int object_type;
  } quicktime_faac_codec_t;


static int delete_codec(quicktime_audio_map_t *track_map)
  {
  quicktime_faac_codec_t *codec = track_map->codec->priv;
  if(codec->sample_buffer)
    free(codec->sample_buffer);
  if(codec->chunk_buffer)
    free(codec->chunk_buffer);
  if(codec->enc)
    faacEncClose(codec->enc);
  return 0;
  }

static int encode_frame(quicktime_t *file, 
                        int track, int num_samples)
  {
  int imax, i;
  int bytes_encoded;
  int result;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_faac_codec_t *codec = track_map->codec->priv;
  quicktime_trak_t * trak = track_map->track;
  /* Normalize input to 16 bit int */

  imax = codec->sample_buffer_size * track_map->channels;
  
  if(!num_samples && (codec->encoder_delay < 0))
    return 0;
  
  for(i = 0; i < imax; i++)
    {
    codec->sample_buffer[i] *= 32767.0;
    }
  
  codec->encoder_delay += num_samples;
  
  /* Encode this frame */
  bytes_encoded = faacEncEncode(codec->enc,
                                (int32_t*)codec->sample_buffer,
                                codec->sample_buffer_size ?
                                codec->samples_per_frame *
                                track_map->channels : 0,
                                codec->chunk_buffer,
                                codec->chunk_buffer_size);

  codec->sample_buffer_size = 0;
    
  if(bytes_encoded <= 0)
    {
    return 0;
    }

  codec->encoder_delay -= codec->samples_per_frame;
  
  /* Write these data */

  if(!codec->chunk_started)
    {
    codec->chunk_started = 1;
    lqt_start_audio_vbr_chunk(file, track);
    quicktime_write_chunk_header(file, trak, &codec->chunk_atom);
    }
  
  lqt_start_audio_vbr_frame(file, track);
  result = !quicktime_write_data(file, codec->chunk_buffer,
                                 bytes_encoded);
  if(codec->encoder_delay < 0)
    {
    lqt_finish_audio_vbr_frame(file, track, codec->samples_per_frame +
                               codec->encoder_delay);
    }
  else
    lqt_finish_audio_vbr_frame(file, track, codec->samples_per_frame);
  
  return 1;
  }

static int encode(quicktime_t *file, 
                  void *_input,
                  long samples,
                  int track)
  {
  quicktime_esds_t * esds;
  int samples_read;
  int samples_to_copy;
  
  faacEncConfigurationPtr enc_config;
  unsigned long input_samples;
  unsigned long output_bytes;
  float * input;

  uint8_t * decoderConfig;
  unsigned long decoderConfigLen;
  
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_faac_codec_t *codec = track_map->codec->priv;
  quicktime_trak_t * trak = track_map->track;
  uint8_t mp4a_atom[4];
  
  if(!codec->initialized)
    {
    lqt_init_vbr_audio(file, track);
    /* Create encoder */
    
    codec->enc = faacEncOpen(track_map->samplerate,
                             track_map->channels,
                             &input_samples,
                             &output_bytes);
    
    /* Set things up */
    enc_config = faacEncGetCurrentConfiguration(codec->enc);
    enc_config->inputFormat = FAAC_INPUT_FLOAT;
    enc_config->bitRate = (codec->bitrate * 1000) / track_map->channels;
    enc_config->quantqual = codec->quality;
    enc_config->outputFormat = 0; /* Raw */
    //    enc_config->aacObjectType = codec->object_type; /* LOW, LTP... */
    enc_config->aacObjectType = LOW; /* LOW, LTP... */
    
    if(!faacEncSetConfiguration(codec->enc, enc_config))
      {
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "Setting encode parameters failed, check settings");
      }
    /* Allocate buffers */

    codec->samples_per_frame = input_samples / track_map->channels;

    codec->sample_buffer =
      malloc(codec->samples_per_frame * track_map->channels * sizeof(float));
#if 0
    for(samples_read = 0;
        samples_read < codec->samples_per_frame * track_map->channels;
        samples_read++)
      codec->sample_buffer[samples_read] = 1.0;
#endif
    codec->chunk_buffer_size = output_bytes;
    codec->chunk_buffer = malloc(codec->chunk_buffer_size);
    
    codec->initialized = 1;

    /* Initialize esds atom */
    
    faacEncGetDecoderSpecificInfo(codec->enc, &decoderConfig,
                                  &decoderConfigLen);
    
    esds = quicktime_set_esds(trak,
                              decoderConfig,
                              decoderConfigLen);
    free(decoderConfig);

    quicktime_set_frma(trak, "mp4a");

    mp4a_atom[0] = 0x00;
    mp4a_atom[1] = 0x00;
    mp4a_atom[2] = 0x00;
    mp4a_atom[3] = 0x00;
    
    quicktime_wave_set_user_atom(trak, "mp4a", mp4a_atom, 4);
#if 0
    quicktime_set_stsd_audio_v2(&(trak->mdia.minf.stbl.stsd.table[0]),
                                0x00000002, /* formatSpecificFlags */
                                0, /* constBytesPerAudioPacket */
                                codec->samples_per_frame /* constLPCMFramesPerAudioPacket */);
#else
    quicktime_set_stsd_audio_v1(&(trak->mdia.minf.stbl.stsd.table[0]),
                                1024, // uint32_t samples_per_packet,
                                768, // uint32_t bytes_per_packet,
                                1536, // uint32_t bytes_per_frame,
                                0 // uint32_t bytes_per_sample
                                );
#endif
    trak->mdia.minf.stbl.stsd.table[0].sample_size = 16;
    
    esds->version         = 0;
    esds->flags           = 0;
    
    esds->esid            = 0;
    esds->stream_priority = 0;
    
    esds->objectTypeId    = 64; /* MPEG-4 audio */
    esds->streamType      = 0x15; /* from qt4l and Autumns Child.m4a */
    //    esds->bufferSizeDB    = 64000; /* Hopefully not important :) */
    esds->bufferSizeDB    = 6144;
    /* Maybe correct these later? */
    esds->maxBitrate      = 128000;
    esds->avgBitrate      = 128000;

    /* No idea if the following is right but other AAC LC files have the same */
    switch(enc_config->aacObjectType)
      {
      case LOW:
        // "High Quality Audio Profile @ Level 2"
        file->moov.iods.audioProfileId  = 0x0f;
        break;
      case SSR:
        file->moov.iods.audioProfileId  = 0x0f;
        break;
      default:
        file->moov.iods.audioProfileId  = 0x0f;
        break;
      }
    }
  
  /* Encode samples */
  samples_read = 0;

  input = (float*)_input;
  
  
  
  while(samples_read < samples)
    {
    /* Put samples into sample buffer */
    
    samples_to_copy = codec->samples_per_frame - codec->sample_buffer_size;
    if(samples_read + samples_to_copy > samples)
      samples_to_copy = samples - samples_read;

    memcpy(codec->sample_buffer + track_map->channels * codec->sample_buffer_size,
           input + samples_read * track_map->channels,
           samples_to_copy * track_map->channels * sizeof(float));
    
    codec->sample_buffer_size += samples_to_copy;
    samples_read += samples_to_copy;
    
    /* Encode one frame, possibly starting a new audio chunk */
    if(codec->sample_buffer_size == codec->samples_per_frame)
      encode_frame(file, track, codec->samples_per_frame);
    
    }

  /* Finalize audio chunk */
  if(codec->chunk_started)
    {
    quicktime_write_chunk_footer(file,
                                 trak,
                                 track_map->cur_chunk,
                                 &codec->chunk_atom,
                                 track_map->vbr_num_frames);
    track_map->cur_chunk++;
    codec->chunk_started = 0;
    }
  return 0;
  }

static int set_parameter(quicktime_t *file, 
                         int track, 
                         const char *key, 
                         const void *value)
  {
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_faac_codec_t *codec = track_map->codec->priv;
    
  if(!strcasecmp(key, "faac_bitrate"))
    codec->bitrate = *(int*)value;
  else if(!strcasecmp(key, "faac_quality"))
    codec->quality = *(int*)value;
  else if(!strcasecmp(key, "faac_object_type"))
    {
    if(!strcmp((char*)value, "Low"))
      codec->object_type = LOW;
    else if(!strcmp((char*)value, "Main"))
      codec->object_type = MAIN;
    else if(!strcmp((char*)value, "SSR"))
      codec->object_type = SSR;
    else if(!strcmp((char*)value, "LTP"))
      codec->object_type = LTP;
    }
  return 0;
  }

static int flush(quicktime_t *file, int track)
  {
  int i;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_faac_codec_t *codec = track_map->codec->priv;
  quicktime_trak_t * trak = track_map->track;

  if(!codec->initialized)
    return 0;
  
  /* Mute the rest of the sample buffer */
  if(codec->sample_buffer_size)
    {
    for(i = codec->sample_buffer_size * track_map->channels;
        i < codec->samples_per_frame * track_map->channels; i++)
      {
      codec->sample_buffer[i] = 0.0;
      }
    //    codec->sample_buffer_size = codec->samples_per_frame;
    }

  while(encode_frame(file, track, codec->sample_buffer_size))
    ;
  
  /* Finalize audio chunk */
  if(codec->chunk_started)
    {
    quicktime_write_chunk_footer(file,
                                 trak,
                                 track_map->cur_chunk,
                                 &codec->chunk_atom,
                                 track_map->vbr_num_frames);
    track_map->cur_chunk++;
    codec->chunk_started = 0;
    return 1;
    }
  return 0;
  }

void quicktime_init_codec_faac(quicktime_audio_map_t *track_map)
  {
  quicktime_codec_t *codec_base = track_map->codec;
  quicktime_faac_codec_t *codec;
  
  /* Init public items */
  codec_base->priv = calloc(1, sizeof(quicktime_faac_codec_t));
  codec_base->delete_acodec = delete_codec;
  codec_base->encode_audio = encode;
  codec_base->set_parameter = set_parameter;
  codec_base->flush = flush;
  
  codec = codec_base->priv;

  codec->bitrate = 0;
  codec->quality = 100;
  
  track_map->sample_format = LQT_SAMPLE_FLOAT;

  if(track_map->channels <= 6)
    {
    /* Set default AAC channel setup */
    track_map->channel_setup = calloc(track_map->channels,
                                      sizeof(*track_map->channel_setup));

    switch(track_map->channels)
      {
      case 1:
        track_map->channel_setup[0] = LQT_CHANNEL_FRONT_CENTER;
        break;
      case 2:
        track_map->channel_setup[0] = LQT_CHANNEL_FRONT_LEFT;
        track_map->channel_setup[1] = LQT_CHANNEL_FRONT_RIGHT;
        break;
      case 3:
        track_map->channel_setup[0] = LQT_CHANNEL_FRONT_CENTER;
        track_map->channel_setup[1] = LQT_CHANNEL_FRONT_LEFT;
        track_map->channel_setup[2] = LQT_CHANNEL_FRONT_RIGHT;
        break;
      case 4:
        track_map->channel_setup[0] = LQT_CHANNEL_FRONT_CENTER;
        track_map->channel_setup[1] = LQT_CHANNEL_FRONT_LEFT;
        track_map->channel_setup[2] = LQT_CHANNEL_FRONT_RIGHT;
        track_map->channel_setup[3] = LQT_CHANNEL_BACK_CENTER;
        break;
      case 5:
        track_map->channel_setup[0] = LQT_CHANNEL_FRONT_CENTER;
        track_map->channel_setup[1] = LQT_CHANNEL_FRONT_LEFT;
        track_map->channel_setup[2] = LQT_CHANNEL_FRONT_RIGHT;
        track_map->channel_setup[3] = LQT_CHANNEL_BACK_LEFT;
        track_map->channel_setup[4] = LQT_CHANNEL_BACK_RIGHT;
        break;
      case 6:
        track_map->channel_setup[0] = LQT_CHANNEL_FRONT_CENTER;
        track_map->channel_setup[1] = LQT_CHANNEL_FRONT_LEFT;
        track_map->channel_setup[2] = LQT_CHANNEL_FRONT_RIGHT;
        track_map->channel_setup[3] = LQT_CHANNEL_BACK_LEFT;
        track_map->channel_setup[4] = LQT_CHANNEL_BACK_RIGHT;
        track_map->channel_setup[5] = LQT_CHANNEL_LFE;
        break;
      }
    quicktime_set_chan(track_map);
    }
  }
