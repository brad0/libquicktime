/*******************************************************************************
 audio.c

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
#include "ffmpeg.h"
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>
#include <stdlib.h>
#include <string.h>

#define LOG_DOMAIN "ffmpeg_audio"

/* Different decoding functions */

#if LIBAVCODEC_BUILD >= ((51<<16)+(29<<8)+0)
#define DECODE_FUNC avcodec_decode_audio2
#else
#define DECODE_FUNC avcodec_decode_audio
#endif

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

static int mpa_header_check(uint32_t head)
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
  } mpa_header;

static int mpa_header_equal(const mpa_header * h1, const mpa_header * h2)
  {
  return ((h1->layer == h2->layer) && (h1->version == h2->version) &&
          (h1->samplerate == h2->samplerate));
  }

static int mpa_decode_header(mpa_header * h, uint8_t * ptr,
                             const mpa_header * ref)
  {
  uint32_t header;
  int index;
  /* For calculation of the byte length of a frame */
  int pad;
  int slots_per_frame;
  h->frame_bytes = 0;
  header =
    ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24);
  if(!mpa_header_check(header))
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

  /* Check against reference header */

  if(ref && !mpa_header_equal(ref, h))
    return 0;
  return 1;
  }

/* AC3 header */

#ifdef HAVE_GPL // Taken from a52dec
typedef struct
  {
  int total_bytes;
  int samplerate;
  int bitrate;

  int acmod;
  int lfe;
  int dolby;

  float cmixlev;
  float smixlev;
  
  } a52_header;

#define LEVEL_3DB 0.7071067811865476
#define LEVEL_45DB 0.5946035575013605
#define LEVEL_6DB 0.5

#define A52_FRAME_SAMPLES 1536

static const uint8_t halfrate[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3};
static const int rate[] = { 32,  40,  48,  56,  64,  80,  96, 112,
                      128, 160, 192, 224, 256, 320, 384, 448,
                      512, 576, 640};
static const uint8_t lfeon[8] = {0x10, 0x10, 0x04, 0x04, 0x04, 0x01, 0x04, 0x01};

static const float clev[4] = {LEVEL_3DB, LEVEL_45DB, LEVEL_6DB, LEVEL_45DB};
static const float slev[4] = {LEVEL_3DB, LEVEL_6DB,          0, LEVEL_6DB};


/* Needs 7 bytes */

static int a52_header_read(a52_header * ret, uint8_t * buf)
  {
  int half;
  int frmsizecod;
  int bitrate;

  int cmixlev;
  int smixlev;

  memset(ret, 0, sizeof(*ret));
  
  if ((buf[0] != 0x0b) || (buf[1] != 0x77))   /* syncword */
    {
    return 0;
    }
  if (buf[5] >= 0x60)         /* bsid >= 12 */
    {
    return 0;
    }
  half = halfrate[buf[5] >> 3];

  /* acmod, dsurmod and lfeon */
  ret->acmod = buf[6] >> 5;

  /* cmixlev and surmixlev */

  if((ret->acmod & 0x01) && (ret->acmod != 0x01))
    {
    cmixlev = (buf[6] & 0x18) >> 3;
    }
  else
    cmixlev = -1;
  
  if(ret->acmod & 0x04)
    {
    if((cmixlev) == -1)
      smixlev = (buf[6] & 0x18) >> 3;
    else
      smixlev = (buf[6] & 0x06) >> 1;
    }
  else
    smixlev = -1;

  if(smixlev >= 0)
    ret->smixlev = slev[smixlev];
  if(cmixlev >= 0)
    ret->cmixlev = clev[cmixlev];
  
  if((buf[6] & 0xf8) == 0x50)
    ret->dolby = 1;

  if(buf[6] & lfeon[ret->acmod])
    ret->lfe = 1;

  frmsizecod = buf[4] & 63;
  if (frmsizecod >= 38)
    return 0;
  bitrate = rate[frmsizecod >> 1];
  ret->bitrate = (bitrate * 1000) >> half;

  switch (buf[4] & 0xc0)
    {
    case 0:
      ret->samplerate = 48000 >> half;
      ret->total_bytes = 4 * bitrate;
      break;
    case 0x40:
      ret->samplerate = 44100 >> half;
      ret->total_bytes =  2 * (320 * bitrate / 147 + (frmsizecod & 1));
      break;
    case 0x80:
      ret->samplerate = 32000 >> half;
      ret->total_bytes =  6 * bitrate;
      break;
    default:
      return 0;
    }
  
  return 1;
  }
