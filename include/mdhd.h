/* This file was automatically generated.  Do not edit! */
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_write_int16(quicktime_t *file,int number);
int quicktime_write_int32(quicktime_t *file,long value);
int quicktime_write_int24(quicktime_t *file,long number);
int quicktime_write_char(quicktime_t *file,char x);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_mdhd(quicktime_t *file,quicktime_mdhd_t *mdhd);
void quicktime_mdhd_dump(quicktime_mdhd_t *mdhd);
int quicktime_read_int16(quicktime_t *file);
long quicktime_read_int32(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
int quicktime_read_char(quicktime_t *file);
void quicktime_read_mdhd(quicktime_t *file,quicktime_mdhd_t *mdhd);
void quicktime_mdhd_delete(quicktime_mdhd_t *mdhd);
void quicktime_mdhd_init_audio(quicktime_t *file,quicktime_mdhd_t *mdhd,int channels,int sample_rate,int bits,char *compressor);
int quicktime_get_timescale(float frame_rate);
int quicktime_get_timescale(float frame_rate);
void quicktime_mdhd_init_video(quicktime_t *file,quicktime_mdhd_t *mdhd,int frame_w,int frame_h,float frame_rate);
unsigned long quicktime_current_time(void);
void quicktime_mdhd_init(quicktime_mdhd_t *mdhd);