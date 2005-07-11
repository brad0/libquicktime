#include <quicktime/lqt.h>
#include <quicktime/colormodels.h>
#include <string.h>
#include <dlfcn.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

typedef struct
  {
  char * name;
  int colormodel;
  } lqt_colormodel_tab;

static lqt_colormodel_tab colormodel_table[] =
  {
    { "Compressed",                BC_COMPRESSED },
    { "16 bpp RGB 565",            BC_RGB565 },
    { "16 bpp BGR 565",            BC_BGR565 },
    { "24 bpp BGR",                BC_BGR888 },
    { "32 bpp BGR",                BC_BGR8888 },
    { "24 bpp RGB",                BC_RGB888 },
    { "32 bpp RGBA",               BC_RGBA8888 },
    { "48 bpp RGB",                BC_RGB161616  }, 
    { "64 bpp RGBA",               BC_RGBA16161616  },
    { "32 bpp YUVA",               BC_YUVA8888  },   
    { "YUV 4:2:2 packed (YUY2)",   BC_YUV422  },
    { "YUV 4:2:0 planar",          BC_YUV420P },
    { "YUV 4:2:2 planar",          BC_YUV422P },
    { "YUV 4:4:4 planar",          BC_YUV444P },
    { "YUV 4:2:2 planar (16 bit)", BC_YUV422P16 },
    { "YUV 4:4:4 planar (16 bit)", BC_YUV444P16 },
    { "YUV 4:2:0 planar (jpeg)",   BC_YUVJ420P },
    { "YUV 4:2:2 planar (jpeg)",   BC_YUVJ422P },
    { "YUV 4:4:4 planar (jpeg)",   BC_YUVJ444P },
    { "YUV 4:1:1 planar",          BC_YUV411P },
    { (char*)0, LQT_COLORMODEL_NONE }
  };

/* Some functions to find out, how cheap a colorspace conversion can be */

int lqt_colormodel_is_yuv(int colormodel)
  {
  switch(colormodel)
    {
    case BC_YUVA8888:
    case BC_YUV422:
    case BC_YUV420P:
    case BC_YUV422P:
    case BC_YUV444P:
    case BC_YUV422P16:
    case BC_YUV444P16:
    case BC_YUVJ420P:
    case BC_YUVJ422P:
    case BC_YUVJ444P:
    case BC_YUV411P:
      return 1;
    default:
      return 0;
    }
  }

int lqt_colormodel_is_rgb(int colormodel)
  {
  switch(colormodel)
    {
    case BC_RGB565:
    case BC_BGR565:
    case BC_BGR888:
    case BC_BGR8888:
    case BC_RGB888:
    case BC_RGBA8888:
    case BC_RGB161616:
    case BC_RGBA16161616:
      return 1;
    default:
      return 0;
    }
  }

int lqt_colormodel_has_alpha(int colormodel)
  {
  switch(colormodel)
    {
    case BC_RGBA8888:
    case BC_RGBA16161616:
    case BC_YUVA8888:   
      return 1;
    default:
      return 0;
    }
  }

int lqt_colormodel_is_planar(int colormodel)
  {
  switch(colormodel)
    {
    case BC_YUV420P:
    case BC_YUV422P:
    case BC_YUV444P:
    case BC_YUV422P16:
    case BC_YUV444P16:
    case BC_YUVJ422P:
    case BC_YUVJ444P:
    case BC_YUVJ420P:
    case BC_YUV411P:
      return 1;
    default:
      return 0;
    }
  
  }

/* Get chroma subsampling factors */

void lqt_colormodel_get_chroma_sub(int colormodel, int * sub_h, int * sub_v)
  {
  switch(colormodel)
    {
    case BC_YUV420P:
    case BC_YUVJ420P:
      *sub_h = 2;
      *sub_v = 2;
      break;
    case BC_YUV422P:
    case BC_YUVJ422P:
    case BC_YUV422P16:
      *sub_h = 2;
      *sub_v = 1;
      break;
    case BC_YUV411P:
      *sub_h = 4;
      *sub_v = 1;
      break;
    default:
      *sub_h = 1;
      *sub_v = 1;
      break;
    }

  }


