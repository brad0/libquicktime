#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <time.h>
#include <string.h>
#include "qtvorbis.h"
#include "vorbis/vorbisenc.h"

#define OUTPUT_ALLOCATION 0x100000
#define CLAMP(x, y, z) ((x) = ((x) <  (y) ? (y) : ((x) > (z) ? (z) : (x))))

typedef struct
{
	int max_bitrate;
	int nominal_bitrate;
	int min_bitrate;
	int encode_initialized;
	ogg_stream_state enc_os;
	ogg_page enc_og;
	ogg_packet enc_op;
	vorbis_info enc_vi;
	vorbis_comment enc_vc;
	vorbis_dsp_state enc_vd;
	vorbis_block enc_vb;




	ogg_sync_state   dec_oy; /* sync and verify incoming physical bitstream */
	ogg_stream_state dec_os; /* take physical pages, weld into a logical
				stream of packets */
	ogg_page         dec_og; /* one Ogg bitstream page.  Vorbis packets are inside */
	ogg_packet       dec_op; /* one raw packet of data for decode */

	vorbis_info      dec_vi; /* struct that stores all the static vorbis bitstream
				settings */
	vorbis_comment   dec_vc; /* struct that stores all the bitstream user comments */
	vorbis_dsp_state dec_vd; /* central working state for the packet->PCM decoder */
	vorbis_block     dec_vb; /* local working space for packet->PCM decode */

	unsigned int dec_current_serialno;
	int decode_initialized;
	float **output;

// Number of last sample relative to file
	longest output_position;
// Number of last sample relative to output buffer
	long output_end;
// Number of samples in output buffer
	long output_size;
// Number of samples allocated in output buffer
	long output_allocated;
// Current reading position in file
	longest chunk;
// Number of samples decoded in the current chunk
	int chunk_samples;
} quicktime_vorbis_codec_t;


/* =================================== public for vorbis */

static int delete_codec(quicktime_audio_map_t *atrack)
{
	quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;
	int i;

	if(codec->encode_initialized)
	{
		ogg_stream_clear(&codec->enc_os);
		vorbis_block_clear(&codec->enc_vb);
		vorbis_dsp_clear(&codec->enc_vd);
		vorbis_comment_clear(&codec->enc_vc);
		vorbis_info_clear(&codec->enc_vi);
	}

	if(codec->decode_initialized)
	{
		if(codec->output) 
		{
			for(i = 0; i < atrack->channels; i++)
				free(codec->output[i]);

			free(codec->output);
		}

		ogg_stream_clear(&codec->dec_os);
		vorbis_block_clear(&codec->dec_vb);
		vorbis_dsp_clear(&codec->dec_vd);
		vorbis_comment_clear(&codec->dec_vc);
		vorbis_info_clear(&codec->dec_vi);
	}

	free(codec);
	return 0;
}





// Buffer fragment should be bigger then largest page header so
// all lacing bytes can be decoded.
#define BUFFER_FRAGMENT 4096

// Calculate chunk length from OggS headers.  This is the worst case
// but it's better not to assume libogg is going to do anything for us.

#define SEGMENT_OFFSET 0x1a
#define LACE_OFFSET 0x1b
static int chunk_len(quicktime_t *file, longest offset, longest next_chunk)
{
	int result = 0;
	unsigned char buffer[BUFFER_FRAGMENT];
	int accum = 0;
	int segment_count = 0;
	int segment_size = 0;
	int lace_size = 0;
	int page_size = 0;
	int i, j;

	while(offset < next_chunk)
	{
		quicktime_set_position(file, offset);
		result = !quicktime_read_data(file, buffer, BUFFER_FRAGMENT);

		if(result)
		{
//printf("chunk_len 1 %x\n", accum);
			return accum;
		}

		if(memcmp(buffer, "OggS", 4))
		{
//printf("chunk_len 2 %x %02x %02x %02x %02x\n", offset, buffer[0], buffer[1], buffer[2], buffer[3]);
			return accum;
		}
		else
		{
//printf("chunk_len 3 %x\n", offset);
		}

// Decode size of OggS page
		segment_count = buffer[SEGMENT_OFFSET];
//printf("chunk_len 4 %x\n", segment_count);

// Decode one segment at a time
		i = LACE_OFFSET;
		page_size = 0;
		while(segment_count > 0)
		{
			page_size += buffer[i++];
			segment_count--;
		}
//printf("chunk_len 6 %x\n", page_size);
		accum += i + page_size;
		offset += i + page_size;
	}

//printf("chunk_len 7 %x %02x %02x %02x %02x\n", page_size);

	return accum;
}


