/* This file was automatically generated.  Do not edit! */
int quicktime_atom_skip(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_atom_is(quicktime_atom_t *atom,char *type);
int quicktime_set_position(quicktime_t *file,longest position);
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
int quicktime_write_int64(quicktime_t *file,longest value);
int quicktime_write_char32(quicktime_t *file,char *string);
int quicktime_write_int32(quicktime_t *file,long value);
int quicktime_atom_write_header64(quicktime_t *file,quicktime_atom_t *atom,char *text);
longest quicktime_atom_read_size64(char *data);
int quicktime_match_32(char *input,char *output);
unsigned long quicktime_atom_read_size(char *data);
int quicktime_read_data(quicktime_t *file,char *data,longest size);
longest quicktime_position(quicktime_t *file);
int quicktime_atom_read_header(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_atom_read_type(char *data,char *type);
int quicktime_atom_reset(quicktime_atom_t *atom);
