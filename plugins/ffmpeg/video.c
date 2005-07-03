/* 
 * video.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
 * Based entirely upon qtpng.c from libquicktime 
 * (http://libquicktime.sf.net).
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


#include "config.h"
#include "funcprotos.h"
#include <quicktime/colormodels.h>
#include "ffmpeg.h"

#include <string.h>

/* ffmpeg <-> libquicktime colormodels */

/* Exact entries MUST come first */

static struct
  {
  enum PixelFormat ffmpeg_id;
  int              lqt_id;
  int              exact;
  }
colormodels[] =
  {
    { PIX_FMT_YUV420P,  BC_YUV420P,  1 },  ///< Planar YUV 4:2:0 (1 Cr & Cb sample per 2x2 Y samples)
    { PIX_FMT_YUV422,   BC_YUV422,   1 },
    { PIX_FMT_RGB24,    BC_RGB888,   1 },  ///< Packed pixel, 3 bytes per pixel, RGBRGB...
    { PIX_FMT_BGR24,    BC_BGR888,   1 },  ///< Packed pixel, 3 bytes per pixel, BGRBGR...
    { PIX_FMT_YUV422P,  BC_YUV422P,  1 },  ///< Planar YUV 4:2:2 (1 Cr & Cb sample per 2x1 Y samples)
    { PIX_FMT_YUV444P,  BC_YUV444P,  1 },  ///< Planar YUV 4:4:4 (1 Cr & Cb sample per 1x1 Y samples)
    { PIX_FMT_YUV411P,  BC_YUV411P,  1 },  ///< Planar YUV 4:1:1 (1 Cr & Cb sample per 4x1 Y samples)
    { PIX_FMT_RGB565,   BC_RGB565,   1 },  ///< always stored in cpu endianness
    { PIX_FMT_YUVJ420P, BC_YUVJ420P, 1 }, ///< Planar YUV 4:2:0 full scale (jpeg)
    { PIX_FMT_YUVJ422P, BC_YUVJ422P, 1 }, ///< Planar YUV 4:2:2 full scale (jpeg)
    { PIX_FMT_YUVJ444P, BC_YUVJ444P, 1 }, ///< Planar YUV 4:4:4 full scale (jpeg)
    { PIX_FMT_RGBA32,   BC_RGBA8888, 0 },  ///< Packed pixel, 4 bytes per pixel, BGRABGRA...
    { PIX_FMT_RGB555,   BC_RGB888,   0 },  ///< always stored in cpu endianness, most significant bit to 1
    { PIX_FMT_GRAY8,    BC_RGB888,   0 },
    { PIX_FMT_MONOWHITE, BC_RGB888,  0 },///< 0 is white
    { PIX_FMT_MONOBLACK, BC_RGB888,  0 },///< 0 is black
    { PIX_FMT_PAL8,      BC_RGB888,  0 },    ///< 8 bit with RGBA palette
    { PIX_FMT_YUV410P,   BC_YUV420P, 0 },  ///< Planar YUV 4:1:0 (1 Cr & Cb sample per 4x4 Y samples)
  };

#if 0
/*
 *  We need this only for colormodels, which serve as a replacement for nonsupported
 *  ffmpeg pixel formats
 */

static unsigned char ** alloc_rows(int width, int height, int colormodel)
  {
  unsigned char ** ret = (unsigned char**)0;
  int i;
  switch(colormodel)
    {
    case BC_RGB888:
      ret    = malloc(height * sizeof(*ret));
      ret[0] = malloc(width * height * 3);
      for(i = 1; i < height; i++)
        ret[i] = ret[0] + 3 * i * width;
      break;
    case BC_RGBA8888:
      ret    = malloc(height * sizeof(*ret));
      ret[0] = malloc(width * height * 4);
      for(i = 1; i < height; i++)
        ret[i] = ret[0] + 4 * i * width;
      break;
    case BC_YUV420P:
      ret    = malloc(3 * sizeof(*ret));
      ret[0] = malloc((width * height * 3) / 2);
      ret[1] = ret[0] + width * height;
      ret[2] = ret[1] + (width * height)/4;
      break;
    case BC_YUV422P:
      ret    = malloc(3 * sizeof(*ret));
      ret[0] = malloc((width * height * 2));
      ret[1] = ret[0] + width * height;
      ret[2] = ret[1] + (width * height)/2;
      break;
    case BC_YUV444P:
      ret    = malloc(3 * sizeof(*ret));
      ret[0] = malloc((width * height * 3));
      ret[1] = ret[0] + width * height;
      ret[2] = ret[1] + width * height;
      break;
    }
  return ret;
  }


