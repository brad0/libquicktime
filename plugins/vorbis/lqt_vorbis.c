#include "../../lqt.h"
#include "../../lqt_codecapi.h"
#include "qtvorbis.h"

static char * fourccs_vorbis[]  = { QUICKTIME_VORBIS };


static lqt_codec_info_static_t codec_info_vorbis =
  {
    "vorbis",
    "Ogg Vorbis audio codec",     /* Long name of the codec */
    "Patent free audio codec (see http://www.vorbis.com)",      /* Description            */
    
    LQT_CODEC_AUDIO,
    LQT_DIRECTION_BOTH
    
  };

static lqt_codec_parameter_info_t encode_parameters_vorbis[] =
  {
     { 
       "vorbis_bitrate",  /* Name for quicktime_set_parameter */
       "Nominal Bitrate", /* Name for dialog boxes            */
       LQT_PARAMETER_INT, /* Type                             */
       { 128000 },        /* Default value                    */
       {0},               /* Minimum value                    */
       {0}                /* Maximum value                    */
     },
     { 
       "vorbis_max_bitrate",            /* Name for quicktime_set_parameter */
       "Maximum Bitrate (-1: no limit)",/* Name for dialog boxes            */
       LQT_PARAMETER_INT,               /* Type                             */
       { -1 },                          /* Default value                    */
       {0},                             /* Minimum value                    */
       {0}                              /* Maximum value                    */
     },
     { 
       "vorbis_min_bitrate",            /* Name for quicktime_set_parameter */
       "Minimum Bitrate (-1: no limit)",/* Name for dialog boxes            */
       LQT_PARAMETER_INT,               /* Type                             */
       { -1 },                          /* Default value                    */
       {0},                             /* Minimum value                    */
       {0}                              /* Maximum value                    */
     }
  };




/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_t * get_codec_info(int index)
  {
  if(!index)
    return lqt_create_codec_info(&codec_info_vorbis,
                                 fourccs_vorbis,
                                 sizeof(fourccs_vorbis)/sizeof(char*),
                                 encode_parameters_vorbis,
                                 sizeof(encode_parameters_vorbis)/sizeof(lqt_codec_parameter_info_t),
                                 (const lqt_codec_parameter_info_t*)0,
                                 0);
  
  return (lqt_codec_info_t*)0;
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

