/*******************************************************************************
 stts.c

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
#include <stdlib.h>
#include <string.h>

#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>


void lqt_packet_index_delete(lqt_packet_index_t * pi)
  {
  if(pi->entries)
    free(pi->entries);
  }

void lqt_packet_index_dump(const lqt_packet_index_t* pi)
  {
  int i;
  char c;
  lqt_dump("     Packet index (generated)\n");
  lqt_dump("      num_entries %d\n", pi->num_entries);
  for(i = 0; i < pi->num_entries; i++)
    {
    lqt_dump("       pos %"PRId64" pts %"PRId64" dts %"PRId64" dur %"PRId64" sz %"PRId64" ",
             pi->entries[i].position,
             pi->entries[i].pts, pi->entries[i].dts,
             pi->entries[i].duration,
             pi->entries[i].size);

    switch(pi->entries[i].flags & LQT_PACKET_TYPE_MASK)
      {
      case LQT_PACKET_TYPE_I:
        c = 'I';
        break;
      case LQT_PACKET_TYPE_P:
        c = 'P';
        break;
      case LQT_PACKET_TYPE_B:
        c = 'B';
        break;
      default:
        c = '?';
        break;
      }
    lqt_dump("FT %c ", c);

    lqt_dump("stsd_id %d ", pi->entries[i].stsd_index);
    
    if(pi->entries[i].flags & LQT_PACKET_KEYFRAME)
      lqt_dump("key ");
    if(pi->entries[i].flags & LQT_PACKET_REF_FRAME)
      lqt_dump("ref ");

    lqt_dump("\n");
    
    }
  }

void lqt_packet_index_alloc(lqt_packet_index_t * pi, int num)
  {
  pi->entries_alloc = num;

  pi->entries = realloc(pi->entries,
                        pi->entries_alloc * sizeof(*pi->entries));
  memset(pi->entries + pi->num_entries, 0,
         (pi->entries_alloc - pi->num_entries) * sizeof(*pi->entries));
  }

void lqt_packet_index_append(lqt_packet_index_t * pi,
                             const lqt_packet_index_entry_t * e)
  {
  if(pi->num_entries >= pi->entries_alloc)
    lqt_packet_index_alloc(pi, pi->num_entries + 1024);
  
  memcpy(pi->entries + pi->num_entries,
         e, sizeof(*e));
  pi->num_entries++;
  }

static void
packet_index_create_video(quicktime_t *file,
                          quicktime_trak_t * trak,
                          lqt_packet_index_t * idx)
  {
  int i;

  int stts_index = 0;
  int stts_count = 0;

  int ctts_index = 0;
  int ctts_count = 0;
  
  lqt_packet_index_entry_t e;
  int stsc_index = 0;
  int stsc_count = 0;
  int stsc_chunk = 1;

  int stss_index = 0;
  int stps_index = 0;
  
  int64_t max_pts = 0;

  quicktime_stss_t *stss;
  quicktime_stps_t *stps;
  
  quicktime_ctts_t *ctts;
  quicktime_stts_t *stts = &trak->mdia.minf.stbl.stts;
  quicktime_stsc_t *stsc = &trak->mdia.minf.stbl.stsc;
  int num = 0;

  /* Count frames */
  for(i = 0; i < stts->total_entries; i++)
    num += stts->table[i].sample_count;
  
  if(trak->mdia.minf.stbl.has_ctts)
    ctts = &trak->mdia.minf.stbl.ctts;
  else
    ctts = NULL;
  
  memset(&e, 0, sizeof(e));
  
  lqt_packet_index_alloc(idx, num);

  e.position = quicktime_chunk_to_offset(file, trak, 0);

  if(trak->mdia.minf.stbl.stsz.sample_size)
    e.size = trak->mdia.minf.stbl.stsz.sample_size;

  if(trak->mdia.minf.stbl.stss.total_entries > 0)
    stss = &trak->mdia.minf.stbl.stss;
  else
    stss = NULL;
  
  if(trak->mdia.minf.stbl.stps.total_entries > 0)
    stps = &trak->mdia.minf.stbl.stps;
  else
    stps = NULL;
  
  for(i = 0; i < num; i++)
    {
    e.flags = 0;
    
    /* Duration */
    e.duration = stts->table[stts_index].sample_duration;;

    /* PTS */
    e.pts = e.dts;
    
    /* PTS = DTS + CTS if there are B-frames */
    if(ctts)
      {
      e.pts += ctts->table[ctts_index].sample_duration -
        ctts->table[0].sample_duration;
      }

    /* Determine size */
    if(!trak->mdia.minf.stbl.stsz.sample_size)
      e.size = trak->mdia.minf.stbl.stsz.table[i].size;
    
    /* Determine if we have a keyframe */
    if(stss &&
       (stss_index < stss->total_entries) &&
       (stss->table[stss_index].sample == i+1))
      {
      e.flags |= LQT_PACKET_KEYFRAME | LQT_PACKET_REF_FRAME;
      stss_index++;
      }
    else if(stps && 
            (stps_index < stps->total_entries) &&
            (stps->table[stps_index].sample == i+1))
      {
      e.flags |= LQT_PACKET_KEYFRAME | LQT_PACKET_REF_FRAME;
      stps_index++;
      }
    else if(!stss)
      e.flags |= LQT_PACKET_KEYFRAME;

    if(e.flags & LQT_PACKET_KEYFRAME)
      e.flags |= LQT_PACKET_TYPE_I;
    else if(e.pts < max_pts)
      e.flags |= LQT_PACKET_TYPE_B;
    else
      e.flags |= LQT_PACKET_TYPE_P | LQT_PACKET_REF_FRAME;
    
    if(e.pts > max_pts)
      max_pts = e.pts;
    
    /* Append entry */
    lqt_packet_index_append(idx, &e);

    if(i == num - 1)
      break;
    
    /* Advance packet */
    e.dts += e.duration;
    e.position += e.size;
    
    /* Advance indices */

    /* stts */
    stts_count++;
    if(stts_count >= stts->table[stts_index].sample_count)
      {
      stts_index++;
      stts_count = 0;
      }

    /* ctts */
    if(ctts)
      {
      ctts_count++;
      if(ctts_count >= ctts->table[ctts_index].sample_count)
        {
        ctts_index++;
        ctts_count = 0;
        }
      }

    /* stsc */
    stsc_count++;
    if(stsc_count >= stsc->table[stsc_index].samples)
      {
      /* Increment chunk */
      stsc_chunk++;
      stsc_count = 0;

      /* stsc position */
      if((stsc_index < stsc->total_entries - 1) &&
         (stsc_chunk == stsc->table[stsc_index+1].chunk))
        {
        stsc_index++;
        e.stsd_index = stsc->table[stsc_index].id - 1;
        }
      e.position = trak->mdia.minf.stbl.stco.table[stsc_chunk-1].offset;
      }
    }

  /* Find Reference B-Frames */

  if(ctts)
    {
    int64_t min_b_pts = -1;
    i = num;

    while(--i >= 0)
      {
      if((idx->entries[i].flags & LQT_PACKET_TYPE_MASK) == LQT_PACKET_TYPE_B)
        {
        if(min_b_pts == -1)
          min_b_pts = idx->entries[i].pts;
        else if(idx->entries[i].pts > min_b_pts)
          idx->entries[i].flags |= LQT_PACKET_REF_FRAME;
        else
          min_b_pts = idx->entries[i].pts;
        }
      }
    }
  
  }