/*
 *   Return the bits of a colormodel. This is used only internally
 *   and returns the sum of the bits of all components. Downsampling isn't
 *   taken into account here, YUV 420 has 24 bits. We need to test this,
 *   because e.g. RGBA8888 -> RGBA16161616 cost extra.
 */

static int colormodel_get_bits(int colormodel)
  {
  switch(colormodel)
    {
    case BC_RGB565:
    case BC_BGR565:
      return 16;
    case BC_BGR888:
    case BC_BGR8888:
    case BC_RGB888:
    case BC_YUV422:
    case BC_YUV420P:
    case BC_YUV422P:
    case BC_YUV444P:
    case BC_YUVJ420P:
    case BC_YUVJ422P:
    case BC_YUVJ444P:
    case BC_YUV411P:
      return 24;
    case BC_RGBA8888:
    case BC_YUVA8888:   
      return 32;
    case BC_RGB161616: 
    case BC_YUV422P16:
    case BC_YUV444P16:
      return 48;
    case BC_RGBA16161616:
      return 64;
    default:
      fprintf(stderr,"lqt: warning: unknown colormodel (%d)\n",colormodel);
      return 0;
    }
  }

/*
 *  Get the "Price" of a colormodel conversion
 */

static int get_conversion_price(int in_colormodel, int out_colormodel)
  {
  int input_is_rgb  = lqt_colormodel_is_rgb(in_colormodel);
  int output_is_rgb = lqt_colormodel_is_rgb(out_colormodel);
  
  int input_is_yuv  = lqt_colormodel_is_yuv(in_colormodel);
  int output_is_yuv = lqt_colormodel_is_yuv(out_colormodel);

  int input_has_alpha  = lqt_colormodel_has_alpha(in_colormodel);
  int output_has_alpha = lqt_colormodel_has_alpha(out_colormodel);

 
  /* Zero conversions are for free :-) */
  
  if(in_colormodel == out_colormodel)
    return 0;

  /*
   *  Don't know what to do here. It can happen for very few
   *  colormodels which aren't supported by any codecs.
   */
  
  if(!input_is_rgb && !input_is_yuv)
    {
#ifndef NDEBUG
    fprintf(stderr,
            "Input colorspace is neither RGB nor YUV, can't predict conversion price\n");
#endif
    return 6;
    }
  
  if(!output_is_rgb && !output_is_yuv)
    {
#ifndef NDEBUG
    fprintf(stderr,
            "Output colorspace is neither RGB nor YUV, can't predict conversion price\n");
#endif
    return 6;
    }

  /*
   *  Adding or removing the alpha channel means losing information or
   *  adding unneccesary information -> too bad
   */

  if(input_has_alpha != output_has_alpha)
    return 5;
  
  /*
   *  YUV <-> RGB conversion costs 4
   */
  
  if((input_is_yuv && output_is_rgb) ||
     (input_is_rgb && output_is_yuv))
    return 4;
  
  /*
   *  Alpha blending is a bit more simple
   */

  if((input_is_yuv && output_is_rgb) ||
     (input_is_rgb && output_is_yuv))
    return 3;
  
  /* Bit with conversion costs 2   */
  
  if(colormodel_get_bits(in_colormodel) !=
     colormodel_get_bits(out_colormodel))
    return 2;

  /* Reordering of components is cheapest */
  
  return 1;
  }

int lqt_get_decoder_colormodel(quicktime_t * file, int track)
  {
  return file->vtracks[track].stream_cmodel;
  }

