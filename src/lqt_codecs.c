/*******************************************************************************
 lqt_codecs.c

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
#include "quicktime/colormodels.h"
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#define LOG_DOMAIN "codecs"

static int quicktime_delete_codec_stub(quicktime_codec_t *codec)
  {
  lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN,
          "quicktime_delete_stub called");
  return 0;
  }

static int quicktime_decode_video_stub(quicktime_t *file, 
                                       unsigned char **row_pointers, 
                                       int track)
  {
  lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
          "quicktime_decode_video_stub called");
  return 1;
  }

static int quicktime_encode_video_stub(quicktime_t *file, 
                                       unsigned char **row_pointers, 
                                       int track)
  {
  lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
          "quicktime_encode_video_stub called");
  return 1;
  }

static int quicktime_decode_audio_packet_stub(quicktime_t *file, 
                                              int track, lqt_audio_buffer_t * buf)
  {
  lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
          "quicktime_decode_audio_packet_stub called");
  return 0;
  }

static int quicktime_encode_audio_stub(quicktime_t *file, 
                                       void * input,
                                       long samples,
                                       int track)
  {
  lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
          "quicktime_encode_audio_stub called");
  return 1;
  }

static int quicktime_flush_codec_stub(quicktime_t *file, int track)
  {
  return 0;
  }

/*
 *  Original quicktime4linux function changed for dynamic loading
 */
quicktime_codec_t * quicktime_load_codec(lqt_codec_info_t * info,
                                         quicktime_audio_map_t * amap,
                                         quicktime_video_map_t * vmap)
  {
  quicktime_codec_t * codec;
  lqt_init_codec_func_t init_codec;
  lqt_init_codec_func_t (*get_codec)(int);

  codec = calloc(1, sizeof(*codec));

  /* Set stubs */
  codec->delete_codec = quicktime_delete_codec_stub;
  codec->decode_video = quicktime_decode_video_stub;
  codec->encode_video = quicktime_encode_video_stub;
  codec->encode_audio = quicktime_encode_audio_stub;
  codec->decode_audio_packet = quicktime_decode_audio_packet_stub;
  codec->flush = quicktime_flush_codec_stub;
  
  if(!info)
    return codec;
  
  codec->info = lqt_codec_info_copy_single(info);
  
  lqt_log(NULL, LQT_LOG_DEBUG, LOG_DOMAIN,
          "Loading module %s", info->module_filename);

  /* dlopen the module */
  codec->module = dlopen(info->module_filename, RTLD_NOW);
  
  if(!codec->module)
    {
    lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN, "Loading module %s failed: %s",
            info->module_filename, dlerror());
    goto fail;
    }

  get_codec =
    (lqt_init_codec_func_t(*)(int))dlsym(codec->module, "get_codec");
  
  if(!get_codec)
    {
    lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN,
            "Module %s contains no function get_codec",
            info->module_filename);
    goto fail;
    }

  init_codec = get_codec(info->module_index);
  init_codec(codec, amap, vmap);
  
  return codec;

  fail:
  if(codec->module)
    dlclose(codec->module);
  free(codec);
  return NULL;
  }

int quicktime_init_vcodec(quicktime_video_map_t *vtrack, int encode,
                          lqt_codec_info_t * codec_info)
  {
  lqt_codec_info_t ** codec_array = (lqt_codec_info_t**)0;
  
  char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
  
  /* Try to find the codec */

  if(!codec_info)
    {
    codec_array = lqt_find_video_codec(compressor, encode);
  
    if(!codec_array)
      {
      lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN,
              "Could not find video %s for fourcc %4s",
              (encode ? "Encoder" : "Decoder"), compressor);
      }
    else
      codec_info = *codec_array;
    }
  
  vtrack->codec = quicktime_load_codec(codec_info, NULL, vtrack);

  if(!vtrack->codec)
    {
    lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN, "Loading codec %s failed",
            codec_info->name);
    
    if(codec_array)
      lqt_destroy_codec_info(codec_array);

    return -1;
    
    }

  if(codec_array)
    lqt_destroy_codec_info(codec_array);

  //  vtrack->stream_cmodel = lqt_get_decoder_colormodel(quicktime_t * file, int track);
  
  return 0;
  
  }

