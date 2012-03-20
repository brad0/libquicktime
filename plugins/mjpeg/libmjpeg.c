/*******************************************************************************
 libmjpeg.c

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
#include "libmjpeg.h"
#include <quicktime/colormodels.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define LOG_DOMAIN "libmjpeg"

/* JPEG MARKERS */
#define   M_SOF0    0xc0
#define   M_SOF1    0xc1
#define   M_SOF2    0xc2
#define   M_SOF3    0xc3
#define   M_SOF5    0xc5
#define   M_SOF6    0xc6
#define   M_SOF7    0xc7
#define   M_JPG     0xc8
#define   M_SOF9    0xc9
#define   M_SOF10   0xca
#define   M_SOF11   0xcb
#define   M_SOF13   0xcd
#define   M_SOF14   0xce
#define   M_SOF15   0xcf
#define   M_DHT     0xc4
#define   M_DAC     0xcc
#define   M_RST0    0xd0
#define   M_RST1    0xd1
#define   M_RST2    0xd2
#define   M_RST3    0xd3
#define   M_RST4    0xd4
#define   M_RST5    0xd5
#define   M_RST6    0xd6
#define   M_RST7    0xd7
#define   M_SOI     0xd8
#define   M_EOI     0xd9
#define   M_SOS     0xda
#define   M_DQT     0xdb
#define   M_DNL     0xdc
#define   M_DRI     0xdd
#define   M_DHP     0xde
#define   M_EXP     0xdf
#define   M_APP0    0xe0
#define   M_APP1    0xe1
#define   M_APP2    0xe2
#define   M_APP3    0xe3
#define   M_APP4    0xe4
#define   M_APP5    0xe5
#define   M_APP6    0xe6
#define   M_APP7    0xe7
#define   M_APP8    0xe8
#define   M_APP9    0xe9
#define   M_APP10   0xea
#define   M_APP11   0xeb
#define   M_APP12   0xec
#define   M_APP13   0xed
#define   M_APP14   0xee
#define   M_APP15   0xef
#define   M_JPG0    0xf0
#define   M_JPG13   0xfd
#define   M_COM     0xfe
#define   M_TEM     0x01
#define   M_ERROR   0x100

#define QUICKTIME_MARKER_SIZE 0x2c
#define QUICKTIME_JPEG_TAG 0x6d6a7067


METHODDEF(void) mjpeg_error_exit (j_common_ptr cinfo)
  {
  /* cinfo->err really points to a mjpeg_error_mgr struct, so coerce pointer */
  mjpeg_error_ptr mjpegerr = (mjpeg_error_ptr) cinfo->err;

  /* Always display the message. */
  /* We could postpone this until after returning, if we chose. */
  (*cinfo->err->output_message) (cinfo);

  /* Return control to the setjmp point */
  longjmp(mjpegerr->setjmp_buffer, 1);
  }

typedef struct 
  {
  struct jpeg_destination_mgr pub; /* public fields */

  JOCTET *buffer;		/* Pointer to buffer */
  mjpeg_compressor *engine;
  } mjpeg_destination_mgr;

typedef mjpeg_destination_mgr *mjpeg_dest_ptr;


/*
 * Initialize destination --- called by jpeg_start_compress
 * before any data is actually written.
 */

METHODDEF(void) init_destination(j_compress_ptr cinfo)
  {
  mjpeg_dest_ptr dest = (mjpeg_dest_ptr)cinfo->dest;

  /* Set the pointer to the preallocated buffer */
  if(!dest->engine->output_buffer)
    {
    dest->engine->output_buffer = lqt_bufalloc(65536);
    dest->engine->output_allocated = 65536;
    }
  dest->buffer = dest->engine->output_buffer;
  dest->pub.next_output_byte = dest->engine->output_buffer;
  dest->pub.free_in_buffer = dest->engine->output_allocated;
  }

/*
 * Empty the output buffer --- called whenever buffer fills up.
 *
 * In typical applications, this should write the entire output buffer
 * (ignoring the current state of next_output_byte & free_in_buffer),
 * reset the pointer & count to the start of the buffer, and return TRUE
 * indicating that the buffer has been dumped.
 *
 * In applications that need to be able to suspend compression due to output
 * overrun, a FALSE return indicates that the buffer cannot be emptied now.
 * In this situation, the compressor will return to its caller (possibly with
 * an indication that it has not accepted all the supplied scanlines).  The
 * application should resume compression after it has made more room in the
 * output buffer.  Note that there are substantial restrictions on the use of
 * suspension --- see the documentation.
 *
 * When suspending, the compressor will back up to a convenient restart point
 * (typically the start of the current MCU). next_output_byte & free_in_buffer
 * indicate where the restart point will be if the current call returns FALSE.
 * Data beyond this point will be regenerated after resumption, so do not
 * write it out when emptying the buffer externally.
 */

METHODDEF(boolean) empty_output_buffer(j_compress_ptr cinfo)
  {
  /* Allocate a bigger buffer. */
  mjpeg_dest_ptr dest = (mjpeg_dest_ptr)cinfo->dest;

  dest->engine->output_size = dest->engine->output_allocated;
  dest->engine->output_allocated *= 2;
  dest->engine->output_buffer = realloc(dest->engine->output_buffer, 
                                        dest->engine->output_allocated);
  dest->buffer = dest->engine->output_buffer;
  dest->pub.next_output_byte = dest->buffer + dest->engine->output_size;
  dest->pub.free_in_buffer = dest->engine->output_allocated - dest->engine->output_size;

  return TRUE;
  }

/*
 * Terminate destination --- called by jpeg_finish_compress
 * after all data has been written.  Usually needs to flush buffer.
 *
 * NB: *not* called by jpeg_abort or jpeg_destroy; surrounding
 * application must deal with any cleanup that should happen even
 * for error exit.
 */
METHODDEF(void) term_destination(j_compress_ptr cinfo)
  {
  /* Just get the length */
  mjpeg_dest_ptr dest = (mjpeg_dest_ptr)cinfo->dest;
  dest->engine->output_size = dest->engine->output_allocated - dest->pub.free_in_buffer;
  }

