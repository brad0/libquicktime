#include <dlfcn.h>
#include <quicktime/colormodels.h>
#include <quicktime/lqt.h>
#include <funcprotos.h>
#include <lqt_codecinfo_private.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

static int quicktime_delete_vcodec_stub(quicktime_video_map_t *vtrack)
{
	printf("quicktime_delete_vcodec_stub called\n");
	return 0;
}

static int quicktime_delete_acodec_stub(quicktime_audio_map_t *atrack)
{
	printf("quicktime_delete_acodec_stub called\n");
	return 0;
}

static int quicktime_decode_video_stub(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track)
{
	printf("quicktime_decode_video_stub called\n");
	return 1;
}

static int quicktime_encode_video_stub(quicktime_t *file, 
				unsigned char **row_pointers, 
				int track)
{
	printf("quicktime_encode_video_stub called\n");
	return 1;
}

static int quicktime_decode_audio_stub(quicktime_t *file, 
                                       int16_t ** output_i, 
                                       float ** output_f, 
                                       long samples,
                                       int track)
{
	printf("quicktime_decode_audio_stub called\n");
	return 0;
}

static int quicktime_encode_audio_stub(quicktime_t *file, 
				int16_t **input_i, 
				float **input_f, 
				int track, 
				long samples)
{
	printf("quicktime_encode_audio_stub called\n");
	return 1;
}


static int quicktime_reads_colormodel_stub(quicktime_t *file, 
		int colormodel, 
		int track)
{
	fprintf(stderr, "quicktime_reads_colormodel_stub called\n");
	return (colormodel == BC_RGB888);
}

static int quicktime_writes_colormodel_stub(quicktime_t *file, 
		int colormodel, 
		int track)
{
	return (colormodel == BC_RGB888);
}

static void quicktime_flush_codec_stub(quicktime_t *file, int track)
{
}

/* Convert Microsoft audio id to codec */
void quicktime_id_to_codec(char *result, int id)
{
        switch(id)
        {
                case 0x55:
                        memcpy(result, QUICKTIME_MP3, 4);
                        break;
                case 0x161:
                        memcpy(result, QUICKTIME_WMA, 4);
                        break;
                default:
                        printf("quicktime_id_to_codec: unknown audio id: %p\n", (void*)id);
                        break;
        }
}

int quicktime_codec_to_id(char *codec)
{
        if(quicktime_match_32(codec, QUICKTIME_MP3))
                return 0x55;
        else
        if(quicktime_match_32(codec, QUICKTIME_WMA))
                return 0x161;
        else
          {
                printf("quicktime_codec_to_id: unknown codec %c%c%c%c\n", codec[0], codec[1], codec[2], codec[3]);
                return -1;
          }
        
}


int quicktime_codec_defaults(quicktime_codec_t *codec)
{
	codec->delete_vcodec = quicktime_delete_vcodec_stub;
	codec->delete_acodec = quicktime_delete_acodec_stub;
	codec->decode_video = quicktime_decode_video_stub;
	codec->encode_video = quicktime_encode_video_stub;
	codec->decode_audio = quicktime_decode_audio_stub;
	codec->encode_audio = quicktime_encode_audio_stub;
	codec->reads_colormodel = quicktime_reads_colormodel_stub;
	codec->writes_colormodel = quicktime_writes_colormodel_stub;
	codec->flush = quicktime_flush_codec_stub;
	return 0;
}
#if 0 /* Not needed in libquicktime */
static int get_vcodec_index(char *compressor)
{
	int index;
/* Initialize internal codecs on the first call */
	if(quicktime_vcodec_size() == 0)
		quicktime_register_internal_vcodec();

/* Try internal codec */
	index = quicktime_find_vcodec(compressor);

//printf("get_vcodec_index %d\n", index);
/* Try external codec */
	if(index < 0)
	{
		index = quicktime_register_external_vcodec(compressor);
	}

	if(index < 0)
		return -1;
	return index;
}

static int get_acodec_index(char *compressor)
{
	int index;
/* Initialize internal codecs on the first call */
	if(quicktime_acodec_size() == 0)
		quicktime_register_internal_acodec();

/* Try internal codec */
	index = quicktime_find_acodec(compressor);

/* Try external codec */
	if(index < 0)
	{
		index = quicktime_register_external_acodec(compressor);
	}

	if(index < 0)
		return -1;
	return index;
}
#endif /* libquicktime */

