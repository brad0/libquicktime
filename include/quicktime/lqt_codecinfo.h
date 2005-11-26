/*
 *   Codec info structure for libquicktime
 *   (first approximation)
 */

/* Type of a codec parameter */

#ifndef _LQT_CODEC_INFO_H_
#define _LQT_CODEC_INFO_H_

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
   \defgroup codec_registry Codec registry
   \brief Informations about installing codecs

   One of the goals when forking libquicktime was to have a modular structure.
   Codecs were moved to plugins and were compiled according to the dependencies
   found by the configure script. As a result, a mechanism became necessary for
   finding informations about available codecs at runtime, their properties, supported
   parameters etc.

   The codec parameters are defined in a way that GUI configuration dialogs can be
   built at runtime. An example for this is the libquicktime_config utility.

   Libquicktime saves data of the codecs in the file .libquicktime_codecs in your
   home directory. This saves the long time needed for opening each codec module
   to see what's inside. The codec registry can be configured with the libquicktime_config
   program.
*/
  
/* 
 * Compatibility flags for encoders
 */
  
#define LQT_CODEC_COMPATIBILITY_QUICKTIME (1<<0) /* Codec can be used for Quicktime files       */
#define LQT_CODEC_COMPATIBILITY_AVI       (1<<1) /* Codec can be used for AVI files             */
#define LQT_CODEC_COMPATIBILITY_HVIRTUAL  (1<<2) /* Codec can be played back by Heroine Virtual */

/*
 *  Note, that additional codecs (e.g. Divx) might be required for the following 
 *  2 Programs
 */
  
#define LQT_CODEC_COMPATIBILITY_APPLE     (1<<3) /* Codec can be played back by Apple Quicktime */
#define LQT_CODEC_COMPATIBILITY_WMP       (1<<4) /* Codec can be played by Windows Media Player */


/**
   \defgroup codec_parameters Structures describing codec parameters
   \ingroup codec_registry
   \brief Informations about supported codec parameters
*/
  
/**
   \ingroup codec_parameters
   \brief Parameter types

   These describe the datatypes of a parameter.
 */

typedef enum
  {
    LQT_PARAMETER_INT,     /*!< Integer */
    LQT_PARAMETER_STRING,  /*!< String  */
    LQT_PARAMETER_STRINGLIST, /*!< String with fixed set of options */
    /* This dummy type is used to separate sections (real_name will be on tab-label) */
    LQT_PARAMETER_SECTION, /*!< Dummy type to group parameters into sections. */
  } lqt_parameter_type_t;

/**
   \ingroup codec_parameters
   \brief Union for holding parameter values

 */

typedef union
  {
  int val_int;
  char * val_string;
  } lqt_parameter_value_t;

/** \ingroup codec_parameters
 *  \brief Structure describing a parameter
 *
 * This completely describes a parameter. Bool parameters will have the type
 * \ref LQT_PARAMETER_INT , val_min = 0 and val_max = 1.
 */

typedef struct
  {
  char * name;   /*!< Parameter name to be passed to on of the parameter setting functions */

  char * real_name; /*!< More human readable name for configuration dialogs */
  
  lqt_parameter_type_t type; /*!< Datatype */
  
  lqt_parameter_value_t val_default; /*!< Default value */
  
  /*
   *   Minimum and maximum values:
   *   These are only valid for numeric types and if val_min < val_max
   */

  int val_min; /*!< Minimum value for integer parameter */
  int val_max; /*!< Maximum value for integer parameter */

  /*
   *  Possible options (only valid for LQT_STRINGLIST)
   */
  
  int num_stringlist_options; /*!< Number of options for \ref LQT_PARAMETER_STRINGLIST */
  char ** stringlist_options; /*!< Options for \ref LQT_PARAMETER_STRINGLIST */
  
  } lqt_parameter_info_t;

/** \ingroup codec_registry
    \brief Type of a codec (Audio or video)
*/

typedef enum
  {
    LQT_CODEC_AUDIO,
    LQT_CODEC_VIDEO
  } lqt_codec_type;

/** \ingroup codec_registry
    \brief Direction of the codec
*/
  
typedef enum
  {
    LQT_DIRECTION_ENCODE,
    LQT_DIRECTION_DECODE,
    LQT_DIRECTION_BOTH
  } lqt_codec_direction;

/** \ingroup codec_registry
    \brief Structure describing a codec
*/
  
