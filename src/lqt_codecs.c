#include <dlfcn.h>
#include <quicktime/colormodels.h>
#include <quicktime/lqt.h>
#include <funcprotos.h>
#include <lqt_codecinfo_private.h>
#define LQT_LIBQUICKTIME
#include <quicktime/lqt_codecapi.h>
#include <string.h>

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
					int16_t *output_i, 
					float *output_f, 
					long samples, 
					int track, 
					int channel)
{
	printf("quicktime_decode_audio_stub called\n");
	return 1;
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

  atrack->codec = calloc(1, sizeof(quicktime_codec_t));
  quicktime_codec_defaults((quicktime_codec_t*)atrack->codec);

  ((quicktime_codec_t*)(atrack->codec))->module = (void*)0;
  
  /* Try to find the codec */

  if(!codec_info)
    {
    
#ifndef NDEBUG
    fprintf(stderr, "Trying to find Audio %s for fourcc \"%s\"...",
            (encode ? "Encoder" : "Decoder"), compressor);
#endif
    
    codec_array = lqt_find_audio_codec(compressor, encode);
    
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
  
  init_codec = get_codec(codec_info->module_index);
  
  init_codec(atrack);

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
	if(!lqt_find_video_codec(compressor, file->wr)) return 0;
	return 1;
}

int quicktime_supported_audio(quicktime_t *file, int track)
{
	char *compressor = quicktime_audio_compressor(file, track);
	if(!lqt_find_audio_codec(compressor, file->wr)) return 0;
	return 1;
}

int quicktime_decode_video(quicktime_t *file,
                           unsigned char **row_pointers, int track)
{
	int result;
	quicktime_trak_t *trak = file->vtracks[track].track;
	int track_height = trak->tkhd.track_height;
	int track_width = trak->tkhd.track_width;

//printf("quicktime_decode_video 1\n");
// Fake scaling parameters
	file->do_scaling = 0;
	file->color_model = BC_RGB888;
	file->in_x = 0;
	file->in_y = 0;
	file->in_w = track_width;
	file->in_h = track_height;
	file->out_w = track_width;
	file->out_h = track_height;

//printf("quicktime_decode_video 1\n");
	result = ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file, row_pointers, track);
	file->vtracks[track].current_position++;
//printf("quicktime_decode_video 2\n");
	return result;
}

/*
 *  Same as quicktime_decode_video but doesn't force BC_RGB888
 */

int lqt_decode_video(quicktime_t *file,
                     unsigned char **row_pointers, int track)
{
	int result;
	quicktime_trak_t *trak = file->vtracks[track].track;
	int track_height = trak->tkhd.track_height;
	int track_width = trak->tkhd.track_width;

//printf("quicktime_decode_video 1\n");
// Fake scaling parameters
	file->do_scaling = 0;
	file->in_x = 0;
	file->in_y = 0;
	file->in_w = track_width;
	file->in_h = track_height;
	file->out_w = track_width;
	file->out_h = track_height;

//printf("quicktime_decode_video 1\n");
	result = ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file, row_pointers, track);
	file->vtracks[track].current_position++;
//printf("quicktime_decode_video 2\n");
	return result;
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

	file->do_scaling = 1;
	file->color_model = color_model;
	file->in_x = in_x;
	file->in_y = in_y;
	file->in_w = in_w;
	file->in_h = in_h;
	file->out_w = out_w;
	file->out_h = out_h;

	result = ((quicktime_codec_t*)file->vtracks[track].codec)->decode_video(file, row_pointers, track);
	file->vtracks[track].current_position++;
	return result;
}


int quicktime_encode_video(quicktime_t *file, 
	unsigned char **row_pointers, 
	int track)
{
	int result;
//printf("quicktime_encode_video 1 %p\n", ((quicktime_codec_t*)file->vtracks[track].codec)->encode_video);
	result = ((quicktime_codec_t*)file->vtracks[track].codec)->encode_video(file, row_pointers, track);
//printf("quicktime_encode_video 2\n");
	file->vtracks[track].current_position++;
	return result;
}


int quicktime_decode_audio(quicktime_t *file, 
				int16_t *output_i, 
				float *output_f, 
				long samples, 
				int channel)
{
	int quicktime_track, quicktime_channel;
	int result = 1;

	quicktime_channel_location(file, &quicktime_track, &quicktime_channel, channel);
	result = ((quicktime_codec_t*)file->atracks[quicktime_track].codec)->decode_audio(file, 
				output_i, 
				output_f, 
				samples, 
				quicktime_track, 
				quicktime_channel);
	file->atracks[quicktime_track].current_position += samples;

	return result;
}

/* Since all channels are written at the same time: */
/* Encode using the compressor for the first audio track. */
/* Which means all the audio channels must be on the same track. */

int quicktime_encode_audio(quicktime_t *file, int16_t **input_i, float **input_f, long samples)
{
	int result = 1;
	char *compressor = quicktime_audio_compressor(file, 0);

	result = ((quicktime_codec_t*)file->atracks[0].codec)->encode_audio(file, 
		input_i, 
		input_f,
		0, 
		samples);
	file->atracks[0].current_position += samples;

	return result;
}

int quicktime_reads_cmodel(quicktime_t *file, 
	int colormodel, 
	int track)
{
	int result = ((quicktime_codec_t*)file->vtracks[track].codec)->reads_colormodel(file, colormodel, track);
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

longest quicktime_samples_to_bytes(quicktime_trak_t *track, long samples)
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
