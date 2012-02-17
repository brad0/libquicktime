/*******************************************************************************
 lqt_transcode.c

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

/*
 *  Simple quicktime->quicktime transcoder
 *  Used mainly for testing the encoder capabilities
 *  of libquicktime
 */

/* Limitation: Handles only 1 audio- and one video stream per file */

#include <config.h> // ONLY for the PACKAGE macro. Usually, applications never need
                    // to include config.h

#define _(str) dgettext(PACKAGE, str)
#include <libintl.h>
#include <locale.h>

#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>



/* Supported colormodels */

int colormodels[] =
  {
    BC_RGB565,
    BC_BGR565,
    BC_BGR888,
    BC_BGR8888,
    BC_RGB888,
    BC_RGBA8888,
    BC_RGB161616,
    BC_RGBA16161616,
    BC_YUVA8888,
    BC_YUV422,
    BC_YUV420P,
    BC_YUV422P,
    BC_YUV444P,
    BC_YUV411P,
    LQT_COLORMODEL_NONE
  };

static struct
  {
  char * name;
  lqt_file_type_t type;
  char * extension;
  char * description;
  char * default_audio_codec;
  char * default_video_codec;
  }
formats[] =
  {
    { "qt",       LQT_FILE_QT,        "mov", "Quicktime (QT7 compatible)",   "faac", "ffmpeg_mpg4" },
    { "qtold",    LQT_FILE_QT_OLD,    "mov", "Quicktime (qt4l and old lqt)", "twos", "mjpa" },
    { "avi",      LQT_FILE_AVI,       "avi", "AVI (< 2G)",                   "lame", "ffmpeg_msmpeg4v3" },
    { "avi_odml", LQT_FILE_AVI_ODML, "avi", "AVI (> 2G)",                   "lame", "ffmpeg_msmpeg4v3" },
    { "mp4",      LQT_FILE_MP4,       "mp4", "ISO MPEG-4",                    "faac", "ffmpeg_mpg4" },
    { "m4a",      LQT_FILE_M4A,       "m4a", "m4a (iTunes compatible)",       "faac", "ffmpeg_mpg4"  },
  };

static void list_formats()
  {
  int i;
  printf(_("Supported formats\n"));
  for(i = 0; i < sizeof(formats)/sizeof(formats[0]); i++)
    {
    printf(_("%8s: %s (default codecs: %s/%s)\n"), formats[i].name, formats[i].description,
           formats[i].default_audio_codec, formats[i].default_video_codec);
    }
  }

typedef struct
  {
  quicktime_t * in_file;
  quicktime_t * out_file;
  
  int64_t num_video_frames;
  int64_t video_duration;
  
  int64_t num_audio_samples;

  int64_t audio_samples_written;
  int64_t video_frames_written;
  
  unsigned char ** video_buffer;

  float   ** audio_buffer_f;
  int16_t ** audio_buffer_i;
  int samples_per_frame;

  int do_audio;
  int do_video;

  /* Format information */
    
  int colormodel;
  int width;
  int height;
  int rowspan;
  int rowspan_uv;

  int frame_duration;
  int timescale;
  int samplerate;
  int num_channels;
  int audio_bits;

  /* Progress (0..1) */

  float progress;
  } transcode_handle;

static void print_usage()
  {
  printf(_("Usage: lqt_transcode [[-avi]|[-f <format>]] [-floataudio] [-qtvr <obj|pano>] [-qtvr_columns <columns>] [-qtvr_rows <rows>] [-ac <audio_codec>] [-vc <video_codec>] <in_file> <out_file>\n"));
  printf(_("       Transcode <in_file> to <out_file> using <audio_codec> and <video_codec>\n\n"));
  printf(_("       lqt_transcode -lv\n"));
  printf(_("       List video encoders\n\n"));
  printf(_("       lqt_transcode -la\n"));
  printf(_("       List audio encoders\n"));
  printf(_("       lqt_transcode -lf\n"));
  printf(_("       List output formats\n"));
  }

static void list_info(lqt_codec_info_t ** info)
  {
  int i, j;
  int max_len;
  int len;
  max_len = 0;
  i = 0;
  while(info[i])
    {
    len = strlen(info[i]->name);
    if(len > max_len)
      max_len = len;
    i++;
    }
  max_len++;
  i = 0;

  while(info[i])
    {
    len = strlen(info[i]->name);

    printf("%s:", info[i]->name);
    len = strlen(info[i]->name);

    for(j = 0; j < max_len - len; j++)
      printf(" ");

    printf("%s\n", info[i]->long_name);
    
    i++;
    }
  
  }

