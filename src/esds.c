#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <stdlib.h>

static int read_mp4_descr_length(quicktime_t * file)
  {
  uint8_t b;
  int num_bytes = 0;
  unsigned int length = 0;

  do{
    if(!quicktime_read_data(file, &b, 1))
      return -1;
    num_bytes++;
    length = (length << 7) | (b & 0x7F);
    } while ((b & 0x80) && (num_bytes < 4));
  return length;
  }

void quicktime_read_esds(quicktime_t * file, quicktime_esds_t * esds)
  {
  uint8_t tag;
  esds->version = quicktime_read_char(file);
  esds->flags = quicktime_read_int24(file);

  quicktime_read_data(file, &tag, 1);

  if(tag == 0x03)
    {
    if(read_mp4_descr_length(file) < 20)
      return;
// elementary stream id
    quicktime_read_int16(file);
// stream priority
    quicktime_read_char(file);
    }
  else
    quicktime_read_int16(file); /* Skip 2 bytes */

  quicktime_read_data(file, &tag, 1);
  
  if(tag != 0x04)
    return;

  if(read_mp4_descr_length(file) < 15)
    return;

  quicktime_read_data(file, &esds->objectTypeId, 1);
  quicktime_read_data(file, &esds->streamType, 1);
  esds->bufferSizeDB = quicktime_read_int24(file);
  esds->maxBitrate = quicktime_read_int32(file);
  esds->avgBitrate = quicktime_read_int32(file);
  
  quicktime_read_data(file, &tag, 1);
  
  if(tag != 0x05)
    return;

  esds->decoderConfigLen = read_mp4_descr_length(file);

  /* Need some padding for ffmpeg */
  esds->decoderConfig = calloc(esds->decoderConfigLen+16, 1);
  quicktime_read_data(file, esds->decoderConfig, esds->decoderConfigLen);
  
  }

void quicktime_write_esds(quicktime_t * file, quicktime_esds_t * esds)
  {

  }

void quicktime_esds_dump(quicktime_esds_t * esds)
  {
  int i;
  printf("         esds: \n");
  printf("           Version:          %d\n", esds->version);
  printf("           Flags:            0x%06lx\n", esds->flags);
  printf("           objectTypeId:     %d\n", esds->objectTypeId);
  printf("           streamType:       0x%02x\n", esds->streamType);
  printf("           bufferSizeDB:     %d\n", esds->bufferSizeDB);

  printf("           maxBitrate:       %d\n", esds->maxBitrate);
  printf("           avgBitrate:       %d\n", esds->avgBitrate);
  printf("           decoderConfigLen: %d\n", esds->decoderConfigLen);
  printf("           decoderConfig:");

  for(i = 0; i < esds->decoderConfigLen; i++)
    {
    if(!(i % 16))
      printf("\n           ");
    printf("%02x", esds->decoderConfig[i]);
    }
  printf("\n");
  }

void quicktime_esds_delete(quicktime_esds_t * esds)
  {
  if(esds->decoderConfig) free(esds->decoderConfig);
  }
