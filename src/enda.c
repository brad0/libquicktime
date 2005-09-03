#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

void quicktime_read_enda(quicktime_t *file, quicktime_enda_t *enda,
                         quicktime_atom_t *enda_atom)
  {
  enda->littleEndian = quicktime_read_int16(file);
  }

void quicktime_write_enda(quicktime_t *file, quicktime_enda_t *enda)
  {
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "enda");
  quicktime_write_int16(file, !!enda->littleEndian);
  quicktime_atom_write_footer(file, &atom);
  }

void quicktime_set_enda(quicktime_trak_t * trak, int little_endian)
  {
  quicktime_wave_t * wave = &(trak->mdia.minf.stbl.stsd.table[0].wave);
  wave->enda.littleEndian = little_endian;
  wave->has_enda = 1;
  }

void quicktime_enda_dump(quicktime_enda_t *enda)
  {
  printf("         enda: \n");
  printf("           littleEndian: %d\n", enda->littleEndian);
  }

/* Returns TRUE if little endian */
int quicktime_get_enda(quicktime_trak_t * trak)
  {
  quicktime_wave_t * wave = &(trak->mdia.minf.stbl.stsd.table[0].wave);
  if(wave->has_enda)
    {
    return wave->enda.littleEndian;
    }
  else
    return 0;
  }
