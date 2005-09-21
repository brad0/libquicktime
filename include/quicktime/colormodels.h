/*
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 
 * USA
 */
 
 
#ifndef COLORMODELS_H
#define COLORMODELS_H

// Colormodels
#define BC_COMPRESSED   1

#define BC_RGB565       2
#define BC_BGR565       3
#define BC_BGR888       4
#define BC_BGR8888      5
// Working bitmaps are packed to simplify processing
#define BC_RGB888       6
#define BC_RGBA8888     7
#define BC_RGB161616    8
#define BC_RGBA16161616 9
#define BC_YUVA8888     10
#define BC_YUV422       13
// Planar
#define BC_YUV420P      14
#define BC_YUV422P      15
#define BC_YUV444P      16
#define BC_YUV411P      17
/* JPEG scaled colormodels */
#define BC_YUVJ420P     18
#define BC_YUVJ422P     19
#define BC_YUVJ444P     20
/* 16 bit per component planar formats */
#define BC_YUV422P16    21
#define BC_YUV444P16    22


// Colormodels purely used by Quicktime are done in Quicktime.

// For communication with the X Server
#define FOURCC_YV12 0x32315659  /* YV12   YUV420P */
#define FOURCC_YUV2 0x32595559  /* YUV2   YUV422 */
#define FOURCC_I420 0x30323449  /* I420   Intel Indeo 4 */

// #undef RECLIP
// #define RECLIP(x, y, z) ((x) = ((x) < (y) ? (y) : ((x) > (z) ? (z) : (x))))

#ifdef __cplusplus
extern "C" {
#endif

int cmodel_calculate_pixelsize(int colormodel);
int cmodel_calculate_datasize(int w, int h, int bytes_per_line, int color_model);
int cmodel_calculate_max(int colormodel);
int cmodel_components(int colormodel);
int cmodel_is_yuv(int colormodel);

void cmodel_transfer(unsigned char **output_rows, /* Leave NULL if non existent */
	unsigned char **input_rows,
	int in_x,        /* Dimensions to capture from input frame */
	int in_y, 
	int in_w, 
	int in_h,
	int out_w, 
	int out_h,
	int in_colormodel, 
	int out_colormodel,
	int in_rowspan,       /* For planar use the luma rowspan */
        int out_rowspan,      /* For planar use the luma rowspan */
        int in_rowspan_uv,    /* Chroma rowspan */
        int out_rowspan_uv    /* Chroma rowspan */);     

int cmodel_bc_to_x(int color_model);
// Tell when to use plane arguments or row pointer arguments to functions
int cmodel_is_planar(int color_model);





#ifdef __cplusplus
}
#endif

#endif
