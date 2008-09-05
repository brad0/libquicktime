/*******************************************************************************
 videocodec.h

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

#ifndef QUICKTIME_VIDEOCODEC_H
#define QUICKTIME_VIDEOCODEC_H

#include <quicktime/quicktime.h>

void lqt_set_fiel_uncompressed(quicktime_t * file, int track);
void quicktime_init_codec_raw(quicktime_video_map_t *vtrack);
void quicktime_init_codec_rawalpha(quicktime_video_map_t *vtrack);

void quicktime_init_codec_v210(quicktime_video_map_t *vtrack);
void quicktime_init_codec_v308(quicktime_video_map_t *vtrack);
void quicktime_init_codec_v408(quicktime_video_map_t *vtrack);
void quicktime_init_codec_v410(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yuv2(quicktime_video_map_t *vtrack);
void quicktime_init_codec_2vuy(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yuv4(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yv12(quicktime_video_map_t *vtrack);

#endif
