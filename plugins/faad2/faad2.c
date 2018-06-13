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

  faacDecFrameInfo frame_info;
    
  int upsample;

  lqt_packet_t pkt;
  float * samples;
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

  lqt_packet_free(&codec->pkt);
  
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
  

static int decode_packet_faad2(quicktime_t * file, int track, lqt_audio_buffer_t * buf)
  {
  int i;
  quicktime_audio_map_t *atrack = &file->atracks[track];
  quicktime_faad2_codec_t *codec = atrack->codec->priv;

  if(!codec->samples)
    {
    codec->frame_info.samples = 0;

    while(!codec->frame_info.samples)
      {
      /* Decode packet */
      // fprintf(stderr, "Read packet %d\n", atrack->track->idx_pos);
      if(!quicktime_trak_read_packet(file, atrack->track, &codec->pkt))
        return 0;

      memset(&codec->frame_info, 0, sizeof(codec->frame_info));
      
      codec->samples = faacDecDecode(codec->dec, &codec->frame_info,
                                     codec->pkt.data, codec->pkt.data_len);
      
      if(!codec->samples)
        {
        lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "faacDecDecode failed %s",
                faacDecGetErrorMessage(codec->frame_info.error));
        return 0;
        }
    
      /* Set up channel map */
      if(!atrack->channel_setup)
        {
        atrack->sample_format = LQT_SAMPLE_FLOAT;
        atrack->channel_setup = calloc(atrack->channels, sizeof(*(atrack->channel_setup)));
        for(i = 0; i < atrack->channels; i++)
          {
          atrack->channel_setup[i] =
            get_channel(codec->frame_info.channel_position[i]);
          }

        if(codec->frame_info.sbr == 1)
          atrack->ci.flags |= LQT_COMPRESSION_SBR;
        }
      }
    
    if((atrack->channels == 1) && (codec->frame_info.channels == 2))
      {
      for(i = 0; i < codec->frame_info.samples/2; i++)
        codec->samples[i] = codec->samples[2*i];
      codec->frame_info.samples/=2;
      }
    }

  if(!buf)
    return 0; // Init

  lqt_audio_buffer_alloc(buf, codec->frame_info.samples/atrack->channels, atrack->channels, 0, LQT_SAMPLE_FLOAT);
  memcpy(buf->channels[0].f, codec->samples, sizeof(*codec->samples) * codec->frame_info.samples);
  buf->size = codec->frame_info.samples/atrack->channels;

  codec->samples = NULL;
  return buf->size;
  }

static void resync_faad2(quicktime_t * file, int track)
  {
  quicktime_audio_map_t *atrack = &file->atracks[track];
  quicktime_faad2_codec_t *codec = atrack->codec->priv;
  codec->samples = NULL;
  faacDecPostSeekReset(codec->dec, 1); // Don't skip the first frame
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
  codec_base->decode_audio_packet = decode_packet_faad2;
  codec_base->resync = resync_faad2;

  codec_base->set_parameter = set_parameter;
  
  /* Ok, usually, we initialize decoders during the first
     decode() call. But in this case, we might need to
     set the correct samplerate, which should be known before */

  codec->dec = faacDecOpen();

  if(!atrack)
    return;
  
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
  
  faacDecPostSeekReset(codec->dec, 1); // Don't skip the first frame
  
  atrack->ci.id = LQT_COMPRESSION_AAC;

  lqt_compression_info_set_header(&atrack->ci,
                                  extradata,
                                  extradata_size);

  
  if(atrack->samplerate != samplerate)
    {
    lqt_audio_set_sbr(atrack);
    codec->upsample = 1;
    }
  
  atrack->preroll = 1024;
  
  stsd->table[0].channels = channels;
  atrack->channels = channels;
  
  }
