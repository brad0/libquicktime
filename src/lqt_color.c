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
    { "Transparency",            BC_TRANSPARENCY },
    { "Compressed",              BC_COMPRESSED },
    { "8 bpp BGB",               BC_RGB8 },
    { "16 bpp RGB 565",          BC_RGB565 },
    { "16 bpp BGR 565",          BC_BGR565 },
    { "24 bpp BGR",              BC_BGR888 },
    { "32 bpp BGR",              BC_BGR8888 },
    { "24 bpp RGB",              BC_RGB888 },
    { "32 bpp RGBA",             BC_RGBA8888 },
    { "32 bpp ARGB",             BC_ARGB8888 },
    { "32 bpp ABGR",             BC_ABGR8888  },  
    { "48 bpp RGB",              BC_RGB161616  }, 
    { "64 bpp RGBA",             BC_RGBA16161616  },
    { "24 bpp YUV",              BC_YUV888  },
    { "32 bpp YUVA",             BC_YUVA8888  },   
    { "48 bpp YUV",              BC_YUV161616  }, 
    { "64 bpp YUVA",             BC_YUVA16161616  },
    { "YUV 4:2:2 packed (YUY2)", BC_YUV422  },
    { "8 bpp Alpha",             BC_A8  },
    { "16 bpp Alpha",            BC_A16 },
    { "30 bpp YUV",              BC_YUV101010 },
    { "24 bpp VYU",              BC_VYU888 }, 
    { "32 bpp UYVA",             BC_UYVA8888 },
    { "YUV 4:2:0 planar",        BC_YUV420P },
    { "YUV 4:2:2 planar",        BC_YUV422P },
    { "YUV 4:1:1 planar",        BC_YUV411P },
    { (char*)0, LQT_COLORMODEL_NONE }
  };

/* Some functions to find out, how cheap a colorspace conversion can be */

static int colormodel_is_yuv(int colormodel)
  {
  switch(colormodel)
    {
    case BC_YUV888:
    case BC_YUVA8888:
    case BC_YUV161616: 
    case BC_YUVA16161616:
    case BC_YUV422:
    case BC_YUV101010:
    case BC_VYU888:
    case BC_UYVA8888:
    case BC_YUV420P:
    case BC_YUV422P:
    case BC_YUV411P:
      return 1;
    default:
      return 0;
    }
  }

static int colormodel_is_rgb(int colormodel)
  {
  switch(colormodel)
    {
    case BC_RGB8:
    case BC_RGB565:
    case BC_BGR565:
    case BC_BGR888:
    case BC_BGR8888:
    case BC_RGB888:
    case BC_RGBA8888:
    case BC_ARGB8888:
    case BC_ABGR8888:  
    case BC_RGB161616:
    case BC_RGBA16161616:
      return 1;
    default:
      return 0;
    }
  }

