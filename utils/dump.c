/*******************************************************************************
 dump.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2010 Members of the libquicktime project.

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
#include <stdio.h>

int main(int argc, char *argv[])
{
	quicktime_t *file;
	
	if(argc < 2)
	{
		printf("Dump all tables in movie.\n");
		exit(1);
	}

	if(!(file = quicktime_open(argv[1], 1, 0)))
	{
		printf("Open failed\n");
		exit(1);
	}

	quicktime_dump(file);

	quicktime_close(file);
	exit (EXIT_SUCCESS);
}
