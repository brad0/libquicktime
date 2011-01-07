/*******************************************************************************
 schroedinger_decode.c

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

static void copy_frame_8(quicktime_t * file,
                         unsigned char **row_pointers,
                         int track);


static int next_startcode(uint8_t * data, int len)
  {
  int ret;
  ret = (data[5]<<24) | (data[6]<<16) | (data[7]<<8) | (data[8]);
  if(!ret)
    ret = 13;
  return ret;
  }

static void
buffer_free (SchroBuffer *buf, void *priv)
  {
  free (priv);
  }

static SchroBuffer * get_data(quicktime_t *file, int track)
  {
  uint8_t * data;
  int size;
  SchroBuffer * ret;
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;

  if(codec->dec_eof)
    return NULL;
  
  if(codec->dec_buffer_size < 13)
    {
    //    fprintf(stderr, "Read frame: %ld + %d\n",
    //            vtrack->current_position, codec->dec_delay);
    
    codec->dec_buffer_size = lqt_read_video_frame(file, &codec->dec_buffer,
                                                  &codec->dec_buffer_alloc,
                                                  vtrack->current_position + codec->dec_delay,
                                                  NULL, track);
    codec->dec_buffer_ptr = codec->dec_buffer;
    }

  //  fprintf(stderr, "dec_buffer_size: %d\n", codec->dec_buffer_size);
  
  if(!codec->dec_buffer_size)
    {
    codec->dec_eof = 1;
    schro_decoder_push_end_of_stream(codec->dec);
    return NULL;
    }
  
  size = next_startcode(codec->dec_buffer_ptr, codec->dec_buffer_size);
  
  if(SCHRO_PARSE_CODE_IS_PICTURE(codec->dec_buffer_ptr[4]))
    {
    codec->dec_delay++;
    //    fprintf(stderr, "** Delay++: %d\n", codec->dec_delay);
    }
  data = malloc(size);
  memcpy(data, codec->dec_buffer_ptr, size);

  //  fprintf(stderr, "Buffer %d\n", size);
  //  lqt_hexdump(data, 128 > size ? size : 128, 16);
  
  ret = schro_buffer_new_with_data(data, size);
  
  ret->free = buffer_free;
  ret->priv = data;
         
  codec->dec_buffer_size -= size;
  codec->dec_buffer_ptr += size;
 
  return ret;
  }

static void get_format(quicktime_t *file, int track)
  {
  SchroVideoFormat * format;

  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;
  quicktime_trak_t *trak = vtrack->track;
  
  format = schro_decoder_get_video_format(codec->dec);

  /* Get colormodel */
  vtrack->stream_cmodel = lqt_schrodinger_get_colormodel(format);

  if((vtrack->stream_cmodel == BC_YUV422P16) ||
     (vtrack->stream_cmodel == BC_YUV444P16))
    {
    
    }
  else
    codec->dec_copy_frame = copy_frame_8;
  
  codec->frame_format = lqt_schrodinger_get_frame_format(format);
  
  /* Get interlace mode */
  if(format->interlaced)
    {
    if(format->top_field_first)
      vtrack->interlace_mode = LQT_INTERLACE_TOP_FIRST;
    else
      vtrack->interlace_mode = LQT_INTERLACE_BOTTOM_FIRST;
    }
  else
    vtrack->interlace_mode = LQT_INTERLACE_NONE;
  
  /* Get pixel aspect */
  trak->mdia.minf.stbl.stsd.table[0].pasp.hSpacing =
    format->aspect_ratio_numerator;
  trak->mdia.minf.stbl.stsd.table[0].pasp.vSpacing =
    format->aspect_ratio_denominator;
  
  free(format);
  }

