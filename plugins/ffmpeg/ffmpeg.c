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

#include <string.h>
#include <quicktime/colormodels.h>
#include <funcprotos.h>
#include "ffmpeg.h"

static int delete_codec(quicktime_video_map_t *vtrack)
{
	quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	
	if(codec->init_enc)
		avcodec_close(&codec->ffcodec_enc);
	if(codec->init_dec)
		avcodec_close(&codec->ffcodec_dec);

	if(codec->encode_frame) free(codec->encode_frame);
	if(codec->write_buffer) free(codec->write_buffer);

	if(codec->read_buffer) free(codec->read_buffer);
	
	if(codec->ae_buf) free(codec->ae_buf);
	if(codec->ad_buf) free(codec->ad_buf);
	return 0;
}

int read_video_frame(quicktime_t *file, quicktime_ffmpeg_codec_t *codec, longest frameno, int track)
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

static int decode_video(quicktime_t *file, unsigned char **row_pointers, int track)
{
	int result = 0;
	int t;
	int buffer_size;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
	quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int got_pic;
	AVPicture pic_dec;
	AVPicture pic_out;
	int ofmt = PIX_FMT_YUV420P;
	unsigned char *dp, *sp;

	if(!codec->init_dec) {
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
		codec->ffcodec_dec.width = file->in_w;
		codec->ffcodec_dec.height = file->in_h;
#define SP(x) codec->ffcodec_dec.x = codec->params.x
		SP(workaround_bugs);
		SP(error_resilience);
		SP(flags);
#undef SP
		if(avcodec_open(&codec->ffcodec_dec, codec->ffc_dec) != 0)
			return -1;
		codec->init_dec = 1;
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
				avcodec_decode_video(&codec->ffcodec_dec,
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
	buffer_size = read_video_frame(file, codec, vtrack->current_position, track);
	if(buffer_size > 0) {
		if(!result) {
			t = avcodec_decode_video(&codec->ffcodec_dec,
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
					codec->ffcodec_dec.pix_fmt,
					width,
					height);
			}
		}
	}
	return result;
}

static int encode_video(quicktime_t *file, unsigned char **row_pointers, int track)
{
	longest offset = quicktime_position(file);
	int result = 0;
	int i = 0;
	quicktime_video_map_t *vtrack = &(file->vtracks[track]);
	quicktime_trak_t *trak = vtrack->track;
	quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)vtrack->codec)->priv;
	int height = trak->tkhd.track_height;
	int width = trak->tkhd.track_width;
	AVPicture pic;

	if(!codec->init_enc) {
		codec->ffcodec_enc.frame_rate = (int)(quicktime_frame_rate(file, track) * (float)FRAME_RATE_BASE);
		codec->ffcodec_enc.width = width;
		codec->ffcodec_enc.height = height;
#define SP(x) codec->ffcodec_enc.x = codec->params.x
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
		if(avcodec_open(&codec->ffcodec_enc, codec->ffc_enc) != 0)
			return -1;
		codec->init_enc = 1;
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

	i = avcodec_encode_video(&codec->ffcodec_enc,
		codec->write_buffer,
		codec->write_buffer_size,
		&pic);

	result = !quicktime_write_data(file, 
			codec->write_buffer, 
			i);

	quicktime_update_tables(file,
		file->vtracks[track].track,
		offset,
		file->vtracks[track].current_chunk,
		file->vtracks[track].current_position,
		1,
		i);

	file->vtracks[track].current_chunk++;
	if(codec->ffcodec_enc.key_frame)
		quicktime_insert_keyframe(file, file->vtracks[track].current_position, track);
	return result;
}

