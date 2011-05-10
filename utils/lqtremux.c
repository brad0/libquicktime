/*******************************************************************************
 lqt_remux.c

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

#include <quicktime/lqt.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static void print_usage()
  {
  fprintf(stderr, "Usage: lqtremux [-pre <outprefix>] infile\n");
  fprintf(stderr, "       lqtremux infile1 infile2 ... outfile\n");
  }

typedef struct
  {
  const lqt_compression_info_t * ci;
  quicktime_t * in_file;
  quicktime_t * out_file;
  double time;
  int timescale;
  
  int in_index;
  int out_index;
  int eof;
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
int num_files;

int nfiles;
int num_audio_tracks, num_video_tracks;
quicktime_t * file;

int prefix_len;
char * prefix = NULL;

lqt_codec_info_t ** audio_encoders;
lqt_codec_info_t ** video_encoders;

static lqt_codec_info_t * find_audio_encoder(lqt_compression_id_t id)
  {
  int i = 0;

  while(audio_encoders[i])
    {
    if(audio_encoders[i]->compression_id == id)
      return audio_encoders[i];
    i++;
    }
  return NULL;
  }

static lqt_codec_info_t * find_video_encoder(lqt_compression_id_t id)
  {
  int i = 0;
  
  while(video_encoders[i])
    {
    // fprintf(stderr, "find_video_encoder %s\n", video_encoders[i]->name);
    if(video_encoders[i]->compression_id == id)
      return video_encoders[i];
    i++;
    }
  return NULL;
  }

static void audio_iteration(track_t * track, lqt_packet_t * p)
  {
  int samples_read = 0;
  int samples_to_read = track->ci->samplerate / 2; /* We make half second audio chunks */

  while(samples_read < samples_to_read)
    {
    if(!lqt_read_audio_packet(track->in_file, p, track->in_index))
      {
      track->eof = 1;
      return;
      }

    //    fprintf(stderr, "Got audio packet\n");
    //    lqt_packet_dump(p);
    
    lqt_write_audio_packet(track->out_file, p, track->out_index);

    samples_read += p->duration;
    }
  
  track->time = (double)(p->timestamp + p->duration) / (double)(track->ci->samplerate);
  
  }

static void video_iteration(track_t * track, lqt_packet_t * p)
  {
  double test_time;
  
  if(!lqt_read_video_packet(track->in_file, p, track->in_index))
    {
    track->eof = 1;
    return;
    }

  // fprintf(stderr, "Got video packet\n");
  // lqt_packet_dump(p);
  
  lqt_write_video_packet(track->out_file, p, track->out_index);
  
  test_time = (double)(p->timestamp + p->duration) / (double)(track->ci->video_timescale);
  if(test_time > track->time)
    track->time = test_time;
  
  }

static track_t * tracks_min_time(track_t * tracks, int num)
  {
  int i;
  track_t * ret = NULL;
  double min_time = 0.0;
  
  for(i = 0; i < num; i++)
    {
    if(tracks[i].in_file && !tracks[i].eof &&
       (!ret || (tracks[i].time < min_time)))
      {
      min_time = tracks[i].time;
      ret = &tracks[i];
      }
    }
  return ret;
  }

static int init_demultiplex(char * filename)
  {
  int i;
  char * tmp_string;
  
  lqt_codec_info_t * codec_info;
  
  file = lqt_open_read(filename);
  if(!file)
    {
    fprintf(stderr, "Couldn't open file %s\n", filename);
    return -1;
    }
    
  num_audio_tracks = quicktime_audio_tracks(file);
  num_video_tracks = quicktime_video_tracks(file);
  
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
      lqt_compression_info_dump(audio_tracks[i].ci);
      
      codec_info = find_audio_encoder(audio_tracks[i].ci->id);
      if(!codec_info)
        {
        fprintf(stderr, "No audio encoder found for compressed writing\n");
        continue;
        }
      
      sprintf(tmp_string, "%s_audio_%02d.mov", prefix, i+1);
      audio_tracks[i].in_file = file;
      audio_tracks[i].out_file = lqt_open_write(tmp_string, LQT_FILE_QT);
      
      if(!lqt_writes_compressed(lqt_get_file_type(audio_tracks[i].out_file),
                                audio_tracks[i].ci, codec_info))
        {
        fprintf(stderr, "Audio track %d cannot be written compressed\n", i+1);
        quicktime_close(audio_tracks[i].out_file);
        remove(tmp_string);
        audio_tracks[i].out_file = NULL;
        audio_tracks[i].in_file = NULL;
        continue;
        }
      total_tracks++;
      
      lqt_add_audio_track_compressed(audio_tracks[i].out_file,
                                     audio_tracks[i].ci, codec_info);
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
      lqt_compression_info_dump(video_tracks[i].ci);

      codec_info = find_video_encoder(video_tracks[i].ci->id);
      if(!codec_info)
        {
        fprintf(stderr, "No video encoder found for compressed writing of %s\n",
                lqt_compression_id_to_string(video_tracks[i].ci->id));
        continue;
        }
      
      sprintf(tmp_string, "%s_video_%02d.mov", prefix, i+1);
      video_tracks[i].in_file = file;
      video_tracks[i].out_file = lqt_open_write(tmp_string, LQT_FILE_QT);

      if(!lqt_writes_compressed(lqt_get_file_type(video_tracks[i].out_file),
                                video_tracks[i].ci, codec_info))
        {
        fprintf(stderr, "Video track %d cannot be written compressed\n", i+1);
        quicktime_close(video_tracks[i].out_file);
        remove(tmp_string);
        video_tracks[i].out_file = NULL;
        video_tracks[i].in_file = NULL;
        continue;
        }
      total_tracks++;

      lqt_add_video_track_compressed(video_tracks[i].out_file,
                                     video_tracks[i].ci, codec_info);
      video_tracks[i].in_index = i;
      video_tracks[i].out_index = 0;
      }
    }

  free(tmp_string);
  
  return 1;
  }

