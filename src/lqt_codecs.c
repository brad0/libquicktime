/*******************************************************************************
 lqt_codecs.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2007 Members of the libquicktime project.

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

static int quicktime_delete_vcodec_stub(quicktime_video_map_t *vtrack)
{
        lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN,
                "quicktime_delete_vcodec_stub called");
	return 0;
}

static int quicktime_delete_acodec_stub(quicktime_audio_map_t *atrack)
{
        lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN,
                "quicktime_delete_acodec_stub called");
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

static int quicktime_decode_audio_stub(quicktime_t *file, 
                                       void * output,
                                       long samples,
                                       int track)
{
        lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
                "quicktime_decode_audio_stub called");
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


static int quicktime_codec_defaults(quicktime_codec_t *codec)
{
	codec->delete_vcodec = quicktime_delete_vcodec_stub;
	codec->delete_acodec = quicktime_delete_acodec_stub;
	codec->decode_video = quicktime_decode_video_stub;
	codec->encode_video = quicktime_encode_video_stub;
	codec->decode_audio = quicktime_decode_audio_stub;
	codec->encode_audio = quicktime_encode_audio_stub;
	codec->flush = quicktime_flush_codec_stub;
	return 0;
}

/*
 *  Original quicktime4linux function changed for dynamic loading
 */

int quicktime_init_vcodec(quicktime_video_map_t *vtrack, int encode,
                          lqt_codec_info_t * codec_info)
  {
  lqt_codec_info_t ** codec_array = (lqt_codec_info_t**)0;
  
  lqt_init_video_codec_func_t init_codec;
  lqt_init_video_codec_func_t (*get_codec)(int);
    
  void * module = (void*)0;
  
  char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;

  vtrack->codec = calloc(1, sizeof(quicktime_codec_t));
  quicktime_codec_defaults((quicktime_codec_t*)vtrack->codec);

  ((quicktime_codec_t*)vtrack->codec)->module = (void*)0;
  
  /* Try to find the codec */

  if(!codec_info)
    {
    
    codec_array = lqt_find_video_codec(compressor, encode);
  
    if(!codec_array)
      {
      lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN, "Could not find video %s for fourcc %4s",
              (encode ? "Encoder" : "Decoder"), compressor);
      return -1;
      }
    codec_info = *codec_array;
    }

  vtrack->compatibility_flags = codec_info->compatibility_flags;
  
  lqt_log(NULL, LQT_LOG_DEBUG, LOG_DOMAIN,
          "Loading module %s", codec_info->module_filename);
  
  /* dlopen the module */
  module = dlopen(codec_info->module_filename, RTLD_NOW);
  
  if(!module)
    {
    lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN, "Loading module %s failed: %s",
            codec_info->module_filename, dlerror());
    
    if(codec_array)
      lqt_destroy_codec_info(codec_array);

    return -1;
    }
  
  ((quicktime_codec_t*)vtrack->codec)->codec_name =
    malloc(strlen(codec_info->name)+1);
  strcpy(((quicktime_codec_t*)vtrack->codec)->codec_name, codec_info->name);
  
  /* Set the module */
  
  ((quicktime_codec_t*)vtrack->codec)->module = module;
  
  /* Get the codec finder for the module */
  
  get_codec =
    (lqt_init_video_codec_func_t(*)(int))dlsym(module,
                                               "get_video_codec");
  
  if(!get_codec)
    {
    lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN,
            "Module %s contains no function get_video_codec",
            codec_info->module_filename);
    if(codec_array)
      lqt_destroy_codec_info(codec_array);

    return -1;
    }
  
  /* Obtain the initializer for the actual codec */
  
  init_codec = get_codec(codec_info->module_index);
  
  init_codec(vtrack);
  if(codec_array)
    lqt_destroy_codec_info(codec_array);

  //  vtrack->stream_cmodel = lqt_get_decoder_colormodel(quicktime_t * file, int track);
  
  return 0;
  
  }

