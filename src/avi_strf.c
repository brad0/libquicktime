#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

void quicktime_read_strf_audio(quicktime_t *file,
                               quicktime_strf_t *strf,
                               quicktime_atom_t *parent_atom)
  {
  
  strf->wf.type = LQT_WAVEFORMAT_WAVEFORMAT;

  strf->wf.f.WAVEFORMAT.wFormatTag = quicktime_read_int16_le(file);     /* value that identifies compression format */
  strf->wf.f.WAVEFORMAT.nChannels = quicktime_read_int16_le(file);
  strf->wf.f.WAVEFORMAT.nSamplesPerSec = quicktime_read_int32_le(file);
  strf->wf.f.WAVEFORMAT.nAvgBytesPerSec = quicktime_read_int32_le(file);
  strf->wf.f.WAVEFORMAT.nBlockAlign = quicktime_read_int16_le(file);    /* size of a data sample */

  if(parent_atom->size >= 16)
    {
    strf->wf.type = LQT_WAVEFORMAT_PCMWAVEFORMAT;
    strf->wf.f.PCMWAVEFORMAT.wBitsPerSample = quicktime_read_int16_le(file);
    }
  if(parent_atom->size >= 18)
    {
    strf->wf.type = LQT_WAVEFORMAT_WAVEFORMATEX;
    strf->wf.f.WAVEFORMATEX.cbSize = quicktime_read_int16_le(file);

    if(strf->wf.f.WAVEFORMATEX.cbSize)
      {

      if(strf->wf.f.WAVEFORMAT.wFormatTag == 0xfffe && (strf->wf.f.WAVEFORMATEX.cbSize >= 22))
        {
        strf->wf.type = LQT_WAVEFORMAT_WAVEFORMATEXTENSIBLE;
        strf->wf.f.WAVEFORMATEXTENSIBLE.Samples.wValidBitsPerSample = quicktime_read_int16_le(file);;
        strf->wf.f.WAVEFORMATEXTENSIBLE.dwChannelMask = quicktime_read_int32_le(file);;

        quicktime_GUID_read(file, &(strf->wf.f.WAVEFORMATEXTENSIBLE.SubFormat));
        
        if((strf->wf.f.WAVEFORMATEX.cbSize > 22) &&
           (parent_atom->size >= 18 + strf->wf.f.WAVEFORMATEX.cbSize - 22))
          {
          strf->wf.f.WAVEFORMATEX.ext_data = malloc(strf->wf.f.WAVEFORMATEX.cbSize - 22);
          strf->wf.f.WAVEFORMATEX.ext_size = strf->wf.f.WAVEFORMATEX.cbSize - 22;
          quicktime_read_data(file, strf->wf.f.WAVEFORMATEX.ext_data, strf->wf.f.WAVEFORMATEX.cbSize - 22);
          }

        }
      else
        {
        if((parent_atom->size >= 18 + strf->wf.f.WAVEFORMATEX.cbSize))
          {
          strf->wf.f.WAVEFORMATEX.ext_data = malloc(strf->wf.f.WAVEFORMATEX.cbSize);
          strf->wf.f.WAVEFORMATEX.ext_size = strf->wf.f.WAVEFORMATEX.cbSize;
          quicktime_read_data(file, strf->wf.f.WAVEFORMATEX.ext_data, strf->wf.f.WAVEFORMATEX.cbSize);
          }
        }
      }
    }
  }

void quicktime_write_strf_audio(quicktime_t *file,
                                quicktime_strf_t *strf)
  {
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "strf");
  
  quicktime_write_int16_le(file, strf->wf.f.WAVEFORMAT.wFormatTag);     /* value that identifies compression format */
  quicktime_write_int16_le(file, strf->wf.f.WAVEFORMAT.nChannels);
  quicktime_write_int32_le(file, strf->wf.f.WAVEFORMAT.nSamplesPerSec);
  quicktime_write_int32_le(file, strf->wf.f.WAVEFORMAT.nAvgBytesPerSec);
  quicktime_write_int16_le(file, strf->wf.f.WAVEFORMAT.nBlockAlign);    /* size of a data sample */

  switch(strf->wf.type)
    {
    case LQT_WAVEFORMAT_WAVEFORMAT:
      break;
    case LQT_WAVEFORMAT_PCMWAVEFORMAT:
      quicktime_write_int16_le(file, strf->wf.f.PCMWAVEFORMAT.wBitsPerSample);
      break;
    case LQT_WAVEFORMAT_WAVEFORMATEX:
      quicktime_write_int16_le(file, strf->wf.f.PCMWAVEFORMAT.wBitsPerSample);
      quicktime_write_int16_le(file, strf->wf.f.WAVEFORMATEX.cbSize);
      break;
    case LQT_WAVEFORMAT_WAVEFORMATEXTENSIBLE:
      quicktime_write_int16_le(file, strf->wf.f.PCMWAVEFORMAT.wBitsPerSample);
      quicktime_write_int16_le(file, strf->wf.f.WAVEFORMATEX.cbSize);

      quicktime_write_int16_le(file, strf->wf.f.WAVEFORMATEXTENSIBLE.Samples.wValidBitsPerSample);
      quicktime_write_int32_le(file, strf->wf.f.WAVEFORMATEXTENSIBLE.dwChannelMask);;
      quicktime_GUID_write(file, &(strf->wf.f.WAVEFORMATEXTENSIBLE.SubFormat));
      break;
    }

  if(strf->wf.f.WAVEFORMATEX.ext_data)
    quicktime_write_data(file, strf->wf.f.WAVEFORMATEX.ext_data, strf->wf.f.WAVEFORMATEX.ext_size);
  quicktime_atom_write_footer(file, &atom);
  }



