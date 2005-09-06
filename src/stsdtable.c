#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

void quicktime_mjqt_init(quicktime_mjqt_t *mjqt)
{
}

void quicktime_mjqt_delete(quicktime_mjqt_t *mjqt)
{
}

void quicktime_mjqt_dump(quicktime_mjqt_t *mjqt)
{
}


void quicktime_mjht_init(quicktime_mjht_t *mjht)
{
}

void quicktime_mjht_delete(quicktime_mjht_t *mjht)
{
}

void quicktime_mjht_dump(quicktime_mjht_t *mjht)
{
}


void quicktime_read_stsd_audio(quicktime_t *file, quicktime_stsd_table_t *table, quicktime_atom_t *parent_atom)
{
  quicktime_atom_t leaf_atom;
	table->version = quicktime_read_int16(file);
	table->revision = quicktime_read_int16(file);
	quicktime_read_data(file, (uint8_t*)(table->vendor), 4);
	table->channels = quicktime_read_int16(file);
	table->sample_size = quicktime_read_int16(file);
	table->compression_id = quicktime_read_int16(file);
	table->packet_size = quicktime_read_int16(file);
	table->sample_rate = quicktime_read_fixed32(file);
// Kluge for fixed32 limitation
        if(table->sample_rate + 65536 == 96000 ||
           table->sample_rate + 65536 == 88200) table->sample_rate += 65536;

        if(table->version == 1)
          {
          table->audio_samples_per_packet = quicktime_read_int32(file);
          table->audio_bytes_per_packet = quicktime_read_int32(file);
          table->audio_bytes_per_frame  = quicktime_read_int32(file);
          table->audio_bytes_per_sample  = quicktime_read_int32(file);
          }
        
/* Read additional atoms */
        while(quicktime_position(file) < parent_atom->end)
          {
          quicktime_atom_read_header(file, &leaf_atom);
          if(quicktime_atom_is(&leaf_atom, "wave"))
            {
            quicktime_read_wave(file, &(table->wave), &leaf_atom);
            table->has_wave = 1;
            }
          else
            {
            quicktime_atom_skip(file, &leaf_atom);
            }
          } 
}

void quicktime_write_stsd_audio(quicktime_t *file, quicktime_stsd_table_t *table)
{
	quicktime_write_int16(file, table->version);
	quicktime_write_int16(file, table->revision);
	quicktime_write_data(file, (uint8_t*)(table->vendor), 4);
	quicktime_write_int16(file, table->channels);
	quicktime_write_int16(file, table->sample_size);
	quicktime_write_int16(file, table->compression_id);
	quicktime_write_int16(file, table->packet_size);
	quicktime_write_fixed32(file, table->sample_rate);
}

/* LQT: This reads the extradata */

static void read_extradata(quicktime_t *file, quicktime_stsd_table_t *table, quicktime_atom_t *parent_atom)
  {
  int64_t old_position;

  old_position = quicktime_position(file);

  table->extradata_size = parent_atom->end - (parent_atom->start+4);
  table->extradata = malloc(parent_atom->size);
  quicktime_set_position(file, parent_atom->start+4);
  quicktime_read_data(file, table->extradata, table->extradata_size);
  quicktime_set_position(file, old_position);
  }

