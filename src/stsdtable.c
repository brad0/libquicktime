#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

void quicktime_read_stsd_audio(quicktime_t *file, quicktime_stsd_table_t *table,
                               quicktime_atom_t *parent_atom)
{
  quicktime_atom_t leaf_atom;
	table->version = quicktime_read_int16(file);
	table->revision = quicktime_read_int16(file);
	quicktime_read_data(file, (uint8_t*)(table->vendor), 4);

        if(table->version < 2)
          {
          table->channels = quicktime_read_int16(file);
          table->sample_size = quicktime_read_int16(file);
          table->compression_id = quicktime_read_int16(file);
          table->packet_size = quicktime_read_int16(file);
          table->samplerate = quicktime_read_fixed32(file);
          // Kluge for fixed32 limitation
          if(table->samplerate + 65536 == 96000 ||
             table->samplerate + 65536 == 88200) table->samplerate += 65536;
          
          //          fprintf(stderr, "stsd version: %d\n", table->version);
          
          if(table->version == 1)
            {
            table->audio_samples_per_packet = quicktime_read_int32(file);
            table->audio_bytes_per_packet = quicktime_read_int32(file);
            table->audio_bytes_per_frame  = quicktime_read_int32(file);
            table->audio_bytes_per_sample  = quicktime_read_int32(file);
            
            if(table->version == 2) // Skip another 20 bytes
              quicktime_set_position(file, quicktime_position(file) + 20);
            
            }
          }
        else /* SoundDescriptionV2 */
          {
          /*
           *  SInt16     always3;
           *  SInt16     always16;
           *  SInt16     alwaysMinus2;
           *  SInt16     always0;
           *  UInt32     always65536;
           *  UInt32     sizeOfStructOnly;
           */
          quicktime_set_position(file, quicktime_position(file) + 16);
          //          quicktime_set_position(file, quicktime_position(file) + 12);
          //          fprintf(stderr, "sizeOfStructOnly: %d\n", quicktime_read_int32(file));
          
          /*
           * Float64    audioSampleRate;
           */
          table->samplerate = quicktime_read_double64(file);

          /*
           * UInt32     numAudioChannels;
           */

          table->channels = quicktime_read_int32(file);
          
          /*
           * SInt32     always7F000000;
           */
          
          quicktime_set_position(file, quicktime_position(file) + 4);
          //          fprintf(stderr, "always7F000000: %08lx\n", quicktime_read_int32(file));

          

          table->sample_size = quicktime_read_int32(file);
          //          fprintf(stderr, "constBitsPerChannel: %d\n", table->sample_size);
          table->formatSpecificFlags = quicktime_read_int32(file);
          //          fprintf(stderr, "formatSpecificFlags: %08x\n", table->formatSpecificFlags);

          /* The following 2 are (hopefully) unused */
          
          
          table->constBytesPerAudioPacket = quicktime_read_int32(file);
          table->constLPCMFramesPerAudioPacket = quicktime_read_int32(file);

          //          quicktime_set_position(file, quicktime_position(file) + 8);

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
          else if(quicktime_atom_is(&leaf_atom, "esds"))
            {
            quicktime_read_esds(file, &(table->esds));
            table->has_esds = 1;
            quicktime_atom_skip(file, &leaf_atom);
            // fprintf(stderr, "Got esds\n");
            }
          else if(quicktime_atom_is(&leaf_atom, "chan"))
            {
            quicktime_read_chan(file, &(table->chan));
            table->has_chan = 1;
            quicktime_atom_skip(file, &leaf_atom);
            //            fprintf(stderr, "Got chan\n");
            }
          else
            {
            fprintf(stderr, "Skipping unknown atom \"%4s\" inside audio stsd\n",
                    leaf_atom.type);
            quicktime_atom_skip(file, &leaf_atom);
            }
          } 

}

