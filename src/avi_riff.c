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
          else if(quicktime_match_32(data, "movi"))  // Got LIST 'movi'
            {
            //printf("quicktime_read_riff 30 %llx\n", quicktime_position(file));
            quicktime_read_movi(file, &leaf_atom, &riff->movi);
            //printf("quicktime_read_riff 40 %llx\n", quicktime_position(file));
            }
          else if(quicktime_match_32(data, "INFO"))
            {
            quicktime_read_riffinfo(file, &riff->info, &leaf_atom);
            riff->have_info = 1;
            }
          else
            // Skip it
            quicktime_atom_skip(file, &leaf_atom);
          
          }
        }
      else
        // Got 'movi'
        if(quicktime_atom_is(&leaf_atom, "movi"))
          {
          quicktime_read_movi(file, &leaf_atom, &riff->movi);
          
          }
      // Got 'idx1' original index
        else if(quicktime_atom_is(&leaf_atom, "idx1"))
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
        else if(quicktime_atom_is(&leaf_atom, "INFO"))
          {
          quicktime_read_riffinfo(file, &riff->info, &leaf_atom);
          riff->have_info = 1;
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
	if(riff->have_hdrl) quicktime_delete_hdrl(file, &riff->hdrl);
	quicktime_delete_movi(file, &riff->movi);
	quicktime_delete_idx1(&riff->idx1);
	quicktime_delete_riffinfo(&riff->info);
	free(riff);
}

void quicktime_init_riff(quicktime_t *file)
  {
  int i;
  // Create new RIFF
  quicktime_riff_t *riff = quicktime_new_riff(file);
  
  // Write riff header
  // RIFF 'AVI '
  quicktime_atom_write_header(file, &riff->atom, "RIFF");
  
  // Write header list in first RIFF only
  if(file->total_riffs < 2)
    {
    quicktime_write_char32(file, "AVI ");
    quicktime_init_hdrl(file, &riff->hdrl);
    riff->have_hdrl = 1;
    quicktime_init_riffinfo(&riff->info);
    
    quicktime_udta_2_riffinfo(&file->moov.udta, &riff->info);
    quicktime_write_riffinfo(file, &riff->info);
    riff->have_info = 1;
    }
  else
    quicktime_write_char32(file, "AVIX");
    
  quicktime_init_movi(file, riff);
  
  if(file->file_type == LQT_FILE_AVI_ODML)
    {
    //    fprintf(stderr, "init_riff (ODML)\n");
    for(i = 0; i < file->moov.total_tracks; i++)
      {
      quicktime_indx_init_riff(file, file->moov.trak[i]);
      }
    }
  //  else
  //    fprintf(stderr, "init_riff (AVI 1.0)\n");
  }

void quicktime_finalize_riff(quicktime_t *file, quicktime_riff_t *riff)
  {
  int i;
  // Write partial indexes
  if(file->file_type == LQT_FILE_AVI_ODML)
    {
    for(i = 0; i < file->moov.total_tracks; i++)
      {
      quicktime_indx_finalize_riff(file, file->moov.trak[i]);
      }
    }

  quicktime_finalize_movi(file, &riff->movi);
  if(riff->have_hdrl)
    {
    //    printf("quicktime_finalize_riff 1\n");
    quicktime_finalize_hdrl(file, &riff->hdrl);
    //    printf("quicktime_finalize_riff 10\n");

    // Write original index for first RIFF
    quicktime_write_idx1(file, &riff->idx1);
    //    printf("quicktime_finalize_riff 100\n");
    }
  quicktime_atom_write_footer(file, &riff->atom);
  //  printf("quicktime_finalize_riff\n");
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

  int64_t base_offset;
  
  if(idx1->table[0].offset == 4)
    base_offset = 8 + first_riff->movi.atom.start;
  else
    /* For invalid files, which have the index relative to file start */
    base_offset = 8 + first_riff->movi.atom.start - (idx1->table[0].offset - 4);
  
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
                          idx1table->offset + base_offset,
                          idx1table->size);

    else if(trak->mdia.minf.is_video)
      insert_video_packet(trak,
                          idx1table->offset + base_offset,
                          idx1table->size,
                          !!(idx1table->flags & AVI_KEYFRAME));
    }
  }

/* Build index tables from an indx index */

static void indx_build_index(quicktime_t *file)
  {
  int i, j, k;
  quicktime_trak_t * trak;
  quicktime_indx_t  * indx;
  quicktime_ix_t *ix;

  for(i = 0; i < file->moov.total_tracks; i++)
    {
    trak = file->moov.trak[i];
    indx = &(trak->strl->indx);
    
    for(j = 0; j < indx->table_size; j++)
      {
      ix = indx->table[j].ix;

      for(k = 0; k < ix->table_size; k++)
        {
        if(trak->mdia.minf.is_audio)
          insert_audio_packet(trak,
                              ix->base_offset + ix->table[k].relative_offset,
                              ix->table[k].size);
        
        else if(trak->mdia.minf.is_video)
          insert_video_packet(trak,
                              ix->base_offset + ix->table[k].relative_offset,
                              ix->table[k].size & 0x7FFFFFFF,
                              !(ix->table[k].size & 0x80000000));
        
        }
      
      }
    
    }
  
  }
/*
 * Import index tables
 */

int quicktime_import_avi(quicktime_t *file)
  {
  int i;

  quicktime_riff_t *first_riff = file->riff[0];
  quicktime_idx1_t *idx1 = &first_riff->idx1;

  if(file->file_type == LQT_FILE_AVI)
    {
    if(!idx1->table_size)
      return 1;
    idx1_build_index(file);
    }
  else if(file->file_type == LQT_FILE_AVI_ODML)
    {
    indx_build_index(file);
    }
  /* Compress stts */
  for(i = 0; i < file->moov.total_tracks; i++)
    {
    if(file->moov.trak[i]->mdia.minf.is_video)
       quicktime_compress_stts(&(file->moov.trak[i]->mdia.minf.stbl.stts));
    }
  if(first_riff->have_info)
    {
    quicktime_riffinfo_2_udta(&first_riff->info, &(file->moov.udta));
    }
  //  quicktime_moov_dump(&file->moov);
  
  return 0;
  }









