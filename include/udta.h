/* This file was automatically generated.  Do not edit! */
int quicktime_set_udta_string(char **string,int *size,char *new_string);
int quicktime_write_data(quicktime_t *file,char *data,int size);
int quicktime_write_int16(quicktime_t *file,int number);
int quicktime_read_data(quicktime_t *file,char *data,longest size);
int quicktime_read_int16(quicktime_t *file);
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_write_udta_string(quicktime_t *file,char *string,int size);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_udta(quicktime_t *file,quicktime_udta_t *udta);
longest quicktime_position(quicktime_t *file);
int quicktime_atom_skip(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_read_udta_string(quicktime_t *file,char **string,int *size);
int quicktime_atom_is(quicktime_atom_t *atom,char *type);
int quicktime_atom_read_header(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_read_udta(quicktime_t *file,quicktime_udta_t *udta,quicktime_atom_t *udta_atom);
void quicktime_udta_dump(quicktime_udta_t *udta);
int quicktime_udta_delete(quicktime_udta_t *udta);
int quicktime_udta_init(quicktime_udta_t *udta);
