/* 
 * audio.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
 * Based entirely upon qtpng.c from libquicktime 
 * (http://libquicktime.sf.net).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA    
 */

#include <string.h>
#include "config.h"

#include "funcprotos.h"
#include <quicktime/quicktime.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

#include "funcprotos.h"
#include <quicktime/colormodels.h>

#include "ffmpeg.h"



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

static int header_equal(const mpeg_header * h1, const mpeg_header * h2)
  {
  return ((h1->layer == h2->layer) && (h1->version == h2->version) &&
          (h1->samplerate == h2->samplerate));
  }

static int decode_header(mpeg_header * h, uint8_t * ptr, const mpeg_header * ref)
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

  /* Check against reference header */

  if(ref && !header_equal(ref, h))
    return 0;
  return 1;
  }

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

  mpeg_header mph;
  int have_mpeg_header;

  uint8_t * extradata;
    
  } quicktime_ffmpeg_audio_codec_t;


static int lqt_ffmpeg_delete_audio(quicktime_audio_map_t *vtrack)
  {
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  
  if(codec->avctx)
    {
    if(codec->initialized)
      avcodec_close(codec->avctx);
    else
      free(codec->avctx);
    }
  if(codec->sample_buffer) free(codec->sample_buffer);
  if(codec->chunk_buffer)  free(codec->chunk_buffer);
  if(codec->extradata) free(codec->extradata); 
  free(codec);
  return 0;
  }

static int set_parameter(quicktime_t *file, 
                  int track, 
                  char *key, 
                  void *value)
  {
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;
  lqt_ffmpeg_set_parameter(codec->avctx, key, value);
  return 0;
  }


/* Decode VBR chunk into the sample buffer */

static int decode_chunk_vbr(quicktime_t * file, int track)
  {
  int chunk_packets, i, num_samples, bytes_decoded;
  int packet_size, packet_samples;
  int frame_bytes;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

  chunk_packets = lqt_audio_num_vbr_packets(file, track, track_map->current_chunk, &num_samples);

  if(!chunk_packets)
    return 0;
  
  if(codec->sample_buffer_alloc < codec->sample_buffer_end - codec->sample_buffer_start + num_samples)
    {
    
    codec->sample_buffer_alloc = codec->sample_buffer_end - codec->sample_buffer_start + num_samples;
    //    fprintf(stderr, "codec->sample_buffer_alloc: %d\n", codec->sample_buffer_alloc);
    codec->sample_buffer = realloc(codec->sample_buffer, 2 * codec->sample_buffer_alloc *
                                   track_map->channels);
    }

  for(i = 0; i < chunk_packets; i++)
    {
    packet_size = lqt_audio_read_vbr_packet(file, track, track_map->current_chunk, i,
                                            &(codec->chunk_buffer), &(codec->chunk_buffer_alloc),
                                            &packet_samples);
    if(!packet_size)
      return 0;
    
    frame_bytes =
      avcodec_decode_audio(codec->avctx,
                           &(codec->sample_buffer[track_map->channels *
                                                  (codec->sample_buffer_end - codec->sample_buffer_start)]),
                           &bytes_decoded,
                           codec->chunk_buffer,
                           packet_size + FF_INPUT_BUFFER_PADDING_SIZE);
    if(frame_bytes < 0)
      {
      fprintf(stderr, "avcodec_decode_audio error\n");
      break;
      }
#if 0
    fprintf(stderr, "decode_chunk_vbr: Samples decoded: %d, Bytes used: %d\n",
            bytes_decoded / (track_map->channels * 2),
            frame_bytes);
#endif
    codec->sample_buffer_end += (bytes_decoded / (track_map->channels * 2));
    }
  track_map->current_chunk++;
  return num_samples;
  }


/* Decode the current chunk into the sample buffer */

