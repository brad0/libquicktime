#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


int quicktime_qtvr_init(quicktime_qtvr_t *qtvr)
{
	memset(qtvr, 0, sizeof(*qtvr));
	quicktime_imgp_init(&(qtvr->imgp));
	quicktime_vrsc_init(&(qtvr->vrsc));
	quicktime_vrnp_init(&(qtvr->vrnp));
	return 0;
}

int quicktime_qtvr_delete(quicktime_qtvr_t *qtvr)
{
	return 0;
}

void quicktime_qtvr_dump(quicktime_qtvr_t *qtvr)
{
	printf("       qtvr world\n");
	quicktime_vrsc_dump(&(qtvr->vrsc));
	quicktime_imgp_dump(&(qtvr->imgp));
	quicktime_vrnp_dump(&(qtvr->vrnp));
}

int quicktime_read_qtvr(quicktime_t *file, quicktime_qtvr_t *qtvr, quicktime_atom_t *qtvr_atom)
{
	quicktime_qtatom_t leaf_atom, root_atom;
	int result = 0;
	
	quicktime_qtatom_read_container_header(file);
	quicktime_qtatom_read_header(file, &root_atom);
	do
	{
	    	quicktime_qtatom_read_header(file, &leaf_atom);
		if(quicktime_qtatom_is(&leaf_atom, "vrsc"))
		{
		    	result += quicktime_read_vrsc(file, &(qtvr->vrsc), &leaf_atom);
		} else
		if(quicktime_qtatom_is(&leaf_atom, "imgp"))
		{
		    	result += quicktime_read_imgp(file, &(qtvr->imgp), &leaf_atom);
		} else
		if(quicktime_qtatom_is(&leaf_atom, "vrnp"))
		{
		    	qtvr->vrnp.children = leaf_atom.child_count;
		    	result += quicktime_read_vrnp(file, &(qtvr->vrnp), &leaf_atom);
		} else
			quicktime_qtatom_skip(file, &leaf_atom);
	}while(quicktime_position(file) < root_atom.end);
	
	return result;
}

void quicktime_write_qtvr(quicktime_t *file, quicktime_qtvr_t *qtvr )
{
	quicktime_qtatom_t subatom;

	quicktime_qtatom_write_container_header(file);
	quicktime_qtatom_write_header(file, &subatom, "sean", 1);
    	if(file->moov.udta.is_qtvr) // boo
	{
	    	subatom.child_count = 3;
		quicktime_write_vrsc(file, &(qtvr->vrsc));
		quicktime_write_imgp(file, &(qtvr->imgp));
		quicktime_write_vrnp(file, &(qtvr->vrnp));
	}	
	quicktime_qtatom_write_footer(file, &subatom);

}
