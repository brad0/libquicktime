/*******************************************************************************
 video.c

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

#include "quicktime/colormodels.h"


//#define LQT_LIBQUICKTIME
//#include <quicktime/lqt_codecapi.h>

#define LOG_DOMAIN "video"

int quicktime_video_tracks(quicktime_t *file)
  {
  int i, result = 0;
  for(i = 0; i < file->moov.total_tracks; i++)
    {
    if(file->moov.trak[i]->mdia.minf.is_video) result++;
    }
  return result;
  }

int lqt_set_video(quicktime_t *file, 
                  int tracks, 
                  int frame_w, 
                  int frame_h,
                  int frame_duration,
                  int timescale,
                  lqt_codec_info_t * info)
  {
  int i;

  for(i = 0; i < tracks; i++)
    {
    if(lqt_add_video_track(file, frame_w, frame_h,
                           frame_duration, timescale, info))
      return 1;
    }
  return 0;
  }

int quicktime_set_video(quicktime_t *file, 
                        int tracks, 
                        int frame_w, 
                        int frame_h,
                        double frame_rate,
                        char *compressor)
  {
  lqt_codec_info_t ** info;
  int timescale, frame_duration;
  timescale = quicktime_get_timescale(frame_rate);
  frame_duration = (int)((double)(timescale)/frame_rate+0.5);
  info = lqt_find_video_codec(compressor, 1);
  lqt_set_video(file, tracks, frame_w, frame_h, frame_duration, timescale, *info);
  lqt_destroy_codec_info(info);
  return 0;
  }

static int check_image_size(lqt_codec_info_t * info,
                            int frame_w, int frame_h)
  {
  int i;
  if(info->num_image_sizes)
    {
    for(i = 0; i < info->num_image_sizes; i++)
      {
      if((frame_w == info->image_sizes[i].width) &&
         (frame_h == info->image_sizes[i].height))
        return 1;
      }
    
    return 0;
    }
  else
    return 1;
  }

int lqt_set_video_codec(quicktime_t *file, int track,
                        lqt_codec_info_t * info)
  {
  quicktime_stsd_t *stsd;
  
  if(!check_image_size(info,
                       quicktime_video_width(file, track),
                       quicktime_video_height(file, track)))
    return 1;

  stsd = &file->vtracks[track].track->mdia.minf.stbl.stsd;
  
  quicktime_stsd_set_video_codec(stsd, info->fourccs[0]);
  
  quicktime_init_video_map(&file->vtracks[track],
                           file->wr, info);
  
  lqt_set_default_video_parameters(file, track);
  
  /* Get encoding colormodel */
  file->vtracks[file->total_vtracks-1].codec->encode_video(file,
                                                           (uint8_t**)0,
                                                           track);
  file->vtracks[track].io_cmodel =
    file->vtracks[track].stream_cmodel;
  
  
  return 0;
  }

int lqt_add_video_track_internal(quicktime_t *file,
                                 int frame_w, int frame_h,
                                 int frame_duration, int timescale,
                                 lqt_codec_info_t * info,
                                 const lqt_compression_info_t * ci)
  {
  char * compressor = info ? info->fourccs[0] : NULL;
  quicktime_trak_t *trak;

  /* Check if the image size is supported */
  if(info && !check_image_size(info, frame_w, frame_h))
    {
    lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN,
            "Adding video track failed, unsupported image size");
    return 1;
    }
  
  if(!file->total_vtracks)
    quicktime_mhvd_init_video(file, &file->moov.mvhd, timescale);
  file->vtracks = realloc(file->vtracks, (file->total_vtracks+1) *
                          sizeof(*file->vtracks));
  memset(&file->vtracks[file->total_vtracks], 0, sizeof(*file->vtracks));
  
  if(ci)
    {
    lqt_compression_info_copy(&file->vtracks[file->total_vtracks].ci, ci);
    file->vtracks[file->total_vtracks].stream_cmodel = ci->colormodel;
    }
  trak = quicktime_add_track(file);
  file->vtracks[file->total_vtracks].track = trak;
  
  file->total_vtracks++;
        
  quicktime_trak_init_video(file, trak, frame_w, frame_h,
                            frame_duration, timescale, compressor);


  if(info)
    return lqt_set_video_codec(file, file->total_vtracks-1, info);
  
  return 0;
  }

int lqt_add_video_track(quicktime_t *file,
                        int frame_w, int frame_h,
                        int frame_duration, int timescale,
                        lqt_codec_info_t * info)
  {
  return lqt_add_video_track_internal(file,
                                      frame_w, frame_h,
                                      frame_duration, timescale,
                                      info, NULL);
  }

