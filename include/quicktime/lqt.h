#ifndef _LQT_H_
#define _LQT_H_

#include "quicktime.h"
#include "lqt_atoms.h"
#include "lqt_codecinfo.h"
#include "lqt_qtvr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @file lqt.h
 * Public api header for libquicktime extensions.
 */
  
void *lqt_bufalloc(size_t size);

/** \ingroup general
 *
 * \brief Return the raw filedescriptor associated with the file
 * \param file A quicktime handle
 * \returns The filesecriptor
 *
 * Use this of you want to call some low-level functions of the file.
 * Note, that this routine should be used with care, since it's easy
 * to screw things up.
 */
  
int lqt_fileno(quicktime_t *file);

/** \ingroup audio
 *  \brief Set a codec parameter for an audio track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param key Short name of the parameter
 *  \param value Parameter value.
 *
 *  For integer parameters, value must be of the type int*. For string parameters,
 *  use char*.
 */
  
void lqt_set_audio_parameter(quicktime_t *file,int track, char *key,void *value);

/** \ingroup video
 *  \brief Set a codec parameter for a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param key Short name of the parameter
 *  \param value Parameter value.
 *
 *  For integer parameters, value must be of the type int*. For string parameters,
 *  use char*.
 */

void lqt_set_video_parameter(quicktime_t *file,int track, char *key,void *value);

/** \ingroup video_decode
 *  \brief Get the pixel aspect ratio of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param pixel_width returns the pixel width
 *  \param pixel_height returns the pixel height
 *  \returns 1 if the call was successful, 0 if there is no such track
 */


int lqt_get_pixel_aspect(quicktime_t *file, int track, int * pixel_width,
                         int * pixel_height);

/** \ingroup video_encode
 *  \brief Set the pixel aspect ratio of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param pixel_width Pixel width
 *  \param pixel_height Pixel height
 *  \returns 1 if the call was successful, 0 if there is no such track
 *
 */

int lqt_set_pixel_aspect(quicktime_t *file, int track, int pixel_width,
                         int pixel_height);

/** \ingroup video_decode
    \brief Get the interlace mode
    \param file A quicktime handle
    \param track Track index (starting with 0)
    \returns The interlace mode.

    The interlace mode is stored in the fiel atom, which is used
    by default. If there is no fiel atom and the interlace mode is
    not implied by
    the codec, \ref LQT_INTERLACE_NONE is returned, which might be wrong.
*/

lqt_interlace_mode_t lqt_get_interlace_mode(quicktime_t * file, int track);

/** \ingroup video
    \brief Convert an interlace mode to a human readable string
    \param mode An interlace mode
    \returns A description of the interlace mode
*/
const char * lqt_interlace_mode_to_string(lqt_interlace_mode_t mode);

/** \ingroup video_decode
    \brief Get the chroma placement
    \param file A quicktime handle
    \param track Track index (starting with 0)
    \returns The chroma placement

    The chroma placement is implied by the codec and makes only sense
    for YUV420 formats.
*/
  
lqt_chroma_placement_t lqt_get_chroma_placement(quicktime_t * file, int track);

/** \ingroup video
    \brief Convert a chroma placement to a human readable string
    \param chroma_placement A chroma placement
    \returns A description of the chroma placement
*/

const char * lqt_chroma_placement_to_string(lqt_chroma_placement_t chroma_placement);

/** \ingroup general
    \brief Get the codec API version.
    \returns The codec API version, libquicktime was compiled with

    Normally you don't need this function. It is used internally to
    detect codec modules, which were compiled with an incompatible
    libquicktime version.
    
*/
  
int lqt_get_codec_api_version();

/***********************************************
 * Advanced colormodel handling.
 * (defined in lqt_color.c)
 ***********************************************/

/** \ingroup color
 *
 *  This value is used for termination of colormodel arrays
 */
  
#define LQT_COLORMODEL_NONE -1
  
/* Colormodel <-> string conversion (used by registry file routines) */

/** \ingroup color
    \brief Convert a colormodel to a human readable string
    \param colormodel A colormodel
    \returns A description of the colormodel
*/
  
