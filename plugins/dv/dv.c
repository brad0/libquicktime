#include <quicktime/colormodels.h>
#include <funcprotos.h>
#include <libdv/dv.h>
#include <quicktime/quicktime.h>
#include <pthread.h>
#include <string.h>

// Buffer sizes
#define DV_NTSC_SIZE 120000
#define DV_PAL_SIZE 140000

typedef struct
{
	dv_decoder_t *dv_decoder;
	unsigned char *data;
	unsigned char *temp_frame, **temp_rows;

	/* Parameters */
	int decode_quality;
	int anamorphic16x9;
	int vlc_encode_passes;
} quicktime_dv_codec_t;

static pthread_mutex_t libdv_mutex = PTHREAD_MUTEX_INITIALIZER;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

	pthread_mutex_lock( &libdv_mutex );

	// This is a memory leak. dv_decoder_t contains pointers so just
	// free'ing leaves those pointers in limbo.
	if(codec->dv_decoder) free( codec->dv_decoder );
	
	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->temp_rows) free(codec->temp_rows);
	free(codec->data);
	free(codec);
	
	pthread_mutex_unlock( &libdv_mutex );
	return 0;
}

static int check_sequentiality( unsigned char **row_pointers,
								int bytes_per_row,
								int height )
{
	int i = 0;
//printf( "dv.c check_sequentiality: %p, %d, %d\n", row_pointers, bytes_per_row, height );
 
	for(; i < height-1; i++)
	{
		if( row_pointers[i+1] - row_pointers[i] != bytes_per_row )
		{
//printf( "dv.c check_sequentiality: %p - %p != %p\n", row_pointers[i+1], row_pointers[i], bytes_per_row );
			return 0;
		}
	}
	return 1;
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	long bytes;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	int result = 0;
	int i;
	int decode_colormodel = 0;
	int pitches[3] = { 720 * 2, 0, 0 };

//printf( "dv.c decode: row_pointers=%p {%p, %p, %p, %p, ...}\n", row_pointers, row_pointers[0], row_pointers[1], row_pointers[2], row_pointers[3] );

	quicktime_set_video_position(file, vtrack->current_position, track);
	bytes = quicktime_frame_size(file, vtrack->current_position, track);
	result = !quicktime_read_data(file, (char*)codec->data, bytes);

	pthread_mutex_lock( &libdv_mutex );

	if( ! codec->dv_decoder )
	{
			dv_init();
			codec->dv_decoder = dv_decoder_new();
			codec->dv_decoder->prev_frame_decoded = 0;
	}

	codec->dv_decoder->quality = codec->decode_quality;

	if(codec->dv_decoder)
	{
		int is_sequential =
			check_sequentiality( row_pointers,
								 720 * cmodel_calculate_pixelsize(file->color_model),
								 file->out_h );

		dv_parse_header( codec->dv_decoder, codec->data );
		
		if( (file->color_model == BC_YUV422
			 || file->color_model == BC_RGB888) &&
			file->in_x == 0 && 
			file->in_y == 0 && 
			file->in_w == width &&
			file->in_h == height &&
			file->out_w == width &&
			file->out_h == height &&
			is_sequential )
		{
			if( file->color_model == BC_YUV422 )
			{
				pitches[0] = 720 * 2;
				dv_decode_full_frame( codec->dv_decoder, codec->data,
									  e_dv_color_yuv, row_pointers,
									  pitches );
			}
			else if( file->color_model == BC_RGB888)
 			{
				pitches[0] = 720 * 3;
				dv_decode_full_frame( codec->dv_decoder, codec->data,
									  e_dv_color_rgb, row_pointers,
									  pitches );
			}
		}
		else
		{
			if(!codec->temp_frame)
			{
				codec->temp_frame = malloc(720 * 576 * 2);
				codec->temp_rows = malloc(sizeof(unsigned char*) * 576);
				for(i = 0; i < 576; i++)
					codec->temp_rows[i] = codec->temp_frame + 720 * 2 * i;
			}

			/*if( file->color_model == BC_YUV422 ||
			  file->color_model == BC_YUV888 )
			  {*/

		    decode_colormodel = BC_YUV422;
			pitches[0] = 720 * 2;
			dv_decode_full_frame( codec->dv_decoder, codec->data,
								  e_dv_color_yuv, codec->temp_rows,
								  pitches );
			
			/*}
			  else if( file->color_model == BC_RGB888 ||
			  file->color_model == BC_BGR888 ||
			  file->color_model == BC_RGBA8888 )
			  {
			  decode_colormodel = BC_RGB888;
			  pitches[0] = 720 * 3;
			  dv_decode_full_frame( codec->dv_decoder, codec->data,
			  e_dv_color_rgb, row_pointers,
			  pitches );
			  }*/

//printf( "dv.c decode: doing cmodel_transfer\n" );

			cmodel_transfer(row_pointers, 
				codec->temp_rows,
				row_pointers[0],
				row_pointers[1],
				row_pointers[2],
				codec->temp_rows[0],
				codec->temp_rows[1],
				codec->temp_rows[2],
				file->in_x, 
				file->in_y, 
				file->in_w, 
				file->in_h,
				0, 
				0, 
				file->out_w, 
				file->out_h,
				decode_colormodel, 
				file->color_model,
				0,
				width,
				file->out_w);
//printf("decode 3\n");
		}
	}

	pthread_mutex_unlock( &libdv_mutex );

	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
	longest offset = quicktime_position(file);
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = trak->tkhd.track_width;
	int height = trak->tkhd.track_height;
	int width_i = 720;
	int height_i = (height <= 480) ? 480 : 576;
	int i;
	unsigned char **input_rows;
	int isPAL = (height_i == 480) ? 0 : 1;
	int data_length = isPAL ? DV_PAL_SIZE : DV_NTSC_SIZE;
	int result = 0;
	int encode_colormodel = 0;
	dv_color_space_t encode_dv_colormodel = 0;

	int is_sequential =
		check_sequentiality( row_pointers,
							 width_i * cmodel_calculate_pixelsize(file->color_model),
							 height );

	if( ( file->color_model == BC_YUV422
		  || file->color_model == BC_RGB888 ) &&
		width == width_i &&
		height == height_i &&
		is_sequential )
	{
		input_rows = row_pointers;
		encode_colormodel = file->color_model;
		switch( file->color_model )
		{
			case BC_YUV422:
				encode_dv_colormodel = e_dv_color_yuv;
				break;
			case BC_RGB888:
				encode_dv_colormodel = e_dv_color_rgb;
				break;
			default:
				return 0;
				break;
		}
	}
	else
	{
		if(!codec->temp_frame)
		{
			codec->temp_frame = malloc(720 * 576 * 2);
			codec->temp_rows = malloc(sizeof(unsigned char*) * 576);
			for(i = 0; i < 576; i++)
				codec->temp_rows[i] = codec->temp_frame + 720 * 2 * i;
		}
		
printf( "dv.c encode: doing cmodel_transfer. BUGGY CODE!!!\n" );

        cmodel_transfer(codec->temp_rows, /* Leave NULL if non existent */
			row_pointers,
			codec->temp_rows[0], /* Leave NULL if non existent */
			codec->temp_rows[1],
			codec->temp_rows[2],
			row_pointers[0], /* Leave NULL if non existent */
			row_pointers[1],
			row_pointers[2],
			0,        /* Dimensions to capture from input frame */
			0, 
			MIN(width, width_i), 
			MIN(height, height_i),
			0,       /* Dimensions to project on output frame */
			0, 
			MIN(width, width_i), 
			MIN(height, height_i),
			file->color_model, 
			BC_YUV422,
			0,         /* When transfering BC_RGBA8888 to non-alpha this is the background color in 0xRRGGBB hex */
			width,       /* For planar use the luma rowspan */
			width_i);


		input_rows = codec->temp_rows;
		encode_colormodel = BC_YUV422;
		encode_dv_colormodel = e_dv_color_yuv;
	}

//printf("dv.c encode: 1 %d %d %d\n", width_i, height_i, encode_dv_colormodel);
	dv_encode_full_frame( input_rows, codec->data, encode_dv_colormodel,
						  isPAL, codec->anamorphic16x9,
						  codec->vlc_encode_passes, 0, DV_DCT_AUTO );
//printf("dv.c encode: 2 %d %d\n", width_i, height_i);

	result = !quicktime_write_data(file, codec->data, data_length);
	quicktime_update_tables(file,
						file->vtracks[track].track,
						offset,
						file->vtracks[track].current_chunk,
						file->vtracks[track].current_position,
						1,
						data_length);
	file->vtracks[track].current_chunk++;
//printf("encode 3\n", width_i, height_i);

	return result;
}

// Logic: DV contains a mixture of 420 and 411 so can only
// output/input 444 or 422 and libdv can output/input RGB as well so
// we include that.

// This function is used as both reads_colormodel and writes_colormodel

static int colormodel_dv(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888 ||
			colormodel == BC_YUV888 ||
			colormodel == BC_YUV422);
}

static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;
	
	if(!strcasecmp(key, "dv_decode_quality"))
	{
		codec->decode_quality = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_anamorphic16x9"))
	{
		codec->anamorphic16x9 = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_vlc_encode_passes"))
	{
		codec->vlc_encode_passes = *(int*)value;
	}
	return 0;
}

void quicktime_init_codec_dv(quicktime_video_map_t *vtrack)
{
	quicktime_dv_codec_t *codec;
	int i;

/* Init public items */
	((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_dv_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->decode_video = decode;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->decode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->encode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = colormodel_dv;
	((quicktime_codec_t*)vtrack->codec)->writes_colormodel = colormodel_dv;
	((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;


	/* Init private items */

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	
	codec->dv_decoder = NULL;
	codec->decode_quality = DV_QUALITY_BEST;
	codec->anamorphic16x9 = 0;
	codec->vlc_encode_passes = 3;
	codec->data = calloc(1, 140000);
}