#endif // HAVE_GPL

/* Codec */

typedef struct
  {
  AVCodecContext * avctx;
  AVCodec * encoder;
  AVCodec * decoder;

  int initialized;
  
  /* Interleaved samples as avcodec needs them */
    
  int16_t * sample_buffer;
  int sample_buffer_alloc; 
  int samples_in_buffer;
  
  /* Buffer for the entire chunk */

  uint8_t * chunk_buffer;
  int chunk_buffer_alloc;
  int bytes_in_chunk_buffer;

  /* Start and end positions of the sample buffer */
    
  int64_t sample_buffer_start;
  int64_t sample_buffer_end;

  mpa_header mph;
  int have_mpa_header;

  uint8_t * extradata;
#if LIBAVCODEC_BUILD >= ((52<<16)+(26<<8)+0)
  AVPacket pkt;
#endif

  int64_t pts;
  } quicktime_ffmpeg_audio_codec_t;

static int lqt_ffmpeg_delete_audio(quicktime_codec_t *codec_base)
  {
  quicktime_ffmpeg_audio_codec_t * codec = codec_base->priv;
  if(codec->avctx)
    {
    if(codec->initialized)
      avcodec_close(codec->avctx);
    av_free(codec->avctx);
    }
  if(codec->sample_buffer)
    free(codec->sample_buffer);
  if(codec->chunk_buffer)
    free(codec->chunk_buffer);
  if(codec->extradata)
    free(codec->extradata); 
  free(codec);
  return 0;
  }

static int set_parameter(quicktime_t *file, 
                  int track, 
                  const char *key, 
                  const void *value)
  {
  quicktime_ffmpeg_audio_codec_t *codec = file->atracks[track].codec->priv;
  lqt_ffmpeg_set_parameter(codec->avctx, key, value);
  return 0;
  }


/* Decode VBR chunk into the sample buffer */

static int decode_chunk_vbr(quicktime_t * file, int track)
  {
  int chunk_packets, i, num_samples, bytes_decoded;
  int packet_size, packet_samples;
  int frame_bytes;
  int new_samples;
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = track_map->codec->priv;

  chunk_packets = lqt_audio_num_vbr_packets(file, track, track_map->cur_chunk, &num_samples);

  if(!chunk_packets)
    return 0;

  new_samples = num_samples + AVCODEC_MAX_AUDIO_FRAME_SIZE / (2 * track_map->channels);
  
  if(codec->sample_buffer_alloc <
     codec->sample_buffer_end - codec->sample_buffer_start + new_samples)
    {
    
    codec->sample_buffer_alloc = codec->sample_buffer_end - codec->sample_buffer_start + new_samples;
    codec->sample_buffer = realloc(codec->sample_buffer, 2 * codec->sample_buffer_alloc *
                                   track_map->channels);
    }

  for(i = 0; i < chunk_packets; i++)
    {
    packet_size = lqt_audio_read_vbr_packet(file, track, track_map->cur_chunk, i,
                                            &codec->chunk_buffer, &codec->chunk_buffer_alloc,
                                            &packet_samples);
    if(!packet_size)
      return 0;

#if LIBAVCODEC_BUILD >= 3349760
    bytes_decoded = codec->sample_buffer_alloc -
      (codec->sample_buffer_end - codec->sample_buffer_start);
    bytes_decoded *= 2 * track_map->channels;
#else
    bytes_decoded = 0;
#endif

#if LIBAVCODEC_BUILD >= ((52<<16)+(26<<8)+0)
    codec->pkt.data = codec->chunk_buffer;
    codec->pkt.size = packet_size + FF_INPUT_BUFFER_PADDING_SIZE;
    
    frame_bytes = avcodec_decode_audio3(codec->avctx,
                                        &codec->sample_buffer[track_map->channels *
                                                               (codec->sample_buffer_end -
                                                                codec->sample_buffer_start)],
                                        &bytes_decoded,
                                        &codec->pkt);
#else
    frame_bytes =
      DECODE_FUNC(codec->avctx,
                  &codec->sample_buffer[track_map->channels *
                                         (codec->sample_buffer_end - codec->sample_buffer_start)],
                  &bytes_decoded,
                  codec->chunk_buffer,
                  packet_size + FF_INPUT_BUFFER_PADDING_SIZE);
#endif
    if(frame_bytes < 0)
      {
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "avcodec_decode_audio error");
      break;
      }
    codec->sample_buffer_end += (bytes_decoded / (track_map->channels * 2));
    }
  track_map->cur_chunk++;
  return num_samples;
  }


