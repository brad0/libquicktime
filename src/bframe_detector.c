/*******************************************************************************
 trak.c

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

void quicktime_init_bframe_detector(quicktime_bframe_detector* ctx, quicktime_video_map_t* vtrack)
  {
  ctx->frame = 0;
  ctx->ctts_entry_idx = 0;
  ctx->sample_within_entry = 0;
  ctx->ctts = &vtrack->track->mdia.minf.stbl.ctts;
  }

/* Returns 1 for true, 0 for false, or a negative value if impossible to tell. */
int quicktime_is_bframe(quicktime_bframe_detector* ctx, int64_t frame)
  {

  if(ctx->ctts_entry_idx < 0 || ctx->ctts_entry_idx >= ctx->ctts->total_entries)
    return -1;

  if(frame > ctx->frame)
    {
    while(frame - ctx->frame >= ctx->ctts->table[ctx->ctts_entry_idx].sample_count - ctx->sample_within_entry)
      {
      if(ctx->ctts_entry_idx + 1 >= ctx->ctts->total_entries)
        return -1;

      ctx->frame += ctx->ctts->table[ctx->ctts_entry_idx].sample_count - ctx->sample_within_entry;
      ctx->ctts_entry_idx++;
      ctx->sample_within_entry = 0;
      }
    ctx->sample_within_entry += frame - ctx->frame;
    ctx->frame = frame;
    }
  else if(frame < ctx->frame)
    {
    while(ctx->frame - frame > ctx->sample_within_entry)
      {
      if(ctx->ctts_entry_idx < 1)
        return -1;

      ctx->frame -= ctx->sample_within_entry + 1;
      ctx->ctts_entry_idx--;
      ctx->sample_within_entry = ctx->ctts->table[ctx->ctts_entry_idx].sample_count - 1;
      }
    ctx->sample_within_entry -= ctx->frame - frame;
    ctx->frame = frame;
    }

    return ctx->ctts->table[ctx->ctts_entry_idx].sample_duration < 
       ctx->ctts->table[0].sample_duration;;
  }
