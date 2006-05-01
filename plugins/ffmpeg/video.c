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

typedef struct
  {
  AVCodecContext * avctx;
  AVCodec * encoder;
  AVCodec * decoder;
  int initialized;
  

  unsigned char * buffer;
  int buffer_size;
  
  int64_t last_frame;

  AVFrame * frame;
  uint8_t * frame_buffer;

  /* Colormodel */

  int do_imgconvert;
    
  //  unsigned char ** tmp_buffer;
  unsigned char ** row_pointers;
    
  /* Quality must be passed to the individual frame */

  int qscale;

  AVPaletteControl palette;

  /* We decode the first frame during the init() function to
     obtain the stream colormodel */

  int have_frame;

  int encode_colormodel;

  int write_global_header;
  int global_header_written;

  uint8_t * extradata; /* For decoding only, for encoding extradata is owned by lavc */

  /* Multipass control */
  int total_passes;
  int pass;
  char * stats_filename;
  FILE * stats_file;
  
  } quicktime_ffmpeg_video_codec_t;

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

static int lqt_ffmpeg_delete_video(quicktime_video_map_t *vtrack)
{
	quicktime_ffmpeg_video_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
          
        if(codec->extradata)
          free(codec->extradata);

        if(codec->stats_filename)
          free(codec->stats_filename);
        
        if(codec->stats_file)
          fclose(codec->stats_file);
        
	if(codec->initialized)
          {
          if(codec->avctx->stats_in)
            free(codec->avctx->stats_in);
          avcodec_close(codec->avctx);
          }
        else
          free(codec->avctx);
        
        if(codec->frame_buffer) free(codec->frame_buffer);
	if(codec->buffer) free(codec->buffer);
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

static enum PixelFormat lqt_ffmpeg_get_ffmpeg_colormodel(int id)
  {
  int i;

  for(i = 0; i < sizeof(colormodels)/sizeof(colormodels[0]); i++)
    {
    if(colormodels[i].lqt_id == id)
      return colormodels[i].ffmpeg_id;
    }
  return PIX_FMT_NB;
  }

static int lqt_ffmpeg_get_lqt_colormodel(enum PixelFormat id, int * exact)
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
	if(i + FF_INPUT_BUFFER_PADDING_SIZE > codec->buffer_size)
	{
		codec->buffer_size = i + 1024 + FF_INPUT_BUFFER_PADDING_SIZE;
		codec->buffer = realloc(codec->buffer, codec->buffer_size);
		if(!codec->buffer)
			return -1;
	}
	if(quicktime_read_data(file, codec->buffer, i) < 0)
		return -1;
        codec->buffer[i] = 0;
        codec->buffer[i+1] = 0;
        codec->buffer[i+2] = 0;
        codec->buffer[i+3] = 0;
        //        fprintf(stderr, "Read video frame %p\n", codec->buffer);
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

static int lqt_ffmpeg_decode_video(quicktime_t *file, unsigned char **row_pointers,
                            int track)
  {
  uint8_t * user_atom;
  uint32_t user_atom_len;
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
  int exact = 0;
  
  uint8_t * extradata = (uint8_t*)0;
  int extradata_size = 0;
  
  uint8_t * cpy_rows[3];

  //  fprintf(stderr, "decode video\n");
  
  height = quicktime_video_height(file, track);
  width =  quicktime_video_width(file, track);
  
  /* Initialize decoder */
  
  if(!codec->initialized)
    {
    codec->avctx->width           = width;
    codec->avctx->height          = height;
    codec->avctx->bits_per_sample = quicktime_video_depth(file, track);

    /* Set extradata: It's done differently for each codec */

    if(codec->decoder->id == CODEC_ID_SVQ3)
      {
      extradata       = trak->mdia.minf.stbl.stsd.table[0].table_raw + 4;
      extradata_size  = trak->mdia.minf.stbl.stsd.table[0].table_raw_size - 4;
      
      //      fprintf(stderr, "Extradata: %d bytes\n", codec->avctx->extradata_size);
      //      lqt_hexdump(codec->avctx->extradata, codec->avctx->extradata_size, 16);
      }
    else if(codec->decoder->id == CODEC_ID_H264)
      {
      user_atom = quicktime_stsd_get_user_atom(trak, "avcC", &user_atom_len);

      if(!user_atom)
        fprintf(stderr, "No avcC atom present, decoding is likely to fail\n");
      else
        {
        extradata = user_atom + 8;
        extradata_size = user_atom_len - 8;
        }
      //      fprintf(stderr, "Got avcc atom\n");
      //      lqt_hexdump(extradata,
      //                  extradata_size, 16);
      
      }
    else if(codec->decoder->id == CODEC_ID_MPEG4)
      {
      if(trak->mdia.minf.stbl.stsd.table[0].has_esds)
        {
        //        fprintf(stderr, "Setting MPEG-4 extradata %d bytes\n", 
        //                trak->mdia.minf.stbl.stsd.table[0].esds.decoderConfigLen);
        
        extradata = trak->mdia.minf.stbl.stsd.table[0].esds.decoderConfig;
        extradata_size = trak->mdia.minf.stbl.stsd.table[0].esds.decoderConfigLen;
        }
#if 0 /* Hope, that the global header comes with the I-frames */
      else
        {
        fprintf(stderr, "No MPEG-4 extradata!!\n");
        }
#endif
      }

    if(extradata)
      {
      codec->extradata = calloc(1, extradata_size + FF_INPUT_BUFFER_PADDING_SIZE);
      memcpy(codec->extradata, extradata, extradata_size);
      codec->avctx->extradata_size = extradata_size;
      codec->avctx->extradata = codec->extradata;
      }
    /* Add palette info */
    
    ctab = &(trak->mdia.minf.stbl.stsd.table->ctab);
    if(ctab->size)
      {
      codec->avctx->palctrl = &(codec->palette);
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
    
    //    codec->avctx->get_buffer = avcodec_default_get_buffer;
    //    codec->avctx->release_buffer = avcodec_default_release_buffer;
    
    if(avcodec_open(codec->avctx, codec->decoder) != 0)
      return -1;
    codec->frame = avcodec_alloc_frame();
    vtrack->stream_cmodel = LQT_COLORMODEL_NONE;
    codec->initialized = 1;
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
        avcodec_decode_video(codec->avctx,
                             codec->frame,
                             &got_pic,
                             codec->buffer,
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
      fprintf(stderr, "Decode video...");
      fprintf(stderr, "Frame size: %d\n", buffer_size);
      lqt_hexdump(codec->buffer, 16, 16);
      
#endif 
      if(avcodec_decode_video(codec->avctx,
                              codec->frame,
                              &got_pic,
                              codec->buffer,
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
      lqt_ffmpeg_get_lqt_colormodel(codec->avctx->pix_fmt, &exact);
    if(!exact)
      codec->do_imgconvert = 1;
    //    fprintf(stderr, "Detected stream colormodel: %s %s %d\n",
    //            lqt_colormodel_to_string(codec->lqt_colormodel),
    //            avcodec_get_pix_fmt_name(codec->avctx->pix_fmt), exact);

    if(codec->decoder->id == CODEC_ID_DVVIDEO)
      {
      if(vtrack->stream_cmodel == BC_YUV420P)
        vtrack->chroma_placement = LQT_CHROMA_PLACEMENT_DVPAL;
      vtrack->interlace_mode = LQT_INTERLACE_BOTTOM_FIRST;
      }

    if(codec->avctx->sample_aspect_ratio.num)
      {
      trak->mdia.minf.stbl.stsd.table[0].pasp.hSpacing = codec->avctx->sample_aspect_ratio.num;
      trak->mdia.minf.stbl.stsd.table[0].pasp.vSpacing = codec->avctx->sample_aspect_ratio.den;
      }
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
  
  if(!codec->do_imgconvert)
    {
    cpy_rows[0] = codec->frame->data[0];
    cpy_rows[1] = codec->frame->data[1];
    cpy_rows[2] = codec->frame->data[2];

    lqt_rows_copy(row_pointers, cpy_rows, width, height,
                  codec->frame->linesize[0], codec->frame->linesize[1],
                  vtrack->stream_row_span, vtrack->stream_row_span_uv,
                  vtrack->stream_cmodel);
    }
  else
    {
    //    fprintf(stderr, "img_convert %d...", codec->avctx->pix_fmt);
    convert_image_decode(codec->frame, codec->avctx->pix_fmt,
                         row_pointers, vtrack->stream_cmodel,
                         width, height, vtrack->stream_row_span,
                         vtrack->stream_row_span_uv);
    }
  //  fprintf(stderr, "Done\n"); 
  codec->have_frame = 0;
  return result;
  }

static int set_pass_ffmpeg(quicktime_t *file, 
                           int track, int pass, int total_passes,
                           const char * stats_file)
  {
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_ffmpeg_video_codec_t *codec =
    ((quicktime_codec_t*)vtrack->codec)->priv;

  codec->total_passes = total_passes;
  codec->pass = pass;
  codec->stats_filename = malloc(strlen(stats_file)+1);
  strcpy(codec->stats_filename, stats_file);
  
  fprintf(stderr, "set_pass_ffmpeg %d %d %s\n", pass, total_passes, stats_file);
  return 1;
  }

static int lqt_ffmpeg_encode_video(quicktime_t *file, unsigned char **row_pointers,
                            int track)
{
        quicktime_esds_t * esds;
        int result = 0;
        int pixel_width, pixel_height;
	int bytes_encoded;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_ffmpeg_video_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
        quicktime_atom_t chunk_atom;
        int stats_len;
        if(!row_pointers)
          {
          vtrack->stream_cmodel = codec->encode_colormodel;
          return 0;
          }
        
	if(!codec->initialized)
          {
          codec->frame = avcodec_alloc_frame();

          /* time_base is 1/framerate for constant framerate */
          
          codec->avctx->time_base.den = lqt_video_time_scale(file, track);
          codec->avctx->time_base.num = lqt_frame_duration(file, track, NULL);

          //          codec->avctx->time_base.den = 1;
          //          codec->avctx->time_base.num = lqt_video_time_scale(file, track);

          if(codec->avctx->flags & CODEC_FLAG_QSCALE)
            codec->avctx->global_quality = codec->qscale;
          
          //          fprintf(stderr, "Init %d %d\n", width, height);
          
          
          codec->avctx->width = width;
          codec->avctx->height = height;
                
          codec->avctx->pix_fmt = lqt_ffmpeg_get_ffmpeg_colormodel(vtrack->stream_cmodel);

          lqt_get_pixel_aspect(file, track, &pixel_width, &pixel_height);
          codec->avctx->sample_aspect_ratio.num = pixel_width;
          codec->avctx->sample_aspect_ratio.den = pixel_height;
          /* Use global headers for mp4v */
          if(codec->encoder->id == CODEC_ID_MPEG4)
            {
            codec->avctx->flags |= CODEC_FLAG_GLOBAL_HEADER;
            codec->write_global_header = 1;
            }

          /* Initialize 2-pass */
          if(codec->total_passes)
            {
            if(codec->pass == 1)
              {
              codec->stats_file = fopen(codec->stats_filename, "w");
              codec->avctx->flags |= CODEC_FLAG_PASS1;
              }
            else if(codec->pass == codec->total_passes)
              {
              codec->stats_file = fopen(codec->stats_filename, "r");
              fseek(codec->stats_file, 0, SEEK_END);
              stats_len = ftell(codec->stats_file);
              fseek(codec->stats_file, 0, SEEK_SET);

              codec->avctx->stats_in = av_malloc(stats_len + 1);
              fread(codec->avctx->stats_in, stats_len, 1, codec->stats_file);
              codec->avctx->stats_in[stats_len] = '\0';

              fclose(codec->stats_file);
              codec->stats_file = (FILE*)0;
              
              codec->avctx->flags |= CODEC_FLAG_PASS2;
              }
            }
          /* Open codec */
          if(avcodec_open(codec->avctx, codec->encoder) != 0)
            return -1;
          codec->buffer_size = width * height * 4 + 1024*256;
          codec->buffer = malloc(codec->buffer_size);
          if(!codec->buffer)
            return -1;

          if(codec->avctx->max_b_frames > 0)
            vtrack->has_b_frames = 1;
          
          codec->initialized = 1;

          
          
	}
        //        codec->lqt_colormodel = ffmepg_2_lqt(codec->com.ffcodec_enc);
        
        //        fprintf(stderr, "Encode, PTS: %lld...\n", vtrack->timestamp);
        codec->frame->data[0] = row_pointers[0];
        codec->frame->data[1] = row_pointers[1];
        codec->frame->data[2] = row_pointers[2];
        codec->frame->linesize[0] = vtrack->stream_row_span;
        codec->frame->linesize[1] = vtrack->stream_row_span_uv;
        codec->frame->linesize[2] = vtrack->stream_row_span_uv;
        codec->frame->pts = vtrack->timestamp;
        if(codec->avctx->flags & CODEC_FLAG_QSCALE)
          codec->frame->quality = codec->qscale;
        //        fprintf(stderr, "Encode %p, %d\n", codec->buffer,
        //                codec->buffer_size);
        
	bytes_encoded = avcodec_encode_video(codec->avctx,
                                             codec->buffer,
                                             codec->buffer_size,
                                             codec->frame);
#if 0
        fprintf(stderr, "Encoded %d bytes, ", bytes_encoded);
        switch(codec->avctx->coded_frame->pict_type)
          {
          case FF_I_TYPE:
            fprintf(stderr, "pict_type: I, ");
            break;
          case FF_P_TYPE:
            fprintf(stderr, "pict_type: P, ");
            break;
          case FF_B_TYPE:
            fprintf(stderr, "pict_type: B, ");
            break;
          }
        fprintf(stderr, "PTS: %lld\n", codec->avctx->coded_frame->pts);
#endif

        if(bytes_encoded)
          {
          vtrack->coded_timestamp = codec->avctx->coded_frame->pts;
          quicktime_write_chunk_header(file, trak, &chunk_atom);
          
          result = !quicktime_write_data(file, 
                                         codec->buffer, 
                                         bytes_encoded);
          quicktime_write_chunk_footer(file, 
                                       trak, 
                                       vtrack->current_chunk,
                                       &chunk_atom, 
                                       1);
          
          if(codec->avctx->coded_frame->key_frame)
            quicktime_insert_keyframe(file, vtrack->current_chunk-1, track);
          vtrack->current_chunk++;

          /* Write stats */
          
          if((codec->pass == 1) && codec->avctx->stats_out && codec->stats_file)
            fprintf(codec->stats_file, codec->avctx->stats_out);
          }
        
        /* Check whether to write the global header */
        
        if(codec->write_global_header && !codec->global_header_written)
          {
          esds = quicktime_set_esds(trak,
                                    codec->avctx->extradata,
                                    codec->avctx->extradata_size);
          //          fprintf(stderr, "Setting MPEG-4 extradata\n");
          //          lqt_hexdump(codec->avctx->extradata, codec->avctx->extradata_size,
          //                      16);
          
          esds->version         = 0;
          esds->flags           = 0;
          
          esds->esid            = 0;
          esds->stream_priority = 0;
          
          esds->objectTypeId    = 32; /* MPEG-4 video */
          esds->streamType      = 0x11; /* from qt4l and CDR_Dinner_350k-994.mp4 */
          esds->bufferSizeDB    = 64000; /* Hopefully not important :) */
          
          /* Maybe correct these later? */
          esds->maxBitrate      = 200000;
          esds->avgBitrate      = 200000;
          codec->global_header_written = 1;
          }

        return result;
}

static int flush(quicktime_t *file, int track)
  {
        int result = 0;
	int bytes_encoded;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_ffmpeg_video_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
        quicktime_atom_t chunk_atom;

        /* Do nothing if we didn't encode anything yet */
        if(!codec->initialized)
          return 0;
        
	bytes_encoded = avcodec_encode_video(codec->avctx,
                                             codec->buffer,
                                             codec->buffer_size,
                                             (AVFrame*)0);
#if 0
        fprintf(stderr, "Flush: encoded %d bytes, ", bytes_encoded);
        switch(codec->avctx->coded_frame->pict_type)
          {
          case FF_I_TYPE:
            fprintf(stderr, "pict_type: I, ");
            break;
          case FF_P_TYPE:
            fprintf(stderr, "pict_type: P, ");
            break;
          case FF_B_TYPE:
            fprintf(stderr, "pict_type: B, ");
            break;
          }
        fprintf(stderr, "PTS: %lld\n", codec->avctx->coded_frame->pts);
#endif
        vtrack->coded_timestamp = codec->avctx->coded_frame->pts;

        if(bytes_encoded)
          {
          quicktime_write_chunk_header(file, trak, &chunk_atom);
          
          result = !quicktime_write_data(file, 
                                         codec->buffer, 
                                         bytes_encoded);
          quicktime_write_chunk_footer(file, 
                                       trak, 
                                       vtrack->current_chunk,
                                       &chunk_atom, 
                                       1);
          
          if(codec->avctx->coded_frame->key_frame)
            quicktime_insert_keyframe(file, vtrack->current_chunk-1, track);
          vtrack->current_chunk++;

          if((codec->pass == 1) && codec->avctx->stats_out && codec->stats_file)
            fprintf(codec->stats_file, codec->avctx->stats_out);
          
          return 1;
          }
        return 0;
        
  }

static int set_parameter_video(quicktime_t *file, 
                               int track, 
                               char *key, 
                               void *value)
  {
  quicktime_ffmpeg_video_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;
#if 1
  if(!strcasecmp(key, "ff_qscale"))
    {
    codec->qscale = *(int*)(value) * FF_QP2LAMBDA;
    return 0;
    }

  lqt_ffmpeg_set_parameter(codec->avctx, key, value);
#endif
  return 0;
  }

void quicktime_init_video_codec_ffmpeg(quicktime_video_map_t *vtrack, AVCodec *encoder,
                                       AVCodec *decoder)
{
	quicktime_ffmpeg_video_codec_t *codec;

	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
        
	avcodec_init();

	codec = calloc(1, sizeof(*codec));
	if(!codec)
	  return;
        codec->avctx = avcodec_alloc_context();
        
	if(quicktime_match_32(compressor, "dvc "))
	  codec->encode_colormodel = BC_YUV411P;
        else if(quicktime_match_32(compressor, "MJPG"))
          codec->encode_colormodel = BC_YUVJ420P;
        //        else if(quicktime_match_32(compressor, "dvcp"))
        //          codec->encode_colormodel = BC_YUV411P;
        else
          codec->encode_colormodel = BC_YUV420P;
        
	codec->encoder = encoder;
	codec->decoder = decoder;
        
	((quicktime_codec_t*)vtrack->codec)->priv = (void *)codec;
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = lqt_ffmpeg_delete_video;
	((quicktime_codec_t*)vtrack->codec)->flush = flush;

        if(encoder)
          {
          ((quicktime_codec_t*)vtrack->codec)->encode_video = lqt_ffmpeg_encode_video;
          ((quicktime_codec_t*)vtrack->codec)->set_pass = set_pass_ffmpeg;
          }
	if(decoder)
          ((quicktime_codec_t*)vtrack->codec)->decode_video = lqt_ffmpeg_decode_video;

        ((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter_video;
}