const char * lqt_colormodel_to_string(int colormodel);

/** \ingroup color
    \brief Convert a description string to a colormodel
    \param str A colormodel description (as returned by \ref lqt_colormodel_to_string)
    \returns The corresponding colormodel or \ref LQT_COLORMODEL_NONE
*/
  
int lqt_string_to_colormodel(const char * str);

/* Query information about the colormodel */

/** \ingroup color
   \brief Check if a colormodel is planar
   \param colormodel A colormodel
   \returns 1 if the colormodel is planar, 0 else
*/

int lqt_colormodel_is_planar(int colormodel);

  /** \ingroup color
   \brief Check if a colormodel has an alpha (transperency) channel
   \param colormodel A colormodel
   \returns 1 if the colormodel has an alpha channel, 0 else
*/

int lqt_colormodel_has_alpha(int colormodel);

/** \ingroup color
   \brief Check, if a colormodel is RGB based
   \param colormodel A colormodel
   \returns 1 if the colormodel is RGB based, 0 else
*/
  
int lqt_colormodel_is_rgb(int colormodel);

/** \ingroup color
   \brief Check, if a colormodel is YUV based
   \param colormodel A colormodel
   \returns 1 if the colormodel is YUV based, 0 else
*/

int lqt_colormodel_is_yuv(int colormodel);

/** \ingroup color
   \brief Get the chroma subsampling factors
   \param colormodel A colormodel
   \param sub_h Returns the horizontal subsampling factor
   \param sub_v Returns the vertical subsampling factor
   
*/

void lqt_colormodel_get_chroma_sub(int colormodel, int * sub_h, int * sub_v);

/** \ingroup color
   \brief Get the default row span for a colormodel and an image width
   \param colormodel A colormodel
   \param width Image width
   \param rowspan Returns the rowspan for the luminance (Y) plane 
   \param rowspan_uv Returns the rowspan for the chrominance (U/V) planes 
   
   The rowspan is the byte offset between scanlines. It can be calculated
   from the colormodel and the image width. Some APIs however, padd the scanlines to
   certain boundaries, so the rowspans might become larger here (see \ref lqt_set_row_span and
   \ref lqt_set_row_span_uv).
*/

void lqt_get_default_rowspan(int colormodel, int width, int * rowspan, int * rowspan_uv);

/** \ingroup color
 * \brief Check if a colormodel conversion is supported by libquicktime
 * \param in_cmodel Input colormodel
 * \param out_cmodel Output colormodel
 * \returns 1 if the requested conversion is possible, 0 else
 *
 * As noted before, the colormodel converter is not complete, and this function
 * checks it. As a fallback, conversions from and to \ref BC_RGB888 are always supported.
 * If you need a converison, which is not present, contact the authors for hints how to
 * write it :)
 */
 
int lqt_colormodel_has_conversion(int in_cmodel, int out_cmodel);
  
/* Query supported colormodels */

/** \ingroup color
    \brief Get number of supported colormodels
    \returns The number of colormodels known to your version of libquicktime
*/
  
int lqt_num_colormodels();

/** \ingroup color
    \brief Get a colormodel string
    \param index Index of the colormodel (between 0 and the return value of \ref lqt_num_colormodels - 1)
    \returns A description of the colormodel according to index or NULL.
*/  
const char * lqt_get_colormodel_string(int index);

/** \ingroup color
    \brief Get a colormodel
    \param index Index of the colormodel (between 0 and the return value of \ref lqt_num_colormodels - 1)
    \returns The colormodel according to index or \ref LQT_COLORMODEL_NONE
*/  
  
int lqt_get_colormodel(int index);

/** \ingroup video_decode
 *  \brief Get the native colormodel of the decoder
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *
 *  This returns the colormodel, which is used by the stream natively. If your
 *  application can handle the colormodel, you can use \ref lqt_decode_video for
 *  decoding in the native colormodel. This will bypass all internal colormodel conversions.
 */

int lqt_get_decoder_colormodel(quicktime_t * file, int track);

