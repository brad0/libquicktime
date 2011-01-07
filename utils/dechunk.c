/*******************************************************************************
 dechunk.c

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

static int usage(void)
{
	printf("usage: dechunk [-f framerate] <input movie> <output prefix>\n");
	printf("	Movies containing rgb frames are written as ppm images.\n");
	exit(1);
	return 0;
}

int main(int argc, char *argv[])
{
	quicktime_t *file;
	FILE *output;
	int i, j;
	int64_t length;
	char string[1024], *prefix = 0, *input = 0;
	uint8_t *data = 0;
	int bytes = 0, old_bytes = 0;
	float output_rate = 0;
	float input_rate;
	int64_t input_frame;
	int64_t new_length;
	int width, height;
	int rgb_to_ppm = 0;

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
				output_rate = atof(argv[++i]);
			}
			else
				usage();
		}
		else
		if(j == 0)
		{
			input = argv[i];
			j++;
		}
		else
		if(j == 1)
		{
			prefix = argv[i];
			j++;
		}
	}

	if(!prefix || !input) usage();

	if(!(file = quicktime_open(input, 1, 0)))
	{
		printf("Open failed\n");
		exit(1);
	}
	
	if(!quicktime_video_tracks(file))
	{
		printf("No video tracks.\n");
		exit(1);
	}
	
	if(quicktime_match_32(quicktime_video_compressor(file, 0), QUICKTIME_RAW))
	{
		printf("Converting to ppm.\n");
		rgb_to_ppm = 1;
	}

	length = quicktime_video_length(file, 0);
	input_rate = quicktime_frame_rate(file, 0);
	if(!output_rate) output_rate = input_rate;
	new_length = output_rate / input_rate * length;
	width = quicktime_video_width(file, 0);
	height = quicktime_video_height(file, 0);

	for(i = 0; i < new_length; i++)
	{
/* Get output file */
		sprintf(string, "%s%06d", prefix, i);
		if(!(output = fopen(string, "wb")))
		{
			perror("Open failed");
			exit(1);
		}

/* Get input frame */
		input_frame = (int64_t)(input_rate / output_rate * i);
		bytes = quicktime_frame_size(file, input_frame, 0);

		if(data)
		{
			if(bytes > old_bytes) { free(data); data = 0; }
		}

		if(!data)
		{
			old_bytes = bytes;
			data = malloc(bytes);
		}

		quicktime_set_video_position(file, input_frame, 0);
		quicktime_read_data(file, data, bytes);
		if(rgb_to_ppm)
		{
			fprintf(output, "P6\n%d %d\n%d\n", width, height, 0xff);
		}

		if(!fwrite(data, bytes, 1, output))
		{
			perror("write failed");
		}
		fclose(output);
	}

	quicktime_close(file);
	exit (EXIT_SUCCESS);
}
