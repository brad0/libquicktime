#include "funcprotos.h"
#include "ima4.h"
#include <quicktime/quicktime.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>


typedef struct
{
/* During decoding the work_buffer contains the most recently read chunk. */
/* During encoding the work_buffer contains interlaced overflow samples  */
/* from the last chunk written. */
	int16_t *work_buffer;
	unsigned char *read_buffer;    /* Temporary buffer for drive reads. */

/* Starting information for all channels during encoding. */
	int *last_samples, *last_indexes;
	long chunk; /* Number of chunk in work buffer */
	int buffer_channel; /* Channel of work buffer */

/* Number of samples in largest chunk read. */
/* Number of samples plus overflow in largest chunk write, interlaced. */
	long work_size;     
	long work_overflow; /* Number of overflow samples from the last chunk written. */
	long read_size;     /* Size of read buffer. */

/* Decode specific stuff */
        int decode_buffer_size;
        int decode_buffer_alloc;
        uint8_t * decode_buffer;
        uint8_t * decode_buffer_ptr;

        int16_t ** decode_sample_buffer;
        int decode_sample_buffer_size;
        int decode_initialized;
  } quicktime_ima4_codec_t;

static int ima4_step[89] = 
{
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static int ima4_index[16] = 
{
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

/* Known by divine revelation */

#define BLOCK_SIZE 0x22
#define SAMPLES_PER_BLOCK 0x40

/* ================================== private for ima4 */


static void ima4_decode_sample(int *predictor, int *nibble, int *index, int *step)
{
	int difference, sign;

/* Get new index value */
	*index += ima4_index[*nibble];

	if(*index < 0) *index = 0; 
	else 
	if(*index > 88) *index = 88;

/* Get sign and magnitude from *nibble */
	sign = *nibble & 8;
	*nibble = *nibble & 7;

/* Get difference */
	difference = *step >> 3;
	if(*nibble & 4) difference += *step;
	if(*nibble & 2) difference += *step >> 1;
	if(*nibble & 1) difference += *step >> 2;

/* Predict value */
	if(sign) 
	*predictor -= difference;
	else 
	*predictor += difference;

	if(*predictor > 32767) *predictor = 32767;
	else
	if(*predictor < -32768) *predictor = -32768;

/* Update the step value */
	*step = ima4_step[*index];
}

static void ima4_decode_block(quicktime_audio_map_t *atrack, int16_t *output, unsigned char *input)
{
	int predictor;
	int index;
	int step;
	int nibble, nibble_count;
	unsigned char *input_end = input + BLOCK_SIZE;

/* Get the chunk header */
	predictor = *input++ << 8;
	predictor |= *input++;

	index = predictor & 0x7f;
	if(index > 88) index = 88;

	predictor &= 0xff80;
	if(predictor & 0x8000) predictor -= 0x10000;
	step = ima4_step[index];

/* Read the input buffer sequentially, one nibble at a time */
	nibble_count = 0;
	while(input < input_end)
	{
		nibble = nibble_count ? (*input++  >> 4) & 0x0f : *input & 0x0f;

		ima4_decode_sample(&predictor, &nibble, &index, &step);
		*output++ = predictor;

		nibble_count ^= 1;
	}
}

static void ima4_encode_sample(int *last_sample, int *last_index, int *nibble, int next_sample)
{
	int difference, new_difference, mask, step;

	difference = next_sample - *last_sample;
	*nibble = 0;
	step = ima4_step[*last_index];
	new_difference = step >> 3;

	if(difference < 0)
	{
		*nibble = 8;
		difference = -difference;
	}

	mask = 4;
	while(mask)
	{
		if(difference >= step)
		{
			*nibble |= mask;
			difference -= step;
			new_difference += step;
		}

		step >>= 1;
		mask >>= 1;
	}

	if(*nibble & 8)
		*last_sample -= new_difference;
	else
		*last_sample += new_difference;

	if(*last_sample > 32767) *last_sample = 32767;
	else
	if(*last_sample < -32767) *last_sample = -32767;

	*last_index += ima4_index[*nibble];

	if(*last_index < 0) *last_index = 0;
	else
	if(*last_index > 88) *last_index= 88;
}

static void ima4_encode_block(quicktime_audio_map_t *atrack, unsigned char *output, int16_t *input, int step, int channel)
{
	quicktime_ima4_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;
	int i, nibble_count = 0, nibble, header;

/* Get a fake starting sample */
	header = codec->last_samples[channel];
/* Force rounding. */
	if(header < 0x7fc0) header += 0x40;
	if(header < 0) header += 0x10000;
	header &= 0xff80;
	*output++ = (header & 0xff00) >> 8;
	*output++ = (header & 0x80) + (codec->last_indexes[channel] & 0x7f);

	for(i = 0; i < SAMPLES_PER_BLOCK; i++)
	{
		ima4_encode_sample(&(codec->last_samples[channel]), 
							&(codec->last_indexes[channel]), 
							&nibble, 
							*input);

		if(nibble_count)
			*output++ |= (nibble << 4);
		else
			*output = nibble;

		input += step;
		nibble_count ^= 1;
	}
}

/* Convert the number of samples in a chunk into the number of bytes in that */
/* chunk.  The number of samples in a chunk should end on a block boundary. */
static long ima4_samples_to_bytes(long samples, int channels)
{
	long bytes = samples / SAMPLES_PER_BLOCK * BLOCK_SIZE * channels;
	return bytes;
}

/* =================================== public for ima4 */

static int delete_codec(quicktime_audio_map_t *atrack)
{
	quicktime_ima4_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(codec->work_buffer) free(codec->work_buffer);
	if(codec->read_buffer) free(codec->read_buffer);
	if(codec->last_samples) free(codec->last_samples);
	if(codec->last_indexes) free(codec->last_indexes);

        if(codec->decode_sample_buffer)
          {
          free(codec->decode_sample_buffer[0]);
          free(codec->decode_sample_buffer);
          }
        if(codec->decode_buffer)
          free(codec->decode_buffer);
        
        free(codec);
	return 0;
}

static int decode(quicktime_t *file, 
					int16_t **output_i, 
					float **output_f,
					long samples, 
					int track)
{
	int64_t chunk_sample;
	int64_t i;
        int64_t chunk;
        int samples_decoded, samples_copied;
	quicktime_ima4_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;
        int samples_to_skip = 0;

        if(!codec->decode_initialized)
          {
          codec->decode_initialized = 1;

          codec->decode_sample_buffer    = malloc(sizeof(*(codec->decode_sample_buffer)) * file->atracks[track].channels);
          codec->decode_sample_buffer[0] = malloc(sizeof(*(codec->decode_sample_buffer[0])) * file->atracks[track].channels *
                                           SAMPLES_PER_BLOCK);

          for(i = 1; i < file->atracks[track].channels; i++)
            codec->decode_sample_buffer[i] = codec->decode_sample_buffer[0] + i * SAMPLES_PER_BLOCK;

          /* Read first chunk */
          
          codec->decode_buffer_size = lqt_read_audio_chunk(file,
                                                           track, file->atracks[track].current_chunk,
                                                           &(codec->decode_buffer),
                                                           &(codec->decode_buffer_alloc));
          if(codec->decode_buffer_size <= 0)
            return 0;
          codec->decode_buffer_ptr = codec->decode_buffer;
          }
        
        if(file->atracks[track].current_position != file->atracks[track].last_position)
          {
          /* Seeking happened */
          quicktime_chunk_of_sample(&chunk_sample, &chunk,
                                    file->atracks[track].track,
                                    file->atracks[track].current_position);

          /* Read the chunk */

          if(file->atracks[track].current_chunk != chunk)
            //          if(1)
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
          
#if 0          
          fprintf(stderr, "Seeking done, pos: %lld, chunk_sample: %lld, chunk: %lld\n",
                  file->atracks[track].current_position,
                  chunk_sample,
                  file->atracks[track].current_chunk);
#endif    
          samples_to_skip = file->atracks[track].current_position - chunk_sample;
          if(samples_to_skip < 0)
            {
            fprintf(stderr, "ima4: Cannot skip backwards\n");
            samples_to_skip = 0;
            }
          
          /* Skip frames */

          while(samples_to_skip > SAMPLES_PER_BLOCK)
            {
            codec->decode_buffer_ptr += file->atracks[track].channels * BLOCK_SIZE;
            codec->decode_buffer_size -= file->atracks[track].channels * BLOCK_SIZE;
            samples_to_skip -= SAMPLES_PER_BLOCK;
            }

          /* Decode frame */

          for(i = 0; i < file->atracks[track].channels; i++)
            {
            ima4_decode_block(&(file->atracks[track]), codec->decode_sample_buffer[i], codec->decode_buffer_ptr);
            codec->decode_buffer_ptr += BLOCK_SIZE;
            codec->decode_buffer_size -= BLOCK_SIZE;
            }

          /* Skip samples */
          
          codec->decode_sample_buffer_size = SAMPLES_PER_BLOCK - samples_to_skip;
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
                break;
              codec->decode_buffer_ptr = codec->decode_buffer;
              }
            
            /* Decode one frame */

            for(i = 0; i < file->atracks[track].channels; i++)
              {
              ima4_decode_block(&(file->atracks[track]), codec->decode_sample_buffer[i], codec->decode_buffer_ptr);
              codec->decode_buffer_ptr += BLOCK_SIZE;
              codec->decode_buffer_size -= BLOCK_SIZE;
              }
            codec->decode_sample_buffer_size = SAMPLES_PER_BLOCK;
            }
          
          /* Copy samples */

          samples_copied = lqt_copy_audio(output_i,                                     // int16_t ** dst_i
                                          output_f,                                     // float ** dst_f
                                          codec->decode_sample_buffer,                         // int16_t ** src_i
                                          NULL,                                         // float ** src_f
                                          samples_decoded,                              // int dst_pos
                                          SAMPLES_PER_BLOCK - codec->decode_sample_buffer_size,// int src_pos
                                          samples - samples_decoded,                    // int dst_size
                                          codec->decode_sample_buffer_size,                    // int src_size
                                          file->atracks[track].channels);               // int num_channels
          
          samples_decoded += samples_copied;
          codec->decode_sample_buffer_size -= samples_copied;
          }
        
        file->atracks[track].last_position = file->atracks[track].current_position + samples_decoded;
        return samples_decoded;
#if 0 /* Old version, hopefully obsolete */
        
/* Get the first chunk with this routine and then increase the chunk number. */
	quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, file->atracks[track].current_position);

