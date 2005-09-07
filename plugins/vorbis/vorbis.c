#include "funcprotos.h"
#include <quicktime/quicktime.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

#include "qtvorbis.h"
#include <vorbis/vorbisenc.h>

#include <string.h>
#include <time.h>

typedef struct
{

/* Common stuff */
        float ** sample_buffer;
        int sample_buffer_alloc; 
/* Encoder stuff */
        int max_bitrate;
	int nominal_bitrate;
	int min_bitrate;
	int use_vbr;
	int encode_initialized;
	ogg_stream_state enc_os;
	ogg_page enc_og;
        uint8_t * enc_header;
        int enc_header_len;
        int header_written;

ogg_packet enc_op;
	vorbis_info enc_vi;
	vorbis_comment enc_vc;
	vorbis_dsp_state enc_vd;
	vorbis_block enc_vb;
//        int64_t last_granulepos;

// Number of samples written to disk
	int encoded_samples;

        int enc_samples_in_buffer;

        int chunk_started;
        quicktime_atom_t chunk_atom;

/* Decoder stuff */

	ogg_sync_state   dec_oy; /* sync and verify incoming physical bitstream */
	ogg_stream_state dec_os; /* take physical pages, weld into a logical
				stream of packets */
	ogg_page         dec_og; /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet       dec_op; /* one raw packet of data for decode */

	vorbis_info      dec_vi; /* struct that stores all the static vorbis bitstream
				settings */
	vorbis_comment   dec_vc; /* struct that stores all the bitstream user comments */
	vorbis_dsp_state dec_vd; /* central working state for the packet->PCM decoder */
	vorbis_block     dec_vb; /* local working space for packet->PCM decode */

	int decode_initialized;
        int stream_initialized;

  
  /* Buffer for the entire chunk */

        uint8_t * chunk_buffer;
        int chunk_buffer_alloc;
        int bytes_in_chunk_buffer;

  /* Start and end positions of the sample buffer */
    
        int64_t sample_buffer_start;
        int64_t sample_buffer_end;


// Number of last sample relative to file
	int64_t output_position;
// Number of last sample relative to output buffer
	long output_end;
// Number of samples in output buffer
	long output_size;
// Number of samples allocated in output buffer
	long output_allocated;
// Current reading position in file
	int64_t chunk;
// Number of samples decoded in the current chunk
	int chunk_samples;

        int header_read;

  } quicktime_vorbis_codec_t;


/* =================================== public for vorbis */

static int delete_codec(quicktime_audio_map_t *atrack)
  {
  quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;
  int i;

  if(codec->encode_initialized)
    {
    ogg_stream_clear(&codec->enc_os);
    vorbis_block_clear(&codec->enc_vb);
    vorbis_dsp_clear(&codec->enc_vd);
    vorbis_comment_clear(&codec->enc_vc);
    vorbis_info_clear(&codec->enc_vi);
    }

  if(codec->decode_initialized)
    {
    ogg_stream_clear(&codec->dec_os);
    vorbis_block_clear(&codec->dec_vb);
    vorbis_dsp_clear(&codec->dec_vd);
    vorbis_comment_clear(&codec->dec_vc);
    vorbis_info_clear(&codec->dec_vi);
    }

  if(codec->sample_buffer) 
    {
    for(i = 0; i < atrack->channels; i++)
      free(codec->sample_buffer[i]);
    free(codec->sample_buffer);
    }
  if(codec->chunk_buffer)
    free(codec->chunk_buffer);
  if(codec->enc_header)
    free(codec->enc_header);
        
  free(codec);
  return 0;
  }

static int next_chunk(quicktime_t * file, int track)
  {
  int chunk_size;
  char * buffer;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

  //  fprintf(stderr, "Next chunk %lld\n", track_map->current_chunk);

  chunk_size = lqt_read_audio_chunk(file,
                                    track, track_map->current_chunk,
                                    &(codec->chunk_buffer),
                                    &(codec->chunk_buffer_alloc), (int*)0);
  if(chunk_size <= 0)
    {
    //    fprintf(stderr, "Shit\n");
    return 0;
    }
  
  track_map->current_chunk++;
  
  buffer = ogg_sync_buffer(&codec->dec_oy, chunk_size);
  memcpy(buffer, codec->chunk_buffer, chunk_size);
  ogg_sync_wrote(&codec->dec_oy, chunk_size);
  
  return 1;
  }

