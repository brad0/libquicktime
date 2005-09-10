#ifndef _LQT_H_
#define _LQT_H_

#include "quicktime.h"
#include "lqt_codecinfo.h"
#include "lqt_qtvr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void *lqt_bufalloc(size_t size);

int lqt_fileno(quicktime_t *file);

/* Call quicktime_set_parameter with our codec info */

void lqt_set_audio_parameter(quicktime_t *file,int stream, char *key,void *value);
void lqt_set_video_parameter(quicktime_t *file,int stream, char *key,void *value);

int lqt_set_fiel(quicktime_t *, int, int, int);
int lqt_get_fiel(quicktime_t *, int, int *, int *);
  
int lqt_get_pixel_aspect(quicktime_t *file, int track, int * pixel_width,
                         int * pixel_height);
int lqt_set_pixel_aspect(quicktime_t *file, int track, int pixel_width,
                         int pixel_height);


lqt_interlace_mode_t lqt_get_interlace_mode(quicktime_t * file, int track);
const char * lqt_interlace_mode_to_string(lqt_interlace_mode_t);
  

  
lqt_chroma_placement_t lqt_get_chroma_placement(quicktime_t * file, int track);
const char * lqt_chroma_placement_to_string(lqt_chroma_placement_t);

  
int lqt_get_codec_api_version();

/***********************************************
 * Advanced colormodel handling.
 * (defined in lqt_color.c)
 ***********************************************/

/*
 *  For initialization of colormodel arrays: This must be something
 *  NOT defined in colormodels.h
 */

#define LQT_COLORMODEL_NONE -1

/* Colormodel <-> string conversion (used by registry file routines) */

const char * lqt_colormodel_to_string(int colormodel);

int lqt_string_to_colormodel(const char * str);

/* Query information about the colormodel */
  
int lqt_colormodel_is_planar(int colormodel);
int lqt_colormodel_has_alpha(int colormodel);
int lqt_colormodel_is_rgb(int colormodel);
int lqt_colormodel_is_yuv(int colormodel);
void lqt_colormodel_get_chroma_sub(int colormodel, int * sub_h, int * sub_v);
void lqt_get_default_rowspan(int colormodel, int width, int * rowspan, int * rowspan_uv);

int lqt_colormodel_has_conversion(int in_cmodel, int out_cmodel);
  
  
/* Query supported colormodels */

int lqt_num_colormodels();

const char * lqt_get_colormodel_string(int index);

int lqt_get_colormodel(int index);

/*
 *  Return decoder colormodel
 *  This function can fail in some pathological cases,
 *  it returns LQT_COLORMODEL_NONE then
 */

int lqt_get_decoder_colormodel(quicktime_t * file, int track);

/*
 *   Convenience function for application developers:
 *   It takes an array with supported colormodels
 *   (Terminated with LQT_COLORMODEL_NONE) and returns
 *   the best colormodel
 *   Works for en- and decoding
 */

int lqt_get_best_colormodel(quicktime_t * file, int track, int * supported);

/*
 *  Get the colormodel, which will be valid for the next decode() call.
 *  By default, it will return the colormodel, which is read/written natively
 *  by the codec
 */
  
int lqt_get_cmodel(quicktime_t * file, int track);
  
/*
 *  Allocate and free row_pointers for use with libquicktime
 *  Rowspan can be <= 0, in this case it's set from width, and
 *  the values will be updated with the row_span actually used
 */

  
uint8_t ** lqt_rows_alloc(int width, int height, int colormodel, int * rowspan, int * rowspan_uv);

void lqt_rows_copy(uint8_t **out_rows, uint8_t **in_rows, int width, int height, int in_rowspan, int in_rowspan_uv,
                   int out_rowspan, int out_rowspan_uv, int colormodel);
  
  
void lqt_rows_free(uint8_t **);
  

/**************************************
 * Set streams for encoding
 **************************************/
  
/*
 *   Versions of quicktime_set_audio and quicktime_set_video,
 *   which take codec infos as arguments
 */

int lqt_set_video(quicktime_t *file, int tracks, 
                  int frame_w, int frame_h,
                  int frame_duration, int timescale,
                  lqt_codec_info_t * info);

int lqt_set_audio(quicktime_t *file, int channels,
                  long sample_rate,  int bits,
                  lqt_codec_info_t * codec_info);


/*
 *  Add streams. These allow different formats in the streams
 */

int lqt_add_audio_track(quicktime_t *file,
                        int channels, long sample_rate, int bits, lqt_codec_info_t * info);

int lqt_add_video_track(quicktime_t *file,
                        int frame_w, int frame_h,
                        int frame_duration, int timescale,
                        lqt_codec_info_t * info);
  
  
/*
 *  Same as quicktime_decode_video but doesn't force BC_RGB888
 */

int lqt_decode_video(quicktime_t *file,
                     unsigned char **row_pointers, int track);

