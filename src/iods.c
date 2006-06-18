#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <stdlib.h>
#include <string.h>

void quicktime_iods_init(quicktime_iods_t * iods)
  {
  iods->version              = 0;
  iods->flags                = 0;
  iods->ObjectDescriptorID   = 0x004F;
  iods->ODProfileLevel       = 0xff;
  iods->sceneProfileLevel    = 0xff;
  iods->audioProfileId       = 0xff;
  iods->videoProfileId       = 0xff;
  iods->graphicsProfileLevel = 0xff;

  }

void quicktime_iods_delete(quicktime_iods_t * iods)
  {
  if(iods->tracks)
    free(iods->tracks);
  }

void quicktime_iods_dump(quicktime_iods_t * iods)
  {
  
  }

void quicktime_iods_add_track(quicktime_iods_t * iods, quicktime_trak_t * trak)
  {
  iods->tracks = realloc(iods->tracks, sizeof(*iods->tracks)*(iods->num_tracks+1));
  iods->tracks[iods->num_tracks].ES_ID_IncTag = 0x0e;
  iods->tracks[iods->num_tracks].length       = 0x04;
  iods->tracks[iods->num_tracks].track_id     = trak->tkhd.track_id;
  }

void quicktime_read_iods(quicktime_t *file, quicktime_iods_t * iods)
  {
  iods->version = quicktime_read_char(file);
  iods->flags = quicktime_read_int24(file);
  quicktime_read_char(file); /* skip tag */
  quicktime_read_mp4_descr_length(file);  /* skip length */
  /* skip ODID, ODProfile, sceneProfile */
  iods->ObjectDescriptorID = quicktime_read_int16(file);
  iods->ODProfileLevel     = quicktime_read_char(file);
  iods->sceneProfileLevel  = quicktime_read_char(file);
  iods->audioProfileId = quicktime_read_char(file);
  iods->videoProfileId = quicktime_read_char(file);
  iods->graphicsProfileLevel  = quicktime_read_char(file);
  /* will skip the remainder of the atom */
  }

void quicktime_write_iods(quicktime_t *file, quicktime_iods_t * iods)
  {
  quicktime_atom_t atom;
  int i;

  quicktime_atom_write_header(file, &atom, "iods");

  quicktime_write_char(file, iods->version);
  quicktime_write_int24(file, iods->flags);

  quicktime_write_char(file, 0x10);       /* MP4_IOD_Tag */
  quicktime_write_mp4_descr_length(file,
                                   7 + (file->moov.total_tracks * (1+1+4)), 0);    /* length */
  quicktime_write_int16(file, 0x004F); /* ObjectDescriptorID = 1 */
  quicktime_write_char(file, iods->ODProfileLevel);       /* ODProfileLevel */
  quicktime_write_char(file, iods->sceneProfileLevel);       /* sceneProfileLevel */
  quicktime_write_char(file, iods->audioProfileId);       /* audioProfileLevel */
  quicktime_write_char(file, iods->videoProfileId);       /* videoProfileLevel */
  quicktime_write_char(file, iods->graphicsProfileLevel);       /* graphicsProfileLevel */
  
  for (i = 0; i < file->moov.total_tracks; i++)
    {
    quicktime_write_char(file, 0x0E);       /* ES_ID_IncTag */
    quicktime_write_char(file, 0x04);       /* length */
    quicktime_write_int32(file, file->moov.trak[i]->tkhd.track_id);
    }
  
  /* no OCI_Descriptors */
  /* no IPMP_DescriptorPointers */
  /* no Extenstion_Descriptors */
  quicktime_atom_write_footer(file, &atom);
  }
