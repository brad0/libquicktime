#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include "dv.h"
#include <libdv/dv.h>

static char * fourccs_dv[]  = { QUICKTIME_DVm, (char*)0 };

static lqt_codec_info_static_t codec_info_dv =
  {
    name:        "dv",
    long_name:   "Quasar DV Codec",
    description: "Codec for digital video camaras. Based on\
 libdv (http://libdv.sourceforge.net/).",
    fourccs:     fourccs_dv,
    type:        LQT_CODEC_VIDEO,
    direction:   LQT_DIRECTION_BOTH
    
  };

static lqt_parameter_info_t decode_parameters_dv[] =
  {
     { 
       name:               "dv_decode_quality",
       real_name:          "Decoding Quality",
       type:               LQT_PARAMETER_INT,
       val_default:        {DV_QUALITY_BEST},
       val_min:            {0},
       val_max:            {DV_QUALITY_BEST},
       stringlist_options: (char**)0
     }
  };

static lqt_parameter_info_t encode_parameters_dv[] =
  {
     { /* Set to one if the input is anamorphic 16x9 (stretched letter box). This is produced by many standard DV camcorders in 16x9 mode. */
	name:               "dv_anamorphic16x9",
	real_name:          "Is Anamorphic 16x9",
	type:               LQT_PARAMETER_INT,
	val_default:        {0},
	val_min:            {0},
	val_max:            {1},
        stringlist_options: (char**)0

     },
     { 
       name:               "dv_vlc_encode_passes",
       real_name:          "VLC Encode Passes",
       type:               LQT_PARAMETER_INT,
       val_default:        {3},
       val_min:            {1},
       val_max:            {3},
       stringlist_options: (char**)0
     }
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_t * get_codec_info(int index)
  {
  if(index == 0)
    return lqt_create_codec_info(&codec_info_dv,
                                 encode_parameters_dv,
                                 sizeof(encode_parameters_dv)
								 / sizeof(lqt_parameter_info_t),
                                 decode_parameters_dv,
                                 sizeof(decode_parameters_dv)
								 / sizeof(lqt_parameter_info_t) );

  return (lqt_codec_info_t*)0;
  }
     

/*
 *   Return the actual codec constructor
 */

extern lqt_init_video_codec_func_t get_video_codec(int index)
  {
  if(index == 0)
    return quicktime_init_codec_dv;
  return (lqt_init_video_codec_func_t)0;
  }
