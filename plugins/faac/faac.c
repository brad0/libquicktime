/*****************************************************************

  faac.c

  Copyright (c) 2005 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de

  http://libquicktime.sourceforge.net

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.

*****************************************************************/

#include <quicktime/lqt.h>
#include <lqt_funcprotos.h>

#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

#include <faac.h>
#include <string.h>

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
  
  /* Configuration stuff */
  int bitrate;
  int quality;
  
  } quicktime_faac_codec_t;


static int delete_codec(quicktime_audio_map_t *track_map)
  {
  
  return 0;
  }

static int encode_frame(quicktime_t *file, 
                        int track)
  {
  int imax, i;
  int bytes_encoded;
  int result;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_faac_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  quicktime_trak_t * trak = track_map->track;
  
  /* Normalize input to 16 bit int */

  imax = codec->sample_buffer_size * track_map->channels;

  for(i = 0; i < imax; i++)
    codec->sample_buffer[i] *= 32767.0;
  
  /* Encode this frame */
  bytes_encoded = faacEncEncode(codec->enc,
                                (int32_t*)codec->sample_buffer,
                                codec->sample_buffer_size * track_map->channels,
                                codec->chunk_buffer,
                                codec->chunk_buffer_size);
  
  if(bytes_encoded <= 0)
    {
    fprintf(stderr, "Warning: Encoded %d bytes\n", bytes_encoded);
    return 1;
    }
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
  
  lqt_finish_audio_vbr_frame(file, track, codec->sample_buffer_size);
  codec->sample_buffer_size = 0;
  
  return result;
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
  quicktime_faac_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  quicktime_trak_t * trak = track_map->track;
    
  if(!codec->initialized)
    {
    lqt_init_vbr_audio(file, track);

    /* Create encoder */
    codec->enc = faacEncOpen(trak->mdia.minf.stbl.stsd.table[0].sample_rate,
                             track_map->channels,
                             &input_samples,
                             &output_bytes);

    /* Set things up */
    enc_config = faacEncGetCurrentConfiguration(codec->enc);
    enc_config->inputFormat = FAAC_INPUT_FLOAT;
    enc_config->bitRate = (codec->bitrate * 1000) / track_map->channels;
    enc_config->quantqual = codec->quality;
    enc_config->outputFormat = 0; /* Raw */
    
    
    if(!faacEncSetConfiguration(codec->enc, enc_config))
      {
      fprintf(stderr, "Setting encode parameters failed, check settings\n");
      }

    /* Allocate buffers */

    codec->samples_per_frame = input_samples / track_map->channels;
    codec->sample_buffer =
      malloc(codec->samples_per_frame * track_map->channels * sizeof(float));

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

    esds->version         = 0;
    esds->flags           = 0;
    
    esds->esid            = 0;
    esds->stream_priority = 0;
    
    esds->objectTypeId    = 64; /* MPEG-4 audio */
    esds->streamType      = 0x15; /* from qt4l and Autumns Child.m4a */
    esds->bufferSizeDB    = 64000; /* Hopefully not important :) */

    /* Maybe correct these later? */
    esds->maxBitrate      = 128000;
    esds->avgBitrate      = 128000;
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
      encode_frame(file, track);
    
    }

  /* Finalize audio chunk */
  if(codec->chunk_started)
    {
    quicktime_write_chunk_footer(file,
                                 trak,
                                 track_map->current_chunk,
                                 &codec->chunk_atom,
                                 track_map->vbr_num_frames);
    track_map->current_chunk++;
    codec->chunk_started = 0;
    }
  return 0;
  }

static int set_parameter(quicktime_t *file, 
                         int track, 
                         char *key, 
                         void *value)
  {
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_faac_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  
  if(!strcasecmp(key, "faac_bitrate"))
    codec->bitrate = *(int*)value;
  else if(!strcasecmp(key, "faac_quality"))
    codec->quality = *(int*)value;
  return 0;
  }

static void flush(quicktime_t *file, int track)
  {

  }

void quicktime_init_codec_faac(quicktime_audio_map_t *track_map)
  {
  quicktime_codec_t *codec_base = (quicktime_codec_t*)track_map->codec;
  quicktime_faac_codec_t *codec;
  
  /* Init public items */
  codec_base->priv = calloc(1, sizeof(quicktime_faac_codec_t));
  codec_base->delete_acodec = delete_codec;
  codec_base->encode_audio = encode;
  codec_base->set_parameter = set_parameter;
  codec_base->flush = flush;
  codec_base->fourcc = "mp4a";
  codec_base->title = "AAC encoder";
  codec_base->desc = "AAC encoder (based on faac)";
  
  codec = codec_base->priv;

  codec->bitrate = 0;
  codec->quality = 100;
  
  track_map->sample_format = LQT_SAMPLE_FLOAT;
  }