void quicktime_set_framerate(quicktime_t *file, double framerate)
  {
  int i;
  int new_time_scale, new_sample_duration;

  if(!file->wr)
    {
    lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
            "quicktime_set_framerate shouldn't be called in read mode.");
    return;
    }

  new_time_scale = quicktime_get_timescale(framerate);
  new_sample_duration = (int)((float)new_time_scale / framerate + 0.5);

  for(i = 0; i < file->total_vtracks; i++)
    {
    file->vtracks[i].track->mdia.mdhd.time_scale = new_time_scale;
    file->vtracks[i].track->mdia.minf.stbl.stts.table[0].sample_duration =
      new_sample_duration;
    }
  }

long quicktime_video_length(quicktime_t *file, int track)
  {
  /*printf("quicktime_video_length %d %d\n", quicktime_track_samples(file, file->vtracks[track].track), track); */
  if(file->total_vtracks > 0)
    return quicktime_track_samples(file, file->vtracks[track].track);
  return 0;
  }

long quicktime_video_position(quicktime_t *file, int track)
  {
  return file->vtracks[track].current_position;
  }


int quicktime_set_video_position(quicktime_t *file, int64_t frame, int track)
  {
#if 0
  int64_t chunk_sample, chunk;
  quicktime_trak_t *trak;
  quicktime_codec_t * codec;
  
  if((track < 0) || (track >= file->total_vtracks))
    return 0;

  trak = file->vtracks[track].track;

  if((frame < 0) || (frame >= quicktime_track_samples(file, trak)))
    return 0;
  
  file->vtracks[track].current_position = frame;
  quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, frame);
  file->vtracks[track].cur_chunk = chunk;
  
  file->vtracks[track].timestamp =
    quicktime_sample_to_time(&trak->mdia.minf.stbl.stts,
                             frame,
                             &file->vtracks[track].stts_index,
                             &file->vtracks[track].stts_count);
#else
  quicktime_video_map_t *vtrack;
  quicktime_trak_t *trak;
  quicktime_codec_t * codec;
  
  if((track < 0) || (track >= file->total_vtracks))
    return 0;

  vtrack = file->vtracks + track;
  trak = vtrack->track;
  
  if((frame < 0) || (frame >= trak->idx.num_entries))
    return 0;
  
  vtrack->timestamp = trak->idx.entries[frame].pts;
  trak->idx_pos = lqt_packet_index_get_keyframe_before(&trak->idx, frame);
  vtrack->next_display_frame = trak->idx_pos;

#endif

  /* Resync codec */
  if((codec = file->vtracks[track].codec) && codec->resync)
    codec->resync(file, track);
  return 0;
  }

void lqt_seek_video(quicktime_t * file, int track, int64_t time)
  {
#if 0
  int64_t frame;
  quicktime_trak_t *trak;

  if((track < 0) || (track >= file->total_vtracks))
    return;
  
  trak = file->vtracks[track].track;

  file->vtracks[track].timestamp = time;
  
  frame =
    quicktime_time_to_sample(&trak->mdia.minf.stbl.stts,
                             &file->vtracks[track].timestamp,
                             &file->vtracks[track].stts_index,
                             &file->vtracks[track].stts_count);
  
  quicktime_set_video_position(file, frame, track);
#else
  quicktime_trak_t *trak;
  int64_t pos;
  if((track < 0) || (track >= file->total_vtracks))
    return;
  trak = file->vtracks[track].track;
  pos = lqt_packet_index_seek(&trak->idx, time);
  if(pos >= 0)
    quicktime_set_video_position(file, pos, track);
#endif
  }


#define FRAME_PADDING 128

int lqt_read_video_frame(quicktime_t * file,
                         uint8_t ** buffer, int * buffer_alloc,
                         int64_t frame, int64_t * time, int track)
  {
  int64_t offset, chunk_sample, chunk;
  int result;
  quicktime_trak_t *trak;
  int len;
  
  if((track >= file->total_vtracks) || (track < 0))
    return 0;
  
  trak = file->vtracks[track].track;

  if((frame < 0) || (frame >= quicktime_track_samples(file, trak)))
    return 0;
  
  //  file->vtracks[track].current_position = frame;
  quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, frame);
  file->vtracks[track].cur_chunk = chunk;
  offset = quicktime_sample_to_offset(file, trak, frame);
  quicktime_set_position(file, offset);

  if(time)
    *time = quicktime_sample_to_time(&trak->mdia.minf.stbl.stts,
                                     frame,
                                     &file->vtracks[track].stts_index,
                                     &file->vtracks[track].stts_count);

  len = quicktime_frame_size(file, frame, track);
  
  if(len + FRAME_PADDING > *buffer_alloc)
    {
    *buffer_alloc = len + FRAME_PADDING + 1024;
    *buffer = realloc(*buffer, *buffer_alloc);
    }

  result = quicktime_read_data(file, *buffer, len);


  if(result < len)
    {
    return 0;
    }
  memset(*buffer + len, 0, FRAME_PADDING);
  return len;
  }

#undef FRAME_PADDING

