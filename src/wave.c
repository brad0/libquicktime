#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

void quicktime_read_wave(quicktime_t *file, quicktime_wave_t *wave,
                         quicktime_atom_t *wave_atom)
  {
  quicktime_atom_t leaf_atom;
  printf("quicktime_read_wave");
  do{
    quicktime_atom_read_header(file, &leaf_atom);
    
    if(quicktime_atom_is(&leaf_atom, "frma"))
      {
      quicktime_read_frma(file, &(wave->frma), &leaf_atom);
      }
    else if(quicktime_atom_is(&leaf_atom, "enda"))
      {
      quicktime_read_enda(file, &(wave->enda), &leaf_atom);
      wave->has_enda = 1;
      }
    quicktime_atom_skip(file, &leaf_atom);

  }while(quicktime_position(file) < wave_atom->end);

  }

void quicktime_write_wave(quicktime_t *file, quicktime_wave_t *wave)
  {
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "wave");

  quicktime_write_frma(file, &wave->frma);
  if(wave->has_enda)
    quicktime_write_enda(file, &wave->enda);
  
  quicktime_atom_write_footer(file, &atom);
  }

void quicktime_wave_dump(quicktime_wave_t *wave)
  {
  printf("       wave: \n");
  quicktime_frma_dump(&wave->frma);
  quicktime_enda_dump(&wave->enda);
  
  }