void quicktime_write_stsd_audio(quicktime_t *file, quicktime_stsd_table_t *table)
{
        int tmp_version = file->file_type & (LQT_FILE_QT|LQT_FILE_QT_OLD) ? table->version : 0;
	quicktime_write_int16(file, tmp_version);
	quicktime_write_int16(file, table->revision);
	quicktime_write_data(file, (uint8_t*)(table->vendor), 4);

        if(tmp_version < 2)
          {
          quicktime_write_int16(file, table->channels);
          quicktime_write_int16(file, (file->file_type & (LQT_FILE_QT|LQT_FILE_QT_OLD)) ? table->sample_size : 16);
          quicktime_write_int16(file, (file->file_type & (LQT_FILE_QT|LQT_FILE_QT_OLD)) ? table->compression_id : 0);
          quicktime_write_int16(file, table->packet_size);
          quicktime_write_fixed32(file, table->samplerate);
          if(tmp_version == 1)
            {
            quicktime_write_int32(file, table->audio_samples_per_packet);
            quicktime_write_int32(file, table->audio_bytes_per_packet);
            quicktime_write_int32(file, table->audio_bytes_per_frame);
            quicktime_write_int32(file, table->audio_bytes_per_sample);
            }
          }
        else
          {
          quicktime_write_int16(file, 0x0003); //                               SInt16 always3;
          quicktime_write_int16(file, 0x0010); //                               SInt16 always16;
          quicktime_write_int16(file, 0xFFFE); //                               SInt16 alwaysMinus2;
          quicktime_write_int16(file, 0x0000); //                               SInt16 always0;
          quicktime_write_int32(file, 0x00010000); //                           UInt32 always65536;
          quicktime_write_int32(file, 0x00000048); //                           UInt32 sizeOfStructOnly;
          quicktime_write_double64(file, table->samplerate); //                 Float64 audioSampleRate;
          quicktime_write_int32(file, table->channels); //                      UInt32 numAudioChannels;
          quicktime_write_int32(file, 0x7F000000); //                           SInt32 always7F000000;
          quicktime_write_int32(file, table->sample_size); //                   UInt32 constBitsPerChannel;
          quicktime_write_int32(file, table->formatSpecificFlags); //          UInt32 formatSpecificFlags;
          quicktime_write_int32(file, table->constBytesPerAudioPacket);      // UInt32 constBytesPerAudioPacket;
          quicktime_write_int32(file, table->constLPCMFramesPerAudioPacket); // UInt32 constLPCMFramesPerAudioPacket;
          }
        if(file->file_type & (LQT_FILE_QT|LQT_FILE_QT_OLD))
          {
          if(table->has_wave)
            {
            /* For quicktime, we must put the esds atom into the wave atom */
            if(table->has_esds)
              {
              memcpy(&table->wave.esds, &table->esds, sizeof(table->wave.esds));
              table->wave.has_esds = 1;
              }
            quicktime_write_wave(file, &table->wave);
            if(table->has_esds)
              {
              memset(&table->wave.esds, 0, sizeof(table->wave.esds));
              table->wave.has_esds = 0;
              }
            }
          if(table->has_chan)
            quicktime_write_chan(file, &table->chan);
          }
        else
          {
          if(table->has_esds)
            quicktime_write_esds(file, &table->esds);
          }
        
}


void quicktime_read_stsd_table_raw(quicktime_t *file, quicktime_stsd_table_t *table)
  {
  quicktime_atom_t leaf_atom;
  int64_t old_position;
  old_position = quicktime_position(file);

  quicktime_atom_read_header(file, &leaf_atom);

  /* We write the raw atom verbatim into the raw table */
  table->table_raw_size = leaf_atom.size;

  table->table_raw = malloc(table->table_raw_size);
  quicktime_set_position(file, leaf_atom.start);
  quicktime_read_data(file, table->table_raw, table->table_raw_size);
  

  }