int quicktime_has_video(quicktime_t *file)
  {
  if(quicktime_video_tracks(file)) return 1;
  return 0;
  }

int quicktime_video_width(quicktime_t *file, int track)
  {
  if((track < 0) || (track >= file->total_vtracks))
    return 0;
  return file->vtracks[track].track->mdia.minf.stbl.stsd.table->width;
  }

int quicktime_video_height(quicktime_t *file, int track)
  {
  if((track < 0) || (track >= file->total_vtracks))
    return 0;
  return file->vtracks[track].track->mdia.minf.stbl.stsd.table->height;
  }

int quicktime_video_depth(quicktime_t *file, int track)
  {
  if((track < 0) || (track >= file->total_vtracks))
    return 0;
  return file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].depth;
  }

void quicktime_set_cmodel(quicktime_t *file, int colormodel)
  {
  int i;
  for(i = 0; i < file->total_vtracks; i++)
    file->vtracks[i].io_cmodel = colormodel;
  }

int lqt_get_cmodel(quicktime_t * file, int track)
  {
  if((track < file->total_vtracks) && (track >= 0))
    return file->vtracks[track].io_cmodel;
  else
    return LQT_COLORMODEL_NONE;
  }


void lqt_set_cmodel(quicktime_t *file, int track, int colormodel)
  {
  if((track < file->total_vtracks) && (track >= 0))
    {
    file->vtracks[track].io_cmodel = colormodel;

    /* Maybe switch the encoding colormodel to better match the IO one. */
    if(file->wr && !file->encoding_started)
      {
      lqt_codec_info_t * info = file->vtracks[track].codec->info;
      int encoding_cmodel = lqt_get_best_target_colormodel(
              colormodel, info->encoding_colormodels);
      
      if (encoding_cmodel != LQT_COLORMODEL_NONE)
        {
        file->vtracks[track].stream_cmodel = encoding_cmodel;
        }
      }
    }
  else
    lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN, "lqt_set_cmodel: No track No. %d", track);
  }

void quicktime_set_row_span(quicktime_t *file, int row_span)
  {
  int i;
  for(i = 0; i < file->total_vtracks; i++)
    file->vtracks[i].io_row_span = row_span;
  }

void lqt_set_row_span(quicktime_t *file, int track, int row_span)
  {
  file->vtracks[track].io_row_span = row_span;
  }

void lqt_set_row_span_uv(quicktime_t *file, int track, int row_span_uv)
  {
  file->vtracks[track].io_row_span_uv = row_span_uv;
  }

void quicktime_set_depth(quicktime_t *file, int depth, int track)
  {
  int i;

  for(i = 0; i < file->total_vtracks; i++)
    {
    file->vtracks[i].track->mdia.minf.stbl.stsd.table[0].depth = depth;
    }
  }

double quicktime_frame_rate(quicktime_t *file, int track)
  {
  if(file->total_vtracks > track)
    {
    if(file->vtracks[track].track->mdia.minf.stbl.stts.table)
      return (float)file->vtracks[track].track->mdia.mdhd.time_scale / 
        file->vtracks[track].track->mdia.minf.stbl.stts.table[0].sample_duration;
    else
      return (float)file->vtracks[track].track->mdia.mdhd.time_scale / 
        file->vtracks[track].track->mdia.minf.stbl.stts.default_duration;
    }
  return 0;
  }

int64_t lqt_get_frame_time(quicktime_t * file, int track, int frame)
  {
  quicktime_video_map_t * vtrack;
  
  if((track < 0) || (track >= file->total_vtracks))
    return -1;
  vtrack = file->vtracks + track;
  return
    vtrack->track->idx.entries[frame].pts;
  }

/*
 *  Return the timestamp of the NEXT frame to be decoded.
 *  Call this BEFORE one of the decoding functions.
 */
  
int64_t lqt_frame_time(quicktime_t * file, int track)
  {
  quicktime_video_map_t * vtrack;
  
  if((track < 0) || (track >= file->total_vtracks))
    return -1;
  vtrack = file->vtracks + track;
  return vtrack->track->idx.entries[vtrack->next_display_frame].pts;
  }

/*
 *  Return the Duration of the entire track
 */

int64_t lqt_video_duration(quicktime_t * file, int track)
  {
  return file->vtracks[track].track->idx.max_pts;
  }


/*
 *  Get the timescale of the track. Divide the return values
 *  of lqt_frame_duration and lqt_frame_time by the scale to
 *  get the time in seconds.
 */
  
int lqt_video_time_scale(quicktime_t * file, int track)
  {
  if(file->total_vtracks <= track)
    return 0;
  return file->vtracks[track].track->mdia.mdhd.time_scale;
  }

/*
 *  Get the duration of the NEXT frame to be decoded.
 *  If constant is not NULL it will be set to 1 if the
 *  frame duration is constant throughout the whole track
 */

