#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include <quicktime/colormodels.h>
#include "dv.h"
#include <libdv/dv.h>


// static char * fourccs_dv[]  = { QUICKTIME_DV, QUICKTIME_DV_AVID, QUICKTIME_DV_AVID_A, (char*)0 };

static char * fourccs_dv[]  = { (char*)0 };


static int encoding_colormodels_dv[] =
  {
    BC_YUV422,
    LQT_COLORMODEL_NONE
  };
  
static lqt_parameter_info_static_t decode_parameters_dv[] =
  {
     { 
       name:               "dv_decode_quality",
       real_name:          "Decoding Quality",
       type:               LQT_PARAMETER_INT,
       val_default:        {DV_QUALITY_BEST},
       val_min:            0,
       val_max:            DV_QUALITY_BEST,
       stringlist_options: (char**)0
     },
	 { 
       name:               "dv_clamp_luma",
       real_name:          "Clamp Luma Values",
       type:               LQT_PARAMETER_INT,
       val_default:        {0},
       val_min:            0,
       val_max:            1,
       stringlist_options: (char**)0
     },
	 { 
       name:               "dv_clamp_chroma",
       real_name:          "Clamp Chroma Values",
       type:               LQT_PARAMETER_INT,
       val_default:        {0},
       val_min:            0,
       val_max:            1,
       stringlist_options: (char**)0
     },
	 { 
       name:               "dv_add_ntsc_setup",
       real_name:          "Compensate for 7.5IRE NTSC setup",
       type:               LQT_PARAMETER_INT,
       val_default:        {0},
       val_min:            0,
       val_max:            1,
       stringlist_options: (char**)0
     },
     { /* End of parameters */ }
  };

static lqt_parameter_info_static_t encode_parameters_dv[] =
  {
    { /* Set to one if the input is anamorphic 16x9 (stretched letter box). This is produced by many standard DV camcorders in 16x9 mode. */
      name:               "dv_anamorphic16x9",
      real_name:          "Is Anamorphic 16x9",
      type:               LQT_PARAMETER_INT,
      val_default:        {0},
      val_min:            0,
      val_max:            1,
      stringlist_options: (char**)0
    },
    { 
      name:               "dv_vlc_encode_passes",
      real_name:          "VLC Encode Passes",
      type:               LQT_PARAMETER_INT,
      val_default:        {3},
      val_min:            1,
      val_max:            3,
      stringlist_options: (char**)0
    },
	{ 
      name:               "dv_clamp_luma",
      real_name:          "Clamp Luma Values",
      type:               LQT_PARAMETER_INT,
      val_default:        {0},
      val_min:            0,
      val_max:            1,
      stringlist_options: (char**)0
	},
	{ 
      name:               "dv_clamp_chroma",
      real_name:          "Clamp Chroma Values",
      type:               LQT_PARAMETER_INT,
      val_default:        {0},
      val_min:            0,
      val_max:            1,
      stringlist_options: (char**)0
	},
	{ 
      name:               "dv_rem_ntsc_setup",
      real_name:          "Compensate for 7.5IRE NTSC setup",
      type:               LQT_PARAMETER_INT,
      val_default:        {0},
      val_min:            0,
      val_max:            1,
      stringlist_options: (char**)0
	},
    { /* End of parameters */ }
  };

static lqt_codec_info_static_t codec_info_dv =
  {
    name:        "dv",
    long_name:   "Quasar DV Codec",
    description: "Codec for digital video camaras. Based on\
 libdv (http://libdv.sourceforge.net/).",
    fourccs:     fourccs_dv,
    type:        LQT_CODEC_VIDEO,
    direction:   LQT_DIRECTION_BOTH,
    encoding_parameters: encode_parameters_dv,
    decoding_parameters: decode_parameters_dv,
    encoding_colormodels: encoding_colormodels_dv,
    decoding_colormodel:  BC_YUV422
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  if(index == 0)
    return &codec_info_dv;
  
  return (lqt_codec_info_static_t*)0;
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
