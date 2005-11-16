/*****************************************************************

  faad2.c

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

#include "funcprotos.h"
#include <quicktime/quicktime.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

#include <string.h>
#include <faad.h>


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

static int delete_codec(quicktime_audio_map_t *atrack)
  {
  quicktime_faad2_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

  if(codec->dec)
    faacDecClose(codec->dec);

  if(codec->sample_buffer)
    free(codec->sample_buffer);

  if(codec->data)
    free(codec->data);
      
  
  free(codec);
  return 0;
  }

static int decode_chunk(quicktime_t *file, int track)
  {
  int i, num_packets, num_samples, packet_size;
  float * samples;
  faacDecFrameInfo frame_info;

  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  
  quicktime_faad2_codec_t *codec =
    ((quicktime_codec_t*)file->atracks[track].codec)->priv;
  
  num_packets = lqt_audio_num_vbr_packets(file, track, track_map->current_chunk, &num_samples);
  if(!num_packets)
    return 0;

  if(codec->upsample)
    num_samples *= 2;
  
  //  fprintf(stderr, "VBR Chunk: packets: %d, samples: %d\n", num_packets, num_samples);
  
  if(codec->sample_buffer_alloc < codec->sample_buffer_end - codec->sample_buffer_start + num_samples)
    {
    codec->sample_buffer_alloc = codec->sample_buffer_end - codec->sample_buffer_start + num_samples + 1024;
    codec->sample_buffer = realloc(codec->sample_buffer,
                                   codec->sample_buffer_alloc * track_map->channels * sizeof(float));
    }
  
  for(i = 0; i < num_packets; i++)
    {
    packet_size = lqt_audio_read_vbr_packet(file, track, track_map->current_chunk, i,
                                            &(codec->data),
                                            &(codec->data_alloc), &num_samples);

    if(codec->upsample)
      num_samples *= 2;
    
    //    fprintf(stderr, "Read VBR packet, chunk: %lld, packet: %d, bytes: %d, samples: %d\n",
    //            track_map->current_chunk, i, packet_size, num_samples);


    samples = faacDecDecode(codec->dec, &frame_info,
                            codec->data, packet_size);
#if 0
    fprintf(stderr, "Decoded: samples: %p, bytes_used: %d/%ld, samples: %ld/%d\n",
            samples, packet_size, frame_info.bytesconsumed, frame_info.samples / track_map->channels,
            num_samples);
#endif
    memcpy(&(codec->sample_buffer[(codec->sample_buffer_end -
                                   codec->sample_buffer_start)*track_map->channels]),
           samples, frame_info.samples * sizeof(float));
    codec->sample_buffer_end += frame_info.samples / track_map->channels;
    }
  
  track_map->current_chunk++;
  return 1;
  }

static int decode(quicktime_t *file, 
                  void * output,
                  long samples, 
                  int track) 
  {
  int samples_copied = 0;
  int samples_to_skip;
  int samples_to_move;
  int samples_decoded;
  
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_faad2_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

  /* TODO Check whether seeking happened */
    
  /* Flush unneeded samples */
  
  if(track_map->current_position > codec->sample_buffer_start)
    {
    samples_to_skip = track_map->current_position - codec->sample_buffer_start;
#if 0
    fprintf(stderr, "Flush start: %lld end: %lld current: %lld\n",
            codec->sample_buffer_start, codec->sample_buffer_end, track_map->current_position);
#endif

    samples_to_move = codec->sample_buffer_end - track_map->current_position;

    if(samples_to_move > 0)
      {
      //      fprintf(stderr, "Memmove %d %d...", samples_to_skip, samples_to_move);
      memmove(codec->sample_buffer, codec->sample_buffer + samples_to_skip * track_map->channels,
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
    //    fprintf(stderr, "Decoded frame\n");
    }

  samples_decoded = codec->sample_buffer_end - codec->sample_buffer_start;

  samples_copied = (samples_decoded > samples) ? samples : samples_decoded;
  
  memcpy(output, codec->sample_buffer, samples_copied * track_map->channels * sizeof(float));

  file->atracks[track].last_position = file->atracks[track].current_position + samples_copied;
  
  return samples_copied;
  }

static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
  {
  return 0;
  }

void quicktime_init_codec_faad2(quicktime_audio_map_t *atrack)
  {
  uint8_t * extradata;
  int extradata_size;
  quicktime_stsd_t * stsd;
  unsigned long samplerate;
  unsigned char channels;

  faacDecConfigurationPtr cfg;
  
  quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
  quicktime_faad2_codec_t *codec;

  /* Init public items */
  codec_base->priv = calloc(1, sizeof(quicktime_faad2_codec_t));
  codec_base->delete_acodec = delete_codec;
  codec_base->decode_audio = decode;
  codec_base->set_parameter = set_parameter;
  codec_base->fourcc = "mp4a";
  codec_base->title = "AAC decoder";
  codec_base->desc = "MPEG-2/4 AAC decoder (faad2 based)";
  
  codec = codec_base->priv;

  atrack->sample_format = LQT_SAMPLE_FLOAT;

  /* Ok, usually, we initialize decoders during the first
     decode() call, but in this case, we might need to
     set the correct samplerate, which should be known before */

  codec->dec = faacDecOpen();

  stsd = &(atrack->track->mdia.minf.stbl.stsd);
  
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
    fprintf(stderr, "No extradata found, decoding is likely to fail\n");
    }

  faacDecInit2(codec->dec, extradata, extradata_size,
               &samplerate, &channels);

  if((int)stsd->table[0].sample_rate != (int)samplerate)
    {
    fprintf(stderr, "faad2: Changing samplerate: %d -> %d\n", (int)stsd->table[0].sample_rate, (int)samplerate);
    stsd->table[0].sample_rate = samplerate;
    codec->upsample = 1;
    }
  
  stsd->table[0].channels = channels;
  
  cfg = faacDecGetCurrentConfiguration(codec->dec);
  cfg->outputFormat = FAAD_FMT_FLOAT;
  faacDecSetConfiguration(codec->dec, cfg);

  }
