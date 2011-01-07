/*******************************************************************************
 faad2.c

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

#include "faad2.h"

#include <string.h>
#include <stdlib.h>

#ifdef HAVE_NEAACDEC_H
#include <neaacdec.h>
/*
 *  Backwards compatibility names (currently in neaacdec.h,
 *  but might be removed in future versions)
 */
#ifndef faacDecHandle
/* structs */
#define faacDecHandle                  NeAACDecHandle
#define faacDecConfiguration           NeAACDecConfiguration
#define faacDecConfigurationPtr        NeAACDecConfigurationPtr
#define faacDecFrameInfo               NeAACDecFrameInfo
/* functions */
#define faacDecGetErrorMessage         NeAACDecGetErrorMessage
#define faacDecSetConfiguration        NeAACDecSetConfiguration
#define faacDecGetCurrentConfiguration NeAACDecGetCurrentConfiguration
#define faacDecInit                    NeAACDecInit
#define faacDecInit2                   NeAACDecInit2
#define faacDecInitDRM                 NeAACDecInitDRM
#define faacDecPostSeekReset           NeAACDecPostSeekReset
#define faacDecOpen                    NeAACDecOpen
#define faacDecClose                   NeAACDecClose
#define faacDecDecode                  NeAACDecDecode
#define AudioSpecificConfig            NeAACDecAudioSpecificConfig
#endif

#else
#include <faad.h>
#endif

#define LOG_DOMAIN "faad2"

typedef struct
  {
  faacDecHandle dec;

  /* Start and end positions of the sample buffer */
  
  int64_t sample_buffer_start;
  int64_t sample_buffer_end;
    
  uint8_t * data;
  int data_alloc;
  
  float * sample_buffer;
  int sample_buffer_alloc;

  int upsample;
  
  } quicktime_faad2_codec_t;

static int delete_codec(quicktime_codec_t *codec_base)
  {
  quicktime_faad2_codec_t *codec = codec_base->priv;

  if(codec->dec)
    faacDecClose(codec->dec);

  if(codec->sample_buffer)
    free(codec->sample_buffer);

  if(codec->data)
    free(codec->data);
      
  
  free(codec);
  return 0;
  }

/* Channel IDs */

static struct
  {
  int faad_ch;
  lqt_channel_t lqt_ch;
  }
channels[] = 
  {
    { FRONT_CHANNEL_CENTER, LQT_CHANNEL_FRONT_CENTER },
    { FRONT_CHANNEL_LEFT,   LQT_CHANNEL_FRONT_LEFT },
    { FRONT_CHANNEL_RIGHT,  LQT_CHANNEL_FRONT_RIGHT },
    { SIDE_CHANNEL_LEFT,    LQT_CHANNEL_SIDE_LEFT },
    { SIDE_CHANNEL_RIGHT,   LQT_CHANNEL_SIDE_RIGHT },
    { BACK_CHANNEL_LEFT,    LQT_CHANNEL_BACK_LEFT },
    { BACK_CHANNEL_RIGHT,   LQT_CHANNEL_BACK_RIGHT },
    { BACK_CHANNEL_CENTER,  LQT_CHANNEL_BACK_CENTER },
    { LFE_CHANNEL,          LQT_CHANNEL_LFE },
    { UNKNOWN_CHANNEL,      LQT_CHANNEL_UNKNOWN }
  };
  
static lqt_channel_t get_channel(int channel)
  {
  int i;
  for(i = 0; i < sizeof(channels)/sizeof(channels[0]); i++)
    {
    if(channels[i].faad_ch == channel)
      return channels[i].lqt_ch;
    }
  return LQT_CHANNEL_UNKNOWN;
  }
  
