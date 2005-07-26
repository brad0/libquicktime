#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


int quicktime_navg_init(quicktime_navg_t *navg)
{
	navg->version = 1;
	navg->columns = 1;
	navg->rows = 1;
	navg->reserved = 0;
	navg->loop_frames = 1;
	navg->loop_dur = 75;
	navg->movietype = 1;
	navg->loop_timescale = 0;
	navg->fieldofview = 180;
	navg->startHPan = 0;
	navg->endHPan = 360;
	navg->endVPan = -90;
	navg->startVPan = 90;
	navg->initialHPan = 180;
	navg->initialVPan = 30;
	navg->reserved2 = 0;
	return 0;
}

int quicktime_navg_delete(quicktime_navg_t *navg)
{
    return 0;
}

void quicktime_navg_dump(quicktime_navg_t *navg)
{
	printf("  object parameters (navg)\n");
	printf("    version %i\n", navg->version);
	printf("    columns %i\n", navg->columns);
	printf("    rows %i\n", navg->rows);
	printf("    loop frames %i\n", navg->loop_frames);
	printf("    loop frame duration %i\n", navg->loop_dur);
	printf("    movie type %i\n", navg->movietype);
	printf("    loop timescale %i\n", navg->loop_timescale);
	printf("    field of view %f\n", navg->fieldofview);
	printf("    horizontal start pan %f\n", navg->startHPan);
	printf("    horizontal end pan %f\n", navg->endHPan);
	printf("    vertical end pan %f\n", navg->endVPan);
	printf("    vertical start pan %f\n", navg->startVPan);
	printf("    initial horizontal pan %f\n", navg->initialHPan);
	printf("    initial vertical pan %f\n", navg->initialVPan);
}

int quicktime_read_navg(quicktime_t *file, quicktime_navg_t *navg, quicktime_atom_t *navg_atom)
{
	navg->version =  quicktime_read_int16(file);
	navg->columns = quicktime_read_int16(file);
	navg->rows = quicktime_read_int16(file);
	navg->reserved = quicktime_read_int16(file);
	navg->loop_frames = quicktime_read_int16(file);
	navg->loop_dur = quicktime_read_int16(file);
	navg->movietype = quicktime_read_int16(file);
	navg->loop_timescale = quicktime_read_int16(file);
	navg->fieldofview = quicktime_read_fixed32(file);
	navg->startHPan = quicktime_read_fixed32(file);
	navg->endHPan = quicktime_read_fixed32(file);
	navg->endVPan = quicktime_read_fixed32(file);
	navg->startVPan = quicktime_read_fixed32(file);
	navg->initialHPan = quicktime_read_fixed32(file);
	navg->initialVPan = quicktime_read_fixed32(file);
	navg->reserved2 = quicktime_read_int32(file);
	return 0;
}

void quicktime_write_navg(quicktime_t *file, quicktime_navg_t *navg)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "NAVG");
	quicktime_write_int16(file, navg->version);
	quicktime_write_int16(file, navg->columns);
	quicktime_write_int16(file, navg->rows);
	quicktime_write_int16(file, navg->reserved);
	quicktime_write_int16(file, navg->loop_frames);
	quicktime_write_int16(file, navg->loop_dur);
	quicktime_write_int16(file, navg->movietype);
	quicktime_write_int16(file, navg->loop_timescale);
	quicktime_write_fixed32(file, navg->fieldofview);
	quicktime_write_fixed32(file, navg->startHPan);
	quicktime_write_fixed32(file, navg->endHPan);
	quicktime_write_fixed32(file, navg->endVPan);
	quicktime_write_fixed32(file, navg->startVPan);
	quicktime_write_fixed32(file, navg->initialHPan);
	quicktime_write_fixed32(file, navg->initialVPan);
	quicktime_write_fixed32(file, navg->reserved2);	

	quicktime_atom_write_footer(file, &atom);
}

