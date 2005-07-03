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
#include <inttypes.h>
#include <cmodel_permutation.h>
#include <lqt.h>
#include <lqt_funcprotos.h>



#define TRANSFER_FRAME_DEFAULT(output, \
	input, \
	y_in_offset, \
	u_in_offset, \
	v_in_offset, \
	input_column) \
{ \
	register int i, j; \
 \
	switch(in_colormodel) \
	{ \
		case BC_RGB565: \
			switch(out_colormodel) \
			{ \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
                                          transfer_RGB565_to_RGB565((uint16_t**)(output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
                                          transfer_RGB565_to_RGB888((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
                        } \
                        break; \
		case BC_BGR565: \
			switch(out_colormodel) \
			{ \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
                                          transfer_BGR565_to_BGR565((uint16_t**)(output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
                                          transfer_BGR565_to_RGB888((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
                        } \
                        break; \
		case BC_YUVA8888: \
			switch(out_colormodel) \
			{ \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
                                          transfer_YUVA8888_to_BGR565((uint16_t**)(output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_RGB565((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_BGR888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_BGR8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_RGB888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_RGBA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_YUVA8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_YUVA8888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_YUVA8888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_YUVA8888_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_YUVA8888_to_YUV422((output), \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_RGB888: \
			switch(out_colormodel) \
			{ \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_BGR565((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGB565((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_BGR888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGB888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_RGBA8888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616: \
					TRANSFER_FRAME_HEAD_16 \
					transfer_RGB888_to_RGB161616((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616: \
					TRANSFER_FRAME_HEAD_16 \
					transfer_RGB888_to_RGBA16161616((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_BGR8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_YUVA8888((output), (input));   \
                                        TRANSFER_FRAME_TAIL \
                                        break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGB888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVJ420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGB888_to_YUVJ420P_YUVJ422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB888_to_YUV422((output), (input), j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGB888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVJ422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGB888_to_YUVJ420P_YUVJ422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P16: \
					TRANSFER_YUV422P16_OUT_HEAD \
                                          transfer_RGB888_to_YUV422P16((uint16_t *)output_y, \
						(uint16_t *)output_u, \
						(uint16_t *)output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV411P: \
					TRANSFER_YUV411P_OUT_HEAD \
					transfer_RGB888_to_YUV411P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGB888_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVJ444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGB888_to_YUVJ444P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
                                case BC_YUV444P16:                        \
					TRANSFER_YUV444P_OUT_HEAD \
                                          transfer_RGB888_to_YUV444P16((uint16_t*)output_y, \
                                                                       (uint16_t*)output_u, \
                                                                       (uint16_t*)output_v, \
                                                                       (input), \
                                                                       j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_RGBA8888: \
			switch(out_colormodel) \
			{ \
				case BC_BGR565: \
						TRANSFER_FRAME_HEAD \
                                                  transfer_RGBA8888_to_BGR565((uint16_t**)(output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_RGB565((uint16_t**)(output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_BGR888((output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_RGB888((output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
						TRANSFER_FRAME_HEAD \
						transfer_RGBA8888_to_RGBA8888((output), (input)); \
						TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616: \
					TRANSFER_FRAME_HEAD_16 \
					transfer_RGBA8888_to_RGB161616((uint16_t**)(output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616: \
					TRANSFER_FRAME_HEAD_16 \
					transfer_RGBA8888_to_RGBA16161616((uint16_t**)(output), (input)); \
 					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
                                        TRANSFER_FRAME_HEAD                   \
                                        transfer_RGBA8888_to_BGR8888((output), (input)); \
                                        TRANSFER_FRAME_TAIL                   \
                                        break;                              \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA8888_to_YUVA8888((output), (input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGBA888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA888_to_YUV422((output), (input), j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGBA888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGBA888_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_RGB161616: \
			switch(out_colormodel) \
			{ \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_BGR565((uint16_t**)(output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_RGB565((uint16_t**)(output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_BGR888((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_BGR8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_RGB888((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_RGBA8888((output), (uint16_t*)(input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGB161616_to_YUVA8888((output), (uint16_t*)(input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGB161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGB161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGB161616_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_RGBA16161616: \
			switch(out_colormodel) \
			{ \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_BGR565((uint16_t**)(output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_RGB565((uint16_t**)(output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888:      \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_BGR888((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
				break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_BGR8888((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_RGB888((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_RGBA16161616_to_RGBA8888((output), (uint16_t*)(input)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_RGBA16161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_RGBA16161616_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_RGBA16161616_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(uint16_t*)(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_BGR8888: \
			switch(out_colormodel) \
			{ \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR8888_to_RGB888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR8888_to_BGR8888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
      				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_BGR888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_BGR888: \
			switch(out_colormodel) \
			{ \
				case BC_RGB888: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR888_to_RGB888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR565: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR888_to_BGR565((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR888_to_RGB565((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR888_to_BGR888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR888_to_RGBA8888((output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616: \
					TRANSFER_FRAME_HEAD_16 \
					transfer_BGR888_to_RGB161616((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616: \
					TRANSFER_FRAME_HEAD_16 \
					transfer_BGR888_to_RGBA16161616((uint16_t**)(output), (input));    \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR888_to_BGR8888((output), (input));   \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR888_to_YUVA8888((output), (input));   \
                                        TRANSFER_FRAME_TAIL \
                                        break; \
				case BC_YUV420P: \
					TRANSFER_YUV420P_OUT_HEAD \
					transfer_BGR888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422: \
					TRANSFER_FRAME_HEAD \
					transfer_BGR888_to_YUV422((output), (input), j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					TRANSFER_YUV422P_OUT_HEAD \
					transfer_BGR888_to_YUV420P_YUV422P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV444P: \
					TRANSFER_YUV444P_OUT_HEAD \
					transfer_BGR888_to_YUV444P(output_y, \
						output_u, \
						output_v, \
						(input), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
			}      \
			break; \
	} \
}




void cmodel_default(PERMUTATION_ARGS)
{
	if(scale)
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + column_table[j] * in_pixelsize,
			0,
			0,
			0,
			0);
	}
	else
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + j * in_pixelsize,
			0,
			0,
			0,
			0);
	}
}
