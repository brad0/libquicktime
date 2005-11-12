#include <lqt.h>
#include <qtprivate.h>
/* atom.c */

int quicktime_atom_read_header(quicktime_t *file, quicktime_atom_t *atom);
int quicktime_atom_write_header64(quicktime_t *file, quicktime_atom_t *atom, char *text);
int quicktime_atom_write_header(quicktime_t *file, quicktime_atom_t *atom, char *text);
void quicktime_atom_write_footer(quicktime_t *file, quicktime_atom_t *atom);
int quicktime_atom_is(quicktime_atom_t *atom, char *type);
int quicktime_atom_skip(quicktime_t *file, quicktime_atom_t *atom);

/* audio.c */

void lqt_convert_audio_encode(int16_t ** in_int, float ** in_float,
                              void * out, int num_channels, int num_samples,
                              lqt_sample_format_t stream_format);

void lqt_convert_audio_decode(void * in, int16_t ** out_int,
                              float ** out_float, int num_channels, int num_samples,
                              lqt_sample_format_t stream_format);
 

/* avi_hdrl.c */

void quicktime_read_hdrl(quicktime_t *file, 
                         quicktime_hdrl_t *hdrl,
                         quicktime_atom_t *parent_atom);

void quicktime_write_hdrl(quicktime_t *file);

void quicktime_finalize_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl);
void quicktime_delete_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl);
void quicktime_init_hdrl(quicktime_t *file, quicktime_hdrl_t *hdrl);

/* avi_idx1.c */

void quicktime_read_idx1(quicktime_t *file,
                         quicktime_riff_t *riff,
                         quicktime_atom_t *parent_atom);

void quicktime_delete_idx1(quicktime_idx1_t *idx1);

void quicktime_write_idx1(quicktime_t *file,
                          quicktime_idx1_t *idx1);
void quicktime_set_idx1_keyframe(quicktime_t *file,
                                 quicktime_trak_t *trak,
                                 int new_keyframe);

void quicktime_update_idx1table(quicktime_t *file,
                                quicktime_trak_t *trak,
                                int offset,
                                int size);

/* avi_indx.c */

void quicktime_finalize_indx(quicktime_t *file);

void quicktime_delete_indx(quicktime_indx_t *indx);
void quicktime_init_indx(quicktime_t *file,
                         quicktime_indx_t *indx,
                         quicktime_strl_t *strl);

void quicktime_update_indx(quicktime_t *file,
                           quicktime_indx_t *indx,
                           quicktime_ix_t *ix);
void quicktime_read_indx(quicktime_t *file,
                         quicktime_strl_t *strl,
                         quicktime_atom_t *parent_atom);

/* avi_ix.c */

quicktime_ix_t* quicktime_new_ix(quicktime_t *file,
                                 quicktime_trak_t *trak,
                                 quicktime_strl_t *strl);
void quicktime_delete_ix(quicktime_ix_t *ix);
void quicktime_update_ixtable(quicktime_t *file,
                              quicktime_trak_t *trak,
                              int64_t offset,
                              int size);
void quicktime_write_ix(quicktime_t *file,
                        quicktime_ix_t *ix,
                        int track);

void quicktime_read_ix(quicktime_t *file,
                       quicktime_ix_t *ix);



/* avi_movi.c */

void quicktime_read_movi(quicktime_t *file, 
                         quicktime_atom_t *parent_atom,
                         quicktime_movi_t *movi);
void quicktime_write_movi(quicktime_t *file);
void quicktime_delete_movi(quicktime_t *file, quicktime_movi_t *movi);
void quicktime_init_movi(quicktime_t *file, quicktime_riff_t *riff);
void quicktime_finalize_movi(quicktime_t *file, quicktime_movi_t *movi);

/* avi_odml.c */

void quicktime_finalize_odml(quicktime_t *file, quicktime_hdrl_t *hdrl);
void quicktime_init_odml(quicktime_t *file, quicktime_hdrl_t *hdrl);

/* avi_riff.c */

void quicktime_read_riff(quicktime_t *file, quicktime_atom_t *parent_atom);
void quicktime_delete_riff(quicktime_t *file, quicktime_riff_t *riff);
int quicktime_import_avi(quicktime_t *file);
void quicktime_init_riff(quicktime_t *file);
void quicktime_finalize_riff(quicktime_t *file, quicktime_riff_t *riff);
quicktime_riff_t* quicktime_new_riff(quicktime_t *file);

/* avi_strl.c */

void quicktime_delete_strl(quicktime_strl_t *strl);
quicktime_strl_t* quicktime_new_strl();
void quicktime_read_strl(quicktime_t *file,
                         quicktime_strl_t *strl,
                         quicktime_atom_t *parent_atom);

void quicktime_init_strl(quicktime_t *file,
                         quicktime_audio_map_t *atrack,
                         quicktime_video_map_t *vtrack,
                         quicktime_trak_t *trak,
                         quicktime_strl_t *strl);

/* clap.c */

void quicktime_clap_init(quicktime_clap_t *clap);
void quicktime_clap_delete(quicktime_clap_t *clap);
void quicktime_clap_dump(quicktime_clap_t *clap);
void quicktime_read_clap(quicktime_t *file, quicktime_clap_t *clap);
void quicktime_write_clap(quicktime_t *file, quicktime_clap_t *clap);

// ******************************** Permutation *******************************

