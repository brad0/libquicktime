int quicktime_init_audio_map(quicktime_audio_map_t *atrack,
                             quicktime_trak_t *trak, int encode,
                             lqt_codec_info_t * info);
int quicktime_init_video_map(quicktime_video_map_t *vtrack,
                             quicktime_trak_t *trak, int encode,
                             lqt_codec_info_t * info);
int quicktime_init_vcodec(quicktime_video_map_t *vtrack, int encode,
                          lqt_codec_info_t * info);
int quicktime_init_acodec(quicktime_audio_map_t *atrack, int encode,
                          lqt_codec_info_t * info);


/*
 *  Set the default codec parameters from the registry
 *  Works for reading and writing
 */

void lqt_set_default_video_parameters(quicktime_t * file);
void lqt_set_default_audio_parameters(quicktime_t * file);
