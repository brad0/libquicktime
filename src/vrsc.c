#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


int quicktime_vrsc_init(quicktime_vrsc_t *vrsc)
{
	vrsc->version = 2;
	vrsc->revision = 0;
	vrsc->DefaultNodeID = 1;
	return 0;
}

int quicktime_vrsc_delete(quicktime_vrsc_t *vrsc)
{
    return 0;
}

void quicktime_vrsc_dump(quicktime_vrsc_t *vrsc)
{
	printf("        world header (vrsc)\n");
	printf("         version %i\n",  vrsc->version);
	printf("         revision %i\n",  vrsc->revision);
	printf("         name atom id %ld\n",  vrsc->NameAtomID);
	printf("         default node %ld\n", vrsc->DefaultNodeID);
	printf("         world flags %ld\n", vrsc->flags);
}

int quicktime_read_vrsc(quicktime_t *file, quicktime_vrsc_t *vrsc, quicktime_qtatom_t *vrsc_atom)
{
	vrsc->version =  quicktime_read_int16(file);
	vrsc->revision = quicktime_read_int16(file);
	vrsc->NameAtomID = quicktime_read_int32(file);
	vrsc->DefaultNodeID = quicktime_read_int32(file);
	vrsc->flags = quicktime_read_int32(file);
	vrsc->reserved1 = quicktime_read_int32(file);
	vrsc->reserved2 = quicktime_read_int32(file);
	return 0;
}

void quicktime_write_vrsc(quicktime_t *file, quicktime_vrsc_t *vrsc)
{
	quicktime_qtatom_t atom;
	quicktime_qtatom_write_header(file, &atom, "vrsc", 1);
	quicktime_write_int16(file, vrsc->version);
	quicktime_write_int16(file, vrsc->revision);
	quicktime_write_int32(file, vrsc->NameAtomID);
	quicktime_write_int32(file, vrsc->DefaultNodeID);
	quicktime_write_int32(file, vrsc->flags);
	quicktime_write_int32(file, vrsc->reserved1);
	quicktime_write_int32(file, vrsc->reserved2);
	quicktime_qtatom_write_footer(file, &atom);
}