/*
 *  Original quicktime4linux function changed for dynamic loading
 */

int quicktime_init_vcodec(quicktime_video_map_t *vtrack, int encode,
                          lqt_codec_info_t * codec_info)
  {
  lqt_codec_info_t ** codec_array = (lqt_codec_info_t**)0;
  
  lqt_init_video_codec_func_t init_codec;
  lqt_init_video_codec_func_t (*get_codec)(int);
    
  void * module = (void*)0;
  
  char *compressor = vtrack->track->mdia.minf.stbl.stsd.table[0].format;

  vtrack->codec = calloc(1, sizeof(quicktime_codec_t));
  quicktime_codec_defaults((quicktime_codec_t*)vtrack->codec);

  ((quicktime_codec_t*)vtrack->codec)->module = (void*)0;
  
  /* Try to find the codec */

  if(!codec_info)
    {
    
#ifndef NDEBUG
    fprintf(stderr, "Trying to find %s for fourcc \"%s\"...",
            (encode ? "Encoder" : "Decoder"), compressor);
#endif
    
    codec_array = lqt_find_video_codec(compressor, encode);
  
    if(!codec_array)
      {
#ifndef NDEBUG
      fprintf(stderr, "failed\n");
#endif
      return -1;
      }
    codec_info = *codec_array;
    }
  
#ifndef NDEBUG
  fprintf(stderr, "Found %s\n", codec_info->long_name);
#endif
  
  /* dlopen the module */
#ifndef NDEBUG
  fprintf(stderr, "Trying to load module %s...",
          codec_info->module_filename);
#endif
  module = dlopen(codec_info->module_filename, RTLD_NOW);
  
  if(!module)
    {
#ifndef NDEBUG
    fprintf(stderr, "failed, %s\n", dlerror());
#endif

    if(codec_array)
      lqt_destroy_codec_info(codec_array);

    return -1;
    }
#ifndef NDEBUG
  fprintf(stderr, "Success\n");
#endif
  
  ((quicktime_codec_t*)vtrack->codec)->codec_name =
    malloc(strlen(codec_info->name)+1);
  strcpy(((quicktime_codec_t*)vtrack->codec)->codec_name, codec_info->name);
  
  /* Set the module */
  
  ((quicktime_codec_t*)vtrack->codec)->module = module;
  
  /* Get the codec finder for the module */
  
  get_codec =
    (lqt_init_video_codec_func_t(*)(int))dlsym(module,
                                               "get_video_codec");
  
  if(!get_codec)
    {
    fprintf(stderr, "Module %s contains no function get_video_codec",
            codec_info->module_filename);
    if(codec_array)
      lqt_destroy_codec_info(codec_array);

    return -1;
    }
  
  /* Obtain the initializer for the actual codec */
  
  init_codec = get_codec(codec_info->module_index);
  
  init_codec(vtrack);
  if(codec_array)
    lqt_destroy_codec_info(codec_array);

  //  vtrack->stream_cmodel = lqt_get_decoder_colormodel(quicktime_t * file, int track);
  
  return 0;
  
  }