/* Decode the current chunk into the sample buffer */

static int decode_chunk(quicktime_t * file, int track)
  {
  mpa_header mph;
    
  int frame_bytes;
  int num_samples;
  int new_samples;
  int samples_decoded = 0;
  int bytes_decoded;
  int bytes_used, bytes_skipped;
  int64_t chunk_size;
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = track_map->codec->priv;

  /* Read chunk */
  
  chunk_size = lqt_append_audio_chunk(file,
                                      track, track_map->cur_chunk,
                                      &codec->chunk_buffer,
                                      &codec->chunk_buffer_alloc,
                                      codec->bytes_in_chunk_buffer);

  
  if(!chunk_size)
    {
    /* If the codec is mp3, make sure to decode the very last frame */

    if((codec->avctx->codec_id == CODEC_ID_MP3) &&
       (codec->bytes_in_chunk_buffer >= 4))
      {
      if(!mpa_decode_header(&mph, codec->chunk_buffer, (const mpa_header*)0))
        {
        lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "Decode header failed");
        return 0;
        }
      if(mph.frame_bytes <= codec->bytes_in_chunk_buffer)
        {
        lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "Huh, frame not decoded?");
        return 0;
        }

      if(codec->chunk_buffer_alloc < mph.frame_bytes + FF_INPUT_BUFFER_PADDING_SIZE)
        {
        codec->chunk_buffer_alloc = mph.frame_bytes + FF_INPUT_BUFFER_PADDING_SIZE;
        codec->chunk_buffer = realloc(codec->chunk_buffer, codec->chunk_buffer_alloc);
        }
      memset(codec->chunk_buffer + codec->bytes_in_chunk_buffer, 0,
             mph.frame_bytes - codec->bytes_in_chunk_buffer + FF_INPUT_BUFFER_PADDING_SIZE);
      num_samples = mph.samples_per_frame;
      codec->bytes_in_chunk_buffer = mph.frame_bytes;
      }
    else
      return 0;
    }
  else
    {
    num_samples = quicktime_chunk_samples(track_map->track, track_map->cur_chunk);
    track_map->cur_chunk++;
    codec->bytes_in_chunk_buffer += chunk_size;
    }
  

  if(!num_samples)
    {
    return 0;
    }
  /*
   *  For AVIs, chunk samples are not always 100% correct.
   *  Furthermore, there can be a complete mp3 frame from the last chunk!
   */

  num_samples += 8192;
  new_samples = num_samples + AVCODEC_MAX_AUDIO_FRAME_SIZE / (2 * track_map->channels);
  
  /* Reallocate sample buffer */
  
  if(codec->sample_buffer_alloc < codec->sample_buffer_end - codec->sample_buffer_start + new_samples)
    {
    
    codec->sample_buffer_alloc = codec->sample_buffer_end - codec->sample_buffer_start + new_samples;
    codec->sample_buffer = realloc(codec->sample_buffer, 2 * codec->sample_buffer_alloc *
                                   track_map->channels);
    }
  
  /* Decode this */

  bytes_used = 0;
  while(1)
    {

        
    /* BIG NOTE: We pass extra FF_INPUT_BUFFER_PADDING_SIZE for the buffer size
       because we know, that lqt_read_audio_chunk allocates 16 extra bytes for us */
    
    /* Some really broken mp3 files have the header bytes split across 2 chunks */

    if(codec->avctx->codec_id == CODEC_ID_MP3)
      {
      if(codec->bytes_in_chunk_buffer < 4)
        {
        
        if(codec->bytes_in_chunk_buffer > 0)
          memmove(codec->chunk_buffer,
                  codec->chunk_buffer + bytes_used, codec->bytes_in_chunk_buffer);
        return 1;
        }

      bytes_skipped = 0;
      while(1)
        {
        if(!codec->have_mpa_header)
          {
          if(mpa_decode_header(&mph, &codec->chunk_buffer[bytes_used], NULL))
            {
            memcpy(&codec->mph, &mph, sizeof(mph));
            codec->have_mpa_header = 1;
            break;
            }
          }
        else if(mpa_decode_header(&mph, &codec->chunk_buffer[bytes_used], &codec->mph))
          break;
        
        bytes_used++;
        codec->bytes_in_chunk_buffer--;
        bytes_skipped++;
        if(codec->bytes_in_chunk_buffer <= 4)
          {

          if(codec->bytes_in_chunk_buffer > 0)
            memmove(codec->chunk_buffer,
                    codec->chunk_buffer + bytes_used, codec->bytes_in_chunk_buffer);
          return 1;
          }
        }
      if(codec->bytes_in_chunk_buffer < mph.frame_bytes)
        {
        
        if(codec->bytes_in_chunk_buffer > 0)
          memmove(codec->chunk_buffer,
                  codec->chunk_buffer + bytes_used, codec->bytes_in_chunk_buffer);
        return 1;
        }
      }

    /*
     *  decode an audio frame. return -1 if error, otherwise return the
     *  number of bytes used. If no frame could be decompressed,
     *  frame_size_ptr is zero. Otherwise, it is the decompressed frame
     *  size in BYTES.
     */

#if LIBAVCODEC_BUILD >= 3349760
    bytes_decoded = codec->sample_buffer_alloc -
      (codec->sample_buffer_end - codec->sample_buffer_start);
    bytes_decoded *= 2 * track_map->channels;
#else
    bytes_decoded = 0;
#endif

#if LIBAVCODEC_BUILD >= ((52<<16)+(26<<8)+0)
    codec->pkt.data = &codec->chunk_buffer[bytes_used];
    codec->pkt.size = codec->bytes_in_chunk_buffer + FF_INPUT_BUFFER_PADDING_SIZE;
    
    frame_bytes =
      avcodec_decode_audio3(codec->avctx,
                            &codec->sample_buffer[track_map->channels *
                                                   (codec->sample_buffer_end - codec->sample_buffer_start)],
                            &bytes_decoded,
                            &codec->pkt);
#else
    frame_bytes =
      DECODE_FUNC(codec->avctx,
                  &codec->sample_buffer[track_map->channels *
                                        (codec->sample_buffer_end - codec->sample_buffer_start)],
                  &bytes_decoded,
                  &codec->chunk_buffer[bytes_used],
                  codec->bytes_in_chunk_buffer + FF_INPUT_BUFFER_PADDING_SIZE);
#endif
    if(frame_bytes < 0)
      {
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "avcodec_decode_audio error");
      break;
      }

    bytes_used                   += frame_bytes;
    codec->bytes_in_chunk_buffer -= frame_bytes;
    
    if(bytes_decoded < 0)
      {
      if(codec->avctx->codec_id == CODEC_ID_MP3)
        {
        /* For mp3, bytes_decoded < 0 means, that the frame should be muted */
        memset(&codec->sample_buffer[track_map->channels * (codec->sample_buffer_end -
                                                            codec->sample_buffer_start)],
               0, 2 * mph.samples_per_frame * track_map->channels);
        
        codec->sample_buffer_end += mph.samples_per_frame * track_map->channels;

        if(codec->bytes_in_chunk_buffer < 0)
          codec->bytes_in_chunk_buffer = 0;

        if(!codec->bytes_in_chunk_buffer)
          break;

        continue;
        }
      else
        {
        /* Incomplete frame, save the data for later use and exit here */
        if(codec->bytes_in_chunk_buffer > 0)
          memmove(codec->chunk_buffer,
                  codec->chunk_buffer + bytes_used, codec->bytes_in_chunk_buffer);
        return 1;
        }
      }
    
    /* This happens because ffmpeg adds FF_INPUT_BUFFER_PADDING_SIZE to the bytes returned */
    
    if(codec->bytes_in_chunk_buffer < 0)
      codec->bytes_in_chunk_buffer = 0;

    if(bytes_decoded < 0)
      {
      if(codec->bytes_in_chunk_buffer > 0)
        codec->bytes_in_chunk_buffer = 0;
      break;
      }
    
    samples_decoded += (bytes_decoded / (track_map->channels * 2));
    codec->sample_buffer_end += (bytes_decoded / (track_map->channels * 2));

    if((int)(codec->sample_buffer_end - codec->sample_buffer_start) > codec->sample_buffer_alloc)
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "BUUUUG, buffer overflow, %d %d",
              (int)(codec->sample_buffer_end - codec->sample_buffer_start),
              codec->sample_buffer_alloc);
    
    if(!codec->bytes_in_chunk_buffer)
      break;

    }
  //  track_map->current_chunk++;
  return samples_decoded;
  }

