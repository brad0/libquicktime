#ifndef FUNCPROTOS_H
#define FUNCPROTOS_H

#include <graphics.h>
#include <quicktime/qtprivate.h>
#include <quicktime/lqt_codecinfo.h>

#include <lqt_funcprotos.h>
#include <cmodel_permutation.h>

#include <cmodel_default.h>
#include <cmodel_yuv420p.h>
#include <cmodel_yuv422.h>
#include <util.h>


/* Most codecs don't specify the actual number of bits on disk in the stbl. */
/* Convert the samples to the number of bytes for reading depending on the codec. */
int64_t quicktime_samples_to_bytes(quicktime_trak_t *track, long samples);

#endif
