/*
 *   Codec info structure for libquicktime
 *   (first approximation)
 */

/* Type of a codec parameter */

#ifndef _LQT_CODEC_INFO_H_
#define _LQT_CODEC_INFO_H_

/*
 *  Parameter types:
 *  Right now, only int is present, but the structure itself
 *  should work for more types
 */

typedef enum
  {
    LQT_PARAMETER_INT,
    LQT_PARAMETER_STRING,
    LQT_PARAMETER_STRINGLIST /* String with options */
  } lqt_parameter_type_t;

/*
 *   Value of a codec parameter
 */

typedef union
  {
  int val_int;
  char * val_string;
  } lqt_parameter_value_t;

/*
 *
 */

typedef struct
  {
  char * name;         /* Parameter name (to be passed to quicktime_set_parameter() ) */
  char * real_name;    /* Other name (for making dialogs)                             */
  
  lqt_parameter_type_t type;
    
  lqt_parameter_value_t val_default;

  /*
   *   Minimum and maximum values:
   *   These are only valid for numeric types and if val_min < val_max
   */

  lqt_parameter_value_t val_min;
  lqt_parameter_value_t val_max;

  /*
   *  Possible options (only valid for LQT_STRINGLIST)
   */
  
  int num_stringlist_options;
  char ** stringlist_options;
  
  } lqt_parameter_info_t;

/*
 *   This is the structre, returned by the plugin
 */

typedef enum
  {
    LQT_CODEC_AUDIO,
    LQT_CODEC_VIDEO
  } lqt_codec_type;

typedef enum
  {
    LQT_DIRECTION_ENCODE,
    LQT_DIRECTION_DECODE,
    LQT_DIRECTION_BOTH
  } lqt_codec_direction;

struct lqt_codec_info_s
  {
  /* These are set by the plugins */
  
  char * name;               /* Name of the codec              */
  char * long_name;          /* Long name of the codec         */
  char * description;        /* Description                    */

  lqt_codec_type type;
  lqt_codec_direction direction;
  
  int num_fourccs;           /* Fourccs, this codec can handle */
  char ** fourccs;

  int num_encoding_parameters;
  lqt_parameter_info_t * encoding_parameters;

  int num_decoding_parameters;
  lqt_parameter_info_t * decoding_parameters;
  
  /* The following members are set by libquicktime      */
  
  char * module_filename;    /* Filename of the module  */
  int module_index;          /* Index inside the module */
  
  uint32_t file_time;        /* File modification time  */

  struct lqt_codec_info_s * next;   /* For chaining */
  };

typedef struct lqt_codec_info_s lqt_codec_info_t;

/* Global Entry points */

/*
 *   Scan the codec info files and the codec directories
 */

void lqt_registry_init();


/*
 *   This frees memory for the whole codec database
 *   Is is normally called automatically, but you will need to call
 *   it exclicitely, if you want to reinitialize the codec database
 */

void lqt_registry_destroy();

/*
 *  Routines, you need, when you want to build configuration dialogs
 *  or other fun stuff
 */

/*
 *  Get the numbers of codecs
 */

int lqt_get_num_audio_codecs();

int lqt_get_num_video_codecs();

/*
 *   Get corresponding info structures
 *   These point to the original database entries,
 *   so they are returned as const here
 */

const lqt_codec_info_t * lqt_get_audio_codec_info(int index);

const lqt_codec_info_t * lqt_get_video_codec_info(int index);

/*
 *   Get corresponding info structures
 *   This creates a local copy of the info structure,
 *   which must be freed by the caller
 */

lqt_codec_info_t * lqt_get_audio_codec_info_c(int index);

lqt_codec_info_t * lqt_get_video_codec_info_c(int index);

/*
 *  Destroys the codec info structure returned by the functions
 *  above
 */

void lqt_destroy_codec_info(lqt_codec_info_t * info);

/*
 *  Dump codec info, only for debugging + testing
 */

void lqt_dump_codec_info(const lqt_codec_info_t * info);
  

#endif /* _LQT_CODEC_INFO_H_ */