// Calculates the chunk size based on ogg pages.
#define READ_CHUNK(chunk) \
{ \
	longest offset1 = quicktime_chunk_to_offset(trak, (chunk)); \
	longest offset2 = quicktime_chunk_to_offset(trak, (chunk) + 1); \
	int size = 0; \
	if(offset2 == offset1) \
		result = 1; \
	else \
	{ \
		size = chunk_len(file, offset1, \
			offset2 > offset1 ? offset2 : offset1 + 0xfffff); \
 \
		buffer = ogg_sync_buffer(&codec->dec_oy, size); \
		quicktime_set_position(file, offset1); \
		result = !quicktime_read_data(file, buffer, size); \
		ogg_sync_wrote(&codec->dec_oy, size); \
	} \
/* printf("%llx %x: ", quicktime_chunk_to_offset(trak, (chunk)), size); */ \
/* for(i = 0; i < 16; i++) */ \
/* 	printf("%02x ", buffer[i]); */ \
/* printf("result=%d\n", result); */ \
}




static int decode(quicktime_t *file, 
					int16_t *output_i, 
					float *output_f, 
					long samples, 
					int track, 
					int channel)
{
	int result = 0;
	int bytes;
	int i, j;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	long current_position = track_map->current_position;
	long end_position = current_position + samples;
  	unsigned char *buffer;
// End of data in ogg buffer
	int eos = 0;
// End of file
	int eof = 0;
	float *pcm;
	int have_chunk = 0;



//printf("decode 1\n");



	if(output_i) bzero(output_i, sizeof(int16_t) * samples);
	if(output_f) bzero(output_f, sizeof(float) * samples);







// Seeked outside output buffer's range or not initialized: restart
	if(current_position < codec->output_position - codec->output_size ||
		current_position > codec->output_position ||
		!codec->decode_initialized)
	{

//printf("decode 1 %d %lld %ld\n", current_position, codec->output_position - codec->output_size,	codec->output_position);
		quicktime_chunk_of_sample(&codec->output_position, 
			&codec->chunk, 
			trak, 
			current_position);
// We know the first ogg packet in the chunk has a pcm_offset from the encoding.

		codec->output_size = 0;
		codec->output_end = 0;
		codec->chunk_samples = 0;



	
// Initialize and load initial buffer for decoding
		if(!codec->decode_initialized)
		{
			int init_chunk = 1;
			codec->decode_initialized = 1;
//printf("decode 2\n");

			codec->output = malloc(sizeof(float*) * track_map->channels);
			for(i = 0; i < track_map->channels; i++)
			{
				codec->output[i] = malloc(sizeof(float) * OUTPUT_ALLOCATION);
			}

			codec->output_allocated = OUTPUT_ALLOCATION;

        	ogg_sync_init(&codec->dec_oy); /* Now we can read pages */




//printf("decode 3\n");
			READ_CHUNK(init_chunk);
			init_chunk++;

//for(i = 0; i < 16; i++)
//	printf("%02x ", buffer[i]);
//printf("%llx\n", codec->chunk);
//printf("\n");
//printf("decode 4\n");

   	 		if(ogg_sync_pageout(&codec->dec_oy, &codec->dec_og)!=1)
			{
				fprintf(stderr, "decode: ogg_sync_pageout: Must not be Vorbis data\n");
				return 1;
			}

//printf("decode 4\n");

    		ogg_stream_init(&codec->dec_os, ogg_page_serialno(&codec->dec_og));
    		vorbis_info_init(&codec->dec_vi);
    		vorbis_comment_init(&codec->dec_vc);

//printf("decode 4\n");
    		if(ogg_stream_pagein(&codec->dec_os, &codec->dec_og) < 0)
			{
    	  		fprintf(stderr,"decode: ogg_stream_pagein: stream version mismatch perhaps.\n");
    	  		return 1;
    		}
//printf("decode 4\n");

			if(ogg_stream_packetout(&codec->dec_os, &codec->dec_op) != 1)
			{
				fprintf(stderr, "decode: ogg_stream_packetout: Must not be Vorbis data\n");
    	  		return 1;
			}
//printf("decode 4\n");

			if(vorbis_synthesis_headerin(&codec->dec_vi, &codec->dec_vc, &codec->dec_op) < 0)
			{
				fprintf(stderr, "decode: vorbis_synthesis_headerin: not a vorbis header\n");
				return 1;
			}
//printf("decode 4\n");


			i = 0;
			while(i < 2)
			{
				while(i < 2)
				{
					result = ogg_sync_pageout(&codec->dec_oy, &codec->dec_og);
					if(result == 0) break;

					if(result == 1)
					{
						ogg_stream_pagein(&codec->dec_os, &codec->dec_og);

						while(i < 2)
						{
							result = ogg_stream_packetout(&codec->dec_os, &codec->dec_op);

							if(result == 0) break;

							if(result < 0)
							{
								fprintf(stderr, "decode: ogg_stream_packetout: corrupt secondary header\n");
								return 1;
							}

							vorbis_synthesis_headerin(&codec->dec_vi, &codec->dec_vc, &codec->dec_op);
							i++;




						}
					}
				}

//printf("decode 5\n");
				if(i < 2)
				{
					READ_CHUNK(init_chunk);
					init_chunk++;
				}

// Header should never span more than one chunk so assume it's done here
			}

//printf("decode 6\n");
			vorbis_synthesis_init(&codec->dec_vd, &codec->dec_vi);
			vorbis_block_init(&codec->dec_vd, &codec->dec_vb);
//printf("decode 7\n");

// Also the first chunk needed in decoding so don't reread after this.
			if(codec->chunk == init_chunk - 1) 
			{
				have_chunk = 1;
				codec->chunk++;
			}
		}




// Don't already have initial chunk from header
		if(!have_chunk)
		{
// Get initial chunk for decoding at new location
// From vorbisfile.c
/* clear out decoding machine state */
			ogg_stream_clear(&codec->dec_os);
			vorbis_dsp_clear(&codec->dec_vd);
			vorbis_block_clear(&codec->dec_vb);
    		ogg_sync_reset(&codec->dec_oy);

    		ogg_stream_init(&codec->dec_os, ogg_page_serialno(&codec->dec_og));
        	ogg_sync_init(&codec->dec_oy);
			vorbis_synthesis_init(&codec->dec_vd, &codec->dec_vi);
			vorbis_block_init(&codec->dec_vd, &codec->dec_vb);


			READ_CHUNK(codec->chunk);
			codec->chunk++;
			have_chunk = 1;
		}
	}
//printf("decode 1 codec->output_position=%lld codec->output_end=%d codec->output_size=%d\n", 
//	codec->output_position, codec->output_end, codec->output_size);

// Assume the chunk exists by now and rely on libogg to say if it's out of
// data.
	have_chunk = 1;









//printf("decode 4 %llx %lx\n", codec->output_position, end_position);


// Read chunks until output buffer is on or after end_position
	result = 0;
	while(codec->output_position < end_position)
	{


// Read chunk to decode if it hasn't been read yet.
		if(!have_chunk)
		{
			codec->chunk_samples = 0;

//printf("decode 5 expected=%x\n", quicktime_chunk_samples(trak, codec->chunk));
			READ_CHUNK(codec->chunk);
			if(result) break;
			codec->chunk++;
		}

		eos = 0;
		while(!eos)
		{
			result = ogg_sync_pageout(&codec->dec_oy, &codec->dec_og);
//printf("decode 6 %d\n", result);







// Need more data from chunk
			if(result == 0)
			{
//printf("ogg_sync_pageout=0\n");
// End of chunk
				eos = 1;
			}
			else
// This stage checks for OggS and a checksum error.
// It doesn't tell if it's the end of a chunk.  Need to manually parse OggS
// pages to figure out how big the chunk is.
			if(result < 0)
			{
//printf("ogg_sync_pageout=-1\n");
				;
			}
			else
			{
				ogg_stream_pagein(&codec->dec_os, &codec->dec_og);



				while(!eos)
				{
//printf("decode 7\n");
					result = ogg_stream_packetout(&codec->dec_os, &codec->dec_op);

//printf("decode 8 %d\n", result);
					if(result == 0)
					{
//printf("ogg_stream_packetout=0\n");
// End of page
						eos = 1;
					}
					else
// This stage doesn't check for OggS.
					if(result < 0)
					{
//printf("ogg_stream_packetout=-1\n");
					}
					else
					{
						float **pcm;







						if(vorbis_synthesis(&codec->dec_vb, &codec->dec_op) == 0)
						{
							vorbis_synthesis_blockin(&codec->dec_vd, 
								&codec->dec_vb);
						}


						while((result = vorbis_synthesis_pcmout(&codec->dec_vd, &pcm)) > 0)
						{
//printf("vorbis_synthesis_pcmout=%x\n", result);
							for(i = 0; i < track_map->channels; i++)
							{
								float *output_channel = codec->output[i];
								float *input_channel = pcm[i];
								int k = codec->output_end;

								for(j = 0; j < result; j++)
								{
									output_channel[k++] = input_channel[j];
									if(k >= codec->output_allocated)
										k = 0;
								}
								
								if(i == track_map->channels - 1) 
									codec->output_end = k;
							}
//printf("codec->output_end = %d\n", codec->output_end);

							codec->output_position += result;
							codec->output_size += result;
							codec->chunk_samples += result;
							if(codec->output_size > codec->output_allocated)
								codec->output_size = codec->output_allocated;
							vorbis_synthesis_read(&codec->dec_vd, result);
						}
					}
//printf("decode 11\n");
				}

// Reset end of page so it isn't interpreted as an end of chunk
				eos = 0;
			}
		}


// Next chunk
		if(eos)
		{
//printf("decode 12 got=%x\n", codec->chunk_samples);
			have_chunk = 0;
		}
	}


// Fill silence
	while(codec->output_position < end_position)
	{
		for(i = 0; i < track_map->channels; i++)
			codec->output[i][codec->output_end] = 0;
		
		codec->output_end++;
		if(codec->output_end >= codec->output_allocated)
			codec->output_end = 0;
		codec->output_position++;
	}
//printf("decode 15\n");


//printf("decode 2 codec->output_position=%lld codec->output_end=%d codec->output_size=%d\n", 
//	codec->output_position, codec->output_end, codec->output_size);

	current_position = track_map->current_position;
	i = codec->output_end - (codec->output_position - current_position);
	j = 0;
	while(i < 0) i += codec->output_allocated;
	pcm = codec->output[channel];

	if(output_i)
	{
		for( ; j < samples; j++)
		{
			int sample = pcm[i] * 32767;
			CLAMP(sample, -32768, 32767);
			output_i[j] = sample;

			i++;
			if(i >= codec->output_allocated) i = 0;
		}
	}
	else
	if(output_f)
	{
		for( ; j < samples; j++)
		{
			output_f[j] = pcm[i];
			i++;
			if(i >= codec->output_allocated) i = 0;
		}
	}
//printf("decode 16\n");

	return 0;
}




















