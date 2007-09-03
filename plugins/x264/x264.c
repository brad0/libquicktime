/*******************************************************************************
 x264.c

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
#include <x264.h>
#include <stdlib.h>
#include <string.h>
#include "colormodels.h"

#define LOG_DOMAIN "x264"

// #define DUMP_CONFIG
#ifdef DUMP_CONFIG


static void dump_params(x264_param_t * params)
  {
  lqt_dump("X264 params:\n");
  lqt_dump("  cpu:       %08x\n", params->cpu);
  lqt_dump("  i_threads: %d\n", params->i_threads);
  lqt_dump("  i_width: %d\n", params->i_width);
  lqt_dump("  i_height: %d\n", params->i_height);
  lqt_dump("  i_csp: %d\n", params->i_csp);
  lqt_dump("  i_level_idc: %d\n", params->i_level_idc);
  lqt_dump("  i_frame_total: %d\n", params->i_frame_total);
  lqt_dump("  vui:\n");
  lqt_dump("    i_sar_height: %d\n", params->vui.i_sar_height);
  lqt_dump("    i_sar_width:  %d\n", params->vui.i_sar_width);
  lqt_dump("    i_overscan:   %d\n", params->vui.i_overscan);
  lqt_dump("    i_vidformat:  %d\n", params->vui.i_vidformat);
  lqt_dump("    b_fullrange:  %d\n", params->vui.b_fullrange);
  lqt_dump("    i_colorprim:  %d\n", params->vui.i_colorprim);
  lqt_dump("    i_transfer:   %d\n", params->vui.i_transfer);
  lqt_dump("    i_colmatrix:  %d\n", params->vui.i_colmatrix);
  lqt_dump("    i_chroma_loc: %d\n", params->vui.i_chroma_loc);

  lqt_dump("  fps: %d:%d\n", params->i_fps_num, params->i_fps_den);

  /* Bitstream parameters */
  
  lqt_dump("  i_frame_reference:           %d\n", params->i_frame_reference); // 1..16 
  lqt_dump("  i_keyint_min:                %d\n", params->i_keyint_min);      
  lqt_dump("  i_keyint_max:                %d\n", params->i_keyint_max);
  lqt_dump("  i_scenecut_threshold:        %d\n", params->i_scenecut_threshold); 
  lqt_dump("  i_bframe:                    %d\n", params->i_bframe);          // 0.. X264_BFRAME_MAX
  lqt_dump("  b_bframe_adaptive:           %d\n", params->b_bframe_adaptive);
  lqt_dump("  i_bframe_bias:               %d\n", params->i_bframe_bias);
  lqt_dump("  b_bframe_pyramid:            %d\n", params->b_bframe_pyramid);
  
  lqt_dump("  b_deblocking_filter:         %d\n", params->b_deblocking_filter);
  lqt_dump("  i_deblocking_filter_alphac0: %d\n", params->i_deblocking_filter_alphac0); // -6..6
  lqt_dump("  i_deblocking_filter_beta:    %d\n", params->i_deblocking_filter_beta);    // -6..6

  lqt_dump("  b_cabac:                     %d\n", params->b_cabac);
  lqt_dump("  i_cabac_init_idc:            %d\n", params->i_cabac_init_idc); // 0..2

  lqt_dump("  i_cqm_preset:                %d\n", params->i_cqm_preset);  
  lqt_dump("  psz_cqm_file:                %s\n", params->psz_cqm_file);

  lqt_dump("  Analyze:\n");

  // ORED value of X264_ANALYSE_I4x4, X264_ANALYSE_I8x8, X264_ANALYSE_PSUB16x16, X264_ANALYSE_PSUB8x8,
  // X264_ANALYSE_BSUB16x16
  
  lqt_dump("    intra:              %d\n", params->analyse.intra);
  lqt_dump("    inter:              %d\n", params->analyse.inter);
  
  
  lqt_dump("    b_transform_8x8:    %d\n", params->analyse.b_transform_8x8);
  lqt_dump("    b_weighted_bipred:  %d\n", params->analyse.b_weighted_bipred);
  lqt_dump("    i_direct_mv_pred:   %d\n", params->analyse.i_direct_mv_pred);
  lqt_dump("    i_chroma_qp_offset: %d\n", params->analyse.i_chroma_qp_offset);

  // X264_ME_DIA, X264_ME_HEX, X264_ME_UMH, X264_ME_ESA
  lqt_dump("    i_me_method:        %d\n", params->analyse.i_me_method); 

  
  lqt_dump("    i_me_range:         %d\n", params->analyse.i_me_range);
  lqt_dump("    i_mv_range:         %d\n", params->analyse.i_mv_range);
  lqt_dump("    i_subpel_refine:    %d\n", params->analyse.i_subpel_refine); // 1..7
  lqt_dump("    b_bidir_me:         %d\n", params->analyse.b_bidir_me);
  lqt_dump("    b_chroma_me:        %d\n", params->analyse.b_chroma_me);
  lqt_dump("    b_bframe_rdo:       %d\n", params->analyse.b_bframe_rdo);
  lqt_dump("    b_mixed_references: %d\n", params->analyse.b_mixed_references);
  lqt_dump("    i_trellis:          %d\n", params->analyse.i_trellis);         // 0..2
  lqt_dump("    b_fast_pskip:       %d\n", params->analyse.b_fast_pskip);
  lqt_dump("    i_noise_reduction:  %d\n", params->analyse.i_noise_reduction); // 0..1<<16
  lqt_dump("    b_psnr:             %d\n", params->analyse.b_psnr);

  lqt_dump("  Rate control:\n");
  lqt_dump("    i_qp_constant:      %d\n", params->rc.i_qp_constant);
  lqt_dump("    i_qp_min:           %d\n", params->rc.i_qp_min);
  lqt_dump("    i_qp_max:           %d\n", params->rc.i_qp_max);
  lqt_dump("    i_qp_step:          %d\n", params->rc.i_qp_step);
  lqt_dump("    b_cbr:              %d\n", params->rc.b_cbr);
  lqt_dump("    i_bitrate:          %d\n", params->rc.i_bitrate);