int quicktime_init_acodec(quicktime_audio_map_t *atrack, int encode,
                          lqt_codec_info_t * codec_info)
  {
  lqt_codec_info_t ** codec_array = (lqt_codec_info_t**)0;
  lqt_init_audio_codec_func_t init_codec;
  lqt_init_audio_codec_func_t (*get_codec)(int);
    
  void * module;

  char *compressor = atrack->track->mdia.minf.stbl.stsd.table[0].format;
  int wav_id       = atrack->track->mdia.minf.stbl.stsd.table[0].compression_id;
  atrack->codec = calloc(1, sizeof(quicktime_codec_t));
  quicktime_codec_defaults((quicktime_codec_t*)atrack->codec);

  ((quicktime_codec_t*)(atrack->codec))->module = (void*)0;
  
  /* Try to find the codec */

  if(!codec_info)
    {
    

    if(compressor && (*compressor != '\0'))
      {
#ifndef NDEBUG
    fprintf(stderr, "Trying to find Audio %s for fourcc \"%s\"...",
            (encode ? "Encoder" : "Decoder"), compressor);
#endif
      codec_array = lqt_find_audio_codec(compressor, encode);
      }
    else if(wav_id)
      {
#ifndef NDEBUG
      fprintf(stderr, "Trying to find Audio %s for wav_id 0x%02x...",
              (encode ? "Encoder" : "Decoder"), wav_id);
#endif
      codec_array = lqt_find_audio_codec_by_wav_id(wav_id, encode);
      }
    if(!codec_array)
      {
#ifndef NDEBUG
      fprintf(stderr, "failed\n");
#endif
      return -1;
      }
    codec_info = *codec_array;
    }
  
#ifndef NDEBUG
  fprintf(stderr, "Found %s\n", codec_info->long_name);
#endif
  
  /* dlopen the module */
#ifndef NDEBUG
  fprintf(stderr, "Trying to load module %s...",
          codec_info->module_filename);
#endif
  module = dlopen(codec_info->module_filename, RTLD_NOW);
  
  if(!module)
    {
#ifndef NDEBUG
    fprintf(stderr, "failed, %s\n", dlerror());
#endif
    if(codec_array)
      lqt_destroy_codec_info(codec_array);
    return -1;
    }
#ifndef NDEBUG
  fprintf(stderr, "Success\n");
#endif

  ((quicktime_codec_t*)atrack->codec)->codec_name = malloc(strlen(codec_info->name)+1);
  strcpy(((quicktime_codec_t*)atrack->codec)->codec_name, codec_info->name);
    
  /* Set the module */
  
  ((quicktime_codec_t*)((quicktime_codec_t*)atrack->codec))->module = module;
  
  /* Get the codec finder for the module */
  
  get_codec =
    (lqt_init_audio_codec_func_t(*)(int))dlsym(module,
                                               "get_audio_codec");
  
  if(!get_codec)
    {
    fprintf(stderr, "Module %s contains no function get_audio_codec",
            codec_info->module_filename);
    if(codec_array)
      lqt_destroy_codec_info(codec_array);
    return -1;
    }
  
  /* Obtain the initializer for the actual codec */

#ifndef NDEBUG
  fprintf(stderr, "Creating codec %s, module %s, index %d\n",
          codec_info->name, codec_info->module_filename,
          codec_info->module_index);
#endif
  init_codec = get_codec(codec_info->module_index);
  
  init_codec(atrack);

  /* We set the wav ids from our info structure, so we don't have to do this
     in the plugin sources */

  if(codec_info->num_wav_ids)
    ((quicktime_codec_t*)((quicktime_codec_t*)atrack->codec))->wav_id = codec_info->wav_ids[0];
  
  if(codec_array)
    lqt_destroy_codec_info(codec_array);
  return 0;
  }


int quicktime_delete_vcodec(quicktime_video_map_t *vtrack)
  {
  ((quicktime_codec_t*)vtrack->codec)->delete_vcodec(vtrack);
  /* Close the module */
  
#ifndef NDEBUG
  fprintf(stderr, "Closing module...");
#endif
  if(((quicktime_codec_t*)vtrack->codec)->module)
    dlclose(((quicktime_codec_t*)vtrack->codec)->module);
#ifndef NDEBUG
  fprintf(stderr, "done\n");
#endif
  if(((quicktime_codec_t*)vtrack->codec)->codec_name)
    free(((quicktime_codec_t*)vtrack->codec)->codec_name);
  free(vtrack->codec);
  vtrack->codec = 0;
  return 0;
}

int quicktime_delete_acodec(quicktime_audio_map_t *atrack)
  {
  ((quicktime_codec_t*)atrack->codec)->delete_acodec(atrack);
  /* Close the module */
#ifndef NDEBUG
  fprintf(stderr, "Closing module...");
#endif
  if(((quicktime_codec_t*)atrack->codec)->module)
    dlclose(((quicktime_codec_t*)atrack->codec)->module);
#ifndef NDEBUG
  fprintf(stderr, "done\n");
#endif
  if(((quicktime_codec_t*)atrack->codec)->codec_name)
    free(((quicktime_codec_t*)atrack->codec)->codec_name);
  free(atrack->codec);
  atrack->codec = 0;
  return 0;
  }

int quicktime_supported_video(quicktime_t *file, int track)
{
	char *compressor = quicktime_video_compressor(file, track);
        lqt_codec_info_t ** test_codec =
          lqt_find_video_codec(compressor, file->wr);
        if(!test_codec)
          return 0;
        
        lqt_destroy_codec_info(test_codec);
	return 1;
}

