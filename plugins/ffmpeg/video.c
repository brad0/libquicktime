/* 
   ffmpeg.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
   Based entirely upon qtpng.c from libquicktime 
   (http://libquicktime.sf.net).

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
    
*/


#include <quicktime/colormodels.h>
#include <funcprotos.h>
#include "ffmpeg.h"

static int read_video_frame(quicktime_t *file, quicktime_ffmpeg_video_codec_t *codec,
                     int64_t frameno, int track)
{
	int i;
	
	quicktime_set_video_position(file, frameno, track);
	i = quicktime_frame_size(file, frameno, track);
	if(i > codec->read_buffer_size)
	{
		free(codec->read_buffer);
		codec->read_buffer = malloc(i + 1024);
		if(!codec->read_buffer)
			return -1;
		codec->read_buffer_size = i + 1024;
	}
	if(quicktime_read_data(file, codec->read_buffer, i) < 0)
		return -1;
	return i;
}


int lqt_ffmpeg_decode_video(quicktime_t *file, unsigned char **row_pointers,
                                   int track)
{
	int result = 0;
	int t;
	int buffer_size;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
	quicktime_ffmpeg_video_codec_t *codec =
          ((quicktime_codec_t*)vtrack->codec)->priv;
	int got_pic;
	AVPicture pic_dec;
	AVPicture pic_out;
	int ofmt = PIX_FMT_YUV420P;
	unsigned char *dp, *sp;

	if(!codec->com.init_dec) {
		switch(file->color_model) {
			case BC_YUV420P: ofmt = PIX_FMT_YUV420P; break;
//			case BC_YUV422: ofmt = PIX_FMT_YUV422; break;
//			case BC_RGB888: ofmt = PIX_FMT_RGB24; break;
//			case BC_BGR888: ofmt = PIX_FMT_BGR24; break;
			case BC_YUV422P: ofmt = PIX_FMT_YUV422P; break;
			default:
				fprintf(stderr, "unsupported pixel format!\n");
				return -1;
		}
		codec->com.ffcodec_dec.width = file->in_w;
		codec->com.ffcodec_dec.height = file->in_h;
#define SP(x) codec->com.ffcodec_dec.x = codec->com.params.x
		SP(workaround_bugs);
		SP(error_resilience);
		SP(flags);
#undef SP
		if(avcodec_open(&codec->com.ffcodec_dec, codec->com.ffc_dec) != 0)
			return -1;
		codec->com.init_dec = 1;
	}
	
	if((quicktime_has_keyframes(file, track)) &&
	   (vtrack->current_position != codec->last_frame + 1)) {
		int frame1, frame2 = vtrack->current_position;
		frame1 = quicktime_get_keyframe_before(file, vtrack->current_position, track);
		if((frame1 < codec->last_frame) &&
		   (frame2 > codec->last_frame))
			 frame1 = codec->last_frame + 1;
		while(frame1 < frame2) {
			buffer_size = read_video_frame(file, codec, frame1, track);
			if(buffer_size > 0) {
				avcodec_decode_video(&codec->com.ffcodec_dec,
					&pic_dec,
					&got_pic,
					codec->read_buffer,
					buffer_size);
			}
			frame1++;
		}
		vtrack->current_position = frame2;
	}
	codec->last_frame = vtrack->current_position;
	buffer_size = read_video_frame(file, codec,
                                       vtrack->current_position, track);
	if(buffer_size > 0) {
		if(!result) {
			t = avcodec_decode_video(&codec->com.ffcodec_dec,
				&pic_dec,
				&got_pic,
				codec->read_buffer,
				buffer_size);
			if(t < 0) {
				fprintf(stderr, "error decoding frame\n");
				return 0;
			}
			if(got_pic) {
				pic_out.data[0] = row_pointers[0];
				pic_out.data[1] = row_pointers[1];
				pic_out.data[2] = row_pointers[2];
				pic_out.linesize[0] = width;
				pic_out.linesize[1] = width/2;
				pic_out.linesize[2] = width/2;
				img_convert(&pic_out,
					ofmt,
					&pic_dec,
					codec->com.ffcodec_dec.pix_fmt,
					width,
					height);
			}
		}
	}
	return result;
}

