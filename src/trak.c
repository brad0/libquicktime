/*******************************************************************************
 trak.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2007 Members of the libquicktime project.

 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2.1 of the License, or (at your option)
 any later version.

 This library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this library; if not, write to the Free Software Foundation, Inc., 51
 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*******************************************************************************/ 

#include "lqt_private.h"
#include <stdlib.h>

int quicktime_trak_init(quicktime_trak_t *trak)
{
	quicktime_tkhd_init(&(trak->tkhd));
	quicktime_edts_init(&(trak->edts));
	quicktime_mdia_init(&(trak->mdia));
	quicktime_tref_init(&(trak->tref));
	trak->has_tref = 0;
	return 0;
}

int quicktime_trak_init_video(quicktime_t *file, 
							quicktime_trak_t *trak, 
							int frame_w, 
							int frame_h, 
                                                        int frame_duration,
                                                        int timescale,
							char *compressor)
{

	quicktime_tkhd_init_video(file, 
		&(trak->tkhd), 
		frame_w, 
		frame_h);
	quicktime_mdia_init_video(file, 
		&(trak->mdia), 
		frame_w, 
		frame_h, 
                frame_duration,
                timescale,
		compressor);
	quicktime_edts_init_table(&(trak->edts));
        trak->has_edts = 1;
        return 0;
}


int quicktime_trak_init_qtvr(quicktime_t *file, quicktime_trak_t *trak, int track_type, int width, int height, int frame_duration, int timescale)
{

	quicktime_tkhd_init_video(file, 
		&(trak->tkhd),
		height, 
		width);
	quicktime_mdia_init_qtvr(file, 
		&(trak->mdia), track_type, timescale, frame_duration);
	quicktime_edts_init_table(&(trak->edts));
        trak->has_edts = 1;
	quicktime_tref_init_qtvr(&(trak->tref), track_type);
        trak->has_tref = 1;
	return 0;
}

int quicktime_trak_init_panorama(quicktime_t *file, quicktime_trak_t *trak, int width, int height, int frame_duration, int timescale)
{

	quicktime_tkhd_init_video(file, 
		&(trak->tkhd), 
		height, 
		height/2);
	quicktime_mdia_init_panorama(file, 
		&(trak->mdia), width, height, timescale, frame_duration);
	quicktime_edts_init_table(&(trak->edts));
        trak->has_edts = 1;

	return 0;
}

int quicktime_trak_init_audio(quicktime_t *file, 
							quicktime_trak_t *trak, 
							int channels, 
							int sample_rate, 
							int bits, 
							char *compressor)
  {
  quicktime_mdia_init_audio(file, &(trak->mdia), 
                            channels, 
                            sample_rate, 
                            bits, 
                            compressor);
  quicktime_edts_init_table(&(trak->edts));
  trak->has_edts = 1;

  return 0;
  }

int quicktime_trak_init_text(quicktime_t * file, quicktime_trak_t * trak,
                             int timescale)
  {
  trak->tkhd.volume = 0;
  trak->tkhd.flags = 3;
  quicktime_mdia_init_text(file, &(trak->mdia), 
                           timescale);
  quicktime_edts_init_table(&(trak->edts));
  trak->has_edts = 1;
  return 0;
  }

int quicktime_trak_init_tx3g(quicktime_t * file, quicktime_trak_t * trak,
                             int timescale)
  {
  trak->tkhd.volume = 0;
  trak->tkhd.flags = 1;
  quicktime_mdia_init_tx3g(file, &(trak->mdia), 
                           timescale);
 
  return 0;
  }
                            
                            

int quicktime_trak_delete(quicktime_trak_t *trak)
{
	quicktime_mdia_delete(&(trak->mdia));
	quicktime_edts_delete(&(trak->edts));
	quicktime_tkhd_delete(&(trak->tkhd));
	quicktime_tref_delete(&(trak->tref));

        if(trak->chunk_sizes)
          free(trak->chunk_sizes);
        return 0;
}