int quicktime_supported_audio(quicktime_t *file, int track)
{
         lqt_codec_info_t ** test_codec;
         char *compressor = quicktime_audio_compressor(file, track);

         test_codec = (lqt_codec_info_t**)0;
         
         if(compressor && (*compressor != '\0'))
           test_codec = lqt_find_audio_codec(compressor, file->wr);
         else if(lqt_is_avi(file))
           test_codec = lqt_find_audio_codec_by_wav_id(lqt_get_wav_id(file, track), file->wr);
         
         if(!test_codec)
           return 0;
        
        lqt_destroy_codec_info(test_codec);
        return 1;
}

void lqt_update_frame_position(quicktime_video_map_t * track)
  {
  track->timestamp +=
    track->track->mdia.minf.stbl.stts.table[track->stts_index].sample_duration;

  track->stts_count++;

  if(track->stts_count >=
     track->track->mdia.minf.stbl.stts.table[track->stts_index].sample_count)
    {
    track->stts_index++;
    track->stts_count = 0;
    }
  track->current_position++;
  }

/*
 *  Same as quicktime_decode_video but doesn't force BC_RGB888
 */

int lqt_decode_video(quicktime_t *file,
                     unsigned char **row_pointers, int track)
{
	int result;
        int height;
	int width;
        
	height = quicktime_video_height(file, track);
	width =  quicktime_video_width(file, track);

//printf("quicktime_decode_video 1\n");
// Fake scaling parameters
      
        if(file->vtracks[track].io_cmodel != file->vtracks[track].stream_cmodel)
          {
          if(!file->vtracks[track].temp_frame)
            {
            file->vtracks[track].temp_frame =
              lqt_rows_alloc(width, height, file->vtracks[track].stream_cmodel,
                             &(file->vtracks[track].stream_row_span),
                             &(file->vtracks[track].stream_row_span_uv));
            }
          result =
            ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file,
                                                                           file->vtracks[track].temp_frame,
                                                                           track);
          cmodel_transfer(row_pointers,                    //    unsigned char **output_rows, /* Leave NULL if non existent */
                          file->vtracks[track].temp_frame, //    unsigned char **input_rows,
                          0, //                                  int in_x,        /* Dimensions to capture from input frame */
                          0, //                                  int in_y, 
                          width, //                              int in_w, 
                          height, //                             int in_h,
                          width, //                              int out_w, 
                          height, //                             int out_h,
                          file->vtracks[track].stream_cmodel, // int in_colormodel, 
                          file->vtracks[track].io_cmodel,     // int out_colormodel,
                          0, //                                  int bg_color,
                          file->vtracks[track].stream_row_span,   /* For planar use the luma rowspan */
                          file->vtracks[track].io_row_span,       /* For planar use the luma rowspan */
                          file->vtracks[track].stream_row_span_uv, /* Chroma rowspan */
                          file->vtracks[track].io_row_span_uv      /* Chroma rowspan */);
         
          }
        else
          {
          file->vtracks[track].stream_row_span    = file->vtracks[track].io_row_span;
          file->vtracks[track].stream_row_span_uv = file->vtracks[track].io_row_span_uv;
          
          result = ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file, row_pointers, track);
          
          }
        
        //printf("quicktime_decode_video 1\n");
        lqt_update_frame_position(&(file->vtracks[track]));
//printf("quicktime_decode_video 2\n");
	return result;
}

/* The original function, which forces BG_RGB888 */
int quicktime_decode_video(quicktime_t *file,
                           unsigned char **row_pointers, int track)
  {
  
  file->vtracks[track].io_cmodel = BC_RGB888;
  return lqt_decode_video(file, row_pointers, track);
  }


