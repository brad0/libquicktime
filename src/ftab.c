#include "lqt_private.h"
#include <stdlib.h>
#include <string.h>

int quicktime_ftab_init(quicktime_ftab_t *ftab, int font_id, const char * font_name)
  {
  ftab->num_fonts = 1;
  ftab->fonts = calloc(ftab->num_fonts, sizeof(*ftab->fonts));
  ftab->fonts[0].font_id = font_id;
  strcpy(ftab->fonts[0].font_name, font_name);
  return 0;
  }

int quicktime_ftab_delete(quicktime_ftab_t *ftab)
  {
  if(ftab->fonts)
    free(ftab->fonts);
  return 0;
  }

void quicktime_ftab_dump(quicktime_ftab_t *ftab)
  {
  int i;
  lqt_dump("       font table (ftab)\n");
  lqt_dump("         num_fonts: %d\n", ftab->num_fonts);
  
  for(i = 0; i < ftab->num_fonts; i++)
    {
    lqt_dump("         Font %d, ID: %d, name: %s\n",
             i+1, ftab->fonts[i].font_id, ftab->fonts[i].font_name);
    }
  }

int quicktime_read_ftab(quicktime_t *file, quicktime_ftab_t *ftab)
  {
  int i;
  ftab->num_fonts = quicktime_read_int16(file);
  ftab->fonts = calloc(ftab->num_fonts, sizeof(*ftab->fonts));

  for(i = 0; i < ftab->num_fonts; i++)
    {
    ftab->fonts[i].font_id = quicktime_read_int16(file);
    quicktime_read_pascal(file, ftab->fonts[i].font_name);
    }
  return 0;
  }

void quicktime_write_ftab(quicktime_t *file, quicktime_ftab_t *ftab)
  {
  int i;
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "ftab");
  quicktime_write_int16(file, ftab->num_fonts);

  for(i = 0; i < ftab->num_fonts; i++)
    {
    quicktime_write_int16(file, ftab->fonts[i].font_id);
    quicktime_write_pascal(file, ftab->fonts[i].font_name);
    }
  quicktime_atom_write_footer(file, &atom);
  }
