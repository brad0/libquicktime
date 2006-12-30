#include <funcprotos.h>
#include <quicktime/quicktime.h>

#define MP4_DEFAULT_LANGUAGE 5575

void quicktime_mdhd_init(quicktime_mdhd_t *mdhd)
{
	mdhd->version = 0;
	mdhd->flags = 0;
	mdhd->creation_time = quicktime_current_time();
	mdhd->modification_time = quicktime_current_time();
	mdhd->time_scale = 0;
	mdhd->duration = 0;
	mdhd->language = 0;
	mdhd->quality = 100;
}

void quicktime_mdhd_init_video(quicktime_t *file, 
								quicktime_mdhd_t *mdhd,
								int timescale)
  {
  if(file->file_type & (LQT_FILE_MP4 | LQT_FILE_M4A))
    mdhd->language = MP4_DEFAULT_LANGUAGE;
  mdhd->time_scale = timescale;
  mdhd->duration = 0;      /* set this when closing */
  }

void quicktime_mdhd_init_audio(quicktime_t *file,
                               quicktime_mdhd_t *mdhd, 
                               int sample_rate)
  {
  if(file->file_type & (LQT_FILE_MP4 | LQT_FILE_M4A))
    mdhd->language = MP4_DEFAULT_LANGUAGE;
  mdhd->time_scale = sample_rate;
  mdhd->duration = 0;      /* set this when closing */
  }
  
void quicktime_mdhd_init_text(quicktime_t *file,
                              quicktime_mdhd_t *mdhd, 
                              int timescale)
  {
  if(file->file_type & (LQT_FILE_MP4 | LQT_FILE_M4A))
    mdhd->language = MP4_DEFAULT_LANGUAGE;
  
  mdhd->time_scale = timescale;
  mdhd->duration = 0;      /* set this when closing */
  }


void quicktime_mdhd_delete(quicktime_mdhd_t *mdhd)
  {
  }

void quicktime_read_mdhd(quicktime_t *file, quicktime_mdhd_t *mdhd)
{
	mdhd->version = quicktime_read_char(file);
	mdhd->flags = quicktime_read_int24(file);
	mdhd->creation_time = quicktime_read_int32(file);
	mdhd->modification_time = quicktime_read_int32(file);
	mdhd->time_scale = quicktime_read_int32(file);
	mdhd->duration = quicktime_read_int32(file);
	mdhd->language = quicktime_read_int16(file);
	mdhd->quality = quicktime_read_int16(file);
}

void quicktime_mdhd_dump(quicktime_mdhd_t *mdhd)
{
	lqt_dump("   media header\n");
	lqt_dump("    version %d\n", mdhd->version);
	lqt_dump("    flags %ld\n", mdhd->flags);
	lqt_dump("    creation_time %lu\n", mdhd->creation_time);
	lqt_dump("    modification_time %lu\n", mdhd->modification_time);
	lqt_dump("    time_scale %ld\n", mdhd->time_scale);
	lqt_dump("    duration %ld\n", mdhd->duration);
	lqt_dump("    language %d\n", mdhd->language);
	lqt_dump("    quality %d\n", mdhd->quality);
}

void quicktime_write_mdhd(quicktime_t *file, quicktime_mdhd_t *mdhd)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "mdhd");

	quicktime_write_char(file, mdhd->version);
	quicktime_write_int24(file, mdhd->flags);
	quicktime_write_int32(file, mdhd->creation_time);
	quicktime_write_int32(file, mdhd->modification_time);
	quicktime_write_int32(file, mdhd->time_scale);
	quicktime_write_int32(file, mdhd->duration);
	quicktime_write_int16(file, mdhd->language);
	quicktime_write_int16(file, mdhd->quality);	

	quicktime_atom_write_footer(file, &atom);
}

