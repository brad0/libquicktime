#include "funcprotos.h"
#include <quicktime/quicktime.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

#include "ulaw.h"

/* We decode this many samples at a time */

#define SAMPLES_PER_BLOCK 1024

typedef struct
{
	float *ulawtofloat_table;
	float *ulawtofloat_ptr;
	int16_t *ulawtoint16_table;
	int16_t *ulawtoint16_ptr;
	unsigned char *int16toulaw_table;
	unsigned char *int16toulaw_ptr;
        unsigned char *read_buffer;
	long read_size;
        int encode_initialized;

/* Decode specific stuff */
        int decode_buffer_size;
        int decode_buffer_alloc;
        uint8_t * decode_buffer;
        uint8_t * decode_buffer_ptr;

        int16_t ** decode_sample_buffer_i;
        int decode_sample_buffer_size;
        int decode_sample_buffer_pos;
        int decode_initialized;
        int decode_block_align;

  } quicktime_ulaw_codec_t;

/* ==================================== private for ulaw */

#define uBIAS 0x84
#define uCLIP 32635

static int ulaw_init_ulawtofloat(quicktime_t *file, int track);

static int ulaw_init_ulawtoint16(quicktime_t *file, int track)
{
	int i;
	quicktime_audio_map_t *atrack = &(file->atracks[track]);
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

/* We use the floating point table to get values for the 16 bit table */
	ulaw_init_ulawtofloat(file, track);
	if(!codec->ulawtoint16_table)
	{
		codec->ulawtoint16_table = malloc(sizeof(int16_t) * 256);
		codec->ulawtoint16_ptr = codec->ulawtoint16_table;

		for(i = 0; i < 256; i++)
		{
			codec->ulawtoint16_table[i] = (int)(32768 * codec->ulawtofloat_ptr[i]);
		}
	}
	return 0;
}

static int16_t ulaw_bytetoint16(quicktime_ulaw_codec_t *codec, unsigned char input)
{
	return codec->ulawtoint16_ptr[input];
}

static int ulaw_init_ulawtofloat(quicktime_t *file, int track)
{
	int i;
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;

	if(!codec->ulawtofloat_table)
	{
    	static int exp_lut[8] = { 0, 132, 396, 924, 1980, 4092, 8316, 16764 };
    	int sign, exponent, mantissa, sample;
		unsigned char ulawbyte;

		codec->ulawtofloat_table = malloc(sizeof(float) * 256);
		codec->ulawtofloat_ptr = codec->ulawtofloat_table;
		for(i = 0; i < 256; i++)
		{
			ulawbyte = (unsigned char)i;
    		ulawbyte = ~ulawbyte;
    		sign = (ulawbyte & 0x80);
    		exponent = (ulawbyte >> 4) & 0x07;
    		mantissa = ulawbyte & 0x0F;
    		sample = exp_lut[exponent] + (mantissa << (exponent + 3));
    		if(sign != 0) sample = -sample;

			codec->ulawtofloat_ptr[i] = (float)sample / 32768;
		}
	}
	return 0;
}

#if 0
static float ulaw_bytetofloat(quicktime_ulaw_codec_t *codec, unsigned char input)
{
	return codec->ulawtofloat_ptr[input];
}
#endif

static int ulaw_init_int16toulaw(quicktime_t *file, int track)
{
	quicktime_audio_map_t *atrack = &(file->atracks[track]);
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(!codec->int16toulaw_table)
	{
    	int sign, exponent, mantissa;
    	unsigned char ulawbyte;
		int sample;
		int i;
    	int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                               4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};

 		codec->int16toulaw_table = malloc(65536);
		codec->int16toulaw_ptr = codec->int16toulaw_table + 32768;

		for(i = -32768; i < 32768; i++)
		{
			sample = i;
/* Get the sample into sign-magnitude. */
    		sign = (sample >> 8) & 0x80;		/* set aside the sign */
    		if(sign != 0) sample = -sample;		/* get magnitude */
    		if(sample > uCLIP) sample = uCLIP;		/* clip the magnitude */

/* Convert from 16 bit linear to ulaw. */
    		sample = sample + uBIAS;
		    exponent = exp_lut[(sample >> 7) & 0xFF];
		    mantissa = (sample >> (exponent + 3)) & 0x0F;
		    ulawbyte = ~(sign | (exponent << 4) | mantissa);
#ifdef ZEROTRAP
		    if (ulawbyte == 0) ulawbyte = 0x02;	    /* optional CCITT trap */
#endif

		    codec->int16toulaw_ptr[i] = ulawbyte;
		}
	}
	return 0;
}

static float ulaw_int16tobyte(quicktime_ulaw_codec_t *codec, int16_t input)
{
	return codec->int16toulaw_ptr[input];
}

static float ulaw_floattobyte(quicktime_ulaw_codec_t *codec, float input)
{
	return codec->int16toulaw_ptr[(int)(input * 32768)];
}