static void list_video_codecs()
  {
  lqt_codec_info_t ** info;
  info = lqt_query_registry(0, 1, 1, 0);
  list_info(info);
  lqt_destroy_codec_info(info);
  }

static void list_audio_codecs()
  {
  lqt_codec_info_t ** info;
  info = lqt_query_registry(1, 0, 1, 0);
  list_info(info);
  lqt_destroy_codec_info(info);
  }

static int transcode_init(transcode_handle * h,
                          char * in_file,
                          char * out_file,
                          char * video_codec,
                          char * audio_codec,
                          int floataudio,
                          lqt_file_type_t type,
                          char * qtvr,
                          int qtvr_rows,
                          int qtvr_columns)
  {
  lqt_codec_info_t ** codec_info;
  int i;
  int in_cmodel, out_cmodel;
  char * extension;
  
  h->in_file = quicktime_open(in_file, 1, 0);
  if(!h->in_file)
    {
    fprintf(stderr, _("Cannot open input file %s\n"), in_file);
    return 0;
    }

  /* Get the output format */

  if(type == LQT_FILE_NONE)
    {
    extension = strrchr(out_file, '.');
    if(!extension)
      {
      fprintf(stderr, _("Need a file extension when autoguessing output format\n"));
      return 0;
      }
    extension++;
    
    for(i = 0; i < sizeof(formats)/sizeof(formats[0]); i++)
      {
      if(!strcasecmp(extension, formats[i].extension))
        {
        type = formats[i].type;
        break;
        }
      }
    }
  if(type == LQT_FILE_NONE)
    {
    fprintf(stderr, _("Cannot detect output format. Specify a valid extension or use -f <format>\n"));
    return 0;
    }

  if(!audio_codec || !video_codec)
    {
    for(i = 0; i < sizeof(formats)/sizeof(formats[0]); i++)
      {
      if(type == formats[i].type)
        {
        if(!audio_codec) audio_codec = formats[i].default_audio_codec;
        if(!video_codec) video_codec = formats[i].default_video_codec;
        }
      }
    
    }
  
  h->out_file = lqt_open_write(out_file, type);
  if(!h->out_file)
    {
    fprintf(stderr, _("Cannot open output file %s\n"), out_file);
    return 0;
    }
    
  /* Check for video */

  if(quicktime_video_tracks(h->in_file) &&
     quicktime_supported_video(h->in_file, 0))
    h->do_video = 1;
  
  if(h->do_video)
    {
    h->width     = quicktime_video_width(h->in_file, 0);
    h->height    = quicktime_video_height(h->in_file, 0);
    
    h->timescale      = lqt_video_time_scale(h->in_file, 0);
    h->frame_duration = lqt_frame_duration(h->in_file, 0, NULL);
    
    /* Codec info for encoding */
    
    codec_info = lqt_find_video_codec_by_name(video_codec);
    if(!codec_info)
      {
      fprintf(stderr, _("Unsupported video cocec %s, try -lv\n"), video_codec);
      return 0;
      }

    /* Set up the output track */
    
    lqt_set_video(h->out_file, 1, h->width, h->height, h->frame_duration, h->timescale, codec_info[0]);
    
    /* Get colormodel */
    in_cmodel = lqt_get_cmodel(h->in_file, 0);
    out_cmodel = lqt_get_cmodel(h->out_file, 0);
    
    if(quicktime_reads_cmodel(h->in_file, out_cmodel, 0))
      {
      h->colormodel = out_cmodel;
      }
    else if(quicktime_writes_cmodel(h->out_file, in_cmodel, 0))
      {
      h->colormodel = in_cmodel;
      }
    else
      {
      h->colormodel = BC_RGB888;
      }
    
    h->video_buffer = lqt_rows_alloc(h->width, h->height, h->colormodel, &(h->rowspan), &(h->rowspan_uv));
    
    quicktime_set_cmodel(h->in_file,  h->colormodel);
    quicktime_set_cmodel(h->out_file, h->colormodel);
    
    lqt_destroy_codec_info(codec_info);

    h->num_video_frames = quicktime_video_length(h->in_file, 0);
    h->video_duration = lqt_video_duration(h->in_file, 0);
    }
  /* Check for audio */

  if(quicktime_audio_tracks(h->in_file) &&
     quicktime_supported_audio(h->in_file, 0))
    h->do_audio = 1;
  
  if(h->do_audio)
    {
    h->audio_bits = quicktime_audio_bits(h->in_file, 0);
    h->samplerate = quicktime_sample_rate(h->in_file, 0);
    h->num_channels = lqt_total_channels(h->in_file);
        
    /* Codec info for encoding */
    
    codec_info = lqt_find_audio_codec_by_name(audio_codec);
    if(!codec_info)
      {
      fprintf(stderr, _("Unsupported audio codec %s, try -la\n"), audio_codec);
      return 0;
      }

    /* Set up audio track */

    lqt_set_audio(h->out_file, h->num_channels,
                  h->samplerate, h->audio_bits,
                  codec_info[0]);
    lqt_destroy_codec_info(codec_info);
    
    /* Decide about audio frame size */

    /* Ok, we must take care about the audio frame size.
       The sample count, we pass to encode_audio() directly affects interleaving.
       Many small audio chunks make decoding inefficient, few large chunks make seeking
       slower because many samples inside a chunk have to be skipped.
       
       Ok, then lets just take half a second and see how it works :-)

       On a 25 fps system this means, that one audio chunks comes after an average of
       12.5 video frames. This is roughly what we see in files created with other
       Software
    */
    
    h->samples_per_frame = h->samplerate / 2;
    /* Avoid too odd numbers */
    h->samples_per_frame = 16 * ((h->samples_per_frame + 15) / 16);

    /* Allocate output buffer */

    if(floataudio)
      {
      h->audio_buffer_f = malloc(h->num_channels * sizeof(float*));
      h->audio_buffer_f[0] = malloc(h->num_channels * h->samples_per_frame * sizeof(float));
      for(i = 1; i < h->num_channels; i++)
        h->audio_buffer_f[i] = &(h->audio_buffer_f[0][i*h->samples_per_frame]);
      }
    else
      {
      h->audio_buffer_i = malloc(h->num_channels * sizeof(int16_t*));
      h->audio_buffer_i[0] = malloc(h->num_channels * h->samples_per_frame * sizeof(int16_t));
      for(i = 1; i < h->num_channels; i++)
        h->audio_buffer_i[i] = &(h->audio_buffer_i[0][i*h->samples_per_frame]);
      }
    h->num_audio_samples = quicktime_audio_length(h->in_file, 0);
    }

    if (qtvr) {
	if(strncmp(qtvr,"obj", 3) == 0) {
	    lqt_qtvr_set_type(h->out_file, QTVR_OBJ, h->width , h->height, h->frame_duration, h->timescale, 0);
	}
	
	if(strncmp(qtvr,"pano", 4) == 0) {
	    lqt_qtvr_set_type(h->out_file, QTVR_PAN, h->width/2, h->width, h->frame_duration, h->timescale, 0);
	}
	
	if(qtvr_columns && qtvr_rows) {
	    lqt_qtvr_set_rows(h->out_file, qtvr_rows);
	    lqt_qtvr_set_columns(h->out_file, qtvr_columns);
	}
    }
    
  return 1;
  }

