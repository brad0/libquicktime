#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include "qtvorbis.h"

static char * fourccs_vorbis[]  = { QUICKTIME_VORBIS, (char*)0 };

static lqt_parameter_info_static_t encode_parameters_vorbis[] =
  {
     { 
       name:               "vorbis_bitrate",
       real_name:          "Nominal Bitrate",
       type:               LQT_PARAMETER_INT,
       val_default:        { 128000 },
       val_min:            0,
       val_max:            0,
       stringlist_options: (char**)0
     },
     { 
       name:      "vorbis_max_bitrate",
       real_name: "Maximum Bitrate (-1: no limit)",
       type:      LQT_PARAMETER_INT,
       val_default:        { -1 },
       val_min:            0,
       val_max:            0,
       stringlist_options: (char**)0
     },
     { 
       name:      "vorbis_min_bitrate",
       real_name: "Minimum Bitrate (-1: no limit)",
       type:      LQT_PARAMETER_INT,
       val_default:        { -1 },
       val_min:            0,
       val_max:            0,
       stringlist_options: (char**)0
     },
     { /* End of paramaters */ }
  };

static lqt_codec_info_static_t codec_info_vorbis =
  {
    name:                "vorbis",
    long_name:           "Ogg Vorbis audio codec",
    description:         "Patent free audio codec (see http://www.vorbis.com)",
    fourccs:             fourccs_vorbis,
    type:                LQT_CODEC_AUDIO,
    direction:           LQT_DIRECTION_BOTH,
    encoding_parameters: encode_parameters_vorbis,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };


/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  if(!index)
    return &codec_info_vorbis;
  
  return (lqt_codec_info_static_t*)0;
  }
     
/*
 *   Return the actual codec constructor
 */

extern lqt_init_audio_codec_func_t get_audio_codec(int index)
  {
  if(index == 0)
    return quicktime_init_codec_vorbis;
  return (lqt_init_audio_codec_func_t)0;
  }

