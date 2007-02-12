#include "lqt_private.h"
#include "qtvorbis.h"
#include <quicktime/lqt_codecapi.h>

static char * fourccs_vorbis[]     = { QUICKTIME_VORBIS, "OggV", (char*)0 };
static char * fourccs_vorbis_qt[]  = { "OggV", QUICKTIME_VORBIS, (char*)0 };

static lqt_parameter_info_static_t encode_parameters_vorbis[] =
  {
     { 
       .name =        "vorbis_bitrate",
       .real_name =   "Nominal Bitrate",
       .type =        LQT_PARAMETER_INT,
       .val_default = { .val_int = 128000 },
     },
     { 
       .name =        "vorbis_vbr",
       .real_name =   "Use variable bitrate",
       .type =        LQT_PARAMETER_INT,
       .val_default = { .val_int = 1 },
       .val_min =     { .val_int = 0 },
       .val_max =     { .val_int = 1 },
     },
     {          
       .name =        "vorbis_max_bitrate",
       .real_name =   "Maximum Bitrate (-1 = no limit)",
       .type =        LQT_PARAMETER_INT,
       .val_default = { .val_int = -1 },
       .val_min =     { .val_int = 0 },
       .val_max =     { .val_int = 0 },
     },
     { 
       .name =        "vorbis_min_bitrate",
       .real_name =   "Minimum Bitrate (-1 = no limit)",
       .type =        LQT_PARAMETER_INT,
       .val_default = { .val_int = -1 },
       .val_min =     { .val_int = 0 },
       .val_max =     { .val_int = 0 }
     },
     { /* End of paramaters */ }
  };

static lqt_codec_info_static_t codec_info_vorbis =
  {
    .name =                "vorbis",
    .long_name =           "Ogg Vorbis (qt4l compatible)",
    .description =         "Patent free audio codec (see http://www.vorbis.com)",
    .fourccs =             fourccs_vorbis,
    .type =                LQT_CODEC_AUDIO,
    .direction =           LQT_DIRECTION_BOTH,
    .encoding_parameters = encode_parameters_vorbis,
    .decoding_parameters = (lqt_parameter_info_static_t*)0,
    .compatibility_flags = LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_vorbis_qt =
  {
    .name =                "vorbis_qt",
    .long_name =           "Ogg Vorbis (qtcomponents compatible)",
    .description =         "Patent free audio codec (see http://www.vorbis.com)",
    .fourccs =             fourccs_vorbis_qt,
    .type =                LQT_CODEC_AUDIO,
    .direction =           LQT_DIRECTION_BOTH,
    .encoding_parameters = encode_parameters_vorbis,
    .decoding_parameters = (lqt_parameter_info_static_t*)0,
    .compatibility_flags = LQT_FILE_QT,
  };


/* These are called from the plugin loader */

extern int get_num_codecs() { return 2; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return &codec_info_vorbis;
      break;
    case 1:
      return &codec_info_vorbis_qt;
      break;
    }  
  return (lqt_codec_info_static_t*)0;
  }
     
/*
 *   Return the actual codec constructor
 */

extern lqt_init_audio_codec_func_t get_audio_codec(int index)
  {
  return quicktime_init_codec_vorbis;
  }