#if 1
static int ulaw_get_read_buffer(quicktime_t *file, int track, long samples)
{
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;

	if(codec->read_buffer && codec->read_size != samples)
	{
		free(codec->read_buffer);
		codec->read_buffer = 0;
	}
	
	if(!codec->read_buffer) 
	{
		int64_t bytes = samples * file->atracks[track].channels;
		codec->read_size = samples;
		if(!(codec->read_buffer = malloc(bytes))) return 1;
	}
	return 0;
}
#endif 
static int ulaw_delete_tables(quicktime_audio_map_t *atrack)
{
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(codec->ulawtofloat_table) 
	{ 
		free(codec->ulawtofloat_table); 
	}
	if(codec->ulawtoint16_table) 
	{ 
		free(codec->ulawtoint16_table); 
	}
	if(codec->int16toulaw_table) 
	{ 
		free(codec->int16toulaw_table); 
	}
	if(codec->read_buffer) free(codec->read_buffer);
	codec->int16toulaw_table = 0;
	codec->ulawtoint16_table = 0;
	codec->ulawtofloat_table = 0;
	codec->read_buffer = 0;
	codec->read_size = 0;
	return 0;
}

/* =================================== public for ulaw */

static int quicktime_delete_codec_ulaw(quicktime_audio_map_t *atrack)
{
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;
        
        if(codec->decode_sample_buffer_i)
          {
          free(codec->decode_sample_buffer_i[0]);
          free(codec->decode_sample_buffer_i);
          }
        if(codec->decode_buffer)
          {
          free(codec->decode_buffer);
          }
	ulaw_delete_tables(atrack);
	free(codec);
	return 0;
}

static int quicktime_decode_ulaw(quicktime_t *file, 
					int16_t **output_i, 
					float **output_f, 
					long samples, 
					int track) 
{

	int j;
        quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

	int64_t chunk, chunk_sample;
	int64_t i;

        int samples_decoded, samples_copied;
        
        int samples_to_skip = 0;
        
        if(!codec->decode_initialized)
          {
          codec->decode_initialized = 1;

          codec->decode_sample_buffer_i    = malloc(sizeof(*(codec->decode_sample_buffer_i)) * file->atracks[track].channels);
          codec->decode_sample_buffer_i[0] = malloc(sizeof(*(codec->decode_sample_buffer_i[0])) * file->atracks[track].channels *
                                                    SAMPLES_PER_BLOCK);
          for(i = 1; i < file->atracks[track].channels; i++)
            codec->decode_sample_buffer_i[i] = codec->decode_sample_buffer_i[0] + i * SAMPLES_PER_BLOCK;
          ulaw_init_ulawtoint16(file, track);
          
          codec->decode_block_align = track_map->channels;
          }
        
        if(file->atracks[track].current_position != file->atracks[track].last_position)
          {
          /* Seeking happened */
          quicktime_chunk_of_sample(&chunk_sample, &chunk,
                                    file->atracks[track].track,
                                    file->atracks[track].current_position);

          
          
          /* Read the chunk */

          if(file->atracks[track].current_chunk != chunk)
            {
            file->atracks[track].current_chunk = chunk;
            codec->decode_buffer_size = lqt_read_audio_chunk(file,
                                                             track, file->atracks[track].current_chunk,
                                                             &(codec->decode_buffer),
                                                             &(codec->decode_buffer_alloc));
            if(codec->decode_buffer_size <= 0)
              return 0;
            
            codec->decode_buffer_ptr = codec->decode_buffer;
            }
          else
            {
            codec->decode_buffer_size += (int)(codec->decode_buffer_ptr - codec->decode_buffer);
            codec->decode_buffer_ptr = codec->decode_buffer;
            }
          /* Skip frames */
          
          samples_to_skip = file->atracks[track].current_position - chunk_sample;
          if(samples_to_skip < 0)
            {
            fprintf(stderr, "ulaw: Cannot skip %d samples backwards %lld %ld\n", samples_to_skip,
                    file->atracks[track].current_position, file->atracks[track].last_position);
            samples_to_skip = 0;
            }
          codec->decode_buffer_ptr = codec->decode_buffer + samples_to_skip * codec->decode_block_align;
          codec->decode_buffer_size -= samples_to_skip * codec->decode_block_align;
          codec->decode_sample_buffer_size = 0;
          codec->decode_sample_buffer_pos = 0;
          }

        /* Decode until we are done */

        samples_decoded = 0;

        while(samples_decoded < samples)
          {
          /* Decode new frame if necessary */
          if(!codec->decode_sample_buffer_size)
            {
            /* Get new chunk if necessary */
            
            if(!codec->decode_buffer_size)
              {
              file->atracks[track].current_chunk++;
              codec->decode_buffer_size = lqt_read_audio_chunk(file,
                                                               track, file->atracks[track].current_chunk,
                                                               &(codec->decode_buffer),
                                                               &(codec->decode_buffer_alloc));
              
              if(codec->decode_buffer_size <= 0)
                return 0;
              codec->decode_buffer_ptr = codec->decode_buffer;
              }

            if(!codec->decode_buffer_size)
              break;
            
            codec->decode_sample_buffer_size = codec->decode_buffer_size / codec->decode_block_align;
            if(codec->decode_sample_buffer_size > SAMPLES_PER_BLOCK)
              {
              codec->decode_sample_buffer_size = SAMPLES_PER_BLOCK;
              }

            codec->decode_sample_buffer_pos = 0;
            
            /* Decode the stuff */
            for(i = 0; i < codec->decode_sample_buffer_size; i++)
              {
              for(j = 0; j < track_map->channels; j++)
                {
                codec->decode_sample_buffer_i[j][i] = ulaw_bytetoint16(codec, *codec->decode_buffer_ptr);
                codec->decode_buffer_ptr++;
                codec->decode_buffer_size--;
                }
              }
            }
          
          /* Copy samples */
          
          samples_copied = lqt_copy_audio(output_i,                                     // int16_t ** dst_i
                                          output_f,                                     // float ** dst_f
                                          codec->decode_sample_buffer_i,                  // int16_t ** src_i
                                          NULL,                  // float ** src_f
                                          samples_decoded,                              // int dst_pos
                                          codec->decode_sample_buffer_pos,              // int src_pos
                                          samples - samples_decoded,                    // int dst_size
                                          codec->decode_sample_buffer_size,                    // int src_size
                                          file->atracks[track].channels);               // int num_channels

          //          fprintf(stderr, "Decode %d %d %d\n", codec->decode_sample_buffer_size, samples_copied, samples);
          
          samples_decoded += samples_copied;
          codec->decode_sample_buffer_size -= samples_copied;
          codec->decode_sample_buffer_pos += samples_copied;
          }
        
        file->atracks[track].last_position = file->atracks[track].current_position + samples_decoded;
        return samples_decoded;


}

