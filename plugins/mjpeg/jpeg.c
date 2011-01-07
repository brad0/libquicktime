/*******************************************************************************
 jpeg.c

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
#include "libmjpeg.h"
#include <quicktime/colormodels.h>
#include <stdlib.h>
#include <string.h>
#include "jpeg.h"

#define LOG_DOMAIN "mjpeg"

// Jpeg types
#define JPEG_PROGRESSIVE 0
#define JPEG_MJPA 1

/* MJPB isn't supported anyway */
// #define JPEG_MJPB 2 

typedef struct
  {
  unsigned char *buffer;
  int buffer_alloc;
  mjpeg_t *mjpeg;
  int jpeg_type;
  unsigned char *temp_video;
  
  int have_frame;
  int initialized;

  int quality;
  int usefloat;
  } quicktime_jpeg_codec_t;

static int delete_codec(quicktime_codec_t *codec_base)
  {
  quicktime_jpeg_codec_t *codec = codec_base->priv;
  if(codec->mjpeg)
    mjpeg_delete(codec->mjpeg);
  if(codec->buffer)
    free(codec->buffer);
  if(codec->temp_video)
    free(codec->temp_video);
  free(codec);
  return 0;
  }

static int decode(quicktime_t *file, 
                  unsigned char **row_pointers, 
                  int track)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  quicktime_jpeg_codec_t *codec = vtrack->codec->priv;
  mjpeg_t *mjpeg;
  long size, field2_offset;
  int result = 0;

  if(!codec->initialized)
    {
    /* Special treatment of bottom_field_first.
       We cannot use lqt_get_interlace_mode at this point,
       since this piece is called *before* the interlace mode
       is set by quicktime_init_maps() */
    int nfields = 0, dominance = 0;
    if(!lqt_get_fiel(file, track, &nfields, &dominance))
      nfields = 1;
    codec->mjpeg = mjpeg_new(quicktime_video_width(file, track),
                             quicktime_video_height(file, track),
                             nfields, LQT_COLORMODEL_NONE);
    if((nfields == 2) && (dominance == 6))
      codec->mjpeg->bottom_first = 1;

    codec->initialized = 1;
    }
  
  mjpeg = codec->mjpeg;
   
  if(!codec->have_frame)
    {
    size = lqt_read_video_frame(file, &codec->buffer, &codec->buffer_alloc,
                                vtrack->current_position, NULL, track);
    
    if(size <= 0)
      return -1;
    
    if(mjpeg_get_fields(mjpeg) == 2)
      {
      field2_offset = mjpeg_get_quicktime_field2(codec->buffer, size);
      }
    else
      field2_offset = 0;
    
    mjpeg_decompress(codec->mjpeg, 
                     codec->buffer, 
                     size,
                     field2_offset);
    
    if(!row_pointers)
      {
      /* Detect colormodel and return */
      vtrack->stream_cmodel = mjpeg->jpeg_color_model;
      codec->have_frame = 1;

      /* Set compression info */
      if(file->file_type & (LQT_FILE_QT | LQT_FILE_QT_OLD))
        vtrack->ci.id = LQT_COMPRESSION_JPEG;
      return 0;
      }
    }

  if(file->vtracks[track].stream_row_span) 
    mjpeg_set_rowspan(codec->mjpeg, file->vtracks[track].stream_row_span,
                      file->vtracks[track].stream_row_span_uv);
  else
    mjpeg_set_rowspan(codec->mjpeg, 0, 0);
    
  mjpeg_get_frame(codec->mjpeg, row_pointers);
  codec->have_frame = 0;
  
  return result;
  }

static int writes_compressed(lqt_file_type_t type, const lqt_compression_info_t * ci)
  {
  if((ci->colormodel == BC_YUVJ444P) ||
     (ci->colormodel == BC_YUVJ422P) ||
     (ci->colormodel == BC_YUVJ420P))
    return 1;
  else
    return 0;
  }

  
static void resync(quicktime_t *file, int track)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  quicktime_jpeg_codec_t *codec = vtrack->codec->priv;
  codec->have_frame = 0;
  }
     