int lqt_frame_duration(quicktime_t * file, int track, int *constant)
  {
  quicktime_video_map_t * vtrack;
  if(file->total_vtracks <= track)
    return 0;

  vtrack = file->vtracks + track;
  
  if(constant)
    {
    if(vtrack->track->idx.max_packet_duration ==
       vtrack->track->idx.min_packet_duration)
      *constant = 1;
    }
  return vtrack->track->idx.entries[vtrack->next_display_frame].duration;
  }


char* quicktime_video_compressor(quicktime_t *file, int track)
  {
  if ((track < 0) || (track >= file->total_vtracks))
    return NULL;
  return file->vtracks[track].track->mdia.minf.stbl.stsd.table[0].format;
  }


int quicktime_write_frame(quicktime_t *file,
                          unsigned char *video_buffer,
                          int64_t bytes, int track)
  {
  int result = 0;
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  
  lqt_video_append_timestamp(file, track, vtrack->timestamp,
                             file->vtracks[track].track->mdia.minf.stbl.stts.default_duration);
  vtrack->timestamp += file->vtracks[track].track->mdia.minf.stbl.stts.default_duration;

  lqt_write_frame_header(file, track, file->vtracks[track].current_position,
                         -1, 0 /* int keyframe */ );
  
  result = !quicktime_write_data(file, video_buffer, bytes);

  lqt_write_frame_footer(file, track);
  
  if(file->vtracks[track].timecode_track)
    lqt_flush_timecode(file, track,
                       file->vtracks[track].current_position*
                       (int64_t)file->vtracks[track].track->mdia.minf.stbl.stts.default_duration, 0);
  
  file->vtracks[track].current_position++;
  return result;
  }

long quicktime_frame_size(quicktime_t *file, long frame, int track)
  {
  long bytes = 0;
  quicktime_trak_t *trak = file->vtracks[track].track;

  if(trak->mdia.minf.stbl.stsz.sample_size)
    {
    bytes = trak->mdia.minf.stbl.stsz.sample_size;
    }
  else
    {
    long total_frames = quicktime_track_samples(file, trak);
    if(frame < 0) frame = 0;
    else
      if(frame > total_frames - 1) frame = total_frames - 1;
    bytes = trak->mdia.minf.stbl.stsz.table[frame].size;
    }


  return bytes;
  }


int quicktime_read_frame_init(quicktime_t *file, int track)
  {
  int64_t offset;
  quicktime_trak_t *trak = file->vtracks[track].track;
  offset = quicktime_sample_to_offset(file, trak, file->vtracks[track].current_position);
  quicktime_set_position(file, offset);
  
  if(quicktime_ftell(file) != file->file_position) 
    {
    fseeko(file->stream, file->file_position, SEEK_SET);
    file->ftell_position = file->file_position;
    }
  return 0;
  }

int quicktime_read_frame_end(quicktime_t *file, int track)
  {
  file->file_position = quicktime_ftell(file);
  file->vtracks[track].current_position++;
  return 0;
  }


long quicktime_read_frame(quicktime_t *file, unsigned char *video_buffer, int track)
  {
  int64_t bytes, offset, chunk_sample, chunk;
  int result = 0;
  quicktime_trak_t *trak = file->vtracks[track].track;
    
  bytes = quicktime_frame_size(file, file->vtracks[track].current_position, track);

  quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, file->vtracks[track].current_position);
  file->vtracks[track].cur_chunk = chunk;
  offset = quicktime_sample_to_offset(file, trak, file->vtracks[track].current_position);
  quicktime_set_position(file, offset);
          
  result = quicktime_read_data(file, video_buffer, bytes);
  lqt_update_frame_position(&file->vtracks[track]);

  if(!result) return 0;
  return bytes;
  }

long quicktime_get_keyframe_before(quicktime_t *file, long frame, int track)
  {
  quicktime_trak_t *trak = file->vtracks[track].track;
  quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
  int i;
  
  // Offset 1
  frame++;
  
  for(i = stss->total_entries - 1; i >= 0; i--)
    {
    if(stss->table[i].sample <= frame) return stss->table[i].sample - 1;
    }

  return 0;
  }

long quicktime_get_partial_keyframe_before(quicktime_t *file, long frame, int track)
  {
  quicktime_trak_t *trak = file->vtracks[track].track;
  quicktime_stps_t *stps = &trak->mdia.minf.stbl.stps;
  int i;

  // Offset 1
  frame++;

  for(i = stps->total_entries - 1; i >= 0; i--)
    {
    if(stps->table[i].sample <= frame) return stps->table[i].sample - 1;
    }

  return 0;
  }