long quicktime_decode_scaled(quicktime_t *file, 
	int in_x,                    /* Location of input frame to take picture */
	int in_y,
	int in_w,
	int in_h,
	int out_w,                   /* Dimensions of output frame */
	int out_h,
	int color_model,             /* One of the color models defined above */
	unsigned char **row_pointers, 
	int track)
{
	int result;

        int height;
	int width;
        
	height = quicktime_video_height(file, track);
	width =  quicktime_video_width(file, track);

        
	file->vtracks[track].io_cmodel = color_model;

        if(!file->vtracks[track].temp_frame)
          {
          file->vtracks[track].temp_frame =
            lqt_rows_alloc(width, height, file->vtracks[track].stream_cmodel,
                           &(file->vtracks[track].stream_row_span),
                           &(file->vtracks[track].stream_row_span_uv));
          }
        result =
          ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file,
                                                                         file->vtracks[track].temp_frame,
                                                                         track);
        cmodel_transfer(row_pointers,                    //    unsigned char **output_rows, /* Leave NULL if non existent */
                        file->vtracks[track].temp_frame, //    unsigned char **input_rows,
                        in_x, //                               int in_x,        /* Dimensions to capture from input frame */
                        in_y, //                               int in_y, 
                        in_w, //                               int in_w, 
                        in_h, //                               int in_h,
                        out_w, //                              int out_w, 
                        out_h, //                              int out_h,
                        file->vtracks[track].stream_cmodel, // int in_colormodel, 
                        file->vtracks[track].io_cmodel,     // int out_colormodel,
                        0, //                                  int bg_color,
                        file->vtracks[track].stream_row_span,   /* For planar use the luma rowspan */
                        file->vtracks[track].io_row_span,       /* For planar use the luma rowspan */
                        file->vtracks[track].stream_row_span_uv, /* Chroma rowspan */
                        file->vtracks[track].io_row_span_uv      /* Chroma rowspan */);
        
        lqt_update_frame_position(&(file->vtracks[track]));
	return result;
}

static int do_encode_video(quicktime_t *file, 
                           unsigned char **row_pointers, 
                           int track)
  {
  int result;

  int height;
  int width;
  
  height = quicktime_video_height(file, track);
  width =  quicktime_video_width(file, track);
  
  //printf("quicktime_decode_video 1\n");
  // Fake scaling parameters
  
  if(file->vtracks[track].io_cmodel != file->vtracks[track].stream_cmodel)
    {
    if(!file->vtracks[track].temp_frame)
      {
      file->vtracks[track].temp_frame =
        lqt_rows_alloc(width, height, file->vtracks[track].stream_cmodel,
                       &(file->vtracks[track].stream_row_span),
                       &(file->vtracks[track].stream_row_span_uv));
      }
    result =
      ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file,
                                                                     file->vtracks[track].temp_frame,
                                                                     track);
    cmodel_transfer(file->vtracks[track].temp_frame, //    unsigned char **output_rows, /* Leave NULL if non existent */
                    row_pointers,                    //    unsigned char **input_rows,
                    0, //                                  int in_x,        /* Dimensions to capture from input frame */
                    0, //                                  int in_y, 
                    width, //                              int in_w, 
                    height, //                             int in_h,
                    width, //                              int out_w, 
                    height, //                             int out_h,
                    file->vtracks[track].io_cmodel, // int in_colormodel, 
                    file->vtracks[track].stream_cmodel,     // int out_colormodel,
                    0, //                                  int bg_color,
                    file->vtracks[track].io_row_span,   /* For planar use the luma rowspan */
                    file->vtracks[track].stream_row_span,       /* For planar use the luma rowspan */
                    file->vtracks[track].io_row_span_uv, /* Chroma rowspan */
                    file->vtracks[track].stream_row_span_uv      /* Chroma rowspan */);
    result = ((quicktime_codec_t*)file->vtracks[track].codec)->encode_video(file, file->vtracks[track].temp_frame, track);
    }
  else
    {
    file->vtracks[track].stream_row_span    = file->vtracks[track].io_row_span;
    file->vtracks[track].stream_row_span_uv = file->vtracks[track].io_row_span_uv;
    result = ((quicktime_codec_t*)file->vtracks[track].codec)->encode_video(file, row_pointers, track);
    }
  return result;
  }

int quicktime_encode_video(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track)
{
	int result;

        result = do_encode_video(file, row_pointers, track);
        
        quicktime_update_stts(&file->vtracks[track].track->mdia.minf.stbl.stts,
                              file->vtracks[track].current_position, 0);
        file->vtracks[track].current_position++;
	return result;
}

