/*****************************************************************
 
  lqttranscode.c
 
  Copyright (c) 2003 by Burkhard Plaum - plaum@ipf.uni-stuttgart.de
 
  http://libquicktime.sourceforge.net
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111, USA.
 
*****************************************************************/

/*
 *  Simple quicktime->quicktime transcoder
 *  Used mainly for testing the encoder capabilities
 *  of libquicktime
 */

/* Limitation: Handles only 1 audio- and one video stream per file */

#include <string.h>

#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>

/* Define this for floating point audio support */
// #define USEFLOAT

/* Supported colormodels */

int colormodels[] =
  {
    BC_RGB565,
    BC_BGR565,
    BC_BGR888,
    BC_BGR8888,
    BC_RGB888,
    BC_RGBA8888,
    BC_ARGB8888,
    BC_ABGR8888,
    BC_RGB161616,
    BC_RGBA16161616,
    BC_YUV888,
    BC_YUVA8888,
    BC_YUV161616,
    BC_YUVA16161616,
    BC_YUV422,
    BC_YUV101010,
    BC_VYU888,
    BC_UYVA8888,
    BC_YUV420P,
    BC_YUV422P,
    BC_YUV444P,
    BC_YUV411P,
    LQT_COLORMODEL_NONE
  };

typedef struct
  {
  quicktime_t * in_file;
  quicktime_t * out_file;

  int64_t num_audio_samples;
  int64_t num_video_frames;

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

  int frame_duration;
  int timescale;
  int samplerate;
  int num_channels;
  int audio_bits;

  float progress;
  } transcode_handle;

static void print_usage()
  {
  printf("Usage: lqt_transcode [-avi] [-floataudio] [-qtvr <obj|pano>] [-qtvr_columns <columns>] [-qtvr_rows <rows>] [-ac <audio_codec>] [-vc <video_codec>] <in_file> <out_file>\n");
  printf("       Transcode <in_file> to <out_file> using <audio_codec> and <video_codec>\n\n");
  printf("       lqt_transcode -lv\n");
  printf("       List video encoders encoders\n\n");
  printf("       lqt_transcode -la\n");
  printf("       List audio encoders encoders\n");
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

static unsigned char ** alloc_video_buffer(int width, int height, int colormodel)
  {
  int bytes_per_line = 0;
  int i;
  int y_size, uv_size = 0;
  unsigned char ** video_buffer;
  /* Allocate frame buffer */

  if(cmodel_is_planar(colormodel))
    {
    y_size = width * height;
    
    switch(colormodel)
      {
      case BC_YUV420P:
      case BC_YUV411P:
        uv_size = (width * height)/4;
        break;
      case BC_YUV422P:
        uv_size = (width * height)/2;
        break;
      case BC_YUV444P:
        uv_size = (width * height);
      }
    video_buffer    = malloc(3 * sizeof(unsigned char*));
    video_buffer[0] = malloc(y_size + 2 * uv_size);
    video_buffer[1] = &(video_buffer[0][y_size]);
    video_buffer[2] = &(video_buffer[0][y_size+uv_size]);
    }
  else
    {
    video_buffer    = malloc(height * sizeof(unsigned char*));
    switch(colormodel)
      {
      case BC_RGB565:
      case BC_BGR565:
      case BC_YUV422:
        bytes_per_line = width * 2;
        break;
      case BC_BGR888:
      case BC_RGB888:
      case BC_YUV888:
      case BC_VYU888:
        bytes_per_line = width * 3;
        break;
      case BC_BGR8888:
      case BC_RGBA8888:
      case BC_ARGB8888:
      case BC_ABGR8888:
      case BC_YUV101010:
      case BC_YUVA8888:
      case BC_UYVA8888:
        bytes_per_line = width * 4;
        break;
        
      case BC_RGB161616:
      case BC_YUV161616:
        bytes_per_line = width * 6;
        break;
      case BC_RGBA16161616:
      case BC_YUVA16161616:
        bytes_per_line = width * 8;
        break;
      }
    video_buffer[0] = malloc(height * bytes_per_line);
    for(i = 1; i < height; i++)
      video_buffer[i] = &(video_buffer[0][i*bytes_per_line]);
    
    }
  return video_buffer;
  }

static int transcode_init(transcode_handle * h,
                          char * in_file,
                          char * out_file,
                          char * video_codec,
                          char * audio_codec,
                          int floataudio,
                          int use_avi,
                          char * qtvr,
                          int qtvr_rows,
                          int qtvr_columns)
  {
  lqt_codec_info_t ** codec_info;
  int * colormodels;
  int i;
  int bytes_per_line;
  
  h->in_file = quicktime_open(in_file, 1, 0);
  if(!h->in_file)
    {
    fprintf(stderr, "Cannot open input file %s\n", in_file);
    return 0;
    }
  h->out_file = quicktime_open(out_file, 0, 1);
  if(!h->out_file)
    {
    fprintf(stderr, "Cannot open output file %s\n", out_file);
    return 0;
    }
    
  /* Check for video */

  h->do_video = !!quicktime_video_tracks(h->in_file);

  if(h->do_video)
    {
    h->width     = quicktime_video_width(h->in_file, 0);
    h->height    = quicktime_video_height(h->in_file, 0);
    
    h->timescale = lqt_video_time_scale(h->in_file, 0);
    h->frame_duration = lqt_frame_duration(h->in_file, 0, NULL);
    
    /* Codec info for encoding */
    
    codec_info = lqt_find_video_codec_by_name(video_codec);
    if(!codec_info)
      {
      fprintf(stderr, "Unsupported video cocec %s, try -lv\n", video_codec);
      return 0;
      }
    

    /* Set up the output track */
    
    lqt_set_video(h->out_file, 1,
                  h->width, h->height,
                  h->frame_duration, h->timescale,
                  codec_info[0]);
    
    /* Get colormodel */
    
    colormodels = malloc((codec_info[0]->num_encoding_colormodels + 1) * sizeof(int));
    for(i = 0; i < codec_info[0]->num_encoding_colormodels; i++)
      colormodels[i] = codec_info[0]->encoding_colormodels[i];
    colormodels[codec_info[0]->num_encoding_colormodels] = LQT_COLORMODEL_NONE;
    
    h->colormodel = lqt_get_best_colormodel(h->in_file, 0, colormodels);
    
    fprintf(stderr, "Video stream: %dx%d, Colormodel: %s\n",
            h->width, h->height, lqt_colormodel_to_string(h->colormodel));
    
    h->video_buffer = alloc_video_buffer(h->width, h->height, h->colormodel);
    
    quicktime_set_cmodel(h->in_file,  h->colormodel);
    quicktime_set_cmodel(h->out_file, h->colormodel);
    
    lqt_destroy_codec_info(codec_info);

    h->num_video_frames = quicktime_video_length(h->in_file, 0);

    }
  /* Check for audio */
  
  h->do_audio = !!quicktime_audio_tracks(h->in_file);
  
  if(h->do_audio)
    {
    h->audio_bits = quicktime_audio_bits(h->in_file, 0);
    h->samplerate = quicktime_sample_rate(h->in_file, 0);
    h->num_channels = lqt_total_channels(h->in_file);
        
    /* Codec info for encoding */
    
    codec_info = lqt_find_audio_codec_by_name(audio_codec);
    if(!codec_info)
      {
      fprintf(stderr, "Unsupported audio codec %s, try -la\n", audio_codec);
      return 0;
      }

    /* Set up audio track */

    lqt_set_audio(h->out_file, h->num_channels,
                  h->samplerate, h->audio_bits,
                  codec_info[0]);
    lqt_destroy_codec_info(codec_info);
    
    /* Decide about audio frame size */
#if 1
    if(h->do_video)
      {
      h->samples_per_frame = (int)(((double)h->frame_duration / (double)h->timescale)+0.5);
      /* Avoid too odd numbers */
      h->samples_per_frame = 16 * ((h->samples_per_frame + 15) / 16);
      }
    else
      h->samples_per_frame = 4096;
#else
    h->samples_per_frame = 8192;
#endif
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
	    lqt_qtvr_set_type(h->out_file, QTVR_OBJ, 0 , 0, 0, 0, 0);
	}
	
	if(strncmp(qtvr,"pano", 4) == 0) {
	    lqt_qtvr_set_type(h->out_file, QTVR_PAN, qtvr_columns * h->height, qtvr_rows * h->width, h->frame_duration, h->timescale, 0);
	}
	
	if(qtvr_columns) {
	    lqt_qtvr_set_columns(h->out_file, (short)qtvr_columns);
	}
	
	if(qtvr_rows) {
	    lqt_qtvr_set_rows(h->out_file, (short)qtvr_rows);
	}
    }
    
  if(use_avi)
    quicktime_set_avi(h->out_file, 1);
  return 1;
  }

