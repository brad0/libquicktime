#ifndef _LQT_H_
#define _LQT_H_

#include "quicktime.h"
#include "lqt_codecinfo.h"

/* Call quicktime_set_parameter with our codec info */

void lqt_set_parameter(quicktime_t *file, lqt_parameter_value_t * value,
                       const lqt_parameter_info_t * info);

int lqt_get_codec_api_version();

#endif
