/*
 *   Codec info structure for libquicktime
 *   (first approximation)
 */

/* Type of a codec parameter */

#ifndef _LQT_CODEC_INFO_H_
#define _LQT_CODEC_INFO_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


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

  int num_wav_ids;
  int * wav_ids;
  
  
  int num_encoding_parameters;
  lqt_parameter_info_t * encoding_parameters;

  int num_decoding_parameters;
  lqt_parameter_info_t * decoding_parameters;

  /* Colormodels this codec can handle */
  
  int num_encoding_colormodels;
  int * encoding_colormodels;

  /*
   *  Colormodel for decoding.
   *  Must be set to LQT_COLORMODEL_NONE if the stream colormodel
   *  must be obtained at runtime by the codec
   */
  
  int decoding_colormodel;
  
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
 *  Save the registry file ~/.libquicktime_codecs
 *  (locks the registry)
 */

void lqt_registry_write();


/******************************************************
 *  Non thread save functions for querying the
 *  codec registry. Suitable for single threaded
 *  applications (might become obsolete)
 ******************************************************/

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

/********************************************************************
 *  Thread save function for getting codec parameters
 *  All these functions return a NULL terminated array of local
 *  copies of the codec data which must be freed using 
 *  lqt_destroy_codec_info(lqt_codec_info_t ** info) declared below
 ********************************************************************/

/*
 *  Return an array of any combination of audio/video en/decoders
 */

lqt_codec_info_t ** lqt_query_registry(int audio, int video,
                                       int encode, int decode);

/*
 *  Find a codec by it's unique (short) name
 */

lqt_codec_info_t ** lqt_find_audio_codec_by_name(const char * name);

lqt_codec_info_t ** lqt_find_video_codec_by_name(const char * name);

/*
 *  Get infos about the Codecs of a file
 *  To be called after quicktime_open() when reading
 *  or quicktime_set_audio()/quicktime_set_video() when writing
 */

lqt_codec_info_t ** lqt_audio_codec_from_file(quicktime_t *, int track);

lqt_codec_info_t ** lqt_video_codec_from_file(quicktime_t *, int track);

/*
 *  Reorder video and audio codecs in the registry.
 *  The argument is a NULL terminated array
 *  of codec info structures as returned by the above functions.
 *  You can simply call lqt_query_registry() for getting audio or video
 *  codecs, reorder the returned array and pass this to the functions
 *  below.
 */

void lqt_reorder_audio_codecs(lqt_codec_info_t ** codec_info);
void lqt_reorder_video_codecs(lqt_codec_info_t ** codec_info);
 
  
/*
 *  Destroys the codec info structure returned by the functions
 *  above
 */

void lqt_destroy_codec_info(lqt_codec_info_t ** info);

  
/******************************************************************
 * Store default values in the registry (also thread save)
 ******************************************************************/

void lqt_set_default_parameter(lqt_codec_type type, int encode,
                               const char * codec_name,
                               const char * parameter_name,
                               lqt_parameter_value_t * val);

/*****************************************************************
 * Restore codec parameters from the module
 *****************************************************************/

void lqt_restore_default_parameters(lqt_codec_info_t * codec_info,
                                    int encode, int decode);
    
                                      
/*
 *  Dump codec info, only for debugging + testing
 */

void lqt_dump_codec_info(const lqt_codec_info_t * info);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _LQT_CODEC_INFO_H_ */
