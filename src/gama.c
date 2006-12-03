#include <string.h>
#include <funcprotos.h>
#include <quicktime/quicktime.h>

void quicktime_gama_init(quicktime_gama_t *gama)
{
	memset(gama, 0, sizeof (*gama));
}

void quicktime_gama_delete(quicktime_gama_t *gama) { }

void quicktime_gama_dump(quicktime_gama_t *gama)
{

        lqt_dump("     Gamma value (gama): %f\n", gama->gamma);
}

void quicktime_read_gama(quicktime_t *file, quicktime_gama_t *gama)
{
	gama->gamma = quicktime_read_fixed32(file);
}

void quicktime_write_gama(quicktime_t *file, quicktime_gama_t *gama)
{
	quicktime_atom_t atom;

	quicktime_atom_write_header(file, &atom, "gama");
	quicktime_write_fixed32(file, gama->gamma);
	quicktime_atom_write_footer(file, &atom);
}