static int transcode_iteration(transcode_handle * h)
  {
  double audio_time;
  double video_time;
  int num_samples;
  int do_audio = 0;
  float progress;
  int64_t frame_time;
  
  if(h->do_audio && h->do_video)
    {
    audio_time = (float)(h->audio_samples_written)/(float)(h->samplerate);
    video_time = (float)(h->video_frames_written * h->frame_duration)/h->timescale;

    if(audio_time < video_time)
      do_audio = 1;
    }
  else if(h->do_audio)
    {
    do_audio = 1;
    }

  /* Audio Iteration */

  if(do_audio)
    {
    //    lqt_decode_audio(h->in_file, h->audio_buffer_i, h->audio_buffer_f, h->samples_per_frame);
    lqt_decode_audio_track(h->in_file, h->audio_buffer_i, h->audio_buffer_f, h->samples_per_frame, 0);
    num_samples = lqt_last_audio_position(h->in_file, 0) - h->audio_samples_written;
    //    fprintf(stderr, "Num samples: %d\n",num_samples);
    quicktime_encode_audio(h->out_file, h->audio_buffer_i, h->audio_buffer_f, num_samples);
    h->audio_samples_written += num_samples;

    if(num_samples < h->samples_per_frame)
      h->do_audio = 0;
    progress = (float)(h->audio_samples_written)/(float)(h->num_audio_samples);

    }
  /* Video Iteration */
  else
    {
    frame_time = lqt_frame_time(h->in_file, 0);
    lqt_decode_video(h->in_file, h->video_buffer, 0);
    lqt_encode_video(h->out_file, h->video_buffer, 0, frame_time);
    
    h->video_frames_written++;
    if(h->video_frames_written >= h->num_video_frames)
      h->do_video = 0;
    progress = (float)(h->video_frames_written)/(float)(h->num_video_frames);
    }
  if(!h->do_audio && !h->do_video)
    return 0;

  /* Calculate the progress */

  if(progress > h->progress)
    h->progress = progress;
  return 1;
  }

