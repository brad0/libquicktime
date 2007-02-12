#include "lqt_private.h"

void quicktime_delete_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl)
{
	int i;
	for(i = 0; i < file->moov.total_tracks; i++)
	{
		quicktime_delete_strl(hdrl->strl[i]);
	}
}


void quicktime_read_hdrl(quicktime_t *file, 
	quicktime_hdrl_t *hdrl,
	quicktime_atom_t *parent_atom)
{
	quicktime_atom_t leaf_atom;
	uint8_t data[4];
	int current_track = 0;

	do
	{
		quicktime_atom_read_header(file, &leaf_atom);
		if(quicktime_atom_is(&leaf_atom, "avih"))
		{
                quicktime_read_avih(file, &hdrl->avih, &leaf_atom);
                }

/* Got LIST */
		else if(quicktime_atom_is(&leaf_atom, "LIST"))
		{
			data[0] = data[1] = data[2] = data[3] = 0;
			quicktime_read_data(file, data, 4);

/* Got strl */
			if(quicktime_match_32(data, "strl"))
			{
				quicktime_strl_t *strl = 
					hdrl->strl[current_track++] = 
					quicktime_new_strl();
				quicktime_read_strl(file, strl, &leaf_atom);
				quicktime_strl_2_qt(file, strl);
			}
		}

		quicktime_atom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < parent_atom->end);

	quicktime_atom_skip(file, &leaf_atom);
}

void quicktime_init_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl)
  {
  int i;
  int current_track;

  // LIST 'hdrl'
  quicktime_atom_write_header(file, &hdrl->atom, "LIST");
  quicktime_write_char32(file, "hdrl");

  // avih
  hdrl->avih_offset = quicktime_position(file);

  quicktime_avih_init(&hdrl->avih, file);
  quicktime_write_avih(file, &hdrl->avih);
        


  /* Write stream lists. */
  /* Need the track maps to get the WAV ID for audio. */
  current_track = 0;
  for(i = 0; i < file->total_vtracks; i++)
    {
    quicktime_video_map_t *video_map = &file->vtracks[i];
    quicktime_trak_t *trak = video_map->track;
    quicktime_strl_t *strl = 
      hdrl->strl[current_track++] = 
      quicktime_new_strl();
    quicktime_init_strl(file, 
                        0, 
                        video_map, 
                        trak,
                        strl);
    }

  for(i = 0; i < file->total_atracks; i++)
    {
    quicktime_audio_map_t *audio_map = &file->atracks[i];
    quicktime_trak_t *trak = audio_map->track;
    quicktime_strl_t *strl = 
      hdrl->strl[current_track++] = 
      quicktime_new_strl();
    quicktime_init_strl(file, 
                        audio_map,
                        0,
                        trak,
                        strl);
    }
    

  /* ODML header */

  quicktime_init_odml(file, hdrl);
  quicktime_atom_write_footer(file, &hdrl->atom);
  }

void quicktime_finalize_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl)
  {
  int i;
  int64_t position = quicktime_position(file);
  double frame_rate = 0;
  int64_t total_frames;
  
  for(i = 0; i < file->moov.total_tracks; i++)
    {
    quicktime_trak_t *trak = file->moov.trak[i];
    quicktime_strl_t *strl = trak->strl;

    quicktime_finalize_strl(file, trak, strl);
    }

  if(file->total_vtracks)
    {
    total_frames = quicktime_video_length(file, 0);
    frame_rate   = quicktime_frame_rate(file, 0);

    //    hdrl->avih.dwMaxBytesPerSec = file->total_length / (total_frames / frame_rate);
    hdrl->avih.dwTotalFrames = total_frames;

    quicktime_set_position(file, hdrl->avih_offset);
    quicktime_write_avih(file, &hdrl->avih);
    }
  quicktime_set_position(file, position);
  }







