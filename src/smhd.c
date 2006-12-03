#include <funcprotos.h>
#include <quicktime/quicktime.h>




void quicktime_smhd_init(quicktime_smhd_t *smhd)
{
	smhd->version = 0;
	smhd->flags = 0;
	smhd->balance = 0;
	smhd->reserved = 0;
}

void quicktime_smhd_delete(quicktime_smhd_t *smhd)
{
}

void quicktime_smhd_dump(quicktime_smhd_t *smhd)
{
	lqt_dump("    sound media header\n");
	lqt_dump("     version %d\n", smhd->version);
	lqt_dump("     flags %ld\n", smhd->flags);
	lqt_dump("     balance %d\n", smhd->balance);
	lqt_dump("     reserved %d\n", smhd->reserved);
}

void quicktime_read_smhd(quicktime_t *file, quicktime_smhd_t *smhd)
{
	smhd->version = quicktime_read_char(file);
	smhd->flags = quicktime_read_int24(file);
	smhd->balance = quicktime_read_int16(file);
	smhd->reserved = quicktime_read_int16(file);
}

void quicktime_write_smhd(quicktime_t *file, quicktime_smhd_t *smhd)
{
	quicktime_atom_t atom;
	quicktime_atom_write_header(file, &atom, "smhd");

	quicktime_write_char(file, smhd->version);
	quicktime_write_int24(file, smhd->flags);
	quicktime_write_int16(file, smhd->balance);
	quicktime_write_int16(file, smhd->reserved);

	quicktime_atom_write_footer(file, &atom);
}
