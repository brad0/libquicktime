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

/*
 *   This holds the parts of the codec_info structure,
 *   which can be defined statically in the codec source
 */

typedef struct
  {
  char * name;               /* Name of the codec              */
  char * long_name;          /* Long name of the codec         */
  char * description;        /* Description                    */

  lqt_codec_type type;
  lqt_codec_direction direction;
  } lqt_codec_info_static_t;


/*
 *  Create a lqt_codec_info_t structure from statically defined data
 *
 *  Typically, you will define the lqt_codec_info_static_t as well
 *  as arrays for the (en/de)coding parameters for each codec in the
 *  module.
 *
 *  The get_codec_info() function in your module will then call the
 *  function below to create a lqt_codec_info_t from the arguments.
 *  All data are copied, so the returned structure can be used after
 *  closing the module.
 *
 *  If the numbers of encoding or decoding parameters are zero
 *  the corresponding pointers can be NULL.
 */

lqt_codec_info_t *
lqt_create_codec_info(const lqt_codec_info_static_t * template,
                      char * fourccs[], int num_fourccs,
                      const lqt_codec_parameter_info_t * encoding_parameters,
                      int num_encoding_parameters,
                      const lqt_codec_parameter_info_t * decoding_parameters,
                      int num_decoding_parameters);


