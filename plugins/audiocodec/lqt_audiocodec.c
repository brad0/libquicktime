#include "funcprotos.h"
#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>

/* Let's define prototypes here */

void quicktime_init_codec_ima4(quicktime_audio_map_t *atrack);
void quicktime_init_codec_rawaudio(quicktime_audio_map_t *atrack);
void quicktime_init_codec_twos(quicktime_audio_map_t *atrack);
void quicktime_init_codec_ulaw(quicktime_audio_map_t *atrack);
void quicktime_init_codec_alaw(quicktime_audio_map_t *atrack);
void quicktime_init_codec_sowt(quicktime_audio_map_t *atrack);

void quicktime_init_codec_in24_little(quicktime_audio_map_t *atrack);
void quicktime_init_codec_in24_big(quicktime_audio_map_t *atrack);
void quicktime_init_codec_in24(quicktime_audio_map_t *atrack);

void quicktime_init_codec_in32_little(quicktime_audio_map_t *atrack);
void quicktime_init_codec_in32_big(quicktime_audio_map_t *atrack);
void quicktime_init_codec_in32(quicktime_audio_map_t *atrack);


void quicktime_init_codec_fl32_little(quicktime_audio_map_t *atrack);
void quicktime_init_codec_fl32_big(quicktime_audio_map_t *atrack);
void quicktime_init_codec_fl32(quicktime_audio_map_t *atrack);

void quicktime_init_codec_fl64_little(quicktime_audio_map_t *atrack);
void quicktime_init_codec_fl64_big(quicktime_audio_map_t *atrack);
void quicktime_init_codec_fl64(quicktime_audio_map_t *atrack);


static char * fourccs_ima4[]  = { QUICKTIME_IMA4, (char*)0 };
static char * fourccs_raw[]   = { QUICKTIME_RAW,  (char*)0 };
static char * fourccs_twos[]  = { QUICKTIME_TWOS, (char*)0 };
static char * fourccs_ulaw[]  = { QUICKTIME_ULAW, (char*)0 };
static char * fourccs_sowt[]  = { "sowt",         (char*)0 };

static char * fourccs_alaw[]  = { "alaw", (char*)0 };

static char * fourccs_in24[]  = { "in24", (char*)0 };
static char * fourccs_in32[]  = { "in32", (char*)0 };

static char * fourccs_fl32[]  = { "fl32", (char*)0 };
static char * fourccs_fl64[]  = { "fl64", (char*)0 };


