/* This file was automatically generated.  Do not edit! */
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_write_int16(quicktime_t *file,int number);
int quicktime_write_int24(quicktime_t *file,long number);
int quicktime_write_char(quicktime_t *file,char x);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_smhd(quicktime_t *file,quicktime_smhd_t *smhd);
int quicktime_read_int16(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
int quicktime_read_char(quicktime_t *file);
void quicktime_read_smhd(quicktime_t *file,quicktime_smhd_t *smhd);
void quicktime_smhd_dump(quicktime_smhd_t *smhd);
void quicktime_smhd_delete(quicktime_smhd_t *smhd);
void quicktime_smhd_init(quicktime_smhd_t *smhd);