static int next_page(quicktime_t * file, int track)
  {
  char * buffer;
  uint8_t * header;
  uint32_t header_len;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  int result = 0;

  
  if(!codec->header_read) /* Try OVHS atom */
    {
    header = quicktime_wave_get_user_atom(track_map->track, "OVHS", &header_len);
    if(header)
      {
      fprintf(stderr, "Using OVHS Atom, %d bytes\n", header_len-8);
      buffer = ogg_sync_buffer(&codec->dec_oy, header_len-8);
      memcpy(buffer, header + 8, header_len-8);
      ogg_sync_wrote(&codec->dec_oy, header_len-8);
      }
    }

  while(result < 1)
    {
    result = ogg_sync_pageout(&codec->dec_oy, &codec->dec_og);

    if(result == 0)
      {
      if(!next_chunk(file, track))
        {
        return 0;
        }
      }
    else
      {
      if(!codec->stream_initialized)
        {
        ogg_stream_init(&codec->dec_os, ogg_page_serialno(&codec->dec_og));
        codec->stream_initialized = 1;
        }
      ogg_stream_pagein(&codec->dec_os, &codec->dec_og);
      }
    }
  return 1;
  }


static int next_packet(quicktime_t * file, int track)
  {
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

  int result = 0;
  while(result < 1)
    {
    result = ogg_stream_packetout(&codec->dec_os, &codec->dec_op);
    
    //    fprintf(stderr, "Getting new packet %d\n", result);
    
    if(result == 0)
      {
      if(!next_page(file, track))
        return 0;
      }
    }
  return 1;
  }

float ** alloc_sample_buffer(float ** old, int channels, int samples,
                             int * sample_buffer_alloc)
  {
  int i;
  if(!old)
    {
    old = calloc(channels, sizeof(*(old)));
    }
  //  fprintf(stderr, "alloc_sample_buffer: samples: %d alloc: %d",
  //          samples, *sample_buffer_alloc);
  if(*sample_buffer_alloc < samples)
    {
    *sample_buffer_alloc = samples + 256;

    for(i = 0; i < channels; i++)
      {
      old[i] = realloc(old[i], (*sample_buffer_alloc) * sizeof(float));
      }
    }

  //  fprintf(stderr, " new_alloc: %d", *sample_buffer_alloc);

  return old;
  }

static int decode_frame(quicktime_t * file, int track)
  {
  int i;
  float ** channels;
  int samples_decoded = 0;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

  while(1)
    {
    samples_decoded = vorbis_synthesis_pcmout(&codec->dec_vd, &(channels));

    if(samples_decoded > 0)
      break;

    /* Decode new data */

    if(!next_packet(file, track))
      {
      return 0;
      }

    if(vorbis_synthesis(&codec->dec_vb, &codec->dec_op) == 0)
      {
      vorbis_synthesis_blockin(&codec->dec_vd,
                               &codec->dec_vb);
      }
    }

  codec->sample_buffer = alloc_sample_buffer(codec->sample_buffer,
                                             file->atracks[track].channels,
                                             codec->sample_buffer_end -
                                             codec->sample_buffer_start + samples_decoded,
                                             &(codec->sample_buffer_alloc));
#if 0
  fprintf(stderr, "Copy: %lld %lld %d %d\n",
          codec->sample_buffer_start, codec->sample_buffer_end,
          (int)(codec->sample_buffer_end - codec->sample_buffer_start),
          samples_decoded);
#endif

  for(i = 0; i < track_map->channels; i++)
    {
    memcpy(codec->sample_buffer[i] + (int)(codec->sample_buffer_end - codec->sample_buffer_start),
           channels[i], samples_decoded * sizeof(float));
    }
  vorbis_synthesis_read(&codec->dec_vd, samples_decoded);

  codec->sample_buffer_end += samples_decoded;
  return 1;
  }

