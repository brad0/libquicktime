/* This file was automatically generated.  Do not edit! */
void quicktime_atom_write_footer(quicktime_t *file,quicktime_atom_t *atom);
void quicktime_write_dref(quicktime_t *file,quicktime_dref_t *dref);
int quicktime_atom_write_header(quicktime_t *file,quicktime_atom_t *atom,char *text);
void quicktime_write_dinf(quicktime_t *file,quicktime_dinf_t *dinf);
longest quicktime_position(quicktime_t *file);
int quicktime_atom_skip(quicktime_t *file,quicktime_atom_t *atom);
void quicktime_read_dref(quicktime_t *file,quicktime_dref_t *dref);
int quicktime_atom_is(quicktime_atom_t *atom,char *type);
int quicktime_atom_read_header(quicktime_t *file,quicktime_atom_t *atom);
void quicktime_read_dinf(quicktime_t *file,quicktime_dinf_t *dinf,quicktime_atom_t *dinf_atom);
void quicktime_dref_dump(quicktime_dref_t *dref);
void quicktime_dinf_dump(quicktime_dinf_t *dinf);
void quicktime_dref_init_all(quicktime_dref_t *dref);
void quicktime_dinf_init_all(quicktime_dinf_t *dinf);
void quicktime_dref_delete(quicktime_dref_t *dref);
void quicktime_dinf_delete(quicktime_dinf_t *dinf);
void quicktime_dref_init(quicktime_dref_t *dref);
void quicktime_dinf_init(quicktime_dinf_t *dinf);
