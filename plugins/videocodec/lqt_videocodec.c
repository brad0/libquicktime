#include "../../lqt.h"
#include "../../lqt_codecapi.h"
#include "raw.h"
/* #include "v308.h" */
/* #include "v408.h" */
/* #include "v410.h" */
/* #include "yuv2.h" */
/* #include "yuv4.h" */
/* #include "yv12.h" */

void quicktime_init_codec_v308(quicktime_video_map_t *vtrack);
void quicktime_init_codec_v408(quicktime_video_map_t *vtrack);
void quicktime_init_codec_v410(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yuv2(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yuv4(quicktime_video_map_t *vtrack);
void quicktime_init_codec_yv12(quicktime_video_map_t *vtrack);


static char * fourccs_raw[]  = { QUICKTIME_RAW };

static char * fourccs_v308[] = { QUICKTIME_YUV444 };

static char * fourccs_v408[] = { QUICKTIME_YUVA4444 };

static char * fourccs_v410[] = { QUICKTIME_YUV444_10bit };

static char * fourccs_yuv2[] = { QUICKTIME_YUV422 };

static char * fourccs_yuv4[] = { QUICKTIME_YUV4 };

static char * fourccs_yv12[] = { QUICKTIME_YUV420 };

static lqt_codec_info_static_t codec_info_raw =
  {
  "Raw",
  "RGB uncompressed",                    /* Long name of the codec         */
  "RGB uncompressed. Allows alpha",     /* Description                    */

  LQT_CODEC_VIDEO,
  LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_v308 =
  {
  "v308",
  "8 bit Planar YUV 4:4:4",     /* Long name of the codec */
  "8 bit Planar YUV 4:4:4",      /* Description            */

  LQT_CODEC_VIDEO,
  LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_v408 =
  {
  "v408",
  "8 bit Planar YUVA 4:4:4:4",     /* Long name of the codec */
  "8 bit Planar YUVA 4:4:4:4",      /* Description            */

  LQT_CODEC_VIDEO,
  LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_v410 =
  {
  "v410",
  "10 bit Planar YUV 4:4:4",     /* Long name of the codec */
  "10 bit Planar YUV 4:4:4:4",      /* Description            */

  LQT_CODEC_VIDEO,
  LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_yuv2 =
  {
  "yuv2",
  "8 bit Packed YUV 4:2:2",     /* Long name of the codec */
  "8 bit Packed YUV 4:2:2",      /* Description            */

  LQT_CODEC_VIDEO,
  LQT_DIRECTION_BOTH
    
  };

static lqt_codec_info_static_t codec_info_yuv4 =
  {
  "yuv4", 
  "YUV 4:2:0",     /* Long name of the codec */ 
  "YUV 4:2:0 NOT COMPATIBLE WITH STANDARD QUICKTIME",      /* Description            */
  LQT_CODEC_VIDEO,
  LQT_DIRECTION_BOTH
  };

static lqt_codec_info_static_t codec_info_yv12 =
  {
    "yv12",
    "8 bit Planar YUV 4:2:0",     /* Long name of the codec */
    "8 bit Planar YUV 4:2:0",      /* Description            */
    
    LQT_CODEC_VIDEO,
    LQT_DIRECTION_BOTH
    
  };

/* These are called from the plugin loader */

extern int get_num_codecs() { return 7; }

extern lqt_codec_info_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0: /* raw */
      return lqt_create_codec_info(&codec_info_raw,
                                   fourccs_raw,
                                   sizeof(fourccs_raw)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
      break;
    case 1: /* v308 */
      return lqt_create_codec_info(&codec_info_v308,
                                   fourccs_v308,
                                   sizeof(fourccs_v308)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
      break;

    case 2: /* v408 */
      return lqt_create_codec_info(&codec_info_v408,
                                   fourccs_v408,
                                   sizeof(fourccs_v408)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
      break;
    case 3: /* v410 */
      return lqt_create_codec_info(&codec_info_v410,
                                   fourccs_v410,
                                   sizeof(fourccs_v410)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
      break;
    case 4: /* yuv2 */
      return lqt_create_codec_info(&codec_info_yuv2,
                                   fourccs_yuv2,
                                   sizeof(fourccs_yuv2)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
      break;
    case 5: /* yuv4 */
      return lqt_create_codec_info(&codec_info_yuv4,
                                   fourccs_yuv4,
                                   sizeof(fourccs_yuv4)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
      break;
    case 6: /* vy12 */
      return lqt_create_codec_info(&codec_info_yv12,
                                   fourccs_yv12,
                                   sizeof(fourccs_yv12)/sizeof(char*),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0,
                                   (const lqt_codec_parameter_info_t*)0,
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
