/*******************************************************************************
 schroedinger.h

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
#include <schroedinger/schro.h>
// #include <schroedinger/schrodebug.h>
#include <schroedinger/schrovideoformat.h>

#include "lqt_private.h"
#include <quicktime/lqt_codecapi.h>

#ifdef HAVE_SCHROEDINGER_SCHROVERSION_H
#include <schroedinger/schroversion.h>
#else
#define SCHRO_CHECK_VERSION(a,b,c) 0
#endif

void quicktime_init_codec_schroedinger(quicktime_codec_t * codec_base,
                                       quicktime_audio_map_t *atrack,
                                       quicktime_video_map_t *vtrack);

extern lqt_parameter_info_static_t encode_parameters_schroedinger[];

int lqt_schroedinger_set_enc_parameter(quicktime_t *file, 
                                       int track, 
                                       const char *key, 
                                       const void *value);

SchroChromaFormat lqt_schrodinger_get_chroma_format(int cmodel);

SchroSignalRange lqt_schrodinger_get_signal_range(int cmodel);


typedef struct
  {
  /* Decoder part */
  SchroDecoder * dec;
  SchroFrame * dec_frame;
  uint8_t * dec_buffer;
  uint8_t * dec_buffer_ptr;
  int dec_buffer_size;
  int dec_buffer_alloc;
  int dec_delay;
  int dec_eof;
  
  SchroFrameFormat frame_format;

  void (*dec_copy_frame)(quicktime_t * file,
                         unsigned char **row_pointers,
                         int track);

  /* Encoder part */
  SchroEncoder * enc;

  void (*enc_copy_frame)(quicktime_t * file,
                         unsigned char **row_pointers,
                         SchroFrame * frame,
                         int track);
  
  uint8_t * enc_buffer;
  int enc_buffer_alloc;
  int enc_buffer_size;

  int enc_eof; /* Don't write EOS twice */
  
  } schroedinger_codec_t;

int lqt_schroedinger_delete(quicktime_codec_t *codec_base);

int lqt_schrodinger_get_colormodel(SchroVideoFormat *format);
SchroFrameFormat lqt_schrodinger_get_frame_format(SchroVideoFormat *format);


int lqt_schroedinger_encode_video(quicktime_t *file,
                                  unsigned char **row_pointers,
                                  int track);

int lqt_schroedinger_decode_video(quicktime_t *file,
                                  unsigned char **row_pointers,
                                  int track);

void lqt_schroedinger_resync(quicktime_t *file, int track);
int lqt_schroedinger_flush(quicktime_t *file, int track);
