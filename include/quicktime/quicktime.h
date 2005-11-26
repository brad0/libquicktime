#ifndef QUICKTIME_H
#define QUICKTIME_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

/* Some public enums needed by most subsequent headers */

/**
 * @file quicktime.h
 * Public api header.
 */

/** \defgroup general
    \brief General structures and functions
 */

/** \defgroup audio
    \brief Audio related definitions and functions
 */

/** \defgroup audio_decode Audio decoding
    \ingroup audio
    \brief Audio related definitions and functions (reading)
 */

/** \defgroup audio_encode Audio encoding
    \ingroup audio
    \brief Audio related definitions and functions (writing)
 */

/** \defgroup video
    \brief Video related definitions and functions
 */

/** \defgroup video_decode Video decoding
    \ingroup video
    \brief Video related definitions and functions (reading)
 */

/** \defgroup video_encode Video encoding
    \ingroup video
    \brief Video related definitions and functions (writing)
 */

/** \ingroup video
 * \brief interlace modes
 *
 * This is the interlace mode of a video track. Read it with
 * \ref lqt_get_interlace_mode .
 */
 
typedef enum 
  {
    LQT_INTERLACE_NONE = 0, /*!< No interlacing (= progressive) */
    LQT_INTERLACE_TOP_FIRST, /*!< Top field first */
    LQT_INTERLACE_BOTTOM_FIRST  /*!< Bottom field first */
  } lqt_interlace_mode_t;

/** \ingroup video
 * \brief Chroma placement
 *
 * This describes the chroma placement of a video track. Read it with
 * \ref lqt_get_chroma_placement . Chroma placement makes only sense for
 * YUV420 formats. For other pixelformats, it is set implicitely to
 * LQT_CHROMA_PLACEMENT_DEFAULT.
 */
  
typedef enum 
  {
    LQT_CHROMA_PLACEMENT_DEFAULT = 0, /*!< MPEG-1, JPEG or non 4:2:0 */
    LQT_CHROMA_PLACEMENT_MPEG2,       /*!< MPEG-2 */
    LQT_CHROMA_PLACEMENT_DVPAL,       /*!< DV PAL */
  } lqt_chroma_placement_t;

/** \ingroup audio
 * \brief Sample format definitions for audio
 *
 * This defines the datatype for audio samples, which will be used by a
 * particular codec. You'll need this, if you want to use \ref lqt_decode_audio_raw
 * or \ref lqt_encode_audio_raw . Byte order of the data is always machine native.
 * Endianess conversion is responsibility of the codec.
 */
  
typedef enum 
  {
    LQT_SAMPLE_UNDEFINED, /*!< If this is returned, we have an error */
    LQT_SAMPLE_INT8,      /*!< int8_t */
    LQT_SAMPLE_UINT8,     /*!< uint8_t */
    LQT_SAMPLE_INT16,     /*!< int16_t */
    LQT_SAMPLE_INT32,     /*!< int32_t */
    LQT_SAMPLE_FLOAT      /*!< Float (machine native) */
  } lqt_sample_format_t;
  
  
// #include "qtprivate.h"


  
/** \ingroup general
    \brief Quicktime handle

    Opaque file handle used both for reading and writing. In quicktime4linux, this structure is
    public, resulting in programmers doing wrong things with it. In libquicktime, this is
    a private structure, which is accessed exclusively by functions.
 */
  
typedef struct quicktime_s quicktime_t;
  
/* This is the reference for all your library entry points. */

/* ===== compression formats for which codecs exist ====== */

#define QUICKTIME_DIVX "DIVX"

#define QUICKTIME_DIV3 "DIV3"
  
#define QUICKTIME_DV "dvc "
/* AVID DV codec can be processed with libdv as well */
#define QUICKTIME_DV_AVID "AVdv"
#define QUICKTIME_DV_AVID_A "dvcp"

/* RGB uncompressed.  Allows alpha */
#define QUICKTIME_RAW  "raw "

/* Jpeg Photo */
#define QUICKTIME_JPEG "jpeg"

/* Concatenated png images.  Allows alpha */
#define QUICKTIME_PNG "png "

/* Motion JPEG-A. */
#define QUICKTIME_MJPA "mjpa"

/* YUV 4:2:2 */
#define QUICKTIME_YUV2 "yuv2"

/* YUV 4:2:0  NOT COMPATIBLE WITH STANDARD QUICKTIME */
#define QUICKTIME_YUV4 "yuv4"


