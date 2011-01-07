/*******************************************************************************
 rtjpeg_codec.h

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

#ifndef QUICKTIME_RTJPEG_H
#define QUICKTIME_RTJPEG_H

#include "RTjpeg.h"
#include <quicktime/qtprivate.h>

typedef struct
  {
  uint8_t ** rows;
  int rowspan, rowspan_uv;

  /* Compression stuff */
  RTjpeg_t * compress_struct;
  uint8_t * write_buffer;
  int Q;
  int K;
  int LQ;
  int CQ;
	
  /* DeCompression stuff */
  RTjpeg_t * decompress_struct;
  uint8_t * read_buffer;
  int read_buffer_alloc;
  int jpeg_width;
  int jpeg_height;
  int qt_width;
  int qt_height;
  } quicktime_rtjpeg_codec_t;

#endif

void quicktime_init_codec_rtjpeg(quicktime_codec_t * codec_base,
                                 quicktime_audio_map_t *atrack,
                                 quicktime_video_map_t *vtrack);
