#include "funcprotos.h"
#include <quicktime/quicktime.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>

#include "rawaudio.h"

#define SAMPLES_PER_BLOCK 1024


typedef struct
{
	uint8_t *work_buffer;
	long buffer_size;

/* Decode specific stuff */
        int decode_buffer_size;
        int decode_buffer_alloc;
        uint8_t * decode_buffer;
        uint8_t * decode_buffer_ptr;

        int16_t ** decode_sample_buffer_i;
        float ** decode_sample_buffer_f;
        int decode_sample_buffer_size;
        int decode_sample_buffer_pos;
        int decode_initialized;
        int decode_block_align;

} quicktime_rawaudio_codec_t;

/* =================================== private for rawaudio */

static int get_work_buffer(quicktime_t *file, int track, long bytes)
{
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)file->atracks[track].codec)->priv;

	if(codec->work_buffer && codec->buffer_size != bytes)
	{
		free(codec->work_buffer);
		codec->work_buffer = 0;
	}
	
	if(!codec->work_buffer) 
	{
		codec->buffer_size = bytes;
		if(!(codec->work_buffer = malloc(bytes))) return 1;
	}
	return 0;
}

/* =================================== public for rawaudio */

static int quicktime_delete_codec_rawaudio(quicktime_audio_map_t *atrack)
{
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;

	if(codec->work_buffer) free(codec->work_buffer);
	if(codec->decode_buffer) free(codec->decode_buffer);

        if(codec->decode_sample_buffer_i)
          {
          free(codec->decode_sample_buffer_i[0]);
          free(codec->decode_sample_buffer_i);
          }
        if(codec->decode_sample_buffer_f)
          {
          free(codec->decode_sample_buffer_f[0]);
          free(codec->decode_sample_buffer_f);
          }
        
        codec->work_buffer = 0;
	codec->buffer_size = 0;
	free(codec);
	return 0;
}

static int read_audio_chunk(quicktime_t * file, int track,
                            long chunk,
                            uint8_t ** buffer, int * buffer_alloc)
  {
  int bytes, samples, bytes_from_samples;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

  bytes = lqt_read_audio_chunk(file, track, chunk, buffer, buffer_alloc, &samples);
  bytes_from_samples = samples * codec->decode_block_align;
  if(bytes > bytes_from_samples)
    return bytes_from_samples;
  else
    return bytes;
  }