#if X264_BUILD < 54
  lqt_dump("    i_rf_constant:      %d\n", params->rc.i_rf_constant);
#else
  lqt_dump("    f_rf_constant:      %f\n", params->rc.f_rf_constant);
#endif
  lqt_dump("    f_rate_tolerance:   %f\n", params->rc.f_rate_tolerance);
  lqt_dump("    i_vbv_max_bitrate:  %d\n", params->rc.i_vbv_max_bitrate);
  lqt_dump("    i_vbv_buffer_size:  %d\n", params->rc.i_vbv_buffer_size);
  lqt_dump("    f_vbv_buffer_init:  %f\n", params->rc.f_vbv_buffer_init);
  lqt_dump("    f_ip_factor:        %f\n", params->rc.f_ip_factor);
  lqt_dump("    f_pb_factor:        %f\n", params->rc.f_pb_factor);
  
  }

#endif // DUMP_CONFIG

/*
 *  x264 encoder.
 *  NAL reformatting stuff taken from ffmpeg/libavformat/movenc.c
 */ 

typedef struct
  {
  x264_param_t params;
  x264_t *enc;
  x264_picture_t pic; 
  int initialized;

  /* For raw x264 output */
  uint8_t * work_buffer;
  int work_buffer_size;

  /* Reformatted buffer */
  uint8_t * work_buffer_1;
  int work_buffer_alloc_1;

  int total_passes;
  int pass;
  char * stats_filename;
  
  } quicktime_x264_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
  {
  quicktime_x264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  if(codec->enc)
    x264_encoder_close(codec->enc);
  if(codec->stats_filename)
    free(codec->stats_filename);
  free(codec);
  return 0;
  }