static GLOBAL(void) jpeg_buffer_dest(j_compress_ptr cinfo, mjpeg_compressor *engine)
  {
  mjpeg_dest_ptr dest;

  /* The destination object is made permanent so that multiple JPEG images
   * can be written to the same file without re-executing jpeg_stdio_dest.
   * This makes it dangerous to use this manager and a different destination
   * manager serially with the same JPEG object, because their private object
   * sizes may be different.  Caveat programmer.
   */
  if(cinfo->dest == NULL) 
    {	
    /* first time for this JPEG object? */
    cinfo->dest = (struct jpeg_destination_mgr *)
      (*cinfo->mem->alloc_small)((j_common_ptr)cinfo, 
                                 JPOOL_PERMANENT,
                                 sizeof(mjpeg_destination_mgr));
    }

  dest = (mjpeg_dest_ptr)cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->engine = engine;
  }


typedef struct {
struct jpeg_source_mgr pub;	/* public fields */

JOCTET * buffer;		/* start of buffer */
int bytes;             /* total size of buffer */
  } mjpeg_source_mgr;

typedef mjpeg_source_mgr* mjpeg_src_ptr;

METHODDEF(void) init_source(j_decompress_ptr cinfo)
  {
  }

METHODDEF(boolean) fill_input_buffer(j_decompress_ptr cinfo)
  {
  mjpeg_src_ptr src = (mjpeg_src_ptr) cinfo->src;

  src->buffer[0] = (JOCTET)0xFF;
  src->buffer[1] = (JOCTET)M_EOI;
  src->pub.next_input_byte = src->buffer;
  src->pub.bytes_in_buffer = 2;

  
  return TRUE;
  }


METHODDEF(void) skip_input_data(j_decompress_ptr cinfo, long num_bytes)
  {
  mjpeg_src_ptr src = (mjpeg_src_ptr)cinfo->src;

  src->pub.next_input_byte += (size_t)num_bytes;
  src->pub.bytes_in_buffer -= (size_t)num_bytes;
  }


METHODDEF(void) term_source(j_decompress_ptr cinfo)
  {
  }

static GLOBAL(void) jpeg_buffer_src(j_decompress_ptr cinfo, unsigned char *buffer, long bytes)
  {
  mjpeg_src_ptr src;

  /* first time for this JPEG object? */
  if(cinfo->src == NULL)
    {	
    cinfo->src = (struct jpeg_source_mgr*)
      (*cinfo->mem->alloc_small)((j_common_ptr)cinfo, 
                                 JPOOL_PERMANENT,
                                 sizeof(mjpeg_source_mgr));
    src = (mjpeg_src_ptr)cinfo->src;
    }

  src = (mjpeg_src_ptr)cinfo->src;
  src->pub.init_source = init_source;
  src->pub.fill_input_buffer = fill_input_buffer;
  src->pub.skip_input_data = skip_input_data;
  src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
  src->pub.term_source = term_source;
  src->pub.bytes_in_buffer = bytes;
  src->pub.next_input_byte = buffer;
  src->buffer = buffer;
  src->bytes = bytes;
  }



/*
 *  Buffer handling for variable size output buffers
 */

static void reset_buffer(unsigned char **buffer, long *size, long *allocated)
  {
  *size = 0;
  }

static void delete_buffer(unsigned char **buffer, long *size, long *allocated)
  {
  if(*buffer)
    {
    free(*buffer);
    *size = 0;
    *allocated = 0;
    }
  }

static void append_buffer(unsigned char **buffer, 
                          long *size, 
                          long *allocated,
                          unsigned char *data,
                          long data_size)
  {
  if(!*buffer)
    {
    *buffer = lqt_bufalloc(65536);
    *size = 0;
    *allocated = 65536;
    }

  if(*size + data_size > *allocated)
    {
    *allocated = *size + data_size;
    *buffer = realloc(*buffer, *allocated);
    }

  memcpy(*buffer + *size, data, data_size);
  *size += data_size;
  }

static void allocate_temps(mjpeg_t *mjpeg)
  {
  int i;
  
  if(!mjpeg->temp_data)
    {
    switch(mjpeg->jpeg_color_model)
      {
      case BC_YUVJ422P:
        mjpeg->temp_data = lqt_bufalloc(mjpeg->coded_w * mjpeg->coded_h * 2);
        mjpeg->temp_rows[0] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h);
        mjpeg->temp_rows[1] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h);
        mjpeg->temp_rows[2] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h);
        for(i = 0; i < mjpeg->coded_h; i++)
          {
          mjpeg->temp_rows[0][i] = mjpeg->temp_data + i * mjpeg->coded_w;
          mjpeg->temp_rows[1][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + i * mjpeg->coded_w / 2;
          mjpeg->temp_rows[2][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + mjpeg->coded_w / 2 * mjpeg->coded_h + i * mjpeg->coded_w / 2;
          }
        break;
        
      case BC_YUVJ444P:
        mjpeg->temp_data = lqt_bufalloc(mjpeg->coded_w * mjpeg->coded_h * 3);
        mjpeg->temp_rows[0] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h);
        mjpeg->temp_rows[1] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h);
        mjpeg->temp_rows[2] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h);
        for(i = 0; i < mjpeg->coded_h; i++)
          {
          mjpeg->temp_rows[0][i] = mjpeg->temp_data + i * mjpeg->coded_w;
          mjpeg->temp_rows[1][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + i * mjpeg->coded_w;
          mjpeg->temp_rows[2][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + mjpeg->coded_w * mjpeg->coded_h + i * mjpeg->coded_w;
          }
        break;
        
      case BC_YUVJ420P:
        mjpeg->temp_data = lqt_bufalloc(mjpeg->coded_w * mjpeg->coded_h + mjpeg->coded_w * mjpeg->coded_h / 2);
        mjpeg->temp_rows[0] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h);
        mjpeg->temp_rows[1] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h / 2);
        mjpeg->temp_rows[2] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h / 2);
        for(i = 0; i < mjpeg->coded_h; i++)
          {
          mjpeg->temp_rows[0][i] = mjpeg->temp_data + i * mjpeg->coded_w;
          if(i < mjpeg->coded_h / 2)
            {
            mjpeg->temp_rows[1][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + i * (mjpeg->coded_w / 2);
            mjpeg->temp_rows[2][i] = mjpeg->temp_data + mjpeg->coded_w * mjpeg->coded_h + (mjpeg->coded_h / 2) * (mjpeg->coded_w / 2) + i * (mjpeg->coded_w / 2);
            }
          }
        break;
      }
    }
  }

