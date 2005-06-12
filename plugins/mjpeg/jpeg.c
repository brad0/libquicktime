#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <quicktime/colormodels.h>
#include "funcprotos.h"
#include <string.h>
#include <quicktime.h>

#include "libmjpeg.h"

// Jpeg types
#define JPEG_PROGRESSIVE 0
#define JPEG_MJPA 1

/* MJPB isn't supported anyway */
// #define JPEG_MJPB 2 

typedef struct
  {
  unsigned char *buffer;
  long buffer_allocated;
  long buffer_size;
  mjpeg_t *mjpeg;
  int jpeg_type;
  unsigned char *temp_video;
  
  int have_frame;
  } quicktime_jpeg_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

	mjpeg_delete(codec->mjpeg);
	if(codec->buffer)
		free(codec->buffer);
	if(codec->temp_video)
		free(codec->temp_video);
	free(codec);
	return 0;
}

/*
  void quicktime_set_jpeg(quicktime_t *file, int quality, int use_float)
  {
  int i;
  char *compressor;

  for(i = 0; i < file->total_vtracks; i++)
  {
  if(quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_JPEG) ||
  quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_MJPA) ||
  quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_RTJ0))
  {
  quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)file->vtracks[i].codec)->priv;
  mjpeg_set_quality(codec->mjpeg, quality);
  mjpeg_set_float(codec->mjpeg, use_float);
  }
  }
  }
*/

static int decode(quicktime_t *file, 
                  unsigned char **row_pointers, 
                  int track)
  {
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  mjpeg_t *mjpeg = codec->mjpeg;
  long size, field2_offset;
  int result = 0;
  
  if(!codec->have_frame)
    {
    quicktime_set_video_position(file, vtrack->current_position, track);
    size = quicktime_frame_size(file, vtrack->current_position, track);
    codec->buffer_size = size;
    //    fprintf(stderr, "decode %p\n", row_pointers);
    
    if(size > codec->buffer_allocated)
      {
      codec->buffer_allocated = size;
      codec->buffer = realloc(codec->buffer, codec->buffer_allocated);
      }
    
    result = !quicktime_read_data(file, codec->buffer, size);
    if(result)
      return result;
    
    if(mjpeg_get_fields(mjpeg) == 2)
      {
      field2_offset = mjpeg_get_quicktime_field2(codec->buffer, size);
      }
    else
      field2_offset = 0;
    
    mjpeg_decompress(codec->mjpeg, 
                     codec->buffer, 
                     size,
                     field2_offset);
    
    if(!row_pointers)
      {
      /* Detect colormodel and return */
      vtrack->stream_cmodel = mjpeg->jpeg_color_model;
      fprintf(stderr, "Got colormodel: %s\n", lqt_colormodel_to_string(mjpeg->jpeg_color_model));
      codec->have_frame = 1;
      return 0;
      }
    }

  if(file->vtracks[track].stream_row_span) 
    mjpeg_set_rowspan(codec->mjpeg, file->vtracks[track].stream_row_span, file->vtracks[track].stream_row_span_uv);
  else
    mjpeg_set_rowspan(codec->mjpeg, 0, 0);
    
  mjpeg_get_frame(codec->mjpeg, row_pointers);
  codec->have_frame = 0;
  
  return result;
  }

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int result = 0;
	long field2_offset;
        quicktime_atom_t chunk_atom;

        if(!row_pointers)
          {
          if(codec->jpeg_type == JPEG_PROGRESSIVE)
            vtrack->stream_cmodel = BC_YUV420P;
          else
            vtrack->stream_cmodel = BC_YUV422P;
          return 0;
          }

        if(file->vtracks[track].stream_row_span) 
          mjpeg_set_rowspan(codec->mjpeg, file->vtracks[track].stream_row_span, file->vtracks[track].stream_row_span_uv);
        else
          mjpeg_set_rowspan(codec->mjpeg, 0, 0);
                
	mjpeg_compress(codec->mjpeg, row_pointers);

        if(codec->jpeg_type == JPEG_MJPA) 
		mjpeg_insert_quicktime_markers(&codec->mjpeg->output_data,
			&codec->mjpeg->output_size,
			&codec->mjpeg->output_allocated,
			2,
			&field2_offset);

        quicktime_write_chunk_header(file, trak, &chunk_atom);
	result = !quicktime_write_data(file, 
				mjpeg_output_buffer(codec->mjpeg), 
				mjpeg_output_size(codec->mjpeg));
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
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

// Some JPEG_PROGRESSIVE is BC_YUV422P
	if(codec->jpeg_type == JPEG_PROGRESSIVE)
	{
		return (colormodel == BC_RGB888 ||
			colormodel == BC_YUV888 ||
			colormodel == BC_YUV420P ||
			colormodel == BC_YUV422P ||
			colormodel == BC_YUV422);
	}
	else
	{
		return (colormodel == BC_RGB888 ||
			colormodel == BC_YUV888 ||
//			colormodel == BC_YUV420P ||
			colormodel == BC_YUV422P ||
			colormodel == BC_YUV422);
// The BC_YUV420P option was provided only for mpeg2movie use.because some 
// interlaced movies were accidentally in YUV4:2:0
	}
}

static int writes_colormodel(quicktime_t *file, 
		int colormodel, 
		int track)
{
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

	if(codec->jpeg_type == JPEG_PROGRESSIVE)
	{
		return (colormodel == BC_RGB888 ||
			colormodel == BC_YUV888 ||
			colormodel == BC_YUV420P);
	}
	else
	{
		return (colormodel == BC_RGB888 ||
			colormodel == BC_YUV888 ||
			colormodel == BC_YUV422P);
	}
}

static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;
	
	if(!strcasecmp(key, "jpeg_quality"))
	{
		mjpeg_set_quality(codec->mjpeg, *(int*)value);
	}
	else
	if(!strcasecmp(key, "jpeg_usefloat"))
	{
		mjpeg_set_float(codec->mjpeg, *(int*)value);
	}
	return 0;
}

void quicktime_init_codec_jpeg(quicktime_video_map_t *vtrack)
{
	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
	quicktime_jpeg_codec_t *codec;
	int jpeg_type=0, num_fields = 0;

	if(quicktime_match_32(compressor, QUICKTIME_JPEG))
          {
          jpeg_type = JPEG_PROGRESSIVE;
          num_fields = 1;
          }
	if(quicktime_match_32(compressor, QUICKTIME_MJPA))
          {
          num_fields = 2;
          jpeg_type = JPEG_MJPA;
          }
/* Init public items */
	((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_jpeg_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = decode;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->decode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->encode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = reads_colormodel;
	((quicktime_codec_t*)vtrack->codec)->writes_colormodel = writes_colormodel;
	((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;

/* Init private items */
	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	codec->mjpeg = mjpeg_new(vtrack->track->tkhd.track_width, 
                                 vtrack->track->tkhd.track_height, 
                                 num_fields);
	codec->jpeg_type = jpeg_type;
        
        /* This information must be stored in the initialization routine because of */
        /* direct copy rendering.  Quicktime for Windows must have this information. */
	if(quicktime_match_32(compressor, QUICKTIME_MJPA) && !vtrack->track->mdia.minf.stbl.stsd.table[0].fields)
          {
          vtrack->track->mdia.minf.stbl.stsd.table[0].fields = 2;
          vtrack->track->mdia.minf.stbl.stsd.table[0].field_dominance = 1;
          }
}
