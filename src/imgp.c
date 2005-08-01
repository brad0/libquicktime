#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


int quicktime_imgp_init(quicktime_imgp_t *imgp)
{
    	quicktime_impn_init(&(imgp->impn));
	return 0;
}

int quicktime_imgp_delete(quicktime_imgp_t *imgp)
{
	return 0;
}

void quicktime_imgp_dump(quicktime_imgp_t *imgp)
{
	printf("        Imaging Parent (imgp)\n");
	quicktime_impn_dump(&(imgp->impn));

}

int quicktime_read_imgp(quicktime_t *file, quicktime_imgp_t *imgp, quicktime_qtatom_t *imgp_atom)
{
	quicktime_qtatom_t leaf_atom;
	int result = 0;
	
	do
	{
	    	quicktime_qtatom_read_header(file, &leaf_atom);
		if(quicktime_qtatom_is(&leaf_atom, "impn"))
		{
		    	result += quicktime_read_impn(file, &(imgp->impn), &leaf_atom);
		} else
			quicktime_qtatom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < imgp_atom->end);
	
	return result;
}

void quicktime_write_imgp(quicktime_t *file, quicktime_imgp_t *imgp )
{
	quicktime_qtatom_t atom;
	quicktime_qtatom_write_header(file, &atom, "imgp", 1);
	quicktime_write_impn(file, &(imgp->impn));

	atom.child_count = 1;
	
	quicktime_qtatom_write_footer(file, &atom);
}