static void delete_rows(unsigned char ** rows)
  {
  free(rows[0]);
  free(rows);
  }
#endif

int lqt_ffmpeg_delete_video(quicktime_video_map_t *vtrack)
{
	quicktime_ffmpeg_video_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	
	if(codec->com.init_enc)
		avcodec_close(codec->com.ffcodec_enc);
	if(codec->com.init_dec)
		avcodec_close(codec->com.ffcodec_dec);

	if(codec->frame_buffer) free(codec->frame_buffer);
	if(codec->write_buffer) free(codec->write_buffer);
	if(codec->read_buffer) free(codec->read_buffer);
        if(codec->row_pointers) free(codec->row_pointers);

        if(codec->frame) free(codec->frame);
        
	free(codec);
	
	return 0;
}

static void fill_avpicture(AVPicture * ret, unsigned char ** rows, int lqt_colormodel,
                           int row_span, int row_span_uv)
  {
  switch(lqt_colormodel)
    {
    case BC_YUV420P:
    case BC_YUV422P:
      ret->data[0] = rows[0];
      ret->data[1] = rows[1];
      ret->data[2] = rows[2];
      ret->linesize[0] = row_span;
      ret->linesize[1] = row_span_uv ? row_span_uv : row_span/2;
      ret->linesize[2] = row_span_uv ? row_span_uv : row_span/2;
      break;
    case BC_YUV444P:
      ret->data[0] = rows[0];
      ret->data[1] = rows[1];
      ret->data[2] = rows[2];
      ret->linesize[0] = row_span;
      ret->linesize[1] = row_span_uv ? row_span_uv : row_span;
      ret->linesize[2] = row_span_uv ? row_span_uv : row_span;
      break;
    case BC_YUV411P:
      ret->data[0] = rows[0];
      ret->data[1] = rows[1];
      ret->data[2] = rows[2];
      ret->linesize[0] = row_span;
      ret->linesize[1] = row_span_uv ? row_span_uv : row_span/4;
      ret->linesize[2] = row_span_uv ? row_span_uv : row_span/4;
      break;
    case BC_YUV422:
    case BC_RGB888:  ///< Packed pixel, 3 bytes per pixel, RGBRGB...
    case BC_BGR888:  ///< Packed pixel, 3 bytes per pixel, BGRBGR...
    case BC_RGBA8888:  ///< Packed pixel, 4 bytes per pixel, BGRABGRA...
    case BC_RGB565:  ///< always stored in cpu endianness
      ret->data[0]     = rows[0];
      ret->linesize[0] = (int)(rows[1] - rows[0]);
      //      fprintf(stderr, "Linesize: %d\n", ret->linesize[0]);
      break;
    default:
      break;
    }
  }

enum PixelFormat lqt_ffmpeg_get_ffmpeg_colormodel(int id)
  {
  int i;

  for(i = 0; i < sizeof(colormodels)/sizeof(colormodels[0]); i++)
    {
    if(colormodels[i].lqt_id == id)
      return colormodels[i].ffmpeg_id;
    }
  return PIX_FMT_NB;
  }

int lqt_ffmpeg_get_lqt_colormodel(enum PixelFormat id, int * exact)
  {
  int i;

  for(i = 0; i < sizeof(colormodels)/sizeof(colormodels[0]); i++)
    {
    if(colormodels[i].ffmpeg_id == id)
      {
      *exact = colormodels[i].exact;
      return colormodels[i].lqt_id;
      }
    }
  return LQT_COLORMODEL_NONE;
  }



