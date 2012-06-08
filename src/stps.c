/*******************************************************************************
 stps.c

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

void quicktime_stps_init(quicktime_stps_t *stps)
  {
  stps->version = 0;
  stps->flags = 0;
  stps->total_entries = 0;
  stps->entries_allocated = 0;
  stps->table = NULL;
  }

void quicktime_stps_delete(quicktime_stps_t *stps)
  {
  if(stps->table) free(stps->table);
  stps->total_entries = 0;
  stps->entries_allocated = 0;
  stps->table = 0;
  }

void quicktime_stps_dump(quicktime_stps_t *stps)
  {
  int i;
  lqt_dump("     partial sync sample (stps)\n");
  lqt_dump("      version %d\n", stps->version);
  lqt_dump("      flags %ld\n", stps->flags);
  lqt_dump("      total_entries %ld\n", stps->total_entries);
  for(i = 0; i < stps->total_entries; i++)
    {
    lqt_dump("       sample %lx\n", stps->table[i].sample);
    }
  }

void quicktime_read_stps(quicktime_t *file, quicktime_stps_t *stps)
  {
  int i;
  stps->version = quicktime_read_char(file);
  stps->flags = quicktime_read_int24(file);
  stps->total_entries = quicktime_read_int32(file);

  if(stps->entries_allocated < stps->total_entries)
    {
    stps->entries_allocated = stps->total_entries;
    stps->table = (quicktime_stps_table_t*)realloc(stps->table, sizeof(quicktime_stps_table_t) * stps->entries_allocated);
    }

  for(i = 0; i < stps->total_entries; i++)
    {
    stps->table[i].sample = quicktime_read_int32(file);
    }
  }


void quicktime_write_stps(quicktime_t *file, quicktime_stps_t *stps)
  {
  int i;
  quicktime_atom_t atom;

  if(stps->total_entries)
    {
    quicktime_atom_write_header(file, &atom, "stps");

    quicktime_write_char(file, stps->version);
    quicktime_write_int24(file, stps->flags);
    quicktime_write_int32(file, stps->total_entries);
    for(i = 0; i < stps->total_entries; i++)
      {
      quicktime_write_int32(file, stps->table[i].sample);
      }

    quicktime_atom_write_footer(file, &atom);
    }
  }