static int
lqt_get_best_colormodel_decode(quicktime_t * file, int track, int * supported)
  {
  int ret = LQT_COLORMODEL_NONE;

  int index;

  int best_conversion_price;
  int conversion_price;
  
  int decoder_colormodel = LQT_COLORMODEL_NONE;

  decoder_colormodel = lqt_get_decoder_colormodel(file, track);
  
  /* Check, if the decoder colormodel is directly supported */

  if(decoder_colormodel != LQT_COLORMODEL_NONE)
    {
    index = 0;
    while(supported[index] != LQT_COLORMODEL_NONE)
      {
      if(decoder_colormodel == supported[index])
        {
        ret = supported[index];
        break;
        }
      else
        index++;
      }
    }
  else
    ret = BC_RGB888;
  

  if(ret == LQT_COLORMODEL_NONE)
    {
    index = 0;

    best_conversion_price = 10;

    while(supported[index] != LQT_COLORMODEL_NONE)
      {
      if(quicktime_reads_cmodel(file, supported[index], track))
        {
        conversion_price
          = get_conversion_price(decoder_colormodel, supported[index]);
/*         fprintf(stderr, "Conversion: %s -> %s: %d\n", */
/*                 lqt_colormodel_to_string(decoder_colormodel), */
/*                 lqt_colormodel_to_string(supported[index]), */
/*                 conversion_price);     */
        if(conversion_price < best_conversion_price)
          {
          best_conversion_price = conversion_price;
          ret = supported[index];
          }
        }
      index++;
      }
    }
  if(ret == LQT_COLORMODEL_NONE)
    ret = BC_RGB888;
  return ret;
  }

static int
lqt_get_best_colormodel_encode(quicktime_t * file, int track,
                               int * supported)
  {
  int index_supported;

  int conversion_price = 0, best_conversion_price = 10;
  int ret = LQT_COLORMODEL_NONE;
  
  index_supported = 0;

  fprintf(stderr, "Get best colormodel %s\n", lqt_colormodel_to_string(file->vtracks[track].stream_cmodel));

  while(supported[index_supported] != LQT_COLORMODEL_NONE)
    {
    if(file->vtracks[track].stream_cmodel == supported[index_supported])
      {
      ret = file->vtracks[track].stream_cmodel;
      fprintf(stderr, "*** GOT ENCODER CMODEL: %s\n", lqt_colormodel_to_string(ret));
      break;
      }
    index_supported++;
    }
  
  if(ret == LQT_COLORMODEL_NONE)
    {
    index_supported = 0;

    best_conversion_price = 10;
    
    while(supported[index_supported] != LQT_COLORMODEL_NONE)
      {
      if(quicktime_writes_cmodel(file, supported[index_supported], track))
        {
        conversion_price =
          get_conversion_price(file->vtracks[track].stream_cmodel,
                               supported[index_supported]);
        
        if(conversion_price < best_conversion_price) 
          {
          best_conversion_price = conversion_price;
          ret = supported[index_supported];
          }
        }
      index_supported++;
      }
    }
  
  if(ret == LQT_COLORMODEL_NONE)
    {
    ret = BC_RGB888;
    }
  return ret;
  }

int lqt_get_best_colormodel(quicktime_t * file, int track,
                            int * supported)
  {
  if((track >= file->total_vtracks) || (track < 0))
    return LQT_COLORMODEL_NONE;
  if(file->wr)
    return lqt_get_best_colormodel_encode(file, track, supported);
  else
    return lqt_get_best_colormodel_decode(file, track, supported);
  }

const char * lqt_colormodel_to_string(int colormodel)
  {
  int index = 0;
  
  while(1)
    {
    if((colormodel_table[index].colormodel == colormodel) ||
       (colormodel_table[index].colormodel == LQT_COLORMODEL_NONE))
      break;
    index++;
    }
  return colormodel_table[index].name;
  }

int lqt_string_to_colormodel(const char * str)
  {
  int index = 0;
  
  while(1)
    {
    if((!colormodel_table[index].name) ||
       !strcmp(colormodel_table[index].name, str))
      break;
    index++;
    }
  return colormodel_table[index].colormodel;
  }

int lqt_num_colormodels()
  {
  int ret = 0;
  
  while(colormodel_table[ret].name)
    ret++;
  
  return ret;
  }