void quicktime_read_stsd_video(quicktime_t *file, quicktime_stsd_table_t *table,
                               quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;
	int len, bits_per_pixel;
        
        //        fprintf(stderr, "quicktime_read_stsd_video 1 %lld\n", quicktime_position(file));
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
        
        //  fprintf(stderr, "quicktime_read_stsd_video 2 %lld\n", quicktime_position(file));
        
        /*  If ctab_id is zero, the colortable follows immediately
         *  after the ctab ID
         */
        bits_per_pixel = table->depth & 0x1f;
        if(!table->ctab_id &&
           ((bits_per_pixel == 1) ||
            (bits_per_pixel == 2) ||
            (bits_per_pixel == 4) ||
            (bits_per_pixel == 8)))
          {
          //          fprintf(stderr, "Reading color table, depth: %d\n", table->depth);
          quicktime_read_ctab(file, &(table->ctab));
          }
        else
          quicktime_default_ctab(&(table->ctab), table->depth);
        
        //        fprintf(stderr, "quicktime_read_stsd_video 3 %lld\n",
        //                quicktime_position(file));
	while(quicktime_position(file) + 8 < parent_atom->end)
	{
		quicktime_atom_read_header(file, &leaf_atom);
#if 0
                fprintf(stderr, "quicktime_read_stsd_video 1 %c%c%c%c, pos: %lld, end: %lld\n",
                        leaf_atom.type[0],
                        leaf_atom.type[1],
                        leaf_atom.type[2],
                        leaf_atom.type[3],
                        quicktime_position(file),
                        parent_atom->end
                       );
#endif		
		if(quicktime_atom_is(&leaf_atom, "ctab"))
		{
			quicktime_read_ctab(file, &(table->ctab));
		}
		else
		if(quicktime_atom_is(&leaf_atom, "gama"))
		{
                        quicktime_read_gama(file, &(table->gama));
	                table->has_gama = 1;
		}
		else
		if(quicktime_atom_is(&leaf_atom, "fiel"))
		{
                        quicktime_read_fiel(file, &(table->fiel));
	                table->has_fiel = 1;
		}
		else
		if (quicktime_atom_is(&leaf_atom, "pasp"))
		{
			quicktime_read_pasp(file, &(table->pasp));
                        table->has_pasp = 1;
		}
		else
		if (quicktime_atom_is(&leaf_atom, "clap"))
		{
			quicktime_read_clap(file, &(table->clap));
                        table->has_clap = 1;
		}
		else
		if (quicktime_atom_is(&leaf_atom, "colr"))
		{
			quicktime_read_colr(file, &(table->colr));
                        table->has_colr = 1;
		}
		else
		if (quicktime_atom_is(&leaf_atom, "esds"))
		{
                //                fprintf(stderr, "*** GOT ESDS ATOM\n");
			quicktime_read_esds(file, &(table->esds));
                        table->has_esds = 1;
                        quicktime_atom_skip(file, &leaf_atom);

		}
		else
                {
                quicktime_user_atoms_read_atom(file,
                                               &table->user_atoms,
                                               &leaf_atom);
                }
                quicktime_atom_skip(file, &leaf_atom);
#if 0
                fprintf(stderr, "quicktime_read_stsd_video 2 %c%c%c%c, pos: %lld, end: %lld\n",
                leaf_atom.type[0],
                leaf_atom.type[1],
                leaf_atom.type[2],
                leaf_atom.type[3],
                quicktime_position(file),
                parent_atom->end
                );
#endif
	}
//printf("quicktime_read_stsd_video 2\n");
}