static int quicktime_encode_ulaw(quicktime_t *file, 
							int16_t **input_i, 
							float **input_f, 
							int track, 
							long samples)
{
	int result = 0;
	int channel, step;
	quicktime_ulaw_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;
	quicktime_atom_t chunk_atom;
	quicktime_audio_map_t *track_map = &file->atracks[track];
	quicktime_trak_t *trak = track_map->track;

        if(!codec->encode_initialized)
          {
          if(trak->strl)
            {
            /* strh stuff */
            trak->strl->dwRate = trak->mdia.minf.stbl.stsd.table[0].sample_rate * track_map->channels;
            trak->strl->dwScale = track_map->channels;
            trak->strl->dwSampleSize = 1;
            
            /* WAVEFORMATEX stuff */
            
            trak->strl->nBlockAlign = trak->strl->dwScale;
            trak->strl->nAvgBytesPerSec =  trak->strl->dwRate;
            trak->strl->wBitsPerSample = 8;
            }
          codec->encode_initialized = 1;
          }
        
	result = ulaw_init_int16toulaw(file, track);
	result += ulaw_get_read_buffer(file, track, samples);

	if(!result)
	{
		step = file->atracks[track].channels;

		if(input_f)
		{
			for(channel = 0; channel < file->atracks[track].channels; channel++)
			{
				float *input_ptr = input_f[channel];
				float *input_end = input_f[channel] + samples;
				unsigned char *output = codec->read_buffer + channel;

				while(input_ptr < input_end)
				{
					*output = ulaw_floattobyte(codec, *input_ptr++);
					output += step;
				}
			}
		}
		else
		if(input_i)
		{
			for(channel = 0; channel < file->atracks[track].channels; channel++)
			{
				int16_t *input_ptr = input_i[channel];
				int16_t *input_end = input_i[channel] + samples;
				unsigned char *output = codec->read_buffer + channel;

				while(input_ptr < input_end)
				{
					*output = ulaw_int16tobyte(codec, *input_ptr++);
					output += step;
				}
			}
		}

		quicktime_write_chunk_header(file, trak, &chunk_atom);
		result = quicktime_write_data(file, 
			codec->read_buffer, 
			samples * file->atracks[track].channels);
		quicktime_write_chunk_footer(file, 
						trak,
						track_map->current_chunk,
						&chunk_atom, 
						samples);

/* defeat fwrite's return */
		if(result) 
			result = 0; 
		else 
			result = 1; 

		file->atracks[track].current_chunk++;		
	}

	return result;
}


void quicktime_init_codec_ulaw(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_ulaw_codec_t *codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_ulaw_codec_t));
	codec_base->delete_acodec = quicktime_delete_codec_ulaw;
	codec_base->decode_video = 0;
	codec_base->encode_video = 0;
	codec_base->decode_audio = quicktime_decode_ulaw;
	codec_base->encode_audio = quicktime_encode_ulaw;
	codec_base->fourcc = QUICKTIME_ULAW;
	codec_base->title = "uLaw";
	codec_base->desc = "uLaw";
	codec_base->wav_id = 0x07;

/* Init private items */
	codec = ((quicktime_codec_t*)atrack->codec)->priv;
}