int quicktime_trak_dump(quicktime_trak_t *trak)
{
	lqt_dump(" track\n");
	quicktime_tkhd_dump(&(trak->tkhd));
	if(trak->has_edts) quicktime_edts_dump(&(trak->edts));
	if (trak->has_tref)
	    quicktime_tref_dump(&(trak->tref));
	quicktime_mdia_dump(&(trak->mdia));

	return 0;
}

// Used when reading a file
quicktime_trak_t* quicktime_add_trak(quicktime_t *file)
{
	quicktime_moov_t *moov = &(file->moov);
	if(moov->total_tracks < MAXTRACKS)
	{
        moov->trak[moov->total_tracks] = calloc(1, sizeof(quicktime_trak_t));
		quicktime_trak_init(moov->trak[moov->total_tracks]);
		moov->total_tracks++;
	}
	return moov->trak[moov->total_tracks - 1];
}

int quicktime_delete_trak(quicktime_moov_t *moov)
{
	if(moov->total_tracks)
	{
		moov->total_tracks--;
		quicktime_trak_delete(moov->trak[moov->total_tracks]);
		free(moov->trak[moov->total_tracks]);
	}
	return 0;
}


int quicktime_read_trak(quicktime_t *file, quicktime_trak_t *trak,
                        quicktime_atom_t *trak_atom)
  {
  quicktime_atom_t leaf_atom;

  do
    {
    quicktime_atom_read_header(file, &leaf_atom);

    /* mandatory */
    if(quicktime_atom_is(&leaf_atom, "tkhd"))
      quicktime_read_tkhd(file, &(trak->tkhd));
    else if(quicktime_atom_is(&leaf_atom, "mdia"))
      quicktime_read_mdia(file, trak, &(trak->mdia), &leaf_atom);
    /* optional */
    else if(quicktime_atom_is(&leaf_atom, "clip"))
      quicktime_atom_skip(file, &leaf_atom);
    else if(quicktime_atom_is(&leaf_atom, "matt"))
      quicktime_atom_skip(file, &leaf_atom);
    else if(quicktime_atom_is(&leaf_atom, "edts"))
      {
      quicktime_read_edts(file, &(trak->edts), &leaf_atom);
      trak->has_edts = 1;
      }
    else if(quicktime_atom_is(&leaf_atom, "load"))
      quicktime_atom_skip(file, &leaf_atom);
    else if(quicktime_atom_is(&leaf_atom, "imap"))
      quicktime_atom_skip(file, &leaf_atom);
    else if(quicktime_atom_is(&leaf_atom, "udta"))
      quicktime_atom_skip(file, &leaf_atom);
    else if(quicktime_atom_is(&leaf_atom, "tref"))
      {
      trak->has_tref = 1;
      quicktime_read_tref(file, &(trak->tref), &leaf_atom);
      }
    else quicktime_atom_skip(file, &leaf_atom);
    } while(quicktime_position(file) < trak_atom->end);
  
  return 0;
  }

int quicktime_write_trak(quicktime_t *file, 
	quicktime_trak_t *trak, 
	long moov_time_scale)
{
	long duration;
	long timescale;
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "trak");
	quicktime_trak_duration(trak, &duration, &timescale);
        
/* get duration in movie's units */
	trak->tkhd.duration = (long)((float)duration / timescale * moov_time_scale);
	trak->mdia.mdhd.duration = duration;
	trak->mdia.mdhd.time_scale = timescale;

	quicktime_write_tkhd(file, &(trak->tkhd));
	if (trak->mdia.minf.is_panorama) trak->edts.elst.total_entries = 1;
        
        if(trak->has_edts) quicktime_write_edts(file, &(trak->edts), trak->tkhd.duration);
	quicktime_write_mdia(file, &(trak->mdia));
	
	if (trak->has_tref) 
          quicktime_write_tref(file, &(trak->tref));
//	if (trak->mdia.minf.is_object) 
//	    quicktime_write_tref(file, &(trak->tref));
	
	quicktime_atom_write_footer(file, &atom);

	return 0;
}

