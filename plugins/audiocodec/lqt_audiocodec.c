#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>

/* Let's define prototypes here */

void quicktime_init_codec_ima4(quicktime_audio_map_t *atrack);
void quicktime_init_codec_rawaudio(quicktime_audio_map_t *atrack);
void quicktime_init_codec_twos(quicktime_audio_map_t *atrack);
void quicktime_init_codec_ulaw(quicktime_audio_map_t *atrack);

static char * fourccs_ima4[]  = { QUICKTIME_IMA4, (char*)0 };
static char * fourccs_raw[]   = { QUICKTIME_RAW,  (char*)0 };
static char * fourccs_twos[]  = { QUICKTIME_TWOS, (char*)0 };
static char * fourccs_ulaw[]  = { QUICKTIME_ULAW, (char*)0 };

static lqt_codec_info_static_t codec_info_ima4 =
  {
    name:                "ima4",
    long_name:           "ima4",
    description:         "The IMA4 compressor reduces 16 bit audio data to 1/4\
 size, with very good quality",
    fourccs:             fourccs_ima4,
    type:                LQT_CODEC_AUDIO,
    direction:           LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };

static lqt_codec_info_static_t codec_info_raw =
  {
    name:                "rawaudio",
    long_name:           "Raw 8 bit audio",
    description:         "Don't use this for anything better than telephone \
quality",
    fourccs:             fourccs_raw,
    type:                LQT_CODEC_AUDIO,
    direction:           LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };

static lqt_codec_info_static_t codec_info_twos =
  {
    name:         "twos",
    long_name:    "Twos",
    description:  "Twos is the preferred encoding for uncompressed audio",
    fourccs:      fourccs_twos,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };

static lqt_codec_info_static_t codec_info_ulaw =
  {
    name:         "ulaw",
    long_name:    "Ulaw",     /* Long name of the codec */
    description:  "Ulaw",      /* Description            */
    fourccs:      fourccs_ulaw,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 4; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0: /* ima4 */
      return &codec_info_ima4;
      break;
    case 1: /* raw */
      return &codec_info_raw;

      break;
    case 2: /* Twos */
      return &codec_info_twos;
      break;
    case 3: /* Ulaw */
      return &codec_info_ulaw;
      break;
    }
  
  return (lqt_codec_info_static_t*)0;
  }
     
extern lqt_init_audio_codec_func_t get_audio_codec(int index)
  {
  switch(index)
    {
    case 0: /* ima4 */
      return quicktime_init_codec_ima4;
      break;
    case 1: /* raw */
      return quicktime_init_codec_rawaudio;
      break;
    case 2: /* Twos */
      return quicktime_init_codec_twos;
      break;
    case 3: /* Ulaw */
      return quicktime_init_codec_ulaw;
      break;
    }
  return (lqt_init_audio_codec_func_t)0;
  }