void quicktime_insert_keyframe(quicktime_t *file, long frame, int track)
  {
  quicktime_trak_t *trak = file->vtracks[track].track;
  quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;

  if(file->file_type & (LQT_FILE_AVI|LQT_FILE_AVI_ODML))
    {
    // Set keyframe flag in idx1 table.
    if(file->total_riffs == 1)
      quicktime_set_idx1_keyframe(file, trak, frame);
    // Set keyframe flag in indx table.
    if(file->file_type == LQT_FILE_AVI_ODML)
      {
      quicktime_set_indx_keyframe(file, trak, frame);
      }
    }
  
  // Expand table
  if(stss->entries_allocated <= stss->total_entries)
    {
    stss->entries_allocated += 1024;
    stss->table = realloc(stss->table,
                          sizeof(*stss->table) *
                          stss->entries_allocated);
    }
  
  stss->table[stss->total_entries].sample = frame+1;
  stss->total_entries++;
  }

void quicktime_insert_partial_keyframe(quicktime_t *file, long frame, int track)
  {
  quicktime_trak_t *trak = file->vtracks[track].track;
  quicktime_stps_t *stps = &trak->mdia.minf.stbl.stps;

  if(file->file_type & (LQT_FILE_AVI|LQT_FILE_AVI_ODML))
    {
    // AVI doesn't support partial keyframes.
    return;
    }

  // Expand table
  if(stps->entries_allocated <= stps->total_entries)
    {
    stps->entries_allocated += 1024;
    stps->table = realloc(stps->table,
                          sizeof(*stps->table) *
                          stps->entries_allocated);
    }

  stps->table[stps->total_entries].sample = frame+1;
  stps->total_entries++;
  }

int quicktime_has_keyframes(quicktime_t *file, int track)
  {
  quicktime_trak_t *trak = file->vtracks[track].track;

  if(trak->idx.num_entries == trak->idx.num_key_frames)
    return 0;
  else
    return 1;
  }

int lqt_is_keyframe(quicktime_t *file, int track, int frame)
  {
  int i;
  quicktime_stss_t *stss = &file->vtracks[track].track->mdia.minf.stbl.stss;

  if(!stss->total_entries)
    return 1;

  frame++;
  
  for(i = 0; i < stss->total_entries; i++)
    {
    if(stss->table[i].sample == frame)
      return 1;
    }
  return 0;
  }
void lqt_update_frame_position(quicktime_video_map_t * track)
  {
  track->timestamp +=
    track->track->mdia.minf.stbl.stts.table[track->stts_index].sample_duration;

  track->stts_count++;

  if(track->stts_count >=
     track->track->mdia.minf.stbl.stts.table[track->stts_index].sample_count)
    {
    track->stts_index++;
    track->stts_count = 0;
    }

  if(track->track->mdia.minf.stbl.has_ctts)
    {
    track->ctts_count++;

    if(track->ctts_count >=
       track->track->mdia.minf.stbl.ctts.table[track->ctts_index].sample_count)
      {
      track->ctts_index++;
      track->ctts_count = 0;
      }
    
    }
  
  track->current_position++;

  /* Maybe that's the only thing we need */

  track->next_display_frame =
    lqt_packet_index_get_next_display_frame(&track->track->idx,
                                            track->next_display_frame);
  }

/* Set the io_rowspan for the case the user didn't. */

static void set_default_rowspan(quicktime_t *file, int track)
  {
  if(file->vtracks[track].io_row_span)
    return;

  lqt_get_default_rowspan(file->vtracks[track].io_cmodel,
                          quicktime_video_width(file, track),
                          &file->vtracks[track].io_row_span,
                          &file->vtracks[track].io_row_span_uv);
  }

/*
 *  Same as quicktime_decode_video but doesn't force BC_RGB888
 */

int lqt_decode_video(quicktime_t *file,
                     unsigned char **row_pointers, int track)
  {
  int result;
  int height;
  int width;
  set_default_rowspan(file, track);
        
  height = quicktime_video_height(file, track);
  width =  quicktime_video_width(file, track);
        
  if(file->vtracks[track].io_cmodel != file->vtracks[track].stream_cmodel)
    {
    if(!file->vtracks[track].temp_frame)
      {
      file->vtracks[track].temp_frame =
        lqt_rows_alloc(width, height, file->vtracks[track].stream_cmodel,
                       &file->vtracks[track].stream_row_span,
                       &file->vtracks[track].stream_row_span_uv);
      }
    result =
      file->vtracks[track].codec->decode_video(file,
                                               file->vtracks[track].temp_frame,
                                               track);
    cmodel_transfer(row_pointers,                    //    unsigned char **output_rows, /* Leave NULL if non existent */
                    file->vtracks[track].temp_frame, //    unsigned char **input_rows,
                    0, //                                  int in_x,        /* Dimensions to capture from input frame */
                    0, //                                  int in_y, 
                    width, //                              int in_w, 
                    height + file->vtracks[track].height_extension, // int in_h,
                    width, //                              int out_w,
                    height + file->vtracks[track].height_extension, // int out_h,
                    file->vtracks[track].stream_cmodel, // int in_colormodel,
                    file->vtracks[track].io_cmodel,     // int out_colormodel,
                    file->vtracks[track].stream_row_span,   /* For planar use the luma rowspan */
                    file->vtracks[track].io_row_span,       /* For planar use the luma rowspan */
                    file->vtracks[track].stream_row_span_uv, /* Chroma rowspan */
                    file->vtracks[track].io_row_span_uv      /* Chroma rowspan */);
         
    }
  else
    {
    file->vtracks[track].io_row_span    = file->vtracks[track].stream_row_span;
    file->vtracks[track].io_row_span_uv = file->vtracks[track].stream_row_span_uv;
          
    result = file->vtracks[track].codec->decode_video(file, row_pointers, track);
          
    }
  
  lqt_update_frame_position(&file->vtracks[track]);
  return result;
  }

