#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <stdlib.h>
#include <string.h>

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

  fprintf(stderr, "read_mp4_descr_length: %d\n", length);
  
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
    esds->esid = quicktime_read_int16(file);
// stream priority
    esds->stream_priority = quicktime_read_char(file);
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

static int quicktime_write_mp4_descr_length(quicktime_t *file,
                                            int length, int compact)
  {
  uint8_t b;
  int i;
  int numBytes;
  
  if (compact)
    {
    if (length <= 0x7F)
      {
      numBytes = 1;
      }
    else if(length <= 0x3FFF)
      {
      numBytes = 2;
      }
    else if(length <= 0x1FFFFF)
      {
      numBytes = 3;
      }
    else
      {
      numBytes = 4;
      }
    }
  else
    {
    numBytes = 4;
    }

  for (i = numBytes-1; i >= 0; i--)
    {
    b = (length >> (i * 7)) & 0x7F;
    if (i != 0)
      {
      b |= 0x80;
      }
    quicktime_write_char(file, b);
    }
  
  return numBytes; 
  }

void quicktime_write_esds(quicktime_t * file, quicktime_esds_t * esds)
  {
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "esds");

  quicktime_write_char(file, 0);  /* Version */
  quicktime_write_int24(file, 0); /* Flags   */

  quicktime_write_char(file, 0x03);	/* ES_DescrTag */

  quicktime_write_mp4_descr_length(file, 
        3 + (5 + (13 + (5 + esds->decoderConfigLen))) + 3, 0);

  quicktime_write_int16(file, esds->esid);
  quicktime_write_char(file, esds->stream_priority);

  /* DecoderConfigDescriptor */
  quicktime_write_char(file, 0x04);	/* DecoderConfigDescrTag */
  
  quicktime_write_mp4_descr_length(file, 
                  13 + (5 + esds->decoderConfigLen), 0);

  quicktime_write_char(file, esds->objectTypeId); /* objectTypeIndication */
  quicktime_write_char(file, esds->streamType);   /* streamType */

  quicktime_write_int24(file, esds->bufferSizeDB); /* buffer size */
  quicktime_write_int32(file, esds->maxBitrate);   /* max bitrate */
  quicktime_write_int32(file, esds->avgBitrate);   /* average bitrate */

  quicktime_write_char(file, 0x05);	/* DecSpecificInfoTag */
  quicktime_write_mp4_descr_length(file, esds->decoderConfigLen, 0);
  quicktime_write_data(file, esds->decoderConfig, esds->decoderConfigLen);

  /* SLConfigDescriptor */
  quicktime_write_char(file, 0x06);	/* SLConfigDescrTag */
  quicktime_write_char(file, 0x01);	/* length */
  quicktime_write_char(file, 0x02);	/* constant in mp4 files */
  
  quicktime_atom_write_footer(file, &atom);
  }

void quicktime_esds_dump(quicktime_esds_t * esds)
  {
  int i;
  printf("         esds: \n");
  printf("           Version:          %d\n", esds->version);
  printf("           Flags:            0x%06lx\n", esds->flags);
  printf("           ES ID:            0x%04x\n", esds->esid);
  printf("           Priority:         0x%02x\n", esds->stream_priority);
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
    printf("%02x ", esds->decoderConfig[i]);
    }
  printf("\n");
  }

void quicktime_esds_delete(quicktime_esds_t * esds)
  {
  if(esds->decoderConfig) free(esds->decoderConfig);
  }

quicktime_esds_t * quicktime_set_esds(quicktime_trak_t * trak,
                                      uint8_t * decoderConfig,
                                      int decoderConfigLen)
  {
  quicktime_esds_t * esds;
  trak->mdia.minf.stbl.stsd.table[0].has_esds = 1;
  esds = &(trak->mdia.minf.stbl.stsd.table[0].esds);
  esds->decoderConfigLen = decoderConfigLen;
  esds->decoderConfig = malloc(esds->decoderConfigLen);
  memcpy(esds->decoderConfig, decoderConfig, esds->decoderConfigLen);
  return esds;
  }