static int decode_chunk(quicktime_t *file, int track)
  {
  int i, j, num_packets, num_samples, packet_size;
  float * samples;
  faacDecFrameInfo frame_info;

  quicktime_audio_map_t *track_map = &file->atracks[track];
  
  quicktime_faad2_codec_t *codec =
    file->atracks[track].codec->priv;
  
  num_packets = lqt_audio_num_vbr_packets(file, track,
                                          track_map->cur_chunk,
                                          &num_samples);

  // fprintf(stderr, "Num packets %ld, %d\n", track_map->cur_chunk, num_packets);

  if(!num_packets)
    return 0;

  if(codec->upsample)
    num_samples *= 2;
  
  
  if(codec->sample_buffer_alloc <
     codec->sample_buffer_end - codec->sample_buffer_start + num_samples)
    {
    codec->sample_buffer_alloc =
      codec->sample_buffer_end - codec->sample_buffer_start +
      num_samples + 1024;

    codec->sample_buffer = realloc(codec->sample_buffer,
                                   codec->sample_buffer_alloc *
                                   track_map->channels * sizeof(float));
    }
  
  for(i = 0; i < num_packets; i++)
    {
    packet_size = lqt_audio_read_vbr_packet(file, track,
                                            track_map->cur_chunk, i,
                                            &codec->data,
                                            &codec->data_alloc, &num_samples);

    // fprintf(stderr, "Packet size %d, %d\n", i, packet_size);
    
    if(codec->upsample)
      num_samples *= 2;

    samples = faacDecDecode(codec->dec, &frame_info,
                            codec->data, packet_size);
    if(!samples)
      {
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "faacDecDecode failed %s",
              faacDecGetErrorMessage(frame_info.error));
      
      return 0;
      }

    /* Set up channel map */
    if(!track_map->channel_setup)
      {
      track_map->channel_setup = calloc(track_map->channels,
                                        sizeof(*(track_map->channel_setup)));
      for(i = 0; i < track_map->channels; i++)
        {
        track_map->channel_setup[i] =
          get_channel(frame_info.channel_position[i]);
        }

      }
    
      
    
    
    if((track_map->channels == 1) && (frame_info.channels == 2))
      {
            
      for(j = 0; j < frame_info.samples/2; j++)
        samples[j] = samples[2*j];
      frame_info.samples/=2;
      }
    
    memcpy(&codec->sample_buffer[(codec->sample_buffer_end -
                                   codec->sample_buffer_start)*track_map->channels],
           samples, frame_info.samples * sizeof(float));
    codec->sample_buffer_end += frame_info.samples / track_map->channels;
    }
  
  track_map->cur_chunk++;
  return 1;
  }

static int decode(quicktime_t *file, 
                  void * output,
                  long samples, 
                  int track) 
  {
  int64_t chunk_sample;
  int samples_copied = 0;
  int samples_to_skip;
  int samples_to_move;
  int samples_decoded;
  
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_faad2_codec_t *codec = track_map->codec->priv;

  /* TODO Check whether seeking happened */

  if(!output)
    {
    /* HACK: Sometimes mp4 files don't set this correctly */
    lqt_init_vbr_audio(file, track); 

    /* Set channel setup */
    decode_chunk(file, track);
    return 0;
    }
  
  if(track_map->last_position != track_map->current_position)
    {
    /* Get the next chunk */
    
    if(codec->upsample)
      {
      lqt_chunk_of_sample_vbr(&chunk_sample,
                              &track_map->cur_chunk,
                              track_map->track,
                              track_map->current_position / 2);
      chunk_sample *= 2;
      }
    else
      lqt_chunk_of_sample_vbr(&chunk_sample,
                              &track_map->cur_chunk,
                              track_map->track,
                              track_map->current_position);
    
    if(track_map->cur_chunk >= track_map->track->mdia.minf.stbl.stco.total_entries - 1)
      {
      return 0;
      }
    
    codec->sample_buffer_start = chunk_sample;
    codec->sample_buffer_end   = chunk_sample;

    /* Decode frames until we have enough */

    while(codec->sample_buffer_end < track_map->current_position + samples)
      {
      if(!decode_chunk(file, track))
        break;
      }
    }
  
  /* Flush unneeded samples */
  
  if(track_map->current_position > codec->sample_buffer_start)
    {
    samples_to_skip = track_map->current_position - codec->sample_buffer_start;

    samples_to_move = codec->sample_buffer_end - track_map->current_position;

    if(samples_to_move > 0)
      {
      memmove(codec->sample_buffer,
              codec->sample_buffer + samples_to_skip * track_map->channels,
              samples_to_move * track_map->channels * sizeof(float));
      }
    codec->sample_buffer_start = track_map->current_position;
    if(samples_to_move > 0)
      codec->sample_buffer_end = codec->sample_buffer_start + samples_to_move;
    else
      codec->sample_buffer_end = codec->sample_buffer_start;
    }

  /* Decode new chunks until we have enough samples */

  while(codec->sample_buffer_end < codec->sample_buffer_start + samples)
    {
    if(!decode_chunk(file, track))
      break;
    }

  samples_decoded = codec->sample_buffer_end - codec->sample_buffer_start;

  samples_copied = (samples_decoded > samples) ? samples : samples_decoded;
  
  memcpy(output, codec->sample_buffer, samples_copied * track_map->channels * sizeof(float));

  track_map->last_position = track_map->current_position + samples_copied;
  
  return samples_copied;
  }