/* ======== compression for which no codec exists ========== */
/* These are traditionally converted in hardware or not at all */

/* 8 bit Planar YUV 4:2:0 */
#define QUICKTIME_YUV420  "yv12"
/* 8 bit Planar YUV 4:1:1 */
#define QUICKTIME_YUV411  "y411"
/* 8 bit Packed full-range (not video) YUV 4:2:2 */
#define QUICKTIME_YUV422 "yuv2"
#define QUICKTIME_YUV2 "yuv2"
/* 8 bit Packed YUV (video range) 4:2:2 */
#define QUICKTIME_2VUY "2vuy"
/* 8 bit Planar YUV 4:4:4 */
#define QUICKTIME_YUV444  "v308"
#define QUICKTIME_V308  "v308"
/* 8 bit Planar YUVA 4:4:4:4 */
#define QUICKTIME_YUVA4444 "v408"
#define QUICKTIME_V408 "v408"
/* 10 bit Packed YUV 4:2:2 */
#define QUICKTIME_YUV422_10bit "v210"
#define QUICKTIME_V210 "v210"
/* 10 bit Packed YUV 4:4:4 */
#define QUICKTIME_YUV444_10bit  "v410"
#define QUICKTIME_V410 "v410"

/* ======== Studies in different algorithms =============== */

/* YUV9.  What on earth for? */
#define QUICKTIME_YUV9 "YVU9"

/* RTjpeg, proprietary but fast? */
#define QUICKTIME_RTJ0 "RTJ0"

/* =================== Audio formats ======================= */

/* Unsigned 8 bit */
#ifndef QUICKTIME_RAW
#define QUICKTIME_RAW "raw "
#endif

/* IMA4 */
#define QUICKTIME_IMA4 "ima4"

/* Twos compliment 8, 16, 24 */
#define QUICKTIME_TWOS "twos"

/* ulaw */
#define QUICKTIME_ULAW "ulaw"

#define QUICKTIME_VORBIS "OggS"

#define QUICKTIME_MP3 ".mp3"
#define QUICKTIME_WMA "WMA "
  
/* =========================== public interface ========================= // */

/* Get version information */
int quicktime_major();
int quicktime_minor();
int quicktime_release();

/** \ingroup general
    \brief Test file compatibility
    \param path A path to a regular file
    \returns 1 if the file is decodable by libquicktime.
    
    Check the signature of a path and return 1 is the file is likely to ba
    decodable by libquicktime. This check might return false positives or false
    negatives. In general it's better (although slower) to check, if \ref quicktime_open
    return NULL or not.
 */
  
int quicktime_check_sig(char *path);

/** \ingroup general
    \brief Open a file
    \param filename A path to a regular file
    \param rd 1 for open readonly, 0 else
    \param wr 1 for open writeonly, 0 else
    \returns An initialized file handle or NULL if opening failed.
    
    Note, that files can never be opened read/write mode.
*/
  
quicktime_t* quicktime_open(const char *filename, int rd, int wr);

/** \ingroup general
    \brief Make a file streamable 
    \param in_path Existing non streamable file
    \param out_path Output file
    \returns 1 if an error occurred, 0 else

    This function makes a file streamable by placing the moov header at the beginning of the file.
    Note that you need approximately the twice the disk-space of the file. It is recommended, that
    this function is called only for files, which are encoded by libquicktime. Other files might not
    be correctly written.
*/
  
int quicktime_make_streamable(char *in_path, char *out_path);

/** \defgroup metadata
    \brief Metadata support

    These functions allow you to read/write the metadata of the file. Currently, only the
    metadata in the udta atom are supported
*/

/** \ingroup metadata
    \brief Set the copyright info for the file
    \param file A quicktime handle
    \param string The copyright info
*/
  
void quicktime_set_copyright(quicktime_t *file, char *string);

/** \ingroup metadata
    \brief Set the name for the file
    \param file A quicktime handle
    \param string The name
*/

void quicktime_set_name(quicktime_t *file, char *string);

/** \ingroup metadata
    \brief Set info for the file
    \param file A quicktime handle
    \param string An info string
*/

void quicktime_set_info(quicktime_t *file, char *string);

/** \ingroup metadata
    \brief Get the copyright info from the file
    \param file A quicktime handle
    \returns The copyright info or NULL
*/
  

char* quicktime_get_copyright(quicktime_t *file);

/** \ingroup metadata
    \brief Get the name from the file
    \param file A quicktime handle
    \returns The name or NULL
*/
  