#define PERMUTATION_ARGS \
	unsigned char **output_rows,  \
	unsigned char **input_rows, \
	int in_x,  \
	int in_y,  \
	int in_w,  \
	int in_h, \
	int out_w,  \
	int out_h, \
	int in_colormodel,  \
	int out_colormodel, \
	int in_rowspan, \
	int out_rowspan, \
	int in_rowspan_uv, \
	int out_rowspan_uv, \
	int scale, \
	int out_pixelsize, \
	int in_pixelsize, \
	int *row_table, \
	int *column_table



/* cmodel_default.c */

void cmodel_yuv420p(PERMUTATION_ARGS);
void cmodel_yuv422p(PERMUTATION_ARGS);
void cmodel_yuv411p(PERMUTATION_ARGS);
void cmodel_yuv444p(PERMUTATION_ARGS);
void cmodel_yuv422(PERMUTATION_ARGS);
void cmodel_default(PERMUTATION_ARGS);

/* colr.c */

void quicktime_colr_init(quicktime_colr_t *colr);
void quicktime_colr_delete(quicktime_colr_t *colr);
void quicktime_colr_dump(quicktime_colr_t *colr);
void quicktime_read_colr(quicktime_t *file, quicktime_colr_t *colr);
void quicktime_write_colr(quicktime_t *file, quicktime_colr_t *colr);

/* ctab.c */

int quicktime_ctab_init(quicktime_ctab_t *ctab);
int quicktime_ctab_delete(quicktime_ctab_t *ctab);
void quicktime_ctab_dump(quicktime_ctab_t *ctab);
int quicktime_read_ctab(quicktime_t *file, quicktime_ctab_t *ctab);
void quicktime_default_ctab(quicktime_ctab_t *ctab, int depth);

/* dinf.c */

void quicktime_dinf_init(quicktime_dinf_t *dinf);
void quicktime_dinf_delete(quicktime_dinf_t *dinf);
void quicktime_dinf_init_all(quicktime_dinf_t *dinf);
void quicktime_dinf_dump(quicktime_dinf_t *dinf);
void quicktime_read_dinf(quicktime_t *file, quicktime_dinf_t *dinf, quicktime_atom_t *dinf_atom);
void quicktime_write_dinf(quicktime_t *file, quicktime_dinf_t *dinf);

/* dref.c */

void quicktime_dref_table_init(quicktime_dref_table_t *table);
void quicktime_dref_table_delete(quicktime_dref_table_t *table);
void quicktime_read_dref_table(quicktime_t *file, quicktime_dref_table_t *table);
void quicktime_write_dref_table(quicktime_t *file, quicktime_dref_table_t *table);
void quicktime_dref_table_dump(quicktime_dref_table_t *table);
void quicktime_dref_init(quicktime_dref_t *dref);
void quicktime_dref_init_all(quicktime_dref_t *dref);
void quicktime_dref_delete(quicktime_dref_t *dref);
void quicktime_dref_dump(quicktime_dref_t *dref);
void quicktime_read_dref(quicktime_t *file, quicktime_dref_t *dref);
void quicktime_write_dref(quicktime_t *file, quicktime_dref_t *dref);

/* edts.c */

void quicktime_edts_init(quicktime_edts_t *edts);
void quicktime_edts_delete(quicktime_edts_t *edts);
void quicktime_edts_init_table(quicktime_edts_t *edts);
void quicktime_read_edts(quicktime_t *file, quicktime_edts_t *edts, quicktime_atom_t *edts_atom);
void quicktime_edts_dump(quicktime_edts_t *edts);
void quicktime_write_edts(quicktime_t *file, quicktime_edts_t *edts, long duration);

/* enda.c */

void quicktime_read_enda(quicktime_t *file, quicktime_enda_t *enda,
                         quicktime_atom_t *enda_atom);
void quicktime_write_enda(quicktime_t *file, quicktime_enda_t *enda);
void quicktime_enda_dump(quicktime_enda_t *enda);

void quicktime_set_enda(quicktime_trak_t * trak, int little_endian);

/* Returns TRUE if little endian */
int quicktime_get_enda(quicktime_trak_t * trak);

/* elst.c */

void quicktime_elst_table_init(quicktime_elst_table_t *table);
void quicktime_elst_table_delete(quicktime_elst_table_t *table);
void quicktime_read_elst_table(quicktime_t *file, quicktime_elst_table_t *table);
void quicktime_write_elst_table(quicktime_t *file, quicktime_elst_table_t *table, long duration);
void quicktime_elst_table_dump(quicktime_elst_table_t *table);
void quicktime_elst_init(quicktime_elst_t *elst);
void quicktime_elst_init_all(quicktime_elst_t *elst);
void quicktime_elst_delete(quicktime_elst_t *elst);
void quicktime_elst_dump(quicktime_elst_t *elst);
void quicktime_read_elst(quicktime_t *file, quicktime_elst_t *elst);
void quicktime_write_elst(quicktime_t *file, quicktime_elst_t *elst, long duration);

/* frma.c */

void quicktime_read_frma(quicktime_t *file, quicktime_frma_t *frma,
                         quicktime_atom_t *frma_atom);

void quicktime_write_frma(quicktime_t *file, quicktime_frma_t *frma);
void quicktime_frma_dump(quicktime_frma_t *frma);
void quicktime_set_frma(quicktime_trak_t * trak, char * codec);




/* gmhd.c */