int quicktime_init_acodec(quicktime_audio_map_t *atrack, int encode,
                          lqt_codec_info_t * codec_info)
  {
  lqt_codec_info_t ** codec_array = (lqt_codec_info_t**)0;
  
  char *compressor = atrack->track->mdia.minf.stbl.stsd.table[0].format;
  int wav_id       = atrack->track->mdia.minf.stbl.stsd.table[0].compression_id;
  
  /* Try to find the codec */

  if(!codec_info)
    {
    if(compressor && (*compressor != '\0'))
      {
      codec_array = lqt_find_audio_codec(compressor, encode);
      }
    else if(wav_id)
      {
      codec_array = lqt_find_audio_codec_by_wav_id(wav_id, encode);
      }
    if(!codec_array)
      {
      lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN,
              "Could not find audio %s for fourcc %4s",
              (encode ? "Encoder" : "Decoder"), compressor);
      }
    else
      codec_info = *codec_array;
    }

  if(!codec_info)
    return 0;
  
  atrack->codec = quicktime_load_codec(codec_info, atrack, NULL);
  
  /* We set the wav ids from our info structure, so we don't have to do this
     in the plugin sources */
  
  if(codec_info && codec_info->num_wav_ids)
    atrack->wav_id = codec_info->wav_ids[0];
  
  if(codec_array)
    lqt_destroy_codec_info(codec_array);
  return 0;
  }


int quicktime_delete_codec(quicktime_codec_t *codec)
  {
  codec->delete_codec(codec);
  /* Close the module */
  
  if(codec->module)
    dlclose(codec->module);

  if(codec->info)
    lqt_codec_info_destroy_single(codec->info);

  free(codec);
  return 0;
  }

int quicktime_supported_video(quicktime_t *file, int track)
  {
  char *compressor = quicktime_video_compressor(file, track);
  lqt_codec_info_t ** test_codec =
    lqt_find_video_codec(compressor, file->wr);
  if(!test_codec)
    return 0;
        
  lqt_destroy_codec_info(test_codec);
  return 1;
  }

int quicktime_supported_audio(quicktime_t *file, int track)
  {
  lqt_codec_info_t ** test_codec;
  char *compressor = quicktime_audio_compressor(file, track);

  test_codec = (lqt_codec_info_t**)0;
         
  if(compressor && (*compressor != '\0'))
    test_codec = lqt_find_audio_codec(compressor, file->wr);
  else if(lqt_is_avi(file))
    test_codec = lqt_find_audio_codec_by_wav_id(lqt_get_wav_id(file, track), file->wr);
         
  if(!test_codec)
    return 0;
        
  lqt_destroy_codec_info(test_codec);
  return 1;
  }


void lqt_start_encoding(quicktime_t *file)
  {
  int i;
  if(file->encoding_started)
    return;

  file->encoding_started = 1;

  if(file->file_type & (LQT_FILE_AVI|LQT_FILE_AVI_ODML))
    {
    quicktime_set_position(file, 0);
    // Write RIFF chunk
    quicktime_init_riff(file);
    }
  
  /* Trigger warnings if codecs are in the wrong container */
  for(i = 0; i < file->total_atracks; i++)
    {
    if(!(file->atracks[i].codec->info->compatibility_flags & file->file_type))
      lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
              "Audio codec and container are not known to be compatible. File might be playable by libquicktime only.");
    }
  for(i = 0; i < file->total_vtracks; i++)
    {
    if(!(file->vtracks[i].codec->info->compatibility_flags & file->file_type))
      lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
              "Video codec and container are not known to be compatible. File might be playable by libquicktime only.");
    
    }
  }

/* Compressors that can only encode a window at a time */
/* need to flush extra data here. */

static int quicktime_flush_acodec(quicktime_t *file, int track)
  {
  file->atracks[track].codec->flush(file, track);
  return 0;
  }

static void quicktime_flush_vcodec(quicktime_t *file, int track)
  {
  while(file->vtracks[track].codec->flush(file, track))
    ;
  }

int quicktime_codecs_flush(quicktime_t *file)
  {
  int result = 0;
  int i;
  if(!file->wr) return result;

  if(file->total_atracks)
    {
    for(i = 0; i < file->total_atracks && !result; i++)
      {
      quicktime_flush_acodec(file, i);
      }
    }

  if(file->total_vtracks)
    {
    for(i = 0; i < file->total_vtracks && !result; i++)
      {
      quicktime_flush_vcodec(file, i);
      }
    }
  return result;
  }

