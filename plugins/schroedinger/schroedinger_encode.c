/*******************************************************************************
 schroedinger_encode.c

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

#define LQT_LIBQUICKTIME /* Hack: This prevents multiple compilation of
                            get_codec_api_version() */

#include "schroedinger.h"
#include <quicktime/colormodels.h>
#include <string.h>

#define LOG_DOMAIN "schroenc"

static void copy_frame_8(quicktime_t * file,
                         unsigned char **row_pointers,
                         SchroFrame * frame,
                         int track);

int lqt_schroedinger_set_enc_parameter(quicktime_t *file, 
                                       int track, 
                                       const char *key, 
                                       const void *value)
  {
  int i, j;
  double v;
  int found = 0;
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;
  
  /* Find the parameter */
  i = 0;

  while(encode_parameters_schroedinger[i].name)
    {
    if(!strcmp(key, encode_parameters_schroedinger[i].name))
      break;
    i++;
    }
  if(!encode_parameters_schroedinger[i].name)
    return 0;
  switch(encode_parameters_schroedinger[i].type)
    {
    case LQT_PARAMETER_INT:
      v = (double)(*(int*)(value));
      found = 1;
      break;
    case LQT_PARAMETER_FLOAT:
      v = (double)(*(float*)(value));
      found = 1;
      break;
    case LQT_PARAMETER_STRINGLIST:
      j = 0;
      while(encode_parameters_schroedinger[i].stringlist_options[j])
        {
        if(!strcmp(encode_parameters_schroedinger[i].stringlist_options[j],
                   (char*)value))
          {
          v = (double)(j);
          found = 1;
          break;
          }
        j++;
        }
      break;
    default:
      break;
    }
  if(found)
    {
    //    fprintf(stderr, "schro_encoder_setting_set_double %s %f\n", key + 4, v);
    schro_encoder_setting_set_double(codec->enc, key + 4, v);
    }
  return 0;
  }

static int flush_data(quicktime_t *file, int track)
  {
  SchroStateEnum  state;
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;
  SchroBuffer *enc_buf;
  int presentation_frame;
  int parse_code;
  int result = 0;
  
  while(1)
    {
    state = schro_encoder_wait(codec->enc);

    switch(state)
      {
      case SCHRO_STATE_HAVE_BUFFER:
      case SCHRO_STATE_END_OF_STREAM:
        enc_buf = schro_encoder_pull(codec->enc, &presentation_frame);
        parse_code = enc_buf->data[4];

        //        fprintf(stderr, "Parse code: %d, state: %d\n",
        //                parse_code, state);
        
        /* Append to enc_buffer */
        if(codec->enc_buffer_alloc < codec->enc_buffer_size + enc_buf->length)
          {
          codec->enc_buffer_alloc = codec->enc_buffer_size + enc_buf->length + 1024;
          codec->enc_buffer = realloc(codec->enc_buffer,
                                      codec->enc_buffer_alloc);
          }
        memcpy(codec->enc_buffer + codec->enc_buffer_size,
               enc_buf->data, enc_buf->length);
        codec->enc_buffer_size += enc_buf->length;
        
        if(SCHRO_PARSE_CODE_IS_PICTURE(parse_code))
          {
          int pic_num, keyframe;
          
          pic_num = (enc_buf->data[13] << 24) |
            (enc_buf->data[14] << 16) |
            (enc_buf->data[15] << 8) |
            (enc_buf->data[16]);

          if(SCHRO_PARSE_CODE_IS_INTRA(parse_code) &&
             SCHRO_PARSE_CODE_IS_REFERENCE(parse_code))
            keyframe = 1;
          else
            keyframe = 0;
            
          /* Write the frame */

          lqt_write_frame_header(file, track,
                                 pic_num, -1, keyframe);

          result = !quicktime_write_data(file, codec->enc_buffer,
                                         codec->enc_buffer_size);

          lqt_write_frame_footer(file, track);
          
          codec->enc_buffer_size = 0;
          
          }
        else if(SCHRO_PARSE_CODE_IS_END_OF_SEQUENCE(parse_code))
          {
          
          if(!codec->enc_eof)
            {
            /* Special case: We need to add a final sample to the stream */
          
            if(vtrack->duration <= vtrack->timestamps[vtrack->current_position-1])
              {
              quicktime_trak_t * trak = vtrack->track;
              quicktime_stts_t * stts = &trak->mdia.minf.stbl.stts;
              lqt_video_append_timestamp(file, track,
                                         vtrack->timestamps[vtrack->current_position-1] +
                                         stts->default_duration, 1);
              }
            else
              lqt_video_append_timestamp(file, track, vtrack->duration, 1);
          
            lqt_write_frame_header(file, track, vtrack->current_position, -1, 0);
            result = !quicktime_write_data(file, codec->enc_buffer,
                                           codec->enc_buffer_size);
            lqt_write_frame_footer(file, track);
            vtrack->current_position++;
            codec->enc_eof = 1;
            }
          else
            lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
                    "Discarding redundant sequence end code");
          
          codec->enc_buffer_size = 0;
          }
        
        schro_buffer_unref (enc_buf);
        if(state == SCHRO_STATE_END_OF_STREAM)
          return result;
        break;
      case SCHRO_STATE_NEED_FRAME:
        return result;
        break;
      case SCHRO_STATE_AGAIN:
        break;
      }
    }
  return result;
  }