#define FLUSH_OGG1 \
while(1) \
{ \
	int eos = !ogg_stream_flush(&codec->enc_os, &codec->enc_og); \
	if(eos) break; \
 \
	result = !quicktime_write_data(file, codec->enc_og.header, codec->enc_og.header_len); \
	size += codec->enc_og.header_len; \
 \
	if(!result) \
	{ \
		result = !quicktime_write_data(file, codec->enc_og.body, codec->enc_og.body_len); \
		size += codec->enc_og.body_len; \
	} \
 \
/*printf("FLUSH_OGG1 %d %d %d %d\n", result, codec->og.header_len, codec->og.body_len, size); */ \
 \
	if(!result) break; \
}



#define FLUSH_OGG2 \
while(vorbis_analysis_blockout(&codec->enc_vd, &codec->enc_vb) == 1) \
{ \
/* printf("FLUSH_OGG2 1\n"); */ \
    vorbis_analysis(&codec->enc_vb, &codec->enc_op); \
/* printf("FLUSH_OGG2 2\n"); */ \
    ogg_stream_packetin(&codec->enc_os, &codec->enc_op); \
/* printf("FLUSH_OGG2 3\n"); */ \
 \
	while(!result) \
	{ \
		if(!ogg_stream_pageout(&codec->enc_os, &codec->enc_og)) break; \
 \
		result = !quicktime_write_data(file, codec->enc_og.header, codec->enc_og.header_len); \
		size += codec->enc_og.header_len; \
 \
		if(!result) \
		{ \
			result = !quicktime_write_data(file, codec->enc_og.body, codec->enc_og.body_len); \
			size += codec->enc_og.body_len; \
		} \
 \
/*printf("FLUSH_OGG2 %d %d %d %d\n", result, codec->og.header_len,  codec->og.body_len, size); */\
		if(ogg_page_eos(&codec->enc_og)) break; \
	} \
/* printf("FLUSH_OGG2 4\n"); */ \
}


