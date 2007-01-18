#include <funcprotos.h>
#include <charset.h>
#include <quicktime/quicktime.h>
#include <quicktime/qtprivate.h>
#include <string.h>

#define LOG_DOMAIN "texttrack"

/* Common functions */

void lqt_init_text_map(quicktime_t * file,
                       quicktime_text_map_t * map, quicktime_trak_t * trak,
                       int encode)
  {
  const char * charset;
  map->track = trak;
  map->current_chunk = 1;
  if(!encode)
    {
    charset = lqt_get_charset(trak->mdia.mdhd.language, file->file_type);
    if(!charset)
      {
      lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN,
              "Cannot determine character set of text track, will copy the strings verbatim");
      }
    else
      map->cnv = lqt_charset_converter_create(file, charset, "UTF-8");
    }
  }

void lqt_delete_text_map(quicktime_t * file,
                         quicktime_text_map_t * map)
  {
  if(map->text_buffer) free(map->text_buffer);
  if(map->cnv) lqt_charset_converter_destroy(map->cnv);
  }

/* Decoding */


int lqt_text_tracks(quicktime_t * file)
  {
  int i, result = 0;
  quicktime_minf_t *minf;
  for(i = 0; i < file->moov.total_tracks; i++)
    {
    minf = &(file->moov.trak[i]->mdia.minf);
    if(minf->is_text)
      result++;
    }
  return result;
  }

int lqt_text_time_scale(quicktime_t * file, int track)
  {
  return file->ttracks[track].track->mdia.mdhd.time_scale;
  }

  
int lqt_read_text(quicktime_t * file, int track, char ** text, int * text_alloc,
                  int64_t * timestamp, int64_t * duration)
  {
  int64_t file_position, stts_index = 0, stts_count = 0;
  char * ptr;
  
  int string_length;
  
  quicktime_text_map_t * ttrack = &file->ttracks[track];
  quicktime_trak_t * trak = ttrack->track;
  quicktime_stts_t * stts = &trak->mdia.minf.stbl.stts;
  
  if(ttrack->current_position >= quicktime_track_samples(file, trak))
    return 0; // EOF
  
  /* Get the file position */
  file_position = quicktime_sample_to_offset(file, trak, ttrack->current_position);
  quicktime_set_position(file, file_position);

  string_length = quicktime_read_int16(file);

  if(string_length)
    {
    
    /* Read whole sample */
    if(ttrack->text_buffer_alloc < string_length)
      {
      ttrack->text_buffer_alloc = string_length + 128;
      ttrack->text_buffer = realloc(ttrack->text_buffer, ttrack->text_buffer_alloc);
      }
    quicktime_read_data(file, (uint8_t*)ttrack->text_buffer, string_length);
  
    /* Convert character set */
    lqt_charset_convert_realloc(ttrack->cnv,
                                ttrack->text_buffer, string_length,
                                text, text_alloc, (int*)0);
    }
  else /* Empty string */
    {
    if(*text_alloc < 1)
      {
      *text_alloc = 1;
      *text = realloc(*text, 1);
      }
    (*text)[0] = '\0';
    }
  
  *timestamp = quicktime_sample_to_time(stts, ttrack->current_position,
                                        &stts_index, &stts_count);
  *duration = stts->table[stts_index].sample_duration;

  /* de-macify linebreaks */
  ptr = *text;
  while(*ptr != '\0')
    {
    if(*ptr == '\r')
      *ptr = '\n';
    ptr++;
    }
  ttrack->current_position++;
  return 1;
  }

  
int lqt_is_chapter_track(quicktime_t * file, int track)
  {
  int i, j, k;

  quicktime_trak_t * trak =  file->ttracks[track].track;
  
  for(i = 0; i < file->moov.total_tracks; i++)
    {
    if(file->moov.trak[i] == trak)
      continue;

    /* Track reference present */
    if(file->moov.trak[i]->has_tref)
      {
      for(j = 0; j < file->moov.trak[i]->tref.num_references; j++)
        {
        /* Track reference has type chap */
        if(quicktime_match_32(file->moov.trak[i]->tref.references[j].type, "chap"))
          {
          for(k = 0; k < file->moov.trak[i]->tref.references[j].num_tracks; k++)
            {
            /* Track reference points to us */
            if(file->moov.trak[i]->tref.references[j].tracks[k] == trak->tkhd.track_id)
              return 1;
            }
          }
        }
      }
    }
  return 0;
  }

int64_t lqt_text_samples(quicktime_t * file, int track)
  {
  return quicktime_track_samples(file, file->ttracks[track].track);
  }


void lqt_set_text_position(quicktime_t * file, int track, int64_t position)
  {
  file->ttracks[track].current_position = position;
  }


void lqt_set_text_time(quicktime_t * file, int track, int64_t time)
  {
  int64_t stts_index, stts_count;
  quicktime_text_map_t * ttrack = &file->ttracks[track];
  quicktime_trak_t * trak = ttrack->track;
    
  file->ttracks[track].current_position =
    quicktime_time_to_sample(&(trak->mdia.minf.stbl.stts),
                             &(time),
                             &(stts_index),
                             &(stts_count));
  
  }

/* Encoding */