static int
encode_nals(uint8_t *buf, int size, x264_nal_t *nals, int nnal)
  {
  uint8_t *p = buf;
  int i;
  int s;
  
  for(i = 0; i < nnal; i++)
    {
    s = x264_nal_encode(p, &size, 1, nals + i);
    if(s < 0)
      return -1;
    p += s;
    }
  
  return p - buf;
  }

static uint8_t *avc_find_startcode( uint8_t *p, uint8_t *end )
  {
  uint8_t *a = p + 4 - ((long)p & 3);
  
  for( end -= 3; p < a && p < end; p++ )
    {
    if( p[0] == 0 && p[1] == 0 && p[2] == 1 )
      return p;
    }
  
  for( end -= 3; p < end; p += 4 )
    {
    uint32_t x = *(uint32_t*)p;
    //      if( (x - 0x01000100) & (~x) & 0x80008000 ) // little endian
    //      if( (x - 0x00010001) & (~x) & 0x00800080 ) // big endian
    if( (x - 0x01010101) & (~x) & 0x80808080 )
      { // generic
      if( p[1] == 0 )
        {
        if( p[0] == 0 && p[2] == 1 )
          return p;
        if( p[2] == 0 && p[3] == 1 )
          return p+1;
        }
      if( p[3] == 0 )
        {
        if( p[2] == 0 && p[4] == 1 )
          return p+2;
        if( p[4] == 0 && p[5] == 1 )
          return p+3;
        }
      }
    }
  
  for( end += 3; p < end; p++ )
    {
    if( p[0] == 0 && p[1] == 0 && p[2] == 1 )
      return p;
    }
  return end + 3;
  }

static int avc_parse_nal_units(uint8_t *src, int src_size,
                               uint8_t ** _dst, int * dst_alloc)
  {
  uint8_t *p = src;
  uint8_t *end = p + src_size;
  uint8_t *nal_start, *nal_end;

  uint8_t * dst, * ret_ptr;
  int ret_size = 0;
  uint32_t nal_size;

  dst = *_dst;
  
  /* Get the size */
  nal_start = avc_find_startcode(p, end);
  while (nal_start < end)
    {
    while(!*(nal_start++));
    nal_end = avc_find_startcode(nal_start, end);

    ret_size += 4 + (int)(nal_end - nal_start);
    nal_start = nal_end;
    }

  if(ret_size > *dst_alloc)
    {
    *dst_alloc = ret_size + 1024;
    dst = realloc(dst, *dst_alloc);
    }
  
  /* Copy */
  ret_ptr = dst;
  
  nal_start = avc_find_startcode(p, end);
  while (nal_start < end)
    {
    while(!*(nal_start++));
    nal_end = avc_find_startcode(nal_start, end);

    nal_size = nal_end - nal_start;

    ret_ptr[0] = (nal_size >> 24) & 0xff;
    ret_ptr[1] = (nal_size >> 16) & 0xff;
    ret_ptr[2] = (nal_size >> 8) & 0xff;
    ret_ptr[3] = (nal_size) & 0xff;
    ret_ptr += 4;
    memcpy(ret_ptr, nal_start, nal_size);
    ret_ptr += nal_size;
    
    nal_start = nal_end;
    }
  
  *_dst = dst;
  return ret_size;
  }

