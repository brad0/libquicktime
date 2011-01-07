/*******************************************************************************
 qt2text.c

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
#include <stdio.h>
#include <string.h>

/* Dump all text strings from a quicktime file */

static void usage()
  {
  printf("usage: qt2text [-t track] <file>\n");
  exit(0);
  }

int main(int argc, char ** argv)
  {
  char * filename;
  int track = 1;
  quicktime_t * qtfile;

  char * text_buffer = (char*)0;
  int text_buffer_alloc = 0;
  int timescale, len;
  int i;
  int64_t timestamp;
  int64_t duration;
  
  if(argc < 2)
    {
    usage();
    }
  
  if(argc > 2)
    {
    if(!strcmp(argv[1], "-t"))
      {
      if(argc < 4)
        usage();
      else
        {
        track = atoi(argv[2]);
        filename = argv[3];
        }
      }
    else
      usage();
    }
  else
    filename = argv[1];

  qtfile = lqt_open_read(filename);

  if(!qtfile)
    {
    fprintf(stderr, "Cannot open file %s\n", filename);
    return -1;
    }
  
  if(lqt_text_tracks(qtfile) < track)
    {
    fprintf(stderr, "No text track %d\n", track);
    return -1;
    }
  
  timescale = lqt_text_time_scale(qtfile, track-1);
  len = lqt_text_samples(qtfile, track-1);

  for(i = 0; i < len; i++)
    {
    if(!lqt_read_text(qtfile, track-1, &text_buffer, &text_buffer_alloc,
                      &timestamp, &duration))
      fprintf(stderr, "Reading text sample %d failed\n", i);
    else
      {
      printf("Time: %"PRId64" (%f seconds), Duration: %"PRId64" (%f seconds), String:\n",
             timestamp,
             (double)(timestamp)/(double)(timescale),
             duration,
             (double)(duration)/(double)(timescale));
      printf("\"%s\"\n", text_buffer);
      }
    }
  if(text_buffer)
    free(text_buffer);
  quicktime_close(qtfile);
  return 0;
  }