int64_t quicktime_track_samples(quicktime_t *file, quicktime_trak_t *trak)
{
		quicktime_stts_t *stts = &(trak->mdia.minf.stbl.stts);
		int i;
		int64_t total = 0;

	if(file->wr)
	{
        if(trak->mdia.minf.is_audio)
          {
          for(i = 0; i < stts->total_entries; i++)
            {
            total += stts->table[i].sample_count *
              stts->table[i].sample_duration;
            }
          }
        else
          {
          for(i = 0; i < stts->total_entries; i++)
            {
            total += stts->table[i].sample_count;
            }
          }
        return total;
	}
	else
	{
/* get the sample count when reading only */
                /* Get this from the AVI header */
                //                if(trak->strl)
                //                  {
                //                  total = 
                //                  }
                /* LQT: Make this correct for VBR files */
                if(trak->mdia.minf.is_audio)
                  {
                  for(i = 0; i < stts->total_entries; i++)
                    {
                    total += stts->table[i].sample_count *
                      stts->table[i].sample_duration;
                    }
                  }
                else
                  {
                  for(i = 0; i < stts->total_entries; i++)
                    {
                    total += stts->table[i].sample_count;
                    }
                  }
		return total;
	}
}

long quicktime_sample_of_chunk(quicktime_trak_t *trak, long chunk)
{
	quicktime_stsc_table_t *table = trak->mdia.minf.stbl.stsc.table;
	long total_entries = trak->mdia.minf.stbl.stsc.total_entries;
	long chunk1entry, chunk2entry;
	long chunk1, chunk2, chunks, total = 0;

	for(chunk1entry = total_entries - 1, chunk2entry = total_entries; 
		chunk1entry >= 0; 
		chunk1entry--, chunk2entry--)
	{
		chunk1 = table[chunk1entry].chunk;

		if(chunk > chunk1)
		{
			if(chunk2entry < total_entries)
			{
				chunk2 = table[chunk2entry].chunk;

				if(chunk < chunk2) chunk2 = chunk;
			}
			else
				chunk2 = chunk;

			chunks = chunk2 - chunk1;

			total += chunks * table[chunk1entry].samples;
		}
	}

	return total;
}

// For AVI
int quicktime_avg_chunk_samples(quicktime_t *file, quicktime_trak_t *trak)
{
	int chunk = trak->mdia.minf.stbl.stco.total_entries - 1;
	long total_samples;

	if(chunk >= 0)
	{
		total_samples = quicktime_sample_of_chunk(trak, chunk);
		return total_samples / (chunk + 1);
	}
	else
	{
		total_samples = quicktime_track_samples(file, trak);
		return total_samples;
	}
}

int quicktime_chunk_of_sample(int64_t *chunk_sample, 
                              int64_t *chunk, 
                              quicktime_trak_t *trak, 
                              int64_t sample)
  {
  quicktime_stsc_table_t *table = trak->mdia.minf.stbl.stsc.table;
  long total_entries = trak->mdia.minf.stbl.stsc.total_entries;
  long chunk2entry;
  long chunk1, chunk2, chunk1samples, range_samples, total = 0;
  
  chunk1 = 1;
  chunk1samples = 0;
  chunk2entry = 0;
  
  if(!total_entries)
    {
    *chunk_sample = 0;
    *chunk = 0;
    return 0;
    }

  do
    {
    chunk2 = table[chunk2entry].chunk;
    *chunk = chunk2 - chunk1;
    range_samples = *chunk * chunk1samples;

    if(sample < total + range_samples) break;

    chunk1samples = table[chunk2entry].samples;
    chunk1 = chunk2;

    if(chunk2entry < total_entries)
      {
      chunk2entry++;
      total += range_samples;
      }
    }while(chunk2entry < total_entries);

  if(chunk1samples)
    *chunk = (sample - total) / chunk1samples + chunk1;
  else
    *chunk = 1;

  *chunk_sample = total + (*chunk - chunk1) * chunk1samples;
  return 0;
  }

