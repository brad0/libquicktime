#include "../../lqt.h"
#include "../../lqt_codecapi.h"
#include "qtpng.h"

static char * fourccs_png[]  = { QUICKTIME_PNG };

static lqt_codec_info_static_t codec_info_png =
  {
  "png",
  "PNG",                    /* Long name of the codec         */
  "Lossless compressing video codec, Allows alpha",     /* Description                    */

  LQT_CODEC_VIDEO,
  LQT_DIRECTION_BOTH
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 1; }

extern lqt_codec_info_t * get_codec_info(int index)
  {
  if(!index)
    return lqt_create_codec_info(&codec_info_png,
                                 fourccs_png,
                                 sizeof(fourccs_png)/sizeof(char*),
                                 (const lqt_codec_parameter_info_t*)0,
                                 0,
                                 (const lqt_codec_parameter_info_t*)0,
                                 0);
  return (lqt_codec_info_t*)0;
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