static int transcode_iteration(transcode_handle * h)
  {
  double audio_time;
  double video_time;
  int num_samples;
  int do_audio = 0;
  float progress;
  if(h->do_audio && h->do_video)
    {
    audio_time = (float)(h->audio_samples_written)/(float)(h->samplerate);
    video_time = (float)(h->video_frames_written * h->frame_duration)/h->timescale;

    //    fprintf(stderr, "Transcode time: %f %f\n", audio_time, video_time);
    
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
    /* Last frame needs special attention */
    if(h->audio_samples_written + h->samples_per_frame >= h->num_audio_samples)
      num_samples = h->num_audio_samples - h->audio_samples_written;
    else
      num_samples = h->samples_per_frame;

    lqt_decode_audio(h->in_file, h->audio_buffer_i, h->audio_buffer_f, num_samples);
    quicktime_encode_audio(h->out_file, h->audio_buffer_i, h->audio_buffer_f, num_samples);
    h->audio_samples_written += num_samples;

    if(h->audio_samples_written >= h->num_audio_samples)
      h->do_audio = 0;
    progress = (float)(h->audio_samples_written)/(float)(h->num_audio_samples);
    }
  /* Video Iteration */
  else
    {
    lqt_decode_video(h->in_file, h->video_buffer, 0);
    quicktime_encode_video(h->out_file, h->video_buffer, 0);

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
  }
     
int main(int argc, char ** argv)
  {
  char * in_file = (char*)0;
  char * out_file = (char*)0;
  char * video_codec = (char*)0;
  char * audio_codec = (char*)0;
  char * qtvr = (char*)0;
  unsigned short qtvr_rows = 0;
  unsigned short qtvr_columns = 0;
  int i, j;
  int use_avi = 0, floataudio = 0;
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
      else
        print_usage();
      exit(0);
      break;
    default:
      audio_codec = "rawaudio";
      video_codec = "raw";
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
        else if(!strcmp(argv[i], "-avi"))
          use_avi = 1;
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
  
  if(!transcode_init(&handle, in_file, out_file, video_codec, audio_codec,
                     floataudio, use_avi, qtvr, qtvr_rows, qtvr_columns))
    {
    return 0;
    }
  
  i = 10;
  
  while(transcode_iteration(&handle))
    {
    if(i == 10)
      {
      if(progress_written)
        {
        for(j = 0; j < 17; j++)
          putchar(0x08);
        }
      printf("%6.2f%% Completed", handle.progress*100.0);
      fflush(stdout);
      i = 0;
      progress_written = 1;
      }
    i++;
    }

  transcode_cleanup(&handle);
  return 0;
  }
