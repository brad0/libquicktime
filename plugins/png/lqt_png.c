#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include <quicktime/colormodels.h>
#include "qtpng.h"

static char * fourccs_png[]  = { QUICKTIME_PNG, (char*)0 };

static int encoding_colormodels_png[] =
  {
    BC_RGB888,
    BC_RGBA8888,
    LQT_COLORMODEL_NONE
  };


static lqt_codec_info_static_t codec_info_png =
  {
  name:        "png",
  long_name:   "PNG",
  description: "Lossless compressing video codec, Allows alpha",
  fourccs:     fourccs_png,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH,
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,
  encoding_colormodels: encoding_colormodels_png,
  decoding_colormodel: LQT_COLORMODEL_NONE
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  if(!index)
    return &codec_info_png;
  return (lqt_codec_info_static_t*)0;
  }

/*
 * This is missing in qtpng.h, so we add it here to leave the original
 * source untouched
 */

void quicktime_init_codec_png(quicktime_video_map_t *vtrack);



/*
 *   Return the actual codec constructor
 */

extern lqt_init_video_codec_func_t get_video_codec(int index)
  {
  if(index == 0)
    return quicktime_init_codec_png;
  return (lqt_init_video_codec_func_t)0;
  }


/* 
 *  Return internal colormodel of the stream
 */

int get_stream_colormodel(quicktime_t * file, int track, int codec_index,
                          int * exact)
  {
  int depth;
  if(exact)
    *exact = 1;

  if(codec_index == 0)
    {
    depth = quicktime_video_depth(file, track);
    switch(depth)
      {
      case 24:
        return BC_RGB888;
        break;
      case 32:
        return BC_RGBA8888;
        break;
      default:
        return LQT_COLORMODEL_NONE; /* This should never happen... */
        break;
      }
    }
  return LQT_COLORMODEL_NONE; /* And this neither */
  }