static int colormodel_has_alpha(int colormodel)
  {
  switch(colormodel)
    {
    case BC_RGBA8888:
    case BC_ARGB8888:
    case BC_ABGR8888:  
    case BC_RGBA16161616:
    case BC_YUVA8888:   
    case BC_YUVA16161616:
    case BC_A8:
    case BC_A16:
    case BC_UYVA8888:
      return 1;
    default:
      return 0;
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
    case BC_RGB8:
    case BC_A8:
      return 8;
    case BC_RGB565:
    case BC_BGR565:
    case BC_A16:
      return 16;
    case BC_BGR888:
    case BC_BGR8888:
    case BC_RGB888:
    case BC_YUV888:
    case BC_YUV422:
    case BC_VYU888: 
    case BC_YUV420P:
    case BC_YUV422P:
    case BC_YUV411P:
      return 24;
    case BC_YUV101010:
      return 30;
    case BC_RGBA8888:
    case BC_ARGB8888:
    case BC_ABGR8888:  
    case BC_YUVA8888:   
    case BC_UYVA8888:
      return 32;
    case BC_RGB161616: 
    case BC_YUV161616: 
      return 48;
    case BC_RGBA16161616:
    case BC_YUVA16161616:
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
  int input_is_rgb  = colormodel_is_rgb(in_colormodel);
  int output_is_rgb = colormodel_is_rgb(out_colormodel);
  
  int input_is_yuv  = colormodel_is_yuv(in_colormodel);
  int output_is_yuv = colormodel_is_yuv(out_colormodel);

  int input_has_alpha  = colormodel_has_alpha(in_colormodel);
  int output_has_alpha = colormodel_has_alpha(out_colormodel);
  
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

static int lqt_get_decoder_colormodel_private(quicktime_t * file, int track,
                                              int * exact,
                                              lqt_codec_info_t ** codec_info)
  {
  int codec_info_colormodel;
  quicktime_codec_t * codec =
    (quicktime_codec_t *)(file->vtracks[track].codec);
  void * module = codec->module;
  int module_index;
  int (*get_stream_colormodel)(quicktime_t*,int, int, int *);
    
  codec_info_colormodel = codec_info[0]->decoding_colormodel;
  module_index = codec_info[0]->module_index;
  
    
  /* Check, if the decoder has a fixed colormodel */
  
  if(codec_info_colormodel != LQT_COLORMODEL_NONE)
    {
    if(exact)
      *exact = 1;
    return codec_info_colormodel;
    }

  /* Next try: get colormodel from module */

  get_stream_colormodel =
    (int (*)(quicktime_t*,int, int, int *))(dlsym(module, "get_stream_colormodel"));
  
  if(!get_stream_colormodel)
    return LQT_COLORMODEL_NONE;
  
  return get_stream_colormodel(file, track, module_index, exact);
  }

int lqt_get_decoder_colormodel(quicktime_t * file, int track,
                               int * exact)
  {
  int ret;
  lqt_codec_info_t ** codec_info = lqt_video_codec_from_file(file, track);

  ret = lqt_get_decoder_colormodel_private(file, track, exact, codec_info);

  lqt_destroy_codec_info(codec_info);
  return ret;
  }

static int
lqt_get_best_colormodel_decode(quicktime_t * file, int track, int * supported)
  {
  int ret = LQT_COLORMODEL_NONE;

  int index;

  int best_conversion_price;
  int conversion_price;
  
  int decoder_colormodel = LQT_COLORMODEL_NONE;
  int decoder_colormodel_exact = 1;

  decoder_colormodel = lqt_get_decoder_colormodel(file, track,
                                                  &decoder_colormodel_exact);

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
  

  if(ret != LQT_COLORMODEL_NONE)
    {
    index = 0;

    best_conversion_price = 10;

    while(supported[index] != LQT_COLORMODEL_NONE)
      {
      if(quicktime_reads_cmodel(file, supported[index], track))
        {
        conversion_price
          = get_conversion_price(decoder_colormodel, supported[index]);
        
        if(conversion_price < best_conversion_price)
          {
          best_conversion_price = conversion_price;
          ret = supported[index];
          }
        }
      index++;
      }
    }
  if(ret != LQT_COLORMODEL_NONE)
    ret = BC_RGB888;
  return ret;
  }

static int
lqt_get_best_colormodel_encode(quicktime_t * file, int track,
                               int * supported)
  {
  int index_supported;
  int i;

  int conversion_price = 0, best_conversion_price = 10;
  int ret = LQT_COLORMODEL_NONE;

  lqt_codec_info_t ** codec_info = lqt_video_codec_from_file(file, track);
  
  index_supported = 0;

  while(supported[index_supported] != LQT_COLORMODEL_NONE)
    {
    for(i = 0; i < codec_info[0]->num_encoding_colormodels; i++)
      {
      if(codec_info[0]->encoding_colormodels[i] == supported[index_supported])
        {
        ret = supported[index_supported];
        break;
        }
      }
    if(ret != LQT_COLORMODEL_NONE)
      break;
    index_supported++;
    }
  
  if(ret == LQT_COLORMODEL_NONE)
    {
    index_supported = 0;

    best_conversion_price = 10;
    
    while(supported[index_supported] != LQT_COLORMODEL_NONE)
      {
      fprintf(stderr, "Colormodel: %s\n",
              lqt_colormodel_to_string(supported[index_supported]));
      if(quicktime_writes_cmodel(file, supported[index_supported], track))
        {
        for(i = 0; i < codec_info[0]->num_encoding_colormodels; i++)
          {
          conversion_price =
            get_conversion_price(codec_info[0]->encoding_colormodels[i],
                                 supported[index_supported]);

          if(conversion_price < best_conversion_price) 
            {
            best_conversion_price = conversion_price;
            ret = supported[index_supported];
            }
          }
        }
      index_supported++;
      }
    }
  
  lqt_destroy_codec_info(codec_info);

  if(ret == LQT_COLORMODEL_NONE)
    {
    ret = BC_RGB888;
    fprintf(stderr, "lqt_get_best_colormodel_encode: Defaulting to BC_RGB888 %d\n", best_conversion_price);
    }
  return ret;
  }

int lqt_get_best_colormodel(quicktime_t * file, int track,
                            int * supported)
  {
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