static void
packet_index_create_audio(quicktime_t *file,
                          quicktime_trak_t * trak,
                          lqt_packet_index_t * idx)
  {
  // Each chunk is a packet
  lqt_packet_index_entry_t e;
  int64_t * chunk_sizes;
  int i;
  int stsc_index = 0;
  int num = trak->mdia.minf.stbl.stco.total_entries;
  quicktime_stsc_t *stsc = &trak->mdia.minf.stbl.stsc;

  lqt_packet_index_alloc(idx, num);
    
  memset(&e, 0, sizeof(e));
  chunk_sizes = lqt_get_chunk_sizes(file, trak);

  e.flags |= LQT_PACKET_KEYFRAME;
  e.position = quicktime_chunk_to_offset(file, trak, 0);
  
  for(i = 0; i < num; i++)
    {
    if((stsc_index < stsc->total_entries - 1) &&
       (i+1 == stsc->table[stsc_index+1].chunk))
      {
      stsc_index++;
      e.stsd_index = stsc->table[stsc_index].id - 1;
      }
    e.position = trak->mdia.minf.stbl.stco.table[i].offset;
    e.size     = chunk_sizes[i];
    e.duration = stsc->table[stsc_index].samples;

    /* Append entry */
    lqt_packet_index_append(idx, &e);

    /* Advance */
    e.pts += e.duration;
    e.dts = e.pts;
    }
  
  free(chunk_sizes);
  }

