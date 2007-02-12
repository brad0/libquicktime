#include "lqt_private.h"

int quicktime_vrnp_init(quicktime_vrnp_t *vrnp)
{
	vrnp->children = 0;
	quicktime_vrni_init(&(vrnp->vrni[0]));
	return 0;
}

int quicktime_vrnp_delete(quicktime_vrnp_t *vrnp)
{
	return 0;
}

void quicktime_vrnp_dump(quicktime_vrnp_t *vrnp)
{
    	int i;
	lqt_dump("        node parent (vrnp)\n");
	lqt_dump("         nodes %i\n", vrnp->children);
	for (i = 0; i < vrnp->children; i++)
	{
	    quicktime_vrni_dump(&(vrnp->vrni[i]));
	}
}

int quicktime_read_vrnp(quicktime_t *file, quicktime_vrnp_t *vrnp, quicktime_qtatom_t *vrnp_atom)
{
	quicktime_qtatom_t leaf_atom;
	int result = 0;
	int i;
	
	quicktime_qtatom_read_header(file, &leaf_atom);
	for (i = 0; i < vrnp->children; i++ )
	{
	   	vrnp->vrni[i].ID = leaf_atom.ID;
		result += quicktime_read_vrni(file, &(vrnp->vrni[i]), &leaf_atom);
	}
	
	return result;
}

void quicktime_write_vrnp(quicktime_t *file, quicktime_vrnp_t *vrnp )
{
	quicktime_qtatom_t atom;
	quicktime_qtatom_write_header(file, &atom, "vrnp", 1);
	atom.child_count = 1;
    	quicktime_write_vrni(file, &(vrnp->vrni[0]));
	quicktime_qtatom_write_footer(file, &atom);

}

