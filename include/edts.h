/* This file was automatically generated.  Do not edit! */
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
void quicktime_write_elst(quicktime_t *file,quicktime_elst_t *elst,long duration);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_edts(quicktime_t *file,quicktime_edts_t *edts,long duration);
void quicktime_elst_dump(quicktime_elst_t *elst);
void quicktime_edts_dump(quicktime_edts_t *edts);
longest quicktime_position(quicktime_t *file);
int quicktime_atom_skip(quicktime_t *file,quicktime_atom_t *atom);
void quicktime_read_elst(quicktime_t *file,quicktime_elst_t *elst);
int quicktime_atom_is(quicktime_atom_t *atom,char *type);
int quicktime_atom_read_header(quicktime_t *file,quicktime_atom_t *atom);
void quicktime_read_edts(quicktime_t *file,quicktime_edts_t *edts,quicktime_atom_t *edts_atom);
void quicktime_elst_init_all(quicktime_elst_t *elst);
void quicktime_edts_init_table(quicktime_edts_t *edts);
void quicktime_elst_delete(quicktime_elst_t *elst);
void quicktime_edts_delete(quicktime_edts_t *edts);
void quicktime_elst_init(quicktime_elst_t *elst);
void quicktime_edts_init(quicktime_edts_t *edts);
