#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>

extern void quicktime_init_codec_lame(quicktime_audio_map_t *atrack);


static char * fourccs_mp3[]  = { QUICKTIME_MP3, (char*)0 };

static lqt_parameter_info_static_t encode_parameters_lame[] =
  {
     { 
       name:               "mp3_bitrate",
       real_name:          "Nominal Bitrate",
       type:               LQT_PARAMETER_INT,
       val_default:        { 256000 },
       val_min:            0,
       val_max:            0,
       stringlist_options: (char**)0
     },
     { /* End of paramaters */ }
  };

static lqt_codec_info_static_t codec_info_vorbis =
  {
    name:                "lame",
    long_name:           "Lame mp3 encoder",
    description:         "Lame mp3 encoder (see http://www.mp3dev.org)",
    fourccs:             fourccs_mp3,
    type:                LQT_CODEC_AUDIO,
    direction:           LQT_DIRECTION_ENCODE,
    encoding_parameters: encode_parameters_lame,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };

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
    return quicktime_init_codec_lame;
  return (lqt_init_audio_codec_func_t)0;
  }
