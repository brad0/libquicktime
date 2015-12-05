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

#include "lqt_private.h"
#include "ffmpeg.h"
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>
#include <stdlib.h>
#include <string.h>

#define LOG_DOMAIN "ffmpeg_audio"

#ifndef AVCODEC_MAX_AUDIO_FRAME_SIZE
/* from libavcodec/avcodec.h dated Dec 23 2012 */
#define AVCODEC_MAX_AUDIO_FRAME_SIZE 192000 // 1 second of 48khz 32bit audio
#endif

/* MPEG Audio header */

/* The following code was ported from gmerlin_avdecoder (http://gmerlin.sourceforge.net) */

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


/* Header detection stolen from the mpg123 plugin of xmms */

static int mpa_header_check(uint32_t head);
static int mpa_header_equal(const mpa_header * h1, const mpa_header * h2);
static int mpa_decode_header(mpa_header * h, uint8_t * ptr,  const mpa_header * ref);

static int read_packet_mpa(quicktime_t * file, lqt_packet_t * p, int track);


/* AC3 header */

typedef struct
  {
  /* Primary */
  int fscod;
  int frmsizecod;
  int bsid;
  int bsmod;
  int acmod;
  int cmixlev;
  int surmixlev;
  int dsurmod;
  int lfeon;
  
  /* Secondary */
  int frame_bytes;
  int bitrate;
  } a52_header;

#define A52_FRAME_SAMPLES 1536
static int a52_header_read(a52_header * ret, uint8_t * buf);

static int read_packet_ac3(quicktime_t * file, lqt_packet_t * p, int track);


/* Codec */

typedef struct
  {
  AVCodecContext * avctx;
  AVCodec * encoder;
  AVCodec * decoder;

  AVFrame * frame;
  
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
  AVPacket pkt;
  lqt_packet_t lqt_pkt;
  lqt_packet_t lqt_pkt_parse; // For read_packet() functions
  
  int64_t pts; /* For reading compressed packets */

  int header_written;
  AVDictionary * options;

  AVFrame * f;
  int have_frame;

  int bytes_per_sample;
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
  if(codec->options)
    av_dict_free(&codec->options);
  av_frame_free(&codec->f); 
  lqt_packet_free(&codec->lqt_pkt);
  lqt_packet_free(&codec->lqt_pkt_parse);
  free(codec);
  return 0;
  }
static int set_parameter(quicktime_t *file, 
                  int track, 
                  const char *key, 
                  const void *value)
  {
  quicktime_ffmpeg_audio_codec_t *codec = file->atracks[track].codec->priv;
  lqt_ffmpeg_set_parameter(codec->avctx,
                           &codec->options,
                           key, value);
  return 0;
  }

static void init_compression_info(quicktime_t *file, int track)
  {
  quicktime_audio_map_t *track_map = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = track_map->codec->priv;

  if((codec->decoder->id == AV_CODEC_ID_MP2) ||
     (codec->decoder->id == AV_CODEC_ID_MP3))
    read_packet_mpa(file, NULL, track);
  else if(codec->decoder->id == AV_CODEC_ID_AC3)
    read_packet_ac3(file, NULL, track);
  }

static struct
  {
  enum AVSampleFormat ffmpeg_fmt;
  lqt_sample_format_t lqt_fmt;
  int planar;
  }
sample_formats[] =
  {
    { AV_SAMPLE_FMT_U8,   LQT_SAMPLE_UINT8,  0 }, ///< unsigned 8 bits
    { AV_SAMPLE_FMT_S16,  LQT_SAMPLE_INT16,  0 }, ///< signed 16 bits
    { AV_SAMPLE_FMT_S32,  LQT_SAMPLE_INT32,  0 }, ///< signed 32 bits
    { AV_SAMPLE_FMT_FLT,  LQT_SAMPLE_FLOAT,  0 }, ///< float
    { AV_SAMPLE_FMT_DBL,  LQT_SAMPLE_DOUBLE, 0 }, ///< double

    { AV_SAMPLE_FMT_U8P,  LQT_SAMPLE_UINT8,  1 }, ///< unsigned 8 bits, planar
    { AV_SAMPLE_FMT_S16P, LQT_SAMPLE_INT16,  1 }, ///< signed 16 bits, planar
    { AV_SAMPLE_FMT_S32P, LQT_SAMPLE_INT32,  1 }, ///< signed 32 bits, planar
    { AV_SAMPLE_FMT_FLTP, LQT_SAMPLE_FLOAT,  1 }, ///< float, planar
    { AV_SAMPLE_FMT_DBLP, LQT_SAMPLE_DOUBLE, 1 }, ///< double, planar
    { AV_SAMPLE_FMT_NONE  },
  };