static uint8_t * create_avcc_atom(quicktime_x264_codec_t * codec,
                                  int * ret_size)
  {
  int i;
  x264_nal_t *nal;
  int nnal;
  uint8_t * ret, *ret_ptr;
  uint8_t * tmp_buf, *tmp_buf_1 = (uint8_t*)0;
  int tmp_buf_alloc_1 = 0;
  int tmp_size;
  int tmp_size_1;
  
  uint8_t * ptr, *ptr_end;
  uint32_t nal_size;
  uint32_t sps_size=0, pps_size=0;
  uint8_t *sps=0, *pps=0;
  uint8_t nal_type;
  
  x264_encoder_headers(codec->enc, &nal, &nnal);

  tmp_size = 0;

  /* 5 bytes NAL header + worst case escaping */
  for(i = 0; i < nnal; i++)
    tmp_size += 5 + nal[i].i_payload * 4 / 3;

  tmp_buf = malloc(tmp_size);
  tmp_size = encode_nals(tmp_buf, tmp_size, nal, nnal);
  
  tmp_size_1 = avc_parse_nal_units(tmp_buf, tmp_size,
                                   &tmp_buf_1, &tmp_buf_alloc_1);

  /* Look for sps and pps */

  ptr = tmp_buf_1;
  ptr_end = tmp_buf_1 + tmp_size_1;

  /* look for sps and pps */
  while(ptr < ptr_end)
    {
    nal_size =
      ((uint32_t)ptr[0] << 24) |
      ((uint32_t)ptr[1] << 16) |
      ((uint32_t)ptr[2] << 8) |
      (uint32_t)ptr[3];
    
    nal_type = ptr[4] & 0x1f;
    if (nal_type == 7)
      { /* SPS */
      sps = ptr + 4;
      sps_size = nal_size;
      }
    else if (nal_type == 8)
      { /* PPS */
      pps = ptr + 4;
      pps_size = nal_size;
      }
    ptr += nal_size + 4;
    }

  *ret_size =
    6 +        // Initial bytes
    2 +        // sps_size
    sps_size + // sps
    1 +        // num_pps
    2 +        // pps_size
    pps_size;  // pps

  
  ret = malloc(*ret_size);
  ret_ptr = ret;
  
  /* Start encoding */

  *(ret_ptr++) = 0x01; /* Version */
  *(ret_ptr++) = 0x4d; /* 77, profile */
  *(ret_ptr++) = 0x40; /* 64, profile compat */
  *(ret_ptr++) = 0x1E; /* 30, level (31?) */
  *(ret_ptr++) = 0xFF; /* 6 bits reserved (111111) + 2 bits nal size length - 1 (11) */
  *(ret_ptr++) = 0xE1; /* 3 bits reserved (111) + 5 bits number of sps (00001)       */

  /* sps_size */
  ret_ptr[0] = (sps_size >> 8) & 0xff;
  ret_ptr[1] = sps_size & 0xff;
  ret_ptr += 2;

  /* sps */
  memcpy(ret_ptr, sps, sps_size);
  ret_ptr += sps_size;

  /* num_pps */
  *(ret_ptr++) = 0x01;

  /* pps_size */
  ret_ptr[0] = (pps_size >> 8) & 0xff;
  ret_ptr[1] = pps_size & 0xff;
  ret_ptr += 2;

  /* pps */
  memcpy(ret_ptr, pps, pps_size);
  ret_ptr += pps_size;

  free(tmp_buf);
  free(tmp_buf_1);
  
  return ret;
  }

static int flush_frame(quicktime_t *file, int track,
                       x264_picture_t * pic_in)
  {
  int result;
  quicktime_atom_t chunk_atom;
  x264_nal_t *nal;
  int nnal;
  x264_picture_t pic_out;
  int encoded_size;

  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_x264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  quicktime_trak_t *trak = vtrack->track;
  pic_out.i_pts = 0;
  /* Encode frames, get nals */
  if(x264_encoder_encode(codec->enc, &nal, &nnal, pic_in, &pic_out))
    return 0;
  
  /* Encode nals -> get h264 stream */
  encoded_size = encode_nals(codec->work_buffer,
                             codec->work_buffer_size, nal, nnal);
  
  /* Reformat nals */
  encoded_size = avc_parse_nal_units(codec->work_buffer,
                                     encoded_size,
                                     &codec->work_buffer_1,
                                     &codec->work_buffer_alloc_1);


  if(encoded_size < 0)
    return 0;

  vtrack->coded_timestamp =   pic_out.i_pts;


  if(encoded_size)
    {
    quicktime_write_chunk_header(file, trak, &chunk_atom);
  
    result = !quicktime_write_data(file, 
                                   codec->work_buffer_1, 
                                   encoded_size);
    quicktime_write_chunk_footer(file, 
                                 trak, 
                                 vtrack->current_chunk,
                                 &chunk_atom, 
                                 1);
  
    if(pic_out.i_type == X264_TYPE_IDR)
      quicktime_insert_keyframe(file, vtrack->current_chunk-1, track);
    vtrack->current_chunk++;
    return 1;
    }
  return 0;
  }