static int read_video_frame(quicktime_t *file, quicktime_ffmpeg_video_codec_t *codec,
                     int64_t frameno, int track)
{
	int i;
	
	quicktime_set_video_position(file, frameno, track);
	i = quicktime_frame_size(file, frameno, track);
	if(i + FF_INPUT_BUFFER_PADDING_SIZE > codec->read_buffer_size)
	{
		codec->read_buffer_size = i + 1024 + FF_INPUT_BUFFER_PADDING_SIZE;
		codec->read_buffer = realloc(codec->read_buffer, codec->read_buffer_size);
		if(!codec->read_buffer)
			return -1;
	}
	if(quicktime_read_data(file, codec->read_buffer, i) < 0)
		return -1;
        codec->read_buffer[i] = 0;
        codec->read_buffer[i+1] = 0;
        codec->read_buffer[i+2] = 0;
        codec->read_buffer[i+3] = 0;
        //        fprintf(stderr, "Read video frame %p\n", codec->read_buffer);
	return i;
}

/* Convert ffmpeg RGBA32 to BC_RGBA888 */

/* From avcodec.h: */

/*
 * PIX_FMT_RGBA32 is handled in an endian-specific manner. A RGBA
 * color is put together as:
 *  (A << 24) | (R << 16) | (G << 8) | B
 * This is stored as BGRA on little endian CPU architectures and ARGB on
 * big endian CPUs.
 */

/* The only question is: WHY? */

static void convert_image_decode_rgba(AVFrame * in_frame,
                                      unsigned char ** out_frame,
                                      int width, int height)
  {
  uint32_t r, g, b; // , a;
  uint32_t * src_ptr;
  uint8_t * dst_ptr;
  int i, j;
  //  fprintf(stderr, "DECODE RGBA %d %d\n", width, height);
  for(i = 0; i < height; i++)
    {
    src_ptr = (uint32_t*)(in_frame->data[0] + i * in_frame->linesize[0]);
    dst_ptr = out_frame[i];
    for(j = 0; j < width; j++)
      {
      //      a = ((*src_ptr) & 0xff000000) >> 24;
      r = ((*src_ptr) & 0x00ff0000) >> 16;
      g = ((*src_ptr) & 0x0000ff00) >> 8;
      b = ((*src_ptr) & 0x000000ff);
      dst_ptr[0] = r;
      dst_ptr[1] = g;
      dst_ptr[2] = b;
      //      dst_ptr[3] = a;
      dst_ptr[3] = 0xff;
      dst_ptr += 4;
      src_ptr++;
      }
    }
  }

/*
 *  Do a conversion from a ffmpeg special colorspace
 *  to a libquicktime special one
 */

static void convert_image_decode(AVFrame * in_frame, enum PixelFormat in_format,
                                 unsigned char ** out_frame, int out_format,
                                 int width, int height, int row_span, int row_span_uv)
  {
  AVPicture in_pic;
  AVPicture out_pic;

  /*
   *  Could someone please tell me, how people can make such a brain dead
   *  RGBA format like in ffmpeg??
   */
  if((in_format == PIX_FMT_RGBA32) && (out_format == BC_RGBA8888))
    {
    convert_image_decode_rgba(in_frame, out_frame, width, height);
    return;
    }
  
  memset(&in_pic,  0, sizeof(in_pic));
  memset(&out_pic, 0, sizeof(out_pic));
  
  in_pic.data[0]      = in_frame->data[0];
  in_pic.data[1]      = in_frame->data[1];
  in_pic.data[2]      = in_frame->data[2];
  in_pic.linesize[0]  = in_frame->linesize[0];
  in_pic.linesize[1]  = in_frame->linesize[1];
  in_pic.linesize[2]  = in_frame->linesize[2];
  
  fill_avpicture(&out_pic, out_frame, out_format, row_span, row_span_uv);
#if 0
  fprintf(stderr, "img_convert %s -> %s\n", 
          avcodec_get_pix_fmt_name(in_format),
          avcodec_get_pix_fmt_name(lqt_ffmpeg_get_ffmpeg_colormodel(out_format)));
#endif
  img_convert(&out_pic, lqt_ffmpeg_get_ffmpeg_colormodel(out_format),
              &in_pic,  in_format,
              width, height);
  
  }