static void cleanup_demultiplex()
  {
  int i;
  for(i = 0; i < num_audio_tracks; i++)
    {
    if(audio_tracks[i].out_file)
      quicktime_close(audio_tracks[i].out_file);
    }
  for(i = 0; i < num_video_tracks; i++)
    {
    if(video_tracks[i].out_file)
      quicktime_close(video_tracks[i].out_file);
    }
  quicktime_close(file);
  }

static int init_multiplex(char ** in_files, int num_in_files, char * out_file)
  {
  int i, j;
  int audio_index;
  int video_index;
  char * pos;
  lqt_file_type_t type = LQT_FILE_QT;
  lqt_codec_info_t * codec_info;
  
  files = calloc(num_in_files, sizeof(*files));
  num_files = num_in_files;
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

      codec_info = find_audio_encoder(audio_tracks[i].ci->id);
      if(!codec_info)
        {
        fprintf(stderr, "No audio encoder found for compressed writing\n");
        continue;
        }
      
      if(!lqt_writes_compressed(lqt_get_file_type(file),
                                audio_tracks[audio_index].ci, codec_info))
        {
        fprintf(stderr, "Audio track %d of file %s cannot be written compressed\n",
                j+1, in_files[i]);
        audio_index++;
        continue;
        }

      lqt_compression_info_dump(audio_tracks[i].ci);
      
      audio_tracks[audio_index].in_index = j;
      audio_tracks[audio_index].out_index = audio_index;
      audio_tracks[audio_index].in_file = files[i].file;
      audio_tracks[audio_index].out_file = file;

      lqt_add_audio_track_compressed(audio_tracks[audio_index].out_file,
                                     audio_tracks[audio_index].ci, codec_info);
      
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

      codec_info = find_video_encoder(video_tracks[i].ci->id);
      if(!codec_info)
        {
        fprintf(stderr, "No video encoder found for compressed writing of %s\n",
                lqt_compression_id_to_string(video_tracks[i].ci->id));
        continue;
        }

      if(!lqt_writes_compressed(lqt_get_file_type(file),
                                video_tracks[video_index].ci, codec_info))
        {
        fprintf(stderr, "Video track %d of file %s cannot be written compressed\n",
                j+1, in_files[i]);
        video_index++;
        continue;
        }
      lqt_compression_info_dump(video_tracks[i].ci);
      video_tracks[video_index].in_index = j;
      video_tracks[video_index].out_index = video_index;
      video_tracks[video_index].in_file = files[i].file;
      video_tracks[video_index].out_file = file;

      lqt_add_video_track_compressed(video_tracks[video_index].out_file,
                                     video_tracks[video_index].ci, codec_info);
      
      video_index++;
      total_tracks++;
      
      }
    
    }
  return 1;
  }

static void cleanup_multiplex()
  {
  int i;
  for(i = 0; i < num_files; i++)
    {
    if(files[i].file)
      quicktime_close(files[i].file);
    }
  if(file)
    quicktime_close(file);
  }

int main(int argc, char ** argv)
  {
  int i;
  int first_file;
  int nfiles;
  
  track_t * atrack, *vtrack;
  lqt_packet_t p;

  memset(&p, 0, sizeof(p));
  
  if(argc < 2)
    {
    print_usage();
    return 0;
    }

  /* Parse agruments */

  nfiles = argc - 1;

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
      nfiles -= 2;
      }
    i++;
    }

  if(!nfiles)
    {
    print_usage();
    return -1;
    }

  /* Query registry */
  
  audio_encoders = lqt_query_registry(1, 0, 1, 0);
  video_encoders = lqt_query_registry(0, 1, 1, 0);
  
  if(nfiles == 1)
    {
    /* Demultiplex */
    if(!init_demultiplex(argv[argc-1]))
      return -1;
    }
  else
    {
    /* Multiplex */
    
    if(!init_multiplex(&argv[first_file], nfiles-1, argv[argc-1]))
      return -1;
    }

  if(!total_tracks)
    {
    fprintf(stderr, "No tracks to demultiplex\n");
    return -1;
    }

  /* Transmultiplex */

  while(1)
    {
    atrack = tracks_min_time(audio_tracks, num_audio_tracks);
    vtrack = tracks_min_time(video_tracks, num_video_tracks);

    if(atrack && vtrack)
      {
      fprintf(stderr, "A: %f, V: %f\n", atrack->time, vtrack->time);
      
      if(atrack->time < vtrack->time)
        audio_iteration(atrack, &p);
      else
        video_iteration(vtrack, &p);
      }
    else if(atrack)
      audio_iteration(atrack, &p);
    else if(vtrack)
      video_iteration(vtrack, &p);
    else
      break;
    }

  /* Cleanup */

  if(nfiles == 1)
    cleanup_demultiplex();
  else
    cleanup_multiplex();

  if(files)
    free(files);
  if(audio_tracks)
    free(audio_tracks);
  if(video_tracks)
    free(video_tracks);

  lqt_destroy_codec_info(audio_encoders);
  lqt_destroy_codec_info(video_encoders);
  
  return 0;
  }