static lqt_sample_format_t get_sample_format(enum AVSampleFormat ffmpeg_fmt, int * planar)
  {
  int i = 0;
  while(sample_formats[i].ffmpeg_fmt != AV_SAMPLE_FMT_NONE)
    {
    if(sample_formats[i].ffmpeg_fmt == ffmpeg_fmt)
      {
      *planar = sample_formats[i].planar;
      return sample_formats[i].lqt_fmt;
      }
    i++;
    }
  *planar = 0;
  return LQT_SAMPLE_UNDEFINED;
  }

static int decode_audio_packet_ffmpeg(quicktime_t *file,
                                      int track, lqt_audio_buffer_t * buf)
  {
  uint8_t * header;
  uint32_t header_len;
  
  //  int result = 0;
  quicktime_audio_map_t *atrack = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = atrack->codec->priv;
  //  int64_t total_samples;

  int frame_bytes;
  
  if(!buf) /* Global initialization */
    init_compression_info(file, track);
  
  /* Initialize codec */
  if(!codec->initialized)
    {
    /* Set some mandatory variables */
    codec->avctx->channels        = quicktime_track_channels(file, track);
    codec->avctx->sample_rate     = quicktime_sample_rate(file, track);

    if(atrack->track->mdia.minf.stbl.stsd.table[0].version == 1)
      {
      if(atrack->track->mdia.minf.stbl.stsd.table[0].audio_bytes_per_frame)
        codec->avctx->block_align =
          atrack->track->mdia.minf.stbl.stsd.table[0].audio_bytes_per_frame;
      }
    
    //  priv->ctx->block_align     = s->data.audio.block_align;
    //  priv->ctx->bit_rate        = s->codec_bitrate;
    codec->avctx->bits_per_coded_sample = quicktime_audio_bits(file, track);
    /* Some codecs need extra stuff */

    if(codec->decoder->id == AV_CODEC_ID_ALAC)
      {
      header = quicktime_wave_get_user_atom(atrack->track, "alac", &header_len);
      if(header)
        {
        codec->avctx->extradata = header;
        codec->avctx->extradata_size = header_len;
        }
      }
    if(codec->decoder->id == AV_CODEC_ID_QDM2)
      {
      header = quicktime_wave_get_user_atom(atrack->track, "QDCA", &header_len);
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

    if(avcodec_open2(codec->avctx, codec->decoder, NULL) != 0)
      {
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "avcodec_open2 failed");
      return 0;
      }
    //    codec->sample_buffer_offset = 0;
    codec->initialized = 1;
    }

  while(!codec->have_frame || !codec->f->nb_samples)
    {
    if(!codec->lqt_pkt.data_len)
      {
      if(atrack->codec->read_packet)
        {
        if(!atrack->codec->read_packet(file, &codec->lqt_pkt, track))
          return 0;
        }
      else if(!quicktime_trak_read_packet(file, atrack->track, &codec->lqt_pkt))
        return 0;
      }
    
    codec->pkt.data = codec->lqt_pkt.data;
    codec->pkt.size = codec->lqt_pkt.data_len;
    
    frame_bytes = avcodec_decode_audio4(codec->avctx, codec->f, &codec->have_frame, &codec->pkt);

    if(frame_bytes < 0)
      {
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "avcodec_decode_audio4 failed");
      return 0;
      }
    
    if(frame_bytes > 0)
      lqt_packet_flush(&codec->lqt_pkt, frame_bytes);

    if(atrack->sample_format == LQT_SAMPLE_UNDEFINED)
      {
      /* Set interleaving and sample format */
      atrack->sample_format = get_sample_format(codec->avctx->sample_fmt, &atrack->planar);
      codec->bytes_per_sample = lqt_sample_format_bytes(atrack->sample_format);
      }
    }
  
  if(!buf)
    return 0;

  /* Copy the samples to the buffer */

  lqt_audio_buffer_alloc(buf, codec->f->nb_samples, atrack->channels, atrack->planar, atrack->sample_format);
  
  if(atrack->planar)
    {
    int i;
    for(i = 0; i < atrack->channels; i++)
      memcpy(buf->channels[i].u_8, codec->f->extended_data[i], codec->bytes_per_sample * codec->f->nb_samples);
    }
  else
    {
    memcpy(buf->channels[0].u_8, codec->f->extended_data[0],
           codec->bytes_per_sample * codec->f->nb_samples * atrack->channels);
    }
  buf->size = codec->f->nb_samples;
  codec->have_frame = 0;
  return buf->size;
  }