void quicktime_read_stsd_video(quicktime_t *file, quicktime_stsd_table_t *table, quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;
	int len;

        read_extradata(file, table, parent_atom);
        
	table->version = quicktime_read_int16(file);
	table->revision = quicktime_read_int16(file);
	quicktime_read_data(file, (uint8_t*)table->vendor, 4);
	table->temporal_quality = quicktime_read_int32(file);
	table->spatial_quality = quicktime_read_int32(file);
	table->width = quicktime_read_int16(file);
	table->height = quicktime_read_int16(file);
	table->dpi_horizontal = quicktime_read_fixed32(file);
	table->dpi_vertical = quicktime_read_fixed32(file);
	table->data_size = quicktime_read_int32(file);
	table->frames_per_sample = quicktime_read_int16(file);
	len = quicktime_read_char(file);
	quicktime_read_data(file, (uint8_t*)table->compressor_name, 31);
	table->depth = quicktime_read_int16(file);
	table->ctab_id = quicktime_read_int16(file);

        /*  If ctab_id is zero, the colortable follows immediately
         *  after the ctab ID
         */

        if(!table->ctab_id)
          quicktime_read_ctab(file, &(table->ctab));
        else
          quicktime_default_ctab(&(table->ctab), table->depth);
        
	while(quicktime_position(file) < parent_atom->end)
	{
		quicktime_atom_read_header(file, &leaf_atom);
//printf("quicktime_read_stsd_video 1 %llx %llx %llx\n", leaf_atom.start, leaf_atom.end, quicktime_position(file));
		
		if(quicktime_atom_is(&leaf_atom, "ctab"))
		{
			quicktime_read_ctab(file, &(table->ctab));
		}
		else
		if(quicktime_atom_is(&leaf_atom, "gama"))
		{
			table->gamma = quicktime_read_fixed32(file);
		}
		else
		if(quicktime_atom_is(&leaf_atom, "fiel"))
		{
			table->fields = quicktime_read_char(file);
			table->field_dominance = quicktime_read_char(file);
		}
		else
		if (quicktime_atom_is(&leaf_atom, "pasp"))
		{
			quicktime_read_pasp(file, &(table->pasp));
		}
		else
		if (quicktime_atom_is(&leaf_atom, "clap"))
		{
			quicktime_read_clap(file, &(table->clap));
		}
		else
		if (quicktime_atom_is(&leaf_atom, "colr"))
		{
			quicktime_read_colr(file, &(table->colr));
		}
		else
/* 		if(quicktime_atom_is(&leaf_atom, "mjqt")) */
/* 		{ */
/* 			quicktime_read_mjqt(file, &(table->mjqt)); */
/* 		} */
/* 		else */
/* 		if(quicktime_atom_is(&leaf_atom, "mjht")) */
/* 		{ */
/* 			quicktime_read_mjht(file, &(table->mjht)); */
/* 		} */
/* 		else */
		quicktime_atom_skip(file, &leaf_atom);
	}
//printf("quicktime_read_stsd_video 2\n");
}


void quicktime_write_stsd_video(quicktime_t *file, quicktime_stsd_table_t *table)
{
	quicktime_write_int16(file, table->version);
	quicktime_write_int16(file, table->revision);
	quicktime_write_data(file, (uint8_t*)table->vendor, 4);
	quicktime_write_int32(file, table->temporal_quality);
	quicktime_write_int32(file, table->spatial_quality);
	quicktime_write_int16(file, table->width);
	quicktime_write_int16(file, table->height);
	quicktime_write_fixed32(file, table->dpi_horizontal);
	quicktime_write_fixed32(file, table->dpi_vertical);
	quicktime_write_int32(file, table->data_size);
	quicktime_write_int16(file, table->frames_per_sample);
	quicktime_write_char(file, strlen(table->compressor_name));
	quicktime_write_data(file, (uint8_t*)table->compressor_name, 31);
	quicktime_write_int16(file, table->depth);
	quicktime_write_int16(file, table->ctab_id);

	if (table->pasp.hSpacing)
		quicktime_write_pasp(file, &(table->pasp));
	if (table->clap.cleanApertureWidthN)
		quicktime_write_clap(file, &(table->clap));
	if (table->colr.colorParamType)
		quicktime_write_colr(file, &(table->colr));

	if(table->fields)
	{
		quicktime_atom_t atom;

		quicktime_atom_write_header(file, &atom, "fiel");
		quicktime_write_char(file, table->fields);
		quicktime_write_char(file, table->field_dominance);
		quicktime_atom_write_footer(file, &atom);
	}
}