/* Just for the curious: This function can be called with NULL as row_pointers.
   In this case, have_frame is set to 1 and a subsequent call will take the
   alreydy decoded frame. This madness is necessary because sometimes ffmpeg
   doesn't tells us the true colormodel before decoding the first frame */

int lqt_ffmpeg_decode_video(quicktime_t *file, unsigned char **row_pointers,
                            int track)
  {
  int i, imax;
  int result = 0;
  int buffer_size;
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_trak_t *trak = vtrack->track;
  int height;
  int width;
  quicktime_ffmpeg_video_codec_t *codec =
    ((quicktime_codec_t*)vtrack->codec)->priv;
  int got_pic;
  //  int do_cmodel_transfer;
  quicktime_ctab_t * ctab;
  int exact;

  uint8_t * cpy_rows[3];
    
  height = quicktime_video_height(file, track);
  width =  quicktime_video_width(file, track);
  
  /* Initialize decoder */
  
  if(!codec->com.init_dec)
    {
    codec->com.ffcodec_dec                  = avcodec_alloc_context();
    //    fprintf(stderr, "Created codec context %p\n", codec->com.ffcodec_dec);
    codec->com.ffcodec_dec->width           = width;
    codec->com.ffcodec_dec->height          = height;
    codec->com.ffcodec_dec->bits_per_sample = quicktime_video_depth(file, track);
    codec->com.ffcodec_dec->extradata       = trak->mdia.minf.stbl.stsd.table[0].extradata;
    codec->com.ffcodec_dec->extradata_size  = trak->mdia.minf.stbl.stsd.table[0].extradata_size;

    /* Add palette info */
    
    ctab = &(trak->mdia.minf.stbl.stsd.table->ctab);
    if(ctab->size)
      {
      codec->com.ffcodec_dec->palctrl = &(codec->palette);
      codec->palette.palette_changed = 1;
      imax =
        (ctab->size > AVPALETTE_COUNT)
      ? AVPALETTE_COUNT : ctab->size;

      for(i = 0; i < imax; i++)
        {
        codec->palette.palette[i] =
          ((ctab->alpha[i] >> 8) << 24) |
          ((ctab->red[i] >> 8) << 16) |
          ((ctab->green[i] >> 8) << 8) |
          ((ctab->blue[i] >> 8));
        }
      }
    
#define SP(x) codec->com.ffcodec_dec->x = codec->com.params.x
    SP(workaround_bugs);
    SP(error_resilience);
    SP(flags);
#undef SP
    //    codec->com.ffcodec_dec->get_buffer = avcodec_default_get_buffer;
    //    codec->com.ffcodec_dec->release_buffer = avcodec_default_release_buffer;
    
    if(avcodec_open(codec->com.ffcodec_dec, codec->com.ffc_dec) != 0)
      return -1;
    codec->frame = avcodec_alloc_frame();
    vtrack->stream_cmodel = LQT_COLORMODEL_NONE;
    codec->com.init_dec = 1;
    }

  /* Check if we must seek */
  
  if((quicktime_has_keyframes(file, track)) &&
     (vtrack->current_position != codec->last_frame + 1))
    {
    int64_t frame1, frame2 = vtrack->current_position;

    /* Forget about previously decoded frame */
    codec->have_frame = 0;
    
    frame1 = quicktime_get_keyframe_before(file, vtrack->current_position, track);
    if((frame1 < codec->last_frame) &&
       (frame2 > codec->last_frame))
      frame1 = codec->last_frame + 1;
    while(frame1 < frame2)
      {
      buffer_size = read_video_frame(file, codec, frame1, track);
      if(buffer_size > 0)
        {
        avcodec_decode_video(codec->com.ffcodec_dec,
                             codec->frame,
                             &got_pic,
                             codec->read_buffer,
                             buffer_size);
        }
      frame1++;
      }
    vtrack->current_position = frame2;
    }

  codec->last_frame = vtrack->current_position;

  /* Read the frame from file and decode it */

  got_pic = 0;
  
  if(!codec->have_frame)
    {
    while(!got_pic)
      {
      buffer_size = read_video_frame(file, codec,
                                     vtrack->current_position, track);
      
      if(buffer_size <= 0)
        return 0;
#if 0
      fprintf(stderr, "Frame size: %d\n", buffer_size);

      fprintf(stderr, "Decode video...");
#endif 
      if(avcodec_decode_video(codec->com.ffcodec_dec,
                              codec->frame,
                              &got_pic,
                              codec->read_buffer,
                              buffer_size) < 0)
        {
        fprintf(stderr, "Skipping corrupted frame\n");
        continue;
        }
      //      fprintf(stderr, "done\n");
      }
    }
  
  if(vtrack->stream_cmodel == LQT_COLORMODEL_NONE)
    {
    vtrack->stream_cmodel =
      lqt_ffmpeg_get_lqt_colormodel(codec->com.ffcodec_dec->pix_fmt, &exact);
    if(!exact)
      codec->do_imgconvert = 1;
    //    fprintf(stderr, "Detected stream colormodel: %s %s %d\n",
    //            lqt_colormodel_to_string(codec->lqt_colormodel),
    //            avcodec_get_pix_fmt_name(codec->com.ffcodec_dec->pix_fmt), exact);
    }
  
  /*
   * Check for the colormodel
   * There are 4 possible cases:
   *
   *  1. The decoded frame can be memcopied directly to row_pointers
   *     this is the (most likely) case, when the colormodels of
   *     the decoder and the file match and no scaling is required
   *
   *  2. The colormodel of the file is different from the codec,
   *     but we can use cmodel_transfer
   *
   *  3. The decoder colormodel is not supported by libquicktime,
   *     (e.g. YUV410P for sorenson). We must then use avcodec's
   *     image conversion routines to convert to row_pointers.
   *
   *  4. The decoder colormodel is not supported by libquicktime
   *     AND colorspace conversion is reqested. Is this worst case,
   *     we need 2 conversions: The first converts to a colormodel
   *     we support, the second does a cmodel_transfer
   *
   *  P.S. Volunteers are welcome to reduce this madness by supporting some more
   *       colormodels for libquicktime. Look at quicktime4linux source
   */

  if(!row_pointers)
    {
    codec->have_frame = 1;
    return 1;
    }
  
#if 0
  fprintf(stderr, "decode_video: internal %s, in: %s out: %s, row_span: %d\n",
          avcodec_get_pix_fmt_name(codec->com.ffcodec_dec->pix_fmt),
          lqt_colormodel_to_string(codec->lqt_colormodel),
          lqt_colormodel_to_string(file->vtracks[track].color_model),
          row_span);
#endif         
  if(!codec->do_imgconvert)
    {
    cpy_rows[0] = codec->frame->data[0];
    cpy_rows[1] = codec->frame->data[1];
    cpy_rows[2] = codec->frame->data[2];

    lqt_rows_copy(row_pointers, cpy_rows, width, height, codec->frame->linesize[0], codec->frame->linesize[1],
                  file->vtracks[track].stream_row_span, file->vtracks[track].stream_row_span_uv, vtrack->stream_cmodel);
    }
  else
    {
    //    fprintf(stderr, "img_convert %d...", codec->com.ffcodec_dec->pix_fmt);
    convert_image_decode(codec->frame, codec->com.ffcodec_dec->pix_fmt,
                         row_pointers, vtrack->stream_cmodel,
                         width, height, file->vtracks[track].stream_row_span,
                         file->vtracks[track].stream_row_span_uv);
    }
  //  fprintf(stderr, "Done\n"); 
  codec->have_frame = 0;
  return result;
  }

