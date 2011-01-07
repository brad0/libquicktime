/*******************************************************************************
 ctab.c

 libquicktime - A library for reading and writing quicktime/avi/mp4 files.
 http://libquicktime.sourceforge.net

 Copyright (C) 2002 Heroine Virtual Ltd.
 Copyright (C) 2002-2011 Members of the libquicktime project.

 This library is free software; you can redistribute it and/or modify it under
 the terms of the GNU Lesser General Public License as published by the Free
 Software Foundation; either version 2.1 of the License, or (at your option)
 any later version.

 This library is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 details.

 You should have received a copy of the GNU Lesser General Public License along
 with this library; if not, write to the Free Software Foundation, Inc., 51
 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*******************************************************************************/ 

#include "lqt_private.h"
#include <stdlib.h>

int quicktime_ctab_init(quicktime_ctab_t *ctab)
{
	ctab->seed = 0;
	ctab->flags = 0;
	ctab->size = 0;
	ctab->alpha = 0;
	ctab->red = 0;
	ctab->green = 0;
	ctab->blue = 0;
	return 0;
}

int quicktime_ctab_delete(quicktime_ctab_t *ctab)
{
	if(ctab->alpha) free(ctab->alpha);
	if(ctab->red) free(ctab->red);
	if(ctab->green) free(ctab->green);
	if(ctab->blue) free(ctab->blue);
	return 0;
}

void quicktime_ctab_dump(quicktime_ctab_t *ctab)
{
	int i;
	lqt_dump(" color table (ctab)\n");
	lqt_dump("  seed %ld\n", ctab->seed);
	lqt_dump("  flags %ld\n", ctab->flags);
	lqt_dump("  size %ld\n", ctab->size);
	lqt_dump("  colors ");
	for(i = 0; i < ctab->size; i++)
	{
		lqt_dump("[0x%02x 0x%02x 0x%02x 0x%02x]\n",
                       (ctab->red[i])>>8, (ctab->green[i])>>8,
                       (ctab->blue[i])>>8, (ctab->alpha[i])>>8);
	}
	lqt_dump("\n");
}

int quicktime_read_ctab(quicktime_t *file, quicktime_ctab_t *ctab)
{
	int i;
	
	ctab->seed = quicktime_read_int32(file);
	ctab->flags = quicktime_read_int16(file);
	ctab->size = quicktime_read_int16(file) + 1;
	ctab->alpha = malloc(sizeof(int16_t) * ctab->size);
	ctab->red = malloc(sizeof(int16_t) * ctab->size);
	ctab->green = malloc(sizeof(int16_t) * ctab->size);
	ctab->blue = malloc(sizeof(int16_t) * ctab->size);
	
	for(i = 0; i < ctab->size; i++)
	{
		ctab->alpha[i] = quicktime_read_int16(file);
		ctab->red[i] = quicktime_read_int16(file);
		ctab->green[i] = quicktime_read_int16(file);
		ctab->blue[i] = quicktime_read_int16(file);
	}
        //        quicktime_ctab_dump(ctab);
	return 0;
}

/* Added for libquicktime: Default color tables */

typedef struct
  {
  uint16_t r;
  uint16_t g;
  uint16_t b;
  uint16_t a;
  } palette_entry;

static palette_entry qt_default_palette_2[] = {
 { 0xFFFF, 0xFFFF, 0xFFFF, 0x0000 },
 { 0x0000, 0x0000, 0x0000, 0x0000 },
};

static palette_entry qt_default_palette_4[] = {
 { 0x9393, 0x6565, 0x5E5E, 0x0000 },
 { 0xFFFF, 0xFFFF, 0xFFFF, 0x0000 },
 { 0xDFDF, 0xD0D0, 0xABAB, 0x0000 },
 { 0x0000, 0x0000, 0x0000, 0x0000 }
};

static palette_entry qt_default_palette_16[] = {
 { 0xFFFF, 0xFBFB, 0xFFFF, 0x0000 },
 { 0xEFEF, 0xD9D9, 0xBBBB, 0x0000 },
 { 0xE8E8, 0xC9C9, 0xB1B1, 0x0000 },
 { 0x9393, 0x6565, 0x5E5E, 0x0000 },
 { 0xFCFC, 0xDEDE, 0xE8E8, 0x0000 },
 { 0x9D9D, 0x8888, 0x9191, 0x0000 },
 { 0xFFFF, 0xFFFF, 0xFFFF, 0x0000 },
 { 0xFFFF, 0xFFFF, 0xFFFF, 0x0000 },
 { 0xFFFF, 0xFFFF, 0xFFFF, 0x0000 },
 { 0x4747, 0x4848, 0x3737, 0x0000 },
 { 0x7A7A, 0x5E5E, 0x5555, 0x0000 },
 { 0xDFDF, 0xD0D0, 0xABAB, 0x0000 },
 { 0xFFFF, 0xFBFB, 0xF9F9, 0x0000 },
 { 0xE8E8, 0xCACA, 0xC5C5, 0x0000 },
 { 0x8A8A, 0x7C7C, 0x7777, 0x0000 },
 { 0x0000, 0x0000, 0x0000, 0x0000 }
};

