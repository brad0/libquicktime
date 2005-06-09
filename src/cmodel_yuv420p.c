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
#include <cmodel_permutation.h>


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
		case BC_YUV420P: \
			switch(out_colormodel) \
			{ \
				case BC_BGR565: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV422P_to_BGR565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV422P_to_RGB565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV422P_to_BGR888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV422P_to_BGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = output_rows[0] + i * out_rowspan; \
						unsigned char *output_u = output_rows[1] + i / 2 * out_rowspan_uv; \
						unsigned char *output_v = output_rows[2] + i / 2 * out_rowspan_uv; \
						unsigned char *input_y = input_rows[0] + row_table[i] * in_rowspan; \
						unsigned char *input_u = input_rows[1] + row_table[i] / 2 * in_rowspan_uv; \
						unsigned char *input_v = input_rows[2] + row_table[i] / 2 * in_rowspan_uv; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV422P_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV422: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV422P_to_YUV422((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = output_rows[0] + i * out_rowspan; \
						unsigned char *output_u = output_rows[1] + i * out_rowspan_uv; \
						unsigned char *output_v = output_rows[2] + i * out_rowspan_uv; \
						unsigned char *input_y = input_rows[0] + row_table[i] * in_rowspan; \
						unsigned char *input_u = input_rows[1] + row_table[i] / 2 * in_rowspan_uv; \
						unsigned char *input_v = input_rows[2] + row_table[i] / 2 * in_rowspan_uv; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV422P_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV444P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = output_rows[0] + i * out_rowspan; \
						unsigned char *output_u = output_rows[1] + i * out_rowspan_uv; \
						unsigned char *output_v = output_rows[2] + i * out_rowspan_uv; \
						unsigned char *input_y = input_rows[0] + row_table[i] * in_rowspan; \
						unsigned char *input_u = input_rows[1] + row_table[i] / 2 * in_rowspan_uv; \
						unsigned char *input_v = input_rows[2] + row_table[i] / 2 * in_rowspan_uv; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV422P_to_YUV444P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_RGB888:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV422P_to_RGB888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888:      \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV422P_to_RGBA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616:      \
					TRANSFER_YUV420P_IN_HEAD_16 \
                                        transfer_YUV422P_to_RGB161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616:      \
					TRANSFER_YUV420P_IN_HEAD_16 \
                                                transfer_YUV422P_to_RGBA16161616((output), \
                                                input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV422P_to_YUV888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_YUV420P_IN_HEAD \
					transfer_YUV422P_to_YUVA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_YUV420P_IN_HEAD_16 \
                                        transfer_YUV422P_to_YUV161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_YUV420P_IN_HEAD_16 \
					transfer_YUV422P_to_YUVA16161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
			} \
			break; \
 \
		case BC_YUV422P: \
			switch(out_colormodel) \
			{ \
				case BC_BGR565: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV422P_to_BGR565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV422P_to_RGB565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV422P_to_BGR888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV422P_to_BGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV422P_to_RGB888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888:      \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV422P_to_RGBA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616:      \
					TRANSFER_YUV422P_IN_HEAD_16 \
					transfer_YUV422P_to_RGB161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616:      \
					TRANSFER_YUV422P_IN_HEAD_16 \
					transfer_YUV422P_to_RGBA16161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV422P_to_YUV888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_YUV422P_IN_HEAD \
					transfer_YUV422P_to_YUVA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_YUV422P_IN_HEAD_16 \
                                          transfer_YUV422P_to_YUV161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_YUV422P_IN_HEAD_16 \
					transfer_YUV422P_to_YUVA16161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = output_rows[0] + i * out_rowspan; \
						unsigned char *output_u = output_rows[1] + i / 2 * out_rowspan_uv; \
						unsigned char *output_v = output_rows[2] + i / 2 * out_rowspan_uv; \
						unsigned char *input_y = input_rows[0] + row_table[i] * in_rowspan; \
						unsigned char *input_u = input_rows[1] + row_table[i] * in_rowspan_uv; \
						unsigned char *input_v = input_rows[2] + row_table[i] * in_rowspan_uv; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV422P_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV422: \
					TRANSFER_YUV422_IN_HEAD \
					transfer_YUV422P_to_YUV422((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = output_rows[0] + i * out_rowspan; \
						unsigned char *output_u = output_rows[1] + i * out_rowspan_uv; \
						unsigned char *output_v = output_rows[2] + i * out_rowspan_uv; \
						unsigned char *input_y = input_rows[0] + row_table[i] * in_rowspan; \
						unsigned char *input_u = input_rows[1] + row_table[i] * in_rowspan_uv; \
						unsigned char *input_v = input_rows[2] + row_table[i] * in_rowspan_uv; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV422P_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV444P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = output_rows[0] + i * out_rowspan; \
						unsigned char *output_u = output_rows[1] + i * out_rowspan_uv; \
						unsigned char *output_v = output_rows[2] + i * out_rowspan_uv; \
						unsigned char *input_y = input_rows[0] + row_table[i] * in_rowspan; \
						unsigned char *input_u = input_rows[1] + row_table[i] * in_rowspan_uv; \
						unsigned char *input_v = input_rows[2] + row_table[i] * in_rowspan_uv; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV422P_to_YUV444P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
			} \
			break; \
 \
 \
		case BC_YUV444P: \
			switch(out_colormodel) \
			{ \
				case BC_BGR565: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV422P_to_BGR565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB565: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV422P_to_RGB565((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR888:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV422P_to_BGR888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_BGR8888: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV422P_to_BGR8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB888:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV422P_to_RGB888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA8888:      \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV422P_to_RGBA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGB161616:      \
					TRANSFER_YUV444P_IN_HEAD_16 \
					transfer_YUV422P_to_RGB161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_RGBA16161616:      \
					TRANSFER_YUV444P_IN_HEAD_16 \
					transfer_YUV422P_to_RGBA16161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV888: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV422P_to_YUV888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA8888: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV422P_to_YUVA8888((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV161616: \
					TRANSFER_YUV444P_IN_HEAD_16 \
					transfer_YUV422P_to_YUV161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUVA16161616: \
					TRANSFER_YUV444P_IN_HEAD_16 \
					transfer_YUV422P_to_YUVA16161616((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset)); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV420P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = output_rows[0] + i * out_rowspan; \
						unsigned char *output_u = output_rows[1] + i / 2 * out_rowspan_uv; \
						unsigned char *output_v = output_rows[2] + i / 2 * out_rowspan_uv; \
						unsigned char *input_y = input_rows[0] + row_table[i] * in_rowspan; \
						unsigned char *input_u = input_rows[1] + row_table[i] * in_rowspan_uv; \
						unsigned char *input_v = input_rows[2] + row_table[i] * in_rowspan_uv; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV422P_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV422: \
					TRANSFER_YUV444P_IN_HEAD \
					transfer_YUV422P_to_YUV422((output), \
						input_y + (y_in_offset), \
						input_u + (u_in_offset), \
						input_v + (v_in_offset), \
						j); \
					TRANSFER_FRAME_TAIL \
					break; \
				case BC_YUV422P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = output_rows[0] + i * in_rowspan; \
						unsigned char *output_u = output_rows[1] + i * in_rowspan_uv; \
						unsigned char *output_v = output_rows[2] + i * in_rowspan_uv; \
						unsigned char *input_y = input_rows[0] + row_table[i] * out_rowspan; \
						unsigned char *input_u = input_rows[1] + row_table[i] * out_rowspan_uv; \
						unsigned char *input_v = input_rows[2] + row_table[i] * out_rowspan_uv; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV422P_to_YUV420P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
				case BC_YUV444P: \
					for(i = 0; i < out_h; i++) \
					{ \
						unsigned char *output_y = output_rows[0] + i * out_rowspan; \
						unsigned char *output_u = output_rows[1] + i * out_rowspan_uv; \
						unsigned char *output_v = output_rows[2] + i * out_rowspan_uv; \
						unsigned char *input_y = input_rows[0] + row_table[i] * in_rowspan; \
						unsigned char *input_u = input_rows[1] + row_table[i] * in_rowspan_uv; \
						unsigned char *input_v = input_rows[2] + row_table[i] * in_rowspan_uv; \
						for(j = 0; j < out_w; j++) \
						{ \
							transfer_YUV444P_to_YUV444P(input_y + (y_in_offset), \
								input_u + (u_in_offset), \
								input_v + (v_in_offset), \
								output_y, \
								output_u, \
								output_v, \
								j); \
						} \
					} \
					break; \
			} \
			break; \
	} \
}

void cmodel_yuv420p(PERMUTATION_ARGS)
{
	if(scale)
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + column_table[j] * in_pixelsize,
			column_table[j],
			column_table[j] / 2,
			column_table[j] / 2,
			0);
	}
	else
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + j * in_pixelsize,
			j,
			j / 2,
			j / 2,
			0);
	}
}

void cmodel_yuv444p(PERMUTATION_ARGS)
{
	if(scale)
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + column_table[j] * in_pixelsize,
			column_table[j],
			column_table[j],
			column_table[j],
			0);
	}
	else
	{
		TRANSFER_FRAME_DEFAULT(&output_row, 
			input_row + j * in_pixelsize,
			j,
			j,
			j,
			0);
	}
}
