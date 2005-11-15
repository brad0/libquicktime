#include <funcprotos.h>
#include <quicktime/quicktime.h>

void quicktime_stsd_init(quicktime_stsd_t *stsd)
{
	stsd->version = 0;
	stsd->flags = 0;
	stsd->total_entries = 0;
}

void quicktime_stsd_init_table(quicktime_stsd_t *stsd)
{
	if(!stsd->total_entries)
	{
		stsd->total_entries = 1;
		stsd->table = (quicktime_stsd_table_t*)calloc(1, sizeof(quicktime_stsd_table_t) * stsd->total_entries);
		quicktime_stsd_table_init(&(stsd->table[0]));
	}
}

void quicktime_stsd_init_qtvr(quicktime_t *file, 
								quicktime_stsd_t *stsd,
								int track_type, int width,
								int height)
{
	quicktime_stsd_table_t *table;
	quicktime_stsd_init_table(stsd);
	
	table = &(stsd->table[0]);
	if (track_type == QTVR_QTVR)
	{
	    table->format[0] = 'q';
	    table->format[1] = 't';
	    table->format[2] = 'v';
	    table->format[3] = 'r';
	} else
	if (track_type == QTVR_OBJ)
	{
	    table->format[0] = '\0';
	    table->format[1] = '\0';
	    table->format[2] = '\0';
	    table->format[3] = '\0';
	}

}


void quicktime_stsd_init_panorama(quicktime_t *file, 
								quicktime_stsd_t *stsd,
								int width,
								int height)
{
	quicktime_stsd_table_t *table;
	quicktime_stsd_init_table(stsd);
	
	table = &(stsd->table[0]);
	table->format[0] = 'p';
	table->format[1] = 'a';
	table->format[2] = 'n';
	table->format[3] = 'o';
	table->pano.SWidth = width;
	table->pano.SHeight = height;
}


void quicktime_stsd_init_video(quicktime_t *file, 
								quicktime_stsd_t *stsd, 
								int frame_w,
								int frame_h, 
                                                                char * compression)
{
	quicktime_stsd_table_t *table;
	quicktime_stsd_init_table(stsd);
//printf("quicktime_stsd_init_video 1\n");
	table = &(stsd->table[0]);
//printf("quicktime_stsd_init_video 1\n");

	quicktime_copy_char32(table->format, compression);
//printf("quicktime_stsd_init_video 1\n");
	table->width = frame_w;
//printf("quicktime_stsd_init_video 1\n");
	table->height = frame_h;
//printf("quicktime_stsd_init_video 1\n");
	table->frames_per_sample = 1;
//printf("quicktime_stsd_init_video 1\n");
	table->depth = 24;
//printf("quicktime_stsd_init_video 1\n");
	table->ctab_id = 65535;
//printf("quicktime_stsd_init_video 2\n");
}

void quicktime_stsd_init_audio(quicktime_t *file, 
							quicktime_stsd_t *stsd, 
							int channels,
							int sample_rate, 
							int bits, 
							char *compressor)
{
	quicktime_stsd_table_t *table;
	quicktime_stsd_init_table(stsd);
	table = &(stsd->table[0]);

	quicktime_copy_char32(table->format, compressor);
	quicktime_copy_char32(table->wave.frma.codec, compressor);
	table->channels = channels;
	table->sample_size = bits;
	table->sample_rate = sample_rate;
        //        fprintf(stderr, "stsd_init_audio: %d\n", bits);
}

void quicktime_stsd_delete(quicktime_stsd_t *stsd)
{
	int i;
	if(stsd->total_entries)
	{
		for(i = 0; i < stsd->total_entries; i++)
			quicktime_stsd_table_delete(&(stsd->table[i]));
		free(stsd->table);
	}

	stsd->total_entries = 0;
}

void quicktime_stsd_dump(void *minf_ptr, quicktime_stsd_t *stsd)
{
	int i;
	printf("     sample description (stsd)\n");
	printf("      version %d\n", stsd->version);
	printf("      flags %ld\n", stsd->flags);
	printf("      total_entries %ld\n", stsd->total_entries);
	
	for(i = 0; i < stsd->total_entries; i++)
	{
		quicktime_stsd_table_dump(minf_ptr, &(stsd->table[i]));
	}
}

void quicktime_read_stsd(quicktime_t *file, quicktime_stsd_t *stsd)
{
	int i;

	stsd->version = quicktime_read_char(file);
	stsd->flags = quicktime_read_int24(file);
	stsd->total_entries = quicktime_read_int32(file);
	stsd->table = calloc(stsd->total_entries, sizeof(quicktime_stsd_table_t));
	for(i = 0; i < stsd->total_entries; i++)
	{
		quicktime_read_stsd_table_raw(file, &(stsd->table[i]));
                fprintf(stderr, "Read table raw: %d bytes\n", stsd->table[i].table_raw_size);
	}
}

void quicktime_finalize_stsd(quicktime_t * file, quicktime_trak_t * trak, quicktime_stsd_t * stsd)
{

        int64_t old_preload_size;
        uint8_t *old_preload_buffer;
        int64_t old_preload_start;
        int64_t old_preload_end;
        int64_t old_preload_ptr;
        int64_t old_position;
        int i;

        /* Save old buffers from the file */
        old_preload_size = file->preload_size;
        old_preload_buffer = file->preload_buffer;
        old_preload_start = file->preload_start;
        old_preload_end = file->preload_end;
        old_preload_ptr = file->preload_ptr;
        old_position = quicktime_position(file);
        
	for(i = 0; i < stsd->total_entries; i++)
	{
		quicktime_stsd_table_init(&(stsd->table[i]));
                
                
                quicktime_set_position(file, 0);
#if 0
                fprintf(stderr, "reading final stsd table (%d bytes), file pos: %lld\n",
                        stsd->table[i].table_raw_size, quicktime_position(file));
                lqt_hexdump(stsd->table[i].table_raw, stsd->table[i].table_raw_size, 16);
#endif

                file->preload_size = stsd->table[i].table_raw_size;
                file->preload_buffer = stsd->table[i].table_raw;
                file->preload_start = 0;
                file->preload_end = file->preload_start + stsd->table[i].table_raw_size;
                file->preload_ptr = 0;
                
		quicktime_read_stsd_table(file, &(trak->mdia.minf), &(stsd->table[i]));
                if(trak->mdia.minf.is_video && !stsd->table[i].width && !stsd->table[i].height)
                  {
                  stsd->table[i].width =  (int)(trak->tkhd.track_width);
                  stsd->table[i].height = (int)(trak->tkhd.track_height);
                  }
	}
        file->preload_size = old_preload_size;
        file->preload_buffer = old_preload_buffer;
        file->preload_start = old_preload_start;
        file->preload_end = old_preload_end;
        file->preload_ptr = old_preload_ptr;
        quicktime_set_position(file, old_position);
}

void quicktime_write_stsd(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_t *stsd)
{
	quicktime_atom_t atom;
	int i;
	quicktime_atom_write_header(file, &atom, "stsd");

	quicktime_write_char(file, stsd->version);
	quicktime_write_int24(file, stsd->flags);
	quicktime_write_int32(file, stsd->total_entries);
	for(i = 0; i < stsd->total_entries; i++)
	{
		quicktime_write_stsd_table(file, minf, stsd->table);
	}

	quicktime_atom_write_footer(file, &atom);
}



