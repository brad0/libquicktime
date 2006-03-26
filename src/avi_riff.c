#include <quicktime/quicktime.h>
#include <funcprotos.h>
#include <string.h>


void quicktime_read_riff(quicktime_t *file, quicktime_atom_t *parent_atom)
{
	quicktime_riff_t *riff = quicktime_new_riff(file);
	quicktime_atom_t leaf_atom;
	int result = 0;
	uint8_t data[5];

	riff->atom = *parent_atom;

// AVI
	quicktime_read_data(file, data, 4);
//printf("quicktime_read_riff 1 %llx\n", quicktime_position(file));

// Certain AVI parameters must be copied over to quicktime objects:
// hdrl -> moov
// movi -> mdat
// idx1 -> moov
	do
	{
		result = quicktime_atom_read_header(file, &leaf_atom);

/*
 * printf("quicktime_read_riff 1 %llx %llx %c%c%c%c\n", 
 * leaf_atom.start,
 * leaf_atom.size,
 * leaf_atom.type[0], 
 * leaf_atom.type[1], 
 * leaf_atom.type[2], 
 * leaf_atom.type[3]);
 */
		if(!result)
		{
			if(quicktime_atom_is(&leaf_atom, "LIST"))
			{
				data[4] = 0;


				result = !quicktime_read_data(file, data, 4);


				if(!result)
				{
// Got LIST 'hdrl'
					if(quicktime_match_32(data, "hdrl"))
					{

// No size here.
//printf("quicktime_read_riff 10 %llx\n", quicktime_position(file));
						quicktime_read_hdrl(file, &riff->hdrl, &leaf_atom);
//printf("quicktime_read_riff 20 %llx\n", quicktime_position(file));
					}
					else
// Got LIST 'movi'
					if(quicktime_match_32(data, "movi"))
					{
//printf("quicktime_read_riff 30 %llx\n", quicktime_position(file));
						quicktime_read_movi(file, &leaf_atom, &riff->movi);
//printf("quicktime_read_riff 40 %llx\n", quicktime_position(file));
					}
				}

// Skip it
				quicktime_atom_skip(file, &leaf_atom);

			}
			else
// Got 'movi'
			if(quicktime_atom_is(&leaf_atom, "movi"))
			{
				quicktime_read_movi(file, &leaf_atom, &riff->movi);

			}
			else
// Got 'idx1' original index
			if(quicktime_atom_is(&leaf_atom, "idx1"))
			{

//printf("quicktime_read_riff 50 %llx\n", quicktime_position(file));
// Preload idx1 here
				int64_t start_position = quicktime_position(file);
				long temp_size = leaf_atom.end - start_position;
				unsigned char *temp = malloc(temp_size);
				quicktime_set_preload(file, 
					(temp_size < 0x100000) ? 0x100000 : temp_size);
				quicktime_read_data(file, temp, temp_size);
				quicktime_set_position(file, start_position);
				free(temp);

// Read idx1
				quicktime_read_idx1(file, riff, &leaf_atom);
//printf("quicktime_read_riff 60 %llx\n", quicktime_position(file));

			}
			else
/* Skip it */
			{

				quicktime_atom_skip(file, &leaf_atom);

			}
		}
	}while(!result && quicktime_position(file) < parent_atom->end);

//printf("quicktime_read_riff 10\n");


}


quicktime_riff_t* quicktime_new_riff(quicktime_t *file)
{
	if(file->total_riffs >= MAX_RIFFS)
	{
		fprintf(stderr, "quicktime_new_riff file->total_riffs >= MAX_RIFFS\n");
		return 0;
	}
	else
	{
		quicktime_riff_t *riff = calloc(1, sizeof(quicktime_riff_t));
		file->riff[file->total_riffs++] = riff;
		return riff;
	}
}



void quicktime_delete_riff(quicktime_t *file, quicktime_riff_t *riff)
{
	quicktime_delete_hdrl(file, &riff->hdrl);
	quicktime_delete_movi(file, &riff->movi);
	quicktime_delete_idx1(&riff->idx1);
	free(riff);
}

void quicktime_init_riff(quicktime_t *file)
{

// Create new RIFF
	quicktime_riff_t *riff = quicktime_new_riff(file);

// Write riff header
// RIFF 'AVI '
	quicktime_atom_write_header(file, &riff->atom, "RIFF");
	quicktime_write_char32(file, "AVI ");

// Write header list in first RIFF only
	if(file->total_riffs < 2)
	{
		quicktime_init_hdrl(file, &riff->hdrl);
		riff->have_hdrl = 1;
	}

	quicktime_init_movi(file, riff);
}

