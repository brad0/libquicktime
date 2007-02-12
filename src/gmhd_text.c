#include "lqt_private.h"

#define LOG_DOMAIN "gmhd_text"

/* No idea, what these values mean, they are taken from
   the hexdump of a Quicktime file.
   
   It's not even clear, if they are organized as 32 bit
   integers or something else :)
*/


void quicktime_gmhd_text_init(quicktime_gmhd_text_t *gmhd_text)
  {
  gmhd_text->unk[0] = 0x00010000;
  gmhd_text->unk[1] = 0x00000000;
  gmhd_text->unk[2] = 0x00000000;
  gmhd_text->unk[3] = 0x00000000;
  gmhd_text->unk[4] = 0x00010000;
  gmhd_text->unk[5] = 0x00000000;
  gmhd_text->unk[6] = 0x00000000;
  gmhd_text->unk[7] = 0x00000000;
  gmhd_text->unk[8] = 0x40000000;
  }
     
void quicktime_gmhd_text_delete(quicktime_gmhd_text_t *gmhd_text)
  {

  }

void quicktime_gmhd_text_dump(quicktime_gmhd_text_t *gmhd_text)
  {
  int i;
  lqt_dump("     gmhd text atom (no idea what this is)\n");

  for(i = 0; i < 9; i++)
    {
    lqt_dump("       Unknown %d: 0x%08x\n", i, gmhd_text->unk[i]);
    }
  }

void quicktime_read_gmhd_text(quicktime_t *file,
                              quicktime_gmhd_text_t *gmhd_text,
                              quicktime_atom_t *parent_atom)
  {
  int i;
  for(i = 0; i < 9; i++)
    gmhd_text->unk[i] = quicktime_read_int32(file);
  if(quicktime_position(file) < parent_atom->end)
    {
    lqt_log(file, LQT_LOG_WARNING, LOG_DOMAIN, "More than 36 bytes in the gmhd text atom\n");
    quicktime_atom_skip(file, parent_atom);
    }
  }

void quicktime_write_gmhd_text(quicktime_t *file,
                               quicktime_gmhd_text_t *gmhd_text)
  {
  int i;
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "text");
  for(i = 0; i < 9; i++)
    {
    quicktime_write_int32(file, gmhd_text->unk[i]);
    }
  quicktime_atom_write_footer(file, &atom);
  }