static void init_compression_info(quicktime_t *file, int track)
  {
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = track_map->codec->priv;

  if((codec->decoder->id == CODEC_ID_MP2) ||
     (codec->decoder->id == CODEC_ID_MP3))
    {
    mpa_header h;
    uint32_t header;
    uint8_t * ptr;
    int chunk_size;
    
    chunk_size = lqt_append_audio_chunk(file,
                                        track, track_map->cur_chunk,
                                        &codec->chunk_buffer,
                                        &codec->chunk_buffer_alloc,
                                        codec->bytes_in_chunk_buffer);

    if(chunk_size + codec->bytes_in_chunk_buffer < 4)
      return;

    ptr = codec->chunk_buffer;
    while(1)
      {
      header =
        ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24);
      if(mpa_header_check(header))
        break;

      ptr++;
      if(ptr - codec->chunk_buffer > codec->bytes_in_chunk_buffer - 4)
        return;
      }
    
    if(!mpa_decode_header(&h, ptr, NULL))
      return;

    if(h.layer == 2)
      track_map->ci.id = LQT_COMPRESSION_MP2;
    else if(h.layer == 3)
      track_map->ci.id = LQT_COMPRESSION_MP3;
    
    if(lqt_audio_is_vbr(file, track))
      track_map->ci.bitrate = -1;
    else
      track_map->ci.bitrate = h.bitrate;
    }
