#ifndef _LQT_H_
#define _LQT_H_

#include "quicktime.h"
#include "lqt_codecinfo.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Call quicktime_set_parameter with our codec info */

void lqt_set_parameter(quicktime_t *file, lqt_parameter_value_t * value,
                       const lqt_parameter_info_t * info);

int lqt_get_codec_api_version();

/***********************************************
 * Advanced colormodel handling.
 * (defined in lqt_color.c)
 ***********************************************/

/*
 *  For initialization of colormodel arrays: This must be something
 *  NOT defined in colormodels.h
 */

#define LQT_COLORMODEL_NONE -1

/* Colormodel <-> string conversion (used by registry file routines) */

const char * lqt_colormodel_to_string(int colormodel);

int lqt_string_to_colormodel(const char * str);

/* Query supported colormodels */

int lqt_num_colormodels();

const char * lqt_get_colormodel_string(int index);

int lqt_get_colormodel(int index);

/*
 *  Return decoder colormodel
 *  This function can fail in some pathological cases,
 *  it returns LQT_COLORMODEL_NONE then
 */

int lqt_get_decoder_colormodel(quicktime_t * file, int track,
                               int * exact);

/*
 *   Versions of quicktime_set_audio and quicktime_set_video,
 *   which take codec infos as arguments
 */

int lqt_set_video(quicktime_t *file, int tracks, 
                  int frame_w, int frame_h,
                  float frame_rate,
                  lqt_codec_info_t * info);

int lqt_set_audio(quicktime_t *file, int channels,
                  long sample_rate,  int bits,
                  lqt_codec_info_t * codec_info);

/*
 *  Same as quicktime_decode_video but doesn't force BC_RGB888
 */

int lqt_decode_video(quicktime_t *file,
                     unsigned char **row_pointers, int track);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

  
#endif
