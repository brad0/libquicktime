#include "funcprotos.h"
#include <quicktime/colormodels.h>
#include "quicktime.h"
#include "yuv2.h"

/* U V values are signed but Y R G B values are unsigned! */
/*
 *      R = Y               + 1.40200 * V
 *      G = Y - 0.34414 * U - 0.71414 * V
 *      B = Y + 1.77200 * U
 */

/*
 *		Y =  0.2990 * R + 0.5870 * G + 0.1140 * B
 *		U = -0.1687 * R - 0.3310 * G + 0.5000 * B
 *		V =  0.5000 * R - 0.4187 * G - 0.0813 * B  
 */


typedef struct
{
	int use_float;
	long rtoy_tab[256], gtoy_tab[256], btoy_tab[256];
	long rtou_tab[256], gtou_tab[256], btou_tab[256];
	long rtov_tab[256], gtov_tab[256], btov_tab[256];

	long vtor_tab[256], vtog_tab[256];
	long utog_tab[256], utob_tab[256];
	long *vtor, *vtog, *utog, *utob;
	
	unsigned char *work_buffer;
	int coded_w, coded_h;

/* The YUV2 codec requires a bytes per line that is a multiple of 4 */
	int bytes_per_line;
	int initialized;

        int is_2vuy;
  } quicktime_yuv2_codec_t;

static int quicktime_delete_codec_yuv2(quicktime_video_map_t *vtrack)
{
	quicktime_yuv2_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->work_buffer) free(codec->work_buffer);
	free(codec);
	return 0;
}
#if 0
static int quicktime_reads_colormodel_yuv2(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUV422P);
}
#endif

static void convert_encode_yuv2(quicktime_yuv2_codec_t *codec, unsigned char **row_pointers)
{
	int y, x;
	for(y = 0; y < codec->coded_h; y++)
	{
		unsigned char *out_row = codec->work_buffer + y * codec->bytes_per_line;
		unsigned char *in_row = row_pointers[y];
		for(x = 0; x < codec->bytes_per_line; )
		{
			*out_row++ = *in_row++;
			*out_row++ = (int)(*in_row++) - 128;
			*out_row++ = *in_row++;
			*out_row++ = (int)(*in_row++) - 128;
			x += 4;
		}
	}
}

static void convert_decode_yuv2(quicktime_yuv2_codec_t *codec, unsigned char **row_pointers)
{
	int y, x;
	for(y = 0; y < codec->coded_h; y++)
	{
		unsigned char *in_row = row_pointers[y];
		for(x = 0; x < codec->bytes_per_line; )
		{
			in_row[1] += 128;
			in_row[3] += 128;
			x += 4;
			in_row += 4;
		}
	}
}

static void convert_encode_2vuy(quicktime_yuv2_codec_t *codec, unsigned char **row_pointers)
{
	int y, x;
	for(y = 0; y < codec->coded_h; y++)
	{
		unsigned char *out_row = codec->work_buffer + y * codec->bytes_per_line;
		unsigned char *in_row = row_pointers[y];
		for(x = 0; x < codec->bytes_per_line; )
		{
			out_row[0] = in_row[3];
			out_row[1] = in_row[0];
			out_row[2] = in_row[1];
			out_row[3] = in_row[2];
			x += 4;
                        out_row += 4;
                        in_row += 4;
		}
	}
}

static void convert_decode_2vuy(quicktime_yuv2_codec_t *codec, unsigned char **row_pointers)
{
        unsigned char tmp[4];
        int y, x;
	for(y = 0; y < codec->coded_h; y++)
	{
		unsigned char *in_row = row_pointers[y];
		for(x = 0; x < codec->bytes_per_line; )
		{
                        tmp[0] = in_row[0];
                        tmp[1] = in_row[1];
                        tmp[2] = in_row[2];
                        tmp[3] = in_row[3];

                        in_row[0] = tmp[1];
                        in_row[1] = tmp[2];
                        in_row[2] = tmp[3];
                        in_row[3] = tmp[0];

                        x += 4;
			in_row += 4;
		}
	}
}