static int encode(quicktime_t *file, 
							int16_t **input_i, 
							float **input_f, 
							int track, 
							long samples)
{
	int result = 0;
	longest offset = quicktime_position(file);
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_trak_t *trak = track_map->track;
	quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int samplerate = trak->mdia.minf.stbl.stsd.table[0].sample_rate;
	float **output;
	int size = 0;
	long output_position;







//printf("encode 1\n");
	if(!codec->encode_initialized)
	{
    	ogg_packet header;
    	ogg_packet header_comm;
    	ogg_packet header_code;


		codec->encode_initialized = 1;
//printf("encode 1\n");
  		vorbis_info_init(&codec->enc_vi);
//printf("encode 1\n");

		vorbis_encode_init(&codec->enc_vi,
  			track_map->channels,
			samplerate, 
			codec->max_bitrate, 
			codec->nominal_bitrate, 
			codec->min_bitrate);

//printf("encode 1\n");

//printf("encode %d %d %d\n", codec->max_bitrate, codec->nominal_bitrate, codec->min_bitrate);
  		vorbis_comment_init(&codec->enc_vc);
//printf("encode 1 %p %p\n", &codec->enc_vd, &codec->enc_vi);
  		vorbis_analysis_init(&codec->enc_vd, &codec->enc_vi);
//printf("encode 1\n");
  		vorbis_block_init(&codec->enc_vd, &codec->enc_vb);
//printf("encode 1\n");
    	srand(time(NULL));
//printf("encode 1\n");
		ogg_stream_init(&codec->enc_os, rand());

//printf("encode 1\n");

    	vorbis_analysis_headerout(&codec->enc_vd, 
			&codec->enc_vc,
			&header,
			&header_comm,
			&header_code);
//printf("encode 1\n");

    	ogg_stream_packetin(&codec->enc_os, &header); 
//printf("encode 1\n");
    	ogg_stream_packetin(&codec->enc_os, &header_comm);
//printf("encode 1\n");
    	ogg_stream_packetin(&codec->enc_os, &header_code);
//printf("encode 1\n");

		FLUSH_OGG1
		
//printf("encode 2\n");
		output_position = 0;
	}
	else
		output_position = codec->enc_vd.granulepos;
//printf("encode 3\n");

    output = vorbis_analysis_buffer(&codec->enc_vd, samples);

//printf("encode 4\n");
	if(input_i)
	{
		int i, j;
		for(i = 0; i < track_map->channels; i++)
		{
			for(j = 0; j < samples; j++)
			{
				output[i][j] = (float)input_i[i][j] / (float)32768;
			}
		}
	}
	else
	if(input_f)
	{
		int i;
		for(i = 0; i < track_map->channels; i++)
		{
			memcpy(output[i], input_f[i], sizeof(float) * samples);
		}
	}
//printf("encode 5\n");

    vorbis_analysis_wrote(&codec->enc_vd, samples);

//printf("encode 6 %ld\n", output_position);
	FLUSH_OGG2

//printf("encode 1\n");
//printf("encode %d\n", size);
	quicktime_update_tables(file,
						track_map->track, 
						offset, 
						track_map->current_chunk, 
						track_map->current_chunk - 1, 
						codec->enc_vd.granulepos - output_position, 
						0);
//printf("encode 1\n");
	file->atracks[track].current_chunk++;
//printf("encode 7\n");

	return result;
}