static int decode(quicktime_t *file, 
                  void * _output,
                  long samples, 
                  int track) 
  {
  int i, j;
  int64_t chunk_sample; /* For seeking only */
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  //  int64_t total_samples;
  int samples_decoded;
  int samples_to_skip;
  int samples_to_move;
  int samples_copied;
  float * output;
  //  fprintf(stderr, "decode audio vorbis %lld\n", track_map->current_position);
  
  /* Initialize codec */
  if(!codec->decode_initialized)
    {
    codec->decode_initialized = 1;

    ogg_sync_init(&codec->dec_oy); /* Now we can read pages */

    vorbis_info_init(&codec->dec_vi);
    vorbis_comment_init(&codec->dec_vc);
    
    if(!next_page(file, track))
      {
      fprintf(stderr, "decode: next page failed\n");
      return 0;
      }
    if(!next_packet(file, track))
      {
      fprintf(stderr, "decode: next packet failed\n");
      return 0;
      }
    
    if(vorbis_synthesis_headerin(&codec->dec_vi, &codec->dec_vc,
                                 &codec->dec_op) < 0)
      {
      fprintf(stderr, "decode: vorbis_synthesis_headerin: not a vorbis header\n");
      return 0;
      }

    if(!next_packet(file, track))
      {
      fprintf(stderr, "decode: next packet failed\n");
      return 0;
      }
    if(vorbis_synthesis_headerin(&codec->dec_vi, &codec->dec_vc, &codec->dec_op) < 0)
      {
      fprintf(stderr, "decode: vorbis_synthesis_headerin: not a vorbis header\n");
      return 0;
      }
    if(!next_packet(file, track))
      return 0;

    if(vorbis_synthesis_headerin(&codec->dec_vi, &codec->dec_vc, &codec->dec_op) < 0)
      {
      fprintf(stderr, "decode: vorbis_synthesis_headerin: not a vorbis header\n");
      return 0;
      }
    codec->header_read = 1;
    vorbis_synthesis_init(&codec->dec_vd, &codec->dec_vi);
    vorbis_block_init(&codec->dec_vd, &codec->dec_vb);
    //    fprintf(stderr, "Initialized decoder\n");
    }

  /* Check if we have to reposition the stream */
  
  if(file->atracks[track].last_position != file->atracks[track].current_position)
    {
    if((file->atracks[track].current_position < codec->sample_buffer_start) || 
       (file->atracks[track].current_position + samples > codec->sample_buffer_end))
      {
      /* Get the next chunk */

      quicktime_chunk_of_sample(&chunk_sample,
                                &(track_map->current_chunk),
                                track_map->track,
                                track_map->current_position);
      
      if(track_map->current_chunk >= file->atracks[track].track->mdia.minf.stbl.stco.total_entries)
        {
#if 0
        fprintf(stderr, "Detected EOF %lld %ld\n",
                track_map->current_chunk,
                file->atracks[track].track->mdia.minf.stbl.stco.total_entries);
#endif
        return 0;
        }
#if 0
      fprintf(stderr, "Seek: pos: %lld, chunk: %lld, chunk_sample: %lld\n",
              file->atracks[track].current_position,
              file->atracks[track].current_chunk, chunk_sample);
#endif
      
      
      /* Reset everything */
      
      vorbis_dsp_clear(&codec->dec_vd);
      vorbis_block_clear(&codec->dec_vb);
      
      ogg_stream_clear(&codec->dec_os);
      ogg_sync_reset(&codec->dec_oy);
      codec->stream_initialized = 0;
      if(!next_page(file, track))
        return 0;

      /* Initialize again */
      
      ogg_sync_init(&codec->dec_oy);
      ogg_stream_init(&codec->dec_os, ogg_page_serialno(&codec->dec_og));
      vorbis_synthesis_init(&codec->dec_vd, &codec->dec_vi);
      vorbis_block_init(&codec->dec_vd, &codec->dec_vb);

      /* Reset sample buffer */
            
      codec->sample_buffer_start = chunk_sample;
      codec->sample_buffer_end   = chunk_sample;

      /* Decode frames until we have enough */
      
      while(track_map->current_position + samples > codec->sample_buffer_end)
        {
        if(!decode_frame(file, track))
          break;
        }
#if 0
      fprintf(stderr, "Cur: %lld, start: %lld end: %lld\n", track_map->current_position,
              codec->sample_buffer_start, codec->sample_buffer_end);
#endif
      }
    }

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
      for(i = 0; i < track_map->channels; i++)
        {
        memmove(codec->sample_buffer[i],
                &(codec->sample_buffer[i][samples_to_skip]),
                samples_to_move * sizeof(float));
        }
      //      fprintf(stderr, "done\n");
      }
    codec->sample_buffer_start = track_map->current_position;
    if(samples_to_move > 0)
      codec->sample_buffer_end = codec->sample_buffer_start + samples_to_move;
    else
      codec->sample_buffer_end = codec->sample_buffer_start;
    }
  

