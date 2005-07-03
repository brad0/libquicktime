#include "config.h"
#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include "raw.h"

#include <quicktime/colormodels.h>

void quicktime_init_codec_v308(quicktime_video_map_t *vtrack);
void quicktime_init_codec_v408(quicktime_video_map_t *vtrack);
void quicktime_init_codec_v410(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yuv2(quicktime_video_map_t *vtrack);
void quicktime_init_codec_2vuy(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yuv4(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yv12(quicktime_video_map_t *vtrack);

static char * fourccs_raw[]  = { QUICKTIME_RAW, "raw3",   (char*)0 };

static char * fourccs_v308[] = { QUICKTIME_YUV444,        (char*)0 };

static char * fourccs_v408[] = { QUICKTIME_YUVA4444 ,     (char*)0 };

static char * fourccs_v410[] = { QUICKTIME_YUV444_10bit , (char*)0 };

static char * fourccs_yuv2[] = { QUICKTIME_YUV422 ,       (char*)0 };

static char * fourccs_2vuy[] = { QUICKTIME_2VUY ,         (char*)0 };

static char * fourccs_yuv4[] = { QUICKTIME_YUV4 ,         (char*)0 };

static char * fourccs_yv12[] = { QUICKTIME_YUV420 ,       (char*)0 };

// if DUMMY_PARAMETERS is defined it will cause segfaults
#undef DUMMY_PARAMETERS

#ifdef DUMMY_PARAMETERS

static char * dummy_stringlist_options[] =
  {
    "Option 1",
    "Option 2",
    "Option 3",
    (char*)0
  };

static lqt_parameter_info_static_t dummy_parameters[] =
  {
     {
       name:               "dummy_string_test",
       real_name:          "String Test",
       type:               LQT_PARAMETER_STRING,
       val_default:        { (int)"String Test" },
       val_min:            0,
       val_max:            0,
       stringlist_options: (char**)0
     },
     { 
       name:      "dummy_stringlist_test",
       real_name: "Stringlist test",
       type:      LQT_PARAMETER_STRINGLIST,
       val_default:        { (int)"Option1" },
       val_min:            0,
       val_max:            0,
       stringlist_options: dummy_stringlist_options
     },
     { /* End of array */ }
  };

#endif /* DUMMY_PARAMETERS */

static lqt_codec_info_static_t codec_info_raw =
  {
  name:        "raw",
  long_name:   "RGB uncompressed",
  description: "RGB uncompressed. Allows alpha",
  fourccs:     fourccs_raw,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH,
#ifdef DUMMY_PARAMETERS
  encoding_parameters: dummy_parameters,
  decoding_parameters: dummy_parameters,
#else
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,
#endif
  };

static lqt_codec_info_static_t codec_info_v308 =
  {
  name:        "v308",
  long_name:   "8 bit Planar YUV 4:4:4 (v308)",
  description: "8 bit Planar YUV 4:4:4 (v308)",
  fourccs:     fourccs_v308,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH,
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,
  };

static lqt_codec_info_static_t codec_info_v408 =
  {
  name:              "v408",
  long_name:         "8 bit Planar YUVA 4:4:4:4 (v408)",
  description:       "8 bit Planar YUVA 4:4:4:4 (v408)",
  fourccs:           fourccs_v408,
  type:              LQT_CODEC_VIDEO,
  direction:         LQT_DIRECTION_BOTH,
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,

  };

static lqt_codec_info_static_t codec_info_v410 =
  {
  name:        "v410",
  long_name:   "10 bit Planar YUV 4:4:4 (v410)",
  description: "10 bit Planar YUV 4:4:4 (v410)",
  fourccs:     fourccs_v410,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH,
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,
  };

static lqt_codec_info_static_t codec_info_yuv2 =
  {
  name:        "yuv2",
  long_name:   "8 bit Packed YUV 4:2:2 (yuv2)",     /* Long name of the codec */
  description: "8 bit Packed YUV 4:2:2 (yuv2)",      /* Description            */
  fourccs:     fourccs_yuv2,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH,
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,

  };

static lqt_codec_info_static_t codec_info_2vuy =
  {
  name:        "2vuy",
  long_name:   "8 bit Packed YUV 4:2:2 (2vuy)",     /* Long name of the codec */
  description: "8 bit Packed YUV 4:2:2 (2vuy)",      /* Description            */
  fourccs:     fourccs_2vuy,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH,
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,

  };

static lqt_codec_info_static_t codec_info_yuv4 =
  {
    name:        "yuv4",
    long_name:   "YUV 4:2:0", 
    description: "YUV 4:2:0 (yuv4) NOT COMPATIBLE WITH STANDARD QUICKTIME",
    fourccs:     fourccs_yuv4,
    type:        LQT_CODEC_VIDEO,
    direction:   LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,

  };

static lqt_codec_info_static_t codec_info_yv12 =
  {
    name:        "yv12",
    long_name:   "8 bit Planar YUV 4:2:0",
    description: "8 bit Planar YUV 4:2:0 (yv12)",
    fourccs:     fourccs_yv12,
    type:        LQT_CODEC_VIDEO,
    direction:   LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 8; }

extern lqt_codec_info_static_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0: /* raw */
      return &codec_info_raw;
      break;
    case 1: /* v308 */
      return &codec_info_v308;
      break;
    case 2: /* v408 */
      return &codec_info_v408;
      break;
    case 3: /* v410 */
      return &codec_info_v410;
      break;
    case 4: /* yuv2 */
      return &codec_info_yuv2;
      break;
    case 5: /* yuv4 */
      return &codec_info_yuv4;
      break;
    case 6: /* vy12 */
      return &codec_info_yv12;
      break;
    case 7: /* 2vuy */
      return &codec_info_2vuy;
      break;
    }
  return (lqt_codec_info_static_t*)0;
  }

extern lqt_init_video_codec_func_t get_video_codec(int index)
  {
  switch(index)
    {
    case 0: /* raw */
      return quicktime_init_codec_raw;
      break;
    case 1: /* v308 */
      return quicktime_init_codec_v308;
      break;
    case 2: /* v408 */
      return quicktime_init_codec_v408;
      break;
    case 3: /* v410 */
    return quicktime_init_codec_v410;
      break;
    case 4: /* yuv2 */
      return quicktime_init_codec_yuv2;
      break;
    case 5: /* yuv4 */
      return quicktime_init_codec_yuv4;
      break;
    case 6: /* vy12 */
      return quicktime_init_codec_yv12;
      break;
    case 7: /* 2vuy */
      return quicktime_init_codec_2vuy;
      break;
    }
  return (lqt_init_video_codec_func_t)0;
  }

int get_stream_colormodel(quicktime_t * file, int track,
                          int codec_index)
  {
  int depth;
  
  if(codec_index == 0)
    {
    depth = quicktime_video_depth(file, track);
    switch(depth)
      {
      case 32:
        return BC_RGBA8888;
        break;
      default:
        return BC_RGB888;
        break;
      }
    }
  return LQT_COLORMODEL_NONE;
  }