static palette_entry qt_default_palette_256[256] = {
  /*   0, 0x00 */ { 0xFFFF, 0xFFFF, 0xFFFF, 0x0000 },
  /*   1, 0x01 */ { 0xFFFF, 0xFFFF, 0xCCCC, 0x0000 },
  /*   2, 0x02 */ { 0xFFFF, 0xFFFF, 0x9999, 0x0000 },
  /*   3, 0x03 */ { 0xFFFF, 0xFFFF, 0x6666, 0x0000 },
  /*   4, 0x04 */ { 0xFFFF, 0xFFFF, 0x3333, 0x0000 },
  /*   5, 0x05 */ { 0xFFFF, 0xFFFF, 0x0000, 0x0000 },
  /*   6, 0x06 */ { 0xFFFF, 0xCCCC, 0xFFFF, 0x0000 },
  /*   7, 0x07 */ { 0xFFFF, 0xCCCC, 0xCCCC, 0x0000 },
  /*   8, 0x08 */ { 0xFFFF, 0xCCCC, 0x9999, 0x0000 },
  /*   9, 0x09 */ { 0xFFFF, 0xCCCC, 0x6666, 0x0000 },
  /*  10, 0x0A */ { 0xFFFF, 0xCCCC, 0x3333, 0x0000 },
  /*  11, 0x0B */ { 0xFFFF, 0xCCCC, 0x0000, 0x0000 },
  /*  12, 0x0C */ { 0xFFFF, 0x9999, 0xFFFF, 0x0000 },
  /*  13, 0x0D */ { 0xFFFF, 0x9999, 0xCCCC, 0x0000 },
  /*  14, 0x0E */ { 0xFFFF, 0x9999, 0x9999, 0x0000 },
  /*  15, 0x0F */ { 0xFFFF, 0x9999, 0x6666, 0x0000 },
  /*  16, 0x10 */ { 0xFFFF, 0x9999, 0x3333, 0x0000 },
  /*  17, 0x11 */ { 0xFFFF, 0x9999, 0x0000, 0x0000 },
  /*  18, 0x12 */ { 0xFFFF, 0x6666, 0xFFFF, 0x0000 },
  /*  19, 0x13 */ { 0xFFFF, 0x6666, 0xCCCC, 0x0000 },
  /*  20, 0x14 */ { 0xFFFF, 0x6666, 0x9999, 0x0000 },
  /*  21, 0x15 */ { 0xFFFF, 0x6666, 0x6666, 0x0000 },
  /*  22, 0x16 */ { 0xFFFF, 0x6666, 0x3333, 0x0000 },
  /*  23, 0x17 */ { 0xFFFF, 0x6666, 0x0000, 0x0000 },
  /*  24, 0x18 */ { 0xFFFF, 0x3333, 0xFFFF, 0x0000 },
  /*  25, 0x19 */ { 0xFFFF, 0x3333, 0xCCCC, 0x0000 },
  /*  26, 0x1A */ { 0xFFFF, 0x3333, 0x9999, 0x0000 },
  /*  27, 0x1B */ { 0xFFFF, 0x3333, 0x6666, 0x0000 },
  /*  28, 0x1C */ { 0xFFFF, 0x3333, 0x3333, 0x0000 },
  /*  29, 0x1D */ { 0xFFFF, 0x3333, 0x0000, 0x0000 },
  /*  30, 0x1E */ { 0xFFFF, 0x0000, 0xFFFF, 0x0000 },
  /*  31, 0x1F */ { 0xFFFF, 0x0000, 0xCCCC, 0x0000 },
  /*  32, 0x20 */ { 0xFFFF, 0x0000, 0x9999, 0x0000 },
  /*  33, 0x21 */ { 0xFFFF, 0x0000, 0x6666, 0x0000 },
  /*  34, 0x22 */ { 0xFFFF, 0x0000, 0x3333, 0x0000 },
  /*  35, 0x23 */ { 0xFFFF, 0x0000, 0x0000, 0x0000 },
  /*  36, 0x24 */ { 0xCCCC, 0xFFFF, 0xFFFF, 0x0000 },
  /*  37, 0x25 */ { 0xCCCC, 0xFFFF, 0xCCCC, 0x0000 },
  /*  38, 0x26 */ { 0xCCCC, 0xFFFF, 0x9999, 0x0000 },
  /*  39, 0x27 */ { 0xCCCC, 0xFFFF, 0x6666, 0x0000 },
  /*  40, 0x28 */ { 0xCCCC, 0xFFFF, 0x3333, 0x0000 },
  /*  41, 0x29 */ { 0xCCCC, 0xFFFF, 0x0000, 0x0000 },
  /*  42, 0x2A */ { 0xCCCC, 0xCCCC, 0xFFFF, 0x0000 },
  /*  43, 0x2B */ { 0xCCCC, 0xCCCC, 0xCCCC, 0x0000 },
  /*  44, 0x2C */ { 0xCCCC, 0xCCCC, 0x9999, 0x0000 },
  /*  45, 0x2D */ { 0xCCCC, 0xCCCC, 0x6666, 0x0000 },
  /*  46, 0x2E */ { 0xCCCC, 0xCCCC, 0x3333, 0x0000 },
  /*  47, 0x2F */ { 0xCCCC, 0xCCCC, 0x0000, 0x0000 },
  /*  48, 0x30 */ { 0xCCCC, 0x9999, 0xFFFF, 0x0000 },
  /*  49, 0x31 */ { 0xCCCC, 0x9999, 0xCCCC, 0x0000 },
  /*  50, 0x32 */ { 0xCCCC, 0x9999, 0x9999, 0x0000 },
  /*  51, 0x33 */ { 0xCCCC, 0x9999, 0x6666, 0x0000 },
  /*  52, 0x34 */ { 0xCCCC, 0x9999, 0x3333, 0x0000 },
  /*  53, 0x35 */ { 0xCCCC, 0x9999, 0x0000, 0x0000 },
  /*  54, 0x36 */ { 0xCCCC, 0x6666, 0xFFFF, 0x0000 },
  /*  55, 0x37 */ { 0xCCCC, 0x6666, 0xCCCC, 0x0000 },
  /*  56, 0x38 */ { 0xCCCC, 0x6666, 0x9999, 0x0000 },
  /*  57, 0x39 */ { 0xCCCC, 0x6666, 0x6666, 0x0000 },
  /*  58, 0x3A */ { 0xCCCC, 0x6666, 0x3333, 0x0000 },
  /*  59, 0x3B */ { 0xCCCC, 0x6666, 0x0000, 0x0000 },
  /*  60, 0x3C */ { 0xCCCC, 0x3333, 0xFFFF, 0x0000 },
  /*  61, 0x3D */ { 0xCCCC, 0x3333, 0xCCCC, 0x0000 },
  /*  62, 0x3E */ { 0xCCCC, 0x3333, 0x9999, 0x0000 },
  /*  63, 0x3F */ { 0xCCCC, 0x3333, 0x6666, 0x0000 },
  /*  64, 0x40 */ { 0xCCCC, 0x3333, 0x3333, 0x0000 },
  /*  65, 0x41 */ { 0xCCCC, 0x3333, 0x0000, 0x0000 },
  /*  66, 0x42 */ { 0xCCCC, 0x0000, 0xFFFF, 0x0000 },
  /*  67, 0x43 */ { 0xCCCC, 0x0000, 0xCCCC, 0x0000 },
  /*  68, 0x44 */ { 0xCCCC, 0x0000, 0x9999, 0x0000 },
  /*  69, 0x45 */ { 0xCCCC, 0x0000, 0x6666, 0x0000 },
  /*  70, 0x46 */ { 0xCCCC, 0x0000, 0x3333, 0x0000 },
  /*  71, 0x47 */ { 0xCCCC, 0x0000, 0x0000, 0x0000 },
  /*  72, 0x48 */ { 0x9999, 0xFFFF, 0xFFFF, 0x0000 },
  /*  73, 0x49 */ { 0x9999, 0xFFFF, 0xCCCC, 0x0000 },
  /*  74, 0x4A */ { 0x9999, 0xFFFF, 0x9999, 0x0000 },
  /*  75, 0x4B */ { 0x9999, 0xFFFF, 0x6666, 0x0000 },
  /*  76, 0x4C */ { 0x9999, 0xFFFF, 0x3333, 0x0000 },
  /*  77, 0x4D */ { 0x9999, 0xFFFF, 0x0000, 0x0000 },
  /*  78, 0x4E */ { 0x9999, 0xCCCC, 0xFFFF, 0x0000 },
  /*  79, 0x4F */ { 0x9999, 0xCCCC, 0xCCCC, 0x0000 },
  /*  80, 0x50 */ { 0x9999, 0xCCCC, 0x9999, 0x0000 },
  /*  81, 0x51 */ { 0x9999, 0xCCCC, 0x6666, 0x0000 },
  /*  82, 0x52 */ { 0x9999, 0xCCCC, 0x3333, 0x0000 },
  /*  83, 0x53 */ { 0x9999, 0xCCCC, 0x0000, 0x0000 },
  /*  84, 0x54 */ { 0x9999, 0x9999, 0xFFFF, 0x0000 },
  /*  85, 0x55 */ { 0x9999, 0x9999, 0xCCCC, 0x0000 },
  /*  86, 0x56 */ { 0x9999, 0x9999, 0x9999, 0x0000 },
  /*  87, 0x57 */ { 0x9999, 0x9999, 0x6666, 0x0000 },
  /*  88, 0x58 */ { 0x9999, 0x9999, 0x3333, 0x0000 },
  /*  89, 0x59 */ { 0x9999, 0x9999, 0x0000, 0x0000 },
  /*  90, 0x5A */ { 0x9999, 0x6666, 0xFFFF, 0x0000 },
  /*  91, 0x5B */ { 0x9999, 0x6666, 0xCCCC, 0x0000 },
  /*  92, 0x5C */ { 0x9999, 0x6666, 0x9999, 0x0000 },
  /*  93, 0x5D */ { 0x9999, 0x6666, 0x6666, 0x0000 },
  /*  94, 0x5E */ { 0x9999, 0x6666, 0x3333, 0x0000 },
  /*  95, 0x5F */ { 0x9999, 0x6666, 0x0000, 0x0000 },
  /*  96, 0x60 */ { 0x9999, 0x3333, 0xFFFF, 0x0000 },
  /*  97, 0x61 */ { 0x9999, 0x3333, 0xCCCC, 0x0000 },
  /*  98, 0x62 */ { 0x9999, 0x3333, 0x9999, 0x0000 },
  /*  99, 0x63 */ { 0x9999, 0x3333, 0x6666, 0x0000 },
  /* 100, 0x64 */ { 0x9999, 0x3333, 0x3333, 0x0000 },
  /* 101, 0x65 */ { 0x9999, 0x3333, 0x0000, 0x0000 },
  /* 102, 0x66 */ { 0x9999, 0x0000, 0xFFFF, 0x0000 },
  /* 103, 0x67 */ { 0x9999, 0x0000, 0xCCCC, 0x0000 },
  /* 104, 0x68 */ { 0x9999, 0x0000, 0x9999, 0x0000 },
  /* 105, 0x69 */ { 0x9999, 0x0000, 0x6666, 0x0000 },
  /* 106, 0x6A */ { 0x9999, 0x0000, 0x3333, 0x0000 },
  /* 107, 0x6B */ { 0x9999, 0x0000, 0x0000, 0x0000 },
  /* 108, 0x6C */ { 0x6666, 0xFFFF, 0xFFFF, 0x0000 },
  /* 109, 0x6D */ { 0x6666, 0xFFFF, 0xCCCC, 0x0000 },
  /* 110, 0x6E */ { 0x6666, 0xFFFF, 0x9999, 0x0000 },
  /* 111, 0x6F */ { 0x6666, 0xFFFF, 0x6666, 0x0000 },
  /* 112, 0x70 */ { 0x6666, 0xFFFF, 0x3333, 0x0000 },
  /* 113, 0x71 */ { 0x6666, 0xFFFF, 0x0000, 0x0000 },
  /* 114, 0x72 */ { 0x6666, 0xCCCC, 0xFFFF, 0x0000 },
  /* 115, 0x73 */ { 0x6666, 0xCCCC, 0xCCCC, 0x0000 },
  /* 116, 0x74 */ { 0x6666, 0xCCCC, 0x9999, 0x0000 },
  /* 117, 0x75 */ { 0x6666, 0xCCCC, 0x6666, 0x0000 },
  /* 118, 0x76 */ { 0x6666, 0xCCCC, 0x3333, 0x0000 },
  /* 119, 0x77 */ { 0x6666, 0xCCCC, 0x0000, 0x0000 },
  /* 120, 0x78 */ { 0x6666, 0x9999, 0xFFFF, 0x0000 },
  /* 121, 0x79 */ { 0x6666, 0x9999, 0xCCCC, 0x0000 },
  /* 122, 0x7A */ { 0x6666, 0x9999, 0x9999, 0x0000 },
  /* 123, 0x7B */ { 0x6666, 0x9999, 0x6666, 0x0000 },
  /* 124, 0x7C */ { 0x6666, 0x9999, 0x3333, 0x0000 },
  /* 125, 0x7D */ { 0x6666, 0x9999, 0x0000, 0x0000 },
  /* 126, 0x7E */ { 0x6666, 0x6666, 0xFFFF, 0x0000 },
  /* 127, 0x7F */ { 0x6666, 0x6666, 0xCCCC, 0x0000 },
  /* 128, 0x80 */ { 0x6666, 0x6666, 0x9999, 0x0000 },
  /* 129, 0x81 */ { 0x6666, 0x6666, 0x6666, 0x0000 },
  /* 130, 0x82 */ { 0x6666, 0x6666, 0x3333, 0x0000 },
  /* 131, 0x83 */ { 0x6666, 0x6666, 0x0000, 0x0000 },
  /* 132, 0x84 */ { 0x6666, 0x3333, 0xFFFF, 0x0000 },
  /* 133, 0x85 */ { 0x6666, 0x3333, 0xCCCC, 0x0000 },
  /* 134, 0x86 */ { 0x6666, 0x3333, 0x9999, 0x0000 },
  /* 135, 0x87 */ { 0x6666, 0x3333, 0x6666, 0x0000 },
  /* 136, 0x88 */ { 0x6666, 0x3333, 0x3333, 0x0000 },
  /* 137, 0x89 */ { 0x6666, 0x3333, 0x0000, 0x0000 },
  /* 138, 0x8A */ { 0x6666, 0x0000, 0xFFFF, 0x0000 },
  /* 139, 0x8B */ { 0x6666, 0x0000, 0xCCCC, 0x0000 },
  /* 140, 0x8C */ { 0x6666, 0x0000, 0x9999, 0x0000 },
  /* 141, 0x8D */ { 0x6666, 0x0000, 0x6666, 0x0000 },
  /* 142, 0x8E */ { 0x6666, 0x0000, 0x3333, 0x0000 },
  /* 143, 0x8F */ { 0x6666, 0x0000, 0x0000, 0x0000 },
  /* 144, 0x90 */ { 0x3333, 0xFFFF, 0xFFFF, 0x0000 },
  /* 145, 0x91 */ { 0x3333, 0xFFFF, 0xCCCC, 0x0000 },
  /* 146, 0x92 */ { 0x3333, 0xFFFF, 0x9999, 0x0000 },
  /* 147, 0x93 */ { 0x3333, 0xFFFF, 0x6666, 0x0000 },
  /* 148, 0x94 */ { 0x3333, 0xFFFF, 0x3333, 0x0000 },
  /* 149, 0x95 */ { 0x3333, 0xFFFF, 0x0000, 0x0000 },
  /* 150, 0x96 */ { 0x3333, 0xCCCC, 0xFFFF, 0x0000 },
  /* 151, 0x97 */ { 0x3333, 0xCCCC, 0xCCCC, 0x0000 },
  /* 152, 0x98 */ { 0x3333, 0xCCCC, 0x9999, 0x0000 },
  /* 153, 0x99 */ { 0x3333, 0xCCCC, 0x6666, 0x0000 },
  /* 154, 0x9A */ { 0x3333, 0xCCCC, 0x3333, 0x0000 },
  /* 155, 0x9B */ { 0x3333, 0xCCCC, 0x0000, 0x0000 },
  /* 156, 0x9C */ { 0x3333, 0x9999, 0xFFFF, 0x0000 },
  /* 157, 0x9D */ { 0x3333, 0x9999, 0xCCCC, 0x0000 },
  /* 158, 0x9E */ { 0x3333, 0x9999, 0x9999, 0x0000 },
  /* 159, 0x9F */ { 0x3333, 0x9999, 0x6666, 0x0000 },
  /* 160, 0xA0 */ { 0x3333, 0x9999, 0x3333, 0x0000 },
  /* 161, 0xA1 */ { 0x3333, 0x9999, 0x0000, 0x0000 },
  /* 162, 0xA2 */ { 0x3333, 0x6666, 0xFFFF, 0x0000 },
  /* 163, 0xA3 */ { 0x3333, 0x6666, 0xCCCC, 0x0000 },
  /* 164, 0xA4 */ { 0x3333, 0x6666, 0x9999, 0x0000 },
  /* 165, 0xA5 */ { 0x3333, 0x6666, 0x6666, 0x0000 },
  /* 166, 0xA6 */ { 0x3333, 0x6666, 0x3333, 0x0000 },
  /* 167, 0xA7 */ { 0x3333, 0x6666, 0x0000, 0x0000 },
  /* 168, 0xA8 */ { 0x3333, 0x3333, 0xFFFF, 0x0000 },
  /* 169, 0xA9 */ { 0x3333, 0x3333, 0xCCCC, 0x0000 },
  /* 170, 0xAA */ { 0x3333, 0x3333, 0x9999, 0x0000 },
  /* 171, 0xAB */ { 0x3333, 0x3333, 0x6666, 0x0000 },
  /* 172, 0xAC */ { 0x3333, 0x3333, 0x3333, 0x0000 },
  /* 173, 0xAD */ { 0x3333, 0x3333, 0x0000, 0x0000 },
  /* 174, 0xAE */ { 0x3333, 0x0000, 0xFFFF, 0x0000 },
  /* 175, 0xAF */ { 0x3333, 0x0000, 0xCCCC, 0x0000 },
  /* 176, 0xB0 */ { 0x3333, 0x0000, 0x9999, 0x0000 },
  /* 177, 0xB1 */ { 0x3333, 0x0000, 0x6666, 0x0000 },
  /* 178, 0xB2 */ { 0x3333, 0x0000, 0x3333, 0x0000 },
  /* 179, 0xB3 */ { 0x3333, 0x0000, 0x0000, 0x0000 },
  /* 180, 0xB4 */ { 0x0000, 0xFFFF, 0xFFFF, 0x0000 },
  /* 181, 0xB5 */ { 0x0000, 0xFFFF, 0xCCCC, 0x0000 },
  /* 182, 0xB6 */ { 0x0000, 0xFFFF, 0x9999, 0x0000 },
  /* 183, 0xB7 */ { 0x0000, 0xFFFF, 0x6666, 0x0000 },
  /* 184, 0xB8 */ { 0x0000, 0xFFFF, 0x3333, 0x0000 },
  /* 185, 0xB9 */ { 0x0000, 0xFFFF, 0x0000, 0x0000 },
  /* 186, 0xBA */ { 0x0000, 0xCCCC, 0xFFFF, 0x0000 },
  /* 187, 0xBB */ { 0x0000, 0xCCCC, 0xCCCC, 0x0000 },
  /* 188, 0xBC */ { 0x0000, 0xCCCC, 0x9999, 0x0000 },
  /* 189, 0xBD */ { 0x0000, 0xCCCC, 0x6666, 0x0000 },
  /* 190, 0xBE */ { 0x0000, 0xCCCC, 0x3333, 0x0000 },
  /* 191, 0xBF */ { 0x0000, 0xCCCC, 0x0000, 0x0000 },
  /* 192, 0xC0 */ { 0x0000, 0x9999, 0xFFFF, 0x0000 },
  /* 193, 0xC1 */ { 0x0000, 0x9999, 0xCCCC, 0x0000 },
  /* 194, 0xC2 */ { 0x0000, 0x9999, 0x9999, 0x0000 },
  /* 195, 0xC3 */ { 0x0000, 0x9999, 0x6666, 0x0000 },
  /* 196, 0xC4 */ { 0x0000, 0x9999, 0x3333, 0x0000 },
  /* 197, 0xC5 */ { 0x0000, 0x9999, 0x0000, 0x0000 },
  /* 198, 0xC6 */ { 0x0000, 0x6666, 0xFFFF, 0x0000 },
  /* 199, 0xC7 */ { 0x0000, 0x6666, 0xCCCC, 0x0000 },
  /* 200, 0xC8 */ { 0x0000, 0x6666, 0x9999, 0x0000 },
  /* 201, 0xC9 */ { 0x0000, 0x6666, 0x6666, 0x0000 },
  /* 202, 0xCA */ { 0x0000, 0x6666, 0x3333, 0x0000 },
  /* 203, 0xCB */ { 0x0000, 0x6666, 0x0000, 0x0000 },
  /* 204, 0xCC */ { 0x0000, 0x3333, 0xFFFF, 0x0000 },
  /* 205, 0xCD */ { 0x0000, 0x3333, 0xCCCC, 0x0000 },
  /* 206, 0xCE */ { 0x0000, 0x3333, 0x9999, 0x0000 },
  /* 207, 0xCF */ { 0x0000, 0x3333, 0x6666, 0x0000 },
  /* 208, 0xD0 */ { 0x0000, 0x3333, 0x3333, 0x0000 },
  /* 209, 0xD1 */ { 0x0000, 0x3333, 0x0000, 0x0000 },
  /* 210, 0xD2 */ { 0x0000, 0x0000, 0xFFFF, 0x0000 },
  /* 211, 0xD3 */ { 0x0000, 0x0000, 0xCCCC, 0x0000 },
  /* 212, 0xD4 */ { 0x0000, 0x0000, 0x9999, 0x0000 },
  /* 213, 0xD5 */ { 0x0000, 0x0000, 0x6666, 0x0000 },
  /* 214, 0xD6 */ { 0x0000, 0x0000, 0x3333, 0x0000 },
  /* 215, 0xD7 */ { 0xEEEE, 0x0000, 0x0000, 0x0000 },
  /* 216, 0xD8 */ { 0xDDDD, 0x0000, 0x0000, 0x0000 },
  /* 217, 0xD9 */ { 0xBBBB, 0x0000, 0x0000, 0x0000 },
  /* 218, 0xDA */ { 0xAAAA, 0x0000, 0x0000, 0x0000 },
  /* 219, 0xDB */ { 0x8888, 0x0000, 0x0000, 0x0000 },
  /* 220, 0xDC */ { 0x7777, 0x0000, 0x0000, 0x0000 },
  /* 221, 0xDD */ { 0x5555, 0x0000, 0x0000, 0x0000 },
  /* 222, 0xDE */ { 0x4444, 0x0000, 0x0000, 0x0000 },
  /* 223, 0xDF */ { 0x2222, 0x0000, 0x0000, 0x0000 },
  /* 224, 0xE0 */ { 0x1111, 0x0000, 0x0000, 0x0000 },
  /* 225, 0xE1 */ { 0x0000, 0xEEEE, 0x0000, 0x0000 },
  /* 226, 0xE2 */ { 0x0000, 0xDDDD, 0x0000, 0x0000 },
  /* 227, 0xE3 */ { 0x0000, 0xBBBB, 0x0000, 0x0000 },
  /* 228, 0xE4 */ { 0x0000, 0xAAAA, 0x0000, 0x0000 },
  /* 229, 0xE5 */ { 0x0000, 0x8888, 0x0000, 0x0000 },
  /* 230, 0xE6 */ { 0x0000, 0x7777, 0x0000, 0x0000 },
  /* 231, 0xE7 */ { 0x0000, 0x5555, 0x0000, 0x0000 },
  /* 232, 0xE8 */ { 0x0000, 0x4444, 0x0000, 0x0000 },
  /* 233, 0xE9 */ { 0x0000, 0x2222, 0x0000, 0x0000 },
  /* 234, 0xEA */ { 0x0000, 0x1111, 0x0000, 0x0000 },
  /* 235, 0xEB */ { 0x0000, 0x0000, 0xEEEE, 0x0000 },
  /* 236, 0xEC */ { 0x0000, 0x0000, 0xDDDD, 0x0000 },
  /* 237, 0xED */ { 0x0000, 0x0000, 0xBBBB, 0x0000 },
  /* 238, 0xEE */ { 0x0000, 0x0000, 0xAAAA, 0x0000 },
  /* 239, 0xEF */ { 0x0000, 0x0000, 0x8888, 0x0000 },
  /* 240, 0xF0 */ { 0x0000, 0x0000, 0x7777, 0x0000 },
  /* 241, 0xF1 */ { 0x0000, 0x0000, 0x5555, 0x0000 },
  /* 242, 0xF2 */ { 0x0000, 0x0000, 0x4444, 0x0000 },
  /* 243, 0xF3 */ { 0x0000, 0x0000, 0x2222, 0x0000 },
  /* 244, 0xF4 */ { 0x0000, 0x0000, 0x1111, 0x0000 },
  /* 245, 0xF5 */ { 0xEEEE, 0xEEEE, 0xEEEE, 0x0000 },
  /* 246, 0xF6 */ { 0xDDDD, 0xDDDD, 0xDDDD, 0x0000 },
  /* 247, 0xF7 */ { 0xBBBB, 0xBBBB, 0xBBBB, 0x0000 },
  /* 248, 0xF8 */ { 0xAAAA, 0xAAAA, 0xAAAA, 0x0000 },
  /* 249, 0xF9 */ { 0x8888, 0x8888, 0x8888, 0x0000 },
  /* 250, 0xFA */ { 0x7777, 0x7777, 0x7777, 0x0000 },
  /* 251, 0xFB */ { 0x5555, 0x5555, 0x5555, 0x0000 },
  /* 252, 0xFC */ { 0x4444, 0x4444, 0x4444, 0x0000 },
  /* 253, 0xFD */ { 0x2222, 0x2222, 0x2222, 0x0000 },
  /* 254, 0xFE */ { 0x1111, 0x1111, 0x1111, 0x0000 },
  /* 255, 0xFF */ { 0x0000, 0x0000, 0x0000, 0x0000 }
};

