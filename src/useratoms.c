/*******************************************************************************
 useratoms.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2011 Members of the libquicktime project.

 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2.1 of the License, or (at your option)
 any later version.

 This library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this library; if not, write to the Free Software Foundation, Inc., 51
 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*******************************************************************************/ 

#include "lqt_private.h"
#include <stdlib.h>
#include <string.h>

uint8_t * quicktime_user_atoms_get_atom(quicktime_user_atoms_t * u,
                                        char * name, uint32_t * len)
  {
  int i;

  for(i = 0; i < u->num_atoms; i++)
    {
    if((u->atoms[i][4] == name[0]) &&
       (u->atoms[i][5] == name[1]) &&
       (u->atoms[i][6] == name[2]) &&
       (u->atoms[i][7] == name[3]))
      {
      *len =
        ((uint32_t)u->atoms[i][0] << 24) |
        ((uint32_t)u->atoms[i][1] << 16) |
        ((uint32_t)u->atoms[i][2] <<  8) |
        u->atoms[i][3];
      return u->atoms[i];
      }
    }
  return (uint8_t*)0;
  }

void quicktime_user_atoms_add_atom(quicktime_user_atoms_t * u,
                                   char * name, uint8_t * data,
                                   uint32_t len)
  {
  u->atoms =
    realloc(u->atoms, (u->num_atoms+1)*sizeof(*u->atoms));
  
  u->atoms[u->num_atoms] = malloc(len+8);
  
  u->atoms[u->num_atoms][0] = ((len+8) & 0xff000000) >> 24;
  u->atoms[u->num_atoms][1] = ((len+8) & 0x00ff0000) >> 16;
  u->atoms[u->num_atoms][2] = ((len+8) & 0x0000ff00) >>  8;
  u->atoms[u->num_atoms][3] = ((len+8) & 0x000000ff);

  u->atoms[u->num_atoms][4] = name[0];
  u->atoms[u->num_atoms][5] = name[1];
  u->atoms[u->num_atoms][6] = name[2];
  u->atoms[u->num_atoms][7] = name[3];

  memcpy(u->atoms[u->num_atoms]+8, data, len);
  u->num_atoms++;
  }

void quicktime_user_atoms_read_atom(quicktime_t * file,
                                    quicktime_user_atoms_t * u,
                                    quicktime_atom_t * leaf_atom)
  {
  u->atoms =
    realloc(u->atoms, (u->num_atoms+1)*sizeof(*u->atoms));
  u->atoms[u->num_atoms] = malloc(leaf_atom->size);

  u->atoms[u->num_atoms][0] = ((leaf_atom->size) & 0xff000000) >> 24;
  u->atoms[u->num_atoms][1] = ((leaf_atom->size) & 0x00ff0000) >> 16;
  u->atoms[u->num_atoms][2] = ((leaf_atom->size) & 0x0000ff00) >>  8;
  u->atoms[u->num_atoms][3] = ((leaf_atom->size) & 0x000000ff);

  u->atoms[u->num_atoms][4] = leaf_atom->type[0];
  u->atoms[u->num_atoms][5] = leaf_atom->type[1];
  u->atoms[u->num_atoms][6] = leaf_atom->type[2];
  u->atoms[u->num_atoms][7] = leaf_atom->type[3];

  quicktime_read_data(file, u->atoms[u->num_atoms] + 8, leaf_atom->size - 8);

  u->num_atoms++;
  }

void quicktime_user_atoms_delete(quicktime_user_atoms_t * u)
  {
  int i;
  if(u->atoms)
    {
    for(i = 0; i < u->num_atoms; i++)
      {
      free(u->atoms[i]);
      }
    free(u->atoms);
    }
  }

void quicktime_user_atoms_dump(quicktime_user_atoms_t * u)
  {
  int i;
  uint32_t len;
  
  for(i = 0; i < u->num_atoms; i++)
    {
    len =
      ((uint32_t)u->atoms[i][0] << 24) |
      ((uint32_t)u->atoms[i][1] << 16) |
      ((uint32_t)u->atoms[i][2] <<  8) |
      u->atoms[i][3];
    lqt_dump("         User atom %.4s (%d bytes)\n",
           u->atoms[i] + 4,
           len);
    }
  }


void quicktime_write_user_atoms(quicktime_t * file,
                                quicktime_user_atoms_t * u)
  {
  int i;
  uint32_t len;

  for(i = 0; i < u->num_atoms; i++)
    {
    len =
      ((uint32_t)u->atoms[i][0] << 24) |
      ((uint32_t)u->atoms[i][1] << 16) |
      ((uint32_t)u->atoms[i][2] <<  8) |
      u->atoms[i][3];
    
    quicktime_write_data(file, u->atoms[i], len);
    }
  
  }
