#ifndef _LQT_H_
#define _LQT_H_

#include "quicktime.h"
#include "lqt_codecinfo.h"

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

#endif