void quicktime_finalize_riff(quicktime_t *file, quicktime_riff_t *riff)
{
// Write partial indexes
	quicktime_finalize_movi(file, &riff->movi);
	if(riff->have_hdrl)
	{
        printf("quicktime_finalize_riff 1\n");
		quicktime_finalize_hdrl(file, &riff->hdrl);
                printf("quicktime_finalize_riff 10\n");
// Write original index for first RIFF
		quicktime_write_idx1(file, &riff->idx1);
                printf("quicktime_finalize_riff 100\n");
	}
	quicktime_atom_write_footer(file, &riff->atom);
}


/*
 *  Get the number of audio samples corresponding to a number of bytes
 *  in an audio stream.
 *
 *  Update all arguments for which pointers are passed
 */

static int bytes_to_samples(quicktime_strl_t * strl, int bytes, int samplerate)
  {
  int64_t total_samples;
  int ret;

  strl->total_bytes += bytes;
  if(strl->nBlockAlign)
    {
    strl->total_blocks = (strl->total_bytes + strl->nBlockAlign - 1) / strl->nBlockAlign;
    }
  else
    strl->total_blocks++;
  
  if((strl->dwSampleSize == 0) && (strl->dwScale > 1))
    {
    /* variable bitrate */
    total_samples = (samplerate * strl->total_blocks *
                     strl->dwScale) / strl->dwRate;
    }
  else
    {
    /* constant bitrate */
    if(strl->nBlockAlign)
      {
      total_samples =
        (strl->total_bytes * strl->dwScale * samplerate) /
        (strl->nBlockAlign * strl->dwRate);
      }
    else
      {
      total_samples =
        (samplerate * strl->total_bytes *
         strl->dwScale) / (strl->dwSampleSize * strl->dwRate);
      }
    }

  /* Update stuff */

  ret = total_samples - strl->total_samples;
  strl->total_samples = total_samples;

  return ret;
  }


/* Insert audio chunk from idx1 into quicktime indices */

#define NUM_ALLOC 1024

static void insert_audio_packet(quicktime_trak_t * trak,
                                int64_t offset,
                                int size)
  {
  int num_samples;
  quicktime_stco_t *stco;
  quicktime_stsc_t *stsc;
  quicktime_stts_t *stts = &trak->mdia.minf.stbl.stts;
  
  /* Update stco */

  stco = &trak->mdia.minf.stbl.stco;
  quicktime_update_stco(stco, stco->total_entries + 1, offset);

  /* Update stsc */
  
  num_samples = bytes_to_samples(trak->strl, size, trak->mdia.minf.stbl.stsd.table[0].samplerate);
  
  stsc = &trak->mdia.minf.stbl.stsc;

  /* stsc will be initialized with 1 entry and zero samples */

  if(stsc->table[0].samples == 0)
    quicktime_update_stsc(stsc, 1, num_samples);
  else
    quicktime_update_stsc(stsc, stsc->total_entries+1, num_samples);

  /* Update total samples */

  stts->table[0].sample_count += num_samples;

  if(trak->chunk_sizes_alloc < stco->total_entries)
    {
    trak->chunk_sizes = realloc(trak->chunk_sizes,
                                sizeof(*trak->chunk_sizes) *
                                (trak->chunk_sizes_alloc + NUM_ALLOC));
    memset(trak->chunk_sizes + trak->chunk_sizes_alloc, 0,
           sizeof(*trak->chunk_sizes) * NUM_ALLOC);
    trak->chunk_sizes_alloc += NUM_ALLOC;
    }
  trak->chunk_sizes[stco->total_entries - 1] = size;
  }

#undef NUM_ALLOC
     
/* Insert video chunk from idx1 into quicktime indices */

static void insert_video_packet(quicktime_trak_t * trak,
                                int64_t offset,
                                int size, int keyframe)
  {
  quicktime_stss_t *stss;
  quicktime_stco_t *stco = &trak->mdia.minf.stbl.stco;
  quicktime_stsz_t *stsz = &trak->mdia.minf.stbl.stsz;
  quicktime_stts_t *stts = &trak->mdia.minf.stbl.stts;

  /* If size is zero, the last frame will be repeated */

  if(!size)
    {
    stts->table[stts->total_entries-1].sample_duration += stts->default_duration;
    return;
    }
  
  /* Update stco */
  
  quicktime_update_stco(stco, stco->total_entries + 1, offset);

  /* Update stss */

  if(keyframe)
    {
    stss = &trak->mdia.minf.stbl.stss;
    if(stss->entries_allocated <= stss->total_entries)
      {
      stss->entries_allocated *= 2;
      stss->table = realloc(stss->table, 
                            sizeof(quicktime_stss_table_t) * stss->entries_allocated);
      }
    stss->table[stss->total_entries++].sample = stsz->total_entries+1;
    }

  /* Update sample duration */

  quicktime_update_stts(stts, stsz->total_entries, 0);
  
  /* Update sample size */

  quicktime_update_stsz(stsz, stsz->total_entries, size);
  }

