/* 
   rtjpeg.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
   Based entirely upon qtpng.c from libquicktime 
   (http://libquicktime.sf.net).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    
*/

#include <string.h>
#include <quicktime/colormodels.h>
#include <funcprotos.h>
#include "rtjpeg.h"

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_rtjpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->compress_struct) RTjpeg_close(codec->compress_struct);
	if(codec->encode_frame) free(codec->encode_frame);
	if(codec->write_buffer) free(codec->write_buffer);

	if(codec->decompress_struct) RTjpeg_close(codec->decompress_struct);
	if(codec->decode_frame) free(codec->decode_frame);
	if(codec->read_buffer) free(codec->read_buffer);
	return 0;
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;
	int t;
	int buffer_size;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
	quicktime_rtjpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int use_temp = (BC_YUV420P != file->color_model ||
		file->in_x != 0 ||
		file->in_y != 0 ||
		file->in_w != width ||
		file->in_h != height ||
		file->out_w != width ||
		file->out_h != height);
	
	if(!codec->decompress_struct) {
		codec->decompress_struct = RTjpeg_init();
		if(!codec->decompress_struct)
			return -1;
		t = RTJ_YUV420;
		RTjpeg_set_format(codec->decompress_struct, &t);
		codec->decode_frame = malloc(width * height * 3 / 2);
		if(!codec->decode_frame)
			return -1;
		codec->decode_rows[0] = codec->decode_frame;
		codec->decode_rows[1] = codec->decode_rows[0] + width * height;
		codec->decode_rows[2] = codec->decode_rows[1] + (width * height) / 4;
	}

	quicktime_set_video_position(file, vtrack->current_position, track);
	buffer_size = quicktime_frame_size(file, vtrack->current_position, track);
	if(buffer_size > codec->read_buffer_size)
	{
		free(codec->read_buffer);
		codec->read_buffer = malloc(buffer_size + 1024);
		if(!codec->read_buffer)
			return -1;
		codec->read_buffer_size = buffer_size + 1024;
	}
	result = !quicktime_read_data(file, codec->read_buffer, buffer_size);
	if(buffer_size > 0) {
		if(!result)
			RTjpeg_decompress(codec->decompress_struct, codec->read_buffer, codec->decode_rows);
	}
	if(use_temp) {
		cmodel_transfer(row_pointers, 
			codec->decode_rows,
			row_pointers[0],
			row_pointers[1],
			row_pointers[2],
			codec->decode_rows[0],
			codec->decode_rows[1],
			codec->decode_rows[2],
			file->in_x, 
			file->in_y, 
			file->in_w, 
			file->in_h,
			0, 
			0, 
			file->out_w, 
			file->out_h,
			BC_YUV420P, 
			file->color_model,
			0,
			width,
			file->out_w);
	} else {
		memcpy(row_pointers[0], codec->decode_rows[0], width * height);
		memcpy(row_pointers[1], codec->decode_rows[1], (width * height) / 4);
		memcpy(row_pointers[2], codec->decode_rows[2], (width * height) / 4);
	}
	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	longest offset = quicktime_position(file);
	int result = 0;
	int i;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_rtjpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;

	if(!codec->compress_struct) {
		codec->compress_struct = RTjpeg_init();
		if(!codec->compress_struct)
			return -1;
		RTjpeg_set_size(codec->compress_struct, &width, &height);
		i = codec->Q;
		i *= 255;
		i /= 100;
		RTjpeg_set_quality(codec->compress_struct, &i);
		i = RTJ_YUV420;
		RTjpeg_set_format(codec->compress_struct, &i);
		RTjpeg_set_intra(codec->compress_struct, &codec->K, &codec->LQ, &codec->CQ);
		if(file->color_model != BC_YUV420P) {
			codec->encode_frame = malloc(width * height * 3 / 2);
			if(!codec->encode_frame)
				return -1;
			codec->encode_rows[0] = codec->encode_frame;
			codec->encode_rows[1] = codec->encode_rows[0] + width * height;
			codec->encode_rows[2] = codec->encode_rows[1] + (width * height) / 4;
		}
		codec->write_buffer = malloc(width * height * 3 / 2 + 100);
		if(!codec->write_buffer)
			return -1;
	}
	
	if(file->color_model != BC_YUV420P) {
		cmodel_transfer(codec->encode_rows, 
			row_pointers,
			codec->encode_rows[0],
			codec->encode_rows[1],
			codec->encode_rows[2],
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
			file->color_model,
			BC_YUV420P, 
			0,
			width,
			width);
		i = RTjpeg_compress(codec->compress_struct, codec->write_buffer, codec->encode_rows);
	} else {
		i = RTjpeg_compress(codec->compress_struct, codec->write_buffer, row_pointers);
	}

	result = !quicktime_write_data(file, 
				codec->write_buffer, 
				i);

	quicktime_update_tables(file,
						file->vtracks[track].track,
						offset,
						file->vtracks[track].current_chunk,
						file->vtracks[track].current_position,
						1,
						i);

	file->vtracks[track].current_chunk++;
	return result;
}

static int set_parameter(quicktime_t *file, 
                         int track, 
                         char *key, 
                         void *value)
{
	quicktime_rtjpeg_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;

	if(!strcasecmp(key, "rtjpeg_quality"))
		codec->Q = *(int*)value;
	if(!strcasecmp(key, "rtjpeg_key_rate"))
		codec->K = *(int*)value;
	if(!strcasecmp(key, "rtjpeg_luma_quant"))
		codec->LQ = *(int*)value;
	if(!strcasecmp(key, "rtjpeg_chroma_quant"))
		codec->CQ = *(int*)value;
	return 0;
}

static int reads_colormodel(quicktime_t *file, 
                            int colormodel, 
                            int track)
{
	return (colormodel == BC_YUV420P);
}

static int writes_colormodel(quicktime_t *file, 
                             int colormodel, 
                             int track)
{
	return (colormodel == BC_YUV420P);
}

void quicktime_init_codec_rtjpeg(quicktime_video_map_t *vtrack)
{
	quicktime_rtjpeg_codec_t *codec;

	codec = calloc(1, sizeof(quicktime_rtjpeg_codec_t));
	if(!codec)
		return;
	memset(codec, 0, sizeof(quicktime_rtjpeg_codec_t));
	codec->Q = 100;
	codec->K = 25;
	codec->LQ = 1;
	codec->CQ = 1;
	
	((quicktime_codec_t*)vtrack->codec)->priv = (void *)codec;
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = decode;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = reads_colormodel;
	((quicktime_codec_t*)vtrack->codec)->writes_colormodel = writes_colormodel;
}
