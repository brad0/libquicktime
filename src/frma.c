#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

void quicktime_read_frma(quicktime_t *file, quicktime_frma_t *frma,
                         quicktime_atom_t *frma_atom)
  {
  quicktime_read_data(file, (uint8_t*)frma->codec, 4);
  }

void quicktime_write_frma(quicktime_t *file, quicktime_frma_t *frma)
  {
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "frma");
  quicktime_write_data(file, (uint8_t*)frma->codec, 4);
  quicktime_atom_write_footer(file, &atom);
  }

void quicktime_frma_dump(quicktime_frma_t *frma)
  {
  printf("         frma: \n");
  printf("           codec: %c%c%c%c\n",
         frma->codec[0], frma->codec[1], frma->codec[2], frma->codec[3]);
  }

void quicktime_set_frma(quicktime_trak_t * trak, char * codec)
  {
  quicktime_wave_t * wave = &(trak->mdia.minf.stbl.stsd.table[0].wave);
  memcpy(wave->frma.codec, codec, 4);
  wave->has_frma = 1;
  trak->mdia.minf.stbl.stsd.table[0].has_wave = 1;
  }
