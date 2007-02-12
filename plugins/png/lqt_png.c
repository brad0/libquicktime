#include "lqt_private.h"
#include <quicktime/lqt_codecapi.h>

void quicktime_init_codec_png(quicktime_video_map_t *vtrack);

static char * fourccs_png[]  = { QUICKTIME_PNG, (char*)0 };

static lqt_parameter_info_static_t encode_parameters_png[] =
  {
     { 
       .name =        "png_compression_level",
       .real_name =   "Compression Level",
       .type =        LQT_PARAMETER_INT,
       .val_default = { .val_int = 9 },
       .val_min =     { .val_int = 0 },
       .val_max =     { .val_int = 9 },
     },
     { /* End of parameters */ }
  };

/* RGBA png is supported as a special codec, which supports encoding only.
   The normal png codec can decode both RGB and RGBA */

static lqt_codec_info_static_t codec_info_pngalpha =
  {
  .name =                "pngalpha",
  .long_name =           "PNG (with alpha)",
  .description =         "Lossless video codec (RGBA mode)",
  .fourccs =             fourccs_png,
  .type =                LQT_CODEC_VIDEO,
  .direction =           LQT_DIRECTION_ENCODE,
  .encoding_parameters = encode_parameters_png,
  .decoding_parameters = (lqt_parameter_info_static_t*)0,
  .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_png =
  {
  .name =                "png",
  .long_name =           "PNG",
  .description =         "Lossless video codec (RGB mode)",
  .fourccs =             fourccs_png,
  .type =                LQT_CODEC_VIDEO,
  .direction =           LQT_DIRECTION_BOTH,
  .encoding_parameters = encode_parameters_png,
  .decoding_parameters = (lqt_parameter_info_static_t*)0,
  .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 2; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return &codec_info_png;
    case 1:
      return &codec_info_pngalpha;
    }
  return (lqt_codec_info_static_t*)0;
  }

/*
 * This is missing in qtpng.h, so we add it here to leave the original
 * source untouched
 */

void quicktime_init_codec_png(quicktime_video_map_t *vtrack);
void quicktime_init_codec_pngalpha(quicktime_video_map_t *vtrack);

/*
 *   Return the actual codec constructor
 */

extern lqt_init_video_codec_func_t get_video_codec(int index)
  {
  switch(index)
    {
    case 0:
      return quicktime_init_codec_png;
      break;
    case 1:
      return quicktime_init_codec_pngalpha;
      break;
    }
  return (lqt_init_video_codec_func_t)0;
  }