int lqt_add_text_track(quicktime_t * file, int timescale)
  {
  quicktime_trak_t * trak;
  file->ttracks =
    realloc(file->ttracks, (file->total_ttracks+1)*sizeof(quicktime_text_map_t));
  memset(&(file->ttracks[file->total_ttracks]), 0, sizeof(quicktime_text_map_t));
  
  trak = quicktime_add_track(file);

  if(file->file_type & (LQT_FILE_MP4 | LQT_FILE_M4A))
    quicktime_trak_init_tx3g(file, trak, timescale);
  else if(file->file_type & (LQT_FILE_QT | LQT_FILE_QT_OLD))
    quicktime_trak_init_text(file, trak, timescale);
  else
    lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN,
            "Text track not supported for this file");

  lqt_init_text_map(file, &(file->ttracks[file->total_ttracks]),
                    trak, 1);
  file->total_ttracks++;
  return 0;
  }

void lqt_set_chapter_track(quicktime_t * file, int track)
  {
  file->ttracks[track].is_chapter_track = 1;
  }

static void make_chapter_track(quicktime_t * file,
                               quicktime_trak_t * trak)
  {
  quicktime_trak_t * ref_track;
  
  /* We create one "chap" reference in one other track. Choose the first
     video track if possible */

  if(file->total_vtracks)
    ref_track = file->vtracks[0].track;
  else if(file->total_atracks)
    ref_track = file->atracks[0].track;
  else
    {
    lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN,
            "Need at least one audio or video stream for chapters");
    }
  quicktime_tref_init_chap(&ref_track->tref, trak->tkhd.track_id);
  ref_track->has_tref = 1;
  }

int lqt_write_text(quicktime_t * file, int track, const char * text,
                   int64_t duration)
  {
  const char * charset;
  quicktime_text_map_t * ttrack;
  quicktime_trak_t     * trak;
  quicktime_atom_t       chunk_atom;
  int out_len;
  
  ttrack = &file->ttracks[track];
  trak = ttrack->track;

  /* Issue a warning for AVI files (No subitles are supported here) */
  if(file->file_type & (LQT_FILE_AVI|LQT_FILE_AVI_ODML))
    {
    lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN,
            "Subtitles are not supported in AVI files");
    return 1;
    }
  
  if(!ttrack->initialized)
    {
    /* Create character set converter */
    if(file->file_type & (LQT_FILE_QT_OLD|LQT_FILE_QT))
      {
      charset = lqt_get_charset(trak->mdia.mdhd.language,
                                file->file_type);
      if(!charset)
        {
        lqt_log(file, LQT_LOG_ERROR, LOG_DOMAIN,
                "Subtitles character set could not be determined");
        return 1;
        }
      ttrack->cnv = lqt_charset_converter_create(file, "UTF-8", charset);
      }
    
    /* Set up chapter track */
    if(ttrack->is_chapter_track)
      make_chapter_track(file, trak);
    ttrack->initialized = 1;
    }

  quicktime_write_chunk_header(file, trak, &chunk_atom);

  if(text)
    {
    if(ttrack->cnv)
      {
      lqt_charset_convert_realloc(ttrack->cnv,
                                  text, -1,
                                  &ttrack->text_buffer, &ttrack->text_buffer_alloc,
                                  &out_len);
      quicktime_write_int16(file, out_len);
      quicktime_write_data(file, (uint8_t*)ttrack->text_buffer, out_len);
      }
    else
      {
      out_len = strlen(text);
      
      quicktime_write_int16(file, out_len);
      quicktime_write_data(file, (uint8_t*)text, out_len);
      }
    }
  else
    {
    quicktime_write_int16(file, 0);
    }
  
  quicktime_write_chunk_footer(file, trak, ttrack->current_chunk,
                               &chunk_atom, 1);

  quicktime_update_stts(&trak->mdia.minf.stbl.stts, ttrack->current_position, duration);
  
  ttrack->current_chunk++;
  ttrack->current_position++;
  return 0;
  }

/* Rectangle */

void lqt_set_text_box(quicktime_t * file, int track,
                      uint16_t top, uint16_t left,
                      uint16_t bottom, uint16_t right)
  {
  quicktime_trak_t      * trak;
  quicktime_stsd_table_t * stsd;
  trak = file->ttracks[track].track;
  stsd = &trak->mdia.minf.stbl.stsd.table[0];

  if(quicktime_match_32(stsd->format, "text"))
    {
    stsd->text.defaultTextBox[0] = top;
    stsd->text.defaultTextBox[1] = left;
    stsd->text.defaultTextBox[2] = bottom;
    stsd->text.defaultTextBox[3] = right;
    }
  else if(quicktime_match_32(stsd->format, "tx3g"))
    {
    stsd->tx3g.defaultTextBox[0] = top;
    stsd->tx3g.defaultTextBox[1] = left;
    stsd->tx3g.defaultTextBox[2] = bottom;
    stsd->tx3g.defaultTextBox[3] = right;
    }
  trak->tkhd.track_width  = right - left;
  trak->tkhd.track_height = bottom - top;
  }

void lqt_get_text_box(quicktime_t * file, int track,
                      uint16_t * top, uint16_t * left,
                      uint16_t * bottom, uint16_t * right)
  {
  quicktime_trak_t      * trak;
  quicktime_stsd_table_t * stsd;
  trak = file->ttracks[track].track;
  stsd = &trak->mdia.minf.stbl.stsd.table[0];
  
  if(quicktime_match_32(stsd->format, "text"))
    {
    *top    = stsd->text.defaultTextBox[0];
    *left   = stsd->text.defaultTextBox[1];
    *bottom = stsd->text.defaultTextBox[2];
    *right  = stsd->text.defaultTextBox[3];
    }
  else if(quicktime_match_32(stsd->format, "tx3g"))
    {
    *top    = stsd->tx3g.defaultTextBox[0];
    *left   = stsd->tx3g.defaultTextBox[1];
    *bottom = stsd->tx3g.defaultTextBox[2];
    *right  = stsd->tx3g.defaultTextBox[3];
    }
  }
