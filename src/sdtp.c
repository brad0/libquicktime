/*******************************************************************************
 sdtp.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

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

void quicktime_sdtp_init(quicktime_sdtp_t *sdtp)
  {
  sdtp->version = 0;
  sdtp->flags = 0;
  sdtp->total_entries = 0;
  sdtp->entries_allocated = 0;
  sdtp->table = NULL;
  }

void quicktime_sdtp_delete(quicktime_sdtp_t *sdtp)
  {
  if(sdtp->table) free(sdtp->table);
  sdtp->total_entries = 0;
  sdtp->entries_allocated = 0;
  sdtp->table = 0;
  }

void quicktime_sdtp_dump(quicktime_sdtp_t *sdtp)
  {
  long i;
  lqt_dump("     sample dependencies (sdtp)\n");
  lqt_dump("      version %d\n", sdtp->version);
  lqt_dump("      flags %ld\n", sdtp->flags);
  lqt_dump("      total_entries %ld\n", sdtp->total_entries);
  for(i = 0; i < sdtp->total_entries; i++)
    {
    unsigned flags = sdtp->table[i];
    char separator = ':';
    lqt_dump("       sample %ld: flags 0x%02x", i, flags);
    if(flags & 1)
      {
      lqt_dump("%c has_redundant_coding", separator);
      separator = ',';
      }
    if(flags & 2)
      {
      lqt_dump("%c no_redundant_coding", separator);
      separator = ',';
      }
    if(flags & 4)
      {
      lqt_dump("%c other_samples_depend_on_this_one", separator);
      separator = ',';
      }
    if(flags & 8)
      {
      lqt_dump("%c no_other_samples_depend_on_this_one", separator);
      separator = ',';
      }
    if(flags & 16)
      {
      lqt_dump("%c sample_depends_on_others", separator);
      separator = ',';
      }
    if(flags & 32)
      {
      lqt_dump("%c sample_doesnt_depend_on_others", separator);
      separator = ',';
      }
    if(flags & 64)
      {
      lqt_dump("%c earlier_display_times_allowed", separator);
      separator = ',';
      }
    if(flags & 128)
      {
      lqt_dump("%c reserved_bit_7", separator);
      separator = ',';
      }
    lqt_dump("\n");
    }
  }

void quicktime_read_sdtp(quicktime_t *file, quicktime_sdtp_t *sdtp, long num_entries)
  {
  sdtp->version = quicktime_read_char(file);
  sdtp->flags = quicktime_read_int24(file);
  sdtp->total_entries = num_entries;

  if(sdtp->entries_allocated < sdtp->total_entries)
    {
    sdtp->entries_allocated = sdtp->total_entries;
    sdtp->table = (uint8_t*)realloc(sdtp->table, sdtp->entries_allocated);
    }

  memset(sdtp->table, 0, sdtp->total_entries);
  quicktime_read_data(file, sdtp->table, sdtp->total_entries);
  }

void quicktime_write_sdtp(quicktime_t *file, quicktime_sdtp_t *sdtp)
  {
  quicktime_atom_t atom;

  if(sdtp->total_entries)
    {
    quicktime_atom_write_header(file, &atom, "sdtp");

    quicktime_write_char(file, sdtp->version);
    quicktime_write_int24(file, sdtp->flags);
    quicktime_write_data(file, sdtp->table, sdtp->total_entries);
    quicktime_atom_write_footer(file, &atom);
    }
  }
