#include <funcprotos.h>
#include <quicktime/quicktime.h>
#include <string.h>


int quicktime_impn_init(quicktime_impn_t *impn)
{
	impn->version = 2;
	impn->revision = 0;
	impn->imagingMode = 2;
	impn->imagingValidFlags = 7;
	impn->correction = 2;
	impn->quality = 256;
	impn->directdraw = 1;
	return 0;
}

int quicktime_impn_delete(quicktime_impn_t *impn)
{
    return 0;
}

void quicktime_impn_dump(quicktime_impn_t *impn)
{
	printf("        Panorama Imaging Atom (impn)\n");
	printf("         Version %i\n",  impn->version);
	printf("         Revision %i\n",  impn->revision);
	printf("         imagingMode %ld\n",  impn->imagingMode);
	printf("         imagingValidFlags %ld\n", impn->imagingValidFlags);
	printf("         Correction %ld\n", impn->correction);
	printf("         Quality %ld\n", impn->quality);
	printf("         directdraw %ld\n", impn->directdraw);
	printf("         Imaging Properties %ld %ld %ld %ld %ld %ld \n", impn->imagingProperties[0], impn->imagingProperties[1], impn->imagingProperties[2], impn->imagingProperties[3], impn->imagingProperties[4], impn->imagingProperties[5]);

}

int quicktime_read_impn(quicktime_t *file, quicktime_impn_t *impn, quicktime_qtatom_t *impn_atom)
{
	impn->version =  quicktime_read_int16(file);
	impn->revision = quicktime_read_int16(file);
	impn->imagingMode = quicktime_read_int32(file);
	impn->imagingValidFlags = quicktime_read_int32(file);
	impn->correction = quicktime_read_int32(file);
	impn->quality = quicktime_read_int32(file);
	impn->directdraw = quicktime_read_int32(file);
	impn->imagingProperties[0] = quicktime_read_int32(file);
	impn->imagingProperties[1] = quicktime_read_int32(file);
	impn->imagingProperties[2] = quicktime_read_int32(file);
	impn->imagingProperties[3] = quicktime_read_int32(file);
	impn->imagingProperties[4] = quicktime_read_int32(file);
	impn->imagingProperties[5] = quicktime_read_int32(file);
	impn->reserved1 = quicktime_read_int32(file);
	impn->reserved2 = quicktime_read_int32(file);
	return 0;
}

void quicktime_write_impn(quicktime_t *file, quicktime_impn_t *impn)
{
	quicktime_qtatom_t atom;
	quicktime_qtatom_write_header(file, &atom, "impn", 1);
	quicktime_write_int16(file, impn->version);
	quicktime_write_int16(file, impn->revision);
	quicktime_write_int32(file, impn->imagingMode);
	quicktime_write_int32(file, impn->imagingValidFlags);
	quicktime_write_int32(file, impn->correction);
	quicktime_write_int32(file, impn->quality);
	quicktime_write_int32(file, impn->directdraw);
	quicktime_write_int32(file, impn->imagingProperties[0]);
	quicktime_write_int32(file, impn->imagingProperties[1]);
	quicktime_write_int32(file, impn->imagingProperties[2]);
	quicktime_write_int32(file, impn->imagingProperties[3]);
	quicktime_write_int32(file, impn->imagingProperties[4]);
	quicktime_write_int32(file, impn->imagingProperties[5]);
	quicktime_write_int32(file, impn->reserved1);
	quicktime_write_int32(file, impn->reserved2);
	quicktime_qtatom_write_footer(file, &atom);
}