void quicktime_write_stsd_video(quicktime_t *file, quicktime_stsd_table_t *table)
  {
  int compressor_name_len, i;
  compressor_name_len = strlen(table->compressor_name);
  if(file->file_type & (LQT_FILE_QT|LQT_FILE_QT_OLD))
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
    quicktime_write_char(file, compressor_name_len);
    quicktime_write_data(file, (uint8_t*)table->compressor_name, 31);
    quicktime_write_int16(file, table->depth);
    quicktime_write_int16(file, table->ctab_id);
    }
  else
    {
    quicktime_write_int16(file, 0);
    quicktime_write_int16(file, 0);
    quicktime_write_int32(file, 0);
    quicktime_write_int32(file, 0);
    quicktime_write_int32(file, 0);
    quicktime_write_int16(file, table->width);
    quicktime_write_int16(file, table->height);
    quicktime_write_fixed32(file, 0x00480000);
    quicktime_write_fixed32(file, 0x00480000);
    quicktime_write_int32(file, 0);
    quicktime_write_int16(file, 1);
    quicktime_write_data(file, (uint8_t*)table->compressor_name, compressor_name_len);
    for(i = 0; i < 32 - compressor_name_len; i++)
      quicktime_write_char(file, 0);
    
    quicktime_write_int16(file, 24);
    quicktime_write_int16(file, -1);
    }

  if (table->has_pasp)
    quicktime_write_pasp(file, &(table->pasp));
  if (table->has_clap)
    quicktime_write_clap(file, &(table->clap));
  if (table->has_colr)
    quicktime_write_colr(file, &(table->colr));
  if (table->has_fiel)
    quicktime_write_fiel(file, &(table->fiel));
  if (table->has_gama)
    quicktime_write_gama(file, &(table->gama));
    
  quicktime_write_user_atoms(file,
                             &table->user_atoms);
  if(table->has_esds)
    quicktime_write_esds(file, &table->esds);
  }

void quicktime_read_stsd_table(quicktime_t *file, quicktime_minf_t *minf,
                               quicktime_stsd_table_t *table)
{
	quicktime_atom_t leaf_atom;

	quicktime_atom_read_header(file, &leaf_atom);

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
 	table->vendor[1] = 'q';
 	table->vendor[2] = 't';
 	table->vendor[3] = ' ';

	table->temporal_quality = 100;
	table->spatial_quality = 258;
	table->width = 0;
	table->height = 0;
	table->dpi_horizontal = 72;
	table->dpi_vertical = 72;
	table->data_size = 0;
	table->frames_per_sample = 1;
	for(i = 0; i < 32; i++) table->compressor_name[i] = 0;
	sprintf(table->compressor_name, "%s-%s", PACKAGE, VERSION);
	table->depth = 24;
	table->ctab_id = 65535;
	quicktime_ctab_init(&(table->ctab));
	quicktime_pasp_init(&(table->pasp));
	quicktime_gama_init(&(table->gama));
	quicktime_fiel_init(&(table->fiel));
	quicktime_clap_init(&(table->clap));
	quicktime_colr_init(&(table->colr));
	quicktime_pano_init(&(table->pano));
	quicktime_qtvr_init(&(table->qtvr));
	quicktime_chan_init(&(table->chan));
	
	table->channels = 0;
	table->sample_size = 0;
	table->compression_id = 0;
	table->packet_size = 0;
	table->samplerate = 0.0;
}

void quicktime_stsd_table_delete(quicktime_stsd_table_t *table)
{
        /* LQT: Delete table_raw as well */
        if(table->table_raw)
          free(table->table_raw);
        quicktime_ctab_delete(&(table->ctab));
	quicktime_wave_delete(&(table->wave));
	quicktime_esds_delete(&(table->esds));
        quicktime_user_atoms_delete(&(table->user_atoms));
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

	if (table->has_pasp)
		quicktime_pasp_dump(&(table->pasp));
	if (table->has_clap)
		quicktime_clap_dump(&(table->clap));
	if (table->has_colr)
		quicktime_colr_dump(&(table->colr));
	if (table->has_fiel)
		quicktime_fiel_dump(&(table->fiel));
	if (table->has_gama)
		quicktime_gama_dump(&(table->gama));

	if(!table->ctab_id) quicktime_ctab_dump(&(table->ctab));
	if(table->has_esds) quicktime_esds_dump(&(table->esds));
	quicktime_user_atoms_dump(&(table->user_atoms));
}

