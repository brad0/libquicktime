#include "funcprotos.h"
#include <quicktime/colormodels.h>
#include <workarounds.h>
#include "yv12.h"

#include <stdlib.h>

#ifndef CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#endif

typedef struct
{
	cmodel_yuv_t yuv_table;
	int coded_w, coded_h;
	unsigned char *work_buffer;
	int initialized;
} quicktime_yv12_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_yv12_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	free(codec->work_buffer);
	free(codec);
	return 0;
}

static int reads_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUV420P);
}

#if 0
static int writes_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_YUV420P);
}
#endif
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
		cmodel_init_yuv(&codec->yuv_table);
		codec->work_buffer = malloc(codec->coded_w * codec->coded_h + 
			codec->coded_w * codec->coded_h / 2);
		codec->initialized = 1;
	}
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int64_t bytes;
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

	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);
        
        result = !quicktime_read_data(file, row_pointers[0], y_size);
        result = !quicktime_read_data(file, row_pointers[1], u_size);
        result = !quicktime_read_data(file, row_pointers[2], v_size);

	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yv12_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int result = 0;
	int64_t y_size, u_size, v_size;
	int64_t bytes = (int64_t)0;
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
	bytes = quicktime_add3(y_size, u_size, v_size);

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
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->fourcc = QUICKTIME_YUV420;
	codec_base->title = "YUV 4:2:0 Planar";
	codec_base->desc = "YUV 4:2:0 Planar";
}

