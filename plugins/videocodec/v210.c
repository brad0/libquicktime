#include "funcprotos.h"
#include <quicktime/colormodels.h>
#include <quicktime/quicktime.h>
#include <workarounds.h>
#include "v210.h"

#include <stdlib.h>
#include <string.h>

typedef struct
    {
    uint8_t *work_buffer;
/* The V210 codec reqires a bytes/line that is a multiple of 128 (48 pixels). */
    int64_t    bytes_per_line;
    int    initialized;
    } quicktime_v210_codec_t;

static int delete_codec(quicktime_video_map_t *vtrack)
    {
    quicktime_v210_codec_t *codec;

    codec = ((quicktime_codec_t*)vtrack->codec)->priv;
    if  (codec->work_buffer)
        free(codec->work_buffer);
    free(codec);
    return 0;
    }

/*
 * http://developer.apple.com/quicktime/icefloe/dispatch019.html
 *
 * Lines must be a multiple of 48 pixels.  Bytes per luma sample is 8/3. 
 * Alignment of 48 pixels * bytes/luma = 128.  Q.E.D.
*/
static void initialize(quicktime_video_map_t *vtrack, 
               quicktime_v210_codec_t *codec, int width, int height) 
    {

    if  (codec->initialized != 0)
        return;
    codec->bytes_per_line = ((((width + 47) / 48) * 48 * 8) / 3);;
    if  (!codec->work_buffer)
        codec->work_buffer = malloc(codec->bytes_per_line * 
                                    vtrack->track->tkhd.track_height);
    codec->initialized = 1;
    }

static int decode(quicktime_t *file, unsigned char **row_pointers, int track)
    {
    uint32_t i1, i2, i3, i4;
    uint8_t *in_ptr, *iptr;
    uint16_t * out_y, * out_u, * out_v;
    int i, j;
    int64_t bytes;
    int result = 0;
    quicktime_video_map_t *vtrack = &(file->vtracks[track]);
    quicktime_v210_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
    int width = vtrack->track->tkhd.track_width;
    int height = vtrack->track->tkhd.track_height;

    if  (!row_pointers)
        {
        vtrack->stream_cmodel = BC_YUV444P16;
        return 0;
        }

    initialize(vtrack, codec, width, height);

    quicktime_set_video_position(file, vtrack->current_position, track);
    bytes = quicktime_frame_size(file, vtrack->current_position, track);
    result = !quicktime_read_data(file, codec->work_buffer, bytes);

    in_ptr = codec->work_buffer;

    for (i = 0; i < height; i++, in_ptr += codec->bytes_per_line)
        {
        out_y = (uint16_t*)(row_pointers[0] + i * file->vtracks[track].stream_row_span);
        out_u = (uint16_t*)(row_pointers[1] + i * file->vtracks[track].stream_row_span_uv);
        out_v = (uint16_t*)(row_pointers[2] + i * file->vtracks[track].stream_row_span_uv);

/*
 * 4 32bit words unpack into 6 pixels.  Due to padding (v210 pads lines to
 * the nearest 48pixel boundary) the last several pixels can be mingled with 
 * padding zeroes.  For example 1280 is not a multiple of 48 and is padded 
 * to 1296.
 *
 * Do as many groups of  6 pixels that do not exceed the (unpadded) width. The
 * remaining 2 or 4 pixels will done outside the loop.
*/
        for (iptr = in_ptr, j = 0; j < width / 6; j++, iptr += 16)
            {
            /* v210 is LITTLE endian!! */
            i1 = iptr[0] | (iptr[1] << 8) | (iptr[2] << 16) | (iptr[3] << 24);
            i2 = iptr[4] | (iptr[5] << 8) | (iptr[6] << 16) | (iptr[7] << 24);
            i3 = iptr[8] | (iptr[9] << 8) | (iptr[10] << 16) | (iptr[11] << 24);
            i4 = iptr[12] | (iptr[13] << 8) | (iptr[14] << 16) | (iptr[15] << 24);
/* These are grouped to show the "pixel pairs" of  4:2:2 */
            *(out_u++) = (i1 & 0x3ff) << 6;       /* Cb0 */
            *(out_y++) = (i1 & 0xffc00) >> 4;     /* Y0 */
            *(out_v++) = (i1 & 0x3ff00000) >> 14; /* Cr0 */
            *(out_y++) = (i2 & 0x3ff) << 6;       /* Y1 */

            *(out_u++) = (i2 & 0xffc00) >> 4;     /* Cb1 */
            *(out_y++) = (i2 & 0x3ff00000) >> 14; /* Y2 */
            *(out_v++) = (i3 & 0x3ff) << 6;       /* Cr1 */
            *(out_y++) = (i3 & 0xffc00) >> 4;     /* Y3 */

            *(out_u++) = (i3 & 0x3ff00000) >> 14; /* Cb2 */
            *(out_y++) = (i4 & 0x3ff) << 6;       /* Y4 */
            *(out_v++) = (i4 & 0xffc00) >> 4;     /* Cr2 */
            *(out_y++) = (i4 & 0x3ff00000) >> 14; /* Y5 */
            }
/* Handle the 2 or 4 pixels possibly remaining */
         j = (width - ((width / 6) * 6));
	 if (j != 0)
	    {
            i1 = iptr[0] | (iptr[1] << 8) | (iptr[2] << 16) | (iptr[3] << 24);
            i2 = iptr[4] | (iptr[5] << 8) | (iptr[6] << 16) | (iptr[7] << 24);
            i3 = iptr[8] | (iptr[9] << 8) | (iptr[10] << 16) | (iptr[11] << 24);
            i4 = iptr[12] | (iptr[13] << 8) | (iptr[14] << 16) | (iptr[15] << 24);
            *(out_u++) = (i1 & 0x3ff) << 6;       /* Cb0 */
            *(out_y++) = (i1 & 0xffc00) >> 4;     /* Y0 */
            *(out_v++) = (i1 & 0x3ff00000) >> 14; /* Cr0 */
            *(out_y++) = (i2 & 0x3ff) << 6;       /* Y1 */
	    if (j == 4)
	       {
               *(out_u++) = (i2 & 0xffc00) >> 4;     /* Cb1 */
               *(out_y++) = (i2 & 0x3ff00000) >> 14; /* Y2 */
               *(out_v++) = (i3 & 0x3ff) << 6;       /* Cr1 */
               *(out_y++) = (i3 & 0xffc00) >> 4;     /* Y3 */
	       }
	   }
	}
    return result;
    }

