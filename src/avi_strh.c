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
                         quicktime_strh_t *strh,
                         quicktime_atom_t *parent_atom)
  {
  quicktime_read_data(file, (uint8_t*)(strh->fccType), 4);
  quicktime_read_data(file, (uint8_t*)(strh->fccHandler), 4);

  strh->dwFlags               = quicktime_read_int32_le(file);
  strh->dwReserved1           = quicktime_read_int32_le(file);
  strh->dwInitialFrames       = quicktime_read_int32_le(file);
  strh->dwScale               = quicktime_read_int32_le(file);
  strh->dwRate                = quicktime_read_int32_le(file);
  strh->dwStart               = quicktime_read_int32_le(file);
  strh->dwLength              = quicktime_read_int32_le(file);
  strh->dwSuggestedBufferSize = quicktime_read_int32_le(file);
  strh->dwQuality             = quicktime_read_int32_le(file);
  strh->dwSampleSize          = quicktime_read_int32_le(file);
  }

void quicktime_write_strh(quicktime_t *file,
                          quicktime_strh_t *strh)
  {
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "strh");
  quicktime_write_data(file, (uint8_t*)(strh->fccType), 4);
  quicktime_write_data(file, (uint8_t*)(strh->fccHandler), 4);
  
  quicktime_write_int32_le(file, strh->dwFlags);
  quicktime_write_int32_le(file, strh->dwReserved1);
  quicktime_write_int32_le(file, strh->dwInitialFrames);
  quicktime_write_int32_le(file, strh->dwScale);
  quicktime_write_int32_le(file, strh->dwRate);
  quicktime_write_int32_le(file, strh->dwStart);
  quicktime_write_int32_le(file, strh->dwLength);
  quicktime_write_int32_le(file, strh->dwSuggestedBufferSize);
  quicktime_write_int32_le(file, strh->dwQuality);
  quicktime_write_int32_le(file, strh->dwSampleSize);
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
