/*******************************************************************************
 recover.c

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
#include "lqt_fseek.h"
/*#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <inttypes.h>*/

#define WIDTH 720
#define HEIGHT 480
#define FRAMERATE (float)30000/1001
#define CHANNELS 2
#define SAMPLERATE 96000
#define BITS 24
#define TEMP_FILE "/tmp/temp.mov"






#define SEARCH_FRAGMENT (int64_t)0x1000

void usage()
{
	printf("Recover JPEG and PCM audio in a corrupted movie.\n"
		   "Usage: recover [-w width] [-h height] [-f framerate]\n"
		   "\t[-c audiochannels] [-r samplerate] [-b audiobits] [-t tempfile] <input>\n"
		   "Compiled in defaults:\n"
		   "   WIDTH %d\n"
		   "   HEIGHT %d\n"
		   "   FRAMERATE %.2f\n"
		   "   CHANNELS %d\n"
		   "   SAMPLERATE %d\n"
		   "   BITS %d\n"
		   "   TEMP_FILE %s\n",
		   WIDTH, HEIGHT,
		   FRAMERATE,
		   CHANNELS,
		   SAMPLERATE,
		   BITS,
		   TEMP_FILE
		   );

	exit( -1 );
}

int main(int argc, char *argv[])
{
	FILE *in;
	FILE *temp;
	quicktime_t *out;
	int64_t current_byte, ftell_byte;
	int64_t jpeg_start = 0, jpeg_end = 0;
	int64_t audio_start, audio_end = 0;
	unsigned char *search_buffer = calloc(1, SEARCH_FRAGMENT);
	unsigned char *copy_buffer = 0;
	int i, found_jfif, found_eoi;
	int64_t file_size;
	struct stat status;
	unsigned char data[8];
	struct stat ostat;

	long width = WIDTH, height = HEIGHT;
	float frame_rate = FRAMERATE;
	int channels = CHANNELS, sample_rate = SAMPLERATE, bits = BITS;
	char *temp_file = TEMP_FILE;
	char *input_file = NULL;
	int have_input = 0;

	if(argc < 2)	   
	{				   
		printf("Recover JPEG and PCM audio in a corrupted movie.\n"
			   "Usage: recover [-w width] [-h height] [-f framerate]\n"
			   "\t[-c audiochannels] [-r samplerate] [-b audiobits] <input> [<output>]\n"
			   "Compiled in defaults:\n"
			   "   WIDTH %ld\n"
			   "   HEIGHT %ld\n"
			   "   FRAMERATE %.2f\n"
			   "   CHANNELS %d\n"
			   "   SAMPLERATE %d\n"
			   "   BITS %d\n"
			   "   TEMP_FILE %s\n",
			   width, height,
			   frame_rate,
			   channels,
			   sample_rate,
			   bits,
			   temp_file);
		exit(1);
	}
	
	for(i = 1; i < argc; i++)
	{
		if(!strcmp(argv[i], "-f"))
		{
			if(i + 1 < argc)
			{
				frame_rate = atof(argv[++i]);
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
		if(!strcmp(argv[i], "-r"))
		{
			if(i + 1 < argc)
				sample_rate = atol(argv[++i]);
			else
				usage();
		}
		else
		if(!strcmp(argv[i], "-c"))
		{
			if(i + 1 < argc)
				channels = atol(argv[++i]);
			else
				usage();
		}
		else
		if(!strcmp(argv[i], "-b"))
		{
			if(i + 1 < argc)
				bits = atol(argv[++i]);
			else
				usage();
		}
		else
		if(!strcmp(argv[i], "-t"))
		{
			if(i + 1 < argc)
				temp_file = argv[++i];
			else
				usage();
		}
		else if( ! have_input )
		{
			input_file = argv[i];
			have_input = 1;
		}
		else
		{
			usage();
		}
	}

	printf( "Using options:\n"
			"   WIDTH %ld\n"
			"   HEIGHT %ld\n"
			"   FRAMERATE %.2f\n"
			"   CHANNELS %d\n"
			"   SAMPLERATE %d\n"
			"   BITS %d\n"
			"   INPUT %s\n"
			"   TEMP_FILE %s\n",
			width, height,
			frame_rate,
			channels,
			sample_rate,
			bits,
			input_file,
			temp_file);

		
	in = fopen(input_file, "rb+");
	out = quicktime_open(temp_file, 0, 1);

	if(!in)
	{
		perror("open input");
		exit(1);
	}
	if(!out)
	{
		perror("open temp");
		exit(1);
	}

	quicktime_set_audio(out, 
						channels,
						sample_rate,
						bits, 
						QUICKTIME_TWOS);
	quicktime_set_video(out, 
						1, 
						width, height,
						frame_rate,
						QUICKTIME_JPEG);
	audio_start = (int64_t)0x10;
	found_jfif = 0;
	found_eoi = 0;
	ftell_byte = 0;

	if(fstat(fileno(in), &status))
		perror("get_file_length fstat:");
	file_size = status.st_size;
	

//printf("recover %lld\n", file_size);
	while(ftell_byte < file_size)
	{
// Search forward for JFIF
		current_byte = ftell_byte;
		fread(search_buffer, SEARCH_FRAGMENT, 1, in);
		ftell_byte += SEARCH_FRAGMENT;
		for(i = 0; i < SEARCH_FRAGMENT - 4; i++)
		{
			if(!found_jfif)
			{
				if(search_buffer[i] == 'J' &&
					search_buffer[i + 1] == 'F' &&
					search_buffer[i + 2] == 'I' &&
					search_buffer[i + 3] == 'F')
				{
					current_byte += i - 6;
					fseeko(in, current_byte, SEEK_SET);
					ftell_byte = current_byte;
					found_jfif = 1;
					audio_end = jpeg_start = current_byte;
					break;
				}
			}
			else
			if(!found_eoi)
			{
				if(search_buffer[i] == 0xff &&
					search_buffer[i + 1] == 0xd9)
				{
					current_byte += i + 2;
					fseeko(in, current_byte, SEEK_SET);
					ftell_byte = current_byte;
					found_eoi = 1;
					audio_start = jpeg_end = current_byte;
					break;
				}
			}
		}

// Write audio chunk
		if(found_jfif && !found_eoi && audio_end - audio_start > 0)
		{
			long samples = (audio_end - audio_start) / (channels * bits / 8);
			quicktime_update_tables(out, 
						out->atracks[0].track, 
						audio_start, 
						out->atracks[0].current_chunk, 
						out->atracks[0].current_position, 
						samples, 
						0);
			out->atracks[0].current_position += samples;
			out->atracks[0].current_chunk++;
printf("got audio %llx - %llx = %llx\r", audio_end, audio_start, audio_end - audio_start);
fflush(stdout);
			audio_start = audio_end;
		}
		else
// Write video chunk
		if(found_jfif && found_eoi)
		{
			quicktime_update_tables(out,
						out->vtracks[0].track,
						jpeg_start,
						out->vtracks[0].current_chunk,
						out->vtracks[0].current_position,
						1,
						jpeg_end - jpeg_start);
			out->vtracks[0].current_position++;
			out->vtracks[0].current_chunk++;
			found_jfif = 0;
			found_eoi = 0;
		}
		else
		{
			fseeko(in, current_byte + SEARCH_FRAGMENT - 4, SEEK_SET);
			ftell_byte = current_byte + SEARCH_FRAGMENT - 4;
		}
	}
printf("\n\n");
// Force header out
	quicktime_close(out);

// Transfer header
	fseeko(in, 0x8, SEEK_SET);

	data[0] = (ftell_byte & 0xff00000000000000LL) >> 56;
	data[1] = (ftell_byte & 0xff000000000000LL) >> 48;
	data[2] = (ftell_byte & 0xff0000000000LL) >> 40;
	data[3] = (ftell_byte & 0xff00000000LL) >> 32;
	data[4] = (ftell_byte & 0xff000000LL) >> 24;
	data[5] = (ftell_byte & 0xff0000LL) >> 16;
	data[6] = (ftell_byte & 0xff00LL) >> 8;
	data[7] = ftell_byte & 0xff;
	fwrite(data, 8, 1, in);

	fseeko(in, ftell_byte, SEEK_SET);
	stat(temp_file, &ostat);
	temp = fopen(temp_file, "rb");
	fseeko(temp, 0x10, SEEK_SET);
	copy_buffer = calloc(1, ostat.st_size);
	fread(copy_buffer, ostat.st_size, 1, temp);
	fclose(temp);
	fwrite(copy_buffer, ostat.st_size, 1, in);

	fclose(in);
	exit (EXIT_SUCCESS);
}




