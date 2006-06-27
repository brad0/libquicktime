#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h> // memcpy

void quicktime_delete_indx(quicktime_indx_t *indx)
{
	int i;
	if(indx->table)
	{
		for(i = 0; i < indx->table_size; i++)
		{
			quicktime_indxtable_t *indx_table = &indx->table[i];
			if(indx_table->ix) quicktime_delete_ix(indx_table->ix);
		}
		free(indx->table);
	}
}

void quicktime_init_indx(quicktime_t *file, 
	quicktime_indx_t *indx, 
	quicktime_strl_t *strl)
{
	indx->longs_per_entry = 4;
	indx->index_subtype = 0;
	indx->index_type = AVI_INDEX_OF_INDEXES;
	memcpy(indx->chunk_id, strl->tag, 4);
}

void quicktime_indx_init_riff(quicktime_t *file, quicktime_trak_t * trak)
  {
  quicktime_strl_t * strl;
  quicktime_indx_t * indx;
  quicktime_indxtable_t *indx_table;
  
  strl = trak->strl;
  indx = &(strl->indx);

  if(indx->table_size >= indx->table_allocation)
    {
    int new_allocation = indx->table_allocation * 2;
    if(new_allocation < 1) new_allocation = 1;
    
    indx->table = realloc(indx->table, new_allocation * sizeof(quicktime_indxtable_t));
    memset(indx->table + indx->table_size, 0, new_allocation - indx->table_size);
    indx->table_allocation = new_allocation;
    }
  
  /* Append */
  indx_table = &indx->table[indx->table_size++];
  indx_table->ix = quicktime_new_ix(file, trak, strl);
  }

void quicktime_indx_finalize_riff(quicktime_t *file, quicktime_trak_t * trak)
  {
  quicktime_strl_t * strl;
  quicktime_indx_t * indx;
  quicktime_indxtable_t *indx_table;
  
  strl = trak->strl;
  indx = &(strl->indx);

  indx_table = &indx->table[indx->table_size-1];

  quicktime_write_ix(file, trak);
  
  indx_table->index_offset = indx_table->ix->atom.start - 8;
  indx_table->index_size   = indx_table->ix->atom.size;
  indx_table->duration     = indx_table->ix->table_size;
  }

/* This function has to be called EXCLUSIVELY by quicktime_finalize_strl,
   since the file position is to be expected behind the strf chunk */

void quicktime_finalize_indx(quicktime_t *file, quicktime_indx_t * indx)
  {
  int j;

  /* Write indx */
  //  quicktime_set_position(file, strl->indx_offset);
  quicktime_atom_write_header(file, &indx->atom, "indx");
  /* longs per entry */
  quicktime_write_int16_le(file, indx->longs_per_entry);
  /* index sub type */
  quicktime_write_char(file, indx->index_subtype);
  /* index type */
  quicktime_write_char(file, indx->index_type);
  /* entries in use */
  quicktime_write_int32_le(file, indx->table_size);
  /* chunk ID */
  quicktime_write_char32(file, indx->chunk_id);
  /* reserved */
  quicktime_write_int32_le(file, 0);
  quicktime_write_int32_le(file, 0);
  quicktime_write_int32_le(file, 0);
          
  /* table */
  for(j = 0; j < indx->table_size; j++)
    {
    quicktime_indxtable_t *indx_table = &indx->table[j];
    quicktime_write_int64_le(file, indx_table->index_offset);
    quicktime_write_int32_le(file, indx_table->index_size);
    quicktime_write_int32_le(file, indx_table->duration);
    }
          
  quicktime_atom_write_footer(file, &indx->atom);
  }

void quicktime_read_indx(quicktime_t *file, 
	quicktime_strl_t *strl, 
	quicktime_atom_t *parent_atom)
{
	quicktime_indx_t *indx = &strl->indx;
	quicktime_indxtable_t *indx_table;
	quicktime_ix_t *ix;
	int i;
	int64_t offset;

        file->file_type = LQT_FILE_AVI_ODML;
        
	indx->longs_per_entry = quicktime_read_int16_le(file);
	indx->index_subtype = quicktime_read_char(file);
	indx->index_type = quicktime_read_char(file);
	indx->table_size = quicktime_read_int32_le(file);
	quicktime_read_char32(file, indx->chunk_id);
	quicktime_read_int32_le(file);
	quicktime_read_int32_le(file);
	quicktime_read_int32_le(file);

//printf("quicktime_read_indx 1\n");
/* Read indx entries */
	indx->table = calloc(indx->table_size, sizeof(quicktime_indxtable_t));
	for(i = 0; i < indx->table_size; i++)
	{
		indx_table = &indx->table[i];
		indx_table->index_offset = quicktime_read_int64_le(file);
		indx_table->index_size = quicktime_read_int32_le(file);
		indx_table->duration = quicktime_read_int32_le(file);
		offset = quicktime_position(file);
                
/* Now read the partial index */
		ix = indx_table->ix = calloc(1, sizeof(quicktime_ix_t));
		quicktime_set_position(file, indx_table->index_offset);
		quicktime_read_ix(file, ix);
		quicktime_set_position(file, offset);
	}
//printf("quicktime_read_indx 100\n");

}

void quicktime_set_indx_keyframe(quicktime_t *file, 
                                 quicktime_trak_t *trak,
                                 long new_keyframe)
  {
  long frame_count;
  int i;
  quicktime_indx_t *indx = &trak->strl->indx;

  /* Get the right ix table */
  frame_count = 0;
  i = 0;

  //  fprintf(stderr, "quicktime_set_indx_keyframe %d\n", trak->tkhd.track_id);

  while(frame_count + indx->table[i].ix->table_size < new_keyframe)
    {
    frame_count+= indx->table[i].ix->table_size;
    i++;
    }
  indx->table[i].ix->table[new_keyframe - frame_count].size &= 0x7fffffff;
  }

void quicktime_indx_dump(quicktime_indx_t *indx)
  {
  int i;
  printf(" indx");
  printf(" longs_per_entry: %d\n", indx->longs_per_entry);
  printf(" index_subtype:   %d\n", indx->index_subtype);
  printf(" index_type:      %d\n", indx->index_type);
  printf(" chunk_id:        %s\n", indx->chunk_id);
  printf(" table_size:      %d\n", indx->table_size);

  for(i = 0; i < indx->table_size; i++)
    {
    printf("   index_offset: %lld\n", indx->table[i].index_offset);
    printf("   index_size:   %d\n", indx->table[i].index_size);
    printf("   duration:     %d\n", indx->table[i].duration);
    quicktime_ix_dump(indx->table[i].ix);
    }
  
  }