static int set_pass_x264(quicktime_t *file, 
                           int track, int pass, int total_passes,
                           const char * stats_file)
  {
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_x264_codec_t *codec =
    ((quicktime_codec_t*)vtrack->codec)->priv;

  codec->total_passes = total_passes;
  codec->pass = pass;
  codec->stats_filename = malloc(strlen(stats_file)+1);
  strcpy(codec->stats_filename, stats_file);
  
  return 1;
  }

static struct
  {
  int             x264_level;
  lqt_log_level_t lqt_level;
  }
log_levels[] =
  {
    { X264_LOG_ERROR,   LQT_LOG_ERROR   },
    { X264_LOG_WARNING, LQT_LOG_WARNING },
    { X264_LOG_INFO,    LQT_LOG_INFO    },
    { X264_LOG_DEBUG,   LQT_LOG_DEBUG   },
    { X264_LOG_NONE,    LQT_LOG_WARNING }
  };

static void
x264_do_log( void * priv, int i_level, const char *psz, va_list argp)
  {
  int i;
  lqt_log_level_t lqt_level;
  quicktime_t * file;
  char * msg_string;
  
#ifndef HAVE_VASPRINTF
  int len;
#endif
  
  for(i = 0; i < sizeof(log_levels) / sizeof(log_levels[0]); i++)
    {
    if(log_levels[i].x264_level == i_level)
      {
      lqt_level = log_levels[i].lqt_level;
      break;
      }
    }
  if(i >= sizeof(log_levels) / sizeof(log_levels[0]))
    {
    lqt_log(file, LQT_LOG_WARNING,
            LOG_DOMAIN, "Invalid log level from x264");
    return;
    }
  file = (quicktime_t *)priv;
  
#ifndef HAVE_VASPRINTF
  len = vsnprintf((char*)0, 0, psz, argp);
  msg_string = malloc(len+1);
  vsnprintf(msg_string, len+1, psz, argp);
#else
  vasprintf(&msg_string, psz, argp);
#endif

  i = strlen(msg_string);
   
  if(msg_string[i-1] == '\n')
    msg_string[i-1] = '\0';
     
  
  lqt_logs(file, lqt_level, LOG_DOMAIN, msg_string);
  free(msg_string);
  }


static void set_fiel(quicktime_t * file, int track)
  {
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_stsd_table_t *stsd;
  
  stsd = vtrack->track->mdia.minf.stbl.stsd.table;

  if(stsd->has_fiel)
    return;
  
  switch(vtrack->interlace_mode)
    {
    case LQT_INTERLACE_NONE:
      lqt_set_fiel(file, track, 1, 0);
      break;
    case LQT_INTERLACE_TOP_FIRST:
      lqt_set_fiel(file, track, 2, 9);
      break;
    case LQT_INTERLACE_BOTTOM_FIRST:
      lqt_set_fiel(file, track, 2, 14);
      break;
    }
  }