const char * lqt_get_colormodel_string(int index)
  {
  return colormodel_table[index].name;
  }

int lqt_get_colormodel(int index)
  {
  return colormodel_table[index].colormodel;
  }

static int get_bytes_per_line(int colormodel, int width)
  {
  switch(colormodel)
    {
    case BC_RGB565:
    case BC_BGR565:
    case BC_YUV422:
    case BC_YUV422P16:
    case BC_YUV444P16:
      return width * 2;
      break;
    case BC_BGR888:
    case BC_RGB888:
      return width * 3;
      break;
    case BC_BGR8888:
    case BC_RGBA8888:
    case BC_YUVA8888:
      return width * 4;
      break;
      
    case BC_RGB161616:
      return width * 6;
      break;
    case BC_RGBA16161616:
      return width * 8;
      break;
    default:
      return width;
    }
  }

/* Allocate and free row_pointers for use with libquicktime */

uint8_t ** lqt_rows_alloc(int width, int height, int colormodel, int * rowspan, int * rowspan_uv)
  {
  int bytes_per_line = 0;
  int i;
  int y_size = 0, uv_size = 0;
  uint8_t ** video_buffer;
  int sub_h = 0, sub_v = 0;

  /* Allocate frame buffer */
#if 0  
  fprintf(stderr, "lqt_rows_alloc: %d x %d, %s, %d %d\n", width, height, lqt_colormodel_to_string(colormodel),
          *rowspan, *rowspan_uv);
#endif
  bytes_per_line = get_bytes_per_line(colormodel, width);
    
  if(cmodel_is_planar(colormodel))
    {
    lqt_colormodel_get_chroma_sub(colormodel, &sub_h, &sub_v);

    if(*rowspan <= 0)
      *rowspan = bytes_per_line;

    if(*rowspan_uv <= 0)
      *rowspan_uv = *rowspan / sub_h;

    y_size = *rowspan * height;
    uv_size = (*rowspan_uv * height)/sub_v;

    video_buffer    = malloc(3 * sizeof(unsigned char*));
    video_buffer[0] = malloc(y_size + 2 * uv_size);
    video_buffer[1] = &(video_buffer[0][y_size]);
    video_buffer[2] = &(video_buffer[0][y_size+uv_size]);
    }
  else
    {
    video_buffer    = malloc(height * sizeof(unsigned char*));
        
    if(*rowspan <= 0)
      *rowspan = bytes_per_line;
        
    video_buffer[0] = malloc(height * bytes_per_line);
    for(i = 1; i < height; i++)
      video_buffer[i] = &(video_buffer[0][i*bytes_per_line]);
    
    }
  return video_buffer;
  }

void lqt_get_default_rowspan(int colormodel, int width, int * rowspan, int * rowspan_uv)
  {
  int bytes_per_line;
  int sub_h = 0, sub_v = 0;
  bytes_per_line = get_bytes_per_line(colormodel, width);
  lqt_colormodel_get_chroma_sub(colormodel, &sub_h, &sub_v);

  *rowspan = bytes_per_line;
  *rowspan_uv = bytes_per_line / sub_h;
  }

void lqt_rows_free(uint8_t ** rows)
  {
  free(rows[0]);
  free(rows);
  }

