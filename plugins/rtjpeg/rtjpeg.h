/* 
   rtjpeg.h (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
   Based entirely upon qtpng.h from libquicktime 
   (http://libquicktime.sf.net).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    
*/

#ifndef QUICKTIME_RTJPEG_H
#define QUICKTIME_RTJPEG_H

#include "RTjpeg.h"
#include <quicktime/quicktime.h>

typedef struct
{
	/* Compression stuff */
	RTjpeg_t * compress_struct;
	unsigned char * encode_frame;
	unsigned char * encode_rows[3];
	unsigned char * write_buffer;
	int Q;
	int K;
	int LQ;
	int CQ;
	
	/* DeCompression stuff */
	RTjpeg_t * decompress_struct;
	unsigned char * decode_frame;
	unsigned char * decode_rows[3];
	unsigned char * read_buffer;
	int read_buffer_size;
} quicktime_rtjpeg_codec_t;

#endif

void quicktime_init_codec_rtjpeg(quicktime_video_map_t *vtrack);
