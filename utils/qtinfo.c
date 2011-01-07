/*******************************************************************************
 qtinfo.c

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

#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>
#include <stdio.h>
#include <locale.h>

#include "common.h"

#include <libintl.h>


#include <config.h> // ONLY for the PACKAGE macro. Usually, applications never need
                    // to include config.h

#define _(str) dgettext(PACKAGE, str)


static void
file_info(char *filename)
  {
  quicktime_t* qtfile;
  qtfile = quicktime_open(filename, 1, 0);
  
  if(!qtfile)
    {
    printf(_("Couldn't open %s\n"), filename);
    return;
    }
  quicktime_print_info(qtfile);
  quicktime_close(qtfile);
  }

int main(int argc, char *argv[])
{
	int i;
        setlocale(LC_MESSAGES, "");
        setlocale(LC_CTYPE, "");

        bindtextdomain(PACKAGE, LOCALE_DIR);
	if(argc < 2) {
		printf(_("Usage: %s filename...\n"), argv[0]);
		return 1;
	}

	for(i = 1; i < argc; i++) {
		file_info(argv[i]);
	}

	return 0;
}