int lqt_ffmpeg_encode_video(quicktime_t *file, unsigned char **row_pointers,
                        int track)
{
	int result = 0;
	int bytes_encoded;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_ffmpeg_video_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
        //	AVPicture pic;
        quicktime_atom_t chunk_atom;

        if(!row_pointers)
          {
          vtrack->stream_cmodel = codec->encode_colormodel;
          return 0;
          }
        
	if(!codec->com.init_enc) {
                codec->com.ffcodec_enc = avcodec_alloc_context();
                codec->frame = avcodec_alloc_frame();
#if LIBAVCODEC_BUILD >= 4754
                codec->com.ffcodec_enc->time_base.den = lqt_video_time_scale(file, track);
                codec->com.ffcodec_enc->time_base.num = lqt_frame_duration(file, track, NULL);
#else
		codec->com.ffcodec_enc->frame_rate =
                  (int)(quicktime_frame_rate(file, track) * (float)DEFAULT_FRAME_RATE_BASE);
                codec->com.ffcodec_enc->frame_rate_base = DEFAULT_FRAME_RATE_BASE;
#endif
		codec->com.ffcodec_enc->width = width;
		codec->com.ffcodec_enc->height = height;
                
                codec->com.ffcodec_enc->pix_fmt = lqt_ffmpeg_get_ffmpeg_colormodel(vtrack->stream_cmodel);
                
#define SP(x) codec->com.ffcodec_enc->x = codec->com.params.x

                /* General options */

                SP(strict_std_compliance);
                SP(sample_aspect_ratio.num);
                SP(sample_aspect_ratio.den);
                SP(flags);
		                
                /* Bitrate options */
                
                SP(bit_rate);
                SP(rc_min_rate);
                SP(rc_max_rate);
                SP(bit_rate_tolerance);
                SP(qcompress);
                SP(qblur);

                /* VBR Options */
                //                SP(qscale);
                SP(qmin);
                SP(qmax);
                SP(mb_qmin);
	        SP(mb_qmax);
	        SP(max_qdiff);
                  
                /* Temporal compression */

                SP(gop_size);
                SP(me_method);
                SP(mb_decision);
                
#undef SP

                if(codec->com.ffcodec_enc->rc_max_rate)
                  codec->com.ffcodec_enc->rc_buffer_size = 128 * 1024;
                if(avcodec_open(codec->com.ffcodec_enc, codec->com.ffc_enc) != 0)
			return -1;
                codec->com.init_enc = 1;
		codec->write_buffer_size = width * height * 4;
		codec->write_buffer = malloc(codec->write_buffer_size);
		if(!codec->write_buffer)
                  return -1;

	}
        //        codec->lqt_colormodel = ffmepg_2_lqt(codec->com.ffcodec_enc);
        
        //        fprintf(stderr, "Encode...\n");
        codec->frame->data[0] = row_pointers[0];
        codec->frame->data[1] = row_pointers[1];
        codec->frame->data[2] = row_pointers[2];
        codec->frame->linesize[0] = vtrack->stream_row_span;
        codec->frame->linesize[1] = vtrack->stream_row_span_uv;
        codec->frame->linesize[2] = vtrack->stream_row_span_uv;
        
        codec->frame->quality = codec->qscale;
	bytes_encoded = avcodec_encode_video(codec->com.ffcodec_enc,
                                             codec->write_buffer,
                                             codec->write_buffer_size,
                                             codec->frame);
        //        fprintf(stderr, "done\n");
        
        quicktime_write_chunk_header(file, trak, &chunk_atom);
        
	result = !quicktime_write_data(file, 
                                       codec->write_buffer, 
                                       bytes_encoded);
        quicktime_write_chunk_footer(file, 
                                     trak, 
                                     vtrack->current_chunk,
                                     &chunk_atom, 
                                     1);

	vtrack->current_chunk++;
        if(codec->com.ffcodec_enc->coded_frame->key_frame)
          quicktime_insert_keyframe(file, vtrack->current_position, track);
        return result;
}

