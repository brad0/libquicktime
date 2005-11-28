#include "funcprotos.h"
#include <quicktime/colormodels.h>
#include <quicktime/quicktime.h>
#include <workarounds.h>

#include <stdlib.h>
#include <string.h>

typedef struct
{
	unsigned char *work_buffer;
} quicktime_v410_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_v410_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->work_buffer) free(codec->work_buffer);
	free(codec);
	return 0;
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
        uint32_t input_i;
        uint8_t * in_ptr;
        uint16_t * out_y, * out_u, * out_v;
        int i, j;
	int64_t bytes;
	int result = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_v410_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;

        if(!row_pointers)
          {
          vtrack->stream_cmodel = BC_YUV444P16;
          return 0;
          }

        if(!codec->work_buffer)
		codec->work_buffer = malloc(vtrack->track->tkhd.track_width * 
			vtrack->track->tkhd.track_height *
			4);


	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);
	result = !quicktime_read_data(file, codec->work_buffer, bytes);

        in_ptr = codec->work_buffer;
        
	for(i = 0; i < height; i++)
          {
          out_y = (uint16_t*)(row_pointers[0] + i * file->vtracks[track].stream_row_span);
          out_u = (uint16_t*)(row_pointers[1] + i * file->vtracks[track].stream_row_span_uv);
          out_v = (uint16_t*)(row_pointers[2] + i * file->vtracks[track].stream_row_span_uv);

          for(j = 0; j < width; j++)
            {
            /* v410 is LITTLE endian!! */
            input_i = in_ptr[0] | (in_ptr[1] << 8) | (in_ptr[2] << 16) | (in_ptr[3] << 24);

            *(out_v++) = (input_i & 0xffc00000) >> 16; /* V */
            *(out_y++) = (input_i & 0x3ff000) >> 6;    /* Y */
            *(out_u++) = (input_i & 0xffc) << 4;       /* U */

            in_ptr += 4;
            }
          }
	return result;
}







static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_v410_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	int bytes = width * height * 4;
	int result = 0;
	int i, j;
	quicktime_atom_t chunk_atom;
        uint16_t * in_y, * in_u, * in_v;
        uint8_t * out_ptr;
        uint32_t output_i;
        
        if(!row_pointers)
          {
          vtrack->stream_cmodel = BC_YUV444P16;
          return 0;
          }

        if(!codec->work_buffer)
          codec->work_buffer = malloc(width * height * 4);

        out_ptr = codec->work_buffer;
	for(i = 0; i < height; i++)
          {
          in_y = (uint16_t*)(row_pointers[0] + i * file->vtracks[track].stream_row_span);
          in_u = (uint16_t*)(row_pointers[1] + i * file->vtracks[track].stream_row_span_uv);
          in_v = (uint16_t*)(row_pointers[2] + i * file->vtracks[track].stream_row_span_uv);
          
          for(j = 0; j < width; j++)
            {
            output_i =
              ((*in_v & 0xffc0) << 16) |
              ((*in_y & 0xffc0) << 6) |
              ((*in_u & 0xffc0) >> 4);
            *(out_ptr++) = (output_i & 0xff);
            *(out_ptr++) = (output_i & 0xff00) >> 8;
            *(out_ptr++) = (output_i & 0xff0000) >> 16;
            *(out_ptr++) = (output_i & 0xff000000) >> 24;
            in_y ++;
            in_u ++;
            in_v ++;
            }
          }
        
	quicktime_write_chunk_header(file, trak, &chunk_atom);
	result = !quicktime_write_data(file, codec->work_buffer, bytes);
	quicktime_write_chunk_footer(file, 
		trak,
		vtrack->current_chunk,
		&chunk_atom, 
		1);
//printf("quicktime_encode_yv12 2\n");

	vtrack->current_chunk++;
	
	return result;
}

void quicktime_init_codec_v410(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_v410_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
}