/* Build index tables from an idx1 index */

static void idx1_build_index(quicktime_t *file)
  {
  int i;
  quicktime_idx1table_t *idx1table;
  quicktime_trak_t * trak;
  quicktime_riff_t *first_riff = file->riff[0];
  quicktime_idx1_t *idx1 = &first_riff->idx1;
  int track_number;
    
  for(i = 0; i < idx1->table_size; i++)
    {
    idx1table = idx1->table + i;
    track_number = (idx1table->tag[0] - '0') * 10 +
      (idx1table->tag[1] - '0');
    if((track_number < 0) || (track_number >= file->moov.total_tracks))
      continue;
    trak = file->moov.trak[track_number];

    if(trak->mdia.minf.is_audio)
      insert_audio_packet(trak,
                          idx1table->offset + first_riff->movi.atom.start,
                          idx1table->size);

    else if(trak->mdia.minf.is_video)
      insert_video_packet(trak,
                          idx1table->offset + first_riff->movi.atom.start,
                          idx1table->size,
                          !!(idx1table->flags & AVI_KEYFRAME));
    }
  }

/* Build index tables from an indx index */
#if 0
static void indx_build_index(quicktime_t *file)
  {
  
  }
#endif
/*
 *  Ok, this differs from the qt4l approach:
 *  If indx tables are present for all streams,
 *  we'll use them and forget the idx1. Otherwise,
 *  we build the index from the idx1
 */