static int set_parameter(quicktime_t *file, 
                         int track, 
                         char *key, 
                         void *value)
{
	quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;

#define INTPARM(x, y) \
	if(!strcasecmp(key, #x)) { \
		codec->params.x = (*(int *)value) * y; \
	}
	
	INTPARM(bit_rate, 1000) else 
	INTPARM(bit_rate_tolerance, 1) else
	INTPARM(gop_size, 1) else
	INTPARM(quality, 1) else
	INTPARM(qcompress, 0.01) else
	INTPARM(qblur, 0.01) else
	INTPARM(qmin, 1) else
	INTPARM(qmax, 1) else
	INTPARM(max_qdiff, 1) else
	INTPARM(max_b_frames, 1) else
	INTPARM(b_quant_factor, 1) else
	INTPARM(b_quant_offset, 1) else
	INTPARM(rc_strategy, 1) else
	INTPARM(b_frame_strategy, 1) else
	INTPARM(rtp_payload_size, 1) else
	INTPARM(workaround_bugs, 1) else
	INTPARM(luma_elim_threshold, 1) else
	INTPARM(chroma_elim_threshold, 1) else
	INTPARM(strict_std_compliance, 1) else
	INTPARM(error_resilience, 1) else
	if(!strcasecmp(key, "flags_hq")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_HQ;
		else
			codec->params.flags &= ~CODEC_FLAG_HQ;
	} else
	if(!strcasecmp(key, "flags_4mv")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_4MV;
		else
			codec->params.flags &= ~CODEC_FLAG_4MV;
	} else
	if(!strcasecmp(key, "flags_part")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_PART;
		else
			codec->params.flags &= ~CODEC_FLAG_PART;
	} else
	if(!strcasecmp(key, "flags_gray")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_GRAY;
		else
			codec->params.flags &= ~CODEC_FLAG_GRAY;
	} else
	if(!strcasecmp(key, "flags_fix")) {
		if(*(int *)value == 1)
			codec->params.flags |= CODEC_FLAG_QSCALE;
		else
			codec->params.flags &= ~CODEC_FLAG_QSCALE;
	} else
	if(!strcasecmp(key, "flags_pass")) {
		codec->params.flags &= ~(CODEC_FLAG_PASS1 | CODEC_FLAG_PASS2);
		if(*(int *)value == 1) {
			codec->params.flags |= CODEC_FLAG_PASS1;
		} else if(*(int *)value == 2) {
			codec->params.flags |= CODEC_FLAG_PASS1;
		}
	} else
	if(!strcasecmp(key, "me_method")) {
		if(!strcasecmp((char *)value, "Zero")) {
			codec->params.me_method = ME_ZERO;
		} else if(!strcasecmp((char *)value, "Full")) {
			codec->params.me_method = ME_FULL;
		} else if(!strcasecmp((char *)value, "Log")) {
			codec->params.me_method = ME_LOG;
		} else if(!strcasecmp((char *)value, "Phods")) {
			codec->params.me_method = ME_PHODS;
		} else if(!strcasecmp((char *)value, "Epzs")) {
			codec->params.me_method = ME_EPZS;
		} else if(!strcasecmp((char *)value, "X1")) {
			codec->params.me_method = ME_X1;
		}
	} else
	if(!strcasecmp(key, "aspect_ratio_info")) {
		if(!strcasecmp((char *)value, "Square")) {
			codec->params.aspect_ratio_info = FF_ASPECT_SQUARE;
		} else if(!strcasecmp((char *)value, "4:3 (625)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_4_3_625;
		} else if(!strcasecmp((char *)value, "4:3 (525)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_4_3_525;
		} else if(!strcasecmp((char *)value, "16:9 (625)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_16_9_625;
		} else if(!strcasecmp((char *)value, "16:9 (525)")) {
			codec->params.aspect_ratio_info = FF_ASPECT_16_9_525;
		}
	} else
	{
		fprintf(stderr, "Unknown key: %s\n", key);
		return -1;
	}
	return 0;
}

static int reads_colormodel(quicktime_t *file, 
                            int colormodel, 
                            int track)
{
	return (colormodel == BC_YUV420P);
}

static int writes_colormodel(quicktime_t *file, 
                             int colormodel, 
                             int track)
{
	return (colormodel == BC_YUV420P);
}

/*
 Damned if I know how to handle sample-granular access.  There is no 
 guaranteed 1:1 mapping between compressed and uncompressed samples.
 
 Linear access is easy, but the moment someone seeks, how can we possibly
 find  where to start again?
*/

static int decode_audio(quicktime_t *file, int16_t *output_i, float *output_f, long samples, int track, int channel)
{
#if 0
	int result = -1;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	int channels = file->atracks[track].channels;
	int ad_length;
	int t, i, j, n;
	
	if(!codec->init_dec) {
#define SP(x) codec->ffcodec_dec.x = codec->params.x
		SP(workaround_bugs);
		SP(error_resilience);
		SP(flags);
#undef SP
		if(avcodec_open(&codec->ffcodec_dec, codec->ffc_dec) != 0)
			return -1;
		codec->ad_buf = malloc(AVCODEC_MAX_AUDIO_FRAME_SIZE);
		if(!codec->ad_buf)
			return -1;
		codec->read_buffer = malloc(codec->ffcodec_dec.frame_size * 2 * channels);
		if(!codec->read_buffer)
			return -1;
		codec->init_dec = 1;
	}

	j = 0;
	while(samples) {
		if(codec->ad_pos) {
			n = (samples>codec->ad_pos)?codec->ad_pos:samples;
			if(output_f) {
				for(i = channel; i < n; i += channels, j++)
					output_f[j] = (float)(codec->ad_buf[i]) / 16384.0;
			} else if(output_i) {
				for(i = channel; i < n; i += channels, j++)
					output_i[j] = codec->ad_buf[i];
			}
		}
		n = codec->ad_pos;
		
		if(DEC_BUFFER
	}
	result = !quicktime_read_data(file, codec->read_buffer, buffer_size);
	if((buffer_size > 0) && (!result)) {
		ad_length = DEC_BUFFER;
		t = avcodec_decode_audio(&codec->ffcodec_dec,
			codec->ad_buf,
			&ad_length,
			codec->read_buffer,
			buffer_size);
		if(t < 0) {
			fprintf(stderr, "error decoding frame\n");
			return -1;
		}
		result = t / channels;
		if(result > samples)
			t = samples * channels;
		if(output_f) {
			for(i = channel, j = 0; i < t; i += channels, j++)
				output_f[j] = (float)(codec->ad_buf[i]) / 16384.0;
		}
		if(output_i) {
			for(i = channel, j = 0; i < t; i += channels, j++)
				output_i[j] = codec->ad_buf[i];
		}
	}
	return result;
#else
	return 0;
#endif
}

/* Fill ae_buf with up to samples of data */
void fill_buffer(quicktime_ffmpeg_codec_t *codec, int16_t **input_i, float **input_f, int samples, int *spos, int channels)
{
	int aemax = codec->ffcodec_dec.frame_size * channels;
	channels--; /* 0 or 1 */
	if(input_i) {
		while((codec->ae_pos < aemax) && (*spos < samples)) {
			codec->ae_buf[codec->ae_pos++] = 
				input_i[*spos & channels][*spos >> channels];
			*spos++;
		}
	} else if(input_f) {
		while((codec->ae_pos < aemax) && (*spos < samples)) {
			codec->ae_buf[codec->ae_pos++] = 
				(short)(input_f[*spos & channels][*spos >> channels] * 16383.0);
			*spos++;
		}
	}
}

/*
 Untested, but it should work...   
*/

static int encode_audio(quicktime_t *file, int16_t **input_i, float **input_f, int track, long samples)
{
	int result = -1;
	quicktime_audio_map_t *track_map = &(file->atracks[track]);
	quicktime_ffmpeg_codec_t *codec = ((quicktime_codec_t*)track_map->codec)->priv;
	quicktime_trak_t *trak = track_map->track;
	int samplerate = trak->mdia.minf.stbl.stsd.table[0].sample_rate;
	int channels = file->atracks[track].channels;
	longest offset;
	int size = 0;
	int spos = 0;
	samples *= channels; /* shorts */
	
	if(!codec->init_enc) {
		codec->ffcodec_enc.sample_rate = samplerate;
		codec->ffcodec_enc.channels = channels;
#define SP(x) codec->ffcodec_enc.x = codec->params.x
		SP(bit_rate);
		SP(bit_rate_tolerance);
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
		SP(strict_std_compliance);
		SP(error_resilience);
		SP(flags);
#undef SP
		if(avcodec_open(&codec->ffcodec_enc, codec->ffc_enc) != 0)
			return -1;
		codec->init_enc = 1;
		codec->write_buffer_size = codec->ffcodec_enc.frame_size * 2 * channels * 2;
		codec->write_buffer = malloc(codec->write_buffer_size);
		if(!codec->write_buffer)
			return -1;
		codec->ae_buf = malloc(codec->ffcodec_enc.frame_size * 2 * channels);
		if(!codec->ae_buf)
			return -1;
	}
	/* While there is still data to encode... */
	while(spos < samples) {
		/* Fill codec->ae_buf */
		fill_buffer(codec, input_i, input_f, samples, &spos, channels);
		/* If buffer is full, write a chunk. */
		if(codec->ae_pos == (codec->ffcodec_enc.frame_size * channels)) {
			size = avcodec_encode_audio(&codec->ffcodec_enc,
				codec->write_buffer,
				codec->write_buffer_size,
				codec->ae_buf);
			if(size > 0) {
				offset = quicktime_position(file);
				result = quicktime_write_data(file, codec->write_buffer, size);
				result = (result)?0:1;
				quicktime_update_tables(file, 
					track_map->track,
					offset,
					track_map->current_chunk,
					track_map->current_position,
					size,
					0);
				file->atracks[track].current_chunk++;
			}
		}
	}
	return result;
}

void quicktime_init_codec_ffmpeg(quicktime_video_map_t *vtrack, AVCodec *encoder, AVCodec *decoder)
{
	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
	quicktime_ffmpeg_codec_t *codec;

	avcodec_init();

	codec = calloc(1, sizeof(quicktime_ffmpeg_codec_t));
	if(!codec)
		return;
	memset(codec, 0, sizeof(quicktime_ffmpeg_codec_t));

	codec->ffc_enc = encoder;
	codec->ffc_dec = decoder;
	
	((quicktime_codec_t*)vtrack->codec)->priv = (void *)codec;
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = delete_codec;
	if((encoder) && (encoder->type == CODEC_TYPE_VIDEO))
		((quicktime_codec_t*)vtrack->codec)->encode_video = encode_video;
	if((encoder) && (encoder->type == CODEC_TYPE_AUDIO))
		((quicktime_codec_t*)vtrack->codec)->encode_audio = encode_audio;
	if((decoder) && (decoder->type == CODEC_TYPE_VIDEO))
		((quicktime_codec_t*)vtrack->codec)->decode_video = decode_video;
	if((decoder) && (decoder->type == CODEC_TYPE_AUDIO))
		((quicktime_codec_t*)vtrack->codec)->decode_audio = decode_audio;
	((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = reads_colormodel;
	((quicktime_codec_t*)vtrack->codec)->writes_colormodel = writes_colormodel;
}