/* The original function, which forces BG_RGB888 */
int quicktime_decode_video(quicktime_t *file,
                           unsigned char **row_pointers, int track)
  {
  
  file->vtracks[track].io_cmodel = BC_RGB888;
  return lqt_decode_video(file, row_pointers, track);
  }


long quicktime_decode_scaled(quicktime_t *file, 
                             int in_x,                    /* Location of input frame to take picture */
                             int in_y,
                             int in_w,
                             int in_h,
                             int out_w,                   /* Dimensions of output frame */
                             int out_h,
                             int color_model,             /* One of the color models defined above */
                             unsigned char **row_pointers, 
                             int track)
  {
  int result;

  int height;
  int width;
        
  set_default_rowspan(file, track);
  height = quicktime_video_height(file, track);
  width =  quicktime_video_width(file, track);

        
  file->vtracks[track].io_cmodel = color_model;

  if(!file->vtracks[track].temp_frame)
    {
    file->vtracks[track].temp_frame =
      lqt_rows_alloc(width, height, file->vtracks[track].stream_cmodel,
                     &file->vtracks[track].stream_row_span,
                     &file->vtracks[track].stream_row_span_uv);
    }
  result =
    file->vtracks[track].codec->decode_video(file,
                                             file->vtracks[track].temp_frame,
                                             track);
  cmodel_transfer(row_pointers,                    //    unsigned char **output_rows, /* Leave NULL if non existent */
                  file->vtracks[track].temp_frame, //    unsigned char **input_rows,
                  in_x, //                               int in_x,        /* Dimensions to capture from input frame */
                  in_y, //                               int in_y, 
                  in_w, //                               int in_w, 
                  in_h, //                               int in_h,
                  out_w, //                              int out_w, 
                  out_h, //                              int out_h,
                  file->vtracks[track].stream_cmodel, // int in_colormodel, 
                  file->vtracks[track].io_cmodel,     // int out_colormodel,
                  file->vtracks[track].stream_row_span,   /* For planar use the luma rowspan */
                  file->vtracks[track].io_row_span,       /* For planar use the luma rowspan */
                  file->vtracks[track].stream_row_span_uv, /* Chroma rowspan */
                  file->vtracks[track].io_row_span_uv      /* Chroma rowspan */);
        
  lqt_update_frame_position(&file->vtracks[track]);
  return result;
  }

int lqt_set_video_pass(quicktime_t *file,
                       int pass, int total_passes, 
                       const char * stats_file, int track)
  {
  if(file->vtracks[track].codec->set_pass)
    return file->vtracks[track].codec->set_pass(file,
                                                track,
                                                pass,
                                                total_passes,
                                                stats_file);
  else
    return 0;
  }


static int do_encode_video(quicktime_t *file, 
                           unsigned char **row_pointers, 
                           int track)
  {
  int result;

  int height;
  int width;

  lqt_start_encoding(file);
  
  set_default_rowspan(file, track);
  height = quicktime_video_height(file, track);
  width =  quicktime_video_width(file, track);
  
  if(file->vtracks[track].io_cmodel != file->vtracks[track].stream_cmodel)
    {
    if(!file->vtracks[track].temp_frame)
      {
      file->vtracks[track].temp_frame =
        lqt_rows_alloc(width, height + file->vtracks[track].height_extension,
                       file->vtracks[track].stream_cmodel,
                       &file->vtracks[track].stream_row_span,
                       &file->vtracks[track].stream_row_span_uv);
      }
    cmodel_transfer(file->vtracks[track].temp_frame, //    unsigned char **output_rows, /* Leave NULL if non existent */
                    row_pointers,                    //    unsigned char **input_rows,
                    0, //                                  int in_x,        /* Dimensions to capture from input frame */
                    0, //                                  int in_y, 
                    width, //                              int in_w, 
                    height + file->vtracks[track].height_extension, // int in_h,
                    width, //                              int out_w, 
                    height + file->vtracks[track].height_extension, // int out_h,
                    file->vtracks[track].io_cmodel, // int in_colormodel, 
                    file->vtracks[track].stream_cmodel,     // int out_colormodel,
                    file->vtracks[track].io_row_span,   /* For planar use the luma rowspan */
                    file->vtracks[track].stream_row_span,       /* For planar use the luma rowspan */
                    file->vtracks[track].io_row_span_uv, /* Chroma rowspan */
                    file->vtracks[track].stream_row_span_uv      /* Chroma rowspan */);
    result = file->vtracks[track].codec->encode_video(file, file->vtracks[track].temp_frame, track);
    }
  else
    {
    file->vtracks[track].stream_row_span    = file->vtracks[track].io_row_span;
    file->vtracks[track].stream_row_span_uv = file->vtracks[track].io_row_span_uv;
    result = file->vtracks[track].codec->encode_video(file, row_pointers, track);
    }
  return result;
  }

