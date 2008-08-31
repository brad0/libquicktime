/*******************************************************************************
 tmcd.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2007 Members of the libquicktime project.

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

#include <string.h>

void quicktime_tmcd_init(quicktime_tmcd_t *tmcd)
  {
  quicktime_tcmi_init(&tmcd->tcmi);
  }

void quicktime_tmcd_delete(quicktime_tmcd_t *tmcd)
  {
  quicktime_tcmi_delete(&tmcd->tcmi);
  }

void quicktime_tmcd_dump(quicktime_tmcd_t *tmcd)
  {
  lqt_dump("       tmcd\n");

  quicktime_tcmi_dump(&tmcd->tcmi);
  }

void quicktime_read_tmcd(quicktime_t *file, quicktime_tmcd_t *tmcd,
                         quicktime_atom_t *parent_atom)
  {
  quicktime_atom_t leaf_atom;

  do
    {
    quicktime_atom_read_header(file, &leaf_atom);
	       
    if(quicktime_atom_is(&leaf_atom, "tcmi"))
      {
      quicktime_read_tcmi(file, &tmcd->tcmi);
      }
    else
      quicktime_atom_skip(file, &leaf_atom);

    }while(quicktime_position(file) < parent_atom->end);
  }

void quicktime_write_tmcd(quicktime_t *file, quicktime_tmcd_t *tmcd)
  {
  quicktime_atom_t atom;

  quicktime_atom_write_header(file, &atom, "tmcd");
  quicktime_write_tcmi(file, &tmcd->tcmi);
  quicktime_atom_write_footer(file, &atom);
  }
