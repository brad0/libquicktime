/* This file was automatically generated.  Do not edit! */
void quicktime_update_stsz(quicktime_stsz_t *stsz,long sample,long sample_size);
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_write_int32(quicktime_t *file,long value);
int quicktime_write_int24(quicktime_t *file,long number);
int quicktime_write_char(quicktime_t *file,char x);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_stsz(quicktime_t *file,quicktime_stsz_t *stsz);
long quicktime_read_int32(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
int quicktime_read_char(quicktime_t *file);
void quicktime_read_stsz(quicktime_t *file,quicktime_stsz_t *stsz);
void quicktime_stsz_dump(quicktime_stsz_t *stsz);
void quicktime_stsz_delete(quicktime_stsz_t *stsz);
void quicktime_stsz_init_audio(quicktime_t *file,quicktime_stsz_t *stsz,int channels,int bits,char *compressor);
void quicktime_stsz_init_video(quicktime_t *file,quicktime_stsz_t *stsz);
void quicktime_stsz_init(quicktime_stsz_t *stsz);