void lqt_rows_copy(uint8_t **out_rows, uint8_t **in_rows, int width, int height, int in_rowspan, int in_rowspan_uv,
                   int out_rowspan, int out_rowspan_uv, int colormodel)
  {
  uint8_t * src_ptr, *dst_ptr;
  int i;
  int sub_h = 0, sub_v = 0;

  int bytes_per_line;
#if 0
  fprintf(stderr, "lqt_rows_copy: w: %d, h: %d, in_rowspan: %d, in_rowspan_uv: %d, out_rowspan: %d, out_rowspan_uv: %d, colormodel: %s\n",
          width, height, in_rowspan, in_rowspan_uv, out_rowspan, out_rowspan_uv, lqt_colormodel_to_string(colormodel));
#endif
  if(lqt_colormodel_is_planar(colormodel))
    {
    lqt_colormodel_get_chroma_sub(colormodel, &sub_h, &sub_v);

    /* Luma plane */
        
    src_ptr = in_rows[0];
    dst_ptr = out_rows[0];
    for(i = 0; i < height; i++)
      {
      memcpy(dst_ptr, src_ptr, width);
      src_ptr += in_rowspan;
      dst_ptr += out_rowspan;
      }
    /* Chroma planes */

    src_ptr = in_rows[1];
    dst_ptr = out_rows[1];
    for(i = 0; i < height/sub_v; i++)
      {
      memcpy(dst_ptr, src_ptr, width/sub_h);
      src_ptr += in_rowspan_uv;
      dst_ptr += out_rowspan_uv;
      }

    src_ptr = in_rows[2];
    dst_ptr = out_rows[2];
    for(i = 0; i < height/sub_v; i++)
      {
      memcpy(dst_ptr, src_ptr, width/sub_h);
      src_ptr += in_rowspan_uv;
      dst_ptr += out_rowspan_uv;
      }
    
    }
  else /* Packed */
    {
    bytes_per_line = get_bytes_per_line(colormodel, width);
    for(i = 0; i < height; i++)
      memcpy(out_rows[i], in_rows[i], bytes_per_line);
    }
  
  }

// for i in BC_RGB565 BC_BGR565 BC_BGR888 BC_BGR8888 BC_RGB888 BC_RGBA8888 BC_RGB161616 BC_RGBA16161616 BC_YUVA8888 BC_YUV422 BC_YUV420P BC_YUV422P BC_YUV444P BC_YUV411P BC_YUVJ420P BC_YUVJ422P BC_YUVJ444P BC_YUV422P16 BC_YUV444P16; do for j in BC_RGB565 BC_BGR565 BC_BGR888 BC_BGR8888 BC_RGB888 BC_RGBA8888 BC_RGB161616 BC_RGBA16161616 BC_YUVA8888 BC_YUV422 BC_YUV420P BC_YUV422P BC_YUV444P BC_YUV411P BC_YUVJ420P BC_YUVJ422P BC_YUVJ444P BC_YUV422P16 BC_YUV444P16; do echo $i"_to_"$j.png; gthumb $i"_to_"$j.png; done; done