static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  quicktime_jpeg_codec_t *codec = vtrack->codec->priv;
  quicktime_trak_t *trak = vtrack->track;
  int result = 0;
  long field2_offset;

  if(!row_pointers)
    {
    if(codec->jpeg_type != JPEG_PROGRESSIVE)
      vtrack->stream_cmodel = BC_YUVJ422P;
    else
      vtrack->stream_cmodel = BC_YUVJ420P;
      
    return 0;
    }
        
  if(!codec->initialized)
    {
    /* Quicktime for Windows must have this information. */
    if((codec->jpeg_type == JPEG_MJPA) &&
       !trak->mdia.minf.stbl.stsd.table[0].has_fiel)
      {
      switch(vtrack->interlace_mode)
        {
        case LQT_INTERLACE_TOP_FIRST:
          lqt_set_fiel(file, track, 2, 1);
          break;
        case LQT_INTERLACE_BOTTOM_FIRST:
          lqt_set_fiel(file, track, 2, 6);
          break;
        case LQT_INTERLACE_NONE:
          lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
                  "Encoding progressive video as interlaced. Motion JPEG-A is not suitable for progressive video.");
          lqt_set_fiel(file, track, 2, 1);
          break;
                
        }
      }
    codec->mjpeg = mjpeg_new(quicktime_video_width(file, track),
                             quicktime_video_height(file, track),
                             (codec->jpeg_type == JPEG_MJPA) ? 2 : 1, vtrack->stream_cmodel);
    // Bottom first needs special treatment */
    if(vtrack->interlace_mode == LQT_INTERLACE_BOTTOM_FIRST)
      codec->mjpeg->bottom_first = 1;
          
    mjpeg_set_quality(codec->mjpeg, codec->quality);
    mjpeg_set_float(codec->mjpeg, codec->usefloat);
    codec->initialized = 1;
    }
        
  if(file->vtracks[track].stream_row_span) 
    mjpeg_set_rowspan(codec->mjpeg, file->vtracks[track].stream_row_span, file->vtracks[track].stream_row_span_uv);
  else
    mjpeg_set_rowspan(codec->mjpeg, 0, 0);
                
  mjpeg_compress(codec->mjpeg, row_pointers);

  if(codec->jpeg_type == JPEG_MJPA) 
    mjpeg_insert_quicktime_markers(&codec->mjpeg->output_data,
                                   &codec->mjpeg->output_size,
                                   &codec->mjpeg->output_allocated,
                                   2,
                                   &field2_offset);

  lqt_write_frame_header(file, track,
                         vtrack->current_position,
                         -1, 0);
        
  result = !quicktime_write_data(file, 
                                 mjpeg_output_buffer(codec->mjpeg), 
                                 mjpeg_output_size(codec->mjpeg));

  lqt_write_frame_footer(file, track);
  return result;
  }


static int set_parameter(quicktime_t *file, 
                         int track, 
                         const char *key, 
                         const void *value)
  {
  quicktime_jpeg_codec_t *codec = file->vtracks[track].codec->priv;
	
  if(!strcasecmp(key, "jpeg_quality"))
    {
    codec->quality = *(int*)value;
    }
  else if(!strcasecmp(key, "jpeg_usefloat"))
    {
    codec->usefloat = *(int*)value;
    }
  return 0;
  }

void quicktime_init_codec_jpeg(quicktime_codec_t * codec_base,
                               quicktime_audio_map_t *atrack,
                               quicktime_video_map_t *vtrack)
  {
  char *compressor;
  quicktime_jpeg_codec_t *codec;
  
  /* Init public items */
  codec = calloc(1, sizeof(*codec));
  
  codec_base->priv = codec;
  codec_base->delete_codec = delete_codec;
  codec_base->decode_video = decode;
  codec_base->encode_video = encode;
  codec_base->set_parameter = set_parameter;
  codec_base->resync = resync;
  codec_base->writes_compressed = writes_compressed;
  
  /* Init private items */
  codec->quality = 80;
  codec->usefloat = 0;

  if(!vtrack)
    return;

  compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
  
  if(quicktime_match_32(compressor, QUICKTIME_JPEG))
    codec->jpeg_type = JPEG_PROGRESSIVE;
  else if(quicktime_match_32(compressor, QUICKTIME_MJPA))
    codec->jpeg_type = JPEG_MJPA;
  }