#ifdef HAVE_GPL
  else if(codec->decoder->id == CODEC_ID_AC3)
    {
    a52_header h;
    uint8_t * ptr;
    int chunk_size;

    chunk_size = lqt_append_audio_chunk(file,
                                        track, track_map->cur_chunk,
                                        &codec->chunk_buffer,
                                        &codec->chunk_buffer_alloc,
                                        codec->bytes_in_chunk_buffer);

    if(chunk_size + codec->bytes_in_chunk_buffer < 7)
      return;
    
    ptr = codec->chunk_buffer;
    while(1)
      {
      if(a52_header_read(&h, ptr))
        break;
      
      ptr++;
      if(ptr - codec->chunk_buffer > codec->bytes_in_chunk_buffer - 7)
        return;
      }
    
    track_map->ci.bitrate = h.bitrate;
    track_map->ci.id = LQT_COMPRESSION_AC3;
    }
#endif  
  }

static int lqt_ffmpeg_decode_audio(quicktime_t *file, void * output, long samples, int track)
  {
  uint8_t * header;
  uint32_t header_len;
  
  int samples_decoded;
  //  int result = 0;
  int64_t chunk_sample; /* For seeking only */
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = track_map->codec->priv;
  int channels = file->atracks[track].channels;
  //  int64_t total_samples;

  int samples_to_skip;
  int samples_to_move;


  if(!output) /* Global initialization */
    {
    return 0;
    }
  
  /* Initialize codec */
  if(!codec->initialized)
    {
    init_compression_info(file, track);
    /* Set some mandatory variables */
    codec->avctx->channels        = quicktime_track_channels(file, track);
    codec->avctx->sample_rate     = quicktime_sample_rate(file, track);

    if(track_map->track->mdia.minf.stbl.stsd.table[0].version == 1)
      {
      if(track_map->track->mdia.minf.stbl.stsd.table[0].audio_bytes_per_frame)
        codec->avctx->block_align =
          track_map->track->mdia.minf.stbl.stsd.table[0].audio_bytes_per_frame;
      }
    
    //  priv->ctx->block_align     = s->data.audio.block_align;
    //  priv->ctx->bit_rate        = s->codec_bitrate;
#if LIBAVCODEC_VERSION_INT < ((52<<16)+(0<<8)+0)
    codec->avctx->bits_per_sample = quicktime_audio_bits(file, track);
#else
    codec->avctx->bits_per_coded_sample = quicktime_audio_bits(file, track);
#endif
    /* Some codecs need extra stuff */

    if(codec->decoder->id == CODEC_ID_ALAC)
      {
      header = quicktime_wave_get_user_atom(track_map->track, "alac", &header_len);
      if(header)
        {
        codec->avctx->extradata = header;
        codec->avctx->extradata_size = header_len;
        }
      }
    if(codec->decoder->id == CODEC_ID_QDM2)
      {
      header = quicktime_wave_get_user_atom(track_map->track, "QDCA", &header_len);
      if(header)
        {
        codec->extradata = malloc(header_len + 12);
        /* frma atom */
        codec->extradata[0] = 0x00;
        codec->extradata[1] = 0x00;
        codec->extradata[2] = 0x00;
        codec->extradata[3] = 0x0C;
        memcpy(codec->extradata + 4, "frmaQDM2", 8);
        /* QDCA atom */
        memcpy(codec->extradata + 12, header, header_len);
        codec->avctx->extradata = codec->extradata;
        codec->avctx->extradata_size = header_len + 12;
        }

      }

    
    //    memcpy(&codec->com.ffcodec_enc, &codec->com.params, sizeof(AVCodecContext));
   
    codec->avctx->codec_id = codec->decoder->id;
    codec->avctx->codec_type = codec->decoder->type;
 
    if(avcodec_open(codec->avctx, codec->decoder) != 0)
      {
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "Avcodec open failed");
      return 0;
      }
    
    //    codec->sample_buffer_offset = 0;
    codec->initialized = 1;
    }

  /* Check if we have to reposition the stream */
  
  if(track_map->last_position != track_map->current_position)
    {

    if((track_map->current_position < codec->sample_buffer_start) || 
       (track_map->current_position + samples >= codec->sample_buffer_end))
      {
      if(lqt_audio_is_vbr(file, track))
        lqt_chunk_of_sample_vbr(&chunk_sample,
                                &track_map->cur_chunk,
                                track_map->track,
                                track_map->current_position);
      else
        quicktime_chunk_of_sample(&chunk_sample,
                                  &track_map->cur_chunk,
                                  track_map->track,
                                  track_map->current_position);
      codec->sample_buffer_start = chunk_sample;
      codec->sample_buffer_end   = chunk_sample;
      codec->bytes_in_chunk_buffer = 0;

      if(lqt_audio_is_vbr(file, track))
        decode_chunk_vbr(file, track);
      else
        decode_chunk(file, track);
      }
    }
  
  /* Flush unneeded samples */
  samples_to_skip = 0;
  if(track_map->current_position > codec->sample_buffer_start)
    {
    samples_to_skip = track_map->current_position - codec->sample_buffer_start;
    if(samples_to_skip > (int)(codec->sample_buffer_end - codec->sample_buffer_start))
      samples_to_skip = (int)(codec->sample_buffer_end - codec->sample_buffer_start);
    
    if(codec->sample_buffer_end > track_map->current_position)
      {
      samples_to_move = codec->sample_buffer_end - track_map->current_position;

      memmove(codec->sample_buffer,
              &codec->sample_buffer[samples_to_skip * channels],
              samples_to_move * channels * sizeof(int16_t));
      }
    codec->sample_buffer_start += samples_to_skip;
    

    }
  samples_to_skip = track_map->current_position - codec->sample_buffer_start;
  
  /* Read new chunks until we have enough samples */
  while(codec->sample_buffer_end - codec->sample_buffer_start < samples + samples_to_skip)
    {
    
    //    if(track_map->current_chunk >= track_map->track->mdia.minf.stbl.stco.total_entries)
    //      return 0;

    if(lqt_audio_is_vbr(file, track))
      {
      if(!decode_chunk_vbr(file, track))
        break;
      }
    else
      {
      if(!decode_chunk(file, track))
        break;
      }
    }
  samples_decoded = codec->sample_buffer_end - codec->sample_buffer_start - samples_to_skip;

  
  if(samples_decoded <= 0)
    {
    track_map->last_position = track_map->current_position;
    return 0;
    }
  if(samples_decoded > samples)
    samples_decoded = samples;
  
  /* Deinterleave into the buffer */
  
  
  //  deinterleave(output_i, output_f, codec->sample_buffer + (track_map->channels * samples_to_skip),
  //               channels, samples_decoded);

  memcpy(output, codec->sample_buffer + (channels * samples_to_skip),
         channels * samples_decoded * 2);
  
  track_map->last_position = track_map->current_position + samples_decoded;

  return samples_decoded;
  // #endif
  }

