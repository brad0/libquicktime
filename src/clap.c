/*
 * $Id: clap.c,v 1.2 2004/11/29 00:41:22 gmerlin Exp $
 *
 * init, read, write handler for the "clap" (Clean Aperture) atom
*/

#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

void quicktime_clap_init(quicktime_clap_t *clap)
{
	memset(clap, 0, sizeof (*clap));
}

void quicktime_clap_delete(quicktime_clap_t *clap) { }

void quicktime_clap_dump(quicktime_clap_t *clap)
{

	printf("     clean aperture (clap)\n");
	printf("       cleanApertureWidthN %d\n", clap->cleanApertureWidthN);
	printf("       cleanApertureWidthD %d\n", clap->cleanApertureWidthD);
	printf("       cleanApertureHeightN %d\n", clap->cleanApertureHeightN);
	printf("       cleanApertureHeightD %d\n", clap->cleanApertureHeightD);
	printf("       horizOffN %d\n", clap->horizOffN);
	printf("       horizOffD %d\n", clap->horizOffD);
	printf("       vertOffN %d\n", clap->vertOffN);
	printf("       vertOffD %d\n", clap->vertOffD);
}

void quicktime_read_clap(quicktime_t *file, quicktime_clap_t *clap)
{
	clap->cleanApertureWidthN = quicktime_read_int32(file);
	clap->cleanApertureWidthD = quicktime_read_int32(file);
	clap->cleanApertureHeightN = quicktime_read_int32(file);
	clap->cleanApertureHeightD = quicktime_read_int32(file);
	clap->horizOffN = quicktime_read_int32(file);
	clap->horizOffD = quicktime_read_int32(file);
	clap->vertOffN = quicktime_read_int32(file);
	clap->vertOffD = quicktime_read_int32(file);
}

void quicktime_write_clap(quicktime_t *file, quicktime_clap_t *clap)
{
	quicktime_atom_t atom;

	quicktime_atom_write_header(file, &atom, "clap");
	quicktime_write_int32(file, clap->cleanApertureWidthN);
	quicktime_write_int32(file, clap->cleanApertureWidthD);
	quicktime_write_int32(file, clap->cleanApertureHeightN);
	quicktime_write_int32(file, clap->cleanApertureHeightD);
	quicktime_write_int32(file, clap->horizOffN);
	quicktime_write_int32(file, clap->horizOffD);
	quicktime_write_int32(file, clap->vertOffN);
	quicktime_write_int32(file, clap->vertOffD);
	quicktime_atom_write_footer(file, &atom);
}

int lqt_set_clap(quicktime_t *file, int track, quicktime_clap_t *clap)
{
	quicktime_clap_t *trk_clap;

	if	((track < 0) || (track >= file->total_vtracks))
		return 0;

	trk_clap = &file->vtracks[track].track->mdia.minf.stbl.stsd.table->clap;
	*trk_clap = *clap;
	return 1;
}

int lqt_get_clap(quicktime_t *file, int track, quicktime_clap_t *clap)
{
	quicktime_clap_t *trk_clap;

	if	((track < 0) || (track >= file->total_vtracks))
		return 0;

	trk_clap = &file->vtracks[track].track->mdia.minf.stbl.stsd.table->clap;
	*clap = *trk_clap;
	return 1;
}