static int set_parameter(quicktime_t *file, 
		int track, 
		const char *key, 
		const void *value)
  {
  return 0;
  }

void quicktime_init_codec_faad2(quicktime_codec_t * codec_base,
                                quicktime_audio_map_t *atrack,
                                quicktime_video_map_t *vtrack)
  {
  uint8_t * extradata = (uint8_t *)0;
  int extradata_size = 0;
  quicktime_stsd_t * stsd;
  unsigned long samplerate = 0;
  unsigned char channels;

  faacDecConfigurationPtr cfg;
  
  quicktime_faad2_codec_t *codec;

  codec = calloc(1, sizeof(*codec));
  
  /* Init public items */
  codec_base->priv = codec;
  codec_base->delete_codec = delete_codec;
  codec_base->decode_audio = decode;
  codec_base->set_parameter = set_parameter;
  
  /* Ok, usually, we initialize decoders during the first
     decode() call. But in this case, we might need to
     set the correct samplerate, which should be known before */

  codec->dec = faacDecOpen();

  if(!atrack)
    return;
  
  atrack->sample_format = LQT_SAMPLE_FLOAT;
  stsd = &atrack->track->mdia.minf.stbl.stsd;
  
  if(stsd->table[0].has_esds)
    {
    extradata = stsd->table[0].esds.decoderConfig;
    extradata_size =
      stsd->table[0].esds.decoderConfigLen;
    }
  else if(stsd->table[0].has_wave &&
          stsd->table[0].wave.has_esds)
    {
    extradata = stsd->table[0].wave.esds.decoderConfig;
    extradata_size = stsd->table[0].wave.esds.decoderConfigLen;
    }
  else
    {
    lqt_log(NULL, LQT_LOG_ERROR, LOG_DOMAIN,
            "No extradata found, decoding is doomed to failure");
    }

  cfg = faacDecGetCurrentConfiguration(codec->dec);
  cfg->outputFormat = FAAD_FMT_FLOAT;
  
  faacDecSetConfiguration(codec->dec, cfg);
  
  faacDecInit2(codec->dec, extradata, extradata_size,
               &samplerate, &channels);

  atrack->ci.id = LQT_COMPRESSION_AAC;

  lqt_compression_info_set_header(&atrack->ci,
                                  extradata,
                                  extradata_size);
  
  if(atrack->samplerate != samplerate)
    {
    atrack->samplerate = samplerate;
    codec->upsample = 1;
    atrack->total_samples *= 2;
    atrack->ci.flags |= LQT_COMPRESSION_SBR;
    }
  stsd->table[0].channels = channels;
  atrack->channels = channels;
  
  }