/*
 *   Encoding part
 */

static int lqt_ffmpeg_encode_audio(quicktime_t *file, void * input,
                            long samples, int track)
  {
  int result = -1;
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = track_map->codec->priv;
  quicktime_trak_t *trak = track_map->track;
  int channels = file->atracks[track].channels;
  int frame_bytes;
  int samples_done = 0;
  int samples_encoded;
  /* Initialize encoder */
    
  if(!codec->initialized)
    {
    codec->avctx->sample_rate = track_map->samplerate;
    codec->avctx->channels = channels;

    codec->avctx->codec_id = codec->encoder->id;
    codec->avctx->codec_type = codec->encoder->type;

    if(avcodec_open(codec->avctx, codec->encoder) != 0)
      {
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "Avcodec open failed");
      return -1;
      }
    
    codec->initialized = 1;

    /* One frame is: bitrate * frame_samples / (samplerate * 8) + 1024 */
    codec->chunk_buffer_alloc = ( codec->avctx->frame_size
                                  * sizeof( int16_t )
                                  * codec->avctx->channels);
    codec->chunk_buffer = malloc(codec->chunk_buffer_alloc);

    if(trak->strl)
      {
      /* strh stuff */
      trak->strl->strh.dwRate = codec->avctx->bit_rate / 8;
      trak->strl->strh.dwScale = 1;
      trak->strl->strh.dwSampleSize = 1;
      /* WAVEFORMATEX stuff */
      
      trak->strl->strf.wf.f.WAVEFORMAT.nBlockAlign = 1;
      trak->strl->strf.wf.f.WAVEFORMAT.nAvgBytesPerSec =  codec->avctx->bit_rate / 8;
      trak->strl->strf.wf.f.PCMWAVEFORMAT.wBitsPerSample = 0;
      }

    }

  /* Allocate sample buffer if necessary */

  if(codec->sample_buffer_alloc < (codec->samples_in_buffer + samples))
    {
    codec->sample_buffer_alloc = codec->samples_in_buffer + samples + 16;
    codec->sample_buffer = realloc(codec->sample_buffer,
                                   codec->sample_buffer_alloc * channels * sizeof(int16_t));
    }

  /* Interleave */

  //  interleave(&codec->sample_buffer[codec->samples_in_buffer * channels],
  //             input_i, input_f, samples, channels);

  memcpy(codec->sample_buffer + codec->samples_in_buffer * channels,
         input, samples * channels * 2);
  
  codec->samples_in_buffer += samples;
  
  /* Encode */
  
  while(codec->samples_in_buffer >= codec->avctx->frame_size)
    {
    
    frame_bytes = avcodec_encode_audio(codec->avctx, codec->chunk_buffer,
                                       codec->chunk_buffer_alloc,
                                       &codec->sample_buffer[samples_done*channels]);
    if(frame_bytes > 0)
      {
      quicktime_write_chunk_header(file, trak);
#if 0    // PATCH 3  
      if(codec->avctx->frame_size == 1)
        samples_encoded = codec->samples_in_buffer;
      else
#endif
        samples_encoded = codec->avctx->frame_size;


      samples_done              += samples_encoded;
      codec->samples_in_buffer  -= samples_encoded;
      
      result = !quicktime_write_data(file, codec->chunk_buffer, frame_bytes);
      trak->chunk_samples = samples_encoded;
      quicktime_write_chunk_footer(file, trak);
      file->atracks[track].cur_chunk++;
      }
    }
  if(codec->samples_in_buffer && samples_done)
    memmove(codec->sample_buffer, &codec->sample_buffer[samples_done*channels],
            codec->samples_in_buffer * sizeof(int16_t) * channels);
  return result;
  }

