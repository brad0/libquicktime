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
	printf("  Object Parameter (navg)\n");
	if(navg->version) 	 printf("    Version:             %i\n", navg->version);
	if(navg->columns)        printf("    Columns:             %i\n", navg->columns);
	if(navg->rows)           printf("    rows:                %i\n", navg->rows);
	/* reserved */
	if(navg->loop_frames)    printf("    Loop Frames:         %i\n", navg->loop_frames);
	if(navg->loop_dur)       printf("    Loop Frame duration: %i\n", navg->loop_dur);
	if(navg->movietype)      printf("    Movie Type:          %i\n", navg->movietype);
	if(navg->loop_timescale) printf("    Loop Timescale:      %i\n", navg->loop_timescale);
	if(navg->fieldofview)    printf("    Field of View:       %f\n", navg->fieldofview);
	if(navg->startHPan)      printf("    startHPan:           %f\n", navg->startHPan);
	if(navg->endHPan)        printf("    endHPan:             %f\n", navg->endHPan);
	if(navg->endVPan)        printf("    endVPan:             %f\n", navg->endVPan);
	if(navg->startVPan)      printf("    startVPan:           %f\n", navg->startVPan);
	if(navg->initialHPan)    printf("    initialHPan:         %f\n", navg->initialHPan);
	if(navg->initialVPan)    printf("    initialVPan:         %f\n", navg->initialVPan);
	/* reserved */
}

int quicktime_read_navg(quicktime_t *file, quicktime_navg_t *navg, quicktime_atom_t *navg_atom)
{
	quicktime_atom_t leaf_atom;
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
	quicktime_atom_t atom, subatom;
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

