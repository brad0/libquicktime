/* This file was automatically generated.  Do not edit! */
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_write_int32(quicktime_t *file,long value);
int quicktime_write_int24(quicktime_t *file,long number);
int quicktime_write_char(quicktime_t *file,char x);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_stss(quicktime_t *file,quicktime_stss_t *stss);
long quicktime_read_int32(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
int quicktime_read_char(quicktime_t *file);
void quicktime_read_stss(quicktime_t *file,quicktime_stss_t *stss);
void quicktime_stss_dump(quicktime_stss_t *stss);
void quicktime_stss_delete(quicktime_stss_t *stss);
void quicktime_stss_init(quicktime_stss_t *stss);
