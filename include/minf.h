/* This file was automatically generated.  Do not edit! */
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
void quicktime_write_stbl(quicktime_t *file,quicktime_minf_t *minf,quicktime_stbl_t *stbl);
void quicktime_write_dinf(quicktime_t *file,quicktime_dinf_t *dinf);
void quicktime_write_hdlr(quicktime_t *file,quicktime_hdlr_t *hdlr);
void quicktime_write_smhd(quicktime_t *file,quicktime_smhd_t *smhd);
void quicktime_write_vmhd(quicktime_t *file,quicktime_vmhd_t *vmhd);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_minf(quicktime_t *file,quicktime_minf_t *minf);
longest quicktime_position(quicktime_t *file);
int quicktime_read_stbl(quicktime_t *file,quicktime_minf_t *minf,quicktime_stbl_t *stbl,quicktime_atom_t *parent_atom);
void quicktime_read_dinf(quicktime_t *file,quicktime_dinf_t *dinf,quicktime_atom_t *dinf_atom);
int quicktime_atom_skip(quicktime_t *file,quicktime_atom_t *atom);
void quicktime_read_hdlr(quicktime_t *file,quicktime_hdlr_t *hdlr);
void quicktime_read_smhd(quicktime_t *file,quicktime_smhd_t *smhd);
void quicktime_read_vmhd(quicktime_t *file,quicktime_vmhd_t *vmhd);
int quicktime_atom_is(quicktime_atom_t *atom,char *type);
int quicktime_atom_read_header(quicktime_t *file,quicktime_atom_t *atom);
int quicktime_read_minf(quicktime_t *file,quicktime_minf_t *minf,quicktime_atom_t *parent_atom);
void quicktime_stbl_dump(void *minf_ptr,quicktime_stbl_t *stbl);
void quicktime_dinf_dump(quicktime_dinf_t *dinf);
void quicktime_hdlr_dump(quicktime_hdlr_t *hdlr);
void quicktime_vmhd_dump(quicktime_vmhd_t *vmhd);
void quicktime_smhd_dump(quicktime_smhd_t *smhd);
void quicktime_minf_dump(quicktime_minf_t *minf);
void quicktime_hdlr_delete(quicktime_hdlr_t *hdlr);
void quicktime_stbl_delete(quicktime_stbl_t *stbl);
void quicktime_dinf_delete(quicktime_dinf_t *dinf);
void quicktime_smhd_delete(quicktime_smhd_t *smhd);
void quicktime_vmhd_delete(quicktime_vmhd_t *vmhd);
void quicktime_minf_delete(quicktime_minf_t *minf);
void quicktime_stbl_init_audio(quicktime_t *file,quicktime_stbl_t *stbl,int channels,int sample_rate,int bits,char *compressor);
void quicktime_minf_init_audio(quicktime_t *file,quicktime_minf_t *minf,int channels,int sample_rate,int bits,char *compressor);
void quicktime_dinf_init_all(quicktime_dinf_t *dinf);
void quicktime_hdlr_init_data(quicktime_hdlr_t *hdlr);
void quicktime_stbl_init_video(quicktime_t *file,quicktime_stbl_t *stbl,int frame_w,int frame_h,int time_scale,float frame_rate,char *compressor);
void quicktime_vmhd_init_video(quicktime_t *file,quicktime_vmhd_t *vmhd,int frame_w,int frame_h,float frame_rate);
void quicktime_minf_init_video(quicktime_t *file,quicktime_minf_t *minf,int frame_w,int frame_h,int time_scale,float frame_rate,char *compressor);
void quicktime_stbl_init(quicktime_stbl_t *stbl);
void quicktime_dinf_init(quicktime_dinf_t *dinf);
void quicktime_hdlr_init(quicktime_hdlr_t *hdlr);
void quicktime_smhd_init(quicktime_smhd_t *smhd);
void quicktime_vmhd_init(quicktime_vmhd_t *vmhd);
void quicktime_minf_init(quicktime_minf_t *minf);
