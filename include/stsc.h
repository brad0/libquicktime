/* This file was automatically generated.  Do not edit! */
int quicktime_update_stsc(quicktime_stsc_t *stsc,long chunk,long samples);
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_write_int32(quicktime_t *file,long value);
int quicktime_write_int24(quicktime_t *file,long number);
int quicktime_write_char(quicktime_t *file,char x);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_stsc(quicktime_t *file,quicktime_stsc_t *stsc);
long quicktime_read_int32(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
int quicktime_read_char(quicktime_t *file);
void quicktime_read_stsc(quicktime_t *file,quicktime_stsc_t *stsc);
void quicktime_stsc_dump(quicktime_stsc_t *stsc);
void quicktime_stsc_delete(quicktime_stsc_t *stsc);
void quicktime_stsc_init_audio(quicktime_t *file,quicktime_stsc_t *stsc,int sample_rate);
void quicktime_stsc_init_video(quicktime_t *file,quicktime_stsc_t *stsc);
void quicktime_stsc_init_table(quicktime_t *file,quicktime_stsc_t *stsc);
void quicktime_stsc_init(quicktime_stsc_t *stsc);