void quicktime_gmhd_init(quicktime_gmhd_t *gmhd);
void quicktime_gmhd_delete(quicktime_gmhd_t *gmhd);
void quicktime_gmhd_dump(quicktime_gmhd_t *gmhd);
void quicktime_read_gmhd(quicktime_t *file, quicktime_gmhd_t *gmhd, quicktime_atom_t *parent_atom);
void quicktime_write_gmhd(quicktime_t *file, quicktime_gmhd_t *gmhd);

/* gmin.c */

void quicktime_gmin_init(quicktime_gmin_t *gmin);
void quicktime_gmin_delete(quicktime_gmin_t *gmin);
void quicktime_gmin_dump(quicktime_gmin_t *gmin);
void quicktime_read_gmin(quicktime_t *file, quicktime_gmin_t *gmin);
void quicktime_write_gmin(quicktime_t *file, quicktime_gmin_t *gmin);

/* hdlr.c */

void quicktime_hdlr_init(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_init_video(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_init_panorama(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_init_qtvr(quicktime_hdlr_t *hdlr, int track_type);
void quicktime_hdlr_init_audio(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_init_data(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_delete(quicktime_hdlr_t *hdlr);
void quicktime_hdlr_dump(quicktime_hdlr_t *hdlr);
void quicktime_read_hdlr(quicktime_t *file, quicktime_hdlr_t *hdlr);
void quicktime_write_hdlr(quicktime_t *file, quicktime_hdlr_t *hdlr);

/* imgp.c */

int quicktime_imgp_init(quicktime_imgp_t *imgp);
int quicktime_imgp_delete(quicktime_imgp_t *imgp);
void quicktime_imgp_dump(quicktime_imgp_t *imgp);
int quicktime_read_imgp(quicktime_t *file, quicktime_imgp_t *imgp, quicktime_qtatom_t *imgp_atom);
void quicktime_write_imgp(quicktime_t *file, quicktime_imgp_t *imgp);

/* impn.c */

int quicktime_impn_init(quicktime_impn_t *impn);
int quicktime_impn_delete(quicktime_impn_t *impn);
void quicktime_impn_dump(quicktime_impn_t *impn);
int quicktime_read_impn(quicktime_t *file, quicktime_impn_t *impn, quicktime_qtatom_t *impn_atom);
void quicktime_write_impn(quicktime_t *file, quicktime_impn_t *impn);

/* matrix.c */

void quicktime_matrix_init(quicktime_matrix_t *matrix);
void quicktime_matrix_delete(quicktime_matrix_t *matrix);
void quicktime_read_matrix(quicktime_t *file, quicktime_matrix_t *matrix);
void quicktime_matrix_dump(quicktime_matrix_t *matrix);
void quicktime_write_matrix(quicktime_t *file, quicktime_matrix_t *matrix);

/* mdat.c */

void quicktime_mdat_delete(quicktime_mdat_t *mdat);
void quicktime_read_mdat(quicktime_t *file, quicktime_mdat_t *mdat, quicktime_atom_t *parent_atom);

/* mdhd.c */

void quicktime_mdhd_init(quicktime_mdhd_t *mdhd);
void quicktime_mdhd_delete(quicktime_mdhd_t *mdhd);

void quicktime_mdhd_dump(quicktime_mdhd_t *mdhd);

void quicktime_read_mdhd(quicktime_t *file, quicktime_mdhd_t *mdhd);
void quicktime_write_mdhd(quicktime_t *file, quicktime_mdhd_t *mdhd);

void quicktime_mdhd_init_audio(quicktime_mdhd_t *mdhd, 
                               int sample_rate);

void quicktime_mdhd_init_video(quicktime_t *file, 
                               quicktime_mdhd_t *mdhd,
                               int timescale);

/* mdia.c */

void quicktime_mdia_init(quicktime_mdia_t *mdia);
void quicktime_mdia_delete(quicktime_mdia_t *mdia);
void quicktime_mdia_dump(quicktime_mdia_t *mdia);
int quicktime_read_mdia(quicktime_t *file, quicktime_mdia_t *mdia, quicktime_atom_t *trak_atom);
void quicktime_write_mdia(quicktime_t *file, quicktime_mdia_t *mdia);

void quicktime_mdia_init_audio(quicktime_t *file, 
                               quicktime_mdia_t *mdia, 
                               int channels,
                               int sample_rate, 
                               int bits, 
                               char *compressor);

void quicktime_mdia_init_video(quicktime_t *file, 
                               quicktime_mdia_t *mdia,
                               int frame_w,
                               int frame_h, 
                               int frame_duration,
                               int timescale,
                               char *compressor);

void quicktime_mdia_init_panorama(quicktime_t *file,
				  quicktime_mdia_t *mdia,
				  int width,
				  int height,
				  int frame_duration,
				  int timescale);

void quicktime_mdia_init_qtvr(quicktime_t *file,
				  quicktime_mdia_t *mdia,
				  int track_type,
				  int width,
				  int height,
				  int frame_duration,
				  int timescale);
/* minf.c */

void quicktime_minf_init(quicktime_minf_t *minf);

void quicktime_minf_dump(quicktime_minf_t *minf);
int quicktime_read_minf(quicktime_t *file, quicktime_minf_t *minf, quicktime_atom_t *parent_atom);
void quicktime_write_minf(quicktime_t *file, quicktime_minf_t *minf);
void quicktime_minf_delete(quicktime_minf_t *minf);

void quicktime_minf_init_video(quicktime_t *file, 
                               quicktime_minf_t *minf, 
                               int frame_w,
                               int frame_h, 
                               int frame_duration,
                               int time_scale, 
                               char *compressor);

void quicktime_minf_init_audio(quicktime_t *file, 
                               quicktime_minf_t *minf, 
                               int channels, 
                               int sample_rate, 
                               int bits, 
                               char *compressor);

void quicktime_minf_init_panorama(quicktime_t *file,
				  quicktime_minf_t *minf,
				  int width,
				  int height,
				  int frame_duration);

void quicktime_minf_init_qtvr(quicktime_t *file,
				  quicktime_minf_t *minf,
				  int frame_type,
				  int width,
				  int height,
				  int frame_duration);

/* moov.c */

int quicktime_moov_init(quicktime_moov_t *moov);
int quicktime_moov_delete(quicktime_moov_t *moov);
void quicktime_moov_dump(quicktime_moov_t *moov);
int quicktime_read_moov(quicktime_t *file, quicktime_moov_t *moov, quicktime_atom_t *parent_atom);
void quicktime_finalize_moov(quicktime_t *file, quicktime_moov_t *moov);
void quicktime_write_moov(quicktime_t *file, quicktime_moov_t *moov);
void quicktime_update_durations(quicktime_moov_t *moov);
int quicktime_shift_offsets(quicktime_moov_t *moov, int64_t offset);

/* mvhd.c */

int quicktime_mvhd_init(quicktime_mvhd_t *mvhd);
int quicktime_mvhd_delete(quicktime_mvhd_t *mvhd);
void quicktime_mvhd_dump(quicktime_mvhd_t *mvhd);
void quicktime_read_mvhd(quicktime_t *file, quicktime_mvhd_t *mvhd, quicktime_atom_t *parent_atom);
void quicktime_mhvd_init_video(quicktime_t *file, quicktime_mvhd_t *mvhd, int time_scale);
void quicktime_write_mvhd(quicktime_t *file, quicktime_mvhd_t *mvhd);

/* navg.c */

int quicktime_navg_init(quicktime_navg_t *navg);
int quicktime_navg_delete(quicktime_navg_t *navg);
void quicktime_navg_dump(quicktime_navg_t *navg);
int quicktime_read_navg(quicktime_t *file, quicktime_navg_t *navg, quicktime_atom_t *navg_atom);
void quicktime_write_navg(quicktime_t *file, quicktime_navg_t *navg);

/* ndhd */

int quicktime_ndhd_init(quicktime_ndhd_t *ndhd);  
int quicktime_ndhd_delete(quicktime_ndhd_t *ndhd);
void quicktime_ndhd_dump(quicktime_ndhd_t *ndhd);
int quicktime_read_ndhd(quicktime_t *file, quicktime_ndhd_t *ndhd);
void quicktime_write_ndhd(quicktime_t *file, quicktime_ndhd_t *ndhd);

/* nloc.c */

int quicktime_nloc_init(quicktime_nloc_t *nloc);
int quicktime_nloc_delete(quicktime_nloc_t *nloc);
void quicktime_nloc_dump(quicktime_nloc_t *nloc);
int quicktime_read_nloc(quicktime_t *file, quicktime_nloc_t *nloc, quicktime_qtatom_t *nloc_atom);
void quicktime_write_nloc(quicktime_t *file, quicktime_nloc_t *nloc);

/* obji.c */

int quicktime_obji_init(quicktime_obji_t *obji);  
int quicktime_obji_delete(quicktime_obji_t *obji);
void quicktime_obji_dump(quicktime_obji_t *obji);
int quicktime_read_obji(quicktime_t *file, quicktime_obji_t *obji);
void quicktime_write_obji(quicktime_t *file, quicktime_obji_t *obji);

/* pano.c */

int quicktime_pano_init(quicktime_pano_t *pano);  
int quicktime_pano_delete(quicktime_pano_t *pano);
void quicktime_pano_dump(quicktime_pano_t *pano);
int quicktime_read_pano(quicktime_t *file, quicktime_pano_t *navg, quicktime_atom_t *pano_atom);
void quicktime_write_pano(quicktime_t *file, quicktime_pano_t *pano);

/* pasp.c */

void quicktime_pasp_init(quicktime_pasp_t *pasp);
void quicktime_pasp_delete(quicktime_pasp_t *pasp);
void quicktime_pasp_dump(quicktime_pasp_t *pasp);
void quicktime_read_pasp(quicktime_t *file, quicktime_pasp_t *pasp);
void quicktime_write_pasp(quicktime_t *file, quicktime_pasp_t *pasp);

/* pHdr */

int quicktime_pHdr_init(quicktime_pHdr_t *pHdr);

/* qtatom.c */

int quicktime_qtatom_read_header(quicktime_t *file, quicktime_qtatom_t *atom);
int quicktime_qtatom_write_header(quicktime_t *file, quicktime_qtatom_t *atom, char *text, long ID);
void quicktime_qtatom_write_footer(quicktime_t *file, quicktime_qtatom_t *atom);
int quicktime_qtatom_is(quicktime_qtatom_t *atom, char *type);
int quicktime_qtatom_skip(quicktime_t *file, quicktime_qtatom_t *atom);
void quicktime_qtatom_read_container_header(quicktime_t *file);
void quicktime_qtatom_write_container_header(quicktime_t *file);

/* qtvr.c */

int quicktime_qtvr_init(quicktime_qtvr_t *qtvr);
int quicktime_qtvr_delete(quicktime_qtvr_t *qtvr);
void quicktime_qtvr_dump(quicktime_qtvr_t *qtvr);
int quicktime_read_qtvr(quicktime_t *file, quicktime_qtvr_t *qtvr, quicktime_atom_t *qtvr_atom);
void quicktime_write_qtvr(quicktime_t *file, quicktime_qtvr_t *qtvr);

/* smhd.c */

void quicktime_smhd_init(quicktime_smhd_t *smhd);
void quicktime_smhd_delete(quicktime_smhd_t *smhd);
void quicktime_smhd_dump(quicktime_smhd_t *smhd);
void quicktime_read_smhd(quicktime_t *file, quicktime_smhd_t *smhd);
void quicktime_write_smhd(quicktime_t *file, quicktime_smhd_t *smhd);

/* stbl.c */

void quicktime_stbl_init(quicktime_stbl_t *stbl);
void quicktime_stbl_delete(quicktime_stbl_t *stbl);
void quicktime_stbl_dump(void *minf_ptr, quicktime_stbl_t *stbl);
int quicktime_read_stbl(quicktime_t *file, quicktime_minf_t *minf,
                        quicktime_stbl_t *stbl, quicktime_atom_t *parent_atom);

void quicktime_write_stbl(quicktime_t *file, quicktime_minf_t *minf,
                          quicktime_stbl_t *stbl);

void quicktime_stbl_init_video(quicktime_t *file, 
                               quicktime_stbl_t *stbl, 
                               int frame_w,
                               int frame_h, 
                               int frame_duration,
                               int time_scale,
                               char *compressor);

void quicktime_stbl_init_audio(quicktime_t *file, 
                               quicktime_stbl_t *stbl, 
                               int channels, 
                               int sample_rate, 
                               int bits, 
                               char *compressor);
			       
void quicktime_stbl_init_panorama(quicktime_t *file,
				  quicktime_stbl_t *stbl,
				  int width,
				  int height,
				  int frame_duration);

void quicktime_stbl_init_qtvr(quicktime_t *file,
				  quicktime_stbl_t *stbl,
				  int track_type,
				  int width,
				  int height,
				  int frame_duration);			     
/* stco.c */

void quicktime_stco_init(quicktime_stco_t *stco);
void quicktime_stco_delete(quicktime_stco_t *stco);
void quicktime_stco_init_common(quicktime_t *file, quicktime_stco_t *stco);
void quicktime_stco_dump(quicktime_stco_t *stco);
void quicktime_read_stco(quicktime_t *file, quicktime_stco_t *stco);
void quicktime_read_stco64(quicktime_t *file, quicktime_stco_t *stco);
void quicktime_write_stco(quicktime_t *file, quicktime_stco_t *stco);
void quicktime_update_stco(quicktime_stco_t *stco, long chunk, int64_t offset);

/* stsc.c */

void quicktime_stsc_init(quicktime_stsc_t *stsc);
void quicktime_stsc_init_table(quicktime_t *file, quicktime_stsc_t *stsc);
void quicktime_stsc_init_video(quicktime_t *file, quicktime_stsc_t *stsc);
void quicktime_stsc_init_audio(quicktime_t *file, quicktime_stsc_t *stsc, int sample_rate);
void quicktime_stsc_delete(quicktime_stsc_t *stsc);
void quicktime_stsc_dump(quicktime_stsc_t *stsc);
void quicktime_read_stsc(quicktime_t *file, quicktime_stsc_t *stsc);
void quicktime_write_stsc(quicktime_t *file, quicktime_stsc_t *stsc);
int quicktime_update_stsc(quicktime_stsc_t *stsc, long chunk, long samples);

/* stsd.c */

void quicktime_stsd_init(quicktime_stsd_t *stsd);
void quicktime_stsd_init_table(quicktime_stsd_t *stsd);
void quicktime_stsd_delete(quicktime_stsd_t *stsd);
void quicktime_stsd_dump(void *minf_ptr, quicktime_stsd_t *stsd);
void quicktime_read_stsd(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_t *stsd);
void quicktime_write_stsd(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_t *stsd);

void quicktime_stsd_init_video(quicktime_t *file, 
                               quicktime_stsd_t *stsd, 
                               int frame_w,
                               int frame_h, 
                               char *compression);

void quicktime_stsd_init_audio(quicktime_t *file, 
                               quicktime_stsd_t *stsd, 
                               int channels,
                               int sample_rate, 
                               int bits, 
                               char *compressor);

void quicktime_stsd_init_panorama(quicktime_t *file,
				  quicktime_stsd_t *stsd,
				  int width,
				  int height);

void quicktime_stsd_init_qtvr(quicktime_t *file,
				  quicktime_stsd_t *stsd,
				  int track_type,
				  int width,
				  int height);
/* stsdtable.c */

void quicktime_mjqt_init(quicktime_mjqt_t *mjqt);
void quicktime_mjqt_delete(quicktime_mjqt_t *mjqt);
void quicktime_mjqt_dump(quicktime_mjqt_t *mjqt);
void quicktime_mjht_init(quicktime_mjht_t *mjht);
void quicktime_mjht_delete(quicktime_mjht_t *mjht);
void quicktime_mjht_dump(quicktime_mjht_t *mjht);

void quicktime_read_stsd_audio(quicktime_t *file, quicktime_stsd_table_t *table,
                               quicktime_atom_t *parent_atom);
void quicktime_write_stsd_audio(quicktime_t *file, quicktime_stsd_table_t *table);
void quicktime_read_stsd_video(quicktime_t *file, quicktime_stsd_table_t *table,
                               quicktime_atom_t *parent_atom);

void quicktime_set_stsd_audio_v1(quicktime_stsd_table_t *table,
                                 uint32_t samples_per_packet,
                                 uint32_t bytes_per_packet,
                                 uint32_t bytes_per_frame,
                                 uint32_t bytes_per_sample);

void quicktime_write_stsd_video(quicktime_t *file, quicktime_stsd_table_t *table);
void quicktime_read_stsd_table(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_table_t *table);
void quicktime_stsd_table_init(quicktime_stsd_table_t *table);
  
void quicktime_stsd_table_delete(quicktime_stsd_table_t *table);
void quicktime_stsd_video_dump(quicktime_stsd_table_t *table);
void quicktime_stsd_audio_dump(quicktime_stsd_table_t *table);
void quicktime_stsd_table_dump(void *minf_ptr, quicktime_stsd_table_t *table);
void quicktime_write_stsd_table(quicktime_t *file, quicktime_minf_t *minf, quicktime_stsd_table_t *table);

/* stss.c */

void quicktime_stss_init(quicktime_stss_t *stss);
void quicktime_stss_delete(quicktime_stss_t *stss);
void quicktime_stss_dump(quicktime_stss_t *stss);
void quicktime_read_stss(quicktime_t *file, quicktime_stss_t *stss);
void quicktime_write_stss(quicktime_t *file, quicktime_stss_t *stss);

/* stsz.c */

void quicktime_stsz_init(quicktime_stsz_t *stsz);
void quicktime_stsz_init_video(quicktime_t *file, quicktime_stsz_t *stsz);
void quicktime_stsz_init_audio(quicktime_t *file, 
                               quicktime_stsz_t *stsz, 
                               int channels, 
                               int bits,
                               char *compressor);
void quicktime_stsz_delete(quicktime_stsz_t *stsz);
void quicktime_stsz_dump(quicktime_stsz_t *stsz);
void quicktime_read_stsz(quicktime_t *file, quicktime_stsz_t *stsz);
void quicktime_write_stsz(quicktime_t *file, quicktime_stsz_t *stsz);
void quicktime_update_stsz(quicktime_stsz_t *stsz, 
                           long sample, 
                           long sample_size);




/* stts.c */

void quicktime_stts_init(quicktime_stts_t *stts);
void quicktime_stts_init_table(quicktime_stts_t *stts);
void quicktime_stts_init_video(quicktime_t *file, quicktime_stts_t *stts, int frame_duration);
void quicktime_stts_init_audio(quicktime_t *file, quicktime_stts_t *stts, int sample_rate);
void quicktime_stts_delete(quicktime_stts_t *stts);
void quicktime_stts_dump(quicktime_stts_t *stts);
void quicktime_read_stts(quicktime_t *file, quicktime_stts_t *stts);
void quicktime_write_stts(quicktime_t *file, quicktime_stts_t *stts);
void quicktime_update_stts(quicktime_stts_t *stts, long sample, long duration);
/* The following function kicks out redundant entries */
void quicktime_compress_stts(quicktime_stts_t *stts);

void quicktime_stts_init_panorama(quicktime_t *file, quicktime_stts_t *stts, int frame_duration);


int64_t quicktime_time_to_sample(quicktime_stts_t *stts, int64_t * time,
                                 int64_t * stts_index, int64_t * stts_count);

int64_t quicktime_sample_to_time(quicktime_stts_t *stts, int64_t sample,
                                 int64_t * stts_index, int64_t * stts_count);


/* tkhd.c */

int quicktime_tkhd_init(quicktime_tkhd_t *tkhd);
int quicktime_tkhd_delete(quicktime_tkhd_t *tkhd);
void quicktime_tkhd_dump(quicktime_tkhd_t *tkhd);
void quicktime_read_tkhd(quicktime_t *file, quicktime_tkhd_t *tkhd);
void quicktime_write_tkhd(quicktime_t *file, quicktime_tkhd_t *tkhd);
void quicktime_tkhd_init_video(quicktime_t *file, quicktime_tkhd_t *tkhd, 
                               int frame_w, int frame_h);

/* trak.c */

int quicktime_trak_init(quicktime_trak_t *trak);
int quicktime_trak_init_video(quicktime_t *file, 
                              quicktime_trak_t *trak, 
                              int frame_w, 
                              int frame_h, 
                              int frame_duration,
                              int time_scale,
                              char *compressor);

int quicktime_trak_init_audio(quicktime_t *file, 
                              quicktime_trak_t *trak, 
                              int channels, 
                              int sample_rate, 
                              int bits, 
                              char *compressor);
			      
int quicktime_trak_init_qtvr(quicktime_t *file,
				 quicktime_trak_t *trak,
				 int track_type,
				 int width,
				 int height,
				 int timescale,
				 int frame_duration);

int quicktime_trak_init_panorama(quicktime_t *file,
				 quicktime_trak_t *trak,
				 int width,
				 int height,
				 int timescale,
				 int frame_duration);

int quicktime_trak_delete(quicktime_trak_t *trak);
int quicktime_trak_dump(quicktime_trak_t *trak);
quicktime_trak_t* quicktime_add_trak(quicktime_t *file);
int quicktime_delete_trak(quicktime_moov_t *moov);
int quicktime_read_trak(quicktime_t *file, quicktime_trak_t *trak, quicktime_atom_t *trak_atom);
int quicktime_write_trak(quicktime_t *file, quicktime_trak_t *trak, long moov_time_scale);
int64_t quicktime_track_end(quicktime_trak_t *trak);
long quicktime_track_samples(quicktime_t *file, quicktime_trak_t *trak);
long quicktime_sample_of_chunk(quicktime_trak_t *trak, long chunk);

/* useratoms.c */

uint8_t * quicktime_user_atoms_get_atom(quicktime_user_atoms_t * u, char * name, uint32_t * len);

void quicktime_user_atoms_add_atom(quicktime_user_atoms_t * u,
                                   char * name, uint8_t * data,
                                   uint32_t len);

void quicktime_user_atoms_read_atom(quicktime_t * file,
                                    quicktime_user_atoms_t * u,
                                    quicktime_atom_t * leaf_atom);

void quicktime_user_atoms_free(quicktime_user_atoms_t * u);

void quicktime_write_user_atoms(quicktime_t * file,
                                quicktime_user_atoms_t * u);
void quicktime_user_atoms_dump(quicktime_user_atoms_t * u);






/* For AVI */
int quicktime_avg_chunk_samples(quicktime_t *file, quicktime_trak_t *trak);
int quicktime_chunk_of_sample(int64_t *chunk_sample, 
                              int64_t *chunk, 
                              quicktime_trak_t *trak, 
                              long sample);

int64_t quicktime_chunk_to_offset(quicktime_t *file, quicktime_trak_t *trak, long chunk);

long quicktime_offset_to_chunk(int64_t *chunk_offset, 
                               quicktime_trak_t *trak, 
                               int64_t offset);

int64_t quicktime_sample_range_size(quicktime_trak_t *trak, 
                                    long chunk_sample, 
                                    long sample);
int64_t quicktime_sample_to_offset(quicktime_t *file, quicktime_trak_t *trak, long sample);

long quicktime_offset_to_sample(quicktime_trak_t *trak, int64_t offset);
void quicktime_write_chunk_header(quicktime_t *file, 
                                  quicktime_trak_t *trak, 
                                  quicktime_atom_t *chunk);
void quicktime_write_chunk_footer(quicktime_t *file, 
                                  quicktime_trak_t *trak,
                                  int current_chunk,
                                  quicktime_atom_t *chunk, 
                                  int samples);

int quicktime_trak_duration(quicktime_trak_t *trak, 
                            long *duration, 
                            long *timescale);

int quicktime_trak_fix_counts(quicktime_t *file, quicktime_trak_t *trak);
long quicktime_chunk_samples(quicktime_trak_t *trak, long chunk);
int quicktime_trak_shift_offsets(quicktime_trak_t *trak, int64_t offset);


/* tref.c */

int quicktime_tref_init(quicktime_tref_t *tref);
int quicktime_tref_delete(quicktime_tref_t *tref);
void quicktime_tref_dump(quicktime_tref_t *tref);
int quicktime_read_tref(quicktime_t *file, quicktime_tref_t *tref);
void quicktime_write_tref(quicktime_t *file, quicktime_tref_t *tref);
int quicktime_tref_init_qtvr(quicktime_tref_t *tref, int track_type);

/* udta.c */

int quicktime_udta_init(quicktime_udta_t *udta);
int quicktime_udta_delete(quicktime_udta_t *udta);
void quicktime_udta_dump(quicktime_udta_t *udta);
int quicktime_read_udta_string(quicktime_t *file, char **string, int *size);
int quicktime_write_udta_string(quicktime_t *file, char *string, int size);
int quicktime_read_udta(quicktime_t *file, quicktime_udta_t *udta, quicktime_atom_t *udta_atom);
void quicktime_write_udta(quicktime_t *file, quicktime_udta_t *udta);
int quicktime_set_udta_string(char **string, int *size, char *new_string);

/* vmhd.c */

void quicktime_vmhd_init(quicktime_vmhd_t *vmhd);
void quicktime_vmhd_init_video(quicktime_t *file, 
                               quicktime_vmhd_t *vmhd, 
                               int frame_w,
                               int frame_h, 
                               int frame_duration,
                               int timescale);

void quicktime_vmhd_delete(quicktime_vmhd_t *vmhd);
void quicktime_vmhd_dump(quicktime_vmhd_t *vmhd);
void quicktime_read_vmhd(quicktime_t *file, quicktime_vmhd_t *vmhd);
void quicktime_write_vmhd(quicktime_t *file, quicktime_vmhd_t *vmhd);

/* vrni.c */

int quicktime_vrni_init(quicktime_vrni_t *vrni);
int quicktime_vrni_delete(quicktime_vrni_t *vrni);
void quicktime_vrni_dump(quicktime_vrni_t *vrni);
int quicktime_read_vrni(quicktime_t *file, quicktime_vrni_t *vrni, quicktime_qtatom_t *vrni_atom);
void quicktime_write_vrni(quicktime_t *file, quicktime_vrni_t *vrni);

/* vrnp.c */

int quicktime_vrnp_init(quicktime_vrnp_t *vrnp);
int quicktime_vrnp_delete(quicktime_vrnp_t *vrnp);
void quicktime_vrnp_dump(quicktime_vrnp_t *vrnp);
int quicktime_read_vrnp(quicktime_t *file, quicktime_vrnp_t *vrnp, quicktime_qtatom_t *vrnp_atom);
void quicktime_write_vrnp(quicktime_t *file, quicktime_vrnp_t *vrnp);

/* vrsc.c */

int quicktime_vrsc_init(quicktime_vrsc_t *vrsc);
int quicktime_vrsc_delete(quicktime_vrsc_t *vrsc);
void quicktime_vrsc_dump(quicktime_vrsc_t *vrsc);
int quicktime_read_vrsc(quicktime_t *file, quicktime_vrsc_t *vrsc, quicktime_qtatom_t *vrsc_atom);
void quicktime_write_vrsc(quicktime_t *file, quicktime_vrsc_t *vrsc);

/* wave.c */

void quicktime_read_wave(quicktime_t *file, quicktime_wave_t *wave,
                         quicktime_atom_t *wave_atom);

void quicktime_write_wave(quicktime_t *file, quicktime_wave_t *wave);
void quicktime_wave_dump(quicktime_wave_t *wave);
void quicktime_wave_delete(quicktime_wave_t *wave);

uint8_t * quicktime_wave_get_user_atom(quicktime_trak_t * trak, char * name, uint32_t * len);
void quicktime_wave_set_user_atom(quicktime_trak_t * trak, char * name, uint8_t * data,  uint32_t len);



/* util.c */

/* Disk I/O */
int64_t quicktime_get_file_length(const char *path);
int quicktime_file_open(quicktime_t *file, const char *path, int rd, int wr);
int quicktime_file_close(quicktime_t *file);
int64_t quicktime_ftell(quicktime_t *file);
int quicktime_fseek(quicktime_t *file, int64_t offset);
int quicktime_read_data(quicktime_t *file, uint8_t *data, int64_t size);
int quicktime_write_data(quicktime_t *file, uint8_t *data, int size);
int64_t quicktime_byte_position(quicktime_t *file);
void quicktime_read_pascal(quicktime_t *file, char *data);
void quicktime_write_pascal(quicktime_t *file, char *data);
float quicktime_read_fixed32(quicktime_t *file);
int quicktime_write_fixed32(quicktime_t *file, float number);
float quicktime_read_float(quicktime_t *file);
int quicktime_write_float(quicktime_t *file, float value);
int quicktime_write_int64(quicktime_t *file, int64_t value);
int quicktime_write_int64_le(quicktime_t *file, int64_t value);
int quicktime_write_int32(quicktime_t *file, long value);
int quicktime_write_int32_le(quicktime_t *file, long value);
int quicktime_write_char32(quicktime_t *file, char *string);
float quicktime_read_fixed16(quicktime_t *file);
int quicktime_write_fixed16(quicktime_t *file, float number);
unsigned long quicktime_read_uint32(quicktime_t *file);
long quicktime_read_int32(quicktime_t *file);
long quicktime_read_int32_le(quicktime_t *file);
int64_t quicktime_read_int64(quicktime_t *file);
int64_t quicktime_read_int64_le(quicktime_t *file);
long quicktime_read_int24(quicktime_t *file);
int quicktime_write_int24(quicktime_t *file, long number);
int quicktime_read_int16(quicktime_t *file);
int quicktime_read_int16_le(quicktime_t *file);
int quicktime_write_int16(quicktime_t *file, int number);
int quicktime_write_int16_le(quicktime_t *file, int number);
int quicktime_read_char(quicktime_t *file);
int quicktime_write_char(quicktime_t *file, char x);
void quicktime_read_char32(quicktime_t *file, char *string);
int64_t quicktime_position(quicktime_t *file);
int quicktime_set_position(quicktime_t *file, int64_t position);
void quicktime_copy_char32(char *output, char *input);
void quicktime_print_chars(char *desc, uint8_t *input, int len);
unsigned long quicktime_current_time(void);
int quicktime_match_32(void *input, void *output);
int quicktime_match_24(char *input, char *output);

/* lqt_quicktime.c */

int quicktime_init(quicktime_t *file);
quicktime_trak_t* quicktime_add_track(quicktime_t *);
int quicktime_get_timescale(double frame_rate);

int quicktime_init_audio_map(quicktime_audio_map_t *atrack, quicktime_trak_t *trak,
                             int encode, lqt_codec_info_t * info);

int quicktime_delete_audio_map(quicktime_audio_map_t *atrack);


int quicktime_init_video_map(quicktime_video_map_t *vtrack, quicktime_trak_t *trak,
                             int encode, lqt_codec_info_t * info);

int quicktime_delete_video_map(quicktime_video_map_t *vtrack);

int quicktime_update_positions(quicktime_t *file);

void quicktime_init_maps(quicktime_t * file);
void lqt_update_frame_position(quicktime_video_map_t * track);

void lqt_start_audio_vbr_chunk(quicktime_t * file, int track);
void lqt_init_vbr_audio(quicktime_t * file, int track);

/* Before and after writing subsequent frames, we must call
   quicktime_write_chunk_[header|footer] */

void lqt_start_audio_vbr_frame(quicktime_t * file, int track);
void lqt_finish_audio_vbr_frame(quicktime_t * file, int track, int num_samples);

/*
 *  Convenience function: Returns an array of chunk sizes
 *  for a track. This is expecially useful for compressing audio
 *  codecs since the chunk size cannot quickly be determined from 
 *  the tables.
 *  Call this function in the constructor of the codec and free()
 *  the returned array, when it's no longer needed
 */

int64_t * lqt_get_chunk_sizes(quicktime_t * file, quicktime_trak_t *trak);

/* lqt_codecs.c */

int quicktime_delete_vcodec(quicktime_video_map_t *vtrack);
int quicktime_delete_acodec(quicktime_audio_map_t *vtrack);
int quicktime_codecs_flush(quicktime_t *file);
void quicktime_id_to_codec(char *result, int id);
int quicktime_codec_to_id(char *codec);

/* workarounds.c */

int64_t quicktime_add3(int64_t a, int64_t b, int64_t c);