void quicktime_strf_dump_audio(quicktime_strf_t *strf)
  {
  switch(strf->wf.type)
    {
    case LQT_WAVEFORMAT_WAVEFORMAT:
      printf("  strf (WAVEFORMAT)");
      break;
    case LQT_WAVEFORMAT_PCMWAVEFORMAT:
      printf("  strf (PCMWAVEFORMAT)");
      break;
    case LQT_WAVEFORMAT_WAVEFORMATEX:
      printf("  strf (WAVEFORMATEX)");
      break;
    case LQT_WAVEFORMAT_WAVEFORMATEXTENSIBLE:
      printf("  strf (WAVEFORMATEXTENSIBLE)");
      break;
    }

  printf("\n    wFormatTag:      %04x\n", strf->wf.f.WAVEFORMAT.wFormatTag);     /* value that identifies compression format */
  printf("    nChannels:       %d\n", strf->wf.f.WAVEFORMAT.nChannels);
  printf("    nSamplesPerSec:  %d\n", strf->wf.f.WAVEFORMAT.nSamplesPerSec);
  printf("    nAvgBytesPerSec: %d\n", strf->wf.f.WAVEFORMAT.nAvgBytesPerSec);
  printf("    nBlockAlign:     %d\n", strf->wf.f.WAVEFORMAT.nBlockAlign);    /* size of a data sample */
  
  switch(strf->wf.type)
    {
    case LQT_WAVEFORMAT_WAVEFORMAT:
      break;
    case LQT_WAVEFORMAT_PCMWAVEFORMAT:
      printf("    wBitsPerSample:  %d\n",  strf->wf.f.PCMWAVEFORMAT.wBitsPerSample);
      break;
    case LQT_WAVEFORMAT_WAVEFORMATEX:
      printf("    wBitsPerSample:  %d\n",  strf->wf.f.PCMWAVEFORMAT.wBitsPerSample);
      printf("    cbSize:          %d\n",  strf->wf.f.WAVEFORMATEX.cbSize);
      break;
    case LQT_WAVEFORMAT_WAVEFORMATEXTENSIBLE:
      printf("    wBitsPerSample:      %d\n", strf->wf.f.PCMWAVEFORMAT.wBitsPerSample);
      printf("    cbSize:              %d\n", strf->wf.f.WAVEFORMATEX.cbSize);
      printf("    wValidBitsPerSample: %d\n", strf->wf.f.WAVEFORMATEXTENSIBLE.Samples.wValidBitsPerSample);
      printf("    dwChannelMask:       %d\n", strf->wf.f.WAVEFORMATEXTENSIBLE.dwChannelMask);
      printf("    SubFormat:           ");
      quicktime_GUID_dump(&(strf->wf.f.WAVEFORMATEXTENSIBLE.SubFormat));
      printf("  \n");
      break;
    }
  if(strf->wf.f.WAVEFORMATEX.ext_data)
    {
    printf("    Extradata: %d bytes (hexdump follows)\n",
           strf->wf.f.WAVEFORMATEX.ext_size);
    lqt_hexdump_stdout(strf->wf.f.WAVEFORMATEX.ext_data,
                       strf->wf.f.WAVEFORMATEX.ext_size, 16);
    }
  }

#if 0
  uint32_t  biSize;  /* sizeof(BITMAPINFOHEADER) */
  uint32_t  biWidth;
  uint32_t  biHeight;
  uint16_t  biPlanes; /* unused */
  uint16_t  biBitCount;
  char biCompression[4]; /* fourcc of image */
  uint32_t  biSizeImage;   /* size of image. For uncompressed images */
                           /* ( biCompression 0 or 3 ) can be zero.  */

  uint32_t  biXPelsPerMeter; /* unused */
  uint32_t  biYPelsPerMeter; /* unused */
  uint32_t  biClrUsed;       /* valid only for palettized images. */
  /* Number of colors in palette. */
  uint32_t  biClrImportant;