#if 0
    fprintf(stderr, "Samples: %lld -> %lld %lld, %ld\n",
            codec->sample_buffer_start, codec->sample_buffer_end,
            codec->sample_buffer_end - codec->sample_buffer_start, samples);
#endif

  /* Read new chunks until we have enough samples */
  while(codec->sample_buffer_end < codec->sample_buffer_start + samples)
    {
    if(!decode_frame(file, track))
      break;
    //    fprintf(stderr, "Decoded frame\n");
    }
  samples_decoded = (codec->sample_buffer_end - codec->sample_buffer_start) < samples ?
    (codec->sample_buffer_end - codec->sample_buffer_start) : samples;

  samples_copied = samples;
  if(samples_copied > samples_decoded)
    samples_copied = samples_decoded;

  output = (float*)_output;
  for(i = 0; i < samples_copied; i++)
    {
    for(j = 0; j < track_map->channels; j++)
      {
      *(output++) = codec->sample_buffer[j][i];
      }
    }
  
  file->atracks[track].last_position = file->atracks[track].current_position + samples_copied;
  
  //  fprintf(stderr, "Last pos: %ld\n", file->atracks[track].last_position);
  
  return samples_copied;
  }

/* Encoding part */

#define ENCODE_SAMPLES 4096

static int flush_header(quicktime_t * file, int track)
  {
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  while(ogg_stream_flush(&codec->enc_os, &codec->enc_og))
    {
    //    fprintf(stderr, "Got header page %ld + %ld bytes\n",
    //            codec->enc_og.header_len, codec->enc_og.body_len);

    codec->enc_header = realloc(codec->enc_header, codec->enc_header_len +
                                codec->enc_og.header_len + codec->enc_og.body_len);
    
    memcpy(codec->enc_header + codec->enc_header_len,
           codec->enc_og.header, codec->enc_og.header_len);

    memcpy(codec->enc_header + codec->enc_header_len + codec->enc_og.header_len,
           codec->enc_og.body, codec->enc_og.body_len);
    
    codec->enc_header_len += codec->enc_og.header_len + codec->enc_og.body_len;
    }
  
  return 0;
  }

