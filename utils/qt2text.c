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
      printf("Time: %lld (%f seconds), Duration: %lld (%f seconds), String:\n",
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