typedef struct lqt_codec_info_s
  {
  int compatibility_flags; /*!< Compatibility flags (not used right now) */

  /* These are set by the plugins */
  
  char * name;               /*!< Name of the codec (used internally) */
  char * long_name;          /*!< More human readable name of the codec         */
  char * description;        /*!< Description                    */

  lqt_codec_type type;           /*!< Type (audio or video) */
  lqt_codec_direction direction; /*!< Direction (encode, decode or both) */
  
  int num_fourccs;      /*!< Number of fourccs (Four character codes), this codec can handle */
  char ** fourccs;      /*!< Fourccs this codec can handle */

  int num_wav_ids; /*!< Number of M$ wav ids, this codec can handle */
  int * wav_ids;   /*!< Wav ids, this codec can handle (for AVI only) */
  
  
  int num_encoding_parameters; /*!< Number of encoding parameters */
  lqt_parameter_info_t * encoding_parameters; /*!< Encoding parameters */

  int num_decoding_parameters; /*!< Number of decoding parameters */
  lqt_parameter_info_t * decoding_parameters; /*!< Decoding parameters */

  /* The following members are set by libquicktime      */
  
  char * module_filename;    /*!< Filename of the module */
  int module_index;          /*!< Index inside the module */
  
  uint32_t file_time;        /*!< File modification time of the module */
  
  struct lqt_codec_info_s * next;   /*!< For chaining (used internally only) */
  } lqt_codec_info_t;


/* Global Entry points */

/** \ingroup codec_registry
    \brief Initialize the codec registry

    Under normal circumstances, you never need to call this function, since the
    registry is always initialized on demand.
 */

void lqt_registry_init();

/** \ingroup codec_registry
 *  \brief Destroy the codec registry
 *
 *  
 *   This frees memory for the whole codec database
 *   Is is normally called automatically, but you will need to call
 *   it exclicitely, if you want to reinitialize the codec registry at runtime
 */

void lqt_registry_destroy();

/* \ingroup codec_registry
 *
 * Save the registry file $HOME/.libquicktime_codecs.
 * Under normal circumstances, you never need to call this function
 */

void lqt_registry_write();


/******************************************************
 *  Non thread save functions for querying the
 *  codec registry. Suitable for single threaded
 *  applications (might become obsolete)
 ******************************************************/

/** \ingroup codec_registry
 * \brief Return the number of installed audio codecs
 * \returns Number of installed audio codecs
 *
 * This function is obsolete, use \ref lqt_query_registry instead
 */

int lqt_get_num_audio_codecs();

/** \ingroup codec_registry
 * \brief Return the number of installed video codecs
 * \returns Number of installed video codecs
 *
 * This function is obsolete, use \ref lqt_query_registry instead
 */

int lqt_get_num_video_codecs();

/** \ingroup codec_registry
 * \brief Return an audio codec
 * \param index Index of the codec
 * \returns Pointer to the codec info in the internal database
 *
 * This function is obsolete, use \ref lqt_query_registry instead
 */

const lqt_codec_info_t * lqt_get_audio_codec_info(int index);

/** \ingroup codec_registry
 * \brief Return a video codec
 * \param index Index of the codec
 * \returns Pointer to the codec info in the internal database
 *
 * This function is obsolete, use \ref lqt_query_registry instead
 */
  
const lqt_codec_info_t * lqt_get_video_codec_info(int index);

/********************************************************************
 *  Thread save function for getting codec parameters
 *  All these functions return a NULL terminated array of local
 *  copies of the codec data which must be freed using 
 *  lqt_destroy_codec_info(lqt_codec_info_t ** info) declared below
 ********************************************************************/

/** \ingroup codec_registry
 *  \brief Return an array of any combination of audio/video en/decoders
 *  \param audio Set this to 1 if you want audio codecs to be returned
 *  \param video Set this to 1 if you want video codecs to be returned
 *  \param encode Set this to 1 if you want encoders to be returned
 *  \param encode Set this to 1 if you want decoders to be returned
 *  \returns A NULL terminated array of codec infos.
 *
 *  This function copies the codec infos. Use \ref lqt_destroy_codec_info
 *  to free the returned array.
 */

lqt_codec_info_t ** lqt_query_registry(int audio, int video,
                                       int encode, int decode);