static int decode_picture(quicktime_t *file, int track)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;
  int state;
  SchroBuffer * buf = NULL;
  SchroFrame * frame = NULL;
  
  while(1)
    {
#if 0
    if(buf)
      {
      state = schro_decoder_push(codec->dec, buf);
      buf = NULL;
      }
    else
      state = schro_decoder_wait(codec->dec);
#endif
    state = schro_decoder_wait(codec->dec);
    
    switch (state)
      {
      case SCHRO_DECODER_FIRST_ACCESS_UNIT:
        //        fprintf(stderr, "State: SCHRO_DECODER_FIRST_ACCESS_UNIT\n");

        get_format(file, track);
        
        // libschroedinger_handle_first_access_unit (avccontext);
        break;

      case SCHRO_DECODER_NEED_BITS:
        /* Need more input data - stop iterating over what we have. */
        //        fprintf(stderr, "State: SCHRO_DECODER_NEED_BITS\n");

        buf = get_data(file, track);
#if 1
        if(buf)
          {
          state = schro_decoder_push(codec->dec, buf);
          if(state == SCHRO_DECODER_FIRST_ACCESS_UNIT)
            {
            //            fprintf(stderr, "State: SCHRO_DECODER_FIRST_ACCESS_UNIT\n");
            get_format(file, track);
            }
          }
#endif
        break;

      case SCHRO_DECODER_NEED_FRAME:
        /* Decoder needs a frame - create one and push it in. */
        //        fprintf(stderr, "State: SCHRO_DECODER_NEED_FRAME\n");
        frame = schro_frame_new_and_alloc(NULL,
                                          codec->frame_format,
                                          quicktime_video_width(file, track),
                                          quicktime_video_height(file, track));
        schro_decoder_add_output_picture (codec->dec, frame);
        //        fprintf(stderr, "Need frame %p\n", frame);
        break;

      case SCHRO_DECODER_OK:
        /* Pull a frame out of the decoder. */
        //        fprintf(stderr, "State: SCHRO_DECODER_OK %d\n",
        //                schro_decoder_get_picture_number(codec->dec));
        
        // if(codec->dec_delay)
        //          {
        codec->dec_frame = schro_decoder_pull(codec->dec);
        return 1;
          //          }
        break;
      case SCHRO_DECODER_EOS:
        //        fprintf(stderr, "State: SCHRO_DECODER_EOS\n");
        // p_schro_params->eos_pulled = 1;
        //        schro_decoder_reset (decoder);
        //        outer = 0;
        return 0;
        break;
      case SCHRO_DECODER_ERROR:
        fprintf(stderr, "State: SCHRO_DECODER_ERROR\n");
        return 0;
        break;
      }
    }
  return 0;
  }

static void init_decode(quicktime_t *file, int track)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;

  /* Create stuff */
  codec->dec = schro_decoder_new();
  vtrack->stream_cmodel = LQT_COLORMODEL_NONE;

  schro_decoder_set_skip_ratio(codec->dec, 1.0);
  }



int lqt_schroedinger_decode_video(quicktime_t *file,
                                  unsigned char **row_pointers,
                                  int track)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;

  if(!codec->dec)
    init_decode(file, track);

  if(!codec->dec_frame && !decode_picture(file, track))
    return 0;
  
  if(!row_pointers)
    return 0;

  if(codec->dec_frame)
    {
    //    fprintf(stderr, "Copy frame %p\n", codec->dec_frame);

    if(!codec->dec_frame->width || !codec->dec_frame->height)
      fprintf(stderr, "Zero size\n");
    else
      codec->dec_copy_frame(file, row_pointers, track);
    
    schro_frame_unref(codec->dec_frame);
    codec->dec_frame = NULL;

    codec->dec_delay--;
    //    fprintf(stderr, "** Delay--: %d\n", codec->dec_delay);
    }
  return 0;
  }

void lqt_schroedinger_resync(quicktime_t *file, int track)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;
  
  schro_decoder_reset(codec->dec);
  codec->dec_delay = 0;
  
  if(codec->dec_frame)
    {
    schro_frame_unref(codec->dec_frame);
    codec->dec_frame = NULL;
    }
  
  
  }

static void copy_frame_8(quicktime_t * file,
                         unsigned char **row_pointers,
                         int track)
  {
  uint8_t * cpy_rows[3];
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  schroedinger_codec_t *codec = vtrack->codec->priv;

  cpy_rows[0] = codec->dec_frame->components[0].data;
  cpy_rows[1] = codec->dec_frame->components[1].data;
  cpy_rows[2] = codec->dec_frame->components[2].data;
  
  lqt_rows_copy(row_pointers, cpy_rows,
                quicktime_video_width(file, track),
                quicktime_video_height(file, track),
                codec->dec_frame->components[0].stride,
                codec->dec_frame->components[1].stride,
                vtrack->stream_row_span, vtrack->stream_row_span_uv,
                vtrack->stream_cmodel);
  
  }