int lqt_ffmpeg_encode_video(quicktime_t *file, unsigned char **row_pointers,
                        int track)
{
	int64_t offset = quicktime_position(file);
	int result = 0;
	int i = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_ffmpeg_video_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
	AVPicture pic;
        quicktime_atom_t chunk_atom;
	if(!codec->com.init_enc) {
		codec->com.ffcodec_enc.frame_rate =
                  (int)(quicktime_frame_rate(file, track) * (float)FRAME_RATE_BASE);
		codec->com.ffcodec_enc.width = width;
		codec->com.ffcodec_enc.height = height;
#define SP(x) codec->com.ffcodec_enc.x = codec->com.params.x
		SP(bit_rate);
		SP(bit_rate_tolerance);
		SP(gop_size);
		SP(quality);
		SP(qcompress);
		SP(qblur);
		SP(qmin);
		SP(qmax);
		SP(max_qdiff);
		SP(max_b_frames);
		SP(b_quant_factor);
		SP(b_quant_offset);
		SP(rc_strategy);
		SP(b_frame_strategy);
		SP(rtp_payload_size);
		SP(workaround_bugs);
		SP(luma_elim_threshold);
		SP(chroma_elim_threshold);
		SP(strict_std_compliance);
		SP(error_resilience);
		SP(flags);
		SP(me_method);
		SP(aspect_ratio_info);		
#undef SP
		if(avcodec_open(&codec->com.ffcodec_enc, codec->com.ffc_enc) != 0)
			return -1;
		codec->com.init_enc = 1;
		if(file->color_model != BC_YUV420P) {
			codec->encode_frame = malloc(width * height * 3 / 2);
			if(!codec->encode_frame)
				return -1;
		}
		codec->write_buffer_size = width * height * 4;
		codec->write_buffer = malloc(codec->write_buffer_size);
		if(!codec->write_buffer)
			return -1;
	}

	if(file->color_model != BC_YUV420P) {
		pic.data[0] = codec->encode_frame;
		pic.data[1] = pic.data[0] + width * height;
		pic.data[2] = pic.data[1] + (width * height) / 4;
		pic.linesize[0] = width;
		pic.linesize[1] = width / 2;
		pic.linesize[2] = width / 2;
		cmodel_transfer(pic.data, 
			row_pointers,
			pic.data[0],
			pic.data[1],
			pic.data[2],
			row_pointers[0],
			row_pointers[1],
			row_pointers[2],
			0, 
			0, 
			width, 
			height,
			0, 
			0, 
			width, 
			height,
			file->color_model,
			BC_YUV420P, 
			0,
			width,
			width);
	} else {
		pic.data[0] = row_pointers[0];
		pic.data[1] = row_pointers[1];
		pic.data[2] = row_pointers[2];
		pic.linesize[0] = width;
		pic.linesize[1] = width / 2;
		pic.linesize[2] = width / 2;
	}

	i = avcodec_encode_video(&codec->com.ffcodec_enc,
		codec->write_buffer,
		codec->write_buffer_size,
		&pic);

        quicktime_write_chunk_header(file, trak, &chunk_atom);
        
	result = !quicktime_write_data(file, 
			codec->write_buffer, 
			i);
        quicktime_write_chunk_footer(file, 
                                     trak, 
                                     file->vtracks[track].current_chunk,
                                     &chunk_atom, 
                                     1);

	file->vtracks[track].current_chunk++;
	if(codec->com.ffcodec_enc.key_frame)
		quicktime_insert_keyframe(file, file->vtracks[track].current_position, track);
	return result;
}