static int flush_data(quicktime_t * file, int track)
  {
  int64_t new_encoded_samples;
  int result = 0;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_trak_t *trak = track_map->track;
  quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  
  while(vorbis_analysis_blockout(&codec->enc_vd, &codec->enc_vb) == 1)
    {
    /* While there is compressed data available... */    
    //    vorbis_analysis(&codec->enc_vb, &codec->enc_op);
    //    fprintf(stderr, "Got block\n");
    vorbis_analysis(&codec->enc_vb, NULL);
    //    vorbis_analysis(&codec->enc_vb, &codec->enc_op);
    vorbis_bitrate_addblock(&codec->enc_vb);
    while(vorbis_bitrate_flushpacket(&codec->enc_vd, &codec->enc_op))
      {
      //      fprintf(stderr, "Got packet\n");
      
      /* While packets are available */
      ogg_stream_packetin(&codec->enc_os, &codec->enc_op);
      
      }
    }
  while(!result)
    {
    //        ogg_stream_flush(&codec->enc_os, &codec->enc_og);
    //          break;
    /* While pages are available */
    
    if(!ogg_stream_flush(&codec->enc_os, &codec->enc_og))
      break;

    //    fprintf(stderr, "Got page %ld + %ld bytes\n",
    //            codec->enc_og.header_len, codec->enc_og.body_len);

    
    if(!codec->chunk_started)
      {
      codec->chunk_started = 1;
      lqt_start_audio_vbr_chunk(file, track);
      quicktime_write_chunk_header(file, trak, &codec->chunk_atom);
      //      fprintf(stderr, "Starting chunk\n");
      
      }
    lqt_start_audio_vbr_frame(file, track);
    if(!codec->header_written)
      {
      codec->header_written = 1;
      result = !quicktime_write_data(file, codec->enc_header, codec->enc_header_len);
      }
    
    result = !quicktime_write_data(file, codec->enc_og.header, codec->enc_og.header_len);
    if(!result)
      {
      result = !quicktime_write_data(file, codec->enc_og.body, codec->enc_og.body_len);
      }
    // fprintf(stderr, "Granulepos: %lld\n", ogg_page_granulepos(&(codec->enc_og)));

    new_encoded_samples = codec->enc_os.granulepos;

    lqt_finish_audio_vbr_frame(file, track, new_encoded_samples - codec->encoded_samples);
    codec->encoded_samples = new_encoded_samples;
    
    
#if 0
    fprintf(stderr, "Wrote page %ld + %ld bytes\n",
            codec->enc_og.header_len, codec->enc_og.body_len);
#endif
    if(ogg_page_eos(&codec->enc_og)) break;
    }
  return result;
  }

static void flush_audio(quicktime_t * file, int track)
  {
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  
  float ** output;
  int i;
#if 0
  fprintf(stderr, "Flush audio %d\n", codec->enc_samples_in_buffer);
#endif
  output = vorbis_analysis_buffer(&codec->enc_vd, codec->enc_samples_in_buffer);

  for(i = 0; i < track_map->channels; i++)
    {
    memcpy(output[i], codec->sample_buffer[i], sizeof(float) * codec->enc_samples_in_buffer);
    }
  vorbis_analysis_wrote(&codec->enc_vd, codec->enc_samples_in_buffer);
  codec->enc_samples_in_buffer = 0;
  flush_data(file, track);
  }

static int encode(quicktime_t *file, 
                  void *_input,
                  long samples,
                  int track)
  {
  int i, j;
  int samples_copied, samples_to_copy;
  int result = 0;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_trak_t *trak = track_map->track;
  quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  int samplerate = trak->mdia.minf.stbl.stsd.table[0].sample_rate;
  float * input;
  
  //        fprintf(stderr, "Encode vorbis %ld\n", samples);
        
  if(!codec->encode_initialized)
    {
    ogg_packet header;
    ogg_packet header_comm;
    ogg_packet header_code;

 
    codec->encode_initialized = 1;

    lqt_init_vbr_audio(file, track);


    if(file->use_avi)
      trak->mdia.minf.stbl.stsd.table[0].sample_size = 0;
    vorbis_info_init(&codec->enc_vi);

    if(codec->use_vbr)
      {
      result = vorbis_encode_setup_managed(&codec->enc_vi,
                                           track_map->channels, 
                                           samplerate, 
                                           codec->max_bitrate, 
                                           codec->nominal_bitrate, 
                                           codec->min_bitrate);
      result |= vorbis_encode_ctl(&codec->enc_vi, OV_ECTL_RATEMANAGE_AVG, NULL);
      result |= vorbis_encode_setup_init(&codec->enc_vi);
      }
    else
      {
      vorbis_encode_init(&codec->enc_vi,
                         track_map->channels,
                         samplerate, 
                         codec->max_bitrate, 
                         codec->nominal_bitrate, 
                         codec->min_bitrate);
      }


    vorbis_comment_init(&codec->enc_vc);
    vorbis_analysis_init(&codec->enc_vd, &codec->enc_vi);
    vorbis_block_init(&codec->enc_vd, &codec->enc_vb);
    //    	srand(time(NULL));
    ogg_stream_init(&codec->enc_os, rand());

    vorbis_analysis_headerout(&codec->enc_vd, 
                              &codec->enc_vc,
                              &header,
                              &header_comm,
                              &header_code);

    ogg_stream_packetin(&codec->enc_os, &header); 
    ogg_stream_packetin(&codec->enc_os, &header_comm);
    ogg_stream_packetin(&codec->enc_os, &header_code);

    //        FLUSH_OGG1
    flush_header(file, track);

    codec->sample_buffer = alloc_sample_buffer(codec->sample_buffer, track_map->channels,
                                               ENCODE_SAMPLES,
                                               &codec->sample_buffer_alloc);
    }
        
  samples_copied = 0;

  while(samples_copied < samples)
    {
    samples_to_copy = samples - samples_copied;
    if(samples_to_copy > ENCODE_SAMPLES - codec->enc_samples_in_buffer)
      samples_to_copy = ENCODE_SAMPLES - codec->enc_samples_in_buffer;
    input = ((float*)_input + samples_copied * track_map->channels);
    
    for(j = 0; j < samples_to_copy; j++)
      {
      for(i = 0; i < track_map->channels; i++)
        codec->sample_buffer[i][codec->enc_samples_in_buffer + j] = *(input++);
      
      }
    samples_copied += samples_to_copy;
    codec->enc_samples_in_buffer += samples_to_copy;
    
    if(codec->enc_samples_in_buffer >= ENCODE_SAMPLES)
      flush_audio(file, track);
    }
    
  result = 0;

  //    fprintf(stderr, "Encoded %lld samples\n",
  //            codec->enc_os.granulepos - codec->encoded_samples);
  
  // Wrote a chunk.
  if(codec->chunk_started)
    {
#if 0
    fprintf(stderr, "WRITING CHUNK: %lld %lld\n",
            codec->enc_os.granulepos, codec->enc_vd.granulepos);
#endif           
    quicktime_write_chunk_footer(file, 
                                 trak,
                                 track_map->current_chunk,
                                 &codec->chunk_atom, 
                                 track_map->vbr_num_frames);
    track_map->current_chunk++;
    codec->chunk_started = 0;
    }

  return result;
  }




