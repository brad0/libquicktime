#include <funcprotos.h>
#include <quicktime/quicktime.h>

/* Common functions used by most uncompressed video codecs */

void lqt_set_fiel_uncompressed(quicktime_t * file, int track)
  {
  quicktime_video_map_t *vtrack = &(file->vtracks[track]);
  quicktime_stsd_table_t *stsd;
  
  stsd = vtrack->track->mdia.minf.stbl.stsd.table;

  if(stsd->has_fiel)
    return;
  
  switch(vtrack->interlace_mode)
    {
    case LQT_INTERLACE_NONE:
      lqt_set_fiel(file, track, 1, 0);
      break;
    case LQT_INTERLACE_TOP_FIRST:
      lqt_set_fiel(file, track, 2, 9);
      break;
    case LQT_INTERLACE_BOTTOM_FIRST:
      lqt_set_fiel(file, track, 2, 14);
      break;
    }
  }
