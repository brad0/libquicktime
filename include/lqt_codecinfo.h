/* This file was automatically generated.  Do not edit! */
lqt_codec_info_t *lqt_find_video_codec(char *fourcc,int encode);
lqt_codec_info_t *lqt_find_audio_codec(char *fourcc,int encode);
void lqt_dump_codec_info(const lqt_codec_info_t *info);
lqt_codec_info_t *lqt_create_codec_info(const lqt_codec_info_static_t *template);
lqt_codec_info_t *lqt_get_video_codec_info_c(int index);
lqt_codec_info_t *lqt_get_audio_codec_info_c(int index);
const lqt_codec_info_t *lqt_get_video_codec_info(int index);
const lqt_codec_info_t *lqt_get_audio_codec_info(int index);
int lqt_get_num_video_codecs();
int lqt_get_num_audio_codecs();
void lqt_write_codec_file(const char *filename);
lqt_codec_info_t *lqt_read_codec_file(const char *filename);
void lqt_registry_init();
void lqt_registry_destroy();
void lqt_destroy_codec_info(lqt_codec_info_t *ptr);
void destroy_parameter_info(lqt_parameter_info_t *p);
void lqt_registry_unlock();
void lqt_registry_lock();
extern pthread_mutex_t codecs_mutex;
extern lqt_codec_info_t *lqt_video_codecs;
extern lqt_codec_info_t *lqt_audio_codecs;
extern int lqt_num_video_codecs;
extern int lqt_num_audio_codecs;
int lqt_get_codec_api_version();