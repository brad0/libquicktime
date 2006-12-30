#include <funcprotos.h>
#include <quicktime/quicktime.h>


int quicktime_nmhd_init(quicktime_nmhd_t *nmhd)
  {
  return 0;
  }

int quicktime_nmhd_delete(quicktime_nmhd_t *nmhd)
  {
  return 0;
  }

void quicktime_nmhd_dump(quicktime_nmhd_t *nmhd)
  {
  lqt_dump("   null media header (nmhd)\n");
  lqt_dump("    version %d\n", nmhd->version);
  lqt_dump("    flags %ld\n", nmhd->flags);
  }

int quicktime_read_nmhd(quicktime_t *file, quicktime_nmhd_t *nmhd)
  {
  nmhd->version = quicktime_read_char(file);
  nmhd->flags = quicktime_read_int24(file);
  return 0;
  }

void quicktime_write_nmhd(quicktime_t *file, quicktime_nmhd_t *nmhd)
  {
  quicktime_atom_t atom;
  quicktime_atom_write_header(file, &atom, "nmhd");
  quicktime_write_char(file, nmhd->version);
  quicktime_write_int24(file, nmhd->flags);
  quicktime_atom_write_footer(file, &atom);
  }
