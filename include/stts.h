/* This file was automatically generated.  Do not edit! */
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_write_int32(quicktime_t *file,long value);
int quicktime_write_int24(quicktime_t *file,long number);
int quicktime_write_char(quicktime_t *file,char x);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_stts(quicktime_t *file,quicktime_stts_t *stts);
long quicktime_read_int32(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
int quicktime_read_char(quicktime_t *file);
void quicktime_read_stts(quicktime_t *file,quicktime_stts_t *stts);
void quicktime_stts_dump(quicktime_stts_t *stts);
void quicktime_stts_delete(quicktime_stts_t *stts);
void quicktime_stts_init_audio(quicktime_t *file,quicktime_stts_t *stts,int sample_rate);
void quicktime_stts_init_video(quicktime_t *file,quicktime_stts_t *stts,int time_scale,float frame_rate);
void quicktime_stts_init_table(quicktime_stts_t *stts);
void quicktime_stts_init(quicktime_stts_t *stts);