int quicktime_init_acodec(quicktime_audio_map_t *atrack, int encode,
                          lqt_codec_info_t * codec_info)
  {
  lqt_codec_info_t ** codec_array = (lqt_codec_info_t**)0;
  lqt_init_audio_codec_func_t init_codec;
  lqt_init_audio_codec_func_t (*get_codec)(int);
    
  void * module;

  char *compressor = atrack->track->mdia.minf.stbl.stsd.table[0].format;
  int wav_id       = atrack->track->mdia.minf.stbl.stsd.table[0].compression_id;
  atrack->codec = calloc(1, sizeof(quicktime_codec_t));
  quicktime_codec_defaults((quicktime_codec_t*)atrack->codec);

  ((quicktime_codec_t*)(atrack->codec))->module = (void*)0;
  
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
      lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN, "Could not find audio %s for fourcc %4s",
              (encode ? "Encoder" : "Decoder"), compressor);
      return -1;
      }
    codec_info = *codec_array;
    }

  atrack->compatibility_flags = codec_info->compatibility_flags;
  
  lqt_log(NULL, LQT_LOG_DEBUG, LOG_DOMAIN,
          "Loading module %s", codec_info->module_filename);
  
  /* dlopen the module */
  module = dlopen(codec_info->module_filename, RTLD_NOW);
  
  if(!module)
    {
    lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN, "Loading module %s failed: %s",
            codec_info->module_filename, dlerror());
    if(codec_array)
      lqt_destroy_codec_info(codec_array);
    return -1;
    }

  ((quicktime_codec_t*)atrack->codec)->codec_name = malloc(strlen(codec_info->name)+1);
  strcpy(((quicktime_codec_t*)atrack->codec)->codec_name, codec_info->name);
    
  /* Set the module */
  
  ((quicktime_codec_t*)((quicktime_codec_t*)atrack->codec))->module = module;
  
  /* Get the codec finder for the module */
  
  get_codec =
    (lqt_init_audio_codec_func_t(*)(int))dlsym(module,
                                               "get_audio_codec");
  
  if(!get_codec)
    {
    lqt_log(NULL, LQT_LOG_WARNING, LOG_DOMAIN,
            "Module %s contains no function get_audio_codec",
            codec_info->module_filename);
    if(codec_array)
      lqt_destroy_codec_info(codec_array);
    return -1;
    }
  
  /* Obtain the initializer for the actual codec */

  init_codec = get_codec(codec_info->module_index);
  
  init_codec(atrack);

  /* We set the wav ids from our info structure, so we don't have to do this
     in the plugin sources */
  
  if(codec_info->num_wav_ids)
    atrack->wav_id = codec_info->wav_ids[0];
  
  if(codec_array)
    lqt_destroy_codec_info(codec_array);
  return 0;
  }


int quicktime_delete_vcodec(quicktime_video_map_t *vtrack)
  {
  ((quicktime_codec_t*)vtrack->codec)->delete_vcodec(vtrack);
  /* Close the module */
  
  if(((quicktime_codec_t*)vtrack->codec)->module)
    dlclose(((quicktime_codec_t*)vtrack->codec)->module);
  if(((quicktime_codec_t*)vtrack->codec)->codec_name)
    free(((quicktime_codec_t*)vtrack->codec)->codec_name);
  free(vtrack->codec);
  vtrack->codec = 0;
  return 0;
}

