#include "../../lqt.h"
#include "../../lqt_codecapi.h"
/* #include "ima4.h" */
/* #include "rawaudio.h" */
/* #include "twos.h" */
/* #include "ulaw.h" */

/* Let's define prototypes here */

void quicktime_init_codec_ima4(quicktime_audio_map_t *atrack);
void quicktime_init_codec_rawaudio(quicktime_audio_map_t *atrack);
void quicktime_init_codec_twos(quicktime_audio_map_t *atrack);
void quicktime_init_codec_ulaw(quicktime_audio_map_t *atrack);


static char * fourccs_ima4[]  = { QUICKTIME_IMA4 };
static char * fourccs_raw[]   = { QUICKTIME_RAW  };
static char * fourccs_twos[]  = { QUICKTIME_TWOS };
static char * fourccs_ulaw[]  = { QUICKTIME_ULAW };

static lqt_codec_info_static_t codec_info_ima4 =
  {
    "ima4",
    "ima4",     /* Long name of the codec */
    "The IMA4 compressor reduces 16 bit audio data to 1/4 size, with very good quality",      /* Description            */
    LQT_CODEC_AUDIO,
    LQT_DIRECTION_BOTH
    
  };

static lqt_codec_info_static_t codec_info_raw =
  {
    "raw",
    "Raw 8 bit audio",                                                /* Long name of the codec */
    "Don't use this for anything better than telephone quality",      /* Description            */
    
    LQT_CODEC_AUDIO,
    LQT_DIRECTION_BOTH
    
  };

static lqt_codec_info_static_t codec_info_twos =
  {
    "twos",
    "Twos",                                                     /* Long name of the codec */
    "Twos is the preferred encoding for uncompressed audio",    /* Description            */
    
    LQT_CODEC_AUDIO,
    LQT_DIRECTION_BOTH
    
  };

static lqt_codec_info_static_t codec_info_ulaw =
  {
    "ulaw",
    "Ulaw",     /* Long name of the codec */
    "Ulaw",      /* Description            */
    
    LQT_CODEC_AUDIO,
    LQT_DIRECTION_BOTH
    
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 4; }

extern lqt_codec_info_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0: /* ima4 */
      return lqt_create_codec_info(&codec_info_ima4,
                                   fourccs_ima4,
                                   sizeof(fourccs_ima4)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
      break;
    case 1: /* raw */
      return lqt_create_codec_info(&codec_info_raw,
                                   fourccs_raw,
                                   sizeof(fourccs_raw)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);

      break;
    case 2: /* Twos */
      return lqt_create_codec_info(&codec_info_twos,
                                   fourccs_twos,
                                   sizeof(fourccs_twos)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
      break;
    case 3: /* Ulaw */
      return lqt_create_codec_info(&codec_info_ulaw,
                                   fourccs_ulaw,
                                   sizeof(fourccs_ulaw)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
      break;
    }
  
  return (lqt_codec_info_t*)0;
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