static int quicktime_decode_rawaudio(quicktime_t *file, 
					int16_t ** output_i, 
					float ** output_f, 
					long samples, 
					int track)
{

        int j;
        quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;

	int64_t chunk, chunk_sample;
	int64_t i;

        int samples_decoded, samples_copied;

        int64_t samples_to_skip = 0;
        int bits;
        
        bits = quicktime_audio_bits(file, track);
        
        if(!codec->decode_initialized)
          {
          codec->decode_initialized = 1;

          if(bits <= 16)
            {
            codec->decode_sample_buffer_i    = malloc(sizeof(*(codec->decode_sample_buffer_i)) * file->atracks[track].channels);
            codec->decode_sample_buffer_i[0] = malloc(sizeof(*(codec->decode_sample_buffer_i[0])) * file->atracks[track].channels *
                                                      SAMPLES_PER_BLOCK);
            for(i = 1; i < file->atracks[track].channels; i++)
              codec->decode_sample_buffer_i[i] = codec->decode_sample_buffer_i[0] + i * SAMPLES_PER_BLOCK;
            
            }
          else
            {
            codec->decode_sample_buffer_f    = malloc(sizeof(*(codec->decode_sample_buffer_f)) * file->atracks[track].channels);
            codec->decode_sample_buffer_f[0] = malloc(sizeof(*(codec->decode_sample_buffer_f[0])) * file->atracks[track].channels *
                                                      SAMPLES_PER_BLOCK);

            for(i = 1; i < file->atracks[track].channels; i++)
              codec->decode_sample_buffer_f[i] = codec->decode_sample_buffer_f[0] + i * SAMPLES_PER_BLOCK;
            }

          codec->decode_block_align = track_map->channels * bits / 8;

          /* Read first chunk */

          codec->decode_buffer_size = read_audio_chunk(file,
                                                           track, file->atracks[track].current_chunk,
                                                           &(codec->decode_buffer),
                                                           &(codec->decode_buffer_alloc));
          if(codec->decode_buffer_size <= 0)
            return 0;
          codec->decode_buffer_ptr = codec->decode_buffer;
          }
        
        if(file->atracks[track].current_position != file->atracks[track].last_position)
          {
          //          fprintf(stderr, "SEEKED\n");
          
          /* Seeking happened */
          quicktime_chunk_of_sample(&chunk_sample, &chunk,
                                    file->atracks[track].track,
                                    file->atracks[track].current_position);

          /* Read the chunk */

          if(file->atracks[track].current_chunk != chunk)
            {
            file->atracks[track].current_chunk = chunk;
            codec->decode_buffer_size = read_audio_chunk(file,
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
            fprintf(stderr, "rawaudio: Cannot skip backwards\n");
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
              //              fprintf(stderr, "Reading chunk\n");
              file->atracks[track].current_chunk++;
              codec->decode_buffer_size = read_audio_chunk(file,
                                                               track, file->atracks[track].current_chunk,
                                                               &(codec->decode_buffer),
                                                               &(codec->decode_buffer_alloc));
              
              if(codec->decode_buffer_size <= 0)
                break;
              codec->decode_buffer_ptr = codec->decode_buffer;
              }

            codec->decode_sample_buffer_size = codec->decode_buffer_size / codec->decode_block_align;
            if(codec->decode_sample_buffer_size > SAMPLES_PER_BLOCK)
              {
              codec->decode_sample_buffer_size = SAMPLES_PER_BLOCK;
              }

            codec->decode_sample_buffer_pos = 0;
            
            /* Decode the stuff */
            
            switch(bits)
              {
              case 8:
                for(i = 0; i < codec->decode_sample_buffer_size; i++)
                  {
                  for(j = 0; j < track_map->channels; j++)
                    {
                    codec->decode_sample_buffer_i[j][i] = (((int16_t)(*codec->decode_buffer_ptr)) << 8) ^ 0x8000;
                    codec->decode_buffer_ptr++;
                    codec->decode_buffer_size--;
                    }
                  }
                break;
              case 16:
                for(i = 0; i < codec->decode_sample_buffer_size; i++)
                  {
                  for(j = 0; j < track_map->channels; j++)
                    {
                    codec->decode_sample_buffer_i[j][i] = ((int16_t)codec->decode_buffer_ptr[0]) << 8 |
                      ((unsigned char)codec->decode_buffer_ptr[0]);
                    codec->decode_buffer_ptr+= 2;
                    codec->decode_buffer_size-=2;
                    }
                  }
                break;
              case 24:
                for(i = 0; i < codec->decode_sample_buffer_size; i++)
                  {
                  for(j = 0; j < track_map->channels; j++)
                    {
                    codec->decode_sample_buffer_i[j][i] = (float)((((int)codec->decode_buffer_ptr[0]) << 16) | 
                                                           (((unsigned char)codec->decode_buffer_ptr[1]) << 8) |
                                                           ((unsigned char)codec->decode_buffer_ptr[2])) / 0x7fffff;
                    codec->decode_buffer_ptr+= 3;
                    codec->decode_buffer_size-=3;
                    }
                  }
                break;
              default:
                break;
              }
            }
          
          /* Copy samples */
          //          fprintf(stderr, "Decode: %d\n", codec->decode_sample_buffer_size);
                    
          samples_copied = lqt_copy_audio(output_i,     // int16_t ** dst_i
                                          output_f,     // float ** dst_f
                                          codec->decode_sample_buffer_i,    // int16_t ** src_i
                                          codec->decode_sample_buffer_f,    // float ** src_f
                                          samples_decoded,                  // int dst_pos
                                          codec->decode_sample_buffer_pos,  // int src_pos
                                          samples - samples_decoded,        // int dst_size
                                          codec->decode_sample_buffer_size, // int src_size
                                          file->atracks[track].channels);   // int num_channels
          
          samples_decoded += samples_copied;
          codec->decode_sample_buffer_size -= samples_copied;
          codec->decode_sample_buffer_pos += samples_copied;

          }
        
        file->atracks[track].last_position = file->atracks[track].current_position + samples_decoded;
        return samples_decoded;

#if 0 /* Old (hopefully obsolete) version */

        int step = file->atracks[track].channels * quicktime_audio_bits(file, track) / 8;

	get_work_buffer(file, track, samples * step);
	result = !quicktime_read_audio(file, codec->work_buffer, samples, track);
// Undo increment since this is done in codecs.c
	track_map->current_position -= samples;

	switch(quicktime_audio_bits(file, track))
	{
		case 8:
			if(output_i && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_i[i] = ((int16_t)((unsigned char)codec->work_buffer[j]) << 8);
					j += step;
					output_i[i] -= 0x8000;
				}
			}
			else
			if(output_f && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_f[i] = (float)((unsigned char)codec->work_buffer[j]) - 0x80;
					output_f[i] /= 0x7f;
					j += step;
				}
			}
			break;

		case 16:
			if(output_i && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_i[i] = (int16_t)(codec->work_buffer[j]) << 8 |
									(unsigned char)(codec->work_buffer[j + 1]);
					j += step;
//					output_i[i] -= 0x8000;
				}
			}
			else
			if(output_f && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_f[i] = (float)((int16_t)(codec->work_buffer[j]) << 8 |
//									(unsigned char)(codec->work_buffer[j + 1])) - 0x8000;
                                                                      (unsigned char)(codec->work_buffer[j + 1]));
					output_f[i] /= 0x7fff;
					j += step;
				}
			}
			break;

		case 24:
			if(output_i && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_i[i] = ((int16_t)(codec->work_buffer[j]) << 8) | 
									(unsigned char)(codec->work_buffer[j + 1]);
					output_i[i] -= 0x8000;
					j += step;
				}
			}
			else
			if(output_f && !result)
			{
				for(i = 0, j = 0; i < samples; i++)
				{
					output_f[i] = (float)(((int)(codec->work_buffer[j]) << 16) | 
									((unsigned int)(codec->work_buffer[j + 1]) << 8) |
									(unsigned char)(codec->work_buffer[j + 2])) - 0x800000;
					output_f[i] /= 0x7fffff;
					j += step;
				}
			}
			break;

		default:
			break;
	}
