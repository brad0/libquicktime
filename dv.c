#include "colormodels.h"
#include "funcprotos.h"
#include "libdv.h"
#include "quicktime.h"


typedef struct
{
	dv_t *dv;
	unsigned char *data;
	unsigned char *temp_frame, **temp_rows;
} quicktime_dv_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

	if(codec->dv) dv_delete(codec->dv);
	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->temp_rows) free(codec->temp_rows);
	free(codec->data);
	free(codec);
	return 0;
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	long bytes;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	int result = 0;
	int i;

	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);
	result = !quicktime_read_data(file, (char*)codec->data, bytes);

	if(codec->dv)
	{
		if(file->color_model == BC_YUV422 &&
			file->in_x == 0 && 
			file->in_y == 0 && 
			file->in_w == width &&
			file->in_h == height &&
			file->out_w == width &&
			file->out_h == height)
		{
			dv_read_video(codec->dv, 
				row_pointers, 
				codec->data, 
				bytes, 
				BC_YUV422);
		}
		else
		{

			if(!codec->temp_frame)
			{
				codec->temp_frame = malloc(720 * 576 * 2);
				codec->temp_rows = malloc(sizeof(unsigned char*) * 576);
				for(i = 0; i < 576; i++)
					codec->temp_rows[i] = codec->temp_frame + 720 * 2 * i;
			}



//printf("decode 1 %d %d\n", width, height);
			dv_read_video(codec->dv, 
				codec->temp_rows, 
				codec->data, 
				bytes, 
				BC_YUV422);
//printf("decode 2\n");

			cmodel_transfer(row_pointers, 
				codec->temp_rows,
				row_pointers[0],
				row_pointers[1],
				row_pointers[2],
				codec->temp_rows[0],
				codec->temp_rows[1],
				codec->temp_rows[2],
				file->in_x, 
				file->in_y, 
				file->in_w, 
				file->in_h,
				0, 
				0, 
				file->out_w, 
				file->out_h,
				BC_YUV422, 
				file->color_model,
				0,
				width,
				file->out_w);
//printf("decode 3\n");
		}
	}

	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	longest offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int width_i = 720;
	int height_i = (height <= 480) ? 480 : 576;
	int i;
	unsigned char **input_rows;
	int norm = (height_i == 480) ? DV_NTSC : DV_PAL;
	int data_length = (norm == DV_NTSC) ? DV_NTSC_SIZE : DV_PAL_SIZE;
	int result = 0;

	if(file->color_model == BC_YUV422 &&
		width == width_i &&
		height == height_i)
	{
		input_rows = row_pointers;
	}
	else
	{
		if(!codec->temp_frame)
		{
			codec->temp_frame = malloc(720 * 576 * 2);
			codec->temp_rows = malloc(sizeof(unsigned char*) * 576);
			for(i = 0; i < 576; i++)
				codec->temp_rows[i] = codec->temp_frame + 720 * 2 * i;
		}
		
		
		
		
		cmodel_transfer(codec->temp_rows, /* Leave NULL if non existent */
			row_pointers,
			codec->temp_rows[0], /* Leave NULL if non existent */
			codec->temp_rows[1],
			codec->temp_rows[2],
			row_pointers[0], /* Leave NULL if non existent */
			row_pointers[1],
			row_pointers[2],
			0,        /* Dimensions to capture from input frame */
			0, 
			MIN(width, width_i), 
			MIN(height, height_i),
			0,       /* Dimensions to project on output frame */
			0, 
			MIN(width, width_i), 
			MIN(height, height_i),
			file->color_model, 
			BC_YUV422,
			0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
			width,       /* For planar use the luma rowspan */
			width_i);


		input_rows = codec->temp_rows;
	}

	bzero(codec->data, DV_PAL_SIZE);




//printf("encode 1 %d %d\n", width_i, height_i);
	dv_write_video(codec->dv,
		codec->data,
		input_rows,
		BC_YUV422,
		norm);
//printf("encode 2 %d %d\n", width_i, height_i);

	result = !quicktime_write_data(file, codec->data, data_length);
	quicktime_update_tables(file,
						file->vtracks[track].track,
						offset,
						file->vtracks[track].current_chunk,
						file->vtracks[track].current_position,
						1,
						data_length);
	file->vtracks[track].current_chunk++;
//printf("encode 3\n", width_i, height_i);

	return result;
}

// Logic: DV contains a mixture of 420 and 411 so can only output 444 or 422

static int quicktime_reads_colormodel_dv(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_YUV888 ||
		colormodel == BC_YUV422);
}

void quicktime_init_codec_dv(quicktime_video_map_t *vtrack)
{
	quicktime_dv_codec_t *codec;
	int i;

/* Init public items */
	((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_dv_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = decode;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->decode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->encode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = quicktime_reads_colormodel_dv;

/* Init private items */
	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	codec->dv = dv_new();
	codec->data = calloc(1, 140000);
}