palette_entry qt_default_palette_4_gray[] = {
 { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
 { 0xAAAA, 0xAAAA, 0xAAAA, 0xFFFF },
 { 0x5555, 0x5555, 0x5555, 0xFFFF },
 { 0x0000, 0x0000, 0x0000, 0xFFFF },
};

palette_entry qt_default_palette_16_gray[] = {
 { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
 { 0xEEEE, 0xEEEE, 0xEEEE, 0xFFFF },
 { 0xDDDD, 0xDDDD, 0xDDDD, 0xFFFF },
 { 0xCCCC, 0xCCCC, 0xCCCC, 0xFFFF },
 { 0xBBBB, 0xBBBB, 0xBBBB, 0xFFFF },
 { 0xAAAA, 0xAAAA, 0xAAAA, 0xFFFF },
 { 0x9999, 0x9999, 0x9999, 0xFFFF },
 { 0x8888, 0x8888, 0x8888, 0xFFFF },
 { 0x7777, 0x7777, 0x7777, 0xFFFF },
 { 0x6666, 0x6666, 0x6666, 0xFFFF },
 { 0x5555, 0x5555, 0x5555, 0xFFFF },
 { 0x4444, 0x4444, 0x4444, 0xFFFF },
 { 0x3333, 0x3333, 0x3333, 0xFFFF },
 { 0x2222, 0x2222, 0x2222, 0xFFFF },
 { 0x1111, 0x1111, 0x1111, 0xFFFF },
 { 0x0000, 0x0000, 0x0000, 0xFFFF },
};

palette_entry qt_default_palette_256_gray[] = {
 { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
 { 0xFEFE, 0xFEFE, 0xFEFE, 0xFFFF },
 { 0xFDFD, 0xFDFD, 0xFDFD, 0xFFFF },
 { 0xFCFC, 0xFCFC, 0xFCFC, 0xFFFF },
 { 0xFBFB, 0xFBFB, 0xFBFB, 0xFFFF },
 { 0xFAFA, 0xFAFA, 0xFAFA, 0xFFFF },
 { 0xF9F9, 0xF9F9, 0xF9F9, 0xFFFF },
 { 0xF8F8, 0xF8F8, 0xF8F8, 0xFFFF },
 { 0xF7F7, 0xF7F7, 0xF7F7, 0xFFFF },
 { 0xF6F6, 0xF6F6, 0xF6F6, 0xFFFF },
 { 0xF5F5, 0xF5F5, 0xF5F5, 0xFFFF },
 { 0xF4F4, 0xF4F4, 0xF4F4, 0xFFFF },
 { 0xF3F3, 0xF3F3, 0xF3F3, 0xFFFF },
 { 0xF2F2, 0xF2F2, 0xF2F2, 0xFFFF },
 { 0xF1F1, 0xF1F1, 0xF1F1, 0xFFFF },
 { 0xF0F0, 0xF0F0, 0xF0F0, 0xFFFF },
 { 0xEFEF, 0xEFEF, 0xEFEF, 0xFFFF },
 { 0xEEEE, 0xEEEE, 0xEEEE, 0xFFFF },
 { 0xEDED, 0xEDED, 0xEDED, 0xFFFF },
 { 0xECEC, 0xECEC, 0xECEC, 0xFFFF },
 { 0xEBEB, 0xEBEB, 0xEBEB, 0xFFFF },
 { 0xEAEA, 0xEAEA, 0xEAEA, 0xFFFF },
 { 0xE9E9, 0xE9E9, 0xE9E9, 0xFFFF },
 { 0xE8E8, 0xE8E8, 0xE8E8, 0xFFFF },
 { 0xE7E7, 0xE7E7, 0xE7E7, 0xFFFF },
 { 0xE6E6, 0xE6E6, 0xE6E6, 0xFFFF },
 { 0xE5E5, 0xE5E5, 0xE5E5, 0xFFFF },
 { 0xE4E4, 0xE4E4, 0xE4E4, 0xFFFF },
 { 0xE3E3, 0xE3E3, 0xE3E3, 0xFFFF },
 { 0xE2E2, 0xE2E2, 0xE2E2, 0xFFFF },
 { 0xE1E1, 0xE1E1, 0xE1E1, 0xFFFF },
 { 0xE0E0, 0xE0E0, 0xE0E0, 0xFFFF },
 { 0xDFDF, 0xDFDF, 0xDFDF, 0xFFFF },
 { 0xDEDE, 0xDEDE, 0xDEDE, 0xFFFF },
 { 0xDDDD, 0xDDDD, 0xDDDD, 0xFFFF },
 { 0xDCDC, 0xDCDC, 0xDCDC, 0xFFFF },
 { 0xDBDB, 0xDBDB, 0xDBDB, 0xFFFF },
 { 0xDADA, 0xDADA, 0xDADA, 0xFFFF },
 { 0xD9D9, 0xD9D9, 0xD9D9, 0xFFFF },
 { 0xD8D8, 0xD8D8, 0xD8D8, 0xFFFF },
 { 0xD7D7, 0xD7D7, 0xD7D7, 0xFFFF },
 { 0xD6D6, 0xD6D6, 0xD6D6, 0xFFFF },
 { 0xD5D5, 0xD5D5, 0xD5D5, 0xFFFF },
 { 0xD4D4, 0xD4D4, 0xD4D4, 0xFFFF },
 { 0xD3D3, 0xD3D3, 0xD3D3, 0xFFFF },
 { 0xD2D2, 0xD2D2, 0xD2D2, 0xFFFF },
 { 0xD1D1, 0xD1D1, 0xD1D1, 0xFFFF },
 { 0xD0D0, 0xD0D0, 0xD0D0, 0xFFFF },
 { 0xCFCF, 0xCFCF, 0xCFCF, 0xFFFF },
 { 0xCECE, 0xCECE, 0xCECE, 0xFFFF },
 { 0xCDCD, 0xCDCD, 0xCDCD, 0xFFFF },
 { 0xCCCC, 0xCCCC, 0xCCCC, 0xFFFF },
 { 0xCBCB, 0xCBCB, 0xCBCB, 0xFFFF },
 { 0xCACA, 0xCACA, 0xCACA, 0xFFFF },
 { 0xC9C9, 0xC9C9, 0xC9C9, 0xFFFF },
 { 0xC8C8, 0xC8C8, 0xC8C8, 0xFFFF },
 { 0xC7C7, 0xC7C7, 0xC7C7, 0xFFFF },
 { 0xC6C6, 0xC6C6, 0xC6C6, 0xFFFF },
 { 0xC5C5, 0xC5C5, 0xC5C5, 0xFFFF },
 { 0xC4C4, 0xC4C4, 0xC4C4, 0xFFFF },
 { 0xC3C3, 0xC3C3, 0xC3C3, 0xFFFF },
 { 0xC2C2, 0xC2C2, 0xC2C2, 0xFFFF },
 { 0xC1C1, 0xC1C1, 0xC1C1, 0xFFFF },
 { 0xC0C0, 0xC0C0, 0xC0C0, 0xFFFF },
 { 0xBFBF, 0xBFBF, 0xBFBF, 0xFFFF },
 { 0xBEBE, 0xBEBE, 0xBEBE, 0xFFFF },
 { 0xBDBD, 0xBDBD, 0xBDBD, 0xFFFF },
 { 0xBCBC, 0xBCBC, 0xBCBC, 0xFFFF },
 { 0xBBBB, 0xBBBB, 0xBBBB, 0xFFFF },
 { 0xBABA, 0xBABA, 0xBABA, 0xFFFF },
 { 0xB9B9, 0xB9B9, 0xB9B9, 0xFFFF },
 { 0xB8B8, 0xB8B8, 0xB8B8, 0xFFFF },
 { 0xB7B7, 0xB7B7, 0xB7B7, 0xFFFF },
 { 0xB6B6, 0xB6B6, 0xB6B6, 0xFFFF },
 { 0xB5B5, 0xB5B5, 0xB5B5, 0xFFFF },
 { 0xB4B4, 0xB4B4, 0xB4B4, 0xFFFF },
 { 0xB3B3, 0xB3B3, 0xB3B3, 0xFFFF },
 { 0xB2B2, 0xB2B2, 0xB2B2, 0xFFFF },
 { 0xB1B1, 0xB1B1, 0xB1B1, 0xFFFF },
 { 0xB0B0, 0xB0B0, 0xB0B0, 0xFFFF },
 { 0xAFAF, 0xAFAF, 0xAFAF, 0xFFFF },
 { 0xAEAE, 0xAEAE, 0xAEAE, 0xFFFF },
 { 0xADAD, 0xADAD, 0xADAD, 0xFFFF },
 { 0xACAC, 0xACAC, 0xACAC, 0xFFFF },
 { 0xABAB, 0xABAB, 0xABAB, 0xFFFF },
 { 0xAAAA, 0xAAAA, 0xAAAA, 0xFFFF },
 { 0xA9A9, 0xA9A9, 0xA9A9, 0xFFFF },
 { 0xA8A8, 0xA8A8, 0xA8A8, 0xFFFF },
 { 0xA7A7, 0xA7A7, 0xA7A7, 0xFFFF },
 { 0xA6A6, 0xA6A6, 0xA6A6, 0xFFFF },
 { 0xA5A5, 0xA5A5, 0xA5A5, 0xFFFF },
 { 0xA4A4, 0xA4A4, 0xA4A4, 0xFFFF },
 { 0xA3A3, 0xA3A3, 0xA3A3, 0xFFFF },
 { 0xA2A2, 0xA2A2, 0xA2A2, 0xFFFF },
 { 0xA1A1, 0xA1A1, 0xA1A1, 0xFFFF },
 { 0xA0A0, 0xA0A0, 0xA0A0, 0xFFFF },
 { 0x9F9F, 0x9F9F, 0x9F9F, 0xFFFF },
 { 0x9E9E, 0x9E9E, 0x9E9E, 0xFFFF },
 { 0x9D9D, 0x9D9D, 0x9D9D, 0xFFFF },
 { 0x9C9C, 0x9C9C, 0x9C9C, 0xFFFF },
 { 0x9B9B, 0x9B9B, 0x9B9B, 0xFFFF },
 { 0x9A9A, 0x9A9A, 0x9A9A, 0xFFFF },
 { 0x9999, 0x9999, 0x9999, 0xFFFF },
 { 0x9898, 0x9898, 0x9898, 0xFFFF },
 { 0x9797, 0x9797, 0x9797, 0xFFFF },
 { 0x9696, 0x9696, 0x9696, 0xFFFF },
 { 0x9595, 0x9595, 0x9595, 0xFFFF },
 { 0x9494, 0x9494, 0x9494, 0xFFFF },
 { 0x9393, 0x9393, 0x9393, 0xFFFF },
 { 0x9292, 0x9292, 0x9292, 0xFFFF },
 { 0x9191, 0x9191, 0x9191, 0xFFFF },
 { 0x9090, 0x9090, 0x9090, 0xFFFF },
 { 0x8F8F, 0x8F8F, 0x8F8F, 0xFFFF },
 { 0x8E8E, 0x8E8E, 0x8E8E, 0xFFFF },
 { 0x8D8D, 0x8D8D, 0x8D8D, 0xFFFF },
 { 0x8C8C, 0x8C8C, 0x8C8C, 0xFFFF },
 { 0x8B8B, 0x8B8B, 0x8B8B, 0xFFFF },
 { 0x8A8A, 0x8A8A, 0x8A8A, 0xFFFF },
 { 0x8989, 0x8989, 0x8989, 0xFFFF },
 { 0x8888, 0x8888, 0x8888, 0xFFFF },
 { 0x8787, 0x8787, 0x8787, 0xFFFF },
 { 0x8686, 0x8686, 0x8686, 0xFFFF },
 { 0x8585, 0x8585, 0x8585, 0xFFFF },
 { 0x8484, 0x8484, 0x8484, 0xFFFF },
 { 0x8383, 0x8383, 0x8383, 0xFFFF },
 { 0x8282, 0x8282, 0x8282, 0xFFFF },
 { 0x8181, 0x8181, 0x8181, 0xFFFF },
 { 0x8080, 0x8080, 0x8080, 0xFFFF },
 { 0x7F7F, 0x7F7F, 0x7F7F, 0xFFFF },
 { 0x7E7E, 0x7E7E, 0x7E7E, 0xFFFF },
 { 0x7D7D, 0x7D7D, 0x7D7D, 0xFFFF },
 { 0x7C7C, 0x7C7C, 0x7C7C, 0xFFFF },
 { 0x7B7B, 0x7B7B, 0x7B7B, 0xFFFF },
 { 0x7A7A, 0x7A7A, 0x7A7A, 0xFFFF },
 { 0x7979, 0x7979, 0x7979, 0xFFFF },
 { 0x7878, 0x7878, 0x7878, 0xFFFF },
 { 0x7777, 0x7777, 0x7777, 0xFFFF },
 { 0x7676, 0x7676, 0x7676, 0xFFFF },
 { 0x7575, 0x7575, 0x7575, 0xFFFF },
 { 0x7474, 0x7474, 0x7474, 0xFFFF },
 { 0x7373, 0x7373, 0x7373, 0xFFFF },
 { 0x7272, 0x7272, 0x7272, 0xFFFF },
 { 0x7171, 0x7171, 0x7171, 0xFFFF },
 { 0x7070, 0x7070, 0x7070, 0xFFFF },
 { 0x6F6F, 0x6F6F, 0x6F6F, 0xFFFF },
 { 0x6E6E, 0x6E6E, 0x6E6E, 0xFFFF },
 { 0x6D6D, 0x6D6D, 0x6D6D, 0xFFFF },
 { 0x6C6C, 0x6C6C, 0x6C6C, 0xFFFF },
 { 0x6B6B, 0x6B6B, 0x6B6B, 0xFFFF },
 { 0x6A6A, 0x6A6A, 0x6A6A, 0xFFFF },
 { 0x6969, 0x6969, 0x6969, 0xFFFF },
 { 0x6868, 0x6868, 0x6868, 0xFFFF },
 { 0x6767, 0x6767, 0x6767, 0xFFFF },
 { 0x6666, 0x6666, 0x6666, 0xFFFF },
 { 0x6565, 0x6565, 0x6565, 0xFFFF },
 { 0x6464, 0x6464, 0x6464, 0xFFFF },
 { 0x6363, 0x6363, 0x6363, 0xFFFF },
 { 0x6262, 0x6262, 0x6262, 0xFFFF },
 { 0x6161, 0x6161, 0x6161, 0xFFFF },
 { 0x6060, 0x6060, 0x6060, 0xFFFF },
 { 0x5F5F, 0x5F5F, 0x5F5F, 0xFFFF },
 { 0x5E5E, 0x5E5E, 0x5E5E, 0xFFFF },
 { 0x5D5D, 0x5D5D, 0x5D5D, 0xFFFF },
 { 0x5C5C, 0x5C5C, 0x5C5C, 0xFFFF },
 { 0x5B5B, 0x5B5B, 0x5B5B, 0xFFFF },
 { 0x5A5A, 0x5A5A, 0x5A5A, 0xFFFF },
 { 0x5959, 0x5959, 0x5959, 0xFFFF },
 { 0x5858, 0x5858, 0x5858, 0xFFFF },
 { 0x5757, 0x5757, 0x5757, 0xFFFF },
 { 0x5656, 0x5656, 0x5656, 0xFFFF },
 { 0x5555, 0x5555, 0x5555, 0xFFFF },
 { 0x5454, 0x5454, 0x5454, 0xFFFF },
 { 0x5353, 0x5353, 0x5353, 0xFFFF },
 { 0x5252, 0x5252, 0x5252, 0xFFFF },
 { 0x5151, 0x5151, 0x5151, 0xFFFF },
 { 0x5050, 0x5050, 0x5050, 0xFFFF },
 { 0x4F4F, 0x4F4F, 0x4F4F, 0xFFFF },
 { 0x4E4E, 0x4E4E, 0x4E4E, 0xFFFF },
 { 0x4D4D, 0x4D4D, 0x4D4D, 0xFFFF },
 { 0x4C4C, 0x4C4C, 0x4C4C, 0xFFFF },
 { 0x4B4B, 0x4B4B, 0x4B4B, 0xFFFF },
 { 0x4A4A, 0x4A4A, 0x4A4A, 0xFFFF },
 { 0x4949, 0x4949, 0x4949, 0xFFFF },
 { 0x4848, 0x4848, 0x4848, 0xFFFF },
 { 0x4747, 0x4747, 0x4747, 0xFFFF },
 { 0x4646, 0x4646, 0x4646, 0xFFFF },
 { 0x4545, 0x4545, 0x4545, 0xFFFF },
 { 0x4444, 0x4444, 0x4444, 0xFFFF },
 { 0x4343, 0x4343, 0x4343, 0xFFFF },
 { 0x4242, 0x4242, 0x4242, 0xFFFF },
 { 0x4141, 0x4141, 0x4141, 0xFFFF },
 { 0x4040, 0x4040, 0x4040, 0xFFFF },
 { 0x3F3F, 0x3F3F, 0x3F3F, 0xFFFF },
 { 0x3E3E, 0x3E3E, 0x3E3E, 0xFFFF },
 { 0x3D3D, 0x3D3D, 0x3D3D, 0xFFFF },
 { 0x3C3C, 0x3C3C, 0x3C3C, 0xFFFF },
 { 0x3B3B, 0x3B3B, 0x3B3B, 0xFFFF },
 { 0x3A3A, 0x3A3A, 0x3A3A, 0xFFFF },
 { 0x3939, 0x3939, 0x3939, 0xFFFF },
 { 0x3838, 0x3838, 0x3838, 0xFFFF },
 { 0x3737, 0x3737, 0x3737, 0xFFFF },
 { 0x3636, 0x3636, 0x3636, 0xFFFF },
 { 0x3535, 0x3535, 0x3535, 0xFFFF },
 { 0x3434, 0x3434, 0x3434, 0xFFFF },
 { 0x3333, 0x3333, 0x3333, 0xFFFF },
 { 0x3232, 0x3232, 0x3232, 0xFFFF },
 { 0x3131, 0x3131, 0x3131, 0xFFFF },
 { 0x3030, 0x3030, 0x3030, 0xFFFF },
 { 0x2F2F, 0x2F2F, 0x2F2F, 0xFFFF },
 { 0x2E2E, 0x2E2E, 0x2E2E, 0xFFFF },
 { 0x2D2D, 0x2D2D, 0x2D2D, 0xFFFF },
 { 0x2C2C, 0x2C2C, 0x2C2C, 0xFFFF },
 { 0x2B2B, 0x2B2B, 0x2B2B, 0xFFFF },
 { 0x2A2A, 0x2A2A, 0x2A2A, 0xFFFF },
 { 0x2929, 0x2929, 0x2929, 0xFFFF },
 { 0x2828, 0x2828, 0x2828, 0xFFFF },
 { 0x2727, 0x2727, 0x2727, 0xFFFF },
 { 0x2626, 0x2626, 0x2626, 0xFFFF },
 { 0x2525, 0x2525, 0x2525, 0xFFFF },
 { 0x2424, 0x2424, 0x2424, 0xFFFF },
 { 0x2323, 0x2323, 0x2323, 0xFFFF },
 { 0x2222, 0x2222, 0x2222, 0xFFFF },
 { 0x2121, 0x2121, 0x2121, 0xFFFF },
 { 0x2020, 0x2020, 0x2020, 0xFFFF },
 { 0x1F1F, 0x1F1F, 0x1F1F, 0xFFFF },
 { 0x1E1E, 0x1E1E, 0x1E1E, 0xFFFF },
 { 0x1D1D, 0x1D1D, 0x1D1D, 0xFFFF },
 { 0x1C1C, 0x1C1C, 0x1C1C, 0xFFFF },
 { 0x1B1B, 0x1B1B, 0x1B1B, 0xFFFF },
 { 0x1A1A, 0x1A1A, 0x1A1A, 0xFFFF },
 { 0x1919, 0x1919, 0x1919, 0xFFFF },
 { 0x1818, 0x1818, 0x1818, 0xFFFF },
 { 0x1717, 0x1717, 0x1717, 0xFFFF },
 { 0x1616, 0x1616, 0x1616, 0xFFFF },
 { 0x1515, 0x1515, 0x1515, 0xFFFF },
 { 0x1414, 0x1414, 0x1414, 0xFFFF },
 { 0x1313, 0x1313, 0x1313, 0xFFFF },
 { 0x1212, 0x1212, 0x1212, 0xFFFF },
 { 0x1111, 0x1111, 0x1111, 0xFFFF },
 { 0x1010, 0x1010, 0x1010, 0xFFFF },
 { 0x0F0F, 0x0F0F, 0x0F0F, 0xFFFF },
 { 0x0E0E, 0x0E0E, 0x0E0E, 0xFFFF },
 { 0x0D0D, 0x0D0D, 0x0D0D, 0xFFFF },
 { 0x0C0C, 0x0C0C, 0x0C0C, 0xFFFF },
 { 0x0B0B, 0x0B0B, 0x0B0B, 0xFFFF },
 { 0x0A0A, 0x0A0A, 0x0A0A, 0xFFFF },
 { 0x0909, 0x0909, 0x0909, 0xFFFF },
 { 0x0808, 0x0808, 0x0808, 0xFFFF },
 { 0x0707, 0x0707, 0x0707, 0xFFFF },
 { 0x0606, 0x0606, 0x0606, 0xFFFF },
 { 0x0505, 0x0505, 0x0505, 0xFFFF },
 { 0x0404, 0x0404, 0x0404, 0xFFFF },
 { 0x0303, 0x0303, 0x0303, 0xFFFF },
 { 0x0202, 0x0202, 0x0202, 0xFFFF },
 { 0x0101, 0x0101, 0x0101, 0xFFFF },
 { 0x0000, 0x0000, 0x0000, 0xFFFF },
};

void quicktime_default_ctab(quicktime_ctab_t * ctab, int depth)
  {
  palette_entry * e;
  int i;
  switch(depth)
    {
    case 1:
      e = qt_default_palette_2;
      ctab->size = 2;
      break;
    case 2:
      e = qt_default_palette_4;
      ctab->size = 4;
      break;
    case 4:
      e = qt_default_palette_16;
      ctab->size = 16;
      break;
    case 8:
      e = qt_default_palette_256;
      ctab->size = 256;
      break;
    case 34:
      e = qt_default_palette_4_gray;
      ctab->size = 4;
      break;
    case 36:
      e = qt_default_palette_16_gray;
      ctab->size = 16;
      break;
    case 40:
      e = qt_default_palette_256_gray;
      ctab->size = 256;
      break;
    default: /* No palette available */
      return;
    }
  
  ctab->alpha = malloc(sizeof(int16_t) * ctab->size);
  ctab->red = malloc(sizeof(int16_t) * ctab->size);
  ctab->green = malloc(sizeof(int16_t) * ctab->size);
  ctab->blue = malloc(sizeof(int16_t) * ctab->size);
  
  for(i = 0; i < ctab->size; i++)
    {
    ctab->alpha[i] = e[i].a;
    ctab->red[i]   = e[i].r;
    ctab->green[i] = e[i].g;
    ctab->blue[i]  = e[i].b;
    }
  }
