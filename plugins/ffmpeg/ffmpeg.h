/* 
   ffmpeg.h (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
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

#ifndef QUICKTIME_FFMPEG_H
#define QUICKTIME_FFMPEG_H

#include "ffmpeg/avcodec.h"
#include <quicktime/quicktime.h>

typedef struct
{
	AVCodecContext params;

	/* Compression stuff */
	AVCodecContext ffcodec_enc;
	AVCodec * ffc_enc;
	int init_enc;
	unsigned char * encode_frame;
	unsigned char * write_buffer;
	int write_buffer_size;
	
	/* DeCompression stuff */
	AVCodecContext ffcodec_dec;
	AVCodec * ffc_dec;
	int init_dec;
	unsigned char * read_buffer;
	int read_buffer_size;
	longest last_frame;
	
	/* Audio compression */
	short *ae_buf;
	int ae_pos;

	/* Audio decompression */
	short *ad_buf;
	int ad_pos;
} quicktime_ffmpeg_codec_t;

#endif

void quicktime_init_codec_ffmpeg(quicktime_video_map_t *vtrack, AVCodec *encoder, AVCodec *decoder);