int quicktime_delete_acodec(quicktime_audio_map_t *atrack)
  {
  ((quicktime_codec_t*)atrack->codec)->delete_acodec(atrack);
  /* Close the module */
  if(((quicktime_codec_t*)atrack->codec)->module)
    dlclose(((quicktime_codec_t*)atrack->codec)->module);
  if(((quicktime_codec_t*)atrack->codec)->codec_name)
    free(((quicktime_codec_t*)atrack->codec)->codec_name);
  free(atrack->codec);
  atrack->codec = 0;
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

void lqt_update_frame_position(quicktime_video_map_t * track)
  {
  track->timestamp +=
    track->track->mdia.minf.stbl.stts.table[track->stts_index].sample_duration;

  track->stts_count++;

  if(track->stts_count >=
     track->track->mdia.minf.stbl.stts.table[track->stts_index].sample_count)
    {
    track->stts_index++;
    track->stts_count = 0;
    }
  track->current_position++;
  }

/* Set the io_rowspan for the case the user didn't. */

static void set_default_rowspan(quicktime_t *file, int track)
  {
  if(file->vtracks[track].io_row_span)
    return;

  lqt_get_default_rowspan(file->vtracks[track].io_cmodel,
                          quicktime_video_width(file, track),
                          &(file->vtracks[track].io_row_span),
                          &(file->vtracks[track].io_row_span_uv));
  }

/*
 *  Same as quicktime_decode_video but doesn't force BC_RGB888
 */

int lqt_decode_video(quicktime_t *file,
                     unsigned char **row_pointers, int track)
{
	int result;
        int height;
	int width;
        set_default_rowspan(file, track);
        
        height = quicktime_video_height(file, track);
	width =  quicktime_video_width(file, track);
        
        if(file->vtracks[track].io_cmodel != file->vtracks[track].stream_cmodel)
          {
          if(!file->vtracks[track].temp_frame)
            {
            file->vtracks[track].temp_frame =
              lqt_rows_alloc(width, height, file->vtracks[track].stream_cmodel,
                             &(file->vtracks[track].stream_row_span),
                             &(file->vtracks[track].stream_row_span_uv));
            }
          result =
            ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file,
                                                                           file->vtracks[track].temp_frame,
                                                                           track);
          cmodel_transfer(row_pointers,                    //    unsigned char **output_rows, /* Leave NULL if non existent */
                          file->vtracks[track].temp_frame, //    unsigned char **input_rows,
                          0, //                                  int in_x,        /* Dimensions to capture from input frame */
                          0, //                                  int in_y, 
                          width, //                              int in_w, 
                          height, //                             int in_h,
                          width, //                              int out_w, 
                          height, //                             int out_h,
                          file->vtracks[track].stream_cmodel, // int in_colormodel, 
                          file->vtracks[track].io_cmodel,     // int out_colormodel,
                          file->vtracks[track].stream_row_span,   /* For planar use the luma rowspan */
                          file->vtracks[track].io_row_span,       /* For planar use the luma rowspan */
                          file->vtracks[track].stream_row_span_uv, /* Chroma rowspan */
                          file->vtracks[track].io_row_span_uv      /* Chroma rowspan */);
         
          }
        else
          {
          file->vtracks[track].stream_row_span    = file->vtracks[track].io_row_span;
          file->vtracks[track].stream_row_span_uv = file->vtracks[track].io_row_span_uv;
          
          result = ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file, row_pointers, track);
          
          }
        
        lqt_update_frame_position(&(file->vtracks[track]));
	return result;
}

/* The original function, which forces BG_RGB888 */
int quicktime_decode_video(quicktime_t *file,
                           unsigned char **row_pointers, int track)
  {
  
  file->vtracks[track].io_cmodel = BC_RGB888;
  return lqt_decode_video(file, row_pointers, track);
  }


long quicktime_decode_scaled(quicktime_t *file, 
	int in_x,                    /* Location of input frame to take picture */
	int in_y,
	int in_w,
	int in_h,
	int out_w,                   /* Dimensions of output frame */
	int out_h,
	int color_model,             /* One of the color models defined above */
	unsigned char **row_pointers, 
	int track)
{
	int result;

        int height;
	int width;
        
        set_default_rowspan(file, track);
	height = quicktime_video_height(file, track);
	width =  quicktime_video_width(file, track);

        
	file->vtracks[track].io_cmodel = color_model;

        if(!file->vtracks[track].temp_frame)
          {
          file->vtracks[track].temp_frame =
            lqt_rows_alloc(width, height, file->vtracks[track].stream_cmodel,
                           &(file->vtracks[track].stream_row_span),
                           &(file->vtracks[track].stream_row_span_uv));
          }
        result =
          ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file,
                                                                         file->vtracks[track].temp_frame,
                                                                         track);
        cmodel_transfer(row_pointers,                    //    unsigned char **output_rows, /* Leave NULL if non existent */
                        file->vtracks[track].temp_frame, //    unsigned char **input_rows,
                        in_x, //                               int in_x,        /* Dimensions to capture from input frame */
                        in_y, //                               int in_y, 
                        in_w, //                               int in_w, 
                        in_h, //                               int in_h,
                        out_w, //                              int out_w, 
                        out_h, //                              int out_h,
                        file->vtracks[track].stream_cmodel, // int in_colormodel, 
                        file->vtracks[track].io_cmodel,     // int out_colormodel,
                        file->vtracks[track].stream_row_span,   /* For planar use the luma rowspan */
                        file->vtracks[track].io_row_span,       /* For planar use the luma rowspan */
                        file->vtracks[track].stream_row_span_uv, /* Chroma rowspan */
                        file->vtracks[track].io_row_span_uv      /* Chroma rowspan */);
        
        lqt_update_frame_position(&(file->vtracks[track]));
	return result;
}