/** \ingroup video
 *  \brief Get the best colormodel out of a list of supported colormodels
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param supported An array with supported colormodels.
 *  \returns The best colormodel
 *
 *  This is a convenience function for application developers:
 *  It takes an array with supported colormodels (Terminated with LQT_COLORMODEL_NONE)
 *  and returns the best colormodel. The decision is done according to the conversion
 *  overhead. i.e. you'll get the colormodel of your list, which is "closest" to the
 *  colormodel, the codec delivers. To make sure, that this function never fails, you
 *  should at least support \ref BC_RGB888 .
 *  This function works for en- and decoding.
 */

int lqt_get_best_colormodel(quicktime_t * file, int track, int * supported);

/** \ingroup video
 *  \brief Get the colormodel, which will be valid for the next en-/decode call.
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \returns The colormodel used for en-/decode functions.
 *
 *  By default, it will return the colormodel, which is used natively
 *  by the codec. It can be changed with \ref lqt_set_cmodel .
 */
  
int lqt_get_cmodel(quicktime_t * file, int track);
  
/** \ingroup video
 *  \brief Allocate a frame buffer for use with libquicktime
 *  \param width The width of the frame
 *  \param height The height of the frame
 *  \param colormodel The colormodel of the frame (see \ref color).
 *  \param rowspan Specifies the row span to use for the luma plane. Can be 0 to use default values. After the function call, it will contain the rowspan actually used.
 *  \param rowspan_uv Specifies the row span to use for the chroma planes. Can be 0 to use default values. After the function call, it will contain the rowspan actually used.
 *  \returns An array of pointers to be passed to any of the en-/decoding functions.
 *
 *  What is called "row_pointers" here is a bit misleading: For packed formats,
 *  the pointers point to the beginnings of scanlines. For planar formats, the pointers
 *  point to the beginning of planes. In either case, the byte offsets between scanlines are
 *  be specified by rowspan and rowspan_uv. To free the returned frame, call \ref lqt_rows_free
 */
  
uint8_t ** lqt_rows_alloc(int width, int height, int colormodel, int * rowspan, int * rowspan_uv);

/** \ingroup video
 * \brief Copy a video frame.
 * \param out_rows Destination frame
 * \param in_rows Source frame
 * \param width Width of the frame
 * \param height Height of the frame
 * \param in_rowspan Rowspan for the luma plane of the input frame
 * \param in_rowspan_uv Rowspan for the chroma planes of the input frame
 * \param out_rowspan Rowspan for the luma plane of the output frame
 * \param out_rowspan_uv Rowspan for the chroma planes of the output frame
 * \param colormodel The colormodel of the frames
 */
   
void lqt_rows_copy(uint8_t **out_rows, uint8_t **in_rows, int width, int height, int in_rowspan, int in_rowspan_uv,
                   int out_rowspan, int out_rowspan_uv, int colormodel);
  

/** \ingroup video
 *  \brief Free a frame allocated by \ref lqt_rows_alloc
 *  \param rows The frame to be freed
 */
  
void lqt_rows_free(uint8_t ** rows);
  

/**************************************
 * Set streams for encoding
 **************************************/

/** \ingroup audio_encode
 *  \brief Set up audio tracks for encoding
 *  \param file A quicktime handle
 *  \param channels Number of channels
 *  \param sample_rate Samplerate
 *  \param bits Bits per sample (always 16)
 *  \param codec_info Codec to use (see \ref codec_registry )
 *
 *  This sets one audio track for encoding. Note that the bits argument
 *  should always be 16 since it's implicit to the codec in all cases.
 *  To add more than one audio track, use \ref lqt_add_audio_track .
 */
 
int lqt_set_audio(quicktime_t *file, int channels,
                  long sample_rate,  int bits,
                  lqt_codec_info_t * codec_info);

  