int64_t quicktime_chunk_to_offset(quicktime_t *file,
                                  quicktime_trak_t *trak, long chunk)
{
	quicktime_stco_table_t *table = trak->mdia.minf.stbl.stco.table;
	int64_t result = 0;

	if(trak->mdia.minf.stbl.stco.total_entries && 
		chunk > trak->mdia.minf.stbl.stco.total_entries)
		result = table[trak->mdia.minf.stbl.stco.total_entries - 1].offset;
	else
	if(trak->mdia.minf.stbl.stco.total_entries)
		result = table[chunk - 1].offset;
	else
		result = HEADER_LENGTH * 2;

	return result;
}

long quicktime_offset_to_chunk(int64_t *chunk_offset, 
	quicktime_trak_t *trak, 
	int64_t offset)
{
	quicktime_stco_table_t *table = trak->mdia.minf.stbl.stco.table;
	int i;

	for(i = trak->mdia.minf.stbl.stco.total_entries - 1; i >= 0; i--)
	{
		if(table[i].offset <= offset)
		{
			*chunk_offset = table[i].offset;
			return i + 1;
		}
	}
	*chunk_offset = HEADER_LENGTH * 2;
	return 1;
}


int64_t quicktime_sample_range_size(quicktime_trak_t *trak, 
	long chunk_sample, 
	long sample)
{

int64_t i, total;
        /* LQT: For audio, quicktime_sample_rage_size makes no sense */
        if(trak->mdia.minf.is_audio)
          return 0;
	else
          {
          /* All frames have the same size */
          if(trak->mdia.minf.stbl.stsz.sample_size)
            {
            total = (sample - chunk_sample) *
              trak->mdia.minf.stbl.stsz.sample_size;
            }
/* probably video */
          else
            {
            for(i = chunk_sample, total = 0; i < sample; i++)
              {
              total += trak->mdia.minf.stbl.stsz.table[i].size;
              }
            }
          
	}
	return total;
}

int64_t quicktime_sample_to_offset(quicktime_t *file,
                                   quicktime_trak_t *trak, long sample)
{
	int64_t chunk, chunk_sample, chunk_offset1, chunk_offset2;

	quicktime_chunk_of_sample(&chunk_sample, &chunk, trak, sample);
	chunk_offset1 = quicktime_chunk_to_offset(file, trak, chunk);
	chunk_offset2 = chunk_offset1 +
          quicktime_sample_range_size(trak, chunk_sample, sample);
	return chunk_offset2;
}

void quicktime_write_chunk_header(quicktime_t *file, 
	quicktime_trak_t *trak, 
	quicktime_atom_t *chunk)
  {
  if(file->file_type & (LQT_FILE_AVI|LQT_FILE_AVI_ODML))
    {
    /* Get tag from first riff strl */
    quicktime_strl_t *strl = trak->strl;
    char *tag = strl->tag;
    
    /* Create new RIFF object at 1 Gig mark */

    if(file->file_type == LQT_FILE_AVI_ODML)
      {
      quicktime_riff_t *riff = file->riff[file->total_riffs - 1];
      if(quicktime_position(file) - riff->atom.start > file->max_riff_size)
        {
        quicktime_finalize_riff(file, riff);
        quicktime_init_riff(file);
        }
      }
    
    /* Write AVI header */
    quicktime_atom_write_header(file, chunk, tag);
    }
  else
    {
    chunk->start = quicktime_position(file);
    }
  }

