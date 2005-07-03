/* 
 * rtjpeg_codec.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
 * Based entirely upon qtpng.c from libquicktime 
 * (http://libquicktime.sf.net).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *  
 */

/*
 *  Support for video, which is no multiple of 16 by
 *  Burkhard Plaum
 */

#define BLOCK_SIZE 16

#include "config.h"
#include <string.h>
#include <quicktime/colormodels.h>
#include <funcprotos.h>
#include "rtjpeg_codec.h"
#include <lqt.h>

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_rtjpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->compress_struct) RTjpeg_close(codec->compress_struct);
	if(codec->rows) lqt_rows_free(codec->rows);
        if(codec->write_buffer) free(codec->write_buffer);

	if(codec->decompress_struct) RTjpeg_close(codec->decompress_struct);
	if(codec->read_buffer) free(codec->read_buffer);
        free(codec);
        return 0;
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
        int result = 0;
	int t;
	int buffer_size;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
        //	quicktime_trak_t *trak = vtrack->track;
	quicktime_rtjpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

        if(!row_pointers)
          {
          vtrack->stream_cmodel = BC_YUV420P;
          return 0;
          }
        
        if(!codec->decompress_struct) {
		codec->decompress_struct = RTjpeg_init();
		if(!codec->decompress_struct)
			return -1;
                codec->qt_height = quicktime_video_height(file, track);
                codec->qt_width  = quicktime_video_width(file, track);
                codec->jpeg_height = BLOCK_SIZE * ((codec->qt_height + BLOCK_SIZE - 1) / (BLOCK_SIZE));
                codec->jpeg_width  = BLOCK_SIZE * ((codec->qt_width  + BLOCK_SIZE - 1) / (BLOCK_SIZE));
		t = RTJ_YUV420;
		RTjpeg_set_format(codec->decompress_struct, &t);

                codec->rows = lqt_rows_alloc(codec->jpeg_width, codec->jpeg_height,
                                             vtrack->stream_cmodel, &(codec->rowspan),
                                             &(codec->rowspan_uv));
                
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
                  RTjpeg_decompress(codec->decompress_struct, codec->read_buffer, codec->rows);
	}
        
        lqt_rows_copy(row_pointers, codec->rows, codec->qt_width, codec->qt_height,
                      codec->rowspan, codec->rowspan_uv,
                      vtrack->stream_row_span, vtrack->stream_row_span_uv, vtrack->stream_cmodel);

        
	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;
	int i;
        quicktime_atom_t chunk_atom;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_rtjpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

        if(!row_pointers)
          {
          vtrack->stream_cmodel = BC_YUV420P;
          return 0;
          }
        
	if(!codec->compress_struct) {
		codec->compress_struct = RTjpeg_init();
		if(!codec->compress_struct)
			return -1;
                codec->qt_height = trak->tkhd.track_height;
                codec->qt_width  = trak->tkhd.track_width;
                codec->jpeg_height = BLOCK_SIZE * ((codec->qt_height + BLOCK_SIZE - 1) / (BLOCK_SIZE));
                codec->jpeg_width  = BLOCK_SIZE * ((codec->qt_width  + BLOCK_SIZE - 1) / (BLOCK_SIZE));

		RTjpeg_set_size(codec->compress_struct, &(codec->jpeg_width), &(codec->jpeg_height));
		i = codec->Q;
		i *= 255;
		i /= 100;
		RTjpeg_set_quality(codec->compress_struct, &i);
		i = RTJ_YUV420;
		RTjpeg_set_format(codec->compress_struct, &i);
		RTjpeg_set_intra(codec->compress_struct, &codec->K, &codec->LQ, &codec->CQ);

                codec->rows = lqt_rows_alloc(codec->jpeg_width, codec->jpeg_height,
                                             vtrack->stream_cmodel, &(codec->rowspan),
                                             &(codec->rowspan_uv));
                
		codec->write_buffer = malloc(codec->jpeg_width * codec->jpeg_height * 3 / 2 + 100);
		if(!codec->write_buffer)
			return -1;
	}

        lqt_rows_copy(codec->rows, row_pointers, codec->qt_width, codec->qt_height,
                      vtrack->stream_row_span, vtrack->stream_row_span_uv,
                      codec->rowspan, codec->rowspan_uv,
                      vtrack->stream_cmodel);
        
                
	i = RTjpeg_compress(codec->compress_struct, codec->write_buffer, codec->rows);

        quicktime_write_chunk_header(file, trak, &chunk_atom);
        result = !quicktime_write_data(file, 
				codec->write_buffer, 
				i);
        quicktime_write_chunk_footer(file,
                trak,
                vtrack->current_chunk,
                &chunk_atom,
                1);
        
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
}