/** \ingroup video_encode
 *  \brief Set up video tracks for encoding
 *  \param file A quicktime handle
 *  \param tracks Number of video tracks
 *  \param frame_w Image width
 *  \param frame_h Image height
 *  \param frame_duration Duration of one frame. This can later be overridden
 *  \param timescale Timescale of the track
 *  \param codec_info Codec to use (see \ref codec_registry )
 *
 *  This sets one or more video tracks for encoding. The framerate is
 *  passed as a rational number (timescale/frame_duration). E.g. for an NTSC
 *  stream, you'll choose timescale = 30000 and frame_duration = 1001.
 *  To set up multiple video tracks with different formats and/or codecs,
 *  use \ref lqt_add_video_track .
 */

int lqt_set_video(quicktime_t *file, int tracks, 
                  int frame_w, int frame_h,
                  int frame_duration, int timescale,
                  lqt_codec_info_t * codec_info);

 
/** \ingroup audio_encode
 *  \brief Add an audio tracks for encoding
 *  \param file A quicktime handle
 *  \param channels Number of channels
 *  \param sample_rate Samplerate
 *  \param bits Bits per sample (always 16)
 *  \param codec_info Codec to use (see \ref codec_registry )
 *
 *  This sets adds a new audio track for encoding. Note that the bits argument
 *  should always be 16 since it's implicit to the codec in all cases.
 *  Call this function to subsequently to add as many tracks as you like.
 */

int lqt_add_audio_track(quicktime_t *file,
                        int channels, long sample_rate, int bits,
                        lqt_codec_info_t * codec_info);

/** \ingroup video_encode
 *  \brief Add a video track for encoding
 *  \param file A quicktime handle
 *  \param frame_w Image width
 *  \param frame_h Image height
 *  \param frame_duration Duration of one frame. This can later be overridden
 *  \param timescale Timescale of the track
 *  \param codec_info Codec to use (see \ref codec_registry )
 *
 *  This sets one or more video tracks for encoding. The framerate is
 *  passed as a rational number (timescale/frame_duration). E.g. for an NTSC
 *  stream, you'll choose timescale = 30000 and frame_duration = 1001.
 *  Call this function to subsequently to add as many tracks as you like.
 */
  
int lqt_add_video_track(quicktime_t *file,
                        int frame_w, int frame_h,
                        int frame_duration, int timescale,
                        lqt_codec_info_t * codec_info);
  

/** \ingroup video_decode
 *  \brief Get the timestamp of the next frame to be decoded
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \returns The timestamp of the next frame to be decoded
 *
 *  Call this BEFORE one of the decoding functions to get the
 *  timestamp of the next frame
 */
  
int64_t lqt_frame_time(quicktime_t * file, int track);

/** \ingroup video_decode
 *  \brief Decode one video frame
 *  \param file A quicktime handle
 *  \param row_pointers Frame (see \ref lqt_rows_alloc)
 *  \param track Track index (starting with 0)
 *
 * Decode one video frame and increment the interal frame pointer.
 * To get the presentation timestamp for this frame, call
 * \ref lqt_frame_time before.
 */
  

int lqt_decode_video(quicktime_t *file,
                     unsigned char **row_pointers, int track);

/** \ingroup video_encode
 *  \brief Encode one video frame
 *  \param file A quicktime handle
 *  \param row_pointers Frame (see \ref lqt_rows_alloc)
 *  \param track Track index (starting with 0)
 *  \param time Timestamp of the frame in timescale tics
 *
 * Encode one video frame. The presentation timestamp is in
 * timescale tics with the timescale you passed to
 * \ref lqt_add_video_track or \ref lqt_set_video . WARNING: AVI files
 * don't support arbitrary timestamps. For AVI files
 *  time is ignored, instead it's frame_number * frame_duration,
 */
  
int lqt_encode_video(quicktime_t *file, 
                     unsigned char **row_pointers, 
                     int track, int64_t time);
  
/** \ingroup video_decode
 *  \brief Get the duration of the NEXT frame to be decoded
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param constant If non NULL it will be set to 1 if the frame duration is constant throughout the whole track
 *  \returns The frame duration in timescale tics
 */

int lqt_frame_duration(quicktime_t * file, int track, int *constant);
  
/** \ingroup video_decode
 *  \brief Get the timescale of the track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \returns The timescale of the track
 */
  
int lqt_video_time_scale(quicktime_t * file, int track);

/** \ingroup video_decode
 *  \brief Get the duration of a video track
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \returns The total duration of the track in timescale tics
 *
 *  Use this function if you want to support nonconstant framerates.
 */

int64_t lqt_video_duration(quicktime_t * file, int track);

/** \ingroup video
 * \brief Set the colormodel for en-/decoding
 *  \param file A quicktime handle
 *  \param track Track index (starting with 0)
 *  \param colormodel The colormodel to use.
 *
 *  Set colormodel of a video track. It's the colormodel, libquicktime
 *  will expect for the next call to \ref lqt_encode_video or
 *  \ref lqt_decode_video respectively. Before you should call this,
 *  you should verify, that this colormodel can be used with
 *  \ref quicktime_reads_cmodel (for reading), \ref quicktime_writes_cmodel
 *  (for writing) or \ref lqt_get_best_colormodel (for reading and writing).
  */

void lqt_set_cmodel(quicktime_t *file, int track, int colormodel);

/** \ingroup video
 * \brief Set the row span for the luma plane
 * \param file A quicktime handle
 * \param track Track index (starting with 0)
 * \param row_span The row span for the luma plane
 *
 * This sets the row_span, which will be used for the next en-/decode
 * calls (see \ref lqt_rows_alloc ).
 */

void lqt_set_row_span(quicktime_t *file, int track, int row_span);

/** \ingroup video
 * \brief Set the row span for the chroma planes
 * \param file A quicktime handle
 * \param track Track index (starting with 0)
 * \param row_span_uv The row span for the chroma planes
 *
 * This sets the row_span, which will be used for the next en-/decode
 * calls (see \ref lqt_rows_alloc ).
 */

void lqt_set_row_span_uv(quicktime_t *file, int track, int row_span_uv);
  
/*
 * Same as quicktime_decode_audio, but it grabs all channels at
 * once. Or if you want only some channels you can leave the channels
 * you don't want = NULL in the output array. The output arrays
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
  
/** \ingroup audio_encode
 *  \brief Encode a number of audio samples for the first track
 *  \param file A quicktime handle
 *  \param output_i 16 bit integer output buffer (or NULL)
 *  \param output_f floating point output buffer (or NULL)
 *  \param samples Number of samples to decode
 *  \param track index (starting with 0)
 *
 * Same as \ref quicktime_encode_audio but with an additional track argument
 * for encoding files with more than one audio track. If you want to pass the full
 * resolution even for 24/32 bit audio, use \ref lqt_encode_audio_raw .
 */
  
int lqt_encode_audio_track(quicktime_t *file, 
                           int16_t **output_i, 
                           float **output_f, 
                           long samples,
                           int track);
  
/** \ingroup audio_decode
 *  \brief Decode a number of audio samples
 *  \param file A quicktime handle
 *  \param output_i 16 bit integer output buffer (or NULL)
 *  \param output_f floating point output buffer (or NULL)
 *  \param samples Number of samples to decode
 *  \param track index (starting with 0)
 *
 * Decode a number of samples from an audio track. All channels are decoded at once.
 * output_i and output_f point to noninterleaved arrays for each channel. Depending
 * on what you need, set either output_i or output_f to NULL. If you want the full resolution
 * also for 24/32 bits, use \ref lqt_decode_audio_raw .
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

/** \ingroup audio
 * \brief Get a human readable description for a sample format
 * \param sampleformat A sampleformat
 * \returns The description or NULL
 */
  
const char * lqt_sample_format_to_string(lqt_sample_format_t sampleformat);

/** \ingroup audio
 * \brief Return the sample format used natively by the codec.
 * \param file A quicktime handle
 * \param track Track index (starting with 0)
 * \returns The sampleformat
 *
 * Use this function if you want to use \ref lqt_decode_audio_raw or \ref lqt_encode_audio_raw
 * to bypass libquicktimes internal sample format conversion routines.
 */
  