static int get_input_row(mjpeg_t *mjpeg, int i, int field)
  {
  int input_row;
  if(mjpeg->fields > 1) 
    input_row = i * 2 + field;
  else
    input_row = i;
  if(input_row >= mjpeg->coded_h) input_row = mjpeg->coded_h - 1;
  return input_row;
  }

// Get pointers to rows for the JPEG compressor
static void get_rows(mjpeg_t *mjpeg, mjpeg_compressor *compressor, int field)
  {
  int i;

  if((mjpeg->fields > 1) && (mjpeg->bottom_first))
    field = 1 - field;
    
  switch(mjpeg->jpeg_color_model)
    {
    case BC_YUVJ444P:
      {
      if(!compressor->rows[0])
        {
        compressor->rows[0] = lqt_bufalloc(sizeof(unsigned char*) * compressor->field_h);
        compressor->rows[1] = lqt_bufalloc(sizeof(unsigned char*) * compressor->field_h);
        compressor->rows[2] = lqt_bufalloc(sizeof(unsigned char*) * compressor->field_h);
        }

      for(i = 0; i < compressor->field_h; i++)
        {
        int input_row = get_input_row(mjpeg, i, field);
        compressor->rows[0][i] = mjpeg->temp_rows[0][input_row];
        compressor->rows[1][i] = mjpeg->temp_rows[1][input_row];
        compressor->rows[2][i] = mjpeg->temp_rows[2][input_row];
        }
      break;
      }

    case BC_YUVJ422P:
      {
      if(!compressor->rows[0])
        {
        compressor->rows[0] = lqt_bufalloc(sizeof(unsigned char*) * compressor->field_h);
        compressor->rows[1] = lqt_bufalloc(sizeof(unsigned char*) * compressor->field_h);
        compressor->rows[2] = lqt_bufalloc(sizeof(unsigned char*) * compressor->field_h);
        }

      for(i = 0; i < compressor->field_h; i++)
        {
        int input_row = get_input_row(mjpeg, i, field);
        compressor->rows[0][i] = mjpeg->temp_rows[0][input_row];
        compressor->rows[1][i] = mjpeg->temp_rows[1][input_row];
        compressor->rows[2][i] = mjpeg->temp_rows[2][input_row];
        }
      break;
      }

    case BC_YUVJ420P:
      {
      if(!compressor->rows[0])
        {
        compressor->rows[0] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h);
        compressor->rows[1] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h / 2);
        compressor->rows[2] = lqt_bufalloc(sizeof(unsigned char*) * mjpeg->coded_h / 2);
        }

      for(i = 0; i < compressor->field_h; i++)
        {
        int input_row = get_input_row(mjpeg, i, field);
        compressor->rows[0][i] = mjpeg->temp_rows[0][input_row];
        if(i < compressor->field_h / 2)
          {
          compressor->rows[1][i] = mjpeg->temp_rows[1][input_row];
          compressor->rows[2][i] = mjpeg->temp_rows[2][input_row];
          }
        }
      break;
      }
    }
  }

static void delete_rows(mjpeg_compressor *compressor)
  {
  if(compressor->rows[0])
    {
    free(compressor->rows[0]);
    free(compressor->rows[1]);
    free(compressor->rows[2]);
    }
  }


static void new_jpeg_objects(mjpeg_compressor *engine)
  {
  engine->jpeg_decompress.err = jpeg_std_error(&engine->jpeg_error.pub);
  engine->jpeg_error.pub.error_exit = mjpeg_error_exit;
  /* Ideally the error handler would be set here but it must be called in a thread */
  jpeg_create_decompress(&engine->jpeg_decompress);
  engine->jpeg_decompress.raw_data_out = TRUE;
#if JPEG_LIB_VERSION >= 70
  engine->jpeg_decompress.do_fancy_upsampling = FALSE;
#endif
  engine->jpeg_decompress.dct_method = JDCT_IFAST;
  }

static void delete_jpeg_objects(mjpeg_compressor *engine)
  {
  jpeg_destroy_decompress(&engine->jpeg_decompress);
  }


// Make temp rows for compressor
static void get_mcu_rows(mjpeg_t *mjpeg, 
                         mjpeg_compressor *engine,
                         int start_row)
  {
  int i, j, scanline;
  for(i = 0; i < 3; i++)
    {
    for(j = 0; j < 16; j++)
      {
      if(i > 0 && j >= 8 && mjpeg->jpeg_color_model == BC_YUVJ420P) break;

      scanline = start_row;
      if(i > 0 && mjpeg->jpeg_color_model == BC_YUVJ420P) scanline /= 2;
      scanline += j;
      if(scanline >= engine->field_h) scanline = engine->field_h - 1;
      engine->mcu_rows[i][j] = engine->rows[i][scanline];
      }
    }
  }

/**********************************************************
 * Default huffman table generation: Ported from mjpegtools with
 * permission of the original author
 **********************************************************/

static void add_huff_table (j_decompress_ptr dinfo,
                            JHUFF_TBL **htblptr,
                            const UINT8 *bits, const UINT8 *val)
