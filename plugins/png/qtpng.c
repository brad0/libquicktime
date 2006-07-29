#include "funcprotos.h"

#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>

#include <png.h>

typedef struct
{
	int compression_level;
	unsigned char *buffer;
// Read position
	long buffer_position;
// Frame size
	long buffer_size;
// Buffer allocation
	long buffer_alloc;
	unsigned char *temp_frame;
} quicktime_png_codec_t;


static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_png_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
//printf("quicktime_delete_codec_png 1\n");
	if(codec->buffer) free(codec->buffer);
	if(codec->temp_frame) free(codec->temp_frame);
//printf("quicktime_delete_codec_png 1\n");
	free(codec);
//printf("quicktime_delete_codec_png 2\n");
	return 0;
}

static int source_cmodel(quicktime_t *file, int track)
{
	int depth = quicktime_video_depth(file, track);
	if(depth == 24) 
		return BC_RGB888;
	else
		return BC_RGBA8888;
}

void quicktime_set_png(quicktime_t *file, int compression_level)
{
	int i;

	for(i = 0; i < file->total_vtracks; i++)
	{
		if(quicktime_match_32(quicktime_video_compressor(file, i), QUICKTIME_PNG))
		{
			quicktime_png_codec_t *codec = ((quicktime_codec_t*)file->vtracks[i].codec)->priv;
			codec->compression_level = compression_level;
		}
	}
}

static void read_function(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	quicktime_png_codec_t *codec = png_get_io_ptr(png_ptr);
	
	if((long)(length + codec->buffer_position) <= codec->buffer_size)
	{
		memcpy(data, codec->buffer + codec->buffer_position, length);
		codec->buffer_position += length;
	}
}

static void write_function(png_structp png_ptr, png_bytep data, png_uint_32 length)
{
	quicktime_png_codec_t *codec = png_get_io_ptr(png_ptr);

	if((long)(length + codec->buffer_size) > codec->buffer_alloc)
	{
		codec->buffer_alloc += length;
		codec->buffer = realloc(codec->buffer, codec->buffer_alloc);
	}
	memcpy(codec->buffer + codec->buffer_size, data, length);
	codec->buffer_size += length;
}

static void flush_function(png_structp png_ptr)
{
	;
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info = 0;	
	quicktime_png_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

        if(!row_pointers)
          {
          vtrack->stream_cmodel = source_cmodel(file, track);
          fprintf(stderr, "Detected stream_cmodel: %s\n", lqt_colormodel_to_string(vtrack->stream_cmodel));
          return 0;
          }

        codec->buffer_size = lqt_read_video_frame(file, &codec->buffer, &codec->buffer_alloc,
                                                  vtrack->current_position, track);
        
	if(!result)
	{
		png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
		info_ptr = png_create_info_struct(png_ptr);
		png_set_read_fn(png_ptr, codec, (png_rw_ptr)read_function);
		png_read_info(png_ptr, info_ptr);

/* read the image */
		png_read_image(png_ptr, row_pointers);
		png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	}


	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_png_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
	png_structp png_ptr;
	png_infop info_ptr;
        quicktime_atom_t chunk_atom;

        if(!row_pointers)
          {
          vtrack->stream_cmodel = source_cmodel(file, track);
          fprintf(stderr, "Detected stream_cmodel: %s, %d\n",
                  lqt_colormodel_to_string(vtrack->stream_cmodel),
                  quicktime_video_depth(file, track));
          return 0;
          }
        
	codec->buffer_size = 0;
	codec->buffer_position = 0;

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	info_ptr = png_create_info_struct(png_ptr);
	png_set_write_fn(png_ptr,
               codec, 
			   (png_rw_ptr)write_function,
               (png_flush_ptr)flush_function);
	png_set_compression_level(png_ptr, codec->compression_level);
	png_set_IHDR(png_ptr, 
		info_ptr, 
		width, height,
    	8, 
		vtrack->stream_cmodel == BC_RGB888 ? 
		  PNG_COLOR_TYPE_RGB : 
		  PNG_COLOR_TYPE_RGB_ALPHA, 
		PNG_INTERLACE_NONE, 
		PNG_COMPRESSION_TYPE_DEFAULT, 
		PNG_FILTER_TYPE_DEFAULT);

#if 0
        png_write_info(png_ptr, info_ptr);
	png_write_image(png_ptr, row_pointers);
	png_write_end(png_ptr, info_ptr);
#else
        png_set_rows(png_ptr, info_ptr, row_pointers);
        png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
#endif
	png_destroy_write_struct(&png_ptr, &info_ptr);

        quicktime_write_chunk_header(file, trak, &chunk_atom);
	result = !quicktime_write_data(file, 
				codec->buffer, 
				codec->buffer_size);
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
  quicktime_png_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;
  
  if(!strcasecmp(key, "png_compression_level"))
    codec->compression_level = *(int*)value;
  return 0;
  }

void quicktime_init_codec_png(quicktime_video_map_t *vtrack)
{
	quicktime_png_codec_t *codec;

/* Init public items */
	((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_png_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = decode;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;
	((quicktime_codec_t*)vtrack->codec)->decode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->encode_audio = 0;


/* Init private items */
	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	codec->compression_level = 9;
}

void quicktime_init_codec_pngalpha(quicktime_video_map_t *vtrack)
  {
  /* Set depth to 32 */
  vtrack->track->mdia.minf.stbl.stsd.table[0].depth = 32;
  /* Proceed as normal */
  quicktime_init_codec_png(vtrack);
  }