static int read_packet_mpa(quicktime_t * file, lqt_packet_t * p, int track)
  {
  mpa_header h;
  uint8_t * ptr;
  uint32_t header;
  int chunk_size;
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = track_map->codec->priv;
  
  if(codec->bytes_in_chunk_buffer < 4)
    {
    chunk_size = lqt_append_audio_chunk(file,
                                        track, track_map->cur_chunk,
                                        &codec->chunk_buffer,
                                        &codec->chunk_buffer_alloc,
                                        codec->bytes_in_chunk_buffer);

    if(chunk_size + codec->bytes_in_chunk_buffer < 4)
      return 0;

    codec->bytes_in_chunk_buffer += chunk_size;
    track_map->cur_chunk++;
    }

  /* Check for mpa header */

  ptr = codec->chunk_buffer;
  while(1)
    {
    header =
      ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24);
    if(mpa_header_check(header))
      break;
    
    ptr++;

    if(ptr - codec->chunk_buffer > codec->bytes_in_chunk_buffer - 4)
      return 0;
    }

  if(!mpa_decode_header(&h, ptr, NULL))
    return 0;
  
  lqt_packet_alloc(p, h.frame_bytes);
  memcpy(p->data, ptr, h.frame_bytes);
  ptr += h.frame_bytes;

  codec->bytes_in_chunk_buffer -= (ptr - codec->chunk_buffer);

  if(codec->bytes_in_chunk_buffer)
    memmove(codec->chunk_buffer, ptr, codec->bytes_in_chunk_buffer);

  p->duration = h.samples_per_frame;
  p->timestamp = codec->pts;
  codec->pts += p->duration;
  p->flags = LQT_PACKET_KEYFRAME;
  return 1;
  }

