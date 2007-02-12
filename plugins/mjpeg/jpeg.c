#include "lqt_private.h"
#include "libmjpeg.h"
#include <quicktime/colormodels.h>
#include <stdlib.h>
#include <string.h>

#define LOG_DOMAIN "mjpeg"

// Jpeg types
#define JPEG_PROGRESSIVE 0
#define JPEG_MJPA 1

/* MJPB isn't supported anyway */
// #define JPEG_MJPB 2 

typedef struct
  {
  unsigned char *buffer;
  int buffer_alloc;
  mjpeg_t *mjpeg;
  int jpeg_type;
  unsigned char *temp_video;
  
  int have_frame;
  int initialized;
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

static int decode(quicktime_t *file, 
                  unsigned char **row_pointers, 
                  int track)
  {
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  mjpeg_t *mjpeg = codec->mjpeg;
  long size, field2_offset;
  int result = 0;

  if(!codec->initialized)
    {
    /* Special treatment of bottom_field_first.
       We cannot use lqt_get_interlace_mode at this point,
       since this piece is called *before* the interlace mode
       is set by quicktime_init_maps() */
    int nfields, dominance;
    if(lqt_get_fiel(file, track, &nfields, &dominance) &&
       (nfields == 2) && (dominance == 6))
      codec->mjpeg->bottom_first = 1;
    codec->initialized = 1;
    }
  
  if(!codec->have_frame)
    {
    size = lqt_read_video_frame(file, &codec->buffer, &codec->buffer_alloc,
                                vtrack->current_position, NULL, track);
    
    if(size <= 0)
      return -1;
    
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
      codec->have_frame = 1;
      return 0;
      }
    }

  if(file->vtracks[track].stream_row_span) 
    mjpeg_set_rowspan(codec->mjpeg, file->vtracks[track].stream_row_span,
                      file->vtracks[track].stream_row_span_uv);
  else
    mjpeg_set_rowspan(codec->mjpeg, 0, 0);
    
  mjpeg_get_frame(codec->mjpeg, row_pointers);
  codec->have_frame = 0;
  
  return result;
  }

static void resync(quicktime_t *file, int track)
  {
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_jpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
  codec->have_frame = 0;
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
            vtrack->stream_cmodel = BC_YUVJ420P;
          else
            vtrack->stream_cmodel = BC_YUVJ422P;
          return 0;
          }
        
        if(!codec->initialized)
          {
          // Bottom first needs special treatment */
          if(vtrack->interlace_mode == LQT_INTERLACE_BOTTOM_FIRST)
            codec->mjpeg->bottom_first = 1;
          
          /* Quicktime for Windows must have this information. */
          if((codec->jpeg_type == JPEG_MJPA) &&
             !trak->mdia.minf.stbl.stsd.table[0].has_fiel)
            {
            switch(vtrack->interlace_mode)
              {
              case LQT_INTERLACE_TOP_FIRST:
                lqt_set_fiel(file, track, 2, 1);
                break;
              case LQT_INTERLACE_BOTTOM_FIRST:
                lqt_set_fiel(file, track, 2, 6);
                break;
              case LQT_INTERLACE_NONE:
                lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
                        "Encoding progressive video as interlaced. Motion JPEG-A is not suitable for progressive video.");
                lqt_set_fiel(file, track, 2, 1);
                break;
                
              }
            
            }
          codec->initialized = 1;
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
	((quicktime_codec_t*)vtrack->codec)->priv = lqt_bufalloc(sizeof(quicktime_jpeg_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = decode;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->decode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->encode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;
	((quicktime_codec_t*)vtrack->codec)->resync = resync;

/* Init private items */
	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	codec->mjpeg = mjpeg_new(vtrack->track->tkhd.track_width, 
                                 vtrack->track->tkhd.track_height, 
                                 num_fields);
	codec->jpeg_type = jpeg_type;

}
