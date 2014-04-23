/*******************************************************************************
 ptscache.c

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

#include <stdlib.h>
#include <string.h>

#include "lqt_private.h"
#include "ptscache.h"

lqt_encoder_pts_cache_t * lqt_encoder_pts_cache_create(void)
  {
  lqt_encoder_pts_cache_t * ret = calloc(1, sizeof(*ret));
  return ret;
  }

void lqt_encoder_pts_cache_destroy(lqt_encoder_pts_cache_t * c)
  {
  if(c->entries)
    free(c->entries);
  free(c);
  }

int
lqt_encoder_pts_cache_push_frame(lqt_encoder_pts_cache_t * c,
                                 int64_t pts, int64_t duration)
  {
  if(c->num_frames >= c->frames_alloc)
    {
    c->frames_alloc += 128;
    c->entries = realloc(c->entries, c->frames_alloc * sizeof(*c->entries));
    }
  
  c->entries[c->num_frames].pts       = pts;
  c->entries[c->num_frames].duration  = duration;
  c->entries[c->num_frames].frame_num = c->frame_counter++;
  
  c->num_frames++;
  return 1;
  }

int
lqt_encoder_pts_cache_pop_packet(lqt_encoder_pts_cache_t * c,
                                 lqt_packet_t * p,
                                 int64_t frame_num, int64_t pts)
  {
  int i, idx = -1;

  if(frame_num < 0)
    {
    for(i = 0; i < c->num_frames; i++)
      {
      if(c->entries[i].pts == pts)
        {
        idx = i;
        break;
        }
      }
    }
  else
    {
    for(i = 0; i < c->num_frames; i++)
      {
      if(c->entries[i].frame_num == frame_num)
        {
        idx = i;
        break;
        }
      }
    }

  if(idx < 0)
    return 0;

  p->timestamp = c->entries[idx].pts;
  p->duration  = c->entries[idx].duration;
  
  if(idx < c->num_frames-1)
    {
    memmove(c->entries + idx, c->entries + (idx+1),
            sizeof(*c->entries) * (c->num_frames-1-idx));
    }
  c->num_frames--;
  return 1;
  }
