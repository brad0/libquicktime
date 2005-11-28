#include "funcprotos.h"
#include <quicktime/colormodels.h>
#include <quicktime/quicktime.h>
#include <workarounds.h>

#include <stdlib.h>
#include <string.h>


typedef struct
{
	unsigned char *work_buffer;
} quicktime_v308_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_v308_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->work_buffer) free(codec->work_buffer);
	free(codec);
	return 0;
}





static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
        uint8_t *in_ptr, *out_y, *out_u, *out_v;
        int i, j;
	int64_t bytes;
	int result = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_v308_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;

        if(!row_pointers)
          {
          //          vtrack->stream_cmodel = BC_VYU888;
          vtrack->stream_cmodel = BC_YUV444P;
          return 0;
          }
        
        if(!codec->work_buffer)
          codec->work_buffer = malloc(width * height * 3);
        
	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);
	result = !quicktime_read_data(file, codec->work_buffer, bytes);

        in_ptr = codec->work_buffer;
	for(i = 0; i < height; i++)
          {
          out_y = row_pointers[0] + i * file->vtracks[track].stream_row_span;
          out_u = row_pointers[1] + i * file->vtracks[track].stream_row_span_uv;
          out_v = row_pointers[2] + i * file->vtracks[track].stream_row_span_uv;
          for(j = 0; j < width; j++)
            {
            *out_y = in_ptr[1]; /* Y */
            *out_u = in_ptr[2]; /* U */
            *out_v = in_ptr[0]; /* V */

            out_y++;
            out_u++;
            out_v++;
            
            in_ptr += 3;
            }
          }
        
	return result;
}







static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
        uint8_t *in_y, *in_u, *in_v, *out_ptr;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_v308_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	int bytes = width * height * 3;
	int result = 0;
	int i, j;
	quicktime_atom_t chunk_atom;

        if(!row_pointers)
          {
          vtrack->stream_cmodel = BC_YUV444P;
          //          vtrack->stream_cmodel = BC_VYU888;
          return 0;
          }
        
        if(!codec->work_buffer)
          codec->work_buffer = malloc(width * height * 3);

        out_ptr = codec->work_buffer;
	for(i = 0; i < height; i++)
          {
          in_y = row_pointers[0] + i * file->vtracks[track].stream_row_span;
          in_u = row_pointers[1] + i * file->vtracks[track].stream_row_span_uv;
          in_v = row_pointers[2] + i * file->vtracks[track].stream_row_span_uv;
          
          for(j = 0; j < width; j++)
            {

            out_ptr[1] = *in_y; /* Y */
            out_ptr[2] = *in_u; /* U */
            out_ptr[0] = *in_v; /* V */
            
            out_ptr += 3;
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

	vtrack->current_chunk++;
	
	return result;
}

void quicktime_init_codec_v308(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_v308_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
}