/*printf("quicktime_decode_rawaudio 2\n"); */

	return result;
#endif
}

#define CLAMP(x, y, z) ((x) = ((x) <  (y) ? (y) : ((x) > (z) ? (z) : (x))))

static int quicktime_encode_rawaudio(quicktime_t *file, 
                                     int16_t **input_i, 
                                     float **input_f, 
                                     int track, 
                                     long samples)
  {
  int result = 0;
  long i, j;
  quicktime_audio_map_t *track_map = &(file->atracks[track]);
  quicktime_rawaudio_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
  int step = file->atracks[track].channels * quicktime_audio_bits(file, track) / 8;
  int sample;
  float sample_f;

  get_work_buffer(file, track, samples * step);

  if(input_i)
    {
    for(i = 0; i < track_map->channels; i++)
      {
      switch(quicktime_audio_bits(file, track))
        {
        case 8:
          for(j = 0; j < samples; j++)
            {
            sample = input_i[i][j] >> 8;
            sample += 0x80;
            codec->work_buffer[j * step + i] = sample;
            }
          break;
        case 16:
          for(j = 0; j < samples; j++)
            {
            sample = input_i[i][j];
            //						sample += 0x8000;
            codec->work_buffer[j * step + i * 2] = ((unsigned int)sample & 0xff00) >> 8;
            codec->work_buffer[j * step + i * 2 + 1] = ((unsigned int)sample) & 0xff;
            }
          break;
        case 24:
          for(j = 0; j < samples; j++)
            {
            sample = input_i[i][j];
            sample += 0x8000;
            codec->work_buffer[j * step + i * 3] = ((unsigned int)sample & 0xff00) >> 8;
            codec->work_buffer[j * step + i * 3 + 1] = ((unsigned int)sample & 0xff);
            codec->work_buffer[j * step + i * 3 + 2] = 0;
            }
          break;
        }
      }
    }
  else
    {
    for(i = 0; i < track_map->channels; i++)
      {
      switch(quicktime_audio_bits(file, track))
        {
        case 8:
          for(j = 0; j < samples; j++)
            {
            sample_f = input_f[i][j];
            if(sample_f < 0)
              sample = (int)(sample_f * 0x7f - 0.5);
            else
              sample = (int)(sample_f * 0x7f + 0.5);
            CLAMP(sample, -0x7f, 0x7f);
            sample += 0x80;
            codec->work_buffer[j * step + i] = sample;
            }
          break;
        case 16:
          for(j = 0; j < samples; j++)
            {
            sample_f = input_f[i][j];
            if(sample_f < 0)
              sample = (int)(sample_f * 0x7fff - 0.5);
            else
              sample = (int)(sample_f * 0x7fff + 0.5);
            CLAMP(sample, -0x7fff, 0x7fff);
            //						sample += 0x8000;
            codec->work_buffer[j * step + i * 2] = ((unsigned int)sample & 0xff00) >> 8;
            codec->work_buffer[j * step + i * 2 + 1] = ((unsigned int)sample) & 0xff;
            }
          break;
        case 24:
          for(j = 0; j < samples; j++)
            {
            sample_f = input_f[i][j];
            if(sample_f < 0)
              sample = (int)(sample_f * 0x7fffff - 0.5);
            else
              sample = (int)(sample_f * 0x7fffff + 0.5);
            CLAMP(sample, -0x7fffff, 0x7fffff);
            sample += 0x800000;
            codec->work_buffer[j * step + i * 3] = ((unsigned int)sample & 0xff0000) >> 16;
            codec->work_buffer[j * step + i * 3 + 1] = ((unsigned int)sample & 0xff00) >> 8;
            codec->work_buffer[j * step + i * 3 + 2] = ((unsigned int)sample) & 0xff;
            }
          break;
        }
      }
    }

  result = quicktime_write_audio(file, codec->work_buffer, samples, track);
  return result;
  }


void quicktime_init_codec_rawaudio(quicktime_audio_map_t *atrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)atrack->codec;
	quicktime_rawaudio_codec_t *codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_rawaudio_codec_t));
	codec_base->delete_acodec = quicktime_delete_codec_rawaudio;
	codec_base->decode_video = 0;
	codec_base->encode_video = 0;
	codec_base->decode_audio = quicktime_decode_rawaudio;
	codec_base->encode_audio = quicktime_encode_rawaudio;
	codec_base->fourcc = QUICKTIME_RAW;
	codec_base->title = "8 bit unsigned";
	codec_base->desc = "8 bit unsigned for video";
	codec_base->wav_id = 0x01;

/* Init private items */
	codec = codec_base->priv;
}
