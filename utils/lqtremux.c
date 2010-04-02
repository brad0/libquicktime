/*******************************************************************************
 lqt_remux.c

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

#include <quicktime/lqt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void print_usage()
  {
  fprintf(stderr, "Usage: qtremux [-pre <outprefix>] infile\n");
  fprintf(stderr, "       qtremux infile1 infile2 ... outfile\n");
  }

typedef struct
  {
  const lqt_compression_info_t * ci;
  quicktime_t * in_file;
  quicktime_t * out_file;
  int64_t time;
  lqt_packet_t * p;
  int in_index;
  int out_index;
  } track_t;

typedef struct
  {
  quicktime_t * file;
  int num_audio_tracks;
  int num_video_tracks;
  } file_t;

/* Global variables */
track_t * audio_tracks = NULL;
track_t * video_tracks = NULL;
int total_tracks = 0;
file_t * files;
int num_audio_tracks, num_video_tracks;
quicktime_t * file;

int prefix_len;
char * prefix = NULL;

static void audio_iteration(track_t * track)
  {
  
  }

static void video_iteration(track_t * track)
  {
  
  }

static int init_demultiplex(char * filename)
  {
  int i;
  char * tmp_string;

  file = lqt_open_read(filename);
  if(!file)
    {
    fprintf(stderr, "Couldn't open file %s\n", filename);
    return -1;
    }
    
  num_audio_tracks = quicktime_audio_tracks(file);
  num_video_tracks = quicktime_video_tracks(file);

  files = calloc(num_audio_tracks + num_video_tracks, sizeof(*files));

  if(!prefix) /* Create default prefix */
    {
    char * pos;
    prefix = strdup(filename);
      
    /* Put into current directory */
    pos = strrchr(prefix, '/');
    if(pos)
      {
      pos++;
      memmove(prefix, pos, strlen(pos)+1);
      }

    /* Strip off extension */
    pos = strrchr(prefix, '.');
    if(pos)
      *pos = '\0';
    }

  prefix_len = strlen(prefix);

  tmp_string = malloc(prefix_len + 128);
    
  if(num_audio_tracks)
    {
    audio_tracks = calloc(num_audio_tracks, sizeof(*audio_tracks));
    for(i = 0; i < num_audio_tracks; i++)
      {
      audio_tracks[i].ci = lqt_get_audio_compression_info(file, i);

      if(!audio_tracks[i].ci)
        {
        fprintf(stderr, "Audio track %d cannot be read compressed\n", i+1);
        continue;
        }
      
      sprintf(tmp_string, "%s_audio_%02d.mov", prefix, i+1);
      audio_tracks[i].in_file = file;
      audio_tracks[i].out_file = lqt_open_write(tmp_string, LQT_FILE_QT);

      if(!lqt_writes_audio_compressed(audio_tracks[i].out_file,
                                      audio_tracks[i].ci))
        {
        fprintf(stderr, "Audio track %d cannot be written compressed\n", i+1);
        quicktime_close(audio_tracks[i].out_file);
        audio_tracks[i].out_file = NULL;
        continue;
        }
      total_tracks++;
      
      lqt_add_audio_track_compressed(audio_tracks[i].out_file,
                                     audio_tracks[i].ci);
      audio_tracks[i].in_index = i;
      audio_tracks[i].out_index = 0;
      }
    }
  if(num_video_tracks)
    {
    video_tracks = calloc(num_video_tracks, sizeof(*video_tracks));
    for(i = 0; i < num_video_tracks; i++)
      {
      video_tracks[i].ci = lqt_get_video_compression_info(file, i);

      if(!video_tracks[i].ci)
        {
        fprintf(stderr, "Video track %d cannot be read compressed\n", i+1);
        continue;
        }
      
      sprintf(tmp_string, "%s_video_%02d.mov", prefix, i+1);
      video_tracks[i].in_file = file;
      video_tracks[i].out_file = lqt_open_write(tmp_string, LQT_FILE_QT);

      if(!lqt_writes_video_compressed(video_tracks[i].out_file,
                                      video_tracks[i].ci))
        {
        fprintf(stderr, "Video track %d cannot be written compressed\n", i+1);
        quicktime_close(video_tracks[i].out_file);
        video_tracks[i].out_file = NULL;
        continue;
        }
      total_tracks++;

      lqt_add_video_track_compressed(video_tracks[i].out_file,
                                     video_tracks[i].ci);
      video_tracks[i].in_index = i;
      video_tracks[i].out_index = 0;
      }
    }

  free(tmp_string);
  
  return 1;
  }