static int decode_chunk(quicktime_t * file, int track)
  {
  mpeg_header mph;
    
  int frame_bytes;
  int num_samples;
  int samples_decoded = 0;
  int bytes_decoded;
  int bytes_used, bytes_skipped;
  int64_t chunk_size;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

  /* Read chunk */
  
  chunk_size = lqt_append_audio_chunk(file,
                                      track, track_map->current_chunk,
                                      &(codec->chunk_buffer),
                                      &(codec->chunk_buffer_alloc),
                                      codec->bytes_in_chunk_buffer);

  //  fprintf(stderr, "Got chunk:\n");
  //  lqt_hexdump(codec->chunk_buffer, 16, 16);
  
  if(!chunk_size)
    {
    //    fprintf(stderr, "audio_ffmpeg: EOF 1 (%d bytes left)\n", codec->bytes_in_chunk_buffer);
    /* If the codec is mp3, make sure to decode the very last frame */

    if((codec->avctx->codec_id == CODEC_ID_MP3) &&
       (codec->bytes_in_chunk_buffer >= 4))
      {
      if(!decode_header(&mph, codec->chunk_buffer, (const mpeg_header*)0))
        {
        fprintf(stderr, "Decode header failed\n");
        return 0;
        }
      if(mph.frame_bytes <= codec->bytes_in_chunk_buffer)
        {
        fprintf(stderr, "Huh, frame not decoded?\n");
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
    //  fprintf(stderr, "decode chunk, bytes from last chunk: %d\n", codec->bytes_in_chunk_buffer);
    num_samples = quicktime_chunk_samples(track_map->track, track_map->current_chunk);
    track_map->current_chunk++;
    codec->bytes_in_chunk_buffer += chunk_size;
    }
  
  //  fprintf(stderr, "chunk samples: %d\n", num_samples);

  if(!num_samples)
    {
    //    fprintf(stderr, "audio_ffmpeg: EOF\n");
    return 0;
    }
  /*
   *  For AVIs, chunk samples are not always 100% correct.
   *  Furthermore, there can be a complete mp3 frame from the last chunk!
   */

  num_samples += 8192;

  /* Reallocate sample buffer */
  
  if(codec->sample_buffer_alloc < codec->sample_buffer_end - codec->sample_buffer_start + num_samples)
    {
    
    codec->sample_buffer_alloc = codec->sample_buffer_end - codec->sample_buffer_start + num_samples;
    //    fprintf(stderr, "codec->sample_buffer_alloc: %d\n", codec->sample_buffer_alloc);
    codec->sample_buffer = realloc(codec->sample_buffer, 2 * codec->sample_buffer_alloc *
                                   track_map->channels);
    }
  
  
  /* Decode this */

  bytes_used = 0;
  while(1)
    {
#if 0
    fprintf(stderr, "Avcodec decode audio %d, header: ",
            codec->bytes_in_chunk_buffer);
    fprintf(stderr, "%02x %02x %02x %02x\n",
            codec->chunk_buffer[bytes_used],
            codec->chunk_buffer[bytes_used+1],
            codec->chunk_buffer[bytes_used+2],
            codec->chunk_buffer[bytes_used+3]);
#endif

        
    /* BIG NOTE: We pass extra FF_INPUT_BUFFER_PADDING_SIZE for the buffer size
       because we know, that lqt_read_audio_chunk allocates 16 extra bytes for us */
#if 0
    fprintf(stderr, "decode_chunk: Sample buffer: %d, chunk_buffer: %d\n",
            (int)(codec->sample_buffer_end - codec->sample_buffer_start),
            codec->bytes_in_chunk_buffer);
#endif
    
    /* Some really broken mp3 files have the header bytes split across 2 chunks */

    if(codec->avctx->codec_id == CODEC_ID_MP3)
      {
      if(codec->bytes_in_chunk_buffer < 4)
        {
        //        fprintf(stderr, "decode_chunk: Incomplete frame %d\n", codec->bytes_in_chunk_buffer);
        
        if(codec->bytes_in_chunk_buffer > 0)
          memmove(codec->chunk_buffer,
                  codec->chunk_buffer + bytes_used, codec->bytes_in_chunk_buffer);
        return 1;
        }

      bytes_skipped = 0;
      while(1)
        {
        if(!codec->have_mpeg_header)
          {
          if(decode_header(&mph, &(codec->chunk_buffer[bytes_used]), NULL))
            {
            memcpy(&(codec->mph), &mph, sizeof(mph));
            codec->have_mpeg_header = 1;
            break;
            }
          }
        else if(decode_header(&mph, &(codec->chunk_buffer[bytes_used]), &(codec->mph)))
          break;
        
        bytes_used++;
        codec->bytes_in_chunk_buffer--;
        bytes_skipped++;
        if(codec->bytes_in_chunk_buffer <= 4)
          {
          //          fprintf(stderr, "decode_chunk: Incomplete frame %d\n", codec->bytes_in_chunk_buffer);

          if(codec->bytes_in_chunk_buffer > 0)
            memmove(codec->chunk_buffer,
                    codec->chunk_buffer + bytes_used, codec->bytes_in_chunk_buffer);
          return 1;
          }
        }
#if 0
      if(bytes_skipped)
        fprintf(stderr, "Skipped %d bytes, bytes_in_chunk_buffer: %d\n",
                bytes_skipped, codec->bytes_in_chunk_buffer);
#endif 
      if(codec->bytes_in_chunk_buffer < mph.frame_bytes)
        {
        //        fprintf(stderr, "decode_chunk: Incomplete frame %d\n", codec->bytes_in_chunk_buffer);
        
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
    
    frame_bytes =
      avcodec_decode_audio(codec->avctx,
                           &(codec->sample_buffer[track_map->channels *
                                                  (codec->sample_buffer_end - codec->sample_buffer_start)]),
                           &bytes_decoded,
                           &(codec->chunk_buffer[bytes_used]),
                           codec->bytes_in_chunk_buffer + FF_INPUT_BUFFER_PADDING_SIZE);
    if(frame_bytes < 0)
      {
      fprintf(stderr, "avcodec_decode_audio error\n");
      break;
      }
#if 0
    fprintf(stderr, "decode_chunk: Samples decoded: %d, Bytes used: %d, chunk_size: %lld, chunk buffer: %d\n",
            bytes_decoded / (track_map->channels * 2),
            frame_bytes, chunk_size, codec->bytes_in_chunk_buffer);
#endif

    bytes_used                   += frame_bytes;
    codec->bytes_in_chunk_buffer -= frame_bytes;
    
    if(bytes_decoded < 0)
      {
      if(codec->avctx->codec_id == CODEC_ID_MP3)
        {
        /* For mp3, bytes_decoded < 0 means, that the frame should be muted */
        memset(&(codec->sample_buffer[track_map->channels * (codec->sample_buffer_end - codec->sample_buffer_start)]),
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
#if 0
        fprintf(stderr, "decode_chunk: Incomplete frame %d %d\n", bytes_decoded,
                codec->bytes_in_chunk_buffer);
#endif   
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
      fprintf(stderr, "BUUUUG, buffer overflow, %d %d\n",
              (int)(codec->sample_buffer_end - codec->sample_buffer_start),
              codec->sample_buffer_alloc);
    
    if(!codec->bytes_in_chunk_buffer)
      break;

    }
#if 0
  fprintf(stderr, "Last samples decoded: %d, Bytes used: %d, samples decoded %d, chunk_size: %lld\n",
          bytes_decoded / (track_map->channels * 2),
          bytes_used, samples_decoded, chunk_size);
#endif
  //  track_map->current_chunk++;
  return samples_decoded;
  }

static int lqt_ffmpeg_decode_audio(quicktime_t *file, void * output, long samples, int track)
  {
  uint8_t * header;
  uint32_t header_len;
  
  int samples_decoded;
  //  int result = 0;
  int64_t chunk_sample; /* For seeking only */
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  int channels = file->atracks[track].channels;
  //  int64_t total_samples;

  int samples_to_skip;
  int samples_to_move;

  //  fprintf(stderr, "ffmpeg decode audio %lld %d\n", track_map->current_position, samples);

  if(!output) /* Global initialization */
    {
    return 0;
    }
  
  /* Initialize codec */
  if(!codec->initialized)
    {

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

    codec->avctx->bits_per_sample = quicktime_audio_bits(file, track);

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

    //    memcpy(&(codec->com.ffcodec_enc), &(codec->com.params), sizeof(AVCodecContext));
    
    if(avcodec_open(codec->avctx, codec->decoder) != 0)
      {
      fprintf(stderr, "Avcodec open failed\n");
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
                                &(track_map->current_chunk),
                                track_map->track,
                                track_map->current_position);
      else
        quicktime_chunk_of_sample(&chunk_sample,
                                  &(track_map->current_chunk),
                                  track_map->track,
                                  track_map->current_position);
#if 0
      fprintf(stderr, "Seek: last_pos: %ld, pos: %lld, chunk: %lld, chunk_sample: %lld\n",
              track_map->last_position,
              track_map->current_position,
              track_map->current_chunk, chunk_sample);
#endif       
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
#if 0
    fprintf(stderr, "** Flush samples_to_skip: %d, %d\n", samples_to_skip,
            (int)(codec->sample_buffer_end - codec->sample_buffer_start));
#endif
    if(samples_to_skip > (int)(codec->sample_buffer_end - codec->sample_buffer_start))
      samples_to_skip = (int)(codec->sample_buffer_end - codec->sample_buffer_start);
    
    if(codec->sample_buffer_end > track_map->current_position)
      {
      samples_to_move = codec->sample_buffer_end - track_map->current_position;

      //      fprintf(stderr, "Memmove...");
      memmove(codec->sample_buffer,
              &(codec->sample_buffer[samples_to_skip * channels]),
              samples_to_move * channels * sizeof(int16_t));
      //      fprintf(stderr, "done\n");
      }
    codec->sample_buffer_start += samples_to_skip;
    
    //    fprintf(stderr, " %d\n", (int)(codec->sample_buffer_end - codec->sample_buffer_start));

    }
  samples_to_skip = track_map->current_position - codec->sample_buffer_start;
  
  /* Read new chunks until we have enough samples */
  while(codec->sample_buffer_end - codec->sample_buffer_start < samples + samples_to_skip)
    {
#if 0
    fprintf(stderr, "Samples: %lld -> %lld %lld, %d\n",
            codec->sample_buffer_start, codec->sample_buffer_end,
            codec->sample_buffer_end - codec->sample_buffer_start, samples);
#endif
    
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

  if(samples_decoded > samples)
    samples_decoded = samples;
  
  //  fprintf(stderr, "Samples_decoded: %d\n", samples_decoded);
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
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_ffmpeg_audio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  quicktime_trak_t *trak = track_map->track;
  int channels = file->atracks[track].channels;
  quicktime_atom_t chunk_atom;
  int frame_bytes;
  int samples_done = 0;
  int samples_encoded;
  /* Initialize encoder */
    
  if(!codec->initialized)
    {
    codec->avctx->sample_rate = track_map->samplerate;
    codec->avctx->channels = channels;

    if(avcodec_open(codec->avctx, codec->encoder) != 0)
      {
      fprintf(stderr, "Avcodec open failed\n");
      return -1;
      }
    
    codec->initialized = 1;

    /* One frame is: bitrate * frame_samples / (samplerate * 8) + 1024 */
#if 0		// PATCH 1
    codec->chunk_buffer_alloc = (codec->avctx->bit_rate * codec->avctx->frame_size /
                                (codec->avctx->sample_rate * 8)) + 1024;
#else
    codec->chunk_buffer_alloc = ( codec->avctx->frame_size
									* sizeof( int16_t )
									* codec->avctx->channels
								);
#endif
    codec->chunk_buffer = malloc(codec->chunk_buffer_alloc);
    }

  /* Allocate sample buffer if necessary */

//         PATCH 2 
  if(codec->sample_buffer_alloc < (codec->samples_in_buffer + samples))
    {
    codec->sample_buffer_alloc = codec->samples_in_buffer + samples + 16;
    codec->sample_buffer = realloc(codec->sample_buffer,
                                   codec->sample_buffer_alloc * channels * sizeof(int16_t));
    }

  /* Interleave */

  //  interleave(&(codec->sample_buffer[codec->samples_in_buffer * channels]),
  //             input_i, input_f, samples, channels);

  memcpy(codec->sample_buffer + codec->samples_in_buffer * channels,
         input, samples * channels * 2);
  
  codec->samples_in_buffer += samples;
  
  /* Encode */
  
  //  fprintf(stderr, "codec->samples_in_buffer: %d, codec->avctx->frame_size %d\n",
  //          codec->samples_in_buffer, codec->avctx->frame_size);
  while(codec->samples_in_buffer >= codec->avctx->frame_size)
    {
    //    fprintf(stderr, "avcodec_encode_audio %d...", samples_done);
    
    frame_bytes = avcodec_encode_audio(codec->avctx, codec->chunk_buffer,
                                       codec->chunk_buffer_alloc,
                                       &(codec->sample_buffer[samples_done*channels]));
    if(frame_bytes > 0)
      {
      quicktime_write_chunk_header(file, trak, &chunk_atom);
#if 0    // PATCH 3  
      if(codec->avctx->frame_size == 1)
        samples_encoded = codec->samples_in_buffer;
      else
#endif
        samples_encoded = codec->avctx->frame_size;

      //      fprintf(stderr, "Done %d->%d\n", samples_encoded, frame_bytes);

      samples_done              += samples_encoded;
      codec->samples_in_buffer  -= samples_encoded;
      
      result = !quicktime_write_data(file, codec->chunk_buffer, frame_bytes);
      quicktime_write_chunk_footer(file,
                                   trak, 
                                   file->atracks[track].current_chunk,
                                   &chunk_atom, 
                                   samples_encoded);
      file->atracks[track].current_chunk++;
      }
    }
  if(codec->samples_in_buffer && samples_done)
    memmove(codec->sample_buffer, &(codec->sample_buffer[samples_done*channels]),
            codec->samples_in_buffer * sizeof(int16_t) * channels);
  return result;
  }

void quicktime_init_audio_codec_ffmpeg(quicktime_audio_map_t *atrack, AVCodec *encoder,
                                       AVCodec *decoder)
{
	quicktime_ffmpeg_audio_codec_t *codec;

	avcodec_init();
        fprintf(stderr, "quicktime_init_audio_codec_ffmpeg 1\n");
	codec = calloc(1, sizeof(quicktime_ffmpeg_audio_codec_t));
	if(!codec)
          return;

	codec->encoder = encoder;
	codec->decoder = decoder;
	codec-> avctx = avcodec_alloc_context();
	((quicktime_codec_t*)atrack->codec)->priv = (void *)codec;
	((quicktime_codec_t*)atrack->codec)->delete_acodec = lqt_ffmpeg_delete_audio;
	if(encoder)
          ((quicktime_codec_t*)atrack->codec)->encode_audio = lqt_ffmpeg_encode_audio;
	if(decoder)
          ((quicktime_codec_t*)atrack->codec)->decode_audio = lqt_ffmpeg_decode_audio;
	((quicktime_codec_t*)atrack->codec)->set_parameter = set_parameter;
        atrack->sample_format = LQT_SAMPLE_INT16;
}