static int encode(quicktime_t *file, unsigned char **row_pointers, int track)
    {
    quicktime_video_map_t *vtrack = &(file->vtracks[track]);
    quicktime_v210_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
    quicktime_trak_t *trak = vtrack->track;
    int width = vtrack->track->tkhd.track_width;
    int height = vtrack->track->tkhd.track_height;
    int result = 0;
    int i, j;
    quicktime_atom_t chunk_atom;
    uint16_t * in_y, * in_u, * in_v;
    uint8_t *out_ptr, *optr;
    uint32_t o1, o2, o3, o4;
        
    if  (!row_pointers)
        {
        vtrack->stream_cmodel = BC_YUV444P16;
        return 0;
        }

    initialize(vtrack, codec, width, height);

    out_ptr = codec->work_buffer;

    for (i = 0; i < height; i++, out_ptr += codec->bytes_per_line)
        {
        in_y = (uint16_t*)(row_pointers[0] + i * file->vtracks[track].stream_row_span);
        in_u = (uint16_t*)(row_pointers[1] + i * file->vtracks[track].stream_row_span_uv);
        in_v = (uint16_t*)(row_pointers[2] + i * file->vtracks[track].stream_row_span_uv);
          
/*
 * 12 values (6 pixels = 2 Y', 1 Cb/u and 1 Cr/v) pack into 4 32bit words.
*/
	optr = out_ptr;
        for (j = 0; j < width / 6; j++)
            {
            o1 = ((*in_v & 0xffc0) << 14) |       /* Cr0 */
                 ((*in_y & 0xffc0) << 4) |        /* Y0 */
                 ((*in_u & 0xffc0) >> 6);         /* Cb0 */
            in_y++; in_u++; in_v++;

            o2 = ((in_y[1] & 0xffc0) << 14) |       /* Y2 */
                 ((*in_u & 0xffc0) << 4) |           /* Cb1 */
                 ((in_y[0] & 0xffc0) >> 6);         /* Y1 */
            in_y ++; in_u++; in_y++;

            o3 = ((*in_u & 0xffc0) << 14) |       /* Cb2 */
                 ((*in_y & 0xffc0) << 4) |        /* Y3 */
                 ((*in_v & 0xffc0) >> 6);         /* Cr1 */
	    in_u++; in_y++; in_v++;

            o4 = ((in_y[1] & 0xffc0) << 14) |    /* Y5 */
                 ((*in_v & 0xffc0) << 4) |        /* Cr2 */
                 ((in_y[0] & 0xffc0) >> 6);         /* Y4 */
	    in_y++; in_v++; in_y++;

            *optr++ = (o1 & 0xff);
            *optr++ = (o1 & 0xff00) >> 8;
            *optr++ = (o1 & 0xff0000) >> 16;
            *optr++ = (o1 & 0xff000000) >> 24;

            *optr++ = (o2 & 0xff);
            *optr++ = (o2 & 0xff00) >> 8;
            *optr++ = (o2 & 0xff0000) >> 16;
            *optr++ = (o2 & 0xff000000) >> 24;

            *optr++ = (o3 & 0xff);
            *optr++ = (o3 & 0xff00) >> 8;
            *optr++ = (o3 & 0xff0000) >> 16;
            *optr++ = (o3 & 0xff000000) >> 24;

            *optr++ = (o4 & 0xff);
            *optr++ = (o4 & 0xff00) >> 8;
            *optr++ = (o4 & 0xff0000) >> 16;
            *optr++ = (o4 & 0xff000000) >> 24;
            }
/* Handle the 2 or 4 pixels remaining before the padding */
        j = (width - ((width / 6) * 6));
	if  (j != 0)
	    {
            o1 = ((*in_v & 0xffc0) << 14) |       /* Cr0 */
                 ((*in_y & 0xffc0) << 4) |        /* Y0 */
                 ((*in_u & 0xffc0) >> 6);         /* Cb0 */
            in_y++; in_u++; in_v++;
	    o2 = (*in_y & 0xffc0) >> 6;            /* Y1 */
	    in_y++;
	    
	    if  (j == 4)
	        {
                o2 |= (((in_y[1] & 0xffc0) << 14) |       /* Y2 */
                      ((*in_u & 0xffc0) << 4));          /* Cb1 */
                in_y ++; in_u++;

                o3 = ((*in_y & 0xffc0) << 4) |        /* Y3 */
                     ((*in_v & 0xffc0) >> 6);         /* Cr1 */
	        in_y++; in_v++;
		}
            *optr++ = (o1 & 0xff);
            *optr++ = (o1 & 0xff00) >> 8;
            *optr++ = (o1 & 0xff0000) >> 16;
            *optr++ = (o1 & 0xff000000) >> 24;

            *optr++ = (o2 & 0xff);
            *optr++ = (o2 & 0xff00) >> 8;
            *optr++ = (o2 & 0xff0000) >> 16;
            *optr++ = (o2 & 0xff000000) >> 24;
	    
            *optr++ = (o3 & 0xff);
            *optr++ = (o3 & 0xff00) >> 8;
            *optr++ = (o3 & 0xff0000) >> 16;
            *optr++ = (o3 & 0xff000000) >> 24;
/*
 * The above are sufficient for 2 or 4 pixels, the 4th word (which would be o4)
 * contains the two Y' samples (Y4 and Y5) and the Cr (Cr2) for pixels 5 and 6.
 * We don't need Y4/Y5/Cr2 so don't put any more bytes into the out buffer.
*/
	    }

/*
 * Now compute the number of bytes used in the current line.  Zero pad until
 * the padded width is reached.  If the line does not require padding (for
 * 720xN) then the number of bytes used will be equal to bytes_per_line and
 * no padding will be performed.
*/
	j = out_ptr - optr;
fprintf(stderr, "padding = %d\n", j);
	while (j++ < codec->bytes_per_line)
	      *optr++ = '\0';
        }

    quicktime_write_chunk_header(file, trak, &chunk_atom);
    result = !quicktime_write_data(file,
                                   codec->work_buffer,
                                   codec->bytes_per_line * height);
    quicktime_write_chunk_footer(file,
                                 trak,
                                 vtrack->current_chunk,&chunk_atom,
				 1);
    vtrack->current_chunk++;
    return result;
    }

void quicktime_init_codec_v210(quicktime_video_map_t *vtrack)
    {
    quicktime_codec_t *codec_base = (quicktime_codec_t*)vtrack->codec;

/* Init public items */
    codec_base->priv = calloc(1, sizeof(quicktime_v210_codec_t));
    codec_base->delete_vcodec = delete_codec;
    codec_base->decode_video = decode;
    codec_base->encode_video = encode;
    codec_base->decode_audio = 0;
    codec_base->encode_audio = 0;
    codec_base->fourcc = QUICKTIME_V210;
    codec_base->title = "Component Y'CbCr 10-bit 4:2:2";
    codec_base->desc = "Component Y'CbCr 10-bit 4:2:2";
    }
