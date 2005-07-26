#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


int quicktime_vrni_init(quicktime_vrni_t *vrnp)
{
	vrnp->ID = 1;
	quicktime_nloc_init(&(vrnp->nloc));
	return 0;
}

int quicktime_vrni_delete(quicktime_vrni_t *vrni)
{
	return 0;
}

void quicktime_vrni_dump(quicktime_vrni_t *vrni)
{
    	int i;
	printf("         node id (vrni)\n");
	printf("          id %i\n", vrni->ID);
	quicktime_nloc_dump(&(vrni->nloc));
}

int quicktime_read_vrni(quicktime_t *file, quicktime_vrni_t *vrni, quicktime_qtatom_t *vrni_atom)
{
	quicktime_qtatom_t leaf_atom;
	int result = 0;
	
	quicktime_qtatom_read_header(file, &leaf_atom);
	
	quicktime_read_nloc(file, &(vrni->nloc), &leaf_atom);
	
	return result;
}

void quicktime_write_vrni(quicktime_t *file, quicktime_vrni_t *vrni )
{
	quicktime_qtatom_t atom;
	quicktime_qtatom_write_header(file, &atom, "vrni", 1);
	quicktime_write_nloc(file, &(vrni->nloc));
	atom.child_count = 1;
	quicktime_qtatom_write_footer(file, &atom);

}

