#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


int quicktime_tref_init(quicktime_tref_t *tref)
{
	tref->trackIndex = 1;
	return 0;
}

int quicktime_tref_init_qtvr(quicktime_tref_t *tref, int track_type)
{
    	if (track_type == QTVR_OBJ) {
	    tref->refType[0] = 'i';
	    tref->refType[1] = 'm';
	    tref->refType[2] = 'g';
	    tref->refType[3] = 't';
	} else 
	if (track_type == QTVR_QTVR)
	{
	    tref->refType[0] = 'o';
	    tref->refType[1] = 'b';
	    tref->refType[2] = 'j';
	    tref->refType[3] = 'e';
	}
	return 0;
}

int quicktime_tref_delete(quicktime_tref_t *tref)
{
	return 0;
}

void quicktime_tref_dump(quicktime_tref_t *tref)
{
	lqt_dump("  track reference (tref)\n");
	lqt_dump("   reference type %c%c%c%c\n", tref->refType[0], tref->refType[1], tref->refType[2], tref->refType[3]);
	lqt_dump("   track index: %ld\n", tref->trackIndex);
}

int quicktime_read_tref(quicktime_t *file, quicktime_tref_t *tref)
{
	quicktime_atom_t atom;
	quicktime_atom_read_header(file, &(atom));
	tref->refType[0] = atom.type[0];
	tref->refType[1] = atom.type[1];
	tref->refType[2] = atom.type[2];
	tref->refType[3] = atom.type[3];
	tref->trackIndex = quicktime_read_int32(file);
	return 0;
}

void quicktime_write_tref(quicktime_t *file, quicktime_tref_t *tref )
{
	quicktime_atom_t atom, subatom;
	quicktime_atom_write_header(file, &atom, "tref");
	quicktime_atom_write_header(file, &subatom, tref->refType);
	quicktime_write_int32(file, tref->trackIndex);
	quicktime_atom_write_footer(file, &subatom);
	quicktime_atom_write_footer(file, &atom);
}

