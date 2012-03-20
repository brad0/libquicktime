/*******************************************************************************
 libmjpeg.h

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
 
#ifndef LIBMJPEG_H
#define LIBMJPEG_H

/* use a thread to run compressors / decompressors */
// #define USE_THREAD 0

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

#define MAXFIELDS 2
#define QUICKTIME_MJPA_MARKSIZE 40
#define QUICKTIME_JPEG_TAG 0x6d6a7067



  struct mjpeg_error_mgr {
  struct jpeg_error_mgr pub;	/* "public" fields */
  jmp_buf setjmp_buffer;	/* for return to caller */
    };

  typedef struct mjpeg_error_mgr* mjpeg_error_ptr;

  // The compressor structure is shared between decompressors and compressors
  typedef struct
    {
    void *mjpeg;
    //    int instance;
    unsigned char *output_buffer;    /* Buffer for MJPEG output */
    long output_size;     /* Size of image stored in buffer */
    long output_allocated;    /* Allocated size of output buffer */
    struct jpeg_decompress_struct jpeg_decompress;
    struct jpeg_compress_struct jpeg_compress;
    struct mjpeg_error_mgr jpeg_error;
    int error;
    // Pointer to uncompressed YUV rows
    // [3 planes][downsampled rows][downsampled pixels]
    unsigned char **rows[3];
    /* Temp rows for each MCU */
    unsigned char **mcu_rows[3];
    /* Height of the field (padded value) */
    int field_h; 

    /* Output heights of the fields (can be different for both fields) */
    int field_heights[MAXFIELDS];
    
    } mjpeg_compressor;

  typedef struct
    {
    // Dimensions of user frame buffer
    int output_w;
    int output_h;
    // Dimensions for encoder frame buffer
    int coded_w, coded_w_uv, coded_h;
    int fields;
    int quality;
    int use_float;
    // Color model of compressed data.
    int jpeg_color_model;
    // Error in compression process
    int error;

    mjpeg_compressor * compressor;
    mjpeg_compressor * decompressor;

    // Temp frame for interlacing
    // [3 planes][downsampled rows][downsampled pixels]
    unsigned char *temp_data;
    unsigned char **temp_rows[3];
    //	unsigned char *y_argument, *u_argument, *v_argument;

    // Buffer passed to user
    // This can contain one progressive field or two fields with headers
    unsigned char *output_data;
    long output_size;
    long output_allocated;
    // Offset to field2 in output_data
    long output_field2;
    // Buffer passed from user
    unsigned char *input_data;
    long input_size;
    // Offset to field2 in input_data
    long input_field2;
    int rowspan, rowspan_uv;

    // Bottom first needs special treatment
    int bottom_first;
        
    } mjpeg_t;





  // Entry points
  mjpeg_t* mjpeg_new(int w, int h, int fields, int cmodel);
  void mjpeg_delete(mjpeg_t *mjpeg);

  void mjpeg_set_quality(mjpeg_t *mjpeg, int quality);
  void mjpeg_set_float(mjpeg_t *mjpeg, int use_float);
  // This is useful when producing realtime NTSC output for a JPEG board.
  void mjpeg_set_rowspan(mjpeg_t *mjpeg, int rowspan, int rowspan_uv);


  int mjpeg_get_fields(mjpeg_t *mjpeg);

  int mjpeg_decompress(mjpeg_t *mjpeg, 
                       unsigned char *buffer, 
                       long buffer_len,
                       long input_field2);

  void mjpeg_get_frame(mjpeg_t * mjpeg, uint8_t ** row_pointers);


  int mjpeg_compress(mjpeg_t *mjpeg, 
                     unsigned char **row_pointers);

  // Get buffer information after compressing
  unsigned char* mjpeg_output_buffer(mjpeg_t *mjpeg);
  long mjpeg_output_field2(mjpeg_t *mjpeg);
  long mjpeg_output_size(mjpeg_t *mjpeg);

  // Calculate marker contents and insert them into a buffer.
  // Reallocates the buffer if it isn't big enough so make sure it's big enough
  // when passing VFrames.
  void mjpeg_insert_quicktime_markers(unsigned char **buffer, 
                                      long *buffer_size, 
                                      long *buffer_allocated,
                                      int fields,
                                      long *field2_offset);

  // Get the second field offset from the markers
  long mjpeg_get_quicktime_field2(unsigned char *buffer, long buffer_size);

#ifdef __cplusplus
}
#endif

#endif
