#include <funcprotos.h>
#include <quicktime/quicktime.h>

#include <string.h> // memcpy

// Update during close:
// length
// samples per chunk
#define JUNK_SIZE 0x1018



quicktime_strl_t* quicktime_new_strl()
{
	quicktime_strl_t *strl = calloc(1, sizeof(quicktime_strl_t));
	return strl;
}


void quicktime_init_strl(quicktime_t *file, 
	quicktime_audio_map_t *atrack,
	quicktime_video_map_t *vtrack,
	quicktime_trak_t *trak,
	quicktime_strl_t *strl)
{
	quicktime_atom_t list_atom, strh_atom, strf_atom;
	quicktime_atom_t junk_atom;
	int i;
        trak->strl = strl;
/* Construct tag */
	if(vtrack)
	{
		strl->tag[0] = '0' + (trak->tkhd.track_id - 1) / 10;
		strl->tag[1] = '0' + (trak->tkhd.track_id - 1) % 10;
		strl->tag[2] = 'd';
		strl->tag[3] = 'c';
	}
	else
	if(atrack)
	{
		strl->tag[0] = '0' + (trak->tkhd.track_id - 1) / 10;
		strl->tag[1] = '0' + (trak->tkhd.track_id - 1) % 10;
		strl->tag[2] = 'w';
		strl->tag[3] = 'b';
	}


/* LIST 'strl' */
	quicktime_atom_write_header(file, &list_atom, "LIST");
	quicktime_write_char32(file, "strl");

/* 'strh' */
	quicktime_atom_write_header(file, &strh_atom, "strh");



/* vids */
	if(vtrack)
	{
        quicktime_write_char32(file, "vids");                                         // fccType
        quicktime_write_char32(file, 
                               trak->mdia.minf.stbl.stsd.table[0].format);            // fccHandler
/* flags */
        quicktime_write_int32_le(file, 0);                                            // dwFlags
/* priority */
        quicktime_write_int16_le(file, 0);                                            // dwReserved1
/* language */
        quicktime_write_int16_le(file, 0);                                            // dwReserved1 (32 bit)
/* initial frame */
        quicktime_write_int32_le(file, 0);                                            // dwInitialFrames

/* framerate denominator */
        quicktime_write_int32_le(file, 
                                 trak->mdia.minf.stbl.stts.table[0].sample_duration); // dwScale
/* framerate numerator */
        quicktime_write_int32_le(file, 
                                 trak->mdia.mdhd.time_scale);                         // dwRate

/* start */
        quicktime_write_int32_le(file, 0);                                            // dwStart
        strl->dwLengthOffset = quicktime_position(file);
/* length: fill later */
        quicktime_write_int32_le(file, 0);                                            // dwLength
/* suggested buffer size */
        quicktime_write_int32_le(file, 1024 * 1024);                                  // dwSuggestedBufferSize
/* quality */
        quicktime_write_int32_le(file, -1);                                           // dwQuality
/* sample size */
        quicktime_write_int32_le(file, (int)(trak->tkhd.track_width * trak->tkhd.track_height) * 3); // dwSampleSize
        quicktime_write_int16_le(file, 0);                                            // ??
        quicktime_write_int16_le(file, 0);                                            // ??
        quicktime_write_int16_le(file, trak->tkhd.track_width);                       // ??
        quicktime_write_int16_le(file, trak->tkhd.track_height);                      // ??
	}
	else
/* auds */
	if(atrack)
	{
        quicktime_write_char32(file, "auds"); // fccType
        quicktime_write_int32_le(file, 0);    // fccHandler
/* flags */
        quicktime_write_int32_le(file, 0);    // dwFlags
/* priority */
        quicktime_write_int16_le(file, 0);    // dwReserved1
/* language */
        quicktime_write_int16_le(file, 0);    // dwReserved1 (32 bit)
/* initial frame */
        quicktime_write_int32_le(file, 0);    // dwInitialFrames
	strl->dwScaleOffset = quicktime_position(file);
/* samples per chunk */
        quicktime_write_int32_le(file, 0);    // dwScale
/* sample rate * samples per chunk  if uncompressed */
/* sample rate if compressed */
        quicktime_write_int32_le(file, 0);    // dwRate
/* start */
        quicktime_write_int32_le(file, 0);    // dwStart
		strl->dwLengthOffset = quicktime_position(file);
/* length, XXX: filled later */
        quicktime_write_int32_le(file, 0);    // dwLength
/* suggested buffer size */
        quicktime_write_int32_le(file, 0);    // dwSuggestedBufferSize
/* quality */
        quicktime_write_int32_le(file, -1);   // dwQuality
/* sample size: 0 for compressed and number of bytes for uncompressed */
		strl->dwSampleSizeOffset = quicktime_position(file);
        quicktime_write_int32_le(file, 0);    // dwSampleSize
        quicktime_write_int32_le(file, 0);    // ??
        quicktime_write_int32_le(file, 0);    // ??
	}
	quicktime_atom_write_footer(file, &strh_atom);







/* strf */
	quicktime_atom_write_header(file, &strf_atom, "strf");

	if(vtrack)
	{
/* atom size repeated */
                quicktime_write_int32_le(file, 40); //                            biSize
		quicktime_write_int32_le(file, trak->tkhd.track_width); //        biWidth
		quicktime_write_int32_le(file, trak->tkhd.track_height); //       biHeight
/* planes */
		quicktime_write_int16_le(file, 1); //                             biPlanes
/* depth */
		quicktime_write_int16_le(file, 24); //                            biBitCount
		quicktime_write_char32(file, 
			trak->mdia.minf.stbl.stsd.table[0].format); //            biCompression
		quicktime_write_int32_le(file, 
			trak->tkhd.track_width * trak->tkhd.track_height * 3); // biSizeImage
		quicktime_write_int32_le(file, 0); //                             biXPelsPerMeter
		quicktime_write_int32_le(file, 0); //                             biYPelsPerMeter
		quicktime_write_int32_le(file, 0); //                             biClrUsed
		quicktime_write_int32_le(file, 0); //                             biClrImportant
	}
	else
	if(atrack)
	{
/* By now the codec is instantiated so the WAV ID is available. */
		int wav_id = 0x0;
		quicktime_codec_t *codec_base = atrack->codec;

		if(codec_base->wav_id)
			wav_id = codec_base->wav_id;
		quicktime_write_int16_le(file, //                                 wFormatTag
			wav_id);
		quicktime_write_int16_le(file, 
                        trak->mdia.minf.stbl.stsd.table[0].channels); //          nChannels
		quicktime_write_int32_le(file, 
			trak->mdia.minf.stbl.stsd.table[0].sample_rate); //       nSamplesPerSec
                strl->nAvgBytesPerSecOffset = quicktime_position(file);
                /* bitrate in bytes */
		quicktime_write_int32_le(file, 0); //                             nAvgBytesPerSec
/* block align */
		quicktime_write_int16_le(file, 0); //                             nBlockAlign
/* bits per sample */
		quicktime_write_int16_le(file, 0); //                             wBitsPerSample;
		quicktime_write_int16_le(file, 0); //                             cbSize
	}

	quicktime_atom_write_footer(file, &strf_atom);




/* Junk is required in Windows. */
/* In Heroine Kernel it's padding for the super index */
	strl->indx_offset = quicktime_position(file);
	strl->padding_size = JUNK_SIZE;



	quicktime_atom_write_header(file, &junk_atom, "JUNK");
	for(i = 0; i < JUNK_SIZE; i += 4)
		quicktime_write_int32_le(file, 0);
	quicktime_atom_write_footer(file, &junk_atom);


/* Initialize super index */
	quicktime_init_indx(file, &strl->indx, strl);


	quicktime_atom_write_footer(file, &list_atom);
}