static int set_parameter(quicktime_t *file, 
		int track, 
		char *key, 
		void *value)
{
	quicktime_audio_map_t *atrack = &(file->atracks[track]);
	quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)atrack->codec)->priv;


	if(!strcasecmp(key, "vorbis_bitrate"))
		codec->nominal_bitrate = *(int*)value;
	else
	if(!strcasecmp(key, "vorbis_max_bitrate"))
		codec->max_bitrate = *(int*)value;
	else
	if(!strcasecmp(key, "vorbis_min_bitrate"))
		codec->min_bitrate = *(int*)value;
	return 0;
}


static void flush(quicktime_t *file, int track)
{
	int result = 0;
	int size = 0;
	longest offset = quicktime_position(file);
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_vorbis_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	long output_position = codec->enc_vd.granulepos;

//printf("flush 1\n");
	vorbis_analysis_wrote(&codec->enc_vd,0);

	FLUSH_OGG2
	
//printf("flush 2 %d\n", size);
	quicktime_update_tables(file,
						track_map->track, 
						offset, 
						track_map->current_chunk, 
						track_map->current_chunk, 
						codec->enc_vd.granulepos - output_position, 
						0);
	file->atracks[track].current_chunk++;
}

void quicktime_init_codec_vorbis(quicktime_audio_map_t *atrack)
{
	quicktime_vorbis_codec_t *codec;

/* Init public items */
	((quicktime_codec_t*)atrack->codec)->priv = calloc(1, sizeof(quicktime_vorbis_codec_t));
	((quicktime_codec_t*)atrack->codec)->delete_acodec = delete_codec;
	((quicktime_codec_t*)atrack->codec)->decode_audio = decode;
	((quicktime_codec_t*)atrack->codec)->encode_audio = encode;
	((quicktime_codec_t*)atrack->codec)->set_parameter = set_parameter;
	((quicktime_codec_t*)atrack->codec)->flush = flush;

	codec = ((quicktime_codec_t*)atrack->codec)->priv;
	codec->nominal_bitrate = 128000;
	codec->max_bitrate = -1;
	codec->min_bitrate = -1;
}