/* Define a Huffman table */
{
  int nsymbols, len;

  if (*htblptr == NULL)
    *htblptr = jpeg_alloc_huff_table((j_common_ptr) dinfo);

  /* Copy the number-of-symbols-of-each-code-length counts */
  memcpy((*htblptr)->bits, bits, sizeof((*htblptr)->bits));

  /* Validate the counts.  We do this here mainly so we can copy the right
   * number of symbols from the val[] array, without risking marching off
   * the end of memory.  jchuff.c will do a more thorough test later.
   */
  nsymbols = 0;
  for (len = 1; len <= 16; len++)
    nsymbols += bits[len];
  if (nsymbols < 1 || nsymbols > 256)
    //    mjpeg_error_exit1("jpegutils.c:  add_huff_table failed badly. ");
    lqt_log(NULL, LQT_LOG_ERROR, LOG_DOMAIN, "add_huff_table failed badly.\n");
  
  memcpy((*htblptr)->huffval, val, nsymbols * sizeof(UINT8));
}

static void std_huff_tables (j_decompress_ptr dinfo)
/* Set up the standard Huffman tables (cf. JPEG standard section K.3) */
/* IMPORTANT: these are only valid for 8-bit data precision! */
{
  static const UINT8 bits_dc_luminance[17] =
    { /* 0-base */ 0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_luminance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

  static const UINT8 bits_dc_chrominance[17] =
    { /* 0-base */ 0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  static const UINT8 val_dc_chrominance[] =
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

  static const UINT8 bits_ac_luminance[17] =
    { /* 0-base */ 0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  static const UINT8 val_ac_luminance[] =
    { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
      0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
      0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
      0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
      0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
      0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
      0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
      0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
      0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
      0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
      0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
      0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
      0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
      0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
      0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };
  static const UINT8 bits_ac_chrominance[17] =
    { /* 0-base */ 0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  static const UINT8 val_ac_chrominance[] =
    { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
      0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
      0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
      0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
      0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
      0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
      0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
      0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
      0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
      0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
      0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
      0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
      0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
      0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
      0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
      0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
      0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
      0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
      0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
      0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
      0xf9, 0xfa };

  add_huff_table(dinfo, &dinfo->dc_huff_tbl_ptrs[0],
                 bits_dc_luminance, val_dc_luminance);
  add_huff_table(dinfo, &dinfo->ac_huff_tbl_ptrs[0],
                 bits_ac_luminance, val_ac_luminance);
  add_huff_table(dinfo, &dinfo->dc_huff_tbl_ptrs[1],
                 bits_dc_chrominance, val_dc_chrominance);
  add_huff_table(dinfo, &dinfo->ac_huff_tbl_ptrs[1],
                 bits_ac_chrominance, val_ac_chrominance);
}

static void guarantee_huff_tables(j_decompress_ptr dinfo)
{
  if ( (dinfo->dc_huff_tbl_ptrs[0] == NULL) &&
       (dinfo->dc_huff_tbl_ptrs[1] == NULL) &&
       (dinfo->ac_huff_tbl_ptrs[0] == NULL) &&
       (dinfo->ac_huff_tbl_ptrs[1] == NULL) ) {
  //    mjpeg_debug( "Generating standard Huffman tables for this frame.");
    std_huff_tables(dinfo);
  }
}


static void decompress_field(mjpeg_compressor *engine, int field)
  {
  mjpeg_t *mjpeg = engine->mjpeg;
  long buffer_offset = field * mjpeg->input_field2;
  unsigned char *buffer = mjpeg->input_data + buffer_offset;
  long buffer_size;


  if(mjpeg->fields > 1)
    {
    if(field)
      buffer_size = mjpeg->input_size - buffer_offset;
    else
      buffer_size = mjpeg->input_field2;
    }
  else
    buffer_size = mjpeg->input_size;
  
  mjpeg->error = 0;
#if 1 
  if(setjmp(engine->jpeg_error.setjmp_buffer))
    {
    /* If we get here, the JPEG code has signaled an error. */
    delete_jpeg_objects(engine);
    new_jpeg_objects(engine);
    mjpeg->error = 1;
    goto finish;
    }
#endif
  jpeg_buffer_src(&engine->jpeg_decompress, 
                  buffer, 
                  buffer_size);
  jpeg_read_header(&engine->jpeg_decompress, TRUE);
#if 0
  fprintf(stderr, "decompress_field %d: %d x %d\n",
          field,
          engine->jpeg_decompress.image_width,
          engine->jpeg_decompress.image_height);
#endif
  guarantee_huff_tables(&engine->jpeg_decompress);


  
  // Reset by jpeg_read_header
  engine->jpeg_decompress.raw_data_out = TRUE;
#if JPEG_LIB_VERSION >= 70
  engine->jpeg_decompress.do_fancy_upsampling = FALSE;
#endif
  jpeg_start_decompress(&engine->jpeg_decompress);

  // Generate colormodel from jpeg sampling
  if(engine->jpeg_decompress.comp_info[0].v_samp_factor == 2 &&
     engine->jpeg_decompress.comp_info[0].h_samp_factor == 2)
    {
    mjpeg->jpeg_color_model = BC_YUVJ420P;
    mjpeg->coded_w_uv = mjpeg->coded_w / 2;
    }
  else if(engine->jpeg_decompress.comp_info[0].v_samp_factor == 1 &&
          engine->jpeg_decompress.comp_info[0].h_samp_factor == 2)
    {
    mjpeg->jpeg_color_model = BC_YUVJ422P;
    mjpeg->coded_w_uv = mjpeg->coded_w / 2;
    }
  else
    {
    mjpeg->jpeg_color_model = BC_YUVJ444P;
    mjpeg->coded_w_uv = mjpeg->coded_w;
    }
  // Must be here because the color model isn't known until now
  allocate_temps(mjpeg);
  get_rows(mjpeg, engine, field);

  while(engine->jpeg_decompress.output_scanline < engine->jpeg_decompress.output_height)
    {
    get_mcu_rows(mjpeg, engine, engine->jpeg_decompress.output_scanline);
    jpeg_read_raw_data(&engine->jpeg_decompress, 
                       engine->mcu_rows, 
                       engine->field_h);
    }
  jpeg_finish_decompress(&engine->jpeg_decompress);


  finish:
  ;
  }

static void compress_field(mjpeg_compressor *engine, int field)
  {
  mjpeg_t *mjpeg = engine->mjpeg;

  get_rows(engine->mjpeg, engine, field);
  reset_buffer(&engine->output_buffer, &engine->output_size, &engine->output_allocated);
  jpeg_buffer_dest(&engine->jpeg_compress, engine);

  /* Fields can have different heights when the total height is odd */
  engine->jpeg_compress.image_height = engine->field_heights[field];
#if 0
  fprintf(stderr, "compress_field %d: %d x %d\n",
          field,
          engine->jpeg_compress.image_width,
          engine->jpeg_compress.image_height);
#endif
  
  engine->jpeg_compress.raw_data_in = TRUE;
  jpeg_start_compress(&engine->jpeg_compress, TRUE);

  while(engine->jpeg_compress.next_scanline < engine->jpeg_compress.image_height)
    {
    get_mcu_rows(mjpeg, engine, engine->jpeg_compress.next_scanline);

    jpeg_write_raw_data(&engine->jpeg_compress, 
			engine->mcu_rows, 
			engine->field_h);
    }
  jpeg_finish_compress(&engine->jpeg_compress);
  }



static void delete_temps(mjpeg_t *mjpeg)
  {
  if(mjpeg->temp_data)
    {
    free(mjpeg->temp_data);
    free(mjpeg->temp_rows[0]);
    free(mjpeg->temp_rows[1]);
    free(mjpeg->temp_rows[2]);
    }
  }

static mjpeg_compressor* mjpeg_new_decompressor(mjpeg_t *mjpeg)
  {
  mjpeg_compressor *result = lqt_bufalloc(sizeof(mjpeg_compressor));

  result->mjpeg = mjpeg;
  new_jpeg_objects(result);
  result->field_h = mjpeg->coded_h / mjpeg->fields;

  result->mcu_rows[0] = lqt_bufalloc(16 * sizeof(unsigned char*));
  result->mcu_rows[1] = lqt_bufalloc(16 * sizeof(unsigned char*));
  result->mcu_rows[2] = lqt_bufalloc(16 * sizeof(unsigned char*));

  return result;
  }

static void mjpeg_delete_decompressor(mjpeg_compressor *engine)
  {
  /*
   * Must honor the locking protocol to avoid a race condition with
   * the child going into the cond_wait state for more input
   */

  jpeg_destroy_decompress(&engine->jpeg_decompress);
  delete_rows(engine);
  free(engine->mcu_rows[0]);
  free(engine->mcu_rows[1]);
  free(engine->mcu_rows[2]);
  free(engine);
  }

static mjpeg_compressor* mjpeg_new_compressor(mjpeg_t *mjpeg)
  {
  mjpeg_compressor *result = lqt_bufalloc(sizeof(mjpeg_compressor));

  /* Padded height */
  result->field_h = mjpeg->coded_h / mjpeg->fields;

  if(mjpeg->fields == 2)
    {
    /* Unpadded heights for the internal JPEG header */
    result->field_heights[0] = mjpeg->output_h / 2;
    result->field_heights[1] = mjpeg->output_h / 2;

    /* Top field gets an extra scanline */
    if(mjpeg->output_h % 2)
      {
      /* Top field comes as second field */
      if(mjpeg->bottom_first)
        result->field_heights[1]++;
      else
        result->field_heights[0]++;
      }
    }
  else
    result->field_heights[0] = mjpeg->output_h;
  
  result->mjpeg = mjpeg;
  result->jpeg_compress.err = jpeg_std_error(&result->jpeg_error.pub);
  jpeg_create_compress(&result->jpeg_compress);
  result->jpeg_compress.image_width = mjpeg->output_w;
  result->jpeg_compress.input_components = 3;
  result->jpeg_compress.in_color_space = JCS_RGB;
  jpeg_set_defaults(&result->jpeg_compress);
  result->jpeg_compress.input_components = 3;
  result->jpeg_compress.in_color_space = JCS_RGB;
  jpeg_set_quality(&result->jpeg_compress, mjpeg->quality, 0);
#if JPEG_LIB_VERSION >= 70
  result->jpeg_compress.do_fancy_downsampling = FALSE;
#endif
  if(mjpeg->use_float) 
    result->jpeg_compress.dct_method = JDCT_FLOAT;
  else
    result->jpeg_compress.dct_method = JDCT_IFAST;
  //		result->jpeg_compress.dct_method = JDCT_ISLOW;

  /* Fix sampling */
  switch(mjpeg->fields)
    {
    case 1:
      if(mjpeg->jpeg_color_model == BC_YUVJ420P)
        {
        result->jpeg_compress.comp_info[0].h_samp_factor = 2;
        result->jpeg_compress.comp_info[0].v_samp_factor = 2;
        result->jpeg_compress.comp_info[1].h_samp_factor = 1;
        result->jpeg_compress.comp_info[1].v_samp_factor = 1;
        result->jpeg_compress.comp_info[2].h_samp_factor = 1;
        result->jpeg_compress.comp_info[2].v_samp_factor = 1;
        }
      else if(mjpeg->jpeg_color_model == BC_YUVJ422P)
        {
        result->jpeg_compress.comp_info[0].h_samp_factor = 2;
        result->jpeg_compress.comp_info[0].v_samp_factor = 1;
        result->jpeg_compress.comp_info[1].h_samp_factor = 1;
        result->jpeg_compress.comp_info[1].v_samp_factor = 1;
        result->jpeg_compress.comp_info[2].h_samp_factor = 1;
        result->jpeg_compress.comp_info[2].v_samp_factor = 1;
        }
      else if(mjpeg->jpeg_color_model == BC_YUVJ444P)
        {
        result->jpeg_compress.comp_info[0].h_samp_factor = 1;
        result->jpeg_compress.comp_info[0].v_samp_factor = 1;
        result->jpeg_compress.comp_info[1].h_samp_factor = 1;
        result->jpeg_compress.comp_info[1].v_samp_factor = 1;
        result->jpeg_compress.comp_info[2].h_samp_factor = 1;
        result->jpeg_compress.comp_info[2].v_samp_factor = 1;
        }
      break;
    case 2:
      result->jpeg_compress.comp_info[0].h_samp_factor = 2;
      result->jpeg_compress.comp_info[0].v_samp_factor = 1;
      result->jpeg_compress.comp_info[1].h_samp_factor = 1;
      result->jpeg_compress.comp_info[1].v_samp_factor = 1;
      result->jpeg_compress.comp_info[2].h_samp_factor = 1;
      result->jpeg_compress.comp_info[2].v_samp_factor = 1;
      break;
    }
  allocate_temps(mjpeg);

  result->mcu_rows[0] = lqt_bufalloc(16 * sizeof(unsigned char*));
  result->mcu_rows[1] = lqt_bufalloc(16 * sizeof(unsigned char*));
  result->mcu_rows[2] = lqt_bufalloc(16 * sizeof(unsigned char*));

  return result;
  }

static void mjpeg_delete_compressor(mjpeg_compressor *engine)
  {
  /*
   * Must honor the locking protocol to avoid a race condition with 
   * the child going into the cond_wait state for more input 
   */

  jpeg_destroy((j_common_ptr)&engine->jpeg_compress);
  if(engine->output_buffer) free(engine->output_buffer);
  delete_rows(engine);
  free(engine->mcu_rows[0]);
  free(engine->mcu_rows[1]);
  free(engine->mcu_rows[2]);
  free(engine);
  }

unsigned char* mjpeg_output_buffer(mjpeg_t *mjpeg)
  {
  return mjpeg->output_data;
  }

long mjpeg_output_field2(mjpeg_t *mjpeg)
  {
  return mjpeg->output_field2;
  }

long mjpeg_output_size(mjpeg_t *mjpeg)
  {
  return mjpeg->output_size;
  }

int mjpeg_compress(mjpeg_t *mjpeg, 
                   unsigned char **row_pointers)
  {
  uint8_t * cpy_rows[3];
  int i;

  /* Reset output buffer */
  reset_buffer(&mjpeg->output_data, 
               &mjpeg->output_size, 
               &mjpeg->output_allocated);

  /* Create compression engines as needed */
  if(!mjpeg->compressor)
    {
    mjpeg->compressor = mjpeg_new_compressor(mjpeg);
    }
  
  // Copy to buffer first

  cpy_rows[0] = mjpeg->temp_rows[0][0];
  cpy_rows[1] = mjpeg->temp_rows[1][0];
  cpy_rows[2] = mjpeg->temp_rows[2][0];
  
  lqt_rows_copy(cpy_rows,
                row_pointers, mjpeg->output_w, mjpeg->output_h, mjpeg->rowspan, mjpeg->rowspan_uv,
                mjpeg->coded_w, mjpeg->coded_w_uv, mjpeg->jpeg_color_model);
  
  
  /* Start the compressors on the image fields */

  for(i = 0; i < mjpeg->fields; i++)
    {
    compress_field(mjpeg->compressor, i);
    append_buffer(&mjpeg->output_data, 
                  &mjpeg->output_size, 
                  &mjpeg->output_allocated,
                  mjpeg->compressor->output_buffer, 
                  mjpeg->compressor->output_size);
    if(i == 0) mjpeg->output_field2 = mjpeg->output_size;
    }	

  return 0;
  }


int mjpeg_decompress(mjpeg_t *mjpeg, 
                     unsigned char *buffer, 
                     long buffer_len,
                     long input_field2)
  {
  int i;

  if(buffer_len == 0) return 1;
  if(input_field2 == 0 && mjpeg->fields > 1) return 1;

    
  /* Create decompression engines as needed */
  if(!mjpeg->decompressor)
    {
    mjpeg->decompressor = mjpeg_new_decompressor(mjpeg);
    }
  
  /* Arm YUV buffers */
  //	mjpeg->y_argument = y_plane;
  //	mjpeg->u_argument = u_plane;
  //	mjpeg->v_argument = v_plane;
  mjpeg->input_data = buffer;
  mjpeg->input_size = buffer_len;
  mjpeg->input_field2 = input_field2;
  //	mjpeg->color_model = color_model;

  /* Start decompressors */

  for(i = 0; i < mjpeg->fields; i++)
    {
    decompress_field(mjpeg->decompressor, i);
    }
  return 0;
  }

void mjpeg_get_frame(mjpeg_t * mjpeg, uint8_t ** row_pointers)
  {
  uint8_t * cpy_rows[3];

  // Copy to buffer first

  cpy_rows[0] = mjpeg->temp_rows[0][0];
  cpy_rows[1] = mjpeg->temp_rows[1][0];
  cpy_rows[2] = mjpeg->temp_rows[2][0];
  
  lqt_rows_copy(row_pointers,
                cpy_rows, mjpeg->output_w, mjpeg->output_h, mjpeg->coded_w, mjpeg->coded_w_uv,
                mjpeg->rowspan, mjpeg->rowspan_uv, mjpeg->jpeg_color_model);
  
  }

void mjpeg_set_quality(mjpeg_t *mjpeg, int quality)
  {
  mjpeg->quality = quality;
  }

void mjpeg_set_float(mjpeg_t *mjpeg, int use_float)
  {
  mjpeg->use_float = use_float;
  }


void mjpeg_set_rowspan(mjpeg_t *mjpeg, int rowspan, int rowspan_uv)
  {
  mjpeg->rowspan = rowspan;
  mjpeg->rowspan_uv = rowspan_uv;
  }

int mjpeg_get_fields(mjpeg_t *mjpeg)
  {
  return mjpeg->fields;
  }

mjpeg_t* mjpeg_new(int w, 
                   int h, 
                   int fields, int cmodel)
  {
  mjpeg_t *result = calloc(1, sizeof(*result));
  
  result->output_w = w;
  result->output_h = h;
  result->fields = fields;
  result->quality = 80;
  result->use_float = 0;
  
  // Calculate coded dimensions
  result->jpeg_color_model = cmodel;

  result->coded_w = (w % 16) ? w + (16 - (w % 16)) : w;

  if(result->jpeg_color_model == BC_YUVJ444P)
    result->coded_w_uv = result->coded_w;
  else
    result->coded_w_uv = result->coded_w / 2;
  
  result->coded_h = (h % 16) ? h + (16 - (h % 16)) : h;

  
  return result;
  }

void mjpeg_delete(mjpeg_t *mjpeg)
  {
  if(mjpeg->compressor) mjpeg_delete_compressor(mjpeg->compressor);
  if(mjpeg->decompressor) mjpeg_delete_decompressor(mjpeg->decompressor);
  delete_temps(mjpeg);
  delete_buffer(&mjpeg->output_data, &mjpeg->output_size, &mjpeg->output_allocated);
  free(mjpeg);
  }


/* Open up a space to insert a marker */
static void insert_space(unsigned char **buffer, 
                         long *buffer_size, 
                         long *buffer_allocated,
                         long space_start,
                         long space_len)
  {
  int in, out;
  // Make sure enough space is available
  if(*buffer_allocated - *buffer_size < space_len)
    {
    *buffer_allocated += space_len;
    *buffer = realloc(*buffer, *buffer_allocated);
    }

  // Shift data back
  for(in = *buffer_size - 1, out = *buffer_size - 1 + space_len;
      in >= space_start;
      in--, out--)
    {
    (*buffer)[out] = (*buffer)[in];
    }
  *buffer_size += space_len;
  }

static inline int nextbyte(unsigned char *data, long *offset, long length)
  {
  if(length - *offset < 1) return 0;
  *offset += 1;
  return (unsigned char)data[*offset - 1];
  }

static inline int next_int32(unsigned char *data, long *offset, long length)
  {
  if(length - *offset < 4)
    {
    *offset = length;
    return 0;
    }
  *offset += 4;
  return ((((unsigned int)data[*offset - 4]) << 24) | 
          (((unsigned int)data[*offset - 3]) << 16) | 
          (((unsigned int)data[*offset - 2]) << 8) | 
          (((unsigned int)data[*offset - 1])));
  }

static inline int read_int16(unsigned char *data, long *offset, long length)
  {
  if(length - *offset < 2)	
    {
    *offset = length;
    return 0;
    }

  *offset += 2;
  return ((((unsigned int)data[*offset - 2]) << 8) | 
          (((unsigned int)data[*offset - 1])));
  }

static inline int
next_int16(unsigned char *data, long *offset, long length)
  {
  if(length - *offset < 2)	
    {
    return 0;
    }

  return ((((unsigned int)data[*offset]) << 8) | 
          (((unsigned int)data[*offset + 1])));
  }

static inline void
write_int32(unsigned char *data, long *offset, long length,
            unsigned int value)
  {
  if(length - *offset < 4)
    {
    *offset = length;
    return;
    }


  data[(*offset)++] = (unsigned int)(value & 0xff000000) >> 24;
  data[(*offset)++] = (unsigned int)(value & 0xff0000) >> 16;
  data[(*offset)++] = (unsigned int)(value & 0xff00) >> 8;
  data[(*offset)++] = (unsigned char)(value & 0xff);
  return;
  }

static int next_marker(unsigned char *buffer, long *offset, long buffer_size)
  {

  while(*offset < buffer_size - 1)
    {
    if(buffer[*offset] == 0xff && buffer[*offset + 1] != 0xff)
      {
      (*offset) += 2;
      return buffer[*offset - 1];
      }
		
    (*offset)++;
    }

  return 0;

  }

/* Find the next marker after offset and return 0 on success */
static int find_marker(unsigned char *buffer, 
                       long *offset, 
                       long buffer_size,
                       unsigned long marker_type)
  {
  long result = 0;

  while(!result && *offset < buffer_size)
    {
    int marker = next_marker(buffer, offset, buffer_size);
    if(marker == (marker_type & 0xff)) result = 1;
    }

  return !result;
  }


typedef struct
  {
  int field_size;
  int padded_field_size;
  int next_offset;
  int quant_offset;
  int huffman_offset;
  int image_offset;
  int scan_offset;
  int data_offset;
  } mjpeg_qt_hdr;

static void table_offsets(unsigned char *buffer, 
                          long buffer_size, 
                          mjpeg_qt_hdr *header)
  {
  int done = 0;
  long offset = 0;
  int marker = 0;
  int field = 0;
  int len;

  bzero(header, sizeof(mjpeg_qt_hdr) * 2);

  // Read every marker to get the offsets for the headers
  for(field = 0; field < 2; field++)
    {
    done = 0;
    while(!done)
      {
      marker = next_marker(buffer, 
                           &offset, 
                           buffer_size);
      len = 0;

      switch(marker)
        {
        case M_SOI:
          // The first field may be padded
          if(field > 0) 
            {
            header[0].next_offset = 
              header[0].padded_field_size = 
              offset - 2;
            }
          len = 0;
          break;

        case M_DQT:
          if(!header[field].quant_offset)
            {
            header[field].quant_offset = offset - 2;
            if(field > 0)
              header[field].quant_offset -= header[0].next_offset;
            }
          len = read_int16(buffer, &offset, buffer_size);
          len -= 2;
          break;

        case M_DHT:
          if(!header[field].huffman_offset)
            {
            header[field].huffman_offset = offset - 2;
            if(field > 0)
              header[field].huffman_offset -= header[0].next_offset;
            }
          len = read_int16(buffer, &offset, buffer_size);
          len -= 2;
          break;

        case M_SOF0:
          if(!header[field].image_offset)
            {
            header[field].image_offset = offset - 2;
            if(field > 0)
              header[field].image_offset -= header[0].next_offset;
            }
          len = read_int16(buffer, &offset, buffer_size);
          len -= 2;
          break;

        case M_SOS:
          header[field].scan_offset = offset - 2;
          if(field > 0)
            header[field].scan_offset -= header[0].next_offset;
          len = read_int16(buffer, &offset, buffer_size);
          len -= 2;
          header[field].data_offset = offset + len;
          if(field > 0)
            header[field].data_offset -= header[0].next_offset;
          break;

          //				case 0:
        case M_EOI:
          if(field > 0) 
            {
            header[field].field_size = 
              header[field].padded_field_size = 
              offset - header[0].next_offset;
            header[field].next_offset = 0;
            }
          else
            {
            // Often misses second SOI but gets first EOI
            //						header[0].next_offset = 
            //							header[0].padded_field_size = 
            //							offset;
            }
          done = 1;
          len = 0;
          break;

        default:
          // Junk appears between fields
          len = 0;
          //					len = read_int16(buffer, &offset, buffer_size);
          //					len -= 2;
          break;
        }

      if(!done) offset += len;
      }
    }
  }

static void insert_quicktime_marker(unsigned char *buffer, 
                                    long buffer_size, 
                                    long offset, 
                                    mjpeg_qt_hdr *header)
  {
  write_int32(buffer, &offset, buffer_size, 0xff000000 | 
              ((unsigned long)M_APP1 << 16) | 
              (QUICKTIME_MARKER_SIZE - 2));
  write_int32(buffer, &offset, buffer_size, 0);
  write_int32(buffer, &offset, buffer_size, QUICKTIME_JPEG_TAG);
  write_int32(buffer, &offset, buffer_size, header->field_size);
  write_int32(buffer, &offset, buffer_size, header->padded_field_size);
  write_int32(buffer, &offset, buffer_size, header->next_offset);
  write_int32(buffer, &offset, buffer_size, header->quant_offset);
  write_int32(buffer, &offset, buffer_size, header->huffman_offset);
  write_int32(buffer, &offset, buffer_size, header->image_offset);
  write_int32(buffer, &offset, buffer_size, header->scan_offset);
  write_int32(buffer, &offset, buffer_size, header->data_offset);
  }


void mjpeg_insert_quicktime_markers(unsigned char **buffer, 
                                    long *buffer_size, 
                                    long *buffer_allocated,
                                    int fields,
                                    long *field2_offset)
  {
  mjpeg_qt_hdr header[2];

  if(fields < 2) return;
  // Get offsets for tables in both fields
  table_offsets(*buffer, *buffer_size, header);

  header[0].field_size += QUICKTIME_MARKER_SIZE;
  header[0].padded_field_size += QUICKTIME_MARKER_SIZE;
  header[0].next_offset += QUICKTIME_MARKER_SIZE;
  header[0].quant_offset += QUICKTIME_MARKER_SIZE;
  header[0].huffman_offset += QUICKTIME_MARKER_SIZE;
  header[0].image_offset += QUICKTIME_MARKER_SIZE;
  header[0].scan_offset += QUICKTIME_MARKER_SIZE;
  header[0].data_offset += QUICKTIME_MARKER_SIZE;
  header[1].field_size += QUICKTIME_MARKER_SIZE;
  header[1].padded_field_size += QUICKTIME_MARKER_SIZE;
  header[1].quant_offset += QUICKTIME_MARKER_SIZE;
  header[1].huffman_offset += QUICKTIME_MARKER_SIZE;
  header[1].image_offset += QUICKTIME_MARKER_SIZE;
  header[1].scan_offset += QUICKTIME_MARKER_SIZE;
  header[1].data_offset += QUICKTIME_MARKER_SIZE;
  *field2_offset = header[0].next_offset;



  // Insert APP1 marker
  insert_space(buffer, 
               buffer_size, 
               buffer_allocated,
               2,
               QUICKTIME_MARKER_SIZE);
  insert_quicktime_marker(*buffer, 
                          *buffer_size, 
                          2, 
                          &header[0]);

  insert_space(buffer, 
               buffer_size, 
               buffer_allocated,
               header[0].next_offset + 2,
               QUICKTIME_MARKER_SIZE);
  header[1].next_offset = 0;
  insert_quicktime_marker(*buffer, 
                          *buffer_size, 
                          header[0].next_offset + 2, 
                          &header[1]);
  }


static void read_quicktime_markers(unsigned char *buffer, 
                                   long buffer_size, 
                                   mjpeg_qt_hdr *header)
  {
  long offset = 0;
  int marker_count = 0;
  int result = 0;

  while(marker_count < 2 && offset < buffer_size && !result)
    {
    result = find_marker(buffer, 
                         &offset, 
                         buffer_size,
                         M_APP1);

    if(!result)
      {
      // Marker size
      read_int16(buffer, &offset, buffer_size);
      // Zero
      next_int32(buffer, &offset, buffer_size);
      // MJPA
      next_int32(buffer, &offset, buffer_size);
      // Information
      header[marker_count].field_size = next_int32(buffer, &offset, buffer_size);
      header[marker_count].padded_field_size = next_int32(buffer, &offset, buffer_size);
      header[marker_count].next_offset = next_int32(buffer, &offset, buffer_size);
      header[marker_count].quant_offset = next_int32(buffer, &offset, buffer_size);
      header[marker_count].huffman_offset = next_int32(buffer, &offset, buffer_size);
      header[marker_count].image_offset = next_int32(buffer, &offset, buffer_size);
      header[marker_count].scan_offset = next_int32(buffer, &offset, buffer_size);
      header[marker_count].data_offset = next_int32(buffer, &offset, buffer_size);
      marker_count++;
      }
    }
  }

long mjpeg_get_quicktime_field2(unsigned char *buffer, long buffer_size)
  {
  mjpeg_qt_hdr header[2];
  bzero(&header, sizeof(mjpeg_qt_hdr) * 2);

  read_quicktime_markers(buffer, buffer_size, header);
  return header[0].next_offset;
  }