void quicktime_write_chunk_footer(quicktime_t *file, 
                                  quicktime_trak_t *trak,
                                  int current_chunk,
                                  quicktime_atom_t *chunk, 
                                  int samples)
{
	int64_t offset = chunk->start;
	int sample_size = quicktime_position(file) - offset;

// Write AVI footer
	if(file->file_type & (LQT_FILE_AVI|LQT_FILE_AVI_ODML))
          {
          quicktime_atom_write_footer(file, chunk);
          
          // Save original index entry for first RIFF only
          if(file->total_riffs < 2)
            {
            quicktime_update_idx1table(file, 
                                       trak, 
                                       offset, 
                                       sample_size);
            }
          
          // Save partial index entry
          if(file->file_type == LQT_FILE_AVI_ODML)
            quicktime_update_ixtable(file, trak, offset, sample_size);
          
          
          if(sample_size > trak->strl->strh.dwSuggestedBufferSize)
            trak->strl->strh.dwSuggestedBufferSize = ((sample_size+15)/16)*16;
          }
        if(offset + sample_size > file->mdat.atom.size)
		file->mdat.atom.size = offset + sample_size;

	quicktime_update_stco(&(trak->mdia.minf.stbl.stco), 
		current_chunk, 
		offset);

	if(trak->mdia.minf.is_video || trak->mdia.minf.is_text)
          quicktime_update_stsz(&(trak->mdia.minf.stbl.stsz), 
                                current_chunk - 1, 
                                sample_size);
        /* Need to increase sample count for VBR (the VBR routines to it
           themselves) */
	if(trak->mdia.minf.is_audio && !trak->mdia.minf.is_audio_vbr)
          trak->mdia.minf.stbl.stts.table->sample_count += samples;
        
	if(trak->mdia.minf.is_panorama) {
		quicktime_update_stsz(&(trak->mdia.minf.stbl.stsz), 
		current_chunk - 1, 
		sample_size);	
	}
	
	if(trak->mdia.minf.is_qtvr) {
		quicktime_update_stsz(&(trak->mdia.minf.stbl.stsz), 
		current_chunk - 1, 
		sample_size);
	}
        quicktime_update_stsc(&(trak->mdia.minf.stbl.stsc), 
                              current_chunk, 
                              samples);
        
}

int quicktime_trak_duration(quicktime_trak_t *trak, 
	long *duration, 
	long *timescale)
{
	quicktime_stts_t *stts = &(trak->mdia.minf.stbl.stts);
	int i;

	*duration = 0;

	for(i = 0; i < stts->total_entries; i++)
	{
		*duration += stts->table[i].sample_duration * stts->table[i].sample_count;
	}
	*timescale = trak->mdia.mdhd.time_scale;

	return 0;
}

int quicktime_trak_fix_counts(quicktime_t *file, quicktime_trak_t *trak)
{
	long samples = quicktime_track_samples(file, trak);
        if(trak->mdia.minf.is_video || trak->mdia.minf.is_text)
          {
          quicktime_compress_stts(&(trak->mdia.minf.stbl.stts));
          if(trak->mdia.minf.stbl.stts.total_entries == 1)
            trak->mdia.minf.stbl.stts.table[0].sample_count = samples;
          }
        else if(trak->mdia.minf.is_audio_vbr)
          {
          quicktime_compress_stts(&(trak->mdia.minf.stbl.stts));
          }
        else
          trak->mdia.minf.stbl.stts.table[0].sample_count = samples;
        
        if(!trak->mdia.minf.stbl.stsz.total_entries)
	{
        // trak->mdia.minf.stbl.stsz.sample_size = 1;
		trak->mdia.minf.stbl.stsz.total_entries = samples;
	}

	return 0;
}

long quicktime_chunk_samples(quicktime_trak_t *trak, long chunk)
{
	long result, current_chunk;
	quicktime_stsc_t *stsc = &(trak->mdia.minf.stbl.stsc);
	quicktime_stts_t *stts = &(trak->mdia.minf.stbl.stts);
	quicktime_stsd_t *stsd = &(trak->mdia.minf.stbl.stsd);
	long i = stsc->total_entries - 1;

        if(!stsc->total_entries)
          return 0;
	do
	{
		current_chunk = stsc->table[i].chunk;
		result = stsc->table[i].samples;
		i--;
	}while(i >= 0 && current_chunk > chunk);
        /* LQT: Multiply with duration */
        if(stsd->table[0].compression_id == -2)
          result *= stts->table[0].sample_duration;
	return result;
}

int quicktime_trak_shift_offsets(quicktime_trak_t *trak, int64_t offset)
{
	quicktime_stco_t *stco = &(trak->mdia.minf.stbl.stco);
	int i;

	for(i = 0; i < stco->total_entries; i++)
	{
		stco->table[i].offset += offset;
	}
	return 0;
}