*/
#endif
  
void quicktime_read_strf_video(quicktime_t *file,
                               quicktime_strf_t *strf,
                               quicktime_atom_t *parent_atom)
  {
  strf->bh.biSize     = quicktime_read_int32_le(file);  /* sizeof(BITMAPINFOHEADER) */
  strf->bh.biWidth    = quicktime_read_int32_le(file);
  strf->bh.biHeight   = quicktime_read_int32_le(file);
  strf->bh.biPlanes   = quicktime_read_int16_le(file); /* unused */
  strf->bh.biBitCount = quicktime_read_int16_le(file);

  quicktime_read_data(file, (uint8_t*)strf->bh.biCompression, 4);
  
  strf->bh.biSizeImage = quicktime_read_int32_le(file);   /* size of image. For uncompressed images */
                           /* ( biCompression 0 or 3 ) can be zero.  */

  strf->bh.biXPelsPerMeter = quicktime_read_int32_le(file); /* unused */
  strf->bh.biYPelsPerMeter = quicktime_read_int32_le(file); /* unused */
  strf->bh.biClrUsed = quicktime_read_int32_le(file);       /* valid only for palettized images. */
  /* Number of colors in palette. */
  strf->bh.biClrImportant = quicktime_read_int32_le(file);
  
  if(parent_atom->size > 40)
    {
    strf->bh.ext_size = parent_atom->size - 40;
    strf->bh.ext_data = malloc(strf->bh.ext_size);
    quicktime_read_data(file, strf->bh.ext_data, strf->bh.ext_size);
    }
  }

void quicktime_write_strf_video(quicktime_t *file,
                                quicktime_strf_t *strf)
  {
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "strf");
  
  quicktime_write_int32_le(file, strf->bh.biSize);  /* sizeof(BITMAPINFOHEADER) */
  quicktime_write_int32_le(file, strf->bh.biWidth);
  quicktime_write_int32_le(file, strf->bh.biHeight);
  quicktime_write_int16_le(file,  strf->bh.biPlanes); /* unused */
  quicktime_write_int16_le(file, strf->bh.biBitCount);
  
  quicktime_write_data(file, (uint8_t*)strf->bh.biCompression, 4);
  
  quicktime_write_int32_le(file, strf->bh.biSizeImage);   /* size of image. For uncompressed images */
  /* ( biCompression 0 or 3 ) can be zero.  */

  quicktime_write_int32_le(file, strf->bh.biXPelsPerMeter); /* unused */
  quicktime_write_int32_le(file, strf->bh.biYPelsPerMeter); /* unused */
  quicktime_write_int32_le(file, strf->bh.biClrUsed);       /* valid only for palettized images. */
  /* Number of colors in palette. */
  quicktime_write_int32_le(file, strf->bh.biClrImportant);
  
  if(strf->bh.ext_data)
    {
    quicktime_write_data(file, strf->bh.ext_data, strf->bh.ext_size);
    }
  quicktime_atom_write_footer(file, &atom);
  }

void quicktime_strf_dump_video(quicktime_strf_t *strf)
  {
  printf("  strf (BITMAPINFOHEADER)\n");
  printf("    biSize:          %d\n", strf->bh.biSize);  /* sizeof(BITMAPINFOHEADER) */
  printf("    biWidth:         %d\n", strf->bh.biWidth);
  printf("    biHeight:        %d\n", strf->bh.biHeight);
  printf("    biPlanes:        %d\n", strf->bh.biPlanes); /* unused */
  printf("    biBitCount:      %d\n", strf->bh.biBitCount);
  printf("    biCompression:   %4s\n", strf->bh.biCompression); /* fourcc of image */
  printf("    biSizeImage:     %d\n", strf->bh.biSizeImage);   /* size of image. For uncompressed images */
                           /* ( biCompression 0 or 3 ) can be zero.  */

  printf("    biXPelsPerMeter: %d\n", strf->bh.biXPelsPerMeter); /* unused */
  printf("    biYPelsPerMeter: %d\n", strf->bh.biYPelsPerMeter); /* unused */
  printf("    biClrUsed:       %d\n", strf->bh.biClrUsed);       /* valid only for palettized images. */
  /* Number of colors in palette. */
  printf("    biClrImportant:  %d\n",  strf->bh.biClrImportant);

  if(strf->bh.ext_data)
    {
    printf("    Extradata: %d bytes (hexdump follows)\n", strf->bh.ext_size);
    lqt_hexdump_stdout(strf->bh.ext_data, strf->bh.ext_size, 16);
    }
  }