static void
packet_index_create_audio_vbr(quicktime_t *file,
                              quicktime_trak_t * trak,
                              lqt_packet_index_t * idx)
  {
  // Each sample is a packet
  int stts_index = 0;
  int stts_count = 0;
  int i;
  lqt_packet_index_entry_t e;
  int stsc_index = 0;
  int stsc_count = 0;
  int stsc_chunk = 1;

  quicktime_stts_t *stts = &trak->mdia.minf.stbl.stts;
  quicktime_stsc_t *stsc = &trak->mdia.minf.stbl.stsc;

  int num = 0;

  for(i = 0; i < stts->total_entries; i++)
    num += stts->table[i].sample_count;

  lqt_packet_index_alloc(idx, num);
  
  memset(&e, 0, sizeof(e));

  e.flags |= LQT_PACKET_KEYFRAME;

  if(trak->mdia.minf.stbl.stsz.sample_size)
    e.size = trak->mdia.minf.stbl.stsz.sample_size;

  e.position = quicktime_chunk_to_offset(file, trak, 0);
  
  for(i = 0; i < num; i++)
    {
    e.duration = stts->table[stts_index].sample_duration;
    
    if(!trak->mdia.minf.stbl.stsz.sample_size)
      e.size = trak->mdia.minf.stbl.stsz.table[i].size;
    
    /* Append entry */
    lqt_packet_index_append(idx, &e);

    if(i == num - 1)
      break;
    
    /* Advance */
    e.pts += e.duration;
    e.dts = e.pts;
    e.position += e.size;
    
    /* stts */
    stts_count++;
    if(stts_count >= stts->table[stts_index].sample_count)
      {
      stts_index++;
      stts_count = 0;
      }
    
    /* stsc */
    stsc_count++;
    if(stsc_count >= stsc->table[stsc_index].samples)
      {
      /* Increment chunk */
      stsc_chunk++;
      stsc_count = 0;

      /* stsc position */
      if((stsc_index < stsc->total_entries - 1) &&
         (stsc_chunk == stsc->table[stsc_index+1].chunk))
        {
        stsc_index++;
        e.stsd_index = stsc->table[stsc_index].id - 1;
        }
      e.position = trak->mdia.minf.stbl.stco.table[stsc_chunk-1].offset;
      }
    }
  
  }

void
lqt_packet_index_create_from_trak(quicktime_t *file,
                                  quicktime_trak_t * trak,
                                  lqt_packet_index_t * idx)
  {
  if(trak->idx.num_entries)
    return;

  if(trak->mdia.minf.is_audio)
    {
    if(trak->mdia.minf.is_audio_vbr)
      packet_index_create_audio_vbr(file, trak, idx);
    
    else // Each packet is a "chunk"
      return packet_index_create_audio(file, trak, idx);
    }
  else if(trak->mdia.minf.is_video)
    packet_index_create_video(file, trak, idx);
  }

int lqt_packet_index_peek_packet(quicktime_t *file,
                                 const lqt_packet_index_t * idx,
                                 lqt_packet_t * p, int packetnum)
  {
  const lqt_packet_index_entry_t * e;
  if((packetnum < 0) || (packetnum >= idx->num_entries))
    return 0;
  e = idx->entries + packetnum;

  /* Set fields */
  p->flags = e->flags;
  p->timestamp = e->pts;
  p->duration = e->duration;
  return 1;
  }