static void initialize(quicktime_video_map_t *vtrack, quicktime_yuv2_codec_t *codec,
                       int width, int height)
{
	int i;
	if(!codec->initialized)
	{
/* Init private items */
               for(i = 0; i < 256; i++) {
/* compression */
			codec->rtoy_tab[i] = (long)( 0.2990 * 65536 * i);
			codec->rtou_tab[i] = (long)(-0.1687 * 65536 * i);
			codec->rtov_tab[i] = (long)( 0.5000 * 65536 * i);

			codec->gtoy_tab[i] = (long)( 0.5870 * 65536 * i);
			codec->gtou_tab[i] = (long)(-0.3320 * 65536 * i);
			codec->gtov_tab[i] = (long)(-0.4187 * 65536 * i);

			codec->btoy_tab[i] = (long)( 0.1140 * 65536 * i);
			codec->btou_tab[i] = (long)( 0.5000 * 65536 * i);
			codec->btov_tab[i] = (long)(-0.0813 * 65536 * i);
		}

		codec->vtor = &(codec->vtor_tab[128]);
		codec->vtog = &(codec->vtog_tab[128]);
		codec->utog = &(codec->utog_tab[128]);
		codec->utob = &(codec->utob_tab[128]);

		for(i = -128; i < 128; i++)
		{
/* decompression */
			codec->vtor[i] = (long)( 1.4020 * 65536 * i);
			codec->vtog[i] = (long)(-0.7141 * 65536 * i);

			codec->utog[i] = (long)(-0.3441 * 65536 * i);
			codec->utob[i] = (long)( 1.7720 * 65536 * i);
		}

		codec->coded_w = (int)((float)width / 4 + 0.5) * 4;
                //		codec->coded_h = (int)((float)vtrack->track->tkhd.track_height / 4 + 0.5) * 4;
                codec->coded_h = height;
		codec->bytes_per_line = codec->coded_w * 2;
		codec->work_buffer = malloc(codec->bytes_per_line *
								codec->coded_h);
		codec->initialized = 1;
         }
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int64_t bytes, y;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yuv2_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);
	int result = 0;
	initialize(vtrack, codec, width, height);

	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);
	if(file->vtracks[track].color_model == BC_YUV422 &&
		file->in_x == 0 && 
		file->in_y == 0 && 
		file->in_w == width &&
		file->in_h == height &&
		file->out_w == width &&
		file->out_h == height)
	{
		result = !quicktime_read_data(file, row_pointers[0], bytes);
                if(codec->is_2vuy)
                  convert_decode_2vuy(codec, row_pointers);
                else
                  convert_decode_yuv2(codec, row_pointers);
	}
	else
	{
		unsigned char *input_rows[height];
		result = !quicktime_read_data(file, codec->work_buffer, bytes);
		for(y = 0; y < height; y++)
			input_rows[y] = &codec->work_buffer[y * codec->bytes_per_line];
                if(codec->is_2vuy)
                  convert_decode_2vuy(codec, input_rows);
                else
                  convert_decode_yuv2(codec, input_rows);

		cmodel_transfer(row_pointers, 
			input_rows,
			row_pointers[0],
			row_pointers[1],
			row_pointers[2],
			0,
			0,
			0,
			file->in_x, 
			file->in_y, 
			file->in_w, 
			file->in_h,
			0, 
			0, 
			file->out_w, 
			file->out_h,
			BC_YUV422, 
			file->vtracks[track].color_model,
			0,
			codec->coded_w,
			file->out_w);
	}

	

	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_yuv2_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int result = 1;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	int64_t bytes;
	unsigned char *buffer;
	int i;
	quicktime_atom_t chunk_atom;

	initialize(vtrack, codec, width, height);

	bytes = height * codec->bytes_per_line;
	buffer = codec->work_buffer;
	if(file->vtracks[track].color_model == BC_YUV422)
	{
                if(codec->is_2vuy)
                  convert_encode_2vuy(codec, row_pointers);
                else
                  convert_encode_yuv2(codec, row_pointers);
		quicktime_write_chunk_header(file, trak, &chunk_atom);
		result = !quicktime_write_data(file, buffer, bytes);
	}
	else
	{
		unsigned char **temp_rows = malloc(sizeof(unsigned char*) * height);
		for(i = 0; i < height; i++)
			temp_rows[i] = buffer + i * codec->bytes_per_line;

		cmodel_transfer(temp_rows, 
			row_pointers,
			0,
			0,
			0,
			row_pointers[0],
			row_pointers[1],
			row_pointers[2],
			0, 
			0, 
			width, 
			height,
			0, 
			0, 
			width, 
			height,
			file->vtracks[track].color_model,
			BC_YUV422, 
			0,
			width,
			codec->coded_w);
                if(codec->is_2vuy)
                  convert_encode_2vuy(codec, temp_rows);
                else
                  convert_encode_yuv2(codec, temp_rows);

		quicktime_write_chunk_header(file, trak, &chunk_atom);
		result = !quicktime_write_data(file, buffer, bytes);
		free(temp_rows);
	}

	quicktime_write_chunk_footer(file, 
		trak,
		vtrack->current_chunk,
		&chunk_atom, 
		1);

	vtrack->current_chunk++;
	return result;
}

static int reads_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{

	return (colormodel == BC_RGB888 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUV422P ||
		colormodel == BC_YUV422);
}

static int writes_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{

	return (colormodel == BC_RGB888 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUV422P ||
		colormodel == BC_YUV422);
}

void quicktime_init_codec_yuv2(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_yuv2_codec_t));
	codec_base->delete_vcodec = quicktime_delete_codec_yuv2;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->writes_colormodel = writes_colormodel;
	codec_base->fourcc = QUICKTIME_YUV2;
	codec_base->title = "Component Video";
	codec_base->desc = "YUV 4:2:2";
}

void quicktime_init_codec_2vuy(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;
        quicktime_yuv2_codec_t * codec;
/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_yuv2_codec_t));
	codec_base->delete_vcodec = quicktime_delete_codec_yuv2;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->writes_colormodel = writes_colormodel;
	codec_base->fourcc = "2vuy";
	codec_base->title = "Component Video";
	codec_base->desc = "YUV 4:2:2";
        codec = (quicktime_yuv2_codec_t *)(codec_base->priv);
        codec->is_2vuy = 1;
}

