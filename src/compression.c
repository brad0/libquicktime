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

#include <lqt_private.h>
#include <quicktime/lqt_codecapi.h>

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

  if(!atrack->ci.samplerate)
    {
    atrack->ci.samplerate = quicktime_sample_rate(file, track);
    atrack->ci.num_channels = quicktime_track_channels(file, track);
    }
  return &atrack->ci;
  }

const lqt_compression_info_t *
lqt_get_video_compression_info(quicktime_t * file, int track)
  {
  quicktime_video_map_t * vtrack;

  vtrack = &file->vtracks[track];
  
  if(vtrack->ci.id == LQT_COMPRESSION_NONE)
    return NULL;

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

int lqt_read_audio_packet(quicktime_t * file, lqt_packet_t * p, int track)
  {
  quicktime_audio_map_t *atrack = &file->atracks[track];

  if(atrack->block_align)
    {
    int bytes_from_samples;
    p->data_len =
      lqt_read_audio_chunk(file, track, atrack->cur_chunk,
                           &p->data, &p->data_alloc, &p->duration);

    if(!p->data_len)
      return 0;
    
    bytes_from_samples = p->duration * atrack->block_align;
    if(p->data_len > bytes_from_samples)
      p->data_len = bytes_from_samples;

    p->timestamp = atrack->current_position;
    atrack->current_position += p->duration;
    atrack->cur_chunk++;
    return 1;
    }
  return 0;
  }

int lqt_read_video_packet(quicktime_t * file, lqt_packet_t * p, int track)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];

  p->data_len =
    lqt_read_video_frame(file, &p->data, &p->data_alloc,
                         vtrack->current_position, NULL, track);
  if(!p->data_len)
    return 0;

  p->flags = 0;
  if(lqt_is_keyframe(file, track, vtrack->current_position))
    p->flags |= LQT_PACKET_KEYFRAME;
  
  p->timestamp = vtrack->timestamp;
  p->duration  = vtrack->track->mdia.minf.stbl.stts.table[vtrack->stts_index].sample_duration;
  
  lqt_update_frame_position(vtrack);
  
  return 1;
  }

/* Writing */

int lqt_writes_compressed(lqt_file_type_t type,
                          const lqt_compression_info_t * ci,
                          lqt_codec_info_t * codec_info)
  {
  int ret = 0;
  quicktime_codec_t * codec;
  
  if(codec_info->compression_id != ci->id)
    return 0;

  codec = quicktime_load_codec(codec_info, NULL, NULL);

  if(!codec->writes_compressed ||
     codec->writes_compressed(type, ci))
    ret = 1;

  quicktime_delete_codec(codec);
  
  return ret;
  }

int lqt_add_audio_track_compressed(quicktime_t * file,
                                   const lqt_compression_info_t * ci,
                                   lqt_codec_info_t * codec_info)
  {
  return lqt_add_audio_track_internal(file,
                                      ci->num_channels, ci->samplerate, 16,
                                      codec_info, ci);
  }

int lqt_add_video_track_compressed(quicktime_t * file,
                                   const lqt_compression_info_t * ci,
                                   lqt_codec_info_t * codec_info)
  {
  return lqt_add_video_track_internal(file,
                                      ci->width, ci->height,
                                      0, ci->video_timescale,
                                      codec_info, ci);
  }

int lqt_write_audio_packet(quicktime_t * file,
                           lqt_packet_t * p, int track)
  {
  quicktime_audio_map_t *atrack = &file->atracks[track];

  if(atrack->codec->write_packet)
    return atrack->codec->write_packet(file, p, track);
  else
    {
    int result;
    quicktime_write_chunk_header(file, atrack->track);
    result = quicktime_write_data(file, p->data, p->data_len);

    atrack->track->chunk_samples = p->duration;
    quicktime_write_chunk_footer(file, atrack->track);
    atrack->cur_chunk++;
    if(result)
      return 0;
    else
      return 1;
    }
  
  return 0;
  }

int lqt_write_video_packet(quicktime_t * file,
                           lqt_packet_t * p, int track)
  {
  int result;
  quicktime_video_map_t *vtrack = &file->vtracks[track];
  
  /* Must set valid timestamp for encoders */
  vtrack->timestamp = p->timestamp;
  lqt_video_append_timestamp(file, track, p->timestamp, p->duration);
    
  if(vtrack->codec->write_packet)
    return vtrack->codec->write_packet(file, p, track);
  else
    {
    lqt_write_frame_header(file, track,
                           -1, p->timestamp, !!(p->flags & LQT_PACKET_KEYFRAME));
    
    result = !quicktime_write_data(file, p->data, p->data_len);
    lqt_write_frame_footer(file, track);
    }

  vtrack->current_position++;
  
  return result;
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
  else
    {
    lqt_dump("  Samplerate:  %d\n", ci->samplerate);
    lqt_dump("  Channels:    %d\n", ci->num_channels);
    lqt_dump("  SBR:         %s\n",
            (ci->flags & LQT_COMPRESSION_SBR ? "Yes" : "No"));
    }
  }

void lqt_compression_info_copy(lqt_compression_info_t * dst,
                               const lqt_compression_info_t * src)
  {
  memcpy(dst, src, sizeof(*dst));
  if(dst->global_header)
    {
    dst->global_header = malloc(dst->global_header_len);
    memcpy(dst->global_header, src->global_header, dst->global_header_len);
    }
  }

void lqt_compression_info_set_header(lqt_compression_info_t * info,
                                     uint8_t * header,
                                     int header_len)
  {
  info->global_header = malloc(header_len);
  memcpy(info->global_header, header, header_len);
  info->global_header_len = header_len;
  }


void lqt_packet_dump(const lqt_packet_t * p)
  {
  lqt_dump("Packet: %d bytes, Time: %"PRId64", Duration: %d, Keyframe: %d\n",
           p->data_len, p->timestamp, p->duration, !!(p->flags & LQT_PACKET_KEYFRAME));
  lqt_hexdump(p->data, p->data_len > 16 ? 16 : p->data_len, 16);
  }