char* quicktime_get_name(quicktime_t *file);

/** \ingroup metadata
    \brief Get the info string from the file
    \param file A quicktime handle
    \returns The info string or NULL
*/
  
char* quicktime_get_info(quicktime_t *file);

/* Read all the information about the file. */
/* Requires a MOOV atom be present in the file. */
/* If no MOOV atom exists return 1 else return 0. */
int quicktime_read_info(quicktime_t *file);

/** \ingroup audio_encode
    \brief Set up tracks in a new file after opening and before writing
    \param file A quicktime handle
    \param channels Number of channels
    \param sample_rate Samplerate
    \param bits Bits per sample
    \param compressor Compressor to use

    Returns the number of quicktime tracks allocated. Audio is stored two channels
    per quicktime track.

    This function is depracated and should not be used in newly written code. It won't let you
    add individual tracks with different codecs, samplerates etc. Use \ref lqt_add_audio_track instread.
*/
  
int quicktime_set_audio(quicktime_t *file, 
	int channels, 
	long sample_rate, 
	int bits, 
	char *compressor);

/** \ingroup video_encode
    \brief Set the framerate for encoding
    \param file A quicktime handle
    \param framerate framerate

    Sets the framerate for encoding.

    This function is depracated and should not be used in newly written code.
*/

void quicktime_set_framerate(quicktime_t *file, double framerate);

/** \ingroup video_encode
    \brief Set up video tracks for encoding
    \param file A quicktime handle
    \param tracks Number of tracks
    \param frame_w Frame width
    \param frame_h Frame height
    \param frame_rate Frame rate (in frames per second)

    This function is depracated and should not be used in newly written code. Use \lqt_add_video_track instead.
*/
  
int quicktime_set_video(quicktime_t *file, 
	int tracks, 
	int frame_w, 
	int frame_h, 
	double frame_rate, 
	char *compressor);

/** \ingroup video_encode
    \brief Set jpeg encoding quality
    \param file A quicktime handle
    \param quality Quality (0..100)
    \param use_float Use floating point routines
    
    Set the jpeg encoding quality and whether to use floating point routines.
    This should be called after creating the video track(s).
    
    This function is depracated and should not be used in newly written code.
    Use \ref lqt_set_video_parameter instead.
*/
  
void quicktime_set_jpeg(quicktime_t *file, int quality, int use_float);

/** \ingroup video_encode
 * \brief Set a codec parameter
 *  \param file A quicktime handle
 *  \param key Short name of the parameter
 *  \param value Parameter value.
 *
 *  For integer parameters, value must be of the type int*. For string parameters,
 *  use char*.
 *
 *  This function sets the same parameter for all video AND audio streams, which is quite
 *  idiotic. Use \ref lqt_set_audio_parameter and \ref lqt_set_video_parameter to set
 *  codec parameters on a per stream basis.
 */

void quicktime_set_parameter(quicktime_t *file, char *key, void *value);

/** \ingroup video_encode
 *  \brief Set the depth of a video track.
 *  \param file A quicktime handle
 *  \param depth The depth (bits per pixel)
 *  \param Track index (starting with 0)
 *
 *  This function is deprecated and should never be called.
 *  Use the depth argument of 
 *  
 */
void quicktime_set_depth(quicktime_t *file, 
	int depth, 
	int track);

/* Set the colormodel for encoder input */
void quicktime_set_cmodel(quicktime_t *file, int colormodel);
/* Set row span for decoder output */
void quicktime_set_row_span(quicktime_t *file, int row_span);

/* close the file and delete all the objects */
int quicktime_close(quicktime_t *file);

/* get length information */
/* channel numbers start on 1 for audio and video */
long quicktime_audio_length(quicktime_t *file, int track);
long quicktime_video_length(quicktime_t *file, int track);

/* get position information */
long quicktime_audio_position(quicktime_t *file, int track);
long quicktime_video_position(quicktime_t *file, int track);

/* get file information */
int quicktime_video_tracks(quicktime_t *file);
int quicktime_audio_tracks(quicktime_t *file);

int quicktime_has_audio(quicktime_t *file);
long quicktime_sample_rate(quicktime_t *file, int track);
int quicktime_audio_bits(quicktime_t *file, int track);
int quicktime_track_channels(quicktime_t *file, int track);
char* quicktime_audio_compressor(quicktime_t *file, int track);