int quicktime_import_avi(quicktime_t *file)
  {
#if 1
  int i;

  quicktime_riff_t *first_riff = file->riff[0];
  quicktime_idx1_t *idx1 = &first_riff->idx1;
  if(!idx1->table_size)
    return 1;
  idx1_build_index(file);

  /* Compress stts */
  for(i = 0; i < file->moov.total_tracks; i++)
    {
    if(file->moov.trak[i]->mdia.minf.is_video)
       quicktime_compress_stts(&(file->moov.trak[i]->mdia.minf.stbl.stts));
    }
  
  return 0;
  
#else /* Original version follows */
	int i, j, k;
	quicktime_riff_t *first_riff = file->riff[0];
	quicktime_idx1_t *idx1 = &first_riff->idx1;
	quicktime_hdrl_t *hdrl = &first_riff->hdrl;



/* Determine whether to use idx1 or indx indexes for offsets. */
/* idx1 must always be used for keyframes but it also must be */
/* ignored for offsets if indx exists. */


//printf("quicktime_import_avi 1\n");
/* Convert idx1 to keyframes and load offsets and sizes */
	for(i = 0; i < idx1->table_size; i++)
	{
		quicktime_idx1table_t *idx1table = idx1->table + i;
		char *tag = idx1table->tag;
		int track_number = (tag[0] - '0') * 10 + (tag[1] - '0');
		if(track_number < file->moov.total_tracks)
		{
			quicktime_trak_t *trak = file->moov.trak[track_number];
			int is_audio = trak->mdia.minf.is_audio;
			int is_video = trak->mdia.minf.is_video;


/* Chunk offset */
			quicktime_stco_t *stco = &trak->mdia.minf.stbl.stco;
/* Sample size */
			quicktime_stsz_t *stsz = &trak->mdia.minf.stbl.stsz;
/* Samples per chunk */
			quicktime_stsc_t *stsc = &trak->mdia.minf.stbl.stsc;
/* Sample description */
			quicktime_stsd_t *stsd = &trak->mdia.minf.stbl.stsd;


/* Enter the offset and size no matter what so the sample counts */
/* can be used to set keyframes */
			quicktime_update_stco(stco, 
					stco->total_entries + 1, 
					idx1table->offset + first_riff->movi.atom.start);

			if(is_video)
			{
/* Just get the keyframe flag.  don't call quicktime_insert_keyframe because */
/* that updates idx1 and we don't have a track map. */
				int is_keyframe = (idx1table->flags & AVI_KEYFRAME) == AVI_KEYFRAME;
				if(is_keyframe)
				{
					quicktime_stss_t *stss = &trak->mdia.minf.stbl.stss;
/* This is done before the image size table so this value is right */
					int frame = stsz->total_entries;

/* Expand table */
					if(stss->entries_allocated <= stss->total_entries)
					{
						stss->entries_allocated *= 2;
						stss->table = realloc(stss->table, 
							sizeof(quicktime_stss_table_t) * stss->entries_allocated);
					}
					stss->table[stss->total_entries++].sample = frame;
				}

/* Set image size */
				quicktime_update_stsz(stsz, 
							stsz->total_entries, 
							idx1table->size);
			}
			else
			if(is_audio)
			{
/* Set samples per chunk if PCM */
				if(stsd->table[0].sample_size > 0)
				{
					quicktime_update_stsc(stsc, 
						stsc->total_entries + 1, 
						idx1table->size * 
							8 / 
							stsd->table[0].sample_size / 
							stsd->table[0].channels);
				}
			}
		}
	}

//printf("quicktime_import_avi 10\n");


/* Convert super indexes into Quicktime indexes. */
/* Append to existing entries if idx1 exists. */
/* No keyframes here. */
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_strl_t *strl = first_riff->hdrl.strl[i];

		if(strl->have_indx)
		{
			quicktime_indx_t *indx = &strl->indx;
			quicktime_trak_t *trak = file->moov.trak[i];
			quicktime_stco_t *stco = &trak->mdia.minf.stbl.stco;
			quicktime_stsz_t *stsz = &trak->mdia.minf.stbl.stsz;
			quicktime_stsc_t *stsc = &trak->mdia.minf.stbl.stsc;
			quicktime_stsd_t *stsd = &trak->mdia.minf.stbl.stsd;
/* Get existing chunk count from the idx1 */
			int existing_chunks = stco->total_entries;

/* Read each indx entry */
			for(j = 0; j < indx->table_size; j++)
			{
				quicktime_indxtable_t *indx_table = &indx->table[j];
				quicktime_ix_t *ix = indx_table->ix;

				for(k = 0; k < ix->table_size; k++)
				{
/* Copy from existing chunk count to end of ix table */
					if(existing_chunks <= 0)
					{
						quicktime_ixtable_t *ixtable = &ix->table[k];

/* Do the same things that idx1 did to the chunk tables */
/* Subtract the super indexes by size of the header.  McRoweSoft seems to */
/* want the header before the ix offset but after the idx1 offset. */
						quicktime_update_stco(stco, 
							stco->total_entries + 1, 
							ixtable->relative_offset + ix->base_offset - 8);
						if(strl->is_video)
						{
							quicktime_update_stsz(stsz, 
								stsz->total_entries, 
								ixtable->size);
						}
						else
						if(strl->is_audio)
						{
							if(stsd->table[0].sample_size > 0)
							{
                                                                  quicktime_update_stsc(stsc, 
									stsc->total_entries + 1, 
									ixtable->size * 
									8 / 
									stsd->table[0].sample_size / 
									stsd->table[0].channels);
							}
						}
					}
					else
						existing_chunks--;
				}
			}
		}
	}


//printf("quicktime_import_avi 20\n");




/* Set total samples, time to sample, for audio */
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_trak_t *trak = file->moov.trak[i];
		quicktime_stsz_t *stsz = &trak->mdia.minf.stbl.stsz;
		quicktime_stsc_t *stsc = &trak->mdia.minf.stbl.stsc;
		quicktime_stco_t *stco = &trak->mdia.minf.stbl.stco;
		quicktime_stts_t *stts = &trak->mdia.minf.stbl.stts;

		if(trak->mdia.minf.is_audio)
		{
 			quicktime_stsc_table_t *stsc_table = stsc->table;
			long total_entries = stsc->total_entries;
			long chunk = stco->total_entries;
			long sample = 0;

			if(chunk > 0 && total_entries)
			{
				sample = quicktime_sample_of_chunk(trak, chunk) + 
					stsc_table[total_entries - 1].samples;
			}
			stsz->sample_size = 1;
			stsz->total_entries = sample;
			stts->table[0].sample_count = sample;
		}
		else
		if(trak->mdia.minf.is_video)
		{
			stsc->total_entries = 1;
/* stts has 1 allocation by default */
			stts->table[0].sample_count = stco->total_entries;
		}
	}

//printf("quicktime_import_avi 30\n");
#endif

}