int lqt_set_video_pass(quicktime_t *file,
                       int pass, int total_passes, 
                       const char * stats_file, int track)
  {
  if(((quicktime_codec_t*)file->vtracks[track].codec)->set_pass)
    return ((quicktime_codec_t*)file->vtracks[track].codec)->set_pass(file,
                                                                      track,
                                                                      pass,
                                                                      total_passes,
                                                                      stats_file);
  else
    return 0;
  }

static void start_encoding(quicktime_t *file)
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
    if(!(file->atracks[i].compatibility_flags & file->file_type))
      lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
              "Audio codec and container are not known to be compatible. File might be playable by libquicktime only.");
    }
  for(i = 0; i < file->total_vtracks; i++)
    {
    if(!(file->vtracks[i].compatibility_flags & file->file_type))
      lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
              "Video codec and container are not known to be compatible. File might be playable by libquicktime only.");
    
    }
  }


static int do_encode_video(quicktime_t *file, 
                           unsigned char **row_pointers, 
                           int track)
  {
  int result;

  int height;
  int width;

  start_encoding(file);
  
  set_default_rowspan(file, track);
  height = quicktime_video_height(file, track);
  width =  quicktime_video_width(file, track);
  
  if(file->vtracks[track].io_cmodel != file->vtracks[track].stream_cmodel)
    {
    if(!file->vtracks[track].temp_frame)
      {
      file->vtracks[track].temp_frame =
        lqt_rows_alloc(width, height, file->vtracks[track].stream_cmodel,
                       &(file->vtracks[track].stream_row_span),
                       &(file->vtracks[track].stream_row_span_uv));
      }
    cmodel_transfer(file->vtracks[track].temp_frame, //    unsigned char **output_rows, /* Leave NULL if non existent */
                    row_pointers,                    //    unsigned char **input_rows,
                    0, //                                  int in_x,        /* Dimensions to capture from input frame */
                    0, //                                  int in_y, 
                    width, //                              int in_w, 
                    height, //                             int in_h,
                    width, //                              int out_w, 
                    height, //                             int out_h,
                    file->vtracks[track].io_cmodel, // int in_colormodel, 
                    file->vtracks[track].stream_cmodel,     // int out_colormodel,
                    file->vtracks[track].io_row_span,   /* For planar use the luma rowspan */
                    file->vtracks[track].stream_row_span,       /* For planar use the luma rowspan */
                    file->vtracks[track].io_row_span_uv, /* Chroma rowspan */
                    file->vtracks[track].stream_row_span_uv      /* Chroma rowspan */);
    result = ((quicktime_codec_t*)file->vtracks[track].codec)->encode_video(file, file->vtracks[track].temp_frame, track);
    }
  else
    {
    file->vtracks[track].stream_row_span    = file->vtracks[track].io_row_span;
    file->vtracks[track].stream_row_span_uv = file->vtracks[track].io_row_span_uv;
    result = ((quicktime_codec_t*)file->vtracks[track].codec)->encode_video(file, row_pointers, track);
    }
  return result;
  }

void lqt_write_frame_header(quicktime_t * file, int track,
                            int pic_num1,
                            int64_t pic_pts, int keyframe)
  {
  quicktime_video_map_t * vtrack = &file->vtracks[track];
  quicktime_trak_t * trak = vtrack->track;
  int pic_num ;
  int i;
  if(pic_num1 >= 0)
    pic_num = pic_num1;
  else
    {
    for(i = 0; i < vtrack->current_position; i++)
      {
      if(vtrack->timestamps[i] == pic_pts)
        {
        pic_num = i;
        break;
        }
      }
    }

  if(vtrack->cur_chunk >= vtrack->picture_numbers_alloc)
    {
    vtrack->picture_numbers_alloc += 1024;
    vtrack->picture_numbers = realloc(vtrack->picture_numbers,
                                      sizeof(*vtrack->picture_numbers) *
                                      vtrack->picture_numbers_alloc);
    }
  vtrack->picture_numbers[vtrack->cur_chunk] = pic_num;
  vtrack->keyframe = keyframe;
  
  quicktime_write_chunk_header(file, trak, &vtrack->chunk_atom);
  }