int lqt_encode_video(quicktime_t *file, 
                     unsigned char **row_pointers, 
                     int track, int64_t time)
{
	int result;
        result = do_encode_video(file, row_pointers, track);
        
        if(file->vtracks[track].current_position)
          quicktime_update_stts(&file->vtracks[track].track->mdia.minf.stbl.stts,
                                file->vtracks[track].current_position - 1,
                                time - file->vtracks[track].timestamp);
        else
          quicktime_update_stts(&file->vtracks[track].track->mdia.minf.stbl.stts,
                                file->vtracks[track].current_position, 0);

        file->vtracks[track].timestamp = time;
        file->vtracks[track].current_position++;
	return result;
}



int quicktime_decode_audio(quicktime_t *file, 
                           int16_t *output_i, 
                           float *output_f, 
                           long samples, 
                           int channel)
{
        float   ** channels_f;
        int16_t ** channels_i;

        int quicktime_track, quicktime_channel;
	int result = 1;

        quicktime_channel_location(file, &quicktime_track,
                                   &quicktime_channel, channel);

        if(file->atracks[quicktime_track].eof)
          return 1;
        
        if(output_i)
          {
          channels_i = calloc(quicktime_track_channels(file, quicktime_track), sizeof(*channels_i));
          channels_i[quicktime_channel] = output_i;
          }
        else
          channels_i = (int16_t**)0;
        
        if(output_f)
          {
          channels_f = calloc(quicktime_track_channels(file, quicktime_track), sizeof(*channels_f));
          channels_f[quicktime_channel] = output_f;
          }
        else
          channels_f = (float**)0;
        
	result = ((quicktime_codec_t*)file->atracks[quicktime_track].codec)->decode_audio(file, 
                                                                                          channels_i, 
                                                                                          channels_f, 
                                                                                          samples,
                                                                                          quicktime_track);
	file->atracks[quicktime_track].current_position += result;

        if(channels_i)
          free(channels_i);
        else if(channels_f)
          free(channels_f);
	return ((result < 0) ? 1 : 0);
}

int lqt_total_channels(quicktime_t *file)
{
	int i = 0, total_channels = 0;
	for( i=0; i < file->total_atracks; i++ )
	{
		total_channels += file->atracks[i].channels;
	}

	return total_channels;
}

/*
 * Same as quicktime_decode_audio, but it grabs all channels at
 * once. Or if you want only some channels you can leave the channels
 * you don't want = NULL in the poutput array. The poutput arrays
 * must contain at least lqt_total_channels(file) elements.
 */

int lqt_decode_audio(quicktime_t *file, 
				int16_t **poutput_i, 
				float **poutput_f, 
				long samples)
{
	int result = 1;
	int i = 0;
	int16_t **output_i;
	float   **output_f;

	int total_tracks = quicktime_audio_tracks(file);
        int track_channels;

        if(poutput_i)
          output_i = poutput_i;
        else
          output_i = (int16_t**)0;

        if(poutput_f)
          output_f = poutput_f;
        else
          output_f = (float**)0;
        
	for( i=0; i < total_tracks; i++ )
          {
          track_channels = quicktime_track_channels(file, i);

          if(file->atracks[i].eof)
            return 1;
                    
          result = ((quicktime_codec_t*)file->atracks[i].codec)->decode_audio(
                                                                              file, 
                                                                              output_i, 
                                                                              output_f, 
                                                                              samples, 
                                                                              i);
          if(output_f)
            output_f += track_channels;
          if(output_i)
            output_i += track_channels;

          file->atracks[i].current_position += samples;
          }


	return result;
}

int lqt_decode_audio_track(quicktime_t *file, 
                           int16_t **poutput_i, 
                           float **poutput_f, 
                           long samples,
                           int track)
  {
  int result = 1;
  //  fprintf(stderr, "lqt_decode_audio_track\n");

  if(file->atracks[track].eof)
    return 1;
          
  
  result = !(((quicktime_codec_t*)file->atracks[track].codec)->decode_audio(
                                                                           file, 
                                                                           poutput_i, 
                                                                           poutput_f, 
                                                                           samples, 
                                                                           track));
  
  file->atracks[track].current_position += samples;
  
  return result;
  }

int lqt_encode_audio_track(quicktime_t *file, 
                           int16_t **input_i, 
                           float **input_f, 
                           long samples,
                           int track)
  {
	int result = 1;

	result = ((quicktime_codec_t*)file->atracks[track].codec)->encode_audio(file, input_i, 
                                                                            input_f, track, samples);
	file->atracks[track].current_position += samples;

	return result;
  
  }