void lqt_write_frame_header(quicktime_t * file, int track,
                            int pic_num1, int64_t pic_pts,
                            enum LqtKeyFrame keyframe)
  {
  quicktime_video_map_t * vtrack = &file->vtracks[track];
  quicktime_trak_t * trak = vtrack->track;
  int pic_num = -1;
  int i;
  
  // fprintf(stderr, "Write frame header %d %ld\n", pic_num1, pic_pts);
  
  if(pic_num1 >= 0)
    pic_num = pic_num1;
  else
    {
    /* We start at current_position because this isn't incremented by now */
    for(i = vtrack->current_position; i >= 0; i--)
      {
      if(vtrack->timestamps[i] == pic_pts)
        {
        pic_num = i;
        break;
        }
      }
    }

  //  if(pic_num < 0)
  //    fprintf(stderr, "Picture number not found\n");
  
  if(vtrack->cur_chunk >= vtrack->picture_numbers_alloc)
    {
    vtrack->picture_numbers_alloc += 1024;
    vtrack->picture_numbers = realloc(vtrack->picture_numbers,
                                      sizeof(*vtrack->picture_numbers) *
                                      vtrack->picture_numbers_alloc);
    }
  vtrack->picture_numbers[vtrack->cur_chunk] = pic_num;
  vtrack->keyframe = keyframe;
  
  quicktime_write_chunk_header(file, trak);
  }


void lqt_write_frame_footer(quicktime_t * file, int track)
  {
  quicktime_video_map_t * vtrack = &file->vtracks[track];
  quicktime_trak_t * trak = vtrack->track;

  trak->chunk_samples = 1;
  quicktime_write_chunk_footer(file, trak);
  
  if(vtrack->keyframe == LQT_FULL_KEY_FRAME)
    quicktime_insert_keyframe(file, vtrack->cur_chunk, track);
  else if(vtrack->keyframe == LQT_PARTIAL_KEY_FRAME)
    quicktime_insert_partial_keyframe(file, vtrack->cur_chunk, track);

  vtrack->cur_chunk++;
  }

void lqt_video_build_timestamp_tables(quicktime_t * file, int track)
  {
  int i;
  quicktime_video_map_t * vtrack = &file->vtracks[track];
  quicktime_trak_t * trak = vtrack->track;
  quicktime_stts_t * stts;
  quicktime_ctts_t * ctts;
  quicktime_stts_table_t * stts_tab;
  int64_t dts;
  int has_b_frames = 0;
  
  /* If all frames are keyframes, disable stss */
  if(trak->mdia.minf.stbl.stss.total_entries == vtrack->cur_chunk)
    trak->mdia.minf.stbl.stss.total_entries = 0;
  
  /* If we have no timestamp arrays (e.g. if stream was written compressed)
     return here */

  if(!vtrack->picture_numbers)
    return;
  
  stts = &trak->mdia.minf.stbl.stts;
  ctts = &trak->mdia.minf.stbl.ctts;
  
#if 0
  fprintf (stderr, "Build timestamp tables %ld frames %d\n",
           vtrack->cur_chunk, stts->default_duration);
  for(i = 0; i < vtrack->cur_chunk; i++)
    {
    fprintf(stderr, "PTS[%d]: %ld\n", i, vtrack->timestamps[i]);
    }
  for(i = 0; i < vtrack->cur_chunk; i++)
    {
    fprintf(stderr, "pic_num[%d]: %d\n", i, vtrack->picture_numbers[i]);
    }
#endif
  /* Check if we have B-frames */
  for(i = 0; i < vtrack->cur_chunk-1; i++)
    {
    if(vtrack->picture_numbers[i] + 1 !=
       vtrack->picture_numbers[i+1])
      {
      has_b_frames = 1;
      break;
      }
    }
  
  /* Build preliminary stts */

  if(stts->table)
    {
    free(stts->table);
    stts->table = NULL;
    }
  stts->total_entries = vtrack->cur_chunk;

  if(!stts->total_entries)
    return;
  
  stts->table = malloc(vtrack->cur_chunk * sizeof(*stts->table));
  
  for(i = 0; i < vtrack->cur_chunk-1; i++)
    {
    stts->table[i].sample_count = 1;
    stts->table[i].sample_duration =
      vtrack->timestamps[i+1] - vtrack->timestamps[i];
    }

  /* Last entry */
  stts->table[vtrack->cur_chunk-1].sample_count = 1;
  stts->table[vtrack->cur_chunk-1].sample_duration =
    vtrack->duration - vtrack->timestamps[vtrack->cur_chunk-1];

  if(stts->table[vtrack->cur_chunk-1].sample_duration <= 0)
    stts->table[vtrack->cur_chunk-1].sample_duration = stts->default_duration;
  
  /* If we have no B-frames, exit here */
  if(!has_b_frames)
    return;

  /* If we have B-frames, reorder stts and build ctts */
  
  stts_tab = malloc(vtrack->cur_chunk * sizeof(*stts_tab));
  ctts->table = malloc(vtrack->cur_chunk * sizeof(*ctts->table));
  ctts->total_entries = vtrack->cur_chunk;
  trak->mdia.minf.stbl.has_ctts = 1;

  dts = 0;
  /* Loop over *coded* pictures */
  for(i = 0; i < vtrack->cur_chunk; i++)
    {
    stts_tab[i].sample_duration = stts->table[vtrack->picture_numbers[i]].sample_duration;
    stts_tab[i].sample_count = 1;

    /* CTS = PTS - DTS */
    ctts->table[i].sample_count = 1;
    ctts->table[i].sample_duration     = vtrack->timestamps[vtrack->picture_numbers[i]] - dts;
    dts += stts_tab[i].sample_duration;
    }
  
  free(stts->table);
  stts->table = stts_tab;
  }