int lqt_packet_index_read_packet(quicktime_t *file,
                                 const lqt_packet_index_t * idx,
                                 lqt_packet_t * p, int packetnum)
  {
  const lqt_packet_index_entry_t * e;
  if(!lqt_packet_index_peek_packet(file, idx, p, packetnum))
    return 0;

  e = idx->entries + packetnum;
  
  /* Read data */
  quicktime_set_position(file, e->position);
  lqt_packet_alloc(p, e->size);
  if(quicktime_read_data(file, p->data, e->size) < e->size)
    return 0;
  memset(p->data + e->size, 0, LQT_PACKET_PADDING);
  p->data_len = e->size;
  return 1;
  }

int lqt_packet_index_append_packet(quicktime_t *file,
                                   const lqt_packet_index_t * idx,
                                   lqt_packet_t * p, int packetnum)
  {
  const lqt_packet_index_entry_t * e;
  if(!lqt_packet_index_peek_packet(file, idx, p, packetnum))
    return 0;

  e = idx->entries + packetnum;
  
  /* Read data */
  quicktime_set_position(file, e->position);
  lqt_packet_alloc(p, p->data_len + e->size);
  if(quicktime_read_data(file, p->data + p->data_len, e->size) < e->size)
    return 0;
  p->data_len += e->size;
  memset(p->data + p->data_len, 0, LQT_PACKET_PADDING);
  return 1;
  }

void lqt_packet_index_finish(lqt_packet_index_t * idx)
  {
  int i;
  
  for(i = 0; i < idx->num_entries; i++)
    {
    if(idx->entries[i].flags & LQT_PACKET_KEYFRAME)
      idx->num_key_frames++;
    if((idx->entries[i].flags & LQT_PACKET_TYPE_MASK) == LQT_PACKET_TYPE_B)
      idx->num_b_frames++;

    if(idx->max_packet_size < idx->entries[i].size)
      idx->max_packet_size = idx->entries[i].size;

    if(!i || (idx->entries[i].duration < idx->min_packet_duration))
      idx->min_packet_duration = idx->entries[i].duration;
    
    if(!i || (idx->entries[i].duration > idx->max_packet_duration))
      idx->max_packet_duration = idx->entries[i].duration;
    
    if(idx->max_pts < idx->entries[i].pts + idx->entries[i].duration)
      idx->max_pts = idx->entries[i].pts + idx->entries[i].duration;
    }
  }

int lqt_packet_index_seek(const lqt_packet_index_t * idx,
                          int64_t pts)
  {
  int i;
  int ret = -1;
  for(i = 0; i < idx->num_entries; i++)
    {
    if((idx->entries[i].pts <= pts) &&
       (idx->entries[i].pts + idx->entries[i].duration) > pts)
      {
      ret = i;
      break;
      }
    }
  return ret;
  }

/* get the next frame in display order. This routine is intentionally
   quite stupid, but it should be bullet-proof for all kinds of weird
   reordering schemes */

int lqt_packet_index_get_next_display_frame(const lqt_packet_index_t * idx,
                                            int pos)
  {
  int i, start, end;
  int ret = -1;
  int64_t ret_pts = -1;
  
  if(idx->num_b_frames)
    return ret + 1;

  start = ret - 32;
  if(start > 0)
    start = 0;

  end = ret + 32;

  if(end > idx->num_entries)
    end = idx->num_entries;

  for(i = start; i < end; i++)
    {
    /* Frame comes before in display order */
    if(idx->entries[i].pts <= idx->entries[pos].pts)
      continue;
    
    if((ret_pts < 0) || (idx->entries[i].pts < ret_pts))
      {
      ret = i;
      ret_pts = idx->entries[i].pts;
      }
    }
  return ret;
  }
  
int lqt_packet_index_get_keyframe_before(const lqt_packet_index_t * idx,
                                         int ret)
  {
  int64_t pts = idx->entries[ret].pts;
  
  /* Go to the keyframe before */
  while(ret >= 0)
    {
    if((idx->entries[ret].pts <= pts) &&
       (idx->entries[ret].flags & LQT_PACKET_KEYFRAME))
      break;
    ret--;
    }
  return ret;
  }