static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_audio_map_t *atrack = &(file->atracks[track]);
	quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(!strcasecmp(key, "vorbis_vbr"))
		codec->use_vbr = *(int*)value;
	else
	if(!strcasecmp(key, "vorbis_bitrate"))
		codec->nominal_bitrate = *(int*)value;
	else
	if(!strcasecmp(key, "vorbis_max_bitrate"))
		codec->max_bitrate = *(int*)value;
	else
	if(!strcasecmp(key, "vorbis_min_bitrate"))
		codec->min_bitrate = *(int*)value;
	return 0;
}


static void flush(quicktime_t *file, int track)
{
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	quicktime_trak_t *trak = track_map->track;

//printf("flush 1\n");
        flush_audio(file, track);
	vorbis_analysis_wrote(&codec->enc_vd,0);

        flush_data(file, track);
        
        //	FLUSH_OGG2
	
//printf("flush 2 %d\n", size);
	if(codec->chunk_started)
	{
#if 0
                fprintf(stderr, "WRITING CHUNK: %lld %lld\n",
                        (long long)codec->enc_os.granulepos,
                        (long long)codec->enc_vd.granulepos);
#endif
                quicktime_write_chunk_footer(file, 
                                             trak,
                                             track_map->current_chunk,
                                             &codec->chunk_atom, 
                                             track_map->vbr_num_frames);
		track_map->current_chunk++;
                codec->chunk_started = 0;

	}
}

void quicktime_init_codec_vorbis(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_vorbis_codec_t *codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_vorbis_codec_t));
	codec_base->delete_acodec = delete_codec;
	codec_base->decode_audio = decode;
	codec_base->encode_audio = encode;
	codec_base->set_parameter = set_parameter;
	codec_base->flush = flush;
	codec_base->fourcc = QUICKTIME_VORBIS;
	codec_base->title = "OGG Vorbis";
	codec_base->desc = "OGG Vorbis for video. (Not standardized)";

	codec = codec_base->priv;
	codec->nominal_bitrate = 128000;
	codec->max_bitrate = -1;
	codec->min_bitrate = -1;
        atrack->sample_format = LQT_SAMPLE_FLOAT;
        
}
