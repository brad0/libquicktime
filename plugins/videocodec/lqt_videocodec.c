#include <quicktime/lqt.h>
#include <quicktime/lqt_codecapi.h>
#include "raw.h"

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

static lqt_codec_info_static_t codec_info_raw =
  {
  name:        "Raw",
  long_name:   "RGB uncompressed",
  description: "RGB uncompressed. Allows alpha",
  fourccs:     fourccs_raw,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_v308 =
  {
  name:        "v308",
  long_name:   "8 bit Planar YUV 4:4:4",
  description: "8 bit Planar YUV 4:4:4",
  fourccs:     fourccs_v308,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_v408 =
  {
  name:              "v408",
  long_name:         "8 bit Planar YUVA 4:4:4:4",
  description:       "8 bit Planar YUVA 4:4:4:4",
  fourccs:           fourccs_v408,
  type:              LQT_CODEC_VIDEO,
  direction:         LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_v410 =
  {
  name:        "v410",
  long_name:   "10 bit Planar YUV 4:4:4",
  description: "10 bit Planar YUV 4:4:4:4",
  fourccs:     fourccs_v410,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_yuv2 =
  {
  name:        "yuv2",
  long_name:   "8 bit Packed YUV 4:2:2",     /* Long name of the codec */
  description: "8 bit Packed YUV 4:2:2",      /* Description            */
  fourccs:     fourccs_yuv2,
  type:        LQT_CODEC_VIDEO,
  direction:   LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_yuv4 =
  {
    name:        "yuv4",
    long_name:   "YUV 4:2:0", 
    description: "YUV 4:2:0 NOT COMPATIBLE WITH STANDARD QUICKTIME",
    fourccs:     fourccs_yuv4,
    type:        LQT_CODEC_VIDEO,
    direction:   LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_yv12 =
  {
    name:        "yv12",
    long_name:   "8 bit Planar YUV 4:2:0",
    description: "8 bit Planar YUV 4:2:0",
    fourccs:     fourccs_yv12,
    type:        LQT_CODEC_VIDEO,
    direction:   LQT_DIRECTION_BOTH
    
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 7; }

extern lqt_codec_info_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0: /* raw */
      return lqt_create_codec_info(&codec_info_raw,
                                   (const lqt_parameter_info_static_t*)0,
                                   0,
                                   (const lqt_parameter_info_static_t*)0,
                                   0);
      break;
    case 1: /* v308 */
      return lqt_create_codec_info(&codec_info_v308,
                                   (const lqt_parameter_info_static_t*)0,
                                   0,
                                   (const lqt_parameter_info_static_t*)0,
                                   0);
      break;

    case 2: /* v408 */
      return lqt_create_codec_info(&codec_info_v408,
                                   (const lqt_parameter_info_static_t*)0,
                                   0,
                                   (const lqt_parameter_info_static_t*)0,
                                   0);
      break;
    case 3: /* v410 */
      return lqt_create_codec_info(&codec_info_v410,
                                   (const lqt_parameter_info_static_t*)0,
                                   0,
                                   (const lqt_parameter_info_static_t*)0,
                                   0);
      break;
    case 4: /* yuv2 */
      return lqt_create_codec_info(&codec_info_yuv2,
                                   (const lqt_parameter_info_static_t*)0,
                                   0,
                                   (const lqt_parameter_info_static_t*)0,
                                   0);
      break;
    case 5: /* yuv4 */
      return lqt_create_codec_info(&codec_info_yuv4,
                                   (const lqt_parameter_info_static_t*)0,
                                   0,
                                   (const lqt_parameter_info_static_t*)0,
                                   0);
      break;
    case 6: /* vy12 */
      return lqt_create_codec_info(&codec_info_yv12,
                                   (const lqt_parameter_info_static_t*)0,
                                   0,
                                   (const lqt_parameter_info_static_t*)0,
                                   0);
      break;
    }
  return (lqt_codec_info_t*)0;
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
