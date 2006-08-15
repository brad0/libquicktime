#include "funcprotos.h"
#include <quicktime/colormodels.h>
#include <workarounds.h>

#include <stdlib.h>
#include <string.h>

#ifndef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

typedef struct
{
	int coded_w, coded_h;
	uint8_t *buffer;
        int buffer_alloc;
        int initialized;
} quicktime_yv12_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_yv12_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	free(codec->buffer);
	free(codec);
	return 0;
}

static void initialize(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
	quicktime_yv12_codec_t *codec = codec_base->priv;
	if(!codec->initialized)
	{
/* Init private items */
		codec->coded_w = (int)(vtrack->track->tkhd.track_width / 2);
		codec->coded_w *= 2;
		codec->coded_h = (int)(vtrack->track->tkhd.track_height / 2);
		codec->coded_h *= 2;
                codec->buffer_alloc = codec->coded_w * codec->coded_h + 
                  codec->coded_w * codec->coded_h / 2;

                codec->buffer = malloc(codec->buffer_alloc);
		codec->initialized = 1;
	}
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
        uint8_t * ptr;
        int bytes;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yv12_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int64_t y_size, u_size, v_size;
	int result = 0;

        if(!row_pointers)
          {
          file->vtracks[track].stream_cmodel = BC_YUV420P;
          return 0;
          }
        
        initialize(vtrack);

	y_size = codec->coded_h * codec->coded_w;
	u_size = codec->coded_h * codec->coded_w / 4;
	v_size = codec->coded_h * codec->coded_w / 4;

        bytes = lqt_read_video_frame(file, &codec->buffer, &codec->buffer_alloc,
                                     vtrack->current_position, NULL, track);

        if(bytes <= 0)
          return -1;

        ptr = codec->buffer;
        memcpy(row_pointers[0], ptr, y_size);
        ptr+= y_size;

        memcpy(row_pointers[1], ptr, u_size);
        ptr+= u_size;

        memcpy(row_pointers[1], ptr, v_size);
        ptr+= v_size;
        
	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yv12_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int result = 0;
	int64_t y_size, u_size, v_size;
	quicktime_atom_t chunk_atom;

        if(!row_pointers)
          {
          file->vtracks[track].stream_cmodel = BC_YUV420P;
          return 0;
          }
        
        initialize(vtrack);

	y_size = codec->coded_h * codec->coded_w;
	u_size = codec->coded_h * codec->coded_w / 4;
	v_size = codec->coded_h * codec->coded_w / 4;

	quicktime_write_chunk_header(file, trak, &chunk_atom);
        result = !quicktime_write_data(file, row_pointers[0], y_size);
        if(!result) result = !quicktime_write_data(file, row_pointers[1], u_size);
        if(!result) result = !quicktime_write_data(file, row_pointers[2], v_size);
        quicktime_write_chunk_footer(file, 
		trak,
		vtrack->current_chunk,
		&chunk_atom, 
		1);

	vtrack->current_chunk++;
	return result;
}

void quicktime_init_codec_yv12(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_yv12_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
}

