#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include "dv.h"

/*
 *  We cheat here: Actually, this is one codec (from the code) but we tell the
 *  outer world, that we are 2 codecs
 */

static char * fourccs_dv[]  = { QUICKTIME_DV };

static lqt_codec_info_static_t codec_info_dv =
  {
    "dv",
    "Dv",     /* Long name of the codec */
    "Codec for digital video camaras. Based on libdv",  /* Description            */
    
    LQT_CODEC_VIDEO,
    LQT_DIRECTION_BOTH
    
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_t * get_codec_info(int index)
  {
  if(!index)
    return lqt_create_codec_info(&codec_info_dv,
                                 fourccs_dv,
                                 sizeof(fourccs_dv)/sizeof(char*),
                                 (const lqt_codec_parameter_info_t*)0,
                                 0,
                                 (const lqt_codec_parameter_info_t*)0,
                                 0);

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
