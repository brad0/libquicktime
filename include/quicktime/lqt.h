#ifndef _LQT_H_
#define _LQT_H_

#include "quicktime.h"
#include "lqt_codecinfo.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* Call quicktime_set_parameter with our codec info */

void lqt_set_audio_parameter(quicktime_t *file,int stream, char *key,void *value);
void lqt_set_video_parameter(quicktime_t *file,int stream, char *key,void *value);

int lqt_set_fiel(quicktime_t *, int, int, int);
int lqt_get_fiel(quicktime_t *, int, int *, int *);

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
  
/* Query supported colormodels */

int lqt_num_colormodels();

const char * lqt_get_colormodel_string(int index);

int lqt_get_colormodel(int index);

/*
 *  Return decoder colormodel
 *  This function can fail in some pathological cases,
 *  it returns LQT_COLORMODEL_NONE then
 */

int lqt_get_decoder_colormodel(quicktime_t * file, int track,
                               int * exact);

/*
 *   Convenience function for application developers:
 *   It takes an array with supported colormodels
 *   (Terminated with LQT_COLORMODEL_NONE) and returns
 *   the best colormodel
 *   Works for en- and decoding
 */

int lqt_get_best_colormodel(quicktime_t * file, int track, int * supported);


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


/* QTVR stuff */

/* check if the file is a qtvr file */
/* return values:                                                             */
/* QTVR_OBJ  = file is object movie                                           */
/* QTVR_PAN  = file is panorama                                               */
int lqt_is_qtvr(quicktime_t  *file);


/* initialize qtvr atoms/track */
/* type: QTVR_OBJ == object movie                                             */
/*       QTVR_PAN == panorama movie                                           */
int lqt_qtvr_set_type(quicktime_t  *file,
		      int type,
		      int width,
		      int height,
		      int duration,
		      int time_scale,
		      int scene_track);


/* return full dimensions of the movie*/
int lqt_qtvr_get_width(quicktime_t  *file);
int lqt_qtvr_get_height(quicktime_t  *file);

/* get depth of the movie */
int lqt_qtvr_get_depth(quicktime_t  *file);

/* get/set number of rows */
int lqt_qtvr_set_rows(quicktime_t  *file, short rows);
int lqt_qtvr_get_rows(quicktime_t  *file);

/* get/set number of columns */
int lqt_qtvr_set_columns(quicktime_t  *file, short columns);
int lqt_qtvr_get_columns(quicktime_t  *file);

/* Starting position                                                          */
/* If hpan and/or vpan != NULL they return the degree value as float          */
void lqt_qtvr_set_initial_pan(quicktime_t  *file, float hpan, float vpan);
int lqt_qtvr_get_initial_pan(quicktime_t  *file, float *hpan, float *vpan);


/* panning, fov, movietype are optional parameters to define player behavior  */

/* Object movies:                                                             */
/* get/set the panning/zoom settings                                          */
/* starthpan == 0 and endhpan == 360 means continous horiz. panning           */
/* Most (all?) object movies use these values:                                */
/* starthpan = 0 (angle of thehorizontal starting position)                   */
/* endhpan = 360                                                              */
/* startvpan = 90                                                             */
/* endvpan = -90                                                              */
/* minzoom/maxzoom are ignored                                                */
/*                                                                            */
/* Note: In Apples Player they affect the mouse sensivity.                    */

/* Panoramas:                                                                 */
/* starthpan = 0                                                              */
/* endhpan = 360                                                              */
/* startvpan = ???                                                            */
/* endvpan = ???                                                              */
/* minzoom/maxzoom: restrict zoom levels (Panorama only)                      */
/*                                                                            */
/* Note: For panoramas endvpan/startvpan have to be adjusted for every movie  */

/* To be ignore a parameter set it to NULL                                    */
void lqt_qtvr_get_extra_settings(quicktime_t  *file,
				 float *starthpan,
				 float *endhpan,
				 float *startvpan,
				 float *endvpan,
				 float *minzoom,
				 float *maxzoom);

void lqt_qtvr_set_extra_settings(quicktime_t  *file, 
				 float starthpan,
				 float endhpan,
				 float startvpan,
				 float endvpan,
				 float minzoom,
				 float maxzoom);


/* Object specific*/

/* get number of loop frames */
int lqt_qtvr_get_loop_frames(quicktime_t  *file);

/* get/set field of view                                                      */
/* mouse sensivity                                                            */
/* smaller == slower movement                                                 */
/* usually 180                                                                */
int lqt_qtvr_set_fov(quicktime_t  *file, float fov);
float lqt_qtvr_get_fov(quicktime_t  *file);

/* movietype */
/* get/set the player interface to use                                        */
/* These Interfaces are possible:                                             */
/* QTVR_STANDARD_OBJECT = Hand cursor, view rotates in movement direction     */
/* QTVR_OLD_NAVIGABLE_MOVIE_SCENE "Joystick interface", rotation in movement  */
/* direction                                                                  */
/* QTVR_OBJECT_IN_SCENE "Joystick Interface", rotation against movement       */
int lqt_qtvr_get_movietype(quicktime_t  *file);
int lqt_qtvr_set_movietype(quicktime_t  *file, int movietype);


/* Panorama specific */

/* return/set the dimensions of the player window */
/* returns */
int lqt_qtvr_set_display_width(quicktime_t  *file, int width);
int lqt_qtvr_set_display_height(quicktime_t  *file, int height);
int lqt_qtvr_get_display_width(quicktime_t  *file);
int lqt_qtvr_get_display_height(quicktime_t  *file);

/* get first (and only?) panorama track in file */
int lqt_qtvr_get_panorama_track(quicktime_t  *file);

/* get the scene track */
int lqt_qtvr_get_scene_track(quicktime_t  *file);

/* set the scene track */
/* Note: this also deactivates the track and reactivates the previously set   */
/*       track                                                                */
int lqt_qtvr_set_scene_track(quicktime_t  *file, int track);

/* write a dummy node into panorama track */
int lqt_qtvr_write_dummy_node(quicktime_t *file);

#ifdef __cplusplus
}
#endif /* __cplusplus */

  
#endif
