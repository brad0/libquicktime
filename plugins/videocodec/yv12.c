#include "lqt_private.h"
#include "workarounds.h"
#include <quicktime/colormodels.h>
#include <stdlib.h>
#include <string.h>

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
        if(codec->buffer)
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
    codec->coded_w = (((int)vtrack->track->tkhd.track_width+1) / 2)*2;
    codec->coded_h = (((int)vtrack->track->tkhd.track_height+1) / 2)*2;
    codec->initialized = 1;
    }
  }

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
  {
  uint8_t * src_ptr;
  uint8_t * dst_ptr;

  int bytes, i;
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_yv12_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  int y_size, uv_size;
  int result = 0;

  if(!row_pointers)
    {
    file->vtracks[track].stream_cmodel = BC_YUV420P;
    return 0;
    }
        
  initialize(vtrack);

  y_size  = codec->coded_w;
  uv_size = codec->coded_w / 2;
  
  bytes = lqt_read_video_frame(file, &codec->buffer, &codec->buffer_alloc,
                               vtrack->current_position, NULL, track);
  
  if(bytes <= 0)
    return -1;

  
  src_ptr = codec->buffer;

  /* Y */
  dst_ptr = row_pointers[0];
  for(i = 0; i < codec->coded_h; i++)
    {
    memcpy(dst_ptr, src_ptr, y_size);
    src_ptr += y_size;
    dst_ptr += file->vtracks[track].stream_row_span;
    }

  /* U */
  dst_ptr = row_pointers[1];
  for(i = 0; i < codec->coded_h/2; i++)
    {
    memcpy(dst_ptr, src_ptr, uv_size);
    src_ptr += uv_size;
    dst_ptr += file->vtracks[track].stream_row_span_uv;
    }

  /* V */
  dst_ptr = row_pointers[2];
  for(i = 0; i < codec->coded_h/2; i++)
    {
    memcpy(dst_ptr, src_ptr, uv_size);
    src_ptr += uv_size;
    dst_ptr += file->vtracks[track].stream_row_span_uv;
    }
  
  return result;
  }

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_yv12_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int result = 0;
	int y_size, uv_size;
	quicktime_atom_t chunk_atom;
        int i;
        uint8_t * src_ptr;
        
        if(!row_pointers)
          {
          file->vtracks[track].stream_cmodel = BC_YUV420P;
          return 0;
          }
        
        initialize(vtrack);

	y_size = codec->coded_w;
	uv_size = codec->coded_w / 2;
        
	quicktime_write_chunk_header(file, trak, &chunk_atom);

        /* Y */
        src_ptr = row_pointers[0];
        for(i = 0; i < codec->coded_h; i++)
          {
          result = !quicktime_write_data(file, src_ptr, y_size);
          src_ptr += file->vtracks[track].stream_row_span;
          if(result)
            return result;
          }

        /* U */
        src_ptr = row_pointers[1];
        for(i = 0; i < codec->coded_h/2; i++)
          {
          result = !quicktime_write_data(file, src_ptr, uv_size);
          src_ptr += file->vtracks[track].stream_row_span_uv;
          if(result)
            return result;
          }

        /* V */
        src_ptr = row_pointers[2];
        for(i = 0; i < codec->coded_h/2; i++)
          {
          result = !quicktime_write_data(file, src_ptr, uv_size);
          src_ptr += file->vtracks[track].stream_row_span_uv;
          if(result)
            return result;
          }
        
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

