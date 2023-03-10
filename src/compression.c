/*******************************************************************************
 compression.c

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

#include <lqt_private.h>

#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

void lqt_compression_info_free(lqt_compression_info_t * info)
  {
  if(info->global_header)
    free(info->global_header);
  }

void lqt_packet_alloc(lqt_packet_t * p, int bytes)
  {
  if(p->data_alloc < bytes + LQT_PACKET_PADDING)
    {
    p->data_alloc = bytes  + LQT_PACKET_PADDING + 1024;
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

  if(!lqt_ensure_stream_cmodel_decode(file, track))
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

  if(atrack->codec->read_packet)
    return atrack->codec->read_packet(file, p, track);
  else
    {
    if(!quicktime_trak_read_packet(file, atrack->track, p))
      return 0;
    }
  return 0;
  }

int lqt_read_video_packet(quicktime_t * file, lqt_packet_t * p, int track)
  {
  quicktime_video_map_t *vtrack = &file->vtracks[track];

  if(vtrack->codec->read_packet)
    {
    if(!vtrack->codec->read_packet(file, p, track))
      return 0;
    }
  else
    {
    if(!quicktime_trak_read_packet(file, vtrack->track, p))
      return 0;
    }
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
  quicktime_audio_map_t *atrack;

  int samplerate = ci->samplerate;
  if(ci->flags & LQT_COMPRESSION_SBR)
    samplerate /= 2;
  
  if(lqt_add_audio_track_internal(file,
                                  ci->num_channels, samplerate, 16,
                                  codec_info, ci))
    return 1;

  atrack = &file->atracks[file->total_atracks-1];

  if(atrack->codec->init_compressed)
    atrack->codec->init_compressed(file, file->total_atracks-1);
  return 0;
  }

int lqt_add_video_track_compressed(quicktime_t * file,
                                   const lqt_compression_info_t * ci,
                                   lqt_codec_info_t * codec_info)
  {
  quicktime_video_map_t *vtrack;
  if(lqt_add_video_track_internal(file,
                                  ci->width, ci->height,
                                  0, ci->video_timescale,
                                  codec_info, ci))
    return 1;

  vtrack = &file->vtracks[file->total_vtracks-1];

  if(vtrack->ci.flags & LQT_COMPRESSION_HAS_B_FRAMES)
    vtrack->track->mdia.minf.stbl.has_ctts = 1;
  
  if(vtrack->codec->init_compressed)
    vtrack->codec->init_compressed(file, file->total_vtracks-1);
  return 0;
  
  }

int lqt_write_audio_packet(quicktime_t * file,
                           lqt_packet_t * p, int track)
  {
  int result;
  int samples;
  
  quicktime_audio_map_t *atrack = &file->atracks[track];
  
  lqt_start_encoding(file);

  samples = p->duration;
  if(atrack->ci.flags & LQT_COMPRESSION_SBR)
    samples /= 2;
    
  //  if(atrack->codec->write_packet)
  //    return atrack->codec->write_packet(file, p, track);

  if(atrack->codec->write_packet)
    {
    return atrack->codec->write_packet(file, p, track);
    }
  else if(lqt_audio_is_vbr(file, track))
    {
    if(file->write_trak != atrack->track)
      quicktime_write_chunk_header(file, atrack->track);

    lqt_start_audio_vbr_frame(file, track);
    
    result = !quicktime_write_data(file, p->data, p->data_len);
    
    
    lqt_finish_audio_vbr_frame(file, track, samples);
    
    if(result)
      return 0;
    else
      return 1;

    }
  else
    {
    quicktime_write_chunk_header(file, atrack->track);
    result = quicktime_write_data(file, p->data, p->data_len);

    atrack->track->chunk_samples = samples;
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
  quicktime_trak_t * trak = vtrack->track;

  //  fprintf(stderr, "lqt_write_video_packet %ld %d\n",
  //          p->timestamp, p->duration);
  
  lqt_start_encoding(file);
  
  /* Must set valid timestamp for encoders */
  
  quicktime_write_chunk_header(file, vtrack->track);
  
  if(vtrack->codec->write_packet)
    result = vtrack->codec->write_packet(file, p, track);
  else
    result = !quicktime_write_data(file, p->data, p->data_len);
  
  trak->chunk_samples = 1;
  quicktime_write_chunk_footer(file, trak);
  
  if(p->flags && LQT_PACKET_KEYFRAME)
    quicktime_insert_keyframe(file, vtrack->current_position, track);

  /* Update stts */
  quicktime_update_stts(&trak->mdia.minf.stbl.stts, vtrack->current_position,
                        p->duration);

  if(vtrack->ci.flags & LQT_COMPRESSION_HAS_B_FRAMES)
    {
    /* CTS = PTS - DTS */
    quicktime_update_ctts(&trak->mdia.minf.stbl.ctts, vtrack->current_position,
                          p->timestamp - vtrack->timestamp);
    }
  vtrack->timestamp += p->duration;
  
  vtrack->cur_chunk++;
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
    { LQT_COMPRESSION_DV,        "dv"      },
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
           p->data_len, p->timestamp, p->duration,
           !!(p->flags & LQT_PACKET_KEYFRAME));
  lqt_hexdump(p->data, p->data_len > 16 ? 16 : p->data_len, 16);
  }

void lqt_packet_copy_metadata(lqt_packet_t * dst, const lqt_packet_t * src)
  {
  dst->flags = src->flags;
  dst->timestamp = src->timestamp;
  dst->duration = src->duration;
  dst->header_size = src->header_size;
  }

void lqt_packet_flush(lqt_packet_t * pkt, int bytes)
  {
  if(bytes < pkt->data_len)
    memmove(pkt->data, pkt->data + bytes, pkt->data_len - bytes);
  pkt->data_len -= bytes;
  }