void lqt_write_frame_footer(quicktime_t * file, int track)
  {
  quicktime_video_map_t * vtrack = &file->vtracks[track];
  quicktime_trak_t * trak = vtrack->track;
  
  quicktime_write_chunk_footer(file, 
                               trak, 
                               vtrack->cur_chunk,
                               &vtrack->chunk_atom, 
                               1);

  if(vtrack->keyframe)
    quicktime_insert_keyframe(file, vtrack->cur_chunk, track);
  vtrack->cur_chunk++;
  }

void lqt_video_build_timestamp_tables(quicktime_t * file, int track)
  {
  int i;
  quicktime_video_map_t * vtrack = &file->vtracks[track];
  quicktime_trak_t * trak = vtrack->track;
  quicktime_stts_t * stts;
  quicktime_ctts_t * ctts;
  quicktime_stts_table_t * stts_tab;
  int64_t dts;
  int has_b_frames = 0;

  stts = &trak->mdia.minf.stbl.stts;
  ctts = &trak->mdia.minf.stbl.ctts;
  
  /* Check if we have B-frames */
  for(i = 0; i < vtrack->cur_chunk-1; i++)
    {
    if(vtrack->picture_numbers[i] + 1 !=
       vtrack->picture_numbers[i+1])
      {
      has_b_frames = 1;
      break;
      }
    }
  
  /* Build preliminary stts */

  if(stts->table)
    free(stts->table);
  
  stts->table = malloc(vtrack->cur_chunk * sizeof(*stts->table));
  stts->total_entries = vtrack->cur_chunk;
  
  for(i = 0; i < vtrack->cur_chunk-1; i++)
    {
    stts->table[i].sample_count = 1;
    stts->table[i].sample_duration =
      vtrack->timestamps[i+1] - vtrack->timestamps[i];
    }

  /* Last entry */
  stts->table[vtrack->cur_chunk-1].sample_count = 1;
  stts->table[vtrack->cur_chunk-1].sample_duration =
    vtrack->duration - vtrack->timestamps[vtrack->cur_chunk-1];

  if(stts->table[vtrack->cur_chunk-1].sample_duration <= 0)
    stts->table[vtrack->cur_chunk-1].sample_duration = stts->default_duration;
  
  if(!has_b_frames)
    return;

  /* If we have B-frames, reorder stts and build ctts */
  
  stts_tab = malloc(vtrack->cur_chunk * sizeof(*stts_tab));
  ctts->table = malloc(vtrack->cur_chunk * sizeof(*ctts->table));
  ctts->total_entries = vtrack->cur_chunk;
  trak->mdia.minf.stbl.has_ctts = 1;

  dts = 0;
  /* Loop over *coded* pictures */
  for(i = 0; i < vtrack->cur_chunk; i++)
    {
    stts_tab[i].sample_duration = stts->table[vtrack->picture_numbers[i]].sample_duration;
    stts_tab[i].sample_count = 1;

    /* CTS = PTS - DTS */
    ctts->table[i].sample_count = 1;
    ctts->table[i].sample_duration     = vtrack->timestamps[vtrack->picture_numbers[i]] - dts;
    dts += stts_tab[i].sample_duration;
    }
  
  free(stts->table);
  stts->table = stts_tab;
  }

void lqt_video_append_timestamp(quicktime_t * file, int track,
                                int64_t time, int duration)
  {
  quicktime_video_map_t * vtrack = &file->vtracks[track];
  /* Update timestamp table */
  if(vtrack->current_position >= vtrack->timestamps_alloc)
    {
    vtrack->timestamps_alloc += 1024;
    vtrack->timestamps = realloc(vtrack->timestamps,
                                 vtrack->timestamps_alloc *
                                 sizeof(*vtrack->timestamps));
    }
  vtrack->timestamps[vtrack->current_position] = time;
  vtrack->duration = time + duration;
  }


int lqt_encode_video(quicktime_t *file, 
                     unsigned char **row_pointers, 
                     int track, int64_t time)
  {
  return lqt_encode_video_d(file, row_pointers, track, time, -1);
  }