static int init_multiplex(char ** in_files, int num_in_files, char * out_file)
  {
  int i, j;
  int audio_index;
  int video_index;
  char * pos;
  lqt_file_type_t type = LQT_FILE_QT;
  
  files = calloc(num_in_files, sizeof(*files));

  /* Open input files */
     
  for(i = 0; i < num_in_files; i++)
    {
    files[i].file = lqt_open_read(in_files[i]);
    if(!files[i].file)
      {
      fprintf(stderr, "Opening %s failed\n", in_files[i]);
      return 0;
      }
    files[i].num_audio_tracks = quicktime_audio_tracks(files[i].file);
    files[i].num_video_tracks = quicktime_video_tracks(files[i].file);
    num_audio_tracks += files[i].num_audio_tracks;
    num_video_tracks += files[i].num_video_tracks;
    }
  
  /* Open output file */

  pos = strrchr(out_file, '.');
  if(!pos)
    {
    fprintf(stderr, "Unknown file type for file %s\n", out_file);
    return 0;
    }
  
  pos++;
  if(!strcmp(pos, "mov"))
    type = LQT_FILE_QT;
  else if(!strcmp(pos, "avi"))
    type = LQT_FILE_AVI_ODML;
  else if(!strcmp(pos, "mp4"))
    type = LQT_FILE_MP4;
  else if(!strcmp(pos, "m4a"))
    type = LQT_FILE_M4A;
  else
    {
    fprintf(stderr, "Unknown file type for file %s\n", out_file);
    return 0;
    }
  
  file = lqt_open_write(out_file, type);
  
  audio_tracks = calloc(num_audio_tracks, sizeof(*audio_tracks));
  video_tracks = calloc(num_video_tracks, sizeof(*video_tracks));

  audio_index = 0;
  video_index = 0;

  for(i = 0; i < num_in_files; i++)
    {
    for(j = 0; j < files[i].num_audio_tracks; j++)
      {
      audio_tracks[audio_index].ci = lqt_get_audio_compression_info(files[i].file, j);
      if(!audio_tracks[audio_index].ci)
        {
        fprintf(stderr, "Audio track %d of file %s cannot be read compressed\n",
                j+1, in_files[i]);
        audio_index++;
        continue;
        }
      if(!lqt_writes_audio_compressed(file, audio_tracks[audio_index].ci))
        {
        fprintf(stderr, "Audio track %d of file %s cannot be written compressed\n",
                j+1, in_files[i]);
        audio_index++;
        continue;
        }
      audio_tracks[audio_index].in_index = j;
      audio_tracks[audio_index].out_index = audio_index;
      audio_tracks[audio_index].in_file = files[i].file;
      audio_tracks[audio_index].out_file = file;
      audio_index++;
      total_tracks++;
      }
    for(j = 0; j < files[i].num_video_tracks; j++)
      {
      video_tracks[video_index].ci = lqt_get_video_compression_info(files[i].file, j);
      if(!video_tracks[video_index].ci)
        {
        fprintf(stderr, "Video track %d of file %s cannot be read compressed\n",
                j+1, in_files[i]);
        video_index++;
        continue;
        }
      if(!lqt_writes_video_compressed(file, video_tracks[video_index].ci))
        {
        fprintf(stderr, "Video track %d of file %s cannot be written compressed\n",
                j+1, in_files[i]);
        video_index++;
        continue;
        }
      video_tracks[video_index].in_index = j;
      video_tracks[video_index].out_index = video_index;
      video_tracks[video_index].in_file = files[i].file;
      video_tracks[video_index].out_file = file;
      video_index++;
      total_tracks++;
      
      }
    
    }
  return 1;
  }

int main(int argc, char ** argv)
  {
  int i;
  int first_file;
  int num_files;
  
  if(argc < 2)
    {
    print_usage();
    return 0;
    }

  /* Parse agruments */

  num_files = argc - 1;

  i = 1;
  while(i < argc)
    {
    if(*(argv[i]) != '-')
      {
      first_file = i;
      break;
      }
      
    if(!strcmp(argv[i], "-pre"))
      {
      if(i == argc-1)
        {
        fprintf(stderr, "-pre requires an argument\n");
        return -1;
        }
      prefix = strdup(argv[i+1]);
      i++;
      num_files -= 2;
      }
    i++;
    }

  if(!num_files)
    {
    print_usage();
    return -1;
    }
  
  if(num_files == 1)
    {
    /* Demultiplex */
    if(!init_demultiplex(argv[argc-1]))
      return -1;
    }
  else
    {
    /* Multiplex */
    
    if(!init_multiplex(&argv[first_file], num_files-1, argv[argc-1]))
      return -1;
    }

  if(!total_tracks)
    {
    fprintf(stderr, "No tracks to demultiplex\n");
    return -1;
    }

  /* Transmultiplex */
  
  
  return 0;
  }
