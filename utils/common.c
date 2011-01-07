/*******************************************************************************
 common.c

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

#include <stdio.h>
#include <libintl.h>

#include <config.h> // ONLY for the PACKAGE macro. Usually, applications never need
                    // to include config.h

#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>

#include "common.h"

#define _(str) dgettext(PACKAGE, str)

void quicktime_print_info(quicktime_t * qtfile)
  {
  char * str;
  int n, i, j, channels;
  const lqt_channel_t * channel_setup;
  char language[4];
  int frame_duration, framerate_constant;
  int pixel_width, pixel_height;
  int cmodel;
  uint32_t timecode_flags;
  int timecode_rate;
  
  printf(_("Type: %s\n"), lqt_file_type_to_string(lqt_get_file_type(qtfile)));

  
  str = quicktime_get_copyright(qtfile);
  if (str)
    printf(_("    copyright: %s\n"),str);

  str = quicktime_get_name(qtfile);
  if (str)
    printf(_("    name:      %s\n"),str);

  str = quicktime_get_info(qtfile);
  if (str)
    printf(_("    info:      %s\n"),str);

  str = lqt_get_author(qtfile);
  if (str)
    printf(_("    author:    %s\n"),str);

  str = lqt_get_artist(qtfile);
  if (str)
    printf(_("    artist:    %s\n"),str);

  str = lqt_get_album(qtfile);
  if (str)
    printf(_("    album:     %s\n"),str);

  str = lqt_get_genre(qtfile);
  if (str)
    printf(_("    genre:     %s\n"),str);

  str = lqt_get_track(qtfile);
  if (str)
    printf(_("    track:     %s\n"),str);

  str = lqt_get_comment(qtfile);
  if (str)
    printf(_("    comment:   %s\n"),str);

        
  n = quicktime_audio_tracks(qtfile);
  printf(_("  %d audio tracks.\n"), n);
  for(i = 0; i < n; i++) {
  channels = quicktime_track_channels(qtfile, i);
  channel_setup = lqt_get_channel_setup(qtfile, i);
  printf(_("    %d channels, %d bits, sample rate %ld, length %ld samples, "),
         channels,
         quicktime_audio_bits(qtfile, i),
         quicktime_sample_rate(qtfile, i),
         quicktime_audio_length(qtfile, i));
  if(lqt_is_avi(qtfile))
    {
    printf(_("wav_id 0x%02x.\n"), lqt_get_wav_id(qtfile, i));
    }
  else
    {
    printf(_("compressor %s.\n"), quicktime_audio_compressor(qtfile, i));
    }
  printf(_("    Sample format: %s.\n"),
         lqt_sample_format_to_string(lqt_get_sample_format(qtfile, i)));
  printf(_("    Channel setup: "));
  if(channel_setup)
    {
    for(j = 0; j < channels; j++)
      {
      printf("%s", lqt_channel_to_string(channel_setup[j]));
      if(j < channels-1)
        printf(_(", "));
      }
    printf(_("\n"));
    }
  else
    printf(_("Not available\n"));
  printf(_("    Language: "));
  if(lqt_get_audio_language(qtfile, i, language))
    printf("%c%c%c\n", language[0], language[1], language[2]);
  else
    printf(_("Not available\n"));
  if(quicktime_supported_audio(qtfile, i))
    printf(_("    supported.\n"));
  else
    printf(_("    not supported.\n"));
  }
        
  n = quicktime_video_tracks(qtfile);
  printf(_("  %d video tracks.\n"), n);
  for(i = 0; i < n; i++)
    {
    frame_duration = lqt_frame_duration(qtfile, i, &framerate_constant);
    lqt_get_pixel_aspect(qtfile, i, &pixel_width, &pixel_height);
    printf(_("    %dx%d, depth %d\n    rate %f [%d:%d] %s\n    length %ld frames\n    compressor %s.\n"),
           quicktime_video_width(qtfile, i),
           quicktime_video_height(qtfile, i),
           quicktime_video_depth(qtfile, i),
           quicktime_frame_rate(qtfile, i),
           lqt_video_time_scale(qtfile, i),
           frame_duration, (framerate_constant ? _("constant") : _("not constant")),
           quicktime_video_length(qtfile, i),
           quicktime_video_compressor(qtfile, i));
    cmodel = lqt_get_cmodel(qtfile, i);
    printf(_("    Native colormodel:  %s\n"),
           lqt_colormodel_to_string(cmodel));
    printf(_("    Interlace mode:     %s\n"),
           lqt_interlace_mode_to_string(lqt_get_interlace_mode(qtfile, i)));
    if(cmodel == BC_YUV420P)
      printf(_("    Chroma placement: %s\n"), lqt_chroma_placement_to_string(lqt_get_chroma_placement(qtfile, i)));
    if((pixel_width > 1) || (pixel_height > 1))
      printf(_("    Pixel aspect ratio: %d:%d\n"), pixel_width, pixel_height);

    if(lqt_has_timecode_track(qtfile, i, &timecode_flags, &timecode_rate))
      printf(_("    Timecodes available (flags: %08x, rate: %d)\n"), timecode_flags,
             timecode_rate);
    else
      printf(_("    No timecodes available\n"));
      
    
    if(quicktime_supported_video(qtfile, i))
      printf(_("    supported.\n"));
    else
      printf(_("    not supported.\n"));
    }

  n = lqt_text_tracks(qtfile);
  printf(_("  %d text tracks.\n"), n);
  for(i = 0; i < n; i++)
    {
    printf(_("    timescale: %d, length: %"PRId64", language: "),
           lqt_text_time_scale(qtfile, i), lqt_text_samples(qtfile, i)); 
    if(lqt_get_text_language(qtfile, i, language))
      printf("%c%c%c, ", language[0], language[1], language[2]);
    else
      printf(_("Not available, "));
    printf(_("type: %s\n"), lqt_is_chapter_track(qtfile, i) ? _("Chapters") : _("Subtitles") );
    }
  }