void quicktime_stsd_audio_dump(quicktime_stsd_table_t *table)
{
	printf("       version %d\n", table->version);
	printf("       revision %d\n", table->revision);
	printf("       vendor %c%c%c%c\n", table->vendor[0], table->vendor[1], table->vendor[2], table->vendor[3]);
	printf("       channels %d\n", table->channels);
	printf("       sample_size %d\n", table->sample_size);

        if(table->version < 2)
          {
          printf("       compression_id %d\n", table->compression_id);
          printf("       packet_size %d\n", table->packet_size);
          printf("       samplerate %f\n",  table->samplerate);
          
          if(table->version == 1)
            {
            printf("       samples_per_packet: %d\n", table->audio_samples_per_packet);
            printf("       bytes_per_packet:   %d\n", table->audio_bytes_per_packet);
            printf("       bytes_per_frame:    %d\n", table->audio_bytes_per_frame);
            printf("       bytes_per_samples:  %d\n", table->audio_bytes_per_sample);
            }
          
          }
        else if(table->version == 2)
          {
          printf("       samplerate                     %f\n",  table->samplerate);
          printf("       formatSpecificFlags:           %08x\n", table->formatSpecificFlags);
          printf("       constBytesPerAudioPacket:      %d\n", table->constBytesPerAudioPacket);
          printf("       constLPCMFramesPerAudioPacket: %d\n", table->constLPCMFramesPerAudioPacket);
          }
        
        if(table->has_wave)
          quicktime_wave_dump(&table->wave);
        if(table->has_esds)
          quicktime_esds_dump(&table->esds);
        if(table->has_chan)
          quicktime_chan_dump(&table->chan);
	quicktime_user_atoms_dump(&(table->user_atoms));
}


void quicktime_stsd_table_dump(void *minf_ptr, quicktime_stsd_table_t *table)
{
	quicktime_minf_t *minf = minf_ptr;
	printf("       format %c%c%c%c\n",
               table->format[0], table->format[1],
               table->format[2], table->format[3]);

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

void quicktime_set_stsd_audio_v1(quicktime_stsd_table_t *table,
                                 uint32_t samples_per_packet,
                                 uint32_t bytes_per_packet,
                                 uint32_t bytes_per_frame,
                                 uint32_t bytes_per_sample)
  {
  table->version = 1;
  table->audio_samples_per_packet = samples_per_packet;
  table->audio_bytes_per_packet = bytes_per_packet;
  table->audio_bytes_per_frame = bytes_per_frame;
  table->audio_bytes_per_sample = bytes_per_sample;
  }

#if 1
void quicktime_set_stsd_audio_v2(quicktime_stsd_table_t *table,
                                 uint32_t formatSpecificFlags,
                                 uint32_t constBytesPerAudioPacket,
                                 uint32_t constLPCMFramesPerAudioPacket)
  {
  table->version = 2;
  table->formatSpecificFlags = formatSpecificFlags;
  table->constBytesPerAudioPacket = constBytesPerAudioPacket;
  table->constLPCMFramesPerAudioPacket = constLPCMFramesPerAudioPacket;
  }
#endif

uint8_t * quicktime_stsd_get_user_atom(quicktime_trak_t * trak, char * name, uint32_t * len)
  {
  quicktime_stsd_table_t *table = &(trak->mdia.minf.stbl.stsd.table[0]);
  return(quicktime_user_atoms_get_atom(&table->user_atoms, name, len));
  }

void quicktime_stsd_set_user_atom(quicktime_trak_t * trak, char * name,
                                  uint8_t * data, uint32_t len)
  {
  quicktime_stsd_table_t *table = &(trak->mdia.minf.stbl.stsd.table[0]);
  quicktime_user_atoms_add_atom(&table->user_atoms,
                                name, data, len);
  
  }
