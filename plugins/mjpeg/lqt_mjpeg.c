#include "../../lqt.h"
#include "../../lqt_codecapi.h"
#include "jpeg.h"

/*
 *  We cheat here: Actually, this is one codec (from the code) but we tell the
 *  outer world, that we are 2 codecs
 */

static char * fourccs_jpeg[]  = { QUICKTIME_JPEG };
static char * fourccs_mjpa[]  = { QUICKTIME_MJPA };

static lqt_codec_info_static_t codec_info_jpeg =
  {
    "jpeg",
    "Jpeg photo",     /* Long name of the codec */
    "This format writes a seperate JPEG photo for every frame in YUV 4:2:0",  /* Description            */
    
    LQT_CODEC_VIDEO,
    LQT_DIRECTION_BOTH
    
  };

static lqt_codec_info_static_t codec_info_mjpa =
  {
    "mjpa",
    "Motion Jpeg A",                                                     /* Long name of the codec */
    "MJPA stores each frame as two JPEGs interlaced and in YUV 4:2:2",   /* Description            */
    LQT_CODEC_VIDEO,
    LQT_DIRECTION_BOTH
  };

static lqt_codec_parameter_info_t encode_parameters_jpeg[] =
  {
     { 
       "jpeg_quality",  /* Name for quicktime_set_parameter */
       "Quality", /* Name for dialog boxes            */
       LQT_PARAMETER_INT, /* Type                             */
       {95 },        /* Default value                    */
       {1  },               /* Minimum value                    */
       {100}                /* Maximum value                    */
     },
     { 
       "jpeg_usefloat",                 /* Name for quicktime_set_parameter */
       "Use float",                     /* Name for dialog boxes            */
       LQT_PARAMETER_INT,                    /* Type                             */
       { 0 },                               /* Default value                    */
       { 0 },                                  /* Minimum value                    */
       { 1 }                                   /* Maximum value                    */
     }
  };




/* These are called from the plugin loader */

extern int get_num_codecs() { return 2; }

extern lqt_codec_info_t * get_codec_info(int index)
  {
  switch(index)
    {
    case 0:
      return lqt_create_codec_info(&codec_info_jpeg,
                                   fourccs_jpeg,
                                   sizeof(fourccs_jpeg)/sizeof(char*),
                                   encode_parameters_jpeg,
                                   sizeof(encode_parameters_jpeg)/sizeof(lqt_codec_parameter_info_t),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
    case 1:
      return lqt_create_codec_info(&codec_info_mjpa,
                                   fourccs_mjpa,
                                   sizeof(fourccs_mjpa)/sizeof(char*),
                                   encode_parameters_jpeg,
                                   sizeof(encode_parameters_jpeg)/sizeof(lqt_codec_parameter_info_t),
                                   (const lqt_codec_parameter_info_t*)0,
                                   0);
    }
  return (lqt_codec_info_t*)0;
  }
     