void lqt_video_append_timestamp(quicktime_t * file, int track,
                                int64_t time, int duration)
  {
  quicktime_video_map_t * vtrack = &file->vtracks[track];
  /* Update timestamp table */

  //  fprintf(stderr, "lqt_video_append_timestamp: %ld %d\n",
  //          time, duration);

  if(vtrack->current_position >= vtrack->timestamps_alloc)
    {
    vtrack->timestamps_alloc += 1024;
    vtrack->timestamps = realloc(vtrack->timestamps,
                                 vtrack->timestamps_alloc *
                                 sizeof(*vtrack->timestamps));
    }
  vtrack->timestamps[vtrack->current_position] = time;
  vtrack->duration = time + duration;
  }


int lqt_encode_video(quicktime_t *file, 
                     unsigned char **row_pointers, 
                     int track, int64_t time)
  {
  return lqt_encode_video_d(file, row_pointers, track, time, -1);
  }

int lqt_encode_video_d(quicktime_t *file, 
                       unsigned char **row_pointers, 
                       int track, int64_t time, int duration)
  {
  int result;

  quicktime_video_map_t * vtrack = &file->vtracks[track];

  /* Must set valid timestamp for encoders */
  vtrack->timestamp = time;
  lqt_video_append_timestamp(file, track, time, duration);
  
  result = do_encode_video(file, row_pointers, track);
  if (result)
    return(result);

  if(file->io_error)
    return 1;

  if(file->vtracks[track].timecode_track)
    lqt_flush_timecode(file, track, time, 0);
  
  vtrack->current_position++;

  /* vtrack->current_position now points past the last frame
     we were asked to encode. If that frame was the last one
     but flushing the codec produces more output, there will be
     read access to vtrack->timestamps[vtrack->current_position]
     from lqt_write_frame_header(), therefore we need to make sure
     it's a valid address pointing to initialized memory. */
  if(vtrack->current_position >= vtrack->timestamps_alloc)
    {
    vtrack->timestamps_alloc += 1024;
    vtrack->timestamps = realloc(vtrack->timestamps,
                                 vtrack->timestamps_alloc *
                                 sizeof(*vtrack->timestamps));
    }
  vtrack->timestamps[vtrack->current_position] = -1;
  return 0;
  }

int quicktime_encode_video(quicktime_t *file, 
                           unsigned char **row_pointers, 
                           int track)
  {
  int result;
  
  result = lqt_encode_video_d(file, 
                              row_pointers, 
                              track, file->vtracks[track].timestamp,
                              file->vtracks[track].track->mdia.minf.stbl.stts.default_duration);
  file->vtracks[track].timestamp +=
    file->vtracks[track].track->mdia.minf.stbl.stts.default_duration;
  return result;
  }

int quicktime_reads_cmodel(quicktime_t *file, 
                           int colormodel, 
                           int track)
  {
  return lqt_colormodel_has_conversion(file->vtracks[track].stream_cmodel, colormodel);
  }

int quicktime_writes_cmodel(quicktime_t *file, 
                            int colormodel, 
                            int track)
  {
  return lqt_colormodel_has_conversion(colormodel, file->vtracks[track].stream_cmodel);
  }
