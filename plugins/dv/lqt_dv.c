#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include "dv.h"
#include <libdv/dv.h>

static char * fourccs_dv[]  = { QUICKTIME_DV };

static lqt_codec_info_static_t codec_info_dv =
  {
    "dv",
    "Quasar DV Codec",     /* Long name of the codec */
    "Codec for digital video camaras. Based on libdv (http://libdv.sourceforge.net/).",  /* Description            */
    
    LQT_CODEC_VIDEO,
    LQT_DIRECTION_BOTH
    
  };

static lqt_codec_parameter_info_t decode_parameters_dv[] =
  {
     { 
       "dv_decode_quality",  /* Name for quicktime_set_parameter */
       "Decoding Quality", /* Name for dialog boxes            */
       LQT_PARAMETER_INT, /* Type                             */
       {DV_QUALITY_BEST},        /* Default value                    */
       {0},               /* Minimum value                    */
       {DV_QUALITY_BEST}                /* Maximum value                    */
     }
  };

static lqt_codec_parameter_info_t encode_parameters_dv[] =
  {
     { /* Set to one if the input is anamorphic 16x9 (stretched letter box). This is produced by many standard DV camcorders in 16x9 mode. */
	   "dv_anamorphic16x9",  /* Name for quicktime_set_parameter */
	   "Is Anamorphic 16x9", /* Name for dialog boxes            */
	   LQT_PARAMETER_INT, /* Type                             */
	   {0},        /* Default value                    */
	   {0},               /* Minimum value                    */
	   {1}                /* Maximum value                    */
     },
     { 
	   "dv_vlc_encode_passes",  /* Name for quicktime_set_parameter */
       "VLC Encode Passes", /* Name for dialog boxes            */
       LQT_PARAMETER_INT, /* Type                             */
       {3},        /* Default value                    */
       {1},               /* Minimum value                    */
       {3}                /* Maximum value                    */
     }
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_t * get_codec_info(int index)
  {
  if(index == 0)
    return lqt_create_codec_info(&codec_info_dv,
                                 fourccs_dv,
                                 sizeof(fourccs_dv)/sizeof(char*),
                                 encode_parameters_dv,
                                 sizeof(encode_parameters_dv)
								 / sizeof(lqt_codec_parameter_info_t),
                                 decode_parameters_dv,
                                 sizeof(decode_parameters_dv)
								 / sizeof(lqt_codec_parameter_info_t) );

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
