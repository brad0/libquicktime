/*******************************************************************************
 rechunk.c

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
#include <limits.h>
#include <errno.h>

static int usage(void)
{
	printf("usage: qtrechunk [-f framerate] [-w width] [-h height] [-c fourcc] [-b bitdepth] [-l file_list | <input frames>] <output movie>\n");
	printf("	Concatenate input frames into a Quicktime movie.\n\n");
	printf("	-l <file_list> reads filenames of the frame from\n");
        printf("           <file_list>, which must contain one filename per line\n");
	printf("        -f specifies the framerate either as float or rational number\n");
        printf("           e.g. -f 30000:1001\n");
	exit(1);
	return 0;
}

static char ** add_frames_from_file(char ** input_frames,
                             int * total_input_frames, char * list_filename)
  {
  FILE * input;
  char * pos;
  char filename[PATH_MAX+10];

  input = fopen(list_filename, "r");
  if(!input)
    {
    fprintf(stderr, "Cannot open %s: %s\n", list_filename, strerror(errno));
    exit(1);
    return (char**)0;
    }

  while(fgets(filename, PATH_MAX+10, input))
    {
    /* Delete trailing '\n' and '\r' */

    pos = &(filename[strlen(filename)-1]);

    while(1)
      {
      if(*pos == '\r')
        *pos = '\0';
      if(*pos == '\n')
        *pos = '\0';
      else
        break;

      if(pos == filename)
        return input_frames;
      
      pos--;      
      }

    /* Add filename */

    (*total_input_frames)++;
    input_frames = realloc(input_frames, sizeof(char*) * *total_input_frames);
    input_frames[*total_input_frames - 1] = strdup(filename);
    //    fprintf(stderr, "Adding file %s\n", input_frames[*total_input_frames - 1]);
    }
  return input_frames;
  }

int main(int argc, char *argv[])
{
        lqt_codec_info_t ** codec_info;
	quicktime_t *file;
	FILE *input;
	int i, j;
	char *output = 0;
	uint8_t *data = 0;
	int bytes = 0;
	float output_rate = 0;
	char **input_frames = 0;
	int total_input_frames = 0;
	int width = 720, height = 480, bit_depth = 24;
	char compressor[5] = "yv12";
        int output_rate_num = 0;
	int output_rate_den = 0;
	
	if(argc < 3)
	{
		usage();
	}

	for(i = 1, j = 0; i < argc; i++)
	{
		if(!strcmp(argv[i], "-f"))
		{
			if(i + 1 < argc)
			{
                                i++;
				if(sscanf(argv[i], "%d:%d", &output_rate_num,
                                          &output_rate_den) < 2)
                                  {
                                  output_rate = atof(argv[i]);
                                  output_rate_num = 0;
                                  }
                                fprintf(stderr, "Rate: %f (%d:%d)\n", output_rate,
                                        output_rate_num, output_rate_den);
                        }
			else
				usage();
		}
		else
		if(!strcmp(argv[i], "-w"))
		{
			if(i + 1 < argc)
				width = atol(argv[++i]);
			else
				usage();
		}
		else
		if(!strcmp(argv[i], "-h"))
		{
			if(i + 1 < argc)
				height = atol(argv[++i]);
			else
				usage();
		}
		else
		if(!strcmp(argv[i], "-c"))
		{
			if(i + 1 < argc)
			{
				strncpy(compressor, argv[++i], 4);
			}
			else
				usage();
		}
		else
		if(!strcmp(argv[i], "-b"))
		{
			if(i + 1 < argc)
				bit_depth = atoi(argv[++i]);
			else
				usage();
		}
		else if(i == argc - 1)
		{
			output = argv[i];
		}
		else
		if(!strcmp(argv[i], "-l"))
		{
			if(i + 1 < argc)
                                input_frames =
                                  add_frames_from_file(input_frames,
                                                       &total_input_frames, argv[++i]);
			else
				usage();
		}
		else
		{
			total_input_frames++;
			input_frames = realloc(input_frames, sizeof(char*) * total_input_frames);
			input_frames[total_input_frames - 1] = argv[i];
		}
	}

	input = fopen(output, "rb");
	if(input != NULL)
	{
		printf("Output file already exists.\n");
		exit(1);
	}

	if(!(file = quicktime_open(output, 0, 1)))
	{
		printf("Open failed\n");
		exit(1);
	}
	if(!output_rate_num)
          quicktime_set_video(file, 1, width, height, output_rate, compressor);
        else
	  {
          codec_info = lqt_find_video_codec(compressor, 1);
	  lqt_add_video_track(file, width, height, output_rate_den, output_rate_num, 
			      *codec_info);
          lqt_destroy_codec_info(codec_info);
          }
	quicktime_set_depth(file, bit_depth, 0);
	for(i = 0; i < total_input_frames; i++)
	{
/* Get output file */
		if(!(input = fopen(input_frames[i], "rb")))
		{
			perror("Open failed");
			continue;
		}

/* Get input frame */
		fseek(input, 0, SEEK_END);
		bytes = ftell(input);
		fseek(input, 0, SEEK_SET);
		data = realloc(data, bytes);

		fread(data, bytes, 1, input);
		quicktime_write_frame(file, data, bytes, 0);
		fclose(input);

                if(!(i % 10))
                  {
                  if(i)
                    {
                    for(j = 0; j < 17; j++)
                      putchar(0x08);
                    }
                  printf("%6.2f%% Completed", 100.0 * (float)(i+1) / (float)(total_input_frames));
                  fflush(stdout);
                  }
        }
        printf("\n");
	quicktime_close(file);

	return 0;
}
