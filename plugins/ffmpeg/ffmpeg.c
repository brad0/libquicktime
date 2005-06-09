/* 
 * ffmpeg.c (C) Justin Schoeman 2002 (justin@suntiger.ee.up.ac.za)
 * Based entirely upon qtpng.c from libquicktime 
 * (http://libquicktime.sf.net).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA    
*/

#include "config.h"
#include "funcprotos.h"
#include <string.h>
#include <quicktime/colormodels.h>
#include "ffmpeg.h"


static int set_parameter_video(quicktime_t *file, 
                               int track, 
                               char *key, 
                               void *value)
{
	quicktime_ffmpeg_video_codec_t *codec = ((quicktime_codec_t*)file->vtracks[track].codec)->priv;
        //        fprintf(stderr, "set_parameter_video %s\n", key);
        
#define INTPARM(x, y) \
	if(!strcasecmp(key, #x)) { \
          codec->com.params.x = (*(int *)value) * y; \
          return 0; \
	}

#define FLOATPARM(x, y) \
	if(!strcasecmp(key, #x)) { \
          codec->com.params.x = (float)(*(int *)value) * y; \
          return 0; \
	}

#define FLAGPARM(x, y) \
        if(!strcasecmp(key, x)) { \
          if(*(int *)value == 1) \
            codec->com.params.flags |= y; \
          else \
            codec->com.params.flags &= ~y; \
          return 0; \
        }
        
        /********************
          General Options
        *********************/

	FLAGPARM("flags_gray", CODEC_FLAG_GRAY)
	INTPARM(strict_std_compliance, 1)

	if(!strcasecmp(key, "aspect_ratio_info")) {
		if(!strcasecmp((char *)value, "Square")) {
			codec->com.params.sample_aspect_ratio.num = 1;
			codec->com.params.sample_aspect_ratio.den = 1;
		} else if(!strcasecmp((char *)value, "4:3")) {
                        codec->com.params.sample_aspect_ratio.num = 4;
                        codec->com.params.sample_aspect_ratio.den = 3;
		} else if(!strcasecmp((char *)value, "16:9")) {
                        codec->com.params.sample_aspect_ratio.num = 16;
                        codec->com.params.sample_aspect_ratio.den = 9;
		}
        return 0;
	}

          
        /********************
          Bitrate options
        *********************/

        if(!strcasecmp(key, "bit_rate_video")) 
          { 
          codec->com.params.bit_rate = (*(int *)value) * 1000;
          return 0;
          }

        INTPARM(rc_min_rate, 1000) 
        INTPARM(rc_max_rate, 1000)
	INTPARM(bit_rate_tolerance, 1)
	FLOATPARM(qcompress, 0.01)
	FLOATPARM(qblur, 0.01)
        
        /********************
             VBR Options
        *********************/

        if(!strcasecmp(key, "qscale"))
          {
          if(*(int *)value)
            {
            codec->com.params.flags |= CODEC_FLAG_QSCALE;
            codec->qscale = *(int *)value;
            }
          else
            codec->com.params.flags &= ~CODEC_FLAG_QSCALE;
          return 0;
          }
        INTPARM(qmin, 1)
	INTPARM(qmax, 1)
        INTPARM(mb_qmin, 1)
	INTPARM(mb_qmax, 1)
	INTPARM(max_qdiff, 1)

        /************************
           Temporal compression
        *************************/
        
	INTPARM(gop_size, 1)

        if(!strcasecmp(key, "me_method")) {
          if(!strcasecmp((char *)value, "Zero")) {
            codec->com.params.me_method = ME_ZERO;
            } else if(!strcasecmp((char *)value, "Full")) {
            codec->com.params.me_method = ME_FULL;
            } else if(!strcasecmp((char *)value, "Log")) {
            codec->com.params.me_method = ME_LOG;
            } else if(!strcasecmp((char *)value, "Phods")) {
            codec->com.params.me_method = ME_PHODS;
            } else if(!strcasecmp((char *)value, "Epzs")) {
            codec->com.params.me_method = ME_EPZS;
            } else if(!strcasecmp((char *)value, "X1")) {
            codec->com.params.me_method = ME_X1;
            }
          return 0;
	}

        if(!strcasecmp(key, "mb_decision")) {
          if(!strcasecmp((char *)value, "Simple")) {
            codec->com.params.mb_decision = FF_MB_DECISION_SIMPLE;
            } else if(!strcasecmp((char *)value, "Fewest bits")) {
            codec->com.params.mb_decision = FF_MB_DECISION_BITS;
            } else if(!strcasecmp((char *)value, "Rate distoration")) {
            codec->com.params.mb_decision = FF_MB_DECISION_RD;
            }
          return 0;
        } 

        /********************
              MPEG-4
        *********************/

        FLAGPARM("flags_4mv", CODEC_FLAG_4MV)
        FLAGPARM("flags_part", CODEC_FLAG_PART)
          
        /*********************
                H263+
        **********************/

        FLAGPARM("flags_h263p_aic", CODEC_FLAG_H263P_AIC)
        FLAGPARM("flags_h263p_umv", CODEC_FLAG_H263P_UMV)
        
        /**********************
               Decoding
        ***********************/
                
	INTPARM(workaround_bugs, 1)
          //	INTPARM(luma_elim_threshold, 1)
          //	INTPARM(chroma_elim_threshold, 1)

        INTPARM(error_resilience, 1)
          //	if(!strcasecmp(key, "flags_pass")) {
          //		codec->com.params.flags &= ~(CODEC_FLAG_PASS1 | CODEC_FLAG_PASS2);
          //		if(*(int *)value == 1) {
          //			codec->com.params.flags |= CODEC_FLAG_PASS1;
          //		} else if(*(int *)value == 2) {
          //			codec->com.params.flags |= CODEC_FLAG_PASS1;
          //		}
          //	}
          else
            {
            //            fprintf(stderr, "Unknown key: %s\n", key);
		return -1;
            }
	return 0;
#undef INTPARM
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


void quicktime_init_video_codec_ffmpeg(quicktime_video_map_t *vtrack, AVCodec *encoder,
                                       AVCodec *decoder)
{
	quicktime_ffmpeg_video_codec_t *codec;

	char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;
        
	avcodec_init();

	codec = calloc(1, sizeof(quicktime_ffmpeg_video_codec_t));
	if(!codec)
	  return;

	if(quicktime_match_32(compressor, "dvc "))
	  codec->encode_colormodel = BC_YUV411P;
        //        else if(quicktime_match_32(compressor, "dvcp"))
        //          codec->encode_colormodel = BC_YUV411P;
        else
          codec->encode_colormodel = BC_YUV420P;
        
	codec->com.ffc_enc = encoder;
	codec->com.ffc_dec = decoder;
        
	((quicktime_codec_t*)vtrack->codec)->priv = (void *)codec;
	((quicktime_codec_t*)vtrack->codec)->delete_vcodec = lqt_ffmpeg_delete_video;

        if(encoder)
          ((quicktime_codec_t*)vtrack->codec)->encode_video = lqt_ffmpeg_encode_video;
	if(decoder)
          ((quicktime_codec_t*)vtrack->codec)->decode_video = lqt_ffmpeg_decode_video;

        ((quicktime_codec_t*)vtrack->codec)->set_parameter = set_parameter_video;
	((quicktime_codec_t*)vtrack->codec)->reads_colormodel = reads_colormodel;
	((quicktime_codec_t*)vtrack->codec)->writes_colormodel = writes_colormodel;
}