/** \ingroup codec_registry
 *  \brief Find an audio codec by its name
 *  \param name Short name of the codec (see \ref lqt_codec_info_s)
 *  \returns A NULL terminated array containing one codec info if the codec was found
 *
 *  This function copies the codec info. Use \ref lqt_destroy_codec_info
 *  to free the returned array.
 */

lqt_codec_info_t ** lqt_find_audio_codec_by_name(const char * name);

/** \ingroup codec_registry
 *  \brief Find a video codec by its name
 *  \param name Short name of the codec (see \ref lqt_codec_info_s)
 *  \returns A NULL terminated array containing one codec info if the codec was found
 *
 *  This function copies the codec info. Use \ref lqt_destroy_codec_info
 *  to free the returned array.
 */
  
lqt_codec_info_t ** lqt_find_video_codec_by_name(const char * name);

/** \ingroup codec_registry
 *  \brief Get an audio codec from an open file.
 *  \param file A quicktime handle
 *  \param track Index if the track (starting with 0)
 *  \returns A NULL terminated array containing one codec info if the codec was found
 *
 *  This function copies the codec info. Use \ref lqt_destroy_codec_info
 *  to free the returned array.
 */
  
  
lqt_codec_info_t ** lqt_audio_codec_from_file(quicktime_t * file, int track);

/** \ingroup codec_registry
 *  \brief Get a video codec from an open file.
 *  \param file A quicktime handle
 *  \param track Index if the track (starting with 0)
 *  \returns A NULL terminated array containing one codec info if the codec was found
 *
 *  This function copies the codec info. Use \ref lqt_destroy_codec_info
 *  to free the returned array.
 */

lqt_codec_info_t ** lqt_video_codec_from_file(quicktime_t * file, int track);
  
/** \ingroup codec_registry
 *  \brief Destroy a codec info array
 *  \param info A NULL terminated array of codec infos returned by one of the query functions.
 * 
 *  Destroys a NULL terminared codec info structure returned by
 *  any of the query functions and frees all associated memory.
 */

void lqt_destroy_codec_info(lqt_codec_info_t ** info);

/** \ingroup codec_registry
 *  \brief Reorder audio codecs
 *  \param codec_info A NULL terminated array of codec infos
 *
 *  The codec order is important if there is more than one codec available
 *  for a given fourcc. In this case, the first one will be used.
 *  You can simply call \ref lqt_query_registry for getting audio
 *  codecs, reorder the returned array and pass this to the functions
 *  below.
 */

void lqt_reorder_audio_codecs(lqt_codec_info_t ** codec_info);

/** \ingroup codec_registry
 *  \brief Reorder video codecs
 *  \param codec_info A NULL terminated array of codec infos
 *
 *  The codec order is important if there is more than one codec available
 *  for a given fourcc. In this case, the first one will be used.
 *  You can simply call \ref lqt_query_registry for getting video
 *  codecs, reorder the returned array and pass this to the functions
 *  below.
 */

void lqt_reorder_video_codecs(lqt_codec_info_t ** codec_info);

/** \ingroup codec_registry
 *  \brief Change a default value for a codec parameter
 *  \param type The type of the codec (audio or video)
 *  \param parameter_name The short name of the parameter to change
 *  \param val The new value for the parameter
 *
 *  This stores a new default parameter value in the codec registry.
 *  It will become the new default for all files, you open after.
 *  This will also be saved in $HOME/.libquicktime_config.
 */

void lqt_set_default_parameter(lqt_codec_type type, int encode,
                               const char * codec_name,
                               const char * parameter_name,
                               lqt_parameter_value_t * val);

/** \ingroup codec_registry
 *  \brief Restore a default parameter from the codec module
 *  \param info A codec info referring to the codec
 *  \param encode Set this to 1 to restore encoding parameter, 0 else
 *  \param decode Set this to 1 to restore decoding parameter, 0 else
 *
 *  This will revert previous calls to \ref lqt_set_default_parameter by
 *  loading the module and getting the hardcoded values.
 */

void lqt_restore_default_parameters(lqt_codec_info_t * codec_info,
                                    int encode, int decode);
    
                                      
/** \ingroup codec_registry
 *  \brief Dump a codec info to stderr
 *  \param info A codec info
 *
 *  Dump a human readable description of the codec info to stderr. For testing and
 *  debugging only.
 */

void lqt_dump_codec_info(const lqt_codec_info_t * info);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _LQT_CODEC_INFO_H_ */
