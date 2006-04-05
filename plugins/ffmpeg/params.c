#include <ffmpeg/avcodec.h>
#include <quicktime/lqt.h>
#include "params.h"

#define PARAM_INT(name, key, var, val) \
  if(!strcasecmp(name, key)) \
    { \
    var = *(int*)val; \
    found = 1; \
    }

void lqt_ffmpeg_set_parameter(AVCodecContext * ctx, char * key, void * value)
  {
  int found = 0;
  
  }