static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
  {

  x264_picture_t pic_in;

  int result = 0, pixel_width, pixel_height;
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_trak_t *trak = vtrack->track;
  quicktime_x264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  int height = trak->tkhd.track_height;
  int width = trak->tkhd.track_width;
  uint8_t * avcc;
  int avcc_size;
  
  
  if(!row_pointers)
    {
    vtrack->stream_cmodel = BC_YUV420P;
    return 0;
    }

  /* Initialize */

  if(!codec->initialized)
    {
    codec->work_buffer_size = width * height * 3;
    codec->work_buffer = malloc(codec->work_buffer_size); /* Any smaller value here? */
    
    /* We want a global header */
    codec->params.b_repeat_headers = 0;

    /* Don't output psnr statistics */
    codec->params.analyse.b_psnr = 0;
    
    /* Set format */
    codec->params.i_width = width;
    codec->params.i_height = height;

    lqt_get_pixel_aspect(file, track, &pixel_width, &pixel_height);
    codec->params.vui.i_sar_width  = pixel_width;
    codec->params.vui.i_sar_height = pixel_height;

    codec->params.i_fps_num = lqt_video_time_scale(file, track);
    codec->params.i_fps_den = lqt_frame_duration(file, track, NULL);

    // Set up logging
    codec->params.pf_log = x264_do_log;
    codec->params.p_log_private = file;
    
#if X264_BUILD >= 51
    if(lqt_get_interlace_mode(file, track) != LQT_INTERLACE_NONE)
      {
      lqt_log(file, LQT_LOG_INFO, LOG_DOMAIN,
              "Choosing interlaced encoding");
      codec->params.b_interlaced = 1;
      set_fiel(file, track);
      }
#endif
    
    /* Set multipass control */

    if(codec->total_passes)
      {
      /* Force ABR */
      codec->params.rc.i_rc_method = X264_RC_ABR;
#if X264_BUILD < 54
      codec->params.rc.i_rf_constant = 0;
#else
      codec->params.rc.f_rf_constant = 0.;
#endif
      if(codec->pass == 1)
        {
        /* Strings will be made private by x264 */
        codec->params.rc.psz_stat_out = codec->stats_filename;
        codec->params.rc.b_stat_write = 1;
        }
      else if(codec->pass == codec->total_passes)
        {
        /* Strings will be made private by x264 */
        codec->params.rc.psz_stat_in = codec->stats_filename;
        codec->params.rc.b_stat_read = 1;
        }
      }
    
    /* Open encoder */

    codec->enc = x264_encoder_open(&codec->params);
    if(!codec->enc)
      {
      lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "x264_encoder_open failed");
      return result;
      }

    if(codec->params.i_bframe)
      vtrack->has_b_frames = 1; 
    
    /* Encode global header */
    avcc = create_avcc_atom(codec, &avcc_size);
    
    quicktime_user_atoms_add_atom(&(trak->mdia.minf.stbl.stsd.table[0].user_atoms),
                                  "avcC", avcc, avcc_size);

    file->moov.iods.videoProfileId = 0x15;
    
#ifdef DUMP_CONFIG    
    dump_params(&codec->params);
#endif
    codec->initialized = 1;
    
    }

  /* Encode picture */
  memset(&pic_in, 0, sizeof(pic_in));

  pic_in.img.i_csp = X264_CSP_I420;
  pic_in.img.i_plane = 3;

  pic_in.img.plane[0] = row_pointers[0];
  pic_in.img.plane[1] = row_pointers[1];
  pic_in.img.plane[2] = row_pointers[2];

  pic_in.img.i_stride[0] = vtrack->stream_row_span;
  pic_in.img.i_stride[1] = vtrack->stream_row_span_uv;
  pic_in.img.i_stride[2] = vtrack->stream_row_span_uv;

  pic_in.i_pts = vtrack->timestamp;
  pic_in.i_type = X264_TYPE_AUTO;
  
  flush_frame(file, track, &pic_in);
  
  return result;
  }

static int flush(quicktime_t *file, int track)
  {
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_x264_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  
  /* Do nothing if we didn't encode anything yet */
  if(!codec->initialized)
    return 0;
  
  return flush_frame(file, track, (x264_picture_t*)0);
  }
  

#define INTPARAM(name, var) \
  if(!strcasecmp(key, name)) \
    { \
    var = *((int*)value); \
    found = 1; \
    }

#define FLOATPARAM(name, var)  \
  if(!strcasecmp(key, name)) \
    { \
    var = *((float*)value); \
    found = 1; \
    }

#define ENUMPARAM(name, var, arr) \
  if(!strcasecmp(key, name)) \
    { \
    for(i = 0; i < sizeof(arr)/sizeof(arr[0]); i++) \
      {                                             \
      if(!strcasecmp((char*)value, arr[i].s))       \
        {                                           \
        var = arr[i].i;                             \
        break;                                      \
        }                                           \
      }                                             \
    found = 1;                                      \
    }

