/*******************************************************************************
 stts.c

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

void lqt_packet_index_delete(lqt_packet_index_t * pi)
  {
  if(pi->entries)
    free(pi->entries);
  }

void lqt_packet_index_dump(const lqt_packet_index_t* pi)
  {
  int i;
  lqt_dump("     Packet index (generated)\n");
  lqt_dump("      num_entries %d\n", pi->num_entries);
  for(i = 0; i < pi->num_entries; i++)
    {
    lqt_dump("       pos %"PRId64" pts %"PRId64" dts %"PRId64" dur %"PRId64" sz %"PRId64" fl %08x\n",
             pi->entries[i].position,
             pi->entries[i].pts, pi->entries[i].dts,
             pi->entries[i].duration,
             pi->entries[i].size,
             pi->entries[i].flags);
    }
  }

void lqt_packet_index_alloc(lqt_packet_index_t * pi, int num)
  {
  pi->entries_alloc = num;

  pi->entries = realloc(pi->entries,
                        pi->entries_alloc * sizeof(*pi->entries));
  memset(pi->entries + pi->num_entries, 0,
         (pi->entries_alloc - pi->num_entries) * sizeof(*pi->entries));
  }

void lqt_packet_index_append(lqt_packet_index_t * pi,
                                   const lqt_packet_index_entry_t * e)
  {
  if(pi->num_entries >= pi->entries_alloc)
    lqt_packet_index_alloc(pi, pi->num_entries + 1024);
  
  memcpy(pi->entries + pi->num_entries,
         e, sizeof(*e));
  pi->num_entries++;
  }