static void transcode_cleanup(transcode_handle * h)
  {
  quicktime_close(h->in_file);
  quicktime_close(h->out_file);
  if(h->video_buffer)
    lqt_rows_free(h->video_buffer);

  if(h->audio_buffer_i)
    {
    free(h->audio_buffer_i[0]);
    free(h->audio_buffer_i);
    }
  if(h->audio_buffer_f)
    {
    free(h->audio_buffer_f[0]);
    free(h->audio_buffer_f);
    }
  }


int main(int argc, char ** argv)
  {
  char * in_file = (char*)0;
  char * out_file = (char*)0;
  char * video_codec = (char*)0;
  char * audio_codec = (char*)0;
  char * format = (char*)0;
  char * qtvr = (char*)0;
  unsigned short qtvr_rows = 0;
  unsigned short qtvr_columns = 0;
  int i;
  lqt_file_type_t type = LQT_FILE_NONE, floataudio = 0;
  transcode_handle handle;
  int progress_written = 0;
  
  memset(&handle, 0, sizeof(handle));
  
  switch(argc)
    {
    case 1:
      print_usage();
      exit(0);
      break;
    case 2:
      if(!strcmp(argv[1], "-lv"))
        list_video_codecs();
      else if(!strcmp(argv[1], "-la"))
        list_audio_codecs();
      else if(!strcmp(argv[1], "-lf"))
        list_formats();
      else
        print_usage();
      exit(0);
      break;
    default:
      for(i = 1; i < argc - 2; i++)
        {
        if(!strcmp(argv[i], "-vc"))
          {
          video_codec = argv[i+1];
          i++;
          }
        else if(!strcmp(argv[i], "-ac"))
          {
          audio_codec = argv[i+1];
          i++;
          }
        else if(!strcmp(argv[i], "-f"))
          {
          format = argv[i+1];
          i++;
          }
        else if(!strcmp(argv[i], "-avi"))
          format = "avi";
        else if(!strcmp(argv[i], "-floataudio"))
          floataudio = 1;
        else if(!strcmp(argv[i], "-qtvr")) {
          qtvr = argv[i+1];
	  i++;
	  }
	else if(!strcmp(argv[i], "-qtvr_rows")) {
          qtvr_rows = atoi(argv[i+1]);
	  i++;
	  }
	else if(!strcmp(argv[i], "-qtvr_columns")) {
          qtvr_columns = atoi(argv[i+1]);
	  i++;
	  }
        }
      in_file = argv[argc-2];
      out_file = argv[argc-1];
    }

  /* Get file type */
  if(format)
    {
    for(i = 0; i < sizeof(formats)/sizeof(formats[0]); i++)
      {
      if(!strcasecmp(format, formats[i].name))
        {
        type = formats[i].type;
        break;
        }
      }
    if(type == LQT_FILE_NONE)
      {
      fprintf(stderr, _("Unsupported format %s, try -lf"), format);
      return -1;
      }
    }
  
  if(!transcode_init(&handle, in_file, out_file, video_codec, audio_codec,
                     floataudio, type, qtvr, qtvr_rows, qtvr_columns))
    {
    return -1;
    }
  
  i = 10;
  
  while(transcode_iteration(&handle))
    {
    if(i == 10)
      {
      if(progress_written)
        putchar('\r');
      printf(_("%6.2f%% Completed"), handle.progress*100.0);
      fflush(stdout);
      i = 0;
      progress_written = 1;
      }
    i++;
    }
  printf(_("%6.2f%% Completed\n"), 100.0);
  
  if(handle.audio_samples_written)
    printf("Transcoded %"PRId64" audio samples\n", handle.audio_samples_written);

  if(handle.video_frames_written)
    printf("Transcoded %"PRId64" video frames\n", handle.video_frames_written);
  
  transcode_cleanup(&handle);
  return 0;
  }
