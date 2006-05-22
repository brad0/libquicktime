#include <funcprotos.h>
#include <quicktime/quicktime.h>

void quicktime_delete_movi(quicktime_t *file, quicktime_movi_t *movi)
{
}

void quicktime_init_movi(quicktime_t *file, quicktime_riff_t *riff)
{
	quicktime_movi_t *movi = &riff->movi;

	quicktime_atom_write_header(file, &movi->atom, "LIST");
	quicktime_write_char32(file, "movi");

}

void quicktime_read_movi(quicktime_t *file, 
	quicktime_atom_t *parent_atom,
	quicktime_movi_t *movi)
{
	movi->atom.size = parent_atom->size;
// Relative to start of the movi string
	movi->atom.start = parent_atom->start + 8;
	quicktime_atom_skip(file, parent_atom);
}

void quicktime_finalize_movi(quicktime_t *file, quicktime_movi_t *movi)
  {
  quicktime_atom_write_footer(file, &movi->atom);
  }