int lqt_encode_video_d(quicktime_t *file, 
                       unsigned char **row_pointers, 
                       int track, int64_t time, int duration)
  {
  int result;

  quicktime_video_map_t * vtrack = &file->vtracks[track];
  
  /* Must set valid timestamp for encoders */
  vtrack->timestamp = time;

  lqt_video_append_timestamp(file, track, time, duration);
  
  /* B-Frame handling: B-Frame enabled codecs can have a delay */
  /* Here, we use vtrack->current_chunk as index of the actually written
     frame (implicitely assuming, that ther is never more than one frame per chunk),
     vtrack->current_position is number of frames, which came from the user */
    
  result = do_encode_video(file, row_pointers, track);
  if (result)
     return(result);

  if(file->io_error)
    return 1;
#if 0  
  if(vtrack->has_b_frames)
    {
    file->vtracks[track].track->mdia.minf.stbl.has_ctts = 1;

    if(file->vtracks[track].current_position)
      quicktime_update_stts(&file->vtracks[track].track->mdia.minf.stbl.stts,
                            file->vtracks[track].current_position - 1,
                            time - last_time);
    
    if(file->vtracks[track].current_chunk > 1) /* Update ctts */
      {
      dts = quicktime_sample_to_time(&file->vtracks[track].track->mdia.minf.stbl.stts,
                                     file->vtracks->current_chunk-2,
                                     &stts_index, &stts_count);

      quicktime_update_ctts(&file->vtracks[track].track->mdia.minf.stbl.ctts,
                            file->vtracks[track].current_chunk - 2,
                            file->vtracks[track].coded_timestamp - dts);
      }
    }
  else
    {
    if(file->vtracks[track].current_position)
      quicktime_update_stts(&file->vtracks[track].track->mdia.minf.stbl.stts,
                            file->vtracks[track].current_position - 1,
                            time - last_time);
    }
#endif
  if(file->vtracks[track].timecode_track)
    lqt_flush_timecode(file, track, time, 0);
  
  file->vtracks[track].current_position++;
  return 0;
  }

int quicktime_encode_video(quicktime_t *file, 
                           unsigned char **row_pointers, 
                           int track)
  {
  int result;
  
  result = lqt_encode_video_d(file, 
                              row_pointers, 
                              track, file->vtracks[track].timestamp,
                              file->vtracks[track].track->mdia.minf.stbl.stts.default_duration);
  file->vtracks[track].timestamp +=
    file->vtracks[track].track->mdia.minf.stbl.stts.default_duration;
  return result;
  }

static int bytes_per_sample(lqt_sample_format_t format)
  {
  switch(format)
    {
    case LQT_SAMPLE_INT8:
    case LQT_SAMPLE_UINT8:
      return 1;
      break;
    case LQT_SAMPLE_INT16:
      return 2;
      break;
    case LQT_SAMPLE_INT32:
      return 4;
      break;
    case LQT_SAMPLE_FLOAT: /* Float is ALWAYS machine native */
      return sizeof(float);
      break;
    case LQT_SAMPLE_DOUBLE: /* Double is ALWAYS machine native */
      return sizeof(double);
      break;
    case LQT_SAMPLE_UNDEFINED:
      return 0;
    }
  return 0;
  }

/* Decode raw samples */

int lqt_decode_audio_raw(quicktime_t *file,  void * output, long samples, int track)
  {
  int result;
  quicktime_audio_map_t * atrack;
  atrack = &(file->atracks[track]);
  result = ((quicktime_codec_t*)(atrack->codec))->decode_audio(file, output, 
                                                               samples,
                                                               track);

  file->atracks[track].current_position += samples;
  return result;
  }

int lqt_encode_audio_raw(quicktime_t *file,  void * input, long samples, int track)
  {
  int result;
  quicktime_audio_map_t * atrack;
  if(!samples)
    return 0;
  atrack = &(file->atracks[track]);
  start_encoding(file);
  file->atracks[track].current_position += samples;
  result = ((quicktime_codec_t*)(atrack->codec))->encode_audio(file, input, 
                                                               samples,
                                                               track);
  if(file->io_error)
    return 0;
  else
    return samples;
  }

/* Compatibility function for old decoding API */

