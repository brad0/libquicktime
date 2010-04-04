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
#include <string.h>

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

const lqt_compression_info_t *
lqt_get_audio_compression_info(quicktime_t * file, int track)
  {
  quicktime_audio_map_t * atrack;

  atrack = &file->atracks[track];
  
  if(atrack->ci.id == LQT_COMPRESSION_NONE)
    return NULL;
  else
    return &atrack->ci;
  }

const lqt_compression_info_t *
lqt_get_video_compression_info(quicktime_t * file, int track)
  {
  quicktime_video_map_t * vtrack;

  vtrack = &file->vtracks[track];
  
  if(vtrack->ci.id == LQT_COMPRESSION_NONE)
    return NULL;
  else
    {
    if(!vtrack->ci.width)
      {
      vtrack->ci.width = quicktime_video_width(file, track);
      vtrack->ci.height = quicktime_video_height(file, track);
      lqt_get_pixel_aspect(file, track,
                           &vtrack->ci.pixel_width,
                           &vtrack->ci.pixel_height);
      vtrack->ci.colormodel = vtrack->stream_cmodel;
      vtrack->ci.video_timescale = lqt_video_time_scale(file, track);

      if(vtrack->track->mdia.minf.stbl.stss.total_entries)
        vtrack->ci.flags |= LQT_COMPRESSION_HAS_P_FRAMES;
      if(vtrack->track->mdia.minf.stbl.ctts.total_entries)
        vtrack->ci.flags |= LQT_COMPRESSION_HAS_B_FRAMES;
      }
    return &vtrack->ci;
    }
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

int lqt_writes_audio_compressed(quicktime_t * file,
                                const lqt_compression_info_t * ci,
                                lqt_codec_info_t * codec_info)
  {
  int ret;
  quicktime_codec_t * codec;

  if(codec_info->compression_id != ci->id)
    return 0;
  
  return 0;
  }

int lqt_writes_video_compressed(quicktime_t * file,
                                const lqt_compression_info_t * ci,
                                lqt_codec_info_t * codec_info)
  {
  int ret;
  quicktime_codec_t * codec;
  
  if(codec_info->compression_id != ci->id)
    return 0;
  return 0;
  }

int lqt_add_audio_track_compressed(quicktime_t * file,
                                   const lqt_compression_info_t * info)
  {
  return 0;
  }

int lqt_add_video_track_compressed(quicktime_t * file,
                                   const lqt_compression_info_t * info)
  {
  return 0;
  }

int lqt_write_audio_packet(quicktime_t * file,
                           lqt_packet_t * p, int track)
  {
  return 0;
  }

int lqt_write_video_packet(quicktime_t * file,
                           lqt_packet_t * p, int track)
  {
  return 0;
  }

static const struct
  {
  lqt_compression_id_t id;
  const char * name;
  }
compression_ids[] =  
  {
    /* Audio */
    { LQT_COMPRESSION_ALAW,      "alaw"    },
    { LQT_COMPRESSION_ULAW,      "ulaw"    },
    { LQT_COMPRESSION_MP3,       "mp3" },
    { LQT_COMPRESSION_AC3,       "ac3"     },
    { LQT_COMPRESSION_AAC,       "aac"     },

    /* Video */
    { LQT_COMPRESSION_JPEG,      "jpeg"    },
    { LQT_COMPRESSION_PNG,       "png"     },
    { LQT_COMPRESSION_TIFF,      "tiff"    },
    { LQT_COMPRESSION_TGA,       "tga"     },
    { LQT_COMPRESSION_MPEG4_ASP, "mpeg4"   },
    { LQT_COMPRESSION_H264,      "h264"    },
    { LQT_COMPRESSION_DIRAC,     "dirac"   },
    { LQT_COMPRESSION_D10,       "d10"     },
  };
  
const char * lqt_compression_id_to_string(lqt_compression_id_t id)
  {
  int i;
  for(i = 0; i < sizeof(compression_ids)/sizeof(compression_ids[0]); i++)
    {
    if(compression_ids[i].id == id)
      return compression_ids[i].name;
    }
  return NULL;
  }

lqt_compression_id_t lqt_compression_id_from_string(const char * str)
  {
  int i;
  for(i = 0; i < sizeof(compression_ids)/sizeof(compression_ids[0]); i++)
    {
    if(!strcmp(compression_ids[i].name, str))
      return compression_ids[i].id;
    }
  return LQT_COMPRESSION_NONE;
  }

void lqt_compression_info_dump(const lqt_compression_info_t * ci)
  {
  int is_video = (ci->id >= 0x10000);

  lqt_dump("%s compression info\n", (is_video ? "Video" : "Audio"));
  lqt_dump("  Codec:       %s\n", lqt_compression_id_to_string(ci->id));

  if(ci->bitrate)
    {
    if(ci->bitrate < 0)
      lqt_dump("  Bitrate:     Variable\n");
    else
      lqt_dump("  Bitrate:     %d\n", ci->bitrate);
    }
  if(is_video)
    {
    lqt_dump("  Image size:  %d x %d\n", ci->width, ci->height);
    lqt_dump("  Pixel size:  %d x %d\n", ci->pixel_width, ci->pixel_height);
    lqt_dump("  Colormodel:  %s\n", lqt_colormodel_to_string(ci->colormodel));
    lqt_dump("  Frame types: I");
    if(ci->flags & LQT_COMPRESSION_HAS_P_FRAMES)
      lqt_dump(", P");
    if(ci->flags & LQT_COMPRESSION_HAS_B_FRAMES)
      lqt_dump(", B");
    lqt_dump("\n");
    }
  }