void quicktime_delete_strl(quicktime_strl_t *strl)
{
	quicktime_delete_indx(&strl->indx);
	free(strl);
}

void quicktime_read_strl(quicktime_t *file,
	quicktime_strl_t *strl, 
	quicktime_atom_t *parent_atom)
{
// These are 0 if no track is currently being processed.
// Set to 1 if audio or video track is being processed.
	char data[4], codec[4];
        //	int denominator;
        //	int numerator;
        int frame_duration = 0;
        int timescale = 0;
        int width = 0;
	int height = 0;
	int depth = 0;
	int frames = 0;
	int bytes_per_sample = 0;
	int samples;
        //	int samples_per_chunk = 0;
	int channels = 0;
	int sample_rate = 0;
	int compression_id = 0;
        //	int bytes_per_second;
	quicktime_trak_t *trak = 0;
	quicktime_riff_t *first_riff = file->riff[0];


	codec[0] = codec[1] = codec[2] = codec[3] = 0;

/* AVI translation: */
/* vids -> trak */
/* auds -> trak */
/* Only one track is in each strl object */
	do
	{
		quicktime_atom_t leaf_atom;
		quicktime_atom_read_header(file, &leaf_atom);

// strh
		if(quicktime_atom_is(&leaf_atom, "strh"))
		{
// stream type
			quicktime_read_data(file, data, 4);

			if(quicktime_match_32(data, "vids"))
			{
				trak = quicktime_add_trak(file);
                                trak->strl = strl;
                                width = 0;
				height = 0;
				depth = 24;
				frames = 0;
				strl->is_video = 1;


				trak->tkhd.track_id = file->moov.mvhd.next_track_id;
				file->moov.mvhd.next_track_id++;


/* Codec */
				quicktime_read_data(file,       // fccHandler
					codec, 
					4);
//printf("quicktime_read_strl 1 %c%c%c%c\n", codec[0], codec[1], codec[2], codec[3]);
/* Blank */
                                // Skip dwFlags dwReserved1 dwInitialFrames
				quicktime_set_position(file, quicktime_position(file) + 12);
                                strl->dwScale = quicktime_read_int32_le(file); // dwScale (denominator)
				strl->dwRate = quicktime_read_int32_le(file);  // dwRate (numerator)
				if(strl->dwScale != 0)
                                {
                                        timescale = strl->dwRate;
                                        frame_duration = strl->dwScale;
                                        //  frame_rate = (double)strl->dwRate / strl->dwScale;
                                }
				else
                                {
                                // frame_rate = strl->dwRate;
                                        timescale = strl->dwRate;
                                        frame_duration = 1;
                                }
/* Blank */
                                // Skip dwStart
				quicktime_set_position(file, quicktime_position(file) + 4);
				frames = quicktime_read_int32_le(file);      // dwLength
			}
			else
			if(quicktime_match_32(data, "auds"))
			{
				trak = quicktime_add_trak(file);
				channels = 2;
				sample_rate = 0;
				compression_id = 0;
				strl->is_audio = 1;

				trak->tkhd.track_id = file->moov.mvhd.next_track_id;
				file->moov.mvhd.next_track_id++;
				quicktime_read_data(file,                   // fccHandler
					codec, 
					4);
//printf("quicktime_read_strl 2 %c%c%c%c\n", codec[0], codec[1], codec[2], codec[3]);
                                // Skip dwFlags dwReserved1 dwInitialFrames
				quicktime_set_position(file, quicktime_position(file) + 12);
				strl->dwScale = quicktime_read_int32_le(file); // dwScale (samples_per_chunk)
                                //                                fprintf(stderr, "*** Samples per chunk: %d\n", strl->dwScale);
				strl->dwRate = quicktime_read_int32_le(file);  // dwRate (bytes_per_second)
/* start */
                                // Skip dwStart
				quicktime_set_position(file, quicktime_position(file) + 4);
/* length of track */
				samples = quicktime_read_int32_le(file);           // dwLength
                                fprintf(stderr, "*** Time: %f\n", (float)samples/(float)strl->dwRate);
/* suggested buffer size, quality */
                                // Skip dwSuggestedBufferSize dwQuality
				quicktime_set_position(file, quicktime_position(file) + 8);

// If this is 0 use constant strl->dwScale to guess locations.
// If it isn't 0 synthesize samples per chunk table to get locations.
// McRowesoft doesn't really obey this rule so we may need to base it on codec ID.
				bytes_per_sample = quicktime_read_int32_le(file); // dwSampleSize
//printf("quicktime_read_strl 20 %d\n", strl->dwScale);
			}
		}
// strf
		else
		if(quicktime_atom_is(&leaf_atom, "strf"))
		{
			if(strl->is_video)
			{
/* atom size repeated */
                                quicktime_read_int32_le(file); //          biSize
				width = quicktime_read_int32_le(file); //  biWidth
				height = quicktime_read_int32_le(file); // biHeight
/* Panes */
				quicktime_read_int16_le(file); //          biPlanes
/* Depth in bits */
				depth = quicktime_read_int16_le(file); //  biBitCount
				quicktime_read_data(file,  //              biCompression
					codec, 
					4);
			}
			else
			if(strl->is_audio)
			{
                                compression_id = quicktime_read_int16_le(file); //             wFormatTag
				channels = quicktime_read_int16_le(file); //                   nChannels
				sample_rate = quicktime_read_int32_le(file); //                nSamplesPerSec
                                strl->nAvgBytesPerSec = quicktime_read_int32_le(file); //      nAvgBytesPerSec
                                strl->nBlockAlign = quicktime_read_int32_le(file); //          nBlockAlign
                                // quicktime_set_position(file, quicktime_position(file) + 6);
				strl->wBitsPerSample = quicktime_read_int16_le(file); //                wBitsPerSample
//printf("quicktime_read_strl 40 %d %d %d\n", channels, sample_rate, strl->wBitsPerSample);
			}
		}
		else
// Super index.
// Read the super index + all the partial indexes now
		if(quicktime_atom_is(&leaf_atom, "indx"))
		{
//printf("quicktime_read_strl 50\n");
			quicktime_read_indx(file, strl, &leaf_atom);
			strl->have_indx = 1;
		}

//printf("quicktime_read_strl 60\n");


// Next object
		quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < parent_atom->end);
//printf("quicktime_read_strl 70 %d %d\n", strl->is_audio, strl->is_video);


	if(strl->is_video)
	{
/* Generate quicktime structures */
		quicktime_trak_init_video(file, 
			trak, 
			width, 
			height, 
			frame_duration,
                        timescale,
			codec);
		quicktime_mhvd_init_video(file, 
			&file->moov.mvhd, 
                        timescale);
		trak->mdia.mdhd.duration = frames;
//			trak->mdia.mdhd.time_scale = 1;
		memcpy(trak->mdia.minf.stbl.stsd.table[0].format, codec, 4);
		trak->mdia.minf.stbl.stsd.table[0].depth = depth;
	}
	else
	if(strl->is_audio)
	{
/* Generate quicktime structures */
//printf("quicktime_read_strl 70 %d\n", strl->wBitsPerSample);
		quicktime_trak_init_audio(file, 
					trak, 
					channels, 
					sample_rate, 
					strl->wBitsPerSample, 
					codec);


// We store a constant samples per chunk based on the 
// packet size if sample_size zero
// and calculate the samples per chunk based on the chunk size if sample_size 
// is nonzero.
//		trak->mdia.minf.stbl.stsd.table[0].sample_size = bytes_per_sample;
		trak->mdia.minf.stbl.stsd.table[0].compression_id = compression_id;

/* Synthesize stsc table for constant samples per chunk */
		if(!bytes_per_sample)
		{
/* Should be enough entries allocated in quicktime_stsc_init_table */
			trak->mdia.minf.stbl.stsc.table[0].samples = strl->dwScale;
			trak->mdia.minf.stbl.stsc.total_entries = 1;
		}
#if 0
                else
                {
                fprintf(stderr, "Samples: %d, sample_rate: %d strl->dwRate: %d\n",
                        samples, sample_rate, strl->dwRate);
			trak->mdia.minf.stbl.stsc.table[0].samples =
                          (int)(((float)(samples)*(float)(sample_rate))/(float)strl->dwRate);
			trak->mdia.minf.stbl.stsc.total_entries = 1;
                }
#endif
	}


//printf("quicktime_read_strl 100\n");
}