/*
 *  Encode a video frame with a specified timestamp
 *  Timestamp unit is timescale tics. Timestamp should start with zero.
 *  WARNING: AVI files don't support arbitrary timestamps. For AVI files
 *  time is ignored, instead it's frame_number * frame_duration,
 */
  
int lqt_encode_video(quicktime_t *file, 
                     unsigned char **row_pointers, 
                     int track, int64_t time);

  
/*
 *  Get the duration of the NEXT frame to be decoded.
 *  If constant is not NULL it will be set to 1 if the
 *  frame duration is constant throughout the whole track
 */

int lqt_frame_duration(quicktime_t * file, int track, int *constant);
  
/*
 *  Return the timestamp of the NEXT frame to be decoded.
 *  Call this BEFORE one of the decoding functions.
 */
  
int64_t lqt_frame_time(quicktime_t * file, int track);

/*
 *  Get the timescale of the track. Divide the return values
 *  of lqt_frame_duration and lqt_frame_time by the scale to
 *  get the time in seconds.
 */
  
int lqt_video_time_scale(quicktime_t * file, int track);

/*
 *  Return the duration of the entire track
 */

int64_t lqt_video_duration(quicktime_t * file, int track);

/*
 *  Set colormodel and row_span on a per track basis
 */

void lqt_set_cmodel(quicktime_t *file, int track, int colormodel);
void lqt_set_row_span(quicktime_t *file, int track, int row_span);
void lqt_set_row_span_uv(quicktime_t *file, int track, int row_span_uv);
  
/*
 * Same as quicktime_decode_audio, but it grabs all channels at
 * once. Or if you want only some channels you can leave the channels
 * you don't want = NULL in the poutput array. The poutput arrays
 * must contain at least lqt_total_channels(file) elements.
 */

int lqt_decode_audio(quicktime_t *file, 
					 int16_t **output_i, 
					 float **output_f, 
					 long samples);

/*
 * Returns the position of the last decoded sample. If it is smaller than you expected, EOF is reached.
 */
  
  
int64_t lqt_last_audio_position(quicktime_t * file, int track);
  
/*
 * Same as quicktime_encode_audio but with an additional track argument
 * for encoding files with more than one audio track
 */
  
int lqt_encode_audio_track(quicktime_t *file, 
                           int16_t **output_i, 
                           float **output_f, 
                           long samples,
                           int track);
  
/*
 * This decodes all channels from one track
 * (Was there a reason to hide the difference between tracks and
 * channels from the user?)
 */
  
  
int lqt_decode_audio_track(quicktime_t *file, 
                           int16_t **output_i, 
                           float **output_f, 
                           long samples,
                           int track);

/*
 *  Support for "raw" audio en-/decoding: This bypasses all
 *  internal sampleformat conversions, and allows access to audio
 *  samples in a format, which is closest to the internal representation.
 */
  
/*
 *  Query the internal sample format. Works for decoding (call after quicktime_open)
 *  and encoding (call after lqt_add_audio_track, lqt_set_audio or quicktime_set_audio).
 */

const char * lqt_sample_format_to_string(lqt_sample_format_t);
  
lqt_sample_format_t lqt_get_sample_format(quicktime_t *, int track);

/* The following return the actual number of en-/decoded frames */
  
int lqt_decode_audio_raw(quicktime_t *file, 
                         void * output, 
                         long samples,
                         int track);

int lqt_encode_audio_raw(quicktime_t *file, 
                         void * input, 
                         long samples,
                         int track);
  
/*
 *  Seek to a specified time. Use this instead of quicktime_set_video_position
 *  for streams with nonconstant framerate
 */
  
void lqt_seek_video(quicktime_t * file, int track,
                    int64_t time);
  
/*
 *  AVI Specific stuff
 */

int lqt_is_avi(quicktime_t *file);
int lqt_get_wav_id(quicktime_t *file, int track);
  
/*
 * Returns the total number of audio channels across all tracks.
 */
	
int lqt_total_channels(quicktime_t *file);

/* Extended metadata support */

void lqt_set_album(quicktime_t *file, char *string);
void lqt_set_artist(quicktime_t *file, char *string);
void lqt_set_genre(quicktime_t *file, char *string);
void lqt_set_track(quicktime_t *file, char *string);
void lqt_set_comment(quicktime_t *file, char *string);
void lqt_set_author(quicktime_t *file, char *string);

char * lqt_get_album(quicktime_t * file);
char * lqt_get_artist(quicktime_t * file);
char * lqt_get_genre(quicktime_t * file);
char * lqt_get_track(quicktime_t * file);
char * lqt_get_comment(quicktime_t *file);
char * lqt_get_author(quicktime_t *file);

/* get track number from track id */
int lqt_track_from_id(quicktime_t *file, int track_id);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

  
#endif
