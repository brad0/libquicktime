#include <lqt_funcprotos.h>
#include <quicktime/quicktime.h>

/*
  uint32_t dwMicroSecPerFrame;
  uint32_t dwMaxBytesPerSec;
  uint32_t dwReserved1;
  uint32_t dwFlags;
  uint32_t dwTotalFrames;
  uint32_t dwInitialFrames;
  uint32_t dwStreams;
  uint32_t dwSuggestedBufferSize;
  uint32_t dwWidth;
  uint32_t dwHeight;
  uint32_t dwScale;
  uint32_t dwRate;
  uint32_t dwStart;
  uint32_t dwLength;
*/

void quicktime_read_avih(quicktime_t *file,
                         quicktime_avih_t *avih,
                         quicktime_atom_t *parent_atom)
  {
  avih->dwMicroSecPerFrame    = quicktime_read_int32_le(file);
  avih->dwMaxBytesPerSec      = quicktime_read_int32_le(file);
  avih->dwReserved1           = quicktime_read_int32_le(file);
  avih->dwFlags               = quicktime_read_int32_le(file);
  avih->dwTotalFrames         = quicktime_read_int32_le(file);
  avih->dwInitialFrames       = quicktime_read_int32_le(file);
  avih->dwStreams             = quicktime_read_int32_le(file);
  avih->dwSuggestedBufferSize = quicktime_read_int32_le(file);
  avih->dwWidth               = quicktime_read_int32_le(file);
  avih->dwHeight              = quicktime_read_int32_le(file);
  avih->dwScale               = quicktime_read_int32_le(file);
  avih->dwRate                = quicktime_read_int32_le(file);
  avih->dwStart               = quicktime_read_int32_le(file);
  avih->dwLength              = quicktime_read_int32_le(file);
  }

void quicktime_write_avih(quicktime_t *file,
                          quicktime_avih_t *avih)
  {
  quicktime_write_int32_le(file, avih->dwMicroSecPerFrame);
  quicktime_write_int32_le(file, avih->dwMaxBytesPerSec);
  quicktime_write_int32_le(file, avih->dwReserved1);
  quicktime_write_int32_le(file, avih->dwFlags);
  quicktime_write_int32_le(file, avih->dwTotalFrames);
  quicktime_write_int32_le(file, avih->dwInitialFrames);
  quicktime_write_int32_le(file, avih->dwStreams);
  quicktime_write_int32_le(file, avih->dwSuggestedBufferSize);
  quicktime_write_int32_le(file, avih->dwWidth);
  quicktime_write_int32_le(file, avih->dwHeight);
  quicktime_write_int32_le(file, avih->dwScale);
  quicktime_write_int32_le(file, avih->dwRate);
  quicktime_write_int32_le(file, avih->dwStart);
  quicktime_write_int32_le(file, avih->dwLength);
  }

void quicktime_avih_dump(quicktime_avih_t *avih)
  {
  printf("dwMicroSecPerFrame: %d\n",    avih->dwMicroSecPerFrame);
  printf("dwMaxBytesPerSec: %d\n",      avih->dwMaxBytesPerSec);
  printf("dwReserved1: %d\n",           avih->dwReserved1);
  printf("dwFlags: %d\n",               avih->dwFlags);
  printf("dwTotalFrames: %d\n",         avih->dwTotalFrames);
  printf("dwInitialFrames: %d\n",       avih->dwInitialFrames);
  printf("dwStreams: %d\n",             avih->dwStreams);
  printf("dwSuggestedBufferSize: %d\n", avih->dwSuggestedBufferSize);
  printf("dwWidth: %d\n",               avih->dwWidth);
  printf("dwHeight: %d\n",              avih->dwHeight);
  printf("dwScale: %d\n",               avih->dwScale);
  printf("dwRate: %d\n",                avih->dwRate);
  printf("dwStart: %d\n",               avih->dwStart);
  printf("dwLength: %d\n",              avih->dwLength);
  }

