#include "funcprotos.h"
#include "quicktime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdint.h>

#define FSEEK fseeko64


#define WIDTH 720
#define HEIGHT 480
#define FRAMERATE (float)30000/1001
#define CHANNELS 2
#define SAMPLERATE 96000
#define BITS 24
#define TEMP_FILE "/tmp/temp.mov"






#define SEARCH_FRAGMENT (longest)0x1000

int main(int argc, char *argv[])
{
	FILE *in;
	FILE *temp;
	quicktime_t *out;
	longest current_byte, ftell_byte;
	longest jpeg_start, jpeg_end;
	longest audio_start, audio_end;
	unsigned char *search_buffer = calloc(1, SEARCH_FRAGMENT);
	unsigned char *copy_buffer = 0;
	int i, found_jfif, found_eoi;
	longest file_size;
	struct stat status;
	unsigned char data[8];
	struct stat ostat;

	printf("Recover JPEG and PCM audio in a corrupted movie.\n"
		"Usage: recover <input>\n"
		"Compiled settings:\n"
		"   WIDTH %d\n"
		"   HEIGHT %d\n"
		"   FRAMERATE %.2f\n"
		"   CHANNELS %d\n"
		"   SAMPLERATE %d\n"
		"   BITS %d\n",
		WIDTH,
		HEIGHT,
		FRAMERATE,
		CHANNELS,
		SAMPLERATE,
		BITS);
	if(argc < 2)	   
	{				   
		exit(1);
	}

	in = fopen(argv[1], "rb+");
	out = quicktime_open(TEMP_FILE, 0, 1);

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
		CHANNELS, 
		SAMPLERATE, 
		BITS, 
		QUICKTIME_TWOS);
	quicktime_set_video(out, 
		1, 
		WIDTH, 
		HEIGHT, 
		FRAMERATE, 
		QUICKTIME_JPEG);
	audio_start = (longest)0x10;
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
					FSEEK(in, current_byte, SEEK_SET);
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
					FSEEK(in, current_byte, SEEK_SET);
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
			long samples = (audio_end - audio_start) / (CHANNELS * BITS / 8);
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
			FSEEK(in, current_byte + SEARCH_FRAGMENT - 4, SEEK_SET);
			ftell_byte = current_byte + SEARCH_FRAGMENT - 4;
		}
	}
printf("\n\n");
// Force header out
	quicktime_close(out);

// Transfer header
	FSEEK(in, 0x8, SEEK_SET);

	data[0] = (ftell_byte & 0xff00000000000000LL) >> 56;
	data[1] = (ftell_byte & 0xff000000000000LL) >> 48;
	data[2] = (ftell_byte & 0xff0000000000LL) >> 40;
	data[3] = (ftell_byte & 0xff00000000LL) >> 32;
	data[4] = (ftell_byte & 0xff000000LL) >> 24;
	data[5] = (ftell_byte & 0xff0000LL) >> 16;
	data[6] = (ftell_byte & 0xff00LL) >> 8;
	data[7] = ftell_byte & 0xff;
	fwrite(data, 8, 1, in);

	FSEEK(in, ftell_byte, SEEK_SET);
	stat(TEMP_FILE, &ostat);
	temp = fopen(TEMP_FILE, "rb");
	FSEEK(temp, 0x10, SEEK_SET);
	copy_buffer = calloc(1, ostat.st_size);
	fread(copy_buffer, ostat.st_size, 1, temp);
	fclose(temp);
	fwrite(copy_buffer, ostat.st_size, 1, in);

	fclose(in);
}




