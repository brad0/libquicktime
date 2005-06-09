#include "funcprotos.h"
#include <quicktime/colormodels.h>
#include <quicktime/quicktime.h>
#include <workarounds.h>
#include "v408.h"

#include <stdlib.h>
#include <string.h>

typedef struct
{
	unsigned char *work_buffer;
} quicktime_v408_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_v408_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->work_buffer) free(codec->work_buffer);
	free(codec);
	return 0;
}

static int reads_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888 ||
		colormodel == BC_RGBA8888 ||
		colormodel == BC_RGB161616 ||
		colormodel == BC_RGBA16161616 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUVA8888 ||
		colormodel == BC_YUV161616 ||
		colormodel == BC_YUVA16161616 ||
		colormodel == BC_RGB565 ||
		colormodel == BC_BGR888 ||
		colormodel == BC_BGR8888);
}

static int writes_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888 ||
		colormodel == BC_RGBA8888 ||
		colormodel == BC_RGB161616 ||
		colormodel == BC_RGBA16161616 ||
		colormodel == BC_YUV888 ||
		colormodel == BC_YUVA8888 ||
		colormodel == BC_YUV161616 ||
		colormodel == BC_YUVA16161616);
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
        uint8_t * in_ptr, *out_ptr;
        int i, j;
	int64_t bytes;
	int result = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_v408_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;

        if(!row_pointers)
          {
          //          vtrack->stream_cmodel = BC_UYVA8888;
          vtrack->stream_cmodel = BC_YUVA8888;
          }

        if(!codec->work_buffer)
          codec->work_buffer = malloc(width * height * 4);
        
	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);
	result = !quicktime_read_data(file, codec->work_buffer, bytes);

        in_ptr = codec->work_buffer;
	for(i = 0; i < height; i++)
          {
          out_ptr = row_pointers[i];
          for(j = 0; j < width; j++)
            {
            out_ptr[0] = in_ptr[1];
            out_ptr[1] = in_ptr[0];
            out_ptr[2] = in_ptr[2];
            out_ptr[3] = in_ptr[3];

            out_ptr += 4;
            in_ptr += 4;
            }
          }

	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
        uint8_t * in_ptr, *out_ptr;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_v408_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	int bytes = width * height * 4;
	int result = 0;
	int i, j;
	quicktime_atom_t chunk_atom;

        if(!row_pointers)
          {
          //          vtrack->stream_cmodel = BC_UYVA8888;
          vtrack->stream_cmodel = BC_YUVA8888;
          }
                
        if(!codec->work_buffer)
          codec->work_buffer = malloc(width * height * 4);

        out_ptr = codec->work_buffer;
	for(i = 0; i < height; i++)
          {
          in_ptr = row_pointers[i];
          for(j = 0; j < width; j++)
            {
            out_ptr[0] = in_ptr[1];
            out_ptr[1] = in_ptr[0];
            out_ptr[2] = in_ptr[2];
            out_ptr[3] = in_ptr[3];

            out_ptr += 4;
            in_ptr += 4;
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

void quicktime_init_codec_v408(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_v408_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
	codec_base->reads_colormodel = reads_colormodel;
	codec_base->writes_colormodel = writes_colormodel;
	codec_base->fourcc = QUICKTIME_YUVA4444;
	codec_base->title = "Component Y'CbCrA 8-bit 4:4:4:4";
	codec_base->desc = "Component Y'CbCrA 8-bit 4:4:4:4";
}

