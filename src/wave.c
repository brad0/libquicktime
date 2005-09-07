#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

uint8_t * quicktime_wave_get_user_atom(quicktime_trak_t * trak, char * name, uint32_t * len)
  {
  int i;
  quicktime_wave_t * wave = &(trak->mdia.minf.stbl.stsd.table[0].wave);

  for(i = 0; i < wave->num_user_atoms; i++)
    {
    if((wave->user_atoms[i][4] == name[0]) &&
       (wave->user_atoms[i][5] == name[1]) &&
       (wave->user_atoms[i][6] == name[2]) &&
       (wave->user_atoms[i][7] == name[3]))
      {
      *len =
        ((uint32_t)wave->user_atoms[i][0] << 24) |
        ((uint32_t)wave->user_atoms[i][1] << 16) |
        ((uint32_t)wave->user_atoms[i][2] <<  8) |
        wave->user_atoms[i][3];
      //      fprintf(stderr, "Getting user atom %.4s (%d bytes)\n", name, *len);
      return wave->user_atoms[i];
      }
    }
  return (uint8_t*)0;
  }

uint8_t * set_user_atom(quicktime_wave_t *wave, char * name, uint32_t len)
  {
  fprintf(stderr, "Setting user atom %.4s (%d bytes)\n", name, len);
  wave->user_atoms = realloc(wave->user_atoms, (wave->num_user_atoms+1)*sizeof(*wave->user_atoms));
  wave->user_atoms[wave->num_user_atoms] = malloc(len);

  wave->user_atoms[wave->num_user_atoms][0] = ((len) & 0xff000000) >> 24;
  wave->user_atoms[wave->num_user_atoms][1] = ((len) & 0x00ff0000) >> 16;
  wave->user_atoms[wave->num_user_atoms][2] = ((len) & 0x0000ff00) >>  8;
  wave->user_atoms[wave->num_user_atoms][3] = ((len) & 0x000000ff);

  fprintf(stderr, "Len: %02x %02x %02x %02x\n", 
          wave->user_atoms[wave->num_user_atoms][0],
          wave->user_atoms[wave->num_user_atoms][1],
          wave->user_atoms[wave->num_user_atoms][2],
          wave->user_atoms[wave->num_user_atoms][3]);
  
  wave->user_atoms[wave->num_user_atoms][4] = name[0];
  wave->user_atoms[wave->num_user_atoms][5] = name[1];
  wave->user_atoms[wave->num_user_atoms][6] = name[2];
  wave->user_atoms[wave->num_user_atoms][7] = name[3];
  wave->num_user_atoms++;
  return wave->user_atoms[wave->num_user_atoms-1];
  }

void quicktime_wave_set_user_atom(quicktime_trak_t * trak, char * name, uint8_t * data, uint32_t len)
  {
  uint8_t * ptr;
  quicktime_wave_t * wave = &(trak->mdia.minf.stbl.stsd.table[0].wave);

  ptr = set_user_atom(wave, name, len);
  memcpy(ptr + 8, data, len);
  trak->mdia.minf.stbl.stsd.table[0].has_wave = 1;
  }
  
void quicktime_wave_delete(quicktime_wave_t *wave)
  {
  int i;
  
  if(wave->user_atoms)
    {
    for(i = 0; i < wave->num_user_atoms; i++)
      {
      free(wave->user_atoms[i]);
      }
    free(wave->user_atoms);
    }
  }

void quicktime_read_wave(quicktime_t *file, quicktime_wave_t *wave,
                         quicktime_atom_t *wave_atom)
  {
  uint8_t * ptr;
  quicktime_atom_t leaf_atom;
  //  printf("quicktime_read_wave");
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
    else if(quicktime_atom_is(&leaf_atom, (char[]){ 0x00, 0x00, 0x00, 0x00 }))
      {
      break;
      }
    else /* Add to user atoms */
      {
      ptr = set_user_atom(wave, leaf_atom.type, leaf_atom.size);
      quicktime_read_data(file, ptr + 8, leaf_atom.size - 8);
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
  int i;
  uint32_t len;
  printf("       wave: \n");
  if(wave->has_frma)
    quicktime_frma_dump(&wave->frma);
  if(wave->has_enda)
    quicktime_enda_dump(&wave->enda);

  for(i = 0; i < wave->num_user_atoms; i++)
    {
    len =
      ((uint32_t)wave->user_atoms[i][0] << 24) |
      ((uint32_t)wave->user_atoms[i][1] << 16) |
      ((uint32_t)wave->user_atoms[i][2] <<  8) |
      wave->user_atoms[i][3];
    printf("         User atom %.4s (%d bytes)\n",
           wave->user_atoms[i] + 4,
           len);
    }
  }
