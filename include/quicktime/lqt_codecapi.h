#include <quicktime/lqt_version.h>
#include <quicktime/qtprivate.h>

/*
 *  Return the codec api version. This function will be defined in every
 *  sourcefile, which includes it. This means, that each module must
 *  have exactly one sourcefile which includes it.
 *  This file will be the lqt_* interface file of the module.
 */

#ifndef LQT_LIBQUICKTIME
extern int get_codec_api_version() { return LQT_CODEC_API_VERSION; }
#endif

#define LQT_WAV_ID_NONE -1

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
  int compatibility_flags;

  char * name;               /* Name of the codec              */
  char * long_name;          /* Long name of the codec         */
  char * description;        /* Description                    */

  /* Fourccs, NULL terminated */
  
  char ** fourccs;

  /* WAV IDs, terminated with LQT_WAV_ID_NONE */

  int * wav_ids;
    
  lqt_codec_type type;
  lqt_codec_direction direction;

  lqt_parameter_info_static_t * encoding_parameters;
  lqt_parameter_info_static_t * decoding_parameters;
  
  } lqt_codec_info_static_t;

/*
 *  Create a lqt_codec_info_t structure from statically defined data
 *
 *  Typically, you will define the lqt_codec_info_static_t as well
 *  as arrays for the (en/de)coding parameters for each codec in the
 *  module.
 *
 *  The get_codec_info() function in your module will then call the
codec->decode_buffer *  function below to create a lqt_codec_info_t from the argument.
 *  All data are copied, so the returned structure can be used after
 *  closing the module.
 *
 */

lqt_codec_info_t *
lqt_create_codec_info(const lqt_codec_info_static_t * template);

/* Some audio stuff */

int16_t ** lqt_audio_buffer_create_int16(int num_channels, int num_samples);
float   ** lqt_audio_buffer_create_float(int num_channels, int num_samples);

/* An extremely useful function for audio decoders */

/*
 *  It copies the smaller one of src_size and dst_size
 *  from src_pos in the source to dst_pos in the destination
 *  either src_i or src_f can be NULL. Same for dst_i and 
 *  dst_f. But here, you can also set individual channels to NULL.
 */
   
int lqt_copy_audio(int16_t ** dst_i, float ** dst_f,
                   int16_t ** src_i, float ** src_f,
                   int dst_pos, int src_pos,
                   int dst_size, int src_size, int num_channels);

/*
 *  Read one audio chunk
 *  buffer will be realloced if too small and buffer_alloc will be the
 *  new allocated size. Return value is the number of valid bytes,
 *  which might be smaller than buffer_alloc.
 */

int lqt_read_audio_chunk(quicktime_t * file, int track,
                         long chunk,
                         uint8_t ** buffer, int * buffer_alloc, int * samples);
int lqt_append_audio_chunk(quicktime_t * file, int track,
                           long chunk,
                           uint8_t ** buffer, int * buffer_alloc,
                           int initial_bytes);

/*
 *  Read one video frame
 *  buffer will be realloced if too small and buffer_alloc will be the
 *  new allocated size. Return value is the number of valid bytes,
 *  which might be smaller than buffer_alloc.
 */

int lqt_read_video_frame(quicktime_t * file, int track,
                         long frame,
                         uint8_t ** buffer, int * buffer_alloc);
