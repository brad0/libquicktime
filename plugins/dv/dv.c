#include <quicktime/colormodels.h>
#include <funcprotos.h>
#include <libdv/dv.h>
#include <quicktime/quicktime.h>
#include <pthread.h>
#include <string.h>

// Buffer sizes
#define DV_NTSC_SIZE 120000
#define DV_PAL_SIZE 144000

typedef struct
{
	dv_decoder_t *dv_decoder;
	dv_encoder_t *dv_encoder;
	unsigned char *data;
	unsigned char *temp_frame, **temp_rows;

	/* Parameters */
	int decode_quality;
	int anamorphic16x9;
	int vlc_encode_passes;
	int clamp_luma, clamp_chroma;

	int add_ntsc_setup;

	int rem_ntsc_setup;

	int parameters_changed;
} quicktime_dv_codec_t;

static pthread_mutex_t libdv_init_mutex = PTHREAD_MUTEX_INITIALIZER;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_dv_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;

	if(codec->dv_decoder)
	{
		dv_decoder_free( codec->dv_decoder );
		codec->dv_decoder = NULL;
	}
	
	if(codec->dv_encoder)
	{
		dv_encoder_free( codec->dv_encoder );
		codec->dv_encoder = NULL;
	}
	
	if(codec->temp_frame) free(codec->temp_frame);
	if(codec->temp_rows) free(codec->temp_rows);
	free(codec->data);
	free(codec);
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

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
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
	dv_color_space_t encode_dv_colormodel = 0;
        quicktime_atom_t chunk_atom;

        if(!row_pointers)
          {
          vtrack->stream_cmodel = BC_YUV422;
          return 0;
          }
        
        //  fprintf(stderr, "Encoding %s\n", (isPAL ? "PAL" : "NTSC"));
        
	if( codec->dv_encoder != NULL && codec->parameters_changed )
	{
		dv_encoder_free( codec->dv_encoder );
		codec->dv_encoder = NULL;
		codec->parameters_changed = 0;
	}
	
	if( ! codec->dv_encoder )
	{
		pthread_mutex_lock( &libdv_init_mutex );

		//printf( "dv.c encode: Alloc'ing encoder\n" );
	
		codec->dv_encoder = dv_encoder_new( codec->rem_ntsc_setup,
											codec->clamp_luma,
											codec->clamp_chroma );

		codec->parameters_changed = 0;
		pthread_mutex_unlock( &libdv_init_mutex );
	}

	if(codec->dv_encoder)
	{
		int is_sequential =
			check_sequentiality( row_pointers,
                                             width_i * cmodel_calculate_pixelsize(BC_YUV422),
                                             height );
	
                encode_dv_colormodel = e_dv_color_yuv;
		if( width == width_i &&
                    height == height_i &&
                    is_sequential )
		{
			input_rows = row_pointers;
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

                        for(i = 0; i < MIN(height, height_i); i++)
                          {
                          memcpy(codec->temp_rows[i], row_pointers[i], MIN(width, width_i));
                          }


			input_rows = codec->temp_rows;
		}

		// Setup the encoder
		codec->dv_encoder->is16x9 = codec->anamorphic16x9;
		codec->dv_encoder->vlc_encode_passes = codec->vlc_encode_passes;
		codec->dv_encoder->static_qno = 0;
		codec->dv_encoder->force_dct = DV_DCT_AUTO;
		codec->dv_encoder->isPAL = isPAL;


//printf("dv.c encode: 1 %d %d %d\n", width_i, height_i, encode_dv_colormodel);
		dv_encode_full_frame( codec->dv_encoder,
							  input_rows, encode_dv_colormodel, codec->data );
//printf("dv.c encode: 2 %d %d\n", width_i, height_i);

                
                quicktime_write_chunk_header(file, trak, &chunk_atom);
                result = !quicktime_write_data(file, codec->data, data_length);
                quicktime_write_chunk_footer(file, trak, vtrack->current_chunk, &chunk_atom, 1);
                file->vtracks[track].current_chunk++;
//printf("encode 3\n");
	}

	return result;
}

// Logic: DV contains a mixture of 420 and 411 so can only
// output/input 444 or 422 and libdv can output/input RGB as well so
// we include that.

// This function is used as both reads_colormodel and writes_colormodel

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
	else if(!strcasecmp(key, "dv_clamp_luma"))
	{
		codec->clamp_luma = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_clamp_chroma"))
	{
		codec->clamp_chroma = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_add_ntsc_setup"))
	{
		codec->add_ntsc_setup = *(int*)value;
	}
	else if(!strcasecmp(key, "dv_rem_ntsc_setup"))
	{
		codec->rem_ntsc_setup = *(int*)value;
	}
	else
	{
		return 0;
	}

	codec->parameters_changed = 1;
	return 0;
}

void quicktime_init_codec_dv(quicktime_video_map_t *vtrack)
{
	quicktime_dv_codec_t *codec;

/* Init public items */
	((quicktime_codec_t*)vtrack->codec)->priv = calloc(1, sizeof(quicktime_dv_codec_t));
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	((quicktime_codec_t*)vtrack->codec)->encode_video = encode;
	((quicktime_codec_t*)vtrack->codec)->decode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->encode_audio = 0;
	((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;


	/* Init private items */

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	
	codec->dv_decoder = NULL;
	codec->dv_encoder = NULL;
	codec->decode_quality = DV_QUALITY_BEST;
	codec->anamorphic16x9 = 0;
	codec->vlc_encode_passes = 3;
	codec->clamp_luma = codec->clamp_chroma = 0;
	codec->add_ntsc_setup = 0;
	codec->parameters_changed = 0;
	
	codec->data = calloc(1, 144000);
}
