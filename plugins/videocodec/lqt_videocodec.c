#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include "raw.h"

#include <quicktime/colormodels.h>

void quicktime_init_codec_v308(quicktime_video_map_t *vtrack);
void quicktime_init_codec_v408(quicktime_video_map_t *vtrack);
void quicktime_init_codec_v410(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yuv2(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yuv4(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yv12(quicktime_video_map_t *vtrack);

static char * fourccs_raw[]  = { QUICKTIME_RAW,           (char*)0 };

static char * fourccs_v308[] = { QUICKTIME_YUV444,        (char*)0 };

static char * fourccs_v408[] = { QUICKTIME_YUVA4444 ,     (char*)0 };

static char * fourccs_v410[] = { QUICKTIME_YUV444_10bit , (char*)0 };

static char * fourccs_yuv2[] = { QUICKTIME_YUV422 ,       (char*)0 };

static char * fourccs_yuv4[] = { QUICKTIME_YUV4 ,         (char*)0 };

static char * fourccs_yv12[] = { QUICKTIME_YUV420 ,       (char*)0 };

static int encoding_colormodels_raw[] =
  {
    BC_RGB888,
    BC_ARGB8888,
    LQT_COLORMODEL_NONE
  };

static int encoding_colormodels_v308[] =
  {
    BC_VYU888,
    LQT_COLORMODEL_NONE,
  };

static int encoding_colormodels_v408[] =
  {
    BC_UYVA8888,
    LQT_COLORMODEL_NONE,
  };

static int encoding_colormodels_v410[] =
  {
    BC_YUV101010,
    LQT_COLORMODEL_NONE,
  };

static int encoding_colormodels_yuv2[] =
  {
    BC_YUV422,
    LQT_COLORMODEL_NONE,
  };

static int encoding_colormodels_yuv4[] =
  {
    BC_RGB888,
    LQT_COLORMODEL_NONE,
  };

static int encoding_colormodels_yv12[] =
  {
    BC_YUV420P,
    LQT_COLORMODEL_NONE,
  };

#define DUMMY_PARAMETERS

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
  encoding_colormodels: encoding_colormodels_raw,
  decoding_colormodel:  LQT_COLORMODEL_NONE
  };

static lqt_codec_info_static_t codec_info_v308 =
  {
  name:        "v308",
  long_name:   "8 bit Planar YUV 4:4:4",
  description: "8 bit Planar YUV 4:4:4",
  fourccs:     fourccs_v308,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH,
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,
  encoding_colormodels: encoding_colormodels_v308,
  decoding_colormodel:  BC_VYU888
  };

static lqt_codec_info_static_t codec_info_v408 =
  {
  name:              "v408",
  long_name:         "8 bit Planar YUVA 4:4:4:4",
  description:       "8 bit Planar YUVA 4:4:4:4",
  fourccs:           fourccs_v408,
  type:              LQT_CODEC_VIDEO,
  direction:         LQT_DIRECTION_BOTH,
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,
  encoding_colormodels: encoding_colormodels_v408,
  decoding_colormodel:  BC_UYVA8888

  };

static lqt_codec_info_static_t codec_info_v410 =
  {
  name:        "v410",
  long_name:   "10 bit Planar YUV 4:4:4",
  description: "10 bit Planar YUV 4:4:4:4",
  fourccs:     fourccs_v410,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH,
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,
  encoding_colormodels: encoding_colormodels_v410,
  decoding_colormodel: BC_YUV101010
  };

static lqt_codec_info_static_t codec_info_yuv2 =
  {
  name:        "yuv2",
  long_name:   "8 bit Packed YUV 4:2:2",     /* Long name of the codec */
  description: "8 bit Packed YUV 4:2:2",      /* Description            */
  fourccs:     fourccs_yuv2,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH,
  encoding_parameters: (lqt_parameter_info_static_t*)0,
  decoding_parameters: (lqt_parameter_info_static_t*)0,
  encoding_colormodels: encoding_colormodels_yuv2,
  decoding_colormodel: BC_YUV422

  };

static lqt_codec_info_static_t codec_info_yuv4 =
  {
    name:        "yuv4",
    long_name:   "YUV 4:2:0", 
    description: "YUV 4:2:0 NOT COMPATIBLE WITH STANDARD QUICKTIME",
    fourccs:     fourccs_yuv4,
    type:        LQT_CODEC_VIDEO,
    direction:   LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    encoding_colormodels: encoding_colormodels_yuv4,
    decoding_colormodel: BC_RGB888

  };

static lqt_codec_info_static_t codec_info_yv12 =
  {
    name:        "yv12",
    long_name:   "8 bit Planar YUV 4:2:0",
    description: "8 bit Planar YUV 4:2:0",
    fourccs:     fourccs_yv12,
    type:        LQT_CODEC_VIDEO,
    direction:   LQT_DIRECTION_BOTH,
    encoding_parameters: (lqt_parameter_info_static_t*)0,
    decoding_parameters: (lqt_parameter_info_static_t*)0,
    encoding_colormodels: encoding_colormodels_yv12,
    decoding_colormodel: BC_YUV420P
    
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 7; }

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
    }
  return (lqt_init_video_codec_func_t)0;
  }

int get_stream_colormodel(quicktime_t * file, int track,
                          int codec_index, int * exact)
  {
  int depth;
  if(exact)
    *exact = 1;
  
  if(codec_index == 0)
    {
    depth = quicktime_video_depth(file, track);
    switch(depth)
      {
      case 24:
        return BC_RGB888;
        break;
      case 32:
        return BC_ARGB8888;
        break;
      default:
        return LQT_COLORMODEL_NONE; /* This should never happen... */
        break;
      }
    }
  return LQT_COLORMODEL_NONE; /* And this neither */
  }