void quicktime_read_stsd_table(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_table_t *table)
{
	quicktime_atom_t leaf_atom;

	quicktime_atom_read_header(file, &leaf_atom);

        /* LQT: Initialize these */
        table->extradata = (unsigned char*)0;
        table->extradata_size = 0;
	table->format[0] = leaf_atom.type[0];
	table->format[1] = leaf_atom.type[1];
	table->format[2] = leaf_atom.type[2];
	table->format[3] = leaf_atom.type[3];

	quicktime_read_data(file, table->reserved, 6);
	table->data_reference = quicktime_read_int16(file);

	/* Panoramas are neither audio nor video */
	if (quicktime_match_32(leaf_atom.type, "pano"))
	{	    
	    minf->is_panorama = 1;
	    quicktime_read_pano(file, &(table->pano), &leaf_atom);
	}
	else if (quicktime_match_32(leaf_atom.type, "qtvr"))
	{	    
	    minf->is_qtvr = 1;
	    quicktime_read_qtvr(file, &(table->qtvr), &leaf_atom);
	}
	else if (quicktime_match_32(leaf_atom.type, "\0\0\0\0") && file->moov.udta.is_qtvr)
	{	    
	    minf->is_object = 1;
	}
	else 
	{
	    if(minf->is_audio) quicktime_read_stsd_audio(file, table, &leaf_atom);
	    if(minf->is_video) quicktime_read_stsd_video(file, table, &leaf_atom);
	}
}

void quicktime_stsd_table_init(quicktime_stsd_table_t *table)
{
	int i;
	table->format[0] = 'y';
	table->format[1] = 'u';
	table->format[2] = 'v';
	table->format[3] = '2';
	for(i = 0; i < 6; i++) table->reserved[i] = 0;
	table->data_reference = 1;

	table->version = 0;
	table->revision = 0;
 	table->vendor[0] = 'l';
 	table->vendor[1] = 'n';
 	table->vendor[2] = 'u';
 	table->vendor[3] = 'x';

	table->temporal_quality = 100;
	table->spatial_quality = 258;
	table->width = 0;
	table->height = 0;
	table->dpi_horizontal = 72;
	table->dpi_vertical = 72;
	table->data_size = 0;
	table->frames_per_sample = 1;
	for(i = 0; i < 32; i++) table->compressor_name[i] = 0;
	sprintf(table->compressor_name, "Quicktime for Linux");
	table->depth = 24;
	table->ctab_id = 65535;
	quicktime_ctab_init(&(table->ctab));
	table->gamma = 0;
	table->fields = 0;
	table->field_dominance = 1;
	quicktime_pasp_init(&(table->pasp));
	quicktime_clap_init(&(table->clap));
	quicktime_colr_init(&(table->colr));
	quicktime_mjqt_init(&(table->mjqt));
	quicktime_mjht_init(&(table->mjht));
	quicktime_pano_init(&(table->pano));
	quicktime_qtvr_init(&(table->qtvr));
	
	table->channels = 0;
	table->sample_size = 0;
	table->compression_id = 0;
	table->packet_size = 0;
	table->sample_rate = 0;
        table->extradata = NULL;
        table->extradata_size = 0;
}

void quicktime_stsd_table_delete(quicktime_stsd_table_t *table)
{
        /* LQT: Delete extradata as well */
        if(table->extradata)
          free(table->extradata);
        quicktime_ctab_delete(&(table->ctab));
	quicktime_mjqt_delete(&(table->mjqt));
	quicktime_mjht_delete(&(table->mjht));
	quicktime_wave_delete(&(table->wave));
}