static void resync_ffmpeg(quicktime_t *file, int track)
  {
  quicktime_audio_map_t *atrack = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = atrack->codec->priv;

  codec->lqt_pkt_parse.data_len = 0;
  codec->lqt_pkt.data_len = 0;
  codec->have_frame = 0;
  avcodec_flush_buffers(codec->avctx);
  }

/*
 *   Encoding part
 */

static void create_dac3_atom(quicktime_t * file, int track, uint8_t * buf)
  {
  uint32_t tmp;
  uint8_t dac3_data[3];
  a52_header header;
  quicktime_trak_t *trak = file->atracks[track].track;
  
  if(a52_header_read(&header, buf))
    {
    tmp = header.fscod;

    tmp <<= 5;
    tmp |= header.bsid;
    
    tmp <<= 3;
    tmp |= header.bsmod;

    tmp <<= 3;
    tmp |= header.acmod;

    tmp <<= 1;
    tmp |= header.lfeon;

    tmp <<= 5;
    tmp |= header.frmsizecod >> 1;

    tmp <<= 5;

    dac3_data[0] = tmp >> 16;
    dac3_data[1] = (tmp >>  8) & 0xff;
    dac3_data[2] = tmp & 0xff;
    
    quicktime_user_atoms_add_atom(&trak->mdia.minf.stbl.stsd.table[0].user_atoms,
                                  "dac3", dac3_data,
                                  sizeof(dac3_data));
    }
  
  }


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
  AVPacket pkt;
  int got_packet;
  
  if(!codec->initialized)
    {
    codec->avctx->sample_rate = track_map->samplerate;
    codec->avctx->channels = channels;

    codec->avctx->codec_id = codec->encoder->id;
    codec->avctx->codec_type = codec->encoder->type;

    codec->avctx->sample_fmt = codec->encoder->sample_fmts[0];

    
    if(avcodec_open2(codec->avctx, codec->encoder, NULL) != 0)
      {
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "avcodec_open2 failed");
      return 0;
      }
    
    codec->initialized = 1;

    /* One frame is: bitrate * frame_samples / (samplerate * 8) + 1024 */
    codec->chunk_buffer_alloc = ( codec->avctx->frame_size
                                  * sizeof( int16_t )
                                  * codec->avctx->channels);
    codec->chunk_buffer = malloc(codec->chunk_buffer_alloc);
    
    if(trak->strl)
      lqt_set_audio_bitrate(file, track, codec->avctx->bit_rate);
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
    av_init_packet(&pkt);
    pkt.data = codec->chunk_buffer;
    pkt.size = codec->chunk_buffer_alloc;

    codec->f->nb_samples = codec->avctx->frame_size;
    
    avcodec_fill_audio_frame(codec->f, channels, codec->avctx->sample_fmt,
                             (uint8_t*)&codec->sample_buffer[samples_done*channels],
                             codec->avctx->frame_size * channels * 2, 
                             1);

    if(avcodec_encode_audio2(codec->avctx, &pkt,
                             codec->f, &got_packet) < 0)
      return 0;

    if(got_packet && pkt.size)
      frame_bytes = pkt.size;
    else
      frame_bytes = 0;
    
    if(frame_bytes > 0)
      {
      quicktime_write_chunk_header(file, trak);
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
  uint32_t header;
  uint8_t * ptr;

  quicktime_audio_map_t *atrack = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = atrack->codec->priv;

  /* Read header */

  while(1)
    {
    while(codec->lqt_pkt_parse.data_len < 4)
      {
      if(!quicktime_trak_append_packet(file, atrack->track, &codec->lqt_pkt_parse))
        return 0;
      }

    ptr = codec->lqt_pkt_parse.data;
    
    header =
      ptr[3] | (ptr[2] << 8) | (ptr[1] << 16) | (ptr[0] << 24);
    
    if(mpa_header_check(header))
      break;

    lqt_packet_flush(&codec->lqt_pkt_parse, 1);
    }
  
  if(!mpa_decode_header(&h, ptr, NULL))
    {
    lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "Decode header failed");
    return 0;
    }

  /* Initialize compression info */

  if(atrack->ci.id == LQT_COMPRESSION_NONE)
    {
    if(h.layer == 2)
      atrack->ci.id = LQT_COMPRESSION_MP2;
    else if(h.layer == 3)
      atrack->ci.id = LQT_COMPRESSION_MP3;
  
    if(lqt_audio_is_vbr(file, track))
      atrack->ci.bitrate = -1;
    else
      atrack->ci.bitrate = h.bitrate;
    }

  if(!p)
    return 0;
  
  /* Make sure we have enough data */

  while(codec->lqt_pkt_parse.data_len < h.frame_bytes)
    {
    if(!quicktime_trak_append_packet(file, atrack->track, &codec->lqt_pkt_parse))
      break;
    }

  /* Last packet can be shorter if a bit reservoir is used */
  if(h.frame_bytes > codec->lqt_pkt_parse.data_len)
    h.frame_bytes = codec->lqt_pkt_parse.data_len;

  p->duration = h.samples_per_frame;
  p->timestamp = codec->pts;
  codec->pts += p->duration;
  p->flags = LQT_PACKET_KEYFRAME;

  lqt_packet_alloc(p, p->data_len);
  memcpy(p->data, codec->lqt_pkt_parse.data, h.frame_bytes);
  p->data_len = h.frame_bytes;

  lqt_packet_flush(&codec->lqt_pkt_parse, p->data_len);
  return 1;
  }

