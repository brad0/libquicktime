/*******************************************************************************
 schroedinger_encode.c

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
#include "schroedinger.h"

#include <string.h>

static void copy_frame_8(quicktime_t * file,
                         unsigned char **row_pointers,
                         SchroFrame * frame,
                         int track);

static int flush_data(quicktime_t *file, int track)
  {
  SchroStateEnum  state;
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  schroedinger_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  SchroBuffer *enc_buf;
  int presentation_frame;
  int parse_code;
  quicktime_atom_t chunk_atom;
  quicktime_trak_t *trak = vtrack->track;
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
          /* Write the frame */
          quicktime_write_chunk_header(file, trak, &chunk_atom);
          result = !quicktime_write_data(file, codec->enc_buffer,
                                         codec->enc_buffer_size);

          quicktime_write_chunk_footer(file,
                                       trak,
                                       vtrack->current_chunk,
                                       &chunk_atom,
                                       1);
          vtrack->current_chunk++;
          }
        schro_buffer_unref (enc_buf);
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

int lqt_schroedinger_encode_video(quicktime_t *file,
                                  unsigned char **row_pointers,
                                  int track)
  {
  SchroFrame * frame;
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  schroedinger_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  
  if(!row_pointers)
    {
    
    }
  
  if(!codec->enc_copy_frame)
    {
    /* Initialize */
    codec->enc_copy_frame = copy_frame_8;
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
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  schroedinger_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
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
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  
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