static int writes_compressed_mp2(lqt_file_type_t type, const lqt_compression_info_t * ci)
  {
  return 1;
  }

static int init_compressed_mp2(quicktime_t * file, int track)
  {
  
  return 0;
  }

static int writes_compressed_ac3(lqt_file_type_t type, const lqt_compression_info_t * ci)
  {
  return 1;
  }

static int init_compressed_ac3(quicktime_t * file, int track)
  {
  
  return 0;
  }



#ifdef HAVE_GPL
static int read_packet_ac3(quicktime_t * file, lqt_packet_t * p, int track)
  {
  a52_header h;
  uint8_t * ptr;
  int chunk_size;
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = track_map->codec->priv;
  
  if(codec->bytes_in_chunk_buffer < 7)
    {
    chunk_size = lqt_append_audio_chunk(file,
                                        track, track_map->cur_chunk,
                                        &codec->chunk_buffer,
                                        &codec->chunk_buffer_alloc,
                                        codec->bytes_in_chunk_buffer);

    if(chunk_size + codec->bytes_in_chunk_buffer < 7)
      return 0;

    codec->bytes_in_chunk_buffer += chunk_size;
    track_map->cur_chunk++;
    }

  /* Check for mpa header */

  ptr = codec->chunk_buffer;
  while(1)
    {
    if(!a52_header_read(&h, ptr))
      return 0;
    
    ptr++;

    if(ptr - codec->chunk_buffer > codec->bytes_in_chunk_buffer - 7)
      return 0;
    }
  
  lqt_packet_alloc(p, h.total_bytes);
  memcpy(p->data, ptr, h.total_bytes);
  ptr += h.total_bytes;

  codec->bytes_in_chunk_buffer -= (ptr - codec->chunk_buffer);

  if(codec->bytes_in_chunk_buffer)
    memmove(codec->chunk_buffer, ptr, codec->bytes_in_chunk_buffer);
  
  p->duration = A52_FRAME_SAMPLES;
  p->timestamp = codec->pts;
  codec->pts += p->duration;
  p->flags = LQT_PACKET_KEYFRAME;
  return 1;
  }
#endif

void quicktime_init_audio_codec_ffmpeg(quicktime_codec_t * codec_base,
                                       quicktime_audio_map_t *atrack, AVCodec *encoder,
                                       AVCodec *decoder)
  {
  quicktime_ffmpeg_audio_codec_t *codec;
  
  avcodec_init();
  codec = calloc(1, sizeof(*codec));
  if(!codec)
    return;
  
  codec->encoder = encoder;
  codec->decoder = decoder;
  codec-> avctx = avcodec_alloc_context();
  codec_base->priv = (void *)codec;

  codec_base->delete_codec = lqt_ffmpeg_delete_audio;
  if(encoder)
    codec_base->encode_audio = lqt_ffmpeg_encode_audio;
  if(decoder)
    codec_base->decode_audio = lqt_ffmpeg_decode_audio;
  codec_base->set_parameter = set_parameter;

  if((decoder->id == CODEC_ID_MP3) || (decoder->id == CODEC_ID_MP2))
    codec_base->read_packet = read_packet_mpa;
#ifdef HAVE_GPL
  else if(decoder->id == CODEC_ID_AC3)
    codec_base->read_packet = read_packet_ac3;
#endif
  
  if(!atrack)
    return;
  
  atrack->sample_format = LQT_SAMPLE_INT16;
  }