#if 0
static int writes_compressed_mp2(lqt_file_type_t type,
                                 const lqt_compression_info_t * ci)
  {
  return 1;
  }

static int init_compressed_mp2(quicktime_t * file, int track)
  {
  
  return 0;
  }

static int init_compressed_ac3(quicktime_t * file, int track)
  {
  
  return 0;
  }
#endif

static int write_packet_ac3(quicktime_t * file, lqt_packet_t * p, int track)
  {
  int result;
  quicktime_audio_map_t *atrack = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = atrack->codec->priv;
  
  if(!codec->header_written && (p->data_len >= 8))
    {
    if(file->file_type & (LQT_FILE_QT_OLD | LQT_FILE_QT | LQT_FILE_MP4))
      create_dac3_atom(file, track, p->data);
    else if(file->file_type & (LQT_FILE_AVI | LQT_FILE_AVI_ODML))
      lqt_set_audio_bitrate(file, track, atrack->ci.bitrate);
    codec->header_written = 1;
    }
  
  quicktime_write_chunk_header(file, atrack->track);
  result = !quicktime_write_data(file, p->data, p->data_len);
  
  atrack->track->chunk_samples = p->duration;
  quicktime_write_chunk_footer(file, atrack->track);
  atrack->cur_chunk++;
  if(result)
    return 0;
  else
    return 1;
  
  }

static int read_packet_ac3(quicktime_t * file, lqt_packet_t * p, int track)
  {
  a52_header h;
  uint8_t * ptr;

  quicktime_audio_map_t *atrack = &file->atracks[track];
  quicktime_ffmpeg_audio_codec_t *codec = atrack->codec->priv;

  while(1)
    {
    while(codec->lqt_pkt_parse.data_len < 8)
      {
      if(!quicktime_trak_append_packet(file, atrack->track, &codec->lqt_pkt_parse))
        return 0;
      }

    ptr = codec->lqt_pkt_parse.data;

    /* Check for ac3 header */
    if(a52_header_read(&h, ptr))
      break;
    
    lqt_packet_flush(&codec->lqt_pkt_parse, 1);
    }

  /* Initialize compression info */

  if(atrack->ci.id == LQT_COMPRESSION_NONE)
    {
    atrack->ci.bitrate = h.bitrate;
    atrack->ci.id = LQT_COMPRESSION_AC3;
    }
  
  if(!p)
    return 0;
  
  /* Make sure we have enough data */
  while(codec->lqt_pkt_parse.data_len < h.frame_bytes)
    {
    if(!quicktime_trak_append_packet(file, atrack->track, &codec->lqt_pkt_parse))
      return 0;
    }
  
  lqt_packet_alloc(p, h.frame_bytes);
  memcpy(p->data, ptr, h.frame_bytes);
  
  p->data_len = h.frame_bytes;
  p->duration = A52_FRAME_SAMPLES;
  p->timestamp = codec->pts;
  codec->pts += p->duration;
  p->flags = LQT_PACKET_KEYFRAME;

  lqt_packet_flush(&codec->lqt_pkt_parse, h.frame_bytes);
  
  return 1;
  }

