#include "version.h"

/*
 *  Return the codec api version. This function will be defined in every
 *  sourcefile, which includes it. This means, that each module must
 *  have exactly one sourcefile which includes it.
 *  This file will be the lqt_* interface file of the module.
 */

#ifndef LQT_LIBQUICKTIME
extern int get_codec_api_version() { return LQT_CODEC_API_VERSION; }
#endif


/*
 *  Functions and datatypes for exclusive
 *  use by codec modules
 */



/*
 *   This defines the actual codec creation function,
 *   which should be returned by get_codec()
 */

typedef void (* lqt_init_video_codec_func_t)(quicktime_video_map_t *);
typedef void (* lqt_init_audio_codec_func_t)(quicktime_audio_map_t *);

typedef struct
  {
  /* Parameter name (to be passed to quicktime_set_parameter() ) */
  
  char * name;
  char * real_name; /* Other name (for making dialogs) */
  
  lqt_parameter_type_t type;
    
  lqt_parameter_value_t val_default;

  /*
   *   Minimum and maximum values:
   *   These are only valid for numeric types and if val_min < val_max
   */

  int val_min;
  int val_max;

  /*
   *  Possible options (only valid for LQT_STRINGLIST)
   *  NULL terminated
   */
  
  char ** stringlist_options;
  
  } lqt_parameter_info_static_t;

/*
 *   This holds the parts of the codec_info structure,
 *   which can be defined statically in the codec source
 */

typedef struct
  {
  char * name;               /* Name of the codec              */
  char * long_name;          /* Long name of the codec         */
  char * description;        /* Description                    */

  /* Fourccs, NULL terminated */
  
  char ** fourccs;
  
  lqt_codec_type type;
  lqt_codec_direction direction;

  lqt_parameter_info_static_t * encoding_parameters;
  lqt_parameter_info_static_t * decoding_parameters;

  int * encoding_colormodels;

  int decoding_colormodel;
  
  } lqt_codec_info_static_t;

/*
 *  Create a lqt_codec_info_t structure from statically defined data
 *
 *  Typically, you will define the lqt_codec_info_static_t as well
 *  as arrays for the (en/de)coding parameters for each codec in the
 *  module.
 *
 *  The get_codec_info() function in your module will then call the
 *  function below to create a lqt_codec_info_t from the argument.
 *  All data are copied, so the returned structure can be used after
 *  closing the module.
 *
 */

lqt_codec_info_t *
lqt_create_codec_info(const lqt_codec_info_static_t * template);