static int decode_audio_old(quicktime_t *file, 
                            int16_t ** output_i, 
                            float ** output_f, 
                            long samples, 
                            int track)
  {
  int result;
  quicktime_audio_map_t * atrack;
  atrack = &(file->atracks[track]);

  if(atrack->sample_format == LQT_SAMPLE_UNDEFINED)
    ((quicktime_codec_t*)(atrack->codec))->decode_audio(file, (void*)0, 
                                                        0, track);
  
  
  /* (Re)allocate sample buffer */

  if(atrack->sample_buffer_alloc < samples)
    {
    atrack->sample_buffer_alloc = samples + 1024;
    atrack->sample_buffer = realloc(atrack->sample_buffer,
                                    atrack->sample_buffer_alloc *
                                    atrack->channels *
                                    bytes_per_sample(atrack->sample_format));
    }
  
  /* Decode */

  result = ((quicktime_codec_t*)(atrack->codec))->decode_audio(file, atrack->sample_buffer, 
                                                               samples,
                                                               track);
  
  /* Convert */
  lqt_convert_audio_decode(file, atrack->sample_buffer, output_i, output_f,
                           atrack->channels, samples,
                           atrack->sample_format);
  return result;
  }

int quicktime_decode_audio(quicktime_t *file, 
                           int16_t *output_i, 
                           float *output_f, 
                           long samples, 
                           int channel)
{
        float   ** channels_f;
        int16_t ** channels_i;

        int quicktime_track, quicktime_channel;
	int result = 1;

        quicktime_channel_location(file, &quicktime_track,
                                   &quicktime_channel, channel);

        if(file->atracks[quicktime_track].eof)
          return 1;
        
        if(output_i)
          {
          channels_i = calloc(quicktime_track_channels(file, quicktime_track), sizeof(*channels_i));
          channels_i[quicktime_channel] = output_i;
          }
        else
          channels_i = (int16_t**)0;
        
        if(output_f)
          {
          channels_f = calloc(quicktime_track_channels(file, quicktime_track), sizeof(*channels_f));
          channels_f[quicktime_channel] = output_f;
          }
        else
          channels_f = (float**)0;
        
	result = decode_audio_old(file, channels_i, channels_f, samples, quicktime_track);
	file->atracks[quicktime_track].current_position += result;

        if(channels_i)
          free(channels_i);
        else if(channels_f)
          free(channels_f);
	return ((result < 0) ? 1 : 0);
}

int lqt_total_channels(quicktime_t *file)
{
	int i = 0, total_channels = 0;
	for( i=0; i < file->total_atracks; i++ )
	{
		total_channels += file->atracks[i].channels;
	}

	return total_channels;
}

/*
 * Same as quicktime_decode_audio, but it grabs all channels at
 * once. Or if you want only some channels you can leave the channels
 * you don't want = NULL in the poutput array. The poutput arrays
 * must contain at least lqt_total_channels(file) elements.
 */

int lqt_decode_audio(quicktime_t *file, 
				int16_t **poutput_i, 
				float **poutput_f, 
				long samples)
{
	int result = 1;
	int i = 0;
	int16_t **output_i;
	float   **output_f;

	int total_tracks = quicktime_audio_tracks(file);
        int track_channels;

        if(poutput_i)
          output_i = poutput_i;
        else
          output_i = (int16_t**)0;

        if(poutput_f)
          output_f = poutput_f;
        else
          output_f = (float**)0;
        
	for( i=0; i < total_tracks; i++ )
          {
          track_channels = quicktime_track_channels(file, i);

          if(file->atracks[i].eof)
            return 1;
                    
          result = decode_audio_old(file, output_i, output_f, samples, i);
          if(output_f)
            output_f += track_channels;
          if(output_i)
            output_i += track_channels;

          file->atracks[i].current_position += samples;
          }


	return result;
}

int lqt_decode_audio_track(quicktime_t *file, 
                           int16_t **poutput_i, 
                           float **poutput_f, 
                           long samples,
                           int track)
  {
  int result = 1;

  if(file->atracks[track].eof)
    return 1;
          
  
  result = !decode_audio_old(file, poutput_i, poutput_f, samples, track);
  
  file->atracks[track].current_position += samples;
  
  return result;
  }

