/*
 * $Id: pasp.c,v 1.5 2006/04/13 19:25:48 gmerlin Exp $
 *
 * init, read, write handler for the "pasp" (Pixel Aspect) atom
*/

#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>

void quicktime_pasp_init(quicktime_pasp_t *pasp)
{
        /* We use this also, if no pasp atom is explicitely present,
           since some codecs also set the pixel aspect ratio */
        pasp->hSpacing = 1;
        pasp->vSpacing = 1;
}

void quicktime_pasp_delete(quicktime_pasp_t *pasp) { }

void quicktime_pasp_dump(quicktime_pasp_t *pasp)
{

	printf("     pixel aspect (pasp)\n");
	printf("       hSpacing %d\n", pasp->hSpacing);
	printf("       vSpacing %d\n", pasp->vSpacing);
}

void quicktime_read_pasp(quicktime_t *file, quicktime_pasp_t *pasp)
{
	pasp->hSpacing = quicktime_read_int32(file);
	pasp->vSpacing = quicktime_read_int32(file);
}

void quicktime_write_pasp(quicktime_t *file, quicktime_pasp_t *pasp)
{
	quicktime_atom_t atom;

	quicktime_atom_write_header(file, &atom, "pasp");
	quicktime_write_int32(file, pasp->hSpacing);
	quicktime_write_int32(file, pasp->vSpacing);
	quicktime_atom_write_footer(file, &atom);
}

int lqt_set_pasp(quicktime_t *file, int track, quicktime_pasp_t *pasp)
{
	quicktime_pasp_t *trk_pasp;

	if	((track < 0) || (track >= file->total_vtracks))
		return 0;

	trk_pasp = &file->vtracks[track].track->mdia.minf.stbl.stsd.table->pasp;
	trk_pasp->hSpacing = pasp->hSpacing;
	trk_pasp->vSpacing = pasp->vSpacing;
        file->vtracks[track].track->mdia.minf.stbl.stsd.table->has_pasp = 1;
	return 1;
}

int lqt_get_pasp(quicktime_t *file, int track, quicktime_pasp_t *pasp)
{
	quicktime_pasp_t *trk_pasp;

	if	((track < 0) || (track >= file->total_vtracks))
		return 0;
        if	(!file->vtracks[track].track->mdia.minf.stbl.stsd.table->has_pasp)
		return 0;
	trk_pasp =
          &file->vtracks[track].track->mdia.minf.stbl.stsd.table->pasp;
	pasp->hSpacing = trk_pasp->hSpacing;
	pasp->vSpacing = trk_pasp->vSpacing;
	return 1;
}

int lqt_get_pixel_aspect(quicktime_t *file, int track, int * pixel_width,
                         int * pixel_height)
{
	quicktime_pasp_t *pasp;

	if	((track < 0) || (track >= file->total_vtracks))
		return 0;

	pasp =
          &file->vtracks[track].track->mdia.minf.stbl.stsd.table->pasp;
        *pixel_width  = pasp->hSpacing;
        *pixel_height = pasp->vSpacing;
        return 1;
}

int lqt_set_pixel_aspect(quicktime_t *file, int track, int pixel_width,
                         int pixel_height)
{
	quicktime_pasp_t *pasp;

	if	((track < 0) || (track >= file->total_vtracks))
		return 0;

	pasp =
          &file->vtracks[track].track->mdia.minf.stbl.stsd.table->pasp;
        if(pixel_width == pixel_height) /* Don't write pasp atom */
          {
          pasp->hSpacing = 1;
          pasp->vSpacing = 1;
          }
        else
          {
          pasp->hSpacing = pixel_width;
          pasp->vSpacing = pixel_height;
          file->vtracks[track].track->mdia.minf.stbl.stsd.table->has_pasp = 1;
          }
        
        return 1;
}
