/* This file was automatically generated.  Do not edit! */
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_write_int16(quicktime_t *file,int number);
int quicktime_write_int24(quicktime_t *file,long number);
int quicktime_write_char(quicktime_t *file,char x);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_vmhd(quicktime_t *file,quicktime_vmhd_t *vmhd);
int quicktime_read_int16(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
int quicktime_read_char(quicktime_t *file);
void quicktime_read_vmhd(quicktime_t *file,quicktime_vmhd_t *vmhd);
void quicktime_vmhd_dump(quicktime_vmhd_t *vmhd);
void quicktime_vmhd_delete(quicktime_vmhd_t *vmhd);
void quicktime_vmhd_init_video(quicktime_t *file,quicktime_vmhd_t *vmhd,int frame_w,int frame_h,float frame_rate);
void quicktime_vmhd_init(quicktime_vmhd_t *vmhd);