int lqt_colormodel_has_conversion(int in_cmodel, int out_cmodel)
  {
  if(in_cmodel == out_cmodel)
    return 1;
  
  switch(in_cmodel)
    {
    case BC_RGB565:
      switch(out_cmodel)
        {
        case BC_BGR565:       return 0; break;
        case BC_BGR888:       return 0; break;
        case BC_BGR8888:      return 0; break;
        case BC_RGB888:       return 0; break; // !
        case BC_RGBA8888:     return 0; break;
        case BC_RGB161616:    return 0; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 0; break;
        case BC_YUV422:       return 0; break;
        case BC_YUV420P:      return 0; break;
        case BC_YUV422P:      return 0; break;
        case BC_YUV444P:      return 0; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_BGR565:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 0; break;
        case BC_BGR888:       return 0; break;
        case BC_BGR8888:      return 0; break;
        case BC_RGB888:       return 0; break; // !
        case BC_RGBA8888:     return 0; break;
        case BC_RGB161616:    return 0; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 0; break;
        case BC_YUV422:       return 0; break;
        case BC_YUV420P:      return 0; break;
        case BC_YUV422P:      return 0; break;
        case BC_YUV444P:      return 0; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_BGR888:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 1; break;
        case BC_BGR565:       return 1; break;
        case BC_BGR8888:      return 1; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 1; break;
        case BC_RGB161616:    return 1; break;
        case BC_RGBA16161616: return 1; break;
        case BC_YUVA8888:     return 1; break;
        case BC_YUV422:       return 1; break;
        case BC_YUV420P:      return 1; break;
        case BC_YUV422P:      return 1; break;
        case BC_YUV444P:      return 1; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_BGR8888:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 0; break;
        case BC_BGR565:       return 0; break;
        case BC_BGR888:       return 0; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 0; break;
        case BC_RGB161616:    return 0; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 0; break;
        case BC_YUV422:       return 0; break;
        case BC_YUV420P:      return 1; break;
        case BC_YUV422P:      return 0; break;
        case BC_YUV444P:      return 0; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_RGB888:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 1; break;
        case BC_BGR565:       return 1; break;
        case BC_BGR888:       return 1; break;
        case BC_BGR8888:      return 1; break;
        case BC_RGBA8888:     return 1; break;
        case BC_RGB161616:    return 1; break;
        case BC_RGBA16161616: return 1; break;
        case BC_YUVA8888:     return 1; break;
        case BC_YUV422:       return 1; break;
        case BC_YUV420P:      return 1; break;
        case BC_YUV422P:      return 1; break;
        case BC_YUV444P:      return 1; break;
        case BC_YUV411P:      return 1; break;
        case BC_YUVJ420P:     return 1; break;
        case BC_YUVJ422P:     return 1; break;
        case BC_YUVJ444P:     return 1; break;
        case BC_YUV422P16:    return 1; break;
        case BC_YUV444P16:    return 1; break;
        }
      break;
    case BC_RGBA8888:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 1; break;
        case BC_BGR565:       return 1; break;
        case BC_BGR888:       return 1; break;
        case BC_BGR8888:      return 1; break;
        case BC_RGB888:       return 1; break;
        case BC_RGB161616:    return 1; break;
        case BC_RGBA16161616: return 1; break;
        case BC_YUVA8888:     return 1; break;
        case BC_YUV422:       return 1; break;
        case BC_YUV420P:      return 1; break;
        case BC_YUV422P:      return 1; break;
        case BC_YUV444P:      return 1; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_RGB161616:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 1; break;
        case BC_BGR565:       return 1; break;
        case BC_BGR888:       return 1; break;
        case BC_BGR8888:      return 1; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 1; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 1; break;
        case BC_YUV422:       return 0; break;
        case BC_YUV420P:      return 1; break;
        case BC_YUV422P:      return 1; break;
        case BC_YUV444P:      return 1; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_RGBA16161616:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 1; break;
        case BC_BGR565:       return 1; break;
        case BC_BGR888:       return 1; break;
        case BC_BGR8888:      return 1; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 1; break;
        case BC_RGB161616:    return 0; break;
        case BC_YUVA8888:     return 0; break;
        case BC_YUV422:       return 0; break;
        case BC_YUV420P:      return 1; break;
        case BC_YUV422P:      return 1; break;
        case BC_YUV444P:      return 1; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUVA8888:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 1; break;
        case BC_BGR565:       return 1; break;
        case BC_BGR888:       return 1; break;
        case BC_BGR8888:      return 1; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 0; break;
        case BC_RGB161616:    return 0; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUV422:       return 1; break;
        case BC_YUV420P:      return 1; break;
        case BC_YUV422P:      return 1; break;
        case BC_YUV444P:      return 1; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUV422:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 1; break;
        case BC_BGR565:       return 1; break;
        case BC_BGR888:       return 1; break;
        case BC_BGR8888:      return 1; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 1; break;
        case BC_RGB161616:    return 1; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 1; break;
        case BC_YUV420P:      return 1; break;
        case BC_YUV422P:      return 1; break;
        case BC_YUV444P:      return 0; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 1; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUV420P:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 1; break;
        case BC_BGR565:       return 1; break;
        case BC_BGR888:       return 1; break;
        case BC_BGR8888:      return 1; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 1; break;
        case BC_RGB161616:    return 1; break;
        case BC_RGBA16161616: return 1; break;
        case BC_YUVA8888:     return 1; break;
        case BC_YUV422:       return 1; break;
        case BC_YUV422P:      return 1; break;
        case BC_YUV444P:      return 1; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUV422P:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 1; break;
        case BC_BGR565:       return 1; break;
        case BC_BGR888:       return 1; break;
        case BC_BGR8888:      return 1; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 1; break;
        case BC_RGB161616:    return 1; break;
        case BC_RGBA16161616: return 1; break;
        case BC_YUVA8888:     return 1; break;
        case BC_YUV422:       return 1; break;
        case BC_YUV420P:      return 1; break;
        case BC_YUV444P:      return 1; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUV444P:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 1; break;
        case BC_BGR565:       return 1; break;
        case BC_BGR888:       return 1; break;
        case BC_BGR8888:      return 1; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 1; break;
        case BC_RGB161616:    return 1; break;
        case BC_RGBA16161616: return 1; break;
        case BC_YUVA8888:     return 1; break;
        case BC_YUV422:       return 1; break;
        case BC_YUV420P:      return 1; break;
        case BC_YUV422P:      return 1; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUV411P:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 0; break;
        case BC_BGR565:       return 0; break;
        case BC_BGR888:       return 0; break;
        case BC_BGR8888:      return 0; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 0; break;
        case BC_RGB161616:    return 0; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 0; break;
        case BC_YUV422:       return 0; break;
        case BC_YUV420P:      return 0; break;
        case BC_YUV422P:      return 0; break;
        case BC_YUV444P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUVJ420P:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 0; break;
        case BC_BGR565:       return 0; break;
        case BC_BGR888:       return 0; break;
        case BC_BGR8888:      return 0; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 0; break;
        case BC_RGB161616:    return 0; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 0; break;
        case BC_YUV422:       return 0; break;
        case BC_YUV420P:      return 0; break;
        case BC_YUV422P:      return 0; break;
        case BC_YUV444P:      return 0; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUVJ422P:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 0; break;
        case BC_BGR565:       return 0; break;
        case BC_BGR888:       return 0; break;
        case BC_BGR8888:      return 0; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 0; break;
        case BC_RGB161616:    return 0; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 0; break;
        case BC_YUV422:       return 1; break;
        case BC_YUV420P:      return 0; break;
        case BC_YUV422P:      return 0; break;
        case BC_YUV444P:      return 0; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUVJ444P:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 0; break;
        case BC_BGR565:       return 0; break;
        case BC_BGR888:       return 0; break;
        case BC_BGR8888:      return 0; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 0; break;
        case BC_RGB161616:    return 0; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 0; break;
        case BC_YUV422:       return 0; break;
        case BC_YUV420P:      return 0; break;
        case BC_YUV422P:      return 0; break;
        case BC_YUV444P:      return 0; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUV422P16:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 0; break;
        case BC_BGR565:       return 0; break;
        case BC_BGR888:       return 0; break;
        case BC_BGR8888:      return 0; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 0; break;
        case BC_RGB161616:    return 0; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 0; break;
        case BC_YUV422:       return 0; break;
        case BC_YUV420P:      return 0; break;
        case BC_YUV422P:      return 0; break;
        case BC_YUV444P:      return 0; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV444P16:    return 0; break;
        }
      break;
    case BC_YUV444P16:
      switch(out_cmodel)
        {
        case BC_RGB565:       return 0; break;
        case BC_BGR565:       return 0; break;
        case BC_BGR888:       return 0; break;
        case BC_BGR8888:      return 0; break;
        case BC_RGB888:       return 1; break;
        case BC_RGBA8888:     return 0; break;
        case BC_RGB161616:    return 0; break;
        case BC_RGBA16161616: return 0; break;
        case BC_YUVA8888:     return 0; break;
        case BC_YUV422:       return 0; break;
        case BC_YUV420P:      return 0; break;
        case BC_YUV422P:      return 0; break;
        case BC_YUV444P:      return 0; break;
        case BC_YUV411P:      return 0; break;
        case BC_YUVJ420P:     return 0; break;
        case BC_YUVJ422P:     return 0; break;
        case BC_YUVJ444P:     return 0; break;
        case BC_YUV422P16:    return 0; break;
        }
      break;
    }
  return 0;
  }
