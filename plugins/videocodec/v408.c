#include "funcprotos.h"
#include <quicktime/colormodels.h>
#include <quicktime/quicktime.h>
#include <workarounds.h>

#include <stdlib.h>
#include <string.h>

static uint8_t decode_alpha_v408[256] =
{
  0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x06,
  0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0d, 0x0e, 0x0f,
  0x10, 0x11, 0x12, 0x13, 0x15, 0x16, 0x17, 0x18,
  0x19, 0x1a, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21,
  0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x2a, 0x2b,
  0x2c, 0x2d, 0x2e, 0x2f, 0x31, 0x32, 0x33, 0x34,
  0x35, 0x36, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d,
  0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x46, 0x47,
  0x48, 0x49, 0x4a, 0x4b, 0x4d, 0x4e, 0x4f, 0x50,
  0x51, 0x52, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
  0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x62, 0x63,
  0x64, 0x65, 0x66, 0x67, 0x68, 0x6a, 0x6b, 0x6c,
  0x6d, 0x6e, 0x6f, 0x71, 0x72, 0x73, 0x74, 0x75,
  0x76, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7f,
  0x80, 0x81, 0x82, 0x83, 0x84, 0x86, 0x87, 0x88,
  0x89, 0x8a, 0x8b, 0x8d, 0x8e, 0x8f, 0x90, 0x91,
  0x92, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9b,
  0x9c, 0x9d, 0x9e, 0x9f, 0xa0, 0xa2, 0xa3, 0xa4,
  0xa5, 0xa6, 0xa7, 0xa9, 0xaa, 0xab, 0xac, 0xad,
  0xae, 0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb7,
  0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbf, 0xc0,
  0xc1, 0xc2, 0xc3, 0xc4, 0xc6, 0xc7, 0xc8, 0xc9,
  0xca, 0xcb, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2,
  0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xdb, 0xdc,
  0xdd, 0xde, 0xdf, 0xe0, 0xe2, 0xe3, 0xe4, 0xe5,
  0xe6, 0xe7, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee,
  0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf7, 0xf8,
  0xf9, 0xfa, 0xfb, 0xfc, 0xfe, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

static uint8_t encode_alpha_v408[256] =
{
  0x10, 0x11, 0x12, 0x13, 0x13, 0x14, 0x15, 0x16,
  0x17, 0x18, 0x19, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
  0x1e, 0x1f, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24,
  0x25, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
  0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x31,
  0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x38,
  0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3e, 0x3f,
  0x40, 0x41, 0x42, 0x43, 0x44, 0x44, 0x45, 0x46,
  0x47, 0x48, 0x49, 0x4a, 0x4a, 0x4b, 0x4c, 0x4d,
  0x4e, 0x4f, 0x50, 0x50, 0x51, 0x52, 0x53, 0x54,
  0x55, 0x56, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b,
  0x5c, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62,
  0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x68,
  0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6e, 0x6f,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x74, 0x75, 0x76,
  0x77, 0x78, 0x79, 0x7a, 0x7a, 0x7b, 0x7c, 0x7d,
  0x7e, 0x7f, 0x80, 0x81, 0x81, 0x82, 0x83, 0x84,
  0x85, 0x86, 0x87, 0x87, 0x88, 0x89, 0x8a, 0x8b,
  0x8c, 0x8d, 0x8d, 0x8e, 0x8f, 0x90, 0x91, 0x92,
  0x93, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
  0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 0x9f,
  0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa5, 0xa6,
  0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xab, 0xac, 0xad,
  0xae, 0xaf, 0xb0, 0xb1, 0xb1, 0xb2, 0xb3, 0xb4,
  0xb5, 0xb6, 0xb7, 0xb7, 0xb8, 0xb9, 0xba, 0xbb,
  0xbc, 0xbd, 0xbd, 0xbe, 0xbf, 0xc0, 0xc1, 0xc2,
  0xc3, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9,
  0xca, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0,
  0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd6,
  0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdc, 0xdd,
  0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe2, 0xe3, 0xe4,
  0xe5, 0xe6, 0xe7, 0xe8, 0xe8, 0xe9, 0xea, 0xeb,
};


typedef struct
  {
  uint8_t *buffer;
  int buffer_alloc;
  } quicktime_v408_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_v408_codec_t *codec;

	codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	if(codec->buffer) free(codec->buffer);
	free(codec);
	return 0;
}

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
{
        uint8_t * in_ptr, *out_ptr;
        int i, j;
	int64_t bytes;
	int result = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_v408_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;

        if(!row_pointers)
          {
          //          vtrack->stream_cmodel = BC_UYVA8888;
          vtrack->stream_cmodel = BC_YUVA8888;
          return 0;
          }

        bytes = lqt_read_video_frame(file, &codec->buffer,
                                     &codec->buffer_alloc,
                                     vtrack->current_position, NULL, track);
        if(bytes <= 0)
          return -1;

        in_ptr = codec->buffer;
	for(i = 0; i < height; i++)
          {
          out_ptr = row_pointers[i];
          for(j = 0; j < width; j++)
            {
            out_ptr[0] = in_ptr[1]; /* Y */
            out_ptr[1] = in_ptr[0]; /* U */
            out_ptr[2] = in_ptr[2]; /* V */
            out_ptr[3] = decode_alpha_v408[in_ptr[3]]; /* A */

            out_ptr += 4;
            in_ptr += 4;
            }
          }

	return result;
}

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
{
        uint8_t * in_ptr, *out_ptr;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_v408_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	quicktime_trak_t *trak = vtrack->track;
	int width = vtrack->track->tkhd.track_width;
	int height = vtrack->track->tkhd.track_height;
	int bytes = width * height * 4;
	int result = 0;
	int i, j;
	quicktime_atom_t chunk_atom;

        if(!row_pointers)
          {
          vtrack->stream_cmodel = BC_YUVA8888;
          return 0;
          }
                
        if(!codec->buffer)
          codec->buffer = malloc(width * height * 4);

        out_ptr = codec->buffer;
	for(i = 0; i < height; i++)
          {
          in_ptr = row_pointers[i];
          for(j = 0; j < width; j++)
            {
            out_ptr[0] = in_ptr[1];
            out_ptr[1] = in_ptr[0];
            out_ptr[2] = in_ptr[2];
            out_ptr[3] = encode_alpha_v408[in_ptr[3]];

            out_ptr += 4;
            in_ptr += 4;
            }
          }
        
        quicktime_write_chunk_header(file, trak, &chunk_atom);
	result = !quicktime_write_data(file, codec->buffer, bytes);
	quicktime_write_chunk_footer(file, 
		trak,
		vtrack->current_chunk,
		&chunk_atom, 
		1);

	vtrack->current_chunk++;
	
	return result;
}

void quicktime_init_codec_v408(quicktime_video_map_t *vtrack)
{
	quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
	codec_base->priv = calloc(1, sizeof(quicktime_v408_codec_t));
	codec_base->delete_vcodec = delete_codec;
	codec_base->decode_video = decode;
	codec_base->encode_video = encode;
	codec_base->decode_audio = 0;
	codec_base->encode_audio = 0;
}

