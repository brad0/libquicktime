#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include <string.h>
#include <quicktime/quicktime.h>
#include <quicktime/lqt.h>
#include <funcprotos.h>
#include "config.h"

static int total_vcodecs = 0;
static int total_acodecs = 0;
static quicktime_extern_video_t *vcodecs = NULL; 
static quicktime_extern_audio_t *acodecs = NULL; 


#define CODEC_PREFIX "quicktime_codec_"

int quicktime_find_vcodec(char *fourcc)
{
  	int i;
  	for(i = 0; i < total_vcodecs; i++)
  	{
    	if(quicktime_match_32(fourcc, vcodecs[i].fourcc))
      		return i;
  	}
  	return -1;
}

int quicktime_find_acodec(char *fourcc)
{
	int i;
	for(i = 0; i < total_acodecs; i++)
	{
    	if(quicktime_match_32(fourcc, acodecs[i].fourcc))
    		return i;
	}
	return -1;
}

static int decode_audio_external(quicktime_t *file, 
				int16_t *output_i, 
				float *output_f, 
				long samples, 
				int track,
				int channel)
{
    return -1;
}

static int encode_audio_external(quicktime_t *file, 
				int16_t **input_i, 
				float **input_f, 
				int track, 
				long samples)
{
    return -1;
}

static int decode_video_external(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track)
{
	unsigned char *input;
	unsigned char *output = row_pointers[0]; 
	int	index = 0;
	int error = -1;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	unsigned int bytes;
	char *compressor;

	compressor = quicktime_video_compressor(file, track);
	index = quicktime_find_vcodec(compressor);

	if(index >= 0)
	{
    	quicktime_set_video_position(file, vtrack->current_position, track);
    	bytes = quicktime_frame_size(file, vtrack->current_position, track);

    	if(bytes <= 0) 
		{
    		fprintf(stderr, "decode_video_external: frame size equal %d\n", bytes);
    		return -1;
    	}

    	input = (unsigned char*)malloc(bytes);
    	if(input) 
		{
    	    if(quicktime_read_data(file, input, bytes)) 
				error = vcodecs[index].decode(file, track, bytes, input, output); 
    	  	else 
				fprintf(stderr, "decode_video_external: can't read data from file\n");

    	} 
		else 
			fprintf(stderr, "decode_video_external: Can't allocate decoding buffer");

    	free(input);
	}
	else 
	{
    	fprintf(stderr, 
	    	"decode_video_external: Can't find the corresponding codec: %s",
	    	quicktime_video_compressor(file, track));
	}
    return error;
}



int quicktime_vcodec_size()
{
	return total_vcodecs;
}

int quicktime_acodec_size()
{
	return total_acodecs;
}





int quicktime_register_vcodec(char *fourcc, 
	void (*init_vcodec)(quicktime_video_map_t *))
{
	int index = quicktime_find_vcodec(fourcc);

	if(index == -1)
	{
    	total_vcodecs++;
    	vcodecs = (quicktime_extern_video_t *)realloc(vcodecs,
	    	total_vcodecs * sizeof(quicktime_extern_video_t));

    	vcodecs[total_vcodecs - 1].init = init_vcodec;
    	quicktime_copy_char32(vcodecs[total_vcodecs - 1].fourcc, fourcc);
    	return total_vcodecs - 1;
	}
	return index;
}

int quicktime_register_acodec(char *fourcc, 
	void (*init_acodec)(quicktime_audio_map_t *))
{
	int index = quicktime_find_acodec(fourcc);

	if(index == -1)
	{
    	total_acodecs++;
    	acodecs = (quicktime_extern_audio_t *)realloc(acodecs,
			total_acodecs * sizeof(quicktime_extern_audio_t));
    	acodecs[total_acodecs - 1].init = init_acodec;
    	quicktime_copy_char32(acodecs[total_acodecs - 1].fourcc, fourcc);
    	return total_acodecs - 1;
	}
	return index;
}

static int select_codec(const struct dirent *ptr)
{
	if(strncmp(ptr->d_name, CODEC_PREFIX, 15) == 0)
    	return 1;
	else 
    	return 0;
}

#if 0 /* Not needed in libquicktime */
int quicktime_init_vcodec_core(int index, quicktime_video_map_t *vtrack)
{
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = vcodecs[total_vcodecs - 1].codec.delete_vcodec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = vcodecs[total_vcodecs - 1].codec.decode_video;
	((quicktime_codec_t*)vtrack->codec)->encode_video = vcodecs[total_vcodecs - 1].codec.encode_video;

	vcodecs[index].init(vtrack);
	return 0;
}

int quicktime_init_acodec_core(int index, quicktime_audio_map_t *atrack)
{
	((quicktime_codec_t*)atrack->codec)->delete_acodec = acodecs[total_acodecs - 1].codec.delete_acodec;
	((quicktime_codec_t*)atrack->codec)->decode_audio = acodecs[total_acodecs - 1].codec.decode_audio;
	((quicktime_codec_t*)atrack->codec)->encode_audio = acodecs[total_acodecs - 1].codec.encode_audio;

	acodecs[index].init(atrack);
	return 0;
}
#endif /* libquicktime */

