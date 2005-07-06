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
	unsigned char *work_buffer;
        int work_buffer_alloc;
	int coded_w, coded_h;

/* The YUV2 codec requires a bytes per line that is a multiple of 4 */
	int bytes_per_line;
	int initialized;

        int is_2vuy;
        uint8_t ** rows;
  } quicktime_yuv2_codec_t;

static int quicktime_delete_codec_yuv2(quicktime_video_map_t *vtrack)
{
	quicktime_yuv2_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->work_buffer) free(codec->work_buffer);
        if(codec->rows) free(codec->rows);
        free(codec);
	return 0;
}

static void convert_encode_yuv2(quicktime_t * file, int track, quicktime_yuv2_codec_t *codec, unsigned char **row_pointers)
{
        uint8_t *out_row, *in_y, *in_u, *in_v;
	int y, x;
	for(y = 0; y < codec->coded_h; y++)
	{
		out_row = codec->work_buffer + y * codec->bytes_per_line;

                in_y = row_pointers[0] + y * file->vtracks[track].stream_row_span;
                in_u = row_pointers[1] + y * file->vtracks[track].stream_row_span_uv;
                in_v = row_pointers[2] + y * file->vtracks[track].stream_row_span_uv;

                for(x = 0; x < codec->bytes_per_line; )
		{
			*out_row++ = *in_y++;
			*out_row++ = (int)(*in_u++) - 128;
			*out_row++ = *in_y++;
			*out_row++ = (int)(*in_v++) - 128;
			x += 4;
		}
	}
}

static void convert_decode_yuv2(quicktime_t * file, int track, quicktime_yuv2_codec_t *codec, unsigned char **row_pointers)
{
        uint8_t *in_row, *out_y, *out_u, *out_v;
	int y, x;
	for(y = 0; y < codec->coded_h; y++)
          {
          in_row = codec->work_buffer + y * codec->bytes_per_line;
          
          out_y = row_pointers[0] + y * file->vtracks[track].stream_row_span;
          out_u = row_pointers[1] + y * file->vtracks[track].stream_row_span_uv;
          out_v = row_pointers[2] + y * file->vtracks[track].stream_row_span_uv;
          
          for(x = 0; x < codec->bytes_per_line; )
            {
            *out_y++ = *in_row++;
            *out_u++ = (int)(*in_row++) + 128;
            *out_y++ = *in_row++;
            *out_v++ = (int)(*in_row++) + 128;
            x += 4;
            }
          }
}

static void convert_encode_2vuy(quicktime_yuv2_codec_t *codec, unsigned char **row_pointers)
{
        uint8_t *in_row, *out_row;
	int y, x;
	for(y = 0; y < codec->coded_h; y++)
	{
		out_row = codec->work_buffer + y * codec->bytes_per_line;
		in_row = row_pointers[y];
		for(x = 0; x < codec->bytes_per_line; )
		{
                        out_row[0] = in_row[1]; /* U */
			out_row[1] = in_row[0]; /* Y */
			out_row[2] = in_row[3]; /* V */
			out_row[3] = in_row[2]; /* Y */
			x += 4;
                        out_row += 4;
                        in_row += 4;
		}
	}
}

static void convert_decode_2vuy(quicktime_yuv2_codec_t *codec, unsigned char **row_pointers)
{
        uint8_t *in_row, *out_row;
        int y, x;
	for(y = 0; y < codec->coded_h; y++)
          {
          in_row = codec->work_buffer + y * codec->bytes_per_line;
          out_row = row_pointers[y];
          for(x = 0; x < codec->bytes_per_line; )
            {
            out_row[1] = in_row[0]; /* U */
            out_row[0] = in_row[1]; /* Y */
            out_row[3] = in_row[2]; /* V */
            out_row[2] = in_row[3]; /* Y */
            x += 4;
            out_row += 4;
            in_row += 4;
            }
          }
}


static void initialize(quicktime_video_map_t *vtrack, quicktime_yuv2_codec_t *codec,
                       int width, int height)
{
	if(!codec->initialized)
	{
/* Init private items */
        codec->coded_w = ((width+3)/4)*4;
                //		codec->coded_h = (int)((float)vtrack->track->tkhd.track_height / 4 + 0.5) * 4;
                codec->coded_h = height;
		codec->bytes_per_line = codec->coded_w * 2;
                codec->work_buffer_alloc = codec->bytes_per_line * codec->coded_h;
		codec->work_buffer = calloc(1, codec->work_buffer_alloc);
		codec->initialized = 1;
         }
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int64_t bytes;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yuv2_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = quicktime_video_width(file, track);
	int height = quicktime_video_height(file, track);
	int result = 0;

        if(!row_pointers)
          {
          if(codec->is_2vuy)
            vtrack->stream_cmodel = BC_YUV422;
          else
            vtrack->stream_cmodel = BC_YUVJ422P;
          return 0;
          }

        initialize(vtrack, codec, width, height);

	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);

        result = !quicktime_read_data(file, codec->work_buffer, bytes);
        if(codec->is_2vuy)
          convert_decode_2vuy(codec, row_pointers);
        else
          convert_decode_yuv2(file, track, codec, row_pointers);
        
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
	quicktime_atom_t chunk_atom;

        if(!row_pointers)
          {
          if(codec->is_2vuy)
            vtrack->stream_cmodel = BC_YUV422;
          else
            vtrack->stream_cmodel = BC_YUVJ422P;
          return 0;
          }
        
	initialize(vtrack, codec, width, height);

	bytes = height * codec->bytes_per_line;
	buffer = codec->work_buffer;

        if(codec->is_2vuy)
          convert_encode_2vuy(codec, row_pointers);
        else
          convert_encode_yuv2(file, track, codec, row_pointers);
        quicktime_write_chunk_header(file, trak, &chunk_atom);
        result = !quicktime_write_data(file, buffer, bytes);
        
	quicktime_write_chunk_footer(file, 
		trak,
		vtrack->current_chunk,
		&chunk_atom, 
		1);

	vtrack->current_chunk++;
	return result;
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
	codec_base->fourcc = QUICKTIME_YUV2;
	codec_base->title = "Component Video";
	codec_base->desc = "YUV 4:2:2 (yuv2)";
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
	codec_base->fourcc = QUICKTIME_2VUY;
	codec_base->title = "Component Video";
	codec_base->desc = "YUV 4:2:2 (2vuy)";
        codec = (quicktime_yuv2_codec_t *)(codec_base->priv);
        codec->is_2vuy = 1;
}