void quicktime_stsd_video_dump(quicktime_stsd_table_t *table)
{
	printf("       version %d\n", table->version);
	printf("       revision %d\n", table->revision);
	printf("       vendor %c%c%c%c\n", table->vendor[0], table->vendor[1], table->vendor[2], table->vendor[3]);
	printf("       temporal_quality %ld\n", table->temporal_quality);
	printf("       spatial_quality %ld\n", table->spatial_quality);
	printf("       width %d\n", table->width);
	printf("       height %d\n", table->height);
	printf("       dpi_horizontal %f\n", table->dpi_horizontal);
	printf("       dpi_vertical %f\n", table->dpi_vertical);
	printf("       data_size %lld\n", table->data_size);
	printf("       frames_per_sample %d\n", table->frames_per_sample);
	printf("       compressor_name %s\n", table->compressor_name);
	printf("       depth %d\n", table->depth);
	printf("       ctab_id %d\n", table->ctab_id);
	printf("       gamma %f\n", table->gamma);

	if (table->pasp.hSpacing)
		quicktime_pasp_dump(&(table->pasp));
	if (table->clap.cleanApertureWidthN)
		quicktime_clap_dump(&(table->clap));
	if (table->colr.colorParamType)
		quicktime_colr_dump(&(table->colr));

	if(table->fields)
	{
		printf("     fields %d\n", table->fields);
		printf("     field dominance %d\n", table->field_dominance);
	}
	if(!table->ctab_id) quicktime_ctab_dump(&(table->ctab));
	quicktime_mjqt_dump(&(table->mjqt));
	quicktime_mjht_dump(&(table->mjht));
}

void quicktime_stsd_audio_dump(quicktime_stsd_table_t *table)
{
	printf("       version %d\n", table->version);
	printf("       revision %d\n", table->revision);
	printf("       vendor %c%c%c%c\n", table->vendor[0], table->vendor[1], table->vendor[2], table->vendor[3]);
	printf("       channels %d\n", table->channels);
	printf("       sample_size %d\n", table->sample_size);
	printf("       compression_id %d\n", table->compression_id);
	printf("       packet_size %d\n", table->packet_size);
	printf("       sample_rate %f\n", table->sample_rate);

        if(table->version == 1)
          {
          printf("       samples_per_packet: %d\n", table->audio_samples_per_packet);
          printf("       bytes_per_packet:   %d\n", table->audio_bytes_per_packet);
          printf("       bytes_per_frame:    %d\n", table->audio_bytes_per_frame);
          printf("       bytes_per_samples:  %d\n", table->audio_bytes_per_sample);
          }
        quicktime_wave_dump(&table->wave);
}


void quicktime_stsd_table_dump(void *minf_ptr, quicktime_stsd_table_t *table)
{
	quicktime_minf_t *minf = minf_ptr;
	printf("       format %c%c%c%c\n", table->format[0], table->format[1], table->format[2], table->format[3]);
	quicktime_print_chars("       reserved ", table->reserved, 6);
	printf("       data_reference %d\n", table->data_reference);

	if(minf->is_audio) quicktime_stsd_audio_dump(table);
	if(minf->is_video) quicktime_stsd_video_dump(table);

	if (quicktime_match_32(table->format, "pano"))
	    quicktime_pano_dump(&(table->pano));
	if (quicktime_match_32(table->format, "qtvr"))
	    quicktime_qtvr_dump(&(table->qtvr));
}

void quicktime_write_stsd_table(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_table_t *table)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, table->format);
/*printf("quicktime_write_stsd_table %c%c%c%c\n", table->format[0], table->format[1], table->format[2], table->format[3]); */
	quicktime_write_data(file, table->reserved, 6);
	quicktime_write_int16(file, table->data_reference);

	if(minf->is_audio) quicktime_write_stsd_audio(file, table);
	if(minf->is_video) quicktime_write_stsd_video(file, table);
	if(minf->is_panorama) quicktime_write_pano(file, &(table->pano));
	if(minf->is_qtvr == QTVR_QTVR) quicktime_write_qtvr(file, &(table->qtvr));

	quicktime_atom_write_footer(file, &atom);
}
