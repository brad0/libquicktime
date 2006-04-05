#include "config.h"
#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include <quicktime/colormodels.h>

void quicktime_init_codec_jpeg(quicktime_video_map_t *vtrack);

/*
 *  We cheat here: Actually, this is one codec but we tell the
 *  outer world, that we are 2 codecs because mjpa and jpeg are
 *  to different to be one codec with 2 fourccs
 */

static char * fourccs_jpeg[]  = { QUICKTIME_JPEG, (char*)0 };
static char * fourccs_mjpa[]  = { QUICKTIME_MJPA, (char*)0 };

static lqt_parameter_info_static_t encode_parameters_jpeg[] =
  {
    { 
      name:        "jpeg_quality",
      real_name:   "Quality",
      type:        LQT_PARAMETER_INT,
      val_default: { val_int: 95 },
      val_min:     { val_int: 1 },
      val_max:     { val_int: 100 },
     },
     { 
       name:        "jpeg_usefloat",
       real_name:   "Use float",
       type:        LQT_PARAMETER_INT,
       val_default: { val_int: 0 },
       val_min:     { val_int: 0 },
       val_max:     { val_int: 1 },
     },
     { /* End of parameters */ }
  };


static lqt_codec_info_static_t codec_info_jpeg =
  {
    name:                "jpeg",
    long_name:           "Jpeg photo",
    description:         "This format writes a seperate JPEG photo for\
 every frame in YUV 4:2:0",
    fourccs:             fourccs_jpeg,
    type:                LQT_CODEC_VIDEO,
    direction:           LQT_DIRECTION_BOTH,
    encoding_parameters: encode_parameters_jpeg,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
  };

static lqt_codec_info_static_t codec_info_mjpa =
  {
    name:                "mjpa",
    long_name:           "Motion Jpeg A",
    description:         "MJPA stores each frame as two JPEGs interlaced\
 and in YUV 4:2:2",
    fourccs:             fourccs_mjpa,
    type:                LQT_CODEC_VIDEO,
    direction:           LQT_DIRECTION_BOTH,
    encoding_parameters: encode_parameters_jpeg,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
  };


/* These are called from the plugin loader */

extern int get_num_codecs() { return 2; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return &codec_info_jpeg;
    case 1:
      return &codec_info_mjpa;
    }
  return (lqt_codec_info_static_t*)0;
  }
     

extern lqt_init_video_codec_func_t get_video_codec(int index)
  {
  if((index == 0) || (index == 1))
    return quicktime_init_codec_jpeg;
  return (lqt_init_video_codec_func_t)0;
  }