int quicktime_has_video(quicktime_t *file);
int quicktime_video_width(quicktime_t *file, int track);
int quicktime_video_height(quicktime_t *file, int track);
int quicktime_video_depth(quicktime_t *file, int track);
double quicktime_frame_rate(quicktime_t *file, int track);
char* quicktime_video_compressor(quicktime_t *file, int track);

/* number of bytes of raw data in this frame */
long quicktime_frame_size(quicktime_t *file, long frame, int track);

/* get the quicktime track and channel that the audio channel belongs to */
/* channels and tracks start on 0 */
int quicktime_channel_location(quicktime_t *file, int *quicktime_track, int *quicktime_channel, int channel);

/* file positioning */
int quicktime_seek_end(quicktime_t *file);
int quicktime_seek_start(quicktime_t *file);

/* set position of file descriptor relative to a track */
int quicktime_set_audio_position(quicktime_t *file, int64_t sample, int track);
int quicktime_set_video_position(quicktime_t *file, int64_t frame, int track);

/* ========================== Access to raw data follows. */
/* write data for one quicktime track */
/* the user must handle conversion to the channels in this track */
int quicktime_write_audio(quicktime_t *file, uint8_t *audio_buffer, long samples, int track);
int quicktime_write_frame(quicktime_t *file, uint8_t *video_buffer, int64_t bytes, int track);

/* read raw data */
long quicktime_read_audio(quicktime_t *file, char *audio_buffer, long samples, int track);
long quicktime_read_frame(quicktime_t *file, unsigned char *video_buffer, int track);

/* for reading frame using a library that needs a file descriptor */
/* Frame caching doesn't work here. */
int quicktime_read_frame_init(quicktime_t *file, int track);
int quicktime_read_frame_end(quicktime_t *file, int track);

/* One keyframe table for each track */
long quicktime_get_keyframe_before(quicktime_t *file, long frame, int track);
void quicktime_insert_keyframe(quicktime_t *file, long frame, int track);
/* Track has keyframes */
int quicktime_has_keyframes(quicktime_t *file, int track);

/* ===================== Access to built in codecs follows. */

/* If the codec for this track is supported in the library return 1. */
int quicktime_supported_video(quicktime_t *file, int track);
int quicktime_supported_audio(quicktime_t *file, int track);

/* The codec can generate the color model with no downsampling */
int quicktime_reads_cmodel(quicktime_t *file, 
		int colormodel, 
		int track);

/* The codec can write the color model with no upsampling */
int quicktime_writes_cmodel(quicktime_t *file, 
		int colormodel, 
		int track);


/* Hacks for temporal codec */
int quicktime_divx_is_key(unsigned char *data, long size);
int quicktime_divx_write_vol(unsigned char *data_start,
	int vol_width, 
	int vol_height, 
	int time_increment_resolution, 
	double frame_rate);
int quicktime_divx_has_vol(unsigned char *data);

int quicktime_div3_is_key(unsigned char *data, long size);

/* Decode or encode the frame into a frame buffer. */
/* All the frame buffers passed to these functions are unsigned char */
/* rows with 3 bytes per pixel.  The byte order per 3 byte pixel is */
/* RGB. */
int quicktime_encode_video(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track);

// Decodes RGB only
int quicktime_decode_video(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track);
long quicktime_decode_scaled(quicktime_t *file, 
	int in_x,                    /* Location of input frame to take picture */
	int in_y,
	int in_w,
	int in_h,
	int out_w,                   /* Dimensions of output frame */
	int out_h,
	int color_model,             /* One of the color models defined above */
	unsigned char **row_pointers, 
	int track);

/* Decode or encode audio for a single channel into the buffer. */
/* Pass a buffer for the _i or the _f argument if you want int16 or float data. */
/* Notice that encoding requires an array of pointers to each channel. */
int quicktime_decode_audio(quicktime_t *file, int16_t *output_i, float *output_f, long samples, int channel);
int quicktime_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f, long samples);

/* Dump the file structures for the currently opened file. */
int quicktime_dump(quicktime_t *file);

/* Specify the number of cpus to utilize. */
int quicktime_set_cpus(quicktime_t *file, int cpus);

/* Specify whether to read contiguously or not. */
/* preload is the number of bytes to read ahead. */
/* This is no longer functional to the end user but is used to accelerate */
/* reading the header internally. */
void quicktime_set_preload(quicktime_t *file, int64_t preload);

int64_t quicktime_byte_position(quicktime_t *file);

void quicktime_set_avi(quicktime_t *file, int value);

  

#ifdef __cplusplus
}
#endif

#endif
