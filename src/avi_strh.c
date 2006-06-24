#include <funcprotos.h>
#include <quicktime/quicktime.h>

/*
 *
  char fccType[4];
  char fccHandler[4];
  uint32_t dwFlags;
  uint32_t dwReserved1;
  uint32_t dwInitialFrames;
  uint32_t dwScale;
  uint32_t dwRate;
  uint32_t dwStart;
  uint32_t dwLength;
  uint32_t dwSuggestedBufferSize;
  uint32_t dwQuality;
  uint32_t dwSampleSize;
 
 *
 */

void quicktime_read_strh(quicktime_t *file,
                         quicktime_strh_t *strl,
                         quicktime_atom_t *parent_atom)
  {
  quicktime_read_data(file, (uint8_t*)(strl->fccType), 4);
  quicktime_read_data(file, (uint8_t*)(strl->fccHandler), 4);

  strl->dwFlags               = quicktime_read_int32_le(file);
  strl->dwReserved1           = quicktime_read_int32_le(file);
  strl->dwInitialFrames       = quicktime_read_int32_le(file);
  strl->dwScale               = quicktime_read_int32_le(file);
  strl->dwRate                = quicktime_read_int32_le(file);
  strl->dwStart               = quicktime_read_int32_le(file);
  strl->dwLength              = quicktime_read_int32_le(file);
  strl->dwSuggestedBufferSize = quicktime_read_int32_le(file);
  strl->dwQuality             = quicktime_read_int32_le(file);
  strl->dwSampleSize          = quicktime_read_int32_le(file);
  }

void quicktime_write_strh(quicktime_t *file,
                          quicktime_strh_t *strl)
  {
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "strl");
  quicktime_write_data(file, (uint8_t*)(strl->fccType), 4);
  quicktime_write_data(file, (uint8_t*)(strl->fccHandler), 4);
  
  quicktime_write_int32_le(file, strl->dwFlags);
  quicktime_write_int32_le(file, strl->dwReserved1);
  quicktime_write_int32_le(file, strl->dwInitialFrames);
  quicktime_write_int32_le(file, strl->dwScale);
  quicktime_write_int32_le(file, strl->dwRate);
  quicktime_write_int32_le(file, strl->dwStart);
  quicktime_write_int32_le(file, strl->dwLength);
  quicktime_write_int32_le(file, strl->dwSuggestedBufferSize);
  quicktime_write_int32_le(file, strl->dwQuality);
  quicktime_write_int32_le(file, strl->dwSampleSize);
  quicktime_atom_write_footer(file, &atom);
  
  }

void quicktime_strh_dump(quicktime_strh_t *strh)
  {
  printf("  strh\n");

  printf("    fccType:               %.4s\n",  strh->fccType);
  printf("    fccHandler:            %.4s\n",  strh->fccHandler);
  printf("    dwFlags:               %08x\n", strh->dwFlags);
  printf("    dwReserved1:           %08x\n", strh->dwReserved1);
  printf("    dwInitialFrames:       %d\n", strh->dwInitialFrames);
  printf("    dwScale:               %d\n", strh->dwScale);
  printf("    dwRate:                %d\n", strh->dwRate);
  printf("    dwStart:               %d\n", strh->dwStart);
  printf("    dwLength:              %d\n", strh->dwLength);
  printf("    dwSuggestedBufferSize: %d\n", strh->dwSuggestedBufferSize);
  printf("    dwQuality:             %d\n", strh->dwQuality);
  printf("    dwSampleSize:          %d\n", strh->dwSampleSize);
  }
