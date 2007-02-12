#ifndef LIBQUICKTIME_DV_H
#define LIBQUICKTIME_DV_H

#include <quicktime/qtprivate.h>

void quicktime_init_codec_twos(quicktime_audio_map_t *atrack);
void quicktime_init_codec_sowt(quicktime_audio_map_t *atrack);
void quicktime_init_codec_rawaudio(quicktime_audio_map_t *atrack);
void quicktime_init_codec_ulaw(quicktime_audio_map_t *atrack);

#endif