static lqt_codec_info_static_t codec_info_ima4 =
  {
    name:                "ima4",
    long_name:           "ima4",
    description:         "The IMA4 compressor reduces 16 bit audio data to 1/4\
 size, with very good quality",
    fourccs:             fourccs_ima4,
    wav_ids:             (int[]){ 0x11, LQT_WAV_ID_NONE },
    type:                LQT_CODEC_AUDIO,
    direction:           LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_AVI | LQT_FILE_QT_OLD | LQT_FILE_QT,
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
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT,
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
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_in24_little =
  {
    name:         "in24_little",
    long_name:    "24 bit PCM (little endian)",
    description:  "24 bit PCM (little endian)",
    fourccs:      fourccs_in24,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_ENCODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_in24_big =
  {
    name:         "in24_big",
    long_name:    "24 bit PCM (big endian)",
    description:  "24 bit PCM (big endian)",
    fourccs:      fourccs_in24,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_ENCODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_in24 =
  {
    name:         "in24",
    long_name:    "24 bit PCM",
    description:  "24 bit PCM",
    fourccs:      fourccs_in24,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_DECODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };

static lqt_codec_info_static_t codec_info_in32_little =
  {
    name:         "in32_little",
    long_name:    "32 bit PCM (little endian)",
    description:  "32 bit PCM (little endian)",
    fourccs:      fourccs_in32,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_ENCODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_in32_big =
  {
    name:         "in32_big",
    long_name:    "32 bit PCM (big endian)",
    description:  "32 bit PCM (big endian)",
    fourccs:      fourccs_in32,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_ENCODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_in32 =
  {
    name:         "in32",
    long_name:    "32 bit PCM",
    description:  "32 bit PCM",
    fourccs:      fourccs_in32,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_DECODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };

/* Floating point */

static lqt_codec_info_static_t codec_info_fl32_little =
  {
    name:         "fl32_little",
    long_name:    "32 bit float (little endian)",
    description:  "32 bit float (little endian)",
    fourccs:      fourccs_fl32,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_ENCODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_fl32_big =
  {
    name:         "fl32_big",
    long_name:    "32 bit float (big endian)",
    description:  "32 bit float (big endian)",
    fourccs:      fourccs_fl32,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_ENCODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_fl32 =
  {
    name:         "fl32",
    long_name:    "32 bit float",
    description:  "32 bit float",
    fourccs:      fourccs_fl32,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_DECODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };

static lqt_codec_info_static_t codec_info_fl64_little =
  {
    name:         "fl64_little",
    long_name:    "64 bit float (little endian)",
    description:  "64 bit float (little endian)",
    fourccs:      fourccs_fl64,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_ENCODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_fl64_big =
  {
    name:         "fl64_little",
    long_name:    "64 bit float (big endian)",
    description:  "64 bit float (big endian)",
    fourccs:      fourccs_fl64,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_ENCODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT,
  };

static lqt_codec_info_static_t codec_info_fl64 =
  {
    name:         "fl64",
    long_name:    "64 bit float",
    description:  "64 bit float",
    fourccs:      fourccs_fl64,
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_DECODE,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0
  };




static lqt_codec_info_static_t codec_info_ulaw =
  {
    name:         "ulaw",
    long_name:    "Ulaw",     /* Long name of the codec */
    description:  "Ulaw",      /* Description            */
    fourccs:      fourccs_ulaw,
    wav_ids:      (int[]){ 0x07, LQT_WAV_ID_NONE },
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT | LQT_FILE_AVI,
  };

static lqt_codec_info_static_t codec_info_alaw =
  {
    name:         "alaw",
    long_name:    "Alaw",     /* Long name of the codec */
    description:  "Alaw",      /* Description            */
    fourccs:      fourccs_alaw,
    wav_ids:      (int[]){ 0x06, LQT_WAV_ID_NONE },
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT | LQT_FILE_AVI,
  };


static lqt_codec_info_static_t codec_info_sowt =
  {
    name:         "sowt",
    long_name:    "Sowt",
    description:  "8/16/24 bit PCM Little endian",
    fourccs:      fourccs_sowt,
    wav_ids:      (int[]){ 0x01, LQT_WAV_ID_NONE },
    type:         LQT_CODEC_AUDIO,
    direction:    LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    compatibility_flags: LQT_FILE_QT_OLD | LQT_FILE_QT | LQT_FILE_AVI,
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 18; }

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
    case 4: /* Sowt */
      return &codec_info_sowt;
      break;
    case 5: /* alaw */
      return &codec_info_alaw;
      break;
    case 6: /* in24_little */
      return &codec_info_in24_little;
      break;
    case 7: /* in24_big */
      return &codec_info_in24_big;
      break;
    case 8: /* in24 */
      return &codec_info_in24;
      break;
    case 9: /* in32_little */
      return &codec_info_in32_little;
      break;
    case 10: /* in32_big */
      return &codec_info_in32_big;
      break;
    case 11: /* in32 */
      return &codec_info_in32;
      break;
    case 12: /* fl32_little */
      return &codec_info_fl32_little;
      break;
    case 13: /* fl32_big */
      return &codec_info_fl32_big;
      break;
    case 14: /* fl32 */
      return &codec_info_fl32;
      break;
    case 15: /* fl64_little */
      return &codec_info_fl64_little;
      break;
    case 16: /* fl64_big */
      return &codec_info_fl64_big;
      break;
    case 17: /* fl64 */
      return &codec_info_fl64;
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
    case 4: /* Sowt */
      return quicktime_init_codec_sowt;
      break;
    case 5: /* Alaw */
      return quicktime_init_codec_alaw;
      break;
    case 6: /* in24_little */
      return quicktime_init_codec_in24_little;
      break;
    case 7: /* in24_big */
      return quicktime_init_codec_in24_big;
      break;
    case 8: /* in24 */
      return quicktime_init_codec_in24;
      break;
    case 9: /* in32_little */
      return quicktime_init_codec_in32_little;
      break;
    case 10: /* in32_big */
      return quicktime_init_codec_in32_big;
      break;
    case 11: /* in32 */
      return quicktime_init_codec_in32;
      break;
    case 12: /* fl32_little */
      return quicktime_init_codec_fl32_little;
      break;
    case 13: /* fl32_big */
      return quicktime_init_codec_fl32_big;
      break;
    case 14: /* fl32 */
      return quicktime_init_codec_fl32;
      break;
    case 15: /* fl64_little */
      return quicktime_init_codec_fl64_little;
      break;
    case 16: /* fl64_big */
      return quicktime_init_codec_fl64_big;
      break;
    case 17: /* fl64 */
      return quicktime_init_codec_fl64;
      break;
    }
  return (lqt_init_audio_codec_func_t)0;
  }