typedef struct
  {
  char * s;
  int i;
  } enum_t;
  
enum_t me_methods[] =
  {
    { "Diamond search",       X264_ME_DIA },
    { "Hexagonal search",     X264_ME_HEX },
    { "Uneven Multi-Hexagon", X264_ME_UMH },
    { "Exhaustive search",    X264_ME_ESA }
  };

enum_t trellis[] =
  {
    { "Disabled",         0 },
    { "Enabled (final)",  1 },
    { "Enabled (always)", 2 }
  };

enum_t direct_modes[] =
  {
    { "None",     X264_DIRECT_PRED_NONE },
    { "Spatial",  X264_DIRECT_PRED_SPATIAL },
    { "Temporal", X264_DIRECT_PRED_TEMPORAL },
    { "Auto",     X264_DIRECT_PRED_AUTO }
  };

enum_t rc_methods[] =
  {
    { "Constant quality", X264_RC_CQP },
    { "Average bitrate",  X264_RC_ABR },
    { "CRF based VBR",    X264_RC_CRF }
  };
                                     

static int set_parameter(quicktime_t *file, 
                         int track, 
                         char *key, 
                         void *value)
  {
  int found = 0, i;
  quicktime_x264_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;

  INTPARAM("x264_i_keyint_max", codec->params.i_keyint_max);
  INTPARAM("x264_i_keyint_min", codec->params.i_keyint_min);
  INTPARAM("x264_i_scenecut_threshold", codec->params.i_scenecut_threshold);
  INTPARAM("x264_i_bframe", codec->params.i_bframe);
  INTPARAM("x264_b_bframe_adaptive", codec->params.b_bframe_adaptive);
  INTPARAM("x264_i_bframe_bias", codec->params.i_bframe_bias);
  INTPARAM("x264_b_bframe_pyramid", codec->params.b_bframe_pyramid);

  ENUMPARAM("x264_i_rc_method", codec->params.rc.i_rc_method, rc_methods);
  INTPARAM("x264_i_bitrate", codec->params.rc.i_bitrate);
  
  INTPARAM("x264_i_qp_constant", codec->params.rc.i_qp_constant);
#if X264_BUILD < 54
  INTPARAM("x264_i_rf_constant", codec->params.rc.i_rf_constant);
#else
  FLOATPARAM("x264_f_rf_constant", codec->params.rc.f_rf_constant);
#endif
  INTPARAM("x264_i_qp_min", codec->params.rc.i_qp_min);
  INTPARAM("x264_i_qp_max", codec->params.rc.i_qp_max);
  INTPARAM("x264_i_qp_step", codec->params.rc.i_qp_step);
  FLOATPARAM("x264_f_rate_tolerance", codec->params.rc.f_rate_tolerance);
  INTPARAM("x264_i_vbv_max_bitrate", codec->params.rc.i_vbv_max_bitrate);
  INTPARAM("x264_i_vbv_buffer_size", codec->params.rc.i_vbv_buffer_size);
  FLOATPARAM("x264_f_vbv_buffer_init", codec->params.rc.f_vbv_buffer_init);
  FLOATPARAM("x264_f_ip_factor", codec->params.rc.f_ip_factor);
  FLOATPARAM("x264_f_pb_factor", codec->params.rc.f_pb_factor);
  INTPARAM("x264_analyse_8x8_transform", codec->params.analyse.b_transform_8x8);

  if(!strcasecmp(key, "x264_analyse_psub16x16"))
    {
    if(*(int*)(value))
      {
      codec->params.analyse.inter |= X264_ANALYSE_PSUB16x16;
      }
    else
      {
      codec->params.analyse.inter &= ~X264_ANALYSE_PSUB16x16;
      }
    found = 1;
    }
  if(!strcasecmp(key, "x264_analyse_bsub16x16"))
    {
    if(*(int*)(value))
      {
      codec->params.analyse.inter |= X264_ANALYSE_BSUB16x16;
      }
    else
      {
      codec->params.analyse.inter &= ~X264_ANALYSE_BSUB16x16;
      }
    found = 1;
    }
  if(!strcasecmp(key, "x264_analyse_psub8x8"))
    {
    if(*(int*)(value))
      {
      codec->params.analyse.inter |= X264_ANALYSE_PSUB8x8;
      }
    else
      {
      codec->params.analyse.inter &= ~X264_ANALYSE_PSUB8x8;
      }
    found = 1;
    }
  if(!strcasecmp(key, "x264_analyse_i8x8"))
    {
    if(*(int*)(value))
      {
      codec->params.analyse.intra |= X264_ANALYSE_I8x8;
      codec->params.analyse.inter |= X264_ANALYSE_I8x8;
      }
    else
      {
      codec->params.analyse.intra &= ~X264_ANALYSE_I8x8;
      codec->params.analyse.inter &= ~X264_ANALYSE_I8x8;
      }
    found = 1;
    }
  if(!strcasecmp(key, "x264_analyse_i4x4"))
    {
    if(*(int*)(value))
      {
      codec->params.analyse.intra |= X264_ANALYSE_I4x4;
      codec->params.analyse.inter |= X264_ANALYSE_I4x4;
      }
    else
      {
      codec->params.analyse.intra &= ~X264_ANALYSE_I4x4;
      codec->params.analyse.inter &= ~X264_ANALYSE_I4x4;
      }
    found = 1;
    }
  ENUMPARAM("x264_i_me_method", codec->params.analyse.i_me_method, me_methods);

  INTPARAM("x264_i_subpel_refine", codec->params.analyse.i_subpel_refine);
  INTPARAM("x264_b_bframe_rdo", codec->params.analyse.b_bframe_rdo);
  INTPARAM("x264_i_me_range", codec->params.analyse.i_me_range);
  INTPARAM("x264_i_frame_reference", codec->params.i_frame_reference);
  INTPARAM("x264_b_chroma_me", codec->params.analyse.b_chroma_me);
  INTPARAM("x264_b_mixed_references", codec->params.analyse.b_mixed_references);
  INTPARAM("x264_b_bidir_me", codec->params.analyse.b_bidir_me);
  INTPARAM("x264_b_weighted_bipred", codec->params.analyse.b_weighted_bipred);
    
  ENUMPARAM("x264_i_direct_mv_pred", codec->params.analyse.i_direct_mv_pred, direct_modes);
  
  INTPARAM("x264_b_deblocking_filter", codec->params.b_deblocking_filter);
  INTPARAM("x264_i_deblocking_filter_alphac0", codec->params.i_deblocking_filter_alphac0);
  INTPARAM("x264_i_deblocking_filter_beta", codec->params.i_deblocking_filter_beta);
  INTPARAM("x264_b_cabac", codec->params.b_cabac);

  ENUMPARAM("x264_i_trellis", codec->params.analyse.i_trellis, trellis);

  INTPARAM("x264_i_noise_reduction", codec->params.analyse.i_noise_reduction);
  
  if(!found)
    lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
            "Unrecognized parameter %s", key);
  
  return 0;
  }

void quicktime_init_codec_x264(quicktime_video_map_t *vtrack)
  {
  quicktime_x264_codec_t *codec;
  
  /* Init public items */
  ((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_x264_codec_t));
  ((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
  ((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
  ((quicktime_codec_t*)vtrack->codec)->set_pass = set_pass_x264;
  ((quicktime_codec_t*)vtrack->codec)->flush = flush;
  ((quicktime_codec_t*)vtrack->codec)->decode_video = 0;
  ((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;
  ((quicktime_codec_t*)vtrack->codec)->decode_audio = 0;
  ((quicktime_codec_t*)vtrack->codec)->encode_audio = 0;
  
  /* Init private items */
  codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  x264_param_default(&(codec->params));
  }