static void set_interlacing(quicktime_t * file, int track, SchroVideoFormat * format)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  quicktime_stsd_table_t *stsd;
  
  stsd = vtrack->track->mdia.minf.stbl.stsd.table;

  if(stsd->has_fiel)
    return;
  
  switch(vtrack->interlace_mode)
    {
    case LQT_INTERLACE_NONE:
      lqt_set_fiel(file, track, 1, 0);
      format->interlaced = 0;
      format->top_field_first = 0;
      
      break;
    case LQT_INTERLACE_TOP_FIRST:
      lqt_set_fiel(file, track, 2, 9);
#if 0 /* Don't encode field pictures */
      schro_encoder_setting_set_double(codec->enc,
                                       "interlaced_coding", 1);
#endif
      format->interlaced = 1;
      format->top_field_first = 1;
      break;
    case LQT_INTERLACE_BOTTOM_FIRST:
      lqt_set_fiel(file, track, 2, 14);
#if 0 /* Don't encode field pictures */
      schro_encoder_setting_set_double(codec->enc,
                                       "interlaced_coding", 1);
#endif
      format->interlaced = 1;
      format->top_field_first = 0;
      break;
    }
  }

int lqt_schroedinger_encode_video(quicktime_t *file,
                                  unsigned char **row_pointers,
                                  int track)
  {
  SchroFrame * frame;
  SchroVideoFormat * format;
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;
  
  if(!row_pointers)
    {
    vtrack->stream_cmodel = BC_YUV420P;
    return 0;
    }
  
  if(!codec->enc_copy_frame)
    {
    int pixel_width, pixel_height;
    /* Initialize */
    codec->enc_copy_frame = copy_frame_8;

    format = schro_encoder_get_video_format(codec->enc);

    format->width = quicktime_video_width(file, track);
    format->height = quicktime_video_height(file, track);

    format->clean_width = format->width;
    format->clean_height = format->height;
    format->left_offset = 0;
    format->top_offset = 0;
    
    format->frame_rate_numerator   = lqt_video_time_scale(file, track);
    format->frame_rate_denominator = lqt_frame_duration(file, track, NULL);

    lqt_get_pixel_aspect(file, track, &pixel_width, &pixel_height);
    
    format->aspect_ratio_numerator   = pixel_width;  
    format->aspect_ratio_denominator = pixel_height;

    set_interlacing(file, track, format);
    
    schro_video_format_set_std_signal_range(format,
                                            lqt_schrodinger_get_signal_range(vtrack->stream_cmodel));

    format->chroma_format =
      lqt_schrodinger_get_chroma_format(vtrack->stream_cmodel);

    codec->frame_format = lqt_schrodinger_get_frame_format(format);
    
    schro_encoder_set_video_format(codec->enc, format);

    free(format);
    
    //    schro_debug_set_level(4);
    
    schro_encoder_start(codec->enc);
    }
  
  frame = schro_frame_new_and_alloc(NULL,
                                    codec->frame_format,
                                    quicktime_video_width(file, track),
                                    quicktime_video_height(file, track));

  codec->enc_copy_frame(file, row_pointers, frame, track);

  schro_encoder_push_frame(codec->enc, frame);

  flush_data(file, track);
  
  return 0;
  }

int lqt_schroedinger_flush(quicktime_t *file,
                           int track)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;
  schro_encoder_end_of_stream(codec->enc);
  flush_data(file, track);
  return 0;
  }

static void copy_frame_8(quicktime_t * file,
                         unsigned char **row_pointers,
                         SchroFrame * frame,
                         int track)
  {
  uint8_t * cpy_rows[3];
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  
  cpy_rows[0] = frame->components[0].data;
  cpy_rows[1] = frame->components[1].data;
  cpy_rows[2] = frame->components[2].data;
  
  lqt_rows_copy(cpy_rows, row_pointers,
                quicktime_video_width(file, track),
                quicktime_video_height(file, track),
                vtrack->stream_row_span, vtrack->stream_row_span_uv,
                frame->components[0].stride,
                frame->components[1].stride,
                vtrack->stream_cmodel);
  }
