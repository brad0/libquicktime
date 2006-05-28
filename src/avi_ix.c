#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>



static char* make_tag(int number, char *tag)
{
	tag[0] = 'i';
	tag[1] = 'x';
	tag[2] = '0' + (number / 10);
	tag[3] = '0' + (number % 10);
	return tag;
}

quicktime_ix_t* quicktime_new_ix(quicktime_t *file, 
	quicktime_trak_t *trak,
	quicktime_strl_t *strl)
{
	quicktime_ix_t *ix = calloc(1, sizeof(quicktime_ix_t));
	ix->base_offset = quicktime_position(file);
	make_tag(trak->tkhd.track_id - 1, ix->tag);
	ix->longs_per_entry = 2;
	ix->index_type = AVI_INDEX_OF_CHUNKS;
	memcpy(ix->chunk_id, strl->tag, 4);
	return ix;
}


void quicktime_delete_ix(quicktime_ix_t *ix)
{
	if(ix->table) free(ix->table);
	free(ix);
}

void quicktime_update_ixtable(quicktime_t *file, 
	quicktime_trak_t *trak, 
	int64_t offset,
	int size)
{
	quicktime_riff_t *first_riff = file->riff[0];
        quicktime_indx_t *indx = &(first_riff->hdrl.strl[trak->tkhd.track_id - 1]->indx);
        
        quicktime_ix_t *ix = indx->table[indx->table_size-1].ix;
        quicktime_ixtable_t *ix_table;

/* Allocation */
	if(ix->table_size >= ix->table_allocation)
	{
        int new_allocation = ix->table_allocation * 2;
        if(new_allocation < 1) new_allocation = 1;
        ix->table = realloc(ix->table, sizeof(quicktime_ixtable_t) * new_allocation);
        memset(ix->table + ix->table_size, 0,
               sizeof(quicktime_ixtable_t)*(new_allocation - ix->table_size));
        ix->table_allocation = new_allocation;
	}
        
/* Appendage */
	ix_table = &ix->table[ix->table_size++];
	ix_table->relative_offset = offset - ix->base_offset;
        ix_table->size = size;

        if(!trak->mdia.minf.is_audio &&
           trak->mdia.minf.stbl.stss.total_entries)
          {
          /* If bit 31 of the size is set, we have *no* keyframe */
          ix_table->size |= 0x80000000;
          }
}


void quicktime_write_ix(quicktime_t *file,
                        quicktime_trak_t * trak)
  {
	int i;
        quicktime_strl_t *strl;
        quicktime_indx_t *indx;
        quicktime_ix_t   *ix;

        strl = trak->strl;
        indx = &(strl->indx);
        ix = indx->table[indx->table_size-1].ix;
        
        quicktime_atom_write_header(file, &ix->atom, ix->tag);

/* longs per entry */
	quicktime_write_int16_le(file, ix->longs_per_entry);
/* index sub type */
	quicktime_write_char(file, 0);
/* index type */
	quicktime_write_char(file, ix->index_type);
/* entries in use */
	quicktime_write_int32_le(file, ix->table_size);
/* chunk ID */
	quicktime_write_char32(file, ix->chunk_id);
/* base offset */
	quicktime_write_int64_le(file, ix->base_offset);
/* reserved */
	quicktime_write_int32_le(file, 0);

/* table */
	for(i = 0; i < ix->table_size; i++)
	{
		quicktime_ixtable_t *table = &ix->table[i];
		quicktime_write_int32_le(file, table->relative_offset);
		quicktime_write_int32_le(file, table->size);
	}

	quicktime_atom_write_footer(file, &ix->atom);


}

void quicktime_read_ix(quicktime_t *file,
	quicktime_ix_t *ix)
{
	int i;
	quicktime_atom_t leaf_atom;
	quicktime_atom_read_header(file, &leaf_atom);

	ix->longs_per_entry = quicktime_read_int16_le(file);
/* sub type */
	quicktime_read_char(file);
	ix->index_type = quicktime_read_char(file);
	ix->table_size = quicktime_read_int32_le(file);
	quicktime_read_char32(file, ix->chunk_id);
	ix->base_offset = quicktime_read_int64_le(file);
/* reserved */
	quicktime_read_int32_le(file);

	ix->table = calloc(ix->table_size, sizeof(quicktime_ixtable_t));

	for(i = 0; i < ix->table_size; i++)
	{
		quicktime_ixtable_t *ixtable = &ix->table[i];
		ixtable->relative_offset = quicktime_read_int32_le(file);
		ixtable->size = quicktime_read_int32_le(file);
	}
}