static int encode_audio_old(quicktime_t *file, 
                     int16_t **input_i, 
                     float **input_f, 
                     long samples,
                     int track)
  {
  quicktime_audio_map_t * atrack;
  atrack = &(file->atracks[track]);
  start_encoding(file);
  
  if(!samples)
    return 0;
  
  if(atrack->sample_format == LQT_SAMPLE_UNDEFINED)
    ((quicktime_codec_t*)(atrack->codec))->encode_audio(file, (void*)0, 
                                                        0, track);

  
  /* (Re)allocate sample buffer */

  if(atrack->sample_buffer_alloc < samples)
    {
    atrack->sample_buffer_alloc = samples + 1024;
    atrack->sample_buffer = realloc(atrack->sample_buffer,
                                    atrack->sample_buffer_alloc *
                                    atrack->channels *
                                    bytes_per_sample(atrack->sample_format));
    }

  /* Convert */

  lqt_convert_audio_encode(file, input_i, input_f, atrack->sample_buffer, atrack->channels, samples,
                           atrack->sample_format);

  /* Encode */

  file->atracks[track].current_position += samples;
  
  return ((quicktime_codec_t*)(atrack->codec))->encode_audio(file, atrack->sample_buffer,
                                                             samples, track);
  
  }

int lqt_encode_audio_track(quicktime_t *file, 
                           int16_t **input_i, 
                           float **input_f, 
                           long samples,
                           int track)
  {
	int result = 1;

	result = encode_audio_old(file, input_i, input_f, samples, track);
        return result;
  
  }

int quicktime_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f, long samples)
  {
  return lqt_encode_audio_track(file, input_i, input_f, samples, 0);
  }

int quicktime_reads_cmodel(quicktime_t *file, 
	int colormodel, 
	int track)
{
        return lqt_colormodel_has_conversion(file->vtracks[track].stream_cmodel, colormodel);
}

int quicktime_writes_cmodel(quicktime_t *file, 
	int colormodel, 
	int track)
{
        return lqt_colormodel_has_conversion(colormodel, file->vtracks[track].stream_cmodel);
}

/* Compressors that can only encode a window at a time */
/* need to flush extra data here. */

static int quicktime_flush_acodec(quicktime_t *file, int track)
{
	((quicktime_codec_t*)file->atracks[track].codec)->flush(file, track);
	return 0;
};

static void quicktime_flush_vcodec(quicktime_t *file, int track)
  {
  
  while(((quicktime_codec_t*)file->vtracks[track].codec)->flush(file, track))
    ;
  
  /* Fix stts for timecode track */
  if(file->vtracks[track].timecode_track &&
     file->vtracks[track].timecodes_written)
    {
    int64_t duration;
    quicktime_trak_duration(file->vtracks[track].track,
                            &duration, (int*)0);
    lqt_flush_timecode(file, track, duration, 1);
    }
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

/* Copy audio */

int lqt_copy_audio(int16_t ** dst_i, float ** dst_f,
                   int16_t ** src_i, float ** src_f,
                   int dst_pos, int src_pos,
                   int dst_size, int src_size, int num_channels)
  {
  
  int i, j, i_tmp;
  int samples_to_copy;
  samples_to_copy = src_size < dst_size ? src_size : dst_size;

  
  if(src_i)
    {
    for(i = 0; i < num_channels; i++)
      {
      if(dst_i && dst_i[i]) /* int -> int */
        {
        memcpy(dst_i[i] + dst_pos, src_i[i] + src_pos, samples_to_copy * sizeof(int16_t));
        }
      if(dst_f && dst_f[i]) /* int -> float */
        {
        for(j = 0; j < samples_to_copy; j++)
          {
          dst_f[i][dst_pos + j] = (float)src_i[i][src_pos + j] / 32767.0;
          }
        }
      }
    }
  else if(src_f)
    {
    for(i = 0; i < num_channels; i++)
      {
      if(dst_i && dst_i[i]) /* float -> int */
        {
        for(j = 0; j < samples_to_copy; j++)
          {
          i_tmp = (int)(src_f[i][src_pos + j] * 32767.0);
          
          if(i_tmp > 32767)
            i_tmp = 32767;

          if(i_tmp < -32768)
            i_tmp = -32768;
          
          dst_i[i][dst_pos + j] = i_tmp;
          }
        }
      if(dst_f && dst_f[i]) /* float -> float */
        {
        memcpy(dst_f[i] + dst_pos, src_f[i] + src_pos, samples_to_copy * sizeof(float));
        }
      }
    }
  return samples_to_copy;
  }