int quicktime_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f, long samples)
  {
  return lqt_encode_audio_track(file, input_i, input_f, samples, 0);
  }

int quicktime_reads_cmodel(quicktime_t *file, 
	int colormodel, 
	int track)
{
	int result = ((quicktime_codec_t*)file->vtracks[track].codec)->reads_colormodel(file, colormodel, track);
/*         fprintf(stderr, "quicktime_reads_cmodel: %s\n", lqt_colormodel_to_string(colormodel)); */
	return result;
}

int quicktime_writes_cmodel(quicktime_t *file, 
	int colormodel, 
	int track)
{
	return ((quicktime_codec_t*)file->vtracks[track].codec)->writes_colormodel(file, colormodel, track);
}

/* Compressors that can only encode a window at a time */
/* need to flush extra data here. */

int quicktime_flush_acodec(quicktime_t *file, int track)
{
	((quicktime_codec_t*)file->atracks[track].codec)->flush(file, track);
	return 0;
};

void quicktime_flush_vcodec(quicktime_t *file, int track)
{
	((quicktime_codec_t*)file->vtracks[track].codec)->flush(file, track);
}

#if 0
int64_t quicktime_samples_to_bytes(quicktime_trak_t *track, long samples)
{
	char *compressor = track->mdia.minf.stbl.stsd.table[0].format;
	int channels = track->mdia.minf.stbl.stsd.table[0].channels;

	if(quicktime_match_32(compressor, QUICKTIME_IMA4)) 
		return samples * channels;

	if(quicktime_match_32(compressor, QUICKTIME_ULAW)) 
		return samples * channels;

/* Default use the sample size specification for TWOS and RAW */
	return samples * channels * track->mdia.minf.stbl.stsd.table[0].sample_size / 8;
}
#endif
int quicktime_codecs_flush(quicktime_t *file)
{
	int result = 0;
	int i;
	if(!file->wr) return result;

	if(file->total_atracks)
	{
		for(i = 0; i < file->total_atracks && !result; i++)
		{
			quicktime_flush_acodec(file, i);
		}
	}

	if(file->total_vtracks)
	{
		for(i = 0; i < file->total_vtracks && !result; i++)
		{
			quicktime_flush_vcodec(file, i);
		}
	}
	return result;
}

/* Copy audio */

int lqt_copy_audio(int16_t ** dst_i, float ** dst_f,
                   int16_t ** src_i, float ** src_f,
                   int dst_pos, int src_pos,
                   int dst_size, int src_size, int num_channels)
  {
  
  int i, j, i_tmp;
  int samples_to_copy;
  samples_to_copy = src_size < dst_size ? src_size : dst_size;

  //  fprintf(stderr, "lqt copy audio %d, src_pos: %d, dst_pos: %d, src_size: %d, dst_size: %d, samples_to_copy: %d\n",
  //          num_channels, src_pos, dst_pos, src_size, dst_size, samples_to_copy);
  
  if(src_i)
    {
    for(i = 0; i < num_channels; i++)
      {
      if(dst_i && dst_i[i]) /* int -> int */
        {
        memcpy(dst_i[i] + dst_pos, src_i[i] + src_pos, samples_to_copy * sizeof(int16_t));
        }
      if(dst_f && dst_f[i]) /* int -> float */
        {
        for(j = 0; j < samples_to_copy; j++)
          {
          dst_f[i][dst_pos + j] = (float)src_i[i][src_pos + j] / 32767.0;
          }
        }
      }
    }
  else if(src_f)
    {
    for(i = 0; i < num_channels; i++)
      {
      if(dst_i && dst_i[i]) /* float -> int */
        {
        for(j = 0; j < samples_to_copy; j++)
          {
          i_tmp = (int)(src_f[i][src_pos + j] * 32767.0);
          
          if(i_tmp > 32767)
            i_tmp = 32767;

          if(i_tmp < -32768)
            i_tmp = -32768;
          
          dst_i[i][dst_pos + j] = i_tmp;
          }
        }
      if(dst_f && dst_f[i]) /* float -> float */
        {
        memcpy(dst_f[i] + dst_pos, src_f[i] + src_pos, samples_to_copy * sizeof(float));
        }
      }
    }
  return samples_to_copy;
  }