/* Read chunks and extract ranges of samples until the output is full. */
	for(i = 0; i < samples && !result; )
	{
/* Get chunk we're on. */
		chunk_samples = quicktime_chunk_samples(trak, chunk);

		if(!codec->work_buffer ||
			codec->chunk != chunk ||
			codec->buffer_channel != channel)
		{
/* read a new chunk if necessary */
			result = ima4_decode_chunk(file, track, chunk, channel);
		}

/* Get boundaries from the chunk */
		chunk_start = 0;
		if(chunk_sample < file->atracks[track].current_position)
			chunk_start = file->atracks[track].current_position - chunk_sample;

		chunk_end = chunk_samples;
		if(chunk_sample + chunk_end > file->atracks[track].current_position + samples)
			chunk_end = file->atracks[track].current_position + samples - chunk_sample;

/* Read from the chunk */
		if(output_i)
		{
/*printf("decode_ima4 1 chunk %ld %ld-%ld output %ld\n", chunk, chunk_start + chunk_sample, chunk_end + chunk_sample, i); */
			while(chunk_start < chunk_end)
			{
				output_i[i++] = codec->work_buffer[chunk_start++];
			}
/*printf("decode_ima4 2\n"); */
		}
		else
		if(output_f)
		{
			while(chunk_start < chunk_end)
			{
				output_f[i++] = (float)codec->work_buffer[chunk_start++] / 32767;
			}
		}

		chunk++;
		chunk_sample += chunk_samples;
	}

	return result;