void quicktime_init_audio_codec_ffmpeg(quicktime_codec_t * codec_base,
                                       quicktime_audio_map_t *atrack,
                                       AVCodec *encoder,
                                       AVCodec *decoder)
  {
  quicktime_ffmpeg_audio_codec_t *codec;
  
  //  avcodec_init();
  codec = calloc(1, sizeof(*codec));
  if(!codec)
    return;
  
  codec->encoder = encoder;
  codec->decoder = decoder;
  codec->avctx = avcodec_alloc_context3(NULL);
  codec->f = av_frame_alloc();
  
  codec_base->priv = (void *)codec;

  codec_base->delete_codec = lqt_ffmpeg_delete_audio;
  if(encoder)
    codec_base->encode_audio = lqt_ffmpeg_encode_audio;
  if(decoder)
    {
    codec_base->decode_audio_packet = decode_audio_packet_ffmpeg;
    codec_base->resync = resync_ffmpeg;
    }
  

  codec_base->set_parameter = set_parameter;

  if((decoder->id == AV_CODEC_ID_MP3) || (decoder->id == AV_CODEC_ID_MP2))
    codec_base->read_packet = read_packet_mpa;
  else if(decoder->id == AV_CODEC_ID_AC3)
    {
    codec_base->write_packet = write_packet_ac3;
    codec_base->read_packet = read_packet_ac3;
    }
  if(!atrack)
    return;
  }


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

/* AC3 */

#define PTR_2_32BE(p) \
((*(p) << 24) | \
(*(p+1) << 16) | \
(*(p+2) << 8) | \
*(p+3))

static inline int get_bits(uint32_t * bits, int num)
  {
  int ret;
  
  ret = (*bits) >> (32 - num);
  (*bits) <<= num;
  
  return ret;
  }


/* Tables from ffmpeg (ac3tab.c) */
static const uint16_t ac3_frame_size_tab[38][3] =
  {
    { 64,   69,   96   },
    { 64,   70,   96   },
    { 80,   87,   120  },
    { 80,   88,   120  },
    { 96,   104,  144  },
    { 96,   105,  144  },
    { 112,  121,  168  },
    { 112,  122,  168  },
    { 128,  139,  192  },
    { 128,  140,  192  },
    { 160,  174,  240  },
    { 160,  175,  240  },
    { 192,  208,  288  },
    { 192,  209,  288  },
    { 224,  243,  336  },
    { 224,  244,  336  },
    { 256,  278,  384  },
    { 256,  279,  384  },
    { 320,  348,  480  },
    { 320,  349,  480  },
    { 384,  417,  576  },
    { 384,  418,  576  },
    { 448,  487,  672  },
    { 448,  488,  672  },
    { 512,  557,  768  },
    { 512,  558,  768  },
    { 640,  696,  960  },
    { 640,  697,  960  },
    { 768,  835,  1152 },
    { 768,  836,  1152 },
    { 896,  975,  1344 },
    { 896,  976,  1344 },
    { 1024, 1114, 1536 },
    { 1024, 1115, 1536 },
    { 1152, 1253, 1728 },
    { 1152, 1254, 1728 },
    { 1280, 1393, 1920 },
    { 1280, 1394, 1920 },
  };

const uint16_t ac3_bitrate_tab[19] =
  {
    32, 40, 48, 56, 64, 80, 96, 112, 128,
    160, 192, 224, 256, 320, 384, 448, 512, 576, 640
  };

static int a52_header_read(a52_header * ret, uint8_t * buf)
  {
  int shift;
  uint32_t bits;
  memset(ret, 0, sizeof(*ret));

  /* Check syncword */
  if((buf[0] != 0x0b) || (buf[1] != 0x77))
    return 0;

  /* Skip syncword & crc */
  buf += 4;

  bits = PTR_2_32BE(buf);
  
  ret->fscod      = get_bits(&bits, 2);
  ret->frmsizecod = get_bits(&bits, 6);

  if(ret->frmsizecod > 37)
    return 0;
  
  ret->bsid       = get_bits(&bits, 5);

  if(ret->bsid >= 12)
    return 0;
  
  ret->bsmod      = get_bits(&bits, 3);
  ret->acmod      = get_bits(&bits, 3);

  if((ret->acmod & 0x01) && (ret->acmod != 0x01))
    ret->cmixlev = get_bits(&bits, 2);
  if(ret->acmod & 0x04)
    ret->surmixlev = get_bits(&bits, 2);
  if(ret->acmod == 0x02)
    ret->dsurmod = get_bits(&bits, 2);
  ret->lfeon = get_bits(&bits, 1);

  /* Secondary variables */

  shift = ret->bsid - 8;
  if(shift < 0)
    shift = 0;

  ret->bitrate = (ac3_bitrate_tab[ret->frmsizecod>>1] * 1000) >> shift;
  ret->frame_bytes = ac3_frame_size_tab[ret->frmsizecod][ret->fscod] * 2;
  return 1;
  }
