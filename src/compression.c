/*******************************************************************************
 compression.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2010 Members of the libquicktime project.

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

#include <stdlib.h>

#include "lqt_private.h"

void lqt_compression_info_free(lqt_compression_info_t * info)
  {
  if(info->global_header)
    free(info->global_header);
  }

void lqt_packet_alloc(lqt_packet_t * p, int bytes)
  {
  if(p->data_alloc < bytes)
    {
    p->data_alloc = bytes + 1024;
    p->data = realloc(p->data, p->data_alloc);
    }
  }

void lqt_packet_free(lqt_packet_t * p)
  {
  if(p->data)
    free(p->data);
  }


/* Reading */

const lqt_compression_info_t * lqt_get_audio_compression_info(quicktime_t * file, int track)
  {
  quicktime_audio_map_t * atrack;

  atrack = &file->atracks[track];
  
  if(atrack->ci.id == LQT_COMPRESSION_UNDEFINED)
    {
    /* Try to get compression info */
    }
  
  if(atrack->ci.id == LQT_COMPRESSION_UNDEFINED)
    return NULL;
  else
    return &atrack->ci;
  }

const lqt_compression_info_t * lqt_get_video_compression_info(quicktime_t * file, int track)
  {
  quicktime_video_map_t * vtrack;

  vtrack = &file->vtracks[track];
  
  if(vtrack->ci.id == LQT_COMPRESSION_UNDEFINED)
    {
    /* Try to get compression info */
    }
  
  if(vtrack->ci.id == LQT_COMPRESSION_UNDEFINED)
    return NULL;
  else
    return &vtrack->ci;
  }

int lqt_read_audio_packet(quicktime_t * file, lqt_packet_t * p, int track)
  {
  return 0;
  }

int lqt_read_video_packet(quicktime_t * file, lqt_packet_t * p, int track)
  {
  return 0;
  }

/* Writing */

int lqt_writes_audio_compressed(quicktime_t * file, const lqt_compression_info_t * info)
  {
  return 0;
  }

int lqt_writes_video_compressed(quicktime_t * file, const lqt_compression_info_t * info)
  {
  return 0;
  }

int lqt_add_audio_track_compressed(quicktime_t * file, const lqt_compression_info_t * info)
  {
  return 0;
  }

int lqt_add_video_track_compressed(quicktime_t * file, const lqt_compression_info_t * info)
  {
  return 0;
  }

int lqt_write_audio_packet(quicktime_t * file, lqt_packet_t * p, int track)
  {
  return 0;
  }

int lqt_write_video_packet(quicktime_t * file, lqt_packet_t * p, int track)
  {
  return 0;
  }