#endif
}

static int encode(quicktime_t *file, 
						int16_t **input_i, 
						float **input_f, 
						int track, 
						long samples)
{
	int result = 0;
	int64_t i, j, step;
	int64_t chunk_bytes;
	int64_t overflow_start;
	int64_t chunk_samples; /* Samples in the current chunk to be written */
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_ima4_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	quicktime_trak_t *trak = track_map->track;
	int16_t *input_ptr;
	unsigned char *output_ptr;
	quicktime_atom_t chunk_atom;

/* Get buffer sizes */
	if(codec->work_buffer && codec->work_size < (samples + codec->work_overflow + 1) * track_map->channels)
	{
/* Create new buffer */
		int64_t new_size = (samples + codec->work_overflow + 1) * track_map->channels;
		int16_t *new_buffer = malloc(sizeof(int16_t) * new_size);

/* Copy overflow */
		for(i = 0; i < codec->work_overflow * track_map->channels; i++)
			new_buffer[i] = codec->work_buffer[i];

/* Swap pointers. */
		free(codec->work_buffer);
		codec->work_buffer = new_buffer;
		codec->work_size = new_size;
	}
	else
	if(!codec->work_buffer)
	{
/* No buffer in the first place. */
		codec->work_size = (samples + codec->work_overflow) * track_map->channels;
/* Make the allocation enough for at least the flush routine. */
		if(codec->work_size < SAMPLES_PER_BLOCK * track_map->channels)
			codec->work_size = SAMPLES_PER_BLOCK * track_map->channels;
		codec->work_buffer = malloc(sizeof(int16_t) * codec->work_size);
	}

/* Get output size */
	chunk_bytes = ima4_samples_to_bytes(samples + codec->work_overflow, track_map->channels);
	if(codec->read_buffer && codec->read_size < chunk_bytes)
	{
		free(codec->read_buffer);
		codec->read_buffer = 0;
	}

	if(!codec->read_buffer)
	{
		codec->read_buffer = malloc(chunk_bytes);
		codec->read_size = chunk_bytes;
	}

	if(!codec->last_samples)
	{
		codec->last_samples = malloc(sizeof(int) * track_map->channels);
		for(i = 0; i < track_map->channels; i++)
		{
			codec->last_samples[i] = 0;
		}
	}

	if(!codec->last_indexes)
	{
		codec->last_indexes = malloc(sizeof(int) * track_map->channels);
		for(i = 0; i < track_map->channels; i++)
		{
			codec->last_indexes[i] = 0;
		}
	}

/* Arm the input buffer after the last overflow */
	step = track_map->channels;
	for(j = 0; j < track_map->channels; j++)
	{
		input_ptr = codec->work_buffer + codec->work_overflow * track_map->channels + j;

		if(input_i)
		{
			for(i = 0; i < samples; i++)
			{
				*input_ptr = input_i[j][i];
				input_ptr += step;
			}
		}
		else
		if(input_f)
		{
			for(i = 0; i < samples; i++)
			{
				*input_ptr = (int16_t)(input_f[j][i] * 32767);
				input_ptr += step;
			}
		}
	}

/* Encode from the input buffer to the read_buffer up to a multiple of  */
/* blocks. */
	input_ptr = codec->work_buffer;
	output_ptr = codec->read_buffer;

	for(i = 0; 
		i + SAMPLES_PER_BLOCK <= samples + codec->work_overflow; 
		i += SAMPLES_PER_BLOCK)
	{
		for(j = 0; j < track_map->channels; j++)
		{
			ima4_encode_block(track_map, output_ptr, input_ptr + j, track_map->channels, j);

			output_ptr += BLOCK_SIZE;
		}
		input_ptr += SAMPLES_PER_BLOCK * track_map->channels;
	}

/* Write to disk */
	chunk_samples = (int64_t)((samples + codec->work_overflow) / SAMPLES_PER_BLOCK) * SAMPLES_PER_BLOCK;

/*printf("quicktime_encode_ima4 1 %ld\n", chunk_samples); */
/* The block division may result in 0 samples getting encoded. */
/* Don't write 0 samples. */
	if(chunk_samples)
	{
		quicktime_write_chunk_header(file, trak, &chunk_atom);
		result = quicktime_write_data(file, codec->read_buffer, chunk_bytes);
		quicktime_write_chunk_footer(file, 
			trak,
			track_map->current_chunk,
			&chunk_atom, 
			chunk_samples);

		if(result) 
			result = 0; 
		else 
			result = 1; /* defeat fwrite's return */

		track_map->current_chunk++;
	}

/* Move the last overflow to the front */
	overflow_start = i;
	input_ptr = codec->work_buffer;
	for(i = overflow_start * track_map->channels ; 
		i < (samples + codec->work_overflow) * track_map->channels; 
		i++)
	{
		*input_ptr++ = codec->work_buffer[i];
	}
	codec->work_overflow = samples + codec->work_overflow - overflow_start;

	return result;
}

static void flush(quicktime_t *file, int track)
{
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_ima4_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int result = 0;
	int i;

/*printf("quicktime_flush_ima4 %ld\n", codec->work_overflow); */
	if(codec->work_overflow)
	{
/* Zero out enough to get a block */
		i = codec->work_overflow * track_map->channels;
		while(i < SAMPLES_PER_BLOCK * track_map->channels)
		{
			codec->work_buffer[i++] = 0;
		}
		codec->work_overflow = i / track_map->channels + 1;
/* Write the work_overflow only. */
		result = encode(file, 0, 0, track, 0);
	}
}

void quicktime_init_codec_ima4(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_ima4_codec_t *codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_ima4_codec_t));
	codec_base->delete_acodec = delete_codec;
	codec_base->decode_video = 0;
	codec_base->encode_video = 0;
	codec_base->decode_audio = decode;
	codec_base->encode_audio = encode;
	codec_base->flush = flush;
	codec_base->fourcc = QUICKTIME_IMA4;
	codec_base->title = "IMA 4";
	codec_base->desc = "IMA 4";
	codec_base->wav_id = 0x11;

/* Init private items */
	codec = codec_base->priv;
}