lqt_sample_format_t lqt_get_sample_format(quicktime_t * file, int track);

/* The following return the actual number of en-/decoded frames */

/** \ingroup audio_decode
 * \brief Decode audio in the native sampleformat of the codec
 * \param file A quicktime handle
 * \param output An array of interleaved samples
 * \param samples Number of samples to decode
 * \param track Track index (starting with 0)
 * \returns The number of actually decoded samples. 0 means end of track.
 *
 * This function bypasses all internal sampleformat conversion and allows
 * full resolution output for up to 32 bit integer and 32 bit float.
 * To check, which dataformat the samples will have, use \ref lqt_get_sample_format .
 */

int lqt_decode_audio_raw(quicktime_t *file, 
                         void * output, 
                         long samples,
                         int track);

/** \ingroup audio_encode
 * \brief Encode audio in the native sampleformat of the codec
 * \param file A quicktime handle
 * \param input An array of interleaved samples
 * \param samples Number of samples to encode
 * \param track Track index (starting with 0)
 *
 * This function bypasses all internal sampleformat conversion and allows
 * full resolution input for up to 32 bit integer and 32 bit float.
 * To check, which dataformat the samples will have, use \ref lqt_get_sample_format .
 */
  
int lqt_encode_audio_raw(quicktime_t *file, 
                         void * input, 
                         long samples,
                         int track);

/** \ingroup video_decode
 *  \brief Seek to a specific video time
 * \param file A quicktime handle
 * \param time The desired time of the next frame in timescale tics (starting with 0)
 * \param track index (starting with 0)
 *
 * Use this for seeking. During sequential decode calls, the position will be updated automatically.
 * Replacement of \ref quicktime_set_video_position
 * which also works for streams with nonconstant framerate
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

/** \ingroup metadata
    \brief Set the album for the file
    \param file A quicktime handle
    \param string The album
*/
  
void lqt_set_album(quicktime_t *file, char *string);

/** \ingroup metadata
    \brief Set the artist for the file
    \param file A quicktime handle
    \param string The artist
*/
  
void lqt_set_artist(quicktime_t *file, char *string);

/** \ingroup metadata
    \brief Set the genre for the file
    \param file A quicktime handle
    \param string The genre
*/

void lqt_set_genre(quicktime_t *file, char *string);

/** \ingroup metadata
    \brief Set the track number for the file
    \param file A quicktime handle
    \param string The track number (as string)
*/
  

void lqt_set_track(quicktime_t *file, char *string);

/** \ingroup metadata
    \brief Set the comment for the file
    \param file A quicktime handle
    \param string The comment
*/

void lqt_set_comment(quicktime_t *file, char *string);

/** \ingroup metadata
    \brief Set the author for the file
    \param file A quicktime handle
    \param string The author
*/

void lqt_set_author(quicktime_t *file, char *string);

/** \ingroup metadata
    \brief Get the album from the file
    \param file A quicktime handle
    \returns The album or NULL
*/

char * lqt_get_album(quicktime_t * file);

/** \ingroup metadata
    \brief Get the artist from the file
    \param file A quicktime handle
    \returns The artist or NULL
*/
char * lqt_get_artist(quicktime_t * file);

/** \ingroup metadata
    \brief Get the genre from the file
    \param file A quicktime handle
    \returns The genre or NULL
*/

char * lqt_get_genre(quicktime_t * file);

/** \ingroup metadata
    \brief Get the track number from the file
    \param file A quicktime handle
    \returns The track number (as string) or NULL
*/
char * lqt_get_track(quicktime_t * file);

/** \ingroup metadata
    \brief Get the comment from the file
    \param file A quicktime handle
    \returns The comment or NULL
*/

char * lqt_get_comment(quicktime_t *file);

/** \ingroup metadata
    \brief Get the author from the file
    \param file A quicktime handle
    \returns The author or NULL
*/
char * lqt_get_author(quicktime_t *file);

/* get track number from track id */
int lqt_track_from_id(quicktime_t *file, int track_id);
  
#ifdef __cplusplus
}
#endif /* __cplusplus */

  
#endif